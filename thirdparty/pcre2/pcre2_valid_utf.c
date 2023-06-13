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

#ifndef PCRE2_PCRE2TEST           /* We're compiling the library */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "pcre2_internal.h"
#endif /* PCRE2_PCRE2TEST */

#ifndef SUPPORT_UNICODE

int
PRIV(valid_utf)(PCRE2_SPTR string, PCRE2_SIZE length, PCRE2_SIZE *erroroffset)
{
(void)string;
(void)length;
(void)erroroffset;
return 0;
}
#else  /* UTF is supported */

/*************************************************
*           Validate a UTF string                *
*************************************************/

int
PRIV(valid_utf)(PCRE2_SPTR string, PCRE2_SIZE length, PCRE2_SIZE *erroroffset)
{
PCRE2_SPTR p;
uint32_t c;

/* ----------------- Check a UTF-8 string ----------------- */

#if PCRE2_CODE_UNIT_WIDTH == 8

for (p = string; length > 0; p++)
  {
  uint32_t ab, d;

  c = *p;
  length--;

  if (c < 128) continue;                /* ASCII character */

  if (c < 0xc0)                         /* Isolated 10xx xxxx byte */
    {
    *erroroffset = (PCRE2_SIZE)(p - string);
    return PCRE2_ERROR_UTF8_ERR20;
    }

  if (c >= 0xfe)                        /* Invalid 0xfe or 0xff bytes */
    {
    *erroroffset = (PCRE2_SIZE)(p - string);
    return PCRE2_ERROR_UTF8_ERR21;
    }

  ab = PRIV(utf8_table4)[c & 0x3f];     /* Number of additional bytes (1-5) */
  if (length < ab)                      /* Missing bytes */
    {
    *erroroffset = (PCRE2_SIZE)(p - string);
    switch(ab - length)
      {
      case 1: return PCRE2_ERROR_UTF8_ERR1;
      case 2: return PCRE2_ERROR_UTF8_ERR2;
      case 3: return PCRE2_ERROR_UTF8_ERR3;
      case 4: return PCRE2_ERROR_UTF8_ERR4;
      case 5: return PCRE2_ERROR_UTF8_ERR5;
      }
    }
  length -= ab;                         /* Length remaining */

  /* Check top bits in the second byte */

  if (((d = *(++p)) & 0xc0) != 0x80)
    {
    *erroroffset = (int)(p - string) - 1;
    return PCRE2_ERROR_UTF8_ERR6;
    }

  switch (ab)
    {

    case 1: if ((c & 0x3e) == 0)
      {
      *erroroffset = (int)(p - string) - 1;
      return PCRE2_ERROR_UTF8_ERR15;
      }
    break;

    case 2:
    if ((*(++p) & 0xc0) != 0x80)     /* Third byte */
      {
      *erroroffset = (int)(p - string) - 2;
      return PCRE2_ERROR_UTF8_ERR7;
      }
    if (c == 0xe0 && (d & 0x20) == 0)
      {
      *erroroffset = (int)(p - string) - 2;
      return PCRE2_ERROR_UTF8_ERR16;
      }
    if (c == 0xed && d >= 0xa0)
      {
      *erroroffset = (int)(p - string) - 2;
      return PCRE2_ERROR_UTF8_ERR14;
      }
    break;

    case 3:
    if ((*(++p) & 0xc0) != 0x80)     /* Third byte */
      {
      *erroroffset = (int)(p - string) - 2;
      return PCRE2_ERROR_UTF8_ERR7;
      }
    if ((*(++p) & 0xc0) != 0x80)     /* Fourth byte */
      {
      *erroroffset = (int)(p - string) - 3;
      return PCRE2_ERROR_UTF8_ERR8;
      }
    if (c == 0xf0 && (d & 0x30) == 0)
      {
      *erroroffset = (int)(p - string) - 3;
      return PCRE2_ERROR_UTF8_ERR17;
      }
    if (c > 0xf4 || (c == 0xf4 && d > 0x8f))
      {
      *erroroffset = (int)(p - string) - 3;
      return PCRE2_ERROR_UTF8_ERR13;
      }
    break;

    case 4:
    if ((*(++p) & 0xc0) != 0x80)     /* Third byte */
      {
      *erroroffset = (int)(p - string) - 2;
      return PCRE2_ERROR_UTF8_ERR7;
      }
    if ((*(++p) & 0xc0) != 0x80)     /* Fourth byte */
      {
      *erroroffset = (int)(p - string) - 3;
      return PCRE2_ERROR_UTF8_ERR8;
      }
    if ((*(++p) & 0xc0) != 0x80)     /* Fifth byte */
      {
      *erroroffset = (int)(p - string) - 4;
      return PCRE2_ERROR_UTF8_ERR9;
      }
    if (c == 0xf8 && (d & 0x38) == 0)
      {
      *erroroffset = (int)(p - string) - 4;
      return PCRE2_ERROR_UTF8_ERR18;
      }
    break;

    case 5:
    if ((*(++p) & 0xc0) != 0x80)     /* Third byte */
      {
      *erroroffset = (int)(p - string) - 2;
      return PCRE2_ERROR_UTF8_ERR7;
      }
    if ((*(++p) & 0xc0) != 0x80)     /* Fourth byte */
      {
      *erroroffset = (int)(p - string) - 3;
      return PCRE2_ERROR_UTF8_ERR8;
      }
    if ((*(++p) & 0xc0) != 0x80)     /* Fifth byte */
      {
      *erroroffset = (int)(p - string) - 4;
      return PCRE2_ERROR_UTF8_ERR9;
      }
    if ((*(++p) & 0xc0) != 0x80)     /* Sixth byte */
      {
      *erroroffset = (int)(p - string) - 5;
      return PCRE2_ERROR_UTF8_ERR10;
      }
    if (c == 0xfc && (d & 0x3c) == 0)
      {
      *erroroffset = (int)(p - string) - 5;
      return PCRE2_ERROR_UTF8_ERR19;
      }
    break;
    }

  if (ab > 3)
    {
    *erroroffset = (int)(p - string) - ab;
    return (ab == 4)? PCRE2_ERROR_UTF8_ERR11 : PCRE2_ERROR_UTF8_ERR12;
    }
  }
return 0;


/* ----------------- Check a UTF-16 string ----------------- */

#elif PCRE2_CODE_UNIT_WIDTH == 16

for (p = string; length > 0; p++)
  {
  c = *p;
  length--;

  if ((c & 0xf800) != 0xd800)
    {
    /* Normal UTF-16 code point. Neither high nor low surrogate. */
    }
  else if ((c & 0x0400) == 0)
    {
    /* High surrogate. Must be a followed by a low surrogate. */
    if (length == 0)
      {
      *erroroffset = p - string;
      return PCRE2_ERROR_UTF16_ERR1;
      }
    p++;
    length--;
    if ((*p & 0xfc00) != 0xdc00)
      {
      *erroroffset = p - string - 1;
      return PCRE2_ERROR_UTF16_ERR2;
      }
    }
  else
    {
    /* Isolated low surrogate. Always an error. */
    *erroroffset = p - string;
    return PCRE2_ERROR_UTF16_ERR3;
    }
  }
return 0;



/* ----------------- Check a UTF-32 string ----------------- */

#else

for (p = string; length > 0; length--, p++)
  {
  c = *p;
  if ((c & 0xfffff800u) != 0xd800u)
    {
    /* Normal UTF-32 code point. Neither high nor low surrogate. */
    if (c > 0x10ffffu)
      {
      *erroroffset = p - string;
      return PCRE2_ERROR_UTF32_ERR2;
      }
    }
  else
    {
    /* A surrogate */
    *erroroffset = p - string;
    return PCRE2_ERROR_UTF32_ERR1;
    }
  }
return 0;
#endif  /* CODE_UNIT_WIDTH */
}
#endif  /* SUPPORT_UNICODE */

/* End of pcre2_valid_utf.c */
