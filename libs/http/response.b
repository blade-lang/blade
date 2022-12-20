#!-- part of the http module

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
   */
  var version

  /**
   * The HTTP response status code
   */
  var status

  /**
   * The HTTP response headers
   */
  var headers

  /**
   * Total time taken for the HTTP request that generated this HttpResponse to complete
   */
  var time_taken

  /**
   * The number of times the HTTP request that generated this HttpResponse was redirected.
   */
  var redirects

  /**
   * The final URL that provided the HttpResponse
   * @note This might differ from the original request URI.
   */
  var responder

  /**
   * The content of the HTTP response as bytes
   */
  var body

  /**
   * HttpResponse(body: string, status: int, headers: dict, version: string, time_taken: number, redirects: int, responder: string)
   * @constructor 
   */
  HttpResponse(body, status, headers, version, time_taken, redirects, responder) {
    self.status = status ? status : 200
    self.body = body ? body : bytes(0)
    self.headers = headers ? headers : {
      'Content-Type': 'text/html; charset=utf-8',
      'X-Powered-By': 'Blade',
      'Date': Date().format('r')
    }
    self.version = version ? version : '1.0'
    self.time_taken = time_taken ? time_taken : 0
    self.redirects = redirects
    self.responder = responder
  }

  /**
   * write(data: string | bytes)
   * 
   * Writes data to the response response. 
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
   * redirect(location: string [, status: string])
   * 
   * Redirects the client to a new location. This function simultaneously sets 
   * the `Location` header and returns a 30x status code. If the `status` 
   * parameter is not given, the function defaults to `302`.
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
      body: self.body
    }
  }
}
