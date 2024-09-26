#!-- part of the ssl module

import _ssl
import .context { SSLContext }


/**
 * SSL interface class
 */
class SSL {

  /**
   * @param {SSLContext} context
   * @constructor
   */
  SSL(context) {
    if !instance_of(context, SSLContext)
      die Exception('instance of SSLContext expected')
    self._context = context
    self._ptr = _ssl.new(context.get_pointer())
  }

  /**
   * Puts this SSL instance in the connected mode.
   */
  set_connect_state() {
    _ssl.set_connect_state(self._ptr)
  }

  /**
   * Puts this SSL instance in the accept mode.
   */
  set_accept_state() {
    _ssl.set_accept_state(self._ptr)
  }

  /**
   * Returns the current socket file descriptor.
   * It returns `-1` on failure or a positive integer on success.
   * 
   * @return number
   */
  get_fd() {
    return _ssl.get_fd(self._ptr)
  }

  /**
   * Sets the socket file descriptor for this SSL.
   * 
   * @param int fd
   * @return bool
   */
  set_fd(fd) {
    if !is_int(fd)
      die Exception('fd must be an integer')

    return _ssl.set_fd(self._ptr, fd)
  }

  /**
   * Begins accepting data on SSL and returns `true` if successful or 
   * `false` otherwise.
   * 
   * @return bool
   */
  accept() {
    return _ssl.accept(self._ptr) == 1
  }

  /**
   * Connects to an SSL server instance.
   * 
   * @return bool
   */
  connect() {
    # _ssl.set_connect_state(self._ptr)
    return _ssl.connect(self._ptr)
  }

  /**
   * Writes data to the current I/O stream and return an integer representing 
   * the total bytes written.
   * 
   * @param {string|bytes} data
   * @return int 
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
   * Reads data off the I/O and returns it. Set _length_ to -1 to read 
   * till no data is available in the stream.
   * 
   * @param int? length: Default value is -1
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
   * Returns the last SSL error number
   * 
   * @param int? code
   * @return int
   */
  error(code) {
    if !code code = -1
    return _ssl.error_string(self._ptr, code)
  }

  /**
   * Shutdown the SSL object.
   */
  shutdown() {
    _ssl.shutdown(self._ptr)
  }

  /**
   * Sets the Server Name Indication (SNI) for use by Secure Sockets 
   * Layer (SSL). This function should be called on a client SSL 
   * session before the TLS handshake for the SNI extension 
   * to be set properly.
   * 
   * @param string name
   * @return bool
   */
  set_tlsext_host_name(name) {
    return _ssl.set_tlsext_host_name(self._ptr, name)
  }

  /**
   * Frees this SSL and all associated resources.
   */
  free() {
    _ssl.ssl_free(self._ptr)
  }

  /**
   * Returns the raw OpenSSl SSL pointer.
   * 
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }
}
