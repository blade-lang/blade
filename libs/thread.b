/**
 * @module threads
 * 
 * @copyright 2024, Ore Richard Muyiwa and Blade contributors
 */

import _thread

import reflect

var _MIN_STACK_SIZE = 8 * 1024
var _DEFAULT_STACK_SIZE = 64 * 1024

class Thread {

  var _fn
  var _fn_arity

  var _args
  var _name = 'blade'
  var _size = _DEFAULT_STACK_SIZE

  var _ptr

  Thread(fn) {
    if !is_function(fn)
      raise Exception('function(1..) expected, ${typeof(fn)} given')

    self._fn = fn
    self._fn_arity = reflect.get_function_metadata(fn).arity
  }

  start(...) {
    self.start_from_list(__args__)
  }

  start_from_list(args) {
    if !is_list(args)
      raise Exception('list expected, ${typeof(args)} given')

    self._args = args

    # insert thread itself as the first argument
    args = [self] + args

    # only pass on arguments that the run function is able to accept.
    args = args[0,self._fn_arity]

    self._ptr = _thread.start(
      self._fn,
      args, 
      self._size
    )
  }

  dispose() {
    if self._ptr {
      _thread.dispose(self._ptr)
    }
  }

  await() {
    if self._ptr {
      _thread.await(self._ptr)
    }
  }

  try_await() {}

  sleep(time) {}

  cancel() {}

  yield() {}

  set_name(name) {
    if !is_string(name)
      raise Exception('string expected, ${typeof(name)} given')
    
    if name.length() < 0 or name.length() > 16 {
      raise Exception('string length must be between 1 and 16')
    }

    self._name = name
    _thread.set_name(name)
  }

  get_name() {
    return self._name
  }

  set_stack_size(size) {
    if !is_number(size)
      raise Exception('number expected, ${typeof(size)} given')

    if size < _MIN_STACK_SIZE
      raise Exception('min size if 8kiB')

    self._size = size
  }

  get_stack_size() {
    return self._size
  }

  get_id() {
    return _thread.get_id()
  }

  is_alive() {}
}


def start(function, args) {
  if args == nil args = []

  # we're deliberately not checking the arguments here 
  # because the thread initializer and start_from_list function 
  # will take care of that on their own.

  var thread = Thread(function)
  thread.start_from_list(args)
  return thread
}
