/**
 * @module ast
 *
 * Provides interface for parsing Blade code into Abstract Syntax Trees.
 * 
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

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
 * Parses a given source code and outputs Blade AST objects.
 * 
 * @param string source
 * @param string? path
 * @returns ParseResult
 */
def parse(source, path) {
  if !is_string(source)
    die Exception('string expected in argument 1 (source)')
  if path != nil and !is_string(path)
    die Exception('string expected in argument 2 (path)') 

  # scan the source...
  var scanner = Scanner(source, path)
  var tokens = scanner.scan()
  
  # parse the scanned tokens
  var parser = Parser(tokens, path)
  return parser.parse()
}

/**
 * Parses the give source code and outputs a JSON representation of 
 * it's AST structure.
 * 
 * @param string source
 * @param string? path
 * @returns string
 */
def json(source, path) {
  # return Exception('not yet implemented')
  return js.Encoder(true, 0).encode(parse(source, path))
}