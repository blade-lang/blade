/**
 * HTTP
 *
 * Provides interfaces for working with Http client requests.
 * @copyright 2021, Ore Richard Muyiwa
 */
import 'url'
import 'socket'


/**
 * HttpResponse
 * represents the response to an Http request
 */
class HttpResponse {
  var status_code = 0
  var http_version = '1.0'
  var time_taken = 0
  var redirects = 0
  var responder
  var headers = {}
  var error # Exception instance
  var body

  to_string() {
    return to_string({
      status_code: self.status_code,
      http_version: self.http_version,
      time_taken: self.time_taken,
      redirects: self.redirects,
      responder: self.responder,
      headers: self.headers,
      error: self.error,
      body: self.body
    })
  }
}



/**
 * HttpStatus
 * represents the standard response codes to an Http request
 */
class HttpStatus {
  # Informational
  static var CONTINUE = 100
  static var SWITCHING_PROTOCOLS = 101
  static var PROCESSING = 102

  # Succcess
  static var OK = 200
  static var CREATED = 201
  static var ACCEPTED = 202
  static var NON_AUTHORITATIVE_INFORMATION = 203
  static var NO_CONTENT = 204
  static var RESET_CONTENT = 205
  static var PARTIAL_CONTENT = 206
  static var MULTI_STATUS = 207
  static var ALREADY_REPORTED = 208
  static var IM_USED = 226

  # Redirection
  static var MULTIPLE_CHOICES = 300
  static var MOVED_PERMANENTLY = 301
  static var FOUND = 302
  static var SEE_OTHER = 303
  static var NOT_MODIFIED = 304
  static var USE_PROXY = 305
  static var TEMPORARY_REDIRECT = 307
  static var PERMANENT_REDIRECT = 308

  # Client Error
  static var BAD_REQUEST = 400
  static var UNAUTHORIZED = 401
  static var PAYMENT_REQUIRED = 402
  static var FORBIDDEN = 403
  static var NOT_FOUND = 404
  static var METHOD_NOT_ALLOWED = 405
  static var NOT_ACCEPTABLE = 406
  static var PROXY_AUTHENTICATION_REQUIRED = 407
  static var REQUEST_TIMEOUT = 408
  static var CONFLICT = 409
  static var GONE = 410
  static var LENGTH_REQUIRED = 411
  static var PRECONDITION_FAILED = 412
  static var PAYLOAD_TOO_LARGE = 413
  static var REQUEST_URI_TOO_LONG = 414
  static var UNSUPPORTED_MEDIA_TYPE = 415
  static var REQUESTED_RANGE_NOT_SATISFIABLE = 416
  static var EXPECTATION_FAILED = 417
  static var TEAPOT = 418
  static var MISDIRECTED_REQUEST = 421
  static var UNPROCESSABLE_ENTITY = 422
  static var LOCKED = 423
  static var FAILED_DEPENDENCY = 424
  static var UPGRADE_REQUIRED = 426
  static var PRECONDITION_REQUIRED = 428
  static var TOO_MANY_REQUESTS = 429
  static var REQUEST_HEADER_FIELDS_TOO_LARGE = 431
  static var CONNECTION_CLOSED_WITHOUT_RESPONSE = 444
  static var UNAVAILABLE_FOR_LEGAL_REASONS = 451
  static var CLIENT_CLOSED_REQUEST = 499

  # Server Error
  static var INTERNAL_SERVER_ERROR = 500
  static var NOT_IMPLEMENTED = 501
  static var BAD_GATEWAY = 502
  static var SERVICE_UNAVAILABLE = 503
  static var GATEWAY_TIMEOUT = 504
  static var HTTP_VERSION_NOT_SUPPORTED = 505
  static var VARIANT_ALSO_NEGOTIATES = 506
  static var INSUFFICIENT_STORAGE = 507
  static var LOOP_DETECTED = 508
  static var NOT_EXTENDED = 510
  static var NETWORK_AUTHENTICATION_REQUIRED = 511
  static var NETWORK_CONNECT_TIMEOUT_ERROR = 599
}



/**
 * HttpClient
 *
 * handles http requests.
 */
class HttpClient {
  # the user agent of the client used to make the request
  var user_agent = 'Mozilla/4.0'

  # if we receive a redirect from a server,
  # this flag tells us whether we should follow it or not.
  var follow_redirect = true

  # if the site you're connecting to uses a different host name that what
  # they have mentioned in their server certificate's commonName (or
  # subjectAltName) fields, connection will fail. You can skip
  # this check by setting to true, but this will make the connection less secure.
  var skip_hostname_verification = false

  # if you want to connect to a site who isn't using a certificate that is
  # signed by one of the certs in the CA bundle you have, you can skip the
  # verification of the server's certificate. This makes the connection
  # A LOT LESS SECURE.
  var skip_peer_verification = false

  # ...
  var cookie_file

  # the site that refers us to the current site
  var referer = ''

  # if you have a CA cert for the server stored someplace else 
  # than in the default bundle
  var ca_cert

  # set timeouts to -1 to set no timeout limit
  # The connect timeout duration in milliseconds (default to 60 seconds)
  var connect_timeout = 60000
  # The send timeout duration in milliseconds
  var send_timeout = -1
  # The receive timeout duration in milliseconds
  var receive_timeout = -1

  # request headers
  var headers = {}

  # whether to remove the expect header or not
  # only applies to requests with files in the body
  var no_expect = false

  # the main http request method
  _do_http(url, method, data){

    var responder = url.absolute_uri, headers, body, time_taken, error
    var should_connect = true, redirect_count = 0, http_version = '1.0', status_code = 0

    while should_connect {

      var resolved_host = Socket.get_address_info(url.host)

      if resolved_host {
        var host = resolved_host.ip
        var port = url.port

        # construct message
        var message = '${method} ${url.path} HTTP/1.1'
        if !self.headers.contains('Host') {
          message += '\r\nHost: ${url.host}'
        }

        # handle no_expect
        if self.no_expect message += '\r\nExpect:'

        # add custom headers
        for key, value in self.headers {
          if key != 'Expect' and self.no_expect {
            message += '\r\n${key}: ${value}'
          }
        }

        if data {
          # append the correct content length to the message
          message += '\r\nContent-Length: ${data.length()}'
        }

        # append the body
        message += '\r\n\r\n${data}'

        # do real request here...
        var client = Socket()

        var start = time()

        # connect to the url host on the specified port and send the request message
        client.connect(host, port, self.connect_timeout)
        client.send(message)

        # receive the response...
        var response_data = client.receive()

        # gracefully handle responses being sent in multiple packets
        # if the request header contains the Content-Length,
        # get that length and keep reading until we have read the total
        # length of the response
        if response_data.index_of('Content-Length') {
          var m = response_data.matches('/Content\-Length:\s*\d+/')
          if m {
            var length = to_number(m[0].replace('/[^0-9]/', ''))
            while response_data.length() < length {
              # append the new data in the stream
              response_data += client.receive()
            }
          }
        }

        time_taken = time() - start

        # close client
        client.close()

        # separate the headers and the body
        var body_starts = response_data.index_of('\r\n\r\n')

        if body_starts {
          headers = response_data[0,body_starts].trim()
          body = response_data[body_starts + 2, response_data.length()].trim()
        }

        # @TODO: if there was a redirect, update the host and port
        # and change will connect to true
        headers = self._process_header(headers, |version, status|{
          http_version = version
          status_code  = status
        })

        if self.follow_redirect and headers.contains('Location') {
          url = Url.parse(headers['Location'])
          self.referer = headers['Location']
        } else {
          should_connect = false
        }
      } else {
        should_connect = false
        die Exception('could not resolve ip address')
      }
    }

    # return a valid HttpResponse
    var result = HttpResponse()
    
    result.headers = headers
    result.http_version = http_version
    result.status_code = status_code
    result.body  = body
    result.time_taken = time_taken
    result.redirects = redirect_count
    result.responder = responder

    return result
  }

  _process_header(header, meta_callback) {
    var result = {}

    if header {
      # Follow redirect headers...
      var data = header.trim().split('\r\n')

      iter var i = 0; i < data.length(); i++ {
        var d = data[i].index_of(':')
        if d > -1 {
          var key = data[i][0,d]
          var value = data[i][d + 1,data[i].length()]

          # handle cookies in header
          if key == 'Set-Cookie' {
            if result.contains(key) {
              result[key].append(value)
            } else {
              result[key] = [value]
            }
          } else {
            result.set(key, value)
          }
        } else if(data[i].lower().starts_with('http/')){
          var split = data[i].split(' ')
          var http_version = split[0].replace('http/', '')

          # call back with (version, status code)
          if meta_callback meta_callback(http_version, to_number(split[1]))
        }
      }
    }

    return result
  }

  /**
   * _send(url: string, [method: string = 'GET', data: string])
   *
   * sends an Http request and returns an HttpResponse
   * or throws one of SocketException or Exception if it fails
   */
  _send(url, method, data) {

    if !url or !is_string(url) 
      die Exception('invalid url')

    # parse the url into component parts
    url = Url.parse(url)

    if !method {
      # the request method
      # default = GET
      method = 'GET'
    }

    if data != nil and !is_string(data)
      die Exception('string expected, ${typeof(data)} give')

    return self._do_http(url, method.upper(), data)
  }

  /**
   * get(url: string)
   *
   * sends an Http GET request and returns an HttpResponse
   * or throws one of SocketException or Exception if it fails
   */
  get(url) {
    return self._send(url, 'GET')
  }

  /**
   * post(url: string, [data: string])
   *
   * sends an Http POST request and returns an HttpResponse
   * or throws one of SocketException or Exception if it fails
   */
  post(url, data) {
    return self._send(url, 'POST', data)
  }
}

