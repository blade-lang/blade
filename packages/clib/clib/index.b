import .types { * }

import _clib
import os
import reflect

var _EXT = os.platform == 'windows' ? '.dll' : (
  os.platform == 'linux' ? '.so' : '.dylib'
)

class _CLib {
  var _ptr

  /**
   * CLib([name: string])
   * @constructor
   */
  _CLib(name) {
    if name != nil and !is_string(name)
      die Exception('string expected in argument 1 (name)')

    if name {
      if !name.ends_with(_EXT)
        name += _EXT
      self.load(name)
    }
  }

  _ensure_lib_loaded() {
    if !self._ptr
      die Exception('no library loaded')
  }

  load(name) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')
    if !name.ends_with(_EXT)
      name += _EXT

    if self._ptr
      self.close()
    self._ptr = _clib.load(name)
  }

  close() {
    self._ensure_lib_loaded()
    _clib.close(self._ptr)
  }

  /**
   * @return ptr
   */
  function(name) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')

    self._ensure_lib_loaded()
    return _clib.function(self._ptr, name)
  }

  /**
   * define(name: string, return_type: type, ...type)
   * 
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
   */
  define(name, return_type, ...) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')

    # Ensure valid clib pointer.
    echo to_string(return_type)
    if !(reflect.is_ptr(return_type) and to_string(return_type).match('/clib/')) {
        die Exception('invalid return type')
    }

    var fn = self.function(name)
    var ffi_ptr = _clib.define(fn, name, return_type, __args__)

    def define(...) {
      return _clib.call(ffi_ptr, __args__)
    }

    return define
  }

  /**
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }
}

def load(name) {
  return _CLib(name)
}
