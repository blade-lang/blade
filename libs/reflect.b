#
# @module reflect
# 
# provides functionalities for interacting with and modifying modules, 
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
def has_prop(instance, name) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.hasprop(instance, name)
}

/**
 * get_prop(object: instance, name: string)
 *
 * returns the property of the instance matching the given name
 * or nil if the object contains no property with a matching
 * name
 * @return any
 */
def get_prop(instance, name) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.getprop(instance, name)
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
def set_prop(instance, name, value) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.setprop(instance, name, value)
}

/**
 * del_prop(object: instance, name: string)
 *
 * deletes the named property from the instance
 * @return bool
 */
def del_prop(instance, name) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.delprop(instance, name)
}

/**
 * has_method(object: instance, name: string)
 *
 * returns true if class of the instance has the method name or
 * false if not.
 * @return bool
 */
def has_method(instance, name) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.hasmethod(instance, name)
}

/**
 * has_decorator(object: instance, name: string)
 *
 * returns true if class of the instance implements the decorator name or
 * false if not.
 * @return bool
 */
def has_decorator(instance, name) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.hasmethod(instance, '@${name}')
}

/**
 * get_method(object: instance, name: string)
 *
 * returns the method in a class instance matching the given name
 * or nil if the class of the instance contains no method with
 * a matching name.
 * @return function
 */
def get_method(instance, name) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.getmethod(instance, name)
}

/**
 * get_decorator(object: instance, name: string)
 * 
 * returns the decorator function matching the given name in the class 
 * of the given instance.
 * @note the name of a decorator excludes the `@` character.
 * @return function
 */
def get_decorator(instance, name) {
  if !is_instance(instance) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  if _reflect.hasmethod(instance, '@${name}') {
    return _reflect.getboundmethod(instance, '@${name}')
  } else {
    die Exception("class ${typeof(instance)} does not implement decorator '${name}'")
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
def bind_method(instance, method) {
  if !is_instance(instance) or !is_string(method)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.bindmethod(instance, method)
}