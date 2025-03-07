/**
 * @module log
 * 
 * @copyright 2025, Richard Ore and Blade contributors
 */
import enum
import io
import os
import date


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

/**
 * 
 */
def set_level(level) {
  if !is_number(level) {
    raise Exception('invalid log level')
  }

  _default_log_level = enum.ensure(LogLevel, level)
}


/**
 * 
 */
class Transport {
  var _level = LogLevel.None
  var _log_name = os.base_name(os.dir_name(__root__))

  var _enabled = true
  var _show_name = false
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


# This is the default transport that will be added to a logger.
# we have it here as a module variable so we can use it to remove
# the logger should the user decide to (if they prefer not to)
# simply disable it.
var _default_transport = ConsoleTransport()


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
