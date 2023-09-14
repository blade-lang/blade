import _hash


/**
 * Returns the Keccak-256 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def keccak256(str) {
  return _hash.keccak(str, 0x01, 256)
}

/**
 * Returns the Keccak-384 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def keccak384(str) {
  return _hash.keccak(str, 0x01, 384)
}

/**
 * Returns the Keccak-512 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def keccak512(str) {
  return _hash.keccak(str, 0x01, 512)
}

/**
 * Returns the SHA3-256 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha3_256(str) {
  return _hash.keccak(str, 0x06, 256)
}

/**
 * Returns the SHA3-384 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha3_384(str) {
  return _hash.keccak(str, 0x06, 384)
}

/**
 * Returns the SHA3-512 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def sha3_512(str) {
  return _hash.keccak(str, 0x06, 512)
}

/**
 * Returns the SHAKE128 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @param number length
 * @return string
 */
def shake128(str, length) {
  return _hash.keccak(str, 0x1f, 128, length / 8)
}

/**
 * Returns the SHAKE256 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @param number length
 * @return string
 */
def shake256(str, length) {
  return _hash.keccak(str, 0x1f, 256, length / 8)
}

