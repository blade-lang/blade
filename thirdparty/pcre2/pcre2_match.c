/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
     Original API code Copyright (c) 1997-2012 University of Cambridge
          New API code Copyright (c) 2015-2020 University of Cambridge

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

#ifdef DEBUG_FRAME_DISPLAY
#include <stdarg.h>
#endif

#define NLBLOCK mb              /* Block containing newline information */
#define PSSTART start_subject   /* Field containing processed string start */
#define PSEND   end_subject     /* Field containing processed string end */

#include "pcre2_internal.h"

#define RECURSE_UNSET 0xffffffffu  /* Bigger than max group number */

#define PUBLIC_MATCH_OPTIONS \
  (PCRE2_ANCHORED|PCRE2_ENDANCHORED|PCRE2_NOTBOL|PCRE2_NOTEOL|PCRE2_NOTEMPTY| \
   PCRE2_NOTEMPTY_ATSTART|PCRE2_NO_UTF_CHECK|PCRE2_PARTIAL_HARD| \
   PCRE2_PARTIAL_SOFT|PCRE2_NO_JIT|PCRE2_COPY_MATCHED_SUBJECT)

#define PUBLIC_JIT_MATCH_OPTIONS \
   (PCRE2_NO_UTF_CHECK|PCRE2_NOTBOL|PCRE2_NOTEOL|PCRE2_NOTEMPTY|\
    PCRE2_NOTEMPTY_ATSTART|PCRE2_PARTIAL_SOFT|PCRE2_PARTIAL_HARD|\
    PCRE2_COPY_MATCHED_SUBJECT)

#define MATCH_MATCH        1
#define MATCH_NOMATCH      0

#define MATCH_ACCEPT       (-999)
#define MATCH_KETRPOS      (-998)
/* The next 5 must be kept together and in sequence so that a test that checks
for any one of them can use a range. */
#define MATCH_COMMIT       (-997)
#define MATCH_PRUNE        (-996)
#define MATCH_SKIP         (-995)
#define MATCH_SKIP_ARG     (-994)
#define MATCH_THEN         (-993)
#define MATCH_BACKTRACK_MAX MATCH_THEN
#define MATCH_BACKTRACK_MIN MATCH_COMMIT

#define GF_CAPTURE     0x00010000u
#define GF_NOCAPTURE   0x00020000u
#define GF_CONDASSERT  0x00030000u
#define GF_RECURSE     0x00040000u

#define GF_IDMASK(a)   ((a) & 0xffff0000u)
#define GF_DATAMASK(a) ((a) & 0x0000ffffu)

/* Repetition types */

enum { REPTYPE_MIN, REPTYPE_MAX, REPTYPE_POS };

static const uint32_t rep_min[] = {
  0, 0,       /* * and *? */
  1, 1,       /* + and +? */
  0, 0,       /* ? and ?? */
  0, 0,       /* dummy placefillers for OP_CR[MIN]RANGE */
  0, 1, 0 };  /* OP_CRPOS{STAR, PLUS, QUERY} */

static const uint32_t rep_max[] = {
  UINT32_MAX, UINT32_MAX,      /* * and *? */
  UINT32_MAX, UINT32_MAX,      /* + and +? */
  1, 1,                        /* ? and ?? */
  0, 0,                        /* dummy placefillers for OP_CR[MIN]RANGE */
  UINT32_MAX, UINT32_MAX, 1 }; /* OP_CRPOS{STAR, PLUS, QUERY} */

/* Repetition types - must include OP_CRPOSRANGE (not needed above) */

static const uint32_t rep_typ[] = {
  REPTYPE_MAX, REPTYPE_MIN,    /* * and *? */
  REPTYPE_MAX, REPTYPE_MIN,    /* + and +? */
  REPTYPE_MAX, REPTYPE_MIN,    /* ? and ?? */
  REPTYPE_MAX, REPTYPE_MIN,    /* OP_CRRANGE and OP_CRMINRANGE */
  REPTYPE_POS, REPTYPE_POS,    /* OP_CRPOSSTAR, OP_CRPOSPLUS */
  REPTYPE_POS, REPTYPE_POS };  /* OP_CRPOSQUERY, OP_CRPOSRANGE */

enum { RM1=1, RM2,  RM3,  RM4,  RM5,  RM6,  RM7,  RM8,  RM9,  RM10,
       RM11,  RM12, RM13, RM14, RM15, RM16, RM17, RM18, RM19, RM20,
       RM21,  RM22, RM23, RM24, RM25, RM26, RM27, RM28, RM29, RM30,
       RM31,  RM32, RM33, RM34, RM35, RM36 };

#ifdef SUPPORT_WIDE_CHARS
enum { RM100=100, RM101 };
#endif

#ifdef SUPPORT_UNICODE
enum { RM200=200, RM201, RM202, RM203, RM204, RM205, RM206, RM207,
       RM208,     RM209, RM210, RM211, RM212, RM213, RM214, RM215,
       RM216,     RM217, RM218, RM219, RM220, RM221, RM222 };
#endif

#define Fback_frame        F->back_frame
#define Fcapture_last      F->capture_last
#define Fcurrent_recurse   F->current_recurse
#define Fecode             F->ecode
#define Feptr              F->eptr
#define Fgroup_frame_type  F->group_frame_type
#define Flast_group_offset F->last_group_offset
#define Flength            F->length
#define Fmark              F->mark
#define Frdepth            F->rdepth
#define Fstart_match       F->start_match
#define Foffset_top        F->offset_top
#define Foccu              F->occu
#define Fop                F->op
#define Fovector           F->ovector
#define Freturn_id         F->return_id


#ifdef DEBUG_FRAMES_DISPLAY

static void
display_frames(FILE *f, heapframe *F, heapframe *P, PCRE2_SIZE frame_size,
  match_block *mb, const char *s, ...)
{
uint32_t i;
heapframe *Q;
va_list ap;
va_start(ap, s);

fprintf(f, "FRAMES ");
vfprintf(f, s, ap);
va_end(ap);

if (P != NULL) fprintf(f, " P=%lu",
  ((char *)P - (char *)(mb->match_frames))/frame_size);
fprintf(f, "\n");

for (i = 0, Q = mb->match_frames;
     Q <= F;
     i++, Q = (heapframe *)((char *)Q + frame_size))
  {
  fprintf(f, "Frame %d type=%x subj=%lu code=%d back=%lu id=%d",
    i, Q->group_frame_type, Q->eptr - mb->start_subject, *(Q->ecode),
    Q->back_frame, Q->return_id);

  if (Q->last_group_offset == PCRE2_UNSET)
    fprintf(f, " lgoffset=unset\n");
  else
    fprintf(f, " lgoffset=%lu\n",  Q->last_group_offset/frame_size);
  }
}

#endif

/*************************************************
*                Process a callout               *
*************************************************/

static int
do_callout(heapframe *F, match_block *mb, PCRE2_SIZE *lengthptr)
{
int rc;
PCRE2_SIZE save0, save1;
PCRE2_SIZE *callout_ovector;
pcre2_callout_block *cb;

*lengthptr = (*Fecode == OP_CALLOUT)?
  PRIV(OP_lengths)[OP_CALLOUT] : GET(Fecode, 1 + 2*LINK_SIZE);

if (mb->callout == NULL) return 0;   /* No callout function provided */

callout_ovector = (PCRE2_SIZE *)(Fovector) - 2;

cb = mb->cb;
cb->capture_top      = (uint32_t)Foffset_top/2 + 1;
cb->capture_last     = Fcapture_last;
cb->offset_vector    = callout_ovector;
cb->mark             = mb->nomatch_mark;
cb->current_position = (PCRE2_SIZE)(Feptr - mb->start_subject);
cb->pattern_position = GET(Fecode, 1);
cb->next_item_length = GET(Fecode, 1 + LINK_SIZE);

if (*Fecode == OP_CALLOUT)  /* Numerical callout */
  {
  cb->callout_number = Fecode[1 + 2*LINK_SIZE];
  cb->callout_string_offset = 0;
  cb->callout_string = NULL;
  cb->callout_string_length = 0;
  }
else  /* String callout */
  {
  cb->callout_number = 0;
  cb->callout_string_offset = GET(Fecode, 1 + 3*LINK_SIZE);
  cb->callout_string = Fecode + (1 + 4*LINK_SIZE) + 1;
  cb->callout_string_length =
    *lengthptr - (1 + 4*LINK_SIZE) - 2;
  }

save0 = callout_ovector[0];
save1 = callout_ovector[1];
callout_ovector[0] = callout_ovector[1] = PCRE2_UNSET;
rc = mb->callout(cb, mb->callout_data);
callout_ovector[0] = save0;
callout_ovector[1] = save1;
cb->callout_flags = 0;
return rc;
}

/*************************************************
*          Match a back-reference                *
*************************************************/

static int
match_ref(PCRE2_SIZE offset, BOOL caseless, heapframe *F, match_block *mb,
  PCRE2_SIZE *lengthptr)
{
PCRE2_SPTR p;
PCRE2_SIZE length;
PCRE2_SPTR eptr;
PCRE2_SPTR eptr_start;

if (offset >= Foffset_top || Fovector[offset] == PCRE2_UNSET)
  {
  if ((mb->poptions & PCRE2_MATCH_UNSET_BACKREF) != 0)
    {
    *lengthptr = 0;
    return 0;      /* Match */
    }
  else return -1;  /* No match */
  }

eptr = eptr_start = Feptr;
p = mb->start_subject + Fovector[offset];
length = Fovector[offset+1] - Fovector[offset];

if (caseless)
  {
#if defined SUPPORT_UNICODE
  BOOL utf = (mb->poptions & PCRE2_UTF) != 0;

  if (utf || (mb->poptions & PCRE2_UCP) != 0)
    {
    PCRE2_SPTR endptr = p + length;

    while (p < endptr)
      {
      uint32_t c, d;
      const ucd_record *ur;
      if (eptr >= mb->end_subject) return 1;   /* Partial match */

      if (utf)
        {
        GETCHARINC(c, eptr);
        GETCHARINC(d, p);
        }
      else
        {
        c = *eptr++;
        d = *p++;
        }

      ur = GET_UCD(d);
      if (c != d && c != (uint32_t)((int)d + ur->other_case))
        {
        const uint32_t *pp = PRIV(ucd_caseless_sets) + ur->caseset;
        for (;;)
          {
          if (c < *pp) return -1;  /* No match */
          if (c == *pp++) break;
          }
        }
      }
    }
  else
#endif

    {
    for (; length > 0; length--)
      {
      uint32_t cc, cp;
      if (eptr >= mb->end_subject) return 1;   /* Partial match */
      cc = UCHAR21TEST(eptr);
      cp = UCHAR21TEST(p);
      if (TABLE_GET(cp, mb->lcc, cp) != TABLE_GET(cc, mb->lcc, cc))
        return -1;  /* No match */
      p++;
      eptr++;
      }
    }
  }

else
  {
  if (mb->partial != 0)
    {
    for (; length > 0; length--)
      {
      if (eptr >= mb->end_subject) return 1;   /* Partial match */
      if (UCHAR21INCTEST(p) != UCHAR21INCTEST(eptr)) return -1;  /* No match */
      }
    }

  else
    {
    if ((PCRE2_SIZE)(mb->end_subject - eptr) < length) return 1; /* Partial */
    if (memcmp(p, eptr, CU2BYTES(length)) != 0) return -1;  /* No match */
    eptr += length;
    }
  }

*lengthptr = eptr - eptr_start;
return 0;  /* Match */
}

/*************************************************
*       Macros for the match() function          *
*************************************************/

#define CHECK_PARTIAL()\
  if (Feptr >= mb->end_subject) \
    { \
    SCHECK_PARTIAL(); \
    }

#define SCHECK_PARTIAL()\
  if (mb->partial != 0 && \
      (Feptr > mb->start_used_ptr || mb->allowemptypartial)) \
    { \
    mb->hitend = TRUE; \
    if (mb->partial > 1) return PCRE2_ERROR_PARTIAL; \
    }

#define RMATCH(ra,rb)\
  {\
  start_ecode = ra;\
  Freturn_id = rb;\
  goto MATCH_RECURSE;\
  L_##rb:;\
  }

#define RRETURN(ra)\
  {\
  rrc = ra;\
  goto RETURN_SWITCH;\
  }

/*************************************************
*         Match from current position            *
*************************************************/

static int
match(PCRE2_SPTR start_eptr, PCRE2_SPTR start_ecode, PCRE2_SIZE *ovector,
  uint16_t oveccount, uint16_t top_bracket, PCRE2_SIZE frame_size,
  match_block *mb)
{
/* Frame-handling variables */

heapframe *F;           /* Current frame pointer */
heapframe *N = NULL;    /* Temporary frame pointers */
heapframe *P = NULL;
heapframe *assert_accept_frame = NULL;  /* For passing back a frame with captures */
PCRE2_SIZE frame_copy_size;     /* Amount to copy when creating a new frame */

PCRE2_SPTR bracode;     /* Temp pointer to start of group */
PCRE2_SIZE offset;      /* Used for group offsets */
PCRE2_SIZE length;      /* Used for various length calculations */

int rrc;                /* Return from functions & backtracking "recursions" */
#ifdef SUPPORT_UNICODE
int proptype;           /* Type of character property */
#endif

uint32_t i;             /* Used for local loops */
uint32_t fc;            /* Character values */
uint32_t number;        /* Used for group and other numbers */
uint32_t reptype = 0;   /* Type of repetition (0 to avoid compiler warning) */
uint32_t group_frame_type;  /* Specifies type for new group frames */

BOOL condition;         /* Used in conditional groups */
BOOL cur_is_word;       /* Used in "word" tests */
BOOL prev_is_word;      /* Used in "word" tests */

/* UTF and UCP flags */

#ifdef SUPPORT_UNICODE
BOOL utf = (mb->poptions & PCRE2_UTF) != 0;
BOOL ucp = (mb->poptions & PCRE2_UCP) != 0;
#else
BOOL utf = FALSE;  /* Required for convenience even when no Unicode support */
#endif

frame_copy_size = frame_size - offsetof(heapframe, eptr);

F = mb->match_frames;
Frdepth = 0;                        /* "Recursion" depth */
Fcapture_last = 0;                  /* Number of most recent capture */
Fcurrent_recurse = RECURSE_UNSET;   /* Not pattern recursing. */
Fstart_match = Feptr = start_eptr;  /* Current data pointer and start match */
Fmark = NULL;                       /* Most recent mark */
Foffset_top = 0;                    /* End of captures within the frame */
Flast_group_offset = PCRE2_UNSET;   /* Saved frame of most recent group */
group_frame_type = 0;               /* Not a start of group frame */
goto NEW_FRAME;                     /* Start processing with this frame */

MATCH_RECURSE:

N = (heapframe *)((char *)F + frame_size);
if (N >= mb->match_frames_top)
  {
  PCRE2_SIZE newsize = mb->frame_vector_size * 2;
  heapframe *new;

  if ((newsize / 1024) > mb->heap_limit)
    {
    PCRE2_SIZE maxsize = ((mb->heap_limit * 1024)/frame_size) * frame_size;
    if (mb->frame_vector_size >= maxsize) return PCRE2_ERROR_HEAPLIMIT;
    newsize = maxsize;
    }

  new = mb->memctl.malloc(newsize, mb->memctl.memory_data);
  if (new == NULL) return PCRE2_ERROR_NOMEMORY;
  memcpy(new, mb->match_frames, mb->frame_vector_size);

  F = (heapframe *)((char *)new + ((char *)F - (char *)mb->match_frames));
  N = (heapframe *)((char *)F + frame_size);

  if (mb->match_frames != mb->stack_frames)
    mb->memctl.free(mb->match_frames, mb->memctl.memory_data);
  mb->match_frames = new;
  mb->match_frames_top = (heapframe *)((char *)mb->match_frames + newsize);
  mb->frame_vector_size = newsize;
  }

#ifdef DEBUG_SHOW_RMATCH
fprintf(stderr, "++ RMATCH %2d frame=%d", Freturn_id, Frdepth + 1);
if (group_frame_type != 0)
  {
  fprintf(stderr, " type=%x ", group_frame_type);
  switch (GF_IDMASK(group_frame_type))
    {
    case GF_CAPTURE:
    fprintf(stderr, "capture=%d", GF_DATAMASK(group_frame_type));
    break;

    case GF_NOCAPTURE:
    fprintf(stderr, "nocapture op=%d", GF_DATAMASK(group_frame_type));
    break;

    case GF_CONDASSERT:
    fprintf(stderr, "condassert op=%d", GF_DATAMASK(group_frame_type));
    break;

    case GF_RECURSE:
    fprintf(stderr, "recurse=%d", GF_DATAMASK(group_frame_type));
    break;

    default:
    fprintf(stderr, "*** unknown ***");
    break;
    }
  }
fprintf(stderr, "\n");
#endif

memcpy((char *)N + offsetof(heapframe, eptr),
       (char *)F + offsetof(heapframe, eptr),
       frame_copy_size);

N->rdepth = Frdepth + 1;
F = N;

/* Carry on processing with a new frame. */

NEW_FRAME:
Fgroup_frame_type = group_frame_type;
Fecode = start_ecode;      /* Starting code pointer */
Fback_frame = frame_size;  /* Default is go back one frame */

if (group_frame_type != 0)
  {
  Flast_group_offset = (char *)F - (char *)mb->match_frames;
  if (GF_IDMASK(group_frame_type) == GF_RECURSE)
    Fcurrent_recurse = GF_DATAMASK(group_frame_type);
  group_frame_type = 0;
  }

if (mb->match_call_count++ >= mb->match_limit) return PCRE2_ERROR_MATCHLIMIT;
if (Frdepth >= mb->match_limit_depth) return PCRE2_ERROR_DEPTHLIMIT;

for (;;)
  {
#ifdef DEBUG_SHOW_OPS
fprintf(stderr, "++ op=%d\n", *Fecode);
#endif

  Fop = (uint8_t)(*Fecode);  /* Cast needed for 16-bit and 32-bit modes */
  switch(Fop)
    {

    case OP_CLOSE:
    if (Fcurrent_recurse == RECURSE_UNSET)
      {
      number = GET2(Fecode, 1);
      offset = Flast_group_offset;
      for(;;)
        {
        if (offset == PCRE2_UNSET) return PCRE2_ERROR_INTERNAL;
        N = (heapframe *)((char *)mb->match_frames + offset);
        P = (heapframe *)((char *)N - frame_size);
        if (N->group_frame_type == (GF_CAPTURE | number)) break;
        offset = P->last_group_offset;
        }
      offset = (number << 1) - 2;
      Fcapture_last = number;
      Fovector[offset] = P->eptr - mb->start_subject;
      Fovector[offset+1] = Feptr - mb->start_subject;
      if (offset >= Foffset_top) Foffset_top = offset + 2;
      }
    Fecode += PRIV(OP_lengths)[*Fecode];
    break;

    case OP_ASSERT_ACCEPT:
    if (Feptr > mb->last_used_ptr) mb->last_used_ptr = Feptr;
    assert_accept_frame = F;
    RRETURN(MATCH_ACCEPT);

    case OP_ACCEPT:
    case OP_END:

    /* Handle end of a recursion. */

    if (Fcurrent_recurse != RECURSE_UNSET)
      {
      offset = Flast_group_offset;
      for(;;)
        {
        if (offset == PCRE2_UNSET) return PCRE2_ERROR_INTERNAL;
        N = (heapframe *)((char *)mb->match_frames + offset);
        P = (heapframe *)((char *)N - frame_size);
        if (GF_IDMASK(N->group_frame_type) == GF_RECURSE) break;
        offset = P->last_group_offset;
        }

      P->eptr = Feptr;
      P->mark = Fmark;
      F = P;
      Fecode += 1 + LINK_SIZE;
      continue;
      }

    if (Feptr == Fstart_match &&
         ((mb->moptions & PCRE2_NOTEMPTY) != 0 ||
           ((mb->moptions & PCRE2_NOTEMPTY_ATSTART) != 0 &&
             Fstart_match == mb->start_subject + mb->start_offset)))
      RRETURN(MATCH_NOMATCH);

    if (Feptr < mb->end_subject &&
        ((mb->moptions | mb->poptions) & PCRE2_ENDANCHORED) != 0)
      {
      if (Fop == OP_END) RRETURN(MATCH_NOMATCH);
      return MATCH_NOMATCH;
      }

    mb->end_match_ptr = Feptr;           /* Record where we ended */
    mb->end_offset_top = Foffset_top;    /* and how many extracts were taken */
    mb->mark = Fmark;                    /* and the last success mark */
    if (Feptr > mb->last_used_ptr) mb->last_used_ptr = Feptr;

    ovector[0] = Fstart_match - mb->start_subject;
    ovector[1] = Feptr - mb->start_subject;

    i = 2 * ((top_bracket + 1 > oveccount)? oveccount : top_bracket + 1);
    memcpy(ovector + 2, Fovector, (i - 2) * sizeof(PCRE2_SIZE));
    while (--i >= Foffset_top + 2) ovector[i] = PCRE2_UNSET;
    return MATCH_MATCH;  /* Note: NOT RRETURN */

    case OP_ANY:
    if (IS_NEWLINE(Feptr)) RRETURN(MATCH_NOMATCH);
    if (mb->partial != 0 &&
        Feptr == mb->end_subject - 1 &&
        NLBLOCK->nltype == NLTYPE_FIXED &&
        NLBLOCK->nllen == 2 &&
        UCHAR21TEST(Feptr) == NLBLOCK->nl[0])
      {
      mb->hitend = TRUE;
      if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
      }

    case OP_ALLANY:
    if (Feptr >= mb->end_subject)  /* DO NOT merge the Feptr++ here; it must */
      {                            /* not be updated before SCHECK_PARTIAL. */
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    Feptr++;
#ifdef SUPPORT_UNICODE
    if (utf) ACROSSCHAR(Feptr < mb->end_subject, Feptr, Feptr++);
#endif
    Fecode++;
    break;

    case OP_ANYBYTE:
    if (Feptr >= mb->end_subject)   /* DO NOT merge the Feptr++ here; it must */
      {                             /* not be updated before SCHECK_PARTIAL. */
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    Feptr++;
    Fecode++;
    break;

    case OP_CHAR:
#ifdef SUPPORT_UNICODE
    if (utf)
      {
      Flength = 1;
      Fecode++;
      GETCHARLEN(fc, Fecode, Flength);
      if (Flength > (PCRE2_SIZE)(mb->end_subject - Feptr))
        {
        CHECK_PARTIAL();             /* Not SCHECK_PARTIAL() */
        RRETURN(MATCH_NOMATCH);
        }
      for (; Flength > 0; Flength--)
        {
        if (*Fecode++ != UCHAR21INC(Feptr)) RRETURN(MATCH_NOMATCH);
        }
      }
    else
#endif

    /* Not UTF mode */
      {
      if (mb->end_subject - Feptr < 1)
        {
        SCHECK_PARTIAL();            /* This one can use SCHECK_PARTIAL() */
        RRETURN(MATCH_NOMATCH);
        }
      if (Fecode[1] != *Feptr++) RRETURN(MATCH_NOMATCH);
      Fecode += 2;
      }
    break;

    case OP_CHARI:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }

#ifdef SUPPORT_UNICODE
    if (utf)
      {
      Flength = 1;
      Fecode++;
      GETCHARLEN(fc, Fecode, Flength);

      if (fc < 128)
        {
        uint32_t cc = UCHAR21(Feptr);
        if (mb->lcc[fc] != TABLE_GET(cc, mb->lcc, cc)) RRETURN(MATCH_NOMATCH);
        Fecode++;
        Feptr++;
        }

      else
        {
        uint32_t dc;
        GETCHARINC(dc, Feptr);
        Fecode += Flength;
        if (dc != fc && dc != UCD_OTHERCASE(fc)) RRETURN(MATCH_NOMATCH);
        }
      }

    else if (ucp)
      {
      uint32_t cc = UCHAR21(Feptr);
      fc = Fecode[1];
      if (fc < 128)
        {
        if (mb->lcc[fc] != TABLE_GET(cc, mb->lcc, cc)) RRETURN(MATCH_NOMATCH);
        }
      else
        {
        if (cc != fc && cc != UCD_OTHERCASE(fc)) RRETURN(MATCH_NOMATCH);
        }
      Feptr++;
      Fecode += 2;
      }

    else
#endif   /* SUPPORT_UNICODE */

      {
      if (TABLE_GET(Fecode[1], mb->lcc, Fecode[1])
          != TABLE_GET(*Feptr, mb->lcc, *Feptr)) RRETURN(MATCH_NOMATCH);
      Feptr++;
      Fecode += 2;
      }
    break;

    case OP_NOT:
    case OP_NOTI:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }

#ifdef SUPPORT_UNICODE
    if (utf)
      {
      uint32_t ch;
      Fecode++;
      GETCHARINC(ch, Fecode);
      GETCHARINC(fc, Feptr);
      if (ch == fc)
        {
        RRETURN(MATCH_NOMATCH);  /* Caseful match */
        }
      else if (Fop == OP_NOTI)   /* If caseless */
        {
        if (ch > 127)
          ch = UCD_OTHERCASE(ch);
        else
          ch = (mb->fcc)[ch];
        if (ch == fc) RRETURN(MATCH_NOMATCH);
        }
      }

    else if (ucp)
      {
      uint32_t ch;
      fc = UCHAR21INC(Feptr);
      ch = Fecode[1];
      Fecode += 2;

      if (ch == fc)
        {
        RRETURN(MATCH_NOMATCH);  /* Caseful match */
        }
      else if (Fop == OP_NOTI)   /* If caseless */
        {
        if (ch > 127)
          ch = UCD_OTHERCASE(ch);
        else
          ch = (mb->fcc)[ch];
        if (ch == fc) RRETURN(MATCH_NOMATCH);
        }
      }

    else
#endif  /* SUPPORT_UNICODE */

      {
      uint32_t ch = Fecode[1];
      fc = UCHAR21INC(Feptr);
      if (ch == fc || (Fop == OP_NOTI && TABLE_GET(ch, mb->fcc, ch) == fc))
        RRETURN(MATCH_NOMATCH);
      Fecode += 2;
      }
    break;

#define Loclength    F->temp_size
#define Lstart_eptr  F->temp_sptr[0]
#define Lcharptr     F->temp_sptr[1]
#define Lmin         F->temp_32[0]
#define Lmax         F->temp_32[1]
#define Lc           F->temp_32[2]
#define Loc          F->temp_32[3]

    case OP_EXACT:
    case OP_EXACTI:
    Lmin = Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATCHAR;

    case OP_POSUPTO:
    case OP_POSUPTOI:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATCHAR;

    case OP_UPTO:
    case OP_UPTOI:
    reptype = REPTYPE_MAX;
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATCHAR;

    case OP_MINUPTO:
    case OP_MINUPTOI:
    reptype = REPTYPE_MIN;
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATCHAR;

    case OP_POSSTAR:
    case OP_POSSTARI:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = UINT32_MAX;
    Fecode++;
    goto REPEATCHAR;

    case OP_POSPLUS:
    case OP_POSPLUSI:
    reptype = REPTYPE_POS;
    Lmin = 1;
    Lmax = UINT32_MAX;
    Fecode++;
    goto REPEATCHAR;

    case OP_POSQUERY:
    case OP_POSQUERYI:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = 1;
    Fecode++;
    goto REPEATCHAR;

    case OP_STAR:
    case OP_STARI:
    case OP_MINSTAR:
    case OP_MINSTARI:
    case OP_PLUS:
    case OP_PLUSI:
    case OP_MINPLUS:
    case OP_MINPLUSI:
    case OP_QUERY:
    case OP_QUERYI:
    case OP_MINQUERY:
    case OP_MINQUERYI:
    fc = *Fecode++ - ((Fop < OP_STARI)? OP_STAR : OP_STARI);
    Lmin = rep_min[fc];
    Lmax = rep_max[fc];
    reptype = rep_typ[fc];

    REPEATCHAR:
#ifdef SUPPORT_UNICODE
    if (utf)
      {
      Flength = 1;
      Lcharptr = Fecode;
      GETCHARLEN(fc, Fecode, Flength);
      Fecode += Flength;

      if (Flength > 1)
        {
        uint32_t othercase;

        if (Fop >= OP_STARI &&     /* Caseless */
            (othercase = UCD_OTHERCASE(fc)) != fc)
          Loclength = PRIV(ord2utf)(othercase, Foccu);
        else Loclength = 0;

        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr <= mb->end_subject - Flength &&
            memcmp(Feptr, Lcharptr, CU2BYTES(Flength)) == 0) Feptr += Flength;
          else if (Loclength > 0 &&
                   Feptr <= mb->end_subject - Loclength &&
                   memcmp(Feptr, Foccu, CU2BYTES(Loclength)) == 0)
            Feptr += Loclength;
          else
            {
            CHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          }

        if (Lmin == Lmax) continue;

        if (reptype == REPTYPE_MIN)
          {
          for (;;)
            {
            RMATCH(Fecode, RM202);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr <= mb->end_subject - Flength &&
              memcmp(Feptr, Lcharptr, CU2BYTES(Flength)) == 0) Feptr += Flength;
            else if (Loclength > 0 &&
                     Feptr <= mb->end_subject - Loclength &&
                     memcmp(Feptr, Foccu, CU2BYTES(Loclength)) == 0)
              Feptr += Loclength;
            else
              {
              CHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            }
          /* Control never gets here */
          }

        else  /* Maximize */
          {
          Lstart_eptr = Feptr;
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr <= mb->end_subject - Flength &&
                memcmp(Feptr, Lcharptr, CU2BYTES(Flength)) == 0)
              Feptr += Flength;
            else if (Loclength > 0 &&
                     Feptr <= mb->end_subject - Loclength &&
                     memcmp(Feptr, Foccu, CU2BYTES(Loclength)) == 0)
              Feptr += Loclength;
            else
              {
              CHECK_PARTIAL();
              break;
              }
            }

          if (reptype != REPTYPE_POS) for(;;)
            {
            if (Feptr <= Lstart_eptr) break;
            RMATCH(Fecode, RM203);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            Feptr--;
            BACKCHAR(Feptr);
            }
          }
        break;   /* End of repeated wide character handling */
        }

      Lc = fc;
      }
    else
#endif  /* SUPPORT_UNICODE */

    Lc = *Fecode++;

    if (Fop >= OP_STARI)
      {
#if PCRE2_CODE_UNIT_WIDTH == 8
#ifdef SUPPORT_UNICODE
      if (ucp && !utf && Lc > 127) Loc = UCD_OTHERCASE(Lc);
      else
#endif  /* SUPPORT_UNICODE */
      /* Lc will be < 128 in UTF-8 mode. */
      Loc = mb->fcc[Lc];
#else /* 16-bit & 32-bit */
#ifdef SUPPORT_UNICODE
      if ((utf || ucp) && Lc > 127) Loc = UCD_OTHERCASE(Lc);
      else
#endif  /* SUPPORT_UNICODE */
      Loc = TABLE_GET(Lc, mb->fcc, Lc);
#endif  /* PCRE2_CODE_UNIT_WIDTH == 8 */

      for (i = 1; i <= Lmin; i++)
        {
        uint32_t cc;                 /* Faster than PCRE2_UCHAR */
        if (Feptr >= mb->end_subject)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        cc = UCHAR21TEST(Feptr);
        if (Lc != cc && Loc != cc) RRETURN(MATCH_NOMATCH);
        Feptr++;
        }
      if (Lmin == Lmax) continue;

      if (reptype == REPTYPE_MIN)
        {
        for (;;)
          {
          uint32_t cc;               /* Faster than PCRE2_UCHAR */
          RMATCH(Fecode, RM25);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21TEST(Feptr);
          if (Lc != cc && Loc != cc) RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        /* Control never gets here */
        }

      else  /* Maximize */
        {
        Lstart_eptr = Feptr;
        for (i = Lmin; i < Lmax; i++)
          {
          uint32_t cc;               /* Faster than PCRE2_UCHAR */
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }
          cc = UCHAR21TEST(Feptr);
          if (Lc != cc && Loc != cc) break;
          Feptr++;
          }
        if (reptype != REPTYPE_POS) for (;;)
          {
          if (Feptr == Lstart_eptr) break;
          RMATCH(Fecode, RM26);
          Feptr--;
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          }
        }
      }

    else
      {
      for (i = 1; i <= Lmin; i++)
        {
        if (Feptr >= mb->end_subject)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        if (Lc != UCHAR21INCTEST(Feptr)) RRETURN(MATCH_NOMATCH);
        }

      if (Lmin == Lmax) continue;

      if (reptype == REPTYPE_MIN)
        {
        for (;;)
          {
          RMATCH(Fecode, RM27);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (Lc != UCHAR21INCTEST(Feptr)) RRETURN(MATCH_NOMATCH);
          }
        /* Control never gets here */
        }
      else  /* Maximize */
        {
        Lstart_eptr = Feptr;
        for (i = Lmin; i < Lmax; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }

          if (Lc != UCHAR21TEST(Feptr)) break;
          Feptr++;
          }

        if (reptype != REPTYPE_POS) for (;;)
          {
          if (Feptr <= Lstart_eptr) break;
          RMATCH(Fecode, RM28);
          Feptr--;
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          }
        }
      }
    break;

#undef Loclength
#undef Lstart_eptr
#undef Lcharptr
#undef Lmin
#undef Lmax
#undef Lc
#undef Loc

#define Lstart_eptr  F->temp_sptr[0]
#define Lmin         F->temp_32[0]
#define Lmax         F->temp_32[1]
#define Lc           F->temp_32[2]
#define Loc          F->temp_32[3]

    case OP_NOTEXACT:
    case OP_NOTEXACTI:
    Lmin = Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATNOTCHAR;

    case OP_NOTUPTO:
    case OP_NOTUPTOI:
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    reptype = REPTYPE_MAX;
    Fecode += 1 + IMM2_SIZE;
    goto REPEATNOTCHAR;

    case OP_NOTMINUPTO:
    case OP_NOTMINUPTOI:
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    reptype = REPTYPE_MIN;
    Fecode += 1 + IMM2_SIZE;
    goto REPEATNOTCHAR;

    case OP_NOTPOSSTAR:
    case OP_NOTPOSSTARI:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = UINT32_MAX;
    Fecode++;
    goto REPEATNOTCHAR;

    case OP_NOTPOSPLUS:
    case OP_NOTPOSPLUSI:
    reptype = REPTYPE_POS;
    Lmin = 1;
    Lmax = UINT32_MAX;
    Fecode++;
    goto REPEATNOTCHAR;

    case OP_NOTPOSQUERY:
    case OP_NOTPOSQUERYI:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = 1;
    Fecode++;
    goto REPEATNOTCHAR;

    case OP_NOTPOSUPTO:
    case OP_NOTPOSUPTOI:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATNOTCHAR;

    case OP_NOTSTAR:
    case OP_NOTSTARI:
    case OP_NOTMINSTAR:
    case OP_NOTMINSTARI:
    case OP_NOTPLUS:
    case OP_NOTPLUSI:
    case OP_NOTMINPLUS:
    case OP_NOTMINPLUSI:
    case OP_NOTQUERY:
    case OP_NOTQUERYI:
    case OP_NOTMINQUERY:
    case OP_NOTMINQUERYI:
    fc = *Fecode++ - ((Fop >= OP_NOTSTARI)? OP_NOTSTARI: OP_NOTSTAR);
    Lmin = rep_min[fc];
    Lmax = rep_max[fc];
    reptype = rep_typ[fc];

    REPEATNOTCHAR:
    GETCHARINCTEST(Lc, Fecode);

    if (Fop >= OP_NOTSTARI)     /* Caseless */
      {
#ifdef SUPPORT_UNICODE
      if ((utf || ucp) && Lc > 127)
        Loc = UCD_OTHERCASE(Lc);
      else
#endif /* SUPPORT_UNICODE */

      Loc = TABLE_GET(Lc, mb->fcc, Lc);  /* Other case from table */

#ifdef SUPPORT_UNICODE
      if (utf)
        {
        uint32_t d;
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(d, Feptr);
          if (Lc == d || Loc == d) RRETURN(MATCH_NOMATCH);
          }
        }
      else
#endif  /* SUPPORT_UNICODE */

      /* Not UTF mode */
        {
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (Lc == *Feptr || Loc == *Feptr) RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        }

      if (Lmin == Lmax) continue;  /* Finished for exact count */

      if (reptype == REPTYPE_MIN)
        {
#ifdef SUPPORT_UNICODE
        if (utf)
          {
          uint32_t d;
          for (;;)
            {
            RMATCH(Fecode, RM204);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINC(d, Feptr);
            if (Lc == d || Loc == d) RRETURN(MATCH_NOMATCH);
            }
          }
        else
#endif  /*SUPPORT_UNICODE */

        /* Not UTF mode */
          {
          for (;;)
            {
            RMATCH(Fecode, RM29);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            if (Lc == *Feptr || Loc == *Feptr) RRETURN(MATCH_NOMATCH);
            Feptr++;
            }
          }
        /* Control never gets here */
        }

      else
        {
        Lstart_eptr = Feptr;

#ifdef SUPPORT_UNICODE
        if (utf)
          {
          uint32_t d;
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(d, Feptr, len);
            if (Lc == d || Loc == d) break;
            Feptr += len;
            }

          if (reptype != REPTYPE_POS) for(;;)
            {
            if (Feptr <= Lstart_eptr) break;
            RMATCH(Fecode, RM205);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            Feptr--;
            BACKCHAR(Feptr);
            }
          }
        else
#endif  /* SUPPORT_UNICODE */

        /* Not UTF mode */
          {
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (Lc == *Feptr || Loc == *Feptr) break;
            Feptr++;
            }
          if (reptype != REPTYPE_POS) for (;;)
            {
            if (Feptr == Lstart_eptr) break;
            RMATCH(Fecode, RM30);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            Feptr--;
            }
          }
        }
      }

    else
      {
#ifdef SUPPORT_UNICODE
      if (utf)
        {
        uint32_t d;
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(d, Feptr);
          if (Lc == d) RRETURN(MATCH_NOMATCH);
          }
        }
      else
#endif
      /* Not UTF mode */
        {
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (Lc == *Feptr++) RRETURN(MATCH_NOMATCH);
          }
        }

      if (Lmin == Lmax) continue;

      if (reptype == REPTYPE_MIN)
        {
#ifdef SUPPORT_UNICODE
        if (utf)
          {
          uint32_t d;
          for (;;)
            {
            RMATCH(Fecode, RM206);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINC(d, Feptr);
            if (Lc == d) RRETURN(MATCH_NOMATCH);
            }
          }
        else
#endif
        /* Not UTF mode */
          {
          for (;;)
            {
            RMATCH(Fecode, RM31);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            if (Lc == *Feptr++) RRETURN(MATCH_NOMATCH);
            }
          }
        /* Control never gets here */
        }

      else
        {
        Lstart_eptr = Feptr;

#ifdef SUPPORT_UNICODE
        if (utf)
          {
          uint32_t d;
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(d, Feptr, len);
            if (Lc == d) break;
            Feptr += len;
            }

          if (reptype != REPTYPE_POS) for(;;)
            {
            if (Feptr <= Lstart_eptr) break;
            RMATCH(Fecode, RM207);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            Feptr--;
            BACKCHAR(Feptr);
            }
          }
        else
#endif
        /* Not UTF mode */
          {
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (Lc == *Feptr) break;
            Feptr++;
            }
          if (reptype != REPTYPE_POS) for (;;)
            {
            if (Feptr == Lstart_eptr) break;
            RMATCH(Fecode, RM32);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            Feptr--;
            }
          }
        }
      }
    break;

#undef Lstart_eptr
#undef Lmin
#undef Lmax
#undef Lc
#undef Loc

#define Lmin               F->temp_32[0]
#define Lmax               F->temp_32[1]
#define Lstart_eptr        F->temp_sptr[0]
#define Lbyte_map_address  F->temp_sptr[1]
#define Lbyte_map          ((unsigned char *)Lbyte_map_address)

    case OP_NCLASS:
    case OP_CLASS:
      {
      Lbyte_map_address = Fecode + 1;           /* Save for matching */
      Fecode += 1 + (32 / sizeof(PCRE2_UCHAR)); /* Advance past the item */

      switch (*Fecode)
        {
        case OP_CRSTAR:
        case OP_CRMINSTAR:
        case OP_CRPLUS:
        case OP_CRMINPLUS:
        case OP_CRQUERY:
        case OP_CRMINQUERY:
        case OP_CRPOSSTAR:
        case OP_CRPOSPLUS:
        case OP_CRPOSQUERY:
        fc = *Fecode++ - OP_CRSTAR;
        Lmin = rep_min[fc];
        Lmax = rep_max[fc];
        reptype = rep_typ[fc];
        break;

        case OP_CRRANGE:
        case OP_CRMINRANGE:
        case OP_CRPOSRANGE:
        Lmin = GET2(Fecode, 1);
        Lmax = GET2(Fecode, 1 + IMM2_SIZE);
        if (Lmax == 0) Lmax = UINT32_MAX;       /* Max 0 => infinity */
        reptype = rep_typ[*Fecode - OP_CRSTAR];
        Fecode += 1 + 2 * IMM2_SIZE;
        break;

        default:               /* No repeat follows */
        Lmin = Lmax = 1;
        break;
        }

#ifdef SUPPORT_UNICODE
      if (utf)
        {
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(fc, Feptr);
          if (fc > 255)
            {
            if (Fop == OP_CLASS) RRETURN(MATCH_NOMATCH);
            }
          else
            if ((Lbyte_map[fc/8] & (1u << (fc&7))) == 0) RRETURN(MATCH_NOMATCH);
          }
        }
      else
#endif
      /* Not UTF mode */
        {
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          fc = *Feptr++;
#if PCRE2_CODE_UNIT_WIDTH != 8
          if (fc > 255)
            {
            if (Fop == OP_CLASS) RRETURN(MATCH_NOMATCH);
            }
          else
#endif
          if ((Lbyte_map[fc/8] & (1u << (fc&7))) == 0) RRETURN(MATCH_NOMATCH);
          }
        }

      if (Lmin == Lmax) continue;

      if (reptype == REPTYPE_MIN)
        {
#ifdef SUPPORT_UNICODE
        if (utf)
          {
          for (;;)
            {
            RMATCH(Fecode, RM200);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINC(fc, Feptr);
            if (fc > 255)
              {
              if (Fop == OP_CLASS) RRETURN(MATCH_NOMATCH);
              }
            else
              if ((Lbyte_map[fc/8] & (1u << (fc&7))) == 0) RRETURN(MATCH_NOMATCH);
            }
          }
        else
#endif
        /* Not UTF mode */
          {
          for (;;)
            {
            RMATCH(Fecode, RM23);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            fc = *Feptr++;
#if PCRE2_CODE_UNIT_WIDTH != 8
            if (fc > 255)
              {
              if (Fop == OP_CLASS) RRETURN(MATCH_NOMATCH);
              }
            else
#endif
            if ((Lbyte_map[fc/8] & (1u << (fc&7))) == 0) RRETURN(MATCH_NOMATCH);
            }
          }
        /* Control never gets here */
        }

      else
        {
        Lstart_eptr = Feptr;

#ifdef SUPPORT_UNICODE
        if (utf)
          {
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc > 255)
              {
              if (Fop == OP_CLASS) break;
              }
            else
              if ((Lbyte_map[fc/8] & (1u << (fc&7))) == 0) break;
            Feptr += len;
            }

          if (reptype == REPTYPE_POS) continue;    /* No backtracking */

          for (;;)
            {
            RMATCH(Fecode, RM201);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Feptr-- <= Lstart_eptr) break;  /* Tried at original position */
            BACKCHAR(Feptr);
            }
          }
        else
#endif
          /* Not UTF mode */
          {
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            fc = *Feptr;
#if PCRE2_CODE_UNIT_WIDTH != 8
            if (fc > 255)
              {
              if (Fop == OP_CLASS) break;
              }
            else
#endif
            if ((Lbyte_map[fc/8] & (1u << (fc&7))) == 0) break;
            Feptr++;
            }

          if (reptype == REPTYPE_POS) continue;    /* No backtracking */

          while (Feptr >= Lstart_eptr)
            {
            RMATCH(Fecode, RM24);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            Feptr--;
            }
          }

        RRETURN(MATCH_NOMATCH);
        }
      }
    /* Control never gets here */

#undef Lbyte_map_address
#undef Lbyte_map
#undef Lstart_eptr
#undef Lmin
#undef Lmax

#define Lstart_eptr  F->temp_sptr[0]
#define Lxclass_data F->temp_sptr[1]
#define Lmin         F->temp_32[0]
#define Lmax         F->temp_32[1]

#ifdef SUPPORT_WIDE_CHARS
    case OP_XCLASS:
      {
      Lxclass_data = Fecode + 1 + LINK_SIZE;  /* Save for matching */
      Fecode += GET(Fecode, 1);               /* Advance past the item */

      switch (*Fecode)
        {
        case OP_CRSTAR:
        case OP_CRMINSTAR:
        case OP_CRPLUS:
        case OP_CRMINPLUS:
        case OP_CRQUERY:
        case OP_CRMINQUERY:
        case OP_CRPOSSTAR:
        case OP_CRPOSPLUS:
        case OP_CRPOSQUERY:
        fc = *Fecode++ - OP_CRSTAR;
        Lmin = rep_min[fc];
        Lmax = rep_max[fc];
        reptype = rep_typ[fc];
        break;

        case OP_CRRANGE:
        case OP_CRMINRANGE:
        case OP_CRPOSRANGE:
        Lmin = GET2(Fecode, 1);
        Lmax = GET2(Fecode, 1 + IMM2_SIZE);
        if (Lmax == 0) Lmax = UINT32_MAX;  /* Max 0 => infinity */
        reptype = rep_typ[*Fecode - OP_CRSTAR];
        Fecode += 1 + 2 * IMM2_SIZE;
        break;

        default:               /* No repeat follows */
        Lmin = Lmax = 1;
        break;
        }

      for (i = 1; i <= Lmin; i++)
        {
        if (Feptr >= mb->end_subject)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        GETCHARINCTEST(fc, Feptr);
        if (!PRIV(xclass)(fc, Lxclass_data, utf)) RRETURN(MATCH_NOMATCH);
        }

      if (Lmin == Lmax) continue;

      if (reptype == REPTYPE_MIN)
        {
        for (;;)
          {
          RMATCH(Fecode, RM100);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINCTEST(fc, Feptr);
          if (!PRIV(xclass)(fc, Lxclass_data, utf)) RRETURN(MATCH_NOMATCH);
          }
        /* Control never gets here */
        }

      else
        {
        Lstart_eptr = Feptr;
        for (i = Lmin; i < Lmax; i++)
          {
          int len = 1;
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }
#ifdef SUPPORT_UNICODE
          GETCHARLENTEST(fc, Feptr, len);
#else
          fc = *Feptr;
#endif
          if (!PRIV(xclass)(fc, Lxclass_data, utf)) break;
          Feptr += len;
          }

        if (reptype == REPTYPE_POS) continue;    /* No backtracking */

        for(;;)
          {
          RMATCH(Fecode, RM101);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Feptr-- <= Lstart_eptr) break;  /* Tried at original position */
#ifdef SUPPORT_UNICODE
          if (utf) BACKCHAR(Feptr);
#endif
          }
        RRETURN(MATCH_NOMATCH);
        }

      /* Control never gets here */
      }
#endif  /* SUPPORT_WIDE_CHARS: end of XCLASS */

#undef Lstart_eptr
#undef Lxclass_data
#undef Lmin
#undef Lmax

    case OP_NOT_DIGIT:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    if (CHMAX_255(fc) && (mb->ctypes[fc] & ctype_digit) != 0)
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_DIGIT:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    if (!CHMAX_255(fc) || (mb->ctypes[fc] & ctype_digit) == 0)
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_NOT_WHITESPACE:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    if (CHMAX_255(fc) && (mb->ctypes[fc] & ctype_space) != 0)
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_WHITESPACE:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    if (!CHMAX_255(fc) || (mb->ctypes[fc] & ctype_space) == 0)
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_NOT_WORDCHAR:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    if (CHMAX_255(fc) && (mb->ctypes[fc] & ctype_word) != 0)
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_WORDCHAR:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    if (!CHMAX_255(fc) || (mb->ctypes[fc] & ctype_word) == 0)
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_ANYNL:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    switch(fc)
      {
      default: RRETURN(MATCH_NOMATCH);

      case CHAR_CR:
      if (Feptr >= mb->end_subject)
        {
        SCHECK_PARTIAL();
        }
      else if (UCHAR21TEST(Feptr) == CHAR_LF) Feptr++;
      break;

      case CHAR_LF:
      break;

      case CHAR_VT:
      case CHAR_FF:
      case CHAR_NEL:
#ifndef EBCDIC
      case 0x2028:
      case 0x2029:
#endif  /* Not EBCDIC */
      if (mb->bsr_convention == PCRE2_BSR_ANYCRLF) RRETURN(MATCH_NOMATCH);
      break;
      }
    Fecode++;
    break;

    case OP_NOT_HSPACE:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    switch(fc)
      {
      HSPACE_CASES: RRETURN(MATCH_NOMATCH);  /* Byte and multibyte cases */
      default: break;
      }
    Fecode++;
    break;

    case OP_HSPACE:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    switch(fc)
      {
      HSPACE_CASES: break;  /* Byte and multibyte cases */
      default: RRETURN(MATCH_NOMATCH);
      }
    Fecode++;
    break;

    case OP_NOT_VSPACE:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    switch(fc)
      {
      VSPACE_CASES: RRETURN(MATCH_NOMATCH);
      default: break;
      }
    Fecode++;
    break;

    case OP_VSPACE:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
    switch(fc)
      {
      VSPACE_CASES: break;
      default: RRETURN(MATCH_NOMATCH);
      }
    Fecode++;
    break;


#ifdef SUPPORT_UNICODE

    case OP_PROP:
    case OP_NOTPROP:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(fc, Feptr);
      {
      const uint32_t *cp;
      const ucd_record *prop = GET_UCD(fc);

      switch(Fecode[1])
        {
        case PT_ANY:
        if (Fop == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
        break;

        case PT_LAMP:
        if ((prop->chartype == ucp_Lu ||
             prop->chartype == ucp_Ll ||
             prop->chartype == ucp_Lt) == (Fop == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_GC:
        if ((Fecode[2] != PRIV(ucp_gentype)[prop->chartype]) == (Fop == OP_PROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_PC:
        if ((Fecode[2] != prop->chartype) == (Fop == OP_PROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_SC:
        if ((Fecode[2] != prop->script) == (Fop == OP_PROP))
          RRETURN(MATCH_NOMATCH);
        break;

        /* These are specials */

        case PT_ALNUM:
        if ((PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
             PRIV(ucp_gentype)[prop->chartype] == ucp_N) == (Fop == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_SPACE:    /* Perl space */
        case PT_PXSPACE:  /* POSIX space */
        switch(fc)
          {
          HSPACE_CASES:
          VSPACE_CASES:
          if (Fop == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
          break;

          default:
          if ((PRIV(ucp_gentype)[prop->chartype] == ucp_Z) ==
            (Fop == OP_NOTPROP)) RRETURN(MATCH_NOMATCH);
          break;
          }
        break;

        case PT_WORD:
        if ((PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
             PRIV(ucp_gentype)[prop->chartype] == ucp_N ||
             fc == CHAR_UNDERSCORE) == (Fop == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_CLIST:
        cp = PRIV(ucd_caseless_sets) + Fecode[2];
        for (;;)
          {
          if (fc < *cp)
            { if (Fop == OP_PROP) { RRETURN(MATCH_NOMATCH); } else break; }
          if (fc == *cp++)
            { if (Fop == OP_PROP) break; else { RRETURN(MATCH_NOMATCH); } }
          }
        break;

        case PT_UCNC:
        if ((fc == CHAR_DOLLAR_SIGN || fc == CHAR_COMMERCIAL_AT ||
             fc == CHAR_GRAVE_ACCENT || (fc >= 0xa0 && fc <= 0xd7ff) ||
             fc >= 0xe000) == (Fop == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        /* This should never occur */

        default:
        return PCRE2_ERROR_INTERNAL;
        }

      Fecode += 3;
      }
    break;

    case OP_EXTUNI:
    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    else
      {
      GETCHARINCTEST(fc, Feptr);
      Feptr = PRIV(extuni)(fc, Feptr, mb->start_subject, mb->end_subject, utf,
        NULL);
      }
    CHECK_PARTIAL();
    Fecode++;
    break;

#endif  /* SUPPORT_UNICODE */

#define Lstart_eptr  F->temp_sptr[0]
#define Lmin         F->temp_32[0]
#define Lmax         F->temp_32[1]
#define Lctype       F->temp_32[2]
#define Lpropvalue   F->temp_32[3]

    case OP_TYPEEXACT:
    Lmin = Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATTYPE;

    case OP_TYPEUPTO:
    case OP_TYPEMINUPTO:
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    reptype = (*Fecode == OP_TYPEMINUPTO)? REPTYPE_MIN : REPTYPE_MAX;
    Fecode += 1 + IMM2_SIZE;
    goto REPEATTYPE;

    case OP_TYPEPOSSTAR:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = UINT32_MAX;
    Fecode++;
    goto REPEATTYPE;

    case OP_TYPEPOSPLUS:
    reptype = REPTYPE_POS;
    Lmin = 1;
    Lmax = UINT32_MAX;
    Fecode++;
    goto REPEATTYPE;

    case OP_TYPEPOSQUERY:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = 1;
    Fecode++;
    goto REPEATTYPE;

    case OP_TYPEPOSUPTO:
    reptype = REPTYPE_POS;
    Lmin = 0;
    Lmax = GET2(Fecode, 1);
    Fecode += 1 + IMM2_SIZE;
    goto REPEATTYPE;

    case OP_TYPESTAR:
    case OP_TYPEMINSTAR:
    case OP_TYPEPLUS:
    case OP_TYPEMINPLUS:
    case OP_TYPEQUERY:
    case OP_TYPEMINQUERY:
    fc = *Fecode++ - OP_TYPESTAR;
    Lmin = rep_min[fc];
    Lmax = rep_max[fc];
    reptype = rep_typ[fc];

    REPEATTYPE:
    Lctype = *Fecode++;      /* Code for the character type */

#ifdef SUPPORT_UNICODE
    if (Lctype == OP_PROP || Lctype == OP_NOTPROP)
      {
      proptype = *Fecode++;
      Lpropvalue = *Fecode++;
      }
    else proptype = -1;
#endif

    if (Lmin > 0)
      {
#ifdef SUPPORT_UNICODE
      if (proptype >= 0)  /* Property tests in all modes */
        {
        switch(proptype)
          {
          case PT_ANY:
          if (Lctype == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
          for (i = 1; i <= Lmin; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            }
          break;

          case PT_LAMP:
          for (i = 1; i <= Lmin; i++)
            {
            int chartype;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            chartype = UCD_CHARTYPE(fc);
            if ((chartype == ucp_Lu ||
                 chartype == ucp_Ll ||
                 chartype == ucp_Lt) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_GC:
          for (i = 1; i <= Lmin; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((UCD_CATEGORY(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_PC:
          for (i = 1; i <= Lmin; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((UCD_CHARTYPE(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_SC:
          for (i = 1; i <= Lmin; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((UCD_SCRIPT(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_ALNUM:
          for (i = 1; i <= Lmin; i++)
            {
            int category;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            category = UCD_CATEGORY(fc);
            if ((category == ucp_L || category == ucp_N) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          for (i = 1; i <= Lmin; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            switch(fc)
              {
              HSPACE_CASES:
              VSPACE_CASES:
              if (Lctype == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
              break;

              default:
              if ((UCD_CATEGORY(fc) == ucp_Z) == (Lctype == OP_NOTPROP))
                RRETURN(MATCH_NOMATCH);
              break;
              }
            }
          break;

          case PT_WORD:
          for (i = 1; i <= Lmin; i++)
            {
            int category;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            category = UCD_CATEGORY(fc);
            if ((category == ucp_L || category == ucp_N ||
                fc == CHAR_UNDERSCORE) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_CLIST:
          for (i = 1; i <= Lmin; i++)
            {
            const uint32_t *cp;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            cp = PRIV(ucd_caseless_sets) + Lpropvalue;
            for (;;)
              {
              if (fc < *cp)
                {
                if (Lctype == OP_NOTPROP) break;
                RRETURN(MATCH_NOMATCH);
                }
              if (fc == *cp++)
                {
                if (Lctype == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
                break;
                }
              }
            }
          break;

          case PT_UCNC:
          for (i = 1; i <= Lmin; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((fc == CHAR_DOLLAR_SIGN || fc == CHAR_COMMERCIAL_AT ||
                 fc == CHAR_GRAVE_ACCENT || (fc >= 0xa0 && fc <= 0xd7ff) ||
                 fc >= 0xe000) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          break;

          /* This should not occur */

          default:
          return PCRE2_ERROR_INTERNAL;
          }
        }

      else if (Lctype == OP_EXTUNI)
        {
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          else
            {
            GETCHARINCTEST(fc, Feptr);
            Feptr = PRIV(extuni)(fc, Feptr, mb->start_subject,
              mb->end_subject, utf, NULL);
            }
          CHECK_PARTIAL();
          }
        }
      else
#endif     /* SUPPORT_UNICODE */

/* Handle all other cases in UTF mode */

#ifdef SUPPORT_UNICODE
      if (utf) switch(Lctype)
        {
        case OP_ANY:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (IS_NEWLINE(Feptr)) RRETURN(MATCH_NOMATCH);
          if (mb->partial != 0 &&
              Feptr + 1 >= mb->end_subject &&
              NLBLOCK->nltype == NLTYPE_FIXED &&
              NLBLOCK->nllen == 2 &&
              UCHAR21(Feptr) == NLBLOCK->nl[0])
            {
            mb->hitend = TRUE;
            if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
            }
          Feptr++;
          ACROSSCHAR(Feptr < mb->end_subject, Feptr, Feptr++);
          }
        break;

        case OP_ALLANY:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          Feptr++;
          ACROSSCHAR(Feptr < mb->end_subject, Feptr, Feptr++);
          }
        break;

        case OP_ANYBYTE:
        if (Feptr > mb->end_subject - Lmin) RRETURN(MATCH_NOMATCH);
        Feptr += Lmin;
        break;

        case OP_ANYNL:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(fc, Feptr);
          switch(fc)
            {
            default: RRETURN(MATCH_NOMATCH);

            case CHAR_CR:
            if (Feptr < mb->end_subject && UCHAR21(Feptr) == CHAR_LF) Feptr++;
            break;

            case CHAR_LF:
            break;

            case CHAR_VT:
            case CHAR_FF:
            case CHAR_NEL:
#ifndef EBCDIC
            case 0x2028:
            case 0x2029:
#endif  /* Not EBCDIC */
            if (mb->bsr_convention == PCRE2_BSR_ANYCRLF) RRETURN(MATCH_NOMATCH);
            break;
            }
          }
        break;

        case OP_NOT_HSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(fc, Feptr);
          switch(fc)
            {
            HSPACE_CASES: RRETURN(MATCH_NOMATCH);
            default: break;
            }
          }
        break;

        case OP_HSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(fc, Feptr);
          switch(fc)
            {
            HSPACE_CASES: break;
            default: RRETURN(MATCH_NOMATCH);
            }
          }
        break;

        case OP_NOT_VSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(fc, Feptr);
          switch(fc)
            {
            VSPACE_CASES: RRETURN(MATCH_NOMATCH);
            default: break;
            }
          }
        break;

        case OP_VSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(fc, Feptr);
          switch(fc)
            {
            VSPACE_CASES: break;
            default: RRETURN(MATCH_NOMATCH);
            }
          }
        break;

        case OP_NOT_DIGIT:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(fc, Feptr);
          if (fc < 128 && (mb->ctypes[fc] & ctype_digit) != 0)
            RRETURN(MATCH_NOMATCH);
          }
        break;

        case OP_DIGIT:
        for (i = 1; i <= Lmin; i++)
          {
          uint32_t cc;
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(Feptr);
          if (cc >= 128 || (mb->ctypes[cc] & ctype_digit) == 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          /* No need to skip more code units - we know it has only one. */
          }
        break;

        case OP_NOT_WHITESPACE:
        for (i = 1; i <= Lmin; i++)
          {
          uint32_t cc;
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(Feptr);
          if (cc < 128 && (mb->ctypes[cc] & ctype_space) != 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          ACROSSCHAR(Feptr < mb->end_subject, Feptr, Feptr++);
          }
        break;

        case OP_WHITESPACE:
        for (i = 1; i <= Lmin; i++)
          {
          uint32_t cc;
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(Feptr);
          if (cc >= 128 || (mb->ctypes[cc] & ctype_space) == 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          /* No need to skip more code units - we know it has only one. */
          }
        break;

        case OP_NOT_WORDCHAR:
        for (i = 1; i <= Lmin; i++)
          {
          uint32_t cc;
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(Feptr);
          if (cc < 128 && (mb->ctypes[cc] & ctype_word) != 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          ACROSSCHAR(Feptr < mb->end_subject, Feptr, Feptr++);
          }
        break;

        case OP_WORDCHAR:
        for (i = 1; i <= Lmin; i++)
          {
          uint32_t cc;
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(Feptr);
          if (cc >= 128 || (mb->ctypes[cc] & ctype_word) == 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          /* No need to skip more code units - we know it has only one. */
          }
        break;

        default:
        return PCRE2_ERROR_INTERNAL;
        }  /* End switch(Lctype) */

      else
#endif     /* SUPPORT_UNICODE */

      switch(Lctype)
        {
        case OP_ANY:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (IS_NEWLINE(Feptr)) RRETURN(MATCH_NOMATCH);
          if (mb->partial != 0 &&
              Feptr + 1 >= mb->end_subject &&
              NLBLOCK->nltype == NLTYPE_FIXED &&
              NLBLOCK->nllen == 2 &&
              *Feptr == NLBLOCK->nl[0])
            {
            mb->hitend = TRUE;
            if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
            }
          Feptr++;
          }
        break;

        case OP_ALLANY:
        if (Feptr > mb->end_subject - Lmin)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        Feptr += Lmin;
        break;

        case OP_ANYNL:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*Feptr++)
            {
            default: RRETURN(MATCH_NOMATCH);

            case CHAR_CR:
            if (Feptr < mb->end_subject && *Feptr == CHAR_LF) Feptr++;
            break;

            case CHAR_LF:
            break;

            case CHAR_VT:
            case CHAR_FF:
            case CHAR_NEL:
#if PCRE2_CODE_UNIT_WIDTH != 8
            case 0x2028:
            case 0x2029:
#endif
            if (mb->bsr_convention == PCRE2_BSR_ANYCRLF) RRETURN(MATCH_NOMATCH);
            break;
            }
          }
        break;

        case OP_NOT_HSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*Feptr++)
            {
            default: break;
            HSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
            HSPACE_MULTIBYTE_CASES:
#endif
            RRETURN(MATCH_NOMATCH);
            }
          }
        break;

        case OP_HSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*Feptr++)
            {
            default: RRETURN(MATCH_NOMATCH);
            HSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
            HSPACE_MULTIBYTE_CASES:
#endif
            break;
            }
          }
        break;

        case OP_NOT_VSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*Feptr++)
            {
            VSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
            VSPACE_MULTIBYTE_CASES:
#endif
            RRETURN(MATCH_NOMATCH);
            default: break;
            }
          }
        break;

        case OP_VSPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*Feptr++)
            {
            default: RRETURN(MATCH_NOMATCH);
            VSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
            VSPACE_MULTIBYTE_CASES:
#endif
            break;
            }
          }
        break;

        case OP_NOT_DIGIT:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (MAX_255(*Feptr) && (mb->ctypes[*Feptr] & ctype_digit) != 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        break;

        case OP_DIGIT:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (!MAX_255(*Feptr) || (mb->ctypes[*Feptr] & ctype_digit) == 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        break;

        case OP_NOT_WHITESPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (MAX_255(*Feptr) && (mb->ctypes[*Feptr] & ctype_space) != 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        break;

        case OP_WHITESPACE:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (!MAX_255(*Feptr) || (mb->ctypes[*Feptr] & ctype_space) == 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        break;

        case OP_NOT_WORDCHAR:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (MAX_255(*Feptr) && (mb->ctypes[*Feptr] & ctype_word) != 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        break;

        case OP_WORDCHAR:
        for (i = 1; i <= Lmin; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (!MAX_255(*Feptr) || (mb->ctypes[*Feptr] & ctype_word) == 0)
            RRETURN(MATCH_NOMATCH);
          Feptr++;
          }
        break;

        default:
        return PCRE2_ERROR_INTERNAL;
        }
      }

    /* If Lmin = Lmax we are done. Continue with the main loop. */

    if (Lmin == Lmax) continue;

    if (reptype == REPTYPE_MIN)
      {
#ifdef SUPPORT_UNICODE
      if (proptype >= 0)
        {
        switch(proptype)
          {
          case PT_ANY:
          for (;;)
            {
            RMATCH(Fecode, RM208);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if (Lctype == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_LAMP:
          for (;;)
            {
            int chartype;
            RMATCH(Fecode, RM209);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            chartype = UCD_CHARTYPE(fc);
            if ((chartype == ucp_Lu ||
                 chartype == ucp_Ll ||
                 chartype == ucp_Lt) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_GC:
          for (;;)
            {
            RMATCH(Fecode, RM210);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((UCD_CATEGORY(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_PC:
          for (;;)
            {
            RMATCH(Fecode, RM211);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((UCD_CHARTYPE(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_SC:
          for (;;)
            {
            RMATCH(Fecode, RM212);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((UCD_SCRIPT(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_ALNUM:
          for (;;)
            {
            int category;
            RMATCH(Fecode, RM213);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            category = UCD_CATEGORY(fc);
            if ((category == ucp_L || category == ucp_N) ==
                (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          for (;;)
            {
            RMATCH(Fecode, RM214);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            switch(fc)
              {
              HSPACE_CASES:
              VSPACE_CASES:
              if (Lctype == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
              break;

              default:
              if ((UCD_CATEGORY(fc) == ucp_Z) == (Lctype == OP_NOTPROP))
                RRETURN(MATCH_NOMATCH);
              break;
              }
            }
          /* Control never gets here */

          case PT_WORD:
          for (;;)
            {
            int category;
            RMATCH(Fecode, RM215);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            category = UCD_CATEGORY(fc);
            if ((category == ucp_L ||
                 category == ucp_N ||
                 fc == CHAR_UNDERSCORE) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_CLIST:
          for (;;)
            {
            const uint32_t *cp;
            RMATCH(Fecode, RM216);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            cp = PRIV(ucd_caseless_sets) + Lpropvalue;
            for (;;)
              {
              if (fc < *cp)
                {
                if (Lctype == OP_NOTPROP) break;
                RRETURN(MATCH_NOMATCH);
                }
              if (fc == *cp++)
                {
                if (Lctype == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
                break;
                }
              }
            }
          /* Control never gets here */

          case PT_UCNC:
          for (;;)
            {
            RMATCH(Fecode, RM217);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(fc, Feptr);
            if ((fc == CHAR_DOLLAR_SIGN || fc == CHAR_COMMERCIAL_AT ||
                 fc == CHAR_GRAVE_ACCENT || (fc >= 0xa0 && fc <= 0xd7ff) ||
                 fc >= 0xe000) == (Lctype == OP_NOTPROP))
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          /* This should never occur */
          default:
          return PCRE2_ERROR_INTERNAL;
          }
        }

      else if (Lctype == OP_EXTUNI)
        {
        for (;;)
          {
          RMATCH(Fecode, RM218);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          else
            {
            GETCHARINCTEST(fc, Feptr);
            Feptr = PRIV(extuni)(fc, Feptr, mb->start_subject, mb->end_subject,
              utf, NULL);
            }
          CHECK_PARTIAL();
          }
        }
      else
#endif     /* SUPPORT_UNICODE */

      /* UTF mode for non-property testing character types. */

#ifdef SUPPORT_UNICODE
      if (utf)
        {
        for (;;)
          {
          RMATCH(Fecode, RM219);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (Lctype == OP_ANY && IS_NEWLINE(Feptr)) RRETURN(MATCH_NOMATCH);
          GETCHARINC(fc, Feptr);
          switch(Lctype)
            {
            case OP_ANY:               /* This is the non-NL case */
            if (mb->partial != 0 &&    /* Take care with CRLF partial */
                Feptr >= mb->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                fc == NLBLOCK->nl[0])
              {
              mb->hitend = TRUE;
              if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
              }
            break;

            case OP_ALLANY:
            case OP_ANYBYTE:
            break;

            case OP_ANYNL:
            switch(fc)
              {
              default: RRETURN(MATCH_NOMATCH);

              case CHAR_CR:
              if (Feptr < mb->end_subject && UCHAR21(Feptr) == CHAR_LF) Feptr++;
              break;

              case CHAR_LF:
              break;

              case CHAR_VT:
              case CHAR_FF:
              case CHAR_NEL:
#ifndef EBCDIC
              case 0x2028:
              case 0x2029:
#endif  /* Not EBCDIC */
              if (mb->bsr_convention == PCRE2_BSR_ANYCRLF)
                RRETURN(MATCH_NOMATCH);
              break;
              }
            break;

            case OP_NOT_HSPACE:
            switch(fc)
              {
              HSPACE_CASES: RRETURN(MATCH_NOMATCH);
              default: break;
              }
            break;

            case OP_HSPACE:
            switch(fc)
              {
              HSPACE_CASES: break;
              default: RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_NOT_VSPACE:
            switch(fc)
              {
              VSPACE_CASES: RRETURN(MATCH_NOMATCH);
              default: break;
              }
            break;

            case OP_VSPACE:
            switch(fc)
              {
              VSPACE_CASES: break;
              default: RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_NOT_DIGIT:
            if (fc < 256 && (mb->ctypes[fc] & ctype_digit) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_DIGIT:
            if (fc >= 256 || (mb->ctypes[fc] & ctype_digit) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WHITESPACE:
            if (fc < 256 && (mb->ctypes[fc] & ctype_space) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_WHITESPACE:
            if (fc >= 256 || (mb->ctypes[fc] & ctype_space) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WORDCHAR:
            if (fc < 256 && (mb->ctypes[fc] & ctype_word) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_WORDCHAR:
            if (fc >= 256 || (mb->ctypes[fc] & ctype_word) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            default:
            return PCRE2_ERROR_INTERNAL;
            }
          }
        }
      else
#endif  /* SUPPORT_UNICODE */

      /* Not UTF mode */
        {
        for (;;)
          {
          RMATCH(Fecode, RM33);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (Lctype == OP_ANY && IS_NEWLINE(Feptr))
            RRETURN(MATCH_NOMATCH);
          fc = *Feptr++;
          switch(Lctype)
            {
            case OP_ANY:               /* This is the non-NL case */
            if (mb->partial != 0 &&    /* Take care with CRLF partial */
                Feptr >= mb->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                fc == NLBLOCK->nl[0])
              {
              mb->hitend = TRUE;
              if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
              }
            break;

            case OP_ALLANY:
            case OP_ANYBYTE:
            break;

            case OP_ANYNL:
            switch(fc)
              {
              default: RRETURN(MATCH_NOMATCH);

              case CHAR_CR:
              if (Feptr < mb->end_subject && *Feptr == CHAR_LF) Feptr++;
              break;

              case CHAR_LF:
              break;

              case CHAR_VT:
              case CHAR_FF:
              case CHAR_NEL:
#if PCRE2_CODE_UNIT_WIDTH != 8
              case 0x2028:
              case 0x2029:
#endif
              if (mb->bsr_convention == PCRE2_BSR_ANYCRLF)
                RRETURN(MATCH_NOMATCH);
              break;
              }
            break;

            case OP_NOT_HSPACE:
            switch(fc)
              {
              default: break;
              HSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              HSPACE_MULTIBYTE_CASES:
#endif
              RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_HSPACE:
            switch(fc)
              {
              default: RRETURN(MATCH_NOMATCH);
              HSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              HSPACE_MULTIBYTE_CASES:
#endif
              break;
              }
            break;

            case OP_NOT_VSPACE:
            switch(fc)
              {
              default: break;
              VSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              VSPACE_MULTIBYTE_CASES:
#endif
              RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_VSPACE:
            switch(fc)
              {
              default: RRETURN(MATCH_NOMATCH);
              VSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              VSPACE_MULTIBYTE_CASES:
#endif
              break;
              }
            break;

            case OP_NOT_DIGIT:
            if (MAX_255(fc) && (mb->ctypes[fc] & ctype_digit) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_DIGIT:
            if (!MAX_255(fc) || (mb->ctypes[fc] & ctype_digit) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WHITESPACE:
            if (MAX_255(fc) && (mb->ctypes[fc] & ctype_space) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_WHITESPACE:
            if (!MAX_255(fc) || (mb->ctypes[fc] & ctype_space) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WORDCHAR:
            if (MAX_255(fc) && (mb->ctypes[fc] & ctype_word) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_WORDCHAR:
            if (!MAX_255(fc) || (mb->ctypes[fc] & ctype_word) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            default:
            return PCRE2_ERROR_INTERNAL;
            }
          }
        }
      /* Control never gets here */
      }

    else
      {
      Lstart_eptr = Feptr;  /* Remember where we started */

#ifdef SUPPORT_UNICODE
      if (proptype >= 0)
        {
        switch(proptype)
          {
          case PT_ANY:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            if (Lctype == OP_NOTPROP) break;
            Feptr+= len;
            }
          break;

          case PT_LAMP:
          for (i = Lmin; i < Lmax; i++)
            {
            int chartype;
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            chartype = UCD_CHARTYPE(fc);
            if ((chartype == ucp_Lu ||
                 chartype == ucp_Ll ||
                 chartype == ucp_Lt) == (Lctype == OP_NOTPROP))
              break;
            Feptr+= len;
            }
          break;

          case PT_GC:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            if ((UCD_CATEGORY(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              break;
            Feptr+= len;
            }
          break;

          case PT_PC:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            if ((UCD_CHARTYPE(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              break;
            Feptr+= len;
            }
          break;

          case PT_SC:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            if ((UCD_SCRIPT(fc) == Lpropvalue) == (Lctype == OP_NOTPROP))
              break;
            Feptr+= len;
            }
          break;

          case PT_ALNUM:
          for (i = Lmin; i < Lmax; i++)
            {
            int category;
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            category = UCD_CATEGORY(fc);
            if ((category == ucp_L || category == ucp_N) ==
                (Lctype == OP_NOTPROP))
              break;
            Feptr+= len;
            }
          break;

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            switch(fc)
              {
              HSPACE_CASES:
              VSPACE_CASES:
              if (Lctype == OP_NOTPROP) goto ENDLOOP99;  /* Break the loop */
              break;

              default:
              if ((UCD_CATEGORY(fc) == ucp_Z) == (Lctype == OP_NOTPROP))
                goto ENDLOOP99;   /* Break the loop */
              break;
              }
            Feptr+= len;
            }
          ENDLOOP99:
          break;

          case PT_WORD:
          for (i = Lmin; i < Lmax; i++)
            {
            int category;
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            category = UCD_CATEGORY(fc);
            if ((category == ucp_L || category == ucp_N ||
                 fc == CHAR_UNDERSCORE) == (Lctype == OP_NOTPROP))
              break;
            Feptr+= len;
            }
          break;

          case PT_CLIST:
          for (i = Lmin; i < Lmax; i++)
            {
            const uint32_t *cp;
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            cp = PRIV(ucd_caseless_sets) + Lpropvalue;
            for (;;)
              {
              if (fc < *cp)
                { if (Lctype == OP_NOTPROP) break; else goto GOT_MAX; }
              if (fc == *cp++)
                { if (Lctype == OP_NOTPROP) goto GOT_MAX; else break; }
              }
            Feptr += len;
            }
          GOT_MAX:
          break;

          case PT_UCNC:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(fc, Feptr, len);
            if ((fc == CHAR_DOLLAR_SIGN || fc == CHAR_COMMERCIAL_AT ||
                 fc == CHAR_GRAVE_ACCENT || (fc >= 0xa0 && fc <= 0xd7ff) ||
                 fc >= 0xe000) == (Lctype == OP_NOTPROP))
              break;
            Feptr += len;
            }
          break;

          default:
          return PCRE2_ERROR_INTERNAL;
          }

        /* Feptr is now past the end of the maximum run */

        if (reptype == REPTYPE_POS) continue;    /* No backtracking */

        for(;;)
          {
          if (Feptr <= Lstart_eptr) break;
          RMATCH(Fecode, RM222);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          Feptr--;
          if (utf) BACKCHAR(Feptr);
          }
        }

      else if (Lctype == OP_EXTUNI)
        {
        for (i = Lmin; i < Lmax; i++)
          {
          if (Feptr >= mb->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }
          else
            {
            GETCHARINCTEST(fc, Feptr);
            Feptr = PRIV(extuni)(fc, Feptr, mb->start_subject, mb->end_subject,
              utf, NULL);
            }
          CHECK_PARTIAL();
          }

        /* Feptr is now past the end of the maximum run */

        if (reptype == REPTYPE_POS) continue;    /* No backtracking */

        for(;;)
          {
          int lgb, rgb;
          PCRE2_SPTR fptr;

          if (Feptr <= Lstart_eptr) break;   /* At start of char run */
          RMATCH(Fecode, RM220);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);

          Feptr--;
          if (!utf) fc = *Feptr; else
            {
            BACKCHAR(Feptr);
            GETCHAR(fc, Feptr);
            }
          rgb = UCD_GRAPHBREAK(fc);

          for (;;)
            {
            if (Feptr <= Lstart_eptr) break;   /* At start of char run */
            fptr = Feptr - 1;
            if (!utf) fc = *fptr; else
              {
              BACKCHAR(fptr);
              GETCHAR(fc, fptr);
              }
            lgb = UCD_GRAPHBREAK(fc);
            if ((PRIV(ucp_gbtable)[lgb] & (1u << rgb)) == 0) break;
            Feptr = fptr;
            rgb = lgb;
            }
          }
        }

      else
#endif   /* SUPPORT_UNICODE */

#ifdef SUPPORT_UNICODE
      if (utf)
        {
        switch(Lctype)
          {
          case OP_ANY:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (IS_NEWLINE(Feptr)) break;
            if (mb->partial != 0 &&    /* Take care with CRLF partial */
                Feptr + 1 >= mb->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                UCHAR21(Feptr) == NLBLOCK->nl[0])
              {
              mb->hitend = TRUE;
              if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
              }
            Feptr++;
            ACROSSCHAR(Feptr < mb->end_subject, Feptr, Feptr++);
            }
          break;

          case OP_ALLANY:
          if (Lmax < UINT32_MAX)
            {
            for (i = Lmin; i < Lmax; i++)
              {
              if (Feptr >= mb->end_subject)
                {
                SCHECK_PARTIAL();
                break;
                }
              Feptr++;
              ACROSSCHAR(Feptr < mb->end_subject, Feptr, Feptr++);
              }
            }
          else
            {
            Feptr = mb->end_subject;   /* Unlimited UTF-8 repeat */
            SCHECK_PARTIAL();
            }
          break;

          /* The "byte" (i.e. "code unit") case is the same as non-UTF */

          case OP_ANYBYTE:
          fc = Lmax - Lmin;
          if (fc > (uint32_t)(mb->end_subject - Feptr))
            {
            Feptr = mb->end_subject;
            SCHECK_PARTIAL();
            }
          else Feptr += fc;
          break;

          case OP_ANYNL:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc == CHAR_CR)
              {
              if (++Feptr >= mb->end_subject) break;
              if (UCHAR21(Feptr) == CHAR_LF) Feptr++;
              }
            else
              {
              if (fc != CHAR_LF &&
                  (mb->bsr_convention == PCRE2_BSR_ANYCRLF ||
                   (fc != CHAR_VT && fc != CHAR_FF && fc != CHAR_NEL
#ifndef EBCDIC
                    && fc != 0x2028 && fc != 0x2029
#endif  /* Not EBCDIC */
                    )))
                break;
              Feptr += len;
              }
            }
          break;

          case OP_NOT_HSPACE:
          case OP_HSPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            BOOL gotspace;
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            switch(fc)
              {
              HSPACE_CASES: gotspace = TRUE; break;
              default: gotspace = FALSE; break;
              }
            if (gotspace == (Lctype == OP_NOT_HSPACE)) break;
            Feptr += len;
            }
          break;

          case OP_NOT_VSPACE:
          case OP_VSPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            BOOL gotspace;
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            switch(fc)
              {
              VSPACE_CASES: gotspace = TRUE; break;
              default: gotspace = FALSE; break;
              }
            if (gotspace == (Lctype == OP_NOT_VSPACE)) break;
            Feptr += len;
            }
          break;

          case OP_NOT_DIGIT:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc < 256 && (mb->ctypes[fc] & ctype_digit) != 0) break;
            Feptr+= len;
            }
          break;

          case OP_DIGIT:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc >= 256 ||(mb->ctypes[fc] & ctype_digit) == 0) break;
            Feptr+= len;
            }
          break;

          case OP_NOT_WHITESPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc < 256 && (mb->ctypes[fc] & ctype_space) != 0) break;
            Feptr+= len;
            }
          break;

          case OP_WHITESPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc >= 256 ||(mb->ctypes[fc] & ctype_space) == 0) break;
            Feptr+= len;
            }
          break;

          case OP_NOT_WORDCHAR:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc < 256 && (mb->ctypes[fc] & ctype_word) != 0) break;
            Feptr+= len;
            }
          break;

          case OP_WORDCHAR:
          for (i = Lmin; i < Lmax; i++)
            {
            int len = 1;
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(fc, Feptr, len);
            if (fc >= 256 || (mb->ctypes[fc] & ctype_word) == 0) break;
            Feptr+= len;
            }
          break;

          default:
          return PCRE2_ERROR_INTERNAL;
          }

        if (reptype == REPTYPE_POS) continue;    /* No backtracking */

        for(;;)
          {
          if (Feptr <= Lstart_eptr) break;
          RMATCH(Fecode, RM221);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          Feptr--;
          BACKCHAR(Feptr);
          if (Lctype == OP_ANYNL && Feptr > Lstart_eptr &&
              UCHAR21(Feptr) == CHAR_NL && UCHAR21(Feptr - 1) == CHAR_CR)
            Feptr--;
          }
        }
      else
#endif  /* SUPPORT_UNICODE */

      /* Not UTF mode */
        {
        switch(Lctype)
          {
          case OP_ANY:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (IS_NEWLINE(Feptr)) break;
            if (mb->partial != 0 &&    /* Take care with CRLF partial */
                Feptr + 1 >= mb->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                *Feptr == NLBLOCK->nl[0])
              {
              mb->hitend = TRUE;
              if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
              }
            Feptr++;
            }
          break;

          case OP_ALLANY:
          case OP_ANYBYTE:
          fc = Lmax - Lmin;
          if (fc > (uint32_t)(mb->end_subject - Feptr))
            {
            Feptr = mb->end_subject;
            SCHECK_PARTIAL();
            }
          else Feptr += fc;
          break;

          case OP_ANYNL:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            fc = *Feptr;
            if (fc == CHAR_CR)
              {
              if (++Feptr >= mb->end_subject) break;
              if (*Feptr == CHAR_LF) Feptr++;
              }
            else
              {
              if (fc != CHAR_LF && (mb->bsr_convention == PCRE2_BSR_ANYCRLF ||
                 (fc != CHAR_VT && fc != CHAR_FF && fc != CHAR_NEL
#if PCRE2_CODE_UNIT_WIDTH != 8
                 && fc != 0x2028 && fc != 0x2029
#endif
                 ))) break;
              Feptr++;
              }
            }
          break;

          case OP_NOT_HSPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*Feptr)
              {
              default: Feptr++; break;
              HSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              HSPACE_MULTIBYTE_CASES:
#endif
              goto ENDLOOP00;
              }
            }
          ENDLOOP00:
          break;

          case OP_HSPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*Feptr)
              {
              default: goto ENDLOOP01;
              HSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              HSPACE_MULTIBYTE_CASES:
#endif
              Feptr++; break;
              }
            }
          ENDLOOP01:
          break;

          case OP_NOT_VSPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*Feptr)
              {
              default: Feptr++; break;
              VSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              VSPACE_MULTIBYTE_CASES:
#endif
              goto ENDLOOP02;
              }
            }
          ENDLOOP02:
          break;

          case OP_VSPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*Feptr)
              {
              default: goto ENDLOOP03;
              VSPACE_BYTE_CASES:
#if PCRE2_CODE_UNIT_WIDTH != 8
              VSPACE_MULTIBYTE_CASES:
#endif
              Feptr++; break;
              }
            }
          ENDLOOP03:
          break;

          case OP_NOT_DIGIT:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (MAX_255(*Feptr) && (mb->ctypes[*Feptr] & ctype_digit) != 0)
              break;
            Feptr++;
            }
          break;

          case OP_DIGIT:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (!MAX_255(*Feptr) || (mb->ctypes[*Feptr] & ctype_digit) == 0)
              break;
            Feptr++;
            }
          break;

          case OP_NOT_WHITESPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (MAX_255(*Feptr) && (mb->ctypes[*Feptr] & ctype_space) != 0)
              break;
            Feptr++;
            }
          break;

          case OP_WHITESPACE:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (!MAX_255(*Feptr) || (mb->ctypes[*Feptr] & ctype_space) == 0)
              break;
            Feptr++;
            }
          break;

          case OP_NOT_WORDCHAR:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (MAX_255(*Feptr) && (mb->ctypes[*Feptr] & ctype_word) != 0)
              break;
            Feptr++;
            }
          break;

          case OP_WORDCHAR:
          for (i = Lmin; i < Lmax; i++)
            {
            if (Feptr >= mb->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (!MAX_255(*Feptr) || (mb->ctypes[*Feptr] & ctype_word) == 0)
              break;
            Feptr++;
            }
          break;

          default:
          return PCRE2_ERROR_INTERNAL;
          }

        if (reptype == REPTYPE_POS) continue;    /* No backtracking */

        for (;;)
          {
          if (Feptr == Lstart_eptr) break;
          RMATCH(Fecode, RM34);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          Feptr--;
          if (Lctype == OP_ANYNL && Feptr > Lstart_eptr && *Feptr == CHAR_LF &&
              Feptr[-1] == CHAR_CR) Feptr--;
          }
        }
      }
    break;  /* End of repeat character type processing */

#undef Lstart_eptr
#undef Lmin
#undef Lmax
#undef Lctype
#undef Lpropvalue

#define Lmin      F->temp_32[0]
#define Lmax      F->temp_32[1]
#define Lcaseless F->temp_32[2]
#define Lstart    F->temp_sptr[0]
#define Loffset   F->temp_size

    case OP_DNREF:
    case OP_DNREFI:
    Lcaseless = (Fop == OP_DNREFI);
      {
      int count = GET2(Fecode, 1+IMM2_SIZE);
      PCRE2_SPTR slot = mb->name_table + GET2(Fecode, 1) * mb->name_entry_size;
      Fecode += 1 + 2*IMM2_SIZE;

      while (count-- > 0)
        {
        Loffset = (GET2(slot, 0) << 1) - 2;
        if (Loffset < Foffset_top && Fovector[Loffset] != PCRE2_UNSET) break;
        slot += mb->name_entry_size;
        }
      }
    goto REF_REPEAT;

    case OP_REF:
    case OP_REFI:
    Lcaseless = (Fop == OP_REFI);
    Loffset = (GET2(Fecode, 1) << 1) - 2;
    Fecode += 1 + IMM2_SIZE;

    REF_REPEAT:
    switch (*Fecode)
      {
      case OP_CRSTAR:
      case OP_CRMINSTAR:
      case OP_CRPLUS:
      case OP_CRMINPLUS:
      case OP_CRQUERY:
      case OP_CRMINQUERY:
      fc = *Fecode++ - OP_CRSTAR;
      Lmin = rep_min[fc];
      Lmax = rep_max[fc];
      reptype = rep_typ[fc];
      break;

      case OP_CRRANGE:
      case OP_CRMINRANGE:
      Lmin = GET2(Fecode, 1);
      Lmax = GET2(Fecode, 1 + IMM2_SIZE);
      reptype = rep_typ[*Fecode - OP_CRSTAR];
      if (Lmax == 0) Lmax = UINT32_MAX;  /* Max 0 => infinity */
      Fecode += 1 + 2 * IMM2_SIZE;
      break;

      default:                  /* No repeat follows */
        {
        rrc = match_ref(Loffset, Lcaseless, F, mb, &length);
        if (rrc != 0)
          {
          if (rrc > 0) Feptr = mb->end_subject;   /* Partial match */
          CHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        }
      Feptr += length;
      continue;              /* With the main loop */
      }

    if (Loffset < Foffset_top && Fovector[Loffset] != PCRE2_UNSET)
      {
      if (Fovector[Loffset] == Fovector[Loffset + 1]) continue;
      }
    else  /* Group is not set */
      {
      if (Lmin == 0 || (mb->poptions & PCRE2_MATCH_UNSET_BACKREF) != 0)
        continue;
      }

    /* First, ensure the minimum number of matches are present. */

    for (i = 1; i <= Lmin; i++)
      {
      PCRE2_SIZE slength;
      rrc = match_ref(Loffset, Lcaseless, F, mb, &slength);
      if (rrc != 0)
        {
        if (rrc > 0) Feptr = mb->end_subject;   /* Partial match */
        CHECK_PARTIAL();
        RRETURN(MATCH_NOMATCH);
        }
      Feptr += slength;
      }

    if (Lmin == Lmax) continue;

    if (reptype == REPTYPE_MIN)
      {
      for (;;)
        {
        PCRE2_SIZE slength;
        RMATCH(Fecode, RM20);
        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        if (Lmin++ >= Lmax) RRETURN(MATCH_NOMATCH);
        rrc = match_ref(Loffset, Lcaseless, F, mb, &slength);
        if (rrc != 0)
          {
          if (rrc > 0) Feptr = mb->end_subject;   /* Partial match */
          CHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        Feptr += slength;
        }
      /* Control never gets here */
      }

    else
      {
      BOOL samelengths = TRUE;
      Lstart = Feptr;     /* Starting position */
      Flength = Fovector[Loffset+1] - Fovector[Loffset];

      for (i = Lmin; i < Lmax; i++)
        {
        PCRE2_SIZE slength;
        rrc = match_ref(Loffset, Lcaseless, F, mb, &slength);
        if (rrc != 0)
          {

          if (rrc > 0 && mb->partial != 0 &&
              mb->end_subject > mb->start_used_ptr)
            {
            mb->hitend = TRUE;
            if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
            }
          break;
          }

        if (slength != Flength) samelengths = FALSE;
        Feptr += slength;
        }

      if (samelengths)
        {
        while (Feptr >= Lstart)
          {
          RMATCH(Fecode, RM21);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          Feptr -= Flength;
          }
        }

      else
        {
        Lmax = i;
        for (;;)
          {
          RMATCH(Fecode, RM22);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (Feptr == Lstart) break; /* Failed after minimal repetition */
          Feptr = Lstart;
          Lmax--;
          for (i = Lmin; i < Lmax; i++)
            {
            PCRE2_SIZE slength;
            (void)match_ref(Loffset, Lcaseless, F, mb, &slength);
            Feptr += slength;
            }
          }
        }

      RRETURN(MATCH_NOMATCH);
      }
    /* Control never gets here */

#undef Lcaseless
#undef Lmin
#undef Lmax
#undef Lstart
#undef Loffset

#define Lnext_ecode F->temp_sptr[0]

    case OP_BRAZERO:
    Lnext_ecode = Fecode + 1;
    RMATCH(Lnext_ecode, RM9);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    do Lnext_ecode += GET(Lnext_ecode, 1); while (*Lnext_ecode == OP_ALT);
    Fecode = Lnext_ecode + 1 + LINK_SIZE;
    break;

    case OP_BRAMINZERO:
    Lnext_ecode = Fecode + 1;
    do Lnext_ecode += GET(Lnext_ecode, 1); while (*Lnext_ecode == OP_ALT);
    RMATCH(Lnext_ecode + 1 + LINK_SIZE, RM10);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    Fecode++;
    break;

#undef Lnext_ecode

    case OP_SKIPZERO:
    Fecode++;
    do Fecode += GET(Fecode,1); while (*Fecode == OP_ALT);
    Fecode += 1 + LINK_SIZE;
    break;

#define Lframe_type    F->temp_32[0]
#define Lmatched_once  F->temp_32[1]
#define Lzero_allowed  F->temp_32[2]
#define Lstart_eptr    F->temp_sptr[0]
#define Lstart_group   F->temp_sptr[1]

    case OP_BRAPOSZERO:
    Lzero_allowed = TRUE;                /* Zero repeat is allowed */
    Fecode += 1;
    if (*Fecode == OP_CBRAPOS || *Fecode == OP_SCBRAPOS)
      goto POSSESSIVE_CAPTURE;
    goto POSSESSIVE_NON_CAPTURE;

    case OP_BRAPOS:
    case OP_SBRAPOS:
    Lzero_allowed = FALSE;               /* Zero repeat not allowed */

    POSSESSIVE_NON_CAPTURE:
    Lframe_type = GF_NOCAPTURE;          /* Remembered frame type */
    goto POSSESSIVE_GROUP;

    case OP_CBRAPOS:
    case OP_SCBRAPOS:
    Lzero_allowed = FALSE;               /* Zero repeat not allowed */

    POSSESSIVE_CAPTURE:
    number = GET2(Fecode, 1+LINK_SIZE);
    Lframe_type = GF_CAPTURE | number;   /* Remembered frame type */

    POSSESSIVE_GROUP:
    Lmatched_once = FALSE;               /* Never matched */
    Lstart_group = Fecode;               /* Start of this group */

    for (;;)
      {
      Lstart_eptr = Feptr;               /* Position at group start */
      group_frame_type = Lframe_type;
      RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM8);
      if (rrc == MATCH_KETRPOS)
        {
        Lmatched_once = TRUE;            /* Matched at least once */
        if (Feptr == Lstart_eptr)        /* Empty match; skip to end */
          {
          do Fecode += GET(Fecode, 1); while (*Fecode == OP_ALT);
          break;
          }

        Fecode = Lstart_group;
        continue;
        }

      if (rrc == MATCH_THEN)
        {
        PCRE2_SPTR next_ecode = Fecode + GET(Fecode,1);
        if (mb->verb_ecode_ptr < next_ecode &&
            (*Fecode == OP_ALT || *next_ecode == OP_ALT))
          rrc = MATCH_NOMATCH;
        }

      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      Fecode += GET(Fecode, 1);
      if (*Fecode != OP_ALT) break;
      }

    if (Lmatched_once || Lzero_allowed)
      {
      Fecode += 1 + LINK_SIZE;
      break;
      }

    RRETURN(MATCH_NOMATCH);

#undef Lmatched_once
#undef Lzero_allowed
#undef Lframe_type
#undef Lstart_eptr
#undef Lstart_group

#define Lframe_type F->temp_32[0]     /* Set for all that use GROUPLOOP */
#define Lnext_branch F->temp_sptr[0]  /* Used only in OP_BRA handling */

    case OP_BRA:
    if (mb->hasthen || Frdepth == 0)
      {
      Lframe_type = 0;
      goto GROUPLOOP;
      }

    for (;;)
      {
      Lnext_branch = Fecode + GET(Fecode, 1);
      if (*Lnext_branch != OP_ALT) break;

      RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM1);
      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      Fecode = Lnext_branch;
      }

    Fecode += PRIV(OP_lengths)[*Fecode];
    break;

#undef Lnext_branch

    case OP_CBRA:
    case OP_SCBRA:
    Lframe_type = GF_CAPTURE | GET2(Fecode, 1+LINK_SIZE);
    goto GROUPLOOP;

    case OP_ONCE:
    case OP_SCRIPT_RUN:
    case OP_SBRA:
    Lframe_type = GF_NOCAPTURE | Fop;

    GROUPLOOP:
    for (;;)
      {
      group_frame_type = Lframe_type;
      RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM2);
      if (rrc == MATCH_THEN)
        {
        PCRE2_SPTR next_ecode = Fecode + GET(Fecode,1);
        if (mb->verb_ecode_ptr < next_ecode &&
            (*Fecode == OP_ALT || *next_ecode == OP_ALT))
          rrc = MATCH_NOMATCH;
        }
      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      Fecode += GET(Fecode, 1);
      if (*Fecode != OP_ALT) RRETURN(MATCH_NOMATCH);
      }
    /* Control never reaches here. */

#undef Lframe_type

#define Lframe_type F->temp_32[0]
#define Lstart_branch F->temp_sptr[0]

    case OP_RECURSE:
    bracode = mb->start_code + GET(Fecode, 1);
    number = (bracode == mb->start_code)? 0 : GET2(bracode, 1 + LINK_SIZE);

    if (Fcurrent_recurse != RECURSE_UNSET)
      {
      offset = Flast_group_offset;
      while (offset != PCRE2_UNSET)
        {
        N = (heapframe *)((char *)mb->match_frames + offset);
        P = (heapframe *)((char *)N - frame_size);
        if (N->group_frame_type == (GF_RECURSE | number))
          {
          if (Feptr == P->eptr) return PCRE2_ERROR_RECURSELOOP;
          break;
          }
        offset = P->last_group_offset;
        }
      }

    Lstart_branch = bracode;
    Lframe_type = GF_RECURSE | number;

    for (;;)
      {
      PCRE2_SPTR next_ecode;

      group_frame_type = Lframe_type;
      RMATCH(Lstart_branch + PRIV(OP_lengths)[*Lstart_branch], RM11);
      next_ecode = Lstart_branch + GET(Lstart_branch,1);

      if (rrc >= MATCH_BACKTRACK_MIN && rrc <= MATCH_BACKTRACK_MAX &&
          mb->verb_current_recurse == (Lframe_type ^ GF_RECURSE))
        {
        if (rrc == MATCH_THEN && mb->verb_ecode_ptr < next_ecode &&
            (*Lstart_branch == OP_ALT || *next_ecode == OP_ALT))
          rrc = MATCH_NOMATCH;
        else RRETURN(MATCH_NOMATCH);
        }

      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      Lstart_branch = next_ecode;
      if (*Lstart_branch != OP_ALT) RRETURN(MATCH_NOMATCH);
      }
    /* Control never reaches here. */

#undef Lframe_type
#undef Lstart_branch

#define Lframe_type  F->temp_32[0]

    case OP_ASSERT:
    case OP_ASSERTBACK:
    case OP_ASSERT_NA:
    case OP_ASSERTBACK_NA:
    Lframe_type = GF_NOCAPTURE | Fop;
    for (;;)
      {
      group_frame_type = Lframe_type;
      RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM3);
      if (rrc == MATCH_ACCEPT)
        {
        memcpy(Fovector,
              (char *)assert_accept_frame + offsetof(heapframe, ovector),
              assert_accept_frame->offset_top * sizeof(PCRE2_SIZE));
        Foffset_top = assert_accept_frame->offset_top;
        Fmark = assert_accept_frame->mark;
        break;
        }
      if (rrc != MATCH_NOMATCH && rrc != MATCH_THEN) RRETURN(rrc);
      Fecode += GET(Fecode, 1);
      if (*Fecode != OP_ALT) RRETURN(MATCH_NOMATCH);
      }

    do Fecode += GET(Fecode, 1); while (*Fecode == OP_ALT);
    Fecode += 1 + LINK_SIZE;
    break;

#undef Lframe_type

#define Lframe_type  F->temp_32[0]

    case OP_ASSERT_NOT:
    case OP_ASSERTBACK_NOT:
    Lframe_type  = GF_NOCAPTURE | Fop;

    for (;;)
      {
      group_frame_type = Lframe_type;
      RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM4);
      switch(rrc)
        {
        case MATCH_ACCEPT:   /* Assertion matched, therefore it fails. */
        case MATCH_MATCH:
        RRETURN (MATCH_NOMATCH);

        case MATCH_NOMATCH:  /* Branch failed, try next if present. */
        case MATCH_THEN:
        Fecode += GET(Fecode, 1);
        if (*Fecode != OP_ALT) goto ASSERT_NOT_FAILED;
        break;

        case MATCH_COMMIT:   /* Assertion forced to fail, therefore continue. */
        case MATCH_SKIP:
        case MATCH_PRUNE:
        do Fecode += GET(Fecode, 1); while (*Fecode == OP_ALT);
        goto ASSERT_NOT_FAILED;

        default:             /* Pass back any other return */
        RRETURN(rrc);
        }
      }

    ASSERT_NOT_FAILED:
    Fecode += 1 + LINK_SIZE;
    break;

#undef Lframe_type

    case OP_CALLOUT:
    case OP_CALLOUT_STR:
    rrc = do_callout(F, mb, &length);
    if (rrc > 0) RRETURN(MATCH_NOMATCH);
    if (rrc < 0) RRETURN(rrc);
    Fecode += length;
    break;

    case OP_COND:
    case OP_SCOND:

    Flength = GET(Fecode, 1);    /* Offset to the second branch */
    if (Fecode[Flength] != OP_ALT) Flength -= 1 + LINK_SIZE;
    Fecode += 1 + LINK_SIZE;     /* From this opcode */

    if (*Fecode == OP_CALLOUT || *Fecode == OP_CALLOUT_STR)
      {
      rrc = do_callout(F, mb, &length);
      if (rrc > 0) RRETURN(MATCH_NOMATCH);
      if (rrc < 0) RRETURN(rrc);

      Fecode += length;
      Flength -= length;
      }

    condition = FALSE;
    switch(*Fecode)
      {
      case OP_RREF:                  /* Group recursion test */
      if (Fcurrent_recurse != RECURSE_UNSET)
        {
        number = GET2(Fecode, 1);
        condition = (number == RREF_ANY || number == Fcurrent_recurse);
        }
      break;

      case OP_DNRREF:       /* Duplicate named group recursion test */
      if (Fcurrent_recurse != RECURSE_UNSET)
        {
        int count = GET2(Fecode, 1 + IMM2_SIZE);
        PCRE2_SPTR slot = mb->name_table + GET2(Fecode, 1) * mb->name_entry_size;
        while (count-- > 0)
          {
          number = GET2(slot, 0);
          condition = number == Fcurrent_recurse;
          if (condition) break;
          slot += mb->name_entry_size;
          }
        }
      break;

      case OP_CREF:                         /* Numbered group used test */
      offset = (GET2(Fecode, 1) << 1) - 2;  /* Doubled ref number */
      condition = offset < Foffset_top && Fovector[offset] != PCRE2_UNSET;
      break;

      case OP_DNCREF:      /* Duplicate named group used test */
        {
        int count = GET2(Fecode, 1 + IMM2_SIZE);
        PCRE2_SPTR slot = mb->name_table + GET2(Fecode, 1) * mb->name_entry_size;
        while (count-- > 0)
          {
          offset = (GET2(slot, 0) << 1) - 2;
          condition = offset < Foffset_top && Fovector[offset] != PCRE2_UNSET;
          if (condition) break;
          slot += mb->name_entry_size;
          }
        }
      break;

      case OP_FALSE:
      case OP_FAIL:   /* The assertion (?!) becomes OP_FAIL */
      break;

      case OP_TRUE:
      condition = TRUE;
      break;

#define Lpositive      F->temp_32[0]
#define Lstart_branch  F->temp_sptr[0]

      default:
      Lpositive = (*Fecode == OP_ASSERT || *Fecode == OP_ASSERTBACK);
      Lstart_branch = Fecode;

      for (;;)
        {
        group_frame_type = GF_CONDASSERT | *Fecode;
        RMATCH(Lstart_branch + PRIV(OP_lengths)[*Lstart_branch], RM5);

        switch(rrc)
          {
          case MATCH_ACCEPT:  /* Save captures */
          memcpy(Fovector,
                (char *)assert_accept_frame + offsetof(heapframe, ovector),
                assert_accept_frame->offset_top * sizeof(PCRE2_SIZE));
          Foffset_top = assert_accept_frame->offset_top;

          case MATCH_MATCH:
          condition = Lpositive;   /* TRUE for positive assertion */
          break;

          case MATCH_NOMATCH:
          case MATCH_THEN:
          Lstart_branch += GET(Lstart_branch, 1);
          if (*Lstart_branch == OP_ALT) continue;  /* Try next branch */
          condition = !Lpositive;  /* TRUE for negative assertion */
          break;

          case MATCH_COMMIT:
          case MATCH_SKIP:
          case MATCH_PRUNE:
          condition = !Lpositive;
          break;

          default:
          RRETURN(rrc);
          }
        break;  /* Out of the branch loop */
        }

      if (condition)
        {
        do Fecode += GET(Fecode, 1); while (*Fecode == OP_ALT);
        }
      break;  /* End of assertion condition */
      }

#undef Lpositive
#undef Lstart_branch

    Fecode += condition? PRIV(OP_lengths)[*Fecode] : Flength;

    if (Fop == OP_SCOND)
      {
      group_frame_type  = GF_NOCAPTURE | Fop;
      RMATCH(Fecode, RM35);
      RRETURN(rrc);
      }
    break;


    case OP_REVERSE:
    number = GET(Fecode, 1);
#ifdef SUPPORT_UNICODE
    if (utf)
      {
      while (number-- > 0)
        {
        if (Feptr <= mb->check_subject) RRETURN(MATCH_NOMATCH);
        Feptr--;
        BACKCHAR(Feptr);
        }
      }
    else
#endif

      {
      if ((ptrdiff_t)number > Feptr - mb->start_subject) RRETURN(MATCH_NOMATCH);
      Feptr -= number;
      }

    if (Feptr < mb->start_used_ptr) mb->start_used_ptr = Feptr;
    Fecode += 1 + LINK_SIZE;
    break;

    case OP_ALT:
    do Fecode += GET(Fecode,1); while (*Fecode == OP_ALT);
    break;

    case OP_KET:
    case OP_KETRMIN:
    case OP_KETRMAX:
    case OP_KETRPOS:

    bracode = Fecode - GET(Fecode, 1);

    if (*bracode != OP_BRA && *bracode != OP_COND)
      {
      N = (heapframe *)((char *)mb->match_frames + Flast_group_offset);
      P = (heapframe *)((char *)N - frame_size);
      Flast_group_offset = P->last_group_offset;

#ifdef DEBUG_SHOW_RMATCH
      fprintf(stderr, "++ KET for frame=%d type=%x prev char offset=%lu\n",
        N->rdepth, N->group_frame_type,
        (char *)P->eptr - (char *)mb->start_subject);
#endif

      if (GF_IDMASK(N->group_frame_type) == GF_CONDASSERT)
        {
        memcpy((char *)P + offsetof(heapframe, ovector), Fovector,
          Foffset_top * sizeof(PCRE2_SIZE));
        P->offset_top = Foffset_top;
        P->mark = Fmark;
        Fback_frame = (char *)F - (char *)P;
        RRETURN(MATCH_MATCH);
        }
      }
    else P = NULL;   /* Indicates starting frame not recorded */

    switch (*bracode)
      {
      case OP_BRA:    /* No need to do anything for these */
      case OP_COND:
      case OP_SCOND:
      break;

      case OP_ASSERT_NA:
      case OP_ASSERTBACK_NA:
      if (Feptr > mb->last_used_ptr) mb->last_used_ptr = Feptr;
      Feptr = P->eptr;
      break;

      case OP_ASSERT:
      case OP_ASSERTBACK:
      if (Feptr > mb->last_used_ptr) mb->last_used_ptr = Feptr;
      Feptr = P->eptr;

      case OP_ONCE:
      Fback_frame = ((char *)F - (char *)P);
      for (;;)
        {
        uint32_t y = GET(P->ecode,1);
        if ((P->ecode)[y] != OP_ALT) break;
        P->ecode += y;
        }
      break;

      case OP_ASSERT_NOT:
      case OP_ASSERTBACK_NOT:
      RRETURN(MATCH_MATCH);

      case OP_SCRIPT_RUN:
      if (!PRIV(script_run)(P->eptr, Feptr, utf)) RRETURN(MATCH_NOMATCH);
      break;

      case OP_CBRA:
      case OP_CBRAPOS:
      case OP_SCBRA:
      case OP_SCBRAPOS:
      number = GET2(bracode, 1+LINK_SIZE);

      if (Fcurrent_recurse == number)
        {
        P = (heapframe *)((char *)N - frame_size);
        memcpy((char *)F + offsetof(heapframe, ovector), P->ovector,
          P->offset_top * sizeof(PCRE2_SIZE));
        Foffset_top = P->offset_top;
        Fcapture_last = P->capture_last;
        Fcurrent_recurse = P->current_recurse;
        Fecode = P->ecode + 1 + LINK_SIZE;
        continue;  /* With next opcode */
        }

      offset = (number << 1) - 2;
      Fcapture_last = number;
      Fovector[offset] = P->eptr - mb->start_subject;
      Fovector[offset+1] = Feptr - mb->start_subject;
      if (offset >= Foffset_top) Foffset_top = offset + 2;
      break;
      }  /* End actions relating to the starting opcode */

    if (*Fecode == OP_KETRPOS)
      {
      memcpy((char *)P + offsetof(heapframe, eptr),
             (char *)F + offsetof(heapframe, eptr),
             frame_copy_size);
      RRETURN(MATCH_KETRPOS);
      }

    if (Fop != OP_KET && (P == NULL || Feptr != P->eptr))
      {
      if (Fop == OP_KETRMIN)
        {
        RMATCH(Fecode + 1 + LINK_SIZE, RM6);
        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        Fecode -= GET(Fecode, 1);
        break;   /* End of ket processing */
        }

      RMATCH(bracode, RM7);
      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      }

    Fecode += 1 + LINK_SIZE;
    break;

    case OP_CIRC:   /* Start of line, unless PCRE2_NOTBOL is set. */
    if (Feptr != mb->start_subject || (mb->moptions & PCRE2_NOTBOL) != 0)
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_SOD:    /* Unconditional start of subject */
    if (Feptr != mb->start_subject) RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_DOLL:
    if ((mb->moptions & PCRE2_NOTEOL) != 0) RRETURN(MATCH_NOMATCH);
    if ((mb->poptions & PCRE2_DOLLAR_ENDONLY) == 0) goto ASSERT_NL_OR_EOS;

    case OP_EOD:
    if (Feptr < mb->end_subject) RRETURN(MATCH_NOMATCH);
    if (mb->partial != 0)
      {
      mb->hitend = TRUE;
      if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
      }
    Fecode++;
    break;

    case OP_EODN:
    ASSERT_NL_OR_EOS:
    if (Feptr < mb->end_subject &&
        (!IS_NEWLINE(Feptr) || Feptr != mb->end_subject - mb->nllen))
      {
      if (mb->partial != 0 &&
          Feptr + 1 >= mb->end_subject &&
          NLBLOCK->nltype == NLTYPE_FIXED &&
          NLBLOCK->nllen == 2 &&
          UCHAR21TEST(Feptr) == NLBLOCK->nl[0])
        {
        mb->hitend = TRUE;
        if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
        }
      RRETURN(MATCH_NOMATCH);
      }

    if (mb->partial != 0)
      {
      mb->hitend = TRUE;
      if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
      }
    Fecode++;
    break;

    case OP_CIRCM:
    if ((mb->moptions & PCRE2_NOTBOL) != 0 && Feptr == mb->start_subject)
      RRETURN(MATCH_NOMATCH);
    if (Feptr != mb->start_subject &&
        ((Feptr == mb->end_subject &&
           (mb->poptions & PCRE2_ALT_CIRCUMFLEX) == 0) ||
         !WAS_NEWLINE(Feptr)))
      RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_DOLLM:
    if (Feptr < mb->end_subject)
      {
      if (!IS_NEWLINE(Feptr))
        {
        if (mb->partial != 0 &&
            Feptr + 1 >= mb->end_subject &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            UCHAR21TEST(Feptr) == NLBLOCK->nl[0])
          {
          mb->hitend = TRUE;
          if (mb->partial > 1) return PCRE2_ERROR_PARTIAL;
          }
        RRETURN(MATCH_NOMATCH);
        }
      }
    else
      {
      if ((mb->moptions & PCRE2_NOTEOL) != 0) RRETURN(MATCH_NOMATCH);
      SCHECK_PARTIAL();
      }
    Fecode++;
    break;

    case OP_SOM:
    if (Feptr != mb->start_subject + mb->start_offset) RRETURN(MATCH_NOMATCH);
    Fecode++;
    break;

    case OP_SET_SOM:
    Fstart_match = Feptr;
    Fecode++;
    break;

    case OP_NOT_WORD_BOUNDARY:
    case OP_WORD_BOUNDARY:
    if (Feptr == mb->check_subject) prev_is_word = FALSE; else
      {
      PCRE2_SPTR lastptr = Feptr - 1;
#ifdef SUPPORT_UNICODE
      if (utf)
        {
        BACKCHAR(lastptr);
        GETCHAR(fc, lastptr);
        }
      else
#endif  /* SUPPORT_UNICODE */
      fc = *lastptr;
      if (lastptr < mb->start_used_ptr) mb->start_used_ptr = lastptr;
#ifdef SUPPORT_UNICODE
      if ((mb->poptions & PCRE2_UCP) != 0)
        {
        if (fc == '_') prev_is_word = TRUE; else
          {
          int cat = UCD_CATEGORY(fc);
          prev_is_word = (cat == ucp_L || cat == ucp_N);
          }
        }
      else
#endif  /* SUPPORT_UNICODE */
      prev_is_word = CHMAX_255(fc) && (mb->ctypes[fc] & ctype_word) != 0;
      }

    if (Feptr >= mb->end_subject)
      {
      SCHECK_PARTIAL();
      cur_is_word = FALSE;
      }
    else
      {
      PCRE2_SPTR nextptr = Feptr + 1;
#ifdef SUPPORT_UNICODE
      if (utf)
        {
        FORWARDCHARTEST(nextptr, mb->end_subject);
        GETCHAR(fc, Feptr);
        }
      else
#endif  /* SUPPORT_UNICODE */
      fc = *Feptr;
      if (nextptr > mb->last_used_ptr) mb->last_used_ptr = nextptr;
#ifdef SUPPORT_UNICODE
      if ((mb->poptions & PCRE2_UCP) != 0)
        {
        if (fc == '_') cur_is_word = TRUE; else
          {
          int cat = UCD_CATEGORY(fc);
          cur_is_word = (cat == ucp_L || cat == ucp_N);
          }
        }
      else
#endif  /* SUPPORT_UNICODE */
      cur_is_word = CHMAX_255(fc) && (mb->ctypes[fc] & ctype_word) != 0;
      }

    if ((*Fecode++ == OP_WORD_BOUNDARY)?
         cur_is_word == prev_is_word : cur_is_word != prev_is_word)
      RRETURN(MATCH_NOMATCH);
    break;

    case OP_MARK:
    Fmark = mb->nomatch_mark = Fecode + 2;
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode] + Fecode[1], RM12);

    if (rrc == MATCH_SKIP_ARG &&
             PRIV(strcmp)(Fecode + 2, mb->verb_skip_ptr) == 0)
      {
      mb->verb_skip_ptr = Feptr;   /* Pass back current position */
      RRETURN(MATCH_SKIP);
      }
    RRETURN(rrc);

    case OP_FAIL:
    RRETURN(MATCH_NOMATCH);

    case OP_COMMIT:
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM13);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_COMMIT);

    case OP_COMMIT_ARG:
    Fmark = mb->nomatch_mark = Fecode + 2;
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode] + Fecode[1], RM36);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_COMMIT);

    case OP_PRUNE:
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM14);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_PRUNE);

    case OP_PRUNE_ARG:
    Fmark = mb->nomatch_mark = Fecode + 2;
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode] + Fecode[1], RM15);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_PRUNE);

    case OP_SKIP:
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM16);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    mb->verb_skip_ptr = Feptr;   /* Pass back current position */
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_SKIP);

    case OP_SKIP_ARG:
    mb->skip_arg_count++;
    if (mb->skip_arg_count <= mb->ignore_skip_arg)
      {
      Fecode += PRIV(OP_lengths)[*Fecode] + Fecode[1];
      break;
      }
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode] + Fecode[1], RM17);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);

    mb->verb_skip_ptr = Fecode + 2;
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_SKIP_ARG);

    case OP_THEN:
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode], RM18);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    mb->verb_ecode_ptr = Fecode;
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_THEN);

    case OP_THEN_ARG:
    Fmark = mb->nomatch_mark = Fecode + 2;
    RMATCH(Fecode + PRIV(OP_lengths)[*Fecode] + Fecode[1], RM19);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    mb->verb_ecode_ptr = Fecode;
    mb->verb_current_recurse = Fcurrent_recurse;
    RRETURN(MATCH_THEN);

    default:
    return PCRE2_ERROR_INTERNAL;
    }

  }  /* End of main loop */
/* Control never reaches here */

#define LBL(val) case val: goto L_RM##val;

RETURN_SWITCH:
if (Feptr > mb->last_used_ptr) mb->last_used_ptr = Feptr;
if (Frdepth == 0) return rrc;                     /* Exit from the top level */
F = (heapframe *)((char *)F - Fback_frame);       /* Backtrack */
mb->cb->callout_flags |= PCRE2_CALLOUT_BACKTRACK; /* Note for callouts */

#ifdef DEBUG_SHOW_RMATCH
fprintf(stderr, "++ RETURN %d to %d\n", rrc, Freturn_id);
#endif

switch (Freturn_id)
  {
  LBL( 1) LBL( 2) LBL( 3) LBL( 4) LBL( 5) LBL( 6) LBL( 7) LBL( 8)
  LBL( 9) LBL(10) LBL(11) LBL(12) LBL(13) LBL(14) LBL(15) LBL(16)
  LBL(17) LBL(18) LBL(19) LBL(20) LBL(21) LBL(22) LBL(23) LBL(24)
  LBL(25) LBL(26) LBL(27) LBL(28) LBL(29) LBL(30) LBL(31) LBL(32)
  LBL(33) LBL(34) LBL(35) LBL(36)

#ifdef SUPPORT_WIDE_CHARS
  LBL(100) LBL(101)
#endif

#ifdef SUPPORT_UNICODE
  LBL(200) LBL(201) LBL(202) LBL(203) LBL(204) LBL(205) LBL(206)
  LBL(207) LBL(208) LBL(209) LBL(210) LBL(211) LBL(212) LBL(213)
  LBL(214) LBL(215) LBL(216) LBL(217) LBL(218) LBL(219) LBL(220)
  LBL(221) LBL(222)
#endif

  default:
  return PCRE2_ERROR_INTERNAL;
  }
#undef LBL
}

/*************************************************
*           Match a Regular Expression           *
*************************************************/

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_match(const pcre2_code *code, PCRE2_SPTR subject, PCRE2_SIZE length,
  PCRE2_SIZE start_offset, uint32_t options, pcre2_match_data *match_data,
  pcre2_match_context *mcontext)
{
int rc;
int was_zero_terminated = 0;
const uint8_t *start_bits = NULL;
const pcre2_real_code *re = (const pcre2_real_code *)code;

BOOL anchored;
BOOL firstline;
BOOL has_first_cu = FALSE;
BOOL has_req_cu = FALSE;
BOOL startline;

#if PCRE2_CODE_UNIT_WIDTH == 8
BOOL memchr_not_found_first_cu = FALSE;
BOOL memchr_not_found_first_cu2 = FALSE;
#endif

PCRE2_UCHAR first_cu = 0;
PCRE2_UCHAR first_cu2 = 0;
PCRE2_UCHAR req_cu = 0;
PCRE2_UCHAR req_cu2 = 0;

PCRE2_SPTR bumpalong_limit;
PCRE2_SPTR end_subject;
PCRE2_SPTR true_end_subject;
PCRE2_SPTR start_match = subject + start_offset;
PCRE2_SPTR req_cu_ptr = start_match - 1;
PCRE2_SPTR start_partial;
PCRE2_SPTR match_partial;

#ifdef SUPPORT_JIT
BOOL use_jit;
#endif

BOOL utf = FALSE;

#ifdef SUPPORT_UNICODE
BOOL ucp = FALSE;
BOOL allow_invalid;
uint32_t fragment_options = 0;
#ifdef SUPPORT_JIT
BOOL jit_checked_utf = FALSE;
#endif
#endif  /* SUPPORT_UNICODE */

PCRE2_SIZE frame_size;

pcre2_callout_block cb;
match_block actual_match_block;
match_block *mb = &actual_match_block;

PCRE2_SPTR stack_frames_vector[START_FRAMES_SIZE/sizeof(PCRE2_SPTR)]
    PCRE2_KEEP_UNINITIALIZED;
mb->stack_frames = (heapframe *)stack_frames_vector;

if (length == PCRE2_ZERO_TERMINATED)
  {
  length = PRIV(strlen)(subject);
  was_zero_terminated = 1;
  }
true_end_subject = end_subject = subject + length;

if ((options & ~PUBLIC_MATCH_OPTIONS) != 0) return PCRE2_ERROR_BADOPTION;
if (code == NULL || subject == NULL || match_data == NULL)
  return PCRE2_ERROR_NULL;
if (start_offset > length) return PCRE2_ERROR_BADOFFSET;

if (re->magic_number != MAGIC_NUMBER) return PCRE2_ERROR_BADMAGIC;

if ((re->flags & PCRE2_MODE_MASK) != PCRE2_CODE_UNIT_WIDTH/8)
  return PCRE2_ERROR_BADMODE;

#define FF (PCRE2_NOTEMPTY_SET|PCRE2_NE_ATST_SET)
#define OO (PCRE2_NOTEMPTY|PCRE2_NOTEMPTY_ATSTART)
options |= (re->flags & FF) / ((FF & (~FF+1)) / (OO & (~OO+1)));
#undef FF
#undef OO

#ifdef SUPPORT_JIT
use_jit = (re->executable_jit != NULL &&
          (options & ~PUBLIC_JIT_MATCH_OPTIONS) == 0);
#endif

#ifdef SUPPORT_UNICODE
utf = (re->overall_options & PCRE2_UTF) != 0;
allow_invalid = (re->overall_options & PCRE2_MATCH_INVALID_UTF) != 0;
ucp = (re->overall_options & PCRE2_UCP) != 0;
#endif  /* SUPPORT_UNICODE */

mb->partial = ((options & PCRE2_PARTIAL_HARD) != 0)? 2 :
              ((options & PCRE2_PARTIAL_SOFT) != 0)? 1 : 0;

if (mb->partial != 0 &&
   ((re->overall_options | options) & PCRE2_ENDANCHORED) != 0)
  return PCRE2_ERROR_BADOPTION;

if (mcontext != NULL && mcontext->offset_limit != PCRE2_UNSET &&
     (re->overall_options & PCRE2_USE_OFFSET_LIMIT) == 0)
  return PCRE2_ERROR_BADOFFSETLIMIT;

if ((match_data->flags & PCRE2_MD_COPIED_SUBJECT) != 0)
  {
  match_data->memctl.free((void *)match_data->subject,
    match_data->memctl.memory_data);
  match_data->flags &= ~PCRE2_MD_COPIED_SUBJECT;
  }
match_data->subject = NULL;

match_data->startchar = 0;

/* ============================= JIT matching ============================== */

#ifdef SUPPORT_JIT
if (use_jit)
  {
#ifdef SUPPORT_UNICODE
  if (utf && (options & PCRE2_NO_UTF_CHECK) == 0 && !allow_invalid)
    {
#if PCRE2_CODE_UNIT_WIDTH != 32
    unsigned int i;
#endif

#if PCRE2_CODE_UNIT_WIDTH != 32
    if (start_match < end_subject && NOT_FIRSTCU(*start_match))
      {
      if (start_offset > 0) return PCRE2_ERROR_BADUTFOFFSET;
#if PCRE2_CODE_UNIT_WIDTH == 8
      return PCRE2_ERROR_UTF8_ERR20;  /* Isolated 0x80 byte */
#else
      return PCRE2_ERROR_UTF16_ERR3;  /* Isolated low surrogate */
#endif
      }
#endif  /* WIDTH != 32 */

#if PCRE2_CODE_UNIT_WIDTH != 32
    for (i = re->max_lookbehind; i > 0 && start_match > subject; i--)
      {
      start_match--;
      while (start_match > subject &&
#if PCRE2_CODE_UNIT_WIDTH == 8
      (*start_match & 0xc0) == 0x80)
#else  /* 16-bit */
      (*start_match & 0xfc00) == 0xdc00)
#endif
        start_match--;
      }
#else  /* PCRE2_CODE_UNIT_WIDTH != 32 */

    if (start_offset >= re->max_lookbehind)
      start_match -= re->max_lookbehind;
    else
      start_match = subject;
#endif  /* PCRE2_CODE_UNIT_WIDTH != 32 */

    match_data->rc = PRIV(valid_utf)(start_match,
      length - (start_match - subject), &(match_data->startchar));
    if (match_data->rc != 0)
      {
      match_data->startchar += start_match - subject;
      return match_data->rc;
      }
    jit_checked_utf = TRUE;
    }
#endif  /* SUPPORT_UNICODE */

  rc = pcre2_jit_match(code, subject, length, start_offset, options,
    match_data, mcontext);
  if (rc != PCRE2_ERROR_JIT_BADOPTION)
    {
    if (rc >= 0 && (options & PCRE2_COPY_MATCHED_SUBJECT) != 0)
      {
      length = CU2BYTES(length + was_zero_terminated);
      match_data->subject = match_data->memctl.malloc(length,
        match_data->memctl.memory_data);
      if (match_data->subject == NULL) return PCRE2_ERROR_NOMEMORY;
      memcpy((void *)match_data->subject, subject, length);
      match_data->flags |= PCRE2_MD_COPIED_SUBJECT;
      }
    return rc;
    }
  }
#endif  /* SUPPORT_JIT */

/* ========================= End of JIT matching ========================== */

mb->check_subject = subject;

#ifdef SUPPORT_UNICODE
if (utf &&
#ifdef SUPPORT_JIT
    !jit_checked_utf &&
#endif
    ((options & PCRE2_NO_UTF_CHECK) == 0 || allow_invalid))
  {
#if PCRE2_CODE_UNIT_WIDTH != 32
  BOOL skipped_bad_start = FALSE;
#endif

#if PCRE2_CODE_UNIT_WIDTH != 32
  if (allow_invalid)
    {
    while (start_match < end_subject && NOT_FIRSTCU(*start_match))
      {
      start_match++;
      skipped_bad_start = TRUE;
      }
    }
  else if (start_match < end_subject && NOT_FIRSTCU(*start_match))
    {
    if (start_offset > 0) return PCRE2_ERROR_BADUTFOFFSET;
#if PCRE2_CODE_UNIT_WIDTH == 8
    return PCRE2_ERROR_UTF8_ERR20;  /* Isolated 0x80 byte */
#else
    return PCRE2_ERROR_UTF16_ERR3;  /* Isolated low surrogate */
#endif
    }
#endif  /* WIDTH != 32 */

  mb->check_subject = start_match;

#if PCRE2_CODE_UNIT_WIDTH != 32
  if (!skipped_bad_start)
    {
    unsigned int i;
    for (i = re->max_lookbehind; i > 0 && mb->check_subject > subject; i--)
      {
      mb->check_subject--;
      while (mb->check_subject > subject &&
#if PCRE2_CODE_UNIT_WIDTH == 8
      (*mb->check_subject & 0xc0) == 0x80)
#else  /* 16-bit */
      (*mb->check_subject & 0xfc00) == 0xdc00)
#endif
        mb->check_subject--;
      }
    }
#else  /* PCRE2_CODE_UNIT_WIDTH != 32 */

  /* In the 32-bit library, one code unit equals one character. However,
  we cannot just subtract the lookbehind and then compare pointers, because
  a very large lookbehind could create an invalid pointer. */

  if (start_offset >= re->max_lookbehind)
    mb->check_subject -= re->max_lookbehind;
  else
    mb->check_subject = subject;
#endif  /* PCRE2_CODE_UNIT_WIDTH != 32 */

  for (;;)
    {
    match_data->rc = PRIV(valid_utf)(mb->check_subject,
      length - (mb->check_subject - subject), &(match_data->startchar));

    if (match_data->rc == 0) break;   /* Valid UTF string */

    match_data->startchar += mb->check_subject - subject;
    if (!allow_invalid || match_data->rc > 0) return match_data->rc;
    end_subject = subject + match_data->startchar;

    if (end_subject < start_match)
      {
      mb->check_subject = end_subject + 1;
#if PCRE2_CODE_UNIT_WIDTH != 32
      while (mb->check_subject < start_match && NOT_FIRSTCU(*mb->check_subject))
        mb->check_subject++;
#endif
      }

    else
      {
      fragment_options = PCRE2_NOTEOL;
      break;
      }
    }
  }
#endif  /* SUPPORT_UNICODE */

if (mcontext == NULL)
  {
  mcontext = (pcre2_match_context *)(&PRIV(default_match_context));
  mb->memctl = re->memctl;
  }
else mb->memctl = mcontext->memctl;

anchored = ((re->overall_options | options) & PCRE2_ANCHORED) != 0;
firstline = (re->overall_options & PCRE2_FIRSTLINE) != 0;
startline = (re->flags & PCRE2_STARTLINE) != 0;
bumpalong_limit = (mcontext->offset_limit == PCRE2_UNSET)?
  true_end_subject : subject + mcontext->offset_limit;

mb->cb = &cb;
cb.version = 2;
cb.subject = subject;
cb.subject_length = (PCRE2_SIZE)(end_subject - subject);
cb.callout_flags = 0;

mb->callout = mcontext->callout;
mb->callout_data = mcontext->callout_data;

mb->start_subject = subject;
mb->start_offset = start_offset;
mb->end_subject = end_subject;
mb->hasthen = (re->flags & PCRE2_HASTHEN) != 0;
mb->allowemptypartial = (re->max_lookbehind > 0) ||
    (re->flags & PCRE2_MATCH_EMPTY) != 0;
mb->poptions = re->overall_options;          /* Pattern options */
mb->ignore_skip_arg = 0;
mb->mark = mb->nomatch_mark = NULL;          /* In case never set */

mb->name_table = (PCRE2_UCHAR *)((uint8_t *)re + sizeof(pcre2_real_code));
mb->name_count = re->name_count;
mb->name_entry_size = re->name_entry_size;
mb->start_code = mb->name_table + re->name_count * re->name_entry_size;

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

frame_size = offsetof(heapframe, ovector) +
  re->top_bracket * 2 * sizeof(PCRE2_SIZE);

mb->heap_limit = (mcontext->heap_limit < re->limit_heap)?
  mcontext->heap_limit : re->limit_heap;

mb->match_limit = (mcontext->match_limit < re->limit_match)?
  mcontext->match_limit : re->limit_match;

mb->match_limit_depth = (mcontext->depth_limit < re->limit_depth)?
  mcontext->depth_limit : re->limit_depth;

if (frame_size <= START_FRAMES_SIZE/10)
  {
  mb->match_frames = mb->stack_frames;   /* Initial frame vector on the stack */
  mb->frame_vector_size = ((START_FRAMES_SIZE/frame_size) * frame_size);
  }
else
  {
  mb->frame_vector_size = frame_size * 10;
  if ((mb->frame_vector_size / 1024) > mb->heap_limit)
    {
    if (frame_size > mb->heap_limit * 1024) return PCRE2_ERROR_HEAPLIMIT;
    mb->frame_vector_size = ((mb->heap_limit * 1024)/frame_size) * frame_size;
    }
  mb->match_frames = mb->memctl.malloc(mb->frame_vector_size,
    mb->memctl.memory_data);
  if (mb->match_frames == NULL) return PCRE2_ERROR_NOMEMORY;
  }

mb->match_frames_top =
  (heapframe *)((char *)mb->match_frames + mb->frame_vector_size);

memset((char *)(mb->match_frames) + offsetof(heapframe, ovector), 0xff,
  re->top_bracket * 2 * sizeof(PCRE2_SIZE));

mb->lcc = re->tables + lcc_offset;
mb->fcc = re->tables + fcc_offset;
mb->ctypes = re->tables + ctypes_offset;

if ((re->flags & PCRE2_FIRSTSET) != 0)
  {
  has_first_cu = TRUE;
  first_cu = first_cu2 = (PCRE2_UCHAR)(re->first_codeunit);
  if ((re->flags & PCRE2_FIRSTCASELESS) != 0)
    {
    first_cu2 = TABLE_GET(first_cu, mb->fcc, first_cu);
#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
    if (first_cu > 127 && ucp && !utf) first_cu2 = UCD_OTHERCASE(first_cu);
#else
    if (first_cu > 127 && (utf || ucp)) first_cu2 = UCD_OTHERCASE(first_cu);
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
    req_cu2 = TABLE_GET(req_cu, mb->fcc, req_cu);
#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
    if (req_cu > 127 && ucp && !utf) req_cu2 = UCD_OTHERCASE(req_cu);
#else
    if (req_cu > 127 && (utf || ucp)) req_cu2 = UCD_OTHERCASE(req_cu);
#endif
#endif  /* SUPPORT_UNICODE */
    }
  }

#ifdef SUPPORT_UNICODE
FRAGMENT_RESTART:
#endif

start_partial = match_partial = NULL;
mb->hitend = FALSE;

for(;;)
  {
  PCRE2_SPTR new_start_match;

  if ((re->overall_options & PCRE2_NO_START_OPTIMIZE) == 0)
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
        if (!ok)
          {
          rc = MATCH_NOMATCH;
          break;
          }
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

        else
          {
#if PCRE2_CODE_UNIT_WIDTH != 8
          while (start_match < end_subject && UCHAR21TEST(start_match) !=
                 first_cu)
            start_match++;
#else
          start_match = memchr(start_match, first_cu, end_subject - start_match);
          if (start_match == NULL) start_match = end_subject;
#endif
          }

        if (mb->partial == 0 && start_match >= mb->end_subject)
          {
          rc = MATCH_NOMATCH;
          break;
          }
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

        if (mb->partial == 0 && start_match >= mb->end_subject)
          {
          rc = MATCH_NOMATCH;
          break;
          }
        }
      }   /* End first code unit handling */

    end_subject = mb->end_subject;

    if (mb->partial == 0)
      {
      PCRE2_SPTR p;

      if (end_subject - start_match < re->minlength)
        {
        rc = MATCH_NOMATCH;
        break;
        }

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

          if (p >= end_subject)
            {
            rc = MATCH_NOMATCH;
            break;
            }

          req_cu_ptr = p;
          }
        }
      }
    }

  /* ------------ End of start of match optimizations ------------ */

  if (start_match > bumpalong_limit)
    {
    rc = MATCH_NOMATCH;
    break;
    }

  cb.start_match = (PCRE2_SIZE)(start_match - subject);
  cb.callout_flags |= PCRE2_CALLOUT_STARTMATCH;

  mb->start_used_ptr = start_match;
  mb->last_used_ptr = start_match;
#ifdef SUPPORT_UNICODE
  mb->moptions = options | fragment_options;
#else
  mb->moptions = options;
#endif
  mb->match_call_count = 0;
  mb->end_offset_top = 0;
  mb->skip_arg_count = 0;

  rc = match(start_match, mb->start_code, match_data->ovector,
    match_data->oveccount, re->top_bracket, frame_size, mb);

  if (mb->hitend && start_partial == NULL)
    {
    start_partial = mb->start_used_ptr;
    match_partial = start_match;
    }

  switch(rc)
    {

    case MATCH_SKIP_ARG:
    new_start_match = start_match;
    mb->ignore_skip_arg = mb->skip_arg_count;
    break;

    case MATCH_SKIP:
    if (mb->verb_skip_ptr > start_match)
      {
      new_start_match = mb->verb_skip_ptr;
      break;
      }

    case MATCH_NOMATCH:
    case MATCH_PRUNE:
    case MATCH_THEN:
    mb->ignore_skip_arg = 0;
    new_start_match = start_match + 1;
#ifdef SUPPORT_UNICODE
    if (utf)
      ACROSSCHAR(new_start_match < end_subject, new_start_match,
        new_start_match++);
#endif
    break;

    case MATCH_COMMIT:
    rc = MATCH_NOMATCH;
    goto ENDLOOP;

    default:
    goto ENDLOOP;
    }

  rc = MATCH_NOMATCH;

  if (firstline && IS_NEWLINE(start_match)) break;

  start_match = new_start_match;

  if (anchored || start_match > end_subject) break;

  if (start_match > subject + start_offset &&
      start_match[-1] == CHAR_CR &&
      start_match < end_subject &&
      *start_match == CHAR_NL &&
      (re->flags & PCRE2_HASCRORLF) == 0 &&
        (mb->nltype == NLTYPE_ANY ||
         mb->nltype == NLTYPE_ANYCRLF ||
         mb->nllen == 2))
    start_match++;

  mb->mark = NULL;   /* Reset for start of next match attempt */
  }                  /* End of for(;;) "bumpalong" loop */

/* ==========================================================================*/

/* When we reach here, one of the following stopping conditions is true:

(1) The match succeeded, either completely, or partially;

(2) The pattern is anchored or the match was failed after (*COMMIT);

(3) We are past the end of the subject or the bumpalong limit;

(4) PCRE2_FIRSTLINE is set and we have failed to match at a newline, because
    this option requests that a match occur at or before the first newline in
    the subject.

(5) Some kind of error occurred.

*/

ENDLOOP:

#ifdef SUPPORT_UNICODE
if (utf && end_subject != true_end_subject &&
    (rc == MATCH_NOMATCH || rc == PCRE2_ERROR_PARTIAL))
  {
  for (;;)
    {

    start_match = end_subject + 1;
#if PCRE2_CODE_UNIT_WIDTH != 32
    while (start_match < true_end_subject && NOT_FIRSTCU(*start_match))
      start_match++;
#endif

    if (start_match >= true_end_subject)
      {
      rc = MATCH_NOMATCH;  /* In case it was partial */
      break;
      }

    mb->check_subject = start_match;
    rc = PRIV(valid_utf)(start_match, length - (start_match - subject),
      &(match_data->startchar));

    if (rc == 0)
      {
      mb->end_subject = end_subject = true_end_subject;
      fragment_options = PCRE2_NOTBOL;
      goto FRAGMENT_RESTART;
      }

    else if (rc < 0)
      {
      mb->end_subject = end_subject = start_match + match_data->startchar;
      if (end_subject > start_match)
        {
        fragment_options = PCRE2_NOTBOL|PCRE2_NOTEOL;
        goto FRAGMENT_RESTART;
        }
      }
    }
  }
#endif  /* SUPPORT_UNICODE */

if (mb->match_frames != mb->stack_frames)
  mb->memctl.free(mb->match_frames, mb->memctl.memory_data);

match_data->code = re;
match_data->mark = mb->mark;
match_data->matchedby = PCRE2_MATCHEDBY_INTERPRETER;

if (rc == MATCH_MATCH)
  {
  match_data->rc = ((int)mb->end_offset_top >= 2 * match_data->oveccount)?
    0 : (int)mb->end_offset_top/2 + 1;
  match_data->startchar = start_match - subject;
  match_data->leftchar = mb->start_used_ptr - subject;
  match_data->rightchar = ((mb->last_used_ptr > mb->end_match_ptr)?
    mb->last_used_ptr : mb->end_match_ptr) - subject;
  if ((options & PCRE2_COPY_MATCHED_SUBJECT) != 0)
    {
    length = CU2BYTES(length + was_zero_terminated);
    match_data->subject = match_data->memctl.malloc(length,
      match_data->memctl.memory_data);
    if (match_data->subject == NULL) return PCRE2_ERROR_NOMEMORY;
    memcpy((void *)match_data->subject, subject, length);
    match_data->flags |= PCRE2_MD_COPIED_SUBJECT;
    }
  else match_data->subject = subject;
  return match_data->rc;
  }

match_data->mark = mb->nomatch_mark;

if (rc != MATCH_NOMATCH && rc != PCRE2_ERROR_PARTIAL) match_data->rc = rc;

else if (match_partial != NULL)
  {
  match_data->subject = subject;
  match_data->ovector[0] = match_partial - subject;
  match_data->ovector[1] = end_subject - subject;
  match_data->startchar = match_partial - subject;
  match_data->leftchar = start_partial - subject;
  match_data->rightchar = end_subject - subject;
  match_data->rc = PCRE2_ERROR_PARTIAL;
  }

else match_data->rc = PCRE2_ERROR_NOMATCH;

return match_data->rc;
}

/* End of pcre2_match.c */
