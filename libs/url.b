/**
 * Bird core Url module
 * 
 * provides functionalities for parsing and processing URLs
 * @copyright 2021, Ore Richard Muyiwa
 */
import 'types'

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

  static parse(url) {
    var scheme, host, port, path = '/', query, hash, username, password

    # following that most urls written without indicating the scheme
    # are usually http urls, default url scheme to http if none was given
    if !url.match('://') url = 'http://${url}'

    # query/search and hash entries tracker
    var query_starts = false
    var hash_starts = false

    iter var i = 0; i < url.length(); i++ {
      var is_at_end = i == url.length() - 1

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
          i += 2 # skipping //
        } else if !port {
          # scan the port number
          _scan_port()
        }
      } else if !host and scheme {
        # scan the host
        host = ''
        while i < url.length() and (Type(url[i]).is_digit() or Type(url[i]).is_alpha() or url[i] == '.') {
          host += url[i]
          i++
        }

        if url[i] == ':' {
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

              if url.length() - 1 == i {
                # we read the entire url without a terminating @ sign...
                # something is wrong with this url and the url is definitely
                # malformed...
                die Exception('url malformed')
              }

              # we need to go back to @ to let host be scanned completely
              # at the next iteration because we are now on the first
              # character of the host segement
              i--
            }
          }
          _scan_port()
        }
      } else if path == '/' and host {
        # scan the address
        while i < url.length() and url[i] != '?' and url[i] != '#' {
          path += url[i]
          i++
        }

        # what should we parse next
        if i < url.length() and url[i] == '?' query_starts = true
        if i < url.length() and url[i] == '#' hash_starts = true
      } else if query_starts {
        # scan the query
        # the query is always the last part of a url
        query = ''
        while i < url.length() {
          query += url[i]
          i++
        }

        if i < url.length() and url[i] == '#' hash_starts = true
        # reset for some abnormal urls that may be correct in users implementation
        query_starts = false
      } else if hash_starts {
        # scan the hash
        hash = ''
        while i < url.length() and url[i] != '?' {
          hash += url[i]
          i++
        }
        if i < url.length() and url[i] == '?' query_starts = true
        # reset for some abnormal urls that may be correct in users implementation
        hash_starts = false
      }
    }

    # the default public port scheme if not given
    if !port {
      using scheme {
        when 'http'   port = 80
        when 'https'  port = 443
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

  authority() {
    if self.port > 0
      return '${self.host}:${self.port}'
    return self.host
  }
}