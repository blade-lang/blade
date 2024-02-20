#!-- part of the ssl module

import reflect
import _ssl

/**
 * SSL context representation class
 */
class SSLContext {

  /**
   * @note Method must be a valid SSL method pointer.
   * @param ptr method
   * @constructor
   */
  SSLContext(method) {
    if !reflect.is_ptr(method)
      die Exception('SSL method expected')
    self._method = method
    self._ptr = _ssl.ctx(method)
  }

  /**
   * Sets the verification flags for ctx to be the given mode.
   * 
   * @note The verification of certificates can be controlled by a set of logically or'ed mode flags.
   * @note If the mode is SSL_VERIFY_NONE none of the other flags may be set.
   * @param int mode
   */
  set_verify(mode) {
    if !is_int(mode)
      die Exception('integer expected')
    _ssl.ctx_set_verify(self._ptr, mode)
  }

  /**
   * Sets the default locations for trusted CA certificates.
   * 
   * @param string locations
   */
  set_verify_locations(locations) {
    if !is_string(locations)
      die Exception('location path expected')
    _ssl.ctx_set_verify_locations(self._ptr, locations)
  }

  /**
   * Loads the given SSL/TLS certificate pairs for the given SSL/TLS context.
   * 
   * @param {string|file} cert_file
   * @param {string|file} private_key_file
   * @return bool
   */
  load_certs(cert_file, private_key_file) {
    if !is_string(cert_file) and !is_file(cert_file)
      die Exception('cert_file must be a string or file')
    if private_key_file != nil and !is_string(private_key_file) and !is_file(private_key_file)
      die Exception('private_key_file must be a string or file')
    if !private_key_file private_key_file = cert_file

    if is_file(cert_file) cert_file = cert_file.abs_path()
    if is_file(private_key_file) private_key_file = private_key_file.abs_path()

    return _ssl.load_certs(self._ptr, cert_file, private_key_file)
  }

  /**
   * Sets the list of allowed ciphers. This list must be colon (:) separated.
   * 
   * @param string ciphers
   * @return bool
   */
  set_ciphers(ciphers) {
    if !is_string(ciphers)
      die Exception('string expected')
    return _ssl.set_ciphers(self._ptr, ciphers)
  }

  /**
   * Frees this Context and all associated resources
   */
  free() {
    _ssl.ctx_free(self._ptr)
  }

  /**
   * Returns the raw OpenSSl SSL_CTX pointer.
   * 
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
   * @constructor
   */
  SSLv23ServerContext() {
    parent(SSLv23_server_method)
  }
}
