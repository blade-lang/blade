/**
 * @module template
 * 
 * The template module contains classes and functions for working with Blade's `wire` 
 * templates. Wire templating is an extensible template system built on standard HTML5. 
 * In fact, any valid HTML5 document is also a valid Wire template. Wire builds atop 
 * the existing HTML5 language to provide template support for Web development in Blade 
 * and makes extensive use of HTML attributes for condition and looping.
 * 
 * Wire templates allow creation of custom HTML elements (such as the builtin `<include />` 
 * tag that allows for inlining other template files in a template file.). It's a simple 
 * yet effective system of dynamic programming and the flow altogether feels like writing 
 * in a frontend framework such as VueJS.
 * 
 * ### Basic Usage
 * 
 * ```blade
 * import template
 * 
 * var tpl = template()
 * echo tpl.render('mytemplate')
 * ```
 * 
 * Or to render from a string
 * 
 * ```blade
 * echo tpl.render_string('<p>{{ name }}</p>', {name: 'Hello World'})
 * ```
 * 
 * The last example above will render the following:
 * 
 * ```html
 * <p>Hello World</p>
 * ```
 * 
 * ### Comments
 * 
 * Wire inherits HTMLs comments using the same syntax `<!-- ... -->`. It is important to note 
 * that Wire does not render comments in the output HTML. For web applications, this helps you 
 * to write server side comments in your Wire files without it getting to the frontend since 
 * Wire is backend code.
 * 
 * For example, the code below should return an empty string.
 * 
 * ```blade
 * tpl.render_string('<!-- HTML or Wire comment? -->')
 * ```
 * 
 * ### Variables
 * 
 * Variables in Wire templates are names surrounded by `{{` and `}}` pair. For example, to print 
 * the value of a variable _myvar_ passed into [[Template.render]] or [[Template.render_string]] 
 * in the template, you can do it like this.
 * 
 * ```html
 * <div>{{ myvar }}</div>
 * ```
 * 
 * > **NOTE:** 
 * > - The `<div>` and `</div>` surround the variable and are not part of the variable.
 * > - The spaces around the variable are just formatting and are not required.
 * 
 * The exception to this is when passing a variable to a reserved attribute such as `x-key`. 
 * In this case, you'll need to omit the surrounding braces. 
 * 
 * For example:
 * 
 * ```html
 * <div x-for="myvar" x-key="mykey"></div>
 * ```
 * 
 * As shown in the example above, variables can occur anywhere in a Wire template including in 
 * element attributes.
 * 
 * To print the exact characters `{{ myvar }}` if that's what you actually mean and stop if from 
 * being interpreted as a variable, you'll need to escape the first `{` with the percent sign `%`. 
 * 
 * For example:
 * 
 * ```html
 * <div>%{{ myvar }}</>
 * ```
 * 
 * The example above will return the following:
 * 
 * ```html
 * <div>{{ myvar }}</div>
 * ```
 * 
 * ### Expressions and Modifiers
 * 
 * Expressions in Wire are a feature that allows modification of value directly in the template. 
 * An __Expression__ is value that has been modified by passing it through a functions called 
 * __Modifiers__ using the pipe (`|`) character. Wire comes with a lot of built-in functions for 
 * creating expressions and they are all described at below.
 * 
 * For example:
 * 
 * ```html
 * <div>{{ name|length }}</div>
 * ```
 * 
 * In the example above, the name variable was expressed as its length by passing it into through 
 * the _length_ modifier function. If _name_ contains the value `John Doe`, then the value printed 
 * will be `8`.
 * 
 * The built-in modifiers are:
 * 
 * - {{length}}
 * - {{upper}}
 * - {{lower}}
 * - {{is}}
 * - {{not}}
 * - {{empty}}
 * - {{reverse}}
 * - {{string}}
 * 
 * ### If... and If not...
 * 
 * Wire implements conditionals via the `x-if` and `x-not` attribute that can be attached to any HTML 
 * element. This attributes are never returned in the compiled HTML output and decides wether an 
 * element will be printed or not. The `x-if` attribte evaluates a variable or expression and will only 
 * print the element to which it is attached and its children if the result of the expression or variable 
 * evaulation returns a value that is boolean `true` in Blade. The `x-not` attribute does the reverse of 
 * this (i.e. it only prints if the evaulation returns Blade boolean `false`).
 * 
 * ```blade
 * tpl.render_string('<div x-if="name">Hello</div>')
 * ```
 * 
 * The example above will return an empty string since the variable name was never declared. However, the 
 * reverse is the case if the attribute was `x-not`. 
 * 
 * For example:
 * 
 * ```blade
 * tpl.render_string('<div x-not="name">Hello</div>')
 * ```
 * 
 * The example above will return `<div>Hello</div>`.
 * 
 * ### Loops
 * 
 * Wire templates support for loops is enabled via the `x-for` attribute that can be applied to any 
 * element. When the `x-for` attribute is present on an element, it must declare a corresponding `x-value` 
 * attribute that defines the name of the value variable within the loop. An optional `x-key` attribute 
 * may also be defined to define a variable name that will contain the value of the iteration index/key.
 * 
 * The _x-for_ attribute must declare a variable or expression that evaluates into an iterable (such as a 
 * string, list, dictionary etc.).
 * 
 * For example:
 * 
 * ```blade
 * tpl.render_string('<div x-for="data">Ok</div>', {data: 0..3})
 * ```
 * 
 * The code above will return the following:
 * 
 * ```html
 * <div>Ok</div><div>Ok</div><div>Ok</div>
 * ```
 * 
 * > The original `<div>` was replicated three times without the `x-for` attribute. 
 * > __Wire attributes are applied to an element and their children not the children only.__
 * 
 * Here is an example using the `x-value` attribute to print the items in a list.
 * 
 * ```blade
 * tpl.render_string('<div x-for="data" x-value="val">{{ val }}</div>', {data: ['apple', 'mango']})
 * ```
 * 
 * The code above return
 * 
 * ```html
 * <div>apple</div><div>mango</div>
 * ```
 * 
 * We could decide to print the index as well by adding a new variable using the `x-key` attribute.
 * 
 * ```blade
 * tpl.render_string('<div x-for="data" x-value="val" x-key="key">
 *   <span>{{ key }}</span>
 *   <span>{{ value }}</span>
 * </div>', {data: ['apple', 'mango']})
 * ```
 * 
 * Which will output
 * 
 * ```html
 * <div>
 *   <span>0</span>
 *   <span>apple</span>
 * </div><div>
 *   <span>1</span>
 *   <span>mango</span>
 * </div>
 * ```
 * 
 * ### Custom Modifiers
 * 
 * ### Custom Tags
 * 
 * ### Template Functions
 */

import os
import json
import iters
import html
import reflect
import .functions as fns
import .constants

/**
 * Template string and file processing class.
 * 
 * ##### Usage
 * 
 * You can render templates directly from strings
 * 
 * ```blade
 * import template
 * var tpl = template()
 * 
 * tpl.render_string('{{ name }}', {name: 'John Doe'})
 * ```
 * 
 * Or from files located in your defined root directory. See [[Template.set_root]]
 * 
 * ```blade
 * tpl.render('my_template', {name: 'John Doe'})
 * ```
 * 
 * You can enable initialize your templates with the auto_init option to allow 
 * [[Template]] create the root directory if it does not exist. The default root 
 * directory is a directory "`templates`" in the current working directory.
 * 
 * For example,
 * 
 * ```blade
 * var tpl = template(true)
 * 
 * # Optionally set the root directory to another directory.
 * tpl.set_root('/my/custom/path')
 * ```
 * 
 * The root directory will become the root search path for the `<include />` tag.
 * 
 * The default extension for a template file is the `.html` extension. This extension 
 * allows furnishes the interopability between Blade's Wire templates and HTML5 since the 
 * former is based on the later anyway and allows us to leverage the already near 
 * omnipresent support that HTML files have had over the years. This behavior can be 
 * changed using the [[Template.set_extension]] function to change the extension to any 
 * desired string.
 * 
 * For example,
 * 
 * ```blade
 * tpl.set_extension('.wire')
 * 
 * # render a template from file
 * tpl.render('welcome')
 * ```
 * 
 * This will cause [[Template.render]] to look for the file "`welcome.wire`" in the root 
 * directory and will return an error if the file could not be found and no file matches 
 * exactly "`welcome`" in the directory.
 */
class Template {

  # value modifier functions
  var _functions = fns.mapping.clone()

  # custom  elements
  var _elements = {}

  # root directory
  var _root_dir = constants.DEFUALT_ROOT_DIR

  # auto_init control
  var _auto_init = false

  # template file extension
  var _file_ext = constants.DEFAULT_EXT

  /**
   * The constructor of the Template class.
   * 
   * @param {bool} auto_init: A boolean flag to control whether template root 
   *    directory will be automatically created on [[Template.set_root]] or 
   *    [[Template.render]].
   * @constructor
   */
  Template(auto_init) {
    if auto_init != nil and !is_boool(auto_init)
      die Exception('boolean expected in argument 1 (auto_init)')
    self._auto_init = auto_init == nil ? false : auto_init
  }

  _get_attrs(attrs) {
    return iters.reduce(attrs, @(dict, attr) {
      dict.set(attr.name, attr.value)
      return dict
    }, {})
  }

  # remove comments and surrounding white space
  _strip(txt) {
    return txt.trim().replace(constants.COMMENT_RE, '')
  }

  _strip_attr(element, ...) {
    var attrs = __args__
    element.attributes = iters.filter(element.attributes, @(el) {
      return !attrs.contains(el.name)
    })
  }

  _extract_var(variables, _var, error) {
    var var_split = _var.split('|')
    if var_split {
      var _vars = var_split[0].split('.')
      var real_var
  
      if variables.contains(_vars[0]) {
        if _vars.length() > 1 {
          var final_var = variables[_vars[0]]
          iter var i = 1; i < _vars.length(); i++ {
            if is_dict(final_var) {
              final_var = final_var[_vars[i].matches(constants.NUMBER_RE) ? 
                to_number(_vars[i]) : _vars[i]]
            } else if (is_list(final_var) or is_string(final_var)) and 
              _vars[i].matches(constants.NUMBER_RE) {
              final_var = final_var[to_number(_vars[i])]
            } else {
              error('could not resolve "${_var}" at "${_vars[i]}"')
            }
          }
  
          real_var = final_var
        } else {
          real_var = variables[_vars[0]]
        }
  
        if var_split.length() > 1 {
          iter var i = 1; i < var_split.length(); i++ {
            var fn = var_split[i].split('=')
            if self._functions.contains(fn[0]) {
              if fn.length() == 1 {
                real_var = self._functions[fn[0]](real_var)
              } else {
                var val = fn[1]
                if val.match(constants.QUOTE_VALUE_RE) {
                  real_var = self._functions[fn[0]](real_var, val[1,-1])
                } else {
                  var vval

                  if val == 'nil' vval = nil
                  else if val == 'true' vval = true
                  else if val == 'false' vval = false
                  else if val.match(constants.NUMBER_WITH_DECIMAL_RE) 
                    vval = to_number(val.match(constants.NUMBER_WITH_DECIMAL_RE)[0])
                  else vval = self._extract_var(variables, val, error)

                  real_var = self._functions[fn[0]](real_var, vval)
                }
              }
            } else {
              error('template function "${fn[0]}" not declared')
            }
          }
        }
  
        return real_var
      } else {
        # error('could not resolve "${_vars[0]}"')

        # Instead of returning an error, we'll return an empty string. This allows us to test 
        # for falsey, allows us to catch non-existing variables without an Exception and still 
        # allow modifiers to be used with the non existing value. 
        # 
        # E.g. x-if="nonexistingvar|length" should still be false.
        return ''
      }
    } else {
      error('invalid variable "${_var}"')
    }
  
    return ''
  }

  _replace_funcs(content, error) {
    # prepare
    content = content.replace('%{!', '%{\x01!')
    # replace functions: {! fn !}
    # 
    # NOTE: This must come only just after variable replace as previous actions could generate or 
    # contain functions as well.
    var fn_vars = content.matches(constants.FUNCTION_RE)
    if fn_vars {
      # var_vars = json.decode(json.encode(fn_vars))
      iter var i = 0; i < fn_vars.fn.length(); i++ {
        var fn
        if (fn = self._functions.get(fn_vars.fn[i], nil)) and fn {
          content = content.replace(fn_vars[0][i], fn(), false)
        }
      }
    }
    
    # strip function escapes
    return content.replace('%{\x01!', '{!', false)
  }

  _replace_vars(content, variables, error) {
    # prepare
    content = content.replace('%{{', '%{\x01{')
    # replace variables: {{var_name}}
    # 
    # NOTE: This must come last as previous actions could generate or 
    # contain variables as well.
    var var_vars = content.matches(constants.VAR_RE)
    if var_vars {
      # var_vars = json.decode(json.encode(var_vars))
      iter var i = 0; i < var_vars.variable.length(); i++ {
        content = content.replace(
          var_vars[0][i], 
          to_string(self._extract_var(variables, var_vars.variable[i], error)), 
          false
        )
      }
    }
    
    # strip variable escapes
    return self._replace_funcs(content.replace('%{\x01{', '{{', false), error)
  }

  _process(path, element, variables) {
    if !element return nil
  
    def error(message) {
      if !is_string(element) and !is_list(element) {
        var start = element.position.start
        die Exception('${message} at ${path}[${start.line},${start.column}]') 
      } else {
        { die Exception(message) }
      }
    }
  
    if is_string(element) {
      return self._replace_vars(element, variables, error)
    }
  
    if is_list(element) {
      return iters.map(element, @(el) {
        return self._process(path, el, variables)
      }).compact()
    }
    
    if element.type == 'text' {
      # replace variables: {{var_name}}
      element.content = self._process(path, element.content, variables)
      return element
    } else {
      var attrs = self._get_attrs(element.attributes)
  
      if element {

        # process elements
        if element.name == constants.INCLUDE_TAG {
          if !attrs or !attrs.contains(constants.PATH_ATTR)
            error('missing "${constants.PATH_ATTR}" attribute for include tag')
  
          var include_path = os.join_paths(self._root_dir, attrs.path)
          if !include_path.match(constants.EXT_RE) include_path += constants.DEFAULT_EXT
          var fl = file(include_path)
          if fl.exists() {
            element = self._process(
              include_path, 
              html.decode(self._strip(fl.read()), {with_position: true}), 
              variables
            )
          } else {
            error('template "${attrs.path}" not found')
          }
        } else if self._elements.contains(element.name) {
          # process custom elements
          var processed = self._elements[element.name](self, element)
          if processed {
            if !is_string(processed)
              error('invalid return when processing "${element.name}" tag')
            element = self._process(
              path, 
              html.decode(self._strip(processed), {with_position: true}), 
              variables
            )
          } else {
            element = nil
          }
        }
      }
  
      # process directives
      if attrs.contains(constants.IF_ATTR) {
        # if tag
        var _var = self._extract_var(variables, attrs.get(constants.IF_ATTR), error)
        if _var {
          self._strip_attr(element, constants.IF_ATTR)
          element = self._process(path, element, variables)
        } else {
          element = nil
        }
      } else if attrs.contains(constants.NOT_ATTR) {
        # if not tag
        var _var = self._extract_var(variables, attrs.get(constants.NOT_ATTR), error)
        if !_var {
          self._strip_attr(element, constants.NOT_ATTR)
          element = self._process(path, element, variables)
        } else {
          element = nil
        }
      } else if attrs.contains(constants.FOR_ATTR) {
        # for tag
        
        var data = self._extract_var(variables, attrs.get(constants.FOR_ATTR), error),
            key_name = attrs.get(constants.KEY_ATTR),
            value_name = attrs.get(constants.VALUE_ATTR, nil)
  
        self._strip_attr(
          element, constants.FOR_ATTR, 
          constants.KEY_ATTR, constants.VALUE_ATTR
        )
        var for_vars = variables.clone()
  
        var result = []
        for key, value in data {
          if value_name for_vars.set('${value_name}', value)
          if key_name for_vars.set('${key_name}', key)
          result.append(
            self._process(path, json.decode(json.encode(element)), for_vars)
          )
        }
        return result
      }
      
      if element and element.contains('children') and element.children {
        element.children = self._process(path, element.children, variables)
      }
  
      # replace attribute variables...
      if element and !is_list(element) {
        for attr in element.attributes {
          if attr.value {
            # replace variables: {var_name}
            attr.value = self._process(path, attr.value, variables)
          }
        }
      }
  
      return element
    }
  }

  /**
   * Set the template files root directory for [[Template.render]]. Returns `true` if 
   * the directory was automatically created (See [[Template._auto_init]]) or `false` 
   * if it wasn't.
   * 
   * If your template contains or will contain an `<include />` tag, the path given 
   * here will become the root of the include search path.
   * 
   * @param {string} path
   * @return bool
   */
  set_root(path) {
    if !is_string(path)
      die Exception('string expected in argument 1 (path)')
    
    var directory_created = false

    if !os.dir_exists(path) and self._auto_init {
      directory_created = os.create_dir(path)
    }

    self._root_dir = path
    return directory_created
  }

  /**
   * Sets the default file extension to be used when [[Template.render]] and/or the 
   * `<include />` tag searches for template files in the root directory when the path 
   * given does not match an existing file and does not end with another extension.
   * 
   * @param {string} ext
   */
  set_extension(ext) {
    if !is_string(ext)
      die Exception('string expected at argument 1 (ext)')
    if !ext.starts_with('.')
      die Exception('invalid extension name')
    self._file_ext = ext
  }

  /**
   * Registers a function that can be used to process variables in the template. 
   * The given function must accept a minimum of one argument which will recieve 
   * the value of the value to be processed.
   * 
   * ##### Example
   * 
   * ```blade
   * def firstname_function(value) {
   *   return value.split(' ')[0]
   * }
   * 
   * tpl.register_function('firstname', firstname_function)
   * ```
   * 
   * The registered function can be used in the template to process variables.
   * For example,
   * 
   * ```html
   * <div>{{ my_user|firstname }}</div>
   * ```
   * 
   * @param {string} name
   * @param {function} function
   */
  register_function(name, function) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')
    if !is_function(function)
      die Exception('function expected in argument 1 (function)')
    if reflect.get_function_metadata(element).arity == 1
      die Exception('invalid template function')
    
    self._functions.set(name, function)
  }

  /**
   * Registers a custom HTML element for the template. The function passed must 
   * take exactly two (2) arguments the first of which will recieve the the 
   * template object iteself and the second the HTML as an object of {{html}}.
   * 
   * ##### Example
   * 
   * ```blade
   * def inline_input(wire, value) {
   *   return ...
   * }
   * 
   * tpl.register_element('inline-input', my_custom_function)
   * ```
   * 
   * The above registered element can then be used in the template. For example,
   * 
   * ```html
   * <inline-input value="{{ my_var }}" />
   * ```
   * 
   * @param {string} name
   * @param {function(2)} element
   */
  register_element(name, element) {
    if !is_string(name)
      die Exception('string expected in argument 1 (name)')
    if !is_function(element)
      die Exception('function expected in argument 1 (function)')
    if reflect.get_function_metadata(element).arity != 2
      die Exception('invalid function argument count for template element: ${name}')

    self._elements.set(name, element)
  }

  /**
   * Process and render template contained in the given string. The variables 
   * dictionary is used to pass variable data to the template being processed. 
   * 
   * If a variable is required in the template and is missing in the variables 
   * dictionary or the variables dictionary was not passed to the `render_string()` 
   * call, the process dies with an Exception. The third argument allows specifying 
   * the source file/path of the template being processed and will default to 
   * `<source>` when not passed.
   * 
   * The path argument may be of important if the string was read from a file or a 
   * similar source to provide information on the source of wrong template data such as 
   * line and column information.
   * 
   * ##### Example
   * 
   * ```blade
   * tpl.render_string('<div>{{ name }}</div>', {name: 'Johnson'})
   * ```
   * 
   * The above template should return
   * 
   * ```html
   * <div>Johnson</div>
   * ```
   * 
   * @param {string} source
   * @param {dict?} variables
   * @param {string?} path
   * @return string
   */
  render_string(source, variables, path) {
    if !is_string(source)
      die Exception('template template expected')
  
    if variables != nil and !is_dict(variables)
      die Exception('variables must be passed to render_string() as a dictionary')
    if variables == nil variables = {}

    if !path path = '<source>'
    else path = to_string(path)
  
    return html.encode(
      self._process(
        path,
        html.decode(self._strip(source), {with_position: true}),
        variables
      )
    ).trim()
  }

  /**
   * Process and render template contained in the given template file. The template 
   * path should be a path relative to the root directory (See [[Template]]) and may 
   * or not carry any extension. If the template file uses the template _extension_ 
   * (defualt: `.html`), the path argument may exlcude the extension from the path 
   * altogether provided there is a file with a matching name that may or not have the 
   * default extension (See [[Template.set_extension]]). 
   * 
   * The variables dictionary is used to pass variable data to the template being 
   * processed and behaves exactly the same way as with [[Template.render_string]].
   * 
   * ##### Example
   * 
   * ```blade
   * tpl.render('my_template')
   * ```
   * 
   * The above example renders the template as is and will die if any variable is found in it. 
   * You can pass a variable the same way you do with [[Template.render_string]].
   * 
   * @param {string} path
   * @param {dict?} variables
   * @return string
   */
  render(path, variables) {
    if !is_string(path)
      die Exception('template path expected')

    # confirm/auto create root directory as configured
    if !os.dir_exists(self._root_dir) {
      if !_auto_init 
        die Exception('templates directory "${self._root_Dir}" not found.')
      else os.create_dir(self._root_dir)
    }
  
    var template_path = os.join_paths(self._root_dir, path)
    if !file(template_path).exists() {
      if !template_path.match(constants.EXT_RE) 
        template_path += self._file_ext
    }
  
    if variables != nil and !is_dict(variables)
      die Exception('variables must be passed to render() as a dictionary')
    if variables == nil variables = {}
  
    var template_file = file(template_path)
    if template_file.exists() {
      return self.render_string(template_file.read(), variables, template_path)
    }
  
    die Exception('template "${path}" not found')
  }
}


/**
 * Default function exporting the [[Template]] class that allows function 
 * initialization. See [[Template]].
 * 
 * @param {bool} auto_init
 */
def template(auto_init) {
  return Template(auto_init)
}

