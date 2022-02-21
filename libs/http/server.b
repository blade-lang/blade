#!-- part of the http module

import socket as so

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

  # event handlers.

  var _connect_listeners = [| client | {

  }]

  var _disconnect_listeners = [| client | {

  }]

  var _received_listeners = [| message, client | {
    var response = 'It works!'
    client.send('HTTP/1.1 200 OK\r\n' +
    'X-Powered-By: Blade\r\n' +
    'Access-Control-Allow-Origin: *\r\n' +
    'Content-Type: application/json; charset=utf-8\r\n' +
    'Content-Length: ${response.length()}\r\n' +
    'ETag: W/"20-kpKo63uv4n6XEGgQeIwK7WAi6Ls"\r\n' +
    'Date: Sun, 18 Apr 2021 03:52:16 GMT\r\n' +
    '\r\n' +
    response)
  }]

  var _sent_listeners = [| message, client | {

  }]

  var _error_listeners = [| error, client | {

  }]

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
   * @note Function _fn_ must accept at least one parameter which will be passed the message received as a string.
   * @note If _fn_ accepts a second parameter, it will be passed the client socket object.
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
        if self._connect_listeners {
          for l in self._connect_listeners {
            l(client)
          }
        }

        if is_number(self.read_timeout)
          client.set_option(so.SO_RCVTIMEO, self.read_timeout)
        if is_number(self.write_timeout)
          client.set_option(so.SO_SNDTIMEO, self.write_timeout)

        try {
          var data = client.receive()

          if data {

            # call the received listeners.
            if self._received_listeners {
              for l in self._received_listeners {
                l(data, client)
              }
            }
          }
        } catch Exception e {
          # call the error listeners.
          if self._error_listeners {
            for l in self._error_listeners {
              l(e, client)
            }
          }
        } finally {
          var client_info = client.info()
          client.close()

          # call the disconnect listeners.
          if self._disconnect_listeners {
            for l in self._disconnect_listeners {
              echo l
              l(nil)
            }
          }
        }
      }
    }
  }
}

