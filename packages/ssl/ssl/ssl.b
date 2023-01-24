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
   * connect()
   * 
   * connects to an SSL server instance
   */
  accept() {
    return _ssl.connect(self._ptr)
  }

  /**
   * write(data: string | bytes)
   * 
   * writes data to the current I/O stream.
   * @return int representing the total bytes written
   */
  write(data) {
    if !is_string(data) and !is_bytes(data)
      die Exception('string or bytes expected')

    if is_string(data) data = data.to_bytes()

    var result = _ssl.write(self._ptr, data)
    if result == -1
      die Exception(self.error())
    
    return result
  }

  /**
   * read([length: int])
   * 
   * reads data off the I/O and returns it
   * @default length = -1
   * @return string
   */
  read(length) {
    if !length length = -1
    if !is_int(length)
      die Exception('integer expected')
    
    var result = _ssl.read(self._ptr, length)
    if result == nil {
      die Exception(self.error())
    }

    return result
  }

  /**
   * error([code: int])
   * 
   * returns the last SSL error number
   * @return int
   */
  error(code) {
    if !code code = -1
    return _ssl.error_string(self._ptr, code)
  }

  /**
   * shutdown()
   * 
   * shutdown the SSL object
   */
  shutdown() {
    _ssl.shutdown(self._ptr)
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
