#
# @module os
# 
# provides functionalities for interfacing with the underlying operating system
# @ copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _os { * }

/**
 * stores the name of the current platform in string
 */
var platform = _platform

/**
 * stores the command line arguments passed to the startup script
 */
var args = _args

/**
 * the standard path separator for the current os
 */
var path_separator = _path_separator


/**
 * exec(cmd: string)
 *
 * exec executes the given shell commands
 * @return string
 */
def exec(cmd) {
  return _exec(cmd)
}

/**
 * info()
 *
 * returns information about the current os
 * @return dict
 */
def info() {
  return _info()
}

/**
 * sleep(duration: number)
 *
 * causes the current thread to sleep for the specified number of seconds
 * @return nil
 */
def sleep(duration) {
  return _sleep(duration)
}

/**
 * get_env(name: string)
 *
 * returns the given environment variable if exists or nil otherwise
 * @return string
 */
def get_env(name) {
  return _getenv(name)
}

/**
 * set_env(name: string, value: string, overwrite: bool = true)
 *
 * sets the named environment variable to the given value.
 *
 * on Unix, if overwrite is false, it doesn't set a value if the variable
 * exists before.
 * @return string
 */
def set_env(name, value, overwrite) {
  return _setenv(name, value, overwrite)
}

/**
 * File types
 */
var DT_UNKNOWN = _DT_UNKNOWN
var DT_BLK = _DT_BLK
var DT_CHR = _DT_CHR
var DT_DIR = _DT_DIR
var DT_FIFO = _DT_FIFO
var DT_LNK = _DT_LNK
var DT_REG = _DT_REG
var DT_SOCK = _DT_SOCK
var DT_WHT = _DT_WHT

/**
 * create_dir(path: string, [permission: number = 0c777 [, recursive: boolean = true]])
 * 
 * creates the given directory with the specified permission and optionaly 
 * add new files into it if any is given.
 * 
 * @note if the directory already exists, it returns `false` otherwise, it returns `true`.
 * @note permission should be given as octal number.
 * @return boolean
 */
def create_dir(path, permission, recursive) {

  if path {
    if !is_string(path) die Exception('path must be string')
    path = path
  }
  if !path.ends_with(path_separator)
    path += path_separator

  if platform == 'windows' {
    path = path.replace('@\/@', '\\\\')
  }

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

  return _createdir(path, permission, recursive)
}

/**
 * read_dir(path: string)
 * 
 * scans the given directory returns a list of all matched files
 * @return DirectoryEntry[]
 */
def read_dir(path) {
  return _readdir(path)
}

/**
 * chmod(path: string, mod: number)
 * 
 * changes the permission set on a directory
 * @note mod should be octal number
 * @return boolean
 */
def chmod(path, mod) {
  return _chmod(path, mod)
}

/**
 * is_dir(path: string)
 * 
 * returns `true` if the path is a directory or `false` otherwise.
 * @return bool
 */
def is_dir(path) {
  return _isdir(path)
}

/**
 * remove_dir(path: string [, recursive: boolean = false])
 * 
 * deletes a non-empty directory
 * @return bool
 */
def remove_dir(path, recursive) {
  if recursive != nil {
    if !is_bool(recursive)
      die Exception('boolean expected in argument 2')
  } else {
    recursive = false
  }
  return _removedir(path, recursive)
}

/**
 * cwd()
 * 
 * returns the current working directory
 * @return string
 */
def cwd() {
  return _cwd()
}

/**
 * change_dir(path: string)
 * 
 * navigates the working directory into the specified path
 * @return bool
 */
def change_dir(path) {
  return _chdir(path)
}

/**
 * exists(path: string)
 * 
 * returns `true` if the directory exists or `false` otherwise.
 * @return bool
 */
def dir_exists(path) {
  return _exists(path)
}

/**
 * exit(code: number)
 * 
 * exit the current process and quits the Blade runtime.
 * @return
 */
def exit(code) {
  _exit(code)
}

/**
 * join_paths(paths...)
 * 
 * concatenates the given paths together into a format
 * qualified on the current os
 */
def join_paths(...) {
  # @TODO: remove invalid path characters before return
  var result = ''
  for arg in __args__ {
    if !is_string(arg)
      die Exception('string expected, ${typeof(arg)} given')
    
    result += '${path_separator}${arg}'
  }
  
  if result result = result[1,]
  return result
}

/**
 * real_path(path: string)
 * 
 * returns the original path to a relative path
 */
def real_path(path) {
  return _realpath(path)
}
