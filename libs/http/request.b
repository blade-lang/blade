#!-- part of the http module

import ._process

import .exception { HttpException }
import .status { * }
import .response { HttpResponse }

import url
import curl { Option, Info, Curl, CurlList }

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
   * The hostname derived from the `Host` header or the first instance of 
   * `X-Forwarded-Host` if set.
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

    var responder = uri.absolute_url(), error, referer

    var time_taken = 0, 
        redirect_count = 0, 
        http_version = '1.0', 
        status_code = 0

    var curl = Curl()
    curl.set_option(Option.URL, responder)
    if method.upper() != 'GET' {
      curl.set_option(Option.CUSTOMREQUEST, method.upper())
    }
    curl.set_option(Option.FOLLOWLOCATION, options.get('follow_redirect', true))
    curl.set_option(Option.CONNECTTIMEOUT_MS, options.get('connect_timeout', 2000))
    curl.set_option(Option.TIMEOUT_MS, options.get('receive_timeout', 2000))

    # Just trying to get a little performance boost.
    if uri.scheme.lower() == 'https' {
      curl.set_option(Option.DEFAULT_PROTOCOL, 'https')
    }

    # Set request headers
    var headers = []
    for key, value in self.headers {

      # Make sure to always override user set Content-Length header 
      # to avoid unexpected behavior.
      if key.lower() != 'content-length' {
        headers.append('${key}: ${value}')
      }
    }
    curl.set_option(Option.HTTPHEADER, CurlList(headers))

    if data {
      curl.set_option(Option.POSTFIELDS, data)
    }

    var result = curl.send()

    # Convert raw headers into a dictionary.
    var response_headers = result.headers.trim().split('\r\n\r\n')
    if response_headers.length() > 1 {
      response_headers = response_headers[-1]
    } else {
      response_headers = response_headers[0]
    }
    response_headers = _process.process_header(response_headers)

    # return a valid HttpResponse
    var response =  HttpResponse(
      result.body, 
      curl.get_info(Info.RESPONSE_CODE), 
      response_headers,
      curl.get_info(Info.HTTP_VERSION), 
      curl.get_info(Info.TOTAL_TIME), 
      curl.get_info(Info.REDIRECT_COUNT), 
      curl.get_info(Info.EFFECTIVE_URL)
    )

    # Free curl.
    curl.close()

    return response
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
