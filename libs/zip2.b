# 
# @module zip
# 
# The `zip` module contains classes and functions to make working with zip archives easy.
# 
# > **NOTE**
# > This module isn't suitable for archiving large (over 5gb) files as it keeps 
# > streams in memory.
#
# @copyright 2022, Ore Richard Muyiwa and Blade contributors
#

import struct { pack, unpack }
import zlib
import os
import io
import date

# max size of single file in the archive
# = 2 ** 31 + 1
var _ZIP_FILE_MAX = 2147483649

# max number of files in a zip
# = 2 ** 16 - 1
var _ZIP_FILE_COUNT_LIMIT = 65535

# max size of zip archive
var _ZIP_MAX = _ZIP_FILE_MAX

# length of End of central directory records
var _ZIP_META_SIZE = 22

# Error messages...
var _64_required = 'Zip64 compression required'


/**
 * ZipItem represents a single file or directory in a zip archive.
 */
class ZipItem {
  var name
  var directory

  var compression_method
  var crc
  var last_modified
  var compressed_size
  var uncompressed_size
  var is_encrypted

  var error

  var data

  /**
   * from_dict(dict: dictionary)
   * 
   * Creates a new ZipItem from a dictionary.
   * The dictionary should contain the following keys:
   * - `name`: string
   * - `dir`: string &mdash; optional
   * - `compress_method`: number
   * - `crc`: number
   * - `filemtime`: number
   * - `size_compressed`: number
   * - `size_uncompressed`: number
   * - `encrypted`: boolean
   * - `error`: string &mdash; optional
   * - `data`: bytes
   * 
   * @return ZipItem
   */
  static from_dict(dict) {
    if !is_dict(dict)
      die Exception('dictionary expected from argument 1 (dict)')

    var f = ZipItem()

    f.name = dict.name
    f.directory = dict.get('dir', '')
    f.compression_method = dict.compress_method
    f.crc = dict.crc
    f.last_modified = date.from_time(dict.filemtime)
    f.compressed_size = dict.size_compressed
    f.uncompressed_size = dict.size_uncompressed
    f.is_encrypted = dict.get('encrypted', false)
    f.error = dict.get('error', '')
    f.data = dict.data

    return f
  }

  /**
   * export([base_dir: string = os.cwd()])
   * 
   * Exports the ZipItem to file. If base_dir is given, the file will be 
   * exported into the base_dir and all ZipItem directories will be created 
   * inside of base_dir to reflect the ZipItem's original structure.
   * 
   * This function returns `true` if the operation succeeds or `false` otherwise.
   * @return bool
   */
  export(base_dir) {
    if base_dir != nil and !is_string(base_dir)
      die Exception('string expected in argument 1 (base_dir)')

    if !base_dir base_dir = ''

    var final_dir = os.join_paths(base_dir, self.directory)

    if !os.dir_exists(final_dir)
      os.create_dir(final_dir)

    var path = final_dir ? os.join_paths(final_dir, self.name) : self.name

    return file(path, 'wb').write(self.data)
  }
}


/**
 * ZipFile represents an instance of zip file.
 */
class ZipFile {
  var name
  var last_modified
  var time_created
  var size
  var handle
  var files = []

  /**
   * export([base_dir: string])
   * 
   * Exports the all files in the ZipFile to files on the machine. If base_dir is given, 
   * the files will be exported into the base_dir and all directories will be 
   * created inside of base_dir as is to reflect the ZipFile's original structure.
   * 
   * This function returns `true` if the operation succeeds or `false` otherwise.
   * @return bool
   */
  export(base_dir) {
    if base_dir != nil and !is_string(base_dir)
      die Exception('string expected in argument 1 (base_dir)')

    if !base_dir base_dir = ''
    for zip_file in self.files {
      if !zip_file.export(base_dir)
        return false
    }

    return true
  }
}


/**
 * ZipArchive provides a class for zip archive creation, 
 * manuipulation and extraction.
 */
class ZipArchive {

  # The working zip file
  var _file

  # The working file handle
  var _handle

  # central directory
	var _ctrl_dir = bytes(0) 

  var _ctrl_dir_length = 0
	var _old_offset = 0

  /**
   * ZipArchive(file: string)
   * @constructor
   */
  ZipArchive(file) {
    if !is_string(file)
      die Exception('string expected in argument 1 (file)')
    self._file = file
  }

  _get_new_offset() {
    if self._handle {
      self._handle.seek(0, io.SEEK_END)
      return self._handle.tell()
    }
    return 0
  }

  _write_open() {
    if self._handle == nil {
      self._handle = file(self._file, 'wb')
      if !self._handle.is_open()
        die Exception('could not create new zip file')
    }
  }

  _is_within_bound(data) {
    var min_size = self._ctrl_dir.length() + 
          (self._handle ? self._handle.stats().size : 0) + 
            _ZIP_META_SIZE

    var data_size = data ? data.length() : 0

    if min_size + data_size < _ZIP_MAX and
          data_size < _ZIP_FILE_MAX and
            self._ctrl_dir_length < _ZIP_FILE_COUNT_LIMIT {
      return true
    }

    return false
  }

  /**
   * create_dir(name: string)
   * 
   * Adds a directory to the zip with the given name
   * @return bool
   */
  create_dir(name) {

    # Ensure file is within Zip format limit bound.
    if !self._is_within_bound()
      return false

    self._write_open()

		name = name.replace('/\\\\/', '/')

		var fr = bytes([0x50, 0x4b, 0x03, 0x04])
		fr.extend(bytes([0x0a, 0x00]))	# version needed to extract
		fr.extend(bytes([0x00, 0x00]))	# general purpose bit flag
		fr.extend(bytes([0x00, 0x00]))	# compression method
		fr.extend(bytes([0x00, 0x00])) # last mod time
		fr.extend(bytes([0x00, 0x00])) # last mod date

		fr.extend(pack('V', 0)) # crc32
		fr.extend(pack('V', 0)) #compressed filesize
		fr.extend(pack('V', 0)) #uncompressed filesize
		fr.extend(pack('v', name.length())) #length of pathname
		fr.extend(pack('v', 0)) #extra field length
		fr.extend(name.to_bytes())
		# end of 'local file header' segment

		# no 'file data' segment for path

		# 'data descriptor' segment (optional but necessary if archive is not served as file)
		fr.extend(pack('V', 0)) # crc32
		fr.extend(pack('V', 0)) # compressed filesize
		fr.extend(pack('V', 0)) # uncompressed filesize

		# add this entry to array
    self._handle.puts(fr)
    fr.dispose()

    var new_offset = self._get_new_offset()

		# ext. file attributes mirrors MS-DOS directory attr byte, detailed
		# at http:#support.microsoft.com/support/kb/articles/Q125/0/19.asp

		# now add to central record
		self._ctrl_dir.extend(bytes([0x50, 0x4b, 0x01, 0x02]))
		self._ctrl_dir.extend(bytes([0x00, 0x00]))	# version made by
		self._ctrl_dir.extend(bytes([0x0a, 0x00]))	# version needed to extract
		self._ctrl_dir.extend(bytes([0x00, 0x00]))	# general purpose bit flag
		self._ctrl_dir.extend(bytes([0x00, 0x00]))	# compression method
		self._ctrl_dir.extend(bytes([0x00, 0x00])) # last mod time
		self._ctrl_dir.extend(bytes([0x00, 0x00])) # last mod date
		self._ctrl_dir.extend(pack('V', 0)) # crc32
		self._ctrl_dir.extend(pack('V', 0)) # compressed filesize
		self._ctrl_dir.extend(pack('V', 0)) # uncompressed filesize
		self._ctrl_dir.extend(pack('v', name.length())) # length of filename
		self._ctrl_dir.extend(pack('v', 0)) # extra field length
		self._ctrl_dir.extend(pack('v', 0)) # file comment length
		self._ctrl_dir.extend(pack('v', 0)) # disk number start
		self._ctrl_dir.extend(pack('v', 0)) # internal file attributes
		self._ctrl_dir.extend(pack('V', 16)) # external file attributes  - 'directory' bit set

		self._ctrl_dir.extend(pack('V', self._old_offset)) # relative offset of local header
		self._old_offset = new_offset

		self._ctrl_dir.extend(name.to_bytes())

		# optional extra field, file comment goes here
    self._ctrl_dir_length++

    return true
	}

  /**
   * create_file(path: string, data: bytes | string)
   * 
   * Adds a file to the path specified with the contents given data
   * @return bool
   */
  create_file(path, data) {

    # Ensure file is within Zip format limit bound.
    if !self._is_within_bound(data)
      return false

    self._write_open()

		path = path.replace('/\\\\/', '/')

		var fr = bytes([0x50, 0x4b, 0x03, 0x04])
		fr.extend(bytes([0x14, 0x00]))	# version needed to extract
		fr.extend(bytes([0x00, 0x00]))	# general purpose bit flag
		fr.extend(bytes([0x08, 0x00]))	# compression method
		fr.extend(bytes([0x00, 0x00])) # last mod time
		fr.extend(bytes([0x00, 0x00])) # last mod date

		var unc_len = data.length()
		var crc = zlib.crc32(data)

		var zdata = zlib.compress(data)
		var uzdata = zdata[2, -4] # fix crc bug
    zdata.dispose()

		var c_len = uzdata.length()

		fr.extend(pack('V', crc)) # crc32
		fr.extend(pack('V', c_len)) # compressed filesize
		fr.extend(pack('V', unc_len)) # uncompressed filesize
		fr.extend(pack('v', path.length())) # length of filename
		fr.extend(pack('v', 0)) # extra field length
		fr.extend(path.to_bytes())
		# end of 'local file header' segment

		# 'file data' segment
		fr.extend(uzdata)
    uzdata.dispose()

		# 'data descriptor' segment (optional but necessary if archive is not served as file)
		fr.extend(pack('V', crc)) # crc32
		fr.extend(pack('V', c_len)) # compressed filesize
		fr.extend(pack('V', unc_len)) # uncompressed filesize

		# add this entry to array
    self._handle.puts(fr)
    fr.dispose()

		var new_offset = self._get_new_offset()

		# now add to central directory record
		self._ctrl_dir.extend(bytes([0x50, 0x4b, 0x01, 0x02]))
		self._ctrl_dir.extend(bytes([0x00, 0x00]))	# version made by
		self._ctrl_dir.extend(bytes([0x14, 0x00]))	# version needed to extract
		self._ctrl_dir.extend(bytes([0x00, 0x00]))	# general purpose bit flag
		self._ctrl_dir.extend(bytes([0x08, 0x00]))	# compression method
		self._ctrl_dir.extend(bytes([0x00, 0x00])) # last mod time
		self._ctrl_dir.extend(bytes([0x00, 0x00])) # last mod date
		self._ctrl_dir.extend(pack('V', crc)) # crc32
		self._ctrl_dir.extend(pack('V', c_len)) # compressed filesize
		self._ctrl_dir.extend(pack('V', unc_len)) # uncompressed filesize
		self._ctrl_dir.extend(pack('v', path.length())) # length of filename
		self._ctrl_dir.extend(pack('v', 0)) # extra field length
		self._ctrl_dir.extend(pack('v', 0)) # file comment length
		self._ctrl_dir.extend(pack('v', 0)) # disk number start
		self._ctrl_dir.extend(pack('v', 0)) # internal file attributes
		self._ctrl_dir.extend(pack('V', 32)) # external file attributes - 'archive' bit set

		self._ctrl_dir.extend(pack('V', self._old_offset)) # relative offset of local header
		self._old_offset = new_offset

		self._ctrl_dir.extend(path.to_bytes())

		# optional extra field, file comment goes here
    self._ctrl_dir_length++

    return true
	}

  /**
   * add_file(path: string [, destination: string])
   * 
   * Adds an existing file to the archive. If destination is given, the 
   * file will be written to the destination path in the archive.
   */
  add_file(path, destination) {
    if !is_string(path)
      die Exception('expected string as argument 1 (path)')
    if destination != nil and !is_string(destination)
      die Exception('expected string as argument 2 (destination)')

    var f = file(path, 'rb')
    if !f.exists()
      die Exception('file ${path} not found')

    if !destination destination = f.name()
    else destination = destination.replace('/\\\\/', '/')

    var content = f.read()
    var r = self.create_file(destination, content)
    content.dispose()

    return r
  }

  _add_files(path, dir, file_blacklist, ext_blacklist) {
		var gpath = os.join_paths(path, dir)
    
		if os.dir_exists(gpath) {
			var sources = os.read_dir(gpath)
      for source in sources {
        
        # check ext blacklist here...
        for ext in ext_blacklist {
          if source.ends_with('.${ext}')
          return true
        }

        if source != '.' and source != '..' {
          var npath = os.join_paths(gpath, source)

          var cur_dir = os.cwd()
          if npath.starts_with(cur_dir) {
            npath = npath[cur_dir.length(),]

            # Just in case...
            if npath.starts_with('/')
              npath = npath[1,]
          }

          # check file blacklist here...
          if file_blacklist.contains(npath)
            return true

          if os.is_dir(npath) {
            if !self._add_files(gpath, source, file_blacklist, ext_blacklist)
              return false
          } else {
            var destination = npath.replace('~\\.+\\/~', '')
            
            var content = file(npath, 'rb').read()
            var r = self.create_file(destination, content)
            content.dispose()

            if !r return false
          }
        }
      }
		}

		return true
	}

  /**
   * add_directory(directory: string [, file_blacklist: list = [] [, ext_blacklist: list = []]])
   * 
   * Adds the specified `directory` recursively to the archive and set's it path in the archive to `dir`.
   * 
   * - If `file_blacklist` is not empty, this function will ignore every file with a matching path.
   * - If `ext_blacklist` is not empty, this function will ignore every file with a matching 
   */
  add_directory(directory, file_blacklist, ext_blacklist) {
    if !is_string(directory)
      die Exception('expected string in argument 1 (directory)')
    if file_blacklist != nil and !is_list(file_blacklist)
      die Exception('expected list in argument 2 (file_blacklist)')
    if ext_blacklist != nil and !is_list(ext_blacklist)
      die Exception('expected list in argument 3 (ext_blacklist)')

    directory = directory.replace('/\\\\/', '/')

    if !os.dir_exists(directory)
      die Exception('directory ${directory} not found')

    if !file_blacklist file_blacklist = []
    if !ext_blacklist ext_blacklist = []

		return self._add_files(directory, '', file_blacklist, ext_blacklist)
	}

  /**
   * read(path: string)
   * 
   * Reads the zip file in the specified path and returns a list of
   * ZipFile describing it's contents.
   * @return ZipFile
   */
  read() {

    # Get stats.
    var fh = file(self._file, 'rb')
    var zip_stats = fh.stats()

    if zip_stats.size > _ZIP_FILE_MAX
      die Exception(_64_required)

    var zip_file = ZipFile()

		# File information
		zip_file.name = self._file
		zip_file.last_modified = date.from_time(zip_stats.mtime)
		zip_file.time_created = date.from_time(zip_stats.ctime)
		zip_file.size = zip_stats.size
    zip_file.handle = fh

		# Read file
		var filedata = fh.read()

		# Break into sections
	  var filesecta = filedata.split(bytes([0x50, 0x4b, 0x05, 0x06]))

		# ZIP Comment
		var unpackeda = unpack('x16/v1length', filesecta[1])
		self.comment = filesecta[1][18, 18 + unpackeda['length']]
    # self.comment = self.comment.replace('~\\r(\\n)?~', '\n')  # CR + LF and CR -> LF

		# Cut entries from the central directory
		filesecta = filedata.split(bytes([0x50, 0x4b, 0x01, 0x02]))
		filesecta = filesecta[0].split(bytes([0x50, 0x4b, 0x03, 0x04]))
		filesecta = filesecta[1,] # Removes empty entry/signature

		for filedata in filesecta {
			# CRC:crc, FD:file date, FT: file time, CM: compression method, GPF: general purpose flag, VN: version needed, CS: compressed size, UCS: uncompressed size, FNL: filename length
			var entrya = {}
			entrya['error'] = nil

			unpackeda = unpack('v1version/v1general_purpose/v1compress_method/v1file_time/v1file_date/V1crc/V1size_compressed/V1size_uncompressed/v1filename_length/v1extra_field_length', filedata)

			# Check for encryption
			var isencrypted = (unpackeda['general_purpose'] & 0x0001) > 0 ? true : false

			# Check for value block after compressed data
			if unpackeda['general_purpose'] & 0x0008 > 0 {
				var unpackeda2 = unpack('V1crc/V1size_compressed/V1size_uncompressed', filedata[filedata.length() - 12,])

				unpackeda['crc'] = unpackeda2['crc']
				unpackeda['size_compressed'] = unpackeda2['size_uncompressed']
				unpackeda['size_uncompressed'] = unpackeda2['size_uncompressed']
			}

			entrya['name'] = filedata[26, 26 + unpackeda['filename_length']].to_string()

      # skip directories
			if entrya['name'][-1] == '/'
				continue

			entrya['dir'] = os.dir_name(entrya['name']) or nil
			entrya['dir'] = entrya['dir'] == '.' ? '' : entrya['dir']
			entrya['name'] = os.base_name(entrya['name'])

			filedata = filedata[26 + unpackeda['filename_length'] + unpackeda['extra_field_length'],]

			if isencrypted {
				entrya['error'] = 'Encryption is not supported.'
			} else {

        var method = unpackeda['compress_method']
        var decoded
				using method {
					when 0 {
            # Stored
						# Not compressed, continue
          }
					when 8 { # Deflated
						decoded = zlib.undeflate(filedata)
            filedata.dispose()
          }
					when 12 { # BZIP2
            entrya['error'] = 'bzip2 encoded data is not yet supported.'
					}
					default {
						entrya['error'] = 'Compression method (${method}) not supported.'
          }
				}

        # # The error below is usually irrelevant as the uncompressed output size 
        # # indicated in the zip archive is not always dependable.
        # 
        # if entrya['error'] and filedata.length() + compression_header_length != unpackeda['size_compressed'] {
        #   entrya['error'] = 'Compressed size is not equal to the value given in header.'
        # }

				if !entrya['error'] {
					if filedata == false {
						entrya['error'] = 'Decompression failed.'
					} else if filedata.length() != unpackeda['size_uncompressed'] {
						entrya['error'] = 'File size is not equal to the value given in header.'
					} else if zlib.crc32(filedata) != unpackeda['crc'] {
						entrya['error'] = 'CRC32 checksum is not equal to the value given in header.'
					}
				}

				entrya['filemtime'] = date.mktime(
          ((unpackeda['file_date'] & 0xfe00) >>  9) + 1980, # year
          (unpackeda['file_date']  & 0x01e0) >>  5,         # month
          (unpackeda['file_date']  & 0x001f),               # day
          (unpackeda['file_time']  & 0xf800) >> 11,         # hour
          (unpackeda['file_time']  & 0x07e0) >>  5,         # minute
          (unpackeda['file_time']  & 0x001f) <<  1,         # second
          true
        )
				entrya['data'] = decoded
			}

      entrya.extend(unpackeda)
      entrya.add('encrypted', isencrypted)
      
      zip_file.files.append(ZipItem.from_dict(entrya))
		}

    return zip_file
	}

  /**
   * save(filename: string)
   * 
   * Saves the current Zip archive to file.
   */
  save() {
    if self._handle and self._handle.is_open() {
      
      self._handle.puts(self._ctrl_dir)

      # end of Central directory record
      var ending = bytes([0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00])

      ending.extend(pack('v', self._ctrl_dir_length))  # total number of entries 'on this disk'
      ending.extend(pack('v', self._ctrl_dir_length))  # total number of entries overall
      ending.extend(pack('V', self._ctrl_dir.length())) # size of central dir
      ending.extend(pack('V', self._handle.tell())) # offset to start of central dir
      ending.extend(bytes([0x00, 0x00])) # .zip file comment length

      self._handle.puts(ending)
      self._handle.close()

      ending.dispose()
      self._ctrl_dir.dispose()

      return true
    }
    
    die Exception('zip file not open')
	}
}


/**
 * extract(file: string [, destination: string = os.cwd()])
 * 
 * Extracts the zip archive at the _file_ path to the given _destination_ directory. 
 * If _destination_ is not given, the file will be extracted into the current working 
 * directory.
 * 
 * This function returns `true` if the extraction was successful and `false` otherwise.
 * @return bool
 */
def extract(file, destination) {
  if !is_string(file)
    die Exception('string expected in argument 1 (file)')
  if destination != nil and !is_string(destination)
    die Exception('string expected in argument 2 (destination)')

  var zip = ZipArchive(file)
  var zip_file = zip.read()
  return zip_file.export(destination)
}

/**
 * compress(path: string [, destination: string])
 * 
 * Compresses the given path (file or directory) into the destination zip archive.
 * @throws Exception if file could not be written of zip max size exceeded.
 * 
 * > When an exception is thrown becase max size exceeded, some files could 
 * > have already been compressed. In this case, the zip archive will should still 
 * > be usable but not all desired files will be contained in it.
 * 
 * @return bool
 */
def compress(path, destination) {
  if !is_string(path)
    die Exception('string expected in argument 1 (path)')
  if destination != nil and !is_string(destination)
    die Exception('string expected in argument 2 (destination)')

  if !destination destination = os.join_paths(os.cwd(), os.base_name(path)) + '.zip'

  var zip = ZipArchive(destination)

  var completed = false
  
  # If path points to a directory, archive a directory else archive as a file.
  if os.dir_exists(path) {

    # Enter into the path so that we can treat the path as root.
    os.change_dir(path)

    completed = zip.add_directory(path)
  } else {
    completed = zip.add_file(path)
  }

  if !completed or !zip.save()
    die Exception(_64_required)

  return true
}

# var a = ZipArchive('/Users/mcfriendsy/Desktop/123.zip')
# a.add_file('date.b')
# a.save()

# var start = time()
# echo compress('/Users/mcfriendsy/Movies/NARUTO/naruto', '/Users/mcfriendsy/Desktop/Naruto.zip')
# echo 'Time taken = ${time() - start}s'

var start = time()
echo compress('/Users/mcfriendsy/Desktop/A', '/Users/mcfriendsy/Desktop/A.zip')
echo 'Time taken = ${time() - start}s'
