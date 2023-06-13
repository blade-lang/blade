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

#define NLBLOCK mb             /* Block containing newline information */
#define PSSTART start_subject  /* Field containing processed string start */
#define PSEND   end_subject    /* Field containing processed string end */

#include "pcre2_internal.h"

#define PUBLIC_DFA_MATCH_OPTIONS \
  (PCRE2_ANCHORED|PCRE2_ENDANCHORED|PCRE2_NOTBOL|PCRE2_NOTEOL|PCRE2_NOTEMPTY| \
   PCRE2_NOTEMPTY_ATSTART|PCRE2_NO_UTF_CHECK|PCRE2_PARTIAL_HARD| \
   PCRE2_PARTIAL_SOFT|PCRE2_DFA_SHORTEST|PCRE2_DFA_RESTART| \
   PCRE2_COPY_MATCHED_SUBJECT)

/*************************************************
*      Code parameters and static tables         *
*************************************************/

#define OP_PROP_EXTRA       300
#define OP_EXTUNI_EXTRA     320
#define OP_ANYNL_EXTRA      340
#define OP_HSPACE_EXTRA     360
#define OP_VSPACE_EXTRA     380

static const uint8_t coptable[] = {
  0,                             /* End                                    */
  0, 0, 0, 0, 0,                 /* \A, \G, \K, \B, \b                     */
  0, 0, 0, 0, 0, 0,              /* \D, \d, \S, \s, \W, \w                 */
  0, 0, 0,                       /* Any, AllAny, Anybyte                   */
  0, 0,                          /* \P, \p                                 */
  0, 0, 0, 0, 0,                 /* \R, \H, \h, \V, \v                     */
  0,                             /* \X                                     */
  0, 0, 0, 0, 0, 0,              /* \Z, \z, $, $M, ^, ^M                   */
  1,                             /* Char                                   */
  1,                             /* Chari                                  */
  1,                             /* not                                    */
  1,                             /* noti                                   */
  /* Positive single-char repeats                                          */
  1, 1, 1, 1, 1, 1,              /* *, *?, +, +?, ?, ??                    */
  1+IMM2_SIZE, 1+IMM2_SIZE,      /* upto, minupto                          */
  1+IMM2_SIZE,                   /* exact                                  */
  1, 1, 1, 1+IMM2_SIZE,          /* *+, ++, ?+, upto+                      */
  1, 1, 1, 1, 1, 1,              /* *I, *?I, +I, +?I, ?I, ??I              */
  1+IMM2_SIZE, 1+IMM2_SIZE,      /* upto I, minupto I                      */
  1+IMM2_SIZE,                   /* exact I                                */
  1, 1, 1, 1+IMM2_SIZE,          /* *+I, ++I, ?+I, upto+I                  */
  /* Negative single-char repeats - only for chars < 256                   */
  1, 1, 1, 1, 1, 1,              /* NOT *, *?, +, +?, ?, ??                */
  1+IMM2_SIZE, 1+IMM2_SIZE,      /* NOT upto, minupto                      */
  1+IMM2_SIZE,                   /* NOT exact                              */
  1, 1, 1, 1+IMM2_SIZE,          /* NOT *+, ++, ?+, upto+                  */
  1, 1, 1, 1, 1, 1,              /* NOT *I, *?I, +I, +?I, ?I, ??I          */
  1+IMM2_SIZE, 1+IMM2_SIZE,      /* NOT upto I, minupto I                  */
  1+IMM2_SIZE,                   /* NOT exact I                            */
  1, 1, 1, 1+IMM2_SIZE,          /* NOT *+I, ++I, ?+I, upto+I              */
  /* Positive type repeats                                                 */
  1, 1, 1, 1, 1, 1,              /* Type *, *?, +, +?, ?, ??               */
  1+IMM2_SIZE, 1+IMM2_SIZE,      /* Type upto, minupto                     */
  1+IMM2_SIZE,                   /* Type exact                             */
  1, 1, 1, 1+IMM2_SIZE,          /* Type *+, ++, ?+, upto+                 */
  /* Character class & ref repeats                                         */
  0, 0, 0, 0, 0, 0,              /* *, *?, +, +?, ?, ??                    */
  0, 0,                          /* CRRANGE, CRMINRANGE                    */
  0, 0, 0, 0,                    /* Possessive *+, ++, ?+, CRPOSRANGE      */
  0,                             /* CLASS                                  */
  0,                             /* NCLASS                                 */
  0,                             /* XCLASS - variable length               */
  0,                             /* REF                                    */
  0,                             /* REFI                                   */
  0,                             /* DNREF                                  */
  0,                             /* DNREFI                                 */
  0,                             /* RECURSE                                */
  0,                             /* CALLOUT                                */
  0,                             /* CALLOUT_STR                            */
  0,                             /* Alt                                    */
  0,                             /* Ket                                    */
  0,                             /* KetRmax                                */
  0,                             /* KetRmin                                */
  0,                             /* KetRpos                                */
  0,                             /* Reverse                                */
  0,                             /* Assert                                 */
  0,                             /* Assert not                             */
  0,                             /* Assert behind                          */
  0,                             /* Assert behind not                      */
  0,                             /* NA assert                              */
  0,                             /* NA assert behind                       */
  0,                             /* ONCE                                   */
  0,                             /* SCRIPT_RUN                             */
  0, 0, 0, 0, 0,                 /* BRA, BRAPOS, CBRA, CBRAPOS, COND       */
  0, 0, 0, 0, 0,                 /* SBRA, SBRAPOS, SCBRA, SCBRAPOS, SCOND  */
  0, 0,                          /* CREF, DNCREF                           */
  0, 0,                          /* RREF, DNRREF                           */
  0, 0,                          /* FALSE, TRUE                            */
  0, 0, 0,                       /* BRAZERO, BRAMINZERO, BRAPOSZERO        */
  0, 0, 0,                       /* MARK, PRUNE, PRUNE_ARG                 */
  0, 0, 0, 0,                    /* SKIP, SKIP_ARG, THEN, THEN_ARG         */
  0, 0,                          /* COMMIT, COMMIT_ARG                     */
  0, 0, 0,                       /* FAIL, ACCEPT, ASSERT_ACCEPT            */
  0, 0, 0                        /* CLOSE, SKIPZERO, DEFINE                */
};

static const uint8_t poptable[] = {
  0,                             /* End                                    */
  0, 0, 0, 1, 1,                 /* \A, \G, \K, \B, \b                     */
  1, 1, 1, 1, 1, 1,              /* \D, \d, \S, \s, \W, \w                 */
  1, 1, 1,                       /* Any, AllAny, Anybyte                   */
  1, 1,                          /* \P, \p                                 */
  1, 1, 1, 1, 1,                 /* \R, \H, \h, \V, \v                     */
  1,                             /* \X                                     */
  0, 0, 0, 0, 0, 0,              /* \Z, \z, $, $M, ^, ^M                   */
  1,                             /* Char                                   */
  1,                             /* Chari                                  */
  1,                             /* not                                    */
  1,                             /* noti                                   */
  /* Positive single-char repeats                                          */
  1, 1, 1, 1, 1, 1,              /* *, *?, +, +?, ?, ??                    */
  1, 1, 1,                       /* upto, minupto, exact                   */
  1, 1, 1, 1,                    /* *+, ++, ?+, upto+                      */
  1, 1, 1, 1, 1, 1,              /* *I, *?I, +I, +?I, ?I, ??I              */
  1, 1, 1,                       /* upto I, minupto I, exact I             */
  1, 1, 1, 1,                    /* *+I, ++I, ?+I, upto+I                  */
  /* Negative single-char repeats - only for chars < 256                   */
  1, 1, 1, 1, 1, 1,              /* NOT *, *?, +, +?, ?, ??                */
  1, 1, 1,                       /* NOT upto, minupto, exact               */
  1, 1, 1, 1,                    /* NOT *+, ++, ?+, upto+                  */
  1, 1, 1, 1, 1, 1,              /* NOT *I, *?I, +I, +?I, ?I, ??I          */
  1, 1, 1,                       /* NOT upto I, minupto I, exact I         */
  1, 1, 1, 1,                    /* NOT *+I, ++I, ?+I, upto+I              */
  /* Positive type repeats                                                 */
  1, 1, 1, 1, 1, 1,              /* Type *, *?, +, +?, ?, ??               */
  1, 1, 1,                       /* Type upto, minupto, exact              */
  1, 1, 1, 1,                    /* Type *+, ++, ?+, upto+                 */
  /* Character class & ref repeats                                         */
  1, 1, 1, 1, 1, 1,              /* *, *?, +, +?, ?, ??                    */
  1, 1,                          /* CRRANGE, CRMINRANGE                    */
  1, 1, 1, 1,                    /* Possessive *+, ++, ?+, CRPOSRANGE      */
  1,                             /* CLASS                                  */
  1,                             /* NCLASS                                 */
  1,                             /* XCLASS - variable length               */
  0,                             /* REF                                    */
  0,                             /* REFI                                   */
  0,                             /* DNREF                                  */
  0,                             /* DNREFI                                 */
  0,                             /* RECURSE                                */
  0,                             /* CALLOUT                                */
  0,                             /* CALLOUT_STR                            */
  0,                             /* Alt                                    */
  0,                             /* Ket                                    */
  0,                             /* KetRmax                                */
  0,                             /* KetRmin                                */
  0,                             /* KetRpos                                */
  0,                             /* Reverse                                */
  0,                             /* Assert                                 */
  0,                             /* Assert not                             */
  0,                             /* Assert behind                          */
  0,                             /* Assert behind not                      */
  0,                             /* NA assert                              */
  0,                             /* NA assert behind                       */
  0,                             /* ONCE                                   */
  0,                             /* SCRIPT_RUN                             */
  0, 0, 0, 0, 0,                 /* BRA, BRAPOS, CBRA, CBRAPOS, COND       */
  0, 0, 0, 0, 0,                 /* SBRA, SBRAPOS, SCBRA, SCBRAPOS, SCOND  */
  0, 0,                          /* CREF, DNCREF                           */
  0, 0,                          /* RREF, DNRREF                           */
  0, 0,                          /* FALSE, TRUE                            */
  0, 0, 0,                       /* BRAZERO, BRAMINZERO, BRAPOSZERO        */
  0, 0, 0,                       /* MARK, PRUNE, PRUNE_ARG                 */
  0, 0, 0, 0,                    /* SKIP, SKIP_ARG, THEN, THEN_ARG         */
  0, 0,                          /* COMMIT, COMMIT_ARG                     */
  0, 0, 0,                       /* FAIL, ACCEPT, ASSERT_ACCEPT            */
  0, 0, 0                        /* CLOSE, SKIPZERO, DEFINE                */
};

static const uint8_t toptable1[] = {
  0, 0, 0, 0, 0, 0,
  ctype_digit, ctype_digit,
  ctype_space, ctype_space,
  ctype_word,  ctype_word,
  0, 0                            /* OP_ANY, OP_ALLANY */
};

static const uint8_t toptable2[] = {
  0, 0, 0, 0, 0, 0,
  ctype_digit, 0,
  ctype_space, 0,
  ctype_word,  0,
  1, 1                            /* OP_ANY, OP_ALLANY */
};

typedef struct stateblock {
  int offset;                     /* Offset to opcode (-ve has meaning) */
  int count;                      /* Count for repeats */
  int data;                       /* Some use extra data */
} stateblock;

#define INTS_PER_STATEBLOCK  (int)(sizeof(stateblock)/sizeof(int))

#define OVEC_UNIT  (sizeof(PCRE2_SIZE)/sizeof(int))

#define RWS_BASE_SIZE   (DFA_START_RWS_SIZE/sizeof(int))  /* Stack vector */
#define RWS_RSIZE       1000                    /* Work size for recursion */
#define RWS_OVEC_RSIZE  (1000*OVEC_UNIT)        /* Ovector for recursion */
#define RWS_OVEC_OSIZE  (2*OVEC_UNIT)           /* Ovector in other cases */

typedef struct RWS_anchor {
  struct RWS_anchor *next;
  uint32_t size;  /* Number of ints */
  uint32_t free;  /* Number of ints */
} RWS_anchor;

#define RWS_ANCHOR_SIZE (sizeof(RWS_anchor)/sizeof(int))

/*************************************************
*               Process a callout                *
*************************************************/

static int
do_callout(PCRE2_SPTR code, PCRE2_SIZE *offsets, PCRE2_SPTR current_subject,
  PCRE2_SPTR ptr, dfa_match_block *mb, PCRE2_SIZE extracode,
  PCRE2_SIZE *lengthptr)
{
pcre2_callout_block *cb = mb->cb;

*lengthptr = (code[extracode] == OP_CALLOUT)?
  (PCRE2_SIZE)PRIV(OP_lengths)[OP_CALLOUT] :
  (PCRE2_SIZE)GET(code, 1 + 2*LINK_SIZE + extracode);

if (mb->callout == NULL) return 0;    /* No callout provided */

cb->offset_vector    = offsets;
cb->start_match      = (PCRE2_SIZE)(current_subject - mb->start_subject);
cb->current_position = (PCRE2_SIZE)(ptr - mb->start_subject);
cb->pattern_position = GET(code, 1 + extracode);
cb->next_item_length = GET(code, 1 + LINK_SIZE + extracode);

if (code[extracode] == OP_CALLOUT)
  {
  cb->callout_number = code[1 + 2*LINK_SIZE + extracode];
  cb->callout_string_offset = 0;
  cb->callout_string = NULL;
  cb->callout_string_length = 0;
  }
else
  {
  cb->callout_number = 0;
  cb->callout_string_offset = GET(code, 1 + 3*LINK_SIZE + extracode);
  cb->callout_string = code + (1 + 4*LINK_SIZE + extracode) + 1;
  cb->callout_string_length = *lengthptr - (1 + 4*LINK_SIZE) - 2;
  }

return (mb->callout)(cb, mb->callout_data);
}

/*************************************************
*         Expand local workspace memory          *
*************************************************/

static int
more_workspace(RWS_anchor **rwsptr, unsigned int ovecsize, dfa_match_block *mb)
{
RWS_anchor *rws = *rwsptr;
RWS_anchor *new;

if (rws->next != NULL)
  {
  new = rws->next;
  }

else
  {
  uint32_t newsize = (rws->size >= UINT32_MAX/2)? UINT32_MAX/2 : rws->size * 2;
  uint32_t newsizeK = newsize/(1024/sizeof(int));

  if (newsizeK + mb->heap_used > mb->heap_limit)
    newsizeK = (uint32_t)(mb->heap_limit - mb->heap_used);
  newsize = newsizeK*(1024/sizeof(int));

  if (newsize < RWS_RSIZE + ovecsize + RWS_ANCHOR_SIZE)
    return PCRE2_ERROR_HEAPLIMIT;
  new = mb->memctl.malloc(newsize*sizeof(int), mb->memctl.memory_data);
  if (new == NULL) return PCRE2_ERROR_NOMEMORY;
  mb->heap_used += newsizeK;
  new->next = NULL;
  new->size = newsize;
  rws->next = new;
  }

new->free = new->size - RWS_ANCHOR_SIZE;
*rwsptr = new;
return 0;
}

/*************************************************
*     Match a Regular Expression - DFA engine    *
*************************************************/

#define ADD_ACTIVE(x,y) \
  if (active_count++ < wscount) \
    { \
    next_active_state->offset = (x); \
    next_active_state->count  = (y); \
    next_active_state++; \
    } \
  else return PCRE2_ERROR_DFA_WSSIZE

#define ADD_ACTIVE_DATA(x,y,z) \
  if (active_count++ < wscount) \
    { \
    next_active_state->offset = (x); \
    next_active_state->count  = (y); \
    next_active_state->data   = (z); \
    next_active_state++; \
    } \
  else return PCRE2_ERROR_DFA_WSSIZE

#define ADD_NEW(x,y) \
  if (new_count++ < wscount) \
    { \
    next_new_state->offset = (x); \
    next_new_state->count  = (y); \
    next_new_state++; \
    } \
  else return PCRE2_ERROR_DFA_WSSIZE

#define ADD_NEW_DATA(x,y,z) \
  if (new_count++ < wscount) \
    { \
    next_new_state->offset = (x); \
    next_new_state->count  = (y); \
    next_new_state->data   = (z); \
    next_new_state++; \
    } \
  else return PCRE2_ERROR_DFA_WSSIZE

static int
internal_dfa_match(
  dfa_match_block *mb,
  PCRE2_SPTR this_start_code,
  PCRE2_SPTR current_subject,
  PCRE2_SIZE start_offset,
  PCRE2_SIZE *offsets,
  uint32_t offsetcount,
  int *workspace,
  int wscount,
  uint32_t rlevel,
  int *RWS)
{
stateblock *active_states, *new_states, *temp_states;
stateblock *next_active_state, *next_new_state;
const uint8_t *ctypes, *lcc, *fcc;
PCRE2_SPTR ptr;
PCRE2_SPTR end_code;
dfa_recursion_info new_recursive;
int active_count, new_count, match_count;

PCRE2_SPTR start_subject = mb->start_subject;
PCRE2_SPTR end_subject = mb->end_subject;
PCRE2_SPTR start_code = mb->start_code;

#ifdef SUPPORT_UNICODE
BOOL utf = (mb->poptions & PCRE2_UTF) != 0;
BOOL utf_or_ucp = utf || (mb->poptions & PCRE2_UCP) != 0;
#else
BOOL utf = FALSE;
#endif

BOOL reset_could_continue = FALSE;

if (mb->match_call_count++ >= mb->match_limit) return PCRE2_ERROR_MATCHLIMIT;
if (rlevel++ > mb->match_limit_depth) return PCRE2_ERROR_DEPTHLIMIT;
offsetcount &= (uint32_t)(-2);  /* Round down */

wscount -= 2;
wscount = (wscount - (wscount % (INTS_PER_STATEBLOCK * 2))) /
          (2 * INTS_PER_STATEBLOCK);

ctypes = mb->tables + ctypes_offset;
lcc = mb->tables + lcc_offset;
fcc = mb->tables + fcc_offset;

match_count = PCRE2_ERROR_NOMATCH;   /* A negative number */

active_states = (stateblock *)(workspace + 2);
next_new_state = new_states = active_states + wscount;
new_count = 0;

if (*this_start_code == OP_ASSERTBACK || *this_start_code == OP_ASSERTBACK_NOT)
  {
  size_t max_back = 0;
  size_t gone_back;

  end_code = this_start_code;
  do
    {
    size_t back = (size_t)GET(end_code, 2+LINK_SIZE);
    if (back > max_back) max_back = back;
    end_code += GET(end_code, 1);
    }
  while (*end_code == OP_ALT);

#ifdef SUPPORT_UNICODE

  if (utf)
    {
    for (gone_back = 0; gone_back < max_back; gone_back++)
      {
      if (current_subject <= start_subject) break;
      current_subject--;
      ACROSSCHAR(current_subject > start_subject, current_subject,
        current_subject--);
      }
    }
  else
#endif

    {
    size_t current_offset = (size_t)(current_subject - start_subject);
    gone_back = (current_offset < max_back)? current_offset : max_back;
    current_subject -= gone_back;
    }

  if (current_subject < mb->start_used_ptr)
    mb->start_used_ptr = current_subject;

  end_code = this_start_code;
  do
    {
    uint32_t revlen = (end_code[1+LINK_SIZE] == OP_REVERSE)? 1 + LINK_SIZE : 0;
    size_t back = (revlen == 0)? 0 : (size_t)GET(end_code, 2+LINK_SIZE);
    if (back <= gone_back)
      {
      int bstate = (int)(end_code - start_code + 1 + LINK_SIZE + revlen);
      ADD_NEW_DATA(-bstate, 0, (int)(gone_back - back));
      }
    end_code += GET(end_code, 1);
    }
  while (*end_code == OP_ALT);
 }

else
  {
  end_code = this_start_code;

  if (rlevel == 1 && (mb->moptions & PCRE2_DFA_RESTART) != 0)
    {
    do { end_code += GET(end_code, 1); } while (*end_code == OP_ALT);
    new_count = workspace[1];
    if (!workspace[0])
      memcpy(new_states, active_states, (size_t)new_count * sizeof(stateblock));
    }

  else
    {
    int length = 1 + LINK_SIZE +
      ((*this_start_code == OP_CBRA || *this_start_code == OP_SCBRA ||
        *this_start_code == OP_CBRAPOS || *this_start_code == OP_SCBRAPOS)
        ? IMM2_SIZE:0);
    do
      {
      ADD_NEW((int)(end_code - start_code + length), 0);
      end_code += GET(end_code, 1);
      length = 1 + LINK_SIZE;
      }
    while (*end_code == OP_ALT);
    }
  }

workspace[0] = 0;    /* Bit indicating which vector is current */

ptr = current_subject;
for (;;)
  {
  int i, j;
  int clen, dlen;
  uint32_t c, d;
  int forced_fail = 0;
  BOOL partial_newline = FALSE;
  BOOL could_continue = reset_could_continue;
  reset_could_continue = FALSE;

  if (ptr > mb->last_used_ptr) mb->last_used_ptr = ptr;

  temp_states = active_states;
  active_states = new_states;
  new_states = temp_states;
  active_count = new_count;
  new_count = 0;

  workspace[0] ^= 1;              /* Remember for the restarting feature */
  workspace[1] = active_count;

  next_active_state = active_states + active_count;
  next_new_state = new_states;

  if (ptr < end_subject)
    {
    clen = 1;        /* Number of data items in the character */
#ifdef SUPPORT_UNICODE
    GETCHARLENTEST(c, ptr, clen);
#else
    c = *ptr;
#endif  /* SUPPORT_UNICODE */
    }
  else
    {
    clen = 0;        /* This indicates the end of the subject */
    c = NOTACHAR;    /* This value should never actually be used */
    }

  for (i = 0; i < active_count; i++)
    {
    stateblock *current_state = active_states + i;
    BOOL caseless = FALSE;
    PCRE2_SPTR code;
    uint32_t codevalue;
    int state_offset = current_state->offset;
    int rrc;
    int count;

    if (state_offset < 0)
      {
      if (current_state->data > 0)
        {
        ADD_NEW_DATA(state_offset, current_state->count,
          current_state->data - 1);
        if (could_continue) reset_could_continue = TRUE;
        continue;
        }
      else
        {
        current_state->offset = state_offset = -state_offset;
        }
      }

    for (j = 0; j < i; j++)
      {
      if (active_states[j].offset == state_offset &&
          active_states[j].count == current_state->count)
        goto NEXT_ACTIVE_STATE;
      }

    code = start_code + state_offset;
    codevalue = *code;

    if (clen == 0 && poptable[codevalue] != 0)
      could_continue = TRUE;

    if (coptable[codevalue] > 0)
      {
      dlen = 1;
#ifdef SUPPORT_UNICODE
      if (utf) { GETCHARLEN(d, (code + coptable[codevalue]), dlen); } else
#endif  /* SUPPORT_UNICODE */
      d = code[coptable[codevalue]];
      if (codevalue >= OP_TYPESTAR)
        {
        switch(d)
          {
          case OP_ANYBYTE: return PCRE2_ERROR_DFA_UITEM;
          case OP_NOTPROP:
          case OP_PROP: codevalue += OP_PROP_EXTRA; break;
          case OP_ANYNL: codevalue += OP_ANYNL_EXTRA; break;
          case OP_EXTUNI: codevalue += OP_EXTUNI_EXTRA; break;
          case OP_NOT_HSPACE:
          case OP_HSPACE: codevalue += OP_HSPACE_EXTRA; break;
          case OP_NOT_VSPACE:
          case OP_VSPACE: codevalue += OP_VSPACE_EXTRA; break;
          default: break;
          }
        }
      }
    else
      {
      dlen = 0;         /* Not strictly necessary, but compilers moan */
      d = NOTACHAR;     /* if these variables are not set. */
      }

    switch (codevalue)
      {

      case OP_TABLE_LENGTH:
      case OP_TABLE_LENGTH +
        ((sizeof(coptable) == OP_TABLE_LENGTH) &&
         (sizeof(poptable) == OP_TABLE_LENGTH)):
      return 0;

      case OP_KET:
      case OP_KETRMIN:
      case OP_KETRMAX:
      case OP_KETRPOS:
      if (code != end_code)
        {
        ADD_ACTIVE(state_offset + 1 + LINK_SIZE, 0);
        if (codevalue != OP_KET)
          {
          ADD_ACTIVE(state_offset - (int)GET(code, 1), 0);
          }
        }
      else
        {
        if (ptr > current_subject ||
            ((mb->moptions & PCRE2_NOTEMPTY) == 0 &&
              ((mb->moptions & PCRE2_NOTEMPTY_ATSTART) == 0 ||
                current_subject > start_subject + mb->start_offset)))
          {
          if (match_count < 0) match_count = (offsetcount >= 2)? 1 : 0;
            else if (match_count > 0 && ++match_count * 2 > (int)offsetcount)
              match_count = 0;
          count = ((match_count == 0)? (int)offsetcount : match_count * 2) - 2;
          if (count > 0) (void)memmove(offsets + 2, offsets,
            (size_t)count * sizeof(PCRE2_SIZE));
          if (offsetcount >= 2)
            {
            offsets[0] = (PCRE2_SIZE)(current_subject - start_subject);
            offsets[1] = (PCRE2_SIZE)(ptr - start_subject);
            }
          if ((mb->moptions & PCRE2_DFA_SHORTEST) != 0) return match_count;
          }
        }
      break;

      case OP_ALT:
      do { code += GET(code, 1); } while (*code == OP_ALT);
      ADD_ACTIVE((int)(code - start_code), 0);
      break;

      /*-----------------------------------------------------------------*/
      case OP_BRA:
      case OP_SBRA:
      do
        {
        ADD_ACTIVE((int)(code - start_code + 1 + LINK_SIZE), 0);
        code += GET(code, 1);
        }
      while (*code == OP_ALT);
      break;

      /*-----------------------------------------------------------------*/
      case OP_CBRA:
      case OP_SCBRA:
      ADD_ACTIVE((int)(code - start_code + 1 + LINK_SIZE + IMM2_SIZE),  0);
      code += GET(code, 1);
      while (*code == OP_ALT)
        {
        ADD_ACTIVE((int)(code - start_code + 1 + LINK_SIZE),  0);
        code += GET(code, 1);
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_BRAZERO:
      case OP_BRAMINZERO:
      ADD_ACTIVE(state_offset + 1, 0);
      code += 1 + GET(code, 2);
      while (*code == OP_ALT) code += GET(code, 1);
      ADD_ACTIVE((int)(code - start_code + 1 + LINK_SIZE), 0);
      break;

      /*-----------------------------------------------------------------*/
      case OP_SKIPZERO:
      code += 1 + GET(code, 2);
      while (*code == OP_ALT) code += GET(code, 1);
      ADD_ACTIVE((int)(code - start_code + 1 + LINK_SIZE), 0);
      break;

      /*-----------------------------------------------------------------*/
      case OP_CIRC:
      if (ptr == start_subject && (mb->moptions & PCRE2_NOTBOL) == 0)
        { ADD_ACTIVE(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_CIRCM:
      if ((ptr == start_subject && (mb->moptions & PCRE2_NOTBOL) == 0) ||
          ((ptr != end_subject || (mb->poptions & PCRE2_ALT_CIRCUMFLEX) != 0 )
            && WAS_NEWLINE(ptr)))
        { ADD_ACTIVE(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_EOD:
      if (ptr >= end_subject)
        {
        if ((mb->moptions & PCRE2_PARTIAL_HARD) != 0)
          return PCRE2_ERROR_PARTIAL;
        else { ADD_ACTIVE(state_offset + 1, 0); }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_SOD:
      if (ptr == start_subject) { ADD_ACTIVE(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_SOM:
      if (ptr == start_subject + start_offset) { ADD_ACTIVE(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_ANY:
      if (clen > 0 && !IS_NEWLINE(ptr))
        {
        if (ptr + 1 >= mb->end_subject &&
            (mb->moptions & (PCRE2_PARTIAL_HARD)) != 0 &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            c == NLBLOCK->nl[0])
          {
          could_continue = partial_newline = TRUE;
          }
        else
          {
          ADD_NEW(state_offset + 1, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_ALLANY:
      if (clen > 0)
        { ADD_NEW(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_EODN:
      if (clen == 0 || (IS_NEWLINE(ptr) && ptr == end_subject - mb->nllen))
        {
        if ((mb->moptions & PCRE2_PARTIAL_HARD) != 0)
          return PCRE2_ERROR_PARTIAL;
        ADD_ACTIVE(state_offset + 1, 0);
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_DOLL:
      if ((mb->moptions & PCRE2_NOTEOL) == 0)
        {
        if (clen == 0 && (mb->moptions & PCRE2_PARTIAL_HARD) != 0)
          could_continue = TRUE;
        else if (clen == 0 ||
            ((mb->poptions & PCRE2_DOLLAR_ENDONLY) == 0 && IS_NEWLINE(ptr) &&
               (ptr == end_subject - mb->nllen)
            ))
          { ADD_ACTIVE(state_offset + 1, 0); }
        else if (ptr + 1 >= mb->end_subject &&
                 (mb->moptions & (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT)) != 0 &&
                 NLBLOCK->nltype == NLTYPE_FIXED &&
                 NLBLOCK->nllen == 2 &&
                 c == NLBLOCK->nl[0])
          {
          if ((mb->moptions & PCRE2_PARTIAL_HARD) != 0)
            {
            reset_could_continue = TRUE;
            ADD_NEW_DATA(-(state_offset + 1), 0, 1);
            }
          else could_continue = partial_newline = TRUE;
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_DOLLM:
      if ((mb->moptions & PCRE2_NOTEOL) == 0)
        {
        if (clen == 0 && (mb->moptions & PCRE2_PARTIAL_HARD) != 0)
          could_continue = TRUE;
        else if (clen == 0 ||
            ((mb->poptions & PCRE2_DOLLAR_ENDONLY) == 0 && IS_NEWLINE(ptr)))
          { ADD_ACTIVE(state_offset + 1, 0); }
        else if (ptr + 1 >= mb->end_subject &&
                 (mb->moptions & (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT)) != 0 &&
                 NLBLOCK->nltype == NLTYPE_FIXED &&
                 NLBLOCK->nllen == 2 &&
                 c == NLBLOCK->nl[0])
          {
          if ((mb->moptions & PCRE2_PARTIAL_HARD) != 0)
            {
            reset_could_continue = TRUE;
            ADD_NEW_DATA(-(state_offset + 1), 0, 1);
            }
          else could_continue = partial_newline = TRUE;
          }
        }
      else if (IS_NEWLINE(ptr))
        { ADD_ACTIVE(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/

      case OP_DIGIT:
      case OP_WHITESPACE:
      case OP_WORDCHAR:
      if (clen > 0 && c < 256 &&
            ((ctypes[c] & toptable1[codevalue]) ^ toptable2[codevalue]) != 0)
        { ADD_NEW(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_NOT_DIGIT:
      case OP_NOT_WHITESPACE:
      case OP_NOT_WORDCHAR:
      if (clen > 0 && (c >= 256 ||
            ((ctypes[c] & toptable1[codevalue]) ^ toptable2[codevalue]) != 0))
        { ADD_NEW(state_offset + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_WORD_BOUNDARY:
      case OP_NOT_WORD_BOUNDARY:
        {
        int left_word, right_word;

        if (ptr > start_subject)
          {
          PCRE2_SPTR temp = ptr - 1;
          if (temp < mb->start_used_ptr) mb->start_used_ptr = temp;
#if defined SUPPORT_UNICODE && PCRE2_CODE_UNIT_WIDTH != 32
          if (utf) { BACKCHAR(temp); }
#endif
          GETCHARTEST(d, temp);
#ifdef SUPPORT_UNICODE
          if ((mb->poptions & PCRE2_UCP) != 0)
            {
            if (d == '_') left_word = TRUE; else
              {
              uint32_t cat = UCD_CATEGORY(d);
              left_word = (cat == ucp_L || cat == ucp_N);
              }
            }
          else
#endif
          left_word = d < 256 && (ctypes[d] & ctype_word) != 0;
          }
        else left_word = FALSE;

        if (clen > 0)
          {
          if (ptr >= mb->last_used_ptr)
            {
            PCRE2_SPTR temp = ptr + 1;
#if defined SUPPORT_UNICODE && PCRE2_CODE_UNIT_WIDTH != 32
            if (utf) { FORWARDCHARTEST(temp, mb->end_subject); }
#endif
            mb->last_used_ptr = temp;
            }
#ifdef SUPPORT_UNICODE
          if ((mb->poptions & PCRE2_UCP) != 0)
            {
            if (c == '_') right_word = TRUE; else
              {
              uint32_t cat = UCD_CATEGORY(c);
              right_word = (cat == ucp_L || cat == ucp_N);
              }
            }
          else
#endif
          right_word = c < 256 && (ctypes[c] & ctype_word) != 0;
          }
        else right_word = FALSE;

        if ((left_word == right_word) == (codevalue == OP_NOT_WORD_BOUNDARY))
          { ADD_ACTIVE(state_offset + 1, 0); }
        }
      break;

#ifdef SUPPORT_UNICODE
      case OP_PROP:
      case OP_NOTPROP:
      if (clen > 0)
        {
        BOOL OK;
        const uint32_t *cp;
        const ucd_record * prop = GET_UCD(c);
        switch(code[1])
          {
          case PT_ANY:
          OK = TRUE;
          break;

          case PT_LAMP:
          OK = prop->chartype == ucp_Lu || prop->chartype == ucp_Ll ||
               prop->chartype == ucp_Lt;
          break;

          case PT_GC:
          OK = PRIV(ucp_gentype)[prop->chartype] == code[2];
          break;

          case PT_PC:
          OK = prop->chartype == code[2];
          break;

          case PT_SC:
          OK = prop->script == code[2];
          break;

          case PT_ALNUM:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N;
          break;

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          switch(c)
            {
            HSPACE_CASES:
            VSPACE_CASES:
            OK = TRUE;
            break;

            default:
            OK = PRIV(ucp_gentype)[prop->chartype] == ucp_Z;
            break;
            }
          break;

          case PT_WORD:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N ||
               c == CHAR_UNDERSCORE;
          break;

          case PT_CLIST:
          cp = PRIV(ucd_caseless_sets) + code[2];
          for (;;)
            {
            if (c < *cp) { OK = FALSE; break; }
            if (c == *cp++) { OK = TRUE; break; }
            }
          break;

          case PT_UCNC:
          OK = c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
               c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
               c >= 0xe000;
          break;

          default:
          OK = codevalue != OP_PROP;
          break;
          }

        if (OK == (codevalue == OP_PROP)) { ADD_NEW(state_offset + 3, 0); }
        }
      break;
#endif

      case OP_TYPEPLUS:
      case OP_TYPEMINPLUS:
      case OP_TYPEPOSPLUS:
      count = current_state->count;  /* Already matched */
      if (count > 0) { ADD_ACTIVE(state_offset + 2, 0); }
      if (clen > 0)
        {
        if (d == OP_ANY && ptr + 1 >= mb->end_subject &&
            (mb->moptions & (PCRE2_PARTIAL_HARD)) != 0 &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            c == NLBLOCK->nl[0])
          {
          could_continue = partial_newline = TRUE;
          }
        else if ((c >= 256 && d != OP_DIGIT && d != OP_WHITESPACE && d != OP_WORDCHAR) ||
            (c < 256 &&
              (d != OP_ANY || !IS_NEWLINE(ptr)) &&
              ((ctypes[c] & toptable1[d]) ^ toptable2[d]) != 0))
          {
          if (count > 0 && codevalue == OP_TYPEPOSPLUS)
            {
            active_count--;            /* Remove non-match possibility */
            next_active_state--;
            }
          count++;
          ADD_NEW(state_offset, count);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_TYPEQUERY:
      case OP_TYPEMINQUERY:
      case OP_TYPEPOSQUERY:
      ADD_ACTIVE(state_offset + 2, 0);
      if (clen > 0)
        {
        if (d == OP_ANY && ptr + 1 >= mb->end_subject &&
            (mb->moptions & (PCRE2_PARTIAL_HARD)) != 0 &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            c == NLBLOCK->nl[0])
          {
          could_continue = partial_newline = TRUE;
          }
        else if ((c >= 256 && d != OP_DIGIT && d != OP_WHITESPACE && d != OP_WORDCHAR) ||
            (c < 256 &&
              (d != OP_ANY || !IS_NEWLINE(ptr)) &&
              ((ctypes[c] & toptable1[d]) ^ toptable2[d]) != 0))
          {
          if (codevalue == OP_TYPEPOSQUERY)
            {
            active_count--;            /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW(state_offset + 2, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_TYPESTAR:
      case OP_TYPEMINSTAR:
      case OP_TYPEPOSSTAR:
      ADD_ACTIVE(state_offset + 2, 0);
      if (clen > 0)
        {
        if (d == OP_ANY && ptr + 1 >= mb->end_subject &&
            (mb->moptions & (PCRE2_PARTIAL_HARD)) != 0 &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            c == NLBLOCK->nl[0])
          {
          could_continue = partial_newline = TRUE;
          }
        else if ((c >= 256 && d != OP_DIGIT && d != OP_WHITESPACE && d != OP_WORDCHAR) ||
            (c < 256 &&
              (d != OP_ANY || !IS_NEWLINE(ptr)) &&
              ((ctypes[c] & toptable1[d]) ^ toptable2[d]) != 0))
          {
          if (codevalue == OP_TYPEPOSSTAR)
            {
            active_count--;            /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW(state_offset, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_TYPEEXACT:
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        if (d == OP_ANY && ptr + 1 >= mb->end_subject &&
            (mb->moptions & (PCRE2_PARTIAL_HARD)) != 0 &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            c == NLBLOCK->nl[0])
          {
          could_continue = partial_newline = TRUE;
          }
        else if ((c >= 256 && d != OP_DIGIT && d != OP_WHITESPACE && d != OP_WORDCHAR) ||
            (c < 256 &&
              (d != OP_ANY || !IS_NEWLINE(ptr)) &&
              ((ctypes[c] & toptable1[d]) ^ toptable2[d]) != 0))
          {
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW(state_offset + 1 + IMM2_SIZE + 1, 0); }
          else
            { ADD_NEW(state_offset, count); }
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_TYPEUPTO:
      case OP_TYPEMINUPTO:
      case OP_TYPEPOSUPTO:
      ADD_ACTIVE(state_offset + 2 + IMM2_SIZE, 0);
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        if (d == OP_ANY && ptr + 1 >= mb->end_subject &&
            (mb->moptions & (PCRE2_PARTIAL_HARD)) != 0 &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            c == NLBLOCK->nl[0])
          {
          could_continue = partial_newline = TRUE;
          }
        else if ((c >= 256 && d != OP_DIGIT && d != OP_WHITESPACE && d != OP_WORDCHAR) ||
            (c < 256 &&
              (d != OP_ANY || !IS_NEWLINE(ptr)) &&
              ((ctypes[c] & toptable1[d]) ^ toptable2[d]) != 0))
          {
          if (codevalue == OP_TYPEPOSUPTO)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW(state_offset + 2 + IMM2_SIZE, 0); }
          else
            { ADD_NEW(state_offset, count); }
          }
        }
      break;

#ifdef SUPPORT_UNICODE
      case OP_PROP_EXTRA + OP_TYPEPLUS:
      case OP_PROP_EXTRA + OP_TYPEMINPLUS:
      case OP_PROP_EXTRA + OP_TYPEPOSPLUS:
      count = current_state->count;           /* Already matched */
      if (count > 0) { ADD_ACTIVE(state_offset + 4, 0); }
      if (clen > 0)
        {
        BOOL OK;
        const uint32_t *cp;
        const ucd_record * prop = GET_UCD(c);
        switch(code[2])
          {
          case PT_ANY:
          OK = TRUE;
          break;

          case PT_LAMP:
          OK = prop->chartype == ucp_Lu || prop->chartype == ucp_Ll ||
            prop->chartype == ucp_Lt;
          break;

          case PT_GC:
          OK = PRIV(ucp_gentype)[prop->chartype] == code[3];
          break;

          case PT_PC:
          OK = prop->chartype == code[3];
          break;

          case PT_SC:
          OK = prop->script == code[3];
          break;

          case PT_ALNUM:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N;
          break;

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          switch(c)
            {
            HSPACE_CASES:
            VSPACE_CASES:
            OK = TRUE;
            break;

            default:
            OK = PRIV(ucp_gentype)[prop->chartype] == ucp_Z;
            break;
            }
          break;

          case PT_WORD:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N ||
               c == CHAR_UNDERSCORE;
          break;

          case PT_CLIST:
          cp = PRIV(ucd_caseless_sets) + code[3];
          for (;;)
            {
            if (c < *cp) { OK = FALSE; break; }
            if (c == *cp++) { OK = TRUE; break; }
            }
          break;

          case PT_UCNC:
          OK = c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
               c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
               c >= 0xe000;
          break;

          default:
          OK = codevalue != OP_PROP;
          break;
          }

        if (OK == (d == OP_PROP))
          {
          if (count > 0 && codevalue == OP_PROP_EXTRA + OP_TYPEPOSPLUS)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          count++;
          ADD_NEW(state_offset, count);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_EXTUNI_EXTRA + OP_TYPEPLUS:
      case OP_EXTUNI_EXTRA + OP_TYPEMINPLUS:
      case OP_EXTUNI_EXTRA + OP_TYPEPOSPLUS:
      count = current_state->count;  /* Already matched */
      if (count > 0) { ADD_ACTIVE(state_offset + 2, 0); }
      if (clen > 0)
        {
        int ncount = 0;
        if (count > 0 && codevalue == OP_EXTUNI_EXTRA + OP_TYPEPOSPLUS)
          {
          active_count--;           /* Remove non-match possibility */
          next_active_state--;
          }
        (void)PRIV(extuni)(c, ptr + clen, mb->start_subject, end_subject, utf,
          &ncount);
        count++;
        ADD_NEW_DATA(-state_offset, count, ncount);
        }
      break;
#endif

      /*-----------------------------------------------------------------*/
      case OP_ANYNL_EXTRA + OP_TYPEPLUS:
      case OP_ANYNL_EXTRA + OP_TYPEMINPLUS:
      case OP_ANYNL_EXTRA + OP_TYPEPOSPLUS:
      count = current_state->count;  /* Already matched */
      if (count > 0) { ADD_ACTIVE(state_offset + 2, 0); }
      if (clen > 0)
        {
        int ncount = 0;
        switch (c)
          {
          case CHAR_VT:
          case CHAR_FF:
          case CHAR_NEL:
#ifndef EBCDIC
          case 0x2028:
          case 0x2029:
#endif  /* Not EBCDIC */
          if (mb->bsr_convention == PCRE2_BSR_ANYCRLF) break;
          goto ANYNL01;

          case CHAR_CR:
          if (ptr + 1 < end_subject && UCHAR21TEST(ptr + 1) == CHAR_LF) ncount = 1;
          /* Fall through */

          ANYNL01:
          case CHAR_LF:
          if (count > 0 && codevalue == OP_ANYNL_EXTRA + OP_TYPEPOSPLUS)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          count++;
          ADD_NEW_DATA(-state_offset, count, ncount);
          break;

          default:
          break;
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_VSPACE_EXTRA + OP_TYPEPLUS:
      case OP_VSPACE_EXTRA + OP_TYPEMINPLUS:
      case OP_VSPACE_EXTRA + OP_TYPEPOSPLUS:
      count = current_state->count;  /* Already matched */
      if (count > 0) { ADD_ACTIVE(state_offset + 2, 0); }
      if (clen > 0)
        {
        BOOL OK;
        switch (c)
          {
          VSPACE_CASES:
          OK = TRUE;
          break;

          default:
          OK = FALSE;
          break;
          }

        if (OK == (d == OP_VSPACE))
          {
          if (count > 0 && codevalue == OP_VSPACE_EXTRA + OP_TYPEPOSPLUS)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          count++;
          ADD_NEW_DATA(-state_offset, count, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_HSPACE_EXTRA + OP_TYPEPLUS:
      case OP_HSPACE_EXTRA + OP_TYPEMINPLUS:
      case OP_HSPACE_EXTRA + OP_TYPEPOSPLUS:
      count = current_state->count;  /* Already matched */
      if (count > 0) { ADD_ACTIVE(state_offset + 2, 0); }
      if (clen > 0)
        {
        BOOL OK;
        switch (c)
          {
          HSPACE_CASES:
          OK = TRUE;
          break;

          default:
          OK = FALSE;
          break;
          }

        if (OK == (d == OP_HSPACE))
          {
          if (count > 0 && codevalue == OP_HSPACE_EXTRA + OP_TYPEPOSPLUS)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          count++;
          ADD_NEW_DATA(-state_offset, count, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
#ifdef SUPPORT_UNICODE
      case OP_PROP_EXTRA + OP_TYPEQUERY:
      case OP_PROP_EXTRA + OP_TYPEMINQUERY:
      case OP_PROP_EXTRA + OP_TYPEPOSQUERY:
      count = 4;
      goto QS1;

      case OP_PROP_EXTRA + OP_TYPESTAR:
      case OP_PROP_EXTRA + OP_TYPEMINSTAR:
      case OP_PROP_EXTRA + OP_TYPEPOSSTAR:
      count = 0;

      QS1:

      ADD_ACTIVE(state_offset + 4, 0);
      if (clen > 0)
        {
        BOOL OK;
        const uint32_t *cp;
        const ucd_record * prop = GET_UCD(c);
        switch(code[2])
          {
          case PT_ANY:
          OK = TRUE;
          break;

          case PT_LAMP:
          OK = prop->chartype == ucp_Lu || prop->chartype == ucp_Ll ||
            prop->chartype == ucp_Lt;
          break;

          case PT_GC:
          OK = PRIV(ucp_gentype)[prop->chartype] == code[3];
          break;

          case PT_PC:
          OK = prop->chartype == code[3];
          break;

          case PT_SC:
          OK = prop->script == code[3];
          break;

          case PT_ALNUM:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N;
          break;

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          switch(c)
            {
            HSPACE_CASES:
            VSPACE_CASES:
            OK = TRUE;
            break;

            default:
            OK = PRIV(ucp_gentype)[prop->chartype] == ucp_Z;
            break;
            }
          break;

          case PT_WORD:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N ||
               c == CHAR_UNDERSCORE;
          break;

          case PT_CLIST:
          cp = PRIV(ucd_caseless_sets) + code[3];
          for (;;)
            {
            if (c < *cp) { OK = FALSE; break; }
            if (c == *cp++) { OK = TRUE; break; }
            }
          break;

          case PT_UCNC:
          OK = c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
               c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
               c >= 0xe000;
          break;

          default:
          OK = codevalue != OP_PROP;
          break;
          }

        if (OK == (d == OP_PROP))
          {
          if (codevalue == OP_PROP_EXTRA + OP_TYPEPOSSTAR ||
              codevalue == OP_PROP_EXTRA + OP_TYPEPOSQUERY)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW(state_offset + count, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_EXTUNI_EXTRA + OP_TYPEQUERY:
      case OP_EXTUNI_EXTRA + OP_TYPEMINQUERY:
      case OP_EXTUNI_EXTRA + OP_TYPEPOSQUERY:
      count = 2;
      goto QS2;

      case OP_EXTUNI_EXTRA + OP_TYPESTAR:
      case OP_EXTUNI_EXTRA + OP_TYPEMINSTAR:
      case OP_EXTUNI_EXTRA + OP_TYPEPOSSTAR:
      count = 0;

      QS2:

      ADD_ACTIVE(state_offset + 2, 0);
      if (clen > 0)
        {
        int ncount = 0;
        if (codevalue == OP_EXTUNI_EXTRA + OP_TYPEPOSSTAR ||
            codevalue == OP_EXTUNI_EXTRA + OP_TYPEPOSQUERY)
          {
          active_count--;           /* Remove non-match possibility */
          next_active_state--;
          }
        (void)PRIV(extuni)(c, ptr + clen, mb->start_subject, end_subject, utf,
          &ncount);
        ADD_NEW_DATA(-(state_offset + count), 0, ncount);
        }
      break;
#endif

      /*-----------------------------------------------------------------*/
      case OP_ANYNL_EXTRA + OP_TYPEQUERY:
      case OP_ANYNL_EXTRA + OP_TYPEMINQUERY:
      case OP_ANYNL_EXTRA + OP_TYPEPOSQUERY:
      count = 2;
      goto QS3;

      case OP_ANYNL_EXTRA + OP_TYPESTAR:
      case OP_ANYNL_EXTRA + OP_TYPEMINSTAR:
      case OP_ANYNL_EXTRA + OP_TYPEPOSSTAR:
      count = 0;

      QS3:
      ADD_ACTIVE(state_offset + 2, 0);
      if (clen > 0)
        {
        int ncount = 0;
        switch (c)
          {
          case CHAR_VT:
          case CHAR_FF:
          case CHAR_NEL:
#ifndef EBCDIC
          case 0x2028:
          case 0x2029:
#endif  /* Not EBCDIC */
          if (mb->bsr_convention == PCRE2_BSR_ANYCRLF) break;
          goto ANYNL02;

          case CHAR_CR:
          if (ptr + 1 < end_subject && UCHAR21TEST(ptr + 1) == CHAR_LF) ncount = 1;
          /* Fall through */

          ANYNL02:
          case CHAR_LF:
          if (codevalue == OP_ANYNL_EXTRA + OP_TYPEPOSSTAR ||
              codevalue == OP_ANYNL_EXTRA + OP_TYPEPOSQUERY)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW_DATA(-(state_offset + (int)count), 0, ncount);
          break;

          default:
          break;
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_VSPACE_EXTRA + OP_TYPEQUERY:
      case OP_VSPACE_EXTRA + OP_TYPEMINQUERY:
      case OP_VSPACE_EXTRA + OP_TYPEPOSQUERY:
      count = 2;
      goto QS4;

      case OP_VSPACE_EXTRA + OP_TYPESTAR:
      case OP_VSPACE_EXTRA + OP_TYPEMINSTAR:
      case OP_VSPACE_EXTRA + OP_TYPEPOSSTAR:
      count = 0;

      QS4:
      ADD_ACTIVE(state_offset + 2, 0);
      if (clen > 0)
        {
        BOOL OK;
        switch (c)
          {
          VSPACE_CASES:
          OK = TRUE;
          break;

          default:
          OK = FALSE;
          break;
          }
        if (OK == (d == OP_VSPACE))
          {
          if (codevalue == OP_VSPACE_EXTRA + OP_TYPEPOSSTAR ||
              codevalue == OP_VSPACE_EXTRA + OP_TYPEPOSQUERY)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW_DATA(-(state_offset + (int)count), 0, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_HSPACE_EXTRA + OP_TYPEQUERY:
      case OP_HSPACE_EXTRA + OP_TYPEMINQUERY:
      case OP_HSPACE_EXTRA + OP_TYPEPOSQUERY:
      count = 2;
      goto QS5;

      case OP_HSPACE_EXTRA + OP_TYPESTAR:
      case OP_HSPACE_EXTRA + OP_TYPEMINSTAR:
      case OP_HSPACE_EXTRA + OP_TYPEPOSSTAR:
      count = 0;

      QS5:
      ADD_ACTIVE(state_offset + 2, 0);
      if (clen > 0)
        {
        BOOL OK;
        switch (c)
          {
          HSPACE_CASES:
          OK = TRUE;
          break;

          default:
          OK = FALSE;
          break;
          }

        if (OK == (d == OP_HSPACE))
          {
          if (codevalue == OP_HSPACE_EXTRA + OP_TYPEPOSSTAR ||
              codevalue == OP_HSPACE_EXTRA + OP_TYPEPOSQUERY)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW_DATA(-(state_offset + (int)count), 0, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
#ifdef SUPPORT_UNICODE
      case OP_PROP_EXTRA + OP_TYPEEXACT:
      case OP_PROP_EXTRA + OP_TYPEUPTO:
      case OP_PROP_EXTRA + OP_TYPEMINUPTO:
      case OP_PROP_EXTRA + OP_TYPEPOSUPTO:
      if (codevalue != OP_PROP_EXTRA + OP_TYPEEXACT)
        { ADD_ACTIVE(state_offset + 1 + IMM2_SIZE + 3, 0); }
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        BOOL OK;
        const uint32_t *cp;
        const ucd_record * prop = GET_UCD(c);
        switch(code[1 + IMM2_SIZE + 1])
          {
          case PT_ANY:
          OK = TRUE;
          break;

          case PT_LAMP:
          OK = prop->chartype == ucp_Lu || prop->chartype == ucp_Ll ||
            prop->chartype == ucp_Lt;
          break;

          case PT_GC:
          OK = PRIV(ucp_gentype)[prop->chartype] == code[1 + IMM2_SIZE + 2];
          break;

          case PT_PC:
          OK = prop->chartype == code[1 + IMM2_SIZE + 2];
          break;

          case PT_SC:
          OK = prop->script == code[1 + IMM2_SIZE + 2];
          break;

          case PT_ALNUM:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N;
          break;

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          switch(c)
            {
            HSPACE_CASES:
            VSPACE_CASES:
            OK = TRUE;
            break;

            default:
            OK = PRIV(ucp_gentype)[prop->chartype] == ucp_Z;
            break;
            }
          break;

          case PT_WORD:
          OK = PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
               PRIV(ucp_gentype)[prop->chartype] == ucp_N ||
               c == CHAR_UNDERSCORE;
          break;

          case PT_CLIST:
          cp = PRIV(ucd_caseless_sets) + code[1 + IMM2_SIZE + 2];
          for (;;)
            {
            if (c < *cp) { OK = FALSE; break; }
            if (c == *cp++) { OK = TRUE; break; }
            }
          break;

          case PT_UCNC:
          OK = c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
               c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
               c >= 0xe000;
          break;

          default:
          OK = codevalue != OP_PROP;
          break;
          }

        if (OK == (d == OP_PROP))
          {
          if (codevalue == OP_PROP_EXTRA + OP_TYPEPOSUPTO)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW(state_offset + 1 + IMM2_SIZE + 3, 0); }
          else
            { ADD_NEW(state_offset, count); }
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_EXTUNI_EXTRA + OP_TYPEEXACT:
      case OP_EXTUNI_EXTRA + OP_TYPEUPTO:
      case OP_EXTUNI_EXTRA + OP_TYPEMINUPTO:
      case OP_EXTUNI_EXTRA + OP_TYPEPOSUPTO:
      if (codevalue != OP_EXTUNI_EXTRA + OP_TYPEEXACT)
        { ADD_ACTIVE(state_offset + 2 + IMM2_SIZE, 0); }
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        PCRE2_SPTR nptr;
        int ncount = 0;
        if (codevalue == OP_EXTUNI_EXTRA + OP_TYPEPOSUPTO)
          {
          active_count--;           /* Remove non-match possibility */
          next_active_state--;
          }
        nptr = PRIV(extuni)(c, ptr + clen, mb->start_subject, end_subject, utf,
          &ncount);
        if (nptr >= end_subject && (mb->moptions & PCRE2_PARTIAL_HARD) != 0)
            reset_could_continue = TRUE;
        if (++count >= (int)GET2(code, 1))
          { ADD_NEW_DATA(-(state_offset + 2 + IMM2_SIZE), 0, ncount); }
        else
          { ADD_NEW_DATA(-state_offset, count, ncount); }
        }
      break;
#endif

      /*-----------------------------------------------------------------*/
      case OP_ANYNL_EXTRA + OP_TYPEEXACT:
      case OP_ANYNL_EXTRA + OP_TYPEUPTO:
      case OP_ANYNL_EXTRA + OP_TYPEMINUPTO:
      case OP_ANYNL_EXTRA + OP_TYPEPOSUPTO:
      if (codevalue != OP_ANYNL_EXTRA + OP_TYPEEXACT)
        { ADD_ACTIVE(state_offset + 2 + IMM2_SIZE, 0); }
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        int ncount = 0;
        switch (c)
          {
          case CHAR_VT:
          case CHAR_FF:
          case CHAR_NEL:
#ifndef EBCDIC
          case 0x2028:
          case 0x2029:
#endif  /* Not EBCDIC */
          if (mb->bsr_convention == PCRE2_BSR_ANYCRLF) break;
          goto ANYNL03;

          case CHAR_CR:
          if (ptr + 1 < end_subject && UCHAR21TEST(ptr + 1) == CHAR_LF) ncount = 1;
          /* Fall through */

          ANYNL03:
          case CHAR_LF:
          if (codevalue == OP_ANYNL_EXTRA + OP_TYPEPOSUPTO)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW_DATA(-(state_offset + 2 + IMM2_SIZE), 0, ncount); }
          else
            { ADD_NEW_DATA(-state_offset, count, ncount); }
          break;

          default:
          break;
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_VSPACE_EXTRA + OP_TYPEEXACT:
      case OP_VSPACE_EXTRA + OP_TYPEUPTO:
      case OP_VSPACE_EXTRA + OP_TYPEMINUPTO:
      case OP_VSPACE_EXTRA + OP_TYPEPOSUPTO:
      if (codevalue != OP_VSPACE_EXTRA + OP_TYPEEXACT)
        { ADD_ACTIVE(state_offset + 2 + IMM2_SIZE, 0); }
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        BOOL OK;
        switch (c)
          {
          VSPACE_CASES:
          OK = TRUE;
          break;

          default:
          OK = FALSE;
          }

        if (OK == (d == OP_VSPACE))
          {
          if (codevalue == OP_VSPACE_EXTRA + OP_TYPEPOSUPTO)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW_DATA(-(state_offset + 2 + IMM2_SIZE), 0, 0); }
          else
            { ADD_NEW_DATA(-state_offset, count, 0); }
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_HSPACE_EXTRA + OP_TYPEEXACT:
      case OP_HSPACE_EXTRA + OP_TYPEUPTO:
      case OP_HSPACE_EXTRA + OP_TYPEMINUPTO:
      case OP_HSPACE_EXTRA + OP_TYPEPOSUPTO:
      if (codevalue != OP_HSPACE_EXTRA + OP_TYPEEXACT)
        { ADD_ACTIVE(state_offset + 2 + IMM2_SIZE, 0); }
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        BOOL OK;
        switch (c)
          {
          HSPACE_CASES:
          OK = TRUE;
          break;

          default:
          OK = FALSE;
          break;
          }

        if (OK == (d == OP_HSPACE))
          {
          if (codevalue == OP_HSPACE_EXTRA + OP_TYPEPOSUPTO)
            {
            active_count--;           /* Remove non-match possibility */
            next_active_state--;
            }
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW_DATA(-(state_offset + 2 + IMM2_SIZE), 0, 0); }
          else
            { ADD_NEW_DATA(-state_offset, count, 0); }
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_CHAR:
      if (clen > 0 && c == d) { ADD_NEW(state_offset + dlen + 1, 0); }
      break;

      /*-----------------------------------------------------------------*/
      case OP_CHARI:
      if (clen == 0) break;

#ifdef SUPPORT_UNICODE
      if (utf_or_ucp)
        {
        if (c == d) { ADD_NEW(state_offset + dlen + 1, 0); } else
          {
          unsigned int othercase;
          if (c < 128)
            othercase = fcc[c];
          else
            othercase = UCD_OTHERCASE(c);
          if (d == othercase) { ADD_NEW(state_offset + dlen + 1, 0); }
          }
        }
      else
#endif  /* SUPPORT_UNICODE */
      /* Not UTF or UCP mode */
        {
        if (TABLE_GET(c, lcc, c) == TABLE_GET(d, lcc, d))
          { ADD_NEW(state_offset + 2, 0); }
        }
      break;


#ifdef SUPPORT_UNICODE

      case OP_EXTUNI:
      if (clen > 0)
        {
        int ncount = 0;
        PCRE2_SPTR nptr = PRIV(extuni)(c, ptr + clen, mb->start_subject,
          end_subject, utf, &ncount);
        if (nptr >= end_subject && (mb->moptions & PCRE2_PARTIAL_HARD) != 0)
            reset_could_continue = TRUE;
        ADD_NEW_DATA(-(state_offset + 1), 0, ncount);
        }
      break;
#endif

      case OP_ANYNL:
      if (clen > 0) switch(c)
        {
        case CHAR_VT:
        case CHAR_FF:
        case CHAR_NEL:
#ifndef EBCDIC
        case 0x2028:
        case 0x2029:
#endif  /* Not EBCDIC */
        if (mb->bsr_convention == PCRE2_BSR_ANYCRLF) break;
        /* Fall through */

        case CHAR_LF:
        ADD_NEW(state_offset + 1, 0);
        break;

        case CHAR_CR:
        if (ptr + 1 >= end_subject)
          {
          ADD_NEW(state_offset + 1, 0);
          if ((mb->moptions & PCRE2_PARTIAL_HARD) != 0)
            reset_could_continue = TRUE;
          }
        else if (UCHAR21TEST(ptr + 1) == CHAR_LF)
          {
          ADD_NEW_DATA(-(state_offset + 1), 0, 1);
          }
        else
          {
          ADD_NEW(state_offset + 1, 0);
          }
        break;
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_NOT_VSPACE:
      if (clen > 0) switch(c)
        {
        VSPACE_CASES:
        break;

        default:
        ADD_NEW(state_offset + 1, 0);
        break;
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_VSPACE:
      if (clen > 0) switch(c)
        {
        VSPACE_CASES:
        ADD_NEW(state_offset + 1, 0);
        break;

        default:
        break;
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_NOT_HSPACE:
      if (clen > 0) switch(c)
        {
        HSPACE_CASES:
        break;

        default:
        ADD_NEW(state_offset + 1, 0);
        break;
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_HSPACE:
      if (clen > 0) switch(c)
        {
        HSPACE_CASES:
        ADD_NEW(state_offset + 1, 0);
        break;

        default:
        break;
        }
      break;

      case OP_NOT:
      if (clen > 0 && c != d) { ADD_NEW(state_offset + dlen + 1, 0); }
      break;

      case OP_NOTI:
      if (clen > 0)
        {
        uint32_t otherd;
#ifdef SUPPORT_UNICODE
        if (utf_or_ucp && d >= 128)
          otherd = UCD_OTHERCASE(d);
        else
#endif  /* SUPPORT_UNICODE */
        otherd = TABLE_GET(d, fcc, d);
        if (c != d && c != otherd)
          { ADD_NEW(state_offset + dlen + 1, 0); }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_PLUSI:
      case OP_MINPLUSI:
      case OP_POSPLUSI:
      case OP_NOTPLUSI:
      case OP_NOTMINPLUSI:
      case OP_NOTPOSPLUSI:
      caseless = TRUE;
      codevalue -= OP_STARI - OP_STAR;

      /* Fall through */
      case OP_PLUS:
      case OP_MINPLUS:
      case OP_POSPLUS:
      case OP_NOTPLUS:
      case OP_NOTMINPLUS:
      case OP_NOTPOSPLUS:
      count = current_state->count;  /* Already matched */
      if (count > 0) { ADD_ACTIVE(state_offset + dlen + 1, 0); }
      if (clen > 0)
        {
        uint32_t otherd = NOTACHAR;
        if (caseless)
          {
#ifdef SUPPORT_UNICODE
          if (utf_or_ucp && d >= 128)
            otherd = UCD_OTHERCASE(d);
          else
#endif  /* SUPPORT_UNICODE */
          otherd = TABLE_GET(d, fcc, d);
          }
        if ((c == d || c == otherd) == (codevalue < OP_NOTSTAR))
          {
          if (count > 0 &&
              (codevalue == OP_POSPLUS || codevalue == OP_NOTPOSPLUS))
            {
            active_count--;             /* Remove non-match possibility */
            next_active_state--;
            }
          count++;
          ADD_NEW(state_offset, count);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_QUERYI:
      case OP_MINQUERYI:
      case OP_POSQUERYI:
      case OP_NOTQUERYI:
      case OP_NOTMINQUERYI:
      case OP_NOTPOSQUERYI:
      caseless = TRUE;
      codevalue -= OP_STARI - OP_STAR;
      /* Fall through */
      case OP_QUERY:
      case OP_MINQUERY:
      case OP_POSQUERY:
      case OP_NOTQUERY:
      case OP_NOTMINQUERY:
      case OP_NOTPOSQUERY:
      ADD_ACTIVE(state_offset + dlen + 1, 0);
      if (clen > 0)
        {
        uint32_t otherd = NOTACHAR;
        if (caseless)
          {
#ifdef SUPPORT_UNICODE
          if (utf_or_ucp && d >= 128)
            otherd = UCD_OTHERCASE(d);
          else
#endif  /* SUPPORT_UNICODE */
          otherd = TABLE_GET(d, fcc, d);
          }
        if ((c == d || c == otherd) == (codevalue < OP_NOTSTAR))
          {
          if (codevalue == OP_POSQUERY || codevalue == OP_NOTPOSQUERY)
            {
            active_count--;            /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW(state_offset + dlen + 1, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_STARI:
      case OP_MINSTARI:
      case OP_POSSTARI:
      case OP_NOTSTARI:
      case OP_NOTMINSTARI:
      case OP_NOTPOSSTARI:
      caseless = TRUE;
      codevalue -= OP_STARI - OP_STAR;
      /* Fall through */
      case OP_STAR:
      case OP_MINSTAR:
      case OP_POSSTAR:
      case OP_NOTSTAR:
      case OP_NOTMINSTAR:
      case OP_NOTPOSSTAR:
      ADD_ACTIVE(state_offset + dlen + 1, 0);
      if (clen > 0)
        {
        uint32_t otherd = NOTACHAR;
        if (caseless)
          {
#ifdef SUPPORT_UNICODE
          if (utf_or_ucp && d >= 128)
            otherd = UCD_OTHERCASE(d);
          else
#endif  /* SUPPORT_UNICODE */
          otherd = TABLE_GET(d, fcc, d);
          }
        if ((c == d || c == otherd) == (codevalue < OP_NOTSTAR))
          {
          if (codevalue == OP_POSSTAR || codevalue == OP_NOTPOSSTAR)
            {
            active_count--;            /* Remove non-match possibility */
            next_active_state--;
            }
          ADD_NEW(state_offset, 0);
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_EXACTI:
      case OP_NOTEXACTI:
      caseless = TRUE;
      codevalue -= OP_STARI - OP_STAR;
      /* Fall through */
      case OP_EXACT:
      case OP_NOTEXACT:
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        uint32_t otherd = NOTACHAR;
        if (caseless)
          {
#ifdef SUPPORT_UNICODE
          if (utf_or_ucp && d >= 128)
            otherd = UCD_OTHERCASE(d);
          else
#endif  /* SUPPORT_UNICODE */
          otherd = TABLE_GET(d, fcc, d);
          }
        if ((c == d || c == otherd) == (codevalue < OP_NOTSTAR))
          {
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW(state_offset + dlen + 1 + IMM2_SIZE, 0); }
          else
            { ADD_NEW(state_offset, count); }
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_UPTOI:
      case OP_MINUPTOI:
      case OP_POSUPTOI:
      case OP_NOTUPTOI:
      case OP_NOTMINUPTOI:
      case OP_NOTPOSUPTOI:
      caseless = TRUE;
      codevalue -= OP_STARI - OP_STAR;
      /* Fall through */
      case OP_UPTO:
      case OP_MINUPTO:
      case OP_POSUPTO:
      case OP_NOTUPTO:
      case OP_NOTMINUPTO:
      case OP_NOTPOSUPTO:
      ADD_ACTIVE(state_offset + dlen + 1 + IMM2_SIZE, 0);
      count = current_state->count;  /* Number already matched */
      if (clen > 0)
        {
        uint32_t otherd = NOTACHAR;
        if (caseless)
          {
#ifdef SUPPORT_UNICODE
          if (utf_or_ucp && d >= 128)
            otherd = UCD_OTHERCASE(d);
          else
#endif  /* SUPPORT_UNICODE */
          otherd = TABLE_GET(d, fcc, d);
          }
        if ((c == d || c == otherd) == (codevalue < OP_NOTSTAR))
          {
          if (codevalue == OP_POSUPTO || codevalue == OP_NOTPOSUPTO)
            {
            active_count--;             /* Remove non-match possibility */
            next_active_state--;
            }
          if (++count >= (int)GET2(code, 1))
            { ADD_NEW(state_offset + dlen + 1 + IMM2_SIZE, 0); }
          else
            { ADD_NEW(state_offset, count); }
          }
        }
      break;

      case OP_CLASS:
      case OP_NCLASS:
      case OP_XCLASS:
        {
        BOOL isinclass = FALSE;
        int next_state_offset;
        PCRE2_SPTR ecode;

        if (codevalue != OP_XCLASS)
          {
          ecode = code + 1 + (32 / sizeof(PCRE2_UCHAR));
          if (clen > 0)
            {
            isinclass = (c > 255)? (codevalue == OP_NCLASS) :
              ((((uint8_t *)(code + 1))[c/8] & (1u << (c&7))) != 0);
            }
          }

        else
         {
         ecode = code + GET(code, 1);
         if (clen > 0) isinclass = PRIV(xclass)(c, code + 1 + LINK_SIZE, utf);
         }

        next_state_offset = (int)(ecode - start_code);

        switch (*ecode)
          {
          case OP_CRSTAR:
          case OP_CRMINSTAR:
          case OP_CRPOSSTAR:
          ADD_ACTIVE(next_state_offset + 1, 0);
          if (isinclass)
            {
            if (*ecode == OP_CRPOSSTAR)
              {
              active_count--;           /* Remove non-match possibility */
              next_active_state--;
              }
            ADD_NEW(state_offset, 0);
            }
          break;

          case OP_CRPLUS:
          case OP_CRMINPLUS:
          case OP_CRPOSPLUS:
          count = current_state->count;  /* Already matched */
          if (count > 0) { ADD_ACTIVE(next_state_offset + 1, 0); }
          if (isinclass)
            {
            if (count > 0 && *ecode == OP_CRPOSPLUS)
              {
              active_count--;           /* Remove non-match possibility */
              next_active_state--;
              }
            count++;
            ADD_NEW(state_offset, count);
            }
          break;

          case OP_CRQUERY:
          case OP_CRMINQUERY:
          case OP_CRPOSQUERY:
          ADD_ACTIVE(next_state_offset + 1, 0);
          if (isinclass)
            {
            if (*ecode == OP_CRPOSQUERY)
              {
              active_count--;           /* Remove non-match possibility */
              next_active_state--;
              }
            ADD_NEW(next_state_offset + 1, 0);
            }
          break;

          case OP_CRRANGE:
          case OP_CRMINRANGE:
          case OP_CRPOSRANGE:
          count = current_state->count;  /* Already matched */
          if (count >= (int)GET2(ecode, 1))
            { ADD_ACTIVE(next_state_offset + 1 + 2 * IMM2_SIZE, 0); }
          if (isinclass)
            {
            int max = (int)GET2(ecode, 1 + IMM2_SIZE);

            if (*ecode == OP_CRPOSRANGE && count >= (int)GET2(ecode, 1))
              {
              active_count--;           /* Remove non-match possibility */
              next_active_state--;
              }

            if (++count >= max && max != 0)   /* Max 0 => no limit */
              { ADD_NEW(next_state_offset + 1 + 2 * IMM2_SIZE, 0); }
            else
              { ADD_NEW(state_offset, count); }
            }
          break;

          default:
          if (isinclass) { ADD_NEW(next_state_offset, 0); }
          break;
          }
        }
      break;

      case OP_FAIL:
      forced_fail++;    /* Count FAILs for multiple states */
      break;

      case OP_ASSERT:
      case OP_ASSERT_NOT:
      case OP_ASSERTBACK:
      case OP_ASSERTBACK_NOT:
        {
        int rc;
        int *local_workspace;
        PCRE2_SIZE *local_offsets;
        PCRE2_SPTR endasscode = code + GET(code, 1);
        RWS_anchor *rws = (RWS_anchor *)RWS;

        if (rws->free < RWS_RSIZE + RWS_OVEC_OSIZE)
          {
          rc = more_workspace(&rws, RWS_OVEC_OSIZE, mb);
          if (rc != 0) return rc;
          RWS = (int *)rws;
          }

        local_offsets = (PCRE2_SIZE *)(RWS + rws->size - rws->free);
        local_workspace = ((int *)local_offsets) + RWS_OVEC_OSIZE;
        rws->free -= RWS_RSIZE + RWS_OVEC_OSIZE;

        while (*endasscode == OP_ALT) endasscode += GET(endasscode, 1);

        rc = internal_dfa_match(
          mb,                                   /* static match data */
          code,                                 /* this subexpression's code */
          ptr,                                  /* where we currently are */
          (PCRE2_SIZE)(ptr - start_subject),    /* start offset */
          local_offsets,                        /* offset vector */
          RWS_OVEC_OSIZE/OVEC_UNIT,             /* size of same */
          local_workspace,                      /* workspace vector */
          RWS_RSIZE,                            /* size of same */
          rlevel,                               /* function recursion level */
          RWS);                                 /* recursion workspace */

        rws->free += RWS_RSIZE + RWS_OVEC_OSIZE;

        if (rc < 0 && rc != PCRE2_ERROR_NOMATCH) return rc;
        if ((rc >= 0) == (codevalue == OP_ASSERT || codevalue == OP_ASSERTBACK))
            { ADD_ACTIVE((int)(endasscode + LINK_SIZE + 1 - start_code), 0); }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_COND:
      case OP_SCOND:
        {
        int codelink = (int)GET(code, 1);
        PCRE2_UCHAR condcode;

        if (code[LINK_SIZE + 1] == OP_CALLOUT
            || code[LINK_SIZE + 1] == OP_CALLOUT_STR)
          {
          PCRE2_SIZE callout_length;
          rrc = do_callout(code, offsets, current_subject, ptr, mb,
            1 + LINK_SIZE, &callout_length);
          if (rrc < 0) return rrc;                 /* Abandon */
          if (rrc > 0) break;                      /* Fail this thread */
          code += callout_length;                  /* Skip callout data */
          }

        condcode = code[LINK_SIZE+1];

        if (condcode == OP_CREF || condcode == OP_DNCREF ||
            condcode == OP_DNRREF)
          return PCRE2_ERROR_DFA_UCOND;

        if (condcode == OP_FALSE || condcode == OP_FAIL)
          { ADD_ACTIVE(state_offset + codelink + LINK_SIZE + 1, 0); }

        else if (condcode == OP_TRUE)
          { ADD_ACTIVE(state_offset + LINK_SIZE + 2, 0); }

        else if (condcode == OP_RREF)
          {
          unsigned int value = GET2(code, LINK_SIZE + 2);
          if (value != RREF_ANY) return PCRE2_ERROR_DFA_UCOND;
          if (mb->recursive != NULL)
            { ADD_ACTIVE(state_offset + LINK_SIZE + 2 + IMM2_SIZE, 0); }
          else { ADD_ACTIVE(state_offset + codelink + LINK_SIZE + 1, 0); }
          }

        else
          {
          int rc;
          int *local_workspace;
          PCRE2_SIZE *local_offsets;
          PCRE2_SPTR asscode = code + LINK_SIZE + 1;
          PCRE2_SPTR endasscode = asscode + GET(asscode, 1);
          RWS_anchor *rws = (RWS_anchor *)RWS;

          if (rws->free < RWS_RSIZE + RWS_OVEC_OSIZE)
            {
            rc = more_workspace(&rws, RWS_OVEC_OSIZE, mb);
            if (rc != 0) return rc;
            RWS = (int *)rws;
            }

          local_offsets = (PCRE2_SIZE *)(RWS + rws->size - rws->free);
          local_workspace = ((int *)local_offsets) + RWS_OVEC_OSIZE;
          rws->free -= RWS_RSIZE + RWS_OVEC_OSIZE;

          while (*endasscode == OP_ALT) endasscode += GET(endasscode, 1);

          rc = internal_dfa_match(
            mb,                                   /* fixed match data */
            asscode,                              /* this subexpression's code */
            ptr,                                  /* where we currently are */
            (PCRE2_SIZE)(ptr - start_subject),    /* start offset */
            local_offsets,                        /* offset vector */
            RWS_OVEC_OSIZE/OVEC_UNIT,             /* size of same */
            local_workspace,                      /* workspace vector */
            RWS_RSIZE,                            /* size of same */
            rlevel,                               /* function recursion level */
            RWS);                                 /* recursion workspace */

          rws->free += RWS_RSIZE + RWS_OVEC_OSIZE;

          if (rc < 0 && rc != PCRE2_ERROR_NOMATCH) return rc;
          if ((rc >= 0) ==
                (condcode == OP_ASSERT || condcode == OP_ASSERTBACK))
            { ADD_ACTIVE((int)(endasscode + LINK_SIZE + 1 - start_code), 0); }
          else
            { ADD_ACTIVE(state_offset + codelink + LINK_SIZE + 1, 0); }
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_RECURSE:
        {
        int rc;
        int *local_workspace;
        PCRE2_SIZE *local_offsets;
        RWS_anchor *rws = (RWS_anchor *)RWS;
        dfa_recursion_info *ri;
        PCRE2_SPTR callpat = start_code + GET(code, 1);
        uint32_t recno = (callpat == mb->start_code)? 0 :
          GET2(callpat, 1 + LINK_SIZE);

        if (rws->free < RWS_RSIZE + RWS_OVEC_RSIZE)
          {
          rc = more_workspace(&rws, RWS_OVEC_RSIZE, mb);
          if (rc != 0) return rc;
          RWS = (int *)rws;
          }

        local_offsets = (PCRE2_SIZE *)(RWS + rws->size - rws->free);
        local_workspace = ((int *)local_offsets) + RWS_OVEC_RSIZE;
        rws->free -= RWS_RSIZE + RWS_OVEC_RSIZE;

        for (ri = mb->recursive; ri != NULL; ri = ri->prevrec)
          if (recno == ri->group_num && ptr == ri->subject_position)
            return PCRE2_ERROR_RECURSELOOP;

        new_recursive.group_num = recno;
        new_recursive.subject_position = ptr;
        new_recursive.prevrec = mb->recursive;
        mb->recursive = &new_recursive;

        rc = internal_dfa_match(
          mb,                                   /* fixed match data */
          callpat,                              /* this subexpression's code */
          ptr,                                  /* where we currently are */
          (PCRE2_SIZE)(ptr - start_subject),    /* start offset */
          local_offsets,                        /* offset vector */
          RWS_OVEC_RSIZE/OVEC_UNIT,             /* size of same */
          local_workspace,                      /* workspace vector */
          RWS_RSIZE,                            /* size of same */
          rlevel,                               /* function recursion level */
          RWS);                                 /* recursion workspace */

        rws->free += RWS_RSIZE + RWS_OVEC_RSIZE;
        mb->recursive = new_recursive.prevrec;  /* Done this recursion */

        if (rc == 0) return PCRE2_ERROR_DFA_RECURSE;

        if (rc > 0)
          {
          for (rc = rc*2 - 2; rc >= 0; rc -= 2)
            {
            PCRE2_SIZE charcount = local_offsets[rc+1] - local_offsets[rc];
#if defined SUPPORT_UNICODE && PCRE2_CODE_UNIT_WIDTH != 32
            if (utf)
              {
              PCRE2_SPTR p = start_subject + local_offsets[rc];
              PCRE2_SPTR pp = start_subject + local_offsets[rc+1];
              while (p < pp) if (NOT_FIRSTCU(*p++)) charcount--;
              }
#endif
            if (charcount > 0)
              {
              ADD_NEW_DATA(-(state_offset + LINK_SIZE + 1), 0,
                (int)(charcount - 1));
              }
            else
              {
              ADD_ACTIVE(state_offset + LINK_SIZE + 1, 0);
              }
            }
          }
        else if (rc != PCRE2_ERROR_NOMATCH) return rc;
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_BRAPOS:
      case OP_SBRAPOS:
      case OP_CBRAPOS:
      case OP_SCBRAPOS:
      case OP_BRAPOSZERO:
        {
        int rc;
        int *local_workspace;
        PCRE2_SIZE *local_offsets;
        PCRE2_SIZE charcount, matched_count;
        PCRE2_SPTR local_ptr = ptr;
        RWS_anchor *rws = (RWS_anchor *)RWS;
        BOOL allow_zero;

        if (rws->free < RWS_RSIZE + RWS_OVEC_OSIZE)
          {
          rc = more_workspace(&rws, RWS_OVEC_OSIZE, mb);
          if (rc != 0) return rc;
          RWS = (int *)rws;
          }

        local_offsets = (PCRE2_SIZE *)(RWS + rws->size - rws->free);
        local_workspace = ((int *)local_offsets) + RWS_OVEC_OSIZE;
        rws->free -= RWS_RSIZE + RWS_OVEC_OSIZE;

        if (codevalue == OP_BRAPOSZERO)
          {
          allow_zero = TRUE;
          codevalue = *(++code);  /* Codevalue will be one of above BRAs */
          }
        else allow_zero = FALSE;

        for (matched_count = 0;; matched_count++)
          {
          rc = internal_dfa_match(
            mb,                                   /* fixed match data */
            code,                                 /* this subexpression's code */
            local_ptr,                            /* where we currently are */
            (PCRE2_SIZE)(ptr - start_subject),    /* start offset */
            local_offsets,                        /* offset vector */
            RWS_OVEC_OSIZE/OVEC_UNIT,             /* size of same */
            local_workspace,                      /* workspace vector */
            RWS_RSIZE,                            /* size of same */
            rlevel,                               /* function recursion level */
            RWS);                                 /* recursion workspace */

          if (rc < 0)
            {
            if (rc != PCRE2_ERROR_NOMATCH) return rc;
            break;
            }

          charcount = local_offsets[1] - local_offsets[0];
          if (charcount == 0) break;
          local_ptr += charcount;    /* Advance temporary position ptr */
          }

        rws->free += RWS_RSIZE + RWS_OVEC_OSIZE;

        if (matched_count > 0 || allow_zero)
          {
          PCRE2_SPTR end_subpattern = code;
          int next_state_offset;

          do { end_subpattern += GET(end_subpattern, 1); }
            while (*end_subpattern == OP_ALT);
          next_state_offset =
            (int)(end_subpattern - start_code + LINK_SIZE + 1);

          if (i + 1 >= active_count && new_count == 0)
            {
            ptr = local_ptr;
            clen = 0;
            ADD_NEW(next_state_offset, 0);
            }
          else
            {
            PCRE2_SPTR p = ptr;
            PCRE2_SPTR pp = local_ptr;
            charcount = (PCRE2_SIZE)(pp - p);
#if defined SUPPORT_UNICODE && PCRE2_CODE_UNIT_WIDTH != 32
            if (utf) while (p < pp) if (NOT_FIRSTCU(*p++)) charcount--;
#endif
            ADD_NEW_DATA(-next_state_offset, 0, (int)(charcount - 1));
            }
          }
        }
      break;

      /*-----------------------------------------------------------------*/
      case OP_ONCE:
        {
        int rc;
        int *local_workspace;
        PCRE2_SIZE *local_offsets;
        RWS_anchor *rws = (RWS_anchor *)RWS;

        if (rws->free < RWS_RSIZE + RWS_OVEC_OSIZE)
          {
          rc = more_workspace(&rws, RWS_OVEC_OSIZE, mb);
          if (rc != 0) return rc;
          RWS = (int *)rws;
          }

        local_offsets = (PCRE2_SIZE *)(RWS + rws->size - rws->free);
        local_workspace = ((int *)local_offsets) + RWS_OVEC_OSIZE;
        rws->free -= RWS_RSIZE + RWS_OVEC_OSIZE;

        rc = internal_dfa_match(
          mb,                                   /* fixed match data */
          code,                                 /* this subexpression's code */
          ptr,                                  /* where we currently are */
          (PCRE2_SIZE)(ptr - start_subject),    /* start offset */
          local_offsets,                        /* offset vector */
          RWS_OVEC_OSIZE/OVEC_UNIT,             /* size of same */
          local_workspace,                      /* workspace vector */
          RWS_RSIZE,                            /* size of same */
          rlevel,                               /* function recursion level */
          RWS);                                 /* recursion workspace */

        rws->free += RWS_RSIZE + RWS_OVEC_OSIZE;

        if (rc >= 0)
          {
          PCRE2_SPTR end_subpattern = code;
          PCRE2_SIZE charcount = local_offsets[1] - local_offsets[0];
          int next_state_offset, repeat_state_offset;

          do { end_subpattern += GET(end_subpattern, 1); }
            while (*end_subpattern == OP_ALT);
          next_state_offset =
            (int)(end_subpattern - start_code + LINK_SIZE + 1);

          repeat_state_offset = (*end_subpattern == OP_KETRMAX ||
                                 *end_subpattern == OP_KETRMIN)?
            (int)(end_subpattern - start_code - GET(end_subpattern, 1)) : -1;

          if (charcount == 0)
            {
            ADD_ACTIVE(next_state_offset, 0);
            }

          else if (i + 1 >= active_count && new_count == 0)
            {
            ptr += charcount;
            clen = 0;
            ADD_NEW(next_state_offset, 0);

            if (repeat_state_offset >= 0)
              {
              next_active_state = active_states;
              active_count = 0;
              i = -1;
              ADD_ACTIVE(repeat_state_offset, 0);
              }
            }
          else
            {
#if defined SUPPORT_UNICODE && PCRE2_CODE_UNIT_WIDTH != 32
            if (utf)
              {
              PCRE2_SPTR p = start_subject + local_offsets[0];
              PCRE2_SPTR pp = start_subject + local_offsets[1];
              while (p < pp) if (NOT_FIRSTCU(*p++)) charcount--;
              }
#endif
            ADD_NEW_DATA(-next_state_offset, 0, (int)(charcount - 1));
            if (repeat_state_offset >= 0)
              { ADD_NEW_DATA(-repeat_state_offset, 0, (int)(charcount - 1)); }
            }
          }
        else if (rc != PCRE2_ERROR_NOMATCH) return rc;
        }
      break;

      case OP_CALLOUT:
      case OP_CALLOUT_STR:
        {
        PCRE2_SIZE callout_length;
        rrc = do_callout(code, offsets, current_subject, ptr, mb, 0,
          &callout_length);
        if (rrc < 0) return rrc;   /* Abandon */
        if (rrc == 0)
          { ADD_ACTIVE(state_offset + (int)callout_length, 0); }
        }
      break;

      default:        /* Unsupported opcode */
      return PCRE2_ERROR_DFA_UITEM;
      }

    NEXT_ACTIVE_STATE: continue;

    }      /* End of loop scanning active states */

  if (new_count <= 0)
    {
    if (could_continue &&                            /* Some could go on, and */
        forced_fail != workspace[1] &&               /* Not all forced fail & */
        (                                            /* either... */
        (mb->moptions & PCRE2_PARTIAL_HARD) != 0      /* Hard partial */
        ||                                           /* or... */
        ((mb->moptions & PCRE2_PARTIAL_SOFT) != 0 &&  /* Soft partial and */
         match_count < 0)                             /* no matches */
        ) &&                                         /* And... */
        (
        partial_newline ||                   /* Either partial NL */
          (                                  /* or ... */
          ptr >= end_subject &&              /* End of subject and */
            (                                  /* either */
            ptr > mb->start_used_ptr ||        /* Inspected non-empty string */
            mb->allowemptypartial              /* or pattern has lookbehind */
            )                                  /* or could match empty */
          )
        ))
      match_count = PCRE2_ERROR_PARTIAL;
    break;  /* Exit from loop along the subject string */
    }

  ptr += clen;    /* Advance to next subject character */
  }               /* Loop to move along the subject string */

if (match_count >= 0 &&
    ((mb->moptions | mb->poptions) & PCRE2_ENDANCHORED) != 0 &&
    ptr < end_subject)
  match_count = PCRE2_ERROR_NOMATCH;

return match_count;
}

/*************************************************
*     Match a pattern using the DFA algorithm    *
*************************************************/

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_dfa_match(const pcre2_code *code, PCRE2_SPTR subject, PCRE2_SIZE length,
  PCRE2_SIZE start_offset, uint32_t options, pcre2_match_data *match_data,
  pcre2_match_context *mcontext, int *workspace, PCRE2_SIZE wscount)
{
int rc;
int was_zero_terminated = 0;

const pcre2_real_code *re = (const pcre2_real_code *)code;

PCRE2_SPTR start_match;
PCRE2_SPTR end_subject;
PCRE2_SPTR bumpalong_limit;
PCRE2_SPTR req_cu_ptr;

BOOL utf, anchored, startline, firstline;
BOOL has_first_cu = FALSE;
BOOL has_req_cu = FALSE;

#if PCRE2_CODE_UNIT_WIDTH == 8
BOOL memchr_not_found_first_cu = FALSE;
BOOL memchr_not_found_first_cu2 = FALSE;
#endif

PCRE2_UCHAR first_cu = 0;
PCRE2_UCHAR first_cu2 = 0;
PCRE2_UCHAR req_cu = 0;
PCRE2_UCHAR req_cu2 = 0;

const uint8_t *start_bits = NULL;

pcre2_callout_block cb;
dfa_match_block actual_match_block;
dfa_match_block *mb = &actual_match_block;

int base_recursion_workspace[RWS_BASE_SIZE];
RWS_anchor *rws = (RWS_anchor *)base_recursion_workspace;
rws->next = NULL;
rws->size = RWS_BASE_SIZE;
rws->free = RWS_BASE_SIZE - RWS_ANCHOR_SIZE;

if (length == PCRE2_ZERO_TERMINATED)
  {
  length = PRIV(strlen)(subject);
  was_zero_terminated = 1;
  }

if ((options & ~PUBLIC_DFA_MATCH_OPTIONS) != 0) return PCRE2_ERROR_BADOPTION;
if (re == NULL || subject == NULL || workspace == NULL || match_data == NULL)
  return PCRE2_ERROR_NULL;
if (wscount < 20) return PCRE2_ERROR_DFA_WSSIZE;
if (start_offset > length) return PCRE2_ERROR_BADOFFSET;

if ((options & (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT)) != 0 &&
   ((re->overall_options | options) & PCRE2_ENDANCHORED) != 0)
  return PCRE2_ERROR_BADOPTION;

if ((re->overall_options & PCRE2_MATCH_INVALID_UTF) != 0)
  return PCRE2_ERROR_DFA_UINVALID_UTF;

if (re->magic_number != MAGIC_NUMBER) return PCRE2_ERROR_BADMAGIC;

if ((re->flags & PCRE2_MODE_MASK) != PCRE2_CODE_UNIT_WIDTH/8)
  return PCRE2_ERROR_BADMODE;

#define FF (PCRE2_NOTEMPTY_SET|PCRE2_NE_ATST_SET)
#define OO (PCRE2_NOTEMPTY|PCRE2_NOTEMPTY_ATSTART)
options |= (re->flags & FF) / ((FF & (~FF+1)) / (OO & (~OO+1)));
#undef FF
#undef OO

if ((options & PCRE2_DFA_RESTART) != 0)
  {
  if ((workspace[0] & (-2)) != 0 || workspace[1] < 1 ||
    workspace[1] > (int)((wscount - 2)/INTS_PER_STATEBLOCK))
      return PCRE2_ERROR_DFA_BADRESTART;
  }

utf = (re->overall_options & PCRE2_UTF) != 0;
start_match = subject + start_offset;
end_subject = subject + length;
req_cu_ptr = start_match - 1;
anchored = (options & (PCRE2_ANCHORED|PCRE2_DFA_RESTART)) != 0 ||
  (re->overall_options & PCRE2_ANCHORED) != 0;

startline = (re->flags & PCRE2_STARTLINE) != 0;
firstline = (re->overall_options & PCRE2_FIRSTLINE) != 0;
bumpalong_limit = end_subject;

mb->cb = &cb;
cb.version = 2;
cb.subject = subject;
cb.subject_length = (PCRE2_SIZE)(end_subject - subject);
cb.callout_flags = 0;
cb.capture_top      = 1;      /* No capture support */
cb.capture_last     = 0;
cb.mark             = NULL;   /* No (*MARK) support */

if (mcontext == NULL)
  {
  mb->callout = NULL;
  mb->memctl = re->memctl;
  mb->match_limit = PRIV(default_match_context).match_limit;
  mb->match_limit_depth = PRIV(default_match_context).depth_limit;
  mb->heap_limit = PRIV(default_match_context).heap_limit;
  }
else
  {
  if (mcontext->offset_limit != PCRE2_UNSET)
    {
    if ((re->overall_options & PCRE2_USE_OFFSET_LIMIT) == 0)
      return PCRE2_ERROR_BADOFFSETLIMIT;
    bumpalong_limit = subject + mcontext->offset_limit;
    }
  mb->callout = mcontext->callout;
  mb->callout_data = mcontext->callout_data;
  mb->memctl = mcontext->memctl;
  mb->match_limit = mcontext->match_limit;
  mb->match_limit_depth = mcontext->depth_limit;
  mb->heap_limit = mcontext->heap_limit;
  }

if (mb->match_limit > re->limit_match)
  mb->match_limit = re->limit_match;

if (mb->match_limit_depth > re->limit_depth)
  mb->match_limit_depth = re->limit_depth;

if (mb->heap_limit > re->limit_heap)
  mb->heap_limit = re->limit_heap;

mb->start_code = (PCRE2_UCHAR *)((uint8_t *)re + sizeof(pcre2_real_code)) +
  re->name_count * re->name_entry_size;
mb->tables = re->tables;
mb->start_subject = subject;
mb->end_subject = end_subject;
mb->start_offset = start_offset;
mb->allowemptypartial = (re->max_lookbehind > 0) ||
  (re->flags & PCRE2_MATCH_EMPTY) != 0;
mb->moptions = options;
mb->poptions = re->overall_options;
mb->match_call_count = 0;
mb->heap_used = 0;

mb->bsr_convention = re->bsr_convention;
mb->nltype = NLTYPE_FIXED;
switch(re->newline_convention)
  {
  case PCRE2_NEWLINE_CR:
  mb->nllen = 1;
  mb->nl[0] = CHAR_CR;
  break;

  case PCRE2_NEWLINE_LF:
  mb->nllen = 1;
  mb->nl[0] = CHAR_NL;
  break;

  case PCRE2_NEWLINE_NUL:
  mb->nllen = 1;
  mb->nl[0] = CHAR_NUL;
  break;

  case PCRE2_NEWLINE_CRLF:
  mb->nllen = 2;
  mb->nl[0] = CHAR_CR;
  mb->nl[1] = CHAR_NL;
  break;

  case PCRE2_NEWLINE_ANY:
  mb->nltype = NLTYPE_ANY;
  break;

  case PCRE2_NEWLINE_ANYCRLF:
  mb->nltype = NLTYPE_ANYCRLF;
  break;

  default: return PCRE2_ERROR_INTERNAL;
  }

#ifdef SUPPORT_UNICODE
if (utf && (options & PCRE2_NO_UTF_CHECK) == 0)
  {
  PCRE2_SPTR check_subject = start_match;  /* start_match includes offset */

  if (start_offset > 0)
    {
#if PCRE2_CODE_UNIT_WIDTH != 32
    unsigned int i;
    if (start_match < end_subject && NOT_FIRSTCU(*start_match))
      return PCRE2_ERROR_BADUTFOFFSET;
    for (i = re->max_lookbehind; i > 0 && check_subject > subject; i--)
      {
      check_subject--;
      while (check_subject > subject &&
#if PCRE2_CODE_UNIT_WIDTH == 8
      (*check_subject & 0xc0) == 0x80)
#else  /* 16-bit */
      (*check_subject & 0xfc00) == 0xdc00)
#endif /* PCRE2_CODE_UNIT_WIDTH == 8 */
        check_subject--;
      }
#else   /* In the 32-bit library, one code unit equals one character. */
    check_subject -= re->max_lookbehind;
    if (check_subject < subject) check_subject = subject;
#endif  /* PCRE2_CODE_UNIT_WIDTH != 32 */
    }

  match_data->rc = PRIV(valid_utf)(check_subject,
    length - (PCRE2_SIZE)(check_subject - subject), &(match_data->startchar));
  if (match_data->rc != 0)
    {
    match_data->startchar += (PCRE2_SIZE)(check_subject - subject);
    return match_data->rc;
    }
  }
#endif  /* SUPPORT_UNICODE */

if ((re->flags & PCRE2_FIRSTSET) != 0)
  {
  has_first_cu = TRUE;
  first_cu = first_cu2 = (PCRE2_UCHAR)(re->first_codeunit);
  if ((re->flags & PCRE2_FIRSTCASELESS) != 0)
    {
    first_cu2 = TABLE_GET(first_cu, mb->tables + fcc_offset, first_cu);
#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
    if (first_cu > 127 && !utf && (re->overall_options & PCRE2_UCP) != 0)
      first_cu2 = (PCRE2_UCHAR)UCD_OTHERCASE(first_cu);
#else
    if (first_cu > 127 && (utf || (re->overall_options & PCRE2_UCP) != 0))
      first_cu2 = (PCRE2_UCHAR)UCD_OTHERCASE(first_cu);
#endif
#endif  /* SUPPORT_UNICODE */
    }
  }
else
  if (!startline && (re->flags & PCRE2_FIRSTMAPSET) != 0)
    start_bits = re->start_bitmap;

if ((re->flags & PCRE2_LASTSET) != 0)
  {
  has_req_cu = TRUE;
  req_cu = req_cu2 = (PCRE2_UCHAR)(re->last_codeunit);
  if ((re->flags & PCRE2_LASTCASELESS) != 0)
    {
    req_cu2 = TABLE_GET(req_cu, mb->tables + fcc_offset, req_cu);
#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
    if (req_cu > 127 && !utf && (re->overall_options & PCRE2_UCP) != 0)
      req_cu2 = (PCRE2_UCHAR)UCD_OTHERCASE(req_cu);
#else
    if (req_cu > 127 && (utf || (re->overall_options & PCRE2_UCP) != 0))
      req_cu2 = (PCRE2_UCHAR)UCD_OTHERCASE(req_cu);
#endif
#endif  /* SUPPORT_UNICODE */
    }
  }

if ((match_data->flags & PCRE2_MD_COPIED_SUBJECT) != 0)
  {
  match_data->memctl.free((void *)match_data->subject,
    match_data->memctl.memory_data);
  match_data->flags &= ~PCRE2_MD_COPIED_SUBJECT;
  }

match_data->code = re;
match_data->subject = NULL;  /* Default for no match */
match_data->mark = NULL;
match_data->matchedby = PCRE2_MATCHEDBY_DFA_INTERPRETER;

for (;;)
  {

  if ((re->overall_options & PCRE2_NO_START_OPTIMIZE) == 0 &&
      (options & PCRE2_DFA_RESTART) == 0)
    {

    if (firstline)
      {
      PCRE2_SPTR t = start_match;
#ifdef SUPPORT_UNICODE
      if (utf)
        {
        while (t < end_subject && !IS_NEWLINE(t))
          {
          t++;
          ACROSSCHAR(t < end_subject, t, t++);
          }
        }
      else
#endif
      while (t < end_subject && !IS_NEWLINE(t)) t++;
      end_subject = t;
      }

    if (anchored)
      {
      if (has_first_cu || start_bits != NULL)
        {
        BOOL ok = start_match < end_subject;
        if (ok)
          {
          PCRE2_UCHAR c = UCHAR21TEST(start_match);
          ok = has_first_cu && (c == first_cu || c == first_cu2);
          if (!ok && start_bits != NULL)
            {
#if PCRE2_CODE_UNIT_WIDTH != 8
            if (c > 255) c = 255;
#endif
            ok = (start_bits[c/8] & (1u << (c&7))) != 0;
            }
          }
        if (!ok) break;
        }
      }

    else
      {
      if (has_first_cu)
        {
        if (first_cu != first_cu2)  /* Caseless */
          {
#if PCRE2_CODE_UNIT_WIDTH != 8
          PCRE2_UCHAR smc;
          while (start_match < end_subject &&
                (smc = UCHAR21TEST(start_match)) != first_cu &&
                  smc != first_cu2)
            start_match++;

#else  /* 8-bit code units */
          PCRE2_SPTR pp1 = NULL;
          PCRE2_SPTR pp2 = NULL;
          PCRE2_SIZE cu2size = end_subject - start_match;

          if (!memchr_not_found_first_cu)
            {
            pp1 = memchr(start_match, first_cu, end_subject - start_match);
            if (pp1 == NULL) memchr_not_found_first_cu = TRUE;
              else cu2size = pp1 - start_match;
            }

          if (!memchr_not_found_first_cu2)
            {
            pp2 = memchr(start_match, first_cu2, cu2size);
            memchr_not_found_first_cu2 = (pp2 == NULL && pp1 == NULL);
            }

          if (pp1 == NULL)
            start_match = (pp2 == NULL)? end_subject : pp2;
          else
            start_match = (pp2 == NULL || pp1 < pp2)? pp1 : pp2;
#endif
          }

        /* The caseful case */

        else
          {
#if PCRE2_CODE_UNIT_WIDTH != 8
          while (start_match < end_subject && UCHAR21TEST(start_match) !=
                 first_cu)
            start_match++;
#else  /* 8-bit code units */
          start_match = memchr(start_match, first_cu, end_subject - start_match);
          if (start_match == NULL) start_match = end_subject;
#endif
          }

        if ((mb->moptions & (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT)) == 0 &&
            start_match >= mb->end_subject)
          break;
        }

      else if (startline)
        {
        if (start_match > mb->start_subject + start_offset)
          {
#ifdef SUPPORT_UNICODE
          if (utf)
            {
            while (start_match < end_subject && !WAS_NEWLINE(start_match))
              {
              start_match++;
              ACROSSCHAR(start_match < end_subject, start_match, start_match++);
              }
            }
          else
#endif
          while (start_match < end_subject && !WAS_NEWLINE(start_match))
            start_match++;

          if (start_match[-1] == CHAR_CR &&
               (mb->nltype == NLTYPE_ANY || mb->nltype == NLTYPE_ANYCRLF) &&
               start_match < end_subject &&
               UCHAR21TEST(start_match) == CHAR_NL)
            start_match++;
          }
        }

      else if (start_bits != NULL)
        {
        while (start_match < end_subject)
          {
          uint32_t c = UCHAR21TEST(start_match);
#if PCRE2_CODE_UNIT_WIDTH != 8
          if (c > 255) c = 255;
#endif
          if ((start_bits[c/8] & (1u << (c&7))) != 0) break;
          start_match++;
          }

        if ((mb->moptions & (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT)) == 0 &&
            start_match >= mb->end_subject)
          break;
        }
      }  /* End of first code unit handling */

    end_subject = mb->end_subject;

    if ((mb->moptions & (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT)) == 0)
      {
      PCRE2_SPTR p;

      if (end_subject - start_match < re->minlength) goto NOMATCH_EXIT;

      p = start_match + (has_first_cu? 1:0);
      if (has_req_cu && p > req_cu_ptr)
        {
        PCRE2_SIZE check_length = end_subject - start_match;

        if (check_length < REQ_CU_MAX ||
              (!anchored && check_length < REQ_CU_MAX * 1000))
          {
          if (req_cu != req_cu2)  /* Caseless */
            {
#if PCRE2_CODE_UNIT_WIDTH != 8
            while (p < end_subject)
              {
              uint32_t pp = UCHAR21INCTEST(p);
              if (pp == req_cu || pp == req_cu2) { p--; break; }
              }
#else  /* 8-bit code units */
            PCRE2_SPTR pp = p;
            p = memchr(pp, req_cu, end_subject - pp);
            if (p == NULL)
              {
              p = memchr(pp, req_cu2, end_subject - pp);
              if (p == NULL) p = end_subject;
              }
#endif /* PCRE2_CODE_UNIT_WIDTH != 8 */
            }

          /* The caseful case */

          else
            {
#if PCRE2_CODE_UNIT_WIDTH != 8
            while (p < end_subject)
              {
              if (UCHAR21INCTEST(p) == req_cu) { p--; break; }
              }

#else  /* 8-bit code units */
            p = memchr(p, req_cu, end_subject - p);
            if (p == NULL) p = end_subject;
#endif
            }

          if (p >= end_subject) break;

          req_cu_ptr = p;
          }
        }
      }
    }

  if (start_match > bumpalong_limit) break;

  mb->start_used_ptr = start_match;
  mb->last_used_ptr = start_match;
  mb->recursive = NULL;

  rc = internal_dfa_match(
    mb,                           /* fixed match data */
    mb->start_code,               /* this subexpression's code */
    start_match,                  /* where we currently are */
    start_offset,                 /* start offset in subject */
    match_data->ovector,          /* offset vector */
    (uint32_t)match_data->oveccount * 2,  /* actual size of same */
    workspace,                    /* workspace vector */
    (int)wscount,                 /* size of same */
    0,                            /* function recurse level */
    base_recursion_workspace);    /* initial workspace for recursion */

  if (rc != PCRE2_ERROR_NOMATCH || anchored)
    {
    if (rc == PCRE2_ERROR_PARTIAL && match_data->oveccount > 0)
      {
      match_data->ovector[0] = (PCRE2_SIZE)(start_match - subject);
      match_data->ovector[1] = (PCRE2_SIZE)(end_subject - subject);
      }
    match_data->leftchar = (PCRE2_SIZE)(mb->start_used_ptr - subject);
    match_data->rightchar = (PCRE2_SIZE)( mb->last_used_ptr - subject);
    match_data->startchar = (PCRE2_SIZE)(start_match - subject);
    match_data->rc = rc;

    if (rc >= 0 &&(options & PCRE2_COPY_MATCHED_SUBJECT) != 0)
      {
      length = CU2BYTES(length + was_zero_terminated);
      match_data->subject = match_data->memctl.malloc(length,
        match_data->memctl.memory_data);
      if (match_data->subject == NULL) return PCRE2_ERROR_NOMEMORY;
      memcpy((void *)match_data->subject, subject, length);
      match_data->flags |= PCRE2_MD_COPIED_SUBJECT;
      }
    else
      {
      if (rc >= 0 || rc == PCRE2_ERROR_PARTIAL) match_data->subject = subject;
      }
    goto EXIT;
    }

  if (firstline && IS_NEWLINE(start_match)) break;
  start_match++;
#ifdef SUPPORT_UNICODE
  if (utf)
    {
    ACROSSCHAR(start_match < end_subject, start_match, start_match++);
    }
#endif
  if (start_match > end_subject) break;

  if (UCHAR21TEST(start_match - 1) == CHAR_CR &&
      start_match < end_subject &&
      UCHAR21TEST(start_match) == CHAR_NL &&
      (re->flags & PCRE2_HASCRORLF) == 0 &&
        (mb->nltype == NLTYPE_ANY ||
         mb->nltype == NLTYPE_ANYCRLF ||
         mb->nllen == 2))
    start_match++;

  }   /* "Bumpalong" loop */

NOMATCH_EXIT:
rc = PCRE2_ERROR_NOMATCH;

EXIT:
while (rws->next != NULL)
  {
  RWS_anchor *next = rws->next;
  rws->next = next->next;
  mb->memctl.free(next, mb->memctl.memory_data);
  }

return rc;
}

/* End of pcre2_dfa_match.c */
