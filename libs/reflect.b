#
# @module reflect
# 
# This module provides many functions that can be used to interact with or modify modules, 
# classes and functions. It is well suited for many uses cases such as creating a library 
# that is heavily dependent on decorators (e.g. the `json` module).
# 
# ### For example,
# 
# We can call a decorator using the `reflect` module like this.
# 
# ```blade
# class A {
#   @custom_decorator() {
#     echo 'It works!'
#   }
# }
# 
# import reflect
# 
# var instance_of_a = A()
# var decorator = reflect.get_decorator(instance_of_a, 'custom_decorator')
# 
# # It's always good to check the result first as it will be a good 
# # practice to make decorators optional to make it easy for users to 
# # opt-in and opt-out of features your package or library provide.
# if decorator {
#   decorator()
# }
# ```
# 
# Try it out!
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _reflect

/**
 * has_prop(object: instance, name: string)
 *
 * Returns `true` if instance has the property name or `false` if not
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
 * Returns the property of the instance matching the given name
 * or nil if the object contains no property with a matching name.
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
 * Sets the named property of the object to value.
 * 
 * @note if the property already exist, it overwrites it
 * @return bool: `true` if a new property was set, `false` if a property was updated
 */
def set_prop(object, name, value) {
  if !is_instance(object) or !is_string(name)
    die Exception('arg1 must be instance and arg2 must be string')

  return _reflect.setprop(object, name, value)
}

/**
 * del_prop(object: instance, name: string)
 *
 * Deletes the named property from the instance
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
 * Returns true if class of the instance has the method name or
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
 * Returns true if class of the instance implements the decorator name or
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
 * Returns the method in a class instance matching the given name
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
 * Returns the decorator function matching the given name in the class 
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
 * Binds the given function to the instance, allowing you to access 
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
 * Returns the type of an instance as string
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
