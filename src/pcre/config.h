#ifndef _WIN32
#define HAVE_ATTRIBUTE_UNINITIALIZED 1
#else
#undef HAVE_ATTRIBUTE_UNINITIALIZED
#endif

#define PCRE2_STATIC 1

#define SUPPORT_PCRE2_8 1
#define SUPPORT_UNICODE 1

/* #undef SUPPORT_JIT */
/* #undef SLJIT_PROT_EXECUTABLE_ALLOCATOR */
/* #undef SUPPORT_VALGRIND */

#ifndef LINK_SIZE
#define LINK_SIZE		2
#endif

#ifndef HEAP_LIMIT
#define HEAP_LIMIT              20000000
#endif

#ifndef MATCH_LIMIT
#define MATCH_LIMIT		10000000
#endif

#ifndef MATCH_LIMIT_DEPTH
#define MATCH_LIMIT_DEPTH	MATCH_LIMIT
#endif

/* The value of NEWLINE_DEFAULT determines the default newline character
   sequence. PCRE2 client programs can override this by selecting other values
   at run time. The valid values are 1 (CR), 2 (LF), 3 (CRLF), 4 (ANY), 5
   (ANYCRLF), and 6 (NUL). */
#ifndef NEWLINE_DEFAULT
#define NEWLINE_DEFAULT         2
#endif

#ifndef PARENS_NEST_LIMIT
#define PARENS_NEST_LIMIT       250
#endif

#define PCRE2GREP_BUFSIZE       20480
#define PCRE2GREP_MAX_BUFSIZE   1048576

#ifndef MAX_NAME_SIZE
#define MAX_NAME_SIZE	32
#endif

#ifndef MAX_NAME_COUNT
#define MAX_NAME_COUNT	10000
#endif
