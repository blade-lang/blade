#!-- part of the http module

import ._process

import .request { HttpRequest }

import url
import socket
import .response { HttpResponse }

/**
 * Handles http requests.
 * @note This client do not currently support the compress, deflate and gzip transfer encoding.
 */
class HttpClient {
  
  /**
   * The user agent of the client used to make the request
   * @default Blade HTTP Client/1.0
   */
  var user_agent = 'Blade HTTP Client/1.0'

  /**
   * Indicates if we receive a redirect from a server, this flag tells us whether 
   * we should follow it or not.
   * @default true
   */
  var follow_redirect = true

  /**
   * Indicates if the site you're connecting to uses a different host name that what
   * they have mentioned in their server certificate's commonName (or subjectAltName) 
   * fields, connection will fail. You can skip this check by setting to true, but this 
   * will make the connection less secure.
   * @default false
   */
  var skip_hostname_verification = false

  /**
   * Indicates if you want to connect to a site who isn't using a certificate that is
   * signed by one of the certs in the CA bundle you have, you can skip the verification 
   * of the server's certificate. This makes the connection A LOT LESS SECURE.
   * @default false
   */
  var skip_peer_verification = false

  # ...
  var cookie_file

  /**
   * the site that refers us to the current site
   * @default nil
   */
  var referer

  /**
   * If you have a CA cert for the server stored someplace else than in the default bundle
   */
  var ca_cert

  /**
   * The connect timeout duration in milliseconds
   * @default 60s
   */
  var connect_timeout = 60000

  /**
   * The receive timeout duration in milliseconds
   * @default 300s
   */
  var receive_timeout = 300000

  /**
   * A dictionary of headers sent along with the request
   */
  var headers = {}

  /**
   * Indicates whether to remove the expect header or not only applies to requests with 
   * files in the body
   * @default false
   */
  var no_expect = false

  /**
   * send_request(url: string, [method: string = 'GET', data: string])
   *
   * Sends an Http request and returns a HttpResponse.
   * 
   * @default method: GET
   * @return HttpResponse
   * @throws SocketException, Exception
   */
  send_request(uri, method, data) {

    if !uri or !is_string(uri) 
      die Exception('invalid url')

    if !method method = 'GET'

    if data != nil and !is_string(data) and !is_dict(data)
      die Exception('string expected, ${typeof(data)} given')

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

    return request.send(uri, method.upper(), data, {
      follow_redirect: self.follow_redirect,
      connect_timeout: self.connect_timeout,
      receive_timeout: self.receive_timeout,
      skip_hostname_verification: self.skip_hostname_verification,
      skip_peer_verification: self.skip_peer_verification,
      user_agent: self.user_agent,
      cookie_file: self.cookie_file,
    })
  }

  /**
   * get(url: string)
   *
   * sends an Http GET request and returns an HttpResponse.
   * @returns HttpResponse
   * @throws Exception, SocketExcepion, HttpException
   */
  get(url) {
    return self.send_request(url, 'GET')
  }

  /**
   * post(url: string, [data: string | bytes])
   *
   * sends an Http POST request and returns an HttpResponse.
   * @returns HttpResponse
   * @throws Exception, SocketExcepion, HttpException
   */
  post(url, data) {
    return self.send_request(url, 'POST', data)
  }

  /**
   * put(url: string, [data: string | bytes])
   *
   * sends an Http PUT request and returns an HttpResponse.
   * @returns HttpResponse
   * @throws Exception, SocketExcepion, HttpException
   */
  put(url, data) {
    return self.send_request(url, 'PUT', data)
  }

  /**
   * delete(url: string)
   *
   * sends an Http DELETE request and returns an HttpResponse.
   * @returns HttpResponse
   * @throws Exception, SocketExcepion, HttpException
   */
  delete(url) {
    return self.send_request(url, 'DELETE', nil)
  }
}
