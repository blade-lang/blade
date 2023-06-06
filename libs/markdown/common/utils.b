# Merge objects
def assign(obj, ...) {
  if !is_dict(obj) 
    die Exception('dictionary expected in argument 1')
  for source in __args__ {
    if !source continue
    if !is_dict(source)
      die Exception('invalid dictionary in parameter list')

    obj.extend(source)
  }

  return obj
}

# Remove element from array and put another array at those position.
# Useful for some operations with tokens
def array_replace_at(src, pos, new_elements) {
  return src[,pos] + new_elements + src[pos + 1,]
}

def is_valid_entity_code(c) {
  if is_string(c) c = to_number(c)
  # broken sequence
  if c >= 0xD800 and c <= 0xDFFF return false
  # never used
  if c >= 0xFDD0 and c <= 0xFDEF return false
  if (c & 0xFFFF) == 0xFFFF or (c & 0xFFFF) == 0xFFFE return false
  # control codes
  if c >= 0x00 and c <= 0x08 return false
  if c == 0x0B return false
  if c >= 0x0E and c <= 0x1F return false
  if c >= 0x7F and c <= 0x9F return false
  # out of range
  if c > 0x10FFFF return false
  return true
}

var UNESCAPE_MD_RE  = '\\\\([!"#$%&\'()*+,\-.\/:;<=>?@[\\\\\]^_`{|}~])'
var ENTITY_RE       = '&([a-z#][a-z0-9]{1,31});'
var UNESCAPE_ALL_RE = '/' + UNESCAPE_MD_RE + '|' + ENTITY_RE + '/si'

var DIGITAL_ENTITY_TEST_RE = '/^#((?:x[a-f0-9]{1,8}|[0-9]{1,8}))$/i'

import .entities { entities }

def replace_entity_pattern(match, name) {
  var code

  if entities.contains(name) {
    return entities[name]
  }

  if name[0] == '#' and name.match(DIGITAL_ENTITY_TEST_RE) {
    code = name[1].lower() == 'x' ? name[2,] : name[1,]

    if is_valid_entity_code(code) {
      return chr(code)
    }
  }

  return match
}

def unescape_md(str) {
  if str.index_of('\\') < 0 return str
  return str.replace('/' + UNESCAPE_MD_RE + '/s', '$1')
}

def unescape_all(str) {
  if str.index_of('\\') < 0 and str.index_of('&') < 0 return str

  var matches = str.matches(UNESCAPE_ALL_RE)
  if matches {
    iter var i = 0; i < matches[0].length(); i++ {
      if matches[1].length() > i and matches[1][i] {
        str = str.replace(matches[0][i], matches[1][i], false)
      } else if matches[1].length() > i and matches[2].length() > i {
        str = str.replace(matches[0][i], replace_entity_pattern(matches[0][i], matches[2][i]))
      }
    }
  }
  
  return str
}

var HTML_ESCAPE_TEST_RE = '/[&<>"]/'
var HTML_ESCAPE_REPLACE_RE = '/[&<>"]/'
var HTML_REPLACEMENTS = {
  '&': '&amp;',
  '<': '&lt;',
  '>': '&gt;',
  '"': '&quot;',
}

def replace_unsafe_char(ch) {
  return HTML_REPLACEMENTS[ch]
}

def escape_html(str) {
  if str.match(HTML_ESCAPE_TEST_RE) {
    var matches = str.matches(HTML_ESCAPE_REPLACE_RE)
    var match_processed = []
    if matches {
      for match in matches[0] {
        if !match_processed.contains(match) {
          str = str.replace(match, HTML_REPLACEMENTS[match])
          match_processed.append(match)
        }
      }
    }
  }
  return str
}

var REGEXP_ESCAPE_RE = '/[.?*+^$[\]\\\\(){}|-]/'

def escape_rE(str) {
  return str.replace(REGEXP_ESCAPE_RE, '\\$&')
}

def is_space(code) {
  using code {
    when 0x09, 0x20, '\t', ' ' return true
  }
  return false;
}

# Zs (unicode class) || [\t\f\v\r\n]
def is_white_space(code) {
  if is_string(code) code = ord(code)
  if code >= 0x2000 and code <= 0x200A return true
  using code {
    when  0x09, # \t
          0x0A, # \n
          0x0B, # \v
          0x0C, # \f
          0x0D, # \r
          0x20,
          0xA0,
          0x1680,
          0x202F,
          0x205F,
          0x3000 return true
  }
  return false
}

var UNICODE_PUNCT_RE = '/[!-#%-\*,-\/:;\?@\[-\]_\{\}\xa1\xa7\xab\xb6\xb7' +
  '\xbb\xbf\u037e\u0387\u055a-\u055f\u0589\u058a\u05be\u05c0\u05c3\u05c6' +
  '\u05f3\u05f4\u0609\u060a\u060c\u060d\u061b\u061e\u061f\u066a-\u066d\u06d4' +
  '\u0700-\u070d\u07f7-\u07f9\u0830-\u083e\u085e\u0964\u0965\u0970\u09fd\u0a76' +
  '\u0af0\u0c84\u0df4\u0e4f\u0e5a\u0e5b\u0f04-\u0f12\u0f14\u0f3a-\u0f3d\u0f85' +
  '\u0fd0-\u0fd4\u0fd9\u0fda\u104a-\u104f\u10fb\u1360-\u1368\u1400\u166d\u166e' +
  '\u169b\u169c\u16eb-\u16ed\u1735\u1736\u17d4-\u17d6\u17d8-\u17da\u1800-\u180a' +
  '\u1944\u1945\u1a1e\u1a1f\u1aa0-\u1aa6\u1aa8-\u1aad\u1b5a-\u1b60\u1bfc-\u1bff' +
  '\u1c3b-\u1c3f\u1c7e\u1c7f\u1cc0-\u1cc7\u1cd3\u2010-\u2027\u2030-\u2043\u2045-' +
  '\u2051\u2053-\u205e\u207d\u207e\u208d\u208e\u2308-\u230b\u2329\u232a\u2768-' +
  '\u2775\u27c5\u27c6\u27e6-\u27ef\u2983-\u2998\u29d8-\u29db\u29fc\u29fd\u2cf9-' +
  '\u2cfc\u2cfe\u2cff\u2d70\u2e00-\u2e2e\u2e30-\u2e4e\u3001-\u3003\u3008-\u3011' +
  '\u3014-\u301f\u3030\u303d\u30a0\u30fb\ua4fe\ua4ff\ua60d-\ua60f\ua673\ua67e\ua6f2-' +
  '\ua6f7\ua874-\ua877\ua8ce\ua8cf\ua8f8-\ua8fa\ua8fc\ua92e\ua92f\ua95f\ua9c1-\ua9cd' +
  '\ua9de\ua9df\uaa5c-\uaa5f\uaade\uaadf\uaaf0\uaaf1\uabeb\ufd3e\ufd3f\ufe10-\ufe19' +
  '\ufe30-\ufe52\ufe54-\ufe61\ufe63\ufe68\ufe6a\ufe6b\uff01-\uff03\uff05-\uff0a\uff0c-' +
  '\uff0f\uff1a\uff1b\uff1f\uff20\uff3b-\uff3d\uff3f\uff5b\uff5d\uff5f-\uff65]|\ud800[' +
  '\udd00-\udd02\udf9f\udfd0]|\ud801\udd6f|\ud802[\udc57\udd1f\udd3f\ude50-\ude58\ude7f' +
  '\udef0-\udef6\udf39-\udf3f\udf99-\udf9c]|\ud803[\udf55-\udf59]|\ud804[\udc47-\udc4d' +
  '\udcbb\udcbc\udcbe-\udcc1\udd40-\udd43\udd74\udd75\uddc5-\uddc8\uddcd\udddb\udddd-' +
  '\udddf\ude38-\ude3d\udea9]|\ud805[\udc4b-\udc4f\udc5b\udc5d\udcc6\uddc1-\uddd7\ude41-' +
  '\ude43\ude60-\ude6c\udf3c-\udf3e]|\ud806[\udc3b\ude3f-\ude46\ude9a-\ude9c\ude9e-\udea2]|' +
  '\ud807[\udc41-\udc45\udc70\udc71\udef7\udef8]|\ud809[\udc70-\udc74]|\ud81a[\ude6e\ude6f' +
  '\udef5\udf37-\udf3b\udf44]|\ud81b[\ude97-\ude9a]|\ud82f\udc9f|\ud836[\ude87-\ude8b]|' +
  '\ud83a[\udd5e\udd5f]/'

# Currently without astral characters support.
def is_punct_char(ch) {
  return ch.match(UNICODE_PUNCT_RE)
}

def is_md_ascii_punct(ch) {
  if is_string(ch) ch = ord(ch)
  using ch {
    when  0x21, # !
          0x22, # "
          0x23, # #
          0x24, # $
          0x25, # %
          0x26, # &
          0x27, # '
          0x28, # (
          0x29, # )
          0x2A, # *
          0x2B, # +
          0x2C, # ,
          0x2D, # -
          0x2E, # .
          0x2F, # /
          0x3A, # :
          0x3B, # ;
          0x3C, # <
          0x3D, # =
          0x3E, # >
          0x3F, # ?
          0x40, # @
          0x5B, # [
          0x5C, # \
          0x5D, # ]
          0x5E, # ^
          0x5F, # _
          0x60, # `
          0x7B, # {
          0x7C, # |
          0x7D, # }
          0x7E  # ~
            return true
    default return false
  }
}

def normalize_reference(str) {
  # Trim and collapse whitespace
  return str.trim().replace('/\s+/', ' ').upper()
}