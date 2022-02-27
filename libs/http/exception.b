#!-- part of the http module

/**
 * HTTP related Exceptions
 */
class HttpException < Exception {

  /**
   * HttpException(message: string)
   * @constructor 
   */
  HttpException(message) {
    self.message = message
  }

  @to_string() {
    return '<HttpException: ${self.message}>'
  }
}
