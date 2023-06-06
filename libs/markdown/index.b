import .common.utils
import .helpers
import .renderer { Renderer }
import .parser_core { BlockCore }
import .parser_block { BlockParser }
import .parser_inline { InlineParser }
import .config as presets
import url
import iters
import reflect
import convert { decimal_to_hex }

var _working_rules = [ 'core', 'block', 'inline' ]

var config = {
  zero: presets.zero,
  standard: presets.standard,
  commonmark: presets.commonmark,
}

########################################
#
# This validator can prohibit more than really needed to prevent XSS. It's a
# tradeoff to keep code simple and to be secure by default.
#
# If you need different setup - override validator method as you wish. Or
# replace it with dummy def and use external sanitizer.
var BAD_PROTO_RE = '/^(vbscript|javascript|file|data):/'
var GOOD_DATA_RE = '/^data:image\/(gif|png|jpeg|webp);/'

def validate_link(url) {
  # url should be normalized at this point, and existing entities are decoded
  var str = url.trim().lower()

  return str.match(BAD_PROTO_RE) ? (str.match(GOOD_DATA_RE) ? true : false) : true
}

########################################


var RECODE_HOSTNAME_FOR = [ 'http', 'https', 'mailto' ]

def _md_format(url) {
  var result = ''
  result += url.scheme ? '${url.scheme}:' : ''
  if !url.has_slash {
    if !url.scheme and url.host and !url.username {
      result += url.port or !url.path ? '' : '/'
    }
  } else {
    result += '//'
  }
  result += url.username ? url.username : ''
  result += url.password ? ':${url.password}' : ''
  result += url.username ? '@' : ''
  if url.host and url.host.index_of(':') != -1 {
    # ipv6 address
    result += '[' + url.host + ']'
  } else {
    result += url.host ? url.host.ltrim('/') : ''
  }
  
  result += url.port and url.port != '0' ? ':' + url.port : ''
  result += !url.path or url.path == '/' ? '' : url.path
  result += url.query ? '?${url.query}' : ''
  result += url.hash ? '#${url.hash}' : ''
  return result
}

var encode_cache = {}
def _get_encode_cache(exclude) {
  var i, ch, cache = encode_cache.get(exclude)
  if cache  return cache

  cache = encode_cache[exclude] = []

  iter i = 0; i < 128; i++ {
    ch = chr(i)
    if ch.match('/^[0-9a-z]$/i') {
      #  always allow unencoded alphanumeric characters
      cache.append(ch)
    } else {
      var cache_code = ('0' + decimal_to_hex(i).upper())
      cache.append('%' + cache_code[cache_code.length() - 2,])
    }
  }
  iter i = 0; i < exclude.length(); i++ {
    cache[ord(exclude[i])] = exclude[i]
  }
  return cache
}

/**
 * Encode unsafe characters with percent-encoding, skipping already
 * encoded sequences.
 * 
 * @param {string} string: string to encode
 * @param {list|string} exclude: list of characters to ignore (in addition to a-zA-Z0-9)
 * @param {bool} keep_escaped: don't encode '%' in a correct escape sequence (default: true)
 * @return string
 */
def encode_url(string, exclude, keep_escaped) {
  var i = 0, l, code, nextCode, cache, result = ''
  if !is_string(exclude) {
    # encode(string, keep_escaped)
    keep_escaped = exclude
    exclude = ';/?:@&=+$,-_.!~*\'()#'
  }
  if keep_escaped == nil keep_escaped = true
  
  cache = _get_encode_cache(exclude)
  
  iter l = string.length(); i < l; i++ {
    code = ord(string[i])
    if keep_escaped and code == '%' and i + 2 < l {
      if string[i + 1, i + 3].match('/^[0-9a-f]{2}$/i') {
        result += string[i, i + 3]
        i += 2
        continue
      }
    }
    if code < 128 {
      result += cache[code]
      continue
    }
    if code >= 55296 and code <= 57343 {
      if code >= 55296 and code <= 56319 and i + 1 < l {
        nextCode = ord(string[i + 1])
        if nextCode >= 56320 and nextCode <= 57343 {
          result += url.encode(string[i] + string[i + 1])
          i++
          continue
        }
      }
      result += "%EF%BF%BD"
      continue
    }
    result += url.encode(string[i])
  }
  
  return result
}

def normalize_link(uri) {
  var parsed = url.parse(uri)

  if parsed.host {
    # Encode hostnames in urls like:
    # `http://host/`, `https://host/`, `mailto:user@host`, `//host/`
    #
    # We don't encode unknown schemas, because it's likely that we encode
    # something we shouldn't (e.g. `skype:name` treated as `skype:host`)
    if !parsed.scheme or RECODE_HOSTNAME_FOR.contains(parsed.scheme) {
      parsed.host.ascii()
    }
  }

  # return encode_url(parsed.absolute_url())
  return encode_url(_md_format(parsed))
}

def normalize_link_text(uri) {
  var parsed = url.parse(uri)

  if parsed.host {
    # Encode hostnames in urls like:
    # `http://host/`, `https://host/`, `mailto:user@host`, `#host/`
    #
    # We don't encode unknown schemas, because it's likely that we encode
    # something we shouldn't (e.g. `skype:name` treated as `skype:host`)
    if !parsed.scheme or RECODE_HOSTNAME_FOR.contains(parsed.scheme) {
      parsed.host.ascii(false)
    }
  }

  # return parsed.absolute_url()
  return _md_format(parsed)
}


/**
 * Main parser/renderer class.
 *
 * ##### Usage
 *
 * ```blade
 * import markdown
 * 
 * var md = markdown()
 * echo md.render('# markdown is bae!')
 * ```
 *
 * Single line rendering, without paragraph wrap:
 *
 * ```blade
 * import markdown
 * 
 * var md = markdown()
 * echo md.render_inline('__markdown__ rulezz!')
 * ```
 */
class Markdown {

  /**
   * Instance of [[InlineParser]]. You may need it to add new rules when
   * writing plugins. For simple rules control use [[Markdown.disable]] and
   * [[Markdown.enable]].
   * 
   * @type InlineParser
   */
  var inline = InlineParser()

  /**
   * Instance of [[BlockParser]]. You may need it to add new rules when
   * writing plugins. For simple rules control use [[Markdown.disable]] and
   * [[Markdown.enable]].
   * 
   * @type BlockParser
   */
  var block = BlockParser()

  /**
   * Instance of [[Core]] chain executor. You may need it to add new rules when
   * writing plugins. For simple rules control use [[Markdown.disable]] and
   * [[Markdown.enable]].
   * 
   * @type BlockCore
   */
  var core = BlockCore()

  /**
   * Instance of [[Renderer]]. Use it to modify output look. Or to add rendering
   * rules for new token types, generated by plugins.
   *
   * ##### Example
   *
   * ```blade
   * import markdown
   * var md = markdown()
   *
   * def my_token(tokens, idx, options, env, this) {
   *   #...
   *   return result
   * }
   *
   * md.renderer.rules['my_token'] = my_token
   * ```
   *
   * See [[Renderer]] docs and [source code](https://github.com/blade-lang/blade/blob/master/libs/markdown/renderer.b).
   * 
   * @type Renderer
   */
  var renderer = Renderer()

  /**
   * Link validation function. CommonMark allows too much in links. By default
   * we disable `javascript:`, `vbscript:`, `file:` schemas, and almost all `data:...` schemas
   * except some embedded image types.
   *
   * You can change this behaviour:
   *
   * ```blade
   * import markdown
   * var md = markdown()
   * 
   * # enable everything
   * md.validate_link = @(){ return true; }
   * ```
   * 
   * @param {string} url
   * @return bool
   */
  var validate_link = validate_link

  /**
   * Function used to encode link url to a machine-readable format,
   * which includes url-encoding, punycode, etc.
   * 
   * @param {string} url
   * @return string
   */
  var normalize_link = normalize_link

  /**
   * normalize_link_text(url)
   *
   * Function used to decode link url to a human-readable format`
   * 
   * @param {string} url
   * @return string
   */
  var normalize_link_text = normalize_link_text


  # Expose utils & helpers for easy acces from plugins

  /**
   * Assorted utility functions, useful to write plugins. See details
   * [here](https://github.com/blade-lang/blade/blob/master/libs/markdown/common/utils.b).
   * 
   * @type module
   */
  var utils = utils

  /**
   * Link components parser functions, useful to write plugins. See details
   * [here](https://github.com/blade-lang/blade/blob/master/libs/markdown/helpers).
   * 
   * @type dict
   */
  var helpers = {
    parse_link_destination: helpers.parse_link_destination,
    parse_link_label: helpers.parse_link_label,
    parse_link_title: helpers.parse_link_title,
  }

  /**
   * Creates parser instanse with given config. Can be called without `new`.
   *
   * ##### preset_name:
   *
   * Markdown provides named presets as a convenience to quickly
   * enable/disable active syntax rules and options for common use cases.
   *
   * - `commonmark`: configures parser to strict [CommonMark](http://commonmark.org/) mode.
   * - `standard`: similar to GFM, used when no preset name given. Enables all available rules,
   *   but still without html, typographer & autolinker.
   * - `zero`: all rules disabled. Useful to quickly setup your config via `.enable()`.
   *   For example, when you need only `bold` and `italic` markup and nothing else.
   *
   * ##### options:
   *
   * - __html__ - `false`. Set `true` to enable HTML tags in source. Be careful!
   *   That's not safe! You may need external sanitizer to protect output from XSS.
   *   It's better to extend features via plugins, instead of enabling HTML.
   * - __xhtml_out__ - `false`. Set `true` to add '/' when closing single tags
   *   (`<br />`). This is needed only for full CommonMark compatibility. In real
   *   world you will need HTML output.
   * - __breaks__ - `false`. Set `true` to convert `\n` in paragraphs into `<br>`.
   * - __lang_prefix__ - `language-`. CSS language class prefix for fenced blocks.
   *   Can be useful for external highlighters.
   * - __linkify__ - `false`. Set `true` to autoconvert URL-like text to links.
   * - __typographer__  - `false`. Set `true` to enable [some language-neutral
   *   replacement](https://github.com/blade-lang/blade/blob/master/libs/markdown/rules_core/replacements.b) +
   *   quotes beautification (smartquotes).
   * - __quotes__ - `“”‘’`, String or Array. Double + single quotes replacement
   *   pairs, when typographer enabled and smartquotes on. For example, you can
   *   use `'«»„“'` for Russian, `'„“‚‘'` for German, and
   *   `['«\xA0', '\xA0»', '‹\xA0', '\xA0›']` for French (including nbsp).
   * - __highlight__ - `nil`. Highlighter def for fenced code blocks.
   *   Highlighter `def (str, lang)` should return escaped HTML. It can also
   *   return empty string if the source was not changed and should be escaped
   *   externaly. If result starts with <pre... internal wrapper is skipped.
   *
   * ##### Example
   *
   * ```blade
   * import markdown
   * # commonmark mode
   * var md = markdown('commonmark')
   *
   * # standard mode
   * var md = markdown()
   *
   * # enable everything
   * var md = markdown({
   *   html: true,
   *   linkify: true,
   *   typographer: true
   * })
   * ```
   *
   * ##### Syntax highlighting
   *
   * ```blade
   * var md = markdown({
   *   highlight: @(str, lang) {
   *     if lang and get_language(lang) {
   *       return do_highlight(str, lang)
   *     }
   *
   *     return '' # use external default escaping
   *   }
   * })
   * ```
   *
   * Or with full wrapper override (if you need assign class to `<pre>`):
   *
   * ```blade
   * # Actual default values
   * var md = markdown({
   *   highlight: @(str, lang) {
   *     if lang and get_language(lang) {
   *       return '<pre class="hljs"><code>' +
  *          do_highlight(str, lang).value +
  *        '</code></pre>'
   *     }
   *
   *     return '<pre class="hljs"><code>' + md.utils.escape_html(str) + '</code></pre>'
   *   }
   * })
   * ```
   * 
   * @param {string|nil} preset_name: `commonmark`, `standard` or `zero` (default: `standard`)
   * @param {dict|nil} options
   */
  Markdown(preset_name, options) {
    if !instance_of(self, Markdown) {
      Markdown(preset_name, options)
    } else {
      if !options {
        if !is_string(preset_name) {
          options = preset_name or {}
          preset_name = 'standard'
        }
      }
  
  
      self.options = {}
      self.configure(preset_name)
  
      if options self.set(options)
    }
  }

  /**
   * Set parser options (in the same format as in constructor). Probably, you
   * will never need it, but you can change options after constructor call.
   *
   * ##### Example
   *
   * ```blade
   * import markdown
   * var md = markdown().
   *     set({ html: true, breaks: true }).
   *     set({ typographer, true })
   * ```
   *
   * __Note:__ To achieve the best possible performance, don't modify a
   * `markdown` instance options on the fly. If you need multiple configurations
   * it's best to create multiple instances and initialize each with separate
   * config.
   * 
   * @param {dict} options
   * @chainable
   */
  set(options) {
    utils.assign(self.options, options)
    return self
  }

  /**
   * Batch load of all options and compenent settings. This is internal method,
   * and you probably will not need it. But if you will - see available presets
   * and data structure [here](https://github.com/blade-lang/blade/tree/master/libs/markdown/presets)
   *
   * We strongly recommend to use presets instead of direct config loads. That
   * will give better compatibility with standard versions.
   * 
   * @param {dict|string} options
   * @chainable
   * @internal
   */
  configure(presets) {
    var preset_name

    if is_string(presets) {
      preset_name = presets
      presets = config[preset_name]
      if !presets die Exception('Wrong `markdown` preset "' + preset_name + '", check name')
    }

    if !presets die Exception('Wrong `markdown` preset, can\'t be empty')

    if presets.options self.set(presets.options)

    if presets.components {
      iters.each(presets.components.keys(), @(name) {
        if presets.components[name].get('rules') {
          reflect.get_prop(self, name).ruler.enable_only(presets.components[name].rules)
        }
        if presets.components[name].get('rules2') {
          reflect.get_prop(self, name).ruler2.enable_only(presets.components[name].rules2)
        }
      })
    }

    return self
  }

  /**
   * Enable list or rules. It will automatically find appropriate components,
   * containing rules with given names. If rule not found, and `ignore_invalid`
   * not set - throws exception.
   *
   * ##### Example
   *
   * ```blade
   * import markdown
   * var md = markdown().
   *    enable(['sub', 'sup']).
   *    disable('smartquotes')
   * ```
   * 
   * @param {string|list} list: rule name or list of rule names to enable
   * @param {bool} ignore_invalid: set `true` to ignore errors when rule not found.
   * @chainable
   */
  enable(list, ignore_invalid) {
    var result = []

    if !is_list(list) list = [ list ]

    iters.each(_working_rules, @(chain) {
      result += reflect.get_props(self, chain).ruler.enable(list, true)
    })

    result += self.inline.ruler2.enable(list, true)

    var missed = iters.filter(list, @(name) { return result.index_of(name) < 0 })

    if missed.length() and !ignore_invalid {
      die Exception('Markdown. Failed to enable unknown rule(s): ' + missed)
    }

    return self
  }

  /**
   * The same as [[Markdown.enable]], but turn specified rules off.
   * 
   * @param {string|list} list: rule name or list of rule names to disable.
   * @param {bool} ignore_invalid: set `true` to ignore errors when rule not found.
   * @chainable
   */
  disable(list, ignore_invalid) {
    var result = []

    if !is_list(list) list = [ list ]

    iters.each(_working_rules, @(chain) {
      result += reflect.get_prop(self, chain).ruler.disable(list, true)
    })

    result += self.inline.ruler2.disable(list, true)

    var missed = iters.filter(list, @(name) { return result.index_of(name) < 0 })

    if missed.length() and !ignore_invalid {
      die Exception('Markdown. Failed to disable unknown rule(s): ' + missed)
    }

    return self
  }

  /**
   * Load specified plugin with given params into current parser instance.
   * It's just a sugar to call `plugin(md, params)` with curring.
   *
   * ##### Example
   *
   * ```blade
   * import markdown
   * import .markdown_custom_inline
   * 
   * var md = markdown()
   *    .use(markdown_custom_inline, 'foo_replace', 'text', def (tokens, idx) {
   *      tokens[idx].content = tokens[idx].content.replace('/foo/', 'bar')
   *    })
   * ```
   * 
   * @param {function|module} plugin
   * @param {...any} params
   * @chainable
   */
  use(plugin, ...) {
    plugin(self, __args__)
    return self
  }

  /**
   * Parse input string and return list of block tokens (special token type
   * "inline" will contain list of inline tokens). You should not call this
   * method directly, until you write custom renderer (for example, to produce
   * AST).
   *
   * `env` is used to pass data between "distributed" rules and return additional
   * metadata like reference info, needed for the renderer. It also can be used to
   * inject data in specific cases. Usually, you will be ok to pass `{}`,
   * and then pass updated object to renderer.
   * 
   * @param {string} src: source string
   * @param {dict} env: environment sandbox
   * @return list
   * @internal
   */
  parse(src, env) {
    if !is_string(src) {
      die Exception('Input data should be a String')
    }

    var state = self.core.State(src, self, env)

    self.core.process(state)

    return state.tokens
  }

  /**
   * Render markdown string into html. It does all magic for you :).
   *
   * `env` can be used to inject additional metadata (`{}` by default).
   * But you will not need it with high probability. See also comment
   * in [[Markdown.parse]].
   * 
   * @param {string} src: source string
   * @param {object|nil} env: environment sandbox
   * @return string
   */
  render(src, env) {
    env = env or {}

    return self.renderer.render(self.parse(src, env), self.options, env)
  }

  /**
   * The same as [[Markdown.parse]] but skip all block rules. It returns the
   * block tokens list with the single `inline` element, containing parsed inline
   * tokens in `children` property. Also updates `env` object.
   * 
   * @param {string} src: source string
   * @param {object|nil} env: environment sandbox
   * @return list
   * @internal
   **/
  parse_inline(src, env) {
    var state = self.core.State(src, self, env)

    state.inline_mode = true
    self.core.process(state)

    return state.tokens
  }

  /**
   * Similar to [[Markdown.render]] but for single paragraph content. Result
   * will NOT be wrapped into `<p>` tags.
   * 
   * @param {string} src: source string
   * @param {object|nil} env: environment sandbox
   * @return string
   */
  render_inline(src, env) {
    env = env or {}

    return self.renderer.render(self.parse_inline(src, env), self.options, env)
  }
}

/**
 * Returns a new instance of class Markdown.
 * 
 * @param {string} preset_name: optional, `commonmark` / `zero`
 * @param {dict} options
 * @return {Markdown}
 * @default
 */
def markdown(preset_name, options) {
  return Markdown(preset_name, options)
}

