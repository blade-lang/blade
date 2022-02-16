#!-- part of the ssl module

import .context { SSLContext }
import .ssl { SSL }
import .bio { SSLBIO, ConnectBIO }
import .constants { TLS_method }


/**
 * SSLSocket class provides socket module's Socket class emulation 
 * for SSL for easy interoperability from clients supporting client, 
 * allowing a plug-and-play support for secured connections.
 */
class SSLSocket {

  # BIO tracker.
  var _bio

  # client/server toggle
  var _is_client = true

  /**
   * SSLSocket(method: ptr)
   * @constructor
   * @note method must be a valid method pointer defined in the <em>ssl</em> module
   */
  SSLSocket(method) {
    self._ctx = SSLContext(method)

    # initialize the SSL.
    self._ssl = SSL(self._ctx)

    # use the ssl inside an SSL BIO.
    self._ssl_bio = SSLBIO()
    self._ssl_bio.set_ssl(self._ssl)
  }

  /**
   * connect(host: string, port: int [, timeout: int])
   * 
   * Initiates a connection to the given host on the specified port. If host is `nil`, it will 
   * connect on to the current hostn specified on the socket.
   * 
   * @default timeout = 300,000ms (i.e. 300 seconds)
   * @return bool
   */
  connect(host, port, timeout) {
    self._ssl.set_connect_state()
    var bio = ConnectBIO()
    bio.set_conn_hostname('${host}:${port}')

    if bio.get_conn_hostname() != host
      die Exception('hostname verification failed')

    bio.set_non_blocking(true)
    self._bio = self._ssl_bio.push(bio)

    return self._bio.do_connect() == 1
  }

  /**
   * send(message: string | file | bytes, flags: int)
   * 
   * Sends the specified message to the socket. When this methods accepts a file as a message, 
   * the file is read and the resultant bytes of the file content is streamed to the socket.
   * 
   * @note the flags parameter is currently redundant and is kept only to remanin compatible with future plans for this method.
   * @return number greater than -1 if successful indicating the total number of bytes sent or -1 if it fails.
   */
  send(message, flags) {
    return self._bio.write(message)
  }

  /**
   * receive([length: int [, flags: int]])
   * 
   * Receives bytes of the given length from the socket. If the length is not given, it default length of 
   * -1 indicating that the total available data on the socket stream will be read. 
   * If no data is available for read on the socket, the socket will wait to receive data or until the 
   * `receive_timeout` which is also equal to the `SO_RCVTIMEO` setting of the socket has elapsed before or 
   * until it has received the total number of bytes required (whichever comes first).
   * 
   * @note the flags parameter is currently redundant and is kept only to remanin compatible with future plans for this method.
   * @return string
   */
  receive(length, flags) {
    if length != nil and !is_int(length)
      die Exception('integer expected')

    var data = self._bio.read(10240)
    if length and data.length() > length {
      return data[,length]
    }
    return data
  }

  /**
   * close()
   * 
   * Closes the socket
   * @return bool
   */
  close() {
    self._bio.free()
    self._ctx.free()
  }

  /**
   * get_context()
   * 
   * returns the underlying SSLContext instance
   * @return SSLContext
   */
  get_context() {
    return self._ctx
  }
}



/**
 * TLSSocket is the generic TLS SSL Socket
 */
class TLSSocket < SSLSocket {
  
  /**
   * TLSSocket()
   * @constructor
   */
  TLSSocket() {
    parent(TLS_method)
  }
}
