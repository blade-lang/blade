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
