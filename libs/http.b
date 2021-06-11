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

  # the request method
  # default = GET
  var method = 'GET'

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

  HttpRequest(url) {
    if !url or !is_string(url) 
      die Exception('invalid url')

    # parse the url into component parts
    self.url = Url.parse(url)
  }

  # the main http request method
  __(method, data){

    var responder = self.url.absolute_uri, headers, body, time_taken, error
    var will_connect = true, redirect_count = 0, http_version = '1.0', status_code = 0

    while will_connect {

      # @TODO: in the else clause, get ipv4 address from the hostname
      var resolved_host = Socket.get_address_info(self.url.host)

      if resolved_host {
        var host = resolved_host.ip
        var port = self.url.port

        # construct message
        var message = '${method} ${self.url.path} HTTP/1.1'
        if !self.headers.contains('Host') {
          message += '\r\nHost: ${self.url.host}'
        }
        # add custom headers
        for key, value in self.headers {
          message += '\r\n${key}: ${value}'
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

        # close the client...
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
          self.url = Url.parse(headers['Location'])
          self.referer = headers['Location']
        } else {
          will_connect = false
        }
      } else {
        will_connect = false
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

  send(data) {
    if data != nil and !is_string(data)
      die Exception('string expected, ${typeof(data)} give')
    return self.__(self.method.upper(), data)
  }
}