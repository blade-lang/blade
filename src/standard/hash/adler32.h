#ifndef BLADE_MODULE_HASH_ADLER32_H
#define BLADE_MODULE_HASH_ADLER32_H


#define NO_DIVIDE

#include <stddef.h>

#define ADLER32_BASE 65521U     /* largest prime smaller than 65536 */
#define ADLER32_NMAX 5552
/* ADLER32_NMAX is the largest n such that 255n(n+1)/2 + (n+1)(ADLER32_BASE-1) <= 2^32-1 */

#define ADLER32_DO1(buf, i)  {adler += (buf)[i]; sum2 += adler;}
#define ADLER32_DO2(buf, i)  ADLER32_DO1(buf,i); ADLER32_DO1(buf,(i)+1);
#define ADLER32_DO4(buf, i)  ADLER32_DO2(buf,i); ADLER32_DO2(buf,(i)+2);
#define ADLER32_DO8(buf, i)  ADLER32_DO4(buf,i); ADLER32_DO4(buf,(i)+4);
#define ADLER32_DO16(buf)   ADLER32_DO8(buf,0); ADLER32_DO8(buf,8);

/* use NO_DIVIDE if your processor does not do division in hardware --
   try it both ways to see which is faster */
#ifdef NO_DIVIDE
/* note that this assumes ADLER32_BASE is 65521, where 65536 % 65521 == 15
   (thank you to John Reiser for pointing this out) */
#  define ADLER32_CHOP(a) \
  do { \
    unsigned long tmp = a >> 16; \
    a &= 0xffffUL; \
    a += (tmp << 4) - tmp; \
  } while (0)
#  define ADLER32_MOD28(a) \
  do { \
    ADLER32_CHOP(a); \
    if (a >= ADLER32_BASE) a -= ADLER32_BASE; \
  } while (0)
#  define ADLER32_MOD(a) \
  do { \
    ADLER32_CHOP(a); \
    ADLER32_MOD28(a); \
  } while (0)
#  define ADLER32_MOD63(a) \
  do { /* this assumes a is not negative */ \
    off64_t tmp = a >> 32; \
    a &= 0xffffffffL; \
    a += (tmp << 8) - (tmp << 5) + tmp; \
    tmp = a >> 16; \
    a &= 0xffffL; \
    a += (tmp << 4) - tmp; \
    tmp = a >> 16; \
    a &= 0xffffL; \
    a += (tmp << 4) - tmp; \
    if (a >= ADLER32_BASE) a -= ADLER32_BASE; \
  } while (0)
#else
#  define ADLER32_MOD(a) a %= ADLER32_BASE
#  define ADLER32_MOD28(a) a %= ADLER32_BASE
#  define ADLER32_MOD63(a) a %= ADLER32_BASE
#endif

/* ========================================================================= */
unsigned long adler32(unsigned long adler, const unsigned char *buf, size_t len) {
  unsigned long sum2;
  unsigned n;

  /* split Adler-32 into component sums */
  sum2 = (adler >> 16) & 0xffff;
  adler &= 0xffff;

  /* in case user likes doing a byte at a time, keep it fast */
  if (len == 1) {
    adler += buf[0];
    if (adler >= ADLER32_BASE)
      adler -= ADLER32_BASE;
    sum2 += adler;
    if (sum2 >= ADLER32_BASE)
      sum2 -= ADLER32_BASE;
    return adler | (sum2 << 16);
  }

  /* initial Adler-32 value (deferred check for len == 1 speed) */
  if (buf == NULL)
    return 1L;

  /* in case short lengths are provided, keep it somewhat fast */
  if (len < 16) {
    while (len--) {
      adler += *buf++;
      sum2 += adler;
    }
    if (adler >= ADLER32_BASE)
      adler -= ADLER32_BASE;
    ADLER32_MOD28(sum2);            /* only added so many ADLER32_BASE's */
    return adler | (sum2 << 16);
  }

  /* do length ADLER32_NMAX blocks -- requires just one modulo operation */
  while (len >= ADLER32_NMAX) {
    len -= ADLER32_NMAX;
    n = ADLER32_NMAX / 16;          /* ADLER32_NMAX is divisible by 16 */
    do {
      ADLER32_DO16(buf);          /* 16 sums unrolled */
      buf += 16;
    } while (--n);
    ADLER32_MOD(adler);
    ADLER32_MOD(sum2);
  }

  /* do remaining bytes (less than ADLER32_NMAX, still just one modulo) */
  if (len) {                  /* avoid modulos if none remaining */
    while (len >= 16) {
      len -= 16;
      ADLER32_DO16(buf);
      buf += 16;
    }
    while (len--) {
      adler += *buf++;
      sum2 += adler;
    }
    ADLER32_MOD(adler);
    ADLER32_MOD(sum2);
  }

  /* return recombined sums */
  return adler | (sum2 << 16);
}

#endif //BLADE_MODULE_HASH_ADLER32_H
