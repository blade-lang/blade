/**
 * @module sha3
 * 
 * This module provide the generic Keccak class and functions for 
 * working with SHA3, Keccak and SHAKE hashes.
 * 
 * Like the `hash` module, this module exports quick and simple 
 * functions to compute SHA3/Keccak family of functions.
 * 
 * For example:
 * 
 * ```blade-repl
 * %> import sha3
 * %> 
 * %> sha3.sha3_256('hello')
 * '3338be694f50c5f338814986cdf0686453a888b84f424d792af4b9202398f392'
 * ```
 * 
 * It also exports the generic `Keccak` class that you can customize 
 * to your preference.
 * 
 * For example:
 * 
 * ```blade-repl
 * %> import sha3
 * %> 
 * %> var h = sha3.Keccak(256, 1)
 * %> h.update('How are you?')
 * <class Keccak instance at 0x14010bf40>
 * %> h.digest('')
 * (97 bf fd e9 c7 30 76 73 58 99 84 84 1c cc c1 8c dc 28 90 d6  ...32)
 * ```
 * 
 * You can use the `bytes_to_hex()` function in the `convert` module to 
 * get a hexadecimal string from the digest.
 * 
 * @copyright 2024, Ore Richard Muyiwa and Blade contributors
 * @copyright Marco Paland (marco at paland dot com)
 * @copyright 2015-2018, PALANDesign Hannover, Germany
 */
import convert { bytes_to_hex }

var _rc = [
  1, 0, 32898, 0, 32906, 2147483648, 2147516416, 2147483648, 32907, 0, 2147483649,
  0, 2147516545, 2147483648, 32777, 2147483648, 138, 0, 136, 0, 2147516425, 0,
  2147483658, 0, 2147516555, 0, 139, 2147483648, 32905, 2147483648, 32771,
  2147483648, 32770, 2147483648, 128, 2147483648, 32778, 0, 2147483658, 2147483648,
  2147516545, 2147483648, 32896, 2147483648, 2147483649, 0, 2147516424, 2147483648
]


/**
 * Keccak hash manipulation class.
 */
class Keccak {
  var _hash_size
  var _block_count
  var _byte_count
  var _buffer
  var buffer_index
  var _s
  var _padding

  /**
   * @param number bits: Capacity
   * @param number padding: Padding value - 1 for Keccak, 6 for SHA3 and 31 for SHAKE
   * @param number? length: Optional length of the output hash in bits. If not given bits is taken as default.
   * @constructor
   */
  Keccak(bits, padding, length) {
    if !is_number(bits) or !is_number(padding) or 
      (length != nil and !is_number(length)) {
        raise Exception('number expected')
    }

    self._padding = padding
    self._hash_size = (length or bits) / 8
    self._block_count = (1600 - bits * 2) / 32
    self._byte_count = self._block_count * 4

    self._s = [0] * 50
    self._buffer = bytes(self._byte_count)

    self.init()
  }

  /**
   * Initializes the hash functionality and prepares it for a new round.
   * 
   * @returns self
   */
  init() {
    self._buffer.dispose()
    self._buffer = bytes(50)
    self._buffer_index = 0
    return self
  }

  /**
   * Update the hash with additional message data.
   * 
   * @param string|bytes message
   * @returns self
   */
  update(message) {
    if !is_string(message) and !is_bytes(message) 
      raise Exception('string or bytes expected.')

    if is_string(message) 
      message = message.to_bytes()

    var i = 0
    iter var len = message.length(); i < len; i++ {
      self._buffer[self._buffer_index++ - 1] = message[i]
      if self._buffer_index == self._byte_count {
        self._keccakf()
        self._buffer_index = 0
      }
    }

    return self
  }

  /**
   * Finalize the hash with additional message data and returns 
   * the message digest.
   * 
   * @param string|bytes message
   * @returns bytes
   */
  digest(message) {
    self.update(message)

    var b = self._buffer, idx = self._buffer_index,
        byte_count = self._byte_count
    b[idx++ - 1] = self._padding

    while idx < b.length() {
      b[idx++ - 1] = 0
    }

    if b.length() < byte_count {
      var len = byte_count - b.length()
      b.extend(bytes(len))
      idx += len
    }

    b[byte_count - 1] |= 0x80

    self._keccakf()

    var s = self._s
    var hash_size = self._hash_size

    var hash = bytes(hash_size)
    iter var i = 0; i < hash_size / 4; i++ {
      hash[(i << 2) + 0] = (s[i] >>> 0) & 0xff
      hash[(i << 2) + 1] = (s[i] >>> 8) & 0xff
      hash[(i << 2) + 2] = (s[i] >>> 16) & 0xff
      hash[(i << 2) + 3] = (s[i] >>> 24) & 0xff
    }

    self.init()
    return hash
  }

  /**
   * All-in-one method to initialize a new round, update the 
   * message and generate the final hash digest.
   * 
   * @param string|bytes message
   * @returns string
   */
  hash(message) {
    return self.init().digest(message)
  }

  _keccakf() {
    var s = self._s, b = [0] * 50
    var b0,  b1,  b2,  b3,  b4,  b5,  b6,  b7,  b8,  b9,  b10, b11, b12, b13, b14, b15, b16,
        b17, b18, b19, b20, b21, b22, b23, b24, b25, b26, b27, b28, b29, b30, b31, b32, b33,
        b34, b35, b36, b37, b38, b39, b40, b41, b42, b43, b44, b45, b46, b47, b48, b49

    var buffer = self._buffer

    # convert byte buffer to words and absorb it
    iter var i = 0; i < self._block_count; i++ {
      s[i] ^= ((buffer[(i << 2) + 0]) | (buffer[(i << 2) + 1] << 8) | (buffer[(i << 2) + 2] << 16) | (buffer[(i << 2) + 3] << 24))
    }

    var c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, h, l
    iter var n = 0; n < 48; n += 2 {
      c0 = s[0] ^ s[10] ^ s[20] ^ s[30] ^ s[40]
      c1 = s[1] ^ s[11] ^ s[21] ^ s[31] ^ s[41]
      c2 = s[2] ^ s[12] ^ s[22] ^ s[32] ^ s[42]
      c3 = s[3] ^ s[13] ^ s[23] ^ s[33] ^ s[43]
      c4 = s[4] ^ s[14] ^ s[24] ^ s[34] ^ s[44]
      c5 = s[5] ^ s[15] ^ s[25] ^ s[35] ^ s[45]
      c6 = s[6] ^ s[16] ^ s[26] ^ s[36] ^ s[46]
      c7 = s[7] ^ s[17] ^ s[27] ^ s[37] ^ s[47]
      c8 = s[8] ^ s[18] ^ s[28] ^ s[38] ^ s[48]
      c9 = s[9] ^ s[19] ^ s[29] ^ s[39] ^ s[49]

      h = c8 ^ ((c2 << 1) | (c3 >>> 31))
      l = c9 ^ ((c3 << 1) | (c2 >>> 31))
      s[0]  ^= h
      s[1]  ^= l
      s[10] ^= h
      s[11] ^= l
      s[20] ^= h
      s[21] ^= l
      s[30] ^= h
      s[31] ^= l
      s[40] ^= h
      s[41] ^= l
      h = c0 ^ ((c4 << 1) | (c5 >>> 31))
      l = c1 ^ ((c5 << 1) | (c4 >>> 31))
      s[2]  ^= h
      s[3]  ^= l
      s[12] ^= h
      s[13] ^= l
      s[22] ^= h
      s[23] ^= l
      s[32] ^= h
      s[33] ^= l
      s[42] ^= h
      s[43] ^= l
      h = c2 ^ ((c6 << 1) | (c7 >>> 31))
      l = c3 ^ ((c7 << 1) | (c6 >>> 31))
      s[4]  ^= h
      s[5]  ^= l
      s[14] ^= h
      s[15] ^= l
      s[24] ^= h
      s[25] ^= l
      s[34] ^= h
      s[35] ^= l
      s[44] ^= h
      s[45] ^= l
      h = c4 ^ ((c8 << 1) | (c9 >>> 31))
      l = c5 ^ ((c9 << 1) | (c8 >>> 31))
      s[6]  ^= h
      s[7]  ^= l
      s[16] ^= h
      s[17] ^= l
      s[26] ^= h
      s[27] ^= l
      s[36] ^= h
      s[37] ^= l
      s[46] ^= h
      s[47] ^= l
      h = c6 ^ ((c0 << 1) | (c1 >>> 31))
      l = c7 ^ ((c1 << 1) | (c0 >>> 31))
      s[8]  ^= h
      s[9]  ^= l
      s[18] ^= h
      s[19] ^= l
      s[28] ^= h
      s[29] ^= l
      s[38] ^= h
      s[39] ^= l
      s[48] ^= h
      s[49] ^= l

      b0  = s[0]
      b1  = s[1]
      b32 = (s[11] <<  4) | (s[10] >>> 28)
      b33 = (s[10] <<  4) | (s[11] >>> 28)
      b14 = (s[20] <<  3) | (s[21] >>> 29)
      b15 = (s[21] <<  3) | (s[20] >>> 29)
      b46 = (s[31] <<  9) | (s[30] >>> 23)
      b47 = (s[30] <<  9) | (s[31] >>> 23)
      b28 = (s[40] << 18) | (s[41] >>> 14)
      b29 = (s[41] << 18) | (s[40] >>> 14)
      b20 = (s[2]  <<  1) | (s[3] >>> 31)
      b21 = (s[3]  <<  1) | (s[2] >>> 31)
      b2  = (s[13] << 12) | (s[12] >>> 20)
      b3  = (s[12] << 12) | (s[13] >>> 20)
      b34 = (s[22] << 10) | (s[23] >>> 22)
      b35 = (s[23] << 10) | (s[22] >>> 22)
      b16 = (s[33] << 13) | (s[32] >>> 19)
      b17 = (s[32] << 13) | (s[33] >>> 19)
      b48 = (s[42] <<  2) | (s[43] >>> 30)
      b49 = (s[43] <<  2) | (s[42] >>> 30)
      b40 = (s[5]  << 30) | (s[4] >>> 2)
      b41 = (s[4]  << 30) | (s[5] >>> 2)
      b22 = (s[14] <<  6) | (s[15] >>> 26)
      b23 = (s[15] <<  6) | (s[14] >>> 26)
      b4  = (s[25] << 11) | (s[24] >>> 21)
      b5  = (s[24] << 11) | (s[25] >>> 21)
      b36 = (s[34] << 15) | (s[35] >>> 17)
      b37 = (s[35] << 15) | (s[34] >>> 17)
      b18 = (s[45] << 29) | (s[44] >>> 3)
      b19 = (s[44] << 29) | (s[45] >>> 3)
      b10 = (s[6]  << 28) | (s[7] >>> 4)
      b11 = (s[7]  << 28) | (s[6] >>> 4)
      b42 = (s[17] << 23) | (s[16] >>> 9)
      b43 = (s[16] << 23) | (s[17] >>> 9)
      b24 = (s[26] << 25) | (s[27] >>> 7)
      b25 = (s[27] << 25) | (s[26] >>> 7)
      b6  = (s[36] << 21) | (s[37] >>> 11)
      b7  = (s[37] << 21) | (s[36] >>> 11)
      b38 = (s[47] << 24) | (s[46] >>> 8)
      b39 = (s[46] << 24) | (s[47] >>> 8)
      b30 = (s[8]  << 27) | (s[9] >>> 5)
      b31 = (s[9]  << 27) | (s[8] >>> 5)
      b12 = (s[18] << 20) | (s[19] >>> 12)
      b13 = (s[19] << 20) | (s[18] >>> 12)
      b44 = (s[29] <<  7) | (s[28] >>> 25)
      b45 = (s[28] <<  7) | (s[29] >>> 25)
      b26 = (s[38] <<  8) | (s[39] >>> 24)
      b27 = (s[39] <<  8) | (s[38] >>> 24)
      b8  = (s[48] << 14) | (s[49] >>> 18)
      b9  = (s[49] << 14) | (s[48] >>> 18)

      s[0]  = b0  ^ (~b2  & b4)
      s[1]  = b1  ^ (~b3  & b5)
      s[10] = b10 ^ (~b12 & b14)
      s[11] = b11 ^ (~b13 & b15)
      s[20] = b20 ^ (~b22 & b24)
      s[21] = b21 ^ (~b23 & b25)
      s[30] = b30 ^ (~b32 & b34)
      s[31] = b31 ^ (~b33 & b35)
      s[40] = b40 ^ (~b42 & b44)
      s[41] = b41 ^ (~b43 & b45)
      s[2]  = b2  ^ (~b4  & b6)
      s[3]  = b3  ^ (~b5  & b7)
      s[12] = b12 ^ (~b14 & b16)
      s[13] = b13 ^ (~b15 & b17)
      s[22] = b22 ^ (~b24 & b26)
      s[23] = b23 ^ (~b25 & b27)
      s[32] = b32 ^ (~b34 & b36)
      s[33] = b33 ^ (~b35 & b37)
      s[42] = b42 ^ (~b44 & b46)
      s[43] = b43 ^ (~b45 & b47)
      s[4]  = b4 ^  (~b6  & b8)
      s[5]  = b5 ^  (~b7  & b9)
      s[14] = b14 ^ (~b16 & b18)
      s[15] = b15 ^ (~b17 & b19)
      s[24] = b24 ^ (~b26 & b28)
      s[25] = b25 ^ (~b27 & b29)
      s[34] = b34 ^ (~b36 & b38)
      s[35] = b35 ^ (~b37 & b39)
      s[44] = b44 ^ (~b46 & b48)
      s[45] = b45 ^ (~b47 & b49)
      s[6]  = b6  ^ (~b8  & b0)
      s[7]  = b7  ^ (~b9  & b1)
      s[16] = b16 ^ (~b18 & b10)
      s[17] = b17 ^ (~b19 & b11)
      s[26] = b26 ^ (~b28 & b20)
      s[27] = b27 ^ (~b29 & b21)
      s[36] = b36 ^ (~b38 & b30)
      s[37] = b37 ^ (~b39 & b31)
      s[46] = b46 ^ (~b48 & b40)
      s[47] = b47 ^ (~b49 & b41)
      s[8]  = b8  ^ (~b0  & b2)
      s[9]  = b9  ^ (~b1  & b3)
      s[18] = b18 ^ (~b10 & b12)
      s[19] = b19 ^ (~b11 & b13)
      s[28] = b28 ^ (~b20 & b22)
      s[29] = b29 ^ (~b21 & b23)
      s[38] = b38 ^ (~b30 & b32)
      s[39] = b39 ^ (~b31 & b33)
      s[48] = b48 ^ (~b40 & b42)
      s[49] = b49 ^ (~b41 & b43)
      s[0] ^= _rc[n]
      s[1] ^= _rc[n + 1]
    }
  }
}


/**
 * Returns the Keccak-224 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def keccak_224(message) {
  return bytes_to_hex(Keccak(224, 1).hash(message))
}

/**
 * Returns the Keccak-256 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def keccak_256(message) {
  return bytes_to_hex(Keccak(256, 1).hash(message))
}

/**
 * Returns the Keccak-384 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def keccak_384(message) {
  return bytes_to_hex(Keccak(384, 1).hash(message))
}

/**
 * Returns the Keccak-512 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def keccak_512(message) {
  return bytes_to_hex(Keccak(512, 1).hash(message))
}

/**
 * Returns the SHA3-224 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def sha3_224(message) {
  return bytes_to_hex(Keccak(224, 6).hash(message))
}

/**
 * Returns the SHA3-256 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def sha3_256(message) {
  return bytes_to_hex(Keccak(256, 6).hash(message))
}

/**
 * Returns the SHA3-384 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def sha3_384(message) {
  return bytes_to_hex(Keccak(384, 6).hash(message))
}

/**
 * Returns the SHA3-512 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes message
 * @returns string
 */
def sha3_512(message) {
  return bytes_to_hex(Keccak(512, 6).hash(message))
}

/**
 * Returns the SHAKE-128 cryptographic hash of the given string or bytes computed to the given length.
 * 
 * @param number length
 * @param string|bytes message
 * @returns string
 */
def shake128(length, message) {
  return bytes_to_hex(Keccak(128, 31, length).hash(message))
}

/**
 * Returns the SHAKE-256 cryptographic hash of the given string or bytes computed to the given length.
 * 
 * @param number length
 * @param string|bytes message
 * @returns string
 */
def shake256(length, message) {
  return bytes_to_hex(Keccak(256, 31, length).hash(message))
}
