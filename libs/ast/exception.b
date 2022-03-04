#!-- part of the ast module

/**
 * Exception raised for errors during parsing
 */
class ParseException < Exception {
  /**
   * ParseException(token: Token, message: string)
   * @constructor 
   */
  ParseException(token, message) {
    parent('Error at ${token.literal} on line ${token.line}: ${message}')
  }
}
