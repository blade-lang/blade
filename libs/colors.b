#
# @module colors
#
# This module provides functionalities for color conversion and manipulation.
# 
# This module also provides functionalities that enable cross-platform colored terminal outputs 
# that will allow you create beautiful console apps that are user friendly.
# 
# @copyright 2022, Ore Richard Muyiwa and Blade contributors
#

import os
import math

var styles = {
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

    return '\x1b[${v ? v : ''}${bg ? ';${bg ? bg : ''}' : ''}m'
  }
  return nil
}

def text(value, color, bg) {
  return _get_sequence(color, bg) + value + _get_sequence(0)
}

# From https://github.com/Qix-/color-convert/blob/3f0e0d4e92e235796ccb17f6e85c72094a651f49/conversions.js
def rgb_to_ansi(r, g, b) {
  if r == g and g == b {
    if red < 8 return 16
    if red > 248 return 231
    return math.round(((r - 8) / 247) * 24) + 232
  }

  return 16 + (36 * math.round(r / 255 * 5)) +
      (6 * math.round(g / 255 * 5)) +
      math.round(b / 255 * 5)
}

def hex_to_rgb(h) {
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

  # if we are still less than 8 digits, add two zeros(s).
  v = v.rpad(8, '0')

  var number = to_number('0x${v}')

  var b = number & 0xFF,
      g = (number & 0xFF00) >> 8,
      r = (number & 0xFF0000) >> 16,
      a = ((number & 0xFF000000) >> 24) / 255

  return [r, g, b, a]
}

echo hex_to_rgb('ccca')