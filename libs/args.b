#
# @module args
#
# This module provides functionalities that makes writing command-line 
# interfaces easy. A user can define the options and commands available 
# in a program and this module can automatically figure out how to parse 
# those options and commands out of the CLI arguments. It also provides 
# automatic help and usage messages as well as error/warnings generation 
# for valid/invalid arguments.
# 
# ### Example
# 
# The below is a simple program that shows a typical use of the module.
# 
# ```blade
# import args
# 
# var parser = args.Parser('myprogram')
# parser.add_option('name', 'The name of person to call', {type: args.STRING})
# parser.add_command('call', 'Make a phone call')
# parser.parse()
# ```
# 
# We can simply print help information for the above program if it were saved 
# in a file `myprogram.b` as follows.
# 
# ```sh
# $ blade myprogram.b -h 
# Usage: myprogram [ [-h] | [--name NAME] ] [COMMAND]
# 
# OPTIONS:
#   -h, --help                 Show this help message and exit
#       --name <value>         The name of person to call
# 
# COMMANDS:
#   call                       Make a phone call
# ```
# 
# if we change the last line of the program to `echo parser.parse()` so that we 
# can see the result of the parsing, the following CLI call will yield the given result.
# 
# ```terminal
# $ blade myprogram.b --name 25
# {options: {name: 25}, command: nil}
# 
# $ blade myprogram.b call  
# {options: {}, command: {name: call, value: nil}}
# 
# $ blade myprogram.b call --name 25
# {options: {name: 25}, command: {name: call, value: nil}}
# ```
# 
# Calling name without an option will yield the following result/error.
# 
# ```sh
# $ blade myprogram.b --name   
# error: Option "name" expects a value
# ```
# 
# You may even get help on a command directly like below:
# 
# ```sh
# $ blade myprogram.b --help call
# Usage: myprogram call
# 
#   Make a phone call
# ```
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
#

import os
import colors
import io

/**
 * value type none
 */
var NONE = 0

/**
 * value type integer (allows numbers, but floors them to integers)
 */
var INT = 1

/**
 * value type number
 */
var NUMBER = 2

/**
 * value type boolean (accepts `1` and `0` as well as `true` 
 * and `false` as valid values).
 */
var BOOL = 3

/**
 * value type string
 */
var STRING = 4

/**
 * value type for list
 */
var LIST = 5

/**
 * value type enumeration choices.
 */
var CHOICE = 6

# /**
#  * value type optional.
#  */
var OPTIONAL = 7

var _type_name = {
  0: '',
  1: 'integer',
  2: 'number',
  3: 'boolean',
  4: 'value',
  5: 'list',
  6: 'choice',
  7: 'value',
}

def _muted_text(text) {
  return colors.text(text, colors.text_color.dark_grey)
}

def _bold_text(text) {
  return colors.text(colors.text(text), colors.style.bold)
}

def _main_headings(text) {
  return colors.text(colors.text(text, colors.text_color.green), colors.style.bold)
}

def _cyan_text(text) {
  return colors.text(colors.text(text, colors.text_color.cyan), colors.style.bold)
}

def _get_real_value(item, value) {
  if item.type == INT return to_int(to_number(value))
  else if item.type == NUMBER return to_number(value)
  else if item.type == BOOL return is_bool(value) ? value : (value == 'true' or value == '1')
  else if item.type == STRING return to_string(value)
  else if item.type == LIST return is_list(value) ? value : [value]
  else if item.type == CHOICE {
    if is_list(item.choices)
      return item.choices.contains(value) ? value : value
    else return item.choices.contains(value) ? item.choices[value] : value
  }
  return value
}

/**
 * Commandline argument exception.
 */
class ArgsException < Exception {
  /**
   * ArgsException(message: string)
   * @constructor
   */
  ArgsException(message) {
    parent(message)
  }
}

class _Option {
  _Option(long_name, help, short_name, type, value, choices) {
    self.long_name = long_name
    self.help = help ? help : ''
    self.short_name = short_name
    self.type = type ? type : NONE
    self.value = value
    self.choices = choices
    self.options = nil # required for _Option and subclasses

    if type < NONE or type > OPTIONAL
      die ArgsException('invalid value type')
  }
}

class _Optionable {
  var options = []
  var choices = []
  
  add_option(name, help, opts) {
    if !is_string(name)
      die ArgsException('name expected')
    if help != nil and !is_string(help)
      die ArgsException('help message must be string')
    if opts == nil opts = {}
    else if !is_dict(opts)
      die ArgsException('opts must be a dict')

    # Ensure we don't have duplicated option declarations.
    for o in self.options {
      if o.long_name == name or o.short_name == name
        die ArgsException('option with name "${name}" previously declared')
    }

    var short_name = opts.get('short_name'),
        type = to_int(to_number(opts.get('type', NONE))),
        value = opts.get('value', nil),
        choices = opts.get('choices', [])

    if short_name != nil and !is_string(short_name)
      die ArgsException('short_name must be string')
    if !is_list(choices) and !is_dict(choices)
      die ArgsException('choices must be a list or dictionary')

    self.options.append(_Option(name, help, short_name, type, value, choices))

    if instance_of(self, _Command)
      return self
  }
}

class _Command < _Optionable {
  _Command(name, help, type, action, choices) {
    if !is_string(name)
      die ArgsException('name expected')
    if help != nil and !is_string(help)
      die ArgsException('help message must be string')
    if action != nil and !is_function(action)
      die ArgsException('action must be of type function(options: dict)')
    if choices != nil and !is_list(choices) and !is_dict(choices)
      die ArgsException('choices must be of type list')

    self.name = name
    self.help = help
    self.type = type
    self.choices = choices or []
  }
}

class _Positional < _Optionable {
  _Positional(name, help, type, choices, value) {
    if !is_string(name)
      die ArgsException('name expected')
    if help != nil and !is_string(help)
      die ArgsException('help message must be string')
    if choices != nil and !is_list(choices) and !is_dict(choices)
      die ArgsException('choices must be of type list')

    self.name = name
    self.help = help
    self.type = type
    self.choices = choices or []
    self.value = value
  }
}

/**
 * A configurable commandline parser.
 */
class Parser < _Optionable {

  /**
   * A list of commands supported by the parser.
   */
  var commands = []

  /**
   * A list of positional values supported by the parser.
   */
  var indexes = []

  var _default_help = true

  /**
   * Parser(name: string [, default_help: bool = true])
   * @param `name` refers to the name of the cli program.
   * @param `default_help` whether to show help when no command or option is matched or not.
   * @constructor
   */
  Parser(name, default_help) {
    if !is_string(name)
      die Exception('missing program name')
    if default_help != nil and !is_bool(default_help)
      die Exception('bool expected in argument 2 (default_help)')
    if default_help == nil default_help = true

    self._default_help = default_help

    self.name = name
    self._command = nil

    # Initalize the automatic help option.
    self.add_option(
      'help', 
      'Show this help message and exit', 
      {
        short_name: 'h',
      }
    )
  }

  _get_option(source, name) {
    var result = []
    for opt in source {
      if '--${opt.long_name}' == name {
        result.append(opt)
      } else if name.starts_with('-') and name.length() > 1 {
        if name[1] != '-' {  # we are not matching -- here...
          for n in name {
            if opt.short_name == n
              result.append(opt)
          }
        }
      }
    }
    return result
  }

  _get_command(name) {
    for command in self.commands {
      if command.name == name
        return command
    }
    return nil
  }

  _get_help(opt) {
    var response = opt.help or _muted_text('<no help message>')
    if opt.options {
      var options_width = self._get_options_text_width(),
        commands_width = self._get_commands_text_width(),
        width = options_width > commands_width ? options_width : commands_width
      
      for op in opt.options {

        var line = '\n'
        if op.short_name {
          line += '-${op.short_name},'.lpad(op.short_name.length() + 6) + ' --${op.long_name}'
        } else {
          line += '--${op.long_name}'.lpad(op.long_name.length() + 6)
        }

        line += self._opt_line(op)

        # We want to separate the longtest option names at least 12
        # characters away from the help texts.
        line = line.rpad(width + 8)



        if opt.type == CHOICE {
          response += '\n' + self._get_choice_help(opt.choices)
        }

        response += _muted_text(line + self._get_help(op))
      }
    }
    return response
  }

  _get_choice_help(opt) {
    var options_width = self._get_options_text_width(),
      commands_width = self._get_commands_text_width(),
      width = options_width > commands_width ? options_width : commands_width

    if is_dict(opt) {
      var response = !opt ? _muted_text('<no help message>') : ''
      for k, v in opt {
        var line = '${k}'.lpad(k.length() + 4)

        # We want to separate the longtest option names at least 12
        # characters away from the help texts.
        line = colors.text(line, colors.style.italic).rpad(width + 17)

        response += line + v + '\n'
      }

      return response.rtrim('\n')
    } else {
      return _muted_text(('[' + ', '.join(opt) + ']').lpad(self._get_commands_text_width() + 6))
    }
  }

  _get_hint_line(opt) {
    var line

    if opt.short_name line += '-${opt.short_name}'
    else line += '--${opt.long_name}'

    if opt.type != NONE
      line += ' ${opt.long_name.upper()}'
    return line
  }

  _get_flags_hint() {
    var list = []
    for opt in self.options {
      list.append('[${self._get_hint_line(opt)}]')
    }
    return ' | '.join(list)
  }

  _get_index_hint() {
    var list = []
    for index in self.indexes {
      list.append('[${index.name}]')
    }
    return ' '.join(list)
  }

  _get_options_text_width() {
    var width = 0
    for opt in self.options {
      var n
      if opt.short_name {
        n = opt.short_name.length() + opt.long_name.length() + 2 # +2 for ', '
      } else {
        n = opt.long_name.length() + 4 # 4 should cover all short names.
      }

      if opt.type != NONE
        n += opt.long_name.length() + _type_name[opt.type].length()

      if n > width width = n
    }
    return width
  }

  _get_commands_text_width() {
    var width = 0
    for opt in self.commands {
      var n = opt.name.length()

      if opt.type != NONE
        n += opt.name.length() + _type_name[opt.type].length()

      if n > width width = n
    }
    return width
  }

  _usage_hint(command) {
    if !command {
      var flags_hint = self._get_flags_hint()
      var index_hint = self._get_index_hint()

      echo _main_headings('Usage:') + _cyan_text(' ${self.name} ' + 
        (flags_hint ? '[ ${flags_hint} ]' : '') + 
        (self.commands.length() > 0 ? ' [COMMAND]' : '') + 
        (index_hint ? ' ${index_hint}' : ''))
    } else {
      echo _main_headings('Usage:') + _cyan_text(' ${self.name} ${command.name}' + 
          (command.type != NONE ? ' <${_type_name[command.type]}>' : ''))
    }
  }

  _command_error(name, message) {
    io.stderr.write(colors.text('error: ${message}\n', colors.text_color.red))
    self._help_action(name)
    os.exit(1)
  }

  _option_error(name, message) {
    io.stderr.write(colors.text('error: ${message}\n', colors.text_color.red))
    self._print_help()
    os.exit(1)
  }

  _opt_line(opt) {
    if opt.type != NONE {
      return " <" + _type_name[opt.type] + '>'
    }
    return ' '
  }

  _print_help() {
    var options_width = self._get_options_text_width(),
        commands_width = self._get_commands_text_width(),
        width = options_width > commands_width ? options_width : commands_width
    
    if self.indexes {
      echo ''
      echo _main_headings('POSITIONAL ARGUMENTS:')
      for index in self.indexes {
        var line = '  ' + _bold_text(index.name)

        line += self._opt_line(index)

        # We want to separate the longtest option names at least 12
        # characters away from the help texts.
        line = line.rpad(width + 20)

        echo line + self._get_help(index)
      }
    }
    if self.options {
      echo ''
      echo _main_headings('OPTIONS:')
      for opt in self.options {
        var line = ''
        if opt.short_name {
          line += '-${opt.short_name},'.lpad(opt.short_name.length() + 4) + ' --${opt.long_name}'
        } else {
          line += '--${opt.long_name}'.lpad(opt.long_name.length() + 8)
        }

        line += self._opt_line(opt)

        # We want to separate the longtest option names at least 12
        # characters away from the help texts.
        line = line.rpad(width + 5)

        echo line + self._get_help(opt)
      }
    }
    if self.commands {
      echo ''
      echo _main_headings('COMMANDS:')
      for cmd in self.commands {
        var line = '  ' + _bold_text(cmd.name)

        line += self._opt_line(cmd)

        # We want to separate the longtest option names at least 12
        # characters away from the help texts.
        line = line.rpad(width + 20)

        echo line + self._get_help(cmd)
      }
    }
  }

  # This method should never be called directly.
  _help_action(command) {
    command = command or self._command
    var original_command = command

    if !command {
      self._usage_hint(command)
      self._print_help()
    } else {
      command = self._get_command(command)
      if command {
        self._usage_hint(command)
        echo '  ${self._get_help(command)}'
      } else {
        self._usage_hint(command)
        self._print_help()
      }
    }
    os.exit(0)
  }

  /**
   * add_option(name: string [, help: string [, opts: dict]])
   * 
   * adds a support for a new command to the parser.
   * 
   * The `opts` dictionary can contain one or more of:
   * 
   * - `short_name`: A shorter version of the option name parsed via 
   * single hyphens (`-`) without the hyphen. For example, short_name `v` 
   * will match `-v` in the commandline.
   * - `type`: type must be one of the args types and will indicate 
   * how the parsed data should be interpreted in the final result.
   * - `value`: tells the parser the default value for this option.
   * - `choices`: a list of allowed options or a dictionary of allowed 
   * options with their respective descriptions.
   * 
   * @note the `choices` option only works for type `CHOICE`.
   */
  add_option(name, help, opts) {
    parent.add_option(name, help, opts)
  }

  /**
   * add_command(name: string [, help: string [, opts: dict]])
   * 
   * adds a support for a new command to the parser.
   * 
   * The `opts` dictionary can contain property `type` and `action`.
   * 
   * - The `type` property a must be one of the args types and will indicate 
   * how the parsed data should be interpreted in the final result.
   * - The `action` property must be a function.
   * 
   * The `opts` dictionary can contain one or more of:
   * 
   * - `type`: type must be one of the args types and will indicate 
   * how the parsed data should be interpreted in the final result.
   * - `choices`: a list of allowed options or a dictionary of allowed 
   * options with their respective descriptions.
   * 
   * @note the `choices` option only works for type `CHOICE`.
   */
  add_command(name, help, opts) {
    if !is_string(name)
      die ArgsException('name expected')
    if help != nil and !is_string(help)
      die ArgsException('help message must be string')
    if opts == nil opts = {}
    else if !is_dict(opts)
      die ArgsException('opts must be a dict')

    # Ensure we don't have duplicated option declarations.
    for o in self.commands {
      if o.name == name
        die ArgsException('option with name "${name}" previously declared')
    }

    var type = to_int(to_number(opts.get('type', NONE))),
        action = opts.get('action'),
        choices = opts.get('choices', [])

    var command = _Command(name, help, type, action, choices)
    self.commands.append(command)
    return command
  }

  /**
   * add_index(name: string [, help: string [, opts: dict]])
   * 
   * adds a support for a new positional argument to the parser.
   * 
   * The `opts` dictionary can contain property `type` and `action`.
   * 
   * - The `type` property a must be one of the args types and will indicate 
   * how the parsed data should be interpreted in the final result.
   * 
   * The `opts` dictionary can contain one or more of:
   * 
   * - `type`: type must be one of the args types and will indicate 
   * how the parsed data should be interpreted in the final result.
   * - `value`: tells the parser the default value for this index.
   * - `choices`: a list of allowed options or a dictionary of allowed 
   * values with their respective descriptions.
   * 
   * @note the `choices` option only works for type `CHOICE`.
   */
  add_index(name, help, opts) {
    if !is_string(name)
      die ArgsException('name expected')
    if help != nil and !is_string(help)
      die ArgsException('help message must be string')
    if opts == nil opts = {}
    else if !is_dict(opts)
      die ArgsException('opts must be a dict')

    var type = to_int(to_number(opts.get('type', NONE))),
      choices = opts.get('choices', []),
      value = opts.get('value', nil)

    self.indexes.append(_Positional(name, help, type, choices, value))
  }

  /**
   * parse()
   * 
   * Parses the commandline arguments and returns a dictionary of command 
   * and options.
   * 
   * For example, parsing the commandline
   * `blade test.b install 5 --verbose` may yeild such a result as 
   * `{options: {verbose: true}, command: {name: install, value: 5}}`.
   * 
   * @return dict
   */
  parse() {
    # We have to strip out the application name and the script path.
    var cli_args = os.args[2,]
    var parsed_args = {
      options: {},
      command: nil,
      indexes: []
    }
    var help_shown = false

    var index_start = -1
    iter var i = 0; i < cli_args.length(); i++ {
      var arg = cli_args[i]
      var command_found = false

      # Commands can only occur in the first index of the argument list. 
      # Every other occurrence will be treated as a value.
      var command = self._get_command(arg)

      def parse_options(source, arg, fail) {
        var options = self._get_option(source, arg)
        if options and (options.length() == arg.length() - 1 or arg.starts_with('-')) {
          i++

          # ...
          for option in options {
            # We only automatically trigger actions for options during parsing 
            # if the option is the very first item in the argument list and == 'help'.
            # This is because this action is library bound and are meant to be triggered
            # automatically.
            if option.long_name == 'help' and !command_found {
              help_shown = true
              self._help_action(i < cli_args.length() - 1 ? cli_args[i + 1] : nil)
            } else if option.type == OPTIONAL {
              if i < cli_args.length() {
                i++
                if i < cli_args.length() {
                  var value = cli_args[i]
                  parsed_args.options.set(
                    '${option.long_name}', 
                    _get_real_value(option, value)
                  )
                  continue
                }
                
                self._option_error(option.long_name, 'Option "${option.long_name}" expects a ${_type_name[option.type]}')
              }
            } else if option.type != NONE {
              if i < cli_args.length() {
                var value = cli_args[i]
                var v = _get_real_value(option, value)
                if v {
                  parsed_args.options.set('${option.long_name}', v)
                  continue
                }
              }

              var msg = 'Option "${option.long_name}" expects a ${_type_name[option.type]}'
              if option.type == CHOICE and option.choices 
                msg += " as one of \'${"', '".join(option.choices)}\'"
              self._option_error(option.long_name, msg)
            } else {
              parsed_args.options.set('${option.long_name}', true)
            }
          }
        } else if options and (arg.length() - 1 != options.length() or !arg.starts_with('-')) {
          echo arg.length()
          echo options.length()
          if command_found {
            self._command_error(command.name, 'Unsupported argument encountered at ${arg}')
          } else {
            self._option_error(arg, 'Unsupported argument: ${arg}')
          }
        } else if (fail or arg.starts_with('-')) and !self.indexes {
          if command_found {
            self._command_error(command.name, 'Unknown argument ${arg} for ${command.name}')
          } else {
            self._option_error(arg, 'Unknown argument: ${arg}')
          }
        }
      }

      if command {
        command_found = true
        self._command = command.name

        # If options exist, we must parse them here... before 
        # we parse value.
        while i < cli_args.length() - 1 and cli_args[i + 1].starts_with('-') {
          parse_options(command.options, cli_args[i + 1])
        }

        if command.type != NONE or command.type == OPTIONAL {
          if i < cli_args.length() - 1 {
            i++
            var value = cli_args[i]
            var v = _get_real_value(command, value)

            if command.type != CHOICE or !command.choices or command.choices.contains(v) {
              parsed_args.command = {
                name: command.name,
                value: v
              }

              if command.type == LIST {
                while i < cli_args.length() - 1 {
                  i++
                  parsed_args.command.value.append(cli_args[i])
                }
              }
            } else {
              self._command_error(command.name, 'Command "${command.name}" expects one of \'${"', '".join(command.choices)}\' as argument')
            }
          } else if command.type != OPTIONAL {
            self._command_error(command.name, 'Command "${command.name}" expects a ${_type_name[command.type]}')
          }
        } else {
          parsed_args.command = {
            name: command.name,
            value: nil
          }
        }
      }

      if !command_found {
        # Treat options next.
        parse_options(self.options, arg, true)
      }

      # positional arguments can only come after command and options.
      if self.indexes and i < cli_args.length() - 1 {
        # if index has never been parsed, let's mark the
        # index starting positing in args list now.
        if index_start == -1 index_start = i

        var index_pos = i - index_start
        if index_pos < self.indexes.length() {
          parsed_args.indexes.append(_get_real_value(self.indexes[index_pos], arg))
        }
      }
    }

    # fill default values if missing
    for opt in self.options {
      if opt.value and !parsed_args.options.contains(opt.long_name) {
        parsed_args.options.add(opt.long_name, _get_real_value(opt, opt.value))
      }
    }
    iter var i = parsed_args.indexes.length(); i < self.indexes.length(); i++ {
      parsed_args.indexes.append(_get_real_value(self.indexes[i], self.indexes[i].value))
    }

    if !parsed_args.command and !parsed_args.options and !parsed_args.indexes and self._default_help and !help_shown {
      self._usage_hint()
      self._print_help()
    }

    return parsed_args
  }
}

