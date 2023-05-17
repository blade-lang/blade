#
# @module socket
#
# This module provides access to the underlying system socket management 
# implementations. It is meant to be used to provide more controlled and 
# specific operating system features and for implementing various standard 
# and custom network protocols and specifications for which Blade does not 
# provide a built-in implementation for.
# 
# This module defines a lot of constant that whose value complies with the 
# operating system specification and they should be used instead of a finite 
# value wherever available as values for these constants can change across 
# different OS implementations.
# 
# ### What's a Socket
# 
# Sockets are bidrectional communication medias for information exchange between 
# various processes within the same machine or different machines.
# 
# There are three important concepts that must important to know when working with 
# sockets.
# 
# 1. `Family`: This refer to the general group of sockets that a specific 
# protocol handled by a socket belongs to. This is any of the `AF_` constants.
# 2. `Types`: The type of communication between the two processes involved. And can 
# only be one of `SOCK_STREAM` or `SOCK_DGRAM`.
# 3. `Protocol`: This is to identify the variant protocol on which one or more 
# network protocols are based on. Typically `0` or any of the `IP_` constants.
# 
# A simple socket may be instanciated as follows:
# 
# ```blade
# import socket { Socket }
# var sock = Socket()
# ```
# > The `{ Socket }` in the import statement means we are only importing the `Socket` 
# > class and not the entire `socket` module. Other examples here will skip the assume 
# > you are importing just what you need out of the package but will not show the import 
# > statement.
# 
# The example above instantiates a socket without any arguments, and it is equivalent to:
# 
# ```blade
# Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
# ```
# 
# You can establish a connection with another socket with a known address and port 
# as follows:
# 
# ```blade
# var socket = Socket()
# socket.connect('127.0.0.1', 4000)
# ```
# 
# The above example connects to the process listening at port 4000 on host with IP 
# address 127.0.0.1. A connection is a pre-requisite to writing or reading from a socket.
# 
# After connecting to a socket, you can read and write data as follows:
# 
# ```blade
# var socket = Socket()
# socket.connect('127.0.0.1', 4000)
# 
# var message_from_client = socket.receive()
# socket.send('You sent: ' + message_from_client)
# ```
# 
# The above example simply replies the client with `You sent: ` + whatever the client 
# acutally sent.
# 
# Due to resource limitations, its good practice to always ensure to close sockets when 
# done with it. Doing this is pretty simple.
# 
# ```blade
# socket.close()
# ```
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _socket

# Types

/**
 * stream socket
 */
var SOCK_STREAM = _socket.SOCK_STREAM

/**
 * datagram socket
 */
var SOCK_DGRAM = _socket.SOCK_DGRAM

/**
 * raw-protocol interface
 */
var SOCK_RAW = _socket.SOCK_RAW

/**
 * reliably-delivered message
 */
var SOCK_RDM = _socket.SOCK_RDM

/**
 * sequenced packet stream
 */
var SOCK_SEQPACKET = _socket.SOCK_SEQPACKET


# Option flags per-socket.

/**
 * Turn on debugging info recording
 */
var SO_DEBUG = _socket.SO_DEBUG

/**
 * Socket has had listen()
 */
var SO_ACCEPTCONN = _socket.SO_ACCEPTCONN

/**
 * Allow local address reuse
 */
var SO_REUSEADDR = _socket.SO_REUSEADDR

/**
 * Keep connections alive
 */
var SO_KEEPALIVE = _socket.SO_KEEPALIVE

/**
 * Just use interface addresses
 */
var SO_DONTROUTE = _socket.SO_DONTROUTE

/**
 * Permit sending of broadcast msgs
 */
var SO_BROADCAST = _socket.SO_BROADCAST

/**
 * Bypass hardware when possible
 */
var SO_USELOOPBACK = _socket.SO_USELOOPBACK

/**
 * Linger on close if data present (in ticks)
 */
var SO_LINGER = _socket.SO_LINGER

/**
 * Leave received OOB data in line
 */
var SO_OOBINLINE = _socket.SO_OOBINLINE

/**
 * Allow local address & port reuse
 */
var SO_REUSEPORT = _socket.SO_REUSEPORT

/**
 * Timestamp received dgram traffic
 */
var SO_TIMESTAMP = _socket.SO_TIMESTAMP


# Additional options, not kept in so_options.

/**
 * Send buffer size
 */
var SO_SNDBUF = _socket.SO_SNDBUF

/**
 * Receive buffer size
 */
var SO_RCVBUF = _socket.SO_RCVBUF

/**
 * Send low-water mark
 */
var SO_SNDLOWAT = _socket.SO_SNDLOWAT

/**
 * Receive low-water mark
 */
var SO_RCVLOWAT = _socket.SO_RCVLOWAT

/**
 * Send timeout
 */
var SO_SNDTIMEO = _socket.SO_SNDTIMEO

/**
 * Receive timeout
 */
var SO_RCVTIMEO = _socket.SO_RCVTIMEO

/**
 * Get error status and clear
 */
var SO_ERROR = _socket.SO_ERROR

/**
 * Get socket type
 */
var SO_TYPE = _socket.SO_TYPE


# Level number for (get/set)sockopt() to apply to socket itself.

/**
 * Options for socket level
 */
var SOL_SOCKET = _socket.SOL_SOCKET


# Address families.

/**
 * Unspecified
 */
var AF_UNSPEC = _socket.AF_UNSPEC

/**
 * Local to host (pipes)
 */
var AF_UNIX = _socket.AF_UNIX

/**
 * Backward compatibility with AF_UNIX
 */
var AF_LOCAL = _socket.AF_LOCAL

/**
 * Internetwork: UDP, TCP, etc.
 */
var AF_INET = _socket.AF_INET

/**
 * Arpanet imp addresses
 */
var AF_IMPLINK = _socket.AF_IMPLINK

/**
 * PUP protocols: e.g. BSP
 */
var AF_PUP = _socket.AF_PUP

/**
 * MIT CHAOS protocols
 */
var AF_CHAOS = _socket.AF_CHAOS

/**
 * XEROX NS protocols
 */
var AF_NS = _socket.AF_NS

/**
 * ISO protocols
 */
var AF_ISO = _socket.AF_ISO

/**
 * ISO protocols (same as AF_ISO)
 */
var AF_OSI = _socket.AF_OSI

/**
 * European computer manufacturers
 */
var AF_ECMA = _socket.AF_ECMA

/**
 * Datakit protocols
 */
var AF_DATAKIT = _socket.AF_DATAKIT

/**
 * CCITT protocols, X.25 etc
 */
var AF_CCITT = _socket.AF_CCITT

/**
 * IBM SNA
 */
var AF_SNA = _socket.AF_SNA

/**
 * DECnet
 */
var AF_DECnet = _socket.AF_DECnet

/**
 * DEC Direct data link interface
 */
var AF_DLI = _socket.AF_DLI

/**
 * LAT
 */
var AF_LAT = _socket.AF_LAT

/**
 * NSC Hyperchannel
 */
var AF_HYLINK = _socket.AF_HYLINK

/**
 * Apple Talk
 */
var AF_APPLETALK = _socket.AF_APPLETALK

/**
 * IPv6
 */
var AF_INET6 = _socket.AF_INET6


# Protocol families, same as address families on most platforms.

/**
 * IPPROTO_IP
 */
var IPPROTO_IP = _socket.IPPROTO_IP

/**
 * IPPROTO_ICMP
 */
var IPPROTO_ICMP = _socket.IPPROTO_ICMP

/**
 * IPPROTO_IGMP
 */
var IPPROTO_IGMP = _socket.IPPROTO_IGMP

/**
 * IPPROTO_IPIP
 */
var IPPROTO_IPIP = _socket.IPPROTO_IPIP

/**
 * IPPROTO_TCP
 */
var IPPROTO_TCP = _socket.IPPROTO_TCP

/**
 * IPPROTO_EGP
 */
var IPPROTO_EGP = _socket.IPPROTO_EGP

/**
 * IPPROTO_PUP
 */
var IPPROTO_PUP = _socket.IPPROTO_PUP

/**
 * IPPROTO_UDP
 */
var IPPROTO_UDP = _socket.IPPROTO_UDP

/**
 * IPPROTO_IDP
 */
var IPPROTO_IDP = _socket.IPPROTO_IDP

/**
 * IPPROTO_TP
 */
var IPPROTO_TP = _socket.IPPROTO_TP

/**
 * IPPROTO_DCCP
 */
var IPPROTO_DCCP = _socket.IPPROTO_DCCP

/**
 * IPPROTO_IPV6
 */
var IPPROTO_IPV6 = _socket.IPPROTO_IPV6

/**
 * IPPROTO_RSVP
 */
var IPPROTO_RSVP = _socket.IPPROTO_RSVP

/**
 * IPPROTO_GRE
 */
var IPPROTO_GRE = _socket.IPPROTO_GRE

/**
 * IPPROTO_ESP
 */
var IPPROTO_ESP = _socket.IPPROTO_ESP

/**
 * IPPROTO_AH
 */
var IPPROTO_AH = _socket.IPPROTO_AH

/**
 * IPPROTO_MTP
 */
var IPPROTO_MTP = _socket.IPPROTO_MTP

/**
 * IPPROTO_BEETPH
 */
var IPPROTO_BEETPH = _socket.IPPROTO_BEETPH

/**
 * IPPROTO_ENCAP
 */
var IPPROTO_ENCAP = _socket.IPPROTO_ENCAP

/**
 * IPPROTO_PIM
 */
var IPPROTO_PIM = _socket.IPPROTO_PIM

/**
 * IPPROTO_COMP
 */
var IPPROTO_COMP = _socket.IPPROTO_COMP

/**
 * IPPROTO_SCTP
 */
var IPPROTO_SCTP = _socket.IPPROTO_SCTP

/**
 * IPPROTO_UDPLITE
 */
var IPPROTO_UDPLITE = _socket.IPPROTO_UDPLITE

/**
 * IPPROTO_MPLS
 */
var IPPROTO_MPLS = _socket.IPPROTO_MPLS

/**
 * IPPROTO_RAW
 */
var IPPROTO_RAW = _socket.IPPROTO_RAW

/**
 * IPPROTO_MAX
 */
var IPPROTO_MAX = _socket.IPPROTO_MAX


# howto arguments for shutdown(2), specified by Posix.1g.

/**
 * Shut down the reading side
 */
var SHUT_RD = _socket.SHUT_RD

/**
 * Shut down the writing side
 */
var SHUT_WR = _socket.SHUT_WR

/**
 * Shut down both sides
 */
var SHUT_RDWR = _socket.SHUT_RDWR


/**
 * Maximum queue length specifiable by listen.
 */
var SOMAXCONN = _socket.SOMAXCONN





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
 * The SocketException class is the general Exception type thrown from sockets
 */
class SocketException < Exception {
  @to_string() {
    return '<SocketException: ${self.message}>'
  }
}

/**
 * The Socket class provides interface for working with Socket clients
 * and servers.
 * @printable
 */
class Socket {

  /**
   * This property holds the host bound, to be bound to or connected to by the current socket.
   * Whenever a host is not given, the host will default to localhost.
   */
  var host = IP_LOCAL

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
   * `true` when the socket is running in a blocking mode, `false` otherwise.
   */
  var is_blocking = false

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
   * Socket(family: number [, type: number [, protocol: number]])
   * @example Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
   * @constructor  
   */
  Socket(family, type, protocol, id) {
    # NOTE: NEVER EVER SET `id` YOURSELF.
    # The parameter is meant to make `accept()`.

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

      self.id = self._check_error(_socket.create(self.family, self.type, self.protocol))
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

    var result = self._check_error(_socket.connect(self.id, host, port, self.family, timeout, self.is_blocking))
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

    var result = self._check_error(_socket.bind(self.id, host, port, self.family))
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

    return self._check_error(_socket.send(self.id, message, flags))
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
    
    var result = _socket.recv(self.id, length, flags)
    if is_string(result) or result == nil return result

    return self._check_error(result)
  }

  /**
   * read([length: int])
   * 
   * Reads bytes of the given length from the socket. If the length is not given, it default length of 
   * -1 indicating that the total available data on the socket stream will be read. 
   * 
   * > This method differs from `receive()` in that it does not check for a socket having data to 
   * > read or not and will block until data of _length_ have been read or no more data is available for 
   * > reading.
   * @note Only use this function after a call to `receive()` has succeeded.
   * @default Length = 1024
   * @return string
   */
  read(length) {
    if !length length = 1024

    if !is_int(length) 
      die SocketException('integer expected for length, ${typeof(length)} given')

    if self.id == -1 or self.is_closed or (self.is_shutdown and 
      (self.shutdown_reason == SHUT_RD or 
        self.shutdown_reason == SHUT_RDWR)) 
      die SocketException('socket is in an illegal state')

    if !self.is_listening and !self.is_connected
      die SocketException('socket not listening or connected')
    
    var result = _socket.read(self.id, length, 0)
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

    var result = self._check_error(_socket.listen(self.id, queue_length))
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
   * The accepted socket may not be used to accept more connections.  The original socket remains open.
   * @return Socket
   */
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

  /**
   * close()
   * 
   * Closes the socket
   * @return bool
   */
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

    var result = self._check_error(_socket.shutdown(self.id, how)) >= 0
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

    var result = self._check_error(_socket.setsockopt(self.id, option, value)) >= 0

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
    if !is_int(option) 
      die SocketException('integer expected for option, ${typeof(option)} given')

    # we have a local copy of SO_RCVTIMEO and SO_SNDTIMEO
    # we can simply return them when required
    if option == SO_RCVTIMEO return self.receive_timeout
    else if option == SO_SNDTIMEO return self.send_timeout

    return _socket.getsockopt(self.id, option)
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
   * Returns a dictionary containing the address, ipv6, port and family of the current socket or an 
   * empty dictionary if the socket information could not be retrieved.
   * @return dictionary
   */
  info() {
    return _socket.getsockinfo(self.id)
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

  return _socket.getaddrinfo(address, type, family)
}

/**
 * socket(family: number [, type: number [, protocol: number]])
 * 
 * Returns a new instance of a Socket.
 * @example socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
 * @see class Socket
 */
def socket(family, type, protocol) {
  return Socket(family, type, protocol)
}
