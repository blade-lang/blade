/**
 * @module hash
 *
 * This module provides a framework for cryptographic and non-cryptographic encryption.
 * 
 * Examples,
 * 
 * ```blade-repl
 * %> import hash
 * %> 
 * %> hash.md5('Hello, World')
 * '82bb413746aee42f89dea2b59614f9ef'
 * %> 
 * %> hash.sha256('Hello, World')
 * '03675ac53ff9cd1535ccc7dfcdfa2c458c5218371f418dc136f2d19ac1fbe8a5'
 * %> 
 * %> hash.siphash('mykey', 'Hello, World')
 * 'd8e830a590c92b4c'
 * %> 
 * %> hash.hmac_sha256('mykey', 'Hello, World')
 * '61035d3d2119ffdfd710913bf4161d5fba1c2d9431f7de7ef398d359eb1d2481'
 * %> 
 * %> hash.hmac_sha256(bytes([10, 11, 12]), 'My secure text!')
 * 'd782079145a3476fd4e018d44dd024034fa91f626f7f30f2009200c5ac757723'
 * ```
 * 
 * @copyright 2021, Richard Ore and Blade contributors
 */

import _hash
import convert

/**
 * Returns the hash of a value as used in a dictionary underlying
 * implementation.
 *
 * A class may override the result of this function by implementing the
 * `to_hash` decorator
 * 
 * @param any value
 * @returns number
 */
def hash(value) {
  return _hash.hash(value)
}

/**
 * Returns the adler32 value of the given string or bytes
 *
 * If value is given, it is used as the base value of the adler32
 * computation. Else, 1 is used.
 * 
 * @param string|bytes str
 * @param number? value
 * @returns number
 */
def adler32(str, value) {
  return _hash.adler32(str, value)
}

/**
 * Returns the crc32 value of the given string or bytes
 *
 * If value is given, it is used as the base value of the crc32
 * computation. Else, 0 is used.
 * 
 * @param string|bytes str
 * @param number? value
 * @returns number
 */
def crc32(str, value) {
  return _hash.crc32(str, value)
}

/**
 * Returns the md2 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def md2(str) {
  return _hash.md2(str)
}

/**
 * Returns the md4 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def md4(str) {
  return _hash.md4(str)
}

/**
 * Returns the md5 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def md5(str) {
  return _hash.md5(str)
}

/**
 * Returns the md5 hash of the given file.
 * 
 * @param file file
 * @returns string
 */
def md5_file(file) {
  return _hash.md5_file(file)
}

/**
 * Returns the sha1 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def sha1(str) {
  return _hash.sha1(str)
}

/**
 * Returns the sha224 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def sha224(str) {
  return _hash.sha224(str)
}

/**
 * Returns the sha256 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def sha256(str) {
  return _hash.sha256(str)
}

/**
 * Returns the sha384 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def sha384(str) {
  return _hash.sha384(str)
}

/**
 * Returns the sha512 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def sha512(str) {
  return _hash.sha512(str)
}

/**
 * Returns the 32 bit fnv1 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def fnv1(str) {
  return _hash.fnv1(str)
}

/**
 * Returns the 64 bit fnv1 hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def fnv1_64(str) {
  return _hash.fnv1_64(str)
}

/**
 * Returns the 32 bit fnv1a hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def fnv1a(str) {
  return _hash.fnv1a(str)
}

/**
 * Returns the 64 bit fnv1a hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def fnv1a_64(str) {
  return _hash.fnv1a_64(str)
}

/**
 * Returns the whirlpool hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def whirlpool(str) {
  return _hash.whirlpool(str)
}

/**
 * Returns the Snefru cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def snefru(str) {
  return _hash.snefru(str)
}

/**
 * Returns the SipHash cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def siphash(key, str) {
  if !is_string(key) and !is_bytes(key) {
    raise TypeError('key must be string or bytes')
  } else if !is_string(str) and !is_bytes(str) {
    raise TypeError('str must be string or bytes')
  }

  if key.length() > 16 {
    raise ValueError('key must be maximum of 16 characters/bytes long')
  } else if key.length() < 16 {
    if is_bytes(key) key = key.to_string()
    key = key.rpad(16, '\0')
  }

  if is_string(key) key = key.to_bytes()
  if is_string(str) str = str.to_bytes()

  return _hash.siphash(key, str)
}

/**
 * Returns the Gost cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes str
 * @returns string
 */
def gost(str) {
  return _hash.gost(str)
}

# list of allowed hash functions allowed for computing hmac hash
var _hmac_allowed = [ 
  md2, md4, md5, sha1, 
  sha224, sha256, sha384, 
  sha512, whirlpool, snefru, 
  gost 
]

/**
 * Computes an HMAC with the key and str using the given method.
 * 
 * @param function method
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac(method, key, str) {
  if !_hmac_allowed.contains(method)
    raise ValueError('invalid HMAC method')

  # convert key and str to array of bytes.
  if is_string(key) key = key.to_bytes()
  if is_string(str) str = str.to_bytes()

  var BLOCK_SIZE = 64

  # Keys longer than blockSize are shortened by hashing them
  if key.length() > BLOCK_SIZE {
    # key is outputSize bytes long
    key = convert.hex_to_bytes(method(key))
  }

  # Keys shorter than blockSize are padded to blockSize by 
  # padding with zeros on the right
  iter var i = key.length(); i < BLOCK_SIZE; i++ key.append(0x00)

  # Outer padded key
  var outer = bytes(BLOCK_SIZE)
  iter var i = 0; i < key.length(); i++ outer[i] = 0x5C ^ key[i]
  iter var i = key.length(); i < BLOCK_SIZE; i++ outer[i] = 0x5C ^ 0x00

  # Inner padded key
  var inner = bytes(BLOCK_SIZE)
  iter var i = 0; i < key.length(); i++ inner[i] = 0x36 ^ key[i]
  iter var i = key.length(); i < BLOCK_SIZE; i++ inner[i] = 0x36 ^ 0x00

  var inner_hash = convert.hex_to_bytes(method(inner.extend(str)))

  return method(outer.extend(inner_hash))
}

/**
 * Returns the HMAC-MD2 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_md2(key, str) {
  return hmac(md2, key, str)
}

/**
 * Returns the HMAC-MD4 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_md4(key, str) {
  return hmac(md4, key, str)
}

/**
 * Returns the HMAC-MD5 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_md5(key, str) {
  return hmac(md5, key, str)
}

/**
 * Returns the HMAC-SHA1 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_sha1(key, str) {
  return hmac(sha1, key, str)
}

/**
 * Returns the HMAC-SHA224 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_sha224(key, str) {
  return hmac(sha224, key, str)
}

/**
 * Returns the HMAC-SHA256 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_sha256(key, str) {
  return hmac(sha256, key, str)
}

/**
 * Returns the HMAC-SHA384 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_sha384(key, str) {
  return hmac(sha384, key, str)
}

/**
 * Returns the HMAC-SHA512 cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_sha512(key, str) {
  return hmac(sha512, key, str)
}

/**
 * Returns the HMAC-WHIRLPOOL cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_whirlpool(key, str) {
  return hmac(whirlpool, key, str)
}

/**
 * Returns the HMAC-SNEFRU cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_snefru(key, str) {
  return hmac(snefru, key, str)
}

/**
 * Returns the HMAC-GOST cryptographic hash of the given string or bytes.
 * 
 * @param string|bytes key
 * @param string|bytes str
 * @returns string
 */
def hmac_gost(key, str) {
  return hmac(gost, key, str)
}

