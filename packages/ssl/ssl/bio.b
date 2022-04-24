#!-- part of the ssl module

import _ssl
import reflect
import .ssl { SSL }
import .constants

var _close_opts = [constants.BIO_NOCLOSE, constants.BIO_CLOSE]

/**
 * SSL Binary Input/Output
 */
class BIO {

  # SSL tracker
  var _ssl

  /**
   * BIO(method: ptr)
   * @constructor
   * @note method must be a valid SSL BIO_* method
   */
  BIO(method) {
    if !reflect.is_ptr(method)
      die Exception('SSL BIO method expected')
    self._ptr = _ssl.new_bio(method)
  }

  /**
   * set_ssl(ssl: SSL [, option: int])
   * 
   * sets the working SSL instance for this BIO
   * @note option must be one of the BIO constants if given.
   * @note default option = BIO_NOCLOSE
   */
  set_ssl(ssl, option) {
    if !instance_of(ssl, SSL)
      die Exception('instance of SSL expected')

    if !option option = constants.BIO_CLOSE
    if !is_int(option)
      die Exception('option must be a BIO_* constant')

    self._ssl = ssl
    _ssl.set_ssl(self._ptr, ssl.get_pointer(), option)
  }

  /**
   * set_conn_hostname(name: string)
   * 
   * sets the hostname for the current connected BIO socket
   */
  set_conn_hostname(name) {
    if !is_string(name)
      die Exception('string expected')
    _ssl.set_conn_hostname(self._ptr, name)
  }

  /**
   * set_accept_name(name: string)
   * 
   * sets the address name for the current accepted BIO socket
   */
  set_accept_tname(name) {
    if !is_string(name)
      die Exception('string expected')
    _ssl.set_accept_name(self._ptr, name)
  }

  /**
   * set_conn_address(address: string)
   * 
   * sets the address for the current connected BIO socket
   */
  set_conn_address(address) {
    if !is_string(address)
      die Exception('string expected')
    _ssl.set_conn_address(self._ptr, address)
  }

  /**
   * set_conn_port(port: int | string)
   * 
   * sets the port for the current connected BIO socket
   */
  set_conn_port(port) {
    if is_int(port) port = '${port}'

    if !is_string(port)
      die Exception('integer or string expected')

    _ssl.set_conn_port(self._ptr, port)
  }

  /**
   * set_accept_port(port: int | string)
   * 
   * sets the port for the current accepted BIO socket
   */
  set_accept_port(port) {
    if is_int(port) port = '${port}'

    if !is_string(port)
      die Exception('integer or string expected')

    _ssl.set_accept_port(self._ptr, port)
  }

  /**
   * set_conn_family(family: int)
   * 
   * sets the socket family for the current connected BIO socket
   */
  set_conn_family(family) {
    if !is_int(family)
      die Exception('integer expected')

    _ssl.set_conn_family(self._ptr, family)
  }

  /**
   * set_accept_family(family: int)
   * 
   * sets the socket family for the current accepted BIO socket
   */
  set_accept_family(family) {
    if !is_int(family)
      die Exception('integer expected')

    _ssl.set_accept_family(self._ptr, family)
  }

  /**
   * get_conn_hostname()
   * 
   * returns the hostname for the current connected BIO socket
   * @return string
   */
  get_conn_hostname() {
    return _ssl.get_conn_hostname(self._ptr)
  }

  /**
   * get_accept_name()
   * 
   * returns the hostname for the current accepted BIO socket
   * @return string
   */
  get_accept_name() {
    return _ssl.get_accept_name(self._ptr)
  }

  /**
   * get_conn_address()
   * 
   * returns the address for the current connected BIO socket
   * @return string
   */
  get_conn_address() {
    return _ssl.get_conn_address(self._ptr)
  }

  /**
   * get_conn_port()
   * 
   * returns the port for the current connected BIO socket
   * @return string
   */
  get_conn_port() {
    return to_int(_ssl.get_conn_port(self._ptr))
  }

  /**
   * get_accept_port()
   * 
   * returns the port for the current accepted BIO socket
   * @return string
   */
  get_accept_port() {
    return to_int(_ssl.get_accept_port(self._ptr))
  }

  /**
   * get_conn_family()
   * 
   * returns the family for the current connected BIO socket
   * @return int
   */
  get_conn_family() {
    return _ssl.get_conn_family(self._ptr)
  }

  /**
   * get_accept_family()
   * 
   * returns the family for the current accepted BIO socket
   * @return int
   */
  get_accept_family() {
    return _ssl.get_accept_family(self._ptr)
  }

  /**
   * get_fd()
   * 
   * returns the current socket file descriptor.
   * It returns `-1` on failure or a positive integer on success.
   * @return number
   */
  get_fd() {
    return _ssl.bio_get_fd(self._ptr)
  }

  /**
   * set_fd(fd: int [, opt: int])
   * 
   * sets the socket file descriptor for this BIO
   * @default opt = BIO_NOCLOSE
   */
  set_fd(fd, opt) {
    if !is_int(fd)
      die Exception('fd must be an integer')
    if opt != nil and !_close_opts.contains(fd)
      die Exception('opt must be one of BIO_CLOSE or BIO_NOCLOSE')

    if !opt opt = constants.BIO_NOCLOSE

    return _ssl.bio_set_fd(self._ptr, fd, opt)
  }

  /**
   * set_non_blocking([b: bool])
   * 
   * converts the BIO into a non-blocking I/O stream if b is `true`, otherwise 
   * converts it into a blocking stream.
   * @default true
   */
  set_non_blocking(b) {
    if !b b = true

    if !is_bool(b)
      die Exception('boolean expected')

    _ssl.set_nbio(self._ptr, b)
  }

  /**
   * push(b: BIO)
   * 
   * it appends b, which may be a single BIO or a chain of BIOs, 
   * to the current BIO stack (unless the current pinter is `nil`). 
   * It then makes a control call on BIO b and returns it.
   */
  push(b) {
    if !instance_of(b, BIO)
      die Exception('instance of BIO expected')
    if b {
      _ssl.push(self._ptr, b.get_pointer())
    }
    return self
  }

  /**
   * removes this BIO from any chain is is part of
   */
  pop() {
    _ssl.pop(self._ptr)
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

    if is_bytes(data) data = to_string(data)

    var result = _ssl.bio_write(self._ptr, data)
    if result == -1
      die Exception(self.error_string())
    
    return result
  }

  /**
   * read([length: int])
   * 
   * reads data off the I/O and returns it
   * @default length = 1024
   * @return string
   */
  read(length) {
    if !length length = 1024
    if !is_int(length)
      die Exception('integer expected')
    
    var result = _ssl.bio_read(self._ptr, length)
    if result == nil {
      die Exception(self.error_string())
    }

    return result
  }

  /**
   * should_retry()
   * 
   * returns `true` if this BIO needs to retry its last operation. 
   * `false` otherwise.
   */
  should_retry() {
    return _ssl.should_retry(self._ptr)
  }

  /**
   * do_connect()
   * 
   * attempts to establish a connection to the host.
   */
  do_connect() {
    return _ssl.do_connect(self._ptr)
  }

  /**
   * do_accept()
   * 
   * attempts to accept the connected socket.
   */
  do_accept() {
    return _ssl.do_accept(self._ptr)
  }

  /**
   * error([code: int])
   * 
   * returns the last SSL error number
   * @return int
   */
  error(code) {
    if !code code = -1
    return _ssl.error(self._ptr, code)
  }

  /**
   * error_string([code: int])
   * 
   * returns the last SSL error as string
   * @return string
   */
  error_string() {
    if !code code = -1
    return _ssl.error_string(self._ptr, code)
  }

  /**
   * free()
   * 
   * frees this BIO and all associated resources
   */
  free() {
    _ssl.free(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * returns the raw OpenSSl BIO pointer
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }
}





/**
 * SSLBIO is a generic BIO for SSL I/O
 */
class SSLBIO < BIO {

  /**
   * ConnectBIO()
   * @constructor
   */
  SSLBIO() {
    parent(constants.BIO_f_ssl)
  }
}



/**
 * ConnectBIO is a generic BIO for new secured connections
 */
class ConnectBIO < BIO {

  /**
   * ConnectBIO()
   * @constructor
   */
  ConnectBIO() {
    parent(constants.BIO_s_connect)
  }
}



/**
 * AcceptedBIO is a generic BIO for accepting new secured 
 * connections from a TLS server
 */
class AcceptedBIO < BIO {

  /**
   * AcceptedBIO()
   * @constructor
   */
  AcceptedBIO() {
    parent(constants.BIO_s_accept)
  }
}
