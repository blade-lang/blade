import _hash


/**
 * Returns the Whirlpool hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def whirlpool(str) {
  return _hash.whirlpool(str)
}

/**
 * Returns the Snefru cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def snefru(str) {
  return _hash.snefru(str)
}

/**
 * Returns the Gost cyrptographic hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def gost(str) {
  return _hash.gost(str)
}

