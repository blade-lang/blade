/**
 * @module log
 * 
 * This module implements a simple and flexible event logging system for all 
 * Blade applications and modules. With support for multiple transport systems 
 * as well as custom transports, this module allows easy application logging 
 * and log shipping.
 * 
 * The module selects defaults that is familiar for most end use-cases in order 
 * to allow for a minimal need for configurations so that you can start logging 
 * right out of the box.
 * 
 * Below is a very simple but powerful and complete usage of this module:
 * 
 * ```blade-repl
 * %> import log
 * %> log.info('Starting my application...')
 * '2025-03-07T08:00:33+01:00 INFO [.]: Starting my application...'
 * ```
 * 
 * > **IMPORTANT!**
 * >
 * > Did you notice that `[.]`? That's because we are running in a REPL. By default, 
 * > the `log` module provides information regarding the source of the log i.e. the 
 * > application from which the log came from thereby allowing multiple applications 
 * > log into the same transport pool without ambiguity.
 * >
 * > You can customize this name by setting the name on the transport via 
 * > [[log.Transport.set_name]].
 * 
 * This module provides all functionalities at the module level, allowing 
 * configurations to be carried across all files and modules in the lifetime of an 
 * application.
 * 
 * While allowing creation of custom transports for logs, the module provides 
 * transports for logging to the console (`ConsoleTransport`) and files 
 * (`FileTransport`) out of the box. This covers the most simple use-cases for most 
 * applications. 
 * 
 * The default transport enabled is the `ConsoleTransport` known as the 
 * `default_transport()` and need no extra work to enable unless you have previously 
 * disabled it. The example below shows how to enable the file transport to log to a 
 * file on disk.
 * 
 * ```blade
 * import log
 * 
 * var transport = log.FileTransport('mylog.log')
 * log.add_transport(transport)
 * 
 * log.info('Finished setting up file log...')
 * ```
 * 
 * If you check the file `mylog.log` now, you should see something like this:
 * 
 * ```sh
 * 2025-03-01T12:00:00+01:00 INFO [tmp]: Finished setting up file log...
 * ```
 * 
 * In addition to the log appearing on the console, you can now see the log in a 
 * persistent file. There are many ways to turn off the console output and log to file 
 * alone. 
 * 
 * Firstly, you can simple disable the default transport.
 * 
 * ```blade
 * log.default_transport().disable()
 * ```
 * 
 * The advantage to this approach is that while it disables the default transport, the 
 * transport is still registered and you can simple enable it at any time during the 
 * lifetime of the application by doing the reverse:
 * 
 * ```blade
 * log.default_transport().enable()
 * ```
 * 
 * The same strategy applies to all transports as the `enable()` and `disable()` method 
 * will be inherited from the [[log.Transport]] class.
 * 
 * The second approach is to completely remove the transport from the list of registered 
 * transports.
 * 
 * ```blade
 * log.remove_transport(log.default_transport())
 * ```
 * 
 * With this second approach, you'll need to register the transport again should you 
 * want to continue logging to the console. The same applies to all transport types.
 * 
 * For more complex uses, the process of creating a custom transport is really simple. 
 * To create a custom transport, you'll need to create a class that inherits from 
 * [[log.Transport]] and implement both the `format()` and `write()` method at a minimum.
 * 
 * The example below shows the creation of a custom transport that outputs structured 
 * JSON data to the console.
 * 
 * ```blade
 * # my_custom_transport.b
 * 
 * import log { Transport }
 * import json
 * import enum
 * 
 * class JsonConsoleTransport < Transport {
 *   format(records, level) {
 *     return {
 *       records,
 *       level,
 *     }
 *   }
 * 
 *   write(message) {
 *     echo json.encode({
 *       logtime: time(),
 *       records: message.records,
 *       level: enum.to_value_dict(log.LogLevel)[message.level]
 *     })
 *   }
 * }
 * ```
 * 
 * ```blade
 * import log
 * import .my_custom_transport { JsonConsoleTransport }
 * 
 * log.add_transport(JsonConsoleTransport())
 * 
 * log.info('Finished setting up json console log transport...')
 * ```
 * 
 * You should be seeing something similar to the below if you run the code:
 * 
 * ```sh
 * {"logtime":1741376459,"records":["Finished setting up json console log transport..."],"level":"Info"}
 * ```
 * 
 * You can set multiple transports at the same time as well as set them to only work at 
 * different log levels. Every transport inherits the method [[log.Transport.set_level]] 
 * which allows us set the minimum level at which a transport is available.
 * 
 * There is also a gloabl [[log.set_level]] function that allows us to set the minimum 
 * log level at which all transports can start logging.
 * 
 * ```blade
 * import log
 * 
 * log.set_level(log.Warning)
 * 
 * log.info('Finished setting up json console log transport...')
 * ```
 * 
 * If you try the above code, you won't be seeing anything in the console. This is because 
 * level [[log.Info]] is lower than the minium required [[log.Warning]].
 * 
 * @copyright 2025, Richard Ore and Blade contributors
 */
import enum
import io
import os
import date


/**
 * 
 */
var LogLevel = enum([
  'None',
  'Debug',
  'Info',
  'Warning',
  'Error',
  'Critical',
])

var _level_lookup_table = enum.to_value_dict(LogLevel)
var _default_log_level = LogLevel.None

# LogLevel Exports

/**
 * 
 */
var None =  LogLevel.None

/**
 * 
 */
var Debug = LogLevel.Debug

/**
 * 
 */
var Info = LogLevel.Info

/**
 * 
 */
var Warning = LogLevel.Warning

/**
 * 
 */
var Error = LogLevel.Error

/**
 * 
 */
var Critical = LogLevel.Critical


/**
 * 
 */
class Transport {
  var _level = LogLevel.None
  var _log_name = os.base_name(os.dir_name(__root__))

  var _enabled = true
  var _show_name = true
  var _show_time = true
  var _time_format = 'c'

  /**
   * 
   */
  set_level(level) {
    if !is_number(level) {
      raise Exception('invalid log level')
    }

    self._level = enum.ensure(LogLevel, level)
  }

  /**
   * 
   */
  get_level() {
    return self._level
  }

  /**
   * 
   */
  set_name(name) {
    if !is_string(name) {
      raise Exception('string expected, ${typeof(name)} given')
    }
  
    self._log_name = name
  }

  /**
   * 
   */
  get_name() {
    return self._log_name
  }

  /**
   * 
   */
  set_time_format(format) {
    if !is_string(format) {
      raise Exception('string expected, ${typeof(name)} given')
    }
  
    self._time_format = format
  }

  /**
   * 
   */
  get_time_format() {
    return self._time_format
  }

  /**
   * 
   */
  show_name(bool) {
    if bool == nil bool = true

    if !is_bool(bool) {
      raise Exception('boolean expected, ${typeof(bool)} given')
    }

    return self._show_name = bool
  }

  /**
   * 
   */
  show_time(bool) {
    if bool == nil bool = true

    if !is_bool(bool) {
      raise Exception('boolean expected, ${typeof(bool)} given')
    }

    return self._show_time = bool
  }

  /**
   * 
   */
  can_log(level) {
    if !is_number(level) {
      raise Exception('invalid log level')
    }
    
    return self._enabled and self._level <= level and level >= _default_log_level
  }

  /**
   * 
   */
  enable() {
    self._enabled = true
  }

  /**
   * 
   */
  disable() {
    self._enabled = false
  }

  /**
   * 
   */
  format(records, level) {}

  /**
   * 
   */
  write(message) {}

  /**
   * 
   */
  flush() {}

  /**
   * 
   */
  close() {}
}


# Verifies that obj is a valid Transport and returns obj as is if true 
# or raises an Exception if not.
def _assert_transport(obj) {
  if !instance_of(obj, Transport) {
    raise Exception('invalid Transport')
  }

  return obj
}


/**
 * 
 */
class ConsoleTransport < Transport {
  write(message) {
    echo message
  }

  format(records, level) {
    var message = ''

    if self._show_time {
      message += date().format(self._time_format)
    }

    message += ' ${_level_lookup_table[level].upper()}'

    if self._show_name {
      message += ' [${self._log_name}]'
    }

    message += ':'

    for arg in records {
      message += ' ' + to_string(arg)
    }

    return message
  }

  flush() {
    io.flush(io.stdout)
  }
}


/**
 * 
 */
class FileTransport < ConsoleTransport {
  
  /**
   * 
   */
  FileTransport(path) {
    self.file = file(path, 'a')
  }

  write(message) {
    self.file.write('${message}\n')
  }

  flush() {
    self.file.flush()
  }

  close() {
    self.file.close()
  }
}


# This is the default transport that will be added to a transport.
# we have it here as a module variable so we can use it to remove
# the transport should the user decide to (if they prefer not to)
# simply disable it.
var _default_transport = ConsoleTransport()


/**
 * 
 */
def set_level(level) {
  if !is_number(level) {
    raise Exception('invalid log level')
  }

  _default_log_level = enum.ensure(LogLevel, level)
}


# A list of registered transports
var _transports = [ _default_transport ]

/**
 * 
 */
def add_transport(transport) {
  # ensure we are not adding the same transport again
  if _transports.contains(transport) {
    return
  }

  _transports.append(transport)
}

/**
 * 
 */
def remove_transport(transport) {
  _transports.remove(transport)
}

/**
 * 
 */
def default_transport() {
  return _default_transport
}


# Reuseable write function for the log level helpers.
def _write(level, args) {
  for trans in _transports {
    if trans.can_log(level) {
      trans.write(trans.format(args, level))
    }
  }
}


/**
 * 
 */
def log(...) {
  _write(LogLevel.None, __args__)
}

/**
 * 
 */
def info(...) {
  _write(LogLevel.Info, __args__)
}

/**
 * 
 */
def debug(...) {
  _write(LogLevel.Debug, __args__)
}

/**
 * 
 */
def warn(...) {
  _write(LogLevel.Warning, __args__)
}

/**
 * 
 */
def error(...) {
  _write(LogLevel.Error, __args__)
}

/**
 * 
 */
def critical(...) {
  _write(LogLevel.Critical, __args__)
}
