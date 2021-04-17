/**
 * HTTP
 *
 * Provides interface for working with Http client requests
 * and servers.
 * @copyright Ore Richard
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

  HttpClient() {
  }

  user_agent(str) {
    if !is_string(str) {
      die Exception('string expected')
    }
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
    if !is_string(str) {
      die Exception('string expected')
    }
    self._referer = str
    return self
  }

  timeout(duration) {
    if !is_int(duration) {
      die Exception('integer expected')
    }
    self._timeout = duration
    return self
  }

  # cask method
  __client(url, user_agent, referer, timeout, follow_redirect,
          skip_hostname_verification, skip_peer_verification){
  }

  # Makes Http GET request to the given URL and calls
  # the callback upon success or failure
  get(url, on_success, on_failure) {
    if url.length() > 0 {
      var response = self.__client(url, self._user_agent, 
          self._referer, self._timeout, self._follow_redirect, 
          self._skip_hostname_verification, self._skip_peer_verification
      )

      if !response.error {
        # request was successful... process here... 
      } else {
        die Exception(response.error)
      }
      
      # @TODO: Remove...
      echo response
    } else {
      die Exception("invalid url '${url}'")
    }
    return self
  }
}