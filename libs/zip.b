/**
 * @module zip
 * 
 * The `zip` module contains classes and functions to make working with zip archives easy.
 *
 * @copyright 2022, Richard Ore and Blade contributors
 */

import struct { pack, unpack }
import zlib
import os
import io
import date
import stat as stat_module

/**
 * The maximum size of a single file in a zip archive when zip64 is not used
 * @type number
 */
var ZIP_FILE_MAX = 2147483649   # = 2 ** 31 + 1

/**
 * The maximum number of files in a zip archive when zip64 is not used
 * @type number
 */
var ZIP_FILE_COUNT_LIMIT = 65535  # = 2 ** 16 - 1

/**
 * The maximum size of a zip archive when zip64 is not used
 * @type number
 */
var ZIP_MAX = ZIP_FILE_MAX

/**
 * The default zip file extension
 * @type string
 */
var ZIP_EXT = '.zip'

# length of End of central directory records
var _ZIP_META_SIZE = 22

# compression methods...
/**
 * Compression method that indicates no compression
 * @type number
 */
var ZIP_STORED = 0 # stored/no compression version

/**
 * Compression method that indicates zlib Deflate compression
 * @type number
 */
var ZIP_DEFLATE = 8 # deflate compression version

# Versions...
var _Z_DEFAULT_VERSION = 20
var _Z_64_VERSION = 45

## Constants.
var _path_replace_regex = '/\\\\/'

var _is_unix = os.platform != 'windows'

# Signatures.
var _file_head_sig = 'PK\x03\x04'
var _central_head_sign = 'PK\x01\x02'
var _central_end_sign = 'PK\x05\x06'
var _disk_record_sign = '\x00\x00\x00\x00'

var _central_end_sign64 = 'PK\x06\x06'
var _disk_record_sign64 = '\x00\x00\x00\x00\x00\x00\x00\x00'
var _locator_end_sign64 = 'PK\x06\x07'

# Unpack formats.
var _file_size_unpack = 'V1crc/V1size_compressed/V1size_uncompressed'
var _file_head_unpack = 'v1version/v1general_purpose/v1compress_method/v1file_time/' +
    'v1file_date/${_file_size_unpack}/v1filename_length/v1extra_field_length/' +
    'v1comment_length/v1internal_attribute/V1external_attribute'

var _file_size_unpack64 = 'P1crc/P1size_compressed/P1size_uncompressed'

# Error messages...
var _64_required = 'Zip64 compression required'


/**
 * ZipItem represents a single file or directory in a zip archive.
 */
class ZipItem {

  /**
   * Name of the file or directory
   * @type string
   */
  var name

  /**
   * The directory in which the file or subdirectory belongs
   * @type string
   */
  var directory

  /**
   * The compression method for this file
   * @type string
   */
  var compression_method

  /**
   * The crc32 checksum for the file
   * @type string
   */
  var crc

  /**
   * The last modified date for the file
   * @type Date
   */
  var last_modified

  /**
   * The size of the file as compressed in the archive. You should note 
   * that this value is not often dependable
   * @type number
   */
  var compressed_size

  /**
   * The size of the file when extracted from the archive
   * @type number
   */
  var uncompressed_size

  /**
   * If this file is encrypted or not.
   * @type bool
   */
  var is_encrypted

  /**
   * The file permission
   * @type number
   */
  var permission

  /**
   * Error encountered when attempting to read/extract the file
   * @type string
   */
  var error

  /**
   * The decompressed value of the zip item
   * @type bytes
   */
  var data

  /**
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
   * - `permission`: number
   * 
   * @param dictionary dict
   * @returns ZipItem
   */
  static from_dict(dict) {
    if !is_dict(dict)
      raise Exception('dictionary expected from argument 1 (dict)')

    var f = ZipItem()

    f.name = dict.name
    f.directory = dict.get('dir', '')
    f.compression_method = dict.compress_method
    f.crc = dict.crc
    f.last_modified = date.from_time(dict.filemtime)
    f.compressed_size = dict.size_compressed
    f.uncompressed_size = dict.size_uncompressed
    f.permission = dict.permission
    f.is_encrypted = dict.get('encrypted', false)
    f.error = dict.get('error', '')
    f.data = dict.data

    return f
  }

  /**
   * Exports the ZipItem to file. If base_dir is given, the file will be 
   * exported into the base_dir and all ZipItem directories will be created 
   * inside of base_dir to reflect the ZipItem's original structure.
   * 
   * This function returns `true` if the operation succeeds or `false` otherwise.
   * 
   * @param string? base_dir: Default value is `os.cwd()`.
   * @returns bool
   */
  export(base_dir) {
    if base_dir != nil and !is_string(base_dir)
      raise Exception('string expected in argument 1 (base_dir)')

    if !base_dir base_dir = ''

    var final_dir = os.join_paths(base_dir, self.directory)

    if !os.dir_exists(final_dir)
      os.create_dir(final_dir)

    var path = final_dir ? os.join_paths(final_dir, self.name) : self.name

    if self.data {
      if !file(path, 'wb').write(self.data) {
        return false
      }

      var fh = file(path)

      var last_mod = self.last_modified.to_time()
      if last_mod > 0 {
        fh.set_times(last_mod, last_mod)
      }

      if self.permission > 0 {
        fh.chmod(self.permission)
      }
    }

    return true
  }
}


/**
 * ZipFile represents an instance of zip file.
 */
class ZipFile {

  /**
   * The name of the zip file
   * @type string
   */
  var name

  /**
   * The last modified date for the zip file
   * @type Date
   */
  var last_modified

  /**
   * The time when the zip file was created
   * @type Date
   */
  var time_created

  /**
   * The size of the zip file
   * @type number
   */
  var size

  /**
   * The file handle for this zip file
   * @type file
   */
  var handle

  /**
   * A list of the ZipItems in the zip file
   * @type List<ZipItem>
   */
  var files = []

  /**
   * Exports the all files in the ZipFile to files on the machine. If base_dir is given, 
   * the files will be exported into the base_dir and all directories will be 
   * created inside of base_dir as is to reflect the ZipFile's original structure.
   * 
   * This function returns `true` if the operation succeeds or `false` otherwise.
   * 
   * @param string? base_dir: Default value is `os.cwd()`.
   * @returns bool
   */
  export(base_dir) {
    if base_dir != nil and !is_string(base_dir)
      raise Exception('string expected in argument 1 (base_dir)')

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
 * manipulation and extraction.
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
  var _is_64 = false
  var _compression_method = ZIP_DEFLATE

  /**
   * @param string path
   * @param number? compression_method: Default value is `ZIP_DEFLATE`
   * @param bool? use_zip_64: Default value is `false`.
   * @constructor
   */
  ZipArchive(path, compression_method, use_zip_64) {
    if !is_string(path)
      raise Exception('string expected in argument 1 (path)')
    if compression_method != nil and !is_number(compression_method)
      raise Exception('number expected in argument 2 (compression_method)')
    if use_zip_64 != nil and !is_bool(use_zip_64)
      raise Exception('boolean expected in argument 3 (use_zip_64)')
    self._file = path
    self._is_64 = use_zip_64 == nil ? false : use_zip_64
    
    if compression_method != nil {
      self._compression_method = compression_method
    }
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
        raise Exception('could not create new zip file')
    }
  }

  _is_within_bound(data) {
    # zip64 is always within bound.
    if self._is_64 return true

    var min_size = self._ctrl_dir.length() + 
          (self._handle ? self._handle.stats().size : 0) + 
            _ZIP_META_SIZE

    var data_size = data ? data.length() : 0

    if min_size + data_size < ZIP_MAX and
          data_size < ZIP_FILE_MAX and
            self._ctrl_dir_length < ZIP_FILE_COUNT_LIMIT {
      return true
    }

    return false
  }

  _dos_from_date(date) {
    var dos_date = (date.year - 1980) << 9 | date.month << 5 | date.day,
        dos_time = date.hour << 11 | date.minute << 5 | (date.seconds // 2)

    return [dos_time, dos_date]
  }

  /**
   * Adds a directory to the zip with the given name.
   * 
   * @param string name
   * @returns bool
   */
  create_dir(name) {

    # Ensure file is within Zip format limit bound.
    if !self._is_within_bound()
      return false

    self._write_open()

		name = name.replace(_path_replace_regex, '/')

    var mod_date = self._dos_from_date(date.localtime())

    var fr = _file_head_sig.to_bytes()

    fr.extend(pack(
      'v5', 
      self._is_64 ? _Z_64_VERSION : _Z_DEFAULT_VERSION,  # version needed to extract
      0,                        # general purpose bit flag
      self._compression_method, # compression method
      mod_date[0],              # last mod time
      mod_date[1]               # last mod date
    ))

    fr.extend(pack(
      'V3v2', 
      0,                # crc32
      0,                # compressed filesize
      0,                # uncompressed filesize
      name.length(),    # length of filename
      0                 # extra field length
    ))
    
		fr.extend(name.to_bytes())
		# end of 'local file header' segment

		# no 'file data' segment for path

		# 'data descriptor' segment (optional but necessary if archive is not served as file)
		fr.extend(pack(
      self._is_64 ? 'P3' : 'V3', 
      0,            # crc32
      0,            # compressed filesize
      0             # uncompressed filesize
    ))

		# add this entry to array
    self._handle.puts(fr)
    fr.dispose()

    var new_offset = self._get_new_offset()

		# ext. file attributes mirrors MS-DOS directory attr byte, detailed
		# at http:#support.microsoft.com/support/kb/articles/Q125/0/19.asp

    var extract_version = self._is_64 ? _Z_64_VERSION : _Z_DEFAULT_VERSION
    var creator = ((_is_unix ? 3 : 0) << 8) | extract_version

    # now add to central record
    self._ctrl_dir.extend(_central_head_sign.to_bytes())
    self._ctrl_dir.extend(pack(
      'v6V3v5V2',           
      creator,                  # version made by
      extract_version,          # version needed to extract
      0,                        # general purpose bit flag
      self._compression_method, # compression method
      mod_date[0],              # last mod time
      mod_date[1],              # last mod date
      0,                        # crc32
      0,                        # compressed filesize
      0,                        # uncompressed filesize
      name.length(),            # length of filename
      0,                        # extra field length
      0,                        # file comment length
      0,                        # disk number start
      0,                        # internal file attributes
      16,                       # external file attributes - 'archive' bit set
      self._old_offset          # relative offset of local header
    ))
    
		self._old_offset = new_offset

		self._ctrl_dir.extend(name.to_bytes())

		# optional extra field, file comment goes here
    self._ctrl_dir_length++

    return true
	}

  /**
   * Adds a file to the path specified with the contents given data.
   * 
   * @param string path
   * @param bytes|string data
   * @returns bool
   */
  create_file(path, data, stat) {

    # Ensure file is within Zip format limit bound.
    if !self._is_within_bound(data)
      return false

    self._write_open()

		path = path.replace(_path_replace_regex, '/')

    var mod_date = self._dos_from_date(date.from_time(stat.mtime))

		var fr = _file_head_sig.to_bytes()

    fr.extend(pack(
      'v5', 
      self._is_64 ? _Z_64_VERSION : _Z_DEFAULT_VERSION,  # version needed to extract
      0,                        # general purpose bit flag
      self._compression_method, # compression method
      mod_date[0],              # last mod time
      mod_date[1]               # last mod date
    ))

		var unc_len = data.length()
		var crc = zlib.crc32(data)

    var uzdata = data
    if self._compression_method == ZIP_DEFLATE {
      var zdata = zlib.compress(data)
      uzdata = zdata[2, -4] # fix crc bug
      zdata.dispose()
    }

		var c_len = uzdata.length()

    fr.extend(pack(
      'V3v2', 
      crc,                # crc32
      c_len,              # compressed filesize
      unc_len,            # uncompressed filesize
      path.length(),      # length of filename
      0                   # extra field length
    ))
    
		fr.extend(path.to_bytes())
		# end of 'local file header' segment

		# 'file data' segment
		fr.extend(uzdata)
    uzdata.dispose()

		# 'data descriptor' segment (optional but necessary if archive is not served as file)
    fr.extend(pack(
      self._is_64 ? 'P3' : 'V3', 
      crc,                # crc32
      c_len,              # compressed filesize
      unc_len             # uncompressed filesize
    ))

		# add this entry to array
    self._handle.puts(fr)
    fr.dispose()

		var new_offset = self._get_new_offset()

    var extract_version = self._is_64 ? _Z_64_VERSION : _Z_DEFAULT_VERSION
    var permission = (stat.mode << 16) >>> 0

    var creator = ((_is_unix ? 3 : 0) << 8) | extract_version

		# now add to central directory record
		self._ctrl_dir.extend(_central_head_sign.to_bytes())
    self._ctrl_dir.extend(pack(
      'v6V3v5V2',           
      creator,                  # version made by
      extract_version,          # version needed to extract
      0,                        # general purpose bit flag
      self._compression_method, # compression method
      mod_date[0],              # last mod time
      mod_date[1],              # last mod date
      crc,                      # crc32
      c_len,                    # compressed filesize
      unc_len,                  # uncompressed filesize
      path.length(),            # length of filename
      0,                        # extra field length
      0,                        # file comment length
      0,                        # disk number start
      0,                        # internal file attributes
      permission,               # external file attributes (32 - DOS Archive flag)
      self._old_offset          # relative offset of local header
    ))

		self._ctrl_dir.extend(path.to_bytes())

		# best to set this after adding the central directory.
		self._old_offset = new_offset

		# optional extra field, file comment goes here
    self._ctrl_dir_length++

    return true
	}

  /**
   * Adds an existing file to the archive. If destination is given, the 
   * file will be written to the destination path in the archive.
   * 
   * @param string path
   * @param string? destination
   * @returns bool
   */
  add_file(path, destination) {
    if !is_string(path)
      raise Exception('expected string as argument 1 (path)')
    if destination != nil and !is_string(destination)
      raise Exception('expected string as argument 2 (destination)')

    var f = file(path, 'rb')
    if !f.exists()
      raise Exception('file ${path} not found')

    if !destination destination = f.name()
    else destination = destination.replace(_path_replace_regex, '/')

    var content = f.read()
    var r = self.create_file(destination, content, f.stats())
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
            if npath.starts_with(os.path_separator)
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
            
            var f =  file(npath, 'rb')
            var content = f.read()
            var r = self.create_file(destination, content, f.stats())
            content.dispose()

            if !r return false
          }
        }
      }
		}

		return true
	}

  /**
   * Adds the specified `directory` recursively to the archive and set's it path in the archive to `dir`.
   * 
   * - If `file_blacklist` is not empty, this function will ignore every file with a matching path.
   * - If `ext_blacklist` is not empty, this function will ignore every file with a matching.
   * 
   * @param string directory
   * @param list file_blacklist: Default value is `[]`
   * @param list ext_blacklist: Default value is `[]`
   * @returns bool
   */
  add_directory(directory, file_blacklist, ext_blacklist) {
    if !is_string(directory)
      raise Exception('expected string in argument 1 (directory)')
    if file_blacklist != nil and !is_list(file_blacklist)
      raise Exception('expected list in argument 2 (file_blacklist)')
    if ext_blacklist != nil and !is_list(ext_blacklist)
      raise Exception('expected list in argument 3 (ext_blacklist)')

    directory = directory.replace(_path_replace_regex, '/')

    if !os.dir_exists(directory)
      raise Exception('directory ${directory} not found')

    if !file_blacklist file_blacklist = []
    if !ext_blacklist ext_blacklist = []

		return self._add_files(directory, '', file_blacklist, ext_blacklist)
	}

  /**
   * Reads the zip file in the specified path and returns a list of
   * ZipFile describing it's contents.
   * 
   * @param string path
   * @returns ZipFile
   */
  read() {

    # Get stats.
    var fh = file(self._file, 'rb')
    var zip_stats = fh.stats()

    if zip_stats.size > ZIP_FILE_MAX
      raise Exception(_64_required)

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
	  var filesecta = filedata.split(
      (self._is_64 ? _central_end_sign64 : _central_end_sign).to_bytes()
    )

    if filesecta.length() < 2 return zip_file

		# ZIP Comment
		var unpackeda = unpack('x16/v1length', filesecta[1])
		self.comment = filesecta[1][18, 18 + unpackeda['length']]
    # self.comment = self.comment.replace('~\\r(\\n)?~', '\n')  # CR + LF and CR -> LF

		# Cut entries from the central directory
		filesecta = filedata.split(
      (self._is_64 ? _central_end_sign64 : _central_end_sign).to_bytes()
    )
		filesecta = filesecta[0].split(_file_head_sig.to_bytes())
		filesecta = filesecta[1,] # Removes empty entry/signature

		for filedata in filesecta {
			# CRC:crc, FD:file date, FT: file time, CM: compression method, GPF: general purpose flag, VN: version needed, CS: compressed size, UCS: uncompressed size, FNL: filename length
			var entrya = {}
			entrya['error'] = nil

			unpackeda = unpack(_file_head_unpack, filedata)

			# Check for encryption
			var isencrypted = (unpackeda['general_purpose'] & 0x0001) > 0 ? true : false

			# Check for value block after compressed data
			if unpackeda['general_purpose'] & 0x0008 > 0 {
				var unpackeda2 = unpack(
          self._is_64 ? _file_size_unpack64 : _file_size_unpack, 
          filedata[filedata.length() - 12,]
        )

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
            catch {
              decoded = zlib.undeflate(filedata)
              filedata.dispose()
            } as e

            if e {
              decoded = bytes(0)
            }
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
      entrya.set('encrypted', isencrypted)

      if unpackeda['external_attribute'] > 32 {
        entrya['permission'] = (unpackeda['external_attribute'] >>> 16) ^ 0c100000
      } else {
        entrya['permission'] = 0
      }
      
      zip_file.files.append(ZipItem.from_dict(entrya))
		}

    return zip_file
	}

  /**
   * Saves the current Zip archive to file.
   * 
   * @param string filename
   * @returns bool
   */
  save() {
    if self._handle and self._handle.is_open() {
      
      self._handle.puts(self._ctrl_dir)

      # end of Central directory record
      var ending = (self._is_64 ? _central_end_sign64 : _central_end_sign).to_bytes()

      var extract_version = self._is_64 ? _Z_64_VERSION : _Z_DEFAULT_VERSION
      var creator = ((_is_unix ? 3 : 0) << 8) | extract_version

      if self._is_64 {
        ending.extend(pack(
          'Pvv',
          44,                 # size of zip64 end of central directory record
          creator,            # version made by
          extract_version     # version needed to extract
        ))
      }

      ending.extend(
        (self._is_64 ? _disk_record_sign64 : _disk_record_sign).to_bytes()
      )

      ending.extend(pack(
        self._is_64 ? 'PPPPv' : 'vvVVv', 
        self._ctrl_dir_length,      # total number of entries 'on this disk'
        self._ctrl_dir_length,      # total number of entries overall
        self._ctrl_dir.length(),    # size of central dir
        self._handle.tell(),        # offset to start of central dir
        0                           # .zip file comment length
      ))

      self._handle.puts(ending)
      self._handle.close()

      ending.dispose()
      self._ctrl_dir.dispose()

      return true
    }
    
    raise Exception('zip file not open')
	}
}


/**
 * Extracts the zip archive at the _file_ path to the given _destination_ directory. 
 * If _destination_ is not given, the file will be extracted into the current working 
 * directory.
 *
 * This function returns `true` if the extraction was successful and `false` otherwise.
 * 
 * > **NOTE:**
 * > Set `is_zip64` to true if the size of the zip file exceeds `ZIP_MAX`.
 * 
 * @param string file
 * @param string? destination: Default value is `os.cwd()`.
 * @param bool? is_zip64: Default value is `false`.
 * @returns bool
 */
def extract(file, destination, is_zip64) {
  if !is_string(file)
    raise Exception('string expected in argument 1 (file)')
  if destination != nil and !is_string(destination)
    raise Exception('string expected in argument 2 (destination)')
  if is_zip64 != nil and !is_bool(is_zip64)
    raise Exception('bool expected in argument 3 (use_zip64)')

  if is_zip64 == nil is_zip64 = false

  var zip = ZipArchive(file, nil, is_zip64)
  var zip_file = zip.read()
  return zip_file.export(destination)
}

/**
 * Compresses the given path (file or directory) into the destination zip archive.
 * @raises  Exception if file could not be written of zip max size exceeded.
 * 
 * > When an exception is thrown because max size was exceeded, some files could
 * > have already been compressed. In this case, the zip archive will should still 
 * > be usable but not all desired files will be contained in it.
 * 
 * > **NOTE:**
 * > Set `use_zip64` to true when compressing files exceeding `ZIP_FILE_MAX` or 
 * > `ZIP_FILE_COUNT_LIMIT`
 * 
 * @param string file
 * @param string? destination: Default value is `os.cwd()`.
 * @param number? compression_method: Default value is `ZIP_DEFLATE`
 * @param bool? is_zip64: Default value is `false`.
 * @returns bool
 */
def compress(path, destination, compression_method, use_zip64) {
  if !is_string(path)
    raise Exception('string expected in argument 1 (path)')
  if destination != nil and !is_string(destination)
    raise Exception('string expected in argument 2 (destination)')
  if compression_method != nil and !is_number(compression_method)
    raise Exception('number expected in argument 3 (compression_method)')
  if use_zip64 != nil and !is_bool(use_zip64)
    raise Exception('boolean expected in argument 4 (use_zip64)')

  if !destination 
    destination = os.join_paths(os.cwd(), os.base_name(path)) + ZIP_EXT
  destination = os.abs_path(destination)

  if use_zip64 == nil use_zip64 = false

  var zip = ZipArchive(destination, compression_method, use_zip64)

  var completed = false
  
  # If path points to a directory, archive a directory else archive as a file.
  if os.dir_exists(path) {
    var current_directory = os.cwd()

    # Enter into the path so that we can treat the path as root.
    os.change_dir(path)

    completed = zip.add_directory(path)

    os.change_dir(current_directory)
  } else {
    completed = zip.add_file(path)
  }

  if !completed or !zip.save()
    raise Exception(_64_required)

  return true
}

