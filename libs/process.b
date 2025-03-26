/** 
 * @module process
 * 
 * This module allows parallel processing by providing classes and functions 
 * that allows for spawning operating system processes thereby leveraging multiple 
 * processors on a machine. 
 * 
 * Example Usage:
 * 
 * ```blade
 * var paged = PagedValue()
 * 
 * var pr = Process(@(p, s) {
 *   echo 'It works!'
 *   echo p.id()
 *   s.set({name: 'Richard', age: 3.142})
 * }, paged)
 * 
 * pr.on_complete(@{
 *   echo paged.get()
 * })
 * 
 * pr.start()
 * echo 'It works fine!'
 * # pr.await()  # this can be used to wait for completion.
 * echo 'It works fine again!'
 * ```
 * 
 * Output:
 * 
 * ```sh
 * It works fine!
 * It works fine again!
 * It works!
 * 75608
 * {name: Richard, age: 3.142}
 * ```
 * 
 * @copyright 2022, Richard Ore and Blade contributors
 */

import _process
import reflect
import os
import struct

var _valid_paged_types = ['boolean', 'number', 'string', 'bytes', 'list', 'dict']


/**
 * The number of CPU cores available on the current device.
 * @type number
 */
var cpu_count = _process.cpu_count


/**
 * The PagedValue object allows the sharing of single value/state between 
 * processes and the main application or one another. 
 * 
 * PagedValue supports the following types:
 * 
 * - Boolean
 * - Number
 * - String
 * - List
 * - Dictionary
 * 
 * @note Lists and Dictionaries cannot be nested in a PagedValue.
 */
class PagedValue {

  /**
   * @param bool? executable
   * @param bool? private
   * @constructor
   */
  PagedValue(executable, private) {
    if executable == nil executable = false
    if private == nil private = false

    if !is_bool(executable)
      raise TypeError('boolean value expected in argument 1 (executable)')
    if !is_bool(private)
      raise TypeError('boolean value expected in argument 2 (private)')

    self._ptr = _process.new_paged(executable, private)
  }

  /**
   * Locks the PagedValue and disallows updating the value.
   */
  lock() {
    if self._ptr {
      _process.paged_lock(self._ptr)
    }
  }

  /**
   * Unlocks the PagedValue to allow for updating the value.
   */
  unlock() {
    if self._ptr {
      _process.paged_unlock(self._ptr)
    }
  }

  /**
   * Returns `true` if the PagedValue is locked for updating or `false` otherwise.
   * 
   * @returns boolean
   * @note a PagedValue is locked if in an invalid state.
   */
  is_locked() {
    if self._ptr {
      return _process.paged_islocked(self._ptr)
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
          if !_valid_paged_types.contains(typeof(item))
            raise TypeError('object of type ${typeof(item)} not allowed here')
          format += self._set_format(item)
        }
        return format
      }
      when 'dictionary' {
        var format = ''
        for item in value {
          if !_valid_paged_types.contains(typeof(item))
            raise TypeError('object of type ${typeof(item)} not allowed here')
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
   * Sets the value of the PagedValue to the given value. It returns the number of 
   * bytes written or `false` if the PagedValue is in an invalid state.
   * 
   * @param boolean|number|string|list|dictionary value
   * @returns number | boolean
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
      
      return _process.paged_write(self._ptr, format, get_format, struct.pack_from(format, value))
    }
    return false
  }

  /**
   * Locks the PagedValue for writing then sets the value to the given value and unlocks it. 
   * It returns the number of bytes written or `false` if the PagedValue is in an invalid state.
   * 
   * @param boolean|number|string|list|dictionary value
   * @returns number | boolean
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
   * Returns the value stored in the PagedValue or `nil` if no value has been set.
   * 
   * @returns any
   */
  get() {
    if self._ptr {
      var data = _process.paged_read(self._ptr)
      if data {
        var format = data[0]

        # check palindromes in format
        var palindromes = format.matches('/([a-z])\\1+/')
        if palindromes {
          iter var i = 0; i < palindromes[0].length(); i++ {
            var val = palindromes[0][i]
            var char = palindromes[1][i]
            
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

  /**
   * Returns the pointer to the raw memory paged location pointed to by the object.
   * 
   * @returns ptr
   */
  raw_pointer() {
    return _process.raw_pointer(self._ptr)
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
  var _paged
  var _ptr
  var _on_complete_listeners = []

  /**
   * Creates a new instance of Process for the function _`fn`_. This 
   * constructor accepts an optional PagedValue.
   * 
   * The function passed to a process must accept at least one parameter which 
   * will be passed the instance of the process itself and at most two parameters 
   * if the process was initialized with a PagedValue.
   * 
   * @param function fn
   * @param PageValue? paged
   * @constructor
   */
  Process(fn, paged) {
    if !is_function(fn)
      raise TypeError('function expected in argument 1 (fn)')
    if paged != nil and !instance_of(paged, PagedValue)
      raise TypeError('instance of PagedValue expected in argument 2 (paged)')

    # No windows support yet.
    if os.platform == 'windows' {
      raise NotImplementedError('Process is not yet supported on this OS')
    }

    self._fn = fn
    self._paged = paged
    self._ptr = _process.Process()
  }

  /**
   * Returns the ID of the process or `-1` if the process is in an invalid 
   * state or has not been started.
   * 
   * @returns number
   */
  id() {
    if self._ptr {
      return _process.id(self._ptr)
    }
    return -1
  }

  /**
   * Adds a new listener to be called when the process finishes execution.
   * @param function fn
   */
  on_complete(fn) {
    if !is_function(fn)
      raise TypeError('function expected at argument 1')
    self._on_complete_listeners.append(fn)
  }

  /**
   * Starts/runs the process. This function returns `true` or `false` if the 
   * process is in an invalid state.
   * 
   * @returns boolean
   */
  start() {
    if self._ptr {
      var id = _process.create(self._ptr)
      if id == -1 {
        raise Exception('failed to start process')
      } else if id == 0 {
        var fn_data = reflect.get_function_metadata(self._fn)
        var expected_arity = self._paged ? 2 : 1

        if fn_data.arity != expected_arity
          raise ArgumentError('process function must take ${expected_arity} arguments')

        if expected_arity == 2 {
          if self._paged self._fn(self, self._paged)
          else self._fn(self)
        } else {
          self._fn(self)
        }

        # call the complete listeners
        self._on_complete_listeners.each(@(fn) {
          fn()
        })

        os.exit(0)
      }
      return true
    }
    return false
  }

  /**
   * Awaits for the process to finish running and returns it's exit code or `-1` 
   * if the process is in an invalid state. Await can be used without `start()`.
   * If `await()` is called without a previous call to start(), the await
   * automatically calls start().
   * 
   * @returns number
   */
  await() {
    if self._ptr {
      if !self.is_alive() {
        self.start()
      }
      var result = _process.wait(self._ptr)
      return result
    }
    return -1
  }

  /**
   * Returns `true` if the process is running or `false` if not.
   * 
   * @returns boolean
   */
  is_alive() {
    return _process.is_alive(self._ptr)
  }

  /**
   * Kills the running process. Returns `true` if the process was successfully 
   * killed or `false` otherwise.
   * 
   * @returns boolean
   */
  kill() {
    if self._ptr {
      return _process.kill(self._ptr)
    }
    return false
  }
}

/**
 * Creates a new instance of Process for the function _`fn`_. This 
 * constructor accepts an optional PagedValue.
 * 
 * The function passed to a process must accept at least one parameter which 
 * will be passed the instance of the process itself and at most two parameters 
 * if the process was initialized with a PagedValue.
 * 
 * @param function fn
 * @param PageValue? paged
 * @default
 */
def process(fn, paged) {
  return Process(fn, paged)
}

