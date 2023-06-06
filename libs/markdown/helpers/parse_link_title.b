# Parse link title

import ..common.utils { unescape_all }

def parse_link_title(str, start, max) {
  var code,
      marker,
      lines = 0,
      pos = start,
      result = {
        ok: false,
        pos: 0,
        lines: 0,
        str: '',
      }

  if pos >= max return result

  marker = str[pos]

  if marker != '"' and marker != "'" and marker != '(' return result

  pos++

  # if opening marker is "(", switch it to closing marker ")"
  if marker == '(' marker = ')'

  while pos < max {
    code = str[pos]
    if code == marker {
      result.pos = pos + 1
      result.lines = lines
      result.str = unescape_all(str[start + 1, pos])
      result.ok = true
      return result
    } else if code == '(' and marker == ')' {
      return result
    } else if ord(code) == 0x0A {
      lines++
    } else if code == '\\' and pos + 1 < max {
      pos++
      if ord(str[pos]) == 0x0A {
        lines++
      }
    }

    pos++
  }

  return result
}