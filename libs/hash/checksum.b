import _hash


/**
 * Returns the 32 bit fnv1 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def fnv1(str) {
  return _hash.fnv1(str)
}

/**
 * Returns the 64 bit fnv1 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def fnv1_64(str) {
  return _hash.fnv1_64(str)
}

/**
 * Returns the 32 bit fnv1a hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def fnv1a(str) {
  return _hash.fnv1a(str)
}

/**
 * Returns the 64 bit fnv1a hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def fnv1a_64(str) {
  return _hash.fnv1a_64(str)
}

/**
 * Returns the adler32 value of the given string or bytes
 *
 * If value is given, it is used as the base value of the adler32
 * computation. Else, 1 is used.
 * 
 * @param {string|bytes} str
 * @param number? value
 * @return number
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
 * @param {string|bytes} str
 * @param number? value
 * @return number
 */
def crc32(str, value) {
  return _hash.crc32(str, value)
}

/**
 * Returns the md2 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def md2(str) {
  return _hash.md2(str)
}

/**
 * Returns the md4 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def md4(str) {
  return _hash.md4(str)
}

/**
 * Returns the md5 hash of the given string or bytes.
 * 
 * @param {string|bytes} str
 * @return string
 */
def md5(str) {
  return _hash.md5(str)
}

/**
 * Returns the md5 hash of the given file.
 * 
 * @param file file
 * @return string
 */
def md5_file(file) {
  return _hash.md5_file(file)
}

