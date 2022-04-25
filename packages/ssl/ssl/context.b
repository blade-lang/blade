#!-- part of the ssl module

import reflect
import _ssl

/**
 * SSL context representation class
 */
class SSLContext {

  /**
   * SSLContext(method: ptr)
   * @constructor
   * @note method must be a valid SSL method pointer
   */
  SSLContext(method) {
    if !reflect.is_ptr(method)
      die Exception('SSL method expected')
    self._method = method
    self._ptr = _ssl.ctx(method)
  }

  /**
   * set_verify(mode: int)
   * 
   * sets the verification flags for ctx to be the given mode.
   * @note The verification of certificates can be controlled by a set of logically or'ed mode flags.
   * @note If the mode is SSL_VERIFY_NONE none of the other flags may be set.
   */
  set_verify(mode) {
    if !is_int(mode)
      die Exception('integer expected')
    _ssl.ctx_set_verify(self._ptr, mode)
  }

  /**
   * set_verify_locations(locations: string)
   * 
   * set default locations for trusted CA certificates
   */
  set_verify_locations(locations) {
    if !is_string(locations)
      die Exception('location path expected')
    _ssl.ctx_set_verify_locations(self._ptr, locations)
  }

  /**
   * load_certs(cert_file: string | file, private_key_file: string | file)
   * 
   * loads the given SSL/TLS certificate pairs for the given SSL/TLS context.
   * @return bool
   */
  load_certs(cert_file, private_key_file) {
    if !is_string(cert_file) and !is_file(cert_file)
      die Exception('cert_file must be a string or file')
    if !is_string(private_key_file) and !is_file(private_key_file)
      die Exception('private_key_file must be a string or file')

    if is_file(cert_file) cert_file = cert_file.abs_path()
    if is_file(private_key_file) private_key_file = private_key_file.abs_path()

    return _ssl.load_certs(self._ptr, cert_file, private_key_file)
  }

  /**
   * set_ciphers(ciphers: string)
   * 
   * sets the list of allowed ciphers. This list must be colon (:) separated.
   * @return bool
   */
  set_ciphers(ciphers) {
    if !is_string(ciphers)
      die Exception('string expected')
    return _ssl.set_ciphers(self._ptr, ciphers)
  }

  /**
   * free()
   * 
   * frees this Context and all associated resources
   */
  free() {
    _ssl.ctx_free(self._ptr)
  }

  /**
   * get_pointer()
   * 
   * returns the raw OpenSSl SSL_CTX pointer
   * @return ptr
   */
  get_pointer() {
    return self._ptr
  }
}


import .constants

/**
 * TLSContext is a specialized Context providing generic TLS support 
 * for both client and server mode.
 */
class TLSContext < SSLContext {

  /**
   * TLSContext()
   * @constructor
   */
  TLSContext() {
    parent(TLS_method)
  }
}



/**
 * TLSClientContext is a specialized Context for supporting TLS clients.
 */
class TLSClientContext < SSLContext {

  /**
   * TLSClientContext()
   * @constructor
   */
  TLSClientContext() {
    parent(TLS_client_method)
  }
}



/**
 * TLSServerContext is a specialized Context for supporting TLS servers.
 */
class TLSServerContext < SSLContext {

  /**
   * TLSServerContext()
   * @constructor
   */
  TLSServerContext() {
    parent(TLS_server_method)
  }
}

/**
 * SSLv23Context is a specialized Context providing generic SSLv23 support 
 * for both client and server mode.
 */
class SSLv23Context < SSLContext {

  /**
   * SSLv23Context()
   * @constructor
   */
  SSLv23Context() {
    parent(SSLv23_method)
  }
}



/**
 * SSLv23ClientContext is a specialized Context for supporting SSLv23 clients.
 */
class SSLv23ClientContext < SSLContext {

  /**
   * TLSClientContext()
   * @constructor
   */
  SSLv23ClientContext() {
    parent(SSLv23_client_method)
  }
}



/**
 * SSLv23ServerContext is a specialized Context for supporting SSLv23 servers.
 */
class SSLv23ServerContext < SSLContext {

  /**
   * SSLv23ServerContext()
   * @constructor
   */
  SSLv23ServerContext() {
    parent(SSLv23_server_method)
  }
}
