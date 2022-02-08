#
# @module socket
#
# Provides interface for working with Socket clients
# and servers.
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _socket {
  SOCK_STREAM,
  SOCK_DGRAM,
  SOCK_RAW,
  SOCK_RDM,
  SOCK_SEQPACKET,

  SO_DEBUG,
  SO_ACCEPTCONN,
  SO_REUSEADDR,
  SO_KEEPALIVE,
  SO_DONTROUTE,
  SO_BROADCAST,
  SO_USELOOPBACK,
  SO_LINGER,
  SO_OOBINLINE,
  SO_REUSEPORT,
  SO_TIMESTAMP,

  SO_SNDBUF,
  SO_RCVBUF,
  SO_SNDLOWAT,
  SO_RCVLOWAT,
  SO_SNDTIMEO,
  SO_RCVTIMEO,
  SO_ERROR,
  SO_TYPE,


  SOL_SOCKET,

  AF_UNSPEC,
  AF_UNIX,
  AF_LOCAL,
  AF_INET,
  AF_IMPLINK,
  AF_PUP,
  AF_CHAOS,
  AF_NS,
  AF_ISO,
  AF_OSI,
  AF_ECMA,
  AF_DATAKIT,
  AF_CCITT,
  AF_SNA,
  AF_DECnet,
  AF_DLI,
  AF_LAT,
  AF_HYLINK,
  AF_APPLETALK,
  AF_INET6,

  IPPROTO_IP,
  IPPROTO_ICMP,
  IPPROTO_IGMP,
  IPPROTO_IPIP,
  IPPROTO_TCP,
  IPPROTO_EGP,
  IPPROTO_PUP,
  IPPROTO_UDP,
  IPPROTO_IDP,
  IPPROTO_TP,
  IPPROTO_DCCP,
  IPPROTO_IPV6,
  IPPROTO_RSVP,
  IPPROTO_GRE,
  IPPROTO_ESP,
  IPPROTO_AH,
  IPPROTO_MTP,
  IPPROTO_BEETPH,
  IPPROTO_ENCAP,
  IPPROTO_PIM,
  IPPROTO_COMP,
  IPPROTO_SCTP,
  IPPROTO_UDPLITE,
  IPPROTO_MPLS,
  IPPROTO_RAW,
  IPPROTO_MAX,

  SHUT_RD,
  SHUT_WR,
  SHUT_RDWR,

  SOMAXCONN,


  _create,
  _connect,
  _error,
  _accept,
  _bind,
  _listen,
  _recv,
  _send,
  _setsockopt,
  _shutdown,
  _close,
  _getaddrinfo,
  _getsockinfo,
  _getsockopt
}



/**
 * The non-designated address used to represent "no particular address"
 * (also referred to as "any address")
 */
var IP_ANY     = '0.0.0.0'

/**
 * The loopback address (also known as localhost)
 */
var IP_LOCAL   = '127.0.0.1'

/**
 * The SocketExceptio class is the general Exception type thrown from sockets
 */
class SocketException < Exception {

  /**
   * SocketException(message: string)
   * @constructor 
   */
  SocketException(message) {
    self.message = message
  }

  @to_string() {
    return '<SocketException: ${self.message}>'
  }
}

/**
 * The Socket class provides interface for working with Socket clients
 * and servers.
 */
class Socket {

  /**
   * This property holds the host bound, to be bound to or connected to by the current socket.
   * Whenever a host is not given, the host will default to localhost.
   */
  var host = 'localhost'

  /**
   * The port currently bound or connected to by the socket
   */
  var port = 0

  /**
   * The socket family (which must be one of the `AF_` variables).
   * The default family for the socket is AF_INET
   */
  var family = AF_INET

  /**
   * The type of socket stream used by the socket.
   * The default socket type is `SOCK_STREAM`
   */
  var type = SOCK_STREAM

  /**
   * The current operating protocol of the socket that controls the 
   * underlying behavior of the socket. The default is `IPPROTO_TCP`.
   */
  var protocol = IPPROTO_TCP

  /**
   * The file descriptor id of the current socket on the host machine.
   */
  var id = -1

  /**
   * `true` when the socket is a client to a server socket, `false` otherwise.
   */
  var is_client = false

  /**
   * `true` when the socket is bound to a given port on the device, `false` 
   * otherwise.
   */
  var is_bound = false

  /**
   * `true` when the socket is connected to a server socket, `false` otherwise.
   */
  var is_connected = false

  /**
   * `true` when the socket is currently listening on a host device port as a 
   * server, `false` otherwise.
   */
  var is_listening = false
  
  /**
   * `true` when the socket is closed, `false` otherwise.
   */
  var is_closed = false

  /**
   * `true` when the socket is shutdown, `false` otherwise.
   */
  var is_shutdown = false

  /**
   * The property holds the reason for which the last `shutdown` operation 
   * was called or `-1` if `shutdown` was never requested.
   */
  var shutdown_reason = -1

  /**
   * The amount of time in milliseconds that the socket waits before it 
   * terminates a `send` operation. This is equal to the `SO_SNDTIMEO`.
   */
  var send_timeout = -1

  /**
   * The amount of time in milliseconds that the socket waits before it 
   * terminates a `receive` operation. This is equal to the `SO_RCVTIMEO`.
   */
  var receive_timeout = -1

  /**
   * `true` when the socket is running in a blocking mode, `false` otherwise.
   */
  var is_blocking = false

  /**
   * Socket(family: number [, type: number, protocol: number [, id: number]])
   * @example Socket(AF_INET, SOCK_STREAM, 0)
   * @constructor  
   */
  Socket(family, type, protocol, id) {
    if family self.family = family
    if type self.type = type
    if protocol self.protocol = protocol

    if !id {
      if !is_int(self.family) 
        die SocketException('AF_* expected for family, ${typeof(self.family)} given')
      if !is_int(self.type) 
        die SocketException('SOCK_* expected for type, ${typeof(self.type)} given')
      if !is_int(self.protocol) 
        die SocketException('integer expected for protocol, ${typeof(self.protocol)} given')

      self.id = self._check_error(_create(self.family, self.type, self.protocol))
    } else {
      self.id = id
    }
  }

  # checks if a response code is valid
  # returns the code if it is or throws an SocketException otherwise
  _check_error(code) {
    var err = _error(code)
    if err die SocketException(err)
    return code
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

    var result = self._check_error(_connect(self.id, host, port, self.family, timeout, self.is_blocking))
    if result {
      self.is_client = true
      self.is_connected = true
      self.is_listening = false
      self.is_bound = false
    }
    return result == 0
  }
  
  /**
   * bind(port: int [, host: string])
   * 
   * Binds this socket to the given port on the given host. If host is `nil` or not specified, it will connect 
   * on to the current hostn specified on the socket. 
   * @return bool
   */
  bind(port, host) {
    if !host host = self.host

    if !port die SocketException('port not specified')
    if !is_string(host) 
      die SocketException('string expected for host, ${typeof(host)} given')
    if !is_int(port) 
      die SocketException('integer expected for port, ${typeof(port)} given')

    if self.id == -1 or self.is_closed die SocketException('socket is in an illegal state')


    if self.is_bound die SocketException('socket previously bound')

    var result = self._check_error(_bind(self.id, host, port, self.family))
    if result {
      self.is_bound = true
      self.is_listening = false # it's freshly bound
      self.is_connected = false # a bound socket can't be connected ass well
      self.is_client = false # a bound socket cannot be a client
    }
    return result == 0
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
    if !message message = ''
    if !flags flags = 0

    if !is_string(message) and !is_bytes(message) and !is_file(message) 
      die SocketException('message must string, bytes or file')
    if !is_int(flags) 
      die SocketException('integer expected for flags, ${typeof(flags)} given')

    if self.id == -1 or self.is_closed or (self.is_shutdown and 
      (self.shutdown_reason == SHUT_WR or 
        self.shutdown_reason == SHUT_RDWR)) 
      die SocketException('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die SocketException('socket not listening or connected')

    return self._check_error(_send(self.id, message, flags))
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
    if !length length = -1
    if !flags flags = 0

    if !is_int(length) 
      die SocketException('integer expected for length, ${typeof(length)} given')
    if !is_int(flags) 
      die SocketException('integer expected for flags, ${typeof(flags)} given')

    if self.id == -1 or self.is_closed or (self.is_shutdown and 
      (self.shutdown_reason == SHUT_RD or 
        self.shutdown_reason == SHUT_RDWR)) 
      die SocketException('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die SocketException('socket not listening or connected')
    
    var result = _recv(self.id, length, flags)
    if is_string(result) or result == nil return result

    return self._check_error(result)
  }

  /**
   * listen([queue_length: int])
   * 
   * Listen for connections on a socket
   * 
   * This method puts the socket in a state where it is willing to accept incoming connections and creates 
   * a queue limit of `queue_length` for incoming connections. If a connection request arrives with 
   * the queue full, the client may receive an error with an indication of `ECONNREFUSED`. 
   * Alternatively, if the underlying protocol supports retransmission, the request may be ignored 
   * so that retries may succeed.
   * 
   * When the `queue_length` is ommited or set to -1, the method will use the default queue limit of 
   * the current platform which is usually equal to `SOMAXCONN`.
   * 
   * @note listen() call applies only to sockets of type `SOCK_STREAM` (which is the default)
   * @return bool
   */
  listen(queue_length) {
    if !queue_length queue_length = SOMAXCONN # default to 128 simulataneous clients...

    if !is_int(queue_length) 
      die SocketException('integer expected for queue_length, ${typeof(queue_length)} given')
    if queue_length > SOMAXCONN 
      die SocketException('maximum queue length exceeded')

    if !self.is_bound or self.is_listening or self.is_closed 
      die SocketException('socket is in an illegal state')

    var result = self._check_error(_listen(self.id, queue_length))
    if result {
      self.is_listening = true
    }
    return result == 0
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
   * The accepted socket may not be used to accept more connections.  The original socket socket, remains open.
   * @return Socket
   */
  accept() {
    if self.is_bound and self.is_listening and !self.is_closed {
      var result = _accept(self.id)

      if result and result != -1  {
        var socket = Socket(self.family, self.type, self.protocol, result[0])
        socket.host = result[1]
        socket.port = result[2]
        socket.is_client = true
        socket.is_connected = true
        return socket
      }
    }
    die SocketException('socket not bound/listening')
  }

  /**
   * close()
   * 
   * Closes the socket
   * @return bool
   */
  close() {
    # silently ignore multiple calls to close()
    if self.is_closed return true

    if self._check_error(_close(self.id)) == 0 {
      self.is_connected = false
      self.is_listening = false
      self.is_bound = false
      self.is_client = false # may be reused as a server...
      self.is_closed = true
      return true
    }
    
    return false
  }

  /**
   * shutdown([how: int])
   * 
   * The shutdown() call causes all or part of a full-duplex connection on the socket associated with 
   * socket to be shut down.  If how is `SHUT_RD`, further receives will be disallowed.  If how is `SHUT_WR`, 
   * further sends will be disallowed.  If how is `SHUT_RDWR`, further sends and receives will be disallowed.
   * 
   * When _how_ is not specified, it defaults to `SHUT_RD`.
   * 
   * @return bool
   */
  shutdown(how) {
    if !how how = SHUT_RD
    
    if !is_int(how) 
      die SocketException('integer expected for how, ${typeof(how)} given')

    if how < SHUT_RD or how > SHUT_RDWR
      die SocketException('expected one of SHUT_* flags')

    # consecutive call to the same shutdown type should be ignored
    if self.is_shutdown and self.shutdown_reason == how return true

    if self.is_closed die SocketException('socket is in an illegal state')

    var result = self._check_error(_shutdown(self.id, how)) >= 0
    if result {
      self.is_connected = false
      self.is_listening = false
      self.is_bound = false
      self.is_client = false # may be reused as a server...
      self.is_shutdown = true
      self.shutdown_reason = how
    }
    return result == 0
  }

  /**
   * set_option(option: int, value: any)
   * 
   * Sets the options of the current socket.
   * @note Only `SO_` variables are valid option types
   * @return bool
   */
  set_option(option, value) {
    if !option or !value 
      die SocketException('both option and value are required')
    if !is_int(option) 
      die SocketException('integer expected for option, ${typeof(option)} given')

    if option == SO_TYPE or option == SO_ERROR
      die Exception('the given option is read-only')

    var result = self._check_error(_setsockopt(self.id, option, value)) >= 0

    if result {
      # get an update on SO_SNDTIMEO and SO_RCVTIMEO
      if option == SO_SNDTIMEO self.send_timeout = value
      else if option == SO_RCVTIMEO self.receive_timeout = value
    }

    return result == 0
  }

  /**
   * get_option(option: int)
   * 
   * Gets the options set on the current socket
   * @return any
   */
  get_option(option) {
    if !option
      return nil

    if !is_int(option) 
      die SocketException('integer expected for option, ${typeof(option)} given')

    # we have a local copy of SO_RCVTIMEO and SO_SNDTIMEO
    # we can simply return them when required
    if option == SO_RCVTIMEO return self.receive_timeout
    else if option == SO_SNDTIMEO return self.send_timeout

    return _getsockopt(self.id, option)
  }

  /**
   * set_blocking(mode: bool)
   * 
   * Sets if the socket should operate in blocking or non-blocking mode. `true` for blocking 
   * (default) and `false` for non-blocking.
   */
  set_blocking(mode) {
    if !is_bool(mode) die SocketException('boolean expected')
    self.is_blocking = mode
  }

  /**
   * info()
   * 
   * Returns a dictionary containing the address, port and family of the current socket or an 
   * empty dictionary if the socket information could not be retrieved.
   * @return dictionary
   */
  info() {
    return _getsockinfo(self.id)
  }

  @to_string() {
    return '<Socket id: ${self.id}, closed: ${self.is_closed}, listening: ' +
        '${self.is_listening}, connected: ${self.is_connected}, bound: ${self.is_bound}>'
  }
}

/**
 * get_address_info(address: number [, type: string [, family: int]])
 * 
 * returns ip and name information of a given address
 * @return dictionary
 */
def get_address_info(address, type, family) {
  if !is_string(address)
    die SocketException('string expected for address, ${typeof(address)} given')
  if type != nil and !is_string(type)
    die SocketException('string expected for type, ${typeof(type)} given')

  if !type type = 'http'
  if !family family = AF_INET

  return _getaddrinfo(address, type, family)
}
