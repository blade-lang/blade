/**
 * Adapted from: libkeccak-tiny
 *
 * A single-file implementation of SHA-3 and SHAKE.
 *
 * Implementor: David Leon Gil
 * License: CC0, attribution kindly requested. Blame taken too,
 * but not liability.
 */
#ifndef BLADE_SHA3_H
#define BLADE_SHA3_H

#define __STDC_WANT_LIB_EXT1__ 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__) || defined(_WIN32)
#define memset_s(W,WL,V,OL) memset(W,V,OL)
#endif

/******** The Keccak-f[1600] permutation ********/

/*** Constants. ***/
static const uint8_t rho[24] = \
  { 1,  3,   6, 10, 15, 21,
    28, 36, 45, 55,  2, 14,
    27, 41, 56,  8, 25, 43,
    62, 18, 39, 61, 20, 44};
static const uint8_t pi[24] = \
  {10,  7, 11, 17, 18, 3,
   5, 16,  8, 21, 24, 4,
   15, 23, 19, 13, 12, 2,
   20, 14, 22,  9, 6,  1};
static const uint64_t RC[24] = \
  {1ULL, 0x8082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
   0x808bULL, 0x80000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
   0x8aULL, 0x88ULL, 0x80008009ULL, 0x8000000aULL,
   0x8000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
   0x8000000000008002ULL, 0x8000000000000080ULL, 0x800aULL, 0x800000008000000aULL,
   0x8000000080008081ULL, 0x8000000000008080ULL, 0x80000001ULL, 0x8000000080008008ULL};

/*** Helper macros to unroll the permutation. ***/
#define sha3_rol(x, s) (((x) << s) | ((x) >> (64 - s)))
#define SHA3_REPEAT6(e) e e e e e e
#define SHA3_REPEAT24(e) SHA3_REPEAT6(e e e e)
#define SHA3_REPEAT5(e) e e e e e
#define SHA3_FOR5(v, s, e) \
  v = 0;            \
  SHA3_REPEAT5(e; v += s;)

/*** Keccak-f[1600] ***/
static inline void keccakf(void* state) {
  uint64_t* a = (uint64_t*)state;
  uint64_t b[5] = {0};
  uint64_t t = 0;
  uint8_t x, y, i = 0;

  SHA3_REPEAT24(
    // Theta
    SHA3_FOR5(x, 1,
      b[x] = 0;
      SHA3_FOR5(y, 5,
        b[x] ^= a[x + y]; ))
    SHA3_FOR5(x, 1,
      SHA3_FOR5(y, 5,
        a[y + x] ^= b[(x + 4) % 5] ^ sha3_rol(b[(x + 1) % 5], 1); ))
    // Rho and pi
    t = a[1];
    x = 0;
    SHA3_REPEAT24(b[0] = a[pi[x]];
      a[pi[x]] = sha3_rol(t, rho[x]);
      t = b[0];
      x++; )
    // Chi
    SHA3_FOR5(y,
      5,
      SHA3_FOR5(x, 1,
        b[x] = a[y + x];)
        SHA3_FOR5(x, 1,
          a[y + x] = b[x] ^ ((~b[(x + 1) % 5]) & b[(x + 2) % 5]); ))
    // Iota
    a[0] ^= RC[i];
    i++; )
}

/******** The FIPS202-defined functions. ********/

/*** Some helper macros. ***/

#define SHA3_(S) do { S } while (0)
#define SHA3_FOR(i, ST, L, S) \
  SHA3_(for (size_t i = 0; i < L; i += ST) { S; })
#define mkapply_ds(NAME, S)                                          \
  static inline void NAME(uint8_t* dst,                              \
                          const uint8_t* src,                        \
                          size_t len) {                              \
    SHA3_FOR(i, 1, len, S);                                               \
  }
#define mkapply_sd(NAME, S)                                          \
  static inline void NAME(const uint8_t* src,                        \
                          uint8_t* dst,                              \
                          size_t len) {                              \
    SHA3_FOR(i, 1, len, S);                                               \
  }

mkapply_ds(xorin, dst[i] ^= src[i])  // xorin
mkapply_sd(setout, dst[i] = src[i])  // setout

#define P keccakf
#define Plen 200

// Fold P*F over the full blocks of an input.
#define SHA3_foldP(I, L, F) \
  while (L >= rate) {  \
    F(a, I, rate);     \
    P(a);              \
    I += rate;         \
    L -= rate;         \
  }

/** The sponge-based hash construction. **/
static inline int hash(uint8_t* out, size_t outlen,
                       const uint8_t* in, size_t inlen,
                       size_t rate, uint8_t delim) {
  if ((out == NULL) || ((in == NULL) && inlen != 0) || (rate >= Plen)) {
    return -1;
  }
  uint8_t a[Plen] = {0};
  // Absorb input.
  SHA3_foldP(in, inlen, xorin);
  // Xor in the DS and pad frame.
  a[inlen] ^= delim;
  a[rate - 1] ^= 0x80;
  // Xor in the last block.
  xorin(a, in, inlen);
  // Apply P
  P(a);
  // Squeeze output.
  SHA3_foldP(out, outlen, setout);
  setout(a, out, outlen);
  memset_s(a, 200, 0, 200);
  return 0;
}

char *sha3_keccak_bits(const uint8_t* in, size_t inlen, int delim, int bits, int outlen) {
    uint8_t digest[outlen];
    char *result = (char *) calloc((outlen * 2)+1, sizeof(char));
    if(hash(digest, outlen, in, inlen, 200 - (bits / 4), delim) == 0) {
      for (int i = 0; i < outlen; i++)
        sprintf (result + (i * 2), "%02x", digest[i]);
    }
    return result;
}

#endif //BLADE_SHA3_H
