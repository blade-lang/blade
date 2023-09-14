/**
 * Adapted from the reference implementation at https://datatracker.ietf.org/doc/html/rfc7693
 */

#ifndef BLADE_BLAKE2B_H
#define BLADE_BLAKE2B_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// state context
typedef struct {
  uint8_t b[128];                     // input buffer
  uint64_t h[8];                      // chained state
  uint64_t t[2];                      // total number of bytes
  size_t c;                           // pointer for b[]
  size_t outlen;                      // digest size
} blake2b_ctx;

// Cyclic right rotation.

#ifndef BLAKE_ROTR64
#define BLAKE_ROTR64(x, y)  (((x) >> (y)) ^ ((x) << (64 - (y))))
#endif

// Little-endian byte access.

#define BLAKE2B_GET64(p)                            \
    (((uint64_t) ((uint8_t *) (p))[0]) ^        \
    (((uint64_t) ((uint8_t *) (p))[1]) << 8) ^  \
    (((uint64_t) ((uint8_t *) (p))[2]) << 16) ^ \
    (((uint64_t) ((uint8_t *) (p))[3]) << 24) ^ \
    (((uint64_t) ((uint8_t *) (p))[4]) << 32) ^ \
    (((uint64_t) ((uint8_t *) (p))[5]) << 40) ^ \
    (((uint64_t) ((uint8_t *) (p))[6]) << 48) ^ \
    (((uint64_t) ((uint8_t *) (p))[7]) << 56))

// G Mixing function.

#define BLAKE2B_G(a, b, c, d, x, y) {   \
    v[a] = v[a] + v[b] + x;         \
    v[d] = BLAKE_ROTR64(v[d] ^ v[a], 32); \
    v[c] = v[c] + v[d];             \
    v[b] = BLAKE_ROTR64(v[b] ^ v[c], 24); \
    v[a] = v[a] + v[b] + y;         \
    v[d] = BLAKE_ROTR64(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = BLAKE_ROTR64(v[b] ^ v[c], 63); }

// Initialization Vector.

static const uint64_t blake2b_iv[8] = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,
    0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F,
    0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179
};

// Compression function. "last" flag indicates last block.

static void blake2b_compress(blake2b_ctx *ctx, int last)
{
  const uint8_t sigma[12][16] = {
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
      { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
      { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
      { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
      { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
      { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
      { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
      { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
      { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
      { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
      { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }
  };
  int i;
  uint64_t v[16], m[16];

  for (i = 0; i < 8; i++) {           // init work variables
    v[i] = ctx->h[i];
    v[i + 8] = blake2b_iv[i];
  }
  v[12] ^= ctx->t[0];                 // low 64 bits of offset
  v[13] ^= ctx->t[1];                 // high 64 bits
  if (last)                           // last block flag set ?
    v[14] = ~v[14];

  for (i = 0; i < 16; i++)            // get little-endian words
    m[i] = BLAKE2B_GET64(&ctx->b[8 * i]);

  for (i = 0; i < 12; i++) {          // twelve rounds
    BLAKE2B_G( 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
    BLAKE2B_G( 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
    BLAKE2B_G( 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
    BLAKE2B_G( 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
    BLAKE2B_G( 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
    BLAKE2B_G( 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
    BLAKE2B_G( 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
    BLAKE2B_G( 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
  }

  for( i = 0; i < 8; ++i )
    ctx->h[i] ^= v[i] ^ v[i + 8];
}

// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 64 gives the digest size in bytes.
//      Secret key (also <= 64 bytes) is optional (keylen = 0).

int blake2b_init(blake2b_ctx *ctx)        // (keylen=0: no key)
{
  size_t i;
  size_t outlen = 64;
  size_t keylen = 0;

  if (outlen == 0 || outlen > 64 || keylen > 64)
    return -1;                      // illegal parameters

  for (i = 0; i < 8; i++)             // state, "param block"
    ctx->h[i] = blake2b_iv[i];
  ctx->h[0] ^= 0x01010000 ^ (keylen << 8) ^ outlen;

  ctx->t[0] = 0;                      // input count low word
  ctx->t[1] = 0;                      // input count high word
  ctx->c = 0;                         // pointer within buffer
  ctx->outlen = outlen;
  for (i = keylen; i < 128; i++)      // zero input block
    ctx->b[i] = 0;

  return 0;
}

// Add "inlen" bytes from "in" into the hash.

void blake2b_update(blake2b_ctx *ctx,
                    const void *in, size_t inlen)       // data bytes
{
  size_t i;

  for (i = 0; i < inlen; i++) {
    if (ctx->c == 128) {            // buffer full ?
      ctx->t[0] += ctx->c;        // add counters
      if (ctx->t[0] < ctx->c)     // carry overflow ?
        ctx->t[1]++;            // high word
      blake2b_compress(ctx, 0);   // compress (not last)
      ctx->c = 0;                 // counter to zero
    }
    ctx->b[ctx->c++] = ((const uint8_t *) in)[i];
  }
}

// Generate the message digest (size given in init).
//      Result placed in "out".

void blake2b_final(blake2b_ctx *ctx, void *out)
{
  size_t i;

  ctx->t[0] += ctx->c;                // mark last block offset
  if (ctx->t[0] < ctx->c)             // carry overflow
    ctx->t[1]++;                    // high word

  while (ctx->c < 128)                // fill up with zeros
    ctx->b[ctx->c++] = 0;
  blake2b_compress(ctx, 1);           // final block flag = 1
  // little endian convert and store
  for (i = 0; i < ctx->outlen; i++) {
    ((uint8_t *) out)[i] =
        (ctx->h[i >> 3] >> (8 * (i & 7))) & 0xFF;
  }
}

// Convenience function for all-in-one computation.

char *blake2b(const void *in, size_t inlen)
{
  blake2b_ctx ctx;
  blake2b_init(&ctx);
  blake2b_update(&ctx, in, inlen);

  unsigned char digest[64];
  blake2b_final(&ctx, digest);

  char *result = (char *) calloc(129, sizeof(char));
  for (int i = 0; i < 64; i++)
    sprintf (result + (i * 2), "%02x", digest[i]);

  return result;
}

#endif

#ifndef BLADE_BLAKE2S_H
#define BLADE_BLAKE2S_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// state context
typedef struct {
  uint8_t b[64];                      // input buffer
  uint32_t h[8];                      // chained state
  uint32_t t[2];                      // total number of bytes
  size_t c;                           // pointer for b[]
  size_t outlen;                      // digest size
} blake2s_ctx;

// Cyclic right rotation.

#ifndef BLAKE_ROTR32
#define BLAKE_ROTR32(x, y)  (((x) >> (y)) ^ ((x) << (32 - (y))))
#endif

// Little-endian byte access.

#define BLAKE2S_GET32(p)                            \
    (((uint32_t) ((uint8_t *) (p))[0]) ^        \
    (((uint32_t) ((uint8_t *) (p))[1]) << 8) ^  \
    (((uint32_t) ((uint8_t *) (p))[2]) << 16) ^ \
    (((uint32_t) ((uint8_t *) (p))[3]) << 24))

// Mixing function G.

#define BLAKE2S_G(a, b, c, d, x, y) {   \
    v[a] = v[a] + v[b] + x;         \
    v[d] = BLAKE_ROTR32(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = BLAKE_ROTR32(v[b] ^ v[c], 12); \
    v[a] = v[a] + v[b] + y;         \
    v[d] = BLAKE_ROTR32(v[d] ^ v[a], 8);  \
    v[c] = v[c] + v[d];             \
    v[b] = BLAKE_ROTR32(v[b] ^ v[c], 7); }

// Initialization Vector.

static const uint32_t blake2s_iv[8] =
    {
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
        0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
    };

// Compression function. "last" flag indicates last block.

static void blake2s_compress(blake2s_ctx *ctx, int last)
{
  const uint8_t sigma[10][16] = {
      { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
      { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
      { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
      { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
      { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
      { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
      { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
      { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
      { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
      { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
  };
  int i;
  uint32_t v[16], m[16];

  for (i = 0; i < 8; i++) {           // init work variables
    v[i] = ctx->h[i];
    v[i + 8] = blake2s_iv[i];
  }

  v[12] ^= ctx->t[0];                 // low 32 bits of offset
  v[13] ^= ctx->t[1];                 // high 32 bits
  if (last)                           // last block flag set ?
    v[14] = ~v[14];
  for (i = 0; i < 16; i++)            // get little-endian words
    m[i] = BLAKE2S_GET32(&ctx->b[4 * i]);

  for (i = 0; i < 10; i++) {          // ten rounds
    BLAKE2S_G( 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
    BLAKE2S_G( 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
    BLAKE2S_G( 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
    BLAKE2S_G( 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
    BLAKE2S_G( 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
    BLAKE2S_G( 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
    BLAKE2S_G( 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
    BLAKE2S_G( 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
  }

  for( i = 0; i < 8; ++i )
    ctx->h[i] ^= v[i] ^ v[i + 8];
}

// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 32 gives the digest size in bytes.
//      Secret key (also <= 32 bytes) is optional (keylen = 0).

int blake2s_init(blake2s_ctx *ctx)     // (keylen=0: no key)
{
  size_t i;
  size_t outlen = 32;
  size_t keylen = 0;

  for (i = 0; i < 8; i++)             // state, "param block"
    ctx->h[i] = blake2s_iv[i];
  ctx->h[0] ^= 0x01010000 ^ (keylen << 8) ^ outlen;

  ctx->t[0] = 0;                      // input count low word
  ctx->t[1] = 0;                      // input count high word
  ctx->c = 0;                         // pointer within buffer
  ctx->outlen = outlen;

  for (i = keylen; i < 64; i++)       // zero input block
    ctx->b[i] = 0;

  return 0;
}

// Add "inlen" bytes from "in" into the hash.

void blake2s_update(blake2s_ctx *ctx,
                    const void *in, size_t inlen)       // data bytes
{
  size_t i;

  for (i = 0; i < inlen; i++) {
    if (ctx->c == 64) {             // buffer full ?
      ctx->t[0] += ctx->c;        // add counters
      if (ctx->t[0] < ctx->c)     // carry overflow ?
        ctx->t[1]++;            // high word
      blake2s_compress(ctx, 0);   // compress (not last)
      ctx->c = 0;                 // counter to zero
    }
    ctx->b[ctx->c++] = ((const uint8_t *) in)[i];
  }
}

// Generate the message digest (size given in init).
//      Result placed in "out".

void blake2s_final(blake2s_ctx *ctx, void *out)
{
  size_t i;

  ctx->t[0] += ctx->c;                // mark last block offset
  if (ctx->t[0] < ctx->c)             // carry overflow
    ctx->t[1]++;                    // high word

  while (ctx->c < 64)                 // fill up with zeros
    ctx->b[ctx->c++] = 0;
  blake2s_compress(ctx, 1);           // final block flag = 1

  // little endian convert and store
  for (i = 0; i < ctx->outlen; i++) {
    ((uint8_t *) out)[i] =
        (ctx->h[i >> 2] >> (8 * (i & 3))) & 0xFF;
  }
}

// Convenience function for all-in-one computation.

char *blake2s(const void *in, size_t inlen) {
  blake2s_ctx ctx;
  blake2s_init(&ctx);
  blake2s_update(&ctx, in, inlen);

  unsigned char digest[32];
  blake2s_final(&ctx, digest);

  char *result = (char *) calloc(65, sizeof(char));
  for (int i = 0; i < 32; i++)
    sprintf (result + (i * 2), "%02x", digest[i]);

  return result;
}

#endif