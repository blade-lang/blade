#
# @module base64 (RFC1341)
#
# Provides interface for encoding and decoding base64 data
# @copyright 2021, Ore Richard Muyiwa
#

import _base64

/**
 * encode(data: bytes)
 *
 * Encodes a bytes into a base64 string
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
