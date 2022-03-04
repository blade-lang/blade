#!-- part of the ssl module

import _ssl
import .context { SSLContext }


/**
 * SSL interface class
 */
class SSL {

  /**
   * SSL(context: SSLContext)
   * @constructor
   */
  SSL(context) {
    if !instance_of(context, SSLContext)
      die Exception('instance of SSLContext expected')
    self._context = context
    self._ptr = _ssl.new(context.get_pointer())
  }

  /**
   * set_connect_state()
   * 
   * puts this SSL instance in the connected mode.
   */
  set_connect_state() {
    _ssl.set_connect_state(self._ptr)
  }

  /**
   * set_accept_state()
   * 
   * puts this SSL instance in the accept mode.
   */
  set_accept_state() {
    _ssl.set_accept_state(self._ptr)
  }

  /**
   * get_fd()
   * 
   * returns the current socket file descriptor.
   * It returns `-1` on failure or a positive integer on success.
   * @return number
   */
  get_fd() {
    return _ssl.get_fd(self._ptr)
  }

  /**
   * set_fd(fd: int)
   * 
   * sets the socket file descriptor for this SSL
   */
  set_fd(fd) {
    if !is_int(fd)
      die Exception('fd must be an integer')

    return _ssl.set_fd(self._ptr, fd)
  }

  /**
   * accept()
   * 
   * begins accepting data on SSL
   */
  accept() {
    return _ssl.accept(self._ptr)
  }

  /**
   * free()
   * 
   * frees this SSL and all associated resources
   */
  free() {
    _ssl.ssl_free(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * returns the raw OpenSSl SSL pointer
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }
}
