/**
 * Socket
 *
 * Provides interface for working with Socket clients
 * and servers.
 * @copyright Ore Richard
 */

class Socket {
  /*
   * Types
   */
  static var SOCK_STREAM    = 1 # stream socket
  static var SOCK_DGRAM     = 2 # datagram socket
  static var SOCK_RAW       = 3 # raw-protocol interface
  static var SOCK_SEQPACKET = 5 # sequenced packet stream

  /*
   * Option flags per-socket.
   */
  static var SO_DEBUG         = 0x0001 # turn on debugging info recording
  static var SO_ACCEPTCONN    = 0x0002 # socket has had listen()
  static var SO_REUSEADDR     = 0x0004 # allow local address reuse
  static var SO_KEEPALIVE     = 0x0008 # keep connections alive
  static var SO_DONTROUTE     = 0x0010 # just use interface addresses
  static var SO_BROADCAST     = 0x0020 # permit sending of broadcast msgs
  static var SO_USELOOPBACK   = 0x0040 # bypass hardware when possible
  static var SO_OOBINLINE     = 0x0100 # leave received OOB data in line
  static var SO_REUSEPORT     = 0x0200 # allow local address & port reuse

  /*
   * Additional options, not kept in so_options.
   */
  static var SO_SNDBUF    = 0x1001 # send buffer size
  static var SO_RCVBUF    = 0x1002 # receive buffer size
  static var SO_SNDLOWAT  = 0x1003 # send low-water mark
  static var SO_RCVLOWAT  = 0x1004 # receive low-water mark
  static var SO_SNDTIMEO  = 0x1005 # send timeout
  static var SO_RCVTIMEO  = 0x1006 # receive timeout
  static var SO_ERROR     = 0x1007 # get error status and clear
  static var SO_TYPE      = 0x1008 # get socket type

  /*
   * Level number for (get/set)sockopt() to apply to socket itself.
   */
  static var SOL_SOCKET = 0xffff # options for socket level

  /*
   * Address families.
   */
  static var AF_UNSPEC  = 0 # unspecified
  static var AF_UNIX    = 1 # local to host (pipes)
  static var AF_LOCAL   = 1 # same as AF_UNIX
  static var AF_INET    = 2 # internetwork: UDP, TCP, etc.
  static var AF_INET6   = 30 # ipv6

  /*
   * howto arguments for shutdown(2), specified by Posix.1g.
   */
  static var SHUT_RD       = 0 # shut down the reading side
  static var SHUT_WR       = 1 # shut down the writing side
  static var SHUT_RDWR     = 2 # shut down both sides

  /*
   * Static helpers
   */
  static var IP_ANY     = '0.0.0.0'
  static var IP_LOCAL   = '127.0.0.1'

  # Whenever a host is not given, the host will default
  # to localhost.
  var host = '127.0.0.1'
  var port = 0

  # the default family for the socket is AF_INET
  var _family = 2

  # the default socket type is SOCK_STREAM
  var _type = 1

  # initialize flags for default behavior...
  var _flags = 0

  # this variable holds the id of the socket on
  # the host machine. This value will be passed
  # to the backend whenever the socket id is required.
  var socket_id = -1

  # tracking the kind of socket we have...
  # i.e. client or server!
  var is_client = false

  # we want to know when the server is bound, listening or connected
  var is_bound = false
  var is_connected = false
  var is_listening = false
  # as well as closed or shutdown
  var is_closed = false
  var is_shutdown = false

  # and why we shutdown
  var shutdown_reason = -1

  # and get an update on SO_SNDTIMEO and SO_RCVTIMEO
  var send_timeout = -1
  var receive_timeout = -1

  # constructor...
  # called without parameters, is same as
  # Socket(Socket.AF_INET, Socket.SOCK_STREAM, 0)
  Socket(family, type_, flags) {
    if family self._family = family
    if type_ self._type = type_
    if flags self._flags = flags

    if !is_int(self._family) 
      die Exception('integer expected for family, ${type(self._family)} given')
    if !is_int(self._type) 
      die Exception('integer expected for type, ${type(self._type)} given')
    if !is_int(self._flags) 
      die Exception('integer expected for flags, ${type(self._flags)} given')

    var id = self._create(self._family, self._type, self._flags)
    if id == -1 die Exception('could not create socket')

    self.socket_id = id
  }
  
  # creates a new instance of socket with prefilled information
  # this helps us create sockets on the fly binding to different
  # id's on the device.
  # @return Socket
  static _Socket(family, type, flags, id, host, port, is_client) {
    var socket = Socket(family, type, flags)
    socket.socket_id = id
    socket.host = host
    socket.port = port
    socket.is_client = is_client
    socket.is_connected = true
    return socket
  }

  # checks if a response code is valid
  # returns the code if it is or throws an exception otherwise
  _check_error(code) {
    var err = self._error(code)
    if err die Exception(err)
    return code
  }

  connect(host, port) {
    if !host host = self.host

    if !port die Exception('port not specified')
    if !is_string(host) 
      die Exception('string expected for host, ${type(host)} given')
    if !is_int(port) 
      die Exception('integer expected for port, ${type(port)} given')

    if self.socket_id == -1 or self.is_closed die Exception('socket is in an illegal state')

    if self.is_connected die Exception('socket has existing connection')

    var result = self._check_error(self._connect(self.socket_id, host, port, self._family))
    if result {
      self.is_client = true
      self.is_connected = true
      self.is_listening = false
      self.is_bound = false
    }
    return result
  }
  
  bind(host, port) {
    if !host host = self.host

    if !port die Exception('port not specified')
    if !is_string(host) 
      die Exception('string expected for host, ${type(host)} given')
    if !is_int(port) 
      die Exception('integer expected for port, ${type(port)} given')

    if self.socket_id == -1 or self.is_closed die Exception('socket is in an illegal state')


    if self.is_bound die Exception('socket previously bound')

    var result = self._check_error(self._bind(self.socket_id, host, port, self._family))
    if result {
      self.is_bound = true
      self.is_listening = false # it's freshly bound
      self.is_connected = false # a bound socket can't be connected ass well
      self.is_client = false # a bound socket cannot be a client
    }
    return result
  }

  send(message, flags) {
    if !message message = ''
    if !flags flags = 0

    if !is_string(message) and !is_bytes(message) and !is_file(message) 
      die Exception('message must string, bytes or file')
    if !is_int(flags) 
      die Exception('integer expected for flags, ${type(flags)} given')

    if self.socket_id == -1 or self.is_closed or (self.is_shutdown and 
      (self.shutdown_reason == Socket.SHUT_WR or 
        self.shutdown_reason == Socket.SHUT_RDWR)) 
      die Exception('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die Exception('socket not listening or connected')

    return self._check_error(self._send(self.socket_id, message, flags))
  }

  receive(length, flags) {
    if !length length = -1
    if !flags flags = 0

    if !is_int(length) 
      die Exception('integer expected for length, ${type(length)} given')
    if !is_int(flags) 
      die Exception('integer expected for flags, ${type(flags)} given')

    if self.socket_id == -1 or self.is_closed or (self.is_shutdown and 
      (self.shutdown_reason == Socket.SHUT_RD or 
        self.shutdown_reason == Socket.SHUT_RDWR)) 
      die Exception('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die Exception('socket not listening or connected')
    
    var result = self._recv(self.socket_id, length, flags)
    if is_string(result) return result

    return self._check_error(result)
  }

  listen(max_connections) {
    if !max_connections max_connections = 1024 # default to 1024 simulataneous clients...

    if !is_int(max_connections) 
      die Exception('integer expected for max_connections, ${type(max_connections)} given')

    if !self.is_bound or self.is_listening or self.is_closed die Exception('socket is in an illegal state')

    var result = self._check_error(self._listen(self.socket_id, max_connections))
    if result {
      self.is_listening = true
    }
    return result
  }

  accept() {
    if self.is_bound and self.is_listening and !self.is_closed {
      var result = self._accept(self.socket_id)
      return Socket._Socket(self._family, self._type, self._flags, result[0], result[1], result[2], true)
    }
    die Exception('socket not bound/listening')
  }

  close() {
    # silently ignore multiple calls to close()
    if self.is_closed return true

    var result = self._check_error(self._close(self.socket_id)) > 0
    if result {
      self.is_connected = false
      self.is_listening = false
      self.is_bound = false
      self.is_client = false # may be reused as a server...
      self.is_closed = true
    }
    return result
  }

  shutdown(how) {
    if !how how = Socket.SHUT_RD
    
    if !is_int(how) 
      die Exception('integer expected for how, ${type(how)} given')

    if how < Socket.SHUT_RD or how > Socket.SHUT_RDWR
      die Exception('expected one of Socket.SHUT_* flags')

    # consecutive call to the same shutdown type should be ignored
    if self.is_shutdown and self.shutdown_reason == how return true

    if self.is_closed die Exception('socket is in an illegal state')

    var result = self._check_error(self._shutdown(self.socket_id, how)) > 0
    if result {
      self.is_connected = false
      self.is_listening = false
      self.is_bound = false
      self.is_client = false # may be reused as a server...
      self.is_shutdown = true
      self.shutdown_reason = how
    }
    return result
  }

  set_option(option, value) {
    if !option or !value 
      die Exception('both option and value are required')
    if !is_int(option) 
      die Exception('integer expected for option, ${type(option)} given')
    if option < Socket.SO_DEBUG or option > Socket.SO_TYPE # @TODO: update SO_TYPE as options increase
      die Exception('expected one of Socket.SO_* options')

    var result = self._check_error(self._setsockopt(self.socket_id, option, value)) > 0

    if result {
      # get an update on SO_SNDTIMEO and SO_RCVTIMEO
      if option == Socket.SO_SNDTIMEO self.send_timeout = value
      else if option == Socket.SO_RCVTIMEO self.receive_timeout = value
    }

    return result
  }
}
