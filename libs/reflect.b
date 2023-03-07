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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

  return _reflect.setprop(object, name, value)
}

/**
 * del_prop(object: instance, name: string)
 *
 * Deletes the named property from the instance
 * @return bool
 */
def del_prop(object, name) {
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    die Exception('string expected in argument 2 (name)')

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
  if !is_instance(object)
    die Exception('object instance expected in argument 1 (object)')
  if !is_function(method)
    die Exception('function expected in argument 2 (method)')

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
    die Exception('instance expected in argument 1 (object)')

  return _reflect.gettype(object)
}

/**
 * get_function_metadata(object: function)
 * 
 * Returns the metadata of a function as a dictionary. 
 * This dictionary contains the following keys:
 * 
 * - `name`: The name of the function
 * - `arity`: The number of none variable (...) arguments the function defines.
 * - `is_variadic`: If the function accepts variable arguments
 * - `captured_vars`: The number of variables captured (only greater than zero for captures).
 * - `module`: The name of the module from where the function was defined.
 * - `file`: The file in which the function was defined.
 * 
 * @note This function does not work for built-in functions
 * @return dictionary
 */
def get_function_metadata(function) {
  if !is_function(function)
    die Exception('function expected in argument 1 (function)')

  return _reflect.getfunctionmetadata(function)
}

/**
 * get_class_metadata(klass: class)
 * 
 * Returns the metadata of a class as a dictionary. 
 * This dictionary contains the following keys:
 * 
 * - `name`: The name of the class.
 * - `properties`: a list of the name of non-static properties defined in the class
 * - `static_properties`: a list of the name of static properties defined in the class
 * - `methods`: a list of the name of methods defined in the class
 * - `superclass`: The name of the class it inherits from.
 * 
 * @return dictionary
 */
def get_class_metadata(klass) {
  if !is_class(klass)
    die Exception('class expected in argument 1 (klass)')

  return _reflect.getclassmetadata(klass)
}

/**
 * get_module_metadata(module: imported module)
 * 
 * Returns the metadata of an imported module as a dictionary. 
 * This dictionary contains the following keys:
 * 
 * - `name`: The name of the module.
 * - `file`: The file from which the module was imported.
 * - `has_preloader`: `true` if the module is a C extension with a preloader and `false` otherwise.
 * - `has_unloader`: `true` if the module is a C extension with a unloader and `false` otherwise.
 * - `definitions`: A list of the name of objects defined in the module.
 * 
 * @return dictionary
 */
def get_module_metadata(module) {
  return _reflect.getmodulemetadata(module)
}

/**
 * get_class(object: instance)
 * 
 * Returns the class value of an instance as an object that can be 
 * used to create a new instance of that same class.
 * @return class
 */
def get_class(object) {
  if !is_instance(object)
    die Exception('instance expected in argument 1 (object)')

  return _reflect.getclass(object)
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

/**
 * set_global(fn: function [, name: string])
 * 
 * Sets a function as globally accessible in all modules, function and scopes.
 */
def set_global(fn, name) {
  if !is_function(fn)
    die Exception('function expected in argument 1 (fn)')
  if name != nil and !is_string(name)
    die Exception('string expected in argument 2 (name)')
  _reflect.setglobal(fn, name)
}

/**
 * run_script(path: string)
 * 
 * Runs the content of a given script in-place as if it were part of the current module.
 */
def run_script(path) {
  if !is_string(path)
    die Exception('string expected in argument 1 (path)')

  var fh = file(path)
  if !fh.exists()
    die Exception('cannot find script at "${path}"')

  var content = fh.read()
  _reflect.runscript(content)
}

/* def call_method(instance, name, ...) {
  if !is_instance(instance)
    die Exception('instance of object expected in argument 1 (instance)')
  if !is_string(name)
    die Exception('string of object expected in argument 2 (name)')

  var length = __args__.length() + 3
  _reflect.callmethod(instance, name, __args__)
  return _reflect.valueatdistance(-(length))
} */
