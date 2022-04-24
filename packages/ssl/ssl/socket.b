#!-- part of the ssl module

import .context { SSLContext }
import .ssl { SSL }
import .bio { SSLBIO, ConnectBIO }
import .constants { TLS_method }
import _socket
import socket { Socket, SOCK_STREAM, IPPROTO_TCP, AF_INET }


/**
 * SSLSocket is an SSL/TLS enabled socket.
 * @extends Socket
 */
class SSLSocket < Socket {

  # BIO tracker.
  var _bio

  /**
   * SSLSocket(method: ptr)
   * @constructor
   * @note method must be a valid method pointer defined in the <em>ssl</em> module
   */
  SSLSocket(method, _id, _ctx) {
    # NOTE: NEVER EVER SET `_id` and `_ctx` YOURSELF.
    # The parameter is meant to make `accept()`.

    if method {
      self.id = _socket.create(AF_INET, SOCK_STREAM, IPPROTO_TCP)
      self._ctx = SSLContext(method)

      # initialize the SSL.
      self._ssl = SSL(self._ctx)
  
      # use the ssl inside an SSL BIO.
      self._ssl_bio = SSLBIO()
      self._ssl_bio.set_fd(self.id)
      self._ssl_bio.set_ssl(self._ssl)
    } else {

      if _id != nil and !is_int(_id)
        die Exception('_id must be an integer')
      if _ctx != nil and !instance_of(_ctx, SSLContext)
        die Exception('_ctx must be an instance of SSLContext')

      self.id = _id
      self._ctx = _ctx

      # initialize the SSL.
      self._ssl = SSL(self._ctx)
      # if !self._ssl.accept()
      #   die Exception('failed to accept connection')
  
      # use the ssl inside an SSL BIO.
      self._ssl_bio = SSLBIO()
      self._ssl_bio.set_fd(self.id)
      self._ssl_bio.set_ssl(self._ssl)

      if !self._ssl_bio.do_accept()
        die Exception('failed to accept connection')
        
      self._bio = self._ssl_bio
    }
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

    self.is_client = self.is_connected = self._bio.do_connect() == 1
    if self.is_connected {
      self.is_listening = false
      self.is_bound = false
    }
    return self.is_connected
  }

  /**
   * accept()
   * 
   * Accepts a connection on a socket
   * 
   * This method extracts the first connection request on the queue of pending connections, creates a new socket 
   * with the same properties of the current socket, and allocates a new file descriptor for the socket.  If no 
   * pending connections are present on the queue, and the socket is not marked as non-blocking, accept() blocks 
   * the caller until a connection is present.  If the socket is marked non-blocking and no pending connections 
   * are present on the queue, accept() returns an error as described below.  
   * 
   * @note The accepted socket may not be used to accept more connections.  
   * @note The original socket socket, remains open.
   * @return Socket
   */
  accept() {
    self._ssl.set_accept_state()
    if self.is_bound and self.is_listening and !self.is_closed {
      var result = _socket.accept(self.id)

      if result and result != -1  {
        var socket = SSLSocket(nil, result[0], self._ctx)
        socket.is_client = true
        socket.is_connected = true
        return socket
      }
    }
    die SocketException('socket not bound/listening')
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

    if !length length = 1024
    return self._bio.read(length)
  }

  /**
   * read([length: int])
   * 
   * Reads bytes of the given length from the socket. If the length is not given, it default length of 
   * -1 indicating that the total available data on the socket stream will be read. 
   * 
   * > Unlike with plain `Socket`, this is basically a wrapper for the `receive()` method.
   * @default Length = 1024
   * @return string
   */
  read(length) {
    return self.receive(length, 0)
  }

  /**
   * close()
   * 
   * Closes the socket
   * @return bool
   */
  close() {
    parent.close()
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
