/**
 * HTTP
 *
 * Provides interfaces for working with Http client requests.
 * @copyright 2021, Ore Richard Muyiwa
 */

import url
import socket



/**
 * standard response codes to an Http request
 */
# Informational
var CONTINUE = 100
var SWITCHING_PROTOCOLS = 101
var PROCESSING = 102

# Succcess
var OK = 200
var CREATED = 201
var ACCEPTED = 202
var NON_AUTHORITATIVE_INFORMATION = 203
var NO_CONTENT = 204
var RESET_CONTENT = 205
var PARTIAL_CONTENT = 206
var MULTI_STATUS = 207
var ALREADY_REPORTED = 208
var IM_USED = 226

# Redirection
var MULTIPLE_CHOICES = 300
var MOVED_PERMANENTLY = 301
var FOUND = 302
var SEE_OTHER = 303
var NOT_MODIFIED = 304
var USE_PROXY = 305
var TEMPORARY_REDIRECT = 307
var PERMANENT_REDIRECT = 308

# Client Error
var BAD_REQUEST = 400
var UNAUTHORIZED = 401
var PAYMENT_REQUIRED = 402
var FORBIDDEN = 403
var NOT_FOUND = 404
var METHOD_NOT_ALLOWED = 405
var NOT_ACCEPTABLE = 406
var PROXY_AUTHENTICATION_REQUIRED = 407
var REQUEST_TIMEOUT = 408
var CONFLICT = 409
var GONE = 410
var LENGTH_REQUIRED = 411
var PRECONDITION_FAILED = 412
var PAYLOAD_TOO_LARGE = 413
var REQUEST_URI_TOO_LONG = 414
var UNSUPPORTED_MEDIA_TYPE = 415
var REQUESTED_RANGE_NOT_SATISFIABLE = 416
var EXPECTATION_FAILED = 417
var TEAPOT = 418
var MISDIRECTED_REQUEST = 421
var UNPROCESSABLE_ENTITY = 422
var LOCKED = 423
var FAILED_DEPENDENCY = 424
var UPGRADE_REQUIRED = 426
var PRECONDITION_REQUIRED = 428
var TOO_MANY_REQUESTS = 429
var REQUEST_HEADER_FIELDS_TOO_LARGE = 431
var CONNECTION_CLOSED_WITHOUT_RESPONSE = 444
var UNAVAILABLE_FOR_LEGAL_REASONS = 451
var CLIENT_CLOSED_REQUEST = 499

# Server Error
var INTERNAL_SERVER_ERROR = 500
var NOT_IMPLEMENTED = 501
var BAD_GATEWAY = 502
var SERVICE_UNAVAILABLE = 503
var GATEWAY_TIMEOUT = 504
var HTTP_VERSION_NOT_SUPPORTED = 505
var VARIANT_ALSO_NEGOTIATES = 506
var INSUFFICIENT_STORAGE = 507
var LOOP_DETECTED = 508
var NOT_EXTENDED = 510
var NETWORK_AUTHENTICATION_REQUIRED = 511
var NETWORK_CONNECT_TIMEOUT_ERROR = 599


/**
 * class HttpResponse
 * represents the response to an Http request
 */
class HttpResponse {
  /**
   * HttpResponse(body, status, headers, version, time_taken, redirects, responder)
   * @constructor
   */
  HttpResponse(body, status, headers, version, 
    time_taken, redirects, responder) {
      self.status = status ? status : 200
      self.body = body
      self.headers = headers ? headers : {}
      self.version = version ? version : '1.0'
      self.time_taken = time_taken ? time_taken : 0
      self.redirects = redirects
      self.responder = responder
  }

  @to_string() {
    return to_string({
      status: self.status,
      version: self.version,
      time_taken: self.time_taken,
      redirects: self.redirects,
      responder: self.responder,
      headers: self.headers,
      body: self.body
    })
  }
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
  _do_http(uri, method, data){

    var responder = uri.absolute_url(), headers, body, time_taken = 0, error
    var should_connect = true, redirect_count = 0, http_version = '1.0', status_code = 0

    while should_connect {

      var resolved_host = socket.get_address_info(uri.host)

      if resolved_host {
        var host = resolved_host.ip
        var port = uri.port

        # construct message
        var message = '${method} ${uri.path} HTTP/1.1'
        if !self.headers.contains('Host') {
          message += '\r\nHost: ${uri.host}'
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
        var client = socket.Socket()

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

        time_taken += time() - start

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
          uri = url.parse(headers['Location'])
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
    return HttpResponse(body, status_code, headers, http_version, 
      time_taken, redirect_count, responder)
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
   * send_request(url: string, [method: string = 'GET', data: string])
   *
   * sends an Http request and returns an HttpResponse
   * or throws one of SocketException or Exception if it fails
   */
  send_request(uri, method, data) {

    if !uri or !is_string(uri) 
      die Exception('invalid url')

    # parse the url into component parts
    uri = url.parse(uri)

    if !method {
      # the request method
      # default = GET
      method = 'GET'
    }

    if data != nil and !is_string(data)
      die Exception('string expected, ${typeof(data)} give')

    return self._do_http(uri, method.upper(), data)
  }
}


/**
 * get(url: string)
 *
 * sends an Http GET request and returns an HttpResponse
 * or throws one of SocketException or Exception if it fails
 */
def get(url) {
  return HttpClient().send_request(url, 'GET')
}

/**
 * post(url: string, [data: string])
 *
 * sends an Http POST request and returns an HttpResponse
 * or throws one of SocketException or Exception if it fails
 */
def post(url, data) {
  return HttpClient().send_request(url, 'POST', data)
}

