import curl { 
  Option, 
  Info, 
  Curl, 
  CurlList, 
  CurlMime
}
import .message {
  Message
}
import .constants

var _flags_regex = '/\((\\\?([^)]+))?\)/'
var _list_regex = '/^[*] LIST \(\\\?([^)]*)\) "([^"]+)" (.*)$/'

/**
 * The POP3 class provides an interface for connecting to an POP3 (Post Office Protocol) server 
 * and interacting with the server via the POP3 protocol.
 * 
 * This class includes operations for creating, deleting, and renaming mailboxes, checking for new 
 * messages, permanently removing messages, setting and clearing flags searching, and selective 
 * fetching of message attributes, texts, and portions.
 */
class POP3 {
  var _curl
  var _base_url

  /**
   * The POP3 class accepts a dictionary that can be used to configure how 
   * it behaves. The dictionary can contain one or more of the following.
   * 
   * - __host__: The host address of the POP3 server. (Default: localhost)
   * - __port__: The port number of the POP3 server. (Default: 110)
   * - __username__: The access username for the POP3 user.
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
   * @param {dict?} options
   * @constructor
   */
  POP3(options) {
    if options != nil and !is_dict(options)
      die Exception('dictionary expected as argument to constructor')
    if !options options = {}

    
    self._host = options.get('host', 'localhost')
    self._port = options.get('port', 110)
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

    self._base_url = '${self._tls == constants.TLS_ALL ? "pop3s" : "pop3"}://${self._host}:${self._port}'
  }

  _init(url) {
    if !url url = self._base_url
    else url = self._base_url + url

    var curl = self._curl

    if !curl {
      curl = Curl()

      curl.set_option(Option.USE_SSL, self._tls)
      curl.set_option(Option.USERNAME, self._username)
      curl.set_option(Option.PASSWORD, self._password)
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

      self._curl = curl
    }

    curl.set_option(Option.URL, url)

    return curl
  }

  _to_list(data, type, name) {
    return data.trim().split('\n').reduce(@(initial, x) {
      var data = x.trim().split(' ')

      initial.append({
        uid: to_number(data[0]),
        size: to_number(data[1]),
      })
      
      return initial
    }, [])
  }

  _to_uid_list(data, type, name) {
    return data.trim().split('\n').reduce(@(initial, x) {
      var data = x.trim().split(' ')

      initial.append({
        uid: to_number(data[0]),
        id: data[1],
      })
      
      return initial
    }, [])
  }

  /**
   * Executes an POP3 command.
   * 
   * @param {string} command The command to execute.
   * @param {string?} path The path segement of the request url.
   * @param {bool?} no_transfer Set to `true` if the command will return the requested data 
   *    as response response. Default `false`.
   * @return string The response from the server.
   */
  exec(command, path, no_transfer) {
    if command != nil and !is_string(command)
      die Exception('string expected in argument 1 (command)')
    if path != nil and !is_string(path)
      die Exception('string expected in argument 2 (path)')
    if no_transfer != nil and !is_bool(no_transfer)
      die Exception('boolean expected in argument 3 (no_transfer)')

    var curl = self._init(path)
    curl.set_option(Option.CUSTOMREQUEST, command)

    if no_transfer {
      curl.set_option(Option.NOBODY, true)
    }

    return curl.send().body.to_string()
  }

  /**
   * Returns a list of dictionaries containing the `uid` and `size` of each message in the 
   * mail if the _uid_ argument is not given or the content of the message identified by the 
   * given _uid_.
   * 
   * @param {number?} uid
   * @return {list[dictionary]|string}
   */
  list(uid) {
    if uid != nil and !is_number(uid)
      die Exception('number expected at argument 1 (uid)')

    return uid == nil ? 
      self._to_list(self._init().send().body.to_string()) :
      self._init('/${uid}').send().body.to_string()
  }

  /**
   * Returns a list of dictionaries containing the `uid` and `id` for every message in the mailbox 
   * based on their unique ids.
   * 
   * @return {list[dictionary]}
   */
  uid_list() {
    return self._to_uid_list(self.exec('UIDL'))
  }

  /**
   * Retrieves the whole message with the specified _uid_.
   * 
   * @param {number} uid
   * @return {string}
   */
  retr(uid) {
    if !is_number(uid)
      die Exception('number expected in argument 1 (uid)')
    
    return self._init('/${uid}').send().body.to_string()
  }

  /**
   * Returns a dictionary containing the message `count` and `size` of the mailbox.
   * 
   * @return {dictionary}
   */
  stat() {
    var curl = self._init()
    curl.set_option(Option.CUSTOMREQUEST, 'STAT')
    curl.set_option(Option.NOBODY, true)

    var data = curl.send().headers.trim().split('\r\n').last().split(' ')
    
    if data[0].lower() == '+ok' {
      return {
        count: to_number(data[1]),
        size: to_number(data[2])
      }
    }

    die Exception(' '.join(data))
  }

  /**
   * Instructs the POP3 server to mark the message _uid_ as deleted. Any future reference 
   * to the message-number associated with the message in a POP3 command generates an error.  
   * The POP3 server does not actually delete the message until the POP3 session enters the 
   * UPDATE state.
   * 
   * @param {number} uid
   */
  delete(uid) {
    if !is_string(uid)
      die Exception('string expected in argument 1 (uid)')

    self.exec('DELE', uid, true)
  }

  /**
   * Does nothing. It merely ask the server to reply with a positive response.
   * 
   * @note It's useful for a keep-alive.
   */
  noop() {
    return self.exec('NOOP', nil, true)
  }

  /**
   * Instructs the server to unmark any messages have been marked as deleted.
   */
  rset() {
    return self.exec('RSET', nil, true)
  }

  /**
   * Retrieves the header for the message identified by `uid` plus `count` lines 
   * of the message after the header of message.
   * 
   * @param {number} uid
   * @param {number?} count (Default: 0)
   * @return {string}
   */
  top(uid, count) {
    if !is_number(uid)
      die Exception('number expected in argument 1 (uid)')
    if count != nil and !is_number(count)
      die Exception('number expected in argument 2 (count)')

    if !count count = 0
    return self.exec('TOP ${uid} ${count}')
  }

  /**
   * Closes the current POP3 session and disposes all associated network handles.
   */
  quit() {
    if self._curl {
      self._curl.close()
    }
  }

  /**
   * Returns the raw handle to the underlying networking (curl) client.
   */
  get_handle() {
    return self._curl
  }
}


/**
 * Returns a new instance of the POP3 class with the given options (if any) passed 
 * to the constructor.
 * 
 * @return {POP3}
 * @default
 */
def pop3(options) {
  if options != nil and !is_dict(options)
    die Exception('dictionary expected as argument to constructor')
  return POP3(options)
}
