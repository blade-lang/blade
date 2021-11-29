#
# @module ast
#
# Provides interface for parse Blade code into Abstract Syntax Trees
# @ copyright 2021, Ore Richard Muyiwa and Blade contributors
#

import .scanner { Scanner }
import .parser { Parser, ParseException }
import .expr { * }
import .stmt { * }
import .decl { * }
import .token { * }


/**
 * parse(source: string)
 * 
 * parses a given source code and outputs Blade AST objects.
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

