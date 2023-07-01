# 
# @module process
# 
# This module allows parallel processing by providing classes and functions 
# that allows for spawning operating system processes thereby leveraging multiple 
# processors on a machine. 
# 
# Example Usage:
# 
# ```blade
# var shared = SharedValue()
# 
# var pr = Process(@(p, s) {
#   echo 'It works!'
#   echo p.id()
#   s.set({name: 'Richard', age: 3.142})
# }, shared)
# 
# pr.on_complete(||{
#   echo shared.get()
# })
# 
# pr.start()
# echo 'It works fine!'
# # pr.await()  # this can be used to wait for completion.
# echo 'It works fine again!'
# ```
# 
# Output:
# 
# ```sh
# It works fine!
# It works fine again!
# It works!
# 75608
# {name: Richard, age: 3.142}
# ```
# 
# @copyright 2022, Ore Richard Muyiwa and Blade contributors
# 

import _process
import reflect
import os
import struct
/**
 * The number of CPU cores available on the current device.
 * @type number
 */
var cpu_count = _process.cpu_count


/**
 * The SharedValue object allows the sharing of single value/state between 
 * processes and the main application or one another. 
 * 
 * SharedValue supports the following types:
 * 
 * - Boolean
 * - Number
 * - String
 * - List
 * - Dictionary
 * 
 * @note Lists and Dictionaries cannot be nested in a SharedValue.
 */
class SharedValue {

  /**
   * SharedValue()
   * @constructor
   */
  SharedValue() {
    self._ptr = _process.new_shared()
  }

  /**
   * lock()
   * 
   * Locks the SharedValue and disallows updating the value.
   */
  lock() {
    if self._ptr {
      _process.shared_lock(self._ptr)
    }
  }

  /**
   * unlock()
   * 
   * Unlocks the SharedValue to allow for updating the value.
   */
  unlock() {
    if self._ptr {
      _process.shared_unlock(self._ptr)
    }
  }

  /**
   * is_locked()
   * 
   * Returns `true` if the SharedValue is locked for updating or `false` otherwise.
   * 
   * @return boolean
   * @note a SharedValue is locked if in an invalid state.
   */
  is_locked() {
    if self._ptr {
      return _process.shared_islocked(self._ptr)
    }
    return true
  }

  _set_format(value) {
    using typeof(value) {
      when 'boolean' return 'i'
      when 'number' return 'd'
      when 'string' return 'a${value.length()}'
      when 'bytes' return 'C${value.length()}'
      when 'list' {
        var format = ''
        for item in value {
          if is_list(item) or is_dict(item)
            die Exception('list not allowed here')
          format += self._set_format(item)
        }
        return format
      }
      when 'dictionary' {
        var format = ''
        for item in value {
          if is_list(item) or is_dict(item)
            die Exception('list or dictionay not allowed here')
          format += self._set_format(item)
        }
        return format
      }
    }

    return ''
  }

  _do_get_format(value) {
    var format = ''
    using typeof(value) {
      when 'boolean' return 'i'
      when 'number' return 'd'
      when 'string' return 'a${value.length()}'
      when 'bytes' return 'C${value.length()}'
      when 'list' {
        var format = '', index = 0
        for item in value {
          format += '/' + self._do_get_format(item) + '.${index++}'
        }
        if format format = format[1,]
        return format
      }
      when 'dictionary' {
        var format = '', index = 0
        for key, item in value {
          format += '/' + self._do_get_format(item) + '${key}'
        }
        if format format = format[1,]
        return format
      }
    }

    return ''
  }

  _get_format(value) {
    return self._do_get_format(value)
  }

  /**
   * set(value: boolean | number | string | list | dictionary)
   * 
   * Sets the value of the SharedValue to the given value. It returns the number of 
   * bytes written or `false` if the SharedValue is in an invalid state.
   * 
   * @return number | boolean
   */
  set(value) {
    if self._ptr {
      var format = self._set_format(value),
          get_format = self._get_format(value)

      
      # create value parameter
      if value == nil
        value = bytes(0)
      else if is_bytes(value)
        value = value.to_list()
      else if is_dict(value) 
        value = value.values()
      else if !is_list(value) 
        value = [value]
      
      return _process.shared_write(self._ptr, format, get_format, struct.pack_from(format, value))
    }
    return false
  }

  /**
   * locked_set(value: boolean | number | string | list | dictionary)
   * 
   * Locks the SharedValue for writing then sets the value to the given value and unlocks it. 
   * It returns the number of bytes written or `false` if the SharedValue is in an invalid state.
   * 
   * @return number | boolean
   */
  locked_set(value) {
    if self._ptr {
      self.lock()
      var result = self.set(value)
      self.unlock()
      return result
    }
    return false
  }

  /**
   * get()
   * 
   * Returns the value stored in the SharedValue or `nil` if no value has been set.
   * 
   * @return any
   */
  get() {
    if self._ptr {
      var data = _process.shared_read(self._ptr)
      if data {
        var format = data[0]

        # check palindromes in format
        var palins = format.matches('/([a-z])\\1+/')
        if palins {
          iter var i = 0; i < palins[0].length(); i++ {
            var val = palins[0][i]
            var char = palins[1][i]
            
            var rem = ''
            for j in 0..val.length() {
              rem += '/${char}_${j}'
            }
            rem = rem[1,]

            format = format.replace('/${val}/', rem)
          }
        }

        var result = struct.unpack(format, data[1])
        if result {
          if format.match('/^C\\d+$/') {
            return bytes(result.values())
          } else if format.match('/^[a-z]\\d*$/') {
            return result[1]
          } else if format.index_of('/') > -1 and format.index_of('.') == -1 {
            return result
          } else {
            return result.values()
          }
        }
      }
    }
    return nil
  }

  # by using a decorator, we hide it from ever getting 
  # called by a user directly.
  @get_pointer() {
    return self._ptr
  }
}


/**
 * This class allows creating and spawning operating system processes 
 * and using them to run functions.
 */
class Process {
  var _fn
  var _shared
  var _ptr
  var _on_complete_listeners = []

  /**
   * Process(fn: function [, shared: SharedValue])
   * 
   * Creates a new instance of Process for the function _`fn`_. This 
   * constructor accepts an optional SharedValue.
   * 
   * The function passed to a process must accept at least one parameter which 
   * will be passed the instance of the process itself and at most two parameters 
   * if the process was intitalized with a SharedValue.
   * @constructor
   */
  Process(fn, shared) {
    if !is_function(fn)
      die Exception('function expected in argument 1 (fn)')
    if shared != nil and !instance_of(shared, SharedValue)
      die Exception('instance of SharedValue expected in argument 2 (shared)')

    # No windows support yet.
    if os.platform == 'windows' {
      die Exception('Process is not yet supported on this OS')
    }

    self._fn = fn
    self._shared = shared
    self._ptr = _process.Process()
  }

  /**
   * id()
   * 
   * Returns the ID of the process or `-1` if the process is in an invalid 
   * state or has not been started.
   * 
   * @return number
   */
  id() {
    if self._ptr {
      return _process.id(self._ptr)
    }
    return -1
  }

  /**
   * on_complete(fn: function)
   * 
   * Adds a new listener to be called when the process finishes execution.
   */
  on_complete(fn) {
    if !is_function(fn)
      die Exception('function expected at argument 1')
    self._on_complete_listeners.append(fn)
  }

  /**
   * start()
   * 
   * Starts/runs the process. This function returns `true` or `false` if the 
   * process is in an invalid state.
   * 
   * @return boolean
   */
  start() {
    if self._ptr {
      var id = _process.create(self._ptr)
      if id == -1 {
        die Exception('failed to start process')
      } else if id == 0 {
        var fn_data = reflect.get_function_metadata(self._fn)
        var expected_arity = self._shared ? 2 : 1

        if fn_data.arity != expected_arity
          die Exception('process function must take ${expected_arity} arguments')

        if expected_arity == 2 {
          if self._shared self._fn(self, self._shared)
          else self._fn(self)
        } else {
          self._fn(self)
        }

        # call the complete listeners
        self._on_complete_listeners.each(@(fn, _) {
          fn()
        })

        os.exit(0)
      }
      return true
    }
    return false
  }

  /**
   * await()
   * 
   * Awaits for the process to finish running and returns it's exit code or `-1` 
   * if the process is in an invalid state.
   * 
   * @return number
   */
  await() {
    if self._ptr {
      self.start()
      var result = _process.wait(self._ptr)
      return result
    }
    return -1
  }

  /**
   * is_alive()
   * 
   * Returns `true` if the process is running or `false` if not.
   * 
   * @return boolean
   */
  is_alive() {
    return _process.is_alive(self._ptr)
  }

  /**
   * kill()
   * 
   * Kills the running process. Returns `true` if the process was successfully 
   * killed or `false` otherwise.
   * 
   * @return boolean
   */
  kill() {
    if self._ptr {
      return _process.kill(self._ptr)
    }
    return false
  }
}

/**
 * process(fn: function [, shared: SharedValue])
 * 
 * Creates a new instance of Process for the function _`fn`_. This 
 * constructor accepts an optional SharedValue.
 * 
 * The function passed to a process must accept at least one parameter which 
 * will be passed the instance of the process itself and at most two parameters 
 * if the process was intitalized with a SharedValue.
 */
def process(fn, shared) {
  return Process(fn, shared)
}

