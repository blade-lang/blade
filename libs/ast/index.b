#
# @module ast
#
# Provides interface for parse Blade code into ASTs
# @copyright 2021, Ore Richard Muyiwa
#

import .scanner { Scanner }


def parse(source) {
  if !is_string(source)
    die Exception('source code expected')

  # do parsing here...
  var scanner = Scanner(source)
  var tokens = scanner.scan()
  
  for token in tokens {
    echo to_string(token)
  }
}

