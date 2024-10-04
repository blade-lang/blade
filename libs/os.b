/**
 * @module os
 * 
 * This module provides functions for interfacing with the underlying operating system and directories.
 * 
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

import _os

/**
 * The name of the current platform in string or `unknown` if 
 * the platform name could not be determined.
 * @type string
 * 
 * Example,
 * 
 * ```blade-repl
 * %> import os
 * %> os.platform
 * 'osx'
 * ```
 */
var platform = _os.platform

/**
 * A list containing the command line arguments passed to the startup script.
 * @type list
 */
var args = _os.args

/**
 * The standard path separator for the current operating system.
 * @type string
 */
var path_separator = _os.path_separator

/**
 * The full path to the running Blade executable.
 * @type string
 */
var exe_path = _os.exe_path

# File types
/**
 * Unknown file type
 * @type number
 */
var DT_UNKNOWN = _os.DT_UNKNOWN  # unknown

/**
 * Block device file type
 * @type number
 */
var DT_BLK = _os.DT_BLK  # block device

/**
 * Character device file type
 * @type number
 */
var DT_CHR = _os.DT_CHR  # character device

/**
 * Directory file type
 * @type number
 */
var DT_DIR = _os.DT_DIR  # directory

/**
 * Named pipe file type
 * @type number
 */
var DT_FIFO = _os.DT_FIFO  # named pipe

/**
 * Symbolic link file type
 * @type number
 */
var DT_LNK = _os.DT_LNK  # symbolic link

/**
 * Regular file type
 * @type number
 */
var DT_REG = _os.DT_REG  # regular file

/**
 * Local-domain socket file type
 * @type number
 */
var DT_SOCK = _os.DT_SOCK  # local-domain socket

/**
 * Whiteout file type (only meaningful on UNIX and some unofficial Linux versions).
 * @type number
 * @note value is `-1` on systems where it is not supported.
 */
var DT_WHT = _os.DT_WHT  


/**
 * Executes the given shell (or command prompt for Windows) commands and 
 * returns the output as string.
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.exec('ls -l')
 * 'total 48
 * -rw-r--r--@ 1 username  staff  705 Aug 27  2021 buggy.b
 * -rw-r--r--  1 username  staff  197 Mar  5 05:13 myprogram.b'
 * ```
 * 
 * @param string cmd
 * @returns string
 */
def exec(cmd) {
  return _os.exec(cmd)
}

/**
 * Returns information about the current operation system and machine as a dictionary.
 * The returned dictionary will contain:
 * 
 * - `sysname`: The name of the operating system
 * - `nodename` The name of the current machine
 * - `version`: The operating system version
 * - `release`: The release level/version
 * - `machine`: The hardware/processor type.
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.info()
 * {sysname: Darwin, nodename: MacBook-Pro.local, version: Darwin Kernel Version 
 * 21.1.0: Wed Oct 13 17:33:24 PDT 2021; root:xnu-8019.41.5~1/RELEASE_ARM64_T8101, 
 * release: 21.1.0, machine: arm64}
 * ```
 * 
 * @returns dict
 */
def info() {
  return _os.info()
}

/**
 * Causes the current thread to sleep for the specified number of seconds.
 * 
 * @param number duration
 */
def sleep(duration) {
  _os.sleep(duration)
}

/**
 * Returns the given environment variable if exists or nil otherwise
 * @returns string
 * 
 * Example,
 * 
 * ```blade-repl
 * %> import os
 * %> os.get_env('ENV1')
 * '20'
 * ```
 * 
 * @param string name
 * @returns string|nil
 */
def get_env(name) {
  return _os.getenv(name)
}

/**
 * Sets the named environment variable to the given value.
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
 * @param string name
 * @param string value
 * @param bool? overwrite: Default value is `false`.
 * @returns string
 */
def set_env(name, value, overwrite) {
  if overwrite == nil overwrite = false
  return _os.setenv(name, value, overwrite)
}

/**
 * Creates the given directory with the specified permission and optionaly 
 * add new files into it if any is given.
 * 
 * @note if the directory already exists, it returns `false` otherwise, it returns `true`.
 * @note permission should be given as octal number.
 * @param string path
 * @param number? permission: Default value is `0c777`
 * @param bool? recursive: Default value is `true`.
 * @returns boolean
 */
def create_dir(path, permission, recursive) {

  if path {
    if !is_string(path) raise Exception('path must be string')
    path = path
  }
  if !path.ends_with(path_separator)
    path += path_separator

  if platform == 'windows' {
    path = path.replace('@\/@', '\\')
  }

  if permission {
    if !is_number(permission)
      raise Exception('expected number in first argument, ${typeof(permission)} given')
  } else {
    permission = 0c777
  }

  if recursive != nil {
    if !is_bool(recursive) 
      raise Exception('boolean expected in second argument, ${typeof(recursive)} given')
  } else {
    recursive = true
  }

  return _os.createdir(path, permission, recursive)
}

/**
 * Scans the given directory and returns a list of all matched files
 * @returns list[string]
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.read_dir('./tests')
 * [., .., myprogram.b, single_thread.b, test.b, buggy.b]
 * ```
 * 
 * @note `.` indicates current directory and can be used as argument to _os.path_ as well.
 * @note `..` indicates parent directory and can be used as argument to _os.path_ as well.
 * @param string path
 * @returns List[string]
 */
def read_dir(path) {
  return _os.readdir(path)
}

/**
 * Changes the permission set on a directory to the given mode. It is advisable 
 * to set the mode with an octal number (e.g. 0c777) as this is consistent with 
 * operating system values.
 * 
 * @param string path
 * @param number mode
 * @returns boolean
 */
def chmod(path, mode) {
  return _os.chmod(path, mode)
}

/**
 * Returns `true` if the path is a directory or `false` otherwise.
 * 
 * @param string path
 * @returns bool
 */
def is_dir(path) {
  return _os.isdir(path)
}

/**
 * Deletes a non-empty directory. If recursive is `true`, non-empty directories 
 * will have their contents deleted first.
 * 
 * @param string path
 * @param bool recursive: Default value is `false`.
 * @returns bool
 */
def remove_dir(path, recursive) {
  if recursive != nil {
    if !is_bool(recursive)
      raise Exception('boolean expected in argument 2')
  } else {
    recursive = false
  }
  return _os.removedir(path, recursive)
}

/**
 * Returns the current working directory.
 * 
 * @returns string
 */
def cwd() {
  return _os.cwd()
}

/**
 * Navigates the working directory into the specified path.
 * 
 * @param string path
 * @returns bool
 */
def change_dir(path) {
  return _os.chdir(path)
}

/**
 * Returns `true` if the directory exists or `false` otherwise.
 * 
 * @param string path
 * @returns bool
 */
def dir_exists(path) {
  return _os.exists(path)
}

/**
 * Exit the current process and quits the Blade runtime.
 * 
 * @param number code
 * @returns
 */
def exit(code) {
  _os.exit(code)
}

/**
 * Concatenates the given paths together into a format that is valied on the 
 * current operating system.
 * 
 * Example,
 * 
 * ```blade-repl
 * %> os.join_paths('/home/user', 'path/to/myfile.ext')
 * '/home/user/path/to/myfile.ext'
 * ```
 * 
 * @param string... paths
 * @returns string
 */
def join_paths(...) {
  var result = ''
  for arg in __args__ {
    if !is_string(arg)
      raise Exception('string expected, ${typeof(arg)} given')

    arg = arg.trim()
    
    if arg {
      result = result.rtrim(path_separator)
      if result != '' arg = arg.ltrim(path_separator)
      
      result += '${path_separator}${arg}'
    }
  }
  
  if result result = result[1,]
  
  return result
}

/**
 * Returns the original path to a relative path.
 * 
 * @note if the path is a file, see `abs_path()`.
 * @param string path
 * @returns string
 */
def real_path(path) {
  return _os.realpath(path)
}

/**
 * Returns the original path to a relative path.
 * 
 * @note unlike real_path(), this function returns full path for a file.
 * @param string path
 * @returns string
 */
def abs_path(path) {

  # Return early if we already have an absolute path.
  var regex = platform == 'windows' ? '~^[a-zA-Z]\\:~' : '~^\\/~'
  if path.match(regex)
    return path

  var p = _os.realpath(path)
  if p == path {
    var np = _os.realpath('.')

    if np != path {
      p = join_paths(np, p)
    }
  }

  return p
}

/**
 * Returns the parent directory of the pathname pointed to by `path`.  Any trailing
 * `/` characters are not counted as part of the directory name.  If `path` is an
 * empty string, or contains no `/` characters, dir_name() returns the string ".", 
 * signifying the current directory.
 * 
 * @param string path
 * @returns string
 */
def dir_name(path) {
  return _os.dirname(path)
} 

/**
 * The base_name() function returns the last component from the pathname pointed to by 
 * `path`, deleting any trailing `/` characters.  If path consists entirely of `/` 
 * characters, the string '/' is returned.  If path is an empty string, the string '.' 
 * is returned.
 * 
 * @param string path
 * @returns string
 */
def base_name(path) {
  return _os.basename(path)
}
