#
# @module socket
#
# Provides interface for working with Socket clients
# and servers.
# @copyright 2021, Ore Richard Muyiwa
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
import _os

var _platform = _os.platform



/**
 * address helpers
 */
var IP_ANY     = '0.0.0.0'
var IP_LOCAL   = '127.0.0.1'

/**
 * class SocketException
 * exception class thrown from sockets
 */
class SocketException < Exception {
  SocketException(message) {
    self.message = message
  }

  @to_string() {
    return '<SocketException: ${self.message}>'
  }
}

/**
 * @class Socket
 *
 * Provides interface for working with Socket clients
 * and servers.
 */
class Socket {

  # Whenever a host is not given, the host will default
  # to localhost.
  var host = 'localhost'
  var port = 0

  # the default family for the socket is AF_INET
  var family = AF_INET

  # the default socket type is SOCK_STREAM
  var type = SOCK_STREAM

  # initialize protocol for default behavior...
  var protocol = IPPROTO_TCP

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

  /**
   * @constructor Socket
   * 
   * Socket(family: number [, type: number, protocol: number [, id: number]])
   * @example
   * Socket(AF_INET, SOCK_STREAM, 0)
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

      var id = _create(self.family, self.type, self.protocol)
      if id == -1 die SocketException('could not create socket')
      self.id = id
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

    var result = self._check_error(_bind(self.id, host, port, self.family))
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
      (self.shutdown_reason == SHUT_WR or 
        self.shutdown_reason == SHUT_RDWR)) 
      die SocketException('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die SocketException('socket not listening or connected')

    return self._check_error(_send(self.id, message, flags))
  }

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
    return result
  }

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
    return result
  }

  set_option(option, value) {
    if !option or !value 
      die SocketException('both option and value are required')
    if !is_int(option) 
      die SocketException('integer expected for option, ${typeof(option)} given')
    # if option < SO_DEBUG or option > SO_TYPE # @TODO: update SO_TYPE as options increase
    #   die SocketException('expected one of SO_* options')

    if option == SO_TYPE or option == SO_ERROR
      die Exception('the given option is read-only')

    var result = self._check_error(_setsockopt(self.id, option, value)) >= 0

    if result {
      # get an update on SO_SNDTIMEO and SO_RCVTIMEO
      if option == SO_SNDTIMEO self.send_timeout = value
      else if option == SO_RCVTIMEO self.receive_timeout = value
    }

    return result
  }

  get_option(option) {
    if !option
      return nil

    if !is_int(option) 
      die SocketException('integer expected for option, ${typeof(option)} given')
    # if option < SO_DEBUG or option > SO_TYPE # @TODO: update SO_TYPE as options increase
    #   die SocketException('expected one of SO_* options')

    # we have a local copy of SO_RCVTIMEO and SO_SNDTIMEO
    # we can simply return them when required
    if option == SO_RCVTIMEO return self.receive_timeout
    else if option == SO_SNDTIMEO return self.send_timeout

    return _getsockopt(self.id, option)
  }

  set_blocking(mode) {
    if !is_bool(mode) die SocketException('boolean expected')
    self.is_blocking = mode
  }

  info() {
    return _getsockinfo(self.id)
  }

  @to_string() {
    return '<Socket id: ${self.id}, closed: ${self.is_closed}, listening: ' +
        '${self.is_listening}, connected: ${self.is_connected}, bound: ${self.is_bound}>'
  }
}

/**
 * get_address_info(address: number, type: number, family: number)
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
