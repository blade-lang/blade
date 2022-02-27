#!-- part of the http module

import ._process
import .exception { HttpException }
import url
import socket { Socket }

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

    if parts.length() != 3
      die HttpException('invalid request')

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
  }

  _decode_body(body) {

    # We need to capture multipart/form-data here since it allows 
    # specifying boundaries right in the content-type header.
    var content_type = self.headers.get('Content-Type', nil)
    if content_type {
      content_type = content_type.split(';')[0].trim().lower()
    }

    using content_type {
      when 'application/x-www-form-urlencoded' {
        self.body = self._get_url_encoded_parts(body)
      }
      when 'multipart/form-data' {
        # @TODO: Parse multipart/form-data body.
      }
      default {
        self.body = body
      }
    }
  }

  /**
   * parse(raw_data: string [, client: Socket])
   * 
   * Parses a raw HTTP request string into a correct HttpRequest
   */
  parse(raw_data, client) {

    if !is_string(raw_data)
      die Exception('raw_data must be string')
    if !instance_of(client, Socket) 
      die Exception('invalid Socket')

    self.ip = client.info().address

    # separate the headers and the body
    var body_starts = raw_data.index_of('\r\n\r\n'), headers, body

    if body_starts {
      headers = raw_data[,body_starts].trim().split('\r\n')

      # Remember that body_start returns the position of the first \r in the '\r\n\r\n '
      # sequence.
      # +3 to remove the '\r\n\r\n' which is basically 4 characters.
      body = raw_data.ascii()[body_starts + 4, raw_data.length()]
    }

    if headers {
      var line1 = headers[0]
      self._decode_line1(line1)

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

      # Remove the last '\r\n\r\n' sequence from the body.
      # body = body.to_bytes()[,-4].to_string()

      self._decode_body(body)
    } else {
      die HttpException('Invalid request')
    }
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

