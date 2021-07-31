#ifndef BLADE_MODULE_HASH_SHA512_H
#define BLADE_MODULE_HASH_SHA512_H

#include "common.h"

/*
 * AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
 *
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Modified by Jelte Jansen to fit in ldns, and not clash with any
 * system-defined SHA code.
 * Changes:
 * - Renamed (external) functions and constants to fit ldns style
 * - Removed _End and _Data functions
 * - Added ldns_shaX(data, len, digest) convenience functions
 * - Removed prototypes of _Transform functions and made those static
 * Modified by Wouter, and trimmed, to provide SHA512String for getentropy_fallback.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: sha2.c,v 1.1 2001/11/08 00:01:51 adg Exp adg $
 */

#include <stdint.h>
#include <string.h>  /* memcpy()/memset() or bcopy()/bzero() */
#include <assert.h>  /* assert() */

/* do we have sha512 header defs */
#ifndef SHA512_DIGEST_LENGTH
#define SHA512_BLOCK_LENGTH    128
#define SHA512_DIGEST_LENGTH    64
#define SHA512_DIGEST_STRING_LENGTH  (SHA512_DIGEST_LENGTH * 2 + 1)
typedef struct _SHA512_CTX {
  uint64_t state[8];
  uint64_t bitcount[2];
  uint8_t buffer[SHA512_BLOCK_LENGTH];
} SHA512_CTX;
#endif /* do we have sha512 header defs */

typedef uint8_t sha2_byte;  /* Exactly 1 byte */
typedef uint32_t sha2_word32;  /* Exactly 4 bytes */
#ifdef S_SPLINT_S
typedef unsigned long long sha2_word64; /* lint 8 bytes */
#else
typedef uint64_t sha2_word64;  /* Exactly 8 bytes */
#endif

/*** SHA-256/384/512 Various Length Definitions ***********************/
#define SHA512_SHORT_BLOCK_LENGTH  (SHA512_BLOCK_LENGTH - 16)


/*** ENDIAN REVERSAL MACROS *******************************************/
#if IS_LITTLE_ENDIAN
#define SHA512_REVERSE32(w, x)  { \
  sha2_word32 tmp = (w); \
  tmp = (tmp >> 16) | (tmp << 16); \
  (x) = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); \
}
#ifndef S_SPLINT_S
#define SHA512_REVERSE64(w, x)  { \
  sha2_word64 tmp = (w); \
  tmp = (tmp >> 32) | (tmp << 32); \
  tmp = ((tmp & 0xff00ff00ff00ff00ULL) >> 8) | \
        ((tmp & 0x00ff00ff00ff00ffULL) << 8); \
  (x) = ((tmp & 0xffff0000ffff0000ULL) >> 16) | \
        ((tmp & 0x0000ffff0000ffffULL) << 16); \
}
#else /* splint */
#define SHA512_REVERSE64(w,x) /* splint */
#endif /* splint */
#endif /* IS_LITTLE_ENDIAN */

/*
 * Macro for incrementally adding the unsigned 64-bit integer n to the
 * unsigned 128-bit integer (represented using a two-element array of
 * 64-bit words):
 */
#define SHA512_ADDINC128(w, n)  { \
  (w)[0] += (sha2_word64)(n); \
  if ((w)[0] < (n)) { \
    (w)[1]++; \
  } \
}
#ifdef S_SPLINT_S
#define SHA512_ADDINC128(w,n) /* splint */
#endif

/*
 * Macros for copying blocks of memory and for zeroing out ranges
 * of memory.  Using these macros makes it easy to switch from
 * using memset()/memcpy() and using bzero()/bcopy().
 *
 * Please define either SHA2_USE_MEMSET_MEMCPY or define
 * SHA2_USE_BZERO_BCOPY depending on which function set you
 * choose to use:
 */
#if !defined(SHA2_USE_MEMSET_MEMCPY) && !defined(SHA2_USE_BZERO_BCOPY)
/* Default to memset()/memcpy() if no option is specified */
#define  SHA2_USE_MEMSET_MEMCPY  1
#endif
#if defined(SHA2_USE_MEMSET_MEMCPY) && defined(SHA2_USE_BZERO_BCOPY)
/* Abort with an error if BOTH options are defined */
#error Define either SHA2_USE_MEMSET_MEMCPY or SHA2_USE_BZERO_BCOPY, not both!
#endif

#ifdef SHA2_USE_MEMSET_MEMCPY
#define SHA512_MEMSET_BZERO(p, l)  memset((p), 0, (l))
#define SHA512_MEMCPY_BCOPY(d, s, l)  memcpy((d), (s), (l))
#endif
#ifdef SHA2_USE_BZERO_BCOPY
#define SHA512_MEMSET_BZERO(p,l)	bzero((p), (l))
#define SHA512_MEMCPY_BCOPY(d,s,l)	bcopy((s), (d), (l))
#endif


/*** THE SIX LOGICAL FUNCTIONS ****************************************/
/*
 * Bit shifting and rotation (used by the six SHA-XYZ logical functions:
 *
 *   NOTE:  The naming of R and S appears backwards here (R is a SHIFT and
 *   S is a ROTATION) because the SHA-256/384/512 description document
 *   (see http://csrc.nist.gov/cryptval/shs/sha256-384-512.pdf) uses this
 *   same "backwards" definition.
 */
/* Shift-right (used in SHA-256, SHA-384, and SHA-512): */
#define SHA512_R(b, x)    ((x) >> (b))
/* 64-bit Rotate-right (used in SHA-384 and SHA-512): */
#define SHA512_S64(b, x)  (((x) >> (b)) | ((x) << (64 - (b))))

/* Two of six logical functions used in SHA-256, SHA-384, and SHA-512: */
#define SHA512_Ch(x, y, z)  (((x) & (y)) ^ ((~(x)) & (z)))
#define SHA512_Maj(x, y, z)  (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Four of six logical functions used in SHA-384 and SHA-512: */
#define SHA512_Sigma0_512(x)  (SHA512_S64(28, (x)) ^ SHA512_S64(34, (x)) ^ SHA512_S64(39, (x)))
#define SHA512_Sigma1_512(x)  (SHA512_S64(14, (x)) ^ SHA512_S64(18, (x)) ^ SHA512_S64(41, (x)))
#define SHA512_sigma0_512(x)  (SHA512_S64( 1, (x)) ^ SHA512_S64( 8, (x)) ^ SHA512_R( 7,   (x)))
#define SHA512_sigma1_512(x)  (SHA512_S64(19, (x)) ^ SHA512_S64(61, (x)) ^ SHA512_R( 6,   (x)))

/*** SHA-XYZ INITIAL HASH VALUES AND CONSTANTS ************************/
/* Hash constant words K for SHA-384 and SHA-512: */
static const sha2_word64 K512[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
    0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

/* initial hash value H for SHA-384 */
static const sha2_word64 sha384_initial_hash_value[8] = {
    0xcbbb9d5dc1059ed8ULL,
    0x629a292a367cd507ULL,
    0x9159015a3070dd17ULL,
    0x152fecd8f70e5939ULL,
    0x67332667ffc00b31ULL,
    0x8eb44a8768581511ULL,
    0xdb0c2e0d64f98fa7ULL,
    0x47b5481dbefa4fa4ULL
};

/* initial hash value H for SHA-512 */
static const sha2_word64 sha512_initial_hash_value[8] = {
    0x6a09e667f3bcc908ULL,
    0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL,
    0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL,
    0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL,
    0x5be0cd19137e2179ULL
};

typedef union {
  uint8_t *theChars;
  uint64_t *theLongs;
} ldns_sha2_buffer_union;

static void SHA384_Init(SHA512_CTX *context) {
  if (context == (SHA512_CTX *) 0) {
    return;
  }
  SHA512_MEMCPY_BCOPY(context->state, sha384_initial_hash_value, SHA512_DIGEST_LENGTH);
  SHA512_MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH);
  context->bitcount[0] = context->bitcount[1] = 0;
}

/*** SHA-512: *********************************************************/
static void SHA512_Init(SHA512_CTX *context) {
  if (context == (SHA512_CTX *) 0) {
    return;
  }
  SHA512_MEMCPY_BCOPY(context->state, sha512_initial_hash_value, SHA512_DIGEST_LENGTH);
  SHA512_MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH);
  context->bitcount[0] = context->bitcount[1] = 0;
}

static void SHA512_Transform(SHA512_CTX *context,
                             const sha2_word64 *data) {
  sha2_word64 a, b, c, d, e, f, g, h, s0, s1;
  sha2_word64 T1, T2, *W512 = (sha2_word64 *) context->buffer;
  int j;

  /* initialize registers with the prev. intermediate value */
  a = context->state[0];
  b = context->state[1];
  c = context->state[2];
  d = context->state[3];
  e = context->state[4];
  f = context->state[5];
  g = context->state[6];
  h = context->state[7];

  j = 0;
  do {
#if IS_LITTLE_ENDIAN
    /* Convert TO host byte order */
    SHA512_REVERSE64(*data++, W512[j]);
    /* Apply the SHA-512 compression function to update a..h */
    T1 = h + SHA512_Sigma1_512(e) + Ch(e, f, g) + K512[j] + W512[j];
#else /* IS_LITTLE_ENDIAN */
    /* Apply the SHA-512 compression function to update a..h with copy */
    T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] + (W512[j] = *data++);
#endif /* IS_LITTLE_ENDIAN */
    T2 = SHA512_Sigma0_512(a) + Maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + T1;
    d = c;
    c = b;
    b = a;
    a = T1 + T2;

    j++;
  } while (j < 16);

  do {
    /* Part of the message block expansion: */
    s0 = W512[(j + 1) & 0x0f];
    s0 = SHA512_sigma0_512(s0);
    s1 = W512[(j + 14) & 0x0f];
    s1 = SHA512_sigma1_512(s1);

    /* Apply the SHA-512 compression function to update a..h */
    T1 = h + SHA512_Sigma1_512(e) + Ch(e, f, g) + K512[j] +
         (W512[j & 0x0f] += s1 + W512[(j + 9) & 0x0f] + s0);
    T2 = SHA512_Sigma0_512(a) + Maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + T1;
    d = c;
    c = b;
    b = a;
    a = T1 + T2;

    j++;
  } while (j < 80);

  /* Compute the current intermediate hash value */
  context->state[0] += a;
  context->state[1] += b;
  context->state[2] += c;
  context->state[3] += d;
  context->state[4] += e;
  context->state[5] += f;
  context->state[6] += g;
  context->state[7] += h;

  /* Clean up */
  a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

static void SHA512_Update(SHA512_CTX *context, void *datain, size_t len) {
  size_t freespace, usedspace;
  const sha2_byte *data = (const sha2_byte *) datain;

  if (len == 0) {
    /* Calling with no data is valid - we do nothing */
    return;
  }

  /* Sanity check: */
  assert(context != (SHA512_CTX *) 0 && data != (sha2_byte *) 0);

  usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
  if (usedspace > 0) {
    /* Calculate how much free space is available in the buffer */
    freespace = SHA512_BLOCK_LENGTH - usedspace;

    if (len >= freespace) {
      /* Fill the buffer completely and process it */
      SHA512_MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
      SHA512_ADDINC128(context->bitcount, freespace << 3);
      len -= freespace;
      data += freespace;
      SHA512_Transform(context, (sha2_word64 *) context->buffer);
    } else {
      /* The buffer is not yet full */
      SHA512_MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
      SHA512_ADDINC128(context->bitcount, len << 3);
      /* Clean up: */
      usedspace = freespace = 0;
      return;
    }
  }
  while (len >= SHA512_BLOCK_LENGTH) {
    /* Process as many complete blocks as we can */
    SHA512_Transform(context, (sha2_word64 *) data);
    SHA512_ADDINC128(context->bitcount, SHA512_BLOCK_LENGTH << 3);
    len -= SHA512_BLOCK_LENGTH;
    data += SHA512_BLOCK_LENGTH;
  }
  if (len > 0) {
    /* There's left-overs, so save 'em */
    SHA512_MEMCPY_BCOPY(context->buffer, data, len);
    SHA512_ADDINC128(context->bitcount, len << 3);
  }
  /* Clean up: */
  usedspace = freespace = 0;
}

static void SHA512_Last(SHA512_CTX *context) {
  size_t usedspace;
  ldns_sha2_buffer_union cast_var;

  usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
#if IS_LITTLE_ENDIAN
  /* Convert FROM host byte order */
  SHA512_REVERSE64(context->bitcount[0], context->bitcount[0]);
  SHA512_REVERSE64(context->bitcount[1], context->bitcount[1]);
#endif
  if (usedspace > 0) {
    /* Begin padding with a 1 bit: */
    context->buffer[usedspace++] = 0x80;

    if (usedspace <= SHA512_SHORT_BLOCK_LENGTH) {
      /* Set-up for the last transform: */
      SHA512_MEMSET_BZERO(&context->buffer[usedspace], SHA512_SHORT_BLOCK_LENGTH - usedspace);
    } else {
      if (usedspace < SHA512_BLOCK_LENGTH) {
        SHA512_MEMSET_BZERO(&context->buffer[usedspace], SHA512_BLOCK_LENGTH - usedspace);
      }
      /* Do second-to-last transform: */
      SHA512_Transform(context, (sha2_word64 *) context->buffer);

      /* And set-up for the last transform: */
      SHA512_MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH - 2);
    }
  } else {
    /* Prepare for final transform: */
    SHA512_MEMSET_BZERO(context->buffer, SHA512_SHORT_BLOCK_LENGTH);

    /* Begin padding with a 1 bit: */
    *context->buffer = 0x80;
  }
  /* Store the length of input data (in bits): */
  cast_var.theChars = context->buffer;
  cast_var.theLongs[SHA512_SHORT_BLOCK_LENGTH / 8] = context->bitcount[1];
  cast_var.theLongs[SHA512_SHORT_BLOCK_LENGTH / 8 + 1] = context->bitcount[0];

  /* final transform: */
  SHA512_Transform(context, (sha2_word64 *) context->buffer);
}

static void SHA512_Final(sha2_byte digest[], SHA512_CTX *context) {
  sha2_word64 *d = (sha2_word64 *) digest;

  /* Sanity check: */
  assert(context != (SHA512_CTX *) 0);

  /* If no digest buffer is passed, we don't bother doing this: */
  if (digest != (sha2_byte *) 0) {
    SHA512_Last(context);

    /* Save the hash data for output: */
#if IS_LITTLE_ENDIAN
    {
      /* Convert TO host byte order */
      int j;
      for (j = 0; j < 8; j++) {
        SHA512_REVERSE64(context->state[j], context->state[j]);
        *d++ = context->state[j];
      }
    }
#else
    SHA512_MEMCPY_BCOPY(d, context->state, SHA512_DIGEST_LENGTH);
#endif
  }

  /* Zero out state data */
  SHA512_MEMSET_BZERO(context, sizeof(SHA512_CTX));
}

static char *SHA384String(void *data, unsigned int data_len) {
  unsigned char digest[64];

  SHA512_CTX ctx;
  SHA384_Init(&ctx);
  SHA512_Update(&ctx, data, data_len);
  SHA512_Final(digest, &ctx);

  char *result = (char *) calloc(97, sizeof(char));
  for (int i = 0; i < 48; i++)
    sprintf (result + (i * 2), "%02x", digest[i]);

  return result;
}

static char *SHA512String(void *data, unsigned int data_len) {
  unsigned char digest[64];

  SHA512_CTX ctx;
  SHA512_Init(&ctx);
  SHA512_Update(&ctx, data, data_len);
  SHA512_Final(digest, &ctx);

  char *result = (char *) calloc(129, sizeof(char));
  for (int i = 0; i < 64; i++)
    sprintf (result + (i * 2), "%02x", digest[i]);

  return result;
}

#endif //BLADE_MODULE_HASH_SHA512_H
