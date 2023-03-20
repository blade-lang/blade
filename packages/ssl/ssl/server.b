#!-- part of the ssl module

import http.request { HttpRequest }
import http.response { HttpResponse }
import http.exception { HttpException }
import http.status

import socket as so
import iters
import .constants
import .socket { TLSSocket }

/**
 * TLS server
 * @printable
 */
class TLSServer {

  /**
   * The host address to which this server will be bound
   * @default socket.IP_LOCAL (127.0.0.1)
   */
  var host = so.IP_LOCAL

  /**
   * The port to which this server will be bound to on the host.
   */
  var port = 0

  /**
   * The working TLSSocket instance for the TLSServer.
   */
  var socket

  /**
   * A boolean value indicating whether to reuse socket addresses or not.
   * @default true
   */
  var resuse_address = true

  /**
   * The timeout in milliseconds after which an attempt to read clients 
   * request data will be terminated.
   * @default 2000 (2 seconds)
   */
  var read_timeout = 2000

  /**
   * The timeout in milliseconds after which an attempt to write response data to 
   * clients will be terminated. 
   * 
   * If we cannot send response to a client after the stipulated time, it will be 
   * assumed such clients have disconnected and existing connections for that 
   * client will be closed and their respective sockets will be discarded.
   * 
   * @default 2000 (2 seconds)
   */
  var write_timeout = 2000

  /**
   * The SSL/TLS ceritificate file that will be used be used by a secured server for 
   * serving requests.
   * @note do not set a value to it directly. Use `load_certs()` instead.
   */
  var cert_file

  /**
   * The SSL/TLS private key file that will be used be used by a secured server for 
   * serving requests.
   * @note do not set a value to it directly. Use `load_certs()` instead.
   */
  var private_key_file

  /**
   * This value controls whether the client certificate should be verified 
   * or not.
   * @boolean
   */
  var verify_certs = true

  # status trackers.
  var _is_listening = false

  var _ciphers = 'ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+3DES:!aNULL:!MD5:!DSS'

  # event handler lists.
  var _connect_listeners = []
  var _disconnect_listeners = []
  var _received_listeners = []
  var _sent_listeners = []
  var _error_listeners = []

  /**
   * TLSServer(port: int [, host: string])
   * @constructor
   */
  TLSServer(port, host) {

    if !is_int(port) or port <= 0
      die HttpException('invalid port number')
    else self.port = port

    if host != nil and !is_string(host)
      die HttpException('invalid host')
    else if host != nil self.host = host

    self.socket = TLSSocket()
  }

  /**
   * load_certs(cert_file: string | file [, private_key_file: string | file])
   * 
   * loads the given SSL/TLS certificate pairs for the given SSL/TLS context.
   * @return bool
   */
  load_certs(cert_file, private_key_file) {
    if !private_key_file private_key_file = cert_file

    self.socket.get_context().set_verify(self.verify_certs ? constants.SSL_VERIFY_PEER : constants.SSL_VERIFY_NONE)

    if self.socket.get_context().load_certs(cert_file, private_key_file) {
      self.cert_file = cert_file
      self.private_key_file = private_key_file

      return self.socket.get_context().set_ciphers(self._ciphers)
    } else {
      # die Exception('could not load certificate(s)')
      return false
    }
  }

  /**
   * close()
   * 
   * stops the server
   */
  close() {
    self._is_listening = false
    if !self.socket.is_closed
      self.socket.close()
    self.socket.get_context().free()  # close the TLS socket context.
  }

  /**
   * on_connect(fn: function)
   * 
   * Adds a function to be called when a new client connects.
   * @note Function _fn_ MUST accept at one parameter which will be passed the client TLSSocket object.
   * @note multiple `on_connect()` may be set on a single instance.
   */
  on_connect(fn) {
    self._connect_listeners.append(fn)
  }

  /**
   * on_disconnect(fn: function)
   * 
   * Adds a function to be called when a new client disconnects.
   * @note Function _fn_ MUST accept at one parameter which will be passed the client information.
   * @note multiple `on_disconnect()` may be set on a single instance.
   */
  on_disconnect(fn) {
    self._disconnect_listeners.append(fn)
  }

  /**
   * on_receive(fn: function)
   * 
   * Adds a function to be called when the server receives a message from a client.
   * 
   * > Function _fn_ MUST accept TWO parameters. First parameter will accept the HttpRequest 
   * > object and the second will accept the HttpResponse object.
   * 
   * @note multiple `on_receive()` may be set on a single instance.
   */
  on_receive(fn) {
    self._received_listeners.append(fn)
  }

  /**
   * on_reply(fn: function)
   * 
   * Adds a function to be called when the server sends a reply to a client.
   * 
   * > Function _fn_ MUST accept one parameter which will be passed the HttpResponse object.
   * 
   * @note multiple `on_sent()` may be set on a single instance.
   */
  on_reply(fn) {
    self._sent_listeners.append(fn)
  }

  /**
   * on_error(fn: function)
   * 
   * Adds a function to be called when the server encounters an error with a client.
   * 
   * > Function _fn_ MUST accept two parameters. The first argument will be passed the 
   * > `Exception` object and the second will be passed the client `TLSSocket` object.
   * 
   * @note multiple `on_error()` may be set on a single instance.
   */
  on_error(fn) {
    self._error_listeners.append(fn)
  }

  _get_response_header_string(headers, cookies) {
    var result
    for x, y in headers {
      result += '${x}: ${y}\r\n'
    }
    for x in cookies {
      result += 'Set-Cookie: ${x}\r\n'
    }
    return result
  }

  _process_received(message, client) {
    var request = HttpRequest(),
        response = HttpResponse()
    if !request.parse(message, client)
      response.status = status.BAD_REQUEST

    var feedback = bytes(0)

    # If we have an error in the request message itself, we don't even want to 
    # forward processing to callers. 
    # This is a server level error and should terminate immediately.
    if response.status == status.OK {

      # call the received listeners on the request object.
      iters.each(self._received_listeners, | fn, _ | {
        fn(request, response)
      })

      if response.body {
        feedback += 'Content-Length: ${response.body.length()}\r\n'.to_bytes()
      }
    }

    # clear file buffers...
    if request.files {
      for f in request.files  {
        f.content.dispose()
      }
    }

    var hdrs = self._get_response_header_string(response.headers, response.cookies).to_bytes()
    feedback += hdrs
    hdrs.dispose()
    
    feedback += '\r\n'.to_bytes()
    feedback += response.body

    var hdrv = ('HTTP/${response.version} ${response.status} ' +
    '${status.map.get(response.status, 'UNKNOWN')}\r\n').to_bytes()
    feedback =  hdrv + feedback
    hdrv.dispose()
                
    client.send(feedback)

    # call the reply listeners.
    iters.each(self._sent_listeners, | fn | {
      fn(response)
    })

    feedback.dispose()
    response.body.dispose()
  }

  /**
   * listen()
   * 
   * Binds to the instance port and host and starts listening for incoming 
   * connection from HTTPS clients.
   */
  listen() {
    if !self.cert_file
      die HttpException('no certificate loaded for secure server')
    if !self.private_key_file 
      die HttpException('no private key loaded for secure server')

    if !self.socket.is_listening {
      self.socket.set_option(so.SO_REUSEADDR, is_bool(self.resuse_address) ? self.resuse_address : true)
      self.socket.bind(self.port, self.host)
      self.socket.listen()

      self._is_listening = true
      while self._is_listening {
        var client = self.socket.accept()

        # call the connect listeners.
        iters.each(self._connect_listeners, | fn, _ | {
          fn(client)
        })

        try {
          if is_number(self.read_timeout)
            client.set_option(so.SO_RCVTIMEO, self.read_timeout)
          if is_number(self.write_timeout)
            client.set_option(so.SO_SNDTIMEO, self.write_timeout)

          var data = client.receive()

          if data {
            data = to_string(data)
            self._process_received(data, client)
          }
        } catch Exception e {
          # call the error listeners.
          iters.each(self._error_listeners, | fn, _ | {
            fn(e, client)
          })
        } finally {
          var client_info = client.info()
          client.close()

          # call the disconnect listeners.
          iters.each(self._disconnect_listeners, | fn, _ | {
            fn(client_info)
          })
        }
      }
    }
  }

  @to_string() {
    return '<TLSServer ${self.host}:${self.port}>'
  }
}

