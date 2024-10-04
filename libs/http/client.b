#!-- part of the http module

import .request { HttpRequest }

import url
import socket
import .response { HttpResponse }
import .defaults

/**
 * Handles http requests.
 * 
 * @note This client do not currently support the compress, deflate and gzip transfer encoding.
 */
class HttpClient {
  
  /**
   * The user agent of the client used to make the request. 
   * Default value &mdash; `Blade HTTP Client/1.0`.
   * @type string
   */
  var user_agent = defaults.user_agent

  /**
   * Indicates if we receive a redirect from a server, this flag tells us whether 
   * we should follow it or not. Default value is `true`.
   * @type bool
   */
  var follow_redirect = true

  /**
   * Indicates if the site you're connecting to uses a different host name that what
   * they have mentioned in their server certificate's commonName (or subjectAltName) 
   * fields, connection will fail. You can skip this check by setting to true, but this 
   * will make the connection less secure.
   * @type bool
   */
  var verify_hostname = true

  /**
   * Indicates if you want to connect to a site who isn't using a certificate that is
   * signed by one of the certs in the CA bundle you have, you can skip the verification 
   * of the server's certificate. This makes the connection A LOT LESS SECURE.
   * @type bool
   */
  var verify_peer = true

  # ...
  var cookie_file

  /**
   * The site that refers us to the current site
   * @type string
   */
  var referer

  /**
   * If you have a CA cert for the server stored someplace else than in the default bundle.
   * @type string
   */
  var ca_cert

  /**
   * The connect timeout duration in milliseconds. Default value is 60,000 (1 minute).
   * @type number
   */
  var connect_timeout = 60000

  /**
   * The receive timeout duration in milliseconds. Default value is 2,000 (2 seconds).
   * @type number
   */
  var receive_timeout = 2000

  /**
   * A dictionary of headers sent along with the request.
   * @type dict
   */
  var headers = {}

  /**
   * Indicates whether to remove the expect header or not only applies to requests with 
   * files in the body
   * @type bool
   */
  var no_expect = false

  /**
   * Sends an Http request and returns a HttpResponse.
   * 
   * @param string uri
   * @param string? method: Default value is `GET`.
   * @param string|dict|nil data
   * @param dict? headers: To override the instance options. 
   *    This can be very useful if you want to reuse the same 
   *    instance for multiple requests and headers scenarios.
   * @param dict? client request options
   * @returns HttpResponse
   * @dies SocketException
   * @dies Exception
   */
  send_request(uri, method, data, headers, options) {

    if !uri or !is_string(uri) {
      die Exception('invalid url')
    }

    if !method method = 'GET'

    if data != nil and !is_string(data) and !is_dict(data) {
      die Exception('string expected, ${typeof(data)} given')
    }

    if options != nil and !is_dict(options) {
      die Exception('dictionary expected, ${typeof(options)} given')
    }

    var request = HttpRequest()
    request.headers = self.headers

    if self.referer and !self.headers.contains('Referer') {
      request.headers.extend({
        'Referer': self.referer,
      })
    }

    var hst = request.headers.get('Host', nil)
    if hst and !uri.starts_with(hst) and !uri.starts_with('http://${hst}') and !uri.starts_with('https://${hst}') {
      uri = hst + uri
    }

    # parse the url into component parts
    uri = url.parse(uri)

    return request.send(uri, method.upper(), data, headers, {
      follow_redirect: self.follow_redirect,
      connect_timeout: self.connect_timeout,
      receive_timeout: self.receive_timeout,
      verify_hostname: self.verify_hostname,
      verify_peer: self.verify_peer,
      user_agent: self.user_agent,
      cookie_file: self.cookie_file,
    })
  }

  /**
   * Sends an Http GET request and returns an HttpResponse.
   * 
   * @param string url
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  get(url, headers) {
    return self.send_request(url, 'GET', nil, headers)
  }

  /**
   * Sends an Http POST request and returns an HttpResponse.
   * 
   * @param string url
   * @param string|bytes|nil data
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  post(url, data, headers) {
    return self.send_request(url, 'POST', data, headers)
  }

  /**
   * Sends an Http PUT request and returns an HttpResponse.
   * 
   * @param string url
   * @param string|bytes|nil data
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  put(url, data, headers) {
    return self.send_request(url, 'PUT', data, headers)
  }

  /**
   * Sends an Http PATCH request and returns an HttpResponse.
   * 
   * @param string url
   * @param string|bytes|nil data
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  patch(url, data, headers) {
    return self.send_request(url, 'PATCH', data, headers)
  }

  /**
   * Sends an Http DELETE request and returns an HttpResponse.
   * 
   * @param string url
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  delete(url, headers) {
    return self.send_request(url, 'DELETE', nil, headers)
  }

  /**
   * Sends an Http OPTIONS request and returns an HttpResponse.
   * 
   * @param string url
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  options(url, headers) {
    return self.send_request(url, 'OPTIONS', nil, headers)
  }

  /**
   * Sends an Http TRACE request and returns an HttpResponse.
   * 
   * @param string url
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  trace(url, headers) {
    return self.send_request(url, 'TRACE', nil, headers)
  }

  /**
   * Sends an Http HEAD request and returns an HttpResponse.
   * 
   * @param string url
   * @param dict? headers
   * @returns HttpResponse
   * @dies Exception
   * @dies SocketExcepion
   * @dies HttpException
   */
  head(url, headers) {
    return self.send_request(url, 'HEAD', nil, headers)
  }
}
