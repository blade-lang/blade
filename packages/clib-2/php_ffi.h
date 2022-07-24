/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Dmitry Stogov <dmitry@clib.com>                              |
   +----------------------------------------------------------------------+
 */

#ifndef PHP_FFI_H
#define PHP_FFI_H

#include <blade.h>

#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>

#ifdef HAVE_SIGSETJMP
# define SETJMP(a) sigsetjmp(a, 0)
# define LONGJMP(a,b) siglongjmp(a, b)
# define JMP_BUF sigjmp_buf
#else
# define SETJMP(a) setjmp(a)
# define LONGJMP(a,b) longjmp(a, b)
# define JMP_BUF jmp_buf
#endif

typedef enum _clib_ffi_api_restriction {
	CLIB_FFI_DISABLED = 0,  /* completely disabled */
	CLIB_FFI_ENABLED = 1,   /* enabled everywhere */
	CLIB_FFI_PRELOAD = 2,   /* enabled only in preloaded scripts and CLI */
} clib_ffi_api_restriction;

typedef struct _clib_ffi_type  clib_ffi_type;

typedef struct _b_ffi_globals {
	clib_ffi_api_restriction restriction;
	bool is_cli;

	/* predefined ffi_types */
	b_obj_dict types;

	/* preloading */
	char *preload;
	b_obj_dict *scopes;           /* list of preloaded scopes */

	/* callbacks */
	b_obj_dict *callbacks;

	/* weak type references */
	b_obj_dict *weak_types;

	/* ffi_parser */
	JMP_BUF	bailout;
	unsigned const char *buf;
	unsigned const char *end;
	unsigned const char *pos;
	unsigned const char *text;
	int line;
	b_obj_dict *symbols;
	b_obj_dict *tags;
	bool allow_vla;
	bool attribute_parsing;
	bool persistent;
	uint32_t  default_type_attr;
} b_ffi_globals;

extern b_ffi_globals _clib_ffi_global_data;

#ifdef PHP_WIN32
# define PHP_FFI_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_FFI_API __attribute__ ((visibility("default")))
#else
# define PHP_FFI_API
#endif

#if defined(__GNUC__) && CLIB_GCC_VERSION >= 3004 && defined(__i386__)
# define CLIB_FASTCALL __attribute__((fastcall))
#elif defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1700
# define CLIB_FASTCALL __fastcall
#elif defined(_MSC_VER) && _MSC_VER >= 1800 && !defined(__clang__)
# define CLIB_FASTCALL __vectorcall
#else
# define CLIB_FASTCALL
#endif

#if (defined(__GNUC__) && __GNUC__ >= 3 && !defined(__INTEL_COMPILER) && !defined(DARWIN) && !defined(__hpux) && !defined(_AIX) && !defined(__osf__)) || __has_attribute(noreturn)
# define HAVE_NORETURN
# define CLIB_NORETURN __attribute__((noreturn))
#elif defined(CLIB_WIN32)
# define HAVE_NORETURN
# define CLIB_NORETURN __declspec(noreturn)
#else
# define CLIB_NORETURN
#endif

#if CLIB_DEBUG || defined(CLIB_WIN32_NEVER_INLINE)
# define clib_always_inline inline
# define clib_never_inline
#else
# if defined(__GNUC__)
#  if __GNUC__ >= 3
#   define clib_always_inline inline __attribute__((always_inline))
#   define clib_never_inline __attribute__((noinline))
#  else
#   define clib_always_inline inline
#   define clib_never_inline
#  endif
# elif defined(_MSC_VER)
#  define clib_always_inline __forceinline
#  define clib_never_inline __declspec(noinline)
# else
#  if __has_attribute(always_inline)
#   define clib_always_inline inline __attribute__((always_inline))
#  else
#   define clib_always_inline inline
#  endif
#  if __has_attribute(noinline)
#   define clib_never_inline __attribute__((noinline))
#  else
#   define clib_never_inline
#  endif
# endif
#endif /* CLIB_DEBUG */

#if defined(__GNUC__) && CLIB_GCC_VERSION >= 4003
# define CLIB_COLD __attribute__((cold))
# define CLIB_HOT __attribute__((hot))
# ifdef __OPTIMIZE__
#  define CLIB_OPT_SIZE  __attribute__((optimize("Os")))
#  define CLIB_OPT_SPEED __attribute__((optimize("Ofast")))
# else
#  define CLIB_OPT_SIZE
#  define CLIB_OPT_SPEED
# endif
#else
# define CLIB_COLD
# define CLIB_HOT
# define CLIB_OPT_SIZE
# define CLIB_OPT_SPEED
#endif

//#ifdef HAVE_BUILTIN_EXPECT
//# define EXPECTED(condition)   __builtin_expect(!!(condition), 1)
//# define UNEXPECTED(condition) __builtin_expect(!!(condition), 0)
//#else
# define EXPECTED(condition)   (condition)
# define UNEXPECTED(condition) (condition)
//#endif

#define FFI_G(v) (_clib_ffi_global_data.v)

#define CLIB_FFI_DCL_VOID            (1<<0)
#define CLIB_FFI_DCL_CHAR            (1<<1)
#define CLIB_FFI_DCL_SHORT           (1<<2)
#define CLIB_FFI_DCL_INT             (1<<3)
#define CLIB_FFI_DCL_LONG            (1<<4)
#define CLIB_FFI_DCL_LONG_LONG       (1<<5)
#define CLIB_FFI_DCL_FLOAT           (1<<6)
#define CLIB_FFI_DCL_DOUBLE          (1<<7)
#define CLIB_FFI_DCL_SIGNED          (1<<8)
#define CLIB_FFI_DCL_UNSIGNED        (1<<9)
#define CLIB_FFI_DCL_BOOL            (1<<10)
#define CLIB_FFI_DCL_COMPLEX         (1<<11)

#define CLIB_FFI_DCL_STRUCT          (1<<12)
#define CLIB_FFI_DCL_UNION           (1<<13)
#define CLIB_FFI_DCL_ENUM            (1<<14)
#define CLIB_FFI_DCL_TYPEDEF_NAME    (1<<15)

#define CLIB_FFI_DCL_TYPE_SPECIFIERS \
	(CLIB_FFI_DCL_VOID|CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_SHORT \
	|CLIB_FFI_DCL_INT|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_LONG_LONG \
	|CLIB_FFI_DCL_FLOAT|CLIB_FFI_DCL_DOUBLE|CLIB_FFI_DCL_SIGNED \
	|CLIB_FFI_DCL_UNSIGNED|CLIB_FFI_DCL_BOOL|CLIB_FFI_DCL_COMPLEX \
	|CLIB_FFI_DCL_STRUCT|CLIB_FFI_DCL_UNION|CLIB_FFI_DCL_ENUM \
	|CLIB_FFI_DCL_TYPEDEF_NAME)

#define CLIB_FFI_DCL_TYPEDEF         (1<<16)
#define CLIB_FFI_DCL_EXTERN          (1<<17)
#define CLIB_FFI_DCL_STATIC          (1<<18)
#define CLIB_FFI_DCL_AUTO            (1<<19)
#define CLIB_FFI_DCL_REGISTER        (1<<20)

#define CLIB_FFI_DCL_STORAGE_CLASS \
	(CLIB_FFI_DCL_TYPEDEF|CLIB_FFI_DCL_EXTERN|CLIB_FFI_DCL_STATIC \
	|CLIB_FFI_DCL_AUTO|CLIB_FFI_DCL_REGISTER)

#define CLIB_FFI_DCL_CONST           (1<<21)
#define CLIB_FFI_DCL_RESTRICT        (1<<22)
#define CLIB_FFI_DCL_VOLATILE        (1<<23)
#define CLIB_FFI_DCL_ATOMIC          (1<<24)

#define CLIB_FFI_DCL_TYPE_QUALIFIERS \
	(CLIB_FFI_DCL_CONST|CLIB_FFI_DCL_RESTRICT|CLIB_FFI_DCL_VOLATILE \
	|CLIB_FFI_DCL_ATOMIC)

#define CLIB_FFI_DCL_INLINE          (1<<25)
#define CLIB_FFI_DCL_NO_RETURN       (1<<26)

#define CLIB_FFI_ABI_DEFAULT        0

#define CLIB_FFI_ABI_CDECL          1  // FFI_DEFAULT_ABI
#define CLIB_FFI_ABI_FASTCALL       2  // FFI_FASTCALL
#define CLIB_FFI_ABI_THISCALL       3  // FFI_THISCALL
#define CLIB_FFI_ABI_STDCALL        4  // FFI_STDCALL
#define	CLIB_FFI_ABI_PASCAL         5  // FFI_PASCAL
#define	CLIB_FFI_ABI_REGISTER       6  // FFI_REGISTER
#define	CLIB_FFI_ABI_MS             7  // FFI_MS_CDECL
#define	CLIB_FFI_ABI_SYSV           8  // FFI_SYSV
#define CLIB_FFI_ABI_VECTORCALL     9  // FFI_VECTORCALL

#define CLIB_FFI_ATTR_CONST             (1<<0)
#define CLIB_FFI_ATTR_INCOMPLETE_TAG    (1<<1)
#define CLIB_FFI_ATTR_VARIADIC          (1<<2)
#define CLIB_FFI_ATTR_INCOMPLETE_ARRAY  (1<<3)
#define CLIB_FFI_ATTR_VLA               (1<<4)
#define	CLIB_FFI_ATTR_UNION             (1<<5)
#define	CLIB_FFI_ATTR_PACKED            (1<<6)
#define	CLIB_FFI_ATTR_MS_STRUCT         (1<<7)
#define	CLIB_FFI_ATTR_GCC_STRUCT        (1<<8)

#define	CLIB_FFI_ATTR_PERSISTENT        (1<<9)
#define	CLIB_FFI_ATTR_STORED            (1<<10)

#define CLIB_FFI_STRUCT_ATTRS \
	(CLIB_FFI_ATTR_UNION|CLIB_FFI_ATTR_PACKED|CLIB_FFI_ATTR_MS_STRUCT \
	|CLIB_FFI_ATTR_GCC_STRUCT)

#define CLIB_FFI_ENUM_ATTRS \
	(CLIB_FFI_ATTR_PACKED)

#define CLIB_FFI_ARRAY_ATTRS \
	(CLIB_FFI_ATTR_CONST|CLIB_FFI_ATTR_VLA|CLIB_FFI_ATTR_INCOMPLETE_ARRAY)

#define CLIB_FFI_FUNC_ATTRS \
	(CLIB_FFI_ATTR_VARIADIC)

#define CLIB_FFI_POINTER_ATTRS \
	(CLIB_FFI_ATTR_CONST)

typedef struct _clib_ffi_dcl {
	uint32_t       flags;
	uint32_t       align;
	uint16_t       attr;
	uint16_t       abi;
	clib_ffi_type *type;
} clib_ffi_dcl;

#define CLIB_FFI_ATTR_INIT {0, 0, 0, 0, NULL}

typedef enum _clib_ffi_val_kind {
	CLIB_FFI_VAL_EMPTY,
	CLIB_FFI_VAL_ERROR,
	CLIB_FFI_VAL_INT32,
	CLIB_FFI_VAL_INT64,
	CLIB_FFI_VAL_UINT32,
	CLIB_FFI_VAL_UINT64,
	CLIB_FFI_VAL_FLOAT,
	CLIB_FFI_VAL_DOUBLE,
	CLIB_FFI_VAL_LONG_DOUBLE,
	CLIB_FFI_VAL_CHAR,
	CLIB_FFI_VAL_STRING,
	CLIB_FFI_VAL_NAME, /* attribute value */
} clib_ffi_val_kind;

#ifdef HAVE_LONG_DOUBLE
typedef long double clib_ffi_double;
#else
typedef double clib_ffi_double;
#endif

typedef struct _clib_ffi_val {
	clib_ffi_val_kind   kind;
	union {
		uint64_t        u64;
		int64_t         i64;
		clib_ffi_double d;
		signed char     ch;
		struct {
			const char *str;
			size_t      len;
		};
	};
} clib_ffi_val;

bool clib_ffi_parse_decl(b_vm *vm, const char *str, size_t len);
bool clib_ffi_parse_type(b_vm *vm, const char *str, size_t len, clib_ffi_dcl *dcl);

/* parser callbacks */
void CLIB_NORETURN clib_ffi_parser_error(const char *msg, ...);
bool clib_ffi_is_typedef_name(const char *name, size_t name_len);
void clib_ffi_resolve_typedef(const char *name, size_t name_len, clib_ffi_dcl *dcl);
void clib_ffi_resolve_const(const char *name, size_t name_len, clib_ffi_val *val);
void clib_ffi_declare_tag(const char *name, size_t name_len, clib_ffi_dcl *dcl, bool incomplete);
void clib_ffi_make_enum_type(clib_ffi_dcl *dcl);
void clib_ffi_add_enum_val(clib_ffi_dcl *enum_dcl, const char *name, size_t name_len, clib_ffi_val *val, int64_t *min, int64_t *max, int64_t *last);
void clib_ffi_make_struct_type(clib_ffi_dcl *dcl);
void clib_ffi_add_field(clib_ffi_dcl *struct_dcl, const char *name, size_t name_len, clib_ffi_dcl *field_dcl);
void clib_ffi_add_anonymous_field(clib_ffi_dcl *struct_dcl, clib_ffi_dcl *field_dcl);
void clib_ffi_add_bit_field(clib_ffi_dcl *struct_dcl, const char *name, size_t name_len, clib_ffi_dcl *field_dcl, clib_ffi_val *bits);
void clib_ffi_adjust_struct_size(clib_ffi_dcl *dcl);
void clib_ffi_make_pointer_type(clib_ffi_dcl *dcl);
void clib_ffi_make_array_type(clib_ffi_dcl *dcl, clib_ffi_val *len);
void clib_ffi_make_func_type(clib_ffi_dcl *dcl, b_obj_dict *args, clib_ffi_dcl *nested_dcl);
void clib_ffi_add_arg(b_obj_dict **args, const char *name, size_t name_len, clib_ffi_dcl *arg_dcl);
void clib_ffi_declare(const char *name, size_t name_len, clib_ffi_dcl *dcl);
void clib_ffi_add_attribute(clib_ffi_dcl *dcl, const char *name, size_t name_len);
void clib_ffi_add_attribute_value(clib_ffi_dcl *dcl, const char *name, size_t name_len, int n, clib_ffi_val *val);
void clib_ffi_add_msvc_attribute_value(clib_ffi_dcl *dcl, const char *name, size_t name_len, clib_ffi_val *val);
void clib_ffi_set_abi(clib_ffi_dcl *dcl, uint16_t abi);
void clib_ffi_nested_declaration(clib_ffi_dcl *dcl, clib_ffi_dcl *nested_dcl);
void clib_ffi_align_as_type(clib_ffi_dcl *dcl, clib_ffi_dcl *align_dcl);
void clib_ffi_align_as_val(clib_ffi_dcl *dcl, clib_ffi_val *align_val);
void clib_ffi_validate_type_name(clib_ffi_dcl *dcl);

void clib_ffi_expr_conditional(clib_ffi_val *val, clib_ffi_val *op2, clib_ffi_val *op3);
void clib_ffi_expr_bool_or(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_bool_and(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_bw_or(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_bw_xor(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_bw_and(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_is_equal(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_is_not_equal(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_is_less(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_is_greater(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_is_less_or_equal(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_is_greater_or_equal(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_shift_left(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_shift_right(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_add(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_sub(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_mul(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_div(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_mod(clib_ffi_val *val, clib_ffi_val *op2);
void clib_ffi_expr_cast(clib_ffi_val *val, clib_ffi_dcl *dcl);
void clib_ffi_expr_plus(clib_ffi_val *val);
void clib_ffi_expr_neg(clib_ffi_val *val);
void clib_ffi_expr_bw_not(clib_ffi_val *val);
void clib_ffi_expr_bool_not(clib_ffi_val *val);
void clib_ffi_expr_sizeof_val(clib_ffi_val *val);
void clib_ffi_expr_sizeof_type(clib_ffi_val *val, clib_ffi_dcl *dcl);
void clib_ffi_expr_alignof_val(clib_ffi_val *val);
void clib_ffi_expr_alignof_type(clib_ffi_val *val, clib_ffi_dcl *dcl);

static clib_always_inline void clib_ffi_val_error(clib_ffi_val *val) {
	val->kind = CLIB_FFI_VAL_ERROR;
}

void clib_ffi_val_number(clib_ffi_val *val, int base, const char *str, size_t str_len);
void clib_ffi_val_float_number(clib_ffi_val *val, const char *str, size_t str_len);
void clib_ffi_val_string(clib_ffi_val *val, const char *str, size_t str_len);
void clib_ffi_val_character(clib_ffi_val *val, const char *str, size_t str_len);

#endif	/* PHP_FFI_H */
