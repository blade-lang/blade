#
# @module ast
#
# Provides interface for parsing Blade code into Abstract Syntax Trees
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
#

import json as js

import .scanner { Scanner }
import .parser { Parser }
import .expr { * }
import .stmt { * }
import .decl { * }
import .defn { * }
import .exception { * }
import .token { * }


/**
 * parse(source: string)
 * 
 * parses a given source code and outputs Blade AST objects.
 * @return ParseResult
 */
def parse(source) {
  if !is_string(source)
    die Exception('source code expected')

  # scan the source...
  var scanner = Scanner(source)
  var tokens = scanner.scan()
  
  # parse the scanned tokens
  var parser = Parser(tokens)
  return parser.parse()
}

/**
 * json(source: string)
 * 
 * parses the give source code and outputs a JSON 
 * representation of it's AST structure.
 * @return string
 */
def json(source) {
  # return Exception('not yet implemented')
  return js.Encoder(true, 0).encode(parse(source))
}