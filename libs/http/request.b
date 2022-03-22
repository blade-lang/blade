#!-- part of the http module

import ._process

import .exception { HttpException }
import .status { * }
import .response { HttpResponse }

import url
import socket
import url
import ssl

var _chunk_terminator = '0\r\n\r\n'

/**
 * Http request handler and object.
 * @serializable
 * @printable
 */
class HttpRequest {

  /**
   * The original request URL as sent in the raw request.
   */
  var request_uri

  /**
   * The requested path or file. E.g. if the Request URI is `/users?sort=desc`, 
   * then the path is `/users`.
   */
  var path

  /**
   * A string corresponding to the HTTP method of the request: GET, POST, PUT, etc.
   */
  var method

  /**
   * The hostname derived from the `Host` header or the first instance of `X-Forwarded-Host` if set.
   */
  var host

  /**
   * The IP address of the remote client that initiated the request.
   */
  var ip

  /**
   * A dictionary containing the headers sent with the request.
   */
  var headers

  /**
   * A dictionary containing the entries of the URI query string.
   */
  var queries = {}

  /**
   * A dictionary containing the cookies sent with the request.
   */
  var cookies = {}

  /**
   * A dictionary containing all data submitted in the request body.
   */
  var body = {}

  /**
   * A dictionary containing the data of all files uploaded in the request.
   */
  var files = {}

  # Private fields.
  var _body_type = 'application/x-www-form-urlencoded'

  /**
   * The HTTP version used for the request.
   */
  var http_version = '1.0'

  _read_cookies() {
    var cookies = self.headers.get('Cookie', nil)
    if cookies {
      cookies = cookies.split(';')
      for cookie in cookies {
        cookie = cookie.trim().split('=')
        if cookie {
          if cookie.length() == 2 {
            self.cookies.set(cookie[0], cookie[1])
          } else {
            self.cookies.set(cookie[0], nil)
          }
        }
      }

      # We want to contain all cookie information in the cookies property 
      # and not have them sprinkled everywhere. To do this, we need to 
      # consider cookies as not part of the headers.
      self.headers.remove('Cookie')
    }
  }

  _get_url_encoded_parts(data) {
    var result = {}

    var parts = data.split('&')
    for p in parts {
      p = p.split('=')
      if p {
        var name = url.decode(p[0])
        if p.length() == 2 {
          result.set(name, url.decode(p[1]))
        } else {
          result.set(name, nil)
        }
      }
    }

    return result
  }

  _decode_line1(line1) {
    var parts = line1.split(' ')

    if parts.length() == 3 {
      self.method = parts[0]
      self.request_uri = parts[1]
      self.http_version = parts[2].replace('~http\\/~', '')
  
      var uri_parts = parts[1].split('?')
  
      # The request path must exist before both a query and an hash.
      self.path = uri_parts[0].split('#')[0]
  
      if uri_parts.length() > 1 {
        # A query exists and it must do so before any hash.
        var query = uri_parts[1].split('#')[0]
        self.queries = self._get_url_encoded_parts(query)
      }

      return true
    }

    return false
  }

  _decode_multipart(body, boundary) {

    var boundaries = body.split('--${boundary}')
    var contents = []

    for bound in boundaries {
      bound = bound.ltrim('\r').ltrim('\n')

      # We don't want to treat empty bounds.
      var content_start = bound.index_of('\r\n\r\n')
      if content_start != -1 {
        var content_header = bound[,content_start],
            content_body = bound.ascii()[content_start + 4,]
            
        var content_headers = _process.process_header(content_header),
            dispositions = content_headers.get('Content-Disposition', nil)

        if dispositions {

          # Expand the content-disposition header content to get the correct mapping.
          # disposition := "Content-Disposition" ":"
          #              disposition-type
          #              *(";" disposition-parm)

          # disposition-type := "inline"
          #                   / "attachment"
          #                   / extension-token
          #                   ; values are not case-sensitive

          # disposition-parm := filename-parm / parameter

          # filename-parm := "filename" "=" value;

          dispositions = dispositions.split(';')

          # The first directive is always form-data.
          if dispositions.length() < 2 or dispositions[0] != 'form-data' 
            return false

          var disposition = {}

          iter var i = 1; i < dispositions.length(); i++ {

            # Directives are case-insensitive and have arguments that use 
            # quoted-string syntax after the '=' sign
            var d = dispositions[i].split('=')
            var dname = d[0].trim().lower()

            if d.length() == 2 {

              var dvalue = d[1].trim()
              if dvalue.starts_with('"') and dvalue.ends_with('"')
                dvalue = dvalue[1,-1]

              disposition[dname] = dvalue
            } else {
              disposition.set(dname, nil)
            }
          }

          # and the header must also include a name parameter to identify the 
          # relevant field. 
          if !disposition.contains('name') 
            return false

          var content_type = content_headers.get('Content-Type', nil),
              name = disposition.name,
              value = content_body.rtrim('\n').rtrim('\r')
            
          if content_type {

            # We are dealing with an uploaded file.
            self.files[name] = {
              mime: content_type.trim(),
              content: value.ascii(),
            }

            self.files[name].extend(disposition)
          } else {
            self.body[name] = value
          }
          contents.append(disposition)
        }
      }
    }

    return true
  }

  _decode_body(body) {

    # We need to capture multipart/form-data here since it allows 
    # specifying boundaries right in the content-type header.
    var content_type = self.headers.get('Content-Type', ' ')
    if content_type {
      content_type = content_type.split(';')
    }

    var type = content_type[0].trim().lower()
    self._body_type = type

    using type {
      when 'application/x-www-form-urlencoded' {
        self.body = self._get_url_encoded_parts(body)
      }
      when 'multipart/form-data' {
        # Content type should declare a boundary but nothing else.
        if content_type.length() != 2 return false

        var bound_spec = content_type[1].trim().split('=')

        # Make sure we have a valid boundary=xyz label.
        if bound_spec.length() != 2 or bound_spec[0].lower() != 'boundary' 
          return false

        if !self._decode_multipart(body.trim(), bound_spec[1])
          return false
      }
      default {
        self.body = body
      }
    }

    return true
  }

  /**
   * parse(raw_data: string [, client: Socket])
   * 
   * Parses a raw HTTP request string into a correct HttpRequest
   * @return boolean
   */
  parse(raw_data, client) {

    if !is_string(raw_data)
      die Exception('raw_data must be string')
    if !instance_of(client, socket.Socket) 
      die Exception('invalid Socket')

    self.ip = client.info().address

    # separate the headers and the body
    var body_starts = raw_data.index_of('\r\n\r\n'), headers, body

    if body_starts {
      headers = raw_data[,body_starts].trim().split('\r\n')

      # Remember that body_start returns the position of the first \r in the '\r\n\r\n '
      # sequence.
      # +3 to remove the '\r\n\r\n' which is basically 4 characters.
      body = raw_data.ascii()[body_starts + 4,]
    }

    if headers {
      var line1 = headers[0]
      if !self._decode_line1(line1) return false

      self.headers = _process.process_header('\r\n'.join(headers[1,]))

      # To parse the host, first we try to retrieve the `X-Forwarded-Host` header 
      # is it exists. If it does, we simply set our host to whatever value it has. 
      # Otherwise, We check the `Host` header. If it was set, our host will be set to that. 
      # Otherwise, our host will be an empty string.
      self.host = self.headers.get('X-Forwarded-Host', self.headers.get('Host', '').split(':')[0])

      self._read_cookies()

      # Make sure we have all body contents.
      if self.headers.contains('Content-Length') {
        var content_length = to_number(self.headers['Content-Length'])
        var byte_length = body.to_bytes().length()
        if byte_length < content_length {
          body += client.read(content_length - byte_length)
          body.ascii()
        }
      }

      return self._decode_body(body)
    }

    return false
  }

  _strip_chunk_size(body, chunk_size) {
    var length = body.length()
    # chunk_size += 2 # allocating for the \r\n that lead to it.

    if length > chunk_size {
      # inital processed body is up to the chunk size
      var processed = body[,chunk_size],
          # We start striping from the end of the first chunk size
          start = chunk_size

      while chunk_size > 0 {
        var tmp_body = body[start,].split('\n')
        chunk_size = to_number('0x'+tmp_body[1].trim())
        processed += '\n'.join(tmp_body[2,]).ascii()[,chunk_size]
        start += chunk_size
      }

      return processed
    } else {

      # remove the last chunk-size marking.
      body = body.replace('/0\\r\\n\\r\\n$/', '')
      return body
    }
  }

  /**
   * send(uri: Url, method: string [, data: string | bytes [, options: dict]])
   * @default follow_redirect: true
   */
  send(uri, method, data, options) {

    # arguments validation.
    if !instance_of(uri, url.Url)
      die Exception('uri must be an instance of Url')
    if !is_string(method)
      die Exception('method must be string')
    if data != nil and !is_string(data) and !is_byte(data)
      die Exception('data must be string or bytes')
    if options != nil and !is_dict(options)
      die Exception('options must be a dictionary')

    if options == nil options = {}

    var responder = uri.absolute_url(), 
        headers, body, error, referer

    var should_connect = true, 
        time_taken = 0, 
        redirect_count = 0, 
        http_version = '1.0', 
        status_code = 0

    var follow_redirect = options.get('follow_redirect', true),
        connect_timeout = options.get('connect_timeout', 2000),
        send_timeout = options.get('send_timeout', 2000),
        receive_timeout = options.get('receive_timeout', 2000)

    while should_connect {

      var resolved_host = socket.get_address_info(uri.host)

      if resolved_host {
        var host = resolved_host.ip,
            port = uri.port,
            is_secure = uri.scheme == 'https'

        # construct message
        var message = '${method} ${uri.path}'
        if uri.query message += '?${uri.query}'
        if uri.hash message += '#${uri.hash}'
        message += ' HTTP/1.1\r\n'

        if !self.headers.contains('Host') {
          message += 'Host: ${uri.host}\r\n'
        }

        # add custom headers
        for key, value in self.headers {

          # Make sure to always override user set Content-Length header 
          # to avoid unexpected behavior.
          if key.lower() != 'content-length' {
            message += '${key}: ${value}\r\n'
          }
        }

        if referer and !self.headers.contains('Referer') {
          message += 'Referer: ${referer}\r\n'
        }

        if data {
          # append the correct content length to the message
          message += 'Content-Length: ${data.length()}\r\n'
        }

        # append the body
        message += '\r\n${data}'

        # do real request here...
        var client = !is_secure ? socket.Socket() : ssl.SSLSocket(ssl.TLS_client_method)
        client.set_option(socket.SO_SNDTIMEO, send_timeout)
        client.set_option(socket.SO_RCVTIMEO, receive_timeout)

        var start = time()

        # connect to the url host on the specified port and send the request message
        if client.connect(host, port ? port : (is_secure ? 443 : 80), connect_timeout) {
          client.send(message)

          # receive the response...
          var response_data = client.receive() or ''

          # separate the headers and the body
          var body_starts = response_data.index_of('\r\n\r\n')

          if body_starts {
            headers = response_data[0,body_starts].trim()
            body = response_data[body_starts + 2, response_data.length()].trim()
          } else {
            # Clear the headers here. It may currently be a dictionary.
            headers = ''
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
              while body.ascii().length() < length and data {
                data = client.receive()
                # append the new data in the stream
                body += data
              }
              # body += client.read(length - body.ascii().length())
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

              var tmp_body = body.split('\n')
              var chunk_size = to_number('0x'+tmp_body[0].trim())
              var has_chunks = chunk_size != 0 and 
                !body.ascii().ends_with(_chunk_terminator) and
                !body.ascii().ends_with('\r\n0')
              body = '\n'.join(tmp_body[1,]).ascii()

              # Keeping the original chunk size so that we can use it to process requests
              # that are fully read with the chunk size marks in-between.
              var original_chunk_size = chunk_size

              while true and has_chunks {
                var response = client.receive()
                body += response
                if response.ends_with(_chunk_terminator) or response.ends_with('\r\n0')
                  break
              }

              body = self._strip_chunk_size(body, original_chunk_size)
            }
          }

          time_taken += time() - start

          # close client
          client.close()

          if follow_redirect and headers.contains('Location') {
            referer = uri.absolute_url()
            uri = url.parse(headers['Location'])
            responder = referer
          } else {
            should_connect = false
          }
        } else {
          should_connect = false
          die HttpException('could not connect')
        }
      } else {
        should_connect = false
        die HttpException('could not resolve ip address')
      }
    }

    # return a valid HttpResponse
    return HttpResponse(body, status_code, headers, http_version, 
      time_taken, redirect_count, responder)
  }

  @to_string() {
    return '<HttpRequest method=${self.method}, path=${self.path}>'
  }

  @to_json() {
    return {
      request_uri: self.request_uri,
      path: self.path,
      method: self.method,
      host: self.host,
      ip: self.ip,
      headers: self.headers,
      queries: self.queries,
      cookies: self.cookies,
      body: self.body,
    }
  }
}

