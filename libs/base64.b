#
# @module base64
#
# This module provides interface for encoding binary data into strings and 
# decoding such encoded strings back into binary data based on the base64 
# encoding specified in [RFC4648](https://datatracker.ietf.org/doc/html/rfc4648)
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
#

import _base64

/**
 * encode(data: bytes)
 *
 * Encodes a byte array into a base64 string
 * @return string
 */
def encode(data) {
  return _base64.encode(data)
}

/**
 * decode(data: string)
 *
 * Decodes a base64 string into it's corresponding bytes
 * @return bytes
 */
def decode(data) {
  return _base64.decode(data)
}
