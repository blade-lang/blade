#
# @module os
# 
# This module provides functions for interfacing with the underlying operating system and directories.
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _os { * }

/**
 * The name of the current platform in string or `unknown` if 
 * the platform name could not be determined.
 * 
 * Example,
 * 
 * ```blade-repl
 * %> import os
 * %> os.platform
 * 'osx'
 * ```
 */
var platform = _platform

/**
 * A list containing the command line arguments passed to the startup script.
 */
var args = _args

/**
 * The standard path separator for the current operating system.
 */
var path_separator = _path_separator


/**
 * exec(cmd: string)
 *
 * Executes the given shell (or command prompt for Windows) commands and 
 * returns the output as string.
 * 
 * @return string
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.exec('ls -l')
 * 'total 48
 * -rw-r--r--@ 1 username  staff  705 Aug 27  2021 buggy.b
 * -rw-r--r--  1 username  staff  197 Mar  5 05:13 myprogram.b'
 * ```
 */
def exec(cmd) {
  return _exec(cmd)
}

/**
 * info()
 *
 * Returns information about the current operation system and machine as a dictionary.
 * The returned dictionary will contain:
 * 
 * - `sysname`: The name of the operating system
 * - `nodename` The name of the current machine
 * - `version`: The operating system version
 * - `release`: The release level/version
 * - `machine`: The hardware/processor type.
 * 
 * @return dict
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.info()
 * {sysname: Darwin, nodename: MacBook-Pro.local, version: Darwin Kernel Version 
 * 21.1.0: Wed Oct 13 17:33:24 PDT 2021; root:xnu-8019.41.5~1/RELEASE_ARM64_T8101, 
 * release: 21.1.0, machine: arm64}
 * ```
 */
def info() {
  return _info()
}

/**
 * sleep(duration: number)
 *
 * Causes the current thread to sleep for the specified number of seconds.
 */
def sleep(duration) {
  _sleep(duration)
}

/**
 * get_env(name: string)
 *
 * Returns the given environment variable if exists or nil otherwise
 * @return string
 * 
 * Example,
 * 
 * ```blade-repl
 * %> import os
 * %> os.get_env('ENV1')
 * '20'
 * ```
 */
def get_env(name) {
  return _getenv(name)
}

/**
 * set_env(name: string, value: string, overwrite: bool = true)
 *
 * Sets the named environment variable to the given value.
 * @return string
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.set_env('ENV1', 'New value')
 * true
 * %> os.get_env('ENV1')
 * 'New value'
 * ```
 * 
 * If you are in the REPL and have tried the last example in `get_env()`, 
 * you may notice that the value of `ENV1` doesn't change. This is because 
 * unless you specify, `set_env()` will not overwrite existing environment variables. 
 * For that, you will need to specify `true` as the third parameter to `set_env()`.
 * 
 * For example,
 * 
 * ```blade-repl
 * %> os.set_env('ENV1', 'New value again', true)
 * true
 * %> os.get_env('ENV1')
 * 'New value again'
 * ```
 * 
 * @note Environment variables set will not persist after application exists.
 */
def set_env(name, value, overwrite) {
  if overwrite == nil overwrite = false
  return _setenv(name, value, overwrite)
}

# File types
var DT_UNKNOWN = _DT_UNKNOWN  # unknown
var DT_BLK = _DT_BLK  # block device
var DT_CHR = _DT_CHR  # character device
var DT_DIR = _DT_DIR  # directory
var DT_FIFO = _DT_FIFO  # named pipe
var DT_LNK = _DT_LNK  # symbolic link
var DT_REG = _DT_REG  # regular file
var DT_SOCK = _DT_SOCK  # local-domain socket
var DT_WHT = _DT_WHT  

/**
 * create_dir(path: string, [permission: number = 0c777 [, recursive: boolean = true]])
 * 
 * Creates the given directory with the specified permission and optionaly 
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
    path = path.replace('@\/@', '\\')
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
 * Scans the given directory and returns a list of all matched files
 * @return list[string]
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.read_dir('./tests')
 * [., .., myprogram.b, single_thread.b, test.b, buggy.b]
 * ```
 * 
 * @note `.` indicates current directory and can be used as argument to _path_ as well.
 * @note `..` indicates parent directory and can be used as argument to _path_ as well.
 */
def read_dir(path) {
  return _readdir(path)
}

/**
 * chmod(path: string, mod: number)
 * 
 * Changes the permission set on a directory
 * 
 * @note mod should be octal number (e.g. 0c777)
 * @return boolean
 */
def chmod(path, mod) {
  return _chmod(path, mod)
}

/**
 * is_dir(path: string)
 * 
 * Returns `true` if the path is a directory or `false` otherwise.
 * @return bool
 */
def is_dir(path) {
  return _isdir(path)
}

/**
 * remove_dir(path: string [, recursive: boolean = false])
 * 
 * Deletes a non-empty directory. If recursive is `true`, non-empty directories 
 * will have their contents deleted first.
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
 * Returns the current working directory
 * @return string
 */
def cwd() {
  return _cwd()
}

/**
 * change_dir(path: string)
 * 
 * Navigates the working directory into the specified path.
 * @return bool
 */
def change_dir(path) {
  return _chdir(path)
}

/**
 * exists(path: string)
 * 
 * Returns `true` if the directory exists or `false` otherwise.
 * @return bool
 */
def dir_exists(path) {
  return _exists(path)
}

/**
 * exit(code: number)
 * 
 * Exit the current process and quits the Blade runtime.
 * @return
 */
def exit(code) {
  _exit(code)
}

/**
 * join_paths(paths...)
 * 
 * Concatenates the given paths together into a format that is valied on the 
 * current operating system.
 * @return string
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.join_paths('/home/user', '/path/to/myfile.ext')
 * '/home/user//path/to/myfile.ext'
 * ```
 */
def join_paths(...) {
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
 * @return string
 */
def real_path(path) {
  return _realpath(path)
}
