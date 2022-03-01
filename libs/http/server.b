#!-- part of the http module

import .request { HttpRequest }
import .response { HttpResponse }
import .status

import socket as so
import iters

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
   * @note Function _fn_ MUST accept at one parameter which will be passed the client Socket object.
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
   * > Function _fn_ MUST accept at two parameters. The first argument will be passed the 
   * > Exception object and the second will be passed the client `Socket` object.
   * 
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
    # call the reply listeners.
    iters.each(self._sent_listeners, | fn | {
      fn(response)
    })
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

        try {
          var data = client.receive()

          if data {
            self._process_received(data, client)
          }
        } catch Exception e {
          # call the error listeners.
          iters.each(self._error_listeners, | fn | {
            fn(e, client)
          })
        } finally {
          var client_info = client.info()
          client.close()

          # call the disconnect listeners.
          iters.each(self._disconnect_listeners, | fn | {
            fn(client_info)
          })
        }
      }
    }
  }
}

