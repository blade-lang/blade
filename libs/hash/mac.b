import _hash
import convert
import .checksum { * }
import .sha { * }
import .cipher { * }


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
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac(method, key, str) {
  if !_hmac_allowed.contains(method)
    die Exception('invalid HMAC method')

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
  iter var i = key.length(); i < BLOCK_SIZE; i++ {
    key.append(0x00)
  }

  # Outer padded key
  var outer = bytes(BLOCK_SIZE)
  iter var i = 0; i < key.length(); i++ outer[i] = 0x5C ^ key[i]
  iter var i = key.length(); i < BLOCK_SIZE; i++ {
    outer[i] = 0x5C ^ 0x00
  }

  # Inner padded key
  var inner = bytes(BLOCK_SIZE)
  iter var i = 0; i < key.length(); i++ inner[i] = 0x36 ^ key[i]
  iter var i = key.length(); i < BLOCK_SIZE; i++ {
    inner[i] = 0x36 ^ 0x00
  }

  var inner_hash = convert.hex_to_bytes(method(inner.extend(str)))

  return method(outer.extend(inner_hash))
}

/**
 * Returns the HMAC-MD2 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_md2(key, str) {
  return hmac(md2, key, str)
}

/**
 * Returns the HMAC-MD4 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_md4(key, str) {
  return hmac(md4, key, str)
}

/**
 * Returns the HMAC-MD5 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_md5(key, str) {
  return hmac(md5, key, str)
}

/**
 * Returns the HMAC-SHA1 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_sha1(key, str) {
  return hmac(sha1, key, str)
}

/**
 * Returns the HMAC-SHA224 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_sha224(key, str) {
  return hmac(sha224, key, str)
}

/**
 * Returns the HMAC-SHA256 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_sha256(key, str) {
  return hmac(sha256, key, str)
}

/**
 * Returns the HMAC-SHA384 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_sha384(key, str) {
  return hmac(sha384, key, str)
}

/**
 * Returns the HMAC-SHA512 cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_sha512(key, str) {
  return hmac(sha512, key, str)
}

/**
 * Returns the HMAC-WHIRLPOOL cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_whirlpool(key, str) {
  return hmac(whirlpool, key, str)
}

/**
 * Returns the HMAC-SNEFRU cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_snefru(key, str) {
  return hmac(snefru, key, str)
}

/**
 * Returns the HMAC-GOST cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
 * @return string
 */
def hmac_gost(key, str) {
  return hmac(gost, key, str)
}

/**
 * Returns the SipHash cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} key
 * @param {string|bytes} str
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

