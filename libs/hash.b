/**
 * Hash
 *
 * Provides interface for cryptographic and non-cryptographic encryption
 * @copyright 2021, Ore Richard Muyiwa
 */
class Hash {


  /**
   * adler32(str: string | bytes, [value: number])
   * returns the adler32 value of the given string or bytes
   *
   * if value is given, it is used as the base value of the adler32
   * computation. Else, 1 is used.
   */
  static adler32(str, value) {
    return self._adler32(str, value)
  }

  /**
   * crc32(str: string | bytes, [value: number])
   * returns the crc32 value of the given string or bytes
   *
   * if value is given, it is used as the base value of the crc32
   * computation. Else, 0 is used.
   */
  static crc32(str, value) {
    return self._crc32(str, value)
  }

  /**
   * md2(str: string | bytes)
   * returns the md2 hash of the given string or bytes
   */
  static md2(str) {}

  /**
   * md4(str: string | bytes)
   * returns the md4 hash of the given string or bytes
   */
  static md4(str) {}

  /**
   * md5(str: string | bytes)
   * returns the md5 hash of the given string or bytes
   */
  static md5(str) {}

  /**
   * md5_file(str: file)
   * returns the md5 hash of the given file
   */
  static md5_file(file) {}

  /**
   * sha1(str: string | bytes)
   * returns the sha1 hash of the given string or bytes
   */
  static sha1(str) {}

  /**
   * sha224(str: string | bytes)
   * returns the sha224 hash of the given string or bytes
   */
  static sha224(str) {}

  /**
   * sha256(str: string | bytes)
   * returns the sha256 hash of the given string or bytes
   */
  static sha256(str) {}

  /**
   * sha384(str: string | bytes)
   * returns the sha384 hash of the given string or bytes
   */
  static sha384(str) {}

  /**
   * sha512(str: string | bytes)
   * returns the sha512 hash of the given string or bytes
   */
  static sha512(str) {}

  /**
   * fnv1(str: string | bytes)
   * returns the 32 bit fnv1 hash of the given string or bytes
   */
  static fnv1(str) {}

  /**
   * fnv1_64(str: string | bytes)
   * returns the 64 bit fnv1 hash of the given string or bytes
   */
  static fnv1_64(str) {}

  /**
   * fnv1a(str: string | bytes)
   * returns the 32 bit fnv1a hash of the given string or bytes
   */
  static fnv1a(str) {}

  /**
   * fnv1a_64(str: string | bytes)
   * returns the 64 bit fnv1a hash of the given string or bytes
   */
  static fnv1a_64(str) {}
}

