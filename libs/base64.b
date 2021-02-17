/**
 * Base64 (RFC1341)
 *
 * Provides interface for encoding and decoding base64 data
 * @copyright Ore Richard */

 class Base64 {

  /* 
  The constructor accepts either a string or a bytes.
  To encode, you should pass in a bytes to the constructor 
  and a string for decoding. 
  */
  Base64(data) {
    self.data = data
  }

  # Encodes a bytes into a base64 string
  encode() {
    return self._encode(self.data)
  }

  # Decodes a base64 string into it's corresponding bytes
  decode() {
    return self._decode(self.data)
  }
}