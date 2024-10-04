/**
 * @module clib
 *
 * The `clib` module exposes Blade capabilities to interact with C
 * shared libraries. The workflow follows a simple approach.
 *
 * - Load the library
 * - Define the function schematics
 * - Call the function.
 *
 * That simple!
 *
 * For example, the following code `dirname()` and `cos()` function from the
 * standard C library on a Unix machine (Linux, OSX, FreeBSD etc).
 *
 * ```blade
 * # Import clib
 * import clib
 *
 * # 1. Load 'libc' shared module available on Unix systems
 * var lib = clib.load('libc')
 *
 * # 2. Declare the functions
 * var dirname = lib.define('dirname', clib.char_ptr, clib.char_ptr)
 * var cos = lib.define('cos', clib.double, clib.double)     # this may not work on linux
 *
 * # 3. Call the functions
 * echo dirname('/path/to/my/file.ext')
 * echo cos(23)
 *
 * # Close the library (this is a good practice, but not required)
 * lib.close()
 * ```
 *
 * The first argument to a definition is the name of the function.
 * The second is its return type. If the function takes parameters,
 * the parameter types follow immediately. (See below for a list of the
 * available types.)
 *
 * > **NOT YET SUPPORTED:**
 * > - Variadic functions
 *
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

import .types { * }

import _clib
import os
import reflect

var _EXT = os.platform == 'windows' ? '.dll' : (
  os.platform == 'linux' ? '.so' : '.dylib'
)

/**
 * class CLib provides an interface for interacting with C shared modules.
 */
class Clib {
  
  var _ptr

  /**
   * @note The _name_ should follow the same practice outlined in `load()`.
   * @param string? name
   * @constructor
   */
  Clib(name) {
    if name != nil and !is_string(name)
      raise Exception('string expected in argument 1 (name)')

    if name {
      self.load(name)
    }
  }

  _ensure_lib_loaded() {
    if !self._ptr
      raise Exception('no library loaded')
  }

  /**
   * Loads a new C shared library pointed to by name. Name must be a 
   * relative path, absolute path or the name of a system library. 
   * If the system shared library extension is omitted in the name, 
   * it will be automatically added except on Linux machines.
   * 
   * @param string name
   */
  load(name) {
    if !is_string(name)
      raise Exception('string expected in argument 1 (name)')
    if !name.ends_with(_EXT) and os.platform != 'linux'
      name += _EXT

    if self._ptr
      self.close()
    self._ptr = _clib.load(name)
  }

  /**
   * Closes the handle to the shared library.
   */
  close() {
    self._ensure_lib_loaded()
    _clib.close(self._ptr)
  }

  /**
   * Retrieves the handle to a specific function in the shared library.
   * 
   * @param string name
   * @returns ptr
   */
  function(name) {
    if !is_string(name)
      raise Exception('string expected in argument 1 (name)')

    self._ensure_lib_loaded()
    return _clib.function(self._ptr, name)
  }

  /**
   * Defines a new C function with the given name and return type.
   * -  When there are no more argument, it is declared that the function
   *    takes no argument.
   * -  `define()` expects a list of the argument/parameter types as expected
   *    by the function.
   * 
   * E.g.
   * 
   * ```blade
   * define('myfunc', int, int, ptr)
   * ```
   * 
   * Corresponds to the C declaration:
   * 
   * ```c
   * int myfunc(int a, void *b);
   * ```
   * 
   * @param string name
   * @param clib_type return_type
   * @param clib_type... types
   * @returns function
   */
  define(name, return_type, ...) {
    if !is_string(name)
      raise Exception('string expected in argument 1 (name)')

    # Ensure valid clib pointer.
    if !(reflect.is_ptr(return_type) and to_string(return_type).match('/clib/')) {
      raise Exception('invalid return type')
    }

    var fn = self.function(name)
    var ffi_ptr = _clib.define(fn, name, return_type, __args__)

    def define(...) {
      return _clib.call(ffi_ptr, __args__)
    }

    return define
  }

  /**
   * Returns a pointer to the underlying module.
   * 
   * @returns ptr
   */
  get_pointer() {
    return self._ptr
  }
}

/**
 * Loads a new C shared library pointed to by name. Name must be a 
 * relative path, absolute path or the name of a system library. 
 * If the system shared library extension is omitted in the name, 
 * it will be automatically added.
 * 
 * @param string name
 * @returns CLib
 */
def load(name) {
  return Clib(name)
}

/**
 * Creates a new C value for the specified clib type with the given values.
 * 
 * @param clib_type type
 * @param any... values
 * @returns bytes
 */
def new(type, ...) {
  if __args__.length() == 0
    raise Exception('canot have an empty struct')

  # Ensure a valid and non void clib pointer.
  if !(reflect.is_ptr(type) and to_string(type).match('/clib/')) and type != void
    raise Exception('invalid type for new')

  return _clib.new(type, __args__)
}

/**
 * Returns the data contained in a C type _type_ encoded in the data.
 * The data should either be an output of `clib.new()` or a call to a 
 * function returning one of struct, union or array.
 * 
 * For structures created with `named_struct()`, a dictionary will 
 * automatically be returned with the values mapped to the names of the 
 * structure elements.
 * 
 * @param clib_type type
 * @param string|bytes data
 * @returns list|dictionary
 */
def get(type, data) {
  # Ensure a valid and non void clib pointer.
  if !(reflect.is_ptr(type) and to_string(type).match('/clib/')) and type != void
    raise Exception('invalid type for new')
  if is_string(data) data = data.to_bytes()

  return _clib.get(type, data)
}

/**
 * get_ptr_index(pointer: ptr, type: clib_type, index: number)
 * 
 * Get the value at the given index of a pointer based 
 * on the given CLib type.
 * 
 * @param ptr pointer
 * @param clib_type type 
 * @param number index
 * @returns any
 */
def get_ptr_index(pointer, type, index) {
  return _clib.get_ptr_index(pointer, type, index)
}

/**
 * Sets the value at the given index of a pointer based 
 * on the given CLib type to the given value.
 * 
 * @param ptr pointer
 * @param clib_type type
 * @param number index
 * @param any value
 * @returns any
 */
def set_ptr_index(pointer, type, index, value) {
  return _clib.set_ptr_index(pointer, type, index, value)
}

/**
 * Defines a new C function from an existing handle and return type.
 * -  When there are no more argument, it is declared that the function
 *    takes no argument.
 * -  `define()` expects a list of the argument/parameter types as expected
 *    by the function.
 * 
 * E.g.
 * 
 * ```blade
 * function_handle(my_ptr, int, int, ptr)
 * ```
 * 
 * Corresponds to the C declaration:
 * 
 * ```c
 * int (*my_ptr)(int a, void *b);
 * ```
 * 
 * @param ptr handle
 * @param clib_type return_type
 * @param clib_type... arg_types
 * @returns function
 */
def function_handle(handle, return_type, ...) {
  if !reflect.is_ptr(handle)
    raise Exception('pointer expected in argument 1 (handle)')

  # Ensure valid clib pointer.
  if !(reflect.is_ptr(return_type) and to_string(return_type).match('/clib/')) {
      raise Exception('invalid return type')
  }

  var ffi_ptr = _clib.define(handle, '@', return_type, __args__)

  def define(...) {
    return _clib.call(ffi_ptr, __args__)
  }

  return define
}

/**
 * Creates a callback to be passed to C functions expecting a callback.
 * 
 * For example, imagine a C function defined as below:
 * 
 * ```c
 * void ex_puts(const char *name, void (*fn)(char *req, char *res));
 * ```
 * 
 * To pass the callback (second parameter) to this function, you'll need to 
 * wrap a blade function with `create_callback()` to properly define the 
 * callback return type and parameters.
 * 
 * The above function can be defined as:
 * 
 * ```blade
 * var fn lib.define('ex_puts', clib.void, clib.char_ptr, clib.function)
 * ```
 * 
 * To call this function and pass a Blade function that can be called when C 
 * triggers the callback, the second argument to the function will need to be 
 * wrapped in `create_callback()`. Thus, the above function can be called 
 * like this:
 * 
 * ```blade
 * fn(
 *    'Blade Callbacks', 
 *    clib.create_callback(
 *      @(req, res) {
 *        echo 'Request is: ' + req
 *        echo 'Response is: ' + res
 *      }, 
 *      clib.void, # The return type of the callback
 *      clib.char_ptr, clib.char_ptr  # the parameters of the callback
 *    )
 * )
 * ```
 * 
 * > **NOTE:** A callback can only be passed to a parameter previously defined 
 * > as function.
 * 
 * @param function closure
 * @param clib_type return_type
 * @param clib_type... types
 * @returns clib_callback
 */
def create_callback(closure, return_type, ...) {
  return _clib.new_closure(closure, return_type, __args__)
}
