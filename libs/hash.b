/**
 * Hash
 *
 * Provides interface for cryptographic and non-cryptographic encryption
 * @copyright 2021, Ore Richard Muyiwa
 */
import convert

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

  /**
   * whirlpool(str: string | bytes)
   * returns the whirlpool hash of the given string or bytes
   */
  static whirlpool(str) {}

  /**
   * snefru(str: string | bytes)
   * returns the snefru cyrptographic hash of the given string or bytes
   */
  static snefru(str) {}

  /**
   * siphash(key: string | bytes, str: string | bytes)
   * returns the siphash cyrptographic hash of the given string or bytes
   */
  static siphash(key, str) {
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

    return self._siphash(key, str)
  }

  /**
   * gost(str: string | bytes)
   * returns the gost cyrptographic hash of the given string or bytes
   */
  static gost(str) {}

  /**
   * hmac(method: function, key: string | bytes, str: string | bytes)
   * computes an HMAC with the key and str using the given method
   */
  static hmac(method, key, str) {
    if ![
          Hash.md2,
          Hash.md4,
          Hash.md5,
          Hash.sha1,
          Hash.sha224,
          Hash.sha256,
          Hash.sha384,
          Hash.sha512,
          Hash.whirlpool,
          Hash.snefru,
          Hash.gost
        ].contains(method)
      die Exception('invalid HMAC method')

    # convert key and str to array of bytes.
    key = key.to_bytes()
    str = str.to_bytes()

    var BLOCK_SIZE = 64

    # Keys longer than blockSize are shortened by hashing them
    if key.length() > BLOCK_SIZE {
      # key is outputSize bytes long
      key = Convert.hex_to_bytes(method(key))
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

    var inner_hash = Convert.hex_to_bytes(method(inner.extend(str)))

    return method(outer.extend(inner_hash))
  }

  /**
   * hmac_md2(key: string | bytes, str: string | bytes)
   * returns the HMAC-MD2 cyrptographic hash of the given string or bytes
   */
  static hmac_md2(key, str) {
    return Hash.hmac(Hash.md2, key, str)
  }

  /**
   * hmac_md4(key: string | bytes, str: string | bytes)
   * returns the HMAC-MD4 cyrptographic hash of the given string or bytes
   */
  static hmac_md4(key, str) {
    return Hash.hmac(Hash.md4, key, str)
  }

  /**
   * hmac_md5(key: string | bytes, str: string | bytes)
   * returns the HMAC-MD5 cyrptographic hash of the given string or bytes
   */
  static hmac_md5(key, str) {
    return Hash.hmac(Hash.md5, key, str)
  }

  /**
   * hmac_sha1(key: string | bytes, str: string | bytes)
   * returns the HMAC-SHA1 cyrptographic hash of the given string or bytes
   */
  static hmac_sha1(key, str) {
    return Hash.hmac(Hash.sha1, key, str)
  }

  /**
   * hmac_sha224(key: string | bytes, str: string | bytes)
   * returns the HMAC-SHA224 cyrptographic hash of the given string or bytes
   */
  static hmac_sha224(key, str) {
    return Hash.hmac(Hash.sha224, key, str)
  }

  /**
   * hmac_sha256(key: string | bytes, str: string | bytes)
   * returns the HMAC-SHA256 cyrptographic hash of the given string or bytes
   */
  static hmac_sha256(key, str) {
    return Hash.hmac(Hash.sha256, key, str)
  }

  /**
   * hmac_sha384(key: string | bytes, str: string | bytes)
   * returns the HMAC-SHA384 cyrptographic hash of the given string or bytes
   */
  static hmac_sha384(key, str) {
    return Hash.hmac(Hash.sha384, key, str)
  }

  /**
   * hmac_sha512(key: string | bytes, str: string | bytes)
   * returns the HMAC-SHA512 cyrptographic hash of the given string or bytes
   */
  static hmac_sha512(key, str) {
    return Hash.hmac(Hash.sha512, key, str)
  }

  /**
   * hmac_whirlpool(key: string | bytes, str: string | bytes)
   * returns the HMAC-WHIRLPOOL cyrptographic hash of the given string or bytes
   */
  static hmac_whirlpool(key, str) {
    return Hash.hmac(Hash.whirlpool, key, str)
  }

  /**
   * hmac_snefru(key: string | bytes, str: string | bytes)
   * returns the HMAC-SNEFRU cyrptographic hash of the given string or bytes
   */
  static hmac_snefru(key, str) {
    return Hash.hmac(Hash.snefru, key, str)
  }

  /**
   * hmac_gost(key: string | bytes, str: string | bytes)
   * returns the HMAC-GOST cyrptographic hash of the given string or bytes
   */
  static hmac_gost(key, str) {
    return Hash.hmac(Hash.gost, key, str)
  }
}

