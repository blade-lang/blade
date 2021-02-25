/**
 * Mime
 *
 * Bird's mimetype detection library
 * @copyright Ore Richard */

# Internal helper class to help
# Mime class represent and organize it's mimes better
# @note, This class is not intended for calling outside of this library
# unless it provides you with the exact mechanism you need to solve
# your problem.
class _MimeFormat {
  _MimeFormat(mimetype, header) {
    if header and !is_list(header) {
      die Exception('Mime type definition must set header to nil or list of byte (numbers)')
    }

    self.mimetype = mimetype
    self.header = header
  }
}

# This class is capable of detecting file mimetypes even
# in the abscence of an exception.
# Example:
#
# var f = file('myfile', 'rb')
# 
# # using 'rb' here for two reasons: 
# # 1. Our file has no extension, so extension based detection is impossible
# # 2. We want more accuracy by having Mime check file headers
#
# echo Mime(f).detect()
class Mime {

  # internal library of mime formats mapped by extension
  var _mimes = {
    '.7z': _MimeFormat('application/x-7z-compressed', [[0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C]]),
    '.3gp': _MimeFormat('video/3gpp', [[0x00, 0x00, 0x00, 0x14, 0x66, 0x74, 0x79, 0x70]]),
    '.3g2': _MimeFormat('video/3gpp2', [[0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70]]),
    '.aac': _MimeFormat('audio/aac', nil),
    '.abw': _MimeFormat('application/x-abiword', nil),
    '.ai': _MimeFormat('application/postscript', nil),
    '.aif': _MimeFormat('audio/x-aiff', nil),
    '.aifc': _MimeFormat('audio/x-aiff', nil),
    '.aiff': _MimeFormat('audio/x-aiff', [[0x46, 0x4F, 0x52, 0x4D, 0x00]]),
    '.arc': _MimeFormat('application/x-freearc', nil),
    '.asc': _MimeFormat('text/plain', nil),
    '.atom': _MimeFormat('application/atom+xml', nil),
    '.au': _MimeFormat('audio/basic', nil),
    '.avi': _MimeFormat('video/x-msvideo', [[0x52, 0x49, 0x46, 0x46]]),
    '.azw': _MimeFormat('application/vnd.amazon.ebook', nil),
    '.bcpio': _MimeFormat('application/x-bcpio', nil),
    '.bin': _MimeFormat('application/octet-stream', [[0x42, 0x4C, 0x49, 0x32, 0x32, 0x33, 0x51]]),
    '.bmp': _MimeFormat('image/bmp', [[0x42, 0x4D]]),
    '.bz2': _MimeFormat('application/x-bzip2', [[0x42, 0x5A, 0x68]]),
    '.bz': _MimeFormat('application/x-bzip', nil),
    '.cdf': _MimeFormat('application/x-netcdf', nil),
    '.cgm': _MimeFormat('image/cgm', nil),
    '.class': _MimeFormat('application/octet-stream', [[0xCA, 0xFE, 0xBA, 0xBE]]),
    '.cpio': _MimeFormat('application/x-cpio', nil),
    '.cpt': _MimeFormat('application/mac-compactpro', nil),
    '.csh': _MimeFormat('application/x-csh', nil),
    '.css': _MimeFormat('text/css', nil),
    '.csv': _MimeFormat('text/csv', nil),
    '.dcr': _MimeFormat('application/x-director', nil),
    '.dif': _MimeFormat('video/x-dv', nil),
    '.dir': _MimeFormat('application/x-director', nil),
    '.djv': _MimeFormat('image/vnd.djvu', nil),
    '.djvu': _MimeFormat('image/vnd.djvu', nil),
    '.dll': _MimeFormat('application/octet-stream', [[0x49, 0x5A]]),
    '.dmg': _MimeFormat('application/octet-stream', [[0x78]]),
    '.dms': _MimeFormat('application/octet-stream', nil),
    '.doc': _MimeFormat('application/msword', [
      [0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1],
      [0x0D, 0x44, 0x4F, 0x43], # DestMate document
      [0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1, 0x00], # Perfect Office document
      [0xDB, 0xA5, 0x2D, 0x00], # Word 2.0
      [0xEC, 0xA5, 0xC1, 0x00] # Word document subheader
    ]),
    '.docx': _MimeFormat('application/vnd.openxmlformats-officedocument.wordprocessingml.document', [
      [0x50, 0x4B, 0x03, 0x04], # MS Office Open XML Format Document
      [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00] # MS Office 2007 documents
    ]),
    '.dotx': _MimeFormat('application/vnd.openxmlformats-officedocument.wordprocessingml.template', nil),
    '.docm': _MimeFormat('application/vnd.ms-word.document.macroEnabled.12', nil),
    '.dotm': _MimeFormat('application/vnd.ms-word.template.macroEnabled.12', nil),
    '.dtd': _MimeFormat('application/xml-dtd', nil),
    '.dv': _MimeFormat('video/x-dv', nil),
    '.dvi': _MimeFormat('application/x-dvi', nil),
    '.dvix': _MimeFormat('video/divx', nil),
    '.dwg': _MimeFormat('application/acad', [
      [0x41, 0x43, 0x31, 0x30]
    ]),
    '.dxr': _MimeFormat('application/x-director', nil),
    '.eot': _MimeFormat('application/vnd.ms-fontobject', nil),
    '.eps': _MimeFormat('application/postscript', [
      [0xC5, 0xD0, 0xD3, 0xC6]
    ]),
    '.epub': _MimeFormat('application/epub+zip', nil),
    '.etx': _MimeFormat('text/x-setext', nil),
    '.exe': _MimeFormat('application/octet-stream', [
      [0x4D, 0x5A]
    ]),
    '.ez': _MimeFormat('application/andrew-inset', nil),
    '.flv': _MimeFormat('video/x-flv', [
      [0x46, 0x4C, 0x56, 0x01]
    ]),
    '.gif': _MimeFormat('image/gif', [
      [0x47, 0x49, 0x46, 0x38]
    ]),
    '.gz': _MimeFormat('application/gzip', [
      [0x1F, 0x8B, 0x08]
    ]),
    '.gram': _MimeFormat('application/srgs', nil),
    '.grxml': _MimeFormat('application/srgs+xml', nil),
    '.gtar': _MimeFormat('application/x-gtar', nil),
    '.hdf': _MimeFormat('application/x-hdf', nil),
    '.hqx': _MimeFormat('application/mac-binhex40', nil),
    '.htm': _MimeFormat('text/html', nil),
    '.html': _MimeFormat('text/html', nil),
    '.ice': _MimeFormat('x-conference/x-cooltalk', nil),
    '.ico': _MimeFormat('image/vnd.microsoft.icon', [
      [0x00, 0x00, 0x01, 0x00]
    ]),
    '.ics': _MimeFormat('text/calendar', nil),
    '.ief': _MimeFormat('image/ief', nil),
    '.ifb': _MimeFormat('text/calendar', nil),
    '.iges': _MimeFormat('model/iges', nil),
    '.igs': _MimeFormat('model/iges', nil),
    '.jar': _MimeFormat('application/java-archive', [
      [0x50, 0x4B, 0x03, 0x04],
      [0x5F, 0x27, 0xA8, 0x89],
      [0x4A, 0x41, 0x52, 0x43, 0x53, 0x00],
      [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x08, 0x00]
    ]),
    '.jnlp': _MimeFormat('application/x-java-jnlp-file', nil),
    '.jp2': _MimeFormat('image/jp2', [
      [0x00, 0x00, 0x00, 0x0C, 0x6A, 0x50, 0x20, 0x20]
    ]),
    '.jpe': _MimeFormat('image/jpeg', [
      [0xFF, 0xD8, 0xFF, 0xE0]
    ]),
    '.jpeg': _MimeFormat('image/jpeg', [
      [0xFF, 0xD8, 0xFF, 0xE0], 
      [0xFF, 0xD8, 0xFF, 0xE2], 
      [0xFF, 0xD8, 0xFF, 0xE3]
    ]),
    '.jpg': _MimeFormat('image/jpeg', [
      [0xFF, 0xD8, 0xFF, 0xE0], 
      [0xFF, 0xD8, 0xFF, 0xE1], 
      [0xFF, 0xD8, 0xFF, 0xE8]
    ]),
    '.js': _MimeFormat('application/x-javascript', nil),
    '.json': _MimeFormat('application/json', nil),
    '.jsonld': _MimeFormat('application/ld+json', nil),
    '.kar': _MimeFormat('audio/midi', nil),
    '.latex': _MimeFormat('application/x-latex', nil),
    '.lha': _MimeFormat('application/octet-stream', nil),
    '.lib': _MimeFormat('application/octet-stream', [[0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E, 0x0A]]),
    '.lzh': _MimeFormat('application/octet-stream', nil),
    '.m3u': _MimeFormat('audio/x-mpegurl', nil),
    '.m4a': _MimeFormat('audio/mp4a-latm', nil),
    '.m4b': _MimeFormat('audio/mp4a-latm', nil),
    '.m4p': _MimeFormat('audio/mp4a-latm', nil),
    '.m4u': _MimeFormat('video/vnd.mpegurl', nil),
    '.m4v': _MimeFormat('video/x-m4v', nil),
    '.mac': _MimeFormat('image/x-macpaint', nil),
    '.man': _MimeFormat('application/x-troff-man', nil),
    '.mathml': _MimeFormat('application/mathml+xml', nil),
    '.me': _MimeFormat('application/x-troff-me', nil),
    '.mesh': _MimeFormat('model/mesh', nil),
    '.mid': _MimeFormat('audio/midi', [[0x4D, 0x54, 0x68, 0x64]]),
    '.midi': _MimeFormat('audio/midi', [[0x4D, 0x54, 0x68, 0x64]]),
    '.mif': _MimeFormat('application/vnd.mif', nil),
    '.mjs': _MimeFormat('text/javascript', nil),
    '.mkv': _MimeFormat('video/x-matroska', [[0x1A, 0x45, 0xDF, 0xA3, 0x93, 0x42, 0x82, 0x88]]),
    '.mov': _MimeFormat('video/quicktime', nil),
    '.movie': _MimeFormat('video/x-sgi-movie', nil),
    '.mp2': _MimeFormat('audio/mpeg', nil),
    '.mp3': _MimeFormat('audio/mpeg', [[0x49, 0x44, 0x33]]),
    '.mp4': _MimeFormat('video/mp4', nil),
    '.mpe': _MimeFormat('video/mpeg', nil),
    '.mpeg': _MimeFormat('video/mpeg', nil),
    '.mpg': _MimeFormat('video/mpeg', nil),
    '.mpga': _MimeFormat('audio/mpeg', nil),
    '.mpkg': _MimeFormat('application/vnd.apple.installer+xml', nil),
    '.ms': _MimeFormat('application/x-troff-ms', nil),
    '.msh': _MimeFormat('model/mesh', nil),
    '.mxu': _MimeFormat('video/vnd.mpegurl', nil),
    '.nc': _MimeFormat('application/x-netcdf', nil),
    '.oda': _MimeFormat('application/oda', nil),
    '.odp': _MimeFormat('application/vnd.oasis.opendocument.presentation', [[0x50, 0x4B, 0x03, 0x04]]),
    '.ods': _MimeFormat('application/vnd.oasis.opendocument.spreadsheet', nil),
    '.odt': _MimeFormat('application/vnd.oasis.opendocument.text', [[0x50, 0x4B, 0x03, 0x04]]),
    '.ogg': _MimeFormat('video/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
    '.ogx': _MimeFormat('application/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
    '.oga': _MimeFormat('audio/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
    '.ogv': _MimeFormat('video/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
    '.opus': _MimeFormat('audio/opus', nil),
    '.otf': _MimeFormat('font/otf', nil),
    '.pbm': _MimeFormat('image/x-portable-bitmap', nil),
    '.pct': _MimeFormat('image/pict', nil),
    '.pdb': _MimeFormat('chemical/x-pdb', nil),
    '.pdf': _MimeFormat('application/pdf', [[0x25, 0x50, 0x44, 0x46]]),
    '.pgm': _MimeFormat('image/x-portable-graymap', nil),
    '.pgn': _MimeFormat('application/x-chess-pgn', nil),
    '.php': _MimeFormat('application/x-httpd-php', nil),
    '.pic': _MimeFormat('image/pict', nil),
    '.pict': _MimeFormat('image/pict', nil),
    '.png': _MimeFormat('image/png', [[0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]]), 
    '.pnm': _MimeFormat('image/x-portable-anymap', nil),
    '.pnt': _MimeFormat('image/x-macpaint', nil),
    '.pntg': _MimeFormat('image/x-macpaint', nil),
    '.ppm': _MimeFormat('image/x-portable-pixmap', nil),
    '.ppt': _MimeFormat('application/vnd.ms-powerpoint', [[0xFD, 0xFF, 0xFF, 0xFF, nil, 0x00, 0x00, 0x00]]),
    '.pptx': _MimeFormat('application/vnd.openxmlformats-officedocument.presentationml.presentation', [
      [0x50, 0x4B, 0x03, 0x04],
      [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00]
    ]),
    '.potx': _MimeFormat('application/vnd.openxmlformats-officedocument.presentationml.template', nil),
    '.ppsx': _MimeFormat('application/vnd.openxmlformats-officedocument.presentationml.slideshow', nil),
    '.ppam': _MimeFormat('application/vnd.ms-powerpoint.addin.macroEnabled.12', nil),
    '.pptm': _MimeFormat('application/vnd.ms-powerpoint.presentation.macroEnabled.12', nil),
    '.potm': _MimeFormat('application/vnd.ms-powerpoint.template.macroEnabled.12', nil),
    '.ppsm': _MimeFormat('application/vnd.ms-powerpoint.slideshow.macroEnabled.12', nil),
    '.ps': _MimeFormat('application/postscript', nil),
    '.psd': _MimeFormat('application/octet-stream', [[0x38, 0x42, 0x50, 0x53]]),
    '.pst': _MimeFormat('application/vnd.ms-outlook', [[0x21, 0x42, 0x44, 0x4E]]),
    '.pub': _MimeFormat('application/x-mspublisher', [[0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1]]),
    '.qt': _MimeFormat('video/quicktime', nil),
    '.qti': _MimeFormat('image/x-quicktime', nil),
    '.qtif': _MimeFormat('image/x-quicktime', nil),
    '.ra': _MimeFormat('audio/x-pn-realaudio', [
      [0x2E, 0x52, 0x4D, 0x46, 0x00, 0x00, 0x00, 0x12],
      [0x2E, 0x72, 0x61, 0xFD, 0x00] # the streaming variant
    ]),
    '.ram': _MimeFormat('audio/x-pn-realaudio', [[0x72, 0x74, 0x73, 0x70, 0x3A, 0x2F, 0x2F]]),
    '.rar': _MimeFormat('application/vnd.rar', [[0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00]]),
    '.ras': _MimeFormat('image/x-cmu-raster', nil),
    '.rdf': _MimeFormat('application/rdf+xml', nil),
    '.rgb': _MimeFormat('image/x-rgb', [[0x01, 0xDA, 0x01, 0x01, 0x00, 0x03]]),
    '.rm': _MimeFormat('application/vnd.rn-realmedia', [[0x2E, 0x52, 0x4D, 0x46]]),
    '.roff': _MimeFormat('application/x-troff', nil),
    '.rtf': _MimeFormat('text/rtf', [[0x7B, 0x5C, 0x72, 0x74, 0x66, 0x31]]),
    '.rtx': _MimeFormat('text/richtext', nil),
    '.sgm': _MimeFormat('text/sgml', nil),
    '.sgml': _MimeFormat('text/sgml', nil),
    '.sh': _MimeFormat('application/x-sh', nil),
    '.shar': _MimeFormat('application/x-shar', nil),
    '.silo': _MimeFormat('model/mesh', nil),
    '.sit': _MimeFormat('application/x-stuffit', [
      [0x53, 0x49, 0x54, 0x21, 0x00],
      [0x53, 0x74, 0x75, 0x66, 0x66, 0x49, 0x74, 0x20] # the compressed archive
    ]),
    '.skd': _MimeFormat('application/x-koan', nil),
    '.skm': _MimeFormat('application/x-koan', nil),
    '.skp': _MimeFormat('application/x-koan', nil),
    '.skt': _MimeFormat('application/x-koan', nil),
    '.smi': _MimeFormat('application/smil', nil),
    '.smil': _MimeFormat('application/smil', nil),
    '.snd': _MimeFormat('audio/basic', nil),
    '.so': _MimeFormat('application/octet-stream', nil),
    '.spl': _MimeFormat('application/x-futuresplash', [[0x00, 0x00, 0x01, 0x00]]),
    '.src': _MimeFormat('application/x-wais-source', nil),
    '.sv4cpio': _MimeFormat('application/x-sv4cpio', nil),
    '.sv4crc': _MimeFormat('application/x-sv4crc', nil),
    '.svg': _MimeFormat('image/svg+xml', nil),
    '.swf': _MimeFormat('application/x-shockwave-flash', [[0x46, 0x57, 0x53]]),
    '.t': _MimeFormat('application/x-troff', nil),
    '.tar': _MimeFormat('application/x-tar', [[0x75, 0x73, 0x74, 0x61, 0x72]]),
    '.tcl': _MimeFormat('application/x-tcl', nil),
    '.tex': _MimeFormat('application/x-tex', nil),
    '.texi': _MimeFormat('application/x-texinfo', nil),
    '.texinfo': _MimeFormat('application/x-texinfo', nil),
    '.tgz': _MimeFormat('application/x-gz', [[0x1F, 0x8B, 0x08]]),
    '.tif': _MimeFormat('image/tiff', [
      [0x49, 0x20, 0x49],
      [0x49, 0x49, 0x2A, 0x00],
      [0x4D, 0x4D, 0x00, 0x2A],
      [0x4D, 0x4D, 0x00, 0x2B]
    ]),
    '.tiff': _MimeFormat('image/tiff', [
      [0x49, 0x20, 0x49],
      [0x49, 0x49, 0x2A, 0x00],
      [0x4D, 0x4D, 0x00, 0x2A],
      [0x4D, 0x4D, 0x00, 0x2B]
    ]),
    '.torrent': _MimeFormat('application/x-bittorrent', [[0x64, 0x38, 0x3A, 0x61, 0x6E, 0x6E, 0x6F, 0x75, 0x6E, 0x63, 0x65]]),
    '.tr': _MimeFormat('application/x-troff', nil),
    '.ts': _MimeFormat('video/mp2t', nil),
    '.tsv': _MimeFormat('text/tab-separated-values', nil),
    '.ttf': _MimeFormat('application/x-font-ttf', [[0x00, 0x01, 0x00, 0x00, 0x00]]),
    '.txt': _MimeFormat('text/plain', nil),
    '.ustar': _MimeFormat('application/x-ustar', nil),
    '.vcd': _MimeFormat('application/x-cdlink', nil),
    '.vrml': _MimeFormat('model/vrml', nil),
    '.vsd': _MimeFormat('application/vnd.visio', nil),
    '.vxml': _MimeFormat('application/voicexml+xml', nil),
    '.wav': _MimeFormat('audio/x-wav', [[0x52, 0x49, 0x46, 0x46]]),
    '.wbmp': _MimeFormat('image/vnd.wap.wbmp', nil),
    '.wbmxl': _MimeFormat('application/vnd.wap.wbxml', nil),
    '.weba': _MimeFormat('image/weba', nil),
    '.webm': _MimeFormat('image/webm', nil),
    '.webp': _MimeFormat('image/webp', nil),
    '.woff': _MimeFormat('font/woff', nil),
    '.woff2': _MimeFormat('font/woff2', nil),
    '.wml': _MimeFormat('text/vnd.wap.wml', nil),
    '.wmlc': _MimeFormat('application/vnd.wap.wmlc', nil),
    '.wmls': _MimeFormat('text/vnd.wap.wmlscript', nil),
    '.wmlsc': _MimeFormat('application/vnd.wap.wmlscriptc', nil),
    '.wmv': _MimeFormat('video/x-ms-wmv', [[0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11]]),
    '.wma': _MimeFormat('audio/x-ms-wmv', [[0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11]]),
    '.wrl': _MimeFormat('model/vrml', nil),
    '.xbm': _MimeFormat('image/x-xbitmap', nil),
    '.xht': _MimeFormat('application/xhtml+xml', nil),
    '.xhtml': _MimeFormat('application/xhtml+xml', nil),
    '.xls': _MimeFormat('application/vnd.ms-excel', [[0x09, 0x08, 0x10, 0x00, 0x00, 0x06, 0x05, 0x00]]),                        
    '.xml': _MimeFormat('application/xml', nil),
    '.xpm': _MimeFormat('image/x-xpixmap', nil),
    '.xsl': _MimeFormat('application/xml', nil),
    '.xlsx': _MimeFormat('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet', [
      [0x50, 0x4B, 0x03, 0x04], # MS Office Open XML Format Document
      [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00] # MS Office 2007 documents
    ]),
    '.xltx': _MimeFormat('application/vnd.openxmlformats-officedocument.spreadsheetml.template', nil),
    '.xlsm': _MimeFormat('application/vnd.ms-excel.sheet.macroEnabled.12', nil),
    '.xltm': _MimeFormat('application/vnd.ms-excel.template.macroEnabled.12', nil),
    '.xlam': _MimeFormat('application/vnd.ms-excel.addin.macroEnabled.12', nil),
    '.xlsb': _MimeFormat('application/vnd.ms-excel.sheet.binary.macroEnabled.12', nil),
    '.xslt': _MimeFormat('application/xslt+xml', nil),
    '.xul': _MimeFormat('application/vnd.mozilla.xul+xml', nil),
    '.xwd': _MimeFormat('image/x-xwindowdump', nil),
    '.xyz': _MimeFormat('chemical/x-xyz', nil),
    '.zip': _MimeFormat('application/zip', [
      [0x50, 0x4B, 0x03, 0x04],
      [0x50, 0x4B, 0x4C, 0x49, 0x54, 0x45], # light archive
      [0x50, 0x4B, 0x53, 0x70, 0x58], # self-extracting
      [0x50, 0x4B, 0x05, 0x06],
      [0x50, 0x4B, 0x07, 0x08],
      [0x57, 0x69, 0x6E, 0x5A, 0x69, 0x70], # WinZip compressed archive
      [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x01, 0x00] # ZLock Pro encrypted archive
    ])
  }

  # The Mime class constructor, accepts the file to work on as argument
  # When dealing with not-so-popular files, 
  # the path passed to the file constructor should contain a valid 
  # extension for optimal accuracy,
  # This is however not required for files to contain any...
  Mime(file) {
    if !is_file(file) {
      die Exception('file expected, ${type(file)} given')
    }
    self.file = file
  }

  # Detects the mimetype of a file based on the
  # extension defined in it's path.
  # @note, For popular files such as Jpeg and Pngs,
  # calling this method directly is more efficient and
  # provides a faster lookup
  detect_from_name() {
    for ext, mime in self._mimes {
      if self.file.name().ends_with(ext) return mime.mimetype
    }
    return 'application/octet-stream'
  }

  # Detects the mimetype of a file based on it's file header.
  # When multiple file formats share very similar or shadowing
  # file headers (such as the relationship between Zip files and Docx files),
  # this method will perform an extension before returning it's result.
  # @note, For dealing with files without extension, or where the accuracy of the 
  # file extension cannot be trusted, this method provides a more efficient lookup.
  # @note, This method may produce slightly more rigorous results
  # @note, This method requires that the file must be opened in binary mode
  detect_from_header() {
    if !self.file.mode().match('b') {
      die Exception('detect_from_header expects file to be opened in binary mode')
    }

    var top_16 = self.file.read(16)
    var last_mime_found = nil

    for ext, mime in self._mimes {
      if mime.header {
        iter var i = 0; i < mime.header.length(); i++ {
          var mime_match = true
          var header = mime.header[i]

          iter var j = 0; j < header.length(); j++ {
            if header[j] != top_16[j] {
              mime_match = false
              break
            }
          }

          if mime_match {
            if self.file.name().ends_with(ext) return mime.mimetype
            else last_mime_found = mime.mimetype
          }
        }
      }
    }

    if last_mime_found return last_mime_found
    return 'application/octet-stream'
  }

  # Performs mimetype detection on a file.
  # If the file is opened in binary mode, it first attempt the more
  # accurate header check. If the header check returns a generic result 
  # (i.e. application/octet-stream), it performs an extension lookup.
  # @note, this method gives the best result, but slightly slower than
  # a direct lookup of name or header.
  detect() {
    if self.file.mode().match('b') {
      var mime = self.detect_from_header()
      if mime == 'application/octet-stream' {
        return self.detect_from_name()
      }
      return mime
    }
    return self.detect_from_name()
  }
}