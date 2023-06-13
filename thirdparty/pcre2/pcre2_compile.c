/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
     Original API code Copyright (c) 1997-2012 University of Cambridge
          New API code Copyright (c) 2016-2020 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NLBLOCK cb             /* Block containing newline information */
#define PSSTART start_pattern  /* Field containing processed string start */
#define PSEND   end_pattern    /* Field containing processed string end */

#include "pcre2_internal.h"

#if PCRE2_CODE_UNIT_WIDTH == 8
#define STRING_UTFn_RIGHTPAR     STRING_UTF8_RIGHTPAR, 5
#define XDIGIT(c)                xdigitab[c]

#else  /* Either 16-bit or 32-bit */
#define XDIGIT(c)                (MAX_255(c)? xdigitab[c] : 0xff)

#if PCRE2_CODE_UNIT_WIDTH == 16
#define STRING_UTFn_RIGHTPAR     STRING_UTF16_RIGHTPAR, 6

#else  /* 32-bit */
#define STRING_UTFn_RIGHTPAR     STRING_UTF32_RIGHTPAR, 6
#endif
#endif

#if PCRE2_SIZE_MAX <= UINT32_MAX
#define PUTOFFSET(s,p) *p++ = s
#define GETOFFSET(s,p) s = *p++
#define GETPLUSOFFSET(s,p) s = *(++p)
#define READPLUSOFFSET(s,p) s = p[1]
#define SKIPOFFSET(p) p++
#define SIZEOFFSET 1
#else
#define PUTOFFSET(s,p) \
  { *p++ = (uint32_t)(s >> 32); *p++ = (uint32_t)(s & 0xffffffff); }
#define GETOFFSET(s,p) \
  { s = ((PCRE2_SIZE)p[0] << 32) | (PCRE2_SIZE)p[1]; p += 2; }
#define GETPLUSOFFSET(s,p) \
  { s = ((PCRE2_SIZE)p[1] << 32) | (PCRE2_SIZE)p[2]; p += 2; }
#define READPLUSOFFSET(s,p) \
  { s = ((PCRE2_SIZE)p[1] << 32) | (PCRE2_SIZE)p[2]; }
#define SKIPOFFSET(p) p += 2
#define SIZEOFFSET 2
#endif

#define META_CODE(x)   (x & 0xffff0000u)
#define META_DATA(x)   (x & 0x0000ffffu)
#define META_DIFF(x,y) ((x-y)>>16)

#ifdef SUPPORT_UNICODE
static unsigned int
  add_list_to_class_internal(uint8_t *, PCRE2_UCHAR **, uint32_t,
    compile_block *, const uint32_t *, unsigned int);
#endif

static int
  compile_regex(uint32_t, PCRE2_UCHAR **, uint32_t **, int *, uint32_t,
    uint32_t *, int32_t *, uint32_t *, int32_t *, branch_chain *,
    compile_block *, PCRE2_SIZE *);

static int
  get_branchlength(uint32_t **, int *, int *, parsed_recurse_check *,
    compile_block *);

static BOOL
  set_lookbehind_lengths(uint32_t **, int *, int *, parsed_recurse_check *,
    compile_block *);

static int
  check_lookbehinds(uint32_t *, uint32_t **, parsed_recurse_check *,
    compile_block *);

/*************************************************
*      Code parameters and static tables         *
*************************************************/

#define MAX_GROUP_NUMBER   65535u
#define MAX_REPEAT_COUNT   65535u
#define REPEAT_UNLIMITED   (MAX_REPEAT_COUNT+1)

#define COMPILE_WORK_SIZE (3000*LINK_SIZE)   /* Size in code units */

#define C16_WORK_SIZE \
  ((COMPILE_WORK_SIZE * sizeof(PCRE2_UCHAR))/sizeof(uint16_t))

#define GROUPINFO_DEFAULT_SIZE 256

#define WORK_SIZE_SAFETY_MARGIN (100)

#define NAMED_GROUP_LIST_SIZE  20

#define PARSED_PATTERN_DEFAULT_SIZE 1024

#define OFLOW_MAX (INT_MAX - 20)

#define META_END              0x80000000u  /* End of pattern */

#define META_ALT              0x80010000u  /* alternation */
#define META_ATOMIC           0x80020000u  /* atomic group */
#define META_BACKREF          0x80030000u  /* Back ref */
#define META_BACKREF_BYNAME   0x80040000u  /* \k'name' */
#define META_BIGVALUE         0x80050000u  /* Next is a literal > META_END */
#define META_CALLOUT_NUMBER   0x80060000u  /* (?C with numerical argument */
#define META_CALLOUT_STRING   0x80070000u  /* (?C with string argument */
#define META_CAPTURE          0x80080000u  /* Capturing parenthesis */
#define META_CIRCUMFLEX       0x80090000u  /* ^ metacharacter */
#define META_CLASS            0x800a0000u  /* start non-empty class */
#define META_CLASS_EMPTY      0x800b0000u  /* empty class */
#define META_CLASS_EMPTY_NOT  0x800c0000u  /* negative empty class */
#define META_CLASS_END        0x800d0000u  /* end of non-empty class */
#define META_CLASS_NOT        0x800e0000u  /* start non-empty negative class */
#define META_COND_ASSERT      0x800f0000u  /* (?(?assertion)... */
#define META_COND_DEFINE      0x80100000u  /* (?(DEFINE)... */
#define META_COND_NAME        0x80110000u  /* (?(<name>)... */
#define META_COND_NUMBER      0x80120000u  /* (?(digits)... */
#define META_COND_RNAME       0x80130000u  /* (?(R&name)... */
#define META_COND_RNUMBER     0x80140000u  /* (?(Rdigits)... */
#define META_COND_VERSION     0x80150000u  /* (?(VERSION<op>x.y)... */
#define META_DOLLAR           0x80160000u  /* $ metacharacter */
#define META_DOT              0x80170000u  /* . metacharacter */
#define META_ESCAPE           0x80180000u  /* \d and friends */
#define META_KET              0x80190000u  /* closing parenthesis */
#define META_NOCAPTURE        0x801a0000u  /* no capture parens */
#define META_OPTIONS          0x801b0000u  /* (?i) and friends */
#define META_POSIX            0x801c0000u  /* POSIX class item */
#define META_POSIX_NEG        0x801d0000u  /* negative POSIX class item */
#define META_RANGE_ESCAPED    0x801e0000u  /* range with at least one escape */
#define META_RANGE_LITERAL    0x801f0000u  /* range defined literally */
#define META_RECURSE          0x80200000u  /* Recursion */
#define META_RECURSE_BYNAME   0x80210000u  /* (?&name) */
#define META_SCRIPT_RUN       0x80220000u  /* (*script_run:...) */

#define META_LOOKAHEAD        0x80230000u  /* (?= */
#define META_LOOKAHEADNOT     0x80240000u  /* (?! */
#define META_LOOKBEHIND       0x80250000u  /* (?<= */
#define META_LOOKBEHINDNOT    0x80260000u  /* (?<! */

#define META_LOOKAHEAD_NA     0x80270000u  /* (*napla: */
#define META_LOOKBEHIND_NA    0x80280000u  /* (*naplb: */

#define META_MARK             0x80290000u  /* (*MARK) */
#define META_ACCEPT           0x802a0000u  /* (*ACCEPT) */
#define META_FAIL             0x802b0000u  /* (*FAIL) */
#define META_COMMIT           0x802c0000u  /* These               */
#define META_COMMIT_ARG       0x802d0000u  /*   pairs             */
#define META_PRUNE            0x802e0000u  /*     must            */
#define META_PRUNE_ARG        0x802f0000u  /*       be            */
#define META_SKIP             0x80300000u  /*         kept        */
#define META_SKIP_ARG         0x80310000u  /*           in        */
#define META_THEN             0x80320000u  /*             this    */
#define META_THEN_ARG         0x80330000u  /*               order */

#define META_ASTERISK         0x80340000u  /* *  */
#define META_ASTERISK_PLUS    0x80350000u  /* *+ */
#define META_ASTERISK_QUERY   0x80360000u  /* *? */
#define META_PLUS             0x80370000u  /* +  */
#define META_PLUS_PLUS        0x80380000u  /* ++ */
#define META_PLUS_QUERY       0x80390000u  /* +? */
#define META_QUERY            0x803a0000u  /* ?  */
#define META_QUERY_PLUS       0x803b0000u  /* ?+ */
#define META_QUERY_QUERY      0x803c0000u  /* ?? */
#define META_MINMAX           0x803d0000u  /* {n,m}  repeat */
#define META_MINMAX_PLUS      0x803e0000u  /* {n,m}+ repeat */
#define META_MINMAX_QUERY     0x803f0000u  /* {n,m}? repeat */

#define META_FIRST_QUANTIFIER META_ASTERISK
#define META_LAST_QUANTIFIER  META_MINMAX_QUERY

#define META_ATOMIC_SCRIPT_RUN 0x8fff0000u

static unsigned char meta_extra_lengths[] = {
  0,             /* META_END */
  0,             /* META_ALT */
  0,             /* META_ATOMIC */
  0,             /* META_BACKREF - more if group is >= 10 */
  1+SIZEOFFSET,  /* META_BACKREF_BYNAME */
  1,             /* META_BIGVALUE */
  3,             /* META_CALLOUT_NUMBER */
  3+SIZEOFFSET,  /* META_CALLOUT_STRING */
  0,             /* META_CAPTURE */
  0,             /* META_CIRCUMFLEX */
  0,             /* META_CLASS */
  0,             /* META_CLASS_EMPTY */
  0,             /* META_CLASS_EMPTY_NOT */
  0,             /* META_CLASS_END */
  0,             /* META_CLASS_NOT */
  0,             /* META_COND_ASSERT */
  SIZEOFFSET,    /* META_COND_DEFINE */
  1+SIZEOFFSET,  /* META_COND_NAME */
  1+SIZEOFFSET,  /* META_COND_NUMBER */
  1+SIZEOFFSET,  /* META_COND_RNAME */
  1+SIZEOFFSET,  /* META_COND_RNUMBER */
  3,             /* META_COND_VERSION */
  0,             /* META_DOLLAR */
  0,             /* META_DOT */
  0,             /* META_ESCAPE - more for ESC_P, ESC_p, ESC_g, ESC_k */
  0,             /* META_KET */
  0,             /* META_NOCAPTURE */
  1,             /* META_OPTIONS */
  1,             /* META_POSIX */
  1,             /* META_POSIX_NEG */
  0,             /* META_RANGE_ESCAPED */
  0,             /* META_RANGE_LITERAL */
  SIZEOFFSET,    /* META_RECURSE */
  1+SIZEOFFSET,  /* META_RECURSE_BYNAME */
  0,             /* META_SCRIPT_RUN */
  0,             /* META_LOOKAHEAD */
  0,             /* META_LOOKAHEADNOT */
  SIZEOFFSET,    /* META_LOOKBEHIND */
  SIZEOFFSET,    /* META_LOOKBEHINDNOT */
  0,             /* META_LOOKAHEAD_NA */
  SIZEOFFSET,    /* META_LOOKBEHIND_NA */
  1,             /* META_MARK - plus the string length */
  0,             /* META_ACCEPT */
  0,             /* META_FAIL */
  0,             /* META_COMMIT */
  1,             /* META_COMMIT_ARG - plus the string length */
  0,             /* META_PRUNE */
  1,             /* META_PRUNE_ARG - plus the string length */
  0,             /* META_SKIP */
  1,             /* META_SKIP_ARG - plus the string length */
  0,             /* META_THEN */
  1,             /* META_THEN_ARG - plus the string length */
  0,             /* META_ASTERISK */
  0,             /* META_ASTERISK_PLUS */
  0,             /* META_ASTERISK_QUERY */
  0,             /* META_PLUS */
  0,             /* META_PLUS_PLUS */
  0,             /* META_PLUS_QUERY */
  0,             /* META_QUERY */
  0,             /* META_QUERY_PLUS */
  0,             /* META_QUERY_QUERY */
  2,             /* META_MINMAX */
  2,             /* META_MINMAX_PLUS */
  2              /* META_MINMAX_QUERY */
};

enum { PSKIP_ALT, PSKIP_CLASS, PSKIP_KET };

#define SETBIT(a,b) a[(b)/8] = (uint8_t)(a[(b)/8] | (1u << ((b)&7)))

#define REQ_CASELESS    (1u << 0)       /* Indicates caselessness */
#define REQ_VARY        (1u << 1)       /* reqcu followed non-literal item */
/* Negative values for the firstcu and reqcu flags */
#define REQ_UNSET       (-2)            /* Not yet found anything */
#define REQ_NONE        (-1)            /* Found not fixed char */

#define GI_SET_FIXED_LENGTH    0x80000000u
#define GI_NOT_FIXED_LENGTH    0x40000000u
#define GI_FIXED_LENGTH_MASK   0x0000ffffu

#define IS_DIGIT(x) ((x) >= CHAR_0 && (x) <= CHAR_9)

#ifndef EBCDIC
static const uint8_t xdigitab[] =
  {
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*   0-  7 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*   8- 15 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  16- 23 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  24- 31 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*    - '  */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  ( - /  */
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, /*  0 - 7  */
  0x08,0x09,0xff,0xff,0xff,0xff,0xff,0xff, /*  8 - ?  */
  0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff, /*  @ - G  */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  H - O  */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  P - W  */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  X - _  */
  0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff, /*  ` - g  */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  h - o  */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  p - w  */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  x -127 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 128-135 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 136-143 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 144-151 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 152-159 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 160-167 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 168-175 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 176-183 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 184-191 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 192-199 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 2ff-207 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 208-215 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 216-223 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 224-231 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 232-239 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 240-247 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};/* 248-255 */

#else

/* This is the "abnormal" case, for EBCDIC systems not running in UTF-8 mode. */

static const uint8_t xdigitab[] =
  {
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*   0-  7  0 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*   8- 15    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  16- 23 10 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  24- 31    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  32- 39 20 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  40- 47    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  48- 55 30 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  56- 63    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*    - 71 40 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  72- |     */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  & - 87 50 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  88- 95    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  - -103 60 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 104- ?     */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 112-119 70 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 120- "     */
  0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff, /* 128- g  80 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  h -143    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 144- p  90 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  q -159    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 160- x  A0 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  y -175    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  ^ -183 B0 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /* 184-191    */
  0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff, /*  { - G  C0 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  H -207    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  } - P  D0 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  Q -223    */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  \ - X  E0 */
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  Y -239    */
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, /*  0 - 7  F0 */
  0x08,0x09,0xff,0xff,0xff,0xff,0xff,0xff};/*  8 -255    */
#endif  /* EBCDIC */

#ifndef EBCDIC
#define ESCAPES_FIRST       CHAR_0
#define ESCAPES_LAST        CHAR_z
#define UPPER_CASE(c)       (c-32)

static const short int escapes[] = {
     0,                       0,
     0,                       0,
     0,                       0,
     0,                       0,
     0,                       0,
     CHAR_COLON,              CHAR_SEMICOLON,
     CHAR_LESS_THAN_SIGN,     CHAR_EQUALS_SIGN,
     CHAR_GREATER_THAN_SIGN,  CHAR_QUESTION_MARK,
     CHAR_COMMERCIAL_AT,      -ESC_A,
     -ESC_B,                  -ESC_C,
     -ESC_D,                  -ESC_E,
     0,                       -ESC_G,
     -ESC_H,                  0,
     0,                       -ESC_K,
     0,                       0,
     -ESC_N,                  0,
     -ESC_P,                  -ESC_Q,
     -ESC_R,                  -ESC_S,
     0,                       0,
     -ESC_V,                  -ESC_W,
     -ESC_X,                  0,
     -ESC_Z,                  CHAR_LEFT_SQUARE_BRACKET,
     CHAR_BACKSLASH,          CHAR_RIGHT_SQUARE_BRACKET,
     CHAR_CIRCUMFLEX_ACCENT,  CHAR_UNDERSCORE,
     CHAR_GRAVE_ACCENT,       CHAR_BEL,
     -ESC_b,                  0,
     -ESC_d,                  CHAR_ESC,
     CHAR_FF,                 0,
     -ESC_h,                  0,
     0,                       -ESC_k,
     0,                       0,
     CHAR_LF,                 0,
     -ESC_p,                  0,
     CHAR_CR,                 -ESC_s,
     CHAR_HT,                 0,
     -ESC_v,                  -ESC_w,
     0,                       0,
     -ESC_z
};

#else

#if 'a' == 0x81                    /* Check for a real EBCDIC environment */
#define ESCAPES_FIRST       CHAR_a
#define ESCAPES_LAST        CHAR_9
#define UPPER_CASE(c)       (c+64)
#else                              /* Testing in an ASCII environment */
#define ESCAPES_FIRST  ((unsigned char)'\x81')   /* EBCDIC 'a' */
#define ESCAPES_LAST   ((unsigned char)'\xf9')   /* EBCDIC '9' */
#define UPPER_CASE(c)  (c-32)
#endif

static const short int escapes[] = {
/*  80 */         CHAR_BEL, -ESC_b,       0, -ESC_d, CHAR_ESC, CHAR_FF,      0,
/*  88 */ -ESC_h,        0,      0,     '{',      0,        0,       0,      0,
/*  90 */      0,        0, -ESC_k,       0,      0,  CHAR_LF,       0, -ESC_p,
/*  98 */      0,  CHAR_CR,      0,     '}',      0,        0,       0,      0,
/*  A0 */      0,      '~', -ESC_s, CHAR_HT,      0,   -ESC_v,  -ESC_w,      0,
/*  A8 */      0,   -ESC_z,      0,       0,      0,      '[',       0,      0,
/*  B0 */      0,        0,      0,       0,      0,        0,       0,      0,
/*  B8 */      0,        0,      0,       0,      0,      ']',     '=',    '-',
/*  C0 */    '{',   -ESC_A, -ESC_B,  -ESC_C, -ESC_D,   -ESC_E,       0, -ESC_G,
/*  C8 */ -ESC_H,        0,      0,       0,      0,        0,       0,      0,
/*  D0 */    '}',        0, -ESC_K,       0,      0,   -ESC_N,       0, -ESC_P,
/*  D8 */ -ESC_Q,   -ESC_R,      0,       0,      0,        0,       0,      0,
/*  E0 */   '\\',        0, -ESC_S,       0,      0,   -ESC_V,  -ESC_W, -ESC_X,
/*  E8 */      0,   -ESC_Z,      0,       0,      0,        0,       0,      0,
/*  F0 */      0,        0,      0,       0,      0,        0,       0,      0,
/*  F8 */      0,        0
};

static unsigned char ebcdic_escape_c[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

#endif   /* EBCDIC */

typedef struct verbitem {
  unsigned int len;          /* Length of verb name */
  uint32_t meta;             /* Base META_ code */
  int has_arg;               /* Argument requirement */
} verbitem;

static const char verbnames[] =
  "\0"                       /* Empty name is a shorthand for MARK */
  STRING_MARK0
  STRING_ACCEPT0
  STRING_F0
  STRING_FAIL0
  STRING_COMMIT0
  STRING_PRUNE0
  STRING_SKIP0
  STRING_THEN;

static const verbitem verbs[] = {
  { 0, META_MARK,   +1 },  /* > 0 => must have an argument */
  { 4, META_MARK,   +1 },
  { 6, META_ACCEPT, -1 },  /* < 0 => Optional argument, convert to pre-MARK */
  { 1, META_FAIL,   -1 },
  { 4, META_FAIL,   -1 },
  { 6, META_COMMIT,  0 },
  { 5, META_PRUNE,   0 },  /* Optional argument; bump META code if found */
  { 4, META_SKIP,    0 },
  { 4, META_THEN,    0 }
};

static const int verbcount = sizeof(verbs)/sizeof(verbitem);

static const uint32_t verbops[] = {
  OP_MARK, OP_ACCEPT, OP_FAIL, OP_COMMIT, OP_COMMIT_ARG, OP_PRUNE,
  OP_PRUNE_ARG, OP_SKIP, OP_SKIP_ARG, OP_THEN, OP_THEN_ARG };

typedef struct alasitem {
  unsigned int len;          /* Length of name */
  uint32_t meta;             /* Base META_ code */
} alasitem;

static const char alasnames[] =
  STRING_pla0
  STRING_plb0
  STRING_napla0
  STRING_naplb0
  STRING_nla0
  STRING_nlb0
  STRING_positive_lookahead0
  STRING_positive_lookbehind0
  STRING_non_atomic_positive_lookahead0
  STRING_non_atomic_positive_lookbehind0
  STRING_negative_lookahead0
  STRING_negative_lookbehind0
  STRING_atomic0
  STRING_sr0
  STRING_asr0
  STRING_script_run0
  STRING_atomic_script_run;

static const alasitem alasmeta[] = {
  {  3, META_LOOKAHEAD         },
  {  3, META_LOOKBEHIND        },
  {  5, META_LOOKAHEAD_NA      },
  {  5, META_LOOKBEHIND_NA     },
  {  3, META_LOOKAHEADNOT      },
  {  3, META_LOOKBEHINDNOT     },
  { 18, META_LOOKAHEAD         },
  { 19, META_LOOKBEHIND        },
  { 29, META_LOOKAHEAD_NA      },
  { 30, META_LOOKBEHIND_NA     },
  { 18, META_LOOKAHEADNOT      },
  { 19, META_LOOKBEHINDNOT     },
  {  6, META_ATOMIC            },
  {  2, META_SCRIPT_RUN        }, /* sr = script run */
  {  3, META_ATOMIC_SCRIPT_RUN }, /* asr = atomic script run */
  { 10, META_SCRIPT_RUN        }, /* script run */
  { 17, META_ATOMIC_SCRIPT_RUN }  /* atomic script run */
};

static const int alascount = sizeof(alasmeta)/sizeof(alasitem);

static uint32_t chartypeoffset[] = {
  OP_STAR - OP_STAR,    OP_STARI - OP_STAR,
  OP_NOTSTAR - OP_STAR, OP_NOTSTARI - OP_STAR };

static const char posix_names[] =
  STRING_alpha0 STRING_lower0 STRING_upper0 STRING_alnum0
  STRING_ascii0 STRING_blank0 STRING_cntrl0 STRING_digit0
  STRING_graph0 STRING_print0 STRING_punct0 STRING_space0
  STRING_word0  STRING_xdigit;

static const uint8_t posix_name_lengths[] = {
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 6, 0 };

#define PC_GRAPH  8
#define PC_PRINT  9
#define PC_PUNCT 10

static const int posix_class_maps[] = {
  cbit_word,  cbit_digit, -2,             /* alpha */
  cbit_lower, -1,          0,             /* lower */
  cbit_upper, -1,          0,             /* upper */
  cbit_word,  -1,          2,             /* alnum - word without underscore */
  cbit_print, cbit_cntrl,  0,             /* ascii */
  cbit_space, -1,          1,             /* blank - a GNU extension */
  cbit_cntrl, -1,          0,             /* cntrl */
  cbit_digit, -1,          0,             /* digit */
  cbit_graph, -1,          0,             /* graph */
  cbit_print, -1,          0,             /* print */
  cbit_punct, -1,          0,             /* punct */
  cbit_space, -1,          0,             /* space */
  cbit_word,  -1,          0,             /* word - a Perl extension */
  cbit_xdigit,-1,          0              /* xdigit */
};

#ifdef SUPPORT_UNICODE

static int posix_substitutes[] = {
  PT_GC, ucp_L,     /* alpha */
  PT_PC, ucp_Ll,    /* lower */
  PT_PC, ucp_Lu,    /* upper */
  PT_ALNUM, 0,      /* alnum */
  -1, 0,            /* ascii, treat as non-UCP */
  -1, 1,            /* blank, treat as \h */
  PT_PC, ucp_Cc,    /* cntrl */
  PT_PC, ucp_Nd,    /* digit */
  PT_PXGRAPH, 0,    /* graph */
  PT_PXPRINT, 0,    /* print */
  PT_PXPUNCT, 0,    /* punct */
  PT_PXSPACE, 0,    /* space */   /* Xps is POSIX space, but from 8.34 */
  PT_WORD, 0,       /* word  */   /* Perl and POSIX space are the same */
  -1, 0             /* xdigit, treat as non-UCP */
};
#define POSIX_SUBSIZE (sizeof(posix_substitutes) / (2*sizeof(uint32_t)))
#endif  /* SUPPORT_UNICODE */

#define PUBLIC_LITERAL_COMPILE_OPTIONS \
  (PCRE2_ANCHORED|PCRE2_AUTO_CALLOUT|PCRE2_CASELESS|PCRE2_ENDANCHORED| \
   PCRE2_FIRSTLINE|PCRE2_LITERAL|PCRE2_MATCH_INVALID_UTF| \
   PCRE2_NO_START_OPTIMIZE|PCRE2_NO_UTF_CHECK|PCRE2_USE_OFFSET_LIMIT|PCRE2_UTF)

#define PUBLIC_COMPILE_OPTIONS \
  (PUBLIC_LITERAL_COMPILE_OPTIONS| \
   PCRE2_ALLOW_EMPTY_CLASS|PCRE2_ALT_BSUX|PCRE2_ALT_CIRCUMFLEX| \
   PCRE2_ALT_VERBNAMES|PCRE2_DOLLAR_ENDONLY|PCRE2_DOTALL|PCRE2_DUPNAMES| \
   PCRE2_EXTENDED|PCRE2_EXTENDED_MORE|PCRE2_MATCH_UNSET_BACKREF| \
   PCRE2_MULTILINE|PCRE2_NEVER_BACKSLASH_C|PCRE2_NEVER_UCP| \
   PCRE2_NEVER_UTF|PCRE2_NO_AUTO_CAPTURE|PCRE2_NO_AUTO_POSSESS| \
   PCRE2_NO_DOTSTAR_ANCHOR|PCRE2_UCP|PCRE2_UNGREEDY)

#define PUBLIC_LITERAL_COMPILE_EXTRA_OPTIONS \
   (PCRE2_EXTRA_MATCH_LINE|PCRE2_EXTRA_MATCH_WORD)

#define PUBLIC_COMPILE_EXTRA_OPTIONS \
   (PUBLIC_LITERAL_COMPILE_EXTRA_OPTIONS| \
    PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES|PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL| \
    PCRE2_EXTRA_ESCAPED_CR_IS_LF|PCRE2_EXTRA_ALT_BSUX)

enum { ERR0 = COMPILE_ERROR_BASE,
       ERR1,  ERR2,  ERR3,  ERR4,  ERR5,  ERR6,  ERR7,  ERR8,  ERR9,  ERR10,
       ERR11, ERR12, ERR13, ERR14, ERR15, ERR16, ERR17, ERR18, ERR19, ERR20,
       ERR21, ERR22, ERR23, ERR24, ERR25, ERR26, ERR27, ERR28, ERR29, ERR30,
       ERR31, ERR32, ERR33, ERR34, ERR35, ERR36, ERR37, ERR38, ERR39, ERR40,
       ERR41, ERR42, ERR43, ERR44, ERR45, ERR46, ERR47, ERR48, ERR49, ERR50,
       ERR51, ERR52, ERR53, ERR54, ERR55, ERR56, ERR57, ERR58, ERR59, ERR60,
       ERR61, ERR62, ERR63, ERR64, ERR65, ERR66, ERR67, ERR68, ERR69, ERR70,
       ERR71, ERR72, ERR73, ERR74, ERR75, ERR76, ERR77, ERR78, ERR79, ERR80,
       ERR81, ERR82, ERR83, ERR84, ERR85, ERR86, ERR87, ERR88, ERR89, ERR90,
       ERR91, ERR92, ERR93, ERR94, ERR95, ERR96, ERR97, ERR98 };

enum { PSO_OPT,     /* Value is an option bit */
       PSO_FLG,     /* Value is a flag bit */
       PSO_NL,      /* Value is a newline type */
       PSO_BSR,     /* Value is a \R type */
       PSO_LIMH,    /* Read integer value for heap limit */
       PSO_LIMM,    /* Read integer value for match limit */
       PSO_LIMD };  /* Read integer value for depth limit */

typedef struct pso {
  const uint8_t *name;
  uint16_t length;
  uint16_t type;
  uint32_t value;
} pso;

static pso pso_list[] = {
  { (uint8_t *)STRING_UTFn_RIGHTPAR,                  PSO_OPT, PCRE2_UTF },
  { (uint8_t *)STRING_UTF_RIGHTPAR,                4, PSO_OPT, PCRE2_UTF },
  { (uint8_t *)STRING_UCP_RIGHTPAR,                4, PSO_OPT, PCRE2_UCP },
  { (uint8_t *)STRING_NOTEMPTY_RIGHTPAR,           9, PSO_FLG, PCRE2_NOTEMPTY_SET },
  { (uint8_t *)STRING_NOTEMPTY_ATSTART_RIGHTPAR,  17, PSO_FLG, PCRE2_NE_ATST_SET },
  { (uint8_t *)STRING_NO_AUTO_POSSESS_RIGHTPAR,   16, PSO_OPT, PCRE2_NO_AUTO_POSSESS },
  { (uint8_t *)STRING_NO_DOTSTAR_ANCHOR_RIGHTPAR, 18, PSO_OPT, PCRE2_NO_DOTSTAR_ANCHOR },
  { (uint8_t *)STRING_NO_JIT_RIGHTPAR,             7, PSO_FLG, PCRE2_NOJIT },
  { (uint8_t *)STRING_NO_START_OPT_RIGHTPAR,      13, PSO_OPT, PCRE2_NO_START_OPTIMIZE },
  { (uint8_t *)STRING_LIMIT_HEAP_EQ,              11, PSO_LIMH, 0 },
  { (uint8_t *)STRING_LIMIT_MATCH_EQ,             12, PSO_LIMM, 0 },
  { (uint8_t *)STRING_LIMIT_DEPTH_EQ,             12, PSO_LIMD, 0 },
  { (uint8_t *)STRING_LIMIT_RECURSION_EQ,         16, PSO_LIMD, 0 },
  { (uint8_t *)STRING_CR_RIGHTPAR,                 3, PSO_NL,  PCRE2_NEWLINE_CR },
  { (uint8_t *)STRING_LF_RIGHTPAR,                 3, PSO_NL,  PCRE2_NEWLINE_LF },
  { (uint8_t *)STRING_CRLF_RIGHTPAR,               5, PSO_NL,  PCRE2_NEWLINE_CRLF },
  { (uint8_t *)STRING_ANY_RIGHTPAR,                4, PSO_NL,  PCRE2_NEWLINE_ANY },
  { (uint8_t *)STRING_NUL_RIGHTPAR,                4, PSO_NL,  PCRE2_NEWLINE_NUL },
  { (uint8_t *)STRING_ANYCRLF_RIGHTPAR,            8, PSO_NL,  PCRE2_NEWLINE_ANYCRLF },
  { (uint8_t *)STRING_BSR_ANYCRLF_RIGHTPAR,       12, PSO_BSR, PCRE2_BSR_ANYCRLF },
  { (uint8_t *)STRING_BSR_UNICODE_RIGHTPAR,       12, PSO_BSR, PCRE2_BSR_UNICODE }
};

static const uint8_t opcode_possessify[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 0 - 15  */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 16 - 31 */

  0,                       /* NOTI */
  OP_POSSTAR, 0,           /* STAR, MINSTAR */
  OP_POSPLUS, 0,           /* PLUS, MINPLUS */
  OP_POSQUERY, 0,          /* QUERY, MINQUERY */
  OP_POSUPTO, 0,           /* UPTO, MINUPTO */
  0,                       /* EXACT */
  0, 0, 0, 0,              /* POS{STAR,PLUS,QUERY,UPTO} */

  OP_POSSTARI, 0,          /* STARI, MINSTARI */
  OP_POSPLUSI, 0,          /* PLUSI, MINPLUSI */
  OP_POSQUERYI, 0,         /* QUERYI, MINQUERYI */
  OP_POSUPTOI, 0,          /* UPTOI, MINUPTOI */
  0,                       /* EXACTI */
  0, 0, 0, 0,              /* POS{STARI,PLUSI,QUERYI,UPTOI} */

  OP_NOTPOSSTAR, 0,        /* NOTSTAR, NOTMINSTAR */
  OP_NOTPOSPLUS, 0,        /* NOTPLUS, NOTMINPLUS */
  OP_NOTPOSQUERY, 0,       /* NOTQUERY, NOTMINQUERY */
  OP_NOTPOSUPTO, 0,        /* NOTUPTO, NOTMINUPTO */
  0,                       /* NOTEXACT */
  0, 0, 0, 0,              /* NOTPOS{STAR,PLUS,QUERY,UPTO} */

  OP_NOTPOSSTARI, 0,       /* NOTSTARI, NOTMINSTARI */
  OP_NOTPOSPLUSI, 0,       /* NOTPLUSI, NOTMINPLUSI */
  OP_NOTPOSQUERYI, 0,      /* NOTQUERYI, NOTMINQUERYI */
  OP_NOTPOSUPTOI, 0,       /* NOTUPTOI, NOTMINUPTOI */
  0,                       /* NOTEXACTI */
  0, 0, 0, 0,              /* NOTPOS{STARI,PLUSI,QUERYI,UPTOI} */

  OP_TYPEPOSSTAR, 0,       /* TYPESTAR, TYPEMINSTAR */
  OP_TYPEPOSPLUS, 0,       /* TYPEPLUS, TYPEMINPLUS */
  OP_TYPEPOSQUERY, 0,      /* TYPEQUERY, TYPEMINQUERY */
  OP_TYPEPOSUPTO, 0,       /* TYPEUPTO, TYPEMINUPTO */
  0,                       /* TYPEEXACT */
  0, 0, 0, 0,              /* TYPEPOS{STAR,PLUS,QUERY,UPTO} */

  OP_CRPOSSTAR, 0,         /* CRSTAR, CRMINSTAR */
  OP_CRPOSPLUS, 0,         /* CRPLUS, CRMINPLUS */
  OP_CRPOSQUERY, 0,        /* CRQUERY, CRMINQUERY */
  OP_CRPOSRANGE, 0,        /* CRRANGE, CRMINRANGE */
  0, 0, 0, 0,              /* CRPOS{STAR,PLUS,QUERY,RANGE} */

  0, 0, 0,                 /* CLASS, NCLASS, XCLASS */
  0, 0,                    /* REF, REFI */
  0, 0,                    /* DNREF, DNREFI */
  0, 0                     /* RECURSE, CALLOUT */
};


#ifdef DEBUG_SHOW_PARSED
/*************************************************
*     Show the parsed pattern for debugging      *
*************************************************/

static void show_parsed(compile_block *cb)
{
uint32_t *pptr = cb->parsed_pattern;

for (;;)
  {
  int max, min;
  PCRE2_SIZE offset;
  uint32_t i;
  uint32_t length;
  uint32_t meta_arg = META_DATA(*pptr);

  fprintf(stderr, "+++ %02d %.8x ", (int)(pptr - cb->parsed_pattern), *pptr);

  if (*pptr < META_END)
    {
    if (*pptr > 32 && *pptr < 128) fprintf(stderr, "%c", *pptr);
    pptr++;
    }

  else switch (META_CODE(*pptr++))
    {
    default:
    fprintf(stderr, "**** OOPS - unknown META value - giving up ****\n");
    return;

    case META_END:
    fprintf(stderr, "META_END\n");
    return;

    case META_CAPTURE:
    fprintf(stderr, "META_CAPTURE %d", meta_arg);
    break;

    case META_RECURSE:
    GETOFFSET(offset, pptr);
    fprintf(stderr, "META_RECURSE %d %zd", meta_arg, offset);
    break;

    case META_BACKREF:
    if (meta_arg < 10)
      offset = cb->small_ref_offset[meta_arg];
    else
      GETOFFSET(offset, pptr);
    fprintf(stderr, "META_BACKREF %d %zd", meta_arg, offset);
    break;

    case META_ESCAPE:
    if (meta_arg == ESC_P || meta_arg == ESC_p)
      {
      uint32_t ptype = *pptr >> 16;
      uint32_t pvalue = *pptr++ & 0xffff;
      fprintf(stderr, "META \\%c %d %d", (meta_arg == ESC_P)? 'P':'p',
        ptype, pvalue);
      }
    else
      {
      uint32_t cc;
      if (meta_arg == ESC_g) cc = CHAR_g;
      else for (cc = ESCAPES_FIRST; cc <= ESCAPES_LAST; cc++)
        {
        if (meta_arg == (uint32_t)(-escapes[cc - ESCAPES_FIRST])) break;
        }
      if (cc > ESCAPES_LAST) cc = CHAR_QUESTION_MARK;
      fprintf(stderr, "META \\%c", cc);
      }
    break;

    case META_MINMAX:
    min = *pptr++;
    max = *pptr++;
    if (max != REPEAT_UNLIMITED)
      fprintf(stderr, "META {%d,%d}", min, max);
    else
      fprintf(stderr, "META {%d,}", min);
    break;

    case META_MINMAX_QUERY:
    min = *pptr++;
    max = *pptr++;
    if (max != REPEAT_UNLIMITED)
      fprintf(stderr, "META {%d,%d}?", min, max);
    else
      fprintf(stderr, "META {%d,}?", min);
    break;

    case META_MINMAX_PLUS:
    min = *pptr++;
    max = *pptr++;
    if (max != REPEAT_UNLIMITED)
      fprintf(stderr, "META {%d,%d}+", min, max);
    else
      fprintf(stderr, "META {%d,}+", min);
    break;

    case META_BIGVALUE: fprintf(stderr, "META_BIGVALUE %.8x", *pptr++); break;
    case META_CIRCUMFLEX: fprintf(stderr, "META_CIRCUMFLEX"); break;
    case META_COND_ASSERT: fprintf(stderr, "META_COND_ASSERT"); break;
    case META_DOLLAR: fprintf(stderr, "META_DOLLAR"); break;
    case META_DOT: fprintf(stderr, "META_DOT"); break;
    case META_ASTERISK: fprintf(stderr, "META *"); break;
    case META_ASTERISK_QUERY: fprintf(stderr, "META *?"); break;
    case META_ASTERISK_PLUS: fprintf(stderr, "META *+"); break;
    case META_PLUS: fprintf(stderr, "META +"); break;
    case META_PLUS_QUERY: fprintf(stderr, "META +?"); break;
    case META_PLUS_PLUS: fprintf(stderr, "META ++"); break;
    case META_QUERY: fprintf(stderr, "META ?"); break;
    case META_QUERY_QUERY: fprintf(stderr, "META ??"); break;
    case META_QUERY_PLUS: fprintf(stderr, "META ?+"); break;

    case META_ATOMIC: fprintf(stderr, "META (?>"); break;
    case META_NOCAPTURE: fprintf(stderr, "META (?:"); break;
    case META_LOOKAHEAD: fprintf(stderr, "META (?="); break;
    case META_LOOKAHEADNOT: fprintf(stderr, "META (?!"); break;
    case META_LOOKAHEAD_NA: fprintf(stderr, "META (*napla:"); break;
    case META_SCRIPT_RUN: fprintf(stderr, "META (*sr:"); break;
    case META_KET: fprintf(stderr, "META )"); break;
    case META_ALT: fprintf(stderr, "META | %d", meta_arg); break;

    case META_CLASS: fprintf(stderr, "META ["); break;
    case META_CLASS_NOT: fprintf(stderr, "META [^"); break;
    case META_CLASS_END: fprintf(stderr, "META ]"); break;
    case META_CLASS_EMPTY: fprintf(stderr, "META []"); break;
    case META_CLASS_EMPTY_NOT: fprintf(stderr, "META [^]"); break;

    case META_RANGE_LITERAL: fprintf(stderr, "META - (literal)"); break;
    case META_RANGE_ESCAPED: fprintf(stderr, "META - (escaped)"); break;

    case META_POSIX: fprintf(stderr, "META_POSIX %d", *pptr++); break;
    case META_POSIX_NEG: fprintf(stderr, "META_POSIX_NEG %d", *pptr++); break;

    case META_ACCEPT: fprintf(stderr, "META (*ACCEPT)"); break;
    case META_FAIL: fprintf(stderr, "META (*FAIL)"); break;
    case META_COMMIT: fprintf(stderr, "META (*COMMIT)"); break;
    case META_PRUNE: fprintf(stderr, "META (*PRUNE)"); break;
    case META_SKIP: fprintf(stderr, "META (*SKIP)"); break;
    case META_THEN: fprintf(stderr, "META (*THEN)"); break;

    case META_OPTIONS: fprintf(stderr, "META_OPTIONS 0x%02x", *pptr++); break;

    case META_LOOKBEHIND:
    fprintf(stderr, "META (?<= %d offset=", meta_arg);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_LOOKBEHIND_NA:
    fprintf(stderr, "META (*naplb: %d offset=", meta_arg);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_LOOKBEHINDNOT:
    fprintf(stderr, "META (?<! %d offset=", meta_arg);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_CALLOUT_NUMBER:
    fprintf(stderr, "META (?C%d) next=%d/%d", pptr[2], pptr[0],
       pptr[1]);
    pptr += 3;
    break;

    case META_CALLOUT_STRING:
      {
      uint32_t patoffset = *pptr++;    /* Offset of next pattern item */
      uint32_t patlength = *pptr++;    /* Length of next pattern item */
      fprintf(stderr, "META (?Cstring) length=%d offset=", *pptr++);
      GETOFFSET(offset, pptr);
      fprintf(stderr, "%zd next=%d/%d", offset, patoffset, patlength);
      }
    break;

    case META_RECURSE_BYNAME:
    fprintf(stderr, "META (?(&name) length=%d offset=", *pptr++);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_BACKREF_BYNAME:
    fprintf(stderr, "META_BACKREF_BYNAME length=%d offset=", *pptr++);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_COND_NUMBER:
    fprintf(stderr, "META_COND_NUMBER %d offset=", pptr[SIZEOFFSET]);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    pptr++;
    break;

    case META_COND_DEFINE:
    fprintf(stderr, "META (?(DEFINE) offset=");
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_COND_VERSION:
    fprintf(stderr, "META (?(VERSION%s", (*pptr++ == 0)? "=" : ">=");
    fprintf(stderr, "%d.", *pptr++);
    fprintf(stderr, "%d)", *pptr++);
    break;

    case META_COND_NAME:
    fprintf(stderr, "META (?(<name>) length=%d offset=", *pptr++);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_COND_RNAME:
    fprintf(stderr, "META (?(R&name) length=%d offset=", *pptr++);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    /* This is kept as a name, because it might be. */

    case META_COND_RNUMBER:
    fprintf(stderr, "META (?(Rnumber) length=%d offset=", *pptr++);
    GETOFFSET(offset, pptr);
    fprintf(stderr, "%zd", offset);
    break;

    case META_MARK:
    fprintf(stderr, "META (*MARK:");
    goto SHOWARG;

    case META_COMMIT_ARG:
    fprintf(stderr, "META (*COMMIT:");
    goto SHOWARG;

    case META_PRUNE_ARG:
    fprintf(stderr, "META (*PRUNE:");
    goto SHOWARG;

    case META_SKIP_ARG:
    fprintf(stderr, "META (*SKIP:");
    goto SHOWARG;

    case META_THEN_ARG:
    fprintf(stderr, "META (*THEN:");
    SHOWARG:
    length = *pptr++;
    for (i = 0; i < length; i++)
      {
      uint32_t cc = *pptr++;
      if (cc > 32 && cc < 128) fprintf(stderr, "%c", cc);
        else fprintf(stderr, "\\x{%x}", cc);
      }
    fprintf(stderr, ") length=%u", length);
    break;
    }
  fprintf(stderr, "\n");
  }
return;
}
#endif  /* DEBUG_SHOW_PARSED */

/*************************************************
*               Copy compiled code               *
*************************************************/

PCRE2_EXP_DEFN pcre2_code * PCRE2_CALL_CONVENTION
pcre2_code_copy(const pcre2_code *code)
{
PCRE2_SIZE* ref_count;
pcre2_code *newcode;

if (code == NULL) return NULL;
newcode = code->memctl.malloc(code->blocksize, code->memctl.memory_data);
if (newcode == NULL) return NULL;
memcpy(newcode, code, code->blocksize);
newcode->executable_jit = NULL;

if ((code->flags & PCRE2_DEREF_TABLES) != 0)
  {
  ref_count = (PCRE2_SIZE *)(code->tables + TABLES_LENGTH);
  (*ref_count)++;
  }

return newcode;
}

/*************************************************
*     Copy compiled code and character tables    *
*************************************************/

PCRE2_EXP_DEFN pcre2_code * PCRE2_CALL_CONVENTION
pcre2_code_copy_with_tables(const pcre2_code *code)
{
PCRE2_SIZE* ref_count;
pcre2_code *newcode;
uint8_t *newtables;

if (code == NULL) return NULL;
newcode = code->memctl.malloc(code->blocksize, code->memctl.memory_data);
if (newcode == NULL) return NULL;
memcpy(newcode, code, code->blocksize);
newcode->executable_jit = NULL;

newtables = code->memctl.malloc(TABLES_LENGTH + sizeof(PCRE2_SIZE),
  code->memctl.memory_data);
if (newtables == NULL)
  {
  code->memctl.free((void *)newcode, code->memctl.memory_data);
  return NULL;
  }
memcpy(newtables, code->tables, TABLES_LENGTH);
ref_count = (PCRE2_SIZE *)(newtables + TABLES_LENGTH);
*ref_count = 1;

newcode->tables = newtables;
newcode->flags |= PCRE2_DEREF_TABLES;
return newcode;
}

/*************************************************
*               Free compiled code               *
*************************************************/

PCRE2_EXP_DEFN void PCRE2_CALL_CONVENTION
pcre2_code_free(pcre2_code *code)
{
PCRE2_SIZE* ref_count;

if (code != NULL)
  {
  if (code->executable_jit != NULL)
    PRIV(jit_free)(code->executable_jit, &code->memctl);

  if ((code->flags & PCRE2_DEREF_TABLES) != 0)
    {

    ref_count = (PCRE2_SIZE *)(code->tables + TABLES_LENGTH);
    if (*ref_count > 0)
      {
      (*ref_count)--;
      if (*ref_count == 0)
        code->memctl.free((void *)code->tables, code->memctl.memory_data);
      }
    }

  code->memctl.free(code, code->memctl.memory_data);
  }
}

/*************************************************
*         Read a number, possibly signed         *
*************************************************/

static BOOL
read_number(PCRE2_SPTR *ptrptr, PCRE2_SPTR ptrend, int32_t allow_sign,
  uint32_t max_value, uint32_t max_error, int *intptr, int *errorcodeptr)
{
int sign = 0;
uint32_t n = 0;
PCRE2_SPTR ptr = *ptrptr;
BOOL yield = FALSE;

*errorcodeptr = 0;

if (allow_sign >= 0 && ptr < ptrend)
  {
  if (*ptr == CHAR_PLUS)
    {
    sign = +1;
    max_value -= allow_sign;
    ptr++;
    }
  else if (*ptr == CHAR_MINUS)
    {
    sign = -1;
    ptr++;
    }
  }

if (ptr >= ptrend || !IS_DIGIT(*ptr)) return FALSE;
while (ptr < ptrend && IS_DIGIT(*ptr))
  {
  n = n * 10 + *ptr++ - CHAR_0;
  if (n > max_value)
    {
    *errorcodeptr = max_error;
    goto EXIT;
    }
  }

if (allow_sign >= 0 && sign != 0)
  {
  if (n == 0)
    {
    *errorcodeptr = ERR26;  /* +0 and -0 are not allowed */
    goto EXIT;
    }

  if (sign > 0) n += allow_sign;
  else if ((int)n > allow_sign)
    {
    *errorcodeptr = ERR15;  /* Non-existent subpattern */
    goto EXIT;
    }
  else n = allow_sign + 1 - n;
  }

yield = TRUE;

EXIT:
*intptr = n;
*ptrptr = ptr;
return yield;
}

/*************************************************
*         Read repeat counts                     *
*************************************************/

static BOOL
read_repeat_counts(PCRE2_SPTR *ptrptr, PCRE2_SPTR ptrend, uint32_t *minp,
  uint32_t *maxp, int *errorcodeptr)
{
PCRE2_SPTR p = *ptrptr;
BOOL yield = FALSE;
int32_t min = 0;
int32_t max = REPEAT_UNLIMITED; /* This value is larger than MAX_REPEAT_COUNT */

if (!read_number(&p, ptrend, -1, MAX_REPEAT_COUNT, ERR5, &min, errorcodeptr))
  goto EXIT;

if (p >= ptrend) goto EXIT;

if (*p == CHAR_RIGHT_CURLY_BRACKET)
  {
  p++;
  max = min;
  }

else
  {
  if (*p++ != CHAR_COMMA || p >= ptrend) goto EXIT;
  if (*p != CHAR_RIGHT_CURLY_BRACKET)
    {
    if (!read_number(&p, ptrend, -1, MAX_REPEAT_COUNT, ERR5, &max,
        errorcodeptr) || p >= ptrend ||  *p != CHAR_RIGHT_CURLY_BRACKET)
      goto EXIT;
    if (max < min)
      {
      *errorcodeptr = ERR4;
      goto EXIT;
      }
    }
  p++;
  }

yield = TRUE;
if (minp != NULL) *minp = (uint32_t)min;
if (maxp != NULL) *maxp = (uint32_t)max;

EXIT:
if (yield || *errorcodeptr != 0) *ptrptr = p;
return yield;
}

/*************************************************
*            Handle escapes                      *
*************************************************/

int
PRIV(check_escape)(PCRE2_SPTR *ptrptr, PCRE2_SPTR ptrend, uint32_t *chptr,
  int *errorcodeptr, uint32_t options, uint32_t extra_options, BOOL isclass,
  compile_block *cb)
{
BOOL utf = (options & PCRE2_UTF) != 0;
PCRE2_SPTR ptr = *ptrptr;
uint32_t c, cc;
int escape = 0;
int i;

if (ptr >= ptrend)
  {
  *errorcodeptr = ERR1;
  return 0;
  }

GETCHARINCTEST(c, ptr);         /* Get character value, increment pointer */
*errorcodeptr = 0;              /* Be optimistic */

if (c < ESCAPES_FIRST || c > ESCAPES_LAST) {}  /* Definitely literal */

else if ((i = escapes[c - ESCAPES_FIRST]) != 0)
  {
  if (i > 0)
    {
    c = (uint32_t)i;
    if (c == CHAR_CR && (extra_options & PCRE2_EXTRA_ESCAPED_CR_IS_LF) != 0)
      c = CHAR_LF;
    }
  else  /* Negative table entry */
    {
    escape = -i;                    /* Else return a special escape */
    if (cb != NULL && (escape == ESC_P || escape == ESC_p || escape == ESC_X))
      cb->external_flags |= PCRE2_HASBKPORX;   /* Note \P, \p, or \X */

    if (escape == ESC_N && ptr < ptrend && *ptr == CHAR_LEFT_CURLY_BRACKET)
      {
      PCRE2_SPTR p = ptr + 1;

      if (ptrend - p > 1 && *p == CHAR_U && p[1] == CHAR_PLUS)
        {
#ifdef EBCDIC
        *errorcodeptr = ERR93;
#else
        if (utf)
          {
          ptr = p + 1;
          escape = 0;   /* Not a fancy escape after all */
          goto COME_FROM_NU;
          }
        else *errorcodeptr = ERR93;
#endif
        }

      else
        {
        if (!read_repeat_counts(&p, ptrend, NULL, NULL, errorcodeptr) &&
             *errorcodeptr == 0)
          *errorcodeptr = ERR37;
        }
      }
    }
  }

else
  {
  int s;
  PCRE2_SPTR oldptr;
  BOOL overflow;
  BOOL alt_bsux =
    ((options & PCRE2_ALT_BSUX) | (extra_options & PCRE2_EXTRA_ALT_BSUX)) != 0;

  if (cb == NULL)
    {
    if (c != CHAR_c && c != CHAR_o && c != CHAR_x)
      {
      *errorcodeptr = ERR3;
      return 0;
      }
    alt_bsux = FALSE;   /* Do not modify \x handling */
    }

  switch (c)
    {

    case CHAR_F:
    case CHAR_l:
    case CHAR_L:
    *errorcodeptr = ERR37;
    break;

    case CHAR_u:
    if (!alt_bsux) *errorcodeptr = ERR37; else
      {
      uint32_t xc;

      if (ptr >= ptrend) break;
      if (*ptr == CHAR_LEFT_CURLY_BRACKET &&
          (extra_options & PCRE2_EXTRA_ALT_BSUX) != 0)
        {
        PCRE2_SPTR hptr = ptr + 1;
        cc = 0;

        while (hptr < ptrend && (xc = XDIGIT(*hptr)) != 0xff)
          {
          if ((cc & 0xf0000000) != 0)  /* Test for 32-bit overflow */
            {
            *errorcodeptr = ERR77;
            ptr = hptr;   /* Show where */
            break;        /* *hptr != } will cause another break below */
            }
          cc = (cc << 4) | xc;
          hptr++;
          }

        if (hptr == ptr + 1 ||   /* No hex digits */
            hptr >= ptrend ||    /* Hit end of input */
            *hptr != CHAR_RIGHT_CURLY_BRACKET)  /* No } terminator */
          break;         /* Hex escape not recognized */

        c = cc;          /* Accept the code point */
        ptr = hptr + 1;
        }

      else  /* Must be exactly 4 hex digits */
        {
        if (ptrend - ptr < 4) break;               /* Less than 4 chars */
        if ((cc = XDIGIT(ptr[0])) == 0xff) break;  /* Not a hex digit */
        if ((xc = XDIGIT(ptr[1])) == 0xff) break;  /* Not a hex digit */
        cc = (cc << 4) | xc;
        if ((xc = XDIGIT(ptr[2])) == 0xff) break;  /* Not a hex digit */
        cc = (cc << 4) | xc;
        if ((xc = XDIGIT(ptr[3])) == 0xff) break;  /* Not a hex digit */
        c = (cc << 4) | xc;
        ptr += 4;
        }

      if (utf)
        {
        if (c > 0x10ffffU) *errorcodeptr = ERR77;
        else
          if (c >= 0xd800 && c <= 0xdfff &&
              (extra_options & PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES) == 0)
                *errorcodeptr = ERR73;
        }
      else if (c > MAX_NON_UTF_CHAR) *errorcodeptr = ERR77;
      }
    break;

    case CHAR_U:
    if (!alt_bsux) *errorcodeptr = ERR37;
    break;

    case CHAR_g:
    if (isclass) break;

    if (ptr >= ptrend)
      {
      *errorcodeptr = ERR57;
      break;
      }

    if (*ptr == CHAR_LESS_THAN_SIGN || *ptr == CHAR_APOSTROPHE)
      {
      escape = ESC_g;
      break;
      }

    if (*ptr == CHAR_LEFT_CURLY_BRACKET)
      {
      PCRE2_SPTR p = ptr + 1;
      if (!read_number(&p, ptrend, cb->bracount, MAX_GROUP_NUMBER, ERR61, &s,
          errorcodeptr))
        {
        if (*errorcodeptr == 0) escape = ESC_k;  /* No number found */
        break;
        }
      if (p >= ptrend || *p != CHAR_RIGHT_CURLY_BRACKET)
        {
        *errorcodeptr = ERR57;
        break;
        }
      ptr = p + 1;
      }

    else
      {
      if (!read_number(&ptr, ptrend, cb->bracount, MAX_GROUP_NUMBER, ERR61, &s,
          errorcodeptr))
        {
        if (*errorcodeptr == 0) *errorcodeptr = ERR57;  /* No number found */
        break;
        }
      }

    if (s <= 0)
      {
      *errorcodeptr = ERR15;
      break;
      }

    escape = -s;
    break;

    case CHAR_1: case CHAR_2: case CHAR_3: case CHAR_4: case CHAR_5:
    case CHAR_6: case CHAR_7: case CHAR_8: case CHAR_9:

    if (!isclass)
      {
      oldptr = ptr;
      ptr--;   /* Back to the digit */
      if (!read_number(&ptr, ptrend, -1, INT_MAX/10 - 1, ERR61, &s,
          errorcodeptr))
        break;

      if (s < 10 || oldptr[-1] >= CHAR_8 || s <= (int)cb->bracount)
        {
        if (s > (int)MAX_GROUP_NUMBER) *errorcodeptr = ERR61;
          else escape = -s;     /* Indicates a back reference */
        break;
        }
      ptr = oldptr;      /* Put the pointer back and fall through */
      }

    if (c >= CHAR_8) break;

    case CHAR_0:
    c -= CHAR_0;
    while(i++ < 2 && ptr < ptrend && *ptr >= CHAR_0 && *ptr <= CHAR_7)
        c = c * 8 + *ptr++ - CHAR_0;
#if PCRE2_CODE_UNIT_WIDTH == 8
    if (!utf && c > 0xff) *errorcodeptr = ERR51;
#endif
    break;

    case CHAR_o:
    if (ptr >= ptrend || *ptr++ != CHAR_LEFT_CURLY_BRACKET)
      {
      ptr--;
      *errorcodeptr = ERR55;
      }
    else if (ptr >= ptrend || *ptr == CHAR_RIGHT_CURLY_BRACKET)
      *errorcodeptr = ERR78;
    else
      {
      c = 0;
      overflow = FALSE;
      while (ptr < ptrend && *ptr >= CHAR_0 && *ptr <= CHAR_7)
        {
        cc = *ptr++;
        if (c == 0 && cc == CHAR_0) continue;     /* Leading zeroes */
#if PCRE2_CODE_UNIT_WIDTH == 32
        if (c >= 0x20000000l) { overflow = TRUE; break; }
#endif
        c = (c << 3) + (cc - CHAR_0);
#if PCRE2_CODE_UNIT_WIDTH == 8
        if (c > (utf ? 0x10ffffU : 0xffU)) { overflow = TRUE; break; }
#elif PCRE2_CODE_UNIT_WIDTH == 16
        if (c > (utf ? 0x10ffffU : 0xffffU)) { overflow = TRUE; break; }
#elif PCRE2_CODE_UNIT_WIDTH == 32
        if (utf && c > 0x10ffffU) { overflow = TRUE; break; }
#endif
        }
      if (overflow)
        {
        while (ptr < ptrend && *ptr >= CHAR_0 && *ptr <= CHAR_7) ptr++;
        *errorcodeptr = ERR34;
        }
      else if (ptr < ptrend && *ptr++ == CHAR_RIGHT_CURLY_BRACKET)
        {
        if (utf && c >= 0xd800 && c <= 0xdfff &&
            (extra_options & PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES) == 0)
          {
          ptr--;
          *errorcodeptr = ERR73;
          }
        }
      else
        {
        ptr--;
        *errorcodeptr = ERR64;
        }
      }
    break;

    case CHAR_x:
    if (alt_bsux)
      {
      uint32_t xc;
      if (ptrend - ptr < 2) break;               /* Less than 2 characters */
      if ((cc = XDIGIT(ptr[0])) == 0xff) break;  /* Not a hex digit */
      if ((xc = XDIGIT(ptr[1])) == 0xff) break;  /* Not a hex digit */
      c = (cc << 4) | xc;
      ptr += 2;
      }

    else
      {
      if (ptr < ptrend && *ptr == CHAR_LEFT_CURLY_BRACKET)
        {
#ifndef EBCDIC
        COME_FROM_NU:
#endif
        if (++ptr >= ptrend || *ptr == CHAR_RIGHT_CURLY_BRACKET)
          {
          *errorcodeptr = ERR78;
          break;
          }
        c = 0;
        overflow = FALSE;

        while (ptr < ptrend && (cc = XDIGIT(*ptr)) != 0xff)
          {
          ptr++;
          if (c == 0 && cc == 0) continue;   /* Leading zeroes */
#if PCRE2_CODE_UNIT_WIDTH == 32
          if (c >= 0x10000000l) { overflow = TRUE; break; }
#endif
          c = (c << 4) | cc;
          if ((utf && c > 0x10ffffU) || (!utf && c > MAX_NON_UTF_CHAR))
            {
            overflow = TRUE;
            break;
            }
          }

        if (overflow)
          {
          while (ptr < ptrend && XDIGIT(*ptr) != 0xff) ptr++;
          *errorcodeptr = ERR34;
          }
        else if (ptr < ptrend && *ptr++ == CHAR_RIGHT_CURLY_BRACKET)
          {
          if (utf && c >= 0xd800 && c <= 0xdfff &&
              (extra_options & PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES) == 0)
            {
            ptr--;
            *errorcodeptr = ERR73;
            }
          }

        else
          {
          ptr--;
          *errorcodeptr = ERR67;
          }
        }   /* End of \x{} processing */

      else
        {
        c = 0;
        if (ptr >= ptrend || (cc = XDIGIT(*ptr)) == 0xff) break;  /* Not a hex digit */
        ptr++;
        c = cc;
        if (ptr >= ptrend || (cc = XDIGIT(*ptr)) == 0xff) break;  /* Not a hex digit */
        ptr++;
        c = (c << 4) | cc;
        }     /* End of \xdd handling */
      }       /* End of Perl-style \x handling */
    break;

#if defined EBCDIC && 'a' != 0x81
    case 0x83:
#else
    case CHAR_c:
#endif
    if (ptr >= ptrend)
      {
      *errorcodeptr = ERR2;
      break;
      }
    c = *ptr;
    if (c >= CHAR_a && c <= CHAR_z) c = UPPER_CASE(c);

#ifndef EBCDIC    /* ASCII/UTF-8 coding */
    if (c < 32 || c > 126)  /* Excludes all non-printable ASCII */
      {
      *errorcodeptr = ERR68;
      break;
      }
    c ^= 0x40;

#else
    if (c == CHAR_QUESTION_MARK)
      c = ('\\' == 188 && '`' == 74)? 0x5f : 0xff;
    else
      {
      for (i = 0; i < 32; i++)
        {
        if (c == ebcdic_escape_c[i]) break;
        }
      if (i < 32) c = i; else *errorcodeptr = ERR68;
      }
#endif  /* EBCDIC */

    ptr++;
    break;

    default:
    *errorcodeptr = ERR3;
    *ptrptr = ptr - 1;     /* Point to the character at fault */
    return 0;
    }
  }

*ptrptr = ptr;
*chptr = c;
return escape;
}

#ifdef SUPPORT_UNICODE
/*************************************************
*               Handle \P and \p                 *
*************************************************/

static BOOL
get_ucp(PCRE2_SPTR *ptrptr, BOOL *negptr, uint16_t *ptypeptr,
  uint16_t *pdataptr, int *errorcodeptr, compile_block *cb)
{
PCRE2_UCHAR c;
PCRE2_SIZE i, bot, top;
PCRE2_SPTR ptr = *ptrptr;
PCRE2_UCHAR name[32];

if (ptr >= cb->end_pattern) goto ERROR_RETURN;
c = *ptr++;
*negptr = FALSE;

if (c == CHAR_LEFT_CURLY_BRACKET)
  {
  if (ptr >= cb->end_pattern) goto ERROR_RETURN;
  if (*ptr == CHAR_CIRCUMFLEX_ACCENT)
    {
    *negptr = TRUE;
    ptr++;
    }
  for (i = 0; i < (int)(sizeof(name) / sizeof(PCRE2_UCHAR)) - 1; i++)
    {
    if (ptr >= cb->end_pattern) goto ERROR_RETURN;
    c = *ptr++;
    if (c == CHAR_NUL) goto ERROR_RETURN;
    if (c == CHAR_RIGHT_CURLY_BRACKET) break;
    name[i] = c;
    }
  if (c != CHAR_RIGHT_CURLY_BRACKET) goto ERROR_RETURN;
  name[i] = 0;
  }

else if (MAX_255(c) && (cb->ctypes[c] & ctype_letter) != 0)
  {
  name[0] = c;
  name[1] = 0;
  }
else goto ERROR_RETURN;

*ptrptr = ptr;

bot = 0;
top = PRIV(utt_size);

while (bot < top)
  {
  int r;
  i = (bot + top) >> 1;
  r = PRIV(strcmp_c8)(name, PRIV(utt_names) + PRIV(utt)[i].name_offset);
  if (r == 0)
    {
    *ptypeptr = PRIV(utt)[i].type;
    *pdataptr = PRIV(utt)[i].value;
    return TRUE;
    }
  if (r > 0) bot = i + 1; else top = i;
  }
*errorcodeptr = ERR47;   /* Unrecognized name */
return FALSE;

ERROR_RETURN:            /* Malformed \P or \p */
*errorcodeptr = ERR46;
*ptrptr = ptr;
return FALSE;
}
#endif

/*************************************************
*           Check for POSIX class syntax         *
*************************************************/

static BOOL
check_posix_syntax(PCRE2_SPTR ptr, PCRE2_SPTR ptrend, PCRE2_SPTR *endptr)
{
PCRE2_UCHAR terminator;  /* Don't combine these lines; the Solaris cc */
terminator = *ptr++;     /* compiler warns about "non-constant" initializer. */

for (; ptrend - ptr >= 2; ptr++)
  {
  if (*ptr == CHAR_BACKSLASH &&
      (ptr[1] == CHAR_RIGHT_SQUARE_BRACKET || ptr[1] == CHAR_BACKSLASH))
    ptr++;

  else if ((*ptr == CHAR_LEFT_SQUARE_BRACKET && ptr[1] == terminator) ||
            *ptr == CHAR_RIGHT_SQUARE_BRACKET) return FALSE;

  else if (*ptr == terminator && ptr[1] == CHAR_RIGHT_SQUARE_BRACKET)
    {
    *endptr = ptr;
    return TRUE;
    }
  }

return FALSE;
}

/*************************************************
*          Check POSIX class name                *
*************************************************/

static int
check_posix_name(PCRE2_SPTR ptr, int len)
{
const char *pn = posix_names;
int yield = 0;
while (posix_name_lengths[yield] != 0)
  {
  if (len == posix_name_lengths[yield] &&
    PRIV(strncmp_c8)(ptr, pn, (unsigned int)len) == 0) return yield;
  pn += posix_name_lengths[yield] + 1;
  yield++;
  }
return -1;
}

/*************************************************
*       Read a subpattern or VERB name           *
*************************************************/

static BOOL
read_name(PCRE2_SPTR *ptrptr, PCRE2_SPTR ptrend, BOOL utf, uint32_t terminator,
  PCRE2_SIZE *offsetptr, PCRE2_SPTR *nameptr, uint32_t *namelenptr,
  int *errorcodeptr, compile_block *cb)
{
PCRE2_SPTR ptr = *ptrptr;
BOOL is_group = (*ptr != CHAR_ASTERISK);

if (++ptr >= ptrend)               /* No characters in name */
  {
  *errorcodeptr = is_group? ERR62: /* Subpattern name expected */
                            ERR60; /* Verb not recognized or malformed */
  goto FAILED;
  }

*nameptr = ptr;
*offsetptr = (PCRE2_SIZE)(ptr - cb->start_pattern);

#ifdef SUPPORT_UNICODE
if (utf && is_group)
  {
  uint32_t c, type;

  GETCHAR(c, ptr);
  type = UCD_CHARTYPE(c);

  if (type == ucp_Nd)
    {
    *errorcodeptr = ERR44;
    goto FAILED;
    }

  for(;;)
    {
    if (type != ucp_Nd && PRIV(ucp_gentype)[type] != ucp_L &&
        c != CHAR_UNDERSCORE) break;
    ptr++;
    FORWARDCHARTEST(ptr, ptrend);
    if (ptr >= ptrend) break;
    GETCHAR(c, ptr);
    type = UCD_CHARTYPE(c);
    }
  }
else
#else
(void)utf;  /* Avoid compiler warning */
#endif      /* SUPPORT_UNICODE */

  {
  if (is_group && IS_DIGIT(*ptr))
    {
    *errorcodeptr = ERR44;
    goto FAILED;
    }

  while (ptr < ptrend && MAX_255(*ptr) && (cb->ctypes[*ptr] & ctype_word) != 0)
    {
    ptr++;
    }
  }

/* Check name length */

if (ptr > *nameptr + MAX_NAME_SIZE)
  {
  *errorcodeptr = ERR48;
  goto FAILED;
  }
*namelenptr = ptr - *nameptr;

if (is_group)
  {
  if (ptr == *nameptr)
    {
    *errorcodeptr = ERR62;   /* Subpattern name expected */
    goto FAILED;
    }
  if (ptr >= ptrend || *ptr != (PCRE2_UCHAR)terminator)
    {
    *errorcodeptr = ERR42;
    goto FAILED;
    }
  ptr++;
  }

*ptrptr = ptr;
return TRUE;

FAILED:
*ptrptr = ptr;
return FALSE;
}

/*************************************************
*          Manage callouts at start of cycle     *
*************************************************/

static uint32_t *
manage_callouts(PCRE2_SPTR ptr, uint32_t **pcalloutptr, BOOL auto_callout,
  uint32_t *parsed_pattern, compile_block *cb)
{
uint32_t *previous_callout = *pcalloutptr;

if (previous_callout != NULL) previous_callout[2] = (uint32_t)(ptr -
  cb->start_pattern - (PCRE2_SIZE)previous_callout[1]);

if (!auto_callout) previous_callout = NULL; else
  {
  if (previous_callout == NULL ||
      previous_callout != parsed_pattern - 4 ||
      previous_callout[3] != 255)
    {
    previous_callout = parsed_pattern;  /* Set up new automatic callout */
    parsed_pattern += 4;
    previous_callout[0] = META_CALLOUT_NUMBER;
    previous_callout[2] = 0;
    previous_callout[3] = 255;
    }
  previous_callout[1] = (uint32_t)(ptr - cb->start_pattern);
  }

*pcalloutptr = previous_callout;
return parsed_pattern;
}

/*************************************************
*      Parse regex and identify named groups     *
*************************************************/

typedef struct nest_save {
  uint16_t  nest_depth;
  uint16_t  reset_group;
  uint16_t  max_group;
  uint16_t  flags;
  uint32_t  options;
} nest_save;

#define NSF_RESET          0x0001u
#define NSF_CONDASSERT     0x0002u
#define NSF_ATOMICSR       0x0004u

#define PARSE_TRACKED_OPTIONS (PCRE2_CASELESS|PCRE2_DOTALL|PCRE2_DUPNAMES| \
  PCRE2_EXTENDED|PCRE2_EXTENDED_MORE|PCRE2_MULTILINE|PCRE2_NO_AUTO_CAPTURE| \
  PCRE2_UNGREEDY)

enum { RANGE_NO, RANGE_STARTED, RANGE_OK_ESCAPED, RANGE_OK_LITERAL };

#if PCRE2_CODE_UNIT_WIDTH == 32
#define PARSED_LITERAL(c, p) \
  { \
  if (c >= META_END) *p++ = META_BIGVALUE; \
  *p++ = c; \
  okquantifier = TRUE; \
  }
#else
#define PARSED_LITERAL(c, p) *p++ = c; okquantifier = TRUE;
#endif

/* Here's the actual function. */

static int parse_regex(PCRE2_SPTR ptr, uint32_t options, BOOL *has_lookbehind,
  compile_block *cb)
{
uint32_t c;
uint32_t delimiter;
uint32_t namelen;
uint32_t class_range_state;
uint32_t *verblengthptr = NULL;     /* Value avoids compiler warning */
uint32_t *verbstartptr = NULL;
uint32_t *previous_callout = NULL;
uint32_t *parsed_pattern = cb->parsed_pattern;
uint32_t *parsed_pattern_end = cb->parsed_pattern_end;
uint32_t meta_quantifier = 0;
uint32_t add_after_mark = 0;
uint32_t extra_options = cb->cx->extra_options;
uint16_t nest_depth = 0;
int after_manual_callout = 0;
int expect_cond_assert = 0;
int errorcode = 0;
int escape;
int i;
BOOL inescq = FALSE;
BOOL inverbname = FALSE;
BOOL utf = (options & PCRE2_UTF) != 0;
BOOL auto_callout = (options & PCRE2_AUTO_CALLOUT) != 0;
BOOL isdupname;
BOOL negate_class;
BOOL okquantifier = FALSE;
PCRE2_SPTR thisptr;
PCRE2_SPTR name;
PCRE2_SPTR ptrend = cb->end_pattern;
PCRE2_SPTR verbnamestart = NULL;    /* Value avoids compiler warning */
named_group *ng;
nest_save *top_nest, *end_nests;

if ((extra_options & PCRE2_EXTRA_MATCH_LINE) != 0)
  {
  *parsed_pattern++ = META_CIRCUMFLEX;
  *parsed_pattern++ = META_NOCAPTURE;
  }
else if ((extra_options & PCRE2_EXTRA_MATCH_WORD) != 0)
  {
  *parsed_pattern++ = META_ESCAPE + ESC_b;
  *parsed_pattern++ = META_NOCAPTURE;
  }

if ((options & PCRE2_LITERAL) != 0)
  {
  while (ptr < ptrend)
    {
    if (parsed_pattern >= parsed_pattern_end)
      {
      errorcode = ERR63;  /* Internal error (parsed pattern overflow) */
      goto FAILED;
      }
    thisptr = ptr;
    GETCHARINCTEST(c, ptr);
    if (auto_callout)
      parsed_pattern = manage_callouts(thisptr, &previous_callout,
        auto_callout, parsed_pattern, cb);
    PARSED_LITERAL(c, parsed_pattern);
    }
  goto PARSED_END;
  }

top_nest = NULL;
end_nests = (nest_save *)(cb->start_workspace + cb->workspace_size);

end_nests = (nest_save *)((char *)end_nests -
  ((cb->workspace_size * sizeof(PCRE2_UCHAR)) % sizeof(nest_save)));

/* PCRE2_EXTENDED_MORE implies PCRE2_EXTENDED */

if ((options & PCRE2_EXTENDED_MORE) != 0) options |= PCRE2_EXTENDED;

/* Now scan the pattern */

while (ptr < ptrend)
  {
  int prev_expect_cond_assert;
  uint32_t min_repeat, max_repeat;
  uint32_t set, unset, *optset;
  uint32_t terminator;
  uint32_t prev_meta_quantifier;
  BOOL prev_okquantifier;
  PCRE2_SPTR tempptr;
  PCRE2_SIZE offset;

  if (parsed_pattern >= parsed_pattern_end)
    {
    errorcode = ERR63;  /* Internal error (parsed pattern overflow) */
    goto FAILED;
    }

  if (nest_depth > cb->cx->parens_nest_limit)
    {
    errorcode = ERR19;
    goto FAILED;        /* Parentheses too deeply nested */
    }

  thisptr = ptr;
  GETCHARINCTEST(c, ptr);

  if (inescq)
    {
    if (c == CHAR_BACKSLASH && ptr < ptrend && *ptr == CHAR_E)
      {
      inescq = FALSE;
      ptr++;   /* Skip E */
      }
    else
      {
      if (expect_cond_assert > 0)   /* A literal is not allowed if we are */
        {                           /* expecting a conditional assertion, */
        ptr--;                      /* but an empty \Q\E sequence is OK.  */
        errorcode = ERR28;
        goto FAILED;
        }
      if (inverbname)
        {                          /* Don't use PARSED_LITERAL() because it */
#if PCRE2_CODE_UNIT_WIDTH == 32    /* sets okquantifier. */
        if (c >= META_END) *parsed_pattern++ = META_BIGVALUE;
#endif
        *parsed_pattern++ = c;
        }
      else
        {
        if (after_manual_callout-- <= 0)
          parsed_pattern = manage_callouts(thisptr, &previous_callout,
            auto_callout, parsed_pattern, cb);
        PARSED_LITERAL(c, parsed_pattern);
        }
      meta_quantifier = 0;
      }
    continue;  /* Next character */
    }

  if (inverbname &&
       (
        /* EITHER: not both options set */
        ((options & (PCRE2_EXTENDED | PCRE2_ALT_VERBNAMES)) !=
                    (PCRE2_EXTENDED | PCRE2_ALT_VERBNAMES)) ||
#ifdef SUPPORT_UNICODE
        /* OR: character > 255 AND not Unicode Pattern White Space */
        (c > 255 && (c|1) != 0x200f && (c|1) != 0x2029) ||
#endif
        /* OR: not a # comment or isspace() white space */
        (c < 256 && c != CHAR_NUMBER_SIGN && (cb->ctypes[c] & ctype_space) == 0
#ifdef SUPPORT_UNICODE
        /* and not CHAR_NEL when Unicode is supported */
          && c != CHAR_NEL
#endif
       )))
    {
    PCRE2_SIZE verbnamelength;

    switch(c)
      {
      default:                     /* Don't use PARSED_LITERAL() because it */
#if PCRE2_CODE_UNIT_WIDTH == 32    /* sets okquantifier. */
      if (c >= META_END) *parsed_pattern++ = META_BIGVALUE;
#endif
      *parsed_pattern++ = c;
      break;

      case CHAR_RIGHT_PARENTHESIS:
      inverbname = FALSE;
      /* This is the length in characters */
      verbnamelength = (PCRE2_SIZE)(parsed_pattern - verblengthptr - 1);
      /* But the limit on the length is in code units */
      if (ptr - verbnamestart - 1 > (int)MAX_MARK)
        {
        ptr--;
        errorcode = ERR76;
        goto FAILED;
        }
      *verblengthptr = (uint32_t)verbnamelength;

      if (add_after_mark != 0)
        {
        *parsed_pattern++ = add_after_mark;
        add_after_mark = 0;
        }
      break;

      case CHAR_BACKSLASH:
      if ((options & PCRE2_ALT_VERBNAMES) != 0)
        {
        escape = PRIV(check_escape)(&ptr, ptrend, &c, &errorcode, options,
          cb->cx->extra_options, FALSE, cb);
        if (errorcode != 0) goto FAILED;
        }
      else escape = 0;   /* Treat all as literal */

      switch(escape)
        {
        case 0:                    /* Don't use PARSED_LITERAL() because it */
#if PCRE2_CODE_UNIT_WIDTH == 32    /* sets okquantifier. */
        if (c >= META_END) *parsed_pattern++ = META_BIGVALUE;
#endif
        *parsed_pattern++ = c;
        break;

        case ESC_Q:
        inescq = TRUE;
        break;

        case ESC_E:           /* Ignore */
        break;

        default:
        errorcode = ERR40;    /* Invalid in verb name */
        goto FAILED;
        }
      }
    continue;   /* Next character in pattern */
    }

  if (c == CHAR_BACKSLASH && ptr < ptrend)
    {
    if (*ptr == CHAR_Q || *ptr == CHAR_E)
      {
      inescq = *ptr == CHAR_Q;
      ptr++;
      continue;
      }
    }

  if ((options & PCRE2_EXTENDED) != 0)
    {
    if (c < 256 && (cb->ctypes[c] & ctype_space) != 0) continue;
#ifdef SUPPORT_UNICODE
    if (c == CHAR_NEL || (c|1) == 0x200f || (c|1) == 0x2029) continue;
#endif
    if (c == CHAR_NUMBER_SIGN)
      {
      while (ptr < ptrend)
        {
        if (IS_NEWLINE(ptr))      /* For non-fixed-length newline cases, */
          {                       /* IS_NEWLINE sets cb->nllen. */
          ptr += cb->nllen;
          break;
          }
        ptr++;
#ifdef SUPPORT_UNICODE
        if (utf) FORWARDCHARTEST(ptr, ptrend);
#endif
        }
      continue;  /* Next character in pattern */
      }
    }

  if (c == CHAR_LEFT_PARENTHESIS && ptrend - ptr >= 2 &&
      ptr[0] == CHAR_QUESTION_MARK && ptr[1] == CHAR_NUMBER_SIGN)
    {
    while (++ptr < ptrend && *ptr != CHAR_RIGHT_PARENTHESIS);
    if (ptr >= ptrend)
      {
      errorcode = ERR18;  /* A special error for missing ) in a comment */
      goto FAILED;        /* to make it easier to debug. */
      }
    ptr++;
    continue;  /* Next character in pattern */
    }

  if (c != CHAR_ASTERISK && c != CHAR_PLUS && c != CHAR_QUESTION_MARK &&
       (c != CHAR_LEFT_CURLY_BRACKET ||
         (tempptr = ptr,
         !read_repeat_counts(&tempptr, ptrend, NULL, NULL, &errorcode))))
    {
    if (after_manual_callout-- <= 0)
      parsed_pattern = manage_callouts(thisptr, &previous_callout, auto_callout,
        parsed_pattern, cb);
    }

  if (expect_cond_assert > 0)
    {
    BOOL ok = c == CHAR_LEFT_PARENTHESIS && ptrend - ptr >= 3 &&
              (ptr[0] == CHAR_QUESTION_MARK || ptr[0] == CHAR_ASTERISK);
    if (ok)
      {
      if (ptr[0] == CHAR_ASTERISK)  /* New alpha assertion format, possibly */
        {
        ok = MAX_255(ptr[1]) && (cb->ctypes[ptr[1]] & ctype_lcletter) != 0;
        }
      else switch(ptr[1])  /* Traditional symbolic format */
        {
        case CHAR_C:
        ok = expect_cond_assert == 2;
        break;

        case CHAR_EQUALS_SIGN:
        case CHAR_EXCLAMATION_MARK:
        break;

        case CHAR_LESS_THAN_SIGN:
        ok = ptr[2] == CHAR_EQUALS_SIGN || ptr[2] == CHAR_EXCLAMATION_MARK;
        break;

        default:
        ok = FALSE;
        }
      }

    if (!ok)
      {
      ptr--;   /* Adjust error offset */
      errorcode = ERR28;
      goto FAILED;
      }
    }

  prev_expect_cond_assert = expect_cond_assert;
  expect_cond_assert = 0;

  prev_okquantifier = okquantifier;
  prev_meta_quantifier = meta_quantifier;
  okquantifier = FALSE;
  meta_quantifier = 0;

  if (prev_meta_quantifier != 0 && (c == CHAR_QUESTION_MARK || c == CHAR_PLUS))
    {
    parsed_pattern[(prev_meta_quantifier == META_MINMAX)? -3 : -1] =
      prev_meta_quantifier + ((c == CHAR_QUESTION_MARK)?
        0x00020000u : 0x00010000u);
    continue;  /* Next character in pattern */
    }

  switch(c)
    {
    default:              /* Non-special character */
    PARSED_LITERAL(c, parsed_pattern);
    break;

    case CHAR_BACKSLASH:
    tempptr = ptr;
    escape = PRIV(check_escape)(&ptr, ptrend, &c, &errorcode, options,
      cb->cx->extra_options, FALSE, cb);
    if (errorcode != 0)
      {
      ESCAPE_FAILED:
      if ((extra_options & PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL) == 0)
        goto FAILED;
      ptr = tempptr;
      if (ptr >= ptrend) c = CHAR_BACKSLASH; else
        {
        GETCHARINCTEST(c, ptr);   /* Get character value, increment pointer */
        }
      escape = 0;                 /* Treat as literal character */
      }

    if (escape == 0)
      {
      PARSED_LITERAL(c, parsed_pattern);
      }

    else if (escape < 0)
      {
      offset = (PCRE2_SIZE)(ptr - cb->start_pattern - 1);
      escape = -escape;
      *parsed_pattern++ = META_BACKREF | (uint32_t)escape;
      if (escape < 10)
        {
        if (cb->small_ref_offset[escape] == PCRE2_UNSET)
          cb->small_ref_offset[escape] = offset;
        }
      else
        {
        PUTOFFSET(offset, parsed_pattern);
        }
      okquantifier = TRUE;
      }

    else switch (escape)
      {
      case ESC_C:
#ifdef NEVER_BACKSLASH_C
      errorcode = ERR85;
      goto ESCAPE_FAILED;
#else
      if ((options & PCRE2_NEVER_BACKSLASH_C) != 0)
        {
        errorcode = ERR83;
        goto ESCAPE_FAILED;
        }
#endif
      okquantifier = TRUE;
      *parsed_pattern++ = META_ESCAPE + escape;
      break;

      case ESC_X:
#ifndef SUPPORT_UNICODE
      errorcode = ERR45;   /* Supported only with Unicode support */
      goto ESCAPE_FAILED;
#endif
      case ESC_H:
      case ESC_h:
      case ESC_N:
      case ESC_R:
      case ESC_V:
      case ESC_v:
      okquantifier = TRUE;
      *parsed_pattern++ = META_ESCAPE + escape;
      break;

      default:  /* \A, \B, \b, \G, \K, \Z, \z cannot be quantified. */
      *parsed_pattern++ = META_ESCAPE + escape;
      break;

      case ESC_d:
      case ESC_D:
      case ESC_s:
      case ESC_S:
      case ESC_w:
      case ESC_W:
      okquantifier = TRUE;
      if ((options & PCRE2_UCP) == 0)
        {
        *parsed_pattern++ = META_ESCAPE + escape;
        }
      else
        {
        *parsed_pattern++ = META_ESCAPE +
          ((escape == ESC_d || escape == ESC_s || escape == ESC_w)?
            ESC_p : ESC_P);
        switch(escape)
          {
          case ESC_d:
          case ESC_D:
          *parsed_pattern++ = (PT_PC << 16) | ucp_Nd;
          break;

          case ESC_s:
          case ESC_S:
          *parsed_pattern++ = PT_SPACE << 16;
          break;

          case ESC_w:
          case ESC_W:
          *parsed_pattern++ = PT_WORD << 16;
          break;
          }
        }
      break;

      case ESC_P:
      case ESC_p:
#ifdef SUPPORT_UNICODE
        {
        BOOL negated;
        uint16_t ptype = 0, pdata = 0;
        if (!get_ucp(&ptr, &negated, &ptype, &pdata, &errorcode, cb))
          goto ESCAPE_FAILED;
        if (negated) escape = (escape == ESC_P)? ESC_p : ESC_P;
        *parsed_pattern++ = META_ESCAPE + escape;
        *parsed_pattern++ = (ptype << 16) | pdata;
        okquantifier = TRUE;
        }
#else
      errorcode = ERR45;
      goto ESCAPE_FAILED;
#endif
      break;  /* End \P and \p */

      case ESC_g:
      case ESC_k:
      if (ptr >= ptrend || (*ptr != CHAR_LEFT_CURLY_BRACKET &&
          *ptr != CHAR_LESS_THAN_SIGN && *ptr != CHAR_APOSTROPHE))
        {
        errorcode = (escape == ESC_g)? ERR57 : ERR69;
        goto ESCAPE_FAILED;
        }
      terminator = (*ptr == CHAR_LESS_THAN_SIGN)?
        CHAR_GREATER_THAN_SIGN : (*ptr == CHAR_APOSTROPHE)?
        CHAR_APOSTROPHE : CHAR_RIGHT_CURLY_BRACKET;

      if (escape == ESC_g && terminator != CHAR_RIGHT_CURLY_BRACKET)
        {
        PCRE2_SPTR p = ptr + 1;

        if (read_number(&p, ptrend, cb->bracount, MAX_GROUP_NUMBER, ERR61, &i,
            &errorcode))
          {
          if (p >= ptrend || *p != terminator)
            {
            errorcode = ERR57;
            goto ESCAPE_FAILED;
            }
          ptr = p;
          goto SET_RECURSION;
          }
        if (errorcode != 0) goto ESCAPE_FAILED;
        }

      if (!read_name(&ptr, ptrend, utf, terminator, &offset, &name, &namelen,
          &errorcode, cb)) goto ESCAPE_FAILED;

      *parsed_pattern++ =
        (escape == ESC_k || terminator == CHAR_RIGHT_CURLY_BRACKET)?
          META_BACKREF_BYNAME : META_RECURSE_BYNAME;
      *parsed_pattern++ = namelen;

      PUTOFFSET(offset, parsed_pattern);
      okquantifier = TRUE;
      break;  /* End special escape processing */
      }
    break;    /* End escape sequence processing */

    case CHAR_CIRCUMFLEX_ACCENT:
    *parsed_pattern++ = META_CIRCUMFLEX;
    break;

    case CHAR_DOLLAR_SIGN:
    *parsed_pattern++ = META_DOLLAR;
    break;

    case CHAR_DOT:
    *parsed_pattern++ = META_DOT;
    okquantifier = TRUE;
    break;

    case CHAR_ASTERISK:
    meta_quantifier = META_ASTERISK;
    goto CHECK_QUANTIFIER;

    case CHAR_PLUS:
    meta_quantifier = META_PLUS;
    goto CHECK_QUANTIFIER;

    case CHAR_QUESTION_MARK:
    meta_quantifier = META_QUERY;
    goto CHECK_QUANTIFIER;

    case CHAR_LEFT_CURLY_BRACKET:
    if (!read_repeat_counts(&ptr, ptrend, &min_repeat, &max_repeat,
        &errorcode))
      {
      if (errorcode != 0) goto FAILED;     /* Error in quantifier. */
      PARSED_LITERAL(c, parsed_pattern);   /* Not a quantifier */
      break;                               /* No more quantifier processing */
      }
    meta_quantifier = META_MINMAX;

    CHECK_QUANTIFIER:
    if (!prev_okquantifier)
      {
      errorcode = ERR9;
      goto FAILED_BACK;
      }

    if (parsed_pattern[-1] == META_ACCEPT)
      {
      uint32_t *p;
      for (p = parsed_pattern - 1; p >= verbstartptr; p--) p[1] = p[0];
      *verbstartptr = META_NOCAPTURE;
      parsed_pattern[1] = META_KET;
      parsed_pattern += 2;
      }

    *parsed_pattern++ = meta_quantifier;
    if (c == CHAR_LEFT_CURLY_BRACKET)
      {
      *parsed_pattern++ = min_repeat;
      *parsed_pattern++ = max_repeat;
      }
    break;

    case CHAR_LEFT_SQUARE_BRACKET:
    okquantifier = TRUE;

    if (ptrend - ptr >= 6 &&
         (PRIV(strncmp_c8)(ptr, STRING_WEIRD_STARTWORD, 6) == 0 ||
          PRIV(strncmp_c8)(ptr, STRING_WEIRD_ENDWORD, 6) == 0))
      {
      *parsed_pattern++ = META_ESCAPE + ESC_b;

      if (ptr[2] == CHAR_LESS_THAN_SIGN)
        {
        *parsed_pattern++ = META_LOOKAHEAD;
        }
      else
        {
        *parsed_pattern++ = META_LOOKBEHIND;
        *has_lookbehind = TRUE;

        PUTOFFSET((PCRE2_SIZE)0, parsed_pattern);
        }

      if ((options & PCRE2_UCP) == 0)
        *parsed_pattern++ = META_ESCAPE + ESC_w;
      else
        {
        *parsed_pattern++ = META_ESCAPE + ESC_p;
        *parsed_pattern++ = PT_WORD << 16;
        }
      *parsed_pattern++ = META_KET;
      ptr += 6;
      break;
      }

    if (ptr < ptrend && (*ptr == CHAR_COLON || *ptr == CHAR_DOT ||
         *ptr == CHAR_EQUALS_SIGN) &&
        check_posix_syntax(ptr, ptrend, &tempptr))
      {
      errorcode = (*ptr-- == CHAR_COLON)? ERR12 : ERR13;
      goto FAILED;
      }

    negate_class = FALSE;
    while (ptr < ptrend)
      {
      GETCHARINCTEST(c, ptr);
      if (c == CHAR_BACKSLASH)
        {
        if (ptr < ptrend && *ptr == CHAR_E) ptr++;
        else if (ptrend - ptr >= 3 &&
             PRIV(strncmp_c8)(ptr, STR_Q STR_BACKSLASH STR_E, 3) == 0)
          ptr += 3;
        else
          break;
        }
      else if ((options & PCRE2_EXTENDED_MORE) != 0 &&
               (c == CHAR_SPACE || c == CHAR_HT))  /* Note: just these two */
        continue;
      else if (!negate_class && c == CHAR_CIRCUMFLEX_ACCENT)
        negate_class = TRUE;
      else break;
      }

    if (c == CHAR_RIGHT_SQUARE_BRACKET &&
        (cb->external_options & PCRE2_ALLOW_EMPTY_CLASS) != 0)
      {
      *parsed_pattern++ = negate_class? META_CLASS_EMPTY_NOT : META_CLASS_EMPTY;
      break;  /* End of class processing */
      }

    *parsed_pattern++ = negate_class? META_CLASS_NOT : META_CLASS;
    class_range_state = RANGE_NO;

    for (;;)
      {
      BOOL char_is_literal = TRUE;

      if (inescq)
        {
        if (c == CHAR_BACKSLASH && ptr < ptrend && *ptr == CHAR_E)
          {
          inescq = FALSE;                   /* Reset literal state */
          ptr++;                            /* Skip the 'E' */
          goto CLASS_CONTINUE;
          }
        goto CLASS_LITERAL;
        }

      if ((options & PCRE2_EXTENDED_MORE) != 0 &&
          (c == CHAR_SPACE || c == CHAR_HT))
        goto CLASS_CONTINUE;

      if (c == CHAR_LEFT_SQUARE_BRACKET &&
          ptrend - ptr >= 3 &&
          (*ptr == CHAR_COLON || *ptr == CHAR_DOT ||
           *ptr == CHAR_EQUALS_SIGN) &&
          check_posix_syntax(ptr, ptrend, &tempptr))
        {
        BOOL posix_negate = FALSE;
        int posix_class;

        if (class_range_state == RANGE_STARTED)
          {
          errorcode = ERR50;
          goto FAILED;
          }

        if (*ptr != CHAR_COLON)
          {
          errorcode = ERR13;
          goto FAILED_BACK;
          }

        if (*(++ptr) == CHAR_CIRCUMFLEX_ACCENT)
          {
          posix_negate = TRUE;
          ptr++;
          }

        posix_class = check_posix_name(ptr, (int)(tempptr - ptr));
        if (posix_class < 0)
          {
          errorcode = ERR30;
          goto FAILED;
          }
        ptr = tempptr + 2;

        if (ptr < ptrend - 1 && *ptr == CHAR_MINUS &&
            ptr[1] != CHAR_RIGHT_SQUARE_BRACKET)
          {
          errorcode = ERR50;
          goto FAILED;
          }

        class_range_state = RANGE_NO;

#ifdef SUPPORT_UNICODE
        if ((options & PCRE2_UCP) != 0)
          {
          int ptype = posix_substitutes[2*posix_class];
          int pvalue = posix_substitutes[2*posix_class + 1];
          if (ptype >= 0)
            {
            *parsed_pattern++ = META_ESCAPE + (posix_negate? ESC_P : ESC_p);
            *parsed_pattern++ = (ptype << 16) | pvalue;
            goto CLASS_CONTINUE;
            }

          if (pvalue != 0)
            {
            *parsed_pattern++ = META_ESCAPE + (posix_negate? ESC_H : ESC_h);
            goto CLASS_CONTINUE;
            }

          /* Fall through */
          }
#endif  /* SUPPORT_UNICODE */

        *parsed_pattern++ = posix_negate? META_POSIX_NEG : META_POSIX;
        *parsed_pattern++ = posix_class;
        }

      else if (c == CHAR_MINUS && class_range_state >= RANGE_OK_ESCAPED)
        {
        *parsed_pattern++ = (class_range_state == RANGE_OK_LITERAL)?
          META_RANGE_LITERAL : META_RANGE_ESCAPED;
        class_range_state = RANGE_STARTED;
        }

      else if (c != CHAR_BACKSLASH)
        {
        CLASS_LITERAL:
        if (class_range_state == RANGE_STARTED)
          {
          if (c == parsed_pattern[-2])       /* Optimize one-char range */
            parsed_pattern--;
          else if (parsed_pattern[-2] > c)   /* Check range is in order */
            {
            errorcode = ERR8;
            goto FAILED_BACK;
            }
          else
            {
            if (!char_is_literal && parsed_pattern[-1] == META_RANGE_LITERAL)
              parsed_pattern[-1] = META_RANGE_ESCAPED;
            PARSED_LITERAL(c, parsed_pattern);
            }
          class_range_state = RANGE_NO;
          }
        else  /* Potential start of range */
          {
          class_range_state = char_is_literal?
            RANGE_OK_LITERAL : RANGE_OK_ESCAPED;
          PARSED_LITERAL(c, parsed_pattern);
          }
        }

      else
        {
        tempptr = ptr;
        escape = PRIV(check_escape)(&ptr, ptrend, &c, &errorcode, options,
          cb->cx->extra_options, TRUE, cb);

        if (errorcode != 0)
          {
          if ((extra_options & PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL) == 0)
            goto FAILED;
          ptr = tempptr;
          if (ptr >= ptrend) c = CHAR_BACKSLASH; else
            {
            GETCHARINCTEST(c, ptr);   /* Get character value, increment pointer */
            }
          escape = 0;                 /* Treat as literal character */
          }

        switch(escape)
          {
          case 0:  /* Escaped character code point is in c */
          char_is_literal = FALSE;
          goto CLASS_LITERAL;

          case ESC_b:
          c = CHAR_BS;    /* \b is backspace in a class */
          char_is_literal = FALSE;
          goto CLASS_LITERAL;

          case ESC_Q:
          inescq = TRUE;  /* Enter literal mode */
          goto CLASS_CONTINUE;

          case ESC_E:     /* Ignore orphan \E */
          goto CLASS_CONTINUE;

          case ESC_B:     /* Always an error in a class */
          case ESC_R:
          case ESC_X:
          errorcode = ERR7;
          ptr--;
          goto FAILED;
          }

        if (class_range_state == RANGE_STARTED)
          {
          errorcode = ERR50;
          goto FAILED;  /* Not CLASS_ESCAPE_FAILED; always an error */
          }

        class_range_state = RANGE_NO;
        switch(escape)
          {
          case ESC_N:
          errorcode = ERR71;
          goto FAILED;

          case ESC_H:
          case ESC_h:
          case ESC_V:
          case ESC_v:
          *parsed_pattern++ = META_ESCAPE + escape;
          break;

          case ESC_d:
          case ESC_D:
          case ESC_s:
          case ESC_S:
          case ESC_w:
          case ESC_W:
          if ((options & PCRE2_UCP) == 0)
            {
            *parsed_pattern++ = META_ESCAPE + escape;
            }
          else
            {
            *parsed_pattern++ = META_ESCAPE +
              ((escape == ESC_d || escape == ESC_s || escape == ESC_w)?
                ESC_p : ESC_P);
            switch(escape)
              {
              case ESC_d:
              case ESC_D:
              *parsed_pattern++ = (PT_PC << 16) | ucp_Nd;
              break;

              case ESC_s:
              case ESC_S:
              *parsed_pattern++ = PT_SPACE << 16;
              break;

              case ESC_w:
              case ESC_W:
              *parsed_pattern++ = PT_WORD << 16;
              break;
              }
            }
          break;

          case ESC_P:
          case ESC_p:
#ifdef SUPPORT_UNICODE
            {
            BOOL negated;
            uint16_t ptype = 0, pdata = 0;
            if (!get_ucp(&ptr, &negated, &ptype, &pdata, &errorcode, cb))
              goto FAILED;
            if (negated) escape = (escape == ESC_P)? ESC_p : ESC_P;
            *parsed_pattern++ = META_ESCAPE + escape;
            *parsed_pattern++ = (ptype << 16) | pdata;
            }
#else
          errorcode = ERR45;
          goto FAILED;
#endif
          break;  /* End \P and \p */

          default:    /* All others are not allowed in a class */
          errorcode = ERR7;
          ptr--;
          goto FAILED;
          }

        if (ptr < ptrend - 1 && *ptr == CHAR_MINUS &&
            ptr[1] != CHAR_RIGHT_SQUARE_BRACKET)
          {
          errorcode = ERR50;
          goto FAILED;
          }
        }

      CLASS_CONTINUE:
      if (ptr >= ptrend)
        {
        errorcode = ERR6;  /* Missing terminating ']' */
        goto FAILED;
        }
      GETCHARINCTEST(c, ptr);
      if (c == CHAR_RIGHT_SQUARE_BRACKET && !inescq) break;
      }     /* End of class-processing loop */

    if (class_range_state == RANGE_STARTED)
      {
      parsed_pattern[-1] = CHAR_MINUS;
      class_range_state = RANGE_NO;
      }

    *parsed_pattern++ = META_CLASS_END;
    break;  /* End of character class */

    case CHAR_LEFT_PARENTHESIS:
    if (ptr >= ptrend) goto UNCLOSED_PARENTHESIS;

    if (*ptr != CHAR_QUESTION_MARK)
      {
      const char *vn;

      if (*ptr != CHAR_ASTERISK)
        {
        nest_depth++;
        if ((options & PCRE2_NO_AUTO_CAPTURE) == 0)
          {
          if (cb->bracount >= MAX_GROUP_NUMBER)
            {
            errorcode = ERR97;
            goto FAILED;
            }
          cb->bracount++;
          *parsed_pattern++ = META_CAPTURE | cb->bracount;
          }
        else *parsed_pattern++ = META_NOCAPTURE;
        }

      else if (ptrend - ptr <= 1 || (c = ptr[1]) == CHAR_RIGHT_PARENTHESIS)
        break;

      else if (CHMAX_255(c) && (cb->ctypes[c] & ctype_lcletter) != 0)
        {
        uint32_t meta;

        vn = alasnames;
        if (!read_name(&ptr, ptrend, utf, 0, &offset, &name, &namelen,
          &errorcode, cb)) goto FAILED;
        if (ptr >= ptrend || *ptr != CHAR_COLON)
          {
          errorcode = ERR95;  /* Malformed */
          goto FAILED;
          }

        for (i = 0; i < alascount; i++)
          {
          if (namelen == alasmeta[i].len &&
              PRIV(strncmp_c8)(name, vn, namelen) == 0)
            break;
          vn += alasmeta[i].len + 1;
          }

        if (i >= alascount)
          {
          errorcode = ERR95;  /* Alpha assertion not recognized */
          goto FAILED;
          }

        meta = alasmeta[i].meta;
        if (prev_expect_cond_assert > 0 &&
            (meta < META_LOOKAHEAD || meta > META_LOOKBEHINDNOT))
          {
          errorcode = (meta == META_LOOKAHEAD_NA || meta == META_LOOKBEHIND_NA)?
            ERR98 : ERR28;  /* (Atomic) assertion expected */
          goto FAILED;
          }

        switch(meta)
          {
          default:
          errorcode = ERR89;  /* Unknown code; should never occur because */
          goto FAILED;        /* the meta values come from a table above. */

          case META_ATOMIC:
          goto ATOMIC_GROUP;

          case META_LOOKAHEAD:
          goto POSITIVE_LOOK_AHEAD;

          case META_LOOKAHEAD_NA:
          goto POSITIVE_NONATOMIC_LOOK_AHEAD;

          case META_LOOKAHEADNOT:
          goto NEGATIVE_LOOK_AHEAD;

          case META_LOOKBEHIND:
          case META_LOOKBEHINDNOT:
          case META_LOOKBEHIND_NA:
          *parsed_pattern++ = meta;
          ptr--;
          goto POST_LOOKBEHIND;

          case META_SCRIPT_RUN:
          case META_ATOMIC_SCRIPT_RUN:
#ifdef SUPPORT_UNICODE
          *parsed_pattern++ = META_SCRIPT_RUN;
          nest_depth++;
          ptr++;
          if (meta == META_ATOMIC_SCRIPT_RUN)
            {
            *parsed_pattern++ = META_ATOMIC;
            if (top_nest == NULL) top_nest = (nest_save *)(cb->start_workspace);
            else if (++top_nest >= end_nests)
              {
              errorcode = ERR84;
              goto FAILED;
              }
            top_nest->nest_depth = nest_depth;
            top_nest->flags = NSF_ATOMICSR;
            top_nest->options = options & PARSE_TRACKED_OPTIONS;
            }
          break;
#else  /* SUPPORT_UNICODE */
          errorcode = ERR96;
          goto FAILED;
#endif
          }
        }

      else
        {
        vn = verbnames;
        if (!read_name(&ptr, ptrend, utf, 0, &offset, &name, &namelen,
          &errorcode, cb)) goto FAILED;
        if (ptr >= ptrend || (*ptr != CHAR_COLON &&
                              *ptr != CHAR_RIGHT_PARENTHESIS))
          {
          errorcode = ERR60;  /* Malformed */
          goto FAILED;
          }

        for (i = 0; i < verbcount; i++)
          {
          if (namelen == verbs[i].len &&
              PRIV(strncmp_c8)(name, vn, namelen) == 0)
            break;
          vn += verbs[i].len + 1;
          }

        if (i >= verbcount)
          {
          errorcode = ERR60;  /* Verb not recognized */
          goto FAILED;
          }

        if (*ptr == CHAR_COLON && ptr + 1 < ptrend &&
             ptr[1] == CHAR_RIGHT_PARENTHESIS)
          ptr++;    /* Advance to the closing parens */

        if (verbs[i].has_arg > 0 && *ptr != CHAR_COLON)
          {
          errorcode = ERR66;
          goto FAILED;
          }

        verbstartptr = parsed_pattern;
        okquantifier = (verbs[i].meta == META_ACCEPT);

        if (*ptr++ == CHAR_COLON)   /* Skip past : or ) */
          {

          if (verbs[i].has_arg < 0)
            {
            add_after_mark = verbs[i].meta;
            *parsed_pattern++ = META_MARK;
            }

          else
            {
            *parsed_pattern++ = verbs[i].meta +
              ((verbs[i].meta != META_MARK)? 0x00010000u:0);
            }

          verblengthptr = parsed_pattern++;
          verbnamestart = ptr;
          inverbname = TRUE;
          }
        else  /* No verb "name" argument */
          {
          *parsed_pattern++ = verbs[i].meta;
          }
        }     /* End of (*VERB) handling */
      break;  /* Done with this parenthesis */
      }       /* End of groups that don't start with (? */

    if (++ptr >= ptrend) goto UNCLOSED_PARENTHESIS;

    switch(*ptr)
      {
      default:
      if (*ptr == CHAR_MINUS && ptrend - ptr > 1 && IS_DIGIT(ptr[1]))
        goto RECURSION_BYNUMBER;  /* The + case is handled by CHAR_PLUS */

      nest_depth++;
      if (top_nest == NULL) top_nest = (nest_save *)(cb->start_workspace);
      else if (++top_nest >= end_nests)
        {
        errorcode = ERR84;
        goto FAILED;
        }
      top_nest->nest_depth = nest_depth;
      top_nest->flags = 0;
      top_nest->options = options & PARSE_TRACKED_OPTIONS;

      if (*ptr == CHAR_VERTICAL_LINE)
        {
        top_nest->reset_group = (uint16_t)cb->bracount;
        top_nest->max_group = (uint16_t)cb->bracount;
        top_nest->flags |= NSF_RESET;
        cb->external_flags |= PCRE2_DUPCAPUSED;
        *parsed_pattern++ = META_NOCAPTURE;
        ptr++;
        }

      else
        {
        BOOL hyphenok = TRUE;
        uint32_t oldoptions = options;

        top_nest->reset_group = 0;
        top_nest->max_group = 0;
        set = unset = 0;
        optset = &set;

        if (ptr < ptrend && *ptr == CHAR_CIRCUMFLEX_ACCENT)
          {
          options &= ~(PCRE2_CASELESS|PCRE2_MULTILINE|PCRE2_NO_AUTO_CAPTURE|
                       PCRE2_DOTALL|PCRE2_EXTENDED|PCRE2_EXTENDED_MORE);
          hyphenok = FALSE;
          ptr++;
          }

        while (ptr < ptrend && *ptr != CHAR_RIGHT_PARENTHESIS &&
                               *ptr != CHAR_COLON)
          {
          switch (*ptr++)
            {
            case CHAR_MINUS:
            if (!hyphenok)
              {
              errorcode = ERR94;
              ptr--;  /* Correct the offset */
              goto FAILED;
              }
            optset = &unset;
            hyphenok = FALSE;
            break;

            case CHAR_J:  /* Record that it changed in the external options */
            *optset |= PCRE2_DUPNAMES;
            cb->external_flags |= PCRE2_JCHANGED;
            break;

            case CHAR_i: *optset |= PCRE2_CASELESS; break;
            case CHAR_m: *optset |= PCRE2_MULTILINE; break;
            case CHAR_n: *optset |= PCRE2_NO_AUTO_CAPTURE; break;
            case CHAR_s: *optset |= PCRE2_DOTALL; break;
            case CHAR_U: *optset |= PCRE2_UNGREEDY; break;

            case CHAR_x:
            *optset |= PCRE2_EXTENDED;
            if (ptr < ptrend && *ptr == CHAR_x)
              {
              *optset |= PCRE2_EXTENDED_MORE;
              ptr++;
              }
            break;

            default:
            errorcode = ERR11;
            ptr--;    /* Correct the offset */
            goto FAILED;
            }
          }

        if ((set & (PCRE2_EXTENDED|PCRE2_EXTENDED_MORE)) == PCRE2_EXTENDED ||
            (unset & PCRE2_EXTENDED) != 0)
          unset |= PCRE2_EXTENDED_MORE;

        options = (options | set) & (~unset);

        if (ptr >= ptrend) goto UNCLOSED_PARENTHESIS;
        if (*ptr++ == CHAR_RIGHT_PARENTHESIS)
          {
          nest_depth--;  /* This is not a nested group after all. */
          if (top_nest > (nest_save *)(cb->start_workspace) &&
              (top_nest-1)->nest_depth == nest_depth) top_nest--;
          else top_nest->nest_depth = nest_depth;
          }
        else *parsed_pattern++ = META_NOCAPTURE;

        if (options != oldoptions)
          {
          *parsed_pattern++ = META_OPTIONS;
          *parsed_pattern++ = options;
          }
        }     /* End options processing */
      break;  /* End default case after (? */

      case CHAR_P:
      if (++ptr >= ptrend) goto UNCLOSED_PARENTHESIS;

      if (*ptr == CHAR_LESS_THAN_SIGN)
        {
        terminator = CHAR_GREATER_THAN_SIGN;
        goto DEFINE_NAME;
        }

      if (*ptr == CHAR_GREATER_THAN_SIGN) goto RECURSE_BY_NAME;

      if (*ptr != CHAR_EQUALS_SIGN)
        {
        errorcode = ERR41;
        goto FAILED;
        }
      if (!read_name(&ptr, ptrend, utf, CHAR_RIGHT_PARENTHESIS, &offset, &name,
          &namelen, &errorcode, cb)) goto FAILED;
      *parsed_pattern++ = META_BACKREF_BYNAME;
      *parsed_pattern++ = namelen;
      PUTOFFSET(offset, parsed_pattern);
      okquantifier = TRUE;
      break;   /* End of (?P processing */

      case CHAR_R:
      i = 0;         /* (?R) == (?R0) */
      ptr++;
      if (ptr >= ptrend || *ptr != CHAR_RIGHT_PARENTHESIS)
        {
        errorcode = ERR58;
        goto FAILED;
        }
      goto SET_RECURSION;

      case CHAR_PLUS:
      if (ptrend - ptr < 2 || !IS_DIGIT(ptr[1]))
        {
        errorcode = ERR29;   /* Missing number */
        goto FAILED;
        }

      case CHAR_0: case CHAR_1: case CHAR_2: case CHAR_3: case CHAR_4:
      case CHAR_5: case CHAR_6: case CHAR_7: case CHAR_8: case CHAR_9:
      RECURSION_BYNUMBER:
      if (!read_number(&ptr, ptrend,
          (IS_DIGIT(*ptr))? -1:(int)(cb->bracount), /* + and - are relative */
          MAX_GROUP_NUMBER, ERR61,
          &i, &errorcode)) goto FAILED;
      if (i < 0)  /* NB (?0) is permitted */
        {
        errorcode = ERR15;   /* Unknown group */
        goto FAILED_BACK;
        }
      if (ptr >= ptrend || *ptr != CHAR_RIGHT_PARENTHESIS)
        goto UNCLOSED_PARENTHESIS;

      SET_RECURSION:
      *parsed_pattern++ = META_RECURSE | (uint32_t)i;
      offset = (PCRE2_SIZE)(ptr - cb->start_pattern);
      ptr++;
      PUTOFFSET(offset, parsed_pattern);
      okquantifier = TRUE;
      break;  /* End of recursive call by number handling */

      case CHAR_AMPERSAND:
      RECURSE_BY_NAME:
      if (!read_name(&ptr, ptrend, utf, CHAR_RIGHT_PARENTHESIS, &offset, &name,
          &namelen, &errorcode, cb)) goto FAILED;
      *parsed_pattern++ = META_RECURSE_BYNAME;
      *parsed_pattern++ = namelen;
      PUTOFFSET(offset, parsed_pattern);
      okquantifier = TRUE;
      break;

      case CHAR_C:
      if (++ptr >= ptrend) goto UNCLOSED_PARENTHESIS;

      expect_cond_assert = prev_expect_cond_assert - 1;

      if (previous_callout != NULL && (options & PCRE2_AUTO_CALLOUT) != 0 &&
          previous_callout == parsed_pattern - 4 &&
          parsed_pattern[-1] == 255)
        parsed_pattern = previous_callout;

      previous_callout = parsed_pattern;
      after_manual_callout = 1;

      if (*ptr != CHAR_RIGHT_PARENTHESIS && !IS_DIGIT(*ptr))
        {
        PCRE2_SIZE calloutlength;
        PCRE2_SPTR startptr = ptr;

        delimiter = 0;
        for (i = 0; PRIV(callout_start_delims)[i] != 0; i++)
          {
          if (*ptr == PRIV(callout_start_delims)[i])
            {
            delimiter = PRIV(callout_end_delims)[i];
            break;
            }
          }
        if (delimiter == 0)
          {
          errorcode = ERR82;
          goto FAILED;
          }

        *parsed_pattern = META_CALLOUT_STRING;
        parsed_pattern += 3;   /* Skip pattern info */

        for (;;)
          {
          if (++ptr >= ptrend)
            {
            errorcode = ERR81;
            ptr = startptr;   /* To give a more useful message */
            goto FAILED;
            }
          if (*ptr == delimiter && (++ptr >= ptrend || *ptr != delimiter))
            break;
          }

        calloutlength = (PCRE2_SIZE)(ptr - startptr);
        if (calloutlength > UINT32_MAX)
          {
          errorcode = ERR72;
          goto FAILED;
          }
        *parsed_pattern++ = (uint32_t)calloutlength;
        offset = (PCRE2_SIZE)(startptr - cb->start_pattern);
        PUTOFFSET(offset, parsed_pattern);
        }

      else
        {
        int n = 0;
        *parsed_pattern = META_CALLOUT_NUMBER;     /* Numerical callout */
        parsed_pattern += 3;                       /* Skip pattern info */
        while (ptr < ptrend && IS_DIGIT(*ptr))
          {
          n = n * 10 + *ptr++ - CHAR_0;
          if (n > 255)
            {
            errorcode = ERR38;
            goto FAILED;
            }
          }
        *parsed_pattern++ = n;
        }

      if (ptr >= ptrend || *ptr != CHAR_RIGHT_PARENTHESIS)
        {
        errorcode = ERR39;
        goto FAILED;
        }
      ptr++;

      previous_callout[1] = (uint32_t)(ptr - cb->start_pattern);
      previous_callout[2] = 0;
      break;                  /* End callout */

      case CHAR_LEFT_PARENTHESIS:
      if (++ptr >= ptrend) goto UNCLOSED_PARENTHESIS;
      nest_depth++;

      if (*ptr == CHAR_QUESTION_MARK || *ptr == CHAR_ASTERISK)
        {
        *parsed_pattern++ = META_COND_ASSERT;
        ptr--;   /* Pull pointer back to the opening parenthesis. */
        expect_cond_assert = 2;
        break;  /* End of conditional */
        }

      if (read_number(&ptr, ptrend, cb->bracount, MAX_GROUP_NUMBER, ERR61, &i,
          &errorcode))
        {
        if (i <= 0)
          {
          errorcode = ERR15;
          goto FAILED;
          }
        *parsed_pattern++ = META_COND_NUMBER;
        offset = (PCRE2_SIZE)(ptr - cb->start_pattern - 2);
        PUTOFFSET(offset, parsed_pattern);
        *parsed_pattern++ = i;
        }
      else if (errorcode != 0) goto FAILED;   /* Number too big */

      else if (ptrend - ptr >= 10 &&
               PRIV(strncmp_c8)(ptr, STRING_VERSION, 7) == 0 &&
               ptr[7] != CHAR_RIGHT_PARENTHESIS)
        {
        uint32_t ge = 0;
        int major = 0;
        int minor = 0;

        ptr += 7;
        if (*ptr == CHAR_GREATER_THAN_SIGN)
          {
          ge = 1;
          ptr++;
          }

        if (*ptr != CHAR_EQUALS_SIGN || (ptr++, !IS_DIGIT(*ptr)))
          goto BAD_VERSION_CONDITION;

        if (!read_number(&ptr, ptrend, -1, 1000, ERR79, &major, &errorcode))
          goto FAILED;

        if (ptr >= ptrend) goto BAD_VERSION_CONDITION;
        if (*ptr == CHAR_DOT)
          {
          if (++ptr >= ptrend || !IS_DIGIT(*ptr)) goto BAD_VERSION_CONDITION;
          minor = (*ptr++ - CHAR_0) * 10;
          if (IS_DIGIT(*ptr)) minor += *ptr++ - CHAR_0;
          if (ptr >= ptrend || *ptr != CHAR_RIGHT_PARENTHESIS)
            goto BAD_VERSION_CONDITION;
          }

        *parsed_pattern++ = META_COND_VERSION;
        *parsed_pattern++ = ge;
        *parsed_pattern++ = major;
        *parsed_pattern++ = minor;
        }

      else
        {
        BOOL was_r_ampersand = FALSE;

        if (*ptr == CHAR_R && ptrend - ptr > 1 && ptr[1] == CHAR_AMPERSAND)
          {
          terminator = CHAR_RIGHT_PARENTHESIS;
          was_r_ampersand = TRUE;
          ptr++;
          }
        else if (*ptr == CHAR_LESS_THAN_SIGN)
          terminator = CHAR_GREATER_THAN_SIGN;
        else if (*ptr == CHAR_APOSTROPHE)
          terminator = CHAR_APOSTROPHE;
        else
          {
          terminator = CHAR_RIGHT_PARENTHESIS;
          ptr--;   /* Point to char before name */
          }
        if (!read_name(&ptr, ptrend, utf, terminator, &offset, &name, &namelen,
            &errorcode, cb)) goto FAILED;

        if (was_r_ampersand)
          {
          *parsed_pattern = META_COND_RNAME;
          ptr--;   /* Back to closing parens */
          }

        else if (terminator == CHAR_RIGHT_PARENTHESIS)
          {
          if (namelen == 6 && PRIV(strncmp_c8)(name, STRING_DEFINE, 6) == 0)
            *parsed_pattern = META_COND_DEFINE;
          else
            {
            for (i = 1; i < (int)namelen; i++)
              if (!IS_DIGIT(name[i])) break;
            *parsed_pattern = (*name == CHAR_R && i >= (int)namelen)?
              META_COND_RNUMBER : META_COND_NAME;
            }
          ptr--;   /* Back to closing parens */
          }

        else *parsed_pattern = META_COND_NAME;

        if (*parsed_pattern++ != META_COND_DEFINE) *parsed_pattern++ = namelen;
        PUTOFFSET(offset, parsed_pattern);
        }  /* End cases that read a name */

      if (ptr >= ptrend || *ptr != CHAR_RIGHT_PARENTHESIS)
        {
        errorcode = ERR24;
        goto FAILED;
        }
      ptr++;
      break;  /* End of condition processing */

      case CHAR_GREATER_THAN_SIGN:
      ATOMIC_GROUP:                          /* Come from (*atomic: */
      *parsed_pattern++ = META_ATOMIC;
      nest_depth++;
      ptr++;
      break;

      case CHAR_EQUALS_SIGN:
      POSITIVE_LOOK_AHEAD:                   /* Come from (*pla: */
      *parsed_pattern++ = META_LOOKAHEAD;
      ptr++;
      goto POST_ASSERTION;

      case CHAR_ASTERISK:
      POSITIVE_NONATOMIC_LOOK_AHEAD:         /* Come from (?* */
      *parsed_pattern++ = META_LOOKAHEAD_NA;
      ptr++;
      goto POST_ASSERTION;

      case CHAR_EXCLAMATION_MARK:
      NEGATIVE_LOOK_AHEAD:                   /* Come from (*nla: */
      *parsed_pattern++ = META_LOOKAHEADNOT;
      ptr++;
      goto POST_ASSERTION;

      case CHAR_LESS_THAN_SIGN:
      if (ptrend - ptr <= 1 ||
         (ptr[1] != CHAR_EQUALS_SIGN &&
          ptr[1] != CHAR_EXCLAMATION_MARK &&
          ptr[1] != CHAR_ASTERISK))
        {
        terminator = CHAR_GREATER_THAN_SIGN;
        goto DEFINE_NAME;
        }
      *parsed_pattern++ = (ptr[1] == CHAR_EQUALS_SIGN)?
        META_LOOKBEHIND : (ptr[1] == CHAR_EXCLAMATION_MARK)?
        META_LOOKBEHINDNOT : META_LOOKBEHIND_NA;

      POST_LOOKBEHIND:           /* Come from (*plb: (*naplb: and (*nlb: */
      *has_lookbehind = TRUE;
      offset = (PCRE2_SIZE)(ptr - cb->start_pattern - 2);
      PUTOFFSET(offset, parsed_pattern);
      ptr += 2;

      POST_ASSERTION:
      nest_depth++;
      if (prev_expect_cond_assert > 0)
        {
        if (top_nest == NULL) top_nest = (nest_save *)(cb->start_workspace);
        else if (++top_nest >= end_nests)
          {
          errorcode = ERR84;
          goto FAILED;
          }
        top_nest->nest_depth = nest_depth;
        top_nest->flags = NSF_CONDASSERT;
        top_nest->options = options & PARSE_TRACKED_OPTIONS;
        }
      break;

      case CHAR_APOSTROPHE:
      terminator = CHAR_APOSTROPHE;    /* Terminator */

      DEFINE_NAME:
      if (!read_name(&ptr, ptrend, utf, terminator, &offset, &name, &namelen,
          &errorcode, cb)) goto FAILED;

      if (cb->bracount >= MAX_GROUP_NUMBER)
        {
        errorcode = ERR97;
        goto FAILED;
        }
      cb->bracount++;
      *parsed_pattern++ = META_CAPTURE | cb->bracount;
      nest_depth++;

      if (cb->names_found >= MAX_NAME_COUNT)
        {
        errorcode = ERR49;
        goto FAILED;
        }

      if (namelen + IMM2_SIZE + 1 > cb->name_entry_size)
        cb->name_entry_size = (uint16_t)(namelen + IMM2_SIZE + 1);

      isdupname = FALSE;
      ng = cb->named_groups;
      for (i = 0; i < cb->names_found; i++, ng++)
        {
        if (namelen == ng->length &&
            PRIV(strncmp)(name, ng->name, (PCRE2_SIZE)namelen) == 0)
          {
          if (ng->number == cb->bracount) break;
          if ((options & PCRE2_DUPNAMES) == 0)
            {
            errorcode = ERR43;
            goto FAILED;
            }
          isdupname = ng->isdup = TRUE;     /* Mark as a duplicate */
          cb->dupnames = TRUE;              /* Duplicate names exist */
          }
        else if (ng->number == cb->bracount)
          {
          errorcode = ERR65;
          goto FAILED;
          }
        }

      if (i < cb->names_found) break;   /* Ignore duplicate with same number */

      if (cb->names_found >= cb->named_group_list_size)
        {
        uint32_t newsize = cb->named_group_list_size * 2;
        named_group *newspace =
          cb->cx->memctl.malloc(newsize * sizeof(named_group),
          cb->cx->memctl.memory_data);
        if (newspace == NULL)
          {
          errorcode = ERR21;
          goto FAILED;
          }

        memcpy(newspace, cb->named_groups,
          cb->named_group_list_size * sizeof(named_group));
        if (cb->named_group_list_size > NAMED_GROUP_LIST_SIZE)
          cb->cx->memctl.free((void *)cb->named_groups,
          cb->cx->memctl.memory_data);
        cb->named_groups = newspace;
        cb->named_group_list_size = newsize;
        }

      cb->named_groups[cb->names_found].name = name;
      cb->named_groups[cb->names_found].length = (uint16_t)namelen;
      cb->named_groups[cb->names_found].number = cb->bracount;
      cb->named_groups[cb->names_found].isdup = (uint16_t)isdupname;
      cb->names_found++;
      break;
      }        /* End of (? switch */
    break;     /* End of ( handling */

    case CHAR_VERTICAL_LINE:
    if (top_nest != NULL && top_nest->nest_depth == nest_depth &&
        (top_nest->flags & NSF_RESET) != 0)
      {
      if (cb->bracount > top_nest->max_group)
        top_nest->max_group = (uint16_t)cb->bracount;
      cb->bracount = top_nest->reset_group;
      }
    *parsed_pattern++ = META_ALT;
    break;

    case CHAR_RIGHT_PARENTHESIS:
    okquantifier = TRUE;
    if (top_nest != NULL && top_nest->nest_depth == nest_depth)
      {
      options = (options & ~PARSE_TRACKED_OPTIONS) | top_nest->options;
      if ((top_nest->flags & NSF_RESET) != 0 &&
          top_nest->max_group > cb->bracount)
        cb->bracount = top_nest->max_group;
      if ((top_nest->flags & NSF_CONDASSERT) != 0)
        okquantifier = FALSE;

      if ((top_nest->flags & NSF_ATOMICSR) != 0)
        {
        *parsed_pattern++ = META_KET;
        }

      if (top_nest == (nest_save *)(cb->start_workspace)) top_nest = NULL;
        else top_nest--;
      }
    if (nest_depth == 0)    /* Unmatched closing parenthesis */
      {
      errorcode = ERR22;
      goto FAILED_BACK;
      }
    nest_depth--;
    *parsed_pattern++ = META_KET;
    break;
    }  /* End of switch on pattern character */
  }    /* End of main character scan loop */

if (inverbname && ptr >= ptrend)
  {
  errorcode = ERR60;
  goto FAILED;
  }

PARSED_END:
parsed_pattern = manage_callouts(ptr, &previous_callout, auto_callout,
  parsed_pattern, cb);

if ((extra_options & PCRE2_EXTRA_MATCH_LINE) != 0)
  {
  *parsed_pattern++ = META_KET;
  *parsed_pattern++ = META_DOLLAR;
  }
else if ((extra_options & PCRE2_EXTRA_MATCH_WORD) != 0)
  {
  *parsed_pattern++ = META_KET;
  *parsed_pattern++ = META_ESCAPE + ESC_b;
  }

if (parsed_pattern >= parsed_pattern_end)
  {
  errorcode = ERR63;  /* Internal error (parsed pattern overflow) */
  goto FAILED;
  }

*parsed_pattern = META_END;
if (nest_depth == 0) return 0;

UNCLOSED_PARENTHESIS:
errorcode = ERR14;

FAILED:
cb->erroroffset = (PCRE2_SIZE)(ptr - cb->start_pattern);
return errorcode;

FAILED_BACK:
ptr--;
goto FAILED;

BAD_VERSION_CONDITION:
errorcode = ERR79;
goto FAILED;
}

/*************************************************
*       Find first significant opcode            *
*************************************************/

static const PCRE2_UCHAR*
first_significant_code(PCRE2_SPTR code, BOOL skipassert)
{
for (;;)
  {
  switch ((int)*code)
    {
    case OP_ASSERT_NOT:
    case OP_ASSERTBACK:
    case OP_ASSERTBACK_NOT:
    case OP_ASSERTBACK_NA:
    if (!skipassert) return code;
    do code += GET(code, 1); while (*code == OP_ALT);
    code += PRIV(OP_lengths)[*code];
    break;

    case OP_WORD_BOUNDARY:
    case OP_NOT_WORD_BOUNDARY:
    if (!skipassert) return code;
    /* Fall through */

    case OP_CALLOUT:
    case OP_CREF:
    case OP_DNCREF:
    case OP_RREF:
    case OP_DNRREF:
    case OP_FALSE:
    case OP_TRUE:
    code += PRIV(OP_lengths)[*code];
    break;

    case OP_CALLOUT_STR:
    code += GET(code, 1 + 2*LINK_SIZE);
    break;

    case OP_SKIPZERO:
    code += 2 + GET(code, 2) + LINK_SIZE;
    break;

    case OP_COND:
    case OP_SCOND:
    if (code[1+LINK_SIZE] != OP_FALSE ||   /* Not DEFINE */
        code[GET(code, 1)] != OP_KET)      /* More than one branch */
      return code;
    code += GET(code, 1) + 1 + LINK_SIZE;
    break;

    case OP_MARK:
    case OP_COMMIT_ARG:
    case OP_PRUNE_ARG:
    case OP_SKIP_ARG:
    case OP_THEN_ARG:
    code += code[1] + PRIV(OP_lengths)[*code];
    break;

    default:
    return code;
    }
  }
/* Control never reaches here */
}

#ifdef SUPPORT_UNICODE
/*************************************************
*           Get othercase range                  *
*************************************************/

static int
get_othercase_range(uint32_t *cptr, uint32_t d, uint32_t *ocptr,
  uint32_t *odptr)
{
uint32_t c, othercase, next;
unsigned int co;

for (c = *cptr; c <= d; c++)
  {
  if ((co = UCD_CASESET(c)) != 0)
    {
    *ocptr = c++;   /* Character that has the set */
    *cptr = c;      /* Rest of input range */
    return (int)co;
    }
  if ((othercase = UCD_OTHERCASE(c)) != c) break;
  }

if (c > d) return -1;  /* Reached end of range */

*ocptr = othercase;
next = othercase + 1;

for (++c; c <= d; c++)
  {
  if ((co = UCD_CASESET(c)) != 0 || UCD_OTHERCASE(c) != next) break;
  next++;
  }

*odptr = next - 1;     /* End of othercase range */
*cptr = c;             /* Rest of input range */
return 0;
}
#endif  /* SUPPORT_UNICODE */

/*************************************************
* Add a character or range to a class (internal) *
*************************************************/

static unsigned int
add_to_class_internal(uint8_t *classbits, PCRE2_UCHAR **uchardptr,
  uint32_t options, compile_block *cb, uint32_t start, uint32_t end)
{
uint32_t c;
uint32_t classbits_end = (end <= 0xff ? end : 0xff);
unsigned int n8 = 0;

if ((options & PCRE2_CASELESS) != 0)
  {
#ifdef SUPPORT_UNICODE
  if ((options & (PCRE2_UTF|PCRE2_UCP)) != 0)
    {
    int rc;
    uint32_t oc, od;

    options &= ~PCRE2_CASELESS;   /* Remove for recursive calls */
    c = start;

    while ((rc = get_othercase_range(&c, end, &oc, &od)) >= 0)
      {

      if (rc > 0) n8 += add_list_to_class_internal(classbits, uchardptr, options, cb,
        PRIV(ucd_caseless_sets) + rc, oc);

      else if (oc >= cb->class_range_start && od <= cb->class_range_end) continue;

      else if (oc < start && od >= start - 1) start = oc; /* Extend downwards */
      else if (od > end && oc <= end + 1)
        {
        end = od;       /* Extend upwards */
        if (end > classbits_end) classbits_end = (end <= 0xff ? end : 0xff);
        }
      else n8 += add_to_class_internal(classbits, uchardptr, options, cb, oc, od);
      }
    }
  else
#endif  /* SUPPORT_UNICODE */

  for (c = start; c <= classbits_end; c++)
    {
    SETBIT(classbits, cb->fcc[c]);
    n8++;
    }
  }

if ((options & PCRE2_UTF) == 0 && end > MAX_NON_UTF_CHAR)
  end = MAX_NON_UTF_CHAR;

if (start > cb->class_range_start && end < cb->class_range_end) return n8;

for (c = start; c <= classbits_end; c++)
  {
  /* Regardless of start, c will always be <= 255. */
  SETBIT(classbits, c);
  n8++;
  }

#ifdef SUPPORT_WIDE_CHARS
if (start <= 0xff) start = 0xff + 1;

if (end >= start)
  {
  PCRE2_UCHAR *uchardata = *uchardptr;

#ifdef SUPPORT_UNICODE
  if ((options & PCRE2_UTF) != 0)
    {
    if (start < end)
      {
      *uchardata++ = XCL_RANGE;
      uchardata += PRIV(ord2utf)(start, uchardata);
      uchardata += PRIV(ord2utf)(end, uchardata);
      }
    else if (start == end)
      {
      *uchardata++ = XCL_SINGLE;
      uchardata += PRIV(ord2utf)(start, uchardata);
      }
    }
  else
#endif  /* SUPPORT_UNICODE */

#if PCRE2_CODE_UNIT_WIDTH == 8
    {}
#else
  if (start < end)
    {
    *uchardata++ = XCL_RANGE;
    *uchardata++ = start;
    *uchardata++ = end;
    }
  else if (start == end)
    {
    *uchardata++ = XCL_SINGLE;
    *uchardata++ = start;
    }
#endif  /* PCRE2_CODE_UNIT_WIDTH == 8 */
  *uchardptr = uchardata;   /* Updata extra data pointer */
  }
#else  /* SUPPORT_WIDE_CHARS */
  (void)uchardptr;          /* Avoid compiler warning */
#endif /* SUPPORT_WIDE_CHARS */

return n8;    /* Number of 8-bit characters */
}

#ifdef SUPPORT_UNICODE
/*************************************************
* Add a list of characters to a class (internal) *
*************************************************/

static unsigned int
add_list_to_class_internal(uint8_t *classbits, PCRE2_UCHAR **uchardptr,
  uint32_t options, compile_block *cb, const uint32_t *p, unsigned int except)
{
unsigned int n8 = 0;
while (p[0] < NOTACHAR)
  {
  unsigned int n = 0;
  if (p[0] != except)
    {
    while(p[n+1] == p[0] + n + 1) n++;
    n8 += add_to_class_internal(classbits, uchardptr, options, cb, p[0], p[n]);
    }
  p += n + 1;
  }
return n8;
}
#endif

/*************************************************
*   External entry point for add range to class  *
*************************************************/

static unsigned int
add_to_class(uint8_t *classbits, PCRE2_UCHAR **uchardptr, uint32_t options,
  compile_block *cb, uint32_t start, uint32_t end)
{
cb->class_range_start = start;
cb->class_range_end = end;
return add_to_class_internal(classbits, uchardptr, options, cb, start, end);
}

/*************************************************
*   External entry point for add list to class   *
*************************************************/

static unsigned int
add_list_to_class(uint8_t *classbits, PCRE2_UCHAR **uchardptr, uint32_t options,
  compile_block *cb, const uint32_t *p, unsigned int except)
{
unsigned int n8 = 0;
while (p[0] < NOTACHAR)
  {
  unsigned int n = 0;
  if (p[0] != except)
    {
    while(p[n+1] == p[0] + n + 1) n++;
    cb->class_range_start = p[0];
    cb->class_range_end = p[n];
    n8 += add_to_class_internal(classbits, uchardptr, options, cb, p[0], p[n]);
    }
  p += n + 1;
  }
return n8;
}

/*************************************************
*    Add characters not in a list to a class     *
*************************************************/

static unsigned int
add_not_list_to_class(uint8_t *classbits, PCRE2_UCHAR **uchardptr,
  uint32_t options, compile_block *cb, const uint32_t *p)
{
BOOL utf = (options & PCRE2_UTF) != 0;
unsigned int n8 = 0;
if (p[0] > 0)
  n8 += add_to_class(classbits, uchardptr, options, cb, 0, p[0] - 1);
while (p[0] < NOTACHAR)
  {
  while (p[1] == p[0] + 1) p++;
  n8 += add_to_class(classbits, uchardptr, options, cb, p[0] + 1,
    (p[1] == NOTACHAR) ? (utf ? 0x10ffffu : 0xffffffffu) : p[1] - 1);
  p++;
  }
return n8;
}

/*************************************************
*    Find details of duplicate group names       *
*************************************************/

static BOOL
find_dupname_details(PCRE2_SPTR name, uint32_t length, int *indexptr,
  int *countptr, int *errorcodeptr, compile_block *cb)
{
uint32_t i, groupnumber;
int count;
PCRE2_UCHAR *slot = cb->name_table;

for (i = 0; i < cb->names_found; i++)
  {
  if (PRIV(strncmp)(name, slot+IMM2_SIZE, length) == 0 &&
      slot[IMM2_SIZE+length] == 0) break;
  slot += cb->name_entry_size;
  }

if (i >= cb->names_found)
  {
  *errorcodeptr = ERR53;
  cb->erroroffset = name - cb->start_pattern;
  return FALSE;
  }

*indexptr = i;
count = 0;

for (;;)
  {
  count++;
  groupnumber = GET2(slot,0);
  cb->backref_map |= (groupnumber < 32)? (1u << groupnumber) : 1;
  if (groupnumber > cb->top_backref) cb->top_backref = groupnumber;
  if (++i >= cb->names_found) break;
  slot += cb->name_entry_size;
  if (PRIV(strncmp)(name, slot+IMM2_SIZE, length) != 0 ||
    (slot+IMM2_SIZE)[length] != 0) break;
  }

*countptr = count;
return TRUE;
}

/*************************************************
*           Compile one branch                   *
*************************************************/

static int
compile_branch(uint32_t *optionsptr, PCRE2_UCHAR **codeptr, uint32_t **pptrptr,
  int *errorcodeptr, uint32_t *firstcuptr, int32_t *firstcuflagsptr,
  uint32_t *reqcuptr, int32_t *reqcuflagsptr, branch_chain *bcptr,
  compile_block *cb, PCRE2_SIZE *lengthptr)
{
int bravalue = 0;
int okreturn = -1;
int group_return = 0;
uint32_t repeat_min = 0, repeat_max = 0;      /* To please picky compilers */
uint32_t greedy_default, greedy_non_default;
uint32_t repeat_type, op_type;
uint32_t options = *optionsptr;               /* May change dynamically */
uint32_t firstcu, reqcu;
uint32_t zeroreqcu, zerofirstcu;
uint32_t escape;
uint32_t *pptr = *pptrptr;
uint32_t meta, meta_arg;
int32_t firstcuflags, reqcuflags;
int32_t zeroreqcuflags, zerofirstcuflags;
int32_t req_caseopt, reqvary, tempreqvary;
PCRE2_SIZE offset = 0;
PCRE2_SIZE length_prevgroup = 0;
PCRE2_UCHAR *code = *codeptr;
PCRE2_UCHAR *last_code = code;
PCRE2_UCHAR *orig_code = code;
PCRE2_UCHAR *tempcode;
PCRE2_UCHAR *previous = NULL;
PCRE2_UCHAR op_previous;
BOOL groupsetfirstcu = FALSE;
BOOL had_accept = FALSE;
BOOL matched_char = FALSE;
BOOL previous_matched_char = FALSE;
BOOL reset_caseful = FALSE;
const uint8_t *cbits = cb->cbits;
uint8_t classbits[32];

#ifdef SUPPORT_UNICODE
BOOL utf = (options & PCRE2_UTF) != 0;
BOOL ucp = (options & PCRE2_UCP) != 0;
#else  /* No Unicode support */
BOOL utf = FALSE;
#endif

PCRE2_UCHAR *class_uchardata;
#ifdef SUPPORT_WIDE_CHARS
BOOL xclass;
PCRE2_UCHAR *class_uchardata_base;
#endif

greedy_default = ((options & PCRE2_UNGREEDY) != 0);
greedy_non_default = greedy_default ^ 1;

firstcu = reqcu = zerofirstcu = zeroreqcu = 0;
firstcuflags = reqcuflags = zerofirstcuflags = zeroreqcuflags = REQ_UNSET;

req_caseopt = ((options & PCRE2_CASELESS) != 0)? REQ_CASELESS:0;

for (;; pptr++)
  {
#ifdef SUPPORT_WIDE_CHARS
  BOOL xclass_has_prop;
#endif
  BOOL negate_class;
  BOOL should_flip_negation;
  BOOL match_all_or_no_wide_chars;
  BOOL possessive_quantifier;
  BOOL note_group_empty;
  int class_has_8bitchar;
  int i;
  uint32_t mclength;
  uint32_t skipunits;
  uint32_t subreqcu, subfirstcu;
  uint32_t groupnumber;
  uint32_t verbarglen, verbculen;
  int32_t subreqcuflags, subfirstcuflags;  /* Must be signed */
  open_capitem *oc;
  PCRE2_UCHAR mcbuffer[8];

  meta = META_CODE(*pptr);
  meta_arg = META_DATA(*pptr);

  if (lengthptr != NULL)
    {
    if (code > cb->start_workspace + cb->workspace_size -
        WORK_SIZE_SAFETY_MARGIN)                       /* Check for overrun */
      {
      *errorcodeptr = (code >= cb->start_workspace + cb->workspace_size)?
        ERR52 : ERR86;
      return 0;
      }

    if (code < last_code) code = last_code;

    if (meta < META_ASTERISK || meta > META_MINMAX_QUERY)
      {
      if (OFLOW_MAX - *lengthptr < (PCRE2_SIZE)(code - orig_code))
        {
        *errorcodeptr = ERR20;   /* Integer overflow */
        return 0;
        }
      *lengthptr += (PCRE2_SIZE)(code - orig_code);
      if (*lengthptr > MAX_PATTERN_SIZE)
        {
        *errorcodeptr = ERR20;   /* Pattern is too large */
        return 0;
        }
      code = orig_code;
      }

    last_code = code;
    }

  if (meta < META_ASTERISK || meta > META_MINMAX_QUERY)
    {
    previous = code;
    if (matched_char && !had_accept) okreturn = 1;
    }

  previous_matched_char = matched_char;
  matched_char = FALSE;
  note_group_empty = FALSE;
  skipunits = 0;         /* Default value for most subgroups */

  switch(meta)
    {

    case META_END:
    case META_ALT:
    case META_KET:
    *firstcuptr = firstcu;
    *firstcuflagsptr = firstcuflags;
    *reqcuptr = reqcu;
    *reqcuflagsptr = reqcuflags;
    *codeptr = code;
    *pptrptr = pptr;
    return okreturn;

    case META_CIRCUMFLEX:
    if ((options & PCRE2_MULTILINE) != 0)
      {
      if (firstcuflags == REQ_UNSET)
        zerofirstcuflags = firstcuflags = REQ_NONE;
      *code++ = OP_CIRCM;
      }
    else *code++ = OP_CIRC;
    break;

    case META_DOLLAR:
    *code++ = ((options & PCRE2_MULTILINE) != 0)? OP_DOLLM : OP_DOLL;
    break;

    case META_DOT:
    matched_char = TRUE;
    if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
    zerofirstcu = firstcu;
    zerofirstcuflags = firstcuflags;
    zeroreqcu = reqcu;
    zeroreqcuflags = reqcuflags;
    *code++ = ((options & PCRE2_DOTALL) != 0)? OP_ALLANY: OP_ANY;
    break;

    case META_CLASS_EMPTY:
    case META_CLASS_EMPTY_NOT:
    matched_char = TRUE;
    *code++ = (meta == META_CLASS_EMPTY_NOT)? OP_ALLANY : OP_FAIL;
    if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
    zerofirstcu = firstcu;
    zerofirstcuflags = firstcuflags;
    break;

    case META_CLASS_NOT:
    case META_CLASS:
    matched_char = TRUE;
    negate_class = meta == META_CLASS_NOT;

    if (pptr[1] < META_END && pptr[2] == META_CLASS_END)
      {
#ifdef SUPPORT_UNICODE
      uint32_t d;
#endif
      uint32_t c = pptr[1];

      pptr += 2;                 /* Move on to class end */
      if (meta == META_CLASS)    /* A positive one-char class can be */
        {                        /* handled as a normal literal character. */
        meta = c;                /* Set up the character */
        goto NORMAL_CHAR_SET;
        }

      zeroreqcu = reqcu;
      zeroreqcuflags = reqcuflags;
      if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
      zerofirstcu = firstcu;
      zerofirstcuflags = firstcuflags;

#ifdef SUPPORT_UNICODE
      if ((utf||ucp) && (options & PCRE2_CASELESS) != 0 &&
          (d = UCD_CASESET(c)) != 0)
        {
        *code++ = OP_NOTPROP;
        *code++ = PT_CLIST;
        *code++ = d;
        break;   /* We are finished with this class */
        }
#endif

      *code++ = ((options & PCRE2_CASELESS) != 0)? OP_NOTI: OP_NOT;
      code += PUTCHAR(c, code);
      break;   /* We are finished with this class */
      }        /* End of 1-char optimization */

    if (meta == META_CLASS && pptr[1] < META_END && pptr[2] < META_END &&
        pptr[3] == META_CLASS_END)
      {
      uint32_t c = pptr[1];

#ifdef SUPPORT_UNICODE
      if (UCD_CASESET(c) == 0)
#endif
        {
        uint32_t d;

#ifdef SUPPORT_UNICODE
        if ((utf || ucp) && c > 127) d = UCD_OTHERCASE(c); else
#endif
          {
#if PCRE2_CODE_UNIT_WIDTH != 8
          if (c > 255) d = c; else
#endif
          d = TABLE_GET(c, cb->fcc, c);
          }

        if (c != d && pptr[2] == d)
          {
          pptr += 3;                 /* Move on to class end */
          meta = c;
          if ((options & PCRE2_CASELESS) == 0)
            {
            reset_caseful = TRUE;
            options |= PCRE2_CASELESS;
            req_caseopt = REQ_CASELESS;
            }
          goto CLASS_CASELESS_CHAR;
          }
        }
      }

    should_flip_negation = match_all_or_no_wide_chars = FALSE;

#ifdef SUPPORT_WIDE_CHARS
    xclass = FALSE;
    class_uchardata = code + LINK_SIZE + 2;   /* For XCLASS items */
    class_uchardata_base = class_uchardata;   /* Save the start */
#endif

    class_has_8bitchar = 0;
#ifdef SUPPORT_WIDE_CHARS
    xclass_has_prop = FALSE;
#endif

    memset(classbits, 0, 32 * sizeof(uint8_t));

    while ((meta = *(++pptr)) != META_CLASS_END)
      {

      if (meta == META_POSIX || meta == META_POSIX_NEG)
        {
        BOOL local_negate = (meta == META_POSIX_NEG);
        int posix_class = *(++pptr);
        int taboffset, tabopt;
        uint8_t pbits[32];

        should_flip_negation = local_negate;  /* Note negative special */

        if ((options & PCRE2_CASELESS) != 0 && posix_class <= 2)
          posix_class = 0;

#ifdef SUPPORT_UNICODE
        if ((options & PCRE2_UCP) != 0) switch(posix_class)
          {
          case PC_GRAPH:
          case PC_PRINT:
          case PC_PUNCT:
          *class_uchardata++ = local_negate? XCL_NOTPROP : XCL_PROP;
          *class_uchardata++ = (PCRE2_UCHAR)
            ((posix_class == PC_GRAPH)? PT_PXGRAPH :
             (posix_class == PC_PRINT)? PT_PXPRINT : PT_PXPUNCT);
          *class_uchardata++ = 0;
          xclass_has_prop = TRUE;
          goto CONTINUE_CLASS;

          default:
#if PCRE2_CODE_UNIT_WIDTH == 8
          if (utf)
#endif
          match_all_or_no_wide_chars |= local_negate;
          break;
          }
#endif  /* SUPPORT_UNICODE */

        posix_class *= 3;

        memcpy(pbits, cbits + posix_class_maps[posix_class],
          32 * sizeof(uint8_t));

        taboffset = posix_class_maps[posix_class + 1];
        tabopt = posix_class_maps[posix_class + 2];

        if (taboffset >= 0)
          {
          if (tabopt >= 0)
            for (i = 0; i < 32; i++) pbits[i] |= cbits[(int)i + taboffset];
          else
            for (i = 0; i < 32; i++) pbits[i] &= ~cbits[(int)i + taboffset];
          }

        if (tabopt < 0) tabopt = -tabopt;
        if (tabopt == 1) pbits[1] &= ~0x3c;
          else if (tabopt == 2) pbits[11] &= 0x7f;

        if (local_negate)
          for (i = 0; i < 32; i++) classbits[i] |= ~pbits[i];
        else
          for (i = 0; i < 32; i++) classbits[i] |= pbits[i];

        class_has_8bitchar = 1;
        goto CONTINUE_CLASS;    /* End of POSIX handling */
        }

      if (meta == META_BIGVALUE)
        {
        meta = *(++pptr);
        goto CLASS_LITERAL;
        }

      if (meta >= META_END)
        {
        if (META_CODE(meta) != META_ESCAPE)
          {
#ifdef DEBUG_SHOW_PARSED
          fprintf(stderr, "** Unrecognized parsed pattern item 0x%.8x "
                          "in character class\n", meta);
#endif
          *errorcodeptr = ERR89;  /* Internal error - unrecognized. */
          return 0;
          }
        escape = META_DATA(meta);

        class_has_8bitchar++;

        switch(escape)
          {
          case ESC_d:
          for (i = 0; i < 32; i++) classbits[i] |= cbits[i+cbit_digit];
          break;

          case ESC_D:
          should_flip_negation = TRUE;
          for (i = 0; i < 32; i++) classbits[i] |= ~cbits[i+cbit_digit];
          break;

          case ESC_w:
          for (i = 0; i < 32; i++) classbits[i] |= cbits[i+cbit_word];
          break;

          case ESC_W:
          should_flip_negation = TRUE;
          for (i = 0; i < 32; i++) classbits[i] |= ~cbits[i+cbit_word];
          break;

          case ESC_s:
          for (i = 0; i < 32; i++) classbits[i] |= cbits[i+cbit_space];
          break;

          case ESC_S:
          should_flip_negation = TRUE;
          for (i = 0; i < 32; i++) classbits[i] |= ~cbits[i+cbit_space];
          break;

          case ESC_h:
          (void)add_list_to_class(classbits, &class_uchardata,
            options & ~PCRE2_CASELESS, cb, PRIV(hspace_list), NOTACHAR);
          break;

          case ESC_H:
          (void)add_not_list_to_class(classbits, &class_uchardata,
            options & ~PCRE2_CASELESS, cb, PRIV(hspace_list));
          break;

          case ESC_v:
          (void)add_list_to_class(classbits, &class_uchardata,
            options & ~PCRE2_CASELESS, cb, PRIV(vspace_list), NOTACHAR);
          break;

          case ESC_V:
          (void)add_not_list_to_class(classbits, &class_uchardata,
            options & ~PCRE2_CASELESS, cb, PRIV(vspace_list));
          break;

#ifdef SUPPORT_UNICODE
          case ESC_p:
          case ESC_P:
            {
            uint32_t ptype = *(++pptr) >> 16;
            uint32_t pdata = *pptr & 0xffff;
            *class_uchardata++ = (escape == ESC_p)? XCL_PROP : XCL_NOTPROP;
            *class_uchardata++ = ptype;
            *class_uchardata++ = pdata;
            xclass_has_prop = TRUE;
            class_has_8bitchar--;                /* Undo! */
            }
          break;
#endif
          }

        goto CONTINUE_CLASS;
        }  /* End handling \d-type escapes */

      else
        {
        uint32_t c, d;

        CLASS_LITERAL:
        c = d = meta;

        if (c == CHAR_CR || c == CHAR_NL) cb->external_flags |= PCRE2_HASCRORLF;

        if (pptr[1] == META_RANGE_LITERAL || pptr[1] == META_RANGE_ESCAPED)
          {
#ifdef EBCDIC
          BOOL range_is_literal = (pptr[1] == META_RANGE_LITERAL);
#endif
          pptr += 2;
          d = *pptr;
          if (d == META_BIGVALUE) d = *(++pptr);

          if (d == CHAR_CR || d == CHAR_NL) cb->external_flags |= PCRE2_HASCRORLF;

#ifdef EBCDIC
          if (range_is_literal &&
               (cb->ctypes[c] & ctype_letter) != 0 &&
               (cb->ctypes[d] & ctype_letter) != 0 &&
               (c <= CHAR_z) == (d <= CHAR_z))
            {
            uint32_t uc = (d <= CHAR_z)? 0 : 64;
            uint32_t C = c - uc;
            uint32_t D = d - uc;

            if (C <= CHAR_i)
              {
              class_has_8bitchar +=
                add_to_class(classbits, &class_uchardata, options, cb, C + uc,
                  ((D < CHAR_i)? D : CHAR_i) + uc);
              C = CHAR_j;
              }

            if (C <= D && C <= CHAR_r)
              {
              class_has_8bitchar +=
                add_to_class(classbits, &class_uchardata, options, cb, C + uc,
                  ((D < CHAR_r)? D : CHAR_r) + uc);
              C = CHAR_s;
              }

            if (C <= D)
              {
              class_has_8bitchar +=
                add_to_class(classbits, &class_uchardata, options, cb, C + uc,
                  D + uc);
              }
            }
          else
#endif

          class_has_8bitchar +=
            add_to_class(classbits, &class_uchardata, options, cb, c, d);
          goto CONTINUE_CLASS;   /* Go get the next char in the class */
          }  /* End of range handling */

        class_has_8bitchar +=
          add_to_class(classbits, &class_uchardata, options, cb, meta, meta);
        }

      CONTINUE_CLASS:

#ifdef SUPPORT_WIDE_CHARS

      if (class_uchardata > class_uchardata_base)
        {
        xclass = TRUE;
        if (lengthptr != NULL)
          {
          *lengthptr += class_uchardata - class_uchardata_base;
          class_uchardata = class_uchardata_base;
          }
        }
#endif

      continue;  /* Needed to avoid error when not supporting wide chars */
      }   /* End of main class-processing loop */

    if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
    zerofirstcu = firstcu;
    zerofirstcuflags = firstcuflags;
    zeroreqcu = reqcu;
    zeroreqcuflags = reqcuflags;

#ifdef SUPPORT_WIDE_CHARS  /* Defined for 16/32 bits, or 8-bit with Unicode */
    if (xclass && (
#ifdef SUPPORT_UNICODE
        (options & PCRE2_UCP) != 0 ||
#endif
        xclass_has_prop || !should_flip_negation))
      {
      if (match_all_or_no_wide_chars || (
#if PCRE2_CODE_UNIT_WIDTH == 8
           utf &&
#endif
           should_flip_negation && !negate_class && (options & PCRE2_UCP) == 0))
        {
        *class_uchardata++ = XCL_RANGE;
        if (utf)   /* Will always be utf in the 8-bit library */
          {
          class_uchardata += PRIV(ord2utf)(0x100, class_uchardata);
          class_uchardata += PRIV(ord2utf)(MAX_UTF_CODE_POINT, class_uchardata);
          }
        else       /* Can only happen for the 16-bit & 32-bit libraries */
          {
#if PCRE2_CODE_UNIT_WIDTH == 16
          *class_uchardata++ = 0x100;
          *class_uchardata++ = 0xffffu;
#elif PCRE2_CODE_UNIT_WIDTH == 32
          *class_uchardata++ = 0x100;
          *class_uchardata++ = 0xffffffffu;
#endif
          }
        }
      *class_uchardata++ = XCL_END;    /* Marks the end of extra data */
      *code++ = OP_XCLASS;
      code += LINK_SIZE;
      *code = negate_class? XCL_NOT:0;
      if (xclass_has_prop) *code |= XCL_HASPROP;

      if (class_has_8bitchar > 0)
        {
        *code++ |= XCL_MAP;
        (void)memmove(code + (32 / sizeof(PCRE2_UCHAR)), code,
          CU2BYTES(class_uchardata - code));
        if (negate_class && !xclass_has_prop)
          {
          /* Using 255 ^ instead of ~ avoids clang sanitize warning. */
          for (i = 0; i < 32; i++) classbits[i] = 255 ^ classbits[i];
          }
        memcpy(code, classbits, 32);
        code = class_uchardata + (32 / sizeof(PCRE2_UCHAR));
        }
      else code = class_uchardata;

      PUT(previous, 1, (int)(code - previous));
      break;   /* End of class handling */
      }
#endif  /* SUPPORT_WIDE_CHARS */

    *code++ = (negate_class == should_flip_negation) ? OP_CLASS : OP_NCLASS;
    if (lengthptr == NULL)    /* Save time in the pre-compile phase */
      {
      if (negate_class)
        {
       /* Using 255 ^ instead of ~ avoids clang sanitize warning. */
       for (i = 0; i < 32; i++) classbits[i] = 255 ^ classbits[i];
       }
      memcpy(code, classbits, 32);
      }
    code += 32 / sizeof(PCRE2_UCHAR);
    break;  /* End of class processing */

    case META_ACCEPT:
    cb->had_accept = had_accept = TRUE;
    for (oc = cb->open_caps;
         oc != NULL && oc->assert_depth >= cb->assert_depth;
         oc = oc->next)
      {
      if (lengthptr != NULL)
        {
        *lengthptr += CU2BYTES(1) + IMM2_SIZE;
        }
      else
        {
        *code++ = OP_CLOSE;
        PUT2INC(code, 0, oc->number);
        }
      }
    *code++ = (cb->assert_depth > 0)? OP_ASSERT_ACCEPT : OP_ACCEPT;
    if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
    break;

    case META_PRUNE:
    case META_SKIP:
    cb->had_pruneorskip = TRUE;
    /* Fall through */
    case META_COMMIT:
    case META_FAIL:
    *code++ = verbops[(meta - META_MARK) >> 16];
    break;

    case META_THEN:
    cb->external_flags |= PCRE2_HASTHEN;
    *code++ = OP_THEN;
    break;

    case META_THEN_ARG:
    cb->external_flags |= PCRE2_HASTHEN;
    goto VERB_ARG;

    case META_PRUNE_ARG:
    case META_SKIP_ARG:
    cb->had_pruneorskip = TRUE;
    /* Fall through */
    case META_MARK:
    case META_COMMIT_ARG:
    VERB_ARG:
    *code++ = verbops[(meta - META_MARK) >> 16];
    /* The length is in characters. */
    verbarglen = *(++pptr);
    verbculen = 0;
    tempcode = code++;
    for (i = 0; i < (int)verbarglen; i++)
      {
      meta = *(++pptr);
#ifdef SUPPORT_UNICODE
      if (utf) mclength = PRIV(ord2utf)(meta, mcbuffer); else
#endif
        {
        mclength = 1;
        mcbuffer[0] = meta;
        }
      if (lengthptr != NULL) *lengthptr += mclength; else
        {
        memcpy(code, mcbuffer, CU2BYTES(mclength));
        code += mclength;
        verbculen += mclength;
        }
      }

    *tempcode = verbculen;   /* Fill in the code unit length */
    *code++ = 0;             /* Terminating zero */
    break;

    case META_OPTIONS:
    *optionsptr = options = *(++pptr);
    greedy_default = ((options & PCRE2_UNGREEDY) != 0);
    greedy_non_default = greedy_default ^ 1;
    req_caseopt = ((options & PCRE2_CASELESS) != 0)? REQ_CASELESS : 0;
    break;

    case META_COND_RNUMBER:   /* (?(Rdigits) */
    case META_COND_NAME:      /* (?(name) or (?'name') or ?(<name>) */
    case META_COND_RNAME:     /* (?(R&name) - test for recursion */
    bravalue = OP_COND;
      {
      int count, index;
      PCRE2_SPTR name;
      named_group *ng = cb->named_groups;
      uint32_t length = *(++pptr);

      GETPLUSOFFSET(offset, pptr);
      name = cb->start_pattern + offset;

      for (i = 0; i < cb->names_found; i++, ng++)
        {
        if (length == ng->length &&
            PRIV(strncmp)(name, ng->name, length) == 0)
          {
          if (!ng->isdup)
            {
            code[1+LINK_SIZE] = (meta == META_COND_RNAME)? OP_RREF : OP_CREF;
            PUT2(code, 2+LINK_SIZE, ng->number);
            if (ng->number > cb->top_backref) cb->top_backref = ng->number;
            skipunits = 1+IMM2_SIZE;
            goto GROUP_PROCESS_NOTE_EMPTY;
            }
          break;  /* Found a duplicated name */
          }
        }

      if (i >= cb->names_found)
        {
        groupnumber = 0;
        if (meta == META_COND_RNUMBER)
          {
          for (i = 1; i < (int)length; i++)
            {
            groupnumber = groupnumber * 10 + name[i] - CHAR_0;
            if (groupnumber > MAX_GROUP_NUMBER)
              {
              *errorcodeptr = ERR61;
              cb->erroroffset = offset + i;
              return 0;
              }
            }
          }

        if (meta != META_COND_RNUMBER || groupnumber > cb->bracount)
          {
          *errorcodeptr = ERR15;
          cb->erroroffset = offset;
          return 0;
          }

        if (groupnumber == 0) groupnumber = RREF_ANY;
        code[1+LINK_SIZE] = OP_RREF;
        PUT2(code, 2+LINK_SIZE, groupnumber);
        skipunits = 1+IMM2_SIZE;
        goto GROUP_PROCESS_NOTE_EMPTY;
        }

      code[1+LINK_SIZE] = (meta == META_COND_RNAME)? OP_RREF : OP_CREF;

      count = 0;  /* Values for first pass (avoids compiler warning) */
      index = 0;
      if (lengthptr == NULL && !find_dupname_details(name, length, &index,
            &count, errorcodeptr, cb)) return 0;

      code[1+LINK_SIZE]++;
      skipunits = 1+2*IMM2_SIZE;
      PUT2(code, 2+LINK_SIZE, index);
      PUT2(code, 2+LINK_SIZE+IMM2_SIZE, count);
      }
    goto GROUP_PROCESS_NOTE_EMPTY;

    case META_COND_DEFINE:
    bravalue = OP_COND;
    GETPLUSOFFSET(offset, pptr);
    code[1+LINK_SIZE] = OP_DEFINE;
    skipunits = 1;
    goto GROUP_PROCESS;

    case META_COND_NUMBER:
    bravalue = OP_COND;
    GETPLUSOFFSET(offset, pptr);
    groupnumber = *(++pptr);
    if (groupnumber > cb->bracount)
      {
      *errorcodeptr = ERR15;
      cb->erroroffset = offset;
      return 0;
      }
    if (groupnumber > cb->top_backref) cb->top_backref = groupnumber;
    offset -= 2;   /* Point at initial ( for too many branches error */
    code[1+LINK_SIZE] = OP_CREF;
    skipunits = 1+IMM2_SIZE;
    PUT2(code, 2+LINK_SIZE, groupnumber);
    goto GROUP_PROCESS_NOTE_EMPTY;

    case META_COND_VERSION:
    bravalue = OP_COND;
    if (pptr[1] > 0)
      code[1+LINK_SIZE] = ((PCRE2_MAJOR > pptr[2]) ||
        (PCRE2_MAJOR == pptr[2] && PCRE2_MINOR >= pptr[3]))?
          OP_TRUE : OP_FALSE;
    else
      code[1+LINK_SIZE] = (PCRE2_MAJOR == pptr[2] && PCRE2_MINOR == pptr[3])?
        OP_TRUE : OP_FALSE;
    skipunits = 1;
    pptr += 3;
    goto GROUP_PROCESS_NOTE_EMPTY;

    case META_COND_ASSERT:
    bravalue = OP_COND;
    goto GROUP_PROCESS_NOTE_EMPTY;

    case META_LOOKAHEAD:
    bravalue = OP_ASSERT;
    cb->assert_depth += 1;
    goto GROUP_PROCESS;

    case META_LOOKAHEAD_NA:
    bravalue = OP_ASSERT_NA;
    cb->assert_depth += 1;
    goto GROUP_PROCESS;

    case META_LOOKAHEADNOT:
    if (pptr[1] == META_KET &&
         (pptr[2] < META_ASTERISK || pptr[2] > META_MINMAX_QUERY))
      {
      *code++ = OP_FAIL;
      pptr++;
      }
    else
      {
      bravalue = OP_ASSERT_NOT;
      cb->assert_depth += 1;
      goto GROUP_PROCESS;
      }
    break;

    case META_LOOKBEHIND:
    bravalue = OP_ASSERTBACK;
    cb->assert_depth += 1;
    goto GROUP_PROCESS;

    case META_LOOKBEHINDNOT:
    bravalue = OP_ASSERTBACK_NOT;
    cb->assert_depth += 1;
    goto GROUP_PROCESS;

    case META_LOOKBEHIND_NA:
    bravalue = OP_ASSERTBACK_NA;
    cb->assert_depth += 1;
    goto GROUP_PROCESS;

    case META_ATOMIC:
    bravalue = OP_ONCE;
    goto GROUP_PROCESS_NOTE_EMPTY;

    case META_SCRIPT_RUN:
    bravalue = OP_SCRIPT_RUN;
    goto GROUP_PROCESS_NOTE_EMPTY;

    case META_NOCAPTURE:
    bravalue = OP_BRA;
    /* Fall through */

    GROUP_PROCESS_NOTE_EMPTY:
    note_group_empty = TRUE;

    GROUP_PROCESS:
    cb->parens_depth += 1;
    *code = bravalue;
    pptr++;
    tempcode = code;
    tempreqvary = cb->req_varyopt;        /* Save value before group */
    length_prevgroup = 0;                 /* Initialize for pre-compile phase */

    if ((group_return =
         compile_regex(
         options,                         /* The option state */
         &tempcode,                       /* Where to put code (updated) */
         &pptr,                           /* Input pointer (updated) */
         errorcodeptr,                    /* Where to put an error message */
         skipunits,                       /* Skip over bracket number */
         &subfirstcu,                     /* For possible first char */
         &subfirstcuflags,
         &subreqcu,                       /* For possible last char */
         &subreqcuflags,
         bcptr,                           /* Current branch chain */
         cb,                              /* Compile data block */
         (lengthptr == NULL)? NULL :      /* Actual compile phase */
           &length_prevgroup              /* Pre-compile phase */
         )) == 0)
      return 0;  /* Error */

    cb->parens_depth -= 1;

    if (note_group_empty && bravalue != OP_COND && group_return > 0)
      matched_char = TRUE;

    if (bravalue >= OP_ASSERT && bravalue <= OP_ASSERTBACK_NA)
      cb->assert_depth -= 1;

    if (bravalue == OP_COND && lengthptr == NULL)
      {
      PCRE2_UCHAR *tc = code;
      int condcount = 0;

      do {
         condcount++;
         tc += GET(tc,1);
         }
      while (*tc != OP_KET);

      if (code[LINK_SIZE+1] == OP_DEFINE)
        {
        if (condcount > 1)
          {
          cb->erroroffset = offset;
          *errorcodeptr = ERR54;
          return 0;
          }
        code[LINK_SIZE+1] = OP_FALSE;
        bravalue = OP_DEFINE;   /* A flag to suppress char handling below */
        }

      else
        {
        if (condcount > 2)
          {
          cb->erroroffset = offset;
          *errorcodeptr = ERR27;
          return 0;
          }
        if (condcount == 1) subfirstcuflags = subreqcuflags = REQ_NONE;
          else if (group_return > 0) matched_char = TRUE;
        }
      }

    if (lengthptr != NULL)
      {
      if (OFLOW_MAX - *lengthptr < length_prevgroup - 2 - 2*LINK_SIZE)
        {
        *errorcodeptr = ERR20;
        return 0;
        }
      *lengthptr += length_prevgroup - 2 - 2*LINK_SIZE;
      code++;   /* This already contains bravalue */
      PUTINC(code, 0, 1 + LINK_SIZE);
      *code++ = OP_KET;
      PUTINC(code, 0, 1 + LINK_SIZE);
      break;    /* No need to waste time with special character handling */
      }

    code = tempcode;

    if (bravalue == OP_DEFINE) break;

    zeroreqcu = reqcu;
    zeroreqcuflags = reqcuflags;
    zerofirstcu = firstcu;
    zerofirstcuflags = firstcuflags;
    groupsetfirstcu = FALSE;

    if (bravalue >= OP_ONCE)  /* Not an assertion */
      {

      if (firstcuflags == REQ_UNSET && subfirstcuflags != REQ_UNSET)
        {
        if (subfirstcuflags >= 0)
          {
          firstcu = subfirstcu;
          firstcuflags = subfirstcuflags;
          groupsetfirstcu = TRUE;
          }
        else firstcuflags = REQ_NONE;
        zerofirstcuflags = REQ_NONE;
        }

      else if (subfirstcuflags >= 0 && subreqcuflags < 0)
        {
        subreqcu = subfirstcu;
        subreqcuflags = subfirstcuflags | tempreqvary;
        }

      if (subreqcuflags >= 0)
        {
        reqcu = subreqcu;
        reqcuflags = subreqcuflags;
        }
      }

    else if ((bravalue == OP_ASSERT || bravalue == OP_ASSERT_NA) &&
             subreqcuflags >= 0 && subfirstcuflags >= 0)
      {
      reqcu = subreqcu;
      reqcuflags = subreqcuflags;
      }

    break;  /* End of nested group handling */

    case META_BACKREF_BYNAME:
    case META_RECURSE_BYNAME:
      {
      int count, index;
      PCRE2_SPTR name;
      BOOL is_dupname = FALSE;
      named_group *ng = cb->named_groups;
      uint32_t length = *(++pptr);

      GETPLUSOFFSET(offset, pptr);
      name = cb->start_pattern + offset;

      groupnumber = 0;
      for (i = 0; i < cb->names_found; i++, ng++)
        {
        if (length == ng->length &&
            PRIV(strncmp)(name, ng->name, length) == 0)
          {
          is_dupname = ng->isdup;
          groupnumber = ng->number;

          if (meta == META_RECURSE_BYNAME)
            {
            meta_arg = groupnumber;
            goto HANDLE_NUMERICAL_RECURSION;
            }

          cb->backref_map |= (groupnumber < 32)? (1u << groupnumber) : 1;
          if (groupnumber > cb->top_backref)
            cb->top_backref = groupnumber;
          }
        }

      if (groupnumber == 0)
        {
        *errorcodeptr = ERR15;
        cb->erroroffset = offset;
        return 0;
        }

      if (!is_dupname)
        {
        meta_arg = groupnumber;
        goto HANDLE_SINGLE_REFERENCE;
        }

      count = 0;  /* Values for first pass (avoids compiler warning) */
      index = 0;
      if (lengthptr == NULL && !find_dupname_details(name, length, &index,
            &count, errorcodeptr, cb)) return 0;

      if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
      *code++ = ((options & PCRE2_CASELESS) != 0)? OP_DNREFI : OP_DNREF;
      PUT2INC(code, 0, index);
      PUT2INC(code, 0, count);
      }
    break;

    case META_CALLOUT_NUMBER:
    code[0] = OP_CALLOUT;
    PUT(code, 1, pptr[1]);               /* Offset to next pattern item */
    PUT(code, 1 + LINK_SIZE, pptr[2]);   /* Length of next pattern item */
    code[1 + 2*LINK_SIZE] = pptr[3];
    pptr += 3;
    code += PRIV(OP_lengths)[OP_CALLOUT];
    break;

    case META_CALLOUT_STRING:
    if (lengthptr != NULL)
      {
      *lengthptr += pptr[3] + (1 + 4*LINK_SIZE);
      pptr += 3;
      SKIPOFFSET(pptr);
      }

    else
      {
      PCRE2_SPTR pp;
      uint32_t delimiter;
      uint32_t length = pptr[3];
      PCRE2_UCHAR *callout_string = code + (1 + 4*LINK_SIZE);

      code[0] = OP_CALLOUT_STR;
      PUT(code, 1, pptr[1]);               /* Offset to next pattern item */
      PUT(code, 1 + LINK_SIZE, pptr[2]);   /* Length of next pattern item */

      pptr += 3;
      GETPLUSOFFSET(offset, pptr);         /* Offset to string in pattern */
      pp = cb->start_pattern + offset;
      delimiter = *callout_string++ = *pp++;
      if (delimiter == CHAR_LEFT_CURLY_BRACKET)
        delimiter = CHAR_RIGHT_CURLY_BRACKET;
      PUT(code, 1 + 3*LINK_SIZE, (int)(offset + 1));  /* One after delimiter */

      while (--length > 1)
        {
        if (*pp == delimiter && pp[1] == delimiter)
          {
          *callout_string++ = delimiter;
          pp += 2;
          length--;
          }
        else *callout_string++ = *pp++;
        }
      *callout_string++ = CHAR_NUL;

      PUT(code, 1 + 2*LINK_SIZE, (int)(callout_string - code));
      code = callout_string;
      }
    break;

    case META_MINMAX_PLUS:
    case META_MINMAX_QUERY:
    case META_MINMAX:
    repeat_min = *(++pptr);
    repeat_max = *(++pptr);
    goto REPEAT;

    case META_ASTERISK:
    case META_ASTERISK_PLUS:
    case META_ASTERISK_QUERY:
    repeat_min = 0;
    repeat_max = REPEAT_UNLIMITED;
    goto REPEAT;

    case META_PLUS:
    case META_PLUS_PLUS:
    case META_PLUS_QUERY:
    repeat_min = 1;
    repeat_max = REPEAT_UNLIMITED;
    goto REPEAT;

    case META_QUERY:
    case META_QUERY_PLUS:
    case META_QUERY_QUERY:
    repeat_min = 0;
    repeat_max = 1;

    REPEAT:
    if (previous_matched_char && repeat_min > 0) matched_char = TRUE;

    reqvary = (repeat_min == repeat_max)? 0 : REQ_VARY;
    op_type = 0;

    if (repeat_min == 0)
      {
      firstcu = zerofirstcu;
      firstcuflags = zerofirstcuflags;
      reqcu = zeroreqcu;
      reqcuflags = zeroreqcuflags;
      }

    switch (meta)
      {
      case META_MINMAX_PLUS:
      case META_ASTERISK_PLUS:
      case META_PLUS_PLUS:
      case META_QUERY_PLUS:
      repeat_type = 0;                  /* Force greedy */
      possessive_quantifier = TRUE;
      break;

      case META_MINMAX_QUERY:
      case META_ASTERISK_QUERY:
      case META_PLUS_QUERY:
      case META_QUERY_QUERY:
      repeat_type = greedy_non_default;
      possessive_quantifier = FALSE;
      break;

      default:
      repeat_type = greedy_default;
      possessive_quantifier = FALSE;
      break;
      }

    tempcode = previous;
    op_previous = *previous;

    switch (op_previous)
      {

      case OP_CHAR:
      case OP_CHARI:
      case OP_NOT:
      case OP_NOTI:
      if (repeat_max == 1 && repeat_min == 1) goto END_REPEAT;
      op_type = chartypeoffset[op_previous - OP_CHAR];

#ifdef MAYBE_UTF_MULTI
      if (utf && NOT_FIRSTCU(code[-1]))
        {
        PCRE2_UCHAR *lastchar = code - 1;
        BACKCHAR(lastchar);
        mclength = (uint32_t)(code - lastchar);   /* Length of UTF character */
        memcpy(mcbuffer, lastchar, CU2BYTES(mclength));  /* Save the char */
        }
      else
#endif  /* MAYBE_UTF_MULTI */
        {
        mcbuffer[0] = code[-1];
        mclength = 1;
        if (op_previous <= OP_CHARI && repeat_min > 1)
          {
          reqcu = mcbuffer[0];
          reqcuflags = req_caseopt | cb->req_varyopt;
          }
        }
      goto OUTPUT_SINGLE_REPEAT;  /* Code shared with single character types */

#ifdef SUPPORT_WIDE_CHARS
      case OP_XCLASS:
#endif
      case OP_CLASS:
      case OP_NCLASS:
      case OP_REF:
      case OP_REFI:
      case OP_DNREF:
      case OP_DNREFI:

      if (repeat_max == 0)
        {
        code = previous;
        goto END_REPEAT;
        }
      if (repeat_max == 1 && repeat_min == 1) goto END_REPEAT;

      if (repeat_min == 0 && repeat_max == REPEAT_UNLIMITED)
        *code++ = OP_CRSTAR + repeat_type;
      else if (repeat_min == 1 && repeat_max == REPEAT_UNLIMITED)
        *code++ = OP_CRPLUS + repeat_type;
      else if (repeat_min == 0 && repeat_max == 1)
        *code++ = OP_CRQUERY + repeat_type;
      else
        {
        *code++ = OP_CRRANGE + repeat_type;
        PUT2INC(code, 0, repeat_min);
        if (repeat_max == REPEAT_UNLIMITED) repeat_max = 0;  /* 2-byte encoding for max */
        PUT2INC(code, 0, repeat_max);
        }
      break;

      case OP_FAIL:
      goto END_REPEAT;

      case OP_RECURSE:
      if (repeat_max == 1 && repeat_min == 1 && !possessive_quantifier)
        goto END_REPEAT;

      if (repeat_min > 0 && (repeat_min != 1 || repeat_max != REPEAT_UNLIMITED))
        {
        int replicate = repeat_min;
        if (repeat_min == repeat_max) replicate--;

        if (lengthptr != NULL)
          {
          PCRE2_SIZE delta = replicate*(1 + LINK_SIZE);
          if ((INT64_OR_DOUBLE)replicate*
                (INT64_OR_DOUBLE)(1 + LINK_SIZE) >
                  (INT64_OR_DOUBLE)INT_MAX ||
              OFLOW_MAX - *lengthptr < delta)
            {
            *errorcodeptr = ERR20;
            return 0;
            }
          *lengthptr += delta;
          }

        else for (i = 0; i < replicate; i++)
          {
          memcpy(code, previous, CU2BYTES(1 + LINK_SIZE));
          previous = code;
          code += 1 + LINK_SIZE;
          }

        if (repeat_min == repeat_max) break;
        if (repeat_max != REPEAT_UNLIMITED) repeat_max -= repeat_min;
        repeat_min = 0;
        }

      (void)memmove(previous + 1 + LINK_SIZE, previous, CU2BYTES(1 + LINK_SIZE));
      op_previous = *previous = OP_BRA;
      PUT(previous, 1, 2 + 2*LINK_SIZE);
      previous[2 + 2*LINK_SIZE] = OP_KET;
      PUT(previous, 3 + 2*LINK_SIZE, 2 + 2*LINK_SIZE);
      code += 2 + 2 * LINK_SIZE;
      length_prevgroup = 3 + 3*LINK_SIZE;
      group_return = -1;  /* Set "may match empty string" */

      case OP_ASSERT:
      case OP_ASSERT_NOT:
      case OP_ASSERT_NA:
      case OP_ASSERTBACK:
      case OP_ASSERTBACK_NOT:
      case OP_ASSERTBACK_NA:
      case OP_ONCE:
      case OP_SCRIPT_RUN:
      case OP_BRA:
      case OP_CBRA:
      case OP_COND:
        {
        int len = (int)(code - previous);
        PCRE2_UCHAR *bralink = NULL;
        PCRE2_UCHAR *brazeroptr = NULL;

        if (repeat_max == 1 && repeat_min == 1 && !possessive_quantifier)
          goto END_REPEAT;

        if (op_previous == OP_COND && previous[LINK_SIZE+1] == OP_FALSE &&
            previous[GET(previous, 1)] != OP_ALT)
          goto END_REPEAT;

        if (op_previous < OP_ONCE)    /* Assertion */
          {
          if (repeat_max == REPEAT_UNLIMITED) repeat_max = repeat_min + 1;
          }

        if (repeat_min == 0)
          {

          if (repeat_max <= 1 || repeat_max == REPEAT_UNLIMITED)
            {
            (void)memmove(previous + 1, previous, CU2BYTES(len));
            code++;
            if (repeat_max == 0)
              {
              *previous++ = OP_SKIPZERO;
              goto END_REPEAT;
              }
            brazeroptr = previous;    /* Save for possessive optimizing */
            *previous++ = OP_BRAZERO + repeat_type;
            }

          else
            {
            int linkoffset;
            (void)memmove(previous + 2 + LINK_SIZE, previous, CU2BYTES(len));
            code += 2 + LINK_SIZE;
            *previous++ = OP_BRAZERO + repeat_type;
            *previous++ = OP_BRA;

            linkoffset = (bralink == NULL)? 0 : (int)(previous - bralink);
            bralink = previous;
            PUTINC(previous, 0, linkoffset);
            }

          if (repeat_max != REPEAT_UNLIMITED) repeat_max--;
          }

        else
          {
          if (repeat_min > 1)
            {

            if (lengthptr != NULL)
              {
              PCRE2_SIZE delta = (repeat_min - 1)*length_prevgroup;
              if ((INT64_OR_DOUBLE)(repeat_min - 1)*
                    (INT64_OR_DOUBLE)length_prevgroup >
                      (INT64_OR_DOUBLE)INT_MAX ||
                  OFLOW_MAX - *lengthptr < delta)
                {
                *errorcodeptr = ERR20;
                return 0;
                }
              *lengthptr += delta;
              }

            else
              {
              if (groupsetfirstcu && reqcuflags < 0)
                {
                reqcu = firstcu;
                reqcuflags = firstcuflags;
                }
              for (i = 1; (uint32_t)i < repeat_min; i++)
                {
                memcpy(code, previous, CU2BYTES(len));
                code += len;
                }
              }
            }

          if (repeat_max != REPEAT_UNLIMITED) repeat_max -= repeat_min;
          }

        if (repeat_max != REPEAT_UNLIMITED)
          {

          if (lengthptr != NULL && repeat_max > 0)
            {
            PCRE2_SIZE delta = repeat_max*(length_prevgroup + 1 + 2 + 2*LINK_SIZE) -
                        2 - 2*LINK_SIZE;   /* Last one doesn't nest */
            if ((INT64_OR_DOUBLE)repeat_max *
                  (INT64_OR_DOUBLE)(length_prevgroup + 1 + 2 + 2*LINK_SIZE)
                    > (INT64_OR_DOUBLE)INT_MAX ||
                OFLOW_MAX - *lengthptr < delta)
              {
              *errorcodeptr = ERR20;
              return 0;
              }
            *lengthptr += delta;
            }

          else for (i = repeat_max - 1; i >= 0; i--)
            {
            *code++ = OP_BRAZERO + repeat_type;

            if (i != 0)
              {
              int linkoffset;
              *code++ = OP_BRA;
              linkoffset = (bralink == NULL)? 0 : (int)(code - bralink);
              bralink = code;
              PUTINC(code, 0, linkoffset);
              }

            memcpy(code, previous, CU2BYTES(len));
            code += len;
            }

          while (bralink != NULL)
            {
            int oldlinkoffset;
            int linkoffset = (int)(code - bralink + 1);
            PCRE2_UCHAR *bra = code - linkoffset;
            oldlinkoffset = GET(bra, 1);
            bralink = (oldlinkoffset == 0)? NULL : bralink - oldlinkoffset;
            *code++ = OP_KET;
            PUTINC(code, 0, linkoffset);
            PUT(bra, 1, linkoffset);
            }
          }

        else
          {
          PCRE2_UCHAR *ketcode = code - 1 - LINK_SIZE;
          PCRE2_UCHAR *bracode = ketcode - GET(ketcode, 1);

          if (*bracode == OP_ONCE && possessive_quantifier) *bracode = OP_BRA;

          if (*bracode == OP_ONCE || *bracode == OP_SCRIPT_RUN)
            *ketcode = OP_KETRMAX + repeat_type;

          else
            {

            if (lengthptr == NULL)
              {
              if (group_return < 0) *bracode += OP_SBRA - OP_BRA;
              if (*bracode == OP_COND && bracode[GET(bracode,1)] != OP_ALT)
                *bracode = OP_SCOND;
              }

            if (possessive_quantifier)
              {

              if (*bracode == OP_COND || *bracode == OP_SCOND)
                {
                int nlen = (int)(code - bracode);
                (void)memmove(bracode + 1 + LINK_SIZE, bracode, CU2BYTES(nlen));
                code += 1 + LINK_SIZE;
                nlen += 1 + LINK_SIZE;
                *bracode = (*bracode == OP_COND)? OP_BRAPOS : OP_SBRAPOS;
                *code++ = OP_KETRPOS;
                PUTINC(code, 0, nlen);
                PUT(bracode, 1, nlen);
                }

              else
                {
                *bracode += 1;              /* Switch to xxxPOS opcodes */
                *ketcode = OP_KETRPOS;
                }

              if (brazeroptr != NULL) *brazeroptr = OP_BRAPOSZERO;
              if (repeat_min < 2) possessive_quantifier = FALSE;
              }

            else *ketcode = OP_KETRMAX + repeat_type;
            }
          }
        }
      break;

      default:
      if (op_previous >= OP_EODN)   /* Not a character type - internal error */
        {
        *errorcodeptr = ERR10;
        return 0;
        }
      else
        {
        int prop_type, prop_value;
        PCRE2_UCHAR *oldcode;

        if (repeat_max == 1 && repeat_min == 1) goto END_REPEAT;

        op_type = OP_TYPESTAR - OP_STAR;      /* Use type opcodes */
        mclength = 0;                         /* Not a character */

        if (op_previous == OP_PROP || op_previous == OP_NOTPROP)
          {
          prop_type = previous[1];
          prop_value = previous[2];
          }
        else
          {
          OUTPUT_SINGLE_REPEAT:
          prop_type = prop_value = -1;
          }

        oldcode = code;                   /* Save where we were */
        code = previous;                  /* Usually overwrite previous item */

        if (repeat_max == 0) goto END_REPEAT;

        repeat_type += op_type;

        if (repeat_min == 0)
          {
          if (repeat_max == REPEAT_UNLIMITED) *code++ = OP_STAR + repeat_type;
            else if (repeat_max == 1) *code++ = OP_QUERY + repeat_type;
          else
            {
            *code++ = OP_UPTO + repeat_type;
            PUT2INC(code, 0, repeat_max);
            }
          }

        else if (repeat_min == 1)
          {
          if (repeat_max == REPEAT_UNLIMITED)
            *code++ = OP_PLUS + repeat_type;
          else
            {
            code = oldcode;  /* Leave previous item in place */
            if (repeat_max == 1) goto END_REPEAT;
            *code++ = OP_UPTO + repeat_type;
            PUT2INC(code, 0, repeat_max - 1);
            }
          }

        else
          {
          *code++ = OP_EXACT + op_type;  /* NB EXACT doesn't have repeat_type */
          PUT2INC(code, 0, repeat_min);

          if (repeat_max != repeat_min)
            {
            if (mclength > 0)
              {
              memcpy(code, mcbuffer, CU2BYTES(mclength));
              code += mclength;
              }
            else
              {
              *code++ = op_previous;
              if (prop_type >= 0)
                {
                *code++ = prop_type;
                *code++ = prop_value;
                }
              }

            if (repeat_max == REPEAT_UNLIMITED)
              *code++ = OP_STAR + repeat_type;
            else
              {
              repeat_max -= repeat_min;
              if (repeat_max == 1)
                {
                *code++ = OP_QUERY + repeat_type;
                }
              else
                {
                *code++ = OP_UPTO + repeat_type;
                PUT2INC(code, 0, repeat_max);
                }
              }
            }
          }

        if (mclength > 0)
          {
          memcpy(code, mcbuffer, CU2BYTES(mclength));
          code += mclength;
          }
        else
          {
          *code++ = op_previous;
          if (prop_type >= 0)
            {
            *code++ = prop_type;
            *code++ = prop_value;
            }
          }
        }
      break;
      }  /* End of switch on different op_previous values */

    if (possessive_quantifier)
      {
      int len;

      switch(*tempcode)
        {
        case OP_TYPEEXACT:
        tempcode += PRIV(OP_lengths)[*tempcode] +
          ((tempcode[1 + IMM2_SIZE] == OP_PROP
          || tempcode[1 + IMM2_SIZE] == OP_NOTPROP)? 2 : 0);
        break;

        case OP_CHAR:
        case OP_CHARI:
        case OP_NOT:
        case OP_NOTI:
        case OP_EXACT:
        case OP_EXACTI:
        case OP_NOTEXACT:
        case OP_NOTEXACTI:
        tempcode += PRIV(OP_lengths)[*tempcode];
#ifdef SUPPORT_UNICODE
        if (utf && HAS_EXTRALEN(tempcode[-1]))
          tempcode += GET_EXTRALEN(tempcode[-1]);
#endif
        break;

        case OP_CLASS:
        case OP_NCLASS:
        tempcode += 1 + 32/sizeof(PCRE2_UCHAR);
        break;

#ifdef SUPPORT_WIDE_CHARS
        case OP_XCLASS:
        tempcode += GET(tempcode, 1);
        break;
#endif
        }

      len = (int)(code - tempcode);
      if (len > 0)
        {
        unsigned int repcode = *tempcode;

        if (repcode < OP_CALLOUT && opcode_possessify[repcode] > 0)
          *tempcode = opcode_possessify[repcode];

        else
          {
          (void)memmove(tempcode + 1 + LINK_SIZE, tempcode, CU2BYTES(len));
          code += 1 + LINK_SIZE;
          len += 1 + LINK_SIZE;
          tempcode[0] = OP_ONCE;
          *code++ = OP_KET;
          PUTINC(code, 0, len);
          PUT(tempcode, 1, len);
          }
        }
      }

    END_REPEAT:
    cb->req_varyopt |= reqvary;
    break;

    case META_BIGVALUE:
    pptr++;
    goto NORMAL_CHAR;

    case META_BACKREF:
    if (meta_arg < 10) offset = cb->small_ref_offset[meta_arg];
      else GETPLUSOFFSET(offset, pptr);

    if (meta_arg > cb->bracount)
      {
      cb->erroroffset = offset;
      *errorcodeptr = ERR15;  /* Non-existent subpattern */
      return 0;
      }

    HANDLE_SINGLE_REFERENCE:
    if (firstcuflags == REQ_UNSET) zerofirstcuflags = firstcuflags = REQ_NONE;
    *code++ = ((options & PCRE2_CASELESS) != 0)? OP_REFI : OP_REF;
    PUT2INC(code, 0, meta_arg);

    cb->backref_map |= (meta_arg < 32)? (1u << meta_arg) : 1;
    if (meta_arg > cb->top_backref) cb->top_backref = meta_arg;
    break;

    case META_RECURSE:
    GETPLUSOFFSET(offset, pptr);
    if (meta_arg > cb->bracount)
      {
      cb->erroroffset = offset;
      *errorcodeptr = ERR15;  /* Non-existent subpattern */
      return 0;
      }
    HANDLE_NUMERICAL_RECURSION:
    *code = OP_RECURSE;
    PUT(code, 1, meta_arg);
    code += 1 + LINK_SIZE;
    groupsetfirstcu = FALSE;
    cb->had_recurse = TRUE;
    if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
    zerofirstcu = firstcu;
    zerofirstcuflags = firstcuflags;
    break;

    case META_CAPTURE:
    bravalue = OP_CBRA;
    skipunits = IMM2_SIZE;
    PUT2(code, 1+LINK_SIZE, meta_arg);
    cb->lastcapture = meta_arg;
    goto GROUP_PROCESS_NOTE_EMPTY;

    case META_ESCAPE:

    if (meta_arg > ESC_b && meta_arg < ESC_Z)
      {
      matched_char = TRUE;
      if (firstcuflags == REQ_UNSET) firstcuflags = REQ_NONE;
      }

    zerofirstcu = firstcu;
    zerofirstcuflags = firstcuflags;
    zeroreqcu = reqcu;
    zeroreqcuflags = reqcuflags;

#ifdef SUPPORT_UNICODE
    if (meta_arg == ESC_P || meta_arg == ESC_p)
      {
      uint32_t ptype = *(++pptr) >> 16;
      uint32_t pdata = *pptr & 0xffff;

      if (meta_arg == ESC_p && ptype == PT_ANY)
        {
        *code++ = OP_ALLANY;
        }
      else
        {
        *code++ = (meta_arg == ESC_p)? OP_PROP : OP_NOTPROP;
        *code++ = ptype;
        *code++ = pdata;
        }
      break;  /* End META_ESCAPE */
      }
#endif

    if (meta_arg == ESC_C) cb->external_flags |= PCRE2_HASBKC; /* Record */
    if ((meta_arg == ESC_b || meta_arg == ESC_B || meta_arg == ESC_A) &&
         cb->max_lookbehind == 0)
      cb->max_lookbehind = 1;

#if PCRE2_CODE_UNIT_WIDTH == 32
    *code++ = (meta_arg == ESC_C)? OP_ALLANY : meta_arg;
#else
    *code++ = (!utf && meta_arg == ESC_C)? OP_ALLANY : meta_arg;
#endif
    break;  /* End META_ESCAPE */

    default:
    if (meta >= META_END)
      {
#ifdef DEBUG_SHOW_PARSED
      fprintf(stderr, "** Unrecognized parsed pattern item 0x%.8x\n", *pptr);
#endif
      *errorcodeptr = ERR89;  /* Internal error - unrecognized. */
      return 0;
      }

    NORMAL_CHAR:
    meta = *pptr;     /* Get the full 32 bits */
    NORMAL_CHAR_SET:  /* Character is already in meta */
    matched_char = TRUE;

#ifdef SUPPORT_UNICODE
    if ((utf||ucp) && (options & PCRE2_CASELESS) != 0)
      {
      uint32_t caseset = UCD_CASESET(meta);
      if (caseset != 0)
        {
        *code++ = OP_PROP;
        *code++ = PT_CLIST;
        *code++ = caseset;
        if (firstcuflags == REQ_UNSET)
          firstcuflags = zerofirstcuflags = REQ_NONE;
        break;  /* End handling this meta item */
        }
      }
#endif

    CLASS_CASELESS_CHAR:

#ifdef SUPPORT_UNICODE
    if (utf) mclength = PRIV(ord2utf)(meta, mcbuffer); else
#endif
      {
      mclength = 1;
      mcbuffer[0] = meta;
      }

    *code++ = ((options & PCRE2_CASELESS) != 0)? OP_CHARI : OP_CHAR;
    memcpy(code, mcbuffer, CU2BYTES(mclength));
    code += mclength;

    if (mcbuffer[0] == CHAR_CR || mcbuffer[0] == CHAR_NL)
      cb->external_flags |= PCRE2_HASCRORLF;

    if (firstcuflags == REQ_UNSET)
      {
      zerofirstcuflags = REQ_NONE;
      zeroreqcu = reqcu;
      zeroreqcuflags = reqcuflags;

      if (mclength == 1 || req_caseopt == 0)
        {
        firstcu = mcbuffer[0];
        firstcuflags = req_caseopt;
        if (mclength != 1)
          {
          reqcu = code[-1];
          reqcuflags = cb->req_varyopt;
          }
        }
      else firstcuflags = reqcuflags = REQ_NONE;
      }

    else
      {
      zerofirstcu = firstcu;
      zerofirstcuflags = firstcuflags;
      zeroreqcu = reqcu;
      zeroreqcuflags = reqcuflags;
      if (mclength == 1 || req_caseopt == 0)
        {
        reqcu = code[-1];
        reqcuflags = req_caseopt | cb->req_varyopt;
        }
      }

    if (reset_caseful)
      {
      options &= ~PCRE2_CASELESS;
      req_caseopt = 0;
      reset_caseful = FALSE;
      }

    break;    /* End literal character handling */
    }         /* End of big switch */
  }           /* End of big loop */
}

/*************************************************
*   Compile regex: a sequence of alternatives    *
*************************************************/

static int
compile_regex(uint32_t options, PCRE2_UCHAR **codeptr, uint32_t **pptrptr,
  int *errorcodeptr, uint32_t skipunits, uint32_t *firstcuptr,
  int32_t *firstcuflagsptr, uint32_t *reqcuptr,int32_t *reqcuflagsptr,
  branch_chain *bcptr, compile_block *cb, PCRE2_SIZE *lengthptr)
{
PCRE2_UCHAR *code = *codeptr;
PCRE2_UCHAR *last_branch = code;
PCRE2_UCHAR *start_bracket = code;
BOOL lookbehind;
open_capitem capitem;
int capnumber = 0;
int okreturn = 1;
uint32_t *pptr = *pptrptr;
uint32_t firstcu, reqcu;
uint32_t lookbehindlength;
int32_t firstcuflags, reqcuflags;
uint32_t branchfirstcu, branchreqcu;
int32_t branchfirstcuflags, branchreqcuflags;
PCRE2_SIZE length;
branch_chain bc;

if (cb->cx->stack_guard != NULL &&
    cb->cx->stack_guard(cb->parens_depth, cb->cx->stack_guard_data))
  {
  *errorcodeptr= ERR33;
  return 0;
  }

bc.outer = bcptr;
bc.current_branch = code;

firstcu = reqcu = 0;
firstcuflags = reqcuflags = REQ_UNSET;

length = 2 + 2*LINK_SIZE + skipunits;

lookbehind = *code == OP_ASSERTBACK ||
             *code == OP_ASSERTBACK_NOT ||
             *code == OP_ASSERTBACK_NA;

if (lookbehind)
  {
  lookbehindlength = META_DATA(pptr[-1]);
  pptr += SIZEOFFSET;
  }
else lookbehindlength = 0;

if (*code == OP_CBRA)
  {
  capnumber = GET2(code, 1 + LINK_SIZE);
  capitem.number = capnumber;
  capitem.next = cb->open_caps;
  capitem.assert_depth = cb->assert_depth;
  cb->open_caps = &capitem;
  }

PUT(code, 1, 0);
code += 1 + LINK_SIZE + skipunits;

for (;;)
  {
  int branch_return;

  if (lookbehind && lookbehindlength > 0)
    {
    *code++ = OP_REVERSE;
    PUTINC(code, 0, lookbehindlength);
    length += 1 + LINK_SIZE;
    }

  if ((branch_return =
        compile_branch(&options, &code, &pptr, errorcodeptr, &branchfirstcu,
          &branchfirstcuflags, &branchreqcu, &branchreqcuflags, &bc,
          cb, (lengthptr == NULL)? NULL : &length)) == 0)
    return 0;

  if (branch_return < 0) okreturn = -1;

  if (lengthptr == NULL)
    {

    if (*last_branch != OP_ALT)
      {
      firstcu = branchfirstcu;
      firstcuflags = branchfirstcuflags;
      reqcu = branchreqcu;
      reqcuflags = branchreqcuflags;
      }

    else
      {

      if (firstcuflags != branchfirstcuflags || firstcu != branchfirstcu)
        {
        if (firstcuflags >= 0)
          {
          if (reqcuflags < 0)
            {
            reqcu = firstcu;
            reqcuflags = firstcuflags;
            }
          }
        firstcuflags = REQ_NONE;
        }

      if (firstcuflags < 0 && branchfirstcuflags >= 0 &&
          branchreqcuflags < 0)
        {
        branchreqcu = branchfirstcu;
        branchreqcuflags = branchfirstcuflags;
        }

      if (((reqcuflags & ~REQ_VARY) != (branchreqcuflags & ~REQ_VARY)) ||
          reqcu != branchreqcu)
        reqcuflags = REQ_NONE;
      else
        {
        reqcu = branchreqcu;
        reqcuflags |= branchreqcuflags; /* To "or" REQ_VARY if present */
        }
      }
    }

  if (META_CODE(*pptr) != META_ALT)
    {
    if (lengthptr == NULL)
      {
      PCRE2_SIZE branch_length = code - last_branch;
      do
        {
        PCRE2_SIZE prev_length = GET(last_branch, 1);
        PUT(last_branch, 1, branch_length);
        branch_length = prev_length;
        last_branch -= branch_length;
        }
      while (branch_length > 0);
      }

    *code = OP_KET;
    PUT(code, 1, (int)(code - start_bracket));
    code += 1 + LINK_SIZE;

    if (capnumber > 0) cb->open_caps = cb->open_caps->next;

    *codeptr = code;
    *pptrptr = pptr;
    *firstcuptr = firstcu;
    *firstcuflagsptr = firstcuflags;
    *reqcuptr = reqcu;
    *reqcuflagsptr = reqcuflags;
    if (lengthptr != NULL)
      {
      if (OFLOW_MAX - *lengthptr < length)
        {
        *errorcodeptr = ERR20;
        return 0;
        }
      *lengthptr += length;
      }
    return okreturn;
    }

  if (lengthptr != NULL)
    {
    code = *codeptr + 1 + LINK_SIZE + skipunits;
    length += 1 + LINK_SIZE;
    }
  else
    {
    *code = OP_ALT;
    PUT(code, 1, (int)(code - last_branch));
    bc.current_branch = last_branch = code;
    code += 1 + LINK_SIZE;
    }

  lookbehindlength = META_DATA(*pptr);
  pptr++;
  }
/* Control never reaches here */
}

/*************************************************
*          Check for anchored pattern            *
*************************************************/

static BOOL
is_anchored(PCRE2_SPTR code, unsigned int bracket_map, compile_block *cb,
  int atomcount, BOOL inassert)
{
do {
   PCRE2_SPTR scode = first_significant_code(
     code + PRIV(OP_lengths)[*code], FALSE);
   int op = *scode;

   if (op == OP_BRA  || op == OP_BRAPOS ||
       op == OP_SBRA || op == OP_SBRAPOS)
     {
     if (!is_anchored(scode, bracket_map, cb, atomcount, inassert))
       return FALSE;
     }

   else if (op == OP_CBRA  || op == OP_CBRAPOS ||
            op == OP_SCBRA || op == OP_SCBRAPOS)
     {
     int n = GET2(scode, 1+LINK_SIZE);
     int new_map = bracket_map | ((n < 32)? (1u << n) : 1);
     if (!is_anchored(scode, new_map, cb, atomcount, inassert)) return FALSE;
     }

   else if (op == OP_ASSERT || op == OP_ASSERT_NA)
     {
     if (!is_anchored(scode, bracket_map, cb, atomcount, TRUE)) return FALSE;
     }

   else if (op == OP_COND || op == OP_SCOND)
     {
     if (scode[GET(scode,1)] != OP_ALT) return FALSE;
     if (!is_anchored(scode, bracket_map, cb, atomcount, inassert))
       return FALSE;
     }

   else if (op == OP_ONCE)
     {
     if (!is_anchored(scode, bracket_map, cb, atomcount + 1, inassert))
       return FALSE;
     }

   else if ((op == OP_TYPESTAR || op == OP_TYPEMINSTAR ||
             op == OP_TYPEPOSSTAR))
     {
     if (scode[1] != OP_ALLANY || (bracket_map & cb->backref_map) != 0 ||
         atomcount > 0 || cb->had_pruneorskip || inassert ||
         (cb->external_options & PCRE2_NO_DOTSTAR_ANCHOR) != 0)
       return FALSE;
     }

   else if (op != OP_SOD && op != OP_SOM && op != OP_CIRC) return FALSE;

   code += GET(code, 1);
   }
while (*code == OP_ALT);   /* Loop for each alternative */
return TRUE;
}

/*************************************************
*         Check for starting with ^ or .*        *
*************************************************/

static BOOL
is_startline(PCRE2_SPTR code, unsigned int bracket_map, compile_block *cb,
  int atomcount, BOOL inassert)
{
do {
   PCRE2_SPTR scode = first_significant_code(
     code + PRIV(OP_lengths)[*code], FALSE);
   int op = *scode;

   if (op == OP_COND)
     {
     scode += 1 + LINK_SIZE;

     if (*scode == OP_CALLOUT) scode += PRIV(OP_lengths)[OP_CALLOUT];
       else if (*scode == OP_CALLOUT_STR) scode += GET(scode, 1 + 2*LINK_SIZE);

     switch (*scode)
       {
       case OP_CREF:
       case OP_DNCREF:
       case OP_RREF:
       case OP_DNRREF:
       case OP_FAIL:
       case OP_FALSE:
       case OP_TRUE:
       return FALSE;

       default:     /* Assertion */
       if (!is_startline(scode, bracket_map, cb, atomcount, TRUE)) return FALSE;
       do scode += GET(scode, 1); while (*scode == OP_ALT);
       scode += 1 + LINK_SIZE;
       break;
       }
     scode = first_significant_code(scode, FALSE);
     op = *scode;
     }

   if (op == OP_BRA  || op == OP_BRAPOS ||
       op == OP_SBRA || op == OP_SBRAPOS)
     {
     if (!is_startline(scode, bracket_map, cb, atomcount, inassert))
       return FALSE;
     }

   else if (op == OP_CBRA  || op == OP_CBRAPOS ||
            op == OP_SCBRA || op == OP_SCBRAPOS)
     {
     int n = GET2(scode, 1+LINK_SIZE);
     int new_map = bracket_map | ((n < 32)? (1u << n) : 1);
     if (!is_startline(scode, new_map, cb, atomcount, inassert)) return FALSE;
     }

   else if (op == OP_ASSERT || op == OP_ASSERT_NA)
     {
     if (!is_startline(scode, bracket_map, cb, atomcount, TRUE))
       return FALSE;
     }

   else if (op == OP_ONCE)
     {
     if (!is_startline(scode, bracket_map, cb, atomcount + 1, inassert))
       return FALSE;
     }

   else if (op == OP_TYPESTAR || op == OP_TYPEMINSTAR || op == OP_TYPEPOSSTAR)
     {
     if (scode[1] != OP_ANY || (bracket_map & cb->backref_map) != 0 ||
         atomcount > 0 || cb->had_pruneorskip || inassert ||
         (cb->external_options & PCRE2_NO_DOTSTAR_ANCHOR) != 0)
       return FALSE;
     }

   else if (op != OP_CIRC && op != OP_CIRCM) return FALSE;

   code += GET(code, 1);
   }
while (*code == OP_ALT);  /* Loop for each alternative */
return TRUE;
}

/*************************************************
*   Scan compiled regex for recursion reference  *
*************************************************/

static PCRE2_SPTR
find_recurse(PCRE2_SPTR code, BOOL utf)
{
for (;;)
  {
  PCRE2_UCHAR c = *code;
  if (c == OP_END) return NULL;
  if (c == OP_RECURSE) return code;

  if (c == OP_XCLASS) code += GET(code, 1);
    else if (c == OP_CALLOUT_STR) code += GET(code, 1 + 2*LINK_SIZE);

  else
    {
    switch(c)
      {
      case OP_TYPESTAR:
      case OP_TYPEMINSTAR:
      case OP_TYPEPLUS:
      case OP_TYPEMINPLUS:
      case OP_TYPEQUERY:
      case OP_TYPEMINQUERY:
      case OP_TYPEPOSSTAR:
      case OP_TYPEPOSPLUS:
      case OP_TYPEPOSQUERY:
      if (code[1] == OP_PROP || code[1] == OP_NOTPROP) code += 2;
      break;

      case OP_TYPEPOSUPTO:
      case OP_TYPEUPTO:
      case OP_TYPEMINUPTO:
      case OP_TYPEEXACT:
      if (code[1 + IMM2_SIZE] == OP_PROP || code[1 + IMM2_SIZE] == OP_NOTPROP)
        code += 2;
      break;

      case OP_MARK:
      case OP_COMMIT_ARG:
      case OP_PRUNE_ARG:
      case OP_SKIP_ARG:
      case OP_THEN_ARG:
      code += code[1];
      break;
      }

    code += PRIV(OP_lengths)[c];

#ifdef MAYBE_UTF_MULTI
    if (utf) switch(c)
      {
      case OP_CHAR:
      case OP_CHARI:
      case OP_NOT:
      case OP_NOTI:
      case OP_EXACT:
      case OP_EXACTI:
      case OP_NOTEXACT:
      case OP_NOTEXACTI:
      case OP_UPTO:
      case OP_UPTOI:
      case OP_NOTUPTO:
      case OP_NOTUPTOI:
      case OP_MINUPTO:
      case OP_MINUPTOI:
      case OP_NOTMINUPTO:
      case OP_NOTMINUPTOI:
      case OP_POSUPTO:
      case OP_POSUPTOI:
      case OP_NOTPOSUPTO:
      case OP_NOTPOSUPTOI:
      case OP_STAR:
      case OP_STARI:
      case OP_NOTSTAR:
      case OP_NOTSTARI:
      case OP_MINSTAR:
      case OP_MINSTARI:
      case OP_NOTMINSTAR:
      case OP_NOTMINSTARI:
      case OP_POSSTAR:
      case OP_POSSTARI:
      case OP_NOTPOSSTAR:
      case OP_NOTPOSSTARI:
      case OP_PLUS:
      case OP_PLUSI:
      case OP_NOTPLUS:
      case OP_NOTPLUSI:
      case OP_MINPLUS:
      case OP_MINPLUSI:
      case OP_NOTMINPLUS:
      case OP_NOTMINPLUSI:
      case OP_POSPLUS:
      case OP_POSPLUSI:
      case OP_NOTPOSPLUS:
      case OP_NOTPOSPLUSI:
      case OP_QUERY:
      case OP_QUERYI:
      case OP_NOTQUERY:
      case OP_NOTQUERYI:
      case OP_MINQUERY:
      case OP_MINQUERYI:
      case OP_NOTMINQUERY:
      case OP_NOTMINQUERYI:
      case OP_POSQUERY:
      case OP_POSQUERYI:
      case OP_NOTPOSQUERY:
      case OP_NOTPOSQUERYI:
      if (HAS_EXTRALEN(code[-1])) code += GET_EXTRALEN(code[-1]);
      break;
      }
#else
    (void)(utf);  /* Keep compiler happy by referencing function argument */
#endif  /* MAYBE_UTF_MULTI */
    }
  }
}

/*************************************************
*    Check for asserted fixed first code unit    *
*************************************************/

static uint32_t
find_firstassertedcu(PCRE2_SPTR code, int32_t *flags, uint32_t inassert)
{
uint32_t c = 0;
int cflags = REQ_NONE;

*flags = REQ_NONE;
do {
   uint32_t d;
   int dflags;
   int xl = (*code == OP_CBRA || *code == OP_SCBRA ||
             *code == OP_CBRAPOS || *code == OP_SCBRAPOS)? IMM2_SIZE:0;
   PCRE2_SPTR scode = first_significant_code(code + 1+LINK_SIZE + xl, TRUE);
   PCRE2_UCHAR op = *scode;

   switch(op)
     {
     default:
     return 0;

     case OP_BRA:
     case OP_BRAPOS:
     case OP_CBRA:
     case OP_SCBRA:
     case OP_CBRAPOS:
     case OP_SCBRAPOS:
     case OP_ASSERT:
     case OP_ASSERT_NA:
     case OP_ONCE:
     case OP_SCRIPT_RUN:
     d = find_firstassertedcu(scode, &dflags, inassert +
       ((op == OP_ASSERT || op == OP_ASSERT_NA)?1:0));
     if (dflags < 0)
       return 0;
     if (cflags < 0) { c = d; cflags = dflags; }
       else if (c != d || cflags != dflags) return 0;
     break;

     case OP_EXACT:
     scode += IMM2_SIZE;
     /* Fall through */

     case OP_CHAR:
     case OP_PLUS:
     case OP_MINPLUS:
     case OP_POSPLUS:
     if (inassert == 0) return 0;
     if (cflags < 0) { c = scode[1]; cflags = 0; }
       else if (c != scode[1]) return 0;
     break;

     case OP_EXACTI:
     scode += IMM2_SIZE;
     /* Fall through */

     case OP_CHARI:
     case OP_PLUSI:
     case OP_MINPLUSI:
     case OP_POSPLUSI:
     if (inassert == 0) return 0;

#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
     if (scode[1] >= 0x80) return 0;
#elif PCRE2_CODE_UNIT_WIDTH == 16
     if (scode[1] >= 0xd800 && scode[1] <= 0xdfff) return 0;
#endif
#endif

     if (cflags < 0) { c = scode[1]; cflags = REQ_CASELESS; }
       else if (c != scode[1]) return 0;
     break;
     }

   code += GET(code, 1);
   }
while (*code == OP_ALT);

*flags = cflags;
return c;
}

/*************************************************
*     Add an entry to the name/number table      *
*************************************************/

static void
add_name_to_table(compile_block *cb, PCRE2_SPTR name, int length,
  unsigned int groupno, uint32_t tablecount)
{
uint32_t i;
PCRE2_UCHAR *slot = cb->name_table;

for (i = 0; i < tablecount; i++)
  {
  int crc = memcmp(name, slot+IMM2_SIZE, CU2BYTES(length));
  if (crc == 0 && slot[IMM2_SIZE+length] != 0)
    crc = -1; /* Current name is a substring */

  if (crc < 0)
    {
    (void)memmove(slot + cb->name_entry_size, slot,
      CU2BYTES((tablecount - i) * cb->name_entry_size));
    break;
    }

  slot += cb->name_entry_size;
  }

PUT2(slot, 0, groupno);
memcpy(slot + IMM2_SIZE, name, CU2BYTES(length));

memset(slot + IMM2_SIZE + length, 0,
  CU2BYTES(cb->name_entry_size - length - IMM2_SIZE));
}

/*************************************************
*             Skip in parsed pattern             *
*************************************************/

static uint32_t *
parsed_skip(uint32_t *pptr, uint32_t skiptype)
{
uint32_t nestlevel = 0;

for (;; pptr++)
  {
  uint32_t meta = META_CODE(*pptr);

  switch(meta)
    {
    default:  /* Just skip over most items */
    if (meta < META_END) continue;  /* Literal */
    break;

    case META_END:
    return NULL;

    case META_BACKREF:  /* Offset is present only if group >= 10 */
    if (META_DATA(*pptr) >= 10) pptr += SIZEOFFSET;
    break;

    case META_ESCAPE:   /* A few escapes are followed by data items. */
    switch (META_DATA(*pptr))
      {
      case ESC_P:
      case ESC_p:
      pptr += 1;
      break;

      case ESC_g:
      case ESC_k:
      pptr += 1 + SIZEOFFSET;
      break;
      }
    break;

    case META_MARK:     /* Add the length of the name. */
    case META_COMMIT_ARG:
    case META_PRUNE_ARG:
    case META_SKIP_ARG:
    case META_THEN_ARG:
    pptr += pptr[1];
    break;

    case META_CLASS_END:
    if (skiptype == PSKIP_CLASS) return pptr;
    break;

    case META_ATOMIC:
    case META_CAPTURE:
    case META_COND_ASSERT:
    case META_COND_DEFINE:
    case META_COND_NAME:
    case META_COND_NUMBER:
    case META_COND_RNAME:
    case META_COND_RNUMBER:
    case META_COND_VERSION:
    case META_LOOKAHEAD:
    case META_LOOKAHEADNOT:
    case META_LOOKAHEAD_NA:
    case META_LOOKBEHIND:
    case META_LOOKBEHINDNOT:
    case META_LOOKBEHIND_NA:
    case META_NOCAPTURE:
    case META_SCRIPT_RUN:
    nestlevel++;
    break;

    case META_ALT:
    if (nestlevel == 0 && skiptype == PSKIP_ALT) return pptr;
    break;

    case META_KET:
    if (nestlevel == 0) return pptr;
    nestlevel--;
    break;
    }

  meta = (meta >> 16) & 0x7fff;
  if (meta >= sizeof(meta_extra_lengths)) return NULL;
  pptr += meta_extra_lengths[meta];
  }
/* Control never reaches here */
return pptr;
}

/*************************************************
*       Find length of a parsed group            *
*************************************************/

static int
get_grouplength(uint32_t **pptrptr, BOOL isinline, int *errcodeptr, int *lcptr,
   int group, parsed_recurse_check *recurses, compile_block *cb)
{
int branchlength;
int grouplength = -1;

if (group > 0 && (cb->external_flags & PCRE2_DUPCAPUSED) == 0)
  {
  uint32_t groupinfo = cb->groupinfo[group];
  if ((groupinfo & GI_NOT_FIXED_LENGTH) != 0) return -1;
  if ((groupinfo & GI_SET_FIXED_LENGTH) != 0)
    {
    if (isinline) *pptrptr = parsed_skip(*pptrptr, PSKIP_KET);
    return groupinfo & GI_FIXED_LENGTH_MASK;
    }
  }

for(;;)
  {
  branchlength = get_branchlength(pptrptr, errcodeptr, lcptr, recurses, cb);
  if (branchlength < 0) goto ISNOTFIXED;
  if (grouplength == -1) grouplength = branchlength;
    else if (grouplength != branchlength) goto ISNOTFIXED;
  if (**pptrptr == META_KET) break;
  *pptrptr += 1;   /* Skip META_ALT */
  }

if (group > 0)
  cb->groupinfo[group] |= (uint32_t)(GI_SET_FIXED_LENGTH | grouplength);
return grouplength;

ISNOTFIXED:
if (group > 0) cb->groupinfo[group] |= GI_NOT_FIXED_LENGTH;
return -1;
}

/*************************************************
*        Find length of a parsed branch          *
*************************************************/

static int
get_branchlength(uint32_t **pptrptr, int *errcodeptr, int *lcptr,
  parsed_recurse_check *recurses, compile_block *cb)
{
int branchlength = 0;
int grouplength;
uint32_t lastitemlength = 0;
uint32_t *pptr = *pptrptr;
PCRE2_SIZE offset;
parsed_recurse_check this_recurse;

if ((*lcptr)++ > 2000)
  {
  *errcodeptr = ERR35;  /* Lookbehind is too complicated */
  return -1;
  }

for (;; pptr++)
  {
  parsed_recurse_check *r;
  uint32_t *gptr, *gptrend;
  uint32_t escape;
  uint32_t group = 0;
  uint32_t itemlength = 0;

  if (*pptr < META_END)
    {
    itemlength = 1;
    }

  else switch (META_CODE(*pptr))
    {
    case META_KET:
    case META_ALT:
    goto EXIT;

    case META_ACCEPT:
    case META_FAIL:
    pptr = parsed_skip(pptr, PSKIP_ALT);
    if (pptr == NULL) goto PARSED_SKIP_FAILED;
    goto EXIT;

    case META_MARK:
    case META_COMMIT_ARG:
    case META_PRUNE_ARG:
    case META_SKIP_ARG:
    case META_THEN_ARG:
    pptr += pptr[1] + 1;
    break;

    case META_CIRCUMFLEX:
    case META_COMMIT:
    case META_DOLLAR:
    case META_PRUNE:
    case META_SKIP:
    case META_THEN:
    break;

    case META_OPTIONS:
    pptr += 1;
    break;

    case META_BIGVALUE:
    itemlength = 1;
    pptr += 1;
    break;

    case META_CLASS:
    case META_CLASS_NOT:
    itemlength = 1;
    pptr = parsed_skip(pptr, PSKIP_CLASS);
    if (pptr == NULL) goto PARSED_SKIP_FAILED;
    break;

    case META_CLASS_EMPTY_NOT:
    case META_DOT:
    itemlength = 1;
    break;

    case META_CALLOUT_NUMBER:
    pptr += 3;
    break;

    case META_CALLOUT_STRING:
    pptr += 3 + SIZEOFFSET;
    break;

    case META_ESCAPE:
    escape = META_DATA(*pptr);
    if (escape == ESC_R || escape == ESC_X) return -1;
    if (escape > ESC_b && escape < ESC_Z)
      {
#if PCRE2_CODE_UNIT_WIDTH != 32
      if ((cb->external_options & PCRE2_UTF) != 0 && escape == ESC_C)
        {
        *errcodeptr = ERR36;
        return -1;
        }
#endif
      itemlength = 1;
      if (escape == ESC_p || escape == ESC_P) pptr++;  /* Skip prop data */
      }
    break;

    case META_LOOKAHEAD:
    case META_LOOKAHEADNOT:
    case META_LOOKAHEAD_NA:
    *errcodeptr = check_lookbehinds(pptr + 1, &pptr, recurses, cb);
    if (*errcodeptr != 0) return -1;

    switch (pptr[1])
      {
      case META_ASTERISK:
      case META_ASTERISK_PLUS:
      case META_ASTERISK_QUERY:
      case META_PLUS:
      case META_PLUS_PLUS:
      case META_PLUS_QUERY:
      case META_QUERY:
      case META_QUERY_PLUS:
      case META_QUERY_QUERY:
      pptr++;
      break;

      case META_MINMAX:
      case META_MINMAX_PLUS:
      case META_MINMAX_QUERY:
      pptr += 3;
      break;

      default:
      break;
      }
    break;

    case META_LOOKBEHIND:
    case META_LOOKBEHINDNOT:
    case META_LOOKBEHIND_NA:
    if (!set_lookbehind_lengths(&pptr, errcodeptr, lcptr, recurses, cb))
      return -1;
    break;

    case META_BACKREF_BYNAME:
    if ((cb->external_options & PCRE2_MATCH_UNSET_BACKREF) != 0)
      goto ISNOTFIXED;
    /* Fall through */

    case META_RECURSE_BYNAME:
      {
      int i;
      PCRE2_SPTR name;
      BOOL is_dupname = FALSE;
      named_group *ng = cb->named_groups;
      uint32_t meta_code = META_CODE(*pptr);
      uint32_t length = *(++pptr);

      GETPLUSOFFSET(offset, pptr);
      name = cb->start_pattern + offset;
      for (i = 0; i < cb->names_found; i++, ng++)
        {
        if (length == ng->length && PRIV(strncmp)(name, ng->name, length) == 0)
          {
          group = ng->number;
          is_dupname = ng->isdup;
          break;
          }
        }

      if (group == 0)
        {
        *errcodeptr = ERR15;  /* Non-existent subpattern */
        cb->erroroffset = offset;
        return -1;
        }

      if (meta_code == META_RECURSE_BYNAME ||
          (!is_dupname && (cb->external_flags & PCRE2_DUPCAPUSED) == 0))
        goto RECURSE_OR_BACKREF_LENGTH;  /* Handle as a numbered version. */
      }
    goto ISNOTFIXED;                     /* Duplicate name or number */

    case META_BACKREF:
    if ((cb->external_options & PCRE2_MATCH_UNSET_BACKREF) != 0 ||
        (cb->external_flags & PCRE2_DUPCAPUSED) != 0)
      goto ISNOTFIXED;
    group = META_DATA(*pptr);
    if (group < 10)
      {
      offset = cb->small_ref_offset[group];
      goto RECURSE_OR_BACKREF_LENGTH;
      }

    case META_RECURSE:
    group = META_DATA(*pptr);
    GETPLUSOFFSET(offset, pptr);

    RECURSE_OR_BACKREF_LENGTH:
    if (group > cb->bracount)
      {
      cb->erroroffset = offset;
      *errcodeptr = ERR15;  /* Non-existent subpattern */
      return -1;
      }
    if (group == 0) goto ISNOTFIXED;  /* Local recursion */
    for (gptr = cb->parsed_pattern; *gptr != META_END; gptr++)
      {
      if (META_CODE(*gptr) == META_BIGVALUE) gptr++;
        else if (*gptr == (META_CAPTURE | group)) break;
      }

    gptrend = parsed_skip(gptr + 1, PSKIP_KET);
    if (gptrend == NULL) goto PARSED_SKIP_FAILED;
    if (pptr > gptr && pptr < gptrend) goto ISNOTFIXED;  /* Local recursion */
    for (r = recurses; r != NULL; r = r->prev) if (r->groupptr == gptr) break;
    if (r != NULL) goto ISNOTFIXED;   /* Mutual recursion */
    this_recurse.prev = recurses;
    this_recurse.groupptr = gptr;

    gptr++;
    grouplength = get_grouplength(&gptr, FALSE, errcodeptr, lcptr, group,
      &this_recurse, cb);
    if (grouplength < 0)
      {
      if (*errcodeptr == 0) goto ISNOTFIXED;
      return -1;  /* Error already set */
      }
    itemlength = grouplength;
    break;

    case META_COND_DEFINE:
    pptr = parsed_skip(pptr + 1, PSKIP_KET);
    break;

    case META_COND_NAME:
    case META_COND_NUMBER:
    case META_COND_RNAME:
    case META_COND_RNUMBER:
    pptr += 2 + SIZEOFFSET;
    goto CHECK_GROUP;

    case META_COND_ASSERT:
    pptr += 1;
    goto CHECK_GROUP;

    case META_COND_VERSION:
    pptr += 4;
    goto CHECK_GROUP;

    case META_CAPTURE:
    group = META_DATA(*pptr);
    /* Fall through */

    case META_ATOMIC:
    case META_NOCAPTURE:
    case META_SCRIPT_RUN:
    pptr++;
    CHECK_GROUP:
    grouplength = get_grouplength(&pptr, TRUE, errcodeptr, lcptr, group,
      recurses, cb);
    if (grouplength < 0) return -1;
    itemlength = grouplength;
    break;

    case META_MINMAX:
    case META_MINMAX_PLUS:
    case META_MINMAX_QUERY:
    if (pptr[1] == pptr[2])
      {
      switch(pptr[1])
        {
        case 0:
        branchlength -= lastitemlength;
        break;

        case 1:
        itemlength = 0;
        break;

        default:  /* Check for integer overflow */
        if (lastitemlength != 0 &&  /* Should not occur, but just in case */
            INT_MAX/lastitemlength < pptr[1] - 1)
          {
          *errcodeptr = ERR87;  /* Integer overflow; lookbehind too big */
          return -1;
          }
        itemlength = (pptr[1] - 1) * lastitemlength;
        break;
        }
      pptr += 2;
      break;
      }

    default:
    ISNOTFIXED:
    *errcodeptr = ERR25;   /* Not fixed length */
    return -1;
    }

  if (INT_MAX - branchlength < (int)itemlength ||
      (branchlength += itemlength) > LOOKBEHIND_MAX)
    {
    *errcodeptr = ERR87;
    return -1;
    }

  lastitemlength = itemlength;
  }

EXIT:
*pptrptr = pptr;
return branchlength;

PARSED_SKIP_FAILED:
*errcodeptr = ERR90;
return -1;
}

/*************************************************
*        Set lengths in a lookbehind             *
*************************************************/

static BOOL
set_lookbehind_lengths(uint32_t **pptrptr, int *errcodeptr, int *lcptr,
  parsed_recurse_check *recurses, compile_block *cb)
{
PCRE2_SIZE offset;
int branchlength;
uint32_t *bptr = *pptrptr;

READPLUSOFFSET(offset, bptr);  /* Offset for error messages */
*pptrptr += SIZEOFFSET;

do
  {
  *pptrptr += 1;
  branchlength = get_branchlength(pptrptr, errcodeptr, lcptr, recurses, cb);
  if (branchlength < 0)
    {
    /* The errorcode and offset may already be set from a nested lookbehind. */
    if (*errcodeptr == 0) *errcodeptr = ERR25;
    if (cb->erroroffset == PCRE2_UNSET) cb->erroroffset = offset;
    return FALSE;
    }
  if (branchlength > cb->max_lookbehind) cb->max_lookbehind = branchlength;
  *bptr |= branchlength;  /* branchlength never more than 65535 */
  bptr = *pptrptr;
  }
while (*bptr == META_ALT);

return TRUE;
}

/*************************************************
*         Check parsed pattern lookbehinds       *
*************************************************/

static int
check_lookbehinds(uint32_t *pptr, uint32_t **retptr,
  parsed_recurse_check *recurses, compile_block *cb)
{
int errorcode = 0;
int loopcount = 0;
int nestlevel = 0;

cb->erroroffset = PCRE2_UNSET;

for (; *pptr != META_END; pptr++)
  {
  if (*pptr < META_END) continue;  /* Literal */

  switch (META_CODE(*pptr))
    {
    default:
    return ERR70;  /* Unrecognized meta code */

    case META_ESCAPE:
    if (*pptr - META_ESCAPE == ESC_P || *pptr - META_ESCAPE == ESC_p)
      pptr += 1;
    break;

    case META_KET:
    if (--nestlevel < 0)
      {
      if (retptr != NULL) *retptr = pptr;
      return 0;
      }
    break;

    case META_ATOMIC:
    case META_CAPTURE:
    case META_COND_ASSERT:
    case META_LOOKAHEAD:
    case META_LOOKAHEADNOT:
    case META_LOOKAHEAD_NA:
    case META_NOCAPTURE:
    case META_SCRIPT_RUN:
    nestlevel++;
    break;

    case META_ACCEPT:
    case META_ALT:
    case META_ASTERISK:
    case META_ASTERISK_PLUS:
    case META_ASTERISK_QUERY:
    case META_BACKREF:
    case META_CIRCUMFLEX:
    case META_CLASS:
    case META_CLASS_EMPTY:
    case META_CLASS_EMPTY_NOT:
    case META_CLASS_END:
    case META_CLASS_NOT:
    case META_COMMIT:
    case META_DOLLAR:
    case META_DOT:
    case META_FAIL:
    case META_PLUS:
    case META_PLUS_PLUS:
    case META_PLUS_QUERY:
    case META_PRUNE:
    case META_QUERY:
    case META_QUERY_PLUS:
    case META_QUERY_QUERY:
    case META_RANGE_ESCAPED:
    case META_RANGE_LITERAL:
    case META_SKIP:
    case META_THEN:
    break;

    case META_RECURSE:
    pptr += SIZEOFFSET;
    break;

    case META_BACKREF_BYNAME:
    case META_RECURSE_BYNAME:
    pptr += 1 + SIZEOFFSET;
    break;

    case META_COND_DEFINE:
    pptr += SIZEOFFSET;
    nestlevel++;
    break;

    case META_COND_NAME:
    case META_COND_NUMBER:
    case META_COND_RNAME:
    case META_COND_RNUMBER:
    pptr += 1 + SIZEOFFSET;
    nestlevel++;
    break;

    case META_COND_VERSION:
    pptr += 3;
    nestlevel++;
    break;

    case META_CALLOUT_STRING:
    pptr += 3 + SIZEOFFSET;
    break;

    case META_BIGVALUE:
    case META_OPTIONS:
    case META_POSIX:
    case META_POSIX_NEG:
    pptr += 1;
    break;

    case META_MINMAX:
    case META_MINMAX_QUERY:
    case META_MINMAX_PLUS:
    pptr += 2;
    break;

    case META_CALLOUT_NUMBER:
    pptr += 3;
    break;

    case META_MARK:
    case META_COMMIT_ARG:
    case META_PRUNE_ARG:
    case META_SKIP_ARG:
    case META_THEN_ARG:
    pptr += 1 + pptr[1];
    break;

    case META_LOOKBEHIND:
    case META_LOOKBEHINDNOT:
    case META_LOOKBEHIND_NA:
    if (!set_lookbehind_lengths(&pptr, &errorcode, &loopcount, recurses, cb))
      return errorcode;
    break;
    }
  }

return 0;
}

/*************************************************
*     External function to compile a pattern     *
*************************************************/

PCRE2_EXP_DEFN pcre2_code * PCRE2_CALL_CONVENTION
pcre2_compile(PCRE2_SPTR pattern, PCRE2_SIZE patlen, uint32_t options,
   int *errorptr, PCRE2_SIZE *erroroffset, pcre2_compile_context *ccontext)
{
BOOL utf;                             /* Set TRUE for UTF mode */
BOOL ucp;                             /* Set TRUE for UCP mode */
BOOL has_lookbehind = FALSE;          /* Set TRUE if a lookbehind is found */
BOOL zero_terminated;                 /* Set TRUE for zero-terminated pattern */
pcre2_real_code *re = NULL;           /* What we will return */
compile_block cb;                     /* "Static" compile-time data */
const uint8_t *tables;                /* Char tables base pointer */

PCRE2_UCHAR *code;                    /* Current pointer in compiled code */
PCRE2_SPTR codestart;                 /* Start of compiled code */
PCRE2_SPTR ptr;                       /* Current pointer in pattern */
uint32_t *pptr;                       /* Current pointer in parsed pattern */

PCRE2_SIZE length = 1;                /* Allow for final END opcode */
PCRE2_SIZE usedlength;                /* Actual length used */
PCRE2_SIZE re_blocksize;              /* Size of memory block */
PCRE2_SIZE big32count = 0;            /* 32-bit literals >= 0x80000000 */
PCRE2_SIZE parsed_size_needed;        /* Needed for parsed pattern */

int32_t firstcuflags, reqcuflags;     /* Type of first/req code unit */
uint32_t firstcu, reqcu;              /* Value of first/req code unit */
uint32_t setflags = 0;                /* NL and BSR set flags */

uint32_t skipatstart;                 /* When checking (*UTF) etc */
uint32_t limit_heap  = UINT32_MAX;
uint32_t limit_match = UINT32_MAX;    /* Unset match limits */
uint32_t limit_depth = UINT32_MAX;

int newline = 0;                      /* Unset; can be set by the pattern */
int bsr = 0;                          /* Unset; can be set by the pattern */
int errorcode = 0;                    /* Initialize to avoid compiler warn */
int regexrc;                          /* Return from compile */

uint32_t i;                           /* Local loop counter */

uint32_t stack_groupinfo[GROUPINFO_DEFAULT_SIZE];
uint32_t stack_parsed_pattern[PARSED_PATTERN_DEFAULT_SIZE];
named_group named_groups[NAMED_GROUP_LIST_SIZE];

uint32_t c16workspace[C16_WORK_SIZE];
PCRE2_UCHAR *cworkspace = (PCRE2_UCHAR *)c16workspace;

if (errorptr == NULL || erroroffset == NULL) return NULL;
*errorptr = ERR0;
*erroroffset = 0;

if (pattern == NULL)
  {
  *errorptr = ERR16;
  return NULL;
  }

if (ccontext == NULL)
  ccontext = (pcre2_compile_context *)(&PRIV(default_compile_context));

/* PCRE2_MATCH_INVALID_UTF implies UTF */

if ((options & PCRE2_MATCH_INVALID_UTF) != 0) options |= PCRE2_UTF;

/* Check that all undefined public option bits are zero. */

if ((options & ~PUBLIC_COMPILE_OPTIONS) != 0 ||
    (ccontext->extra_options & ~PUBLIC_COMPILE_EXTRA_OPTIONS) != 0)
  {
  *errorptr = ERR17;
  return NULL;
  }

if ((options & PCRE2_LITERAL) != 0 &&
    ((options & ~PUBLIC_LITERAL_COMPILE_OPTIONS) != 0 ||
     (ccontext->extra_options & ~PUBLIC_LITERAL_COMPILE_EXTRA_OPTIONS) != 0))
  {
  *errorptr = ERR92;
  return NULL;
  }

if ((zero_terminated = (patlen == PCRE2_ZERO_TERMINATED)))
  patlen = PRIV(strlen)(pattern);

if (patlen > ccontext->max_pattern_length)
  {
  *errorptr = ERR88;
  return NULL;
  }

tables = (ccontext->tables != NULL)? ccontext->tables : PRIV(default_tables);

cb.lcc = tables + lcc_offset;          /* Individual */
cb.fcc = tables + fcc_offset;          /*   character */
cb.cbits = tables + cbits_offset;      /*      tables */
cb.ctypes = tables + ctypes_offset;

cb.assert_depth = 0;
cb.bracount = 0;
cb.cx = ccontext;
cb.dupnames = FALSE;
cb.end_pattern = pattern + patlen;
cb.erroroffset = 0;
cb.external_flags = 0;
cb.external_options = options;
cb.groupinfo = stack_groupinfo;
cb.had_recurse = FALSE;
cb.lastcapture = 0;
cb.max_lookbehind = 0;
cb.name_entry_size = 0;
cb.name_table = NULL;
cb.named_groups = named_groups;
cb.named_group_list_size = NAMED_GROUP_LIST_SIZE;
cb.names_found = 0;
cb.open_caps = NULL;
cb.parens_depth = 0;
cb.parsed_pattern = stack_parsed_pattern;
cb.req_varyopt = 0;
cb.start_code = cworkspace;
cb.start_pattern = pattern;
cb.start_workspace = cworkspace;
cb.workspace_size = COMPILE_WORK_SIZE;

cb.top_backref = 0;
cb.backref_map = 0;

for (i = 0; i < 10; i++) cb.small_ref_offset[i] = PCRE2_UNSET;

#ifdef SUPPORT_VALGRIND
if (zero_terminated) VALGRIND_MAKE_MEM_NOACCESS(pattern + patlen, CU2BYTES(1));
#endif

ptr = pattern;
skipatstart = 0;

if ((options & PCRE2_LITERAL) == 0)
  {
  while (patlen - skipatstart >= 2 &&
         ptr[skipatstart] == CHAR_LEFT_PARENTHESIS &&
         ptr[skipatstart+1] == CHAR_ASTERISK)
    {
    for (i = 0; i < sizeof(pso_list)/sizeof(pso); i++)
      {
      uint32_t c, pp;
      pso *p = pso_list + i;

      if (patlen - skipatstart - 2 >= p->length &&
          PRIV(strncmp_c8)(ptr + skipatstart + 2, (char *)(p->name),
            p->length) == 0)
        {
        skipatstart += p->length + 2;
        switch(p->type)
          {
          case PSO_OPT:
          cb.external_options |= p->value;
          break;

          case PSO_FLG:
          setflags |= p->value;
          break;

          case PSO_NL:
          newline = p->value;
          setflags |= PCRE2_NL_SET;
          break;

          case PSO_BSR:
          bsr = p->value;
          setflags |= PCRE2_BSR_SET;
          break;

          case PSO_LIMM:
          case PSO_LIMD:
          case PSO_LIMH:
          c = 0;
          pp = skipatstart;
          if (!IS_DIGIT(ptr[pp]))
            {
            errorcode = ERR60;
            ptr += pp;
            goto HAD_EARLY_ERROR;
            }
          while (IS_DIGIT(ptr[pp]))
            {
            if (c > UINT32_MAX / 10 - 1) break;   /* Integer overflow */
            c = c*10 + (ptr[pp++] - CHAR_0);
            }
          if (ptr[pp++] != CHAR_RIGHT_PARENTHESIS)
            {
            errorcode = ERR60;
            ptr += pp;
            goto HAD_EARLY_ERROR;
            }
          if (p->type == PSO_LIMH) limit_heap = c;
            else if (p->type == PSO_LIMM) limit_match = c;
            else limit_depth = c;
          skipatstart += pp - skipatstart;
          break;
          }
        break;   /* Out of the table scan loop */
        }
      }
    if (i >= sizeof(pso_list)/sizeof(pso)) break;   /* Out of pso loop */
    }
  }

ptr += skipatstart;

#ifndef SUPPORT_UNICODE
if ((cb.external_options & (PCRE2_UTF|PCRE2_UCP)) != 0)
  {
  errorcode = ERR32;
  goto HAD_EARLY_ERROR;
  }
#endif

utf = (cb.external_options & PCRE2_UTF) != 0;
if (utf)
  {
  if ((options & PCRE2_NEVER_UTF) != 0)
    {
    errorcode = ERR74;
    goto HAD_EARLY_ERROR;
    }
  if ((options & PCRE2_NO_UTF_CHECK) == 0 &&
       (errorcode = PRIV(valid_utf)(pattern, patlen, erroroffset)) != 0)
    goto HAD_ERROR;  /* Offset was set by valid_utf() */

#if PCRE2_CODE_UNIT_WIDTH == 16
  if ((ccontext->extra_options & PCRE2_EXTRA_ALLOW_SURROGATE_ESCAPES) != 0)
    {
    errorcode = ERR91;
    goto HAD_EARLY_ERROR;
    }
#endif
  }

ucp = (cb.external_options & PCRE2_UCP) != 0;
if (ucp && (cb.external_options & PCRE2_NEVER_UCP) != 0)
  {
  errorcode = ERR75;
  goto HAD_EARLY_ERROR;
  }

if (bsr == 0) bsr = ccontext->bsr_convention;

if (newline == 0) newline = ccontext->newline_convention;
cb.nltype = NLTYPE_FIXED;
switch(newline)
  {
  case PCRE2_NEWLINE_CR:
  cb.nllen = 1;
  cb.nl[0] = CHAR_CR;
  break;

  case PCRE2_NEWLINE_LF:
  cb.nllen = 1;
  cb.nl[0] = CHAR_NL;
  break;

  case PCRE2_NEWLINE_NUL:
  cb.nllen = 1;
  cb.nl[0] = CHAR_NUL;
  break;

  case PCRE2_NEWLINE_CRLF:
  cb.nllen = 2;
  cb.nl[0] = CHAR_CR;
  cb.nl[1] = CHAR_NL;
  break;

  case PCRE2_NEWLINE_ANY:
  cb.nltype = NLTYPE_ANY;
  break;

  case PCRE2_NEWLINE_ANYCRLF:
  cb.nltype = NLTYPE_ANYCRLF;
  break;

  default:
  errorcode = ERR56;
  goto HAD_EARLY_ERROR;
  }

#if PCRE2_CODE_UNIT_WIDTH == 32
if (!utf)
  {
  PCRE2_SPTR p;
  for (p = ptr; p < cb.end_pattern; p++) if (*p >= META_END) big32count++;
  }
#endif

parsed_size_needed = patlen - skipatstart + big32count;

if ((ccontext->extra_options &
     (PCRE2_EXTRA_MATCH_WORD|PCRE2_EXTRA_MATCH_LINE)) != 0)
  parsed_size_needed += 4;

if ((options & PCRE2_AUTO_CALLOUT) != 0)
  parsed_size_needed = (parsed_size_needed + 1) * 5;

if (parsed_size_needed >= PARSED_PATTERN_DEFAULT_SIZE)
  {
  uint32_t *heap_parsed_pattern = ccontext->memctl.malloc(
    (parsed_size_needed + 1) * sizeof(uint32_t), ccontext->memctl.memory_data);
  if (heap_parsed_pattern == NULL)
    {
    *errorptr = ERR21;
    goto EXIT;
    }
  cb.parsed_pattern = heap_parsed_pattern;
  }
cb.parsed_pattern_end = cb.parsed_pattern + parsed_size_needed + 1;

errorcode = parse_regex(ptr, cb.external_options, &has_lookbehind, &cb);
if (errorcode != 0) goto HAD_CB_ERROR;

if (cb.bracount >= GROUPINFO_DEFAULT_SIZE)
  {
  cb.groupinfo = ccontext->memctl.malloc(
    (cb.bracount + 1)*sizeof(uint32_t), ccontext->memctl.memory_data);
  if (cb.groupinfo == NULL)
    {
    errorcode = ERR21;
    cb.erroroffset = 0;
    goto HAD_CB_ERROR;
    }
  }
memset(cb.groupinfo, 0, (cb.bracount + 1) * sizeof(uint32_t));

if (has_lookbehind)
  {
  errorcode = check_lookbehinds(cb.parsed_pattern, NULL, NULL, &cb);
  if (errorcode != 0) goto HAD_CB_ERROR;
  }

#ifdef DEBUG_SHOW_PARSED
fprintf(stderr, "+++ Pre-scan complete:\n");
show_parsed(&cb);
#endif

#ifdef DEBUG_SHOW_CAPTURES
  {
  named_group *ng = cb.named_groups;
  fprintf(stderr, "+++Captures: %d\n", cb.bracount);
  for (i = 0; i < cb.names_found; i++, ng++)
    {
    fprintf(stderr, "+++%3d %.*s\n", ng->number, ng->length, ng->name);
    }
  }
#endif

cb.erroroffset = patlen;   /* For any subsequent errors that do not set it */
pptr = cb.parsed_pattern;
code = cworkspace;
*code = OP_BRA;

(void)compile_regex(cb.external_options, &code, &pptr, &errorcode, 0, &firstcu,
   &firstcuflags, &reqcu, &reqcuflags, NULL, &cb, &length);

if (errorcode != 0) goto HAD_CB_ERROR;  /* Offset is in cb.erroroffset */

if (length > MAX_PATTERN_SIZE)
  {
  errorcode = ERR20;
  goto HAD_CB_ERROR;
  }

re_blocksize = sizeof(pcre2_real_code) +
  CU2BYTES(length +
  (PCRE2_SIZE)cb.names_found * (PCRE2_SIZE)cb.name_entry_size);
re = (pcre2_real_code *)
  ccontext->memctl.malloc(re_blocksize, ccontext->memctl.memory_data);
if (re == NULL)
  {
  errorcode = ERR21;
  goto HAD_CB_ERROR;
  }

memset((char *)re + sizeof(pcre2_real_code) - 8, 0, 8);
re->memctl = ccontext->memctl;
re->tables = tables;
re->executable_jit = NULL;
memset(re->start_bitmap, 0, 32 * sizeof(uint8_t));
re->blocksize = re_blocksize;
re->magic_number = MAGIC_NUMBER;
re->compile_options = options;
re->overall_options = cb.external_options;
re->extra_options = ccontext->extra_options;
re->flags = PCRE2_CODE_UNIT_WIDTH/8 | cb.external_flags | setflags;
re->limit_heap = limit_heap;
re->limit_match = limit_match;
re->limit_depth = limit_depth;
re->first_codeunit = 0;
re->last_codeunit = 0;
re->bsr_convention = bsr;
re->newline_convention = newline;
re->max_lookbehind = 0;
re->minlength = 0;
re->top_bracket = 0;
re->top_backref = 0;
re->name_entry_size = cb.name_entry_size;
re->name_count = cb.names_found;

codestart = (PCRE2_SPTR)((uint8_t *)re + sizeof(pcre2_real_code)) +
  re->name_entry_size * re->name_count;

cb.parens_depth = 0;
cb.assert_depth = 0;
cb.lastcapture = 0;
cb.name_table = (PCRE2_UCHAR *)((uint8_t *)re + sizeof(pcre2_real_code));
cb.start_code = codestart;
cb.req_varyopt = 0;
cb.had_accept = FALSE;
cb.had_pruneorskip = FALSE;
cb.open_caps = NULL;

if (cb.names_found > 0)
  {
  named_group *ng = cb.named_groups;
  for (i = 0; i < cb.names_found; i++, ng++)
    add_name_to_table(&cb, ng->name, ng->length, ng->number, i);
  }

pptr = cb.parsed_pattern;
code = (PCRE2_UCHAR *)codestart;
*code = OP_BRA;
regexrc = compile_regex(re->overall_options, &code, &pptr, &errorcode, 0,
  &firstcu, &firstcuflags, &reqcu, &reqcuflags, NULL, &cb, NULL);
if (regexrc < 0) re->flags |= PCRE2_MATCH_EMPTY;
re->top_bracket = cb.bracount;
re->top_backref = cb.top_backref;
re->max_lookbehind = cb.max_lookbehind;

if (cb.had_accept)
  {
  reqcu = 0;                     /* Must disable after (*ACCEPT) */
  reqcuflags = REQ_NONE;
  re->flags |= PCRE2_HASACCEPT;  /* Disables minimum length */
  }

*code++ = OP_END;
usedlength = code - codestart;
if (usedlength > length) errorcode = ERR23; else
  {
  re->blocksize -= CU2BYTES(length - usedlength);
#ifdef SUPPORT_VALGRIND
  VALGRIND_MAKE_MEM_NOACCESS(code, CU2BYTES(length - usedlength));
#endif
  }

#define RSCAN_CACHE_SIZE 8

if (errorcode == 0 && cb.had_recurse)
  {
  PCRE2_UCHAR *rcode;
  PCRE2_SPTR rgroup;
  unsigned int ccount = 0;
  int start = RSCAN_CACHE_SIZE;
  recurse_cache rc[RSCAN_CACHE_SIZE];

  for (rcode = (PCRE2_UCHAR *)find_recurse(codestart, utf);
       rcode != NULL;
       rcode = (PCRE2_UCHAR *)find_recurse(rcode + 1 + LINK_SIZE, utf))
    {
    int p, groupnumber;

    groupnumber = (int)GET(rcode, 1);
    if (groupnumber == 0) rgroup = codestart; else
      {
      PCRE2_SPTR search_from = codestart;
      rgroup = NULL;
      for (i = 0, p = start; i < ccount; i++, p = (p + 1) & 7)
        {
        if (groupnumber == rc[p].groupnumber)
          {
          rgroup = rc[p].group;
          break;
          }

        if (groupnumber > rc[p].groupnumber) search_from = rc[p].group;
        }

      if (rgroup == NULL)
        {
        rgroup = PRIV(find_bracket)(search_from, utf, groupnumber);
        if (rgroup == NULL)
          {
          errorcode = ERR53;
          break;
          }
        if (--start < 0) start = RSCAN_CACHE_SIZE - 1;
        rc[start].groupnumber = groupnumber;
        rc[start].group = rgroup;
        if (ccount < RSCAN_CACHE_SIZE) ccount++;
        }
      }

    PUT(rcode, 1, rgroup - codestart);
    }
  }

#ifdef DEBUG_CALL_PRINTINT
pcre2_printint(re, stderr, TRUE);
fprintf(stderr, "Length=%lu Used=%lu\n", length, usedlength);
#endif

if (errorcode == 0 && (re->overall_options & PCRE2_NO_AUTO_POSSESS) == 0)
  {
  PCRE2_UCHAR *temp = (PCRE2_UCHAR *)codestart;
  if (PRIV(auto_possessify)(temp, &cb) != 0) errorcode = ERR80;
  }

if (errorcode != 0) goto HAD_CB_ERROR;

if ((re->overall_options & PCRE2_ANCHORED) == 0 &&
     is_anchored(codestart, 0, &cb, 0, FALSE))
  re->overall_options |= PCRE2_ANCHORED;

if ((re->overall_options & PCRE2_NO_START_OPTIMIZE) == 0)
  {
  int minminlength = 0;  /* For minimal minlength from first/required CU */

  if (firstcuflags < 0)
    firstcu = find_firstassertedcu(codestart, &firstcuflags, 0);

  if (firstcuflags >= 0)
    {
    re->first_codeunit = firstcu;
    re->flags |= PCRE2_FIRSTSET;
    minminlength++;

    if ((firstcuflags & REQ_CASELESS) != 0)
      {
      if (firstcu < 128 || (!utf && !ucp && firstcu < 255))
        {
        if (cb.fcc[firstcu] != firstcu) re->flags |= PCRE2_FIRSTCASELESS;
        }

#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
      else if (ucp && !utf && UCD_OTHERCASE(firstcu) != firstcu)
        re->flags |= PCRE2_FIRSTCASELESS;
#else
      else if ((utf || ucp) && firstcu <= MAX_UTF_CODE_POINT &&
               UCD_OTHERCASE(firstcu) != firstcu)
        re->flags |= PCRE2_FIRSTCASELESS;
#endif
#endif  /* SUPPORT_UNICODE */
      }
    }

  else if ((re->overall_options & PCRE2_ANCHORED) == 0 &&
           is_startline(codestart, 0, &cb, 0, FALSE))
    re->flags |= PCRE2_STARTLINE;

  if (reqcuflags >= 0)
    {
#if PCRE2_CODE_UNIT_WIDTH == 16
    if ((re->overall_options & PCRE2_UTF) == 0 ||   /* Not UTF */
        firstcuflags < 0 ||                         /* First not set */
        (firstcu & 0xf800) != 0xd800 ||             /* First not surrogate */
        (reqcu & 0xfc00) != 0xdc00)                 /* Req not low surrogate */
#elif PCRE2_CODE_UNIT_WIDTH == 8
    if ((re->overall_options & PCRE2_UTF) == 0 ||   /* Not UTF */
        firstcuflags < 0 ||                         /* First not set */
        (firstcu & 0x80) == 0 ||                    /* First is ASCII */
        (reqcu & 0x80) == 0)                        /* Req is ASCII */
#endif
      {
      minminlength++;
      }

    if ((re->overall_options & PCRE2_ANCHORED) == 0 ||
        (reqcuflags & REQ_VARY) != 0)
      {
      re->last_codeunit = reqcu;
      re->flags |= PCRE2_LASTSET;

      if ((reqcuflags & REQ_CASELESS) != 0)
        {
        if (reqcu < 128 || (!utf && !ucp && reqcu < 255))
          {
          if (cb.fcc[reqcu] != reqcu) re->flags |= PCRE2_LASTCASELESS;
          }
#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
      else if (ucp && !utf && UCD_OTHERCASE(reqcu) != reqcu)
        re->flags |= PCRE2_LASTCASELESS;
#else
      else if ((utf || ucp) && reqcu <= MAX_UTF_CODE_POINT &&
               UCD_OTHERCASE(reqcu) != reqcu)
        re->flags |= PCRE2_LASTCASELESS;
#endif
#endif  /* SUPPORT_UNICODE */
        }
      }
    }

  if (PRIV(study)(re) != 0)
    {
    errorcode = ERR31;
    goto HAD_CB_ERROR;
    }

  if ((re->flags & PCRE2_FIRSTMAPSET) != 0 && minminlength == 0)
    minminlength = 1;

  if (re->minlength < minminlength) re->minlength = minminlength;
  }   /* End of start-of-match optimizations. */

EXIT:
#ifdef SUPPORT_VALGRIND
if (zero_terminated) VALGRIND_MAKE_MEM_DEFINED(pattern + patlen, CU2BYTES(1));
#endif
if (cb.parsed_pattern != stack_parsed_pattern)
  ccontext->memctl.free(cb.parsed_pattern, ccontext->memctl.memory_data);
if (cb.named_group_list_size > NAMED_GROUP_LIST_SIZE)
  ccontext->memctl.free((void *)cb.named_groups, ccontext->memctl.memory_data);
if (cb.groupinfo != stack_groupinfo)
  ccontext->memctl.free((void *)cb.groupinfo, ccontext->memctl.memory_data);
return re;    /* Will be NULL after an error */

HAD_CB_ERROR:
ptr = pattern + cb.erroroffset;

HAD_EARLY_ERROR:
*erroroffset = ptr - pattern;

HAD_ERROR:
*errorptr = errorcode;
pcre2_code_free(re);
re = NULL;
goto EXIT;
}

/* End of pcre2_compile.c */
