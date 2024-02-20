#!-- part of the http module

/**
 * HTTP related Exceptions.
 * 
 * @printable
 */
class HttpException < Exception {
  @string() {
    return '<HttpException: ${self.message}>'
  }
}
