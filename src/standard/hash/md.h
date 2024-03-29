#ifndef BLADE_MODULE_HASH_MD_H
#define BLADE_MODULE_HASH_MD_H

/* MD4 context */
typedef struct {
  uint32_t state[4];
  uint32_t count[2];
  unsigned char buffer[64];
} MD4_CTX;

/* MD2 context */
typedef struct {
  unsigned char state[48];
  unsigned char checksum[16];
  unsigned char buffer[16];
  char in_buffer;
} MD2_CTX;

/* MD common stuff */

static const unsigned char MD_PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void MD_memset(unsigned char *output, int value, unsigned int len) {
  unsigned int i;

  for (i = 0; i < len; i++)
    ((unsigned char *) output)[i] = (char) value;
}

/*
   MDEncodes input (uint32_t) into output (unsigned char). Assumes len is
   a multiple of 4.
 */
static void MDEncode(unsigned char *output, uint32_t *input, unsigned int len) {
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[j] = (unsigned char) (input[i] & 0xff);
    output[j + 1] = (unsigned char) ((input[i] >> 8) & 0xff);
    output[j + 2] = (unsigned char) ((input[i] >> 16) & 0xff);
    output[j + 3] = (unsigned char) ((input[i] >> 24) & 0xff);
  }
}

/*
   MDDecodes input (unsigned char) into output (uint32_t). Assumes len is
   a multiple of 4.
 */
static void MDDecode(uint32_t *output, const unsigned char *input, unsigned int len) {
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((uint32_t) input[j]) | (((uint32_t) input[j + 1]) << 8) |
                (((uint32_t) input[j + 2]) << 16) | (((uint32_t) input[j + 3]) << 24);
}

/* MD4 */

#define MD4_F(x, y, z)      ((z) ^ ((x) & ((y) ^ (z))))
#define MD4_G(x, y, z)      (((x) & ((y) | (z))) | ((y) & (z)))
#define MD4_H(x, y, z)      ((x) ^ (y) ^ (z))

#define MD_ROTL32(s, v)        (((v) << (s)) | ((v) >> (32 - (s))))

#define MD4_R1(a, b, c, d, k, s)    a = MD_ROTL32(s, a + MD4_F(b,c,d) + x[k])
#define MD4_R2(a, b, c, d, k, s)    a = MD_ROTL32(s, a + MD4_G(b,c,d) + x[k] + 0x5A827999)
#define MD4_R3(a, b, c, d, k, s)    a = MD_ROTL32(s, a + MD4_H(b,c,d) + x[k] + 0x6ED9EBA1)


static void MD4Transform(uint32_t state[4], const unsigned char block[64]) {
  uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

  MDDecode(x, block, 64);

  /* Round 1 */
  MD4_R1(a, b, c, d, 0, 3);
  MD4_R1(d, a, b, c, 1, 7);
  MD4_R1(c, d, a, b, 2, 11);
  MD4_R1(b, c, d, a, 3, 19);
  MD4_R1(a, b, c, d, 4, 3);
  MD4_R1(d, a, b, c, 5, 7);
  MD4_R1(c, d, a, b, 6, 11);
  MD4_R1(b, c, d, a, 7, 19);
  MD4_R1(a, b, c, d, 8, 3);
  MD4_R1(d, a, b, c, 9, 7);
  MD4_R1(c, d, a, b, 10, 11);
  MD4_R1(b, c, d, a, 11, 19);
  MD4_R1(a, b, c, d, 12, 3);
  MD4_R1(d, a, b, c, 13, 7);
  MD4_R1(c, d, a, b, 14, 11);
  MD4_R1(b, c, d, a, 15, 19);

  /* Round 2 */
  MD4_R2(a, b, c, d, 0, 3);
  MD4_R2(d, a, b, c, 4, 5);
  MD4_R2(c, d, a, b, 8, 9);
  MD4_R2(b, c, d, a, 12, 13);
  MD4_R2(a, b, c, d, 1, 3);
  MD4_R2(d, a, b, c, 5, 5);
  MD4_R2(c, d, a, b, 9, 9);
  MD4_R2(b, c, d, a, 13, 13);
  MD4_R2(a, b, c, d, 2, 3);
  MD4_R2(d, a, b, c, 6, 5);
  MD4_R2(c, d, a, b, 10, 9);
  MD4_R2(b, c, d, a, 14, 13);
  MD4_R2(a, b, c, d, 3, 3);
  MD4_R2(d, a, b, c, 7, 5);
  MD4_R2(c, d, a, b, 11, 9);
  MD4_R2(b, c, d, a, 15, 13);

  /* Round 3 */
  MD4_R3(a, b, c, d, 0, 3);
  MD4_R3(d, a, b, c, 8, 9);
  MD4_R3(c, d, a, b, 4, 11);
  MD4_R3(b, c, d, a, 12, 15);
  MD4_R3(a, b, c, d, 2, 3);
  MD4_R3(d, a, b, c, 10, 9);
  MD4_R3(c, d, a, b, 6, 11);
  MD4_R3(b, c, d, a, 14, 15);
  MD4_R3(a, b, c, d, 1, 3);
  MD4_R3(d, a, b, c, 9, 9);
  MD4_R3(c, d, a, b, 5, 11);
  MD4_R3(b, c, d, a, 13, 15);
  MD4_R3(a, b, c, d, 3, 3);
  MD4_R3(d, a, b, c, 11, 9);
  MD4_R3(c, d, a, b, 7, 11);
  MD4_R3(b, c, d, a, 15, 15);

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
}

static void MD4Init(MD4_CTX *context) {
  context->count[0] = context->count[1] = 0;
  /* Load magic initialization constants.
   */
  context->state[0] = 0x67452301;
  context->state[1] = 0xefcdab89;
  context->state[2] = 0x98badcfe;
  context->state[3] = 0x10325476;
}

/*
   MD4 block update operation. Continues an MD4 message-digest
   operation, processing another message block, and updating the
   context.
 */
static void MD4Update(MD4_CTX *context, const unsigned char *input, size_t inputLen) {
  unsigned int i, index, partLen;

  /* Compute number of bytes mod 64 */
  index = (unsigned int) ((context->count[0] >> 3) & 0x3F);

  /* Update number of bits */
  if ((context->count[0] += ((uint32_t) inputLen << 3))
      < ((uint32_t) inputLen << 3))
    context->count[1]++;
  context->count[1] += ((uint32_t) inputLen >> 29);

  partLen = 64 - index;

  /* Transform as many times as possible.
   */
  if (inputLen >= partLen) {
    memcpy((unsigned char *) &context->buffer[index], (unsigned char *) input, partLen);
    MD4Transform(context->state, context->buffer);

    for (i = partLen; i + 63 < inputLen; i += 64) {
      MD4Transform(context->state, &input[i]);
    }

    index = 0;
  } else {
    i = 0;
  }

  /* Buffer remaining input */
  memcpy((unsigned char *) &context->buffer[index], (unsigned char *) &input[i], inputLen - i);
}

/*
   MD4 finalization. Ends an MD4 message-digest operation, writing the
   the message digest and zeroizing the context.
 */
static void MD4Final(unsigned char digest[16], MD4_CTX *context) {
  unsigned char bits[8];
  unsigned int index, padLen;

  /* Save number of bits */
  MDEncode(bits, context->count, 8);

  /* Pad out to 56 mod 64.
   */
  index = (unsigned int) ((context->count[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  MD4Update(context, MD_PADDING, padLen);

  /* Append length (before MD_PADDING) */
  MD4Update(context, bits, 8);

  /* Store state in digest */
  MDEncode(digest, context->state, 16);

  /* Zeroize sensitive information.
   */
  MD_memset((unsigned char *) context, 0, sizeof(*context));
}

/* MD2 */

static const unsigned char MD2_S[256] = {
    41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6, 19,
    98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188, 76, 130, 202,
    30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24, 138, 23, 229, 18,
    190, 78, 196, 214, 218, 158, 222, 73, 160, 251, 245, 142, 187, 47, 238, 122,
    169, 104, 121, 145, 21, 178, 7, 63, 148, 194, 16, 137, 11, 34, 95, 33,
    128, 127, 93, 154, 90, 144, 50, 39, 53, 62, 204, 231, 191, 247, 151, 3,
    255, 25, 48, 179, 72, 165, 181, 209, 215, 94, 146, 42, 172, 86, 170, 198,
    79, 184, 56, 210, 150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241,
    69, 157, 112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2,
    27, 96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
    85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197, 234, 38,
    44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65, 129, 77, 82,
    106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123, 8, 12, 189, 177, 74,
    120, 136, 149, 139, 227, 99, 232, 109, 233, 203, 213, 254, 59, 0, 29, 57,
    242, 239, 183, 14, 102, 88, 208, 228, 166, 119, 114, 248, 235, 117, 75, 10,
    49, 68, 80, 180, 143, 237, 31, 26, 219, 153, 141, 51, 159, 17, 131, 20};

static void MD2Init(MD2_CTX *context) {
  memset(context, 0, sizeof(MD2_CTX));
}

static void MD2_Transform(MD2_CTX *context, const unsigned char *block) {
  unsigned char i, j, t = 0;

  for (i = 0; i < 16; i++) {
    context->state[16 + i] = block[i];
    context->state[32 + i] = (context->state[16 + i] ^ context->state[i]);
  }

  for (i = 0; i < 18; i++) {
    for (j = 0; j < 48; j++) {
      t = context->state[j] = context->state[j] ^ MD2_S[t];
    }
    t += i;
  }

  /* Update checksum -- must be after transform to avoid fouling up last message block */
  t = context->checksum[15];
  for (i = 0; i < 16; i++) {
    t = context->checksum[i] ^= MD2_S[block[i] ^ t];
  }
}

static void MD2Update(MD2_CTX *context, const unsigned char *buf, size_t len) {
  const unsigned char *p = buf, *e = buf + len;

  if (context->in_buffer) {
    if (context->in_buffer + len < 16) {
      /* Not enough for block, just pass into buffer */
      memcpy(context->buffer + context->in_buffer, p, len);
      context->in_buffer += (char) len;
      return;
    }
    /* Put buffered data together with inbound for a single block */
    memcpy(context->buffer + context->in_buffer, p, 16 - context->in_buffer);
    MD2_Transform(context, context->buffer);
    p += 16 - context->in_buffer;
    context->in_buffer = 0;
  }

  /* Process as many whole blocks as remain */
  while ((p + 16) <= e) {
    MD2_Transform(context, p);
    p += 16;
  }

  /* Copy remaining data to buffer */
  if (p < e) {
    memcpy(context->buffer, p, e - p);
    context->in_buffer = (char) (e - p);
  }
}

static void MD2Final(unsigned char output[16], MD2_CTX *context) {
  memset(context->buffer + context->in_buffer, 16 - context->in_buffer, 16 - context->in_buffer);
  MD2_Transform(context, context->buffer);
  MD2_Transform(context, context->checksum);

  memcpy(output, context->state, 16);
}

static char *MDDigestToString(unsigned char digest[16]) {
  char *result = (char *) calloc(33, sizeof(char));
  for (int i = 0; i < 16; i++)
    sprintf (result + (i * 2), "%02x", digest[i]);

  return result;
}

static char *MD4String(unsigned char *string, int length) {
  MD4_CTX context;
  unsigned char digest[16];

  MD4Init(&context);
  MD4Update(&context, string, length);
  MD4Final(digest, &context);

  return MDDigestToString(digest);
}

static char *MD2String(unsigned char *string, int length) {
  MD2_CTX context;
  unsigned char digest[16];

  MD2Init(&context);
  MD2Update(&context, string, length);
  MD2Final(digest, &context);

  return MDDigestToString(digest);
}


#endif //BLADE_MODULE_HASH_MD_H
