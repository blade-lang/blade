#
# @module json
#
# provides APIs for encoding and decoding json data
# @copyright 2021, Ore Richard Muyiwa
#

import .encoder { * }
import .decoder { * }


/**
 * encode(value: any [, max_depth: number = 1024])
 * 
 * JSON encodes the given value with a recursive depth up to
 * @max_depth. If max_depth is not given, max_depth = 1024.
 */
def encode(value, max_depth) {
  return Encoder(max_depth).encode(value)
}