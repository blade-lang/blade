/**
 * @module colors
 *
 * This module provides functionalities for color conversion and manipulation.
 *
 * This module also provides functionalities that enable cross-platform colored terminal outputs
 * that will allow you create beautiful console apps that are user friendly.
 *
 * RGB conversion to other colors that return a floating point or a list of floating points do so
 * to allow users get absolute precision since its really easy for callers to do a `math.round()`
 * on the components of the resulting list.
 *
 * ### Example
 *
 * The example below uses this module to create a success message that will print correctly
 * on almost all terminals (Only Windows 10 version 1901+ supported. All linux and OSX
 * terminals are supported). Try it out!
 *
 * ```blade
 * import colors
 * colors.text('Successful!', colors.text_color.green)
 * ```
 *
 * The `text()` function can be nested. For example,
 *
 * ```blade
 * colors.text(colors.text('Successful!', colors.style.bold), colors.text_color.green)
 * ```
 *
 * The module also features multiple functions for color conversion. For example,
 *
 * ```blade-repl
 * %> import colors
 * %> colors.rgb_to_cmyk(103, 13, 69)
 * [0, 87.37864077669903, 33.00970873786409, 59.6078431372549]
 * ```
 *
 * The terminal colors also have simple wrappers that allow supplied colors to `text()`
 * from various color formats. For example, we can specify the color from the HTML
 * hexadecimal color.
 *
 * ```blade
 * import colors
 * colors.text('Colored text!', colors.hex('#fc0'))
 * ```
 *
 * @copyright 2022, Ore Richard Muyiwa and Blade contributors
 */

import os
import math

# Since we'll be defining a `hex` function in this module, 
# we need to keep a local copy of the built-in `hex()` function 
# with another name for use later in the module.
# This works because as at the time of compilation, `hex` will 
# still be pointing to the built-in function until it is overwritten 
# by our later declaration.
var _hex = hex

/**
 * ANSI font styles available for console applications.
 * 
 * @type dictionary
 */
var style = {
  reset: 0,
  bold: 1,
  disable: 2,
  italic: 3,
  underline: 4,
  blink: 5,
  reverse: 7,
  invisible: 8,
  strike_through: 9,
}

/**
 * Standard ANSI text colors available for console applications.
 * 
 * @type dictionary
 */
var text_color = {
  black: 30,
  red: 31,
  green: 32,
  orange: 33,
  blue: 34,
  magenta: 35,
  cyan: 36,
  light_grey: 37,
  dark_grey: 90,
  light_red: 91,
  light_green: 92,
  yellow: 93,
  light_blue: 94,
  pink: 95,
  light_cyan: 96,
  white: 97,
}

/**
 * Standard ANSI background colors available for console applications.
 * 
 * @type dictionary
 */
var background = {
  black: 40,
  red: 41,
  green: 42,
  orange: 43,
  blue: 44,
  magenta: 45,
  cyan: 46,
  light_grey: 47,
  dark_grey: 100,
  light_red: 101,
  light_green: 102,
  yellow: 103,
  light_blue: 104,
  pink: 105,
  light_cyan: 106,
  white: 107,
}

def _get_sequence(v, bg) {
  if v or bg {
    if v v = to_string(v).lpad(2, '0')
    if bg bg = to_string(bg).lpad(2, '0')

    return '\x1b[${v ? v : ''}${bg ? ";${bg ? bg : ''}" : ''}m'
  }
  return nil
}

/**
 * Returns a terminal printable text with the given color (or style) and background if given.
 * 
 * @note The color argument can be replace with a style.
 * @param string value
 * @param int? color
 * @param int? bg
 * @return string
 */
def text(value, color, bg) {
  return _get_sequence(color, bg) + value + _get_sequence(0)
}

/**
 * Converts RGB color to ASI-256 color number.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @return number
 */
def rgb_to_ansi256(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('number expected')

  # From https://github.com/Qix-/color-convert/blob/3f0e0d4e92e235796ccb17f6e85c72094a651f49/conversions.js
  if r == g and g == b {
    if r < 8 return 16
    if r > 248 return 231
    return math.round(((r - 8) / 247) * 24) + 232
  }

  return 16 + (36 * math.round(r / 255 * 5)) +
      (6 * math.round(g / 255 * 5)) +
      math.round(b / 255 * 5)
}

/**
 * Converts ANSI-256 color number to ANSI-16 color number.
 * 
 * @param int code
 * @return number
 */
def ansi256_to_ansi(code) {
  if !is_int(code)
    die Exception('number expected')

  if code < 8 return 30 + code
  if code < 16 return 90 + (code - 8)

  var r, g, b

  if code >= 232 {
    r = (((code - 232) * 10) + 8) / 255
    g = r
    b = r
  } else {
    code -= 16

    var rem = code % 36

    r = math.floor(code / 36) / 5
    g = math.floor(rem / 6) / 5
    b = (rem % 6) / 5
  }

  var value = max(r, g, b) * 2
  if value == 0 return 30

  var result = 30 + ((math.round(b) << 2) | (math.round(g) << 1) | math.round(r))
  if result == 2 result += 60

  return result
}

/**
 * Converts the hexadecimal string _h_ to its RGBA component
 * 
 * @param string h
 * @return list
 */
def hex_to_rgb(h) {
  if !is_string(h)
    die Exception('string expected')

  var value = h.match('/^([a-f0-9]{8}|[a-f0-9]{6}|[a-f0-9]{4}|[a-f0-9]{3})$/')
  if !value {
    return [0, 0, 0, 0]
  }

  var v = value[0], rem

  # if we have four characters or less, duplicate every digit.
  if v.length() <= 4 {
    var tmp = ''
    for x in v {
      tmp += '${x}${x}'
    }
    v = tmp
  }

  # if we are still less than 8 digits, add two 'f's.
  v = v.lpad(8, 'f')

  var number = to_number('0x${v}')

  var b = number & 0xFF,
      g = (number & 0xFF00) >> 8,
      r = (number & 0xFF0000) >> 16,
      a = ((number & 0xFF000000) >> 24) / 255

  return [r, g, b, a]
}

/**
 * Converts the given hexadecimal color to its ANSI-256 number.
 * 
 * @param string color
 * @return number
 */
def hex_to_ansi256(color) {
  if !is_string(color)
    die Exception('string expected')

  var v = hex_to_rgb(color)
  return rgb_to_ansi256(v[0], v[1], v[2])
}

/**
 * Converts the given hexadecimal color to its ANSI-16 number.
 * 
 * @note For use with `text()`, this should be prefered over `hex_to_ansi256`
 * @param string color
 * @return number
 */
def hex_to_ansi(color) {
  if !is_string(color)
    die Exception('string expected')

  return ansi256_to_ansi(hex_to_ansi256(color))
}

/**
 * Converts the given hexadecimal color to its terminal compatible color.
 * 
 * @note For use with `text()`, this should be prefered over `hex_to_ansi256` and `hex_to_ansi`
 * @note _color_ can include the '#' character. E.g. `#ff0`.
 * @param string color
 * @return number
 */
def hex(color) {
  if !is_string(color)
    die Exception('string expected')

  if color[0] == '#' color = color[1,]
  return hex_to_ansi(color)
}

/**
 * Converts the given RGB color to its terminal compatible color.
 * 
 * @param number r
 * @param number g
 * @param number b
 * @return number
 */
def rgb(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('number expected')
  return ansi256_to_ansi(rgb_to_ansi256(r, g, b))
}

/**
 * Converts the given HSL color to its terminal compatible color.
 * 
 * @param number h
 * @param number s
 * @param number l
 * @return number
 */
def hsl(h, s, l) {
  if !is_number(h) or !is_number(s) or !is_number(l)
    die Exception('number expected')

  var rgb = hsl_to_rgb(h, s, l)

  return ansi256_to_ansi(rgb_to_ansi256(rgb[0], rgb[1], rgb[2]))
}

/**
 * Converts the given HSV color to its terminal compatible color.
 * 
 * @param number h
 * @param number s
 * @param number v
 * @return number
 */
def hsv(h, s, v) {
  if !is_number(h) or !is_number(s) or !is_number(v)
    die Exception('number expected')

  var rgb = hsv_to_rgb(h, s, v)

  return ansi256_to_ansi(rgb_to_ansi256(rgb[0], rgb[1], rgb[2]))
}

/**
 * Converts the given HWB color to its terminal compatible color.
 * 
 * @param number h
 * @param number w
 * @param number b
 * @return number
 */
def hwb(h, w, b) {
  if !is_number(h) or !is_number(w) or !is_number(b)
    die Exception('number expected')

  var rgb = hwb_to_rgb(h, w, b)

  return ansi256_to_ansi(rgb_to_ansi256(rgb[0], rgb[1], rgb[2]))
}

/**
 * Converts the given CMYK color to its terminal compatible color.
 * 
 * @param number c
 * @param number m
 * @param number y
 * @param number k
 * @return number
 */
def cmyk(c, m, y, k) {
  if !is_number(c) or !is_number(m) or !is_number(y) or !is_number(k)
    die Exception('number expected')

  var rgb = cmyk_to_rgb(c, m, y, k)

  return ansi256_to_ansi(rgb_to_ansi256(rgb[0], rgb[1], rgb[2]))
}

/**
 * Converts the given XYZ color to its terminal compatible color.
 * 
 * @param number x
 * @param number y
 * @param number z
 * @return number
 */
def xyz(x, y, z) {
  if !is_number(x) or !is_number(y) or !is_number(z)
    die Exception('number expected')

  var rgb = xyz_to_rgb(x, y, z)

  return ansi256_to_ansi(rgb_to_ansi256(rgb[0], rgb[1], rgb[2]))
}

/**
 * Converts a RGB components into its corresponding hexadecimal color.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @param int? a
 * @return string
 */
def rgb_to_hex(r, g, b, a) {
  if !is_int(r) or !is_int(g) or !is_int(b) or (a != nil and !is_int(a))
    die Exception('integer expected')

  return '${a ? _hex(a) : nil}${_hex(r)}${_hex(g)}${_hex(b)}'
}

/**
 * Converts a RGB color into its corresponding HSL components.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @return list[float]
 */
def rgb_to_hsl(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('integer expected')

  r /= 255
  g /= 255
  b /= 255

  var _min = min(r, g, b)
  var _max = max(r, g, b)
  var delta = _max - _min, h, s

  if delta == 0 h = 0
  else if r == _max h = (g - b) / delta
  else if g == _max h = 2 + (b - r) / delta
  else if b == _max h = 4 + (r - g) / delta

  h = min(h * 60, 360)
  if h < 0 h += 360

  var l = (_min + _max) / 2

  if delta == 0 s = 0
  # Selected implementation
  else s = delta / (1 - ((2 * l) - 1))
  # # Alternative implementation
  # else if l <= 0.5 s = delta / (_max + _min)
  # else s = delta / (2 - _max - _min)

  return [h, s * 100, l * 100]
}

/**
 * Converts a RGB color into its corresponding HSV components.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @return list[float]
 */
def rgb_to_hsv(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('integer expected')

  r /= 255
  g /= 255
  b /= 255

  var _max = max(r, g, b), _min = min(r, g, b)
  var delta = _max - _min, h, s

  if delta == 0 h = 0
  else if _max == r h = ((g - b) / delta) % 6
  else if _max == g h = 2 + (b - r) / delta
  else if _max == b h = 4 + (r - g) / delta

  h = min(h * 60, 360)
  if h < 0 h += 360

  if _max == 0 s = 0
  else s = delta / _max

  return [h, s * 100, _max * 100]
}

/**
 * Converts a RGB color into its corresponding HWB components.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @return list[float]
 */
def rgb_to_hwb(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('integer expected')

  var h = rgb_to_hsl(r, g, b)[0]
  var w = 1 / 255 * min(r, min(g, b))
  b = 1 - 1 / 255 * max(r, max(g, b))
  return [h, w * 100, b * 100]
}

/**
 * Converts a RGB color into its corresponding CMYK components.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @return list[float]
 */
def rgb_to_cmyk(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('integer expected')

  r /= 255
  g /= 255
  b /= 255

  var k = min(1 - r, 1 - g, 1 - b),
      c = (1 - r - k) / (1 - k) or 0,
      m = (1 - g - k) / (1 - k) or 0,
      y = (1 - b - k) / (1 - k) or 0

  return [c * 100, m * 100, y * 100, k * 100]
}

/**
 * Converts a RGB color into its corresponding XYZ color space components.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @return list[float]
 */
def rgb_to_xyz(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('integer expected')

  r /= 255
	g /= 255
	b /= 255

	r = r > 0.04045 ? (((r + 0.055) / 1.055) ** 2.4) : (r / 12.92)
	g = g > 0.04045 ? (((g + 0.055) / 1.055) ** 2.4) : (g / 12.92)
	b = b > 0.04045 ? (((b + 0.055) / 1.055) ** 2.4) : (b / 12.92)

	var x = (r * 0.4124564) + (g * 0.3575761) + (b * 0.1804375),
	    y = (r * 0.2126729) + (g * 0.7151522) + (b * 0.072175),
	    z = (r * 0.0193339) + (g * 0.119192) + (b * 0.9503041)

	return [x * 100, y * 100, z * 100];
}

/**
 * Converts a RGB color into its corresponding LAB color components.
 * 
 * @param int r
 * @param int g
 * @param int b
 * @return list[float]
 */
def rgb_to_lab(r, g, b) {
  if !is_int(r) or !is_int(g) or !is_int(b)
    die Exception('integer expected')

  var xyz = rgb_to_xyz(r, g, b)

  var x = xyz[0], y = xyz[1], z = xyz[2]

  x /= 95.047
	y /= 100
	z /= 108.883

	x = x > 0.008856 ? (x ** (1 / 3)) : (7.787 * x) + (16 / 116)
	y = y > 0.008856 ? (y ** (1 / 3)) : (7.787 * y) + (16 / 116)
	z = z > 0.008856 ? (z ** (1 / 3)) : (7.787 * z) + (16 / 116)

	var l = (116 * y) - 16,
      a = 500 * (x - y)
  b = 200 * (y - z)

	return [l, a, b]
}

/**
 * Converts a HSL color into its corresponding RGB color components.
 * 
 * @param number h
 * @param number s
 * @param number l
 * @return list[float]
 */
def hsl_to_rgb(h, s, l) {
  if !is_number(h) or !is_number(s) or !is_number(l)
    die Exception('number expected')

  # For HSL, 0 <= H < 360, 0 <= S <= 1 and 0 <= L <= 1
  if h < 0 or h > 360 or s < 0 or l < 0 or s > 100 or l > 100
    die Exception('invalid color components')

  # Based on https://github.com/Qix-/color-convert/blob/3f0e0d4e92e235796ccb17f6e85c72094a651f49/conversions.js#L244

  h /= 360
	s /= 100
	l /= 100

	var t2, t3, val

	if s == 0 {
		val = math.round(l * 255)
		return [val, val, val]
	}

	if l < 0.5 t2 = l * (1 + s)
  else t2 = l + s - l * s

	var t1 = 2 * l - t2, 
      rgb = [0, 0, 0]

	iter var i = 0; i < 3; i++ {
		t3 = h + 1 / 3 * -(i - 1)

		if t3 < 0 t3++
		if t3 > 1 t3--

		if 6 * t3 < 1 
      val = t1 + (t2 - t1) * 6 * t3
    else if 2 * t3 < 1 
      val = t2
    else if 3 * t3 < 2 
      val = t1 + (t2 - t1) * (2 / 3 - t3) * 6
    else val = t1

		rgb[i] = math.round(min(val * 255, 255))
	}

	return rgb
}

/**
 * Converts a HSL color into its corresponding HSV color components.
 * 
 * @param number h
 * @param number s
 * @param number l
 * @return list[float]
 */
def hsl_to_hsv(h, s, l) {
  if !is_number(h) or !is_number(s) or !is_number(l)
    die Exception('number expected')

  s /= 100
  l /= 100
  var v = (l + s * min(l, 1 - l))
  var s2 = v == 0 ? 0 : 2 * (1 - l / v)
  return [h, s2 * 100, v * 100]
}

/**
 * Converts a HSV color into its corresponding RGB color components.
 * 
 * @param number h
 * @param number s
 * @param number v
 * @return list[float]
 */
def hsv_to_rgb(h, s, v) {
  if !is_number(h) or !is_number(s) or !is_number(v)
    die Exception('number expected')

  h /= 60
  s /= 100
  v /= 100
  
  var hi = math.floor(h) % 6,
      f = h - math.floor(h),
      x = math.round(255 * v * (1 - s)),
      y = math.round(255 * v * (1 - (s *f))),
      z = math.round(255 * v * (1 - (s * (1 - f))))
  
  v = math.round(v * 255)

  using hi {
    when 0 return [v, z, x]
    when 1 return [y, v, x]
    when 2 return [x, v, z]
    when 3 return [x, y, v]
    when 4 return [z, x, v]
    when 5 return [v, z, y]
  }
}

/**
 * Converts a HSV color into its corresponding HSL color components.
 * 
 * @param number h
 * @param number s
 * @param number v
 * @return list[float]
 */
def hsv_to_hsl(h, s, v) {
  if !is_number(h) or !is_number(s) or !is_number(v)
    die Exception('number expected')

  s /= 100
  v /= 100
  
  var vmax = max(v, 0.01),
      l = (2 - s) * v,
      lmax = (2 - s) * vmax,
      sl = s * vmax
  
  sl /= lmax <= 1 ? lmax : 2 - lmax
  sl = sl  or 0
  l /= 2

  return [h, sl * 100, l * 100]
}

/**
 * Converts a HWB color into its corresponding RGB color components.
 * 
 * @param number h
 * @param number w
 * @param number b
 * @return list[float]
 */
def hwb_to_rgb(h, w, b) {
  if !is_number(h) or !is_number(w) or !is_number(b)
    die Exception('number expected')

  h /= 360
  w /= 100
  b /= 100

  var ratio = w + b

  # ratio can't be greater than 1
  if ratio > 1 {
    w /= ratio
    h /= ratio
  }

  var i = math.floor(6 * h),
      v = 1 - b
  var f = 6 * h - i

  if i & 0x01 != 0 f = 1 - f

  # Linear interpolation
  var n = w + f * (v - w), r, g, b2

  using i {
    when 1 { r = n; g = v; b2 = w }
    when 2 { r = w; g = v; b2 = n }
    when 3 { r = w; g = n; b2 = v }
    when 4 { r = n; g = w; b2 = v }
    when 5 { r = v; g = w; b2 = n }
    default { r = v; g = n; b2 = w }
  }

  return [math.round(r * 255), math.round(g * 255), math.round(b2 * 255)]
}

/**
 * Converts a CMYK color into its corresponding RGB color components.
 * 
 * @param number c
 * @param number m
 * @param number y
 * @param number k
 * @return list[float]
 */
def cmyk_to_rgb(c, m, y, k) {
  if !is_number(c) or !is_number(m) or !is_number(y) or !is_number(k)
    die Exception('number expected')

  c /= 100
  m /= 100
  y /= 100
  k /= 100

  var r = 1 - min(1, c * (1 - k) + k)
  var g = 1 - min(1, m * (1 - k) + k)
  var b = 1 - min(1, y * (1 - k) + k)

  return [math.round(r * 255), math.round(g * 255), math.round(b * 255)]
}

/**
 * Converts a XYZ color into its corresponding RGB color components.
 * 
 * @param number x
 * @param number y
 * @param number z
 * @return list[float]
 */
def xyz_to_rgb(x, y, z) {
  if !is_number(x) or !is_number(y) or !is_number(z)
    die Exception('number expected')

  x /= 100
  y /= 100
  z /= 100

  var r, g, b

  r = (x * 3.2404542) + (y * -1.5371385) + (z * -0.4985314);
	g = (x * -0.969266) + (y * 1.8760108) + (z * 0.041556);
	b = (x * 0.0556434) + (y * -0.2040259) + (z * 1.0572252);

	#  Assume sRGB
	r = r > 0.0031308 ? 
      ((1.055 * (r ** (1.0 / 2.4))) - 0.055) : 
      r * 12.92

	g = g > 0.0031308 ? 
      ((1.055 * (g ** (1.0 / 2.4))) - 0.055) : 
      g * 12.92

	b = b > 0.0031308 ? 
      ((1.055 * (b ** (1.0 / 2.4))) - 0.055) : 
      b * 12.92

  r = min(max(0, r), 1)
  g = min(max(0, g), 1)
  b = min(max(0, b), 1)

  return [math.round(r * 255), math.round(g * 255), math.round(b * 255)]
}

