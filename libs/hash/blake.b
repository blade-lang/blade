import _hash


/**
 * Returns the BLAKE2b hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def blake2b(str) {
  return _hash.blake2b(str)
}

/**
 * Returns the BLAKE2s hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def blake2s(str) {
  return _hash.blake2s(str)
}

