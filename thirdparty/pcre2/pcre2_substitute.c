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

#include "pcre2_internal.h"

#define PTR_STACK_SIZE 20

#define SUBSTITUTE_OPTIONS \
  (PCRE2_SUBSTITUTE_EXTENDED|PCRE2_SUBSTITUTE_GLOBAL| \
   PCRE2_SUBSTITUTE_LITERAL|PCRE2_SUBSTITUTE_MATCHED| \
   PCRE2_SUBSTITUTE_OVERFLOW_LENGTH|PCRE2_SUBSTITUTE_REPLACEMENT_ONLY| \
   PCRE2_SUBSTITUTE_UNKNOWN_UNSET|PCRE2_SUBSTITUTE_UNSET_EMPTY)

/*************************************************
*           Find end of substitute text          *
*************************************************/

static int
find_text_end(const pcre2_code *code, PCRE2_SPTR *ptrptr, PCRE2_SPTR ptrend,
  BOOL last)
{
int rc = 0;
uint32_t nestlevel = 0;
BOOL literal = FALSE;
PCRE2_SPTR ptr = *ptrptr;

for (; ptr < ptrend; ptr++)
  {
  if (literal)
    {
    if (ptr[0] == CHAR_BACKSLASH && ptr < ptrend - 1 && ptr[1] == CHAR_E)
      {
      literal = FALSE;
      ptr += 1;
      }
    }

  else if (*ptr == CHAR_RIGHT_CURLY_BRACKET)
    {
    if (nestlevel == 0) goto EXIT;
    nestlevel--;
    }

  else if (*ptr == CHAR_COLON && !last && nestlevel == 0) goto EXIT;

  else if (*ptr == CHAR_DOLLAR_SIGN)
    {
    if (ptr < ptrend - 1 && ptr[1] == CHAR_LEFT_CURLY_BRACKET)
      {
      nestlevel++;
      ptr += 1;
      }
    }

  else if (*ptr == CHAR_BACKSLASH)
    {
    int erc;
    int errorcode;
    uint32_t ch;

    if (ptr < ptrend - 1) switch (ptr[1])
      {
      case CHAR_L:
      case CHAR_l:
      case CHAR_U:
      case CHAR_u:
      ptr += 1;
      continue;
      }

    ptr += 1;  /* Must point after \ */
    erc = PRIV(check_escape)(&ptr, ptrend, &ch, &errorcode,
      code->overall_options, code->extra_options, FALSE, NULL);
    ptr -= 1;  /* Back to last code unit of escape */
    if (errorcode != 0)
      {
      rc = errorcode;
      goto EXIT;
      }

    switch(erc)
      {
      case 0:      /* Data character */
      case ESC_E:  /* Isolated \E is ignored */
      break;

      case ESC_Q:
      literal = TRUE;
      break;

      default:
      rc = PCRE2_ERROR_BADREPESCAPE;
      goto EXIT;
      }
    }
  }

rc = PCRE2_ERROR_REPMISSINGBRACE;   /* Terminator not found */

EXIT:
*ptrptr = ptr;
return rc;
}

/*************************************************
*              Match and substitute              *
*************************************************/

#define CHECKMEMCPY(from,length) \
  { \
  if (!overflowed && lengthleft < length) \
    { \
    if ((suboptions & PCRE2_SUBSTITUTE_OVERFLOW_LENGTH) == 0) goto NOROOM; \
    overflowed = TRUE; \
    extra_needed = length - lengthleft; \
    } \
  else if (overflowed) \
    { \
    extra_needed += length; \
    }  \
  else \
    {  \
    memcpy(buffer + buff_offset, from, CU2BYTES(length)); \
    buff_offset += length; \
    lengthleft -= length; \
    } \
  }

PCRE2_EXP_DEFN int PCRE2_CALL_CONVENTION
pcre2_substitute(const pcre2_code *code, PCRE2_SPTR subject, PCRE2_SIZE length,
  PCRE2_SIZE start_offset, uint32_t options, pcre2_match_data *match_data,
  pcre2_match_context *mcontext, PCRE2_SPTR replacement, PCRE2_SIZE rlength,
  PCRE2_UCHAR *buffer, PCRE2_SIZE *blength)
{
int rc;
int subs;
int forcecase = 0;
int forcecasereset = 0;
uint32_t ovector_count;
uint32_t goptions = 0;
uint32_t suboptions;
pcre2_match_data *internal_match_data = NULL;
BOOL escaped_literal = FALSE;
BOOL overflowed = FALSE;
BOOL use_existing_match;
BOOL replacement_only;
#ifdef SUPPORT_UNICODE
BOOL utf = (code->overall_options & PCRE2_UTF) != 0;
BOOL ucp = (code->overall_options & PCRE2_UCP) != 0;
#endif
PCRE2_UCHAR temp[6];
PCRE2_SPTR ptr;
PCRE2_SPTR repend;
PCRE2_SIZE extra_needed = 0;
PCRE2_SIZE buff_offset, buff_length, lengthleft, fraglength;
PCRE2_SIZE *ovector;
PCRE2_SIZE ovecsave[3];
pcre2_substitute_callout_block scb;

/* General initialization */

buff_offset = 0;
lengthleft = buff_length = *blength;
*blength = PCRE2_UNSET;
ovecsave[0] = ovecsave[1] = ovecsave[2] = PCRE2_UNSET;

if ((options & (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT)) != 0)
  return PCRE2_ERROR_BADOPTION;

use_existing_match = ((options & PCRE2_SUBSTITUTE_MATCHED) != 0);
replacement_only = ((options & PCRE2_SUBSTITUTE_REPLACEMENT_ONLY) != 0);

if (match_data == NULL)
  {
  pcre2_general_context *gcontext;
  if (use_existing_match) return PCRE2_ERROR_NULL;
  gcontext = (mcontext == NULL)?
    (pcre2_general_context *)code :
    (pcre2_general_context *)mcontext;
  match_data = internal_match_data =
    pcre2_match_data_create_from_pattern(code, gcontext);
  if (internal_match_data == NULL) return PCRE2_ERROR_NOMEMORY;
  }

else if (use_existing_match)
  {
  pcre2_general_context *gcontext = (mcontext == NULL)?
    (pcre2_general_context *)code :
    (pcre2_general_context *)mcontext;
  int pairs = (code->top_bracket + 1 < match_data->oveccount)?
    code->top_bracket + 1 : match_data->oveccount;
  internal_match_data = pcre2_match_data_create(match_data->oveccount,
    gcontext);
  if (internal_match_data == NULL) return PCRE2_ERROR_NOMEMORY;
  memcpy(internal_match_data, match_data, offsetof(pcre2_match_data, ovector)
    + 2*pairs*sizeof(PCRE2_SIZE));
  match_data = internal_match_data;
  }

/* Remember ovector details */

ovector = pcre2_get_ovector_pointer(match_data);
ovector_count = pcre2_get_ovector_count(match_data);

/* Fixed things in the callout block */

scb.version = 0;
scb.input = subject;
scb.output = (PCRE2_SPTR)buffer;
scb.ovector = ovector;

if (length == PCRE2_ZERO_TERMINATED) length = PRIV(strlen)(subject);
if (rlength == PCRE2_ZERO_TERMINATED) rlength = PRIV(strlen)(replacement);
repend = replacement + rlength;

#ifdef SUPPORT_UNICODE
if (utf && (options & PCRE2_NO_UTF_CHECK) == 0)
  {
  rc = PRIV(valid_utf)(replacement, rlength, &(match_data->startchar));
  if (rc != 0)
    {
    match_data->leftchar = 0;
    goto EXIT;
    }
  }
#endif  /* SUPPORT_UNICODE */

suboptions = options & SUBSTITUTE_OPTIONS;
options &= ~SUBSTITUTE_OPTIONS;

if (start_offset > length)
  {
  match_data->leftchar = 0;
  rc = PCRE2_ERROR_BADOFFSET;
  goto EXIT;
  }

if (!replacement_only) CHECKMEMCPY(subject, start_offset);

subs = 0;
do
  {
  PCRE2_SPTR ptrstack[PTR_STACK_SIZE];
  uint32_t ptrstackptr = 0;

  if (use_existing_match)
    {
    rc = match_data->rc;
    use_existing_match = FALSE;
    }
  else rc = pcre2_match(code, subject, length, start_offset, options|goptions,
    match_data, mcontext);

#ifdef SUPPORT_UNICODE
  if (utf) options |= PCRE2_NO_UTF_CHECK;  /* Only need to check once */
#endif

  if (rc < 0)
    {
    PCRE2_SIZE save_start;

    if (rc != PCRE2_ERROR_NOMATCH) goto EXIT;
    if (goptions == 0 || start_offset >= length) break;

    save_start = start_offset++;
    if (subject[start_offset-1] == CHAR_CR &&
        code->newline_convention != PCRE2_NEWLINE_CR &&
        code->newline_convention != PCRE2_NEWLINE_LF &&
        start_offset < length &&
        subject[start_offset] == CHAR_LF)
      start_offset++;

    else if ((code->overall_options & PCRE2_UTF) != 0)
      {
#if PCRE2_CODE_UNIT_WIDTH == 8
      while (start_offset < length && (subject[start_offset] & 0xc0) == 0x80)
        start_offset++;
#elif PCRE2_CODE_UNIT_WIDTH == 16
      while (start_offset < length &&
            (subject[start_offset] & 0xfc00) == 0xdc00)
        start_offset++;
#endif
      }

    fraglength = start_offset - save_start;
    if (!replacement_only) CHECKMEMCPY(subject + save_start, fraglength);
    goptions = 0;
    continue;
    }

  if (ovector[1] < ovector[0] || ovector[0] < start_offset)
    {
    rc = PCRE2_ERROR_BADSUBSPATTERN;
    goto EXIT;
    }

  if (ovecsave[0] == ovector[0] && ovecsave[1] == ovector[1])
    {
    if (ovector[0] == ovector[1] && ovecsave[2] != start_offset)
      {
      goptions = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
      ovecsave[2] = start_offset;
      continue;    /* Back to the top of the loop */
      }
    rc = PCRE2_ERROR_INTERNAL_DUPMATCH;
    goto EXIT;
    }

  if (subs == INT_MAX)
    {
    rc = PCRE2_ERROR_TOOMANYREPLACE;
    goto EXIT;
    }
  subs++;

  if (rc == 0) rc = ovector_count;
  fraglength = ovector[0] - start_offset;
  if (!replacement_only) CHECKMEMCPY(subject + start_offset, fraglength);
  scb.output_offsets[0] = buff_offset;
  scb.oveccount = rc;

  ptr = replacement;
  if ((suboptions & PCRE2_SUBSTITUTE_LITERAL) != 0)
    {
    CHECKMEMCPY(ptr, rlength);
    }

  else for (;;)
    {
    uint32_t ch;
    unsigned int chlen;

    if (ptr >= repend)
      {
      if (ptrstackptr == 0) break;       /* End of replacement string */
      repend = ptrstack[--ptrstackptr];
      ptr = ptrstack[--ptrstackptr];
      continue;
      }

    if (escaped_literal)
      {
      if (ptr[0] == CHAR_BACKSLASH && ptr < repend - 1 && ptr[1] == CHAR_E)
        {
        escaped_literal = FALSE;
        ptr += 2;
        continue;
        }
      goto LOADLITERAL;
      }

    if (*ptr == CHAR_DOLLAR_SIGN)
      {
      int group, n;
      uint32_t special = 0;
      BOOL inparens;
      BOOL star;
      PCRE2_SIZE sublength;
      PCRE2_SPTR text1_start = NULL;
      PCRE2_SPTR text1_end = NULL;
      PCRE2_SPTR text2_start = NULL;
      PCRE2_SPTR text2_end = NULL;
      PCRE2_UCHAR next;
      PCRE2_UCHAR name[33];

      if (++ptr >= repend) goto BAD;
      if ((next = *ptr) == CHAR_DOLLAR_SIGN) goto LOADLITERAL;

      group = -1;
      n = 0;
      inparens = FALSE;
      star = FALSE;

      if (next == CHAR_LEFT_CURLY_BRACKET)
        {
        if (++ptr >= repend) goto BAD;
        next = *ptr;
        inparens = TRUE;
        }

      if (next == CHAR_ASTERISK)
        {
        if (++ptr >= repend) goto BAD;
        next = *ptr;
        star = TRUE;
        }

      if (!star && next >= CHAR_0 && next <= CHAR_9)
        {
        group = next - CHAR_0;
        while (++ptr < repend)
          {
          next = *ptr;
          if (next < CHAR_0 || next > CHAR_9) break;
          group = group * 10 + next - CHAR_0;

          if (group > code->top_bracket)
            {
            if ((suboptions & PCRE2_SUBSTITUTE_UNKNOWN_UNSET) != 0)
              {
              while (++ptr < repend && *ptr >= CHAR_0 && *ptr <= CHAR_9);
              break;
              }
            else
              {
              rc = PCRE2_ERROR_NOSUBSTRING;
              goto PTREXIT;
              }
            }
          }
        }
      else
        {
        const uint8_t *ctypes = code->tables + ctypes_offset;
        while (MAX_255(next) && (ctypes[next] & ctype_word) != 0)
          {
          name[n++] = next;
          if (n > 32) goto BAD;
          if (++ptr >= repend) break;
          next = *ptr;
          }
        if (n == 0) goto BAD;
        name[n] = 0;
        }

      if (inparens)
        {
        if ((suboptions & PCRE2_SUBSTITUTE_EXTENDED) != 0 &&
             !star && ptr < repend - 2 && next == CHAR_COLON)
          {
          special = *(++ptr);
          if (special != CHAR_PLUS && special != CHAR_MINUS)
            {
            rc = PCRE2_ERROR_BADSUBSTITUTION;
            goto PTREXIT;
            }

          text1_start = ++ptr;
          rc = find_text_end(code, &ptr, repend, special == CHAR_MINUS);
          if (rc != 0) goto PTREXIT;
          text1_end = ptr;

          if (special == CHAR_PLUS && *ptr == CHAR_COLON)
            {
            text2_start = ++ptr;
            rc = find_text_end(code, &ptr, repend, TRUE);
            if (rc != 0) goto PTREXIT;
            text2_end = ptr;
            }
          }

        else
          {
          if (ptr >= repend || *ptr != CHAR_RIGHT_CURLY_BRACKET)
            {
            rc = PCRE2_ERROR_REPMISSINGBRACE;
            goto PTREXIT;
            }
          }

        ptr++;
      }

      if (star)
        {
        if (PRIV(strcmp_c8)(name, STRING_MARK) == 0)
          {
          PCRE2_SPTR mark = pcre2_get_mark(match_data);
          if (mark != NULL)
            {
            PCRE2_SPTR mark_start = mark;
            while (*mark != 0) mark++;
            fraglength = mark - mark_start;
            CHECKMEMCPY(mark_start, fraglength);
            }
          }
        else goto BAD;
        }

      else
        {
        PCRE2_SPTR subptr, subptrend;

        if (group < 0)
          {
          PCRE2_SPTR first, last, entry;
          rc = pcre2_substring_nametable_scan(code, name, &first, &last);
          if (rc == PCRE2_ERROR_NOSUBSTRING &&
              (suboptions & PCRE2_SUBSTITUTE_UNKNOWN_UNSET) != 0)
            {
            group = code->top_bracket + 1;
            }
          else
            {
            if (rc < 0) goto PTREXIT;
            for (entry = first; entry <= last; entry += rc)
              {
              uint32_t ng = GET2(entry, 0);
              if (ng < ovector_count)
                {
                if (group < 0) group = ng;          /* First in ovector */
                if (ovector[ng*2] != PCRE2_UNSET)
                  {
                  group = ng;                       /* First that is set */
                  break;
                  }
                }
              }

            if (group < 0) group = GET2(first, 0);
            }
          }

        rc = pcre2_substring_length_bynumber(match_data, group, &sublength);
        if (rc < 0)
          {
          if (rc == PCRE2_ERROR_NOSUBSTRING &&
              (suboptions & PCRE2_SUBSTITUTE_UNKNOWN_UNSET) != 0)
            {
            rc = PCRE2_ERROR_UNSET;
            }
          if (rc != PCRE2_ERROR_UNSET) goto PTREXIT;  /* Non-unset errors */
          if (special == 0)                           /* Plain substitution */
            {
            if ((suboptions & PCRE2_SUBSTITUTE_UNSET_EMPTY) != 0) continue;
            goto PTREXIT;                             /* Else error */
            }
          }

        if (special != 0)
          {
          if (special == CHAR_MINUS)
            {
            if (rc == 0) goto LITERAL_SUBSTITUTE;
            text2_start = text1_start;
            text2_end = text1_end;
            }

          if (ptrstackptr >= PTR_STACK_SIZE) goto BAD;
          ptrstack[ptrstackptr++] = ptr;
          ptrstack[ptrstackptr++] = repend;

          if (rc == 0)
            {
            ptr = text1_start;
            repend = text1_end;
            }
          else
            {
            ptr = text2_start;
            repend = text2_end;
            }
          continue;
          }

        LITERAL_SUBSTITUTE:
        subptr = subject + ovector[group*2];
        subptrend = subject + ovector[group*2 + 1];

        while (subptr < subptrend)
          {
          GETCHARINCTEST(ch, subptr);
          if (forcecase != 0)
            {
#ifdef SUPPORT_UNICODE
            if (utf || ucp)
              {
              uint32_t type = UCD_CHARTYPE(ch);
              if (PRIV(ucp_gentype)[type] == ucp_L &&
                  type != ((forcecase > 0)? ucp_Lu : ucp_Ll))
                ch = UCD_OTHERCASE(ch);
              }
            else
#endif
              {
              if (((code->tables + cbits_offset +
                  ((forcecase > 0)? cbit_upper:cbit_lower)
                  )[ch/8] & (1u << (ch%8))) == 0)
                ch = (code->tables + fcc_offset)[ch];
              }
            forcecase = forcecasereset;
            }

#ifdef SUPPORT_UNICODE
          if (utf) chlen = PRIV(ord2utf)(ch, temp); else
#endif
            {
            temp[0] = ch;
            chlen = 1;
            }
          CHECKMEMCPY(temp, chlen);
          }
        }
      }

    else if ((suboptions & PCRE2_SUBSTITUTE_EXTENDED) != 0 &&
              *ptr == CHAR_BACKSLASH)
      {
      int errorcode;

      if (ptr < repend - 1) switch (ptr[1])
        {
        case CHAR_L:
        forcecase = forcecasereset = -1;
        ptr += 2;
        continue;

        case CHAR_l:
        forcecase = -1;
        forcecasereset = 0;
        ptr += 2;
        continue;

        case CHAR_U:
        forcecase = forcecasereset = 1;
        ptr += 2;
        continue;

        case CHAR_u:
        forcecase = 1;
        forcecasereset = 0;
        ptr += 2;
        continue;

        default:
        break;
        }

      ptr++;  /* Point after \ */
      rc = PRIV(check_escape)(&ptr, repend, &ch, &errorcode,
        code->overall_options, code->extra_options, FALSE, NULL);
      if (errorcode != 0) goto BADESCAPE;

      switch(rc)
        {
        case ESC_E:
        forcecase = forcecasereset = 0;
        continue;

        case ESC_Q:
        escaped_literal = TRUE;
        continue;

        case 0:      /* Data character */
        goto LITERAL;

        default:
        goto BADESCAPE;
        }
      }

    else
      {
      LOADLITERAL:
      GETCHARINCTEST(ch, ptr);    /* Get character value, increment pointer */

      LITERAL:
      if (forcecase != 0)
        {
#ifdef SUPPORT_UNICODE
        if (utf || ucp)
          {
          uint32_t type = UCD_CHARTYPE(ch);
          if (PRIV(ucp_gentype)[type] == ucp_L &&
              type != ((forcecase > 0)? ucp_Lu : ucp_Ll))
            ch = UCD_OTHERCASE(ch);
          }
        else
#endif
          {
          if (((code->tables + cbits_offset +
              ((forcecase > 0)? cbit_upper:cbit_lower)
              )[ch/8] & (1u << (ch%8))) == 0)
            ch = (code->tables + fcc_offset)[ch];
          }
        forcecase = forcecasereset;
        }

#ifdef SUPPORT_UNICODE
      if (utf) chlen = PRIV(ord2utf)(ch, temp); else
#endif
        {
        temp[0] = ch;
        chlen = 1;
        }
      CHECKMEMCPY(temp, chlen);
      } /* End handling a literal code unit */
    }

  if (!overflowed && mcontext != NULL && mcontext->substitute_callout != NULL)
    {
    scb.subscount = subs;
    scb.output_offsets[1] = buff_offset;
    rc = mcontext->substitute_callout(&scb, mcontext->substitute_callout_data);

    if (rc != 0)
      {
      PCRE2_SIZE newlength = scb.output_offsets[1] - scb.output_offsets[0];
      PCRE2_SIZE oldlength = ovector[1] - ovector[0];

      buff_offset -= newlength;
      lengthleft += newlength;
      if (!replacement_only) CHECKMEMCPY(subject + ovector[0], oldlength);

      /* A negative return means do not do any more. */

      if (rc < 0) suboptions &= (~PCRE2_SUBSTITUTE_GLOBAL);
      }
    }

  ovecsave[0] = ovector[0];
  ovecsave[1] = ovector[1];
  ovecsave[2] = start_offset;

  goptions = (ovector[0] != ovector[1] || ovector[0] > start_offset)? 0 :
    PCRE2_ANCHORED|PCRE2_NOTEMPTY_ATSTART;
  start_offset = ovector[1];
  } while ((suboptions & PCRE2_SUBSTITUTE_GLOBAL) != 0);

if (!replacement_only)
  {
  fraglength = length - start_offset;
  CHECKMEMCPY(subject + start_offset, fraglength);
  }

temp[0] = 0;
CHECKMEMCPY(temp, 1);

if (overflowed)
  {
  rc = PCRE2_ERROR_NOMEMORY;
  *blength = buff_length + extra_needed;
  }

else
  {
  rc = subs;
  *blength = buff_offset - 1;
  }

EXIT:
if (internal_match_data != NULL) pcre2_match_data_free(internal_match_data);
  else match_data->rc = rc;
return rc;

NOROOM:
rc = PCRE2_ERROR_NOMEMORY;
goto EXIT;

BAD:
rc = PCRE2_ERROR_BADREPLACEMENT;
goto PTREXIT;

BADESCAPE:
rc = PCRE2_ERROR_BADREPESCAPE;

PTREXIT:
*blength = (PCRE2_SIZE)(ptr - replacement);
goto EXIT;
}

/* End of pcre2_substitute.c */
