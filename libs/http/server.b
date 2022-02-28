#!-- part of the http module

import socket as so
import iters
import .request { HttpRequest }
import .response { HttpResponse }
import .status

/**
 * HTTP server
 */
class HttpServer {

  var address = so.IP_LOCAL

  var port = 0

  var socket = so.Socket()

  var headers = {}

  var resuse_address = true

  var read_timeout = 2000

  var write_timeout = 2000

  # status trackers.
  var _is_listening = false

  # event handler lists.
  var _connect_listeners = []
  var _disconnect_listeners = []
  var _received_listeners = []
  var _sent_listeners = []
  var _error_listeners = []

  /**
   * HttpServer(port: int [, address: string])
   * @constructor
   */
  HttpServer(port, address) {

    if !is_int(port) or port <= 0
      die Exception('invalid port number')
    else self.port = port

    if address != nil and !is_string(address)
      die Exception('invalid address')
    else if address != nil self.address = address
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
  }

  /**
   * on_connect(fn: function)
   * 
   * Adds a function to be called when a new client connects.
   * @note Function _fn_ must accept at one parameter which will be passed the client socket object.
   * @note multiple `on_connect()` may be set on a single instance.
   */
  on_connect(fn) {
    self._connect_listeners.append(fn)
  }

  /**
   * on_disconnect(fn: function)
   * 
   * Adds a function to be called when a new client disconnects.
   * @note Function _fn_ must accept at one parameter which will be passed the client information.
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
   * > Function _fn_ MUST accept TWO parameter. First parameter will accept the HttpRequest 
   * > object and the second will accept the HttpResponse object.
   * 
   * @note multiple `on_receive()` may be set on a single instance.
   */
  on_receive(fn) {
    self._received_listeners.append(fn)
  }

  /**
   * on_sent(fn: function)
   * 
   * Adds a function to be called when the server sends a message to a client.
   * 
   * @note Function _fn_ must accept at least one parameter which will be passed the message received as a string.
   * @note If _fn_ accepts a second parameter, it will be passed the client socket object.
   * @note multiple `on_sent()` may be set on a single instance.
   */
  on_sent(fn) {
    self._sent_listeners.append(fn)
  }

  /**
   * on_error(fn: function)
   * 
   * Adds a function to be called when the server encounters an error with a client.
   * 
   * @note Function _fn_ must accept at least one parameter which will be passed the Exception object.
   * @note If _fn_ accepts a second parameter, it will be passed the client socket object.
   * @note multiple `on_error()` may be set on a single instance.
   */
  on_error(fn) {
    self._error_listeners.append(fn)
  }

  _get_response_header_string(headers) {
    var result
    for x, y in headers {
      result += '${x}: ${y}\r\n'
    }
    return result
  }

  _process_received(message, client) {
    var request = HttpRequest(),
        response = HttpResponse()
    if !request.parse(message, client)
      response.status = status.BAD_REQUEST

    var feedback = 'HTTP/${response.version} ${response.status} ${status.map.get(response.status, 'UNKNOWN')}\r\n'

    # If we have an error in the request message itself, we don't even want to 
    # forward processing to callers. 
    # This is a server level error and should terminate immediately.
    if response.status == status.OK {

      # call the received listeners on the request object.
      iters.each(self._received_listeners, | fn, _ | {
        fn(request, response)
      })

      if response.body {
        feedback += 'Content-Length: ${response.body.length()}\r\n'
      }
    }

    feedback += self._get_response_header_string(response.headers)
    feedback += '\r\n${response.body}' 
    
    client.send(feedback)
  }

  /**
   * listen()
   * 
   * Binds to the instance port and address and starts listening for incoming 
   * connection from HTTP clients.
   */
  listen() {
    if !self.socket.is_listening {
      self.socket.set_option(so.SO_REUSEADDR, is_bool(self.resuse_address) ? self.resuse_address : true)
      self.socket.bind(self.port, self.address)
      self.socket.listen()

      self._is_listening = true
      while self._is_listening {
        var client = self.socket.accept()

        # call the connect listeners.
        iters.each(self._connect_listeners, | fn | {
          fn(client)
        })

        if is_number(self.read_timeout)
          client.set_option(so.SO_RCVTIMEO, self.read_timeout)
        if is_number(self.write_timeout)
          client.set_option(so.SO_SNDTIMEO, self.write_timeout)

        /* try { */
          var data = client.receive()

          if data {
            self._process_received(data, client)
          }
        /* } catch Exception e {
          # call the error listeners.
          iters.each(self._error_listeners, | fn | {
            fn(e, client)
          })
        } finally { */
          var client_info = client.info()
          client.close()

          # call the disconnect listeners.
          iters.each(self._disconnect_listeners, | fn | {
            fn(client_info)
          })
        /* } */
      }
    }
  }
}

