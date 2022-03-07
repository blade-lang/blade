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
 * value type boolean (accepts `1` and `0` as well as `true` and 
 * `false` as valid values).
 */
var BOOL = 3

/**
 * value type string
 */
var STRING = 4

/**
 * value type enumeration choices.
 */
var CHOICE = 5

var _type_name = {
  0: '',
  1: 'number',
  2: 'number',
  3: 'boolean',
  4: 'value',
  5: 'choice',
}

def _muted_text(text) {
  return colors.text(colors.text(text, colors.text_color.dark_grey), colors.styles.italic)
}

def _get_real_value(type, value) {
  if type == INT return to_int(to_number(value))
  else if type == NUMBER return to_number(value)
  else if type == BOOL return value == 'true' or value == '1'
  else if type == STRING return value
  return nil
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
  _Option(long_name, help, short_name, type, required) {
    self.long_name = long_name
    self.help = help ? help : ''
    self.short_name = short_name
    self.type = type ? type : NONE
    self.required = required

    if type < 0 or type > 5
      die ArgsException('invalid value type')
  }
}

class _Optionable {
  var options = []
  
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
        required = opts.get('required', false)

    if short_name != nil and !is_string(short_name)
      die ArgsException('short_name must be string')
    if required != nil and !is_bool(required)
      die ArgsException('required must be boolean')

    self.options.append(_Option(name, help, short_name, type, required))
  }
}

class _Command < _Optionable {
  _Command(name, help, type, action) {
    if !is_string(name)
      die ArgsException('name expected')
    if help != nil and !is_string(help)
      die ArgsException('help message must be string')
    if action != nil and !is_function(action)
      die ArgsException('action must be of type function(options: dict)')

    self.name = name
    self.help = help
    self.type = type
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
   * Parser(name: string)
   * @param name refers to the name of the cli program.
   * @constructor
   */
  Parser(name) {
    if !is_string(name)
      die Exception('missing program name')

    self.name = name
    self._command = nil

    # Initalize the automatic help option.
    self.add_option(
      'help', 
      'Show this help message and exit', 
      {
        short_name: 'h',
        required: false,
      }
    )
  }

  _get_option(name) {
    for opt in self.options {
      if '--${opt.long_name}' == name or '-${opt.short_name}' == name
        return opt
    }
    return nil
  }

  _get_command(name) {
    for command in self.commands {
      if command.name == name
        return command
    }
    return nil
  }

  _get_help(help) {
    return help or _muted_text('<no help message>')
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

      echo 'Usage: ${self.name} ' + 
        (flags_hint ? '[ ${flags_hint} ]' : '') + 
        (self.commands.length() > 0 ? ' [COMMAND]' : '')
    } else {
      echo 'Usage: ${self.name} ${command.name}' + 
          (command.type != NONE ? ' <${_type_name[command.type]}>' : '')
    }
  }

  _command_error(name, message) {
    io.stderr.write(colors.text('error: ${message}\n', colors.text_color.red))
    echo ''
    self._help_action(name)
    os.exit(1)
  }

  _option_error(name, message) {
    io.stderr.write(colors.text('error: ${message}\n', colors.text_color.red))
    os.exit(1)
  }

  _print_help() {
    var options_width = self._get_options_text_width(),
        commands_width = self._get_commands_text_width(),
        width = options_width > commands_width ? options_width : commands_width
    
    echo ''
    echo 'OPTIONS:'
    for opt in self.options {
      var line = ''
      if opt.short_name {
        line += '-${opt.short_name},'.lpad(opt.short_name.length() + 4) + ' --${opt.long_name}'
      } else {
        line += '--${opt.long_name}'.lpad(opt.long_name.length() + 8)
      }

      if opt.type != NONE
        line += ' <' + _type_name[opt.type] + '>'

      # We want to separate the longtest option names at least 12
      # characters away from the help texts.
      line = line.rpad(width + 12)

      echo line + self._get_help(opt.help)
    }
    echo ''
    echo 'COMMANDS:'
    for opt in self.commands {
      var line = opt.name.lpad(opt.name.length() + 2)

      if opt.type != NONE
        line += ' <' + _type_name[opt.type] + '>'

      # We want to separate the longtest option names at least 12
      # characters away from the help texts.
      line = line.rpad(width + 12)

      echo line + self._get_help(opt.help)
    }
  }

  # This method should ever be called directly.
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
        echo ''
        echo '  ${self._get_help(command.help)}'
        echo ''
      } else {
        self._usage_hint(command)
        self._print_help()
      }
    }
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
   * - `required`: tells the parser if a value is compulsory for this option.
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
   * - The action property must be a function.
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
        action = opts.get('action')

    self.commands.append(_Command(name, help, type, action))
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
    }

    iter var i = 0; i < cli_args.length(); i++ {
      var arg = cli_args[i]
      var command_found = false

      # Commands can only occur in the first index of the argument list. 
      # Every other occurrence will be treated as a value.
      # if i == 0 {
        # Then treat commands.
        var command = self._get_command(arg)
        if command {
          self._command = command.name
          if command.type != NONE {
            if i < cli_args.length() - 1 {
              i++
              var value = cli_args[i]
              parsed_args.command = {
                name: command.name,
                value: _get_real_value(command.type, value)
              }
            } else {
              self._command_error(command.name, 'Command "${command.name}" expects a ${_type_name[command.type]}')
            }
          } else {
            parsed_args.command = {
              name: command.name,
              value: nil
            }
          }
          command_found = true
        }
      # }

      if !command_found {
        # Treat options next.
        var option = self._get_option(arg)
        if option {

          # ...

          # We only automatically trigger actions for options during parsing 
          # if the option is the very first item in the argument list and == 'help'.
          # This is because this action is library bound and are meant to be triggered
          # automatically.
          if option.long_name == 'help' {
            self._help_action(i < cli_args.length() - 1 ? cli_args[i + 1] : nil)
          } else if option.type != NONE {
            if i < cli_args.length() - 1 {
              i++
              var value = cli_args[i]
              parsed_args.options.extend({
                '${option.long_name}': _get_real_value(option.type, value)
              })
            } else {
              self._option_error(option.long_name, 'Option "${option.long_name}" expects a ${_type_name[option.type]}')
            }
          } else {
            parsed_args.options.extend({
              '${option.long_name}': true
            })
          }
        } else {
          if arg.starts_with('-') or i != 0 {
            self._option_error(arg, 'Unknown argument: ${arg}')
          } else {
            self._command_error(arg, 'Unknown command: ${arg}')
          }
        }
      }
    }
    return parsed_args
  }
}

