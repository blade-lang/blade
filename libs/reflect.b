/**
 * @module reflect
 * 
 * This module provides many functions that can be used to interact with or modify modules, 
 * classes and functions. It is well suited for many uses cases such as creating a library 
 * that is heavily dependent on decorators (e.g. the `json` module).
 * 
 * ### For example,
 * 
 * We can call a decorator using the `reflect` module like this.
 * 
 * ```blade
 * class A {
 *   @custom_decorator() {
 *     echo 'It works!'
 *   }
 * }
 * 
 * import reflect
 * 
 * var instance_of_a = A()
 * var decorator = reflect.get_decorator(instance_of_a, 'custom_decorator')
 * 
 * # It's always good to check the result first as it will be a good 
 * # practice to make decorators optional to make it easy for users to 
 * # opt-in and opt-out of features your package or library provide.
 * if decorator {
 *   decorator()
 * }
 * ```
 * 
 * Try it out!
 * 
 * @copyright 2021, Richard Ore and Blade contributors
 */

import _reflect

/**
 * Returns `true` if instance has the property or module has a value with 
 * the given name or `false` if not.
 * 
 * @param instance|module object
 * @param string name
 * @returns bool
 */
def has_prop(object, name) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  return _reflect.hasprop(object, name)
}

/**
 * Returns the property of the instance or value in the module matching the 
 * given name or nil if the object contains no property with a matching name.
 * 
 * @param instance|module object
 * @param string name
 * @returns any
 */
def get_prop(object, name) {
  if !is_instance(object) and typeof(object) != 'module'
    raise Exception('object instance or module expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  return _reflect.getprop(object, name)
}

/**
 * Returns all properties of an instance or value in a module or an empty
 * list if the instance or module has no property.
 *
 * @param instance|module object
 * @param string name
 * @returns list[string]
 */
def get_props(object) {
  if !is_instance(object) and typeof(object) != 'module'
    raise Exception('object instance or module expected in argument 1 (object)')

  return _reflect.getprops(object)
}

/**
 * Sets the named property of the object to value.
 * 
 * @note if the property already exist, it overwrites it.
 * 
 * @param instance object
 * @param string name
 * @param any value
 * @returns bool: `true` if a new property was set, `false` if a property was updated
 */
def set_prop(object, name, value) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  return _reflect.setprop(object, name, value)
}

/**
 * Deletes the named property from the instance
 * 
 * @param instance|module object
 * @param string name
 * @returns bool
 */
def del_prop(object, name) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  return _reflect.delprop(object, name)
}

/**
 * Returns true if class of the instance has the method name or
 * false if not.
 * 
 * @param instance object
 * @param string name
 * @returns bool
 */
def has_method(object, name) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  return _reflect.hasmethod(object, name)
}

/**
 * Returns true if class of the instance implements the decorator name or
 * false if not.
 * 
 * @param instance object
 * @param string name
 * @returns bool
 */
def has_decorator(object, name) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  return _reflect.hasmethod(object, '@${name}')
}

/**
 * Returns the method in a class instance matching the given name
 * or nil if the class of the instance contains no method with
 * a matching name.
 * 
 * @param instance object
 * @param string name
 * @returns function
 */
def get_method(object, name) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  return _reflect.getmethod(object, name)
}

/**
 * Returns the decorator function matching the given name in the class 
 * of the given instance.
 * @note the name of a decorator excludes the `@` character.
 * 
 * @param instance object
 * @param string name
 * @returns function
 */
def get_decorator(object, name) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_string(name)
    raise Exception('string expected in argument 2 (name)')

  if _reflect.hasmethod(object, '@${name}') {
    return _reflect.getboundmethod(object, '@${name}')
  } else {
    raise Exception("class ${typeof(object)} does not implement decorator '${name}'")
  }
}


/**
 * Binds the given function to the instance, allowing you to access 
 * the instance itself in the function via the `self` keyword in 
 * the function.
 * 
 * @param instance object
 * @param function method
 * @returns function
 */
def bind_method(object, method) {
  if !is_instance(object)
    raise Exception('object instance expected in argument 1 (object)')
  if !is_function(method)
    raise Exception('function expected in argument 2 (method)')

  return _reflect.bindmethod(object, method)
}

/**
 * Returns the type of an instance as string
 * 
 * @param instance object
 * @returns string
 */
def get_type(object) {
  if !is_instance(object)
    raise Exception('instance expected in argument 1 (object)')

  return _reflect.gettype(object)
}

/**
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
 * 
 * @param function object
 * @returns dictionary
 */
def get_function_metadata(function) {
  if !is_function(function)
    raise Exception('function expected in argument 1 (function)')

  return _reflect.getfunctionmetadata(function)
}

/**
 * Returns the metadata of a class as a dictionary. 
 * This dictionary contains the following keys:
 * 
 * - `name`: The name of the class.
 * - `properties`: a list of the name of non-static properties defined in the class
 * - `static_properties`: a list of the name of static properties defined in the class
 * - `methods`: a list of the name of methods defined in the class
 * - `superclass`: The name of the class it inherits from.
 * 
 * @param class klass
 * @returns dictionary
 */
def get_class_metadata(klass) {
  if !is_class(klass)
    raise Exception('class expected in argument 1 (klass)')

  return _reflect.getclassmetadata(klass)
}

/**
 * Returns the metadata of an imported module as a dictionary. 
 * This dictionary contains the following keys:
 * 
 * - `name`: The name of the module.
 * - `file`: The file from which the module was imported.
 * - `has_preloader`: `true` if the module is a C extension with a preloader and `false` otherwise.
 * - `has_unloader`: `true` if the module is a C extension with a unloader and `false` otherwise.
 * - `definitions`: A list of the name of objects defined in the module.
 * 
 * @param module module
 * @returns dictionary
 */
def get_module_metadata(module) {
  return _reflect.getmodulemetadata(module)
}

/**
 * Returns the class value of an instance as an object that can be 
 * used to create a new instance of that same class.
 * 
 * @param instance object
 * @returns class
 */
def get_class(object) {
  if !is_instance(object)
    raise Exception('instance expected in argument 1 (object)')

  return _reflect.getclass(object)
}

/**
 * Returns `true` if _value_ is a pointer, `false` otherwise.
 * 
 * @param any value
 * @returns bool
 */
def is_ptr(value) {
  return _reflect.isptr(value)
}

/**
 * Returns a pointer to the given value.
 * 
 * @param any value
 * @returns ptr
 */
def get_ptr(value) {
  return _reflect.getptr(value)
}

/**
 * Sets the value at the given pointer's address to the given value.
 * 
 * @param ptr pointer
 * @param any value
 */
def set_ptr(pointer, value) {
  _reflect.setptrvalue(pointer, value)
}

/**
 * Returns a the address of the pointer to the value in memory.
 * 
 * @param any value
 * @returns ptr
 */
def get_address(value) {
  return _reflect.getaddress(value)
}

/**
 * Returns a pointer to the given memory address.
 * 
 * @param number address
 * @returns ptr
 */
def ptr_from_address(address) {
  return _reflect.ptrfromaddress(address)
}

/**
 * Sets any given value as globally accessible in all modules, function
 * and scopes with the given name.
 *
 * If name is not given and the value is a class or function, the name
 * will automatically be set to the name of the class or function
 * respectively otherwise, an Exception will be raised.
 * 
 * @param any value
 * @param string? name
 */
def set_global(value, name) {
  if typeof(value) == 'module'
    raise Exception('modules cannot be set as global')
  if name != nil and !is_string(name)
    raise Exception('string expected in argument 2 (name)')
  _reflect.setglobal(value, name)
}

/**
 * Runs the content of a given script in-place as if it were part of the 
 * current module.
 * 
 * @param string path
 */
def run_script(path) {
  if !is_string(path)
    raise Exception('string expected in argument 1 (path)')

  var fh = file(path)
  if !fh.exists()
    raise Exception('cannot find script at "${path}"')

  var content = fh.read()

  # for now, returning the call without assigning to a variable is failing.
  _reflect.runscript(path,  content)
}

/**
 * Calls a function with the given arguments.
 * 
 * @param function function
 * @param list args
 * @returns any
 */
def call_function(function, args) {
  return _reflect.callfunction(function, args)
}
