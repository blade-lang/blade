#
# @module reflect
# 
# Provides functionalities for interacting with and modifying modules, 
# classes and functions.
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _reflect

/**
 * has_prop(object: instance, name: string)
 *
 * returns true if instance has the property name or false if not
 * @return bool
 */
def has_prop(object, name) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.hasprop(object, name)
}

/**
 * get_prop(object: instance, name: string)
 *
 * returns the property of the instance matching the given name
 * or nil if the object contains no property with a matching
 * name
 * @return any
 */
def get_prop(object, name) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.getprop(object, name)
}

/**
 * set_prop(object: instance, name: string, value: any)
 *
 * sets the named property of the object to value.
 * 
 * @note if the property already exist, it overwrites it
 * @return bool: true if a new property was set, false if a property was
 * updated
 */
def set_prop(object, name, value) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.setprop(object, name, value)
}

/**
 * del_prop(object: instance, name: string)
 *
 * deletes the named property from the instance
 * @return bool
 */
def del_prop(object, name) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.delprop(object, name)
}

/**
 * has_method(object: instance, name: string)
 *
 * returns true if class of the instance has the method name or
 * false if not.
 * @return bool
 */
def has_method(object, name) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.hasmethod(object, name)
}

/**
 * has_decorator(object: instance, name: string)
 *
 * returns true if class of the instance implements the decorator name or
 * false if not.
 * @return bool
 */
def has_decorator(object, name) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.hasmethod(object, '@${name}')
}

/**
 * get_method(object: instance, name: string)
 *
 * returns the method in a class instance matching the given name
 * or nil if the class of the instance contains no method with
 * a matching name.
 * @return function
 */
def get_method(object, name) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.getmethod(object, name)
}

/**
 * get_decorator(object: instance, name: string)
 * 
 * returns the decorator function matching the given name in the class 
 * of the given instance.
 * @note the name of a decorator excludes the `@` character.
 * @return function
 */
def get_decorator(object, name) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  if _reflect.hasmethod(object, '@${name}') {
    return _reflect.getboundmethod(object, '@${name}')
  } else {
    die Exception("class ${typeof(object)} does not implement decorator '${name}'")
  }
}


/**
 * bind_method(object: instance, method: function)
 *
 * binds the given function to the instance, allowing you to access 
 * the instance itself in the function via the `self` keyword in 
 * the function.
 * @return function
 */
def bind_method(object, method) {
  if !is_instance(object) or !is_string(method)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.bindmethod(object, method)
}

/**
 * get_type(object: instance)
 * 
 * returns the type of an instance as string
 * @return string
 */
def get_type(object) {
  if !is_instance(object)
    die Exception('instance expected')

  return _reflect.gettype(object)
}

/**
 * is_ptr(value: any)
 * 
 * Returns `true` if _value_ is a pointer, `false` otherwise.
 * @return bool
 */
def is_ptr(value) {
  return _reflect.isptr(value)
}
