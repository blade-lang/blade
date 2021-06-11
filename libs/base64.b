/**
 * Base64 (RFC1341)
 *
 * Provides interface for encoding and decoding base64 data
 * @copyright 2021, Ore Richard Muyiwa
 */
class Base64 {

  /* 
  The constructor accepts either a string or bytes.
  To encode, you should pass in bytes to the constructor
  and a string for decoding. 
  */
  Base64(data) {
    self.data = data
  }

  # Encodes a bytes into a base64 string
  # @return string
  encode() {
    return self._encode(self.data)
  }

  # Decodes a base64 string into it's corresponding bytes
  # @return bytes
  decode() {
    return self._decode(self.data)
  }
}

