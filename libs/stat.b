/**
 * @module stat
 * 
 * The module provides constants and functions for interpreting results of file.stat().
 * 
 * @copyright 2022, Richard Ore and Blade contributors
 */

# Indices for stat struct members in the tuple returned by file.stat()

var ST_MODE  = 0
var ST_INO   = 1
var ST_DEV   = 2
var ST_NLINK = 3
var ST_UID   = 4
var ST_GID   = 5
var ST_SIZE  = 6
var ST_ATIME = 7
var ST_MTIME = 8
var ST_CTIME = 9

# Extract bits from the mode

/**
 * Return the portion of the file's mode that can be set by file.chmod().
 * 
 * @param number mode
 * @returns number
 */
def S_IMODE(mode) {
  return mode & 0c7777
}

/**
 * Return the portion of the file's mode that describes the file type.
 * 
 * @param number mode
 * @returns number
 */
def S_IFMT(mode) {
  return mode & 0c170000
}

# Constants used as S_IFMT() for various file types
# (not all are implemented on all systems)

var S_IFDIR  = 0c040000  # directory
var S_IFCHR  = 0c020000  # character device
var S_IFBLK  = 0c060000  # block device
var S_IFREG  = 0c100000  # regular file
var S_IFIFO  = 0c010000  # fifo (named pipe)
var S_IFLNK  = 0c120000  # symbolic link
var S_IFSOCK = 0c140000  # socket file
# Fallbacks for uncommon platform-specific constants
var S_IFDOOR = 0
var S_IFPORT = 0
var S_IFWHT = 0

# Functions to test for each file type

/**
 * Return `true` if mode is from a directory.
 * 
 * @param number mode
 * @returns number
 */
def S_ISDIR(mode) {
  return S_IFMT(mode) == S_IFDIR
}

/**
 * Return `true` if mode is from a character special device file.
 * 
 * @param number mode
 * @returns number
 */
def S_ISCHR(mode) {
  return S_IFMT(mode) == S_IFCHR
}

/**
 * Return `true` if mode is from a block special device file.
 * 
 * @param number mode
 * @returns number
 */
def S_ISBLK(mode) {
  return S_IFMT(mode) == S_IFBLK
}

/**
 * Return `true` if mode is from a regular file.
 * 
 * @param number mode
 * @returns number
 */
def S_ISREG(mode) {
  return S_IFMT(mode) == S_IFREG
}

/**
 * Return `true` if mode is from a FIFO (named pipe).
 * 
 * @param number mode
 * @returns number
 */
def S_ISFIFO(mode) {
  return S_IFMT(mode) == S_IFIFO
}

/**
 * Return `true` if mode is from a symbolic link.
 * 
 * @param number mode
 * @returns number
 */
def S_ISLNK(mode) {
  return S_IFMT(mode) == S_IFLNK
}

/**
 * Return `true` if mode is from a socket.
 * 
 * @param number mode
 * @returns number
 */
def S_ISSOCK(mode) {
  return S_IFMT(mode) == S_IFSOCK
}

/**
 * Return `true` if mode is from a door.
 * 
 * @param number mode
 * @returns number
 */
def S_ISDOOR(mode) {
  return false
}

/**
 * Return `true` if mode is from an event port.
 * 
 * @param number mode
 * @returns number
 */
def S_ISPORT(mode) {
  return false
}

/**
 * Return `true` if mode is from a whiteout.
 * 
 * @param number mode
 * @returns number
 */
def S_ISWHT(mode) {
  return false
}

# Names for permission bits

var S_ISUID = 0c4000  # set UID bit
var S_ISGID = 0c2000  # set GID bit
var S_ENFMT = S_ISGID # file locking enforcement
var S_ISVTX = 0c1000  # sticky bit
var S_IREAD = 0c0400  # Unix V7 synonym for S_IRUSR
var S_IWRITE = 0c0200 # Unix V7 synonym for S_IWUSR
var S_IEXEC = 0c0100  # Unix V7 synonym for S_IXUSR
var S_IRWXU = 0c0700  # mask for owner permissions
var S_IRUSR = 0c0400  # read by owner
var S_IWUSR = 0c0200  # write by owner
var S_IXUSR = 0c0100  # execute by owner
var S_IRWXG = 0c0070  # mask for group permissions
var S_IRGRP = 0c0040  # read by group
var S_IWGRP = 0c0020  # write by group
var S_IXGRP = 0c0010  # execute by group
var S_IRWXO = 0c0007  # mask for others (not in group) permissions
var S_IROTH = 0c0004  # read by others
var S_IWOTH = 0c0002  # write by others
var S_IXOTH = 0c0001  # execute by others

# Names for file flags

var UF_NODUMP    = 0x00000001  # do not dump file
var UF_IMMUTABLE = 0x00000002  # file may not be changed
var UF_APPEND    = 0x00000004  # file may only be appended to
var UF_OPAQUE    = 0x00000008  # directory is opaque when viewed through a union stack
var UF_NOUNLINK  = 0x00000010  # file may not be renamed or deleted
var UF_COMPRESSED = 0x00000020 # OS X: file is hfs-compressed
var UF_HIDDEN    = 0x00008000  # OS X: file should not be displayed
var SF_ARCHIVED  = 0x00010000  # file may be archived
var SF_IMMUTABLE = 0x00020000  # file may not be changed
var SF_APPEND    = 0x00040000  # file may only be appended to
var SF_NOUNLINK  = 0x00100000  # file may not be renamed or deleted
var SF_SNAPSHOT  = 0x00200000  # file is a snapshot file


var _filemode_table = [
    [[S_IFLNK,         "l"],
     [S_IFSOCK,        "s"],  # Must appear before IFREG and IFDIR as IFSOCK == IFREG | IFDIR
     [S_IFREG,         "-"],
     [S_IFBLK,         "b"],
     [S_IFDIR,         "d"],
     [S_IFCHR,         "c"],
     [S_IFIFO,         "p"]],

    [[S_IRUSR,         "r"],],
    [[S_IWUSR,         "w"],],
    [[S_IXUSR|S_ISUID, "s"],
     [S_ISUID,         "S"],
     [S_IXUSR,         "x"]],

    [[S_IRGRP,         "r"],],
    [[S_IWGRP,         "w"],],
    [[S_IXGRP|S_ISGID, "s"],
     [S_ISGID,         "S"],
     [S_IXGRP,         "x"]],

    [[S_IROTH,         "r"],],
    [[S_IWOTH,         "w"],],
    [[S_IXOTH|S_ISVTX, "t"],
     [S_ISVTX,         "T"],
     [S_IXOTH,         "x"]]
]

/**
 * Convert a file's mode to a string of the form '-rwxrwxrwx'.
 * 
 * @param number mode
 * @returns string
 */
def file_mode(mode) {
  var perm = []
  for table in _filemode_table {
    var flag_found = false

    for pair in table {
      var bit = pair.first(), char = pair.last()
      
      if mode & bit == bit {
        perm.append(char)
        flag_found = true
        break
      }
    }

    if !flag_found {
      perm.append('-')
    }
  }

  return ''.join(perm)
}


# Windows FILE_ATTRIBUTE constants for interpreting file.stat()'s
# "st_file_attributes" member

var FILE_ATTRIBUTE_ARCHIVE = 32
var FILE_ATTRIBUTE_COMPRESSED = 2048
var FILE_ATTRIBUTE_DEVICE = 64
var FILE_ATTRIBUTE_DIRECTORY = 16
var FILE_ATTRIBUTE_ENCRYPTED = 16384
var FILE_ATTRIBUTE_HIDDEN = 2
var FILE_ATTRIBUTE_INTEGRITY_STREAM = 32768
var FILE_ATTRIBUTE_NORMAL = 128
var FILE_ATTRIBUTE_NOT_CONTENT_INDEXED = 8192
var FILE_ATTRIBUTE_NO_SCRUB_DATA = 131072
var FILE_ATTRIBUTE_OFFLINE = 4096
var FILE_ATTRIBUTE_READONLY = 1
var FILE_ATTRIBUTE_REPARSE_POINT = 1024
var FILE_ATTRIBUTE_SPARSE_FILE = 512
var FILE_ATTRIBUTE_SYSTEM = 4
var FILE_ATTRIBUTE_TEMPORARY = 256
var FILE_ATTRIBUTE_VIRTUAL = 65536
