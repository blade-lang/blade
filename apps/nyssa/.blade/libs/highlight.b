var blade_keywords = '|'.join([
  'as', 'assert', 'break', 'catch', 'class', 'continue', 
  'def', 'default', 'die', 'do', 'echo', 'else', 'finally', 'for', 
  'if', 'import', 'in', 'iter', 'return', 'static', 'try', 
  'using', 'var', 'when', 'while',
])

var constant_keywords = '|'.join([
  'nil', 'parent', 'self', 'true', 'false', 'and', 'or'
])

var _quote_re = '/((\'(?:[^\'\\\\]|\\.)*\')|("(?:[^"\\\\]|\\.)*"))/'

def highlight_blade(text) {
  text = text.
    # operators
    replace('/([+\-*=/%!<>@]|\.\.)/', '<_o>$1</_o>').
    replace('/\\b(and|or)\\b/', '<_o>$1</_o>').
    # quotes
    replace(_quote_re, '<_q>$1</_q>').
    # constant keywords
    replace('/\\b(${constant_keywords})\\b/', '<_c>$1</_c>').

    # numbers
    replace('/(([0-9][0-9]*\.[0-9]+([eE][0-9]+)?[fd]?)|(0x[0-9a-fA-F]+)|(0c[0-7][0-7]*)|(0b[01][01]*)|([0-9]+))/', '<_n>$1</_n>').

    # functions
    # property/method call and access
    replace('/(?<=\.)[ ]*([a-zA-Z_][a-zA-Z0-9_]*)[ ]*(?=[(])/', '<_m>$1</_m>').
    # definition and call
    replace('/(?<!\.)([a-zA-Z_][a-zA-Z0-9_]*)[ ]*(?=[(])/', '<_f>$1</_f>').

    # keywords
    replace('/\\b(${blade_keywords})\\b/', '<_k>$1</_k>').
    # comments
    replace('/(#[^\\n]*|\/(?!\\\\)\*[\s\S]*?\*(?!\\\\)\/)/', '<_w>$1</_w>')

  # clean up comments
  var comments = text.matches('/<_w>(.*?)<\/_w>/')
  if comments {
    for comment in comments[1] {
      text = text.replace(comment, comment.replace('/<\/?_([^>]+)>/', ''), false)
    }
  }

  # clean up quotes
  var quotes = text.matches('/<_q>(.*?)<\/_q>/')
  if quotes {
    for quote in quotes[1] {
      text = text.replace(
        quote, 
        quote.replace('/<\/?_([^>]+)>/', '').
          # interpolation
          replace('/(\\$\{[^}]+\})/', '<_i>$1</_i>'), 
        false
      )
    }
  }

  # expand styles.
  return text.replace('/<_q>(.*?)<\/_q>/', '<span style="color:#690">$1</span>').
              replace('/<_i>(.*?)<\/_i>/', '<span style="color:#00bcd4">$1</span>').
              replace('/<_c>(.*?)<\/_c>/', '<span style="color:#ff9800">$1</span>').
              replace('/<_m>(.*?)<\/_m>/', '<span style="color:#ff5722;font-style:italic">$1</span>').
              replace('/<_f>(.*?)<\/_f>/', '<span style="color:#ff5722">$1</span>').
              replace('/<_k>(.*?)<\/_k>/', '<span style="color:#2196F3">$1</span>').
              replace('/<_w>(.*?)<\/_w>/', '<span style="color:slategray">$1</span>').
              replace('/<_o>(.*?)<\/_o>/', '<span style="color:#9a6e3a">$1</span>').
              replace('/<_n>(.*?)<\/_n>/', '<span style="color:#905">$1</span>')
}

def highlight_html5(text, lang) {
  var tags = text.matches('/<([^>]+)>/')
  if tags {
    iter var i = 0; i < tags[0].length(); i++ {
      var content = tags[1][i].replace('/([a-zA-Z_\-0-9]+)(?=[=])/', '<^a>$1</^a>').
                        replace(_quote_re, '<^v>$1</^v>') 
      text = text.replace(tags[0][i], '<span style="color:#2196f3">&lt;${content}&gt;</span>', false)
    }
  }

  var result = text.replace('/<\^a>(.*?)<\/\^a>/', '<span style="color:#9a6e3a">$1</span>').
              replace('/<\^v>(.*?)<\/\^v>/', '<span style="color:#690">$1</span>')

  if lang == 'wire' {
    result = result.replace('/(\{\{.+?\}\})/', '<span style="color:#ff9800">$1</span>')
  }

  return result
}

def highlight(text, lang) {
  if lang == 'blade' {
    return highlight_blade(text)
  } else if lang == 'html' or lang == 'html5' or lang == 'wire' {
    return highlight_html5(text, lang)
  }
  return text
}

