import _hash


/**
 * Returns the SHA-1 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha1(str) {
  return _hash.sha1(str)
}

/**
 * Returns the SHA-224 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha224(str) {
  return _hash.sha224(str)
}

/**
 * Returns the SHA-256 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha256(str) {
  return _hash.sha256(str)
}

/**
 * Returns the SHA-384 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha384(str) {
  return _hash.sha384(str)
}

/**
 * Returns the SHA-512 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha512(str) {
  return _hash.sha512(str)
}

