/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
     Original API code Copyright (c) 1997-2012 University of Cambridge
          New API code Copyright (c) 2016-2018 University of Cambridge

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

#undef ACROSSCHAR
#undef BACKCHAR
#undef BYTES2CU
#undef CHMAX_255
#undef CU2BYTES
#undef FORWARDCHAR
#undef FORWARDCHARTEST
#undef GET
#undef GET2
#undef GETCHAR
#undef GETCHARINC
#undef GETCHARINCTEST
#undef GETCHARLEN
#undef GETCHARLENTEST
#undef GETCHARTEST
#undef GET_EXTRALEN
#undef HAS_EXTRALEN
#undef IMM2_SIZE
#undef MAX_255
#undef MAX_MARK
#undef MAX_PATTERN_SIZE
#undef MAX_UTF_SINGLE_CU
#undef NOT_FIRSTCU
#undef PUT
#undef PUT2
#undef PUT2INC
#undef PUTCHAR
#undef PUTINC
#undef TABLE_GET

/* ------------------- 8-bit support  ------------------ */

#if PCRE2_CODE_UNIT_WIDTH == 8

#if LINK_SIZE == 2
#define PUT(a,n,d)   \
  (a[n] = (PCRE2_UCHAR)((d) >> 8)), \
  (a[(n)+1] = (PCRE2_UCHAR)((d) & 255))
#define GET(a,n) \
  (unsigned int)(((a)[n] << 8) | (a)[(n)+1])
#define MAX_PATTERN_SIZE (1 << 16)

#elif LINK_SIZE == 3
#define PUT(a,n,d)       \
  (a[n] = (PCRE2_UCHAR)((d) >> 16)),    \
  (a[(n)+1] = (PCRE2_UCHAR)((d) >> 8)), \
  (a[(n)+2] = (PCRE2_UCHAR)((d) & 255))
#define GET(a,n) \
  (unsigned int)(((a)[n] << 16) | ((a)[(n)+1] << 8) | (a)[(n)+2])
#define MAX_PATTERN_SIZE (1 << 24)

#elif LINK_SIZE == 4
#define PUT(a,n,d)        \
  (a[n] = (PCRE2_UCHAR)((d) >> 24)),     \
  (a[(n)+1] = (PCRE2_UCHAR)((d) >> 16)), \
  (a[(n)+2] = (PCRE2_UCHAR)((d) >> 8)),  \
  (a[(n)+3] = (PCRE2_UCHAR)((d) & 255))
#define GET(a,n) \
  (unsigned int)(((a)[n] << 24) | ((a)[(n)+1] << 16) | ((a)[(n)+2] << 8) | (a)[(n)+3])
#define MAX_PATTERN_SIZE (1 << 30)   /* Keep it positive */

#else
#error LINK_SIZE must be 2, 3, or 4
#endif

/* ------------------- 16-bit support  ------------------ */

#elif PCRE2_CODE_UNIT_WIDTH == 16

#if LINK_SIZE == 2
#undef LINK_SIZE
#define LINK_SIZE 1
#define PUT(a,n,d)   \
  (a[n] = (PCRE2_UCHAR)(d))
#define GET(a,n) \
  (a[n])
#define MAX_PATTERN_SIZE (1 << 16)

#elif LINK_SIZE == 3 || LINK_SIZE == 4
#undef LINK_SIZE
#define LINK_SIZE 2
#define PUT(a,n,d)   \
  (a[n] = (PCRE2_UCHAR)((d) >> 16)), \
  (a[(n)+1] = (PCRE2_UCHAR)((d) & 65535))
#define GET(a,n) \
  (unsigned int)(((a)[n] << 16) | (a)[(n)+1])
#define MAX_PATTERN_SIZE (1 << 30)  /* Keep it positive */

#else
#error LINK_SIZE must be 2, 3, or 4
#endif


/* ------------------- 32-bit support  ------------------ */

#elif PCRE2_CODE_UNIT_WIDTH == 32
#undef LINK_SIZE
#define LINK_SIZE 1
#define PUT(a,n,d)   \
  (a[n] = (d))
#define GET(a,n) \
  (a[n])
#define MAX_PATTERN_SIZE (1 << 30)  /* Keep it positive */

#else
#error Unsupported compiling mode
#endif

/* --------------- Other mode-specific macros ----------------- */

#if PCRE2_CODE_UNIT_WIDTH == 8
#define IMM2_SIZE 2
#define GET2(a,n) (unsigned int)(((a)[n] << 8) | (a)[(n)+1])
#define PUT2(a,n,d) a[n] = (d) >> 8, a[(n)+1] = (d) & 255

#else  /* Code units are 16 or 32 bits */
#define IMM2_SIZE 1
#define GET2(a,n) a[n]
#define PUT2(a,n,d) a[n] = d
#endif

#if PCRE2_CODE_UNIT_WIDTH == 8
#define MAX_255(c) TRUE
#define MAX_MARK ((1u << 8) - 1)
#define TABLE_GET(c, table, default) ((table)[c])
#ifdef SUPPORT_UNICODE
#define SUPPORT_WIDE_CHARS
#define CHMAX_255(c) ((c) <= 255u)
#else
#define CHMAX_255(c) TRUE
#endif  /* SUPPORT_UNICODE */

#else  /* Code units are 16 or 32 bits */
#define CHMAX_255(c) ((c) <= 255u)
#define MAX_255(c) ((c) <= 255u)
#define MAX_MARK ((1u << 16) - 1)
#define SUPPORT_WIDE_CHARS
#define TABLE_GET(c, table, default) (MAX_255(c)? ((table)[c]):(default))
#endif

/* ----------------- Character-handling macros ----------------- */

#define UCHAR21(eptr)        (*(eptr))
#define UCHAR21TEST(eptr)    (*(eptr))
#define UCHAR21INC(eptr)     (*(eptr)++)
#define UCHAR21INCTEST(eptr) (*(eptr)++)

#ifndef SUPPORT_UNICODE

/* #define MAX_UTF_SINGLE_CU */
/* #define HAS_EXTRALEN(c) */
/* #define GET_EXTRALEN(c) */
/* #define NOT_FIRSTCU(c) */
#define GETCHAR(c, eptr) c = *eptr;
#define GETCHARTEST(c, eptr) c = *eptr;
#define GETCHARINC(c, eptr) c = *eptr++;
#define GETCHARINCTEST(c, eptr) c = *eptr++;
#define GETCHARLEN(c, eptr, len) c = *eptr;
#define PUTCHAR(c, p) (*p = c, 1)
/* #define GETCHARLENTEST(c, eptr, len) */
/* #define BACKCHAR(eptr) */
/* #define FORWARDCHAR(eptr) */
/* #define FORWARCCHARTEST(eptr,end) */
/* #define ACROSSCHAR(condition, eptr, action) */

#else   /* SUPPORT_UNICODE */

/* ------------------- 8-bit support  ------------------ */

#if PCRE2_CODE_UNIT_WIDTH == 8
#define MAYBE_UTF_MULTI          /* UTF chars may use multiple code units */

#define MAX_UTF_SINGLE_CU 127

#define HAS_EXTRALEN(c) HASUTF8EXTRALEN(c)

#define GET_EXTRALEN(c) (PRIV(utf8_table4)[(c) & 0x3fu])

#define NOT_FIRSTCU(c) (((c) & 0xc0u) == 0x80u)

#define GETCHAR(c, eptr) \
  c = *eptr; \
  if (c >= 0xc0u) GETUTF8(c, eptr);

#define GETCHARTEST(c, eptr) \
  c = *eptr; \
  if (utf && c >= 0xc0u) GETUTF8(c, eptr);

#define GETCHARINC(c, eptr) \
  c = *eptr++; \
  if (c >= 0xc0u) GETUTF8INC(c, eptr);

#define GETCHARINCTEST(c, eptr) \
  c = *eptr++; \
  if (utf && c >= 0xc0u) GETUTF8INC(c, eptr);

#define GETCHARLEN(c, eptr, len) \
  c = *eptr; \
  if (c >= 0xc0u) GETUTF8LEN(c, eptr, len);

#define GETCHARLENTEST(c, eptr, len) \
  c = *eptr; \
  if (utf && c >= 0xc0u) GETUTF8LEN(c, eptr, len);

#define BACKCHAR(eptr) while((*eptr & 0xc0u) == 0x80u) eptr--

/* Same as above, just in the other direction. */
#define FORWARDCHAR(eptr) while((*eptr & 0xc0u) == 0x80u) eptr++
#define FORWARDCHARTEST(eptr,end) while(eptr < end && (*eptr & 0xc0u) == 0x80u) eptr++

/* Same as above, but it allows a fully customizable form. */
#define ACROSSCHAR(condition, eptr, action) \
  while((condition) && ((*eptr) & 0xc0u) == 0x80u) action

#define PUTCHAR(c, p) ((utf && c > MAX_UTF_SINGLE_CU)? \
  PRIV(ord2utf)(c,p) : (*p = c, 1))

/* ------------------- 16-bit support  ------------------ */

#elif PCRE2_CODE_UNIT_WIDTH == 16
#define MAYBE_UTF_MULTI          /* UTF chars may use multiple code units */

/* The largest UTF code point that can be encoded as a single code unit. */

#define MAX_UTF_SINGLE_CU 65535

/* Tests whether the code point needs extra characters to decode. */

#define HAS_EXTRALEN(c) (((c) & 0xfc00u) == 0xd800u)

/* Returns with the additional number of characters if IS_MULTICHAR(c) is TRUE.
Otherwise it has an undefined behaviour. */

#define GET_EXTRALEN(c) 1

/* Returns TRUE, if the given value is not the first code unit of a UTF
sequence. */

#define NOT_FIRSTCU(c) (((c) & 0xfc00u) == 0xdc00u)

/* Base macro to pick up the low surrogate of a UTF-16 character, not
advancing the pointer. */

#define GETUTF16(c, eptr) \
   { c = (((c & 0x3ffu) << 10) | (eptr[1] & 0x3ffu)) + 0x10000u; }

/* Get the next UTF-16 character, not advancing the pointer. This is called when
we know we are in UTF-16 mode. */

#define GETCHAR(c, eptr) \
  c = *eptr; \
  if ((c & 0xfc00u) == 0xd800u) GETUTF16(c, eptr);

/* Get the next UTF-16 character, testing for UTF-16 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *eptr; \
  if (utf && (c & 0xfc00u) == 0xd800u) GETUTF16(c, eptr);

/* Base macro to pick up the low surrogate of a UTF-16 character, advancing
the pointer. */

#define GETUTF16INC(c, eptr) \
   { c = (((c & 0x3ffu) << 10) | (*eptr++ & 0x3ffu)) + 0x10000u; }

/* Get the next UTF-16 character, advancing the pointer. This is called when we
know we are in UTF-16 mode. */

#define GETCHARINC(c, eptr) \
  c = *eptr++; \
  if ((c & 0xfc00u) == 0xd800u) GETUTF16INC(c, eptr);

/* Get the next character, testing for UTF-16 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-16 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *eptr++; \
  if (utf && (c & 0xfc00u) == 0xd800u) GETUTF16INC(c, eptr);

/* Base macro to pick up the low surrogate of a UTF-16 character, not
advancing the pointer, incrementing the length. */

#define GETUTF16LEN(c, eptr, len) \
   { c = (((c & 0x3ffu) << 10) | (eptr[1] & 0x3ffu)) + 0x10000u; len++; }

/* Get the next UTF-16 character, not advancing the pointer, incrementing
length if there is a low surrogate. This is called when we know we are in
UTF-16 mode. */

#define GETCHARLEN(c, eptr, len) \
  c = *eptr; \
  if ((c & 0xfc00u) == 0xd800u) GETUTF16LEN(c, eptr, len);

/* Get the next UTF-816character, testing for UTF-16 mode, not advancing the
pointer, incrementing length if there is a low surrogate. This is called when
we do not know if we are in UTF-16 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  c = *eptr; \
  if (utf && (c & 0xfc00u) == 0xd800u) GETUTF16LEN(c, eptr, len);

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-16 mode - we don't put a test within the
macro because almost all calls are already within a block of UTF-16 only
code. */

#define BACKCHAR(eptr) if ((*eptr & 0xfc00u) == 0xdc00u) eptr--

/* Same as above, just in the other direction. */
#define FORWARDCHAR(eptr) if ((*eptr & 0xfc00u) == 0xdc00u) eptr++
#define FORWARDCHARTEST(eptr,end) if (eptr < end && (*eptr & 0xfc00u) == 0xdc00u) eptr++

/* Same as above, but it allows a fully customizable form. */
#define ACROSSCHAR(condition, eptr, action) \
  if ((condition) && ((*eptr) & 0xfc00u) == 0xdc00u) action

/* Deposit a character into memory, returning the number of code units. */

#define PUTCHAR(c, p) ((utf && c > MAX_UTF_SINGLE_CU)? \
  PRIV(ord2utf)(c,p) : (*p = c, 1))


/* ------------------- 32-bit support  ------------------ */

#else

/* These are trivial for the 32-bit library, since all UTF-32 characters fit
into one PCRE2_UCHAR unit. */

#define MAX_UTF_SINGLE_CU (0x10ffffu)
#define HAS_EXTRALEN(c) (0)
#define GET_EXTRALEN(c) (0)
#define NOT_FIRSTCU(c) (0)

/* Get the next UTF-32 character, not advancing the pointer. This is called when
we know we are in UTF-32 mode. */

#define GETCHAR(c, eptr) \
  c = *(eptr);

/* Get the next UTF-32 character, testing for UTF-32 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *(eptr);

/* Get the next UTF-32 character, advancing the pointer. This is called when we
know we are in UTF-32 mode. */

#define GETCHARINC(c, eptr) \
  c = *((eptr)++);

/* Get the next character, testing for UTF-32 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-32 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *((eptr)++);

/* Get the next UTF-32 character, not advancing the pointer, not incrementing
length (since all UTF-32 is of length 1). This is called when we know we are in
UTF-32 mode. */

#define GETCHARLEN(c, eptr, len) \
  GETCHAR(c, eptr)

/* Get the next UTF-32character, testing for UTF-32 mode, not advancing the
pointer, not incrementing the length (since all UTF-32 is of length 1).
This is called when we do not know if we are in UTF-32 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  GETCHARTEST(c, eptr)

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-32 mode - we don't put a test within the
macro because almost all calls are already within a block of UTF-32 only
code.

These are all no-ops since all UTF-32 characters fit into one pcre_uchar. */

#define BACKCHAR(eptr) do { } while (0)

/* Same as above, just in the other direction. */

#define FORWARDCHAR(eptr) do { } while (0)
#define FORWARDCHARTEST(eptr,end) do { } while (0)

/* Same as above, but it allows a fully customizable form. */

#define ACROSSCHAR(condition, eptr, action) do { } while (0)

/* Deposit a character into memory, returning the number of code units. */

#define PUTCHAR(c, p) (*p = c, 1)

#endif  /* UTF-32 character handling */
#endif  /* SUPPORT_UNICODE */

/* Mode-dependent macros that have the same definition in all modes. */

#define CU2BYTES(x)     ((x)*((PCRE2_CODE_UNIT_WIDTH/8)))
#define BYTES2CU(x)     ((x)/((PCRE2_CODE_UNIT_WIDTH/8)))
#define PUTINC(a,n,d)   PUT(a,n,d), a += LINK_SIZE
#define PUT2INC(a,n,d)  PUT2(a,n,d), a += IMM2_SIZE

/* ----------------------- HIDDEN STRUCTURES ----------------------------- */

typedef struct pcre2_real_general_context {
  pcre2_memctl memctl;
} pcre2_real_general_context;

typedef struct pcre2_real_compile_context {
  pcre2_memctl memctl;
  int (*stack_guard)(uint32_t, void *);
  void *stack_guard_data;
  const uint8_t *tables;
  PCRE2_SIZE max_pattern_length;
  uint16_t bsr_convention;
  uint16_t newline_convention;
  uint32_t parens_nest_limit;
  uint32_t extra_options;
} pcre2_real_compile_context;

typedef struct pcre2_real_match_context {
  pcre2_memctl memctl;
#ifdef SUPPORT_JIT
  pcre2_jit_callback jit_callback;
  void *jit_callback_data;
#endif
  int    (*callout)(pcre2_callout_block *, void *);
  void    *callout_data;
  int    (*substitute_callout)(pcre2_substitute_callout_block *, void *);
  void    *substitute_callout_data;
  PCRE2_SIZE offset_limit;
  uint32_t heap_limit;
  uint32_t match_limit;
  uint32_t depth_limit;
} pcre2_real_match_context;

typedef struct pcre2_real_convert_context {
  pcre2_memctl memctl;
  uint32_t glob_separator;
  uint32_t glob_escape;
} pcre2_real_convert_context;

#undef  CODE_BLOCKSIZE_TYPE
#define CODE_BLOCKSIZE_TYPE size_t

#undef  LOOKBEHIND_MAX
#define LOOKBEHIND_MAX UINT16_MAX

typedef struct pcre2_real_code {
  pcre2_memctl memctl;            /* Memory control fields */
  const uint8_t *tables;          /* The character tables */
  void    *executable_jit;        /* Pointer to JIT code */
  uint8_t  start_bitmap[32];      /* Bitmap for starting code unit < 256 */
  CODE_BLOCKSIZE_TYPE blocksize;  /* Total (bytes) that was malloc-ed */
  uint32_t magic_number;          /* Paranoid and endianness check */
  uint32_t compile_options;       /* Options passed to pcre2_compile() */
  uint32_t overall_options;       /* Options after processing the pattern */
  uint32_t extra_options;         /* Taken from compile_context */
  uint32_t flags;                 /* Various state flags */
  uint32_t limit_heap;            /* Limit set in the pattern */
  uint32_t limit_match;           /* Limit set in the pattern */
  uint32_t limit_depth;           /* Limit set in the pattern */
  uint32_t first_codeunit;        /* Starting code unit */
  uint32_t last_codeunit;         /* This codeunit must be seen */
  uint16_t bsr_convention;        /* What \R matches */
  uint16_t newline_convention;    /* What is a newline? */
  uint16_t max_lookbehind;        /* Longest lookbehind (characters) */
  uint16_t minlength;             /* Minimum length of match */
  uint16_t top_bracket;           /* Highest numbered group */
  uint16_t top_backref;           /* Highest numbered back reference */
  uint16_t name_entry_size;       /* Size (code units) of table entries */
  uint16_t name_count;            /* Number of name entries in the table */
} pcre2_real_code;

typedef struct pcre2_real_match_data {
  pcre2_memctl     memctl;
  const pcre2_real_code *code;    /* The pattern used for the match */
  PCRE2_SPTR       subject;       /* The subject that was matched */
  PCRE2_SPTR       mark;          /* Pointer to last mark */
  PCRE2_SIZE       leftchar;      /* Offset to leftmost code unit */
  PCRE2_SIZE       rightchar;     /* Offset to rightmost code unit */
  PCRE2_SIZE       startchar;     /* Offset to starting code unit */
  uint8_t          matchedby;     /* Type of match (normal, JIT, DFA) */
  uint8_t          flags;         /* Various flags */
  uint16_t         oveccount;     /* Number of pairs */
  int              rc;            /* The return code from the match */
  PCRE2_SIZE       ovector[131072]; /* Must be last in the structure */
} pcre2_real_match_data;

/* ----------------------- PRIVATE STRUCTURES ----------------------------- */

#ifndef PCRE2_PCRE2TEST

typedef struct recurse_check {
  struct recurse_check *prev;
  PCRE2_SPTR group;
} recurse_check;

typedef struct parsed_recurse_check {
  struct parsed_recurse_check *prev;
  uint32_t *groupptr;
} parsed_recurse_check;

typedef struct recurse_cache {
  PCRE2_SPTR group;
  int groupnumber;
} recurse_cache;

typedef struct branch_chain {
  struct branch_chain *outer;
  PCRE2_UCHAR *current_branch;
} branch_chain;

typedef struct named_group {
  PCRE2_SPTR   name;          /* Points to the name in the pattern */
  uint32_t     number;        /* Group number */
  uint16_t     length;        /* Length of the name */
  uint16_t     isdup;         /* TRUE if a duplicate */
} named_group;

typedef struct compile_block {
  pcre2_real_compile_context *cx;  /* Points to the compile context */
  const uint8_t *lcc;              /* Points to lower casing table */
  const uint8_t *fcc;              /* Points to case-flipping table */
  const uint8_t *cbits;            /* Points to character type table */
  const uint8_t *ctypes;           /* Points to table of type maps */
  PCRE2_SPTR start_workspace;      /* The start of working space */
  PCRE2_SPTR start_code;           /* The start of the compiled code */
  PCRE2_SPTR start_pattern;        /* The start of the pattern */
  PCRE2_SPTR end_pattern;          /* The end of the pattern */
  PCRE2_UCHAR *name_table;         /* The name/number table */
  PCRE2_SIZE workspace_size;       /* Size of workspace */
  PCRE2_SIZE small_ref_offset[10]; /* Offsets for \1 to \9 */
  PCRE2_SIZE erroroffset;          /* Offset of error in pattern */
  uint16_t names_found;            /* Number of entries so far */
  uint16_t name_entry_size;        /* Size of each entry */
  uint16_t parens_depth;           /* Depth of nested parentheses */
  uint16_t assert_depth;           /* Depth of nested assertions */
  open_capitem *open_caps;         /* Chain of open capture items */
  named_group *named_groups;       /* Points to vector in pre-compile */
  uint32_t named_group_list_size;  /* Number of entries in the list */
  uint32_t external_options;       /* External (initial) options */
  uint32_t external_flags;         /* External flag bits to be set */
  uint32_t bracount;               /* Count of capturing parentheses */
  uint32_t lastcapture;            /* Last capture encountered */
  uint32_t *parsed_pattern;        /* Parsed pattern buffer */
  uint32_t *parsed_pattern_end;    /* Parsed pattern should not get here */
  uint32_t *groupinfo;             /* Group info vector */
  uint32_t top_backref;            /* Maximum back reference */
  uint32_t backref_map;            /* Bitmap of low back refs */
  uint32_t nltype;                 /* Newline type */
  uint32_t nllen;                  /* Newline string length */
  uint32_t class_range_start;      /* Overall class range start */
  uint32_t class_range_end;        /* Overall class range end */
  PCRE2_UCHAR nl[4];               /* Newline string when fixed length */
  int  max_lookbehind;             /* Maximum lookbehind (characters) */
  int  req_varyopt;                /* "After variable item" flag for reqbyte */
  BOOL had_accept;                 /* (*ACCEPT) encountered */
  BOOL had_pruneorskip;            /* (*PRUNE) or (*SKIP) encountered */
  BOOL had_recurse;                /* Had a recursion or subroutine call */
  BOOL dupnames;                   /* Duplicate names exist */
} compile_block;

typedef struct pcre2_real_jit_stack {
  pcre2_memctl memctl;
  void* stack;
} pcre2_real_jit_stack;

typedef struct dfa_recursion_info {
  struct dfa_recursion_info *prevrec;
  PCRE2_SPTR subject_position;
  uint32_t group_num;
} dfa_recursion_info;

typedef struct heapframe {

  PCRE2_SPTR ecode;          /* The current position in the pattern */
  PCRE2_SPTR temp_sptr[2];   /* Used for short-term PCRE_SPTR values */
  PCRE2_SIZE length;         /* Used for character, string, or code lengths */
  PCRE2_SIZE back_frame;     /* Amount to subtract on RRETURN */
  PCRE2_SIZE temp_size;      /* Used for short-term PCRE2_SIZE values */
  uint32_t rdepth;           /* "Recursion" depth */
  uint32_t group_frame_type; /* Type information for group frames */
  uint32_t temp_32[4];       /* Used for short-term 32-bit or BOOL values */
  uint8_t return_id;         /* Where to go on in internal "return" */
  uint8_t op;                /* Processing opcode */

#if PCRE2_CODE_UNIT_WIDTH == 8
  PCRE2_UCHAR occu[6];       /* Used for other case code units */
#elif PCRE2_CODE_UNIT_WIDTH == 16
  PCRE2_UCHAR occu[2];       /* Used for other case code units */
  uint8_t unused[2];         /* Ensure 32-bit alignment (see above) */
#else
  uint8_t unused[2];         /* Ensure 32-bit alignment (see above) */
  PCRE2_UCHAR occu[1];       /* Used for other case code units */
#endif

  PCRE2_SPTR eptr;           /* MUST BE FIRST */
  PCRE2_SPTR start_match;    /* Can be adjusted by \K */
  PCRE2_SPTR mark;           /* Most recent mark on the success path */
  uint32_t current_recurse;  /* Current (deepest) recursion number */
  uint32_t capture_last;     /* Most recent capture */
  PCRE2_SIZE last_group_offset;  /* Saved offset to most recent group frame */
  PCRE2_SIZE offset_top;     /* Offset after highest capture */
  PCRE2_SIZE ovector[131072]; /* Must be last in the structure */
} heapframe;

typedef char check_heapframe_size[
  ((sizeof(heapframe) % sizeof(PCRE2_SIZE)) == 0)? (+1):(-1)];

typedef struct match_block {
  pcre2_memctl memctl;            /* For general use */
  PCRE2_SIZE frame_vector_size;   /* Size of a backtracking frame */
  heapframe *match_frames;        /* Points to vector of frames */
  heapframe *match_frames_top;    /* Points after the end of the vector */
  heapframe *stack_frames;        /* The original vector on the stack */
  PCRE2_SIZE heap_limit;          /* As it says */
  uint32_t match_limit;           /* As it says */
  uint32_t match_limit_depth;     /* As it says */
  uint32_t match_call_count;      /* Number of times a new frame is created */
  BOOL hitend;                    /* Hit the end of the subject at some point */
  BOOL hasthen;                   /* Pattern contains (*THEN) */
  BOOL allowemptypartial;         /* Allow empty hard partial */
  const uint8_t *lcc;             /* Points to lower casing table */
  const uint8_t *fcc;             /* Points to case-flipping table */
  const uint8_t *ctypes;          /* Points to table of type maps */
  PCRE2_SIZE start_offset;        /* The start offset value */
  PCRE2_SIZE end_offset_top;      /* Highwater mark at end of match */
  uint16_t partial;               /* PARTIAL options */
  uint16_t bsr_convention;        /* \R interpretation */
  uint16_t name_count;            /* Number of names in name table */
  uint16_t name_entry_size;       /* Size of entry in names table */
  PCRE2_SPTR name_table;          /* Table of group names */
  PCRE2_SPTR start_code;          /* For use when recursing */
  PCRE2_SPTR start_subject;       /* Start of the subject string */
  PCRE2_SPTR check_subject;       /* Where UTF-checked from */
  PCRE2_SPTR end_subject;         /* End of the subject string */
  PCRE2_SPTR end_match_ptr;       /* Subject position at end match */
  PCRE2_SPTR start_used_ptr;      /* Earliest consulted character */
  PCRE2_SPTR last_used_ptr;       /* Latest consulted character */
  PCRE2_SPTR mark;                /* Mark pointer to pass back on success */
  PCRE2_SPTR nomatch_mark;        /* Mark pointer to pass back on failure */
  PCRE2_SPTR verb_ecode_ptr;      /* For passing back info */
  PCRE2_SPTR verb_skip_ptr;       /* For passing back a (*SKIP) name */
  uint32_t verb_current_recurse;  /* Current recurse when (*VERB) happens */
  uint32_t moptions;              /* Match options */
  uint32_t poptions;              /* Pattern options */
  uint32_t skip_arg_count;        /* For counting SKIP_ARGs */
  uint32_t ignore_skip_arg;       /* For re-run when SKIP arg name not found */
  uint32_t nltype;                /* Newline type */
  uint32_t nllen;                 /* Newline string length */
  PCRE2_UCHAR nl[4];              /* Newline string when fixed */
  pcre2_callout_block *cb;        /* Points to a callout block */
  void  *callout_data;            /* To pass back to callouts */
  int (*callout)(pcre2_callout_block *,void *);  /* Callout function or NULL */
} match_block;

typedef struct dfa_match_block {
  pcre2_memctl memctl;            /* For general use */
  PCRE2_SPTR start_code;          /* Start of the compiled pattern */
  PCRE2_SPTR start_subject ;      /* Start of the subject string */
  PCRE2_SPTR end_subject;         /* End of subject string */
  PCRE2_SPTR start_used_ptr;      /* Earliest consulted character */
  PCRE2_SPTR last_used_ptr;       /* Latest consulted character */
  const uint8_t *tables;          /* Character tables */
  PCRE2_SIZE start_offset;        /* The start offset value */
  PCRE2_SIZE heap_limit;          /* As it says */
  PCRE2_SIZE heap_used;           /* As it says */
  uint32_t match_limit;           /* As it says */
  uint32_t match_limit_depth;     /* As it says */
  uint32_t match_call_count;      /* Number of calls of internal function */
  uint32_t moptions;              /* Match options */
  uint32_t poptions;              /* Pattern options */
  uint32_t nltype;                /* Newline type */
  uint32_t nllen;                 /* Newline string length */
  BOOL allowemptypartial;         /* Allow empty hard partial */
  PCRE2_UCHAR nl[4];              /* Newline string when fixed */
  uint16_t bsr_convention;        /* \R interpretation */
  pcre2_callout_block *cb;        /* Points to a callout block */
  void *callout_data;             /* To pass back to callouts */
  int (*callout)(pcre2_callout_block *,void *);  /* Callout function or NULL */
  dfa_recursion_info *recursive;  /* Linked list of recursion data */
} dfa_match_block;

#endif  /* PCRE2_PCRE2TEST */

/* End of pcre2_intmodedep.h */
