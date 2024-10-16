/**
 * @module threads
 * 
 * The thread module provides functionality for creating and 
 * controlling threads. With the thread module, you can create 
 * applications that perform multiple operations at the same 
 * time. 
 * 
 * Operations that can potentially block/hold up other operations 
 * can be executed on separate threads.
 * 
 * As the workload of your application increases, the need to 
 * offload long tasks to a separate thread of execution becomes 
 * aparent when creating scalable applications.
 * 
 * ## Create and start a new thread
 * 
 * ## Stopping a thread
 * 
 * ## Awaiting a thread
 * 
 * ## Renaming a thread
 * 
 * @copyright 2024, Ore Richard Muyiwa and Blade contributors
 */

import _thread

import reflect
import os

var _MIN_STACK_SIZE = 16384 # 16kb
var _DEFAULT_STACK_SIZE = 65536 # 64kb

var _main_thread_id = _thread.get_id()

def _is_not_main_thread(id) {id
  return id != _main_thread_id
}

/**
 * The thread class exposes methods to manage creating, running, 
 * and controlling threads.
 * 
 * @class
 */
class Thread {

  var _delegate
  var _delegate_arity

  var _args
  var _name
  var _size = _DEFAULT_STACK_SIZE

  var _ptr

  var _started = false
  var _joined = false
  var _detached = false

  /**
   * The delegate function passed to the constructor may accept zero 
   * or more parameters. When it accepts no parameter, the function 
   * will be called without any argument when run otherwise, it will 
   * be called with as many argument as it can receive.
   * 
   * When a delegate accepts arguments, the first argument passed 
   * will always be the thread object itself followed by the arguments 
   * it received from start.
   * 
   * For example, in the following thread execution, the first 
   * parameter _t_ in the delegate will receive the thread object 
   * itself.
   * 
   * ```blade
   * var th = Thread(@(t) {
   *   echo t.get_id()
   * })
   * 
   * th.start(21)
   * ```
   * 
   * The delegate function doesn't raise an exception because parameter 
   * _t_ never received the `start()` argument but the thread itself. 
   * In the next example, the function accepts the start argument. Note 
   * that the start argument was received starting from the second 
   * argument.
   * 
   * ```blade
   * var th = Thread(@(t, balance) {
   *   echo balance
   * })
   * 
   * th.start(21)
   * ```
   * 
   * The optional second parameter allows us to set the size of the stack
   * used for the thread when started.
   * 
   * @param function delegate
   * @param number? stack_size
   * @constructor
   */
  Thread(delegate, stack_size) {
    if !is_function(delegate)
      raise Exception('function(1..) expected, ${typeof(delegate)} given')

    self._delegate = delegate
    self._delegate_arity = reflect.get_function_metadata(delegate).arity
    
    # set stack size if given.
    # we're falling back on the type check done by set_stack_size
    # here...
    if stack_size != nil {
      self.set_stack_size(stack_size)
    }
  }

  /**
   * Starts the thread by putting it in the running state and passing 
   * any argument to the function to the thread function itself.
   * 
   * If no argument is provided, no argument is passed to the function 
   * otherwise, arguments will be passed to the thread function only 
   * when the thread function is only able to accept them.
   * 
   * > **NOTE:** A thread can only be started once.
   * 
   * @param any... args
   */
  start(...) {
    self.start_from_list(__args__)
  }

  /**
   * Same as `start()` but takes the argument from a list instead.
   * 
   * @param list args
   */
  start_from_list(args) {
    if !is_list(args)
      raise Exception('list expected, ${typeof(args)} given')

    assert !self._started, 'a thread cannot be started more than once'

    self._args = args

    # insert thread itself as the first argument
    args = [self] + args

    # only pass on arguments that the run function is able to accept.
    args = args[0,self._delegate_arity]

    self._ptr = _thread.new(self._delegate, args)
    self._started = _thread.start(self._ptr, self._size)
  }

  /**
   * Terminates the thread by sending kill signals to the thread 
   * and freeing all associated resources afterwards. If the thread 
   * has already been cancelled, then it does nothing.
   * 
   * Returns `true` if the thread was successfully terminated and all 
   * resources freed otherwise it returns `false`.
   * 
   * @returns bool
   */
  cancel() {
    assert self._ptr, 'thread not started'

    if _thread.cancel(self._ptr) {
      self._ptr = nil
      return true
    }

    return false
  }

  /**
   * Marks this thread as a detached thread.
   * 
   * When a detached thread terminates, its resources are automatically 
   * released back to the system without the need for another thread to 
   * join with the terminated thread.
   * 
   * Once a thread is detached, it can't be awaited anymore.
   * 
   * The detached attribute merely determines the behavior of the
   * system when the thread terminates; it does not prevent the thread
   * from being terminated if the process terminates (or equivalently, if 
   * the main thread returns).
   * 
   * Either `await() or `detach()` should be called for each thread that 
   * an application creates, so that system resources for the thread can 
   * be released.  (But note that the resources of any threads for which 
   * one of these actions has not been done will be freed when the process 
   * terminates.).
   * 
   * If the thread was already detached, it simply returns `true` and 
   * does nothing.
   * 
   * @returns bool
   */
  detach() {
    assert self._ptr, 'thread not started'
    
    if !self._detached {
      self._detached = _thread.detach(self._ptr)
    }

    return self._detached
  }

  /**
   * Suspends execution until the thread terminates. If the thread 
   * has already terminated, it returns immediately.
   * 
   * Multiple threads can call `await()` at the same time, but only 
   * one of them will be actively waiting while others will remain in 
   * a suspended state until the thread exits. If `await()` is called 
   * from another cancelled thread, then this thread will remain 
   * awaitable.
   * 
   * Failure to join with a thread that is joinable (i.e., one that is
   * not detached), produces a "zombie thread".  Avoid doing this,
   * since each zombie thread consumes some system resources, and when
   * enough zombie threads have accumulated, it will no longer be
   * possible to create new threads (or processes).
   * 
   * All of the threads in an application are peers: any thread can join
   * with any other thread in the process.
   */
  await() {
    assert self._ptr, 'thread not started'

    # if the thread has been joined by another thread, then 
    # suspend execution till the thread becomes free.
    while self._joined {}

    self._joined = true
    _thread.await(self._ptr)
    self._joined = false
  }

  /**
   * Causes the current thread to sleep for the specified number of seconds.
   *  
   * @param number duration
   */
  sleep(duration) {
    if !is_number(duration)
      raise Exception('number expected, ${typeof(duration)} given')

    assert _is_not_main_thread(self.get_id()), 'cannot call from main thread'

    os.sleep(duration)
  }

  /**
   * Causes the calling thread to relinquish the CPU.
   * 
   * The thread is moved to the end of the queue for its static 
   * priority and a new thread gets to run.
   * 
   * If the calling thread is the only thread in the highest 
   * priority list at that time, it will continue to run after a 
   * call to `yield()`.
   * 
   * On success, returns `true` and otherwise `false`.
   * 
   * @returns bool
   */
  yield() {
    assert _is_not_main_thread(self.get_id()), 'cannot call from main thread'

    return _thread.yield()
  }

  /**
   * Sets the internal name for the calling thread to string 
   * value specified by name argument.
   * 
   * By default, all the threads inherit the program name. 
   * The `set_name()` function can be used to set a unique name 
   * for a thread, which can be useful for debugging 
   * multithreaded applications. The thread name should be a 
   * meaningful string, whose length is restricted to 15 
   * characters.
   * 
   * Returns `true` if successful and `false` otherwise.
   * 
   * @param string name
   * @returns bool
   */
  set_name(name) {
    if !is_string(name)
      raise Exception('string expected, ${typeof(name)} given')
    
    if name.length() < 0 or name.length() > 15 {
      raise Exception('string length must be between 1 and 15')
    }

    assert _is_not_main_thread(self.get_id()), 'cannot call from main thread'
    assert self._ptr, 'thread not started'

    self._name = name
    return _thread.set_name(self._ptr, name)
  }

  /**
   * Returns the name of the current thread.
   * 
   * @returns string
   */
  get_name() {
    assert _is_not_main_thread(self.get_id()), 'cannot call from main thread'
    assert self._ptr, 'thread not started'

    if self._name
      return self._name
    return _thread.get_name(self._ptr)
  }

  /**
   * sets the stack size attribute of the thread attributes 
   * object to the value specified in _size_.
   * 
   * The stack size attribute determines the minimum size 
   * (in bytes) that will be allocated for the thread when created.
   * 
   * Some systems have a minimum stack size and setting the 
   * number below that can lead to undefined behaviors. For 
   * this reason, the minimum stack size allowed is `16384` 
   * bytes (16kb) and the default stack size when not set is 
   * 65536 bytes (64kb). 
   * 
   * On some systems, this setting will be ignored if _size_ 
   * is not a multiple of the system page size.
   * 
   * @param number size
   */
  set_stack_size(size) {
    if !is_number(size)
      raise Exception('number expected, ${typeof(size)} given')

    if size < _MIN_STACK_SIZE
      raise Exception('min size if 8kiB')

    self._size = size
  }

  /**
   * Returns the size of the stack allocated to the thread 
   * when created.
   * 
   * @returns number
   */
  get_stack_size() {
    return self._size
  }

  /**
   * Returns the ID of the current thread.
   * 
   * @returns number
   */
  get_id() {
    return _thread.get_id()
  }

  /**
   * Returns true if the thread is started and alive (running) 
   * or false if not.
   * 
   * @returns bool
   */
  is_alive() {
    if self._ptr {
      return self._started and _thread.is_alive(self._ptr)
    }

    return false
  }
}


/**
 * Returns a new instance of Thread.
 * 
 * @param function delegate
 * @param number? stack_size
 * @see Constructor
 * @returns Thread
 */
def thread(delegate, stack_size) {
  return Thread(delegate, stack_size)
}


/**
 * Creates a new thread and automatically starts the thread 
 * using the default options and arguments.
 * 
 * @param function delegate
 * @param list args
 */
def start(delegate, args) {
  if args == nil args = []

  # we're deliberately not checking the arguments here 
  # because the thread initializer and start_from_list function 
  # will take care of that on their own.

  var thread = Thread(delegate)
  thread.start_from_list(args)
  return thread
}