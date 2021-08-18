#
# @module socket
#
# Provides interface for working with Socket clients
# and servers.
# @copyright 2021, Ore Richard Muyiwa
# 

import _socket
import _os

var _platform = _os.platform

/**
 * Types
 */
var SOCK_STREAM    = 1 # stream socket
var SOCK_DGRAM     = 2 # datagram socket
var SOCK_RAW       = 3 # raw-protocol interface
var SOCK_RDM       = 4 # reliably-delivered message
var SOCK_SEQPACKET = 5 # sequenced packet stream

/**
 * Option flags per-
 */
var SO_DEBUG         = 1 # turn on debugging info recording
var SO_ACCEPTCONN # socket has had listen()
var SO_REUSEADDR  # allow local address reuse
var SO_KEEPALIVE  # keep connections alive
var SO_DONTROUTE  # just use interface addresses
var SO_BROADCAST  # permit sending of broadcast msgs
var SO_USELOOPBACK   = 40 # bypass hardware when possible
var SO_LINGER     # linger on close if data present (in ticks)
var SO_OOBINLINE  # leave received OOB data in line
var SO_REUSEPORT  # allow local address & port reuse
var SO_TIMESTAMP

/**
 * Additional options, not kept in so_options.
 */
var SO_SNDBUF    # send buffer size
var SO_RCVBUF    # receive buffer size
var SO_SNDLOWAT  # send low-water mark
var SO_RCVLOWAT  # receive low-water mark
var SO_SNDTIMEO  # send timeout
var SO_RCVTIMEO  # receive timeout
var SO_ERROR     # get error status and clear
var SO_TYPE      # get socket type

/**
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
var SOL_SOCKET = 0xffff # options for socket level

using _platform {
  when 'linux' {
    SO_REUSEADDR     = 2
    SO_KEEPALIVE     = 9
    SO_DONTROUTE     = 5
    SO_BROADCAST     = 6
    SO_LINGER        = 13
    SO_OOBINLINE     = 10
    SO_REUSEPORT     = 14
    SO_TIMESTAMP     = 29
    SO_ACCEPTCONN    = 30
    #...
    SO_TYPE      = 3
    SO_ERROR     = 4
    SO_SNDBUF    = 7
    SO_RCVBUF    = 8
    SO_RCVLOWAT  = 18
    SO_SNDLOWAT  = 19
    SO_RCVTIMEO  = 20
    SO_SNDTIMEO  = 21
    #...
    SOL_SOCKET = 1
  }
  default {
    SO_ACCEPTCONN    = 2
    SO_REUSEADDR     = 4
    SO_KEEPALIVE     = 8
    SO_DONTROUTE     = 10
    SO_BROADCAST     = 20
    SO_LINGER        = 80
    SO_OOBINLINE     = 100
    SO_REUSEPORT     = 200
    SO_TIMESTAMP     = 400
    #...
    SO_TYPE      = 1008
    SO_ERROR     = 1007
    SO_SNDBUF    = 1001
    SO_RCVBUF    = 1002
    SO_SNDLOWAT  = 1003
    SO_RCVLOWAT  = 1004
    SO_SNDTIMEO  = 0x1005
    SO_RCVTIMEO  = 0x1006
    #...
    SOL_SOCKET = 0xffff
  }
}

/**
 * Address families.
 */
var AF_UNSPEC    = 0 # unspecified
var AF_UNIX      = 1 # local to host (pipes)
var AF_LOCAL     = 1 # same as AF_UNIX
var AF_INET      = 2 # internetwork: UDP, TCP, etc.
var AF_IMPLINK   = 3 # arpanet imp addresses
var AF_PUP       = 4 # pup protocols: e.g. BSP
var AF_CHAOS     = 5 # mit CHAOS protocols
var AF_NS        = 6 # XEROX NS protocols
var AF_ISO       = 7 # ISO protocols
var AF_OSI       = 7 # OSI protocols (same as ISO)
var AF_ECMA      = 8 # European computer manufacturers
var AF_DATAKIT   = 9 # datakit protocols
var AF_CCITT     = 10 # CITT protocols, X.25 etc
var AF_SNA       = 11 # IBM SNA
var AF_DECnet    = 12 # DECnet
var AF_DLI       = 13 # DEC Direct data link interface
var AF_LAT       = 14 # LAT
var AF_HYLINK    = 15 # NSC Hyperchannel
var AF_APPLETALK = 16 # AppleTalk
var AF_INET6     = 30 # ipv6

/** 
 * Standard well-defined IP protocols.
 */

var IPPROTO_IP = 0            #  Dummy protocol for TCP.  
var IPPROTO_ICMP = 1          #  Internet Control Message Protocol.  
var IPPROTO_IGMP = 2          #  Internet Group Management Protocol. 
var IPPROTO_IPIP = 4          #  IPIP tunnels (older KA9Q tunnels use 94).  
var IPPROTO_TCP = 6           #  Transmission Control Protocol.  
var IPPROTO_EGP = 8           #  Exterior Gateway Protocol.  
var IPPROTO_PUP = 12          #  PUP protocol.  
var IPPROTO_UDP = 17          #  User Datagram Protocol.  
var IPPROTO_IDP = 22          #  XNS IDP protocol.  
var IPPROTO_TP = 29           #  SO Transport Protocol Class 4.  
var IPPROTO_DCCP = 33         #  Datagram Congestion Control Protocol.  
var IPPROTO_IPV6 = 41         #  IPv6 header.  
var IPPROTO_RSVP = 46         #  Reservation Protocol.  
var IPPROTO_GRE = 47          #  General Routing Encapsulation.  
var IPPROTO_ESP = 50          #  encapsulating security payload.  
var IPPROTO_AH = 51           #  authentication header.  
var IPPROTO_MTP = 92          #  Multicast Transport Protocol.  
var IPPROTO_BEETPH = 94       #  IP option pseudo header for BEET.  
var IPPROTO_ENCAP = 98        #  Encapsulation Header.  
var IPPROTO_PIM = 103         #  Protocol Independent Multicast.  
var IPPROTO_COMP = 108        #  Compression Header Protocol.  
var IPPROTO_SCTP = 132        #  Stream Control Transmission Protocol.  
var IPPROTO_UDPLITE = 136     #  UDP-Lite protocol.  
var IPPROTO_MPLS = 137        #  MPLS in IP.  
var IPPROTO_RAW = 255         #  Raw IP packets.  
var IPPROTO_MAX = 256 

/**
 * howto arguments for shutdown(2), specified by Posix.1g.
 */
var SHUT_RD       = 0 # shut down the reading side
var SHUT_WR       = 1 # shut down the writing side
var SHUT_RDWR     = 2 # shut down both sides

/**
 * Maximum queue length specifiable by listen.
 */
var SOMAXCONN = 128

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
}

/**
 * class Socket
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
   * Socket(family: number [, type: number, protocol: number [, id: number]])
   * @constructor
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

      var id = _socket.create(self.family, self.type, self.protocol)
      if id == -1 die SocketException('could not create socket')
      self.id = id
    } else {
      self.id = id
    }
  }

  # checks if a response code is valid
  # returns the code if it is or throws an SocketException otherwise
  _check_error(code) {
    var err = _socket.error(code)
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

    var result = self._check_error(_socket.connect(self.id, host, port, self.family, timeout, self.is_blocking))
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

    var result = self._check_error(_socket.bind(self.id, host, port, self.family))
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

    return self._check_error(_socket.send(self.id, message, flags))
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
    
    var result = _socket.recv(self.id, length, flags)
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

    var result = self._check_error(_socket.listen(self.id, queue_length))
    if result {
      self.is_listening = true
    }
    return result
  }

  accept() {
    if self.is_bound and self.is_listening and !self.is_closed {
      var result = _socket.accept(self.id)

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

    if self._check_error(_socket.close(self.id)) == 0 {
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

    var result = self._check_error(_socket.shutdown(self.id, how)) >= 0
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

    var result = self._check_error(_socket.setsockopt(self.id, option, value)) >= 0

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

    return _socket.getsockopt(self.id, option)
  }

  set_blocking(mode) {
    if !is_bool(mode) die SocketException('boolean expected')
    self.is_blocking = mode
  }

  info() {
    return _socket.getsockinfo(self.id)
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

  return _socket.getaddrinfo(address, type, family)
}
