#!-- part of the os module

import _os {
  # constants
  DT_UNKNOWN,
  DT_BLK,
  DT_CHR,
  DT_DIR,
  DT_FIFO,
  DT_LNK,
  DT_REG,
  DT_SOCK,
  DT_WHT,
  platform,

  # functions
  _mkdir, 
  _readdir,
  _chmod
}

/**
 * @class DirectoryEntry
 * 
 * represent entries in a directory.
 * @note this class does not have separate objects types for 
 * files and subdirectories but rather indicates the type of 
 * the entry which may be one of the types listed below.
 * 
 * DT_UNKNOWN, DT_BLK, DT_CHR, DT_DIR, DT_FIFO, DT_LNK, DT_REG, 
 * DT_SOCK, DT_WHT.
 */
class DirectoryEntry {

  /**
   * @constructor DirectoryEntry
   * DirectoryEntry(path: string, type: number)
   */
  DirectoryEntry(path, type) {
    if !is_string(path) 
      die Exception('entry name expected as first argument')
    if !is_number(type) 
      die Exception('entry type expected as second argument')

    self.path = path
    self.type = type
  }
}

/**
 * @class Directory
 * 
 * represents an os directory/folder object.
 */
class Directory {

  /**
   * list of file in the directory
   */
  var files = []

  /**
   * the path of this directory
   * [default = '.'] (i.e. current directory)
   */
  var path = '.'

  /**
   * @constructor Dicrectory
   * Directory([path: string])
   * 
   * if _path_ is not given, path defaults to `.` representing the 
   * current directory. 
   */
  Directory(path) {
    if path {
      if !is_string(path) die Exception('path must be string')
      self.path = path
    }
    if !self.path.ends_with(path_separator)
      self.path += path_separator

    if platform == 'windows' {
      self.path = self.path.replace('/', '\\')
    }
  }

  /**
   * create([permission: number = 0c777 [, recursive: boolean = false [, files: list = []]]])
   * 
   * creates the given directory with the specified permission and optionaly 
   * add new files into it if any is given.
   * 
   * @note if the directory already exists, it returns `false` otherwise, it returns `true`.
   * @note if files, it will create the files in the current directory files list. 
   * @note if the directory exists and some or all of the files in the list do not exist, 
   * they will be created.
   * @note permission should be given as octal number.
   * @return boolean
   */
  create(permission, recursive, files) {

    if permission {
      if !is_number(permission)
        die Exception('expected number in first argument, ${typeof(permission)} given')
    } else {
      permission = 0c777
    }

    if recursive != nil {
      if !is_bool(recursive) 
        die Exception('boolean expected in second argument, ${typeof(recursive)} given')
    } else {
      recursive = true
    }

    var result = _mkdir(self.path, permission, recursive)

    if files {
      if !is_list(files) {
        die Exception('list of files expected in third argument, ${typeof(files)} given')
      }
      
      for f in files {
        if !is_file(f) die Exception('could not create non-file')
        f.open()
        f.close()
      }
    }

    return result
  }

  /**
   * scan()
   * 
   * scans the given directory and updates its list of DirectoryEntry
   * @return DirectoryEntry[]
   */
  read() {
    var entries = _readdir(self.path)

    # generate list of directory entries
    var list = []
    for entry in entries  {
      list.append(DirectoryEntry(entry[0], entry[1]))
    }

    return list
  }

  /**
   * chmod(mod: number)
   * 
   * changes the permission set on a directory
   * @note mod should be octal number
   * @return boolean
   */
  chmod(mod) {
    if !is_number(mod) 
      die Exception('number expected, ${typeof(mod)} given')

    return _chmod(self.path, mod)
  }
}

