#
# @module reflect
# 
# provides functionalities for interacting with and modifying modules, 
# classes and functions.
# @ copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _reflect

/**
 * has_prop(object: instance, name: string)
 *
 * returns true if instance has the property name or false if not
 */
def has_prop(instance, prop) {
  return _reflect.hasprop(instance, prop)
}

/**
 * get_prop(object: instance, name: string)
 *
 * returns the property of the instance matching the given name
 * or nil if the object contains no property with a matching
 * name
 */
def get_prop(instance, prop) {
  return _reflect.getprop(instance, prop)
}

/**
 * set_prop(object: instance, name: string, value: any)
 *
 * sets the named property of the object to value.
 * 
 * @note if the property already exist, it overwrites it
 * @returns bool: true if a new property was set, false if a property was
 * updated
 */
def set_prop(instance, prop) {
  return _reflect.setprop(instance, prop)
}

/**
 * del_prop(object: instance, name: string)
 *
 * deletes the named property from the instance
 * @returns bool
 */
def del_prop(instance, prop) {
  return _reflect.delprop(instance, prop)
}

/**
 * has_method(object: instance, name: string)
 *
 * returns true if class of the instance has the method name or
 * false if not
 */
def has_method(instance, method) {
  return _reflect.hasmethod(instance, method)
}

/**
 * get_method(object: instance, name: string)
 *
 * returns the method in a class instance matching the given name
 * or nil if the class of the instance contains no method with
 * a matching name
 */
def get_method(instance, method) {
  return _reflect.getmethod(instance, prop)
}

def call_method(instance, method, ...) {
  return _reflect.callmethod(instance, method, __args__)
}