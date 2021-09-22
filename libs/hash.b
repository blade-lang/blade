#
# @module hash
#
# Provides interface for cryptographic and non-cryptographic encryption
# @ copyright 2021, Ore Richard Muyiwa and Blade contributors
#

import _hash
import convert

/**
 * hash(value: any)
 *
 * returns the hash of a value as used in a dictionary underlying
 * implementation.
 *
 * a class may override the result of this function by implementing the
 * @hash() method
 * @return number
 */
def hash(value) {
  return _hash.hash(value)
}

/**
 * adler32(str: string | bytes, [value: number])
 * returns the adler32 value of the given string or bytes
 *
 * if value is given, it is used as the base value of the adler32
 * computation. Else, 1 is used.
 * @return number
 */
def adler32(str, value) {
  return _hash.adler32(str, value)
}

/**
 * crc32(str: string | bytes, [value: number])
 * returns the crc32 value of the given string or bytes
 *
 * if value is given, it is used as the base value of the crc32
 * computation. Else, 0 is used.
 * @return number
 */
def crc32(str, value) {
  return _hash.crc32(str, value)
}

/**
 * md2(str: string | bytes)
 * 
 * returns the md2 hash of the given string or bytes
 * @return string
 */
def md2(str) {
  return _hash.md2(str)
}

/**
 * md4(str: string | bytes)
 * 
 * returns the md4 hash of the given string or bytes
 * @return string
 */
def md4(str) {
  return _hash.md4(str)
}

/**
 * md5(str: string | bytes)
 * 
 * returns the md5 hash of the given string or bytes
 * @return string
 */
def md5(str) {
  return _hash.md5(str)
}

/**
 * md5_file(str: file)
 * 
 * returns the md5 hash of the given file
 * @return string
 */
def md5_file(file) {
  return _hash.md5_file(file)
}

/**
 * sha1(str: string | bytes)
 * 
 * returns the sha1 hash of the given string or bytes
 * @return string
 */
def sha1(str) {
  return _hash.sha1(str)
}

/**
 * sha224(str: string | bytes)
 * 
 * returns the sha224 hash of the given string or bytes
 * @return string
 */
def sha224(str) {
  return _hash.sha224(str)
}

/**
 * sha256(str: string | bytes)
 * 
 * returns the sha256 hash of the given string or bytes
 * @return string
 */
def sha256(str) {
  return _hash.sha256(str)
}

/**
 * sha384(str: string | bytes)
 * 
 * returns the sha384 hash of the given string or bytes
 * @return string
 */
def sha384(str) {
  return _hash.sha384(str)
}

/**
 * sha512(str: string | bytes)
 * 
 * returns the sha512 hash of the given string or bytes
 * @return string
 */
def sha512(str) {
  return _hash.sha512(str)
}

/**
 * fnv1(str: string | bytes)
 * 
 * returns the 32 bit fnv1 hash of the given string or bytes
 * @return string
 */
def fnv1(str) {
  return _hash.fnv1(str)
}

/**
 * fnv1_64(str: string | bytes)
 * 
 * returns the 64 bit fnv1 hash of the given string or bytes
 * @return string
 */
def fnv1_64(str) {
  return _hash.fnv1_64(str)
}

/**
 * fnv1a(str: string | bytes)
 * 
 * returns the 32 bit fnv1a hash of the given string or bytes
 * @return string
 */
def fnv1a(str) {
  return _hash.fnv1a(str)
}

/**
 * fnv1a_64(str: string | bytes)
 * 
 * returns the 64 bit fnv1a hash of the given string or bytes
 * @return string
 */
def fnv1a_64(str) {
  return _hash.fnv1a_64(str)
}

/**
 * whirlpool(str: string | bytes)
 * 
 * returns the whirlpool hash of the given string or bytes
 * @return string
 */
def whirlpool(str) {
  return _hash.whirlpool(str)
}

/**
 * snefru(str: string | bytes)
 * 
 * returns the snefru cyrptographic hash of the given string or bytes
 * @return string
 */
def snefru(str) {
  return _hash.snefru(str)
}

/**
 * siphash(key: string | bytes, str: string | bytes)
 * 
 * returns the siphash cyrptographic hash of the given string or bytes
 * @return string
 */
def siphash(key, str) {
  if !is_string(key) and !is_bytes(key) {
    die Exception('key must be string or bytes')
  } else if !is_string(str) and !is_bytes(str) {
    die Exception('str must be string or bytes')
  }

  if key.length() > 16
    die Exception('key must be maximum of 16 characters/bytes long')
  else if key.length() < 16 {
    if is_bytes(key) key = key.to_string()
    key = key.rpad(16, '\0')
  }

  if is_string(key) key = key.to_bytes()
  if is_string(str) str = str.to_bytes()

  return _hash.siphash(key, str)
}

/**
 * gost(str: string | bytes)
 * 
 * returns the gost cyrptographic hash of the given string or bytes
 * @return string
 */
def gost(str) {
  return _hash.gost(str)
}

/**
 * list of allowed hash functions allowed for computing hmac hash
 */
var _hmac_allowed = [ 
  md2, md4, md5, sha1, 
  sha224, sha256, sha384, 
  sha512, whirlpool, snefru, 
  gost 
]

/**
 * hmac(method: function, key: string | bytes, str: string | bytes)
 * 
 * computes an HMAC with the key and str using the given method
 * @return string
 */
def hmac(method, key, str) {
  if !_hmac_allowed.contains(method)
    die Exception('invalid HMAC method')

  # convert key and str to array of bytes.
  key = key.to_bytes()
  str = str.to_bytes()

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
 * hmac_md2(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-MD2 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_md2(key, str) {
  return hmac(md2, key, str)
}

/**
 * hmac_md4(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-MD4 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_md4(key, str) {
  return hmac(md4, key, str)
}

/**
 * hmac_md5(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-MD5 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_md5(key, str) {
  return hmac(md5, key, str)
}

/**
 * hmac_sha1(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-SHA1 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_sha1(key, str) {
  return hmac(sha1, key, str)
}

/**
 * hmac_sha224(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-SHA224 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_sha224(key, str) {
  return hmac(sha224, key, str)
}

/**
 * hmac_sha256(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-SHA256 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_sha256(key, str) {
  return hmac(sha256, key, str)
}

/**
 * hmac_sha384(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-SHA384 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_sha384(key, str) {
  return hmac(sha384, key, str)
}

/**
 * hmac_sha512(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-SHA512 cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_sha512(key, str) {
  return hmac(sha512, key, str)
}

/**
 * hmac_whirlpool(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-WHIRLPOOL cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_whirlpool(key, str) {
  return hmac(whirlpool, key, str)
}

/**
 * hmac_snefru(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-SNEFRU cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_snefru(key, str) {
  return hmac(snefru, key, str)
}

/**
 * hmac_gost(key: string | bytes, str: string | bytes)
 * 
 * returns the HMAC-GOST cyrptographic hash of the given string or bytes
 * @return string
 */
def hmac_gost(key, str) {
  return hmac(gost, key, str)
}

