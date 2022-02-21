#!-- part of the http module

import url
import socket
import .response { HttpResponse }
import .util
import ._process

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
   * The send timeout duration in milliseconds
   * @default 300s
   */
  var send_timeout = 300000

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
   * the main http request method
   */
  _do_http(uri, method, data){

    var responder = uri.absolute_url(), headers, body, time_taken = 0, error
    var should_connect = true, redirect_count = 0, http_version = '1.0', status_code = 0

    while should_connect {

      var resolved_host = socket.get_address_info(uri.host)

      if resolved_host {
        var host = resolved_host.ip
        var port = uri.port

        # construct message
        var message = '${method} ${uri.path}'
        if uri.query message += '?${uri.query}'
        if uri.hash message += '#${uri.hash}'
        message += ' HTTP/1.1'

        if self.user_agent {
          message += '\r\nUser-Agent: ${self.user_agent}'
        }

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

        if self.referer and !self.headers.contains('Referer') {
          message += '\r\nReferer: ${self.referer}'
        }

        if data {
          # append the correct content length to the message
          message += '\r\nContent-Length: ${data.length()}'
        }

        # append the body
        message += '\r\n\r\n${data}'

        # do real request here...
        var client = socket.Socket()
        client.set_option(socket.SO_SNDTIMEO, self.send_timeout)
        client.set_option(socket.SO_RCVTIMEO, self.receive_timeout)

        var start = time()

        # connect to the url host on the specified port and send the request message
        client.connect(host, port ? port : (uri.scheme == 'https' ? 443 : 80), self.connect_timeout)
        client.send(message)

        # receive the response...
        var response_data = client.receive() or ''

        # separate the headers and the body
        var body_starts = response_data.index_of('\r\n\r\n')

        if body_starts {
          headers = response_data[0,body_starts].trim()
          body = response_data[body_starts + 2, response_data.length()].trim()
        }

        headers = _process.process_header(headers, |version, status|{
          http_version = version
          status_code  = status
        })

        # According to https://datatracker.ietf.org/doc/html/rfc7230#section-3.3
        # 
        # Responses to the HEAD request method (Section 4.3.2
        # of [RFC7231]) never include a message body because the associated
        # response header fields (e.g., Transfer-Encoding, Content-Length,
        # etc.), if present, indicate only what their values would have been if
        # the request method had been GET
        if method.upper() != 'HEAD' {

          # gracefully handle responses being sent in multiple packets
          # if the request header contains the Content-Length,
          # get that length and keep reading until we have read the total
          # length of the response.
          if headers.contains('Content-Length') {
            var length = to_number(headers['Content-Length']) - 2

            # According to: https://datatracker.ietf.org/doc/html/rfc7230#section-3.4
            # A client that receives an incomplete response message, which can
            # occur when a connection is closed prematurely or when decoding a
            # supposedly chunked transfer coding fails, MUST record the message as
            # incomplete.
            var data = body
            while body.length() < length and data {
              data = client.receive()
              # append the new data in the stream
              body += data
            }
          } else if headers.contains('Transfer-Encoding') and headers['Transfer-Encoding'].trim() == 'chunked'  {
            # gracefully handle chuncked data transfer
            # 
            # According to: https://datatracker.ietf.org/doc/html/rfc7230#section-4.1
            # 
            # chunked-body   = *chunk
            #           last-chunk
            #           trailer-part
            #           CRLF
            # 
            # chunk          = chunk-size [ chunk-ext ] CRLF
            #                   chunk-data CRLF
            # chunk-size     = 1*HEXDIG
            # last-chunk     = 1*("0") [ chunk-ext ] CRLF

            var tmp_body = body.split('\n'), do_read = true
            var chunk_size = to_number('0x'+tmp_body[0].trim())
            body = '\n'.join(tmp_body[1,])
            
            var do_fetch = true
            while do_fetch {
              var response = client.receive()
              body += response
              if response.ends_with('\r\n\r\n') do_fetch = false
            }

            # remove the last chunck-size marking.
            body = body.replace('/0\\s+$/', '')
          }
        }

        time_taken += time() - start

        # close client
        client.close()

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

  /**
   * send_request(url: string, [method: string = 'GET', data: string])
   *
   * Sends an Http request and returns a HttpResponse.
   * 
   * @return HttpResponse
   * @throws SocketException, Exception
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
