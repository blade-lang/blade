#
# @module mime
#
# This module provides functions that allow easy mime type detection from files. 
# It offers support for detecting file type based on name or file headers and it 
# is completely extensible so that you can add declarations for your own custom 
# file types.
# 
# _See defined functions for example._
# 
# @copyright O2021, Ore Richard Muyiwa
# 


/**
 * Mime format representation class.
 */
class MimeFormat {
  /**
   * MimeFormat(mimetype: string [, header: list | bytes])
   * 
   * @constructor
   * @note only the first 16 bytes of a file header will be used.
   */
  MimeFormat(mimetype, header) {
    if !is_string(mimetype)
      die Exception('expecting mimetype as string')
    if header and !is_list(header)
      die Exception('Mime type definition must set header to nil, list of numbers or bytes')

    self.mimetype = mimetype
    self.header = header
  }
}

# internal library of mime formats mapped by extension
var _mimes = {
  '.7z': MimeFormat('application/x-7z-compressed', [[0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C]]),
  '.3gp': MimeFormat('video/3gpp', [[0x00, 0x00, 0x00, 0x14, 0x66, 0x74, 0x79, 0x70]]),
  '.3g2': MimeFormat('video/3gpp2', [[0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70]]),
  '.aac': MimeFormat('audio/aac', nil),
  '.abw': MimeFormat('application/x-abiword', nil),
  '.ai': MimeFormat('application/postscript', nil),
  '.aif': MimeFormat('audio/x-aiff', nil),
  '.aifc': MimeFormat('audio/x-aiff', nil),
  '.aiff': MimeFormat('audio/x-aiff', [[0x46, 0x4F, 0x52, 0x4D, 0x00]]),
  '.arc': MimeFormat('application/x-freearc', nil),
  '.asc': MimeFormat('text/plain', nil),
  '.atom': MimeFormat('application/atom+xml', nil),
  '.au': MimeFormat('audio/basic', nil),
  '.avi': MimeFormat('video/x-msvideo', [[0x52, 0x49, 0x46, 0x46]]),
  '.azw': MimeFormat('application/vnd.amazon.ebook', nil),
  '.bcpio': MimeFormat('application/x-bcpio', nil),
  '.bin': MimeFormat('application/octet-stream', [[0x42, 0x4C, 0x49, 0x32, 0x32, 0x33, 0x51]]),
  '.bmp': MimeFormat('image/bmp', [[0x42, 0x4D]]),
  '.bz2': MimeFormat('application/x-bzip2', [[0x42, 0x5A, 0x68]]),
  '.bz': MimeFormat('application/x-bzip', nil),
  '.cab': MimeFormat('application/vnd.ms-cab-compressed', [[0x4D, 0x53, 0x43, 0x46]]),
  '.cdf': MimeFormat('application/x-netcdf', nil),
  '.cgm': MimeFormat('image/cgm', nil),
  '.chm': MimeFormat('application/vnd.ms-htmlhelp', [[0x49, 0x54, 0x53, 0x46, 0x03, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00]]),
  '.class': MimeFormat('application/octet-stream', [[0xCA, 0xFE, 0xBA, 0xBE]]),
  '.conf': MimeFormat('text/plain', nil),
  '.cpio': MimeFormat('application/x-cpio', nil),
  '.cpt': MimeFormat('application/mac-compactpro', nil),
  '.csh': MimeFormat('application/x-csh', nil),
  '.css': MimeFormat('text/css', nil),
  '.csv': MimeFormat('text/csv', nil),
  '.dcr': MimeFormat('application/x-director', nil),
  '.deb': MimeFormat('application/x-debian-package', [[0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E]]),
  '.def': MimeFormat('text/plain', nil),
  '.dif': MimeFormat('video/x-dv', nil),
  '.dir': MimeFormat('application/x-director', nil),
  '.djv': MimeFormat('image/vnd.djvu', nil),
  '.djvu': MimeFormat('image/vnd.djvu', nil),
  '.dll': MimeFormat('application/x-msdownload', [[0x49, 0x5A]]),
  '.dmg': MimeFormat('application/x-apple-diskimage', [[0x78]]),
  '.dms': MimeFormat('application/octet-stream', nil),
  '.doc': MimeFormat('application/msword', [
    [0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1],
    [0x0D, 0x44, 0x4F, 0x43], # DestMate document
    [0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1, 0x00], # Perfect Office document
    [0xDB, 0xA5, 0x2D, 0x00], # Word 2.0
    [0xEC, 0xA5, 0xC1, 0x00] # Word document subheader
  ]),
  '.docx': MimeFormat('application/vnd.openxmlformats-officedocument.wordprocessingml.document', [
    [0x50, 0x4B, 0x03, 0x04], # MS Office Open XML Format Document
    [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00] # MS Office 2007 documents
  ]),
  '.dotx': MimeFormat('application/vnd.openxmlformats-officedocument.wordprocessingml.template', nil),
  '.docm': MimeFormat('application/vnd.ms-word.document.macroEnabled.12', nil),
  '.dotm': MimeFormat('application/vnd.ms-word.template.macroEnabled.12', nil),
  '.dtd': MimeFormat('application/xml-dtd', nil),
  '.dv': MimeFormat('video/x-dv', nil),
  '.dvi': MimeFormat('application/x-dvi', nil),
  '.dvix': MimeFormat('video/divx', nil),
  '.dwg': MimeFormat('application/acad', [
    [0x41, 0x43, 0x31, 0x30]
  ]),
  '.dxr': MimeFormat('application/x-director', nil),
  '.eot': MimeFormat('application/vnd.ms-fontobject', nil),
  '.eps': MimeFormat('application/postscript', [
    [0xC5, 0xD0, 0xD3, 0xC6]
  ]),
  '.epub': MimeFormat('application/epub+zip', nil),
  '.etx': MimeFormat('text/x-setext', nil),
  '.exe': MimeFormat('application/x-msdownload', [
    [0x4D, 0x5A]
  ]),
  '.ez': MimeFormat('application/andrew-inset', nil),
  '.flac': MimeFormat('audio/flac', [[0x66, 0x4C, 0x61, 0x43]]),
  '.flv': MimeFormat('video/x-flv', [
    [0x46, 0x4C, 0x56, 0x01]
  ]),
  '.gif': MimeFormat('image/gif', [
    [0x47, 0x49, 0x46, 0x38, 0x37, 0x61],
    [0x47, 0x49, 0x46, 0x38, 0x39, 0x61]
  ]),
  '.gz': MimeFormat('application/gzip', [
    [0x1F, 0x8B, 0x08]
  ]),
  '.gram': MimeFormat('application/srgs', nil),
  '.grxml': MimeFormat('application/srgs+xml', nil),
  '.gtar': MimeFormat('application/x-gtar', nil),
  '.hdf': MimeFormat('application/x-hdf', nil),
  '.hqx': MimeFormat('application/mac-binhex40', nil),
  '.htm': MimeFormat('text/html', nil),
  '.html': MimeFormat('text/html', nil),
  '.ice': MimeFormat('x-conference/x-cooltalk', nil),
  '.ico': MimeFormat('image/vnd.microsoft.icon', [
    [0x00, 0x00, 0x01, 0x00]
  ]),
  '.ics': MimeFormat('text/calendar', nil),
  '.ief': MimeFormat('image/ief', nil),
  '.ifb': MimeFormat('text/calendar', nil),
  '.iges': MimeFormat('model/iges', nil),
  '.igs': MimeFormat('model/iges', nil),
  '.in': MimeFormat('text/plain', nil),
  '.ini': MimeFormat('text/plain', nil),
  '.iso': MimeFormat('application/x-iso9660-image', [[0x43, 0x44, 0x30, 0x30, 0x31]]),
  '.jar': MimeFormat('application/java-archive', [
    [0x50, 0x4B, 0x03, 0x04],
    [0x5F, 0x27, 0xA8, 0x89],
    [0x4A, 0x41, 0x52, 0x43, 0x53, 0x00],
    [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x08, 0x00]
  ]),
  '.jnlp': MimeFormat('application/x-java-jnlp-file', nil),
  '.jp2': MimeFormat('image/jp2', [
    [0x00, 0x00, 0x00, 0x0C, 0x6A, 0x50, 0x20, 0x20]
  ]),
  '.jpe': MimeFormat('image/jpeg', [
    [0xFF, 0xD8, 0xFF, 0xE0]
  ]),
  '.jpeg': MimeFormat('image/jpeg', [
    [0xFF, 0xD8, 0xFF, 0xE0], 
    [0xFF, 0xD8, 0xFF, 0xE2], 
    [0xFF, 0xD8, 0xFF, 0xE3]
  ]),
  '.jpg': MimeFormat('image/jpeg', [
    [0xFF, 0xD8, 0xFF, 0xE0], 
    [0xFF, 0xD8, 0xFF, 0xE1], 
    [0xFF, 0xD8, 0xFF, 0xE8]
  ]),
  '.jpgv': MimeFormat('video/jpeg', nil),
  '.js': MimeFormat('application/x-javascript', nil),
  '.json': MimeFormat('application/json', nil),
  '.jsonld': MimeFormat('application/ld+json', nil),
  '.kar': MimeFormat('audio/midi', nil),
  '.latex': MimeFormat('application/x-latex', nil),
  '.lha': MimeFormat('application/octet-stream', nil),
  '.lib': MimeFormat('application/octet-stream', [[0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E, 0x0A]]),
  '.list': MimeFormat('text/plain', nil),
  '.log': MimeFormat('text/plain', nil),
  '.lzh': MimeFormat('application/octet-stream', nil),
  '.m3u': MimeFormat('audio/x-mpegurl', nil),
  '.m4a': MimeFormat('audio/mp4a-latm', nil),
  '.m4b': MimeFormat('audio/mp4a-latm', nil),
  '.m4p': MimeFormat('audio/mp4a-latm', nil),
  '.m4u': MimeFormat('video/vnd.mpegurl', nil),
  '.m4v': MimeFormat('video/x-m4v', nil),
  '.mac': MimeFormat('image/x-macpaint', nil),
  '.man': MimeFormat('application/x-troff-man', nil),
  '.mathml': MimeFormat('application/mathml+xml', nil),
  '.me': MimeFormat('application/x-troff-me', nil),
  '.mesh': MimeFormat('model/mesh', nil),
  '.mid': MimeFormat('audio/midi', [[0x4D, 0x54, 0x68, 0x64]]),
  '.midi': MimeFormat('audio/midi', [[0x4D, 0x54, 0x68, 0x64]]),
  '.mif': MimeFormat('application/vnd.mif', [
    [0x3C, 0x4D, 0x61, 0x6B, 0x65, 0x72, 0x46, 0x69], # Adobe FrameMaker
    [0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x20] # MapInfo Interchange Format file
  ]),
  '.mjs': MimeFormat('text/javascript', nil),
  '.mka': MimeFormat('audio/x-matroska', [[0x1A, 0x45, 0xDF, 0xA3]]),
  '.mkv': MimeFormat('video/x-matroska', [[0x1A, 0x45, 0xDF, 0xA3, 0x93, 0x42, 0x82, 0x88]]),
  '.mov': MimeFormat('video/quicktime', [
    [0x6D, 0x6F, 0x6F, 0x76],
    [0x66, 0x72, 0x65, 0x65],
    [0x6D, 0x64, 0x61, 0x74],
    [0x77, 0x69, 0x64, 0x65],
    [0x70, 0x6E, 0x6F, 0x74],
    [0x73, 0x6B, 0x69, 0x70]
  ]),
  '.movie': MimeFormat('video/x-sgi-movie', nil),
  '.mp2': MimeFormat('audio/mpeg', nil),
  '.mp3': MimeFormat('audio/mpeg', [
    [0x49, 0x44, 0x33],
    [0xFF, 0xFB],
    [0xFF, 0xF3],
    [0xFF, 0xF2] # last three are for files without id3 or with id3v1 tag
  ]),
  '.mp4': MimeFormat('video/mp4', [[0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D]]),
  '.mpe': MimeFormat('video/mpeg', nil),
  '.mpeg': MimeFormat('video/mpeg', nil),
  '.mpg': MimeFormat('video/mpeg', [
    [0x00, 0x00, 0x01, 0xBA], # DVD video
    [0x00, 0x00, 0x01, 0xB3] # MPEG video
  ]),
  '.mpga': MimeFormat('audio/mpeg', nil),
  '.mpkg': MimeFormat('application/vnd.apple.installer+xml', nil),
  '.ms': MimeFormat('application/x-troff-ms', nil),
  '.msh': MimeFormat('model/mesh', nil),
  '.msi': MimeFormat('application/x-msdownload', nil),
  '.mxu': MimeFormat('video/vnd.mpegurl', nil),
  '.nc': MimeFormat('application/x-netcdf', nil),
  '.oda': MimeFormat('application/oda', nil),
  '.odp': MimeFormat('application/vnd.oasis.opendocument.presentation', [[0x50, 0x4B, 0x03, 0x04]]),
  '.ods': MimeFormat('application/vnd.oasis.opendocument.spreadsheet', nil),
  '.odt': MimeFormat('application/vnd.oasis.opendocument.text', [[0x50, 0x4B, 0x03, 0x04]]),
  '.ogg': MimeFormat('video/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
  '.ogx': MimeFormat('application/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
  '.oga': MimeFormat('audio/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
  '.ogv': MimeFormat('video/ogg', [[0x4F, 0x67, 0x67, 0x53, 0x00, 0x02, 0x00, 0x00]]),
  '.opus': MimeFormat('audio/opus', nil),
  '.otf': MimeFormat('font/otf', nil),
  '.pbm': MimeFormat('image/x-portable-bitmap', nil),
  '.pct': MimeFormat('image/pict', nil),
  '.pdb': MimeFormat('chemical/x-pdb', nil),
  '.pdf': MimeFormat('application/pdf', [[0x25, 0x50, 0x44, 0x46]]),
  '.pgm': MimeFormat('image/x-portable-graymap', nil),
  '.pgn': MimeFormat('application/x-chess-pgn', nil),
  '.php': MimeFormat('application/x-httpd-php', nil),
  '.pic': MimeFormat('image/pict', nil),
  '.pict': MimeFormat('image/pict', nil),
  '.png': MimeFormat('image/png', [[0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]]), 
  '.pnm': MimeFormat('image/x-portable-anymap', nil),
  '.pnt': MimeFormat('image/x-macpaint', nil),
  '.pntg': MimeFormat('image/x-macpaint', nil),
  '.ppm': MimeFormat('image/x-portable-pixmap', nil),
  '.ppt': MimeFormat('application/vnd.ms-powerpoint', [[0xFD, 0xFF, 0xFF, 0xFF, nil, 0x00, 0x00, 0x00]]),
  '.pptx': MimeFormat('application/vnd.openxmlformats-officedocument.presentationml.presentation', [
    [0x50, 0x4B, 0x03, 0x04],
    [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00]
  ]),
  '.potx': MimeFormat('application/vnd.openxmlformats-officedocument.presentationml.template', nil),
  '.ppsx': MimeFormat('application/vnd.openxmlformats-officedocument.presentationml.slideshow', nil),
  '.ppam': MimeFormat('application/vnd.ms-powerpoint.addin.macroEnabled.12', nil),
  '.pptm': MimeFormat('application/vnd.ms-powerpoint.presentation.macroEnabled.12', nil),
  '.potm': MimeFormat('application/vnd.ms-powerpoint.template.macroEnabled.12', nil),
  '.ppsm': MimeFormat('application/vnd.ms-powerpoint.slideshow.macroEnabled.12', nil),
  '.ps': MimeFormat('application/postscript', nil),
  '.psd': MimeFormat('application/octet-stream', [[0x38, 0x42, 0x50, 0x53]]),
  '.pst': MimeFormat('application/vnd.ms-outlook', [[0x21, 0x42, 0x44, 0x4E]]),
  '.pub': MimeFormat('application/x-mspublisher', [[0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1]]),
  '.qt': MimeFormat('video/quicktime', nil),
  '.qti': MimeFormat('image/x-quicktime', nil),
  '.qtif': MimeFormat('image/x-quicktime', nil),
  '.ra': MimeFormat('audio/x-pn-realaudio', [
    [0x2E, 0x52, 0x4D, 0x46, 0x00, 0x00, 0x00, 0x12],
    [0x2E, 0x72, 0x61, 0xFD, 0x00] # the streaming variant
  ]),
  '.ram': MimeFormat('audio/x-pn-realaudio', [[0x72, 0x74, 0x73, 0x70, 0x3A, 0x2F, 0x2F]]),
  '.rar': MimeFormat('application/vnd.rar', [[0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00]]),
  '.ras': MimeFormat('image/x-cmu-raster', nil),
  '.rdf': MimeFormat('application/rdf+xml', nil),
  '.rgb': MimeFormat('image/x-rgb', [[0x01, 0xDA, 0x01, 0x01, 0x00, 0x03]]),
  '.rm': MimeFormat('application/vnd.rn-realmedia', [[0x2E, 0x52, 0x4D, 0x46]]),
  '.roff': MimeFormat('application/x-troff', nil),
  '.rpm': MimeFormat('application/x-rpm', [[0xED, 0xAB, 0xEE, 0xDB]]),
  '.rtf': MimeFormat('text/rtf', [[0x7B, 0x5C, 0x72, 0x74, 0x66, 0x31]]),
  '.rtx': MimeFormat('text/richtext', nil),
  '.sgm': MimeFormat('text/sgml', nil),
  '.sgml': MimeFormat('text/sgml', nil),
  '.sh': MimeFormat('application/x-sh', nil),
  '.shar': MimeFormat('application/x-shar', nil),
  '.silo': MimeFormat('model/mesh', nil),
  '.sit': MimeFormat('application/x-stuffit', [
    [0x53, 0x49, 0x54, 0x21, 0x00],
    [0x53, 0x74, 0x75, 0x66, 0x66, 0x49, 0x74, 0x20] # the compressed archive
  ]),
  '.skd': MimeFormat('application/x-koan', nil),
  '.skm': MimeFormat('application/x-koan', nil),
  '.skp': MimeFormat('application/x-koan', nil),
  '.skt': MimeFormat('application/x-koan', nil),
  '.smi': MimeFormat('application/smil', nil),
  '.smil': MimeFormat('application/smil', nil),
  '.snd': MimeFormat('audio/basic', nil),
  '.so': MimeFormat('application/octet-stream', nil),
  '.spl': MimeFormat('application/x-futuresplash', [[0x00, 0x00, 0x01, 0x00]]),
  '.sqlite': MimeFormat('application/vnd.sqlite3', [[0x53, 0x51, 0x4C, 0x69, 0x74, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x20, 0x33, 0x00]]),
  '.sqlitedb': MimeFormat('application/vnd.sqlite3', [[0x53, 0x51, 0x4C, 0x69, 0x74, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x20, 0x33, 0x00]]),
  '.src': MimeFormat('application/x-wais-source', nil),
  '.sv4cpio': MimeFormat('application/x-sv4cpio', nil),
  '.sv4crc': MimeFormat('application/x-sv4crc', nil),
  '.svg': MimeFormat('image/svg+xml', nil),
  '.swf': MimeFormat('application/x-shockwave-flash', [[0x46, 0x57, 0x53]]),
  '.t': MimeFormat('application/x-troff', nil),
  '.tar': MimeFormat('application/x-tar', [[0x75, 0x73, 0x74, 0x61, 0x72]]),
  '.tar.z': MimeFormat('application/x-tar', [
    [0x1F, 0x9D], # Compressed using Lempel-Ziv-Welch
    [0x1F, 0xA0] # Compressed using LZH
  ]),
  '.tcl': MimeFormat('application/x-tcl', nil),
  '.tex': MimeFormat('application/x-tex', nil),
  '.texi': MimeFormat('application/x-texinfo', nil),
  '.texinfo': MimeFormat('application/x-texinfo', nil),
  '.tgz': MimeFormat('application/x-gz', [[0x1F, 0x8B, 0x08]]),
  '.tif': MimeFormat('image/tiff', [
    [0x49, 0x20, 0x49],
    [0x49, 0x49, 0x2A, 0x00],
    [0x4D, 0x4D, 0x00, 0x2A],
    [0x4D, 0x4D, 0x00, 0x2B]
  ]),
  '.tiff': MimeFormat('image/tiff', [
    [0x49, 0x20, 0x49],
    [0x49, 0x49, 0x2A, 0x00],
    [0x4D, 0x4D, 0x00, 0x2A],
    [0x4D, 0x4D, 0x00, 0x2B]
  ]),
  '.torrent': MimeFormat('application/x-bittorrent', [[0x64, 0x38, 0x3A, 0x61, 0x6E, 0x6E, 0x6F, 0x75, 0x6E, 0x63, 0x65]]),
  '.tr': MimeFormat('application/x-troff', nil),
  '.ts': MimeFormat('video/mp2t', nil),
  '.tsv': MimeFormat('text/tab-separated-values', nil),
  '.ttf': MimeFormat('application/x-font-ttf', [[0x00, 0x01, 0x00, 0x00, 0x00]]),
  '.txt': MimeFormat('text/plain', nil),
  '.text': MimeFormat('text/plain', nil),
  '.ustar': MimeFormat('application/x-ustar', nil),
  '.vcd': MimeFormat('application/x-cdlink', nil),
  '.vcs': MimeFormat('text/x-vcard', nil),
  '.vcard': MimeFormat('text/vcard', nil),
  '.vrml': MimeFormat('model/vrml', nil),
  '.vsd': MimeFormat('application/vnd.visio', nil),
  '.vxml': MimeFormat('application/voicexml+xml', nil),
  '.wav': MimeFormat('audio/x-wav', [[0x52, 0x49, 0x46, 0x46]]),
  '.wbmp': MimeFormat('image/vnd.wap.wbmp', nil),
  '.wbmxl': MimeFormat('application/vnd.wap.wbxml', nil),
  '.weba': MimeFormat('audio/webm', nil),
  '.webm': MimeFormat('video/webm', [[0x1A, 0x45, 0xDF, 0xA3, 0x93, 0x42, 0x82, 0x88]]),
  '.webmanifest': MimeFormat('application/manifest+json', nil),
  '.webp': MimeFormat('image/webp', [[0x52, 0x49, 0x46, 0x46, nil, nil, nil, nil, 0x57, 0x45, 0x42, 0x50]]),
  '.woff': MimeFormat('font/woff', nil),
  '.woff2': MimeFormat('font/woff2', nil),
  '.wml': MimeFormat('text/vnd.wap.wml', nil),
  '.wmlc': MimeFormat('application/vnd.wap.wmlc', nil),
  '.wmls': MimeFormat('text/vnd.wap.wmlscript', nil),
  '.wmlsc': MimeFormat('application/vnd.wap.wmlscriptc', nil),
  '.wmv': MimeFormat('video/x-ms-wmv', [[0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11]]),
  '.wma': MimeFormat('audio/x-ms-wmv', [[0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11]]),
  '.wrl': MimeFormat('model/vrml', nil),
  '.xbm': MimeFormat('image/x-xbitmap', nil),
  '.xht': MimeFormat('application/xhtml+xml', nil),
  '.xhtml': MimeFormat('application/xhtml+xml', nil),
  '.xls': MimeFormat('application/vnd.ms-excel', [[0x09, 0x08, 0x10, 0x00, 0x00, 0x06, 0x05, 0x00]]),                        
  '.xml': MimeFormat('application/xml', nil),
  '.xpm': MimeFormat('image/x-xpixmap', nil),
  '.xsl': MimeFormat('application/xml', nil),
  '.xlsx': MimeFormat('application/vnd.openxmlformats-officedocument.spreadsheetml.sheet', [
    [0x50, 0x4B, 0x03, 0x04], # MS Office Open XML Format Document
    [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00] # MS Office 2007 documents
  ]),
  '.xltx': MimeFormat('application/vnd.openxmlformats-officedocument.spreadsheetml.template', nil),
  '.xlsm': MimeFormat('application/vnd.ms-excel.sheet.macroEnabled.12', nil),
  '.xltm': MimeFormat('application/vnd.ms-excel.template.macroEnabled.12', nil),
  '.xlam': MimeFormat('application/vnd.ms-excel.addin.macroEnabled.12', nil),
  '.xlsb': MimeFormat('application/vnd.ms-excel.sheet.binary.macroEnabled.12', nil),
  '.xslt': MimeFormat('application/xslt+xml', nil),
  '.xul': MimeFormat('application/vnd.mozilla.xul+xml', nil),
  '.xwd': MimeFormat('image/x-xwindowdump', nil),
  '.xyz': MimeFormat('chemical/x-xyz', nil),
  '.zip': MimeFormat('application/zip', [
    [0x50, 0x4B, 0x03, 0x04],
    [0x50, 0x4B, 0x4C, 0x49, 0x54, 0x45], # light archive
    [0x50, 0x4B, 0x53, 0x70, 0x58], # self-extracting
    [0x50, 0x4B, 0x05, 0x06],
    [0x50, 0x4B, 0x07, 0x08],
    [0x57, 0x69, 0x6E, 0x5A, 0x69, 0x70], # WinZip compressed archive
    [0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x01, 0x00] # ZLock Pro encrypted archive
  ])
}


/**
 * detect_from_name(name: string)
 * Detects the mimetype of a file based on the
 * extension defined in it's path.
 *
 * @return string
 * @note For popular files such as Jpeg and Pngs, calling this method directly is more efficient and provides a faster lookup.
 * 
 * Example,
 * 
 * ```blade
 * import mime
 * echo mime.detect_from_name('myimage.png')
 * ```
 */
def detect_from_name(name) {
  if !is_string(name)
    die Exception('name must be string')

  for ext, mime in _mimes {
    if name.ends_with(ext) return mime.mimetype
  }
  return 'application/octet-stream'
}

/**
 * detect_from_header(file: file) 
 * Detects the mimetype of a file based on it's file header.
 *
 * When multiple file formats share very similar or shadowing
 * file headers (such as the relationship between Zip files and Docx files),
 * this method will perform an extension before returning it's result.
 *
 * @return string
 * @note For dealing with files without extension, or where the accuracy of the file extension cannot be trusted, this method provides a more efficient lookup.
 * @note This method may produce slightly more rigorous results
 * @note This method requires that the file must be opened in binary mode
 * 
 * Example,
 * 
 * ```blade
 * import mime
 * var f = file('my_file.ext', 'rb')
 * echo mime.detect_from_header(f)
 * ```
 */
def detect_from_header(file) {
  if !is_file(file)
    die Exception('file object expected')

  if !file.mode().match('b') {
    die Exception('detect_from_header expects file to be opened in binary mode')
  }

  var top_16 = file.read(16)
  var last_mime_found = nil

  for ext, mime in _mimes {
    if mime.header {
      iter var i = 0; i < mime.header.length(); i++ {
        var mime_match = true
        var header = mime.header[i]

        iter var j = 0; j < header.length(); j++ {
          # we use nil value to skip extra data in header signatures
          # such as webp filesize part...
          # ie. RIFF ... WEBP where ... is represented as nil
          if header[j] != top_16[j] and header[j] != nil {
            mime_match = false
            break
          }
        }

        if mime_match {
          if file.name().ends_with(ext) return mime.mimetype
          else last_mime_found = mime.mimetype
        }
      }
    }
  }

  if last_mime_found return last_mime_found
  return 'application/octet-stream'
}

/**
 * detect(file: file)
 * Performs mimetype detection on a file.
 * 
 * this method is capable of detecting file mimetypes even
 * in the abscence of an extension.
 *
 * If the file is opened in binary mode, it first attempt the more
 * accurate header check. If the header check returns a generic result 
 * (i.e. application/octet-stream), it performs an extension lookup.
 *
 * @return string
 * @note this method gives the best result, but slightly slower than a direct lookup of name or header.
 * 
 * Example,
 * 
 * ```blade
 * import mime
 * var f = file('myfile', 'rb')
 * 
 * # using 'rb' here for two reasons: 
 * # 1. Our file has no extension, so extension based detection is impossible
 * # 2. We want more accuracy by having Mime check file headers
 * 
 * echo mime.detect(f)
 * ```
 */
def detect(file) {
  if !is_file(file)
    die Exception('file object expected')

  if file.mode().match('b') {
    var mime = detect_from_header(file)
    if mime == 'application/octet-stream' {
      return detect_from_name(file.name())
    }
    return mime
  }
  return detect_from_name(file.name())
}

/**
 * extend(extension: string, format: MimeFormat)
 * 
 * Extends the mime module with support for files with the given _extension_ as 
 * defined in the given _format_.
 * 
 * @return bool
 * @note the extension MUST start with `.`
 * 
 * Example,
 * 
 * ```blade-repl
 * %> import mime
 * %> mime.detect_from_name('myfile.ppk')
 * 'application/octet-stream'
 * %> mime.extend('.ppk', mime.MimeFormat('file/ppk'))
 * true
 * %> mime.detect_from_name('myfile.ppk')
 * 'file/ppk'
 * ```
 */
def extend(extension, format) {
  if !is_string(extension) or extension[0] != '.'
    die Exception('invalid file extension')
  if !instance_of(format, MimeFormat)
    die Exception('instance of MimeFormat expected')
  
  extension = extension.lower()
  
  if _mimes.contains(extension) return false
  _mimes.add(extension, format)
  return true
}

