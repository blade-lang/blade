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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre2_internal.h"

/*************************************************
*                Check script run                *
*************************************************/

#define SCRIPT_UNSET        (-99999)
#define SCRIPT_HANPENDING   (-99998)
#define SCRIPT_HANHIRAKATA  (-99997)
#define SCRIPT_HANBOPOMOFO  (-99996)
#define SCRIPT_HANHANGUL    (-99995)
#define SCRIPT_LIST         (-99994)

#define INTERSECTION_LIST_SIZE 50

BOOL
PRIV(script_run)(PCRE2_SPTR ptr, PCRE2_SPTR endptr, BOOL utf)
{
#ifdef SUPPORT_UNICODE
int require_script = SCRIPT_UNSET;
uint8_t intersection_list[INTERSECTION_LIST_SIZE];
const uint8_t *require_list = NULL;
uint32_t require_digitset = 0;
uint32_t c;

#if PCRE2_CODE_UNIT_WIDTH == 32
(void)utf;    /* Avoid compiler warning */
#endif

if (ptr >= endptr) return TRUE;
GETCHARINCTEST(c, ptr);
if (ptr >= endptr) return TRUE;

for (;;)
  {
  const ucd_record *ucd = GET_UCD(c);
  int32_t scriptx = ucd->scriptx;

  if (scriptx == ucp_Unknown) return FALSE;

  if (scriptx != ucp_Inherited)
    {
    if (scriptx != ucp_Common)
      {

      if (scriptx > 0)
        {
        switch(require_script)
          {

          case SCRIPT_UNSET:
          case SCRIPT_HANPENDING:
          switch(scriptx)
            {
            case ucp_Han:
            require_script = SCRIPT_HANPENDING;
            break;

            case ucp_Hiragana:
            case ucp_Katakana:
            require_script = SCRIPT_HANHIRAKATA;
            break;

            case ucp_Bopomofo:
            require_script = SCRIPT_HANBOPOMOFO;
            break;

            case ucp_Hangul:
            require_script = SCRIPT_HANHANGUL;
            break;

            default:
            if (require_script == SCRIPT_HANPENDING) return FALSE;
            require_script = scriptx;
            break;
            }
          break;

          case SCRIPT_HANHIRAKATA:
          if (scriptx != ucp_Han && scriptx != ucp_Hiragana && 
              scriptx != ucp_Katakana)
            return FALSE;
          break;

          case SCRIPT_HANBOPOMOFO:
          if (scriptx != ucp_Han && scriptx != ucp_Bopomofo) return FALSE;
          break;

          case SCRIPT_HANHANGUL:
          if (scriptx != ucp_Han && scriptx != ucp_Hangul) return FALSE;
          break;

          case SCRIPT_LIST:
            {
            const uint8_t *list;
            for (list = require_list; *list != 0; list++)
              {
              if (*list == scriptx) break;
              }
            if (*list == 0) return FALSE;
            }
          
          switch(scriptx)
            {
            case ucp_Han:
            require_script = SCRIPT_HANPENDING;
            break;

            case ucp_Hiragana:
            case ucp_Katakana:
            require_script = SCRIPT_HANHIRAKATA;
            break;

            case ucp_Bopomofo:
            require_script = SCRIPT_HANBOPOMOFO;
            break;

            case ucp_Hangul:
            require_script = SCRIPT_HANHANGUL;
            break;

            default:
            require_script = scriptx;
            break;
            }  
          break;

          default:
          if (scriptx != require_script) return FALSE;
          break;
          }
        }  /* End of handing positive scriptx */

      else
        {
        uint32_t chspecial;
        const uint8_t *clist, *rlist;
        const uint8_t *list = PRIV(ucd_script_sets) - scriptx;
        
        switch(require_script)
          {
          case SCRIPT_UNSET:
          require_list = PRIV(ucd_script_sets) - scriptx;
          require_script = SCRIPT_LIST;
          break;

#define FOUND_BOPOMOFO 1
#define FOUND_HIRAGANA 2
#define FOUND_KATAKANA 4
#define FOUND_HANGUL   8

          case SCRIPT_HANPENDING:
          chspecial = 0;
          for (; *list != 0; list++)
            {
            switch (*list)
              {
              case ucp_Bopomofo: chspecial |= FOUND_BOPOMOFO; break;
              case ucp_Hiragana: chspecial |= FOUND_HIRAGANA; break;
              case ucp_Katakana: chspecial |= FOUND_KATAKANA; break;
              case ucp_Hangul:   chspecial |= FOUND_HANGUL; break;
              default: break;
              }
            }

           if (chspecial == 0) return FALSE;

           if (chspecial == FOUND_BOPOMOFO)
             {
             require_script = SCRIPT_HANBOPOMOFO;
             }
           else if (chspecial == (FOUND_HIRAGANA|FOUND_KATAKANA))
             {
             require_script = SCRIPT_HANHIRAKATA;
             }

          break;

          case SCRIPT_HANHIRAKATA:
          for (; *list != 0; list++)
            {
            if (*list == ucp_Hiragana || *list == ucp_Katakana) break;
            }
          if (*list == 0) return FALSE;
          break;

          case SCRIPT_HANBOPOMOFO:
          for (; *list != 0; list++)
            {
            if (*list == ucp_Bopomofo) break;
            }
          if (*list == 0) return FALSE;
          break;

          case SCRIPT_HANHANGUL:
          for (; *list != 0; list++)
            {
            if (*list == ucp_Hangul) break;
            }
          if (*list == 0) return FALSE;
          break;

          case SCRIPT_LIST:
            {
            int i = 0;
            for (rlist = require_list; *rlist != 0; rlist++)
              {
              for (clist = list; *clist != 0; clist++)
                {
                if (*rlist == *clist)
                  {
                  intersection_list[i++] = *rlist;
                  break;
                  }
                }
              }
            if (i == 0) return FALSE;  /* No scripts in common */

            if (i == 1)
              {
              require_script = intersection_list[0];
              }
            else
              {
              intersection_list[i] = 0;
              require_list = intersection_list;
              }
            }
          break;

          default:
          for (; *list != 0; list++)
            {
            if (*list == require_script) break;
            }
          if (*list == 0) return FALSE;
          break;
          }
        }  /* End of handling negative scriptx */
      }    /* End of checking non-Common character */

    if (ucd->chartype == ucp_Nd)
      {
      uint32_t digitset;

      if (c <= PRIV(ucd_digit_sets)[1]) digitset = 1; else
        {
        int mid;
        int bot = 1;
        int top = PRIV(ucd_digit_sets)[0];
        for (;;)
          {
          if (top <= bot + 1)    /* <= rather than == is paranoia */
            {
            digitset = top;
            break;
            }
          mid = (top + bot) / 2;
          if (c <= PRIV(ucd_digit_sets)[mid]) top = mid; else bot = mid;
          }
        }

      if (require_digitset == 0) require_digitset = digitset;
        else if (digitset != require_digitset) return FALSE;
      }   /* End digit handling */
    }     /* End checking non-Inherited character */

  if (ptr >= endptr) return TRUE;
  GETCHARINCTEST(c, ptr);
  }  /* End checking loop */

#else   /* NOT SUPPORT_UNICODE */
(void)ptr;
(void)endptr;
(void)utf;
return TRUE;
#endif  /* SUPPORT_UNICODE */
}

/* End of pcre2_script_run.c */
