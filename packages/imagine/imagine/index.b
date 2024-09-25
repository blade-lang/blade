/**
 * @module imagine
 * 
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */


import .fonts { * }
import .comparisons { * }
import .interpolations { * }
import .quants { * }
import .crops { * }
import .arcs { * }
import .colors { * }
import .image { * }


/**
 * Compose a truecolor value from its components.
 * 
 *  @param {number?} r - The red channel (0-255) - Default: 0
 *  @param {number?} g - The green channel (0-255) - Default: 0
 *  @param {number?} b - The blue channel (0-255) - Default: 0
 *  @param {number?} a - The alpha channel (0-127, where 127 is 
 *      fully transparent, and 0 is completely opaque) 
 *      - Default: 0.
 * @returns {number}
 */
def true_color(r, g, b, a) {
  if r == nil r = 0
  if g == nil g = 0
  if b == nil b = 0
  if a == nil a = 0

  if !is_number(r) or !is_number(g) or !is_number(b) or !is_number(a) {
    die Exception('number expected')
  }

  if a == 0 {
    return (a << 24) + (r << 16) + (g << 8) + b
  } else {
    return (r << 16) + (g << 8) + b
  }
}


/**
 * Decomposes an Image true color number into it's respective 
 * RGBA components.
 * 
 * The function returns a dictionary that contains the following 
 * decomposed items:
 * 
 * - `r` - The red channel value
 * - `g` - The green channel value
 * - `b` - The blue channel value
 * - `a` - The alpha channel value
 * 
 * @param {number} color
 * @returns {dict}
 */
def decompose(color) {
  var r = (c & 0xFF0000) >> 16
  var g = (c & 0x00FF00) >> 8
  var b = (c & 0x0000FF)
  var a = (c & 0x7F000000) >> 24

  return { r, g, b, a}
}
