/**
 * Socket
 *
 * Provides interface for working with Socket clients
 * and servers.
 * @copyright 2021, Ore Richard Muyiwa
 */

class SocketException < Exception {
  SocketException(message) {
    self.message = message
  }
}

class Socket {
  /*
   * Types
   */
  static var SOCK_STREAM    = 1 # stream socket
  static var SOCK_DGRAM     = 2 # datagram socket
  static var SOCK_RAW       = 3 # raw-protocol interface
  static var SOCK_RDM       = 4 # reliably-delivered message
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
  static var SO_LINGER        = 0x0080 # linger on close if data present (in ticks)
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
  static var AF_UNSPEC    = 0 # unspecified
  static var AF_UNIX      = 1 # local to host (pipes)
  static var AF_LOCAL     = 1 # same as AF_UNIX
  static var AF_INET      = 2 # internetwork: UDP, TCP, etc.
  static var AF_IMPLINK   = 3 # arpanet imp addresses
  static var AF_PUP       = 4 # pup protocols: e.g. BSP
  static var AF_CHAOS     = 5 # mit CHAOS protocols
  static var AF_NS        = 6 # XEROX NS protocols
  static var AF_ISO       = 7 # ISO protocols
  static var AF_OSI       = 7 # OSI protocols (same as ISO)
  static var AF_ECMA      = 8 # European computer manufacturers
  static var AF_DATAKIT   = 9 # datakit protocols
  static var AF_CCITT     = 10 # CITT protocols, X.25 etc
  static var AF_SNA       = 11 # IBM SNA
  static var AF_DECnet    = 12 # DECnet
  static var AF_DLI       = 13 # DEC Direct data link interface
  static var AF_LAT       = 14 # LAT
  static var AF_HYLINK    = 15 # NSC Hyperchannel
  static var AF_APPLETALK = 16 # AppleTalk
  static var AF_INET6     = 30 # ipv6

  /*
   * howto arguments for shutdown(2), specified by Posix.1g.
   */
  static var SHUT_RD       = 0 # shut down the reading side
  static var SHUT_WR       = 1 # shut down the writing side
  static var SHUT_RDWR     = 2 # shut down both sides

  /*
   * Maximum queue length specifiable by listen.
   */
  static var SOMAXCONN = 128

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
  var family = 2

  # the default socket type is SOCK_STREAM
  var type = 1

  # initialize flags for default behavior...
  var flags = 0

  # this variable holds the id of the socket on
  # the host machine. This value will be passed
  # to the backend whenever the socket id is required.
  var id = -1

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

  # as well as when we are running in blocking mode
  var is_blocking = false

  # constructor...
  # called without parameters, is same as
  # Socket(Socket.AF_INET, Socket.SOCK_STREAM, 0)
  Socket(family, type, flags) {
    if family self.family = family
    if type self.type = type
    if flags self.flags = flags

    if !is_int(self.family) 
      die SocketException('AF_* expected for family, ${typeof(self.family)} given')
    if !is_int(self.type) 
      die SocketException('SOCK_* expected for type, ${typeof(self.type)} given')
    if !is_int(self.flags) 
      die SocketException('integer expected for flags, ${typeof(self.flags)} given')

    var id = self._create(self.family, self.type, self.flags)
    if id == -1 die SocketException('could not create socket')

    self.id = id
  }
  
  # creates a new instance of socket with prefilled information
  # this helps us create sockets on the fly binding to different
  # id's on the device.
  # @return Socket
  static New(family, type, flags, id, host, port, is_client) {
    var socket = Socket(family, type, flags)
    socket.id = id
    socket.host = host
    socket.port = port
    socket.is_client = is_client
    socket.is_connected = true
    return socket
  }

  # checks if a response code is valid
  # returns the code if it is or throws an SocketException otherwise
  _check_error(code) {
    var err = self._error(code)
    if err die SocketException(err)
    return code
  }

  connect(host, port, timeout) {
    if !host host = self.host
    if !timeout timeout = 300000 # default timeout is 300 seconds

    if !port die SocketException('port not specified')
    if !is_string(host) 
      die SocketException('string expected for host, ${typeof(host)} given')
    if !is_int(port) 
      die SocketException('integer expected for port, ${typeof(port)} given')
    if !is_int(timeout) 
      die SocketException('integer expected for timeout, ${typeof(timeout)} given')

    if self.id == -1 or self.is_closed die SocketException('socket is in an illegal state')

    if self.is_connected die SocketException('socket has existing connection')

    var result = self._check_error(self._connect(self.id, host, port, self.family, timeout, self.is_blocking))
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

    if !port die SocketException('port not specified')
    if !is_string(host) 
      die SocketException('string expected for host, ${typeof(host)} given')
    if !is_int(port) 
      die SocketException('integer expected for port, ${typeof(port)} given')

    if self.id == -1 or self.is_closed die SocketException('socket is in an illegal state')


    if self.is_bound die SocketException('socket previously bound')

    var result = self._check_error(self._bind(self.id, host, port, self.family))
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
      die SocketException('message must string, bytes or file')
    if !is_int(flags) 
      die SocketException('integer expected for flags, ${typeof(flags)} given')

    if self.id == -1 or self.is_closed or (self.is_shutdown and 
      (self.shutdown_reason == Socket.SHUT_WR or 
        self.shutdown_reason == Socket.SHUT_RDWR)) 
      die SocketException('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die SocketException('socket not listening or connected')

    return self._check_error(self._send(self.id, message, flags))
  }

  receive(length, flags) {
    if !length length = -1
    if !flags flags = 0

    if !is_int(length) 
      die SocketException('integer expected for length, ${typeof(length)} given')
    if !is_int(flags) 
      die SocketException('integer expected for flags, ${typeof(flags)} given')

    if self.id == -1 or self.is_closed or (self.is_shutdown and 
      (self.shutdown_reason == Socket.SHUT_RD or 
        self.shutdown_reason == Socket.SHUT_RDWR)) 
      die SocketException('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die SocketException('socket not listening or connected')
    
    var result = self._recv(self.id, length, flags)
    if is_string(result) or result == nil return result

    return self._check_error(result)
  }

  listen(queue_length) {
    if !queue_length queue_length = Socket.SOMAXCONN # default to 1024 simulataneous clients...

    if !is_int(queue_length) 
      die SocketException('integer expected for queue_length, ${typeof(queue_length)} given')
    if queue_length > Socket.SOMAXCONN 
      die SocketException('maximum queue length exceeded')

    if !self.is_bound or self.is_listening or self.is_closed die SocketException('socket is in an illegal state')

    var result = self._check_error(self._listen(self.id, queue_length))
    if result {
      self.is_listening = true
    }
    return result
  }

  accept() {
    if self.is_bound and self.is_listening and !self.is_closed {
      var result = self._accept(self.id)
      return Socket.New(self.family, self.type, self.flags, result[0], result[1], result[2], true)
    }
    die SocketException('socket not bound/listening')
  }

  close() {
    # silently ignore multiple calls to close()
    if self.is_closed return true

    var result = self._check_error(self._close(self.id)) >= 0
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
      die SocketException('integer expected for how, ${typeof(how)} given')

    if how < Socket.SHUT_RD or how > Socket.SHUT_RDWR
      die SocketException('expected one of Socket.SHUT_* flags')

    # consecutive call to the same shutdown type should be ignored
    if self.is_shutdown and self.shutdown_reason == how return true

    if self.is_closed die SocketException('socket is in an illegal state')

    var result = self._check_error(self._shutdown(self.id, how)) >= 0
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
      die SocketException('both option and value are required')
    if !is_int(option) 
      die SocketException('integer expected for option, ${typeof(option)} given')
    if option < Socket.SO_DEBUG or option > Socket.SO_TYPE # @TODO: update SO_TYPE as options increase
      die SocketException('expected one of Socket.SO_* options')

    if option == Socket.SO_TYPE or option == Socket.SO_ERROR
      die Exception('the given option is read-only')

    var result = self._check_error(self._setsockopt(self.id, option, value)) >= 0

    if result {
      # get an update on SO_SNDTIMEO and SO_RCVTIMEO
      if option == Socket.SO_SNDTIMEO self.send_timeout = value
      else if option == Socket.SO_RCVTIMEO self.receive_timeout = value
    }

    return result
  }

  get_option(option) {
    if !option
      return nil

    if !is_int(option) 
      die SocketException('integer expected for option, ${typeof(option)} given')
    if option < Socket.SO_DEBUG or option > Socket.SO_TYPE # @TODO: update SO_TYPE as options increase
      die SocketException('expected one of Socket.SO_* options')

    # we have a local copy of SO_RCVTIMEO and SO_SNDTIMEO
    # we can simply return them when required
    if option == Socket.SO_RCVTIMEO return self.receive_timeout
    else if option == Socket.SO_SNDTIMEO return self.send_timeout

    return self._getsockopt(self.id, option)
  }

  set_blocking(mode) {
    if !is_bool(mode) die SocketException('boolean expected')
    self.is_blocking = mode
  }

  info() {
    return self._getsockinfo(self.id)
  }

  get_address(address, type, family) {
    if !is_string(address)
      die SocketException('string expected for address, ${typeof(address)} given')
    if type != nil and !is_string(type)
      die SocketException('string expected for type, ${typeof(type)} given')

    if !type type = 'http'
    if !family family = Socket.AF_INET

    return self._getaddrinfo(address, type, family)
  }

  static get_address_info(address, type, family) {
    return Socket().get_address(address, type, family)
  }
}

