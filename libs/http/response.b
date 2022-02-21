#!-- part of the http module

/**
 * Represents the response to an Http request
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
