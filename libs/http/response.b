#!-- part of the http module

import json
import mime
import date { Date }
import .exception { HttpException }

/**
 * Represents the response to an Http request
 * @serializable
 * @printable
 */
class HttpResponse {

  /**
   * The HTTP version of the response
   * @type string
   */
  var version

  /**
   * The HTTP response status code
   * @type number
   */
  var status

  /**
   * The HTTP response headers
   * @type dictionary
   */
  var headers

  /**
   * Total time taken for the HTTP request that generated this HttpResponse to complete
   * @type number
   */
  var time_taken

  /**
   * The number of times the HTTP request that generated this HttpResponse was redirected.
   * @type number
   */
  var redirects

  /**
   * The final URL that provided the HttpResponse
   * @type string
   * @note This might differ from the original request URI.
   */
  var responder

  /**
   * The content of the HTTP response as bytes
   * @type bytes
   */
  var body

  /**
   * The cookies to be sent back to the client
   * @type list
   */
  var cookies

  /**
   * HttpResponse(body: string, status: int, headers: dict, cookies: list, version: string, time_taken: number, redirects: int, responder: string)
   * @constructor 
   */
  HttpResponse(body, status, headers, cookies, version, time_taken, redirects, responder) {
    self.status = status ? status : 200
    self.body = body ? body : bytes(0)
    self.headers = headers ? headers : {
      'Content-Type': 'text/html; charset=utf-8',
      'X-Powered-By': 'Blade',
      'Date': Date().format('r')
    }
    self.cookies = cookies ? cookies : []
    self.version = version ? version : '1.0'
    self.time_taken = time_taken ? time_taken : 0
    self.redirects = redirects
    self.responder = responder
  }

  /**
   * write(data: string | bytes)
   * 
   * Writes data to the response stream. 
   * 
   * > This method should be prefered over writing directly to the body 
   * > property to prevent unexpected behaviors.
   */
  write(data) {
    if !is_string(data) and !is_bytes(data)
      die Exception('data must be bytes or string')
    if is_string(data) self.body += data.to_bytes()
    else self.body += data
  }

  /**
   * json(data: any [, status_code: number])
   * 
   * Writes a json encoded data to the response stream and sets the response 
   * `Content-Type` to `application/json`. If the status code is given, the
   * response will be sent with the given status code.
   */
  json(data, status_code) {
    if status_code != nil {
      if !is_number(status_code)
        die Exception('argument 2 (status_code) expects a number')
      self.status = status_code
    }
    self.content_type('application/json')
    self.write(json.encode(data))
  }

  /**
   * file(path: string [, status_code: number])
   * 
   * Writes a file into the response stream and sets the `Content-Type` to the 
   * correct mimetype for the file. If the status code is given, the
   * response will be sent with the given status code.
   * 
   * @throws {Exception}
   */
  file(path, status_code) {
    if status_code != nil {
      if !is_number(status_code)
        die Exception('argument 2 (status_code) expects a number')
      self.status = status_code
    }

    self.content_type(mime.detect_from_name(path))
    var file_data = file(path, 'rb').read()
    self.write(file_data)
    file_data.dispose()
  }

  /**
   * set_cookie(key: string, value: string [, domain: string [, path: string [, expires: string [, secure: bool [, extras]]]]])
   * 
   * Sets a cookie to be send back to a client with the given _key_ and _value_. 
   * When other parameters are given, they are used to construct a correct Set-Cookie 
   * header based on their named properties.
   * @throw HttpException
   */
  set_cookie(key, value, domain, path, expires, secure, extras) {
    if !is_string(key) or !is_string(value)
      die Exception('argument 1 (key) and argument 2 (value) must be string')
    if (domain != nil and !is_string(domain)) or
        (path != nil and !is_string(path)) or
        (expires != nil and !is_string(expires))
      die Exception(
        'argument 3 (domain), argument 4 (path) and argument 5 (expires) must be string when given'
      )
    if secure != nil and !is_bool(secure)
      die Exception('argument 6 (secure) must be a boolean')
    if extras != nil and !is_string(extras)
      die Exception('argument 7 (extras) must be a string when given')

    # fix common prefix support for clients that implement them
    # NOTE: they have no effect when the client do not.
    if !path and !key.starts_with('__Host-') path = '/'
    if !secure and key.starts_with('__Secure-') secure = true
    
    var cookie = '${key}=${value}'
    if domain cookie += '; Domain=${domain}'
    if path cookie += '; Path=${path}'
    if expires cookie += '; Expires=${expires}'
    if secure cookie += '; Secure'
    if extras cookie += '; ${extra}'

    self.cookies.append(cookie)
  }

  /**
   * redirect(location: string [, status: string])
   * 
   * Redirects the client to a new location. This function simultaneously sets 
   * the `Location` header and returns a 30x status code. If the `status` 
   * parameter is not given, the function defaults to `302`.
   * @throw HttpException
   * 
   * @note when supplying a status, it must be a 30x
   */
  redirect(location, status) {
    if !is_string(location)
      die Exception('location must be a string')
    if status != nil and !is_number(status) and !is_int(status)
      die Exception('status must be an integer if present')

    self.headers.set('Location', location)
    self.status = status ? status : 302

    if self.status < 300 or self.status > 399
      die HttpException('redirect status code must be a 30x')
    self.body = bytes(0)
  }

  /**
   * content_type(mimetype: string)
   * 
   * Sets the content type of the HTTP response.
   */
  content_type(mimetype) {
    if !is_string(mimetype)
      die Exception('argument 1 (mimetype) expects string')

    self.headers.set('Content-Type', mimetype)
  }

  @to_string() {
    return '<HttpResponse status: ${self.status}, version: ${self.version}, time_taken:' +
        ' ${self.time_taken}, redirects: ${self.redirects}, responder: ${self.responder}>'
  }

  @to_json() {
    return {
      status: self.status,
      version: self.version,
      time_taken: self.time_taken,
      redirects: self.redirects,
      responder: self.responder,
      headers: self.headers,
      cookies: self.cookies,
      body: self.body
    }
  }
}
