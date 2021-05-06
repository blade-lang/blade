/**
 * Bird core Url module
 * 
 * provides functionalities for parsing and processing URLs
 * @copyright 2021, Ore Richard Muyiwa
 */
import 'types'

# Excpetion thrown when the url is malformed
class UrlMalformedException < Exception {
  UrlMalformedException(s) {
    self.message = s
  }
}

class Url {

  # the url scheme e.g. http, https, ftp, tcp etc...
  var scheme

  # the host information contained in the url
  var host

  # the port information contained in the url
  # whenever the url doesn't indicate,
  # we try to make a best guess based on the scheme.
  var port

  # the path of the URL. default = /
  var path = '/'

  # hash information contained in the url and it's beginning
  # indicated by and hash (#) sign
  # this value is especially relevant to some http/https urls
  # and are usually references to the content of the document
  # at the given url
  var hash

  # query/search information contained in the url and it's beginning
  # indicated by and question (?) sign
  # this value is especially relevant to some http/https urls
  # and are usually used to convey data to endpoint based on the
  # GET method.
  var query

  # username and password information are sometimes embeded in urls
  # ... add support for parsing them
  var username
  var password

  # a list of schemes that does not conform to the standard ://
  # after the scheme name in their urls
  static var ALTERNATE_SCHEMES = ['mailto', 'tel']

  /**
   * Url.parse(url: string)
   * parses given url string into a Url object
   * @return Url
   */
  static parse(url) {
    if !is_string(url) die Exception('string expected')

    var scheme, host, port, path = '/', query, hash, username, password

    # following that most urls written without indicating the scheme
    # are usually http urls, default url scheme to http if none was given
    # do not do this only when the scheme is mailto:
    if url.index_of('://') < 0 {
      var match_found = false
      for sc in Url.ALTERNATE_SCHEMES {
        if url.starts_with(sc) {
          match_found = true
          break
        }
      }

      if !match_found {
        url = 'http://${url}'
      }
    }

    # query/search and hash entries tracker
    var query_starts = false
    var hash_starts = false

    iter var i = 0; i < url.length(); i++ {

      # simple anonymous function to scan port
      var _scan_port = || {
          var _port = ''
          i++
          while i < url.length() and Type(url[i]).is_digit() { # id_digit
            _port += url[i]
            i++
          }
          port = to_number(_port)
      }

      if url[i] == ':' {
        if !scheme {
          # scan the scheme
          scheme = url[0, i]

          # skipping // if scheme is not mailto as mailto that does not use the //
          if scheme != 'mailto' {
            # if the // is missing, it's a malformed url
            if url[i,i+3] != '://' 
              die UrlMalformedException('expected // at index ${i}')
            i += 2
          }

          # Scheme names consist of a sequence of characters beginning with a
          # letter and followed by any combination of letters, digits, plus
          # ("+"), period ("."), or hyphen ("-").  Although schemes are case-
          # insensitive, the canonical form is lowercase and documents that
          # specify schemes must do so with lowercase letters.  An implementation
          # should accept uppercase letters as equivalent to lowercase in scheme
          # names (e.g., allow "HTTP" as well as "http") for the sake of robustness.
          # https://tools.ietf.org/html/rfc3986#section-3.1
          if !scheme.match('/^[a-z][a-z0-9+.]+$/i')
            die UrlMalformedException('invalid scheme')
        } else if !port {
          # scan the port number
          _scan_port()
        }
      } else if !host and scheme {
        # scan the host
        host = ''
        while i < url.length() and (Type(url[i]).is_digit() or Type(url[i]).is_alpha() or 
            url[i] == '.' or url[i] == '@' or url[i] == '-') {
          host += url[i]
          i++
        }

        if i < url.length() - 1 and url[i] == ':' {
          if url.length() - 1 > i and (Type(url[i + 1]).is_alpha() or url[i + 1] == '@') {
            # username password combo encountered...
            username = host
            host = nil # we'll need to rescan for the host later...

            password = ''

            if url[i + 1] != '@' {
              password = ''
              i++

              while i < url.length() and url[i] != '@' {
                password += url[i]
                i++
              }

              if url.length() == i {
                # we read the entire url without a terminating @ sign...
                # something is wrong with this url and the url is definitely
                # malformed...
                die UrlMalformedException('url not complete')
              }

              # we need to go back to @ to let host be scanned completely
              # at the next iteration because we are now on the first
              # character of the host segement
              i--
            }
          }
          _scan_port()
        }

        # check if the host contains username data
        # e.g. mailto:username@example.com
        if host and host.index_of('@') > -1 {
          var _ = host.split('@')
          username = _[0]
          host = _[1]
        }
      } else if path == '/' and host {
        # scan the address

        # the path is terminated by the first question mark ("?") or 
        # number sign ("#") character, or by the end of the URI
        while i < url.length() and url[i] != '?' and url[i] != '#' {
          path += url[i]
          i++
        }

        # the path cannot begin with two slash characters
        # https://tools.ietf.org/html/rfc3986#section-3.3
        if path.starts_with('//') die UrlMalformedException('invalid path')

        # what should we parse next
        query_starts = i < url.length() and url[i] == '?'
        hash_starts = i < url.length() and url[i] == '#'
      } else if query_starts {

        if hash_starts {
          # this is an edge-case where we have the url hash coming
          # before the url query segement.
          # while this may be allowed by specific implementations,
          # but RFC 3986 doesn't allow this.
          # for this library, we are going strictly RFC 3986
          # https://tools.ietf.org/html/rfc3986#section-3.4
          die UrlMalformedException('query not allowed at index ${i}')
        }

        # scan the query
        # the query is always the last part of a url
        query = ''
        while i < url.length() and url[i] != '#' {
          query += url[i]
          i++
        }

        # reset for some abnormal urls that may be correct in users implementation
        query_starts = false

        hash_starts = i < url.length() and url[i] == '#'
      } else if hash_starts {
        # scan the hash
        hash = ''

        # we are still checking for the ? character
        # this ensures we don't eroneously 
        while i < url.length() {
          hash += url[i]
          i++
        }
      }
    }

    # the default public port scheme if not given
    if !port {
      using scheme {
        when 'http'   port = 80
        when 'https'  port = 443
        default port = 0
      }
    }

    # build a new Url instance and return
    var result = Url()
    result.scheme = scheme
    result.host = host
    result.port = port
    result.path = path
    result.hash = hash
    result.query = query
    result.username = username
    result.password = password

    return result
  }

  /**
   * authority()
   * returns the url authority
   * 
   * The authority component is preceded by a double slash ("//") and is
   * terminated by the next slash ("/"), question mark ("?"), or number
   * sign ("#") character, or by the end of the URI.
   *
   * @note: mailto scheme does not have an authority. For this reason,
   * mailto schemes return an empty string as authority.
   * @return string
   */
  authority() {
    if self.scheme != 'mailto' {
      var authority = ''

      # some schemes do not allow the userinfo and/or port subcomponents
      # RFC 3986 [Page 17]
      if self.username {
        authority += '${self.username}:'

        if self.password authority += self.password
        authority += '@'
      }
      authority += self.host

      # URI producers and normalizers should omit the ":" delimiter that
      # separates host from port if the port component is empty
      if self.port > 0 authority += ':${self.port}'

      return authority
    }
    return ''
  }

  /**
   * Url.encode(url: string, strict: boolean [default = false])
   * URL-encodes string
   * 
   * this function is convenient when encoding a string to be used in 
   * a query part of a URL, as a convenient way to pass variables to 
   * the next page.
   *
   * if strict mode is enabled, space character is encoded with the 
   * percent (%) sign in order to conform with RFC 3986. Otherwise,
   * is is encoded with the plus (+) sign in order to align with
   * the default encoding used by modern browsers.
   * @note strict mode is disabled by default
   * @return string
   */
  static encode(url, strict) {
    if !is_string(url)
      die Exception('string expected at parameter 1')

    if strict != nil and !is_bool(strict) 
      die Exception('boolean expected at parameter 2')

    var result = ''

    for c in url {
      # keep alphanumeric and other accepted characters intact
      if c.is_alnum() or c == '-' or c == '_' or c == '.' or c == '~'
        result += c
      # when not in strict mode
      else if !strict and c == ' ' result += '+'
      # encode all other characters
      else result += '%${hex(ord(c))}'.upper()
    }

    return result
  }

  /**
   * Url.decode(url: string)
   * Decodes URL-encoded string
   * 
   * decodes any %## encoding in the given string. 
   * plus symbols ('+') are decoded to a space character.
   * @return string
   */
  static decode(url) {
    if !is_string(url)
      die Exception('string expected')

    # quick exit strategy
    if url.index_of('%') > -1 return url

    var lookup_table = '0123456789abcdef'

    var result = ''

    iter var i = 0; i < url.length(); i++ {
      if url[i] == '%' {
        # decode percent-encoded data here
        var hexdata = url[i+1, i+3].lower()

        if hexdata.length() != 2 die UrlMalformedException('bad encoding')

        result += chr((lookup_table.index_of(hexdata[0]) * 16) + lookup_table.index_of(hexdata[1]))
        i += 2
      } 
      # + should be converted to space as most browsers
      # will encode space to + (non-strict Url.decode mode)
      else if url[i] == '+' result += ' '
      else result += url[i]
    }

    return result
  }
}