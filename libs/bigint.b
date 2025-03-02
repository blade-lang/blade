import math

var _baseNumbers = '0123456789abcdefghijklmnopqrstuvwxyz'

var _zeros = [
  '',
  '0',
  '00',
  '000',
  '0000',
  '00000',
  '000000',
  '0000000',
  '00000000',
  '000000000',
  '0000000000',
  '00000000000',
  '000000000000',
  '0000000000000',
  '00000000000000',
  '000000000000000',
  '0000000000000000',
  '00000000000000000',
  '000000000000000000',
  '0000000000000000000',
  '00000000000000000000',
  '000000000000000000000',
  '0000000000000000000000',
  '00000000000000000000000',
  '000000000000000000000000',
  '0000000000000000000000000',
]

var _groupSizes = [
  0, 0,
  25, 16, 12, 11, 10, 9, 8,
  8, 7, 7, 7, 7, 6, 6,
  6, 6, 6, 6, 6, 5, 5,
  5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5,
]

var _groupBases = [
  0, 0,
  33554432, 43046721, 16777216, 48828125, 60466176, 40353607, 16777216,
  43046721, 10000000, 19487171, 35831808, 62748517, 7529536, 11390625,
  16777216, 24137569, 34012224, 47045881, 64000000, 4084101, 5153632,
  6436343, 7962624, 9765625, 11881376, 14348907, 17210368, 20511149,
  24300000, 28629151, 33554432, 39135393, 45435424, 52521875, 60466176,
]

def _verify(value, message) {
  if !value {
    raise Exception(message or 'Assertion failed')
  }
}

def _imul(x, y) {
  return (x | 0) * (y | 0) | 0
}

def _numberToBase(n, b) {
  var isNeg = n < 0
  if isNeg {
    n = -n
  }

  if n == 0 return '0'
  var digits = []
  while n > 0 {
    digits.append(_baseNumbers[(n % b) // 1])
    n //= b
  }

  return (isNeg? '-' : '') + ''.join(digits.reverse())
}

def _parseHex4Bits(string, index) {
  var c = ord(string[index])

  # '0' - '9'
  if (c >= 48 and c <= 57) {
    return c - 48
  # 'A' - 'F'
  } else if (c >= 65 and c <= 70) {
    return c - 55
  # 'a' - 'f'
  } else if (c >= 97 and c <= 102) {
    return c - 87
  } else {
    assert false, 'Invalid character in ' + string
  }
}

def _parseHexByte(string, lowerBound, index) {
  var r = _parseHex4Bits(string, index)
  if index - 1 >= lowerBound {
    r |= _parseHex4Bits(string, index - 1) << 4
  }

  return r
}

def _parseBase(str, start, end, mul) {
  var r = 0
  var b = 0

  var len = min(str.length(), end)
  iter var i = start; i < len; i++ {
    var c = ord(str[i]) - 48

    r *= mul

    # 'a'
    if c >= 49 {
      b = c - 49 + 0xa

    # 'A'
    } else if c >= 17 {
      b = c - 17 + 0xa

    # '0' - '9'
    } else {
      b = c
    }

    assert c >= 0 and b < mul, 'Invalid character'
    r += b
  }

  return r
}

def _move(dest, src) {
  dest.words = src.words
  dest.length = src.length
  dest.negative = src.negative
  dest.red = src.red
}

def _toBitArray(num) {
  var w = [0] * num.bitLength()

  iter var bit = 0; bit < w.length(); bit++ {
    var off = (bit / 26) | 0
    var wbit = bit % 26

    w[bit] = (num.words[off] >>> wbit) & 0x01
  }

  return w
}

def _smallMulTo(_self, num, out) {
  out.negative = num.negative ^ _self.negative
  var len = (_self.length + num.length) | 0
  out.length = len
  len = (len - 1) | 0

  # Peel one iteration (compiler can't do it, because of code complexity)
  var a = _self.words[0] | 0
  var b = num.words[0] | 0
  var r = a * b

  var lo = r & 0x3ffffff
  var carry = (r / 0x4000000) | 0
  out.words[0] = lo

  var k = 1
  iter ; k < len; k++ {
    # Sum all words with the same `i + j = k` and accumulate `ncarry`,
    # note that ncarry could be >= 0x3ffffff
    var ncarry = carry >>> 26
    var rword = carry & 0x3ffffff
    var maxJ = min(k, num.length - 1)
    iter var j = max(0, k - _self.length + 1); j <= maxJ; j++ {
      var i = (k - j) | 0
      a = _self.words[i] | 0
      b = num.words[j] | 0
      r = a * b + rword
      ncarry += (r / 0x4000000) | 0
      rword = r & 0x3ffffff
    }

    out.words[k] = rword | 0
    carry = ncarry | 0
  }

  if carry != 0 {
    if k < out.words.length() {
      out.words[k] = carry | 0
    } else {
      out.words.insert(carry | 0, k)
    }
  } else {
    out.length--
  }

  return out.strip()
}

def _comb10MulTo(_this, num, out) {
  var a = _this.words
  var b = num.words
  var o = out.words
  var c = 0
  var lo
  var mid
  var hi
  var a0 = a[0] | 0
  var al0 = a0 & 0x1fff
  var ah0 = a0 >>> 13
  var a1 = a[1] | 0
  var al1 = a1 & 0x1fff
  var ah1 = a1 >>> 13
  var a2 = a[2] | 0
  var al2 = a2 & 0x1fff
  var ah2 = a2 >>> 13
  var a3 = a[3] | 0
  var al3 = a3 & 0x1fff
  var ah3 = a3 >>> 13
  var a4 = a[4] | 0
  var al4 = a4 & 0x1fff
  var ah4 = a4 >>> 13
  var a5 = a[5] | 0
  var al5 = a5 & 0x1fff
  var ah5 = a5 >>> 13
  var a6 = a[6] | 0
  var al6 = a6 & 0x1fff
  var ah6 = a6 >>> 13
  var a7 = a[7] | 0
  var al7 = a7 & 0x1fff
  var ah7 = a7 >>> 13
  var a8 = a[8] | 0
  var al8 = a8 & 0x1fff
  var ah8 = a8 >>> 13
  var a9 = a[9] | 0
  var al9 = a9 & 0x1fff
  var ah9 = a9 >>> 13
  var b0 = b[0] | 0
  var bl0 = b0 & 0x1fff
  var bh0 = b0 >>> 13
  var b1 = b[1] | 0
  var bl1 = b1 & 0x1fff
  var bh1 = b1 >>> 13
  var b2 = b[2] | 0
  var bl2 = b2 & 0x1fff
  var bh2 = b2 >>> 13
  var b3 = b[3] | 0
  var bl3 = b3 & 0x1fff
  var bh3 = b3 >>> 13
  var b4 = b[4] | 0
  var bl4 = b4 & 0x1fff
  var bh4 = b4 >>> 13
  var b5 = b[5] | 0
  var bl5 = b5 & 0x1fff
  var bh5 = b5 >>> 13
  var b6 = b[6] | 0
  var bl6 = b6 & 0x1fff
  var bh6 = b6 >>> 13
  var b7 = b[7] | 0
  var bl7 = b7 & 0x1fff
  var bh7 = b7 >>> 13
  var b8 = b[8] | 0
  var bl8 = b8 & 0x1fff
  var bh8 = b8 >>> 13
  var b9 = b[9] | 0
  var bl9 = b9 & 0x1fff
  var bh9 = b9 >>> 13

  out.negative = _this.negative ^ num.negative
  out.length = 19
  /* k = 0 */
  lo = _imul(al0, bl0)
  mid = _imul(al0, bh0)
  mid = (mid + _imul(ah0, bl0)) | 0
  hi = _imul(ah0, bh0)

  var w0 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w0 >>> 26)) | 0
  w0 &= 0x3ffffff
  /* k = 1 */
  lo = _imul(al1, bl0)
  mid = _imul(al1, bh0)
  mid = (mid + _imul(ah1, bl0)) | 0
  hi = _imul(ah1, bh0)
  lo = (lo + _imul(al0, bl1)) | 0
  mid = (mid + _imul(al0, bh1)) | 0
  mid = (mid + _imul(ah0, bl1)) | 0
  hi = (hi + _imul(ah0, bh1)) | 0

  var w1 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w1 >>> 26)) | 0
  w1 &= 0x3ffffff
  /* k = 2 */
  lo = _imul(al2, bl0)
  mid = _imul(al2, bh0)
  mid = (mid + _imul(ah2, bl0)) | 0
  hi = _imul(ah2, bh0)
  lo = (lo + _imul(al1, bl1)) | 0
  mid = (mid + _imul(al1, bh1)) | 0
  mid = (mid + _imul(ah1, bl1)) | 0
  hi = (hi + _imul(ah1, bh1)) | 0
  lo = (lo + _imul(al0, bl2)) | 0
  mid = (mid + _imul(al0, bh2)) | 0
  mid = (mid + _imul(ah0, bl2)) | 0
  hi = (hi + _imul(ah0, bh2)) | 0

  var w2 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w2 >>> 26)) | 0
  w2 &= 0x3ffffff
  /* k = 3 */
  lo = _imul(al3, bl0)
  mid = _imul(al3, bh0)
  mid = (mid + _imul(ah3, bl0)) | 0
  hi = _imul(ah3, bh0)
  lo = (lo + _imul(al2, bl1)) | 0
  mid = (mid + _imul(al2, bh1)) | 0
  mid = (mid + _imul(ah2, bl1)) | 0
  hi = (hi + _imul(ah2, bh1)) | 0
  lo = (lo + _imul(al1, bl2)) | 0
  mid = (mid + _imul(al1, bh2)) | 0
  mid = (mid + _imul(ah1, bl2)) | 0
  hi = (hi + _imul(ah1, bh2)) | 0
  lo = (lo + _imul(al0, bl3)) | 0
  mid = (mid + _imul(al0, bh3)) | 0
  mid = (mid + _imul(ah0, bl3)) | 0
  hi = (hi + _imul(ah0, bh3)) | 0

  var w3 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w3 >>> 26)) | 0
  w3 &= 0x3ffffff
  /* k = 4 */
  lo = _imul(al4, bl0)
  mid = _imul(al4, bh0)
  mid = (mid + _imul(ah4, bl0)) | 0
  hi = _imul(ah4, bh0)
  lo = (lo + _imul(al3, bl1)) | 0
  mid = (mid + _imul(al3, bh1)) | 0
  mid = (mid + _imul(ah3, bl1)) | 0
  hi = (hi + _imul(ah3, bh1)) | 0
  lo = (lo + _imul(al2, bl2)) | 0
  mid = (mid + _imul(al2, bh2)) | 0
  mid = (mid + _imul(ah2, bl2)) | 0
  hi = (hi + _imul(ah2, bh2)) | 0
  lo = (lo + _imul(al1, bl3)) | 0
  mid = (mid + _imul(al1, bh3)) | 0
  mid = (mid + _imul(ah1, bl3)) | 0
  hi = (hi + _imul(ah1, bh3)) | 0
  lo = (lo + _imul(al0, bl4)) | 0
  mid = (mid + _imul(al0, bh4)) | 0
  mid = (mid + _imul(ah0, bl4)) | 0
  hi = (hi + _imul(ah0, bh4)) | 0

  var w4 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w4 >>> 26)) | 0
  w4 &= 0x3ffffff
  /* k = 5 */
  lo = _imul(al5, bl0)
  mid = _imul(al5, bh0)
  mid = (mid + _imul(ah5, bl0)) | 0
  hi = _imul(ah5, bh0)
  lo = (lo + _imul(al4, bl1)) | 0
  mid = (mid + _imul(al4, bh1)) | 0
  mid = (mid + _imul(ah4, bl1)) | 0
  hi = (hi + _imul(ah4, bh1)) | 0
  lo = (lo + _imul(al3, bl2)) | 0
  mid = (mid + _imul(al3, bh2)) | 0
  mid = (mid + _imul(ah3, bl2)) | 0
  hi = (hi + _imul(ah3, bh2)) | 0
  lo = (lo + _imul(al2, bl3)) | 0
  mid = (mid + _imul(al2, bh3)) | 0
  mid = (mid + _imul(ah2, bl3)) | 0
  hi = (hi + _imul(ah2, bh3)) | 0
  lo = (lo + _imul(al1, bl4)) | 0
  mid = (mid + _imul(al1, bh4)) | 0
  mid = (mid + _imul(ah1, bl4)) | 0
  hi = (hi + _imul(ah1, bh4)) | 0
  lo = (lo + _imul(al0, bl5)) | 0
  mid = (mid + _imul(al0, bh5)) | 0
  mid = (mid + _imul(ah0, bl5)) | 0
  hi = (hi + _imul(ah0, bh5)) | 0

  var w5 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w5 >>> 26)) | 0
  w5 &= 0x3ffffff
  /* k = 6 */
  lo = _imul(al6, bl0)
  mid = _imul(al6, bh0)
  mid = (mid + _imul(ah6, bl0)) | 0
  hi = _imul(ah6, bh0)
  lo = (lo + _imul(al5, bl1)) | 0
  mid = (mid + _imul(al5, bh1)) | 0
  mid = (mid + _imul(ah5, bl1)) | 0
  hi = (hi + _imul(ah5, bh1)) | 0
  lo = (lo + _imul(al4, bl2)) | 0
  mid = (mid + _imul(al4, bh2)) | 0
  mid = (mid + _imul(ah4, bl2)) | 0
  hi = (hi + _imul(ah4, bh2)) | 0
  lo = (lo + _imul(al3, bl3)) | 0
  mid = (mid + _imul(al3, bh3)) | 0
  mid = (mid + _imul(ah3, bl3)) | 0
  hi = (hi + _imul(ah3, bh3)) | 0
  lo = (lo + _imul(al2, bl4)) | 0
  mid = (mid + _imul(al2, bh4)) | 0
  mid = (mid + _imul(ah2, bl4)) | 0
  hi = (hi + _imul(ah2, bh4)) | 0
  lo = (lo + _imul(al1, bl5)) | 0
  mid = (mid + _imul(al1, bh5)) | 0
  mid = (mid + _imul(ah1, bl5)) | 0
  hi = (hi + _imul(ah1, bh5)) | 0
  lo = (lo + _imul(al0, bl6)) | 0
  mid = (mid + _imul(al0, bh6)) | 0
  mid = (mid + _imul(ah0, bl6)) | 0
  hi = (hi + _imul(ah0, bh6)) | 0

  var w6 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w6 >>> 26)) | 0
  w6 &= 0x3ffffff
  /* k = 7 */
  lo = _imul(al7, bl0)
  mid = _imul(al7, bh0)
  mid = (mid + _imul(ah7, bl0)) | 0
  hi = _imul(ah7, bh0)
  lo = (lo + _imul(al6, bl1)) | 0
  mid = (mid + _imul(al6, bh1)) | 0
  mid = (mid + _imul(ah6, bl1)) | 0
  hi = (hi + _imul(ah6, bh1)) | 0
  lo = (lo + _imul(al5, bl2)) | 0
  mid = (mid + _imul(al5, bh2)) | 0
  mid = (mid + _imul(ah5, bl2)) | 0
  hi = (hi + _imul(ah5, bh2)) | 0
  lo = (lo + _imul(al4, bl3)) | 0
  mid = (mid + _imul(al4, bh3)) | 0
  mid = (mid + _imul(ah4, bl3)) | 0
  hi = (hi + _imul(ah4, bh3)) | 0
  lo = (lo + _imul(al3, bl4)) | 0
  mid = (mid + _imul(al3, bh4)) | 0
  mid = (mid + _imul(ah3, bl4)) | 0
  hi = (hi + _imul(ah3, bh4)) | 0
  lo = (lo + _imul(al2, bl5)) | 0
  mid = (mid + _imul(al2, bh5)) | 0
  mid = (mid + _imul(ah2, bl5)) | 0
  hi = (hi + _imul(ah2, bh5)) | 0
  lo = (lo + _imul(al1, bl6)) | 0
  mid = (mid + _imul(al1, bh6)) | 0
  mid = (mid + _imul(ah1, bl6)) | 0
  hi = (hi + _imul(ah1, bh6)) | 0
  lo = (lo + _imul(al0, bl7)) | 0
  mid = (mid + _imul(al0, bh7)) | 0
  mid = (mid + _imul(ah0, bl7)) | 0
  hi = (hi + _imul(ah0, bh7)) | 0

  var w7 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w7 >>> 26)) | 0
  w7 &= 0x3ffffff
  /* k = 8 */
  lo = _imul(al8, bl0)
  mid = _imul(al8, bh0)
  mid = (mid + _imul(ah8, bl0)) | 0
  hi = _imul(ah8, bh0)
  lo = (lo + _imul(al7, bl1)) | 0
  mid = (mid + _imul(al7, bh1)) | 0
  mid = (mid + _imul(ah7, bl1)) | 0
  hi = (hi + _imul(ah7, bh1)) | 0
  lo = (lo + _imul(al6, bl2)) | 0
  mid = (mid + _imul(al6, bh2)) | 0
  mid = (mid + _imul(ah6, bl2)) | 0
  hi = (hi + _imul(ah6, bh2)) | 0
  lo = (lo + _imul(al5, bl3)) | 0
  mid = (mid + _imul(al5, bh3)) | 0
  mid = (mid + _imul(ah5, bl3)) | 0
  hi = (hi + _imul(ah5, bh3)) | 0
  lo = (lo + _imul(al4, bl4)) | 0
  mid = (mid + _imul(al4, bh4)) | 0
  mid = (mid + _imul(ah4, bl4)) | 0
  hi = (hi + _imul(ah4, bh4)) | 0
  lo = (lo + _imul(al3, bl5)) | 0
  mid = (mid + _imul(al3, bh5)) | 0
  mid = (mid + _imul(ah3, bl5)) | 0
  hi = (hi + _imul(ah3, bh5)) | 0
  lo = (lo + _imul(al2, bl6)) | 0
  mid = (mid + _imul(al2, bh6)) | 0
  mid = (mid + _imul(ah2, bl6)) | 0
  hi = (hi + _imul(ah2, bh6)) | 0
  lo = (lo + _imul(al1, bl7)) | 0
  mid = (mid + _imul(al1, bh7)) | 0
  mid = (mid + _imul(ah1, bl7)) | 0
  hi = (hi + _imul(ah1, bh7)) | 0
  lo = (lo + _imul(al0, bl8)) | 0
  mid = (mid + _imul(al0, bh8)) | 0
  mid = (mid + _imul(ah0, bl8)) | 0
  hi = (hi + _imul(ah0, bh8)) | 0

  var w8 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w8 >>> 26)) | 0
  w8 &= 0x3ffffff
  /* k = 9 */
  lo = _imul(al9, bl0)
  mid = _imul(al9, bh0)
  mid = (mid + _imul(ah9, bl0)) | 0
  hi = _imul(ah9, bh0)
  lo = (lo + _imul(al8, bl1)) | 0
  mid = (mid + _imul(al8, bh1)) | 0
  mid = (mid + _imul(ah8, bl1)) | 0
  hi = (hi + _imul(ah8, bh1)) | 0
  lo = (lo + _imul(al7, bl2)) | 0
  mid = (mid + _imul(al7, bh2)) | 0
  mid = (mid + _imul(ah7, bl2)) | 0
  hi = (hi + _imul(ah7, bh2)) | 0
  lo = (lo + _imul(al6, bl3)) | 0
  mid = (mid + _imul(al6, bh3)) | 0
  mid = (mid + _imul(ah6, bl3)) | 0
  hi = (hi + _imul(ah6, bh3)) | 0
  lo = (lo + _imul(al5, bl4)) | 0
  mid = (mid + _imul(al5, bh4)) | 0
  mid = (mid + _imul(ah5, bl4)) | 0
  hi = (hi + _imul(ah5, bh4)) | 0
  lo = (lo + _imul(al4, bl5)) | 0
  mid = (mid + _imul(al4, bh5)) | 0
  mid = (mid + _imul(ah4, bl5)) | 0
  hi = (hi + _imul(ah4, bh5)) | 0
  lo = (lo + _imul(al3, bl6)) | 0
  mid = (mid + _imul(al3, bh6)) | 0
  mid = (mid + _imul(ah3, bl6)) | 0
  hi = (hi + _imul(ah3, bh6)) | 0
  lo = (lo + _imul(al2, bl7)) | 0
  mid = (mid + _imul(al2, bh7)) | 0
  mid = (mid + _imul(ah2, bl7)) | 0
  hi = (hi + _imul(ah2, bh7)) | 0
  lo = (lo + _imul(al1, bl8)) | 0
  mid = (mid + _imul(al1, bh8)) | 0
  mid = (mid + _imul(ah1, bl8)) | 0
  hi = (hi + _imul(ah1, bh8)) | 0
  lo = (lo + _imul(al0, bl9)) | 0
  mid = (mid + _imul(al0, bh9)) | 0
  mid = (mid + _imul(ah0, bl9)) | 0
  hi = (hi + _imul(ah0, bh9)) | 0

  var w9 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w9 >>> 26)) | 0
  w9 &= 0x3ffffff
  /* k = 10 */
  lo = _imul(al9, bl1)
  mid = _imul(al9, bh1)
  mid = (mid + _imul(ah9, bl1)) | 0
  hi = _imul(ah9, bh1)
  lo = (lo + _imul(al8, bl2)) | 0
  mid = (mid + _imul(al8, bh2)) | 0
  mid = (mid + _imul(ah8, bl2)) | 0
  hi = (hi + _imul(ah8, bh2)) | 0
  lo = (lo + _imul(al7, bl3)) | 0
  mid = (mid + _imul(al7, bh3)) | 0
  mid = (mid + _imul(ah7, bl3)) | 0
  hi = (hi + _imul(ah7, bh3)) | 0
  lo = (lo + _imul(al6, bl4)) | 0
  mid = (mid + _imul(al6, bh4)) | 0
  mid = (mid + _imul(ah6, bl4)) | 0
  hi = (hi + _imul(ah6, bh4)) | 0
  lo = (lo + _imul(al5, bl5)) | 0
  mid = (mid + _imul(al5, bh5)) | 0
  mid = (mid + _imul(ah5, bl5)) | 0
  hi = (hi + _imul(ah5, bh5)) | 0
  lo = (lo + _imul(al4, bl6)) | 0
  mid = (mid + _imul(al4, bh6)) | 0
  mid = (mid + _imul(ah4, bl6)) | 0
  hi = (hi + _imul(ah4, bh6)) | 0
  lo = (lo + _imul(al3, bl7)) | 0
  mid = (mid + _imul(al3, bh7)) | 0
  mid = (mid + _imul(ah3, bl7)) | 0
  hi = (hi + _imul(ah3, bh7)) | 0
  lo = (lo + _imul(al2, bl8)) | 0
  mid = (mid + _imul(al2, bh8)) | 0
  mid = (mid + _imul(ah2, bl8)) | 0
  hi = (hi + _imul(ah2, bh8)) | 0
  lo = (lo + _imul(al1, bl9)) | 0
  mid = (mid + _imul(al1, bh9)) | 0
  mid = (mid + _imul(ah1, bl9)) | 0
  hi = (hi + _imul(ah1, bh9)) | 0

  var w10 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w10 >>> 26)) | 0
  w10 &= 0x3ffffff
  /* k = 11 */
  lo = _imul(al9, bl2)
  mid = _imul(al9, bh2)
  mid = (mid + _imul(ah9, bl2)) | 0
  hi = _imul(ah9, bh2)
  lo = (lo + _imul(al8, bl3)) | 0
  mid = (mid + _imul(al8, bh3)) | 0
  mid = (mid + _imul(ah8, bl3)) | 0
  hi = (hi + _imul(ah8, bh3)) | 0
  lo = (lo + _imul(al7, bl4)) | 0
  mid = (mid + _imul(al7, bh4)) | 0
  mid = (mid + _imul(ah7, bl4)) | 0
  hi = (hi + _imul(ah7, bh4)) | 0
  lo = (lo + _imul(al6, bl5)) | 0
  mid = (mid + _imul(al6, bh5)) | 0
  mid = (mid + _imul(ah6, bl5)) | 0
  hi = (hi + _imul(ah6, bh5)) | 0
  lo = (lo + _imul(al5, bl6)) | 0
  mid = (mid + _imul(al5, bh6)) | 0
  mid = (mid + _imul(ah5, bl6)) | 0
  hi = (hi + _imul(ah5, bh6)) | 0
  lo = (lo + _imul(al4, bl7)) | 0
  mid = (mid + _imul(al4, bh7)) | 0
  mid = (mid + _imul(ah4, bl7)) | 0
  hi = (hi + _imul(ah4, bh7)) | 0
  lo = (lo + _imul(al3, bl8)) | 0
  mid = (mid + _imul(al3, bh8)) | 0
  mid = (mid + _imul(ah3, bl8)) | 0
  hi = (hi + _imul(ah3, bh8)) | 0
  lo = (lo + _imul(al2, bl9)) | 0
  mid = (mid + _imul(al2, bh9)) | 0
  mid = (mid + _imul(ah2, bl9)) | 0
  hi = (hi + _imul(ah2, bh9)) | 0

  var w11 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w11 >>> 26)) | 0
  w11 &= 0x3ffffff
  /* k = 12 */
  lo = _imul(al9, bl3)
  mid = _imul(al9, bh3)
  mid = (mid + _imul(ah9, bl3)) | 0
  hi = _imul(ah9, bh3)
  lo = (lo + _imul(al8, bl4)) | 0
  mid = (mid + _imul(al8, bh4)) | 0
  mid = (mid + _imul(ah8, bl4)) | 0
  hi = (hi + _imul(ah8, bh4)) | 0
  lo = (lo + _imul(al7, bl5)) | 0
  mid = (mid + _imul(al7, bh5)) | 0
  mid = (mid + _imul(ah7, bl5)) | 0
  hi = (hi + _imul(ah7, bh5)) | 0
  lo = (lo + _imul(al6, bl6)) | 0
  mid = (mid + _imul(al6, bh6)) | 0
  mid = (mid + _imul(ah6, bl6)) | 0
  hi = (hi + _imul(ah6, bh6)) | 0
  lo = (lo + _imul(al5, bl7)) | 0
  mid = (mid + _imul(al5, bh7)) | 0
  mid = (mid + _imul(ah5, bl7)) | 0
  hi = (hi + _imul(ah5, bh7)) | 0
  lo = (lo + _imul(al4, bl8)) | 0
  mid = (mid + _imul(al4, bh8)) | 0
  mid = (mid + _imul(ah4, bl8)) | 0
  hi = (hi + _imul(ah4, bh8)) | 0
  lo = (lo + _imul(al3, bl9)) | 0
  mid = (mid + _imul(al3, bh9)) | 0
  mid = (mid + _imul(ah3, bl9)) | 0
  hi = (hi + _imul(ah3, bh9)) | 0

  var w12 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w12 >>> 26)) | 0
  w12 &= 0x3ffffff
  /* k = 13 */
  lo = _imul(al9, bl4)
  mid = _imul(al9, bh4)
  mid = (mid + _imul(ah9, bl4)) | 0
  hi = _imul(ah9, bh4)
  lo = (lo + _imul(al8, bl5)) | 0
  mid = (mid + _imul(al8, bh5)) | 0
  mid = (mid + _imul(ah8, bl5)) | 0
  hi = (hi + _imul(ah8, bh5)) | 0
  lo = (lo + _imul(al7, bl6)) | 0
  mid = (mid + _imul(al7, bh6)) | 0
  mid = (mid + _imul(ah7, bl6)) | 0
  hi = (hi + _imul(ah7, bh6)) | 0
  lo = (lo + _imul(al6, bl7)) | 0
  mid = (mid + _imul(al6, bh7)) | 0
  mid = (mid + _imul(ah6, bl7)) | 0
  hi = (hi + _imul(ah6, bh7)) | 0
  lo = (lo + _imul(al5, bl8)) | 0
  mid = (mid + _imul(al5, bh8)) | 0
  mid = (mid + _imul(ah5, bl8)) | 0
  hi = (hi + _imul(ah5, bh8)) | 0
  lo = (lo + _imul(al4, bl9)) | 0
  mid = (mid + _imul(al4, bh9)) | 0
  mid = (mid + _imul(ah4, bl9)) | 0
  hi = (hi + _imul(ah4, bh9)) | 0

  var w13 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w13 >>> 26)) | 0
  w13 &= 0x3ffffff
  /* k = 14 */
  lo = _imul(al9, bl5)
  mid = _imul(al9, bh5)
  mid = (mid + _imul(ah9, bl5)) | 0
  hi = _imul(ah9, bh5)
  lo = (lo + _imul(al8, bl6)) | 0
  mid = (mid + _imul(al8, bh6)) | 0
  mid = (mid + _imul(ah8, bl6)) | 0
  hi = (hi + _imul(ah8, bh6)) | 0
  lo = (lo + _imul(al7, bl7)) | 0
  mid = (mid + _imul(al7, bh7)) | 0
  mid = (mid + _imul(ah7, bl7)) | 0
  hi = (hi + _imul(ah7, bh7)) | 0
  lo = (lo + _imul(al6, bl8)) | 0
  mid = (mid + _imul(al6, bh8)) | 0
  mid = (mid + _imul(ah6, bl8)) | 0
  hi = (hi + _imul(ah6, bh8)) | 0
  lo = (lo + _imul(al5, bl9)) | 0
  mid = (mid + _imul(al5, bh9)) | 0
  mid = (mid + _imul(ah5, bl9)) | 0
  hi = (hi + _imul(ah5, bh9)) | 0

  var w14 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w14 >>> 26)) | 0
  w14 &= 0x3ffffff
  /* k = 15 */
  lo = _imul(al9, bl6)
  mid = _imul(al9, bh6)
  mid = (mid + _imul(ah9, bl6)) | 0
  hi = _imul(ah9, bh6)
  lo = (lo + _imul(al8, bl7)) | 0
  mid = (mid + _imul(al8, bh7)) | 0
  mid = (mid + _imul(ah8, bl7)) | 0
  hi = (hi + _imul(ah8, bh7)) | 0
  lo = (lo + _imul(al7, bl8)) | 0
  mid = (mid + _imul(al7, bh8)) | 0
  mid = (mid + _imul(ah7, bl8)) | 0
  hi = (hi + _imul(ah7, bh8)) | 0
  lo = (lo + _imul(al6, bl9)) | 0
  mid = (mid + _imul(al6, bh9)) | 0
  mid = (mid + _imul(ah6, bl9)) | 0
  hi = (hi + _imul(ah6, bh9)) | 0

  var w15 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w15 >>> 26)) | 0
  w15 &= 0x3ffffff
  /* k = 16 */
  lo = _imul(al9, bl7)
  mid = _imul(al9, bh7)
  mid = (mid + _imul(ah9, bl7)) | 0
  hi = _imul(ah9, bh7)
  lo = (lo + _imul(al8, bl8)) | 0
  mid = (mid + _imul(al8, bh8)) | 0
  mid = (mid + _imul(ah8, bl8)) | 0
  hi = (hi + _imul(ah8, bh8)) | 0
  lo = (lo + _imul(al7, bl9)) | 0
  mid = (mid + _imul(al7, bh9)) | 0
  mid = (mid + _imul(ah7, bl9)) | 0
  hi = (hi + _imul(ah7, bh9)) | 0

  var w16 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w16 >>> 26)) | 0
  w16 &= 0x3ffffff
  /* k = 17 */
  lo = _imul(al9, bl8)
  mid = _imul(al9, bh8)
  mid = (mid + _imul(ah9, bl8)) | 0
  hi = _imul(ah9, bh8)
  lo = (lo + _imul(al8, bl9)) | 0
  mid = (mid + _imul(al8, bh9)) | 0
  mid = (mid + _imul(ah8, bl9)) | 0
  hi = (hi + _imul(ah8, bh9)) | 0

  var w17 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w17 >>> 26)) | 0
  w17 &= 0x3ffffff
  /* k = 18 */
  lo = _imul(al9, bl9)
  mid = _imul(al9, bh9)
  mid = (mid + _imul(ah9, bl9)) | 0
  hi = _imul(ah9, bh9)
  
  var w18 = (((c + lo) | 0) + ((mid & 0x1fff) << 13)) | 0
  c = (((hi + (mid >>> 13)) | 0) + (w18 >>> 26)) | 0
  w18 &= 0x3ffffff
  o[0] = w0
  o[1] = w1
  o[2] = w2
  o[3] = w3
  o[4] = w4
  o[5] = w5
  o[6] = w6
  o[7] = w7
  o[8] = w8
  o[9] = w9
  o[10] = w10
  o[11] = w11
  o[12] = w12
  o[13] = w13
  o[14] = w14
  o[15] = w15
  o[16] = w16
  o[17] = w17
  o[18] = w18
  if c != 0 {
    o[19] = c
    out.length++
  }
  return out
}

def _bigMulTo(_this, num, out) {
  out.negative = num.negative ^ _this.negative
  out.length = _this.length + num.length

  var carry = 0
  var hncarry = 0
  var k = 0
  iter ; k < out.length - 1; k++ {
    # Sum all words with the same `i + j = k` and accumulate `ncarry`,
    # note that ncarry could be >= 0x3ffffff
    var ncarry = hncarry
    hncarry = 0
    var rword = carry & 0x3ffffff
    var maxJ = min(k, num.length - 1)

    iter var j = max(0, k - _this.length + 1); j <= maxJ; j++ {
      var i = k - j
      var a = _this.words[i] | 0
      var b = num.words[j] | 0
      var r = a * b

      var lo = r & 0x3ffffff
      ncarry = (ncarry + ((r / 0x4000000) | 0)) | 0
      lo = (lo + rword) | 0
      rword = lo & 0x3ffffff
      ncarry = (ncarry + (lo >>> 26)) | 0

      hncarry += ncarry >>> 26
      ncarry &= 0x3ffffff
    }

    out.words[k] = rword
    carry = ncarry
    ncarry = hncarry
  }

  if carry != 0 {
    if k < out.words.length() {
      out.words[k] = carry
    } else {
      out.words.insert(carry, k)
    }
  } else {
    out.length--
  }

  return out.strip()
}

def _jumboMulTo(_this, num, out) {
  return _bigMulTo(_this, num, out)
}


/**
 * BigInt class represent integer values which are too high or too low 
 * to be represented by the number primitive. They behave just like 
 * numbers and implement all numeric operators except the unsigned right 
 * shift (>>>). For this reason, they work in all contexts that numbers 
 * can be used. For example, the following code is completely valid:
 * 
 * ```blade-repl
 * %> import bigint
 * %> 
 * %> abs(bigint('-450'))
 * '<BigInt v=450>'
 * ```
 * 
 * As said earlier, BigInt defines arithemetic operations.
 * 
 * ```blade-repl
 * %> bigint('72672676789679863767863976783') * bigint('3679687870387890379')
 * '<BigInt v=267412767291604568032754996823951848184805070757>'
 * ```
 * 
 * Even bitwise operations
 * 
 * ```blade-repl
 * %> bigint('72672676789679863767863976783') & bigint('3679687870387890379')
 * '<BigInt v=72128620998467659>'
 * ```
 * 
 * While this class exports a lot of functions, it is preferred that the 
 * arithemetic operators be used more as they mask the underlying naunces and 
 * will feel and look more natural.
 * 
 * @printable
 * @serializable
 * @numeric
 */
class BigInt {

  var wordSize = 26

  /**
   * @constructor
   */
  BigInt(number, base, endian) {
    if BigInt.isBigInt(number) {
      return
    }

    self.negative = 0
    self.words = nil
    self.length = 0

    # Reduction context
    self.red = nil

    if number != nil {
      if base == 'le' or base == 'be' {
        endian = base
        base = 10
      }

      self._init(
        number == nil ? 0 : number, 
        base or 10, 
        endian or 'be'
      )
    }
  }

  static isBigInt(num) {
    if is_instance(num) and instance_of(num, BigInt) {
      return true
    }

    return false
  }

  static max(left, right) {
    if !BigInt.isBigInt(left) or !BigInt.isBigInt(right) {
      raise Exception('BigInt.max expects both numbers as BigInt')
    }

    if left.cmp(right) > 0 return left
    return right
  }

  static min(left, right) {
    if !BigInt.isBigInt(left) or !BigInt.isBigInt(right) {
      raise Exception('BigInt.min expects both numbers as BigInt')
    }

    if left.cmp(right) < 0 return left
    return right
  }

  _init(number, base, endian) {
    if is_number(number) {
      return self._initNumber(number, base, endian)
    } /* else if is_list(number) {
      return self._initArray(number, base, endian)
    } */

    if base == 'hex' {
      base = 16
    }

    assert base == (base | 0) and base >= 2 and base <= 36,
      'BigInt base out of range'

    number = to_string(number).replace('/\s+/', '')
    assert number.length() > 0, 'Invalid BigInt value'

    var start = 0
    if number[0] == '-' {
      start++
      self.negative = 1
    }

    if start < number.length() {
      if base == 16 {
        self._parseHex(number, start, endian)
      } else {
        self._parseBase(number, base, start)
        if endian == 'le' {
          self._initArray(self.toList(), base, endian)
        }
      }
    }
  }

  _initNumber(number, base, endian) {
    if number < 0 {
      self.negative = 1
      number = -number
    }

    if number < 0x4000000 {
      self.words = [number & 0x3ffffff]
      self.length = 1
    } else if number < 0x10000000000000 {
      self.words = [
        number & 0x3ffffff,
        (number / 0x4000000) & 0x3ffffff,
      ]
      self.length = 2
    } else {
      assert number < 0x20000000000000, 'Unsafe BigInt number' # 2 ^ 53 (unsafe)
      self.words = [
        number & 0x3ffffff,
        (number / 0x4000000) & 0x3ffffff,
        1,
      ]
      self.length = 3
    }

    if endian != 'le' return

    # Reverse the bytes
    self._initArray(self.toList(), base, endian)
  }

  _initArray(number, base, endian) {
    # Perhaps a Uint8Array
    assert is_number(number.length())

    if number.length() <= 0 {
      self.words = [0]
      self.length = 1
      return self
    }

    self.length = math.ceil(number.length() / 3)
    self.words = [0] * self.length

    var j, w
    var off = 0
    if endian == 'be' {
      iter var i = number.length() - 1, j = 0; i >= 0; i -= 3 {
        w = number[i] | (number[i - 1] << 8) | (number[i - 2] << 16)
        self.words[j] |= (w << off) & 0x3ffffff

        if j + 1 < self.words.length() {
          self.words[j + 1] = (w >>> (26 - off)) & 0x3ffffff
        } else {
          self.words.insert((w >>> (26 - off)) & 0x3ffffff, j + 1)
        }

        off += 24
        if off >= 26 {
          off -= 26
          j++
        }
      }
    } else if endian == 'le' {
      iter var i = 0, j = 0; i < number.length(); i += 3 {
        var n1 = i + 1 < number.length() ? number[i + 1] : 0
        var n2 = i + 2 < number.length() ? number[i + 2] : 0

        w = number[i] | (n1 << 8) | (n2 << 16)
        self.words[j] |= (w << off) & 0x3ffffff

        if j + 1 < self.words.length() {
          self.words[j + 1] = (w >>> (26 - off)) & 0x3ffffff
        } else {
          self.words.insert((w >>> (26 - off)) & 0x3ffffff, j + 1)
        }
        
        off += 24
        if off >= 26 {
          off -= 26
          j++
        }
      }
    }

    return self.strip()
  }

  _parseHex(number, start, endian) {
    # Create possibly bigger array to ensure that it fits the number
    self.length = math.ceil((number.length() - start) / 6)
    self.words = [0] * self.length

    # 24-bits chunks
    var off = 0
    var j = 0

    var w
    if endian == 'be' {
      iter var i = number.length() - 1; i >= start; i -= 2 {
        w = _parseHexByte(number, start, i) << off
        self.words[j] |= w & 0x3ffffff
        if off >= 18 {
          off -= 18
          j += 1
          self.words[j] |= w >>> 26
        } else {
          off += 8
        }
      }
    } else {
      var parseLength = number.length() - start
      iter var i = parseLength % 2 == 0 ? start + 1 : start; i < number.length(); i += 2 {
        w = _parseHexByte(number, start, i) << off
        self.words[j] |= w & 0x3ffffff
        if off >= 18 {
          off -= 18
          j += 1
          self.words[j] |= w >>> 26
        } else {
          off += 8
        }
      }
    }

    self.strip()
  }

  _parseBase(number, base, start) {
    # Initialize as zero
    self.words = [0]
    self.length = 1

    # Find length of limb in base
    var limbLen = 0, limbPow = 1
    iter ; limbPow <= 0x3ffffff; limbPow *= base {
      limbLen++
    }
    limbLen--
    limbPow = (limbPow / base) | 0

    var total = number.length() - start
    var mod = total % limbLen
    var end = min(total, total - mod) + start

    var word = 0
    var i = start
    iter ; i < end; i += limbLen {
      word = _parseBase(number, i, i + limbLen, base)

      self.imuln(limbPow)
      if self.words[0] + word < 0x4000000 {
        self.words[0] += word
      } else {
        self._iaddn(word)
      }
    }

    if mod != 0 {
      var pow = 1
      word = _parseBase(number, i, number.length(), base)

      iter var i = 0; i < mod; i++ {
        pow *= base
      }

      self.imuln(pow)
      if self.words[0] + word < 0x4000000 {
        self.words[0] += word
      } else {
        self._iaddn(word)
      }
    }

    self.strip()
  }

  copy(dest) {
    dest.words = self.words.clone()
    dest.length = self.length
    dest.negative = self.negative
    dest.red = self.red
  }

  _move(dest) {
    _move(dest, self)
  }

  /**
   * Clones the BigInt into a new instance
   * 
   * @returns [[bigint.BigInt]]
   */
  clone() {
    var r = BigInt(nil)
    self.copy(r)
    return r
  }

  _expand(size) {
    while self.length < size {
      var i = self.length++ - 1
      if i < self.words.length() {
        self.words[i] = 0
      } else {
        self.words.insert(0, i)
      }
    }

    return self
  }

  strip() {
    while self.length > 1 and self.words[self.length - 1] == 0 {
      self.length--
    }

    return self.normSign()
  }

  normSign () {
    # -0 = 0
    if self.length == 1 and self.words[0] == 0 {
      self.negative = 0
    }

    return self
  }

  /**
   * Convert the number to a string in the given base padded with 
   * the given number of zeroes. If the base is omitted or nil, a 
   * default value of `10` will be set and if the padding is 
   * omitted or nil, it will be ignored altogether.
   * 
   * @param {number} base: Default = `10`
   * @param {number} padding: Default = `1`
   * @returns string
   */
  toString(base, padding) {
    base = base or 10
    padding = padding ? (padding | 0 or 1) : 1

    if base != 'hex' and (!is_number(base) or !((base | 0) and base >= 2 and base <= 36)) {
      raise Exception("base should be between 2 and 36 or the string 'hex'")
    } else if !is_number(padding) {
      raise Exception('padding must be a number')
    }

    var out
    if base == 16 or base == 'hex' {
      out = ''
      var off = 0
      var carry = 0

      iter var i = 0; i < self.length; i++ {
        var w = self.words[i]
        var word = hex((((w << off) | carry) & 0xffffff))
        carry = (w >>> (24 - off)) & 0xffffff
        off += 2

        if off >= 26 {
          off -= 26
          i--
        }

        if carry != 0 or i != self.length - 1 {
          out = _zeros[6 - word.length()] + word + out
        } else {
          out = word + out
        }
      }

      if carry != 0 {
        out = hex(carry) + out
      }

      while out.length() % padding != 0 {
        out = '0' + out
      }

      if self.negative != 0 {
        out = '-' + out
      }

      return out
    }

    # var groupSize = (self.wordSize * math.LOG_2_E / math.log(base)) // 1
    var groupSize = _groupSizes[base]
    # var groupBase = base ** groupSize
    var groupBase = _groupBases[base]
    out = ''
    var c = self.clone()
    c.negative = 0
    while !c.isZero() {
      var r = _numberToBase(c.modrn(groupBase), base)
      c = c.idivn(groupBase)

      if !c.isZero() {
        out = _zeros[groupSize - r.length()] + r + out
      } else {
        out = r + out
      }
    }

    if (self.isZero()) {
      out = '0' + out
    }

    while out.length() % padding != 0 {
      out = '0' + out
    }

    if self.negative != 0 {
      out = '-' + out
    }

    return out
  }

  /**
   * Converts the BigInt to a standard Blade number which has a 
   * precision of up to limited to 53 bits.
   * 
   * @return number
   */
  toNumber() {
    var ret = self.words[0]
    if self.length == 2 {
      ret += self.words[1] * 0x4000000
    } else if self.length == 3 and self.words[2] == 0x01 {
      # NOTE: at self stage it is known that the top bit is set
      ret += 0x10000000000000 + (self.words[1] * 0x4000000)
    } else if self.length > 2 {
      assert false, 'Number can only safely store up to 53 bits'
    }

    return (self.negative != 0) ? -ret : ret
  }

  /**
   * Converts to number to a Json compatible hex string.
   * 
   * @returns string
   */
  toJSON() {
    return self.toString(16, 2)
  }

  /**
   * Converts the number to a byte list and optionally zero pad 
   * to length; throwing if already exceeding.
   * 
   * @param {string} endian: One of `le` or `be`
   * @param {number} length
   * @returns list
   */
  toList(endian, length) {
    self.strip()

    var byteLength = self.byteLength()
    var reqLength = length or max(1, byteLength)

    assert byteLength <= reqLength, 'byte array longer than desired length'
    assert reqLength > 0, 'Requested array length <= 0'

    var res = [0] * reqLength
    if endian == 'le' {
      self._toListLikeLE(res, byteLength)
    } else {
      self._toListLikeBE(res, byteLength)
    }

    return res
  }

  _toListLikeLE(res, byteLength) {
    var position = 0
    var carry = 0

    iter var i = 0, shift = 0; i < self.length; i++ {
      var word = (self.words[i] << shift) | carry

      res[position++ - 1] = word & 0xff
      if position < res.length() {
        res[position++ - 1] = (word >> 8) & 0xff
      }
      if position < res.length() {
        res[position++ - 1] = (word >> 16) & 0xff
      }

      if shift == 6 {
        if position < res.length() {
          res[position++ - 1] = (word >> 24) & 0xff
        }

        carry = 0
        shift = 0
      } else {
        carry = word >>> 24
        shift += 2
      }
    }

    if position < res.length() {
      res[position++ - 1] = carry

      while position < res.length() {
        res[position++ - 1] = 0
      }
    }
  }

  _toListLikeBE(res, byteLength) {
    var position = res.length() - 1
    var carry = 0

    iter var i = 0, shift = 0; i < self.length; i++ {
      var word = (self.words[i] << shift) | carry

      res[position-- + 1] = word & 0xff
      if position >= 0 {
        res[position-- + 1] = (word >> 8) & 0xff
      }
      if position >= 0 {
        res[position-- + 1] = (word >> 16) & 0xff
      }

      if shift == 6 {
        if position >= 0 {
          res[position-- + 1] = (word >> 24) & 0xff
        }

        carry = 0
        shift = 0
      } else {
        carry = word >>> 24
        shift += 2
      }
    }

    if position >= 0 {
      res[position-- + 1] = carry

      while position >= 0 {
        res[position-- + 1] = 0
      }
    }
  }

  _countBits(w) {
    var t = w
    var r = 0
    if t >= 0x1000 {
      r += 13
      t >>>= 13
    }

    if t >= 0x40 {
      r += 7
      t >>>= 7
    }

    if t >= 0x8 {
      r += 4
      t >>>= 4
    }

    if t >= 0x02 {
      r += 2
      t >>>= 2
    }

    return r + t
  }

  _zeroBits(w) {
    # Short-cut
    if w == 0 return 26

    var t = w
    var r = 0
    if (t & 0x1fff) == 0 {
      r += 13
      t >>>= 13
    }

    if (t & 0x7f) == 0 {
      r += 7
      t >>>= 7
    }

    if (t & 0xf) == 0 {
      r += 4
      t >>>= 4
    }

    if (t & 0x3) == 0 {
      r += 2
      t >>>= 2
    }

    if (t & 0x1) == 0 {
      r++
    }

    return r
  }

  /**
   * Returns the number of bits occupied by the bigint
   * 
   * @returns number
   */
  bitLength () {
    var w = self.words[self.length - 1]
    var hi = self._countBits(w)
    return (self.length - 1) * 26 + hi
  }

  /**
   * Returns number of less-significant consequent zero bits 
   * (example: 1010000 has 4 zero bits)
   * 
   * @returns number
   */
  zeroBits() {
    if self.isZero() return 0

    var r = 0
    iter var i = 0; i < self.length; i++ {
      var b = self._zeroBits(self.words[i])
      r += b
      if b != 26 break
    }

    return r
  }

  /**
   * Returns the number of bytes occupied by the bigint
   * 
   * @returns number
   */
  byteLength() {
    return math.ceil(self.bitLength() / 8)
  }

  /**
   * Converts the number to two's complement representation of the given bit _width_.
   * 
   * @param number width
   * @returns [[bigint.BigInt]]
   */
  toTwos(width) {
    if self.negative != 0 {
      return self.abs().inotn(width).iaddn(1)
    }

    return self.clone()
  }

  /**
   * Convert from two's complement representation in the give _width_ to a BigInt number.
   * 
   * @param number width
   * @returns [[bigint.BigInt]]
   */
  fromTwos(width) {
    if self.testn(width - 1) {
      return self.notn(width).iaddn(1).ineg()
    }

    return self.clone()
  }

  /**
   * Returns `true` if the number is negative, returns `false` otherwise.
   * 
   * @returns bool
   */
  isNeg() {
    return self.negative != 0
  }

  /**
   * Negates the sign on the number.
   * 
   * @returns [[bigint.BigInt]]
   */
  neg() {
    return self.clone().ineg()
  }

  /**
   * The in-place version of [[bigint.BigInt.neg]]
   */
  ineg () {
    if !self.isZero() {
      self.negative ^= 1
    }

    return self
  }

  /**
   * The in-place and unsigned version of [[bigint.BigInt.or_]]
   * 
   * @returns self
   */
  iuor(num) {
    while self.length < num.length {
      var i = self.length++ - 1
      if i < self.words.length() {
        self.words[i] = 0
      } else {
        self.words.insert(0, i)
      }
    }

    iter var i = 0; i < num.length; i++ {
      self.words[i] = self.words[i] | num.words[i]
    }

    return self.strip()
  }

  /**
   * The in-place version of [[bigint.BigInt.or_]]
   * 
   * @returns self
   */
  ior(num) {
    assert (self.negative | num.negative) == 0
    return self.iuor(num)
  }

  /**
   * Performs a bitwise OR operation on the number and the given one.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  or_(num) {
    if self.length > num.length {
      return self.clone().ior(num)
    }

    return num.clone().ior(self)
  }

  /**
   * The unsigned version of [[bigint.BigInt.or_]]
   */
  uor(num) {
    if self.length > num.length {
      return self.clone().iuor(num)
    }
    
    return num.clone().iuor(self)
  }

  /**
   * The in-place and unsigned version of [[bigint.BigInt.and_]]
   * 
   * @returns self
   */
  iuand(num) {
    # b = min-length(num, self)
    var b
    if self.length > num.length {
      b = num
    } else {
      b = self
    }

    iter var i = 0; i < b.length; i++ {
      self.words[i] = self.words[i] & num.words[i]
    }

    self.length = b.length

    return self.strip()
  }

  /**
   * The in-place version of [[bigint.BigInt.and_]]
   * 
   * @returns self
   */
  iand(num) {
    assert (self.negative | num.negative) == 0
    return self.iuand(num)
  }

  /**
   * Performs a bitwise AND operation on the number and the given one.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  and_(num) {
    if self.length > num.length {
      return self.clone().iand(num)
    }

    return num.clone().iand(self)
  }

  /**
   * The unsigned version of [[bigint.BigInt.and_]]
   */
  uand(num) {
    if self.length > num.length {
      return self.clone().iuand(num)
    }

    return num.clone().iuand(self)
  }

  /**
   * The in-place and unsigned version of [[bigint.BigInt.xor]]
   * 
   * @returns self
   */
  iuxor(num) {
    # a.length > b.length
    var a, b
    if self.length > num.length {
      a = self
      b = num
    } else {
      a = num
      b = self
    }

    var i = 0
    iter ; i < b.length; i++ {
      self.words[i] = a.words[i] ^ b.words[i]
    }

    if self != a {
      iter ; i < a.length; i++ {
        if i < self.words.length() {
          self.words[i] = a.words[i]
        } else {
          self.words.insert(a.words[i], i)
        }
      }
    }

    self.length = a.length

    return self.strip()
  }

  /**
   * The in-place version of [[bigint.BigInt.xor]]
   * 
   * @returns self
   */
  ixor(num) {
    assert (self.negative | num.negative) == 0
    return self.iuxor(num)
  }

  /**
   * Performs a bitwise XOR operation on the number and the given one.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  xor(num) {
    if self.length > num.length {
      return self.clone().ixor(num)
    }

    return num.clone().ixor(self)
  }

  /**
   * The unsigned version of [[bigint.BigInt.xor]]
   */
  uxor(num) {
    if self.length > num.length {
      return self.clone().iuxor(num)
    }

    return num.clone().iuxor(self)
  }

  /**
   * The in-place version of [[bigint.BigInt.notn]]
   * 
   * @returns self
   */
  inotn(width) {
    assert is_number(width) and width >= 0

    var bytesNeeded = math.ceil(width / 26) | 0
    var bitsLeft = width % 26

    # Extend the buffer with leading zeroes
    self._expand(bytesNeeded)

    if bitsLeft > 0 {
      bytesNeeded--
    }

    # Handle complete words
    var i = 0
    iter ; i < bytesNeeded; i++ {
      self.words[i] = ~self.words[i] & 0x3ffffff
    }

    # Handle the residue
    if bitsLeft > 0 {
      self.words[i] = ~self.words[i] & (0x3ffffff >> (26 - bitsLeft))
    }

    # And remove leading zeroes
    return self.strip()
  }

  /**
   * Performs a bitwise NOT operation on the BigInt and the given one number.
   * 
   * @param number width
   * @returns [[bigint.BigInt]]
   */
  notn(width) {
    return self.clone().inotn(width)
  }

  /**
   * Sets the specified bit in the number to the given value
   * 
   * @param number bit
   * @param {number|bool} val
   * @returns [[bigint.BigInt]]
   */
  setn(bit, val) {
    assert is_number(bit) and bit >= 0

    var off = (bit / 26) | 0
    var wbit = bit % 26

    self._expand(off + 1)

    if val {
      self.words[off] = self.words[off] | (1 << wbit)
    } else {
      self.words[off] = self.words[off] & ~(1 << wbit)
    }

    return self.strip()
  }

  /**
   * The in-place version of [[bigint.BigInt.and]]
   * 
   * @returns self
   */
  iadd(num) {
    var r

    # negative + positive
    if self.negative != 0 and num.negative == 0 {
      self.negative = 0
      r = self.isub(num)
      self.negative ^= 1
      return self.normSign()

    # positive + negative
    } else if self.negative == 0 and num.negative != 0 {
      num.negative = 0
      r = self.isub(num)
      num.negative = 1
      return r.normSign()
    }

    # a.length > b.length
    var a, b
    if self.length > num.length {
      a = self
      b = num
    } else {
      a = num
      b = self
    }

    var carry = 0, i = 0
    iter ; i < b.length; i++ {
      r = (a.words[i] | 0) + (b.words[i] | 0) + carry
      self.words[i] = r & 0x3ffffff
      carry = r >>> 26
    }

    iter ; carry != 0 and i < a.length; i++ {
      r = (a.words[i] | 0) + carry
      self.words[i] = r & 0x3ffffff
      carry = r >>> 26
    }

    self.length = a.length
    if carry != 0 {
      if self.words.length() > self.length {
        self.words[self.length] = carry
      } else {
        self.words.insert(carry, self.length)
      }

      self.length++
    # Copy the rest of the words
    } else if a != self {
      iter ; i < a.length; i++ {
        if i < self.words.length() {
          self.words[i] = a.words[i]
        } else {
          self.words.insert(a.words[i], i)
        }
      }
    }

    return self
  }

  /**
   * Adds the given number to the current one.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  add(num) {
    var res
    if num.negative != 0 and self.negative == 0 {
      num.negative = 0
      res = self.sub(num)
      num.negative ^= 1
      return res
    } else if num.negative == 0 and self.negative != 0 {
      self.negative = 0
      res = num.sub(self)
      self.negative = 1
      return res
    }

    if self.length > num.length {
      return self.clone().iadd(num)
    }

    return num.clone().iadd(self)
  }

  /**
   * The in-place version of [[bigint.BigInt.sub]]
   * 
   * @returns self
   */
  isub(num) {
    # self - (-num) = self + num
    if num.negative != 0 {
      num.negative = 0
      var r = self.iadd(num)
      num.negative = 1
      return r.normSign()

    # -self - num = -(self + num)
    } else if self.negative != 0 {
      self.negative = 0
      self.iadd(num)
      self.negative = 1
      return self.normSign()
    }

    # At self point both numbers are positive
    var cmp = self.cmp(num)

    # Optimization - zeroify
    if cmp == 0 {
      self.negative = 0
      self.length = 1
      self.words[0] = 0
      return self
    }

    # a > b
    var a, b
    if cmp > 0 {
      a = self
      b = num
    } else {
      a = num
      b = self
    }

    var carry = 0, i = 0
    iter ; i < b.length; i++ {
      var r = (a.words[i] | 0) - (b.words[i] | 0) + carry
      carry = r >> 26
      self.words[i] = r & 0x3ffffff
    }

    iter ; carry != 0 and i < a.length; i++ {
      var r = (a.words[i] | 0) + carry
      carry = r >> 26
      if i < self.words.length() {
        self.words[i] = r & 0x3ffffff
      } else {
        self.words.insert(r & 0x3ffffff, i)
      }
    }

    # Copy rest of the words
    if carry == 0 and i < a.length and a != self {
      iter ; i < a.length; i++ {
        if i < self.words.length() {
          self.words[i] = a.words[i]
        } else {
          self.words.insert(a.words[i], i)
        }
      }
    }

    self.length = max(self.length, i)

    if a != self {
      self.negative = 1
    }

    return self.strip()
  }

  /**
   * Subracts the given number from the current one.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  sub(num) {
    return self.clone().isub(num)
  }

  mulTo(num, out) {
    var res
    var len = self.length + num.length
    if self.length == 10 and num.length == 10 {
      res = _comb10MulTo(self, num, out)
    } else if len < 63 {
      res = _smallMulTo(self, num, out)
    } else if len < 1024 {
      res = _bigMulTo(self, num, out)
    } else {
      res = _jumboMulTo(self, num, out)
    }

    return res
  }

  /**
   * Multiplies the given number by the current one.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  mul(num) {
    var out = BigInt(nil)
    out.words = [0] * (self.length + num.length)
    return self.mulTo(num, out)
  }

  /**
   * The in-place version of [[bigint.BigInt.mul]]
   * 
   * @returns self
   */
  imul(num) {
    return self.clone().mulTo(num, self)
  }

  /**
   * The in-place version of [[bigint.BigInt.muln]]
   * 
   * @returns self
   */
  imuln(num) {
    var isNegNum = num < 0
    if isNegNum num = -num

    assert is_number(num)
    assert num < 0x4000000

    # Carry
    var carry = 0
    var i = 0
    iter ; i < self.length; i++ {
      var w = (self.words[i] | 0) * num
      var lo = (w & 0x3ffffff) + (carry & 0x3ffffff)
      carry >>= 26
      carry += (w / 0x4000000) | 0
      # NOTE: lo is 27bit maximum
      carry += lo >>> 26
      self.words[i] = lo & 0x3ffffff
    }

    if carry != 0 {
      if self.words.length() - 1 < i {
        self.words.insert(carry, i)
      } else {
        self.words[i] = carry
      }
      self.length++
    }

    return isNegNum ? self.ineg() : self
  }

  /**
   * Multiplies the given BigInt by the given number.
   * 
   * @param number num
   * @returns [[bigint.BigInt]]
   */
  muln(num) {
    return self.clone().imuln(num)
  }

  /**
   * Returns a number which is equal to the square of the current number.
   * 
   * @returns [[bigint.BigInt]]
   */
  sqr() {
    return self.mul(self)
  }

  /**
   * The in-place version of [[bigint.BigInt.sqr]]
   * 
   * @returns self
   */
  isqr() {
    return self.imul(self.clone())
  }

  /**
   * Returns a number which is equal to the current number raise to the power 
   * of the given number.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  pow(num) {
    var w = _toBitArray(num)
    if w.length == 0 return BigInt(1)

    # Skip leading zeroes
    var res = self
    var i = 0
    iter ; i < w.length(); i++, res = res.sqr() {
      if w[i] != 0 break
    }

    if i++ < w.length() {
      iter var q = res.sqr(); i < w.length(); i++, q = q.sqr() {
        if w[i] == 0 continue

        res = res.mul(q)
      }
    }

    return res
  }

  /**
   * The in-place and unsigned version of [[bigint.BigInt.shln]]
   * 
   * @returns self
   */
  iushln(bits) {
    assert is_number(bits) and bits >= 0

    var r = bits % 26
    var s = (bits - r) / 26
    var carryMask = (0x3ffffff >>> (26 - r)) << (26 - r)
    var i

    if r != 0 {
      var carry = 0

      iter i = 0; i < self.length; i++ {
        var newCarry = (self.words[i] or 0) & carryMask
        var c = (((self.words[i] or 0) | 0) - newCarry) << r
        self.words[i] = c | carry
        carry = newCarry >>> (26 - r)
      }

      if carry {
        if i < self.words.length() {
          self.words[i] = carry
        } else {
          self.words.insert(carry, i)
        }
        self.length++
      }
    }

    if s != 0 {
      iter i = self.length - 1; i >= 0; i-- {
        if i + s < self.words.length() {
          self.words[i + s] = self.words[i]
        } else {
          self.words.insert(self.words[i], i + s)
        }
      }

      iter i = 0; i < s; i++ {
        self.words[i] = 0
      }

      self.length += s
    }

    return self.strip()
  }

  /**
   * The in-place version of [[bigint.BigInt.shln]]
   * 
   * @returns self
   */
  ishln (bits) {
    assert self.negative == 0
    return self.iushln(bits)
  }

  # Shift-right in-place
  # NOTE: `hint` is a lowest bit before trailing zeroes
  # NOTE: if `extended` is present - it will be filled with destroyed bits
  /**
   * The in-place and unsigned version of [[bigint.BigInt.shrn]]
   * 
   * @returns self
   */
  iushrn(bits, hint, extended) {
    assert is_number(bits) and bits >= 0

    var h
    if hint {
      h = (hint - (hint % 26)) / 26
    } else {
      h = 0
    }

    var r = bits % 26
    var s = min((bits - r) / 26, self.length)
    var mask = 0x3ffffff ^ ((0x3ffffff >>> r) << r)
    var maskedWords = extended

    h -= s
    h = max(0, h)

    # Extended mode, copy masked part
    if maskedWords {
      iter var i = 0; i < s; i++ {
        maskedWords.words[i] = self.words[i]
      }

      maskedWords.length = s
    }

    if s == 0 {
      # No-op, we should not move anything at all
    } else if self.length > s {
      self.length -= s
      iter var i = 0; i < self.length; i++ {
        self.words[i] = self.words[i + s]
      }
    } else {
      self.words[0] = 0
      self.length = 1
    }

    var carry = 0
    iter var i = self.length - 1; i >= 0 and (carry != 0 or i >= h); i-- {
      var word = self.words[i] | 0
      self.words[i] = (carry << (26 - r)) | (word >>> r)
      carry = word & mask
    }

    # Push carried bits as a mask
    if maskedWords and carry != 0 {
      maskedWords.words[maskedWords.length++ - 1] = carry
    }

    if self.length == 0 {
      self.words[0] = 0
      self.length = 1
    }

    return self.strip()
  }

  /**
   * The in-place version of [[bigint.BigInt.shrn]]
   * 
   * @returns self
   */
  ishrn(bits, hint, extended) {
    assert self.negative == 0
    return self.iushrn(bits, hint, extended)
  }

  /**
   * Performs a bitwise left shift operation on the BigInt and the given number.
   * 
   * @param number bits
   * @returns [[bigint.BigInt]]
   */
  shln(bits) {
    return self.clone().ishln(bits)
  }

  /**
   * The unsigned version of [[bigint.BigInt.shln]]
   */
  ushln(bits) {
    return self.clone().iushln(bits)
  }

  /**
   * Performs a bitwise right shift operation on the number and the given one.
   * 
   * @param number bits
   * @returns [[bigint.BigInt]]
   */
  shrn(bits) {
    return self.clone().ishrn(bits)
  }

  /**
   * The unsigned version of [[bigint.BigInt.shrn]]
   */
  ushrn(bits) {
    return self.clone().iushrn(bits)
  }

  /**
   * Tests the number to see if the specified bit is set.
   * 
   * @param number bit
   * @returns bool
   */
  testn(bit) {
    assert is_number(bit) and bit >= 0
    var r = bit % 26
    var s = (bit - r) / 26
    var q = 1 << r

    # Fast case: bit is much higher than all existing words
    if self.length <= s return false

    # Check bit and return
    var w = self.words[s]
    var g = w & q
    return g == 0 ? false : !!g
  }

  /**
   * The in-place version of [[bigint.BigInt.maskn]]
   * 
   * @returns self
   */
  imaskn(bits) {
    assert is_number(bits) and bits >= 0

    var r = bits % 26
    var s = (bits - r) / 26

    assert self.negative == 0, 'imaskn works only with positive numbers'

    if self.length <= s {
      return self
    }

    if r != 0 {
      s++
    }
    self.length = min(s, self.length)

    if r != 0 {
      var mask = 0x3ffffff ^ ((0x3ffffff >>> r) << r)
      self.words[self.length - 1] &= mask
    }

    return self.strip()
  }

  /**
   * Clears all bits higher than or equal to the given bit in the number.
   * 
   * @param number bit
   * @returns [[bigint.BigInt]]
   */
  maskn(bit) {
    return self.clone().imaskn(bit)
  }

  /**
   * The in-place version of [[bigint.BigInt.addn]]
   * 
   * @returns self
   */
  iaddn (num) {
    assert is_number(num)
    assert num < 0x4000000
    if num < 0 return self.isubn(-num)

    # Possible sign change
    if self.negative != 0 {
      if self.length == 1 and (self.words[0] | 0) <= num {
        self.words[0] = num - (self.words[0] | 0)
        self.negative = 0
        return self
      }

      self.negative = 0
      self.isubn(num)
      self.negative = 1
      return self
    }

    # Add without checks
    return self._iaddn(num)
  }

  _iaddn (num) {
    self.words[0] += num

    # Carry
    var i = 0
    iter ; i < self.length and self.words[i] >= 0x4000000; i++ {
      self.words[i] -= 0x4000000
      if i == self.length - 1 {
        self.words[i + 1] = 1
      } else {
        self.words[i + 1]++
      }
    }

    self.length = max(self.length, i + 1)

    return self
  }

  /**
   * The in-place version of [[bigint.BigInt.subn]]
   * 
   * @returns self
   */
  isubn (num) {
    assert is_number(num)
    assert num < 0x4000000
    if num < 0 {
      return self.iaddn(-num)
    }

    if self.negative != 0 {
      self.negative = 0
      self.iaddn(num)
      self.negative = 1
      return self
    }

    self.words[0] -= num

    if self.length == 1 and self.words[0] < 0 {
      self.words[0] = -self.words[0]
      self.negative = 1
    } else {
      # Carry
      iter var i = 0; i < self.length and self.words[i] < 0; i++ {
        self.words[i] += 0x4000000
        self.words[i + 1] -= 1
      }
    }

    return self.strip()
  }

  /**
   * Adds the given number _num_ to the current BigInt
   * 
   * @param number num
   * @returns [[bigint.BigInt]]
   */
  addn(num) {
    return self.clone().iaddn(num)
  }

  /**
   * Subracts the given number _num_ from the current BigInt
   * 
   * @param number num
   * @returns [[bigint.BigInt]]
   */
  subn(num) {
    return self.clone().isubn(num)
  }

  /**
   * The in-place version of [[bigint.BigInt.abs]]
   * 
   * @returns self
   */
  iabs() {
    self.negative = 0

    return self
  }

  /**
   * Returns the absolute value of the current number.
   * 
   * @returns [[bigint.BigInt]]
   */
  abs() {
    return self.clone().iabs()
  }

  ishlnsubmul(num, mul, shift) {
    var len = num.length + shift
    var i

    self._expand(len)

    var w
    var carry = 0
    iter i = 0; i < num.length; i++ {
      w = (self.words[i + shift] | 0) + carry
      var right = (num.words[i] | 0) * mul
      w -= right & 0x3ffffff
      carry = (w >> 26) - ((right / 0x4000000) | 0)
      self.words[i + shift] = w & 0x3ffffff
    }
    iter ; i < self.length - shift; i++ {
      w = (self.words[i + shift] | 0) + carry
      carry = w >> 26
      self.words[i + shift] = w & 0x3ffffff
    }

    if carry == 0 {
      return self.strip()
    }

    # Subtraction overflow
    assert carry == -1
    carry = 0
    iter i = 0; i < self.length; i++ {
      w = -(self.words[i] | 0) + carry
      carry = w >> 26
      self.words[i] = w & 0x3ffffff
    }
    self.negative = 1

    return self.strip()
  }

  _wordDiv(num, mode) {
    var shift = self.length - num.length

    var a = self.clone()
    var b = num

    # Normalize
    var bhi = b.words[b.length - 1] | 0
    var bhiBits = self._countBits(bhi)
    shift = 26 - bhiBits
    if shift != 0 {
      b = b.ushln(shift)
      a.iushln(shift)
      bhi = b.words[b.length - 1] | 0
    }

    # Initialize quotient
    var m = a.length - b.length
    var q

    if mode != 'mod' {
      q = BigInt(nil)
      q.length = m + 1
      q.words = [0] * q.length
    }

    var diff = a.clone().ishlnsubmul(b, 1, m)
    if diff.negative == 0 {
      a = diff
      if q {
        q.words[m] = 1
      }
    }

    iter var j = m - 1; j >= 0; j-- {
      var qj = (a.words[b.length + j] | 0) * 0x4000000 +
        (a.words[b.length + j - 1] | 0)

      # NOTE: (qj / bhi) is (0x3ffffff * 0x4000000 + 0x3ffffff) / 0x2000000 max
      # (0x7ffffff)
      qj = min((qj / bhi) | 0, 0x3ffffff)

      a.ishlnsubmul(b, qj, j)
      while a.negative != 0 {
        qj--
        a.negative = 0
        a.ishlnsubmul(b, 1, j)
        if (!a.isZero()) {
          a.negative ^= 1
        }
      }

      if q {
        q.words[j] = qj
      }
    }

    if q {
      q.strip()
    }

    a.strip()

    # Denormalize
    if mode != 'div' and shift != 0 {
      a.iushrn(shift)
    }

    return {
      div: q ? q : nil,
      mod: a
    }
  }

  # NOTE: 1) 
  #       to 
  #       request both div & mod
  #       2) 
  /**
   * Returns the quotient and modulus obtained when the current number is divided by 
   * the given number as a dictionary containg the keys `mod` (modulus) and `div` 
   * (quotient). 
   * 
   * The second paramter of the function (`mode`) allows the customization of the 
   * computation and accepts the values listed below to achive different results.
   * 
   * - `mod` to request mod only,
   * - `div` to request div only, or be absent to
   * 
   * It also accept a third argument `positive` causes the function to return the 
   * unsigned mod if `true`.
   * 
   * @param [[bigint.BigInt]] num
   * @param string? mode: `''` (default) | `'mod'` | `'div'`
   * @param bool? positive
   * @returns dict
   */
  divmod(num, mode, positive) {
    assert !num.isZero()

    if self.isZero() {
      return {
        div: BigInt(0),
        mod: BigInt(0),
      }
    }

    var div, mod, res
    if self.negative != 0 and num.negative == 0 {
      res = self.neg().divmod(num, mode)

      if mode != 'mod' {
        div = res.div.neg()
      }

      if mode != 'div' {
        mod = res.mod.neg()
        if positive and mod.negative != 0 {
          mod.iadd(num)
        }
      }

      return {
        div: div,
        mod: mod,
      }
    }

    if self.negative == 0 and num.negative != 0 {
      res = self.divmod(num.neg(), mode)

      if mode != 'mod' {
        div = res.div.neg()
      }

      return {
        div: div,
        mod: res.mod,
      }
    }

    if (self.negative & num.negative) != 0 {
      res = self.neg().divmod(num.neg(), mode)

      if mode != 'div' {
        mod = res.mod.neg()
        if positive and mod.negative != 0 {
          mod.isub(num)
        }
      }

      return {
        div: res.div,
        mod: mod,
      }
    }

    # Both numbers are positive at self point

    # Strip both numbers to approximate shift value
    if num.length > self.length or self.cmp(num) < 0 {
      return {
        div: BigInt(0),
        mod: self,
      }
    }

    # Very short reduction
    if num.length == 1 {
      if mode == 'div' {
        return {
          div: self.divn(num.words[0]),
          mod: nil,
        }
      }

      if mode == 'mod' {
        return {
          div: nil,
          mod: BigInt(self.modrn(num.words[0])),
        }
      }

      return {
        div: self.divn(num.words[0]),
        mod: BigInt(self.modrn(num.words[0]))
      }
    }

    return self._wordDiv(num, mode)
  }

  /**
   * Divides the current number by the given number _num_.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  div(num) {
    return self.divmod(num, 'div', false).div
  }

  /**
   * Returns the remainder of dividing the current number by the given number _num_.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  mod(num) {
    return self.divmod(num, 'mod', false).mod
  }

  /**
   * The unsigned form of [[bigint.BigInt.mod]]
   */
  umod(num) {
    return self.divmod(num, 'mod', true).mod
  }

  /**
   * Same as [[bigint.BigInt.div]] but rounds the result to the nearest number.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  divRound(num) {
    var dm = self.divmod(num)

    # Fast case - exact division
    if dm.mod.isZero() {
      return dm.div
    }

    var mod = dm.div.negative != 0 ? dm.mod.isub(num) : dm.mod

    var half = num.ushrn(1)
    var r2 = num.andln(1)
    var cmp = mod.cmp(half)

    # Round down
    if cmp < 0 or (r2 == 1 and cmp == 0) {
      return dm.div
    }

    # Round up
    return dm.div.negative != 0 ? dm.div.isubn(1) : dm.div.iaddn(1)
  }

  # leave undocumented
  modrn(num) {
    var isNegNum = num < 0
    if isNegNum num = -num

    assert num <= 0x3ffffff
    var p = (1 << 26) % num

    var acc = 0
    iter var i = self.length - 1; i >= 0; i-- {
      acc = (p * acc + (self.words[i] | 0)) % num
    }

    return isNegNum ? -acc : acc
  }

  /**
   * The in-place version of [[bigint.BigInt.divn]]
   * 
   * @returns self
   */
  idivn(num) {
    var isNegNum = num < 0
    if (isNegNum) num = -num

    assert num <= 0x3ffffff

    var carry = 0
    iter var i = self.length - 1; i >= 0; i-- {
      var w = (self.words[i] | 0) + carry * 0x4000000
      self.words[i] = (w / num) | 0
      carry = w % num
    }

    self.strip()
    return isNegNum ? self.ineg() : self
  }

  /**
   * Divides the current BigInt by the given number _num_.
   * 
   * @param number num
   * @returns [[bigint.BigInt]]
   */
  divn(num) {
    return self.clone().idivn(num)
  }

  /**
   * Caculates the Greatest Common Divisor using the Extended Euclidean algorithm 
   * (ax + by) where _x_ is the current number and returns a dictionary containing 
   * the results of `a`, `b`, and `gcd`.
   * 
   * @param [[bigint.BigInt]] y
   * @returns dict
   */
  egcd(y) {
    assert y.negative == 0
    assert !y.isZero()

    var x = self
    y = y.clone()

    if x.negative != 0 {
      x = x.umod(y)
    } else {
      x = x.clone()
    }

    # A * x + B * y = x
    var A = BigInt(1)
    var B = BigInt(0)

    # C * x + D * y = y
    var C = BigInt(0)
    var D = BigInt(1)

    var g = 0

    while x.isEven() and y.isEven() {
      x.iushrn(1)
      y.iushrn(1)
      g++
    }

    var yp = y.clone()
    var xp = x.clone()

    while !x.isZero() {
      var i = 0
      iter var im = 1; (x.words[0] & im) == 0 and i < 26; i++ {
        im <<= 1
      }

      if i > 0 {
        x.iushrn(i)
        while i-- + 1 > 0 {
          if A.isOdd() or B.isOdd() {
            A.iadd(yp)
            B.isub(xp)
          }

          A.iushrn(1)
          B.iushrn(1)
        }
      }

      var j = 0
      iter var jm = 1; (y.words[0] & jm) == 0 and j < 26; j++ {
        jm <<= 1
      }

      if j > 0 {
        y.iushrn(j)
        while j-- + 1 > 0 {
          if C.isOdd() or D.isOdd() {
            C.iadd(yp)
            D.isub(xp)
          }

          C.iushrn(1)
          D.iushrn(1)
        }
      }

      if x.cmp(y) >= 0 {
        x.isub(y)
        A.isub(C)
        B.isub(D)
      } else {
        y.isub(x)
        C.isub(A)
        D.isub(B)
      }
    }

    return {
      a: C,
      b: D,
      gcd: y.iushln(g)
    }
  }

  # self is reduced incarnation of the binary EEA
  # above, designated to invert members of the
  # _prime_ fields F(p) at a maximal speed
  _invmp(p) {
    assert p.negative == 0
    assert !p.isZero()

    var a = self
    var b = p.clone()

    if a.negative != 0 {
      a = a.umod(p)
    } else {
      a = a.clone()
    }

    var x1 = BigInt(1)
    var x2 = BigInt(0)

    var delta = b.clone()

    while a.cmpn(1) > 0 and b.cmpn(1) > 0 {
      var i = 0
      iter var im = 1; (a.words[0] & im) == 0 and i < 26; i++ {
        im <<= 1
      }

      if i > 0 {
        a.iushrn(i)
        while i-- + 1 > 0 {
          if x1.isOdd() {
            x1.iadd(delta)
          }

          x1.iushrn(1)
        }
      }

      var j = 0
      iter var jm = 1; (b.words[0] & jm) == 0 and j < 26; j++ {
        jm <<= 1
      }

      if j > 0 {
        b.iushrn(j)
        while j-- + 1 > 0 {
          if x2.isOdd() {
            x2.iadd(delta)
          }

          x2.iushrn(1)
        }
      }

      if a.cmp(b) >= 0 {
        a.isub(b)
        x1.isub(x2)
      } else {
        b.isub(a)
        x2.isub(x1)
      }
    }

    var res
    if a.cmpn(1) == 0 {
      res = x1
    } else {
      res = x2
    }

    if res.cmpn(0) < 0 {
      res.iadd(p)
    }

    return res
  }

  /**
   * Calculates the Greatest Common Divisor of the current number and the given number _num_.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  gcd(num) {
    if self.isZero() return num.abs()
    if num.isZero() return self.abs()

    var a = self.clone()
    var b = num.clone()
    a.negative = 0
    b.negative = 0

    # Remove common factor of two
    var shift = 0
    iter ; a.isEven() and b.isEven(); shift++ {
      a.iushrn(1)
      b.iushrn(1)
    }

    do {
      while a.isEven() {
        a.iushrn(1)
      }
      while b.isEven() {
        b.iushrn(1)
      }

      var r = a.cmp(b)
      if r < 0 {
        # Swap `a` and `b` to make `a` always bigger than `b`
        var t = a
        a = b
        b = t
      } else if r == 0 or b.cmpn(1) == 0 {
        break
      }

      a.isub(b)
    } while true

    return b.iushln(shift)
  }

  /**
   * Calculates the inverse of the current number modulo the given number _num_.
   * 
   * @param [[bigint.BigInt]] num
   * @returns [[bigint.BigInt]]
   */
  invm(num) {
    return self.egcd(num).a.umod(num)
  }

  /**
   * Returns `true` if the current number is an even number or `false` otherwise.
   * 
   * @returns bool
   */
  isEven() {
    return (self.words[0] & 1) == 0
  }

  /**
   * Returns `true` if the current number is an odd number or `false` otherwise.
   * 
   * @returns bool
   */
  isOdd() {
    return (self.words[0] & 1) == 1
  }

  /**
   * Perform AND on lo 32 bits of the current number and the given number 
   * and returns a regular number.
   * 
   * @param number num
   * @returns number
   */
  andln(num) {
    if !is_number(num) {
      raise Exception('number expected for operation')
    }

    return self.words[0] & num
  }

  /**
   * Adds the result of calculating `1 << bit` to the current number and returns 
   * the resulting value.
   * 
   * @param number num
   * @returns self
   */
  bincn(bit) {
    if !is_number(bit) {
      raise Exception('number expected for operation')
    }

    var r = bit % 26
    var s = (bit - r) / 26
    var q = 1 << r

    # Fast case: bit is much higher than all existing words
    if self.length <= s {
      self._expand(s + 1)
      self.words[s] |= q
      return self
    }

    # Add bit and propagate, if needed
    var carry = q
    iter var i = s; carry != 0 and i < self.length; i++ {
      var w = self.words[i] | 0
      w += carry
      carry = w >>> 26
      w &= 0x3ffffff
      self.words[i] = w
    }

    if carry != 0 {
      self.words[i] = carry
      self.length++
    }

    return self
  }

  /**
   * Returns `true` if the current number is zero otherwise returns `false`
   * 
   * @returns bool
   */
  isZero() {
    return self.length == 1 and self.words[0] == 0
  }

  /**
   * Compares the current BigInt with the given number and return `-1` if the 
   * current BigInt is less than the given number, `0` if it is equal to the 
   * given number, or `1` if it is greater than the given number.
   * 
   * @param number num
   * @returns number
   */
  cmpn(num) {
    if !is_number(num) {
      raise Exception('number expected for operation')
    }

    var negative = num < 0

    if self.negative != 0 and !negative return -1
    if self.negative == 0 and negative return 1

    self.strip()

    var res
    if self.length > 1 {
      res = 1
    } else {
      if negative {
        num = -num
      }

      assert num <= 0x3ffffff, 'Number is too big'

      var w = self.words[0] | 0
      res = w == num ? 0 : w < num ? -1 : 1
    }

    if self.negative != 0 {
      return -res | 0
    }

    return res
  }

  # Compare two numbers and return:
  # 1 - if `self` > `num`
  # 0 - if `self` == `num`
  # -1 - if `self` < `num`
  /**
   * Compares the current number with the given number and return `-1` if the 
   * current number is less than the given number, `0` if it is equal to the 
   * given number, or `1` if it is greater than the given number.
   * 
   * @param [[bigint.BigInt]] num
   * @returns number
   */
  cmp(num) {
    if self.negative != 0 and num.negative == 0 return -1
    if self.negative == 0 and num.negative != 0 return 1

    var res = self.ucmp(num)
    if self.negative != 0 {
      return -res | 0
    }

    return res
  }

  /**
   * The unsigned version of [[bigint.BigInt.cmp]]
   */
  ucmp(num) {
    # At self point both numbers have the same sign
    if self.length > num.length return 1
    if self.length < num.length return -1

    var res = 0
    iter var i = self.length - 1; i >= 0; i-- {
      var a = self.words[i] | 0
      var b = num.words[i] | 0

      if a == b continue
      if a < b {
        res = -1
      } else if a > b {
        res = 1
      }

      break
    }
    
    return res
  }

  /**
   * 
   * @returns bool
   */
  gtn(num) {
    return self.cmpn(num) == 1
  }

  /**
   * 
   * @returns bool
   */
  gt(num) {
    return self.cmp(num) == 1
  }

  /**
   * 
   * @returns bool
   */
  gten(num) {
    return self.cmpn(num) >= 0
  }

  /**
   * 
   * @returns bool
   */
  gte(num) {
    return self.cmp(num) >= 0
  }

  /**
   * 
   * @returns bool
   */
  ltn(num) {
    return self.cmpn(num) == -1
  }

  /**
   * 
   * @returns bool
   */
  lt(num) {
    return self.cmp(num) == -1
  }

  /**
   * 
   * @returns bool
   */
  lten(num) {
    return self.cmpn(num) <= 0
  }

  /**
   * 
   * @returns bool
   */
  lte(num) {
    return self.cmp(num) <= 0
  }

  /**
   * 
   * @returns bool
   */
  eqn(num) {
    return self.cmpn(num) == 0
  }

  /**
   * 
   * @returns bool
   */
  eq(num) {
    return self.cmp(num) == 0
  }

  @to_string() {
    return '<BigInt v=${self.toString(10)}>'
  }

  @to_int() {
    return self.toNumber()
  }

  @to_json() {
    return self.toJSON()
  }

  @to_list() {
    return self.toList()
  }

  @to_dict() {
    return self.toList().to_dict()
  }

  @to_abs() {
    return self.abs()
  }

  @to_number() {
    return self.toNumber()
  }

  @to_int() {
    return to_int(self.toNumber())
  }

  @to_bin() {
    return self.toString(2)
  }

  @to_oct() {
    return self.toString(8)
  }

  @to_hex() {
    return self.toString(16)
  }

  @to_bool() {
    return self.gten(-1)
  }

  def - {
    if __arg__ == nil { # -x
      return self.neg()
    } else if instance_of(__arg__, BigInt) { # xn - yn
      return self.sub(__arg__)
    } else if is_number(__arg__) {
      return self.subn(__arg__)
    }

    raise Exception('BigInt operation - not permitted on ${typeof(__arg__)}')
  }

  def + {
    if instance_of(__arg__, BigInt) {
      return self.add(__arg__)
    } else if is_number(__arg__) {
      return self.addn(__arg__)
    }

    raise Exception('BigInt operation + not permitted on ${typeof(__arg__)}')
  }

  def * {
    if instance_of(__arg__, BigInt) {
      return self.mul(__arg__)
    } else if is_number(__arg__) {
      return self.muln(__arg__)
    }

    raise Exception('BigInt operation * not permitted on ${typeof(__arg__)}')
  }

  def / {
    if instance_of(__arg__, BigInt) {
      return self.div(__arg__)
    } else if is_number(__arg__) {
      return self.divn(__arg__)
    }

    raise Exception('BigInt operation / not permitted on ${typeof(__arg__)}')
  }

  def ** {
    if instance_of(__arg__, BigInt) {
      return self.pow(__arg__)
    } else if is_number(__arg__) {
      return self.pow(bigint(__arg__))
    }

    raise Exception('BigInt operation ** not permitted on ${typeof(__arg__)}')
  }

  def % {
    if instance_of(__arg__, BigInt) {
      return self.mod(__arg__)
    } else if is_number(__arg__) {
      return BigInt(self.modrn(__arg__))
    }

    raise Exception('BigInt operation % not permitted on ${typeof(__arg__)}')
  }

  def // {
    if instance_of(__arg__, BigInt) {
      return self.divmod(__arg__).div
    } else if is_number(__arg__) {
      return self.divmod(bigint(__arg__)).div
    }

    raise Exception('BigInt operation // not permitted on ${typeof(__arg__)}')
  }

  def | {
    if instance_of(__arg__, BigInt) {
      return self.or_(__arg__)
    } else if is_number(__arg__) {
      return self.or_(bigint(__arg__))
    }

    raise Exception('BigInt operation | not permitted on ${typeof(__arg__)}')
  }

  def & {
    if instance_of(__arg__, BigInt) {
      return self.and_(__arg__)
    } else if is_number(__arg__) {
      return self.andln(__arg__)
    }

    raise Exception('BigInt operation & not permitted on ${typeof(__arg__)}')
  }

  def ^ {
    if instance_of(__arg__, BigInt) {
      return self.xor(__arg__)
    } else if is_number(__arg__) {
      return self.xor(bigint(__arg__))
    }

    raise Exception('BigInt operation ^ not permitted on ${typeof(__arg__)}')
  }

  def << {
    if instance_of(__arg__, BigInt) {
      return self.shln(__arg__.toNumber())
    } else if is_number(__arg__) {
      return self.shln(__arg__)
    }

    raise Exception('BigInt operation << not permitted on ${typeof(__arg__)}')
  }

  def >> {
    if instance_of(__arg__, BigInt) {
      return self.shrn(__arg__.toNumber())
    } else if is_number(__arg__) {
      return self.shrn(__arg__)
    }

    raise Exception('BigInt operation >> not permitted on ${typeof(__arg__)}')
  }

  def ~ {
    return self.neg().isubn(1)
  }

  def >>> {
    raise Exception('BigInt does not support >>> operations')
  }

  def > {
    if instance_of(__arg__, BigInt) {
      return self.gt(__arg__)
    } else if is_number(__arg__) {
      return self.gtn(__arg__)
    }

    raise Exception('BigInt operation > not permitted on ${typeof(__arg__)}')
  }

  def < {
    if instance_of(__arg__, BigInt) {
      return self.lt(__arg__)
    } else if is_number(__arg__) {
      return self.ltn(__arg__)
    }

    raise Exception('BigInt operation < not permitted on ${typeof(__arg__)}')
  }

  def = {
    if instance_of(__arg__, BigInt) {
      return self.eq(__arg__)
    } else if is_number(__arg__) {
      return self.eq(BigInt(__arg__))
    }

    raise Exception('BigInt operation = not permitted on ${typeof(__arg__)}')
  }
}


/**
 * See [[big.BigInt]]
 * 
 * @param number
 * @param base
 * @param endian
 * @returns [[bigint.BigInt]]
 * @exported
 */
def bigint(number, base, endian) {
  if BigInt.isBigInt(number) {
    return number
  }

  return BigInt(number, base, endian)
}


/**
 * @type [[bigint.BigInt]]
 */
var zero = BigInt(0)
