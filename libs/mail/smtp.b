import curl { 
  Option, 
  Info, 
  Curl, 
  CurlList
}
import .message
import .constants

/**
 * Transport class can be used to send email messages through an SMTP server.
 * 
 * The class constructor takes a single argument options, which should be a 
 * dictionary containing various options for the connection. If the options 
 * are not provided or are not a dictionary, the constructor will raise an 
 * exception. 
 * 
 * The class uses the options to set various properties such as the host and 
 * port of the SMTP server, the username and password for authentication, and 
 * various options for connecting to the server such as the use of TLS, and 
 * timeout.
 */
class Transport {
  var messages = []

  /**
   * The Transport class accepts a dictionary that can be used to configure how 
   * it behaves. The dictionary can contain one or more of the following.
   * 
   * - __host__: The host address of the SMTP server. (Default: localhost)
   * - __port__: The port number of the SMTP server. (Default: 465)
   * - __username__: The access username for the SMTP user.
   * - __password__: The password for the connection user.
   * - __tls__: The TLS mode of the connection. One of {TLS_TRY} (default), {TLS_CONTROL}, 
   *    {TLS_ALL} or {TLS_NONE}.
   * - __debug__: Whether to print debug information or not. (Default: false)
   * - __verify_peer__: If the peer certificate should be verified or not. (Default: false)
   * - __verify_host__: If the host certificate should be verified or not. (Default: false)
   * - __proxy__: The address of the proxy server if any.
   * - __proxy_username__: The username for the proxy connection.
   * - __proxy_password__: The password for the user of the proxy connection.
   * - __verify_proxy_peer__: If the peer certificate of the proxy should be verified or 
   *    not. (Default: The value of __verify_peer__)
   * - __verify_proxy_host__: If the host certificate of the proxy should be verified or 
   *    not. (Default: The value of __verify_host__)
   * - __timeout__: The request timeout in milliseconds. (Default: 30,000)
   * 
   * @param dict? options
   * @constructor
   */
  Transport(options) {
    if options != nil and !is_dict(options)
      die Exception('dictionary expected as argument to constructor')
    if !options options = {}

    self._host = options.get('host', 'localhost')
    self._port = options.get('port', 465)
    self._username = options.get('username', nil)
    self._password = options.get('password', nil)
    self._tls = options.get('tls', constants.TLS_TRY)
    self._debug = options.get('debug', false)
    self._verify_peer = options.get('verify_peer', false)
    self._verify_host = options.get('verify_host', false)
    self._proxy = options.get('proxy', nil)
    self._proxy_user = options.get('proxy_username', nil)
    self._proxy_pass = options.get('proxy_password', nil)
    self._verify_proxy_peer = options.get('verify_proxy_peer', self._verify_peer)
    self._verify_proxy_host = options.get('verify_proxy_host', self._verify_host)
    self._timeout = options.get('timeout', 30000)  # in milliseconds (default = 30s)
  }

  _init() {
    var curl = Curl()

    curl.set_option(Option.URL, '${self._tls == constants.TLS_NONE ? "smtp" : "smtps"}://${self._host}:${self._port}')
    curl.set_option(Option.USE_SSL, self._tls)
    curl.set_option(Option.SSL_VERIFYPEER, self._verify_peer)
    curl.set_option(Option.SSL_VERIFYHOST, self._verify_host)
    curl.set_option(Option.VERBOSE, self._debug)
    curl.set_option(Option.TIMEOUT_MS, self._timeout)

    if self._proxy {
      curl.set_option(Option.PROXY, self._proxy)
      if self._proxy_user curl.set_option(Option.PROXYUSERNAME, self._proxy_user)
      if self._proxy_pass curl.set_option(Option.PROXYPASSWORD, self._proxy_pass)
      curl.set_option(Option.PROXY_SSL_VERIFYPEER, self._verify_proxy_peer)
      curl.set_option(Option.PROXY_SSL_VERIFYHOST, self._verify_proxy_host)
    }

    return curl
  }

  /**
   * Adds an email message to the list of messages to be sent.
   * 
   * @param Message message
   * @returns Transport
   */
  add_message(message) {
    self.messages.append(message)
    return self
  }

  /**
   * Tests the connection to the SMTP server
   * 
   * @returns bool
   */
  test_connection() {
    var curl = self._init()
    curl.set_option(Option.CONNECT_ONLY, true)

    # send the email
    curl.send()

    return curl.get_info(Info.RESPONSE_CODE) == 250
  }

  /**
   * Verifys an email address
   * 
   * @param string address
   * @returns bool
   */
  verify(address) {
    var curl = self._init()
    curl.set_option(Option.MAIL_RCPT, CurlList([address]))

    # send the VRFY and close
    curl.send()
    curl.close()

    return curl.get_info(Info.RESPONSE_CODE) == 252
  }

  /**
   * Send the email messages and returns `true` if the message was successfully 
   * sent or `false` otherwise.
   * 
   * @returns bool
   */
  send() {
    var response_codes = []

    var curl = self._init()
    curl.set_option(Option.USERNAME, self._username)
    curl.set_option(Option.PASSWORD, self._password)

    for message in self.messages {
      var mail = message.build(curl)

      curl.set_option(Option.MAIL_FROM, mail.from)
      curl.set_option(Option.MAIL_RCPT, CurlList(mail.to))
      curl.set_option(Option.HTTPHEADER, CurlList(mail.headers))
      curl.set_option(Option.MIMEPOST, mail.mime)

      # send the email
      curl.send()

      # add response to results
      response_codes.append(curl.get_info(Info.RESPONSE_CODE))
    }

    # close the connection
    curl.close()

    # return single response if there was only one message or a list of 
    # response codes if more than one message was sent in the transport.
    return response_codes.length() == 1 ? response_codes[0] : response_codes
  }
}


/**
 * Returns a new instance of SMTP {Transport} with the given __options__.
 * 
 * @params {dict?} options See {Transport}
 * @returns Transport
 */
def smtp(options) {
  if options != nil and !is_dict(options)
    die Exception('dictionary expected as argument to constructor')
  return Transport(options)
}
