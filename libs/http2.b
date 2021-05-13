/**
 * HTTP
 *
 * Provides interface for working with Http client requests
 * and servers.
 * @copyright 2021, Ore Richard Muyiwa
 */
import 'url'
import 'socket'


/**
 * HttpResponse
 * represents the response to an HttpRequest
 */
class HttpResponse {
  var status_code = 0
  var http_version = '1.0'
  var time_taken = 0
  var redirects = 0
  var responder
  var headers = {}
  var error
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

class HttpRequest {
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

  # custom request headers
  var headers = {}

  # whether to remove the expect header or not
  # only applies to requests with files in the body
  var no_expect = false

  HttpRequest(url) {
    if !url or !is_string(url) 
      die Exception('invalid url')

    # parse the url into component parts
    self.url = Url.parse(url)
  }

  # the main http request method
  __(method, data, has_file){

    var responder = self.url.absolute_uri, header, body, time_taken, error
    var will_connect = true, redirect_count = 0

    # construct message
    var message = '${method} ${self.url.path} HTTP/1.1\r\n\r\n'
    
    # do real request here...
    var client = Socket()

    # @TODO: in the else clause, get ipv4 address from the hostname
    var host = self.url.host == 'localhost' ? nil : (
      self.url.host_is_ipv4() ? self.url.host : ''
    )
    var port = self.url.port

    try {
      while will_connect {
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

        # close the client...
        client.close()

        # separate the headers and the body
        var body_starts = response_data.index_of('\r\n\r\n')

        if body_starts {
          header = response_data[0,body_starts].trim()
          body = response_data[body_starts + 2, response_data.length()].trim()
        }

        # @TODO: if there was a redirect, update the host and port
        # and change will connect to true
        will_connect = false
      }
    } catch e {
      error = e.message
    }

    # return a valid HttpResponse
    var result = HttpResponse()
    
    result.error = error
    result.headers = self._process_header(header, |version, status|{
      result.http_version = version
      result.status_code  = status
    })
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
          result.add(data[i][0,d], data[i][d + 1,data[i].length()])
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

  _make_request(method, data) {
    var has_file = false

    if is_dict(data) {
      for value in data {
        if is_file(value) has_file = true
      }
    }

    return self.__(method.upper(), data, has_file)
  }

  # Makes Http GET request to the given URL
  # @return dictionary
  get() {
    return self._make_request('GET')
  }

  # Makes Http POST request to the given URL with the given data
  # @return dictionary
  post(data) {
    if !is_dict(data) and !is_string(data) 
      die Exception('post body must be a dictionary or string')

    return self._make_request('POST', data)
  }
}