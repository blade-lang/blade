import .smtp
import .message
import .imap
import .constants

var _header_line_rgx = '/^([^ ]+): (([^\\n]|(\\n(?= )))+)\\n/m'
var _type_rgx = '/^([^;]+)/'
var _boundary_rgx = '/boundary="?([^\s"]+)"?/'

def parse_headers(header) {
  if !header.ends_with('\n') header += '\n'
  var headers = {}

  var matches = header.matches(_header_line_rgx)
  if matches and matches[0] {
    iter var i = 0; i < matches[0].length(); i++ {
      headers.set(matches[1][i], matches[2][i])
    }
  }

  return headers
}

def _strip_bounday(str, boundary) {
  str = str.trim()
  # remove the trailing `--` and trim the first --boundary out
  var bound_len = '--${boundary}'.length()
  return str[
    # +1 for newline after first boundary occurrence
    bound_len + 1,
    -(bound_len + 2)] # +2 for the ending --
}

def _split_parts(message) {
  var headers_end = message.matches(_header_line_rgx) ? 
      message.index_of('\n\n') : -1

  return {
    headers: parse_headers(message.ascii()[,headers_end]), 
    body: message[headers_end+1,].trim(),
  }
}

/**
 * Attachment class is used to hold the information of attachments in the 
 * message.
 */
class Attachment {
  var headers = []
  var content

  /**
   * @constructor
   */
  Attachment(headers, content) {
    self.headers = headers
    self.content = content
  }

  @to_json() {
    return {
      headers: self.headers,
      content: self.content,
    }
  }

  @to_string() {
    return '<Attachment>' +
    '  <headers>${self.headers}</headers>' +
    '  <content>${self.content}</content>' +
    '</Attachment>'
  }
}

class Mail {

  Mail(headers, body, attachements) {
    self.headers = headers ? headers : {}
    self.body = body ? body : {}
    self.attachements = attachements ? attachements : []
  }

  @to_json() {
    return {
      headers: self.headers,
      body: self.body,
      attachements: self.attachements,
    }
  }

  @to_string() {
    return '<Mail>'+
    '  <headers>${self.headers}</headers>' +
    '  <body>${self.body}</body>' +
    '  <attachements>${self.attachements}</attachements>' +
    '</Mail>'
  }
}

/**
 * Parses email messages and return an instance of Mail representing it.
 * 
 * @param string message
 * @return Mail
 */
def parse(message) {
  message = _split_parts(message.trim() + '\n\n')
  var text = message.body,
      headers = message.headers,
      body = {}, 
      attachements = [],
      content_type

  if content_type = message.headers.get('Content-Type') {
    var type = content_type.match(_type_rgx)
    var boundary = content_type.match(_boundary_rgx)

    if type and boundary and (boundary = boundary[1]) {
      message.body = _strip_bounday(message.body, boundary)
      text = message.body

      # split by boundary
      var body_parts = message.body.split('/\-\-${boundary}/')
      if body_parts {
        iter var i = 0; i < body_parts.length(); i++ {
          message = _split_parts(body_parts[i].trim() + '\n\n')
          content_type = message.headers.get('Content-Type')
          var is_body = content_type ? 
                  content_type.trim().match('/^(text|multipart)\//') :
                  false

          var is_disposed_attachment = false
          var disposition = message.headers.get('Content-Disposition')
          if disposition {
            is_disposed_attachment = disposition.split(';')[0].lower() == 'attachment'
          }

          if !is_disposed_attachment {
            if !content_type {
              text = message.body
            } else if content_type and is_body {
              text = message.body
              type = content_type.match(_type_rgx)
              boundary = content_type.match(_boundary_rgx)
  
              if boundary {
                boundary = boundary[1]
                message.body = _strip_bounday(message.body, boundary)
                var body_parts = message.body.split('/\-\-${boundary}/')
  
                for part in body_parts {
                  message = _split_parts(part.trim() + '\n\n')
                  if !message.headers {
                    text = message.body
                  } else {
                    content_type = message.headers.get('Content-Type')
                    if content_type {
                      body[content_type.split(';')[0]] = message.body
                    }
                  }
                }
              } else {
                body[type[1].split(';')[0]] = message.body
              }
            } else {
              # parse the attachments
              attachements.append(Attachment(message.headers, message.body))
            }
          } else {
            attachements.append(Attachment(message.headers, message.body))
          }
        }
      }
    } else if type {
      body[type[1].split(';')[0]] = message.body
    }
  }

  if text and !body.contains('text/plain') {
    body['text/plain'] = text
  }

  return Mail(headers, body, attachements)
}

