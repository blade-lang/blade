/**
 * HTTP
 *
 * Provides interface for working with Http client requests
 * and servers.
 * @copyright 2021, Ore Richard Muyiwa
 */

class HttpClient {
  # the user agent of the client used to make the request
  var _user_agent = 'Mozilla/4.0'

  # if we receive a redirect from a server,
  # this flag tells us whether we should follow it or not.
  var _follow_redirect = true

  # if the site you're connecting to uses a different host name that what
  # they have mentioned in their server certificate's commonName (or
  # subjectAltName) fields, connection will fail. You can skip
  # this check by setting to true, but this will make the connection less secure.
  var _skip_hostname_verification = false

  # if you want to connect to a site who isn't using a certificate that is
  # signed by one of the certs in the CA bundle you have, you can skip the
  # verification of the server's certificate. This makes the connection
  # A LOT LESS SECURE.
  var _skip_peer_verification = false

  # ...
  var _cookie_file

  # the site that refers us to the current site
  var _referer = ''

  # if you have a CA cert for the server stored someplace else 
  # than in the default bundle
  var _ca_cert

  # The request timeout duration in milliseconds
  # set to -1 to set no timeout limit to the request
  var _timeout = -1

  # custom request headers
  var _headers = {}

  # whether to remove the expect header or not
  # only applies to requests with files in the body
  var _no_expect = false

  HttpClient() {
  }

  user_agent(str) {
    if !is_string(str) die Exception('string expected')
    self._user_agent = str
    return self
  }

  ignore_redirect() {
    self._follow_redirect = false
    return self
  }

  skip_hostname_verification() {
    self._skip_hostname_verification = true
    return self
  }

  skip_peer_verification() {
    self._skip_peer_verification = true
    return self
  }

  referer(str) {
    if !is_string(str) die Exception('string expected')
    self._referer = str
    return self
  }

  timeout(duration) {
    if !is_int(duration) die Exception('integer expected')
    self._timeout = duration
    return self
  }

  headers(data) {
    if !is_dict(data) die Exception('dictionary expected')
    self._headers = data
    return self
  }

  cookie_file(file) {
    if !is_file(file) die Exception('file expected')
    self._cookie_file = file
    return self
  }

  ca_cert_file(file) {
    if !is_file(file) die Exception('file expected')
    self._ca_cert = file
    return self
  }

  no_expect() {
    self._no_expect = true
    return self
  }

  # cask method
  __client(url, user_agent, referer, timeout, follow_redirect,
          skip_hostname_verification, skip_peer_verification, 
          ca_cert, cookie_file, method, no_expect){
  }

  _process_header(header, version_callback) {
    var result = {}

    # Follow redirect headers...
    var data = header.trim().split('\r\n')

    iter var i = 0; i < data.length(); i++ {
      var d = data[i].index_of(':')
      if d > -1 {
        result.add(data[i][0,d], data[i][d + 1,data[i].length()])
      } else if(data[i].lower().starts_with('http/')){
        var http_version = data[i].split(' ')[0].replace('http/', '')
        if version_callback version_callback(http_version)
      }
    }

    return result
  }

  _process_reponse(response) {
    # result to return
    var result = {
      status_code: 0,
      http_version: '1.0',
      time_taken: 0,
      redirects: 0,
      responder: nil,
      headers: {},
      error: nil,
      body: nil
    }

    var status_code = result['status_code']  = response[0]
    var error = result['error'] = response[1]
    var headers = result['headers'] = self._process_header(response[2], |s|{
      result['http_version'] = s
    })
    var body = result['body']  = response[3]
    result['time_taken'] = response[4]
    result['redirects'] = response[5]
    result['responder'] = response[6]

    if error {
      result['error'] = error
    }

    return result
  }

  _make_request(method, url, data) {
    if url.length() > 0 {
      var has_file = false

      if is_dict(data) {
        for value in data {
          if is_file(value) has_file = true
        }
      }

      var response = self.__client(url, self._user_agent,
          self._referer, self._headers, self._timeout, self._follow_redirect, 
          self._skip_hostname_verification, self._skip_peer_verification,
          self._ca_cert, self._cookie_file, method.upper(),
          data, self._no_expect, has_file
      )
      
      return self._process_reponse(response)
    } else {
      die Exception("invalid url '${url}'")
    }
  }

  # Makes Http GET request to the given URL
  # @return dictionary
  get(url) {
    if !is_string(url) die Exception('string expected for url')
    return self._make_request('GET', url)
  }

  # Makes Http POST request to the given URL with the given data
  # @return dictionary
  post(url, data) {
    if !is_dict(data) and !is_string(data) 
      die Exception('post body must be a dictionary or string')
    return self._make_request('POST', url, data)
  }
}