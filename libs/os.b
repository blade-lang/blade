#
# @module os
# 
# provides functionalities for interfacing with the underlying operating system
# @copyright 2021, Ore Richard Muyiwa
# 


/**
 * platform()
 *
 * returns the name of the current platform/OS
 * @return string
 */
def platform(){}


/**
 * exec(cmd: string)
 *
 * exec executes the given shell commands
 * @return string
 */
def exec(cmd){}

/**
 * info()
 *
 * returns information about the current os
 * @return dict
 */
def info(){}

/**
 * sleep(duration: number)
 *
 * causes the current thread to sleep for the specified number of seconds
 * @return nil
 */
def sleep(duration){}

/**
 * getenv(name: string)
 *
 * returns the given environment variable if exists or nil otherwise
 * @return string
 */
def getenv(name){}

/**
 * setenv(name: string, value: string, overwrite: bool = true)
 *
 * sets the named environment variable to the given value.
 *
 * on Unix, if overwrite is false, it doesn't set a value if the variable
 * exists before.
 * @return string
 */
def setenv(name, value, overwrite){}

