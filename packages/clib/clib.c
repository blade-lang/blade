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
   | Author: Dmitry Stogov <dmitry@zend.com>                              |
   +----------------------------------------------------------------------+
*/

#include <blade.h>
#include "php_ffi.h"
#include <ffi.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef __BIGGEST_ALIGNMENT__
/* XXX need something better, perhaps with regard to SIMD, etc. */
# define __BIGGEST_ALIGNMENT__ sizeof(size_t)
#endif

b_ffi_globals _clib_ffi_global_data;

typedef enum _clib_ffi_tag_kind {
	CLIB_FFI_TAG_ENUM,
	CLIB_FFI_TAG_STRUCT,
	CLIB_FFI_TAG_UNION
} clib_ffi_tag_kind;

static const char *clib_ffi_tag_kind_name[3] = {"enum", "struct", "union"};


typedef struct _clib_ffi_tag {
	clib_ffi_tag_kind      kind;
	clib_ffi_type         *type;
} clib_ffi_tag;

typedef enum _clib_ffi_type_kind {
	CLIB_FFI_TYPE_VOID,
	CLIB_FFI_TYPE_FLOAT,
	CLIB_FFI_TYPE_DOUBLE,
#ifdef HAVE_LONG_DOUBLE
	CLIB_FFI_TYPE_LONGDOUBLE,
#endif
	CLIB_FFI_TYPE_UINT8,
	CLIB_FFI_TYPE_SINT8,
	CLIB_FFI_TYPE_UINT16,
	CLIB_FFI_TYPE_SINT16,
	CLIB_FFI_TYPE_UINT32,
	CLIB_FFI_TYPE_SINT32,
	CLIB_FFI_TYPE_UINT64,
	CLIB_FFI_TYPE_SINT64,
	CLIB_FFI_TYPE_ENUM,
	CLIB_FFI_TYPE_BOOL,
	CLIB_FFI_TYPE_CHAR,
	CLIB_FFI_TYPE_POINTER,
	CLIB_FFI_TYPE_FUNC,
	CLIB_FFI_TYPE_ARRAY,
	CLIB_FFI_TYPE_STRUCT,
} clib_ffi_type_kind;

typedef enum _clib_ffi_flags {
	CLIB_FFI_FLAG_CONST      = (1 << 0),
	CLIB_FFI_FLAG_OWNED      = (1 << 1),
	CLIB_FFI_FLAG_PERSISTENT = (1 << 2),
} clib_ffi_flags;

struct _clib_ffi_type {
	clib_ffi_type_kind     kind;
	size_t                 size;
	uint32_t               align;
	uint32_t               attr;
	union {
		struct {
			b_obj_string        *tag_name;
			clib_ffi_type_kind  kind;
		} enumeration;
		struct {
			clib_ffi_type *type;
			long      length;
		} array;
		struct {
			clib_ffi_type *type;
		} pointer;
		struct {
			b_obj_string   *tag_name;
			b_obj_dict      *fields;
		} record;
		struct {
			clib_ffi_type *ret_type;
			b_obj_list     *args;
			ffi_abi        abi;
		} func;
	};
};

typedef struct _clib_ffi_field {
	size_t                 offset;
	bool              is_const;
	bool              is_nested; /* part of nested anonymous struct */
	uint8_t                first_bit;
	uint8_t                bits;
	clib_ffi_type         *type;
} clib_ffi_field;

typedef enum _clib_ffi_symbol_kind {
	CLIB_FFI_SYM_TYPE,
	CLIB_FFI_SYM_CONST,
	CLIB_FFI_SYM_VAR,
	CLIB_FFI_SYM_FUNC
} clib_ffi_symbol_kind;

typedef struct _clib_ffi_symbol {
	clib_ffi_symbol_kind   kind;
	bool              is_const;
	clib_ffi_type         *type;
	union {
		void *addr;
		int64_t value;
	};
} clib_ffi_symbol;

typedef struct _clib_ffi_scope {
	b_obj_dict             *symbols;
	b_obj_dict             *tags;
} clib_ffi_scope;

typedef struct _clib_ffi {
	b_obj_module           *std;
	void                   *lib;
	b_obj_dict             *symbols;
	b_obj_dict             *tags;
	bool                   persistent;
} clib_ffi;

#define CLIB_FFI_TYPE_OWNED        (1<<0)

#define CLIB_FFI_TYPE(t) \
	((clib_ffi_type*)(((uintptr_t)(t)) & ~CLIB_FFI_TYPE_OWNED))

#define CLIB_FFI_TYPE_IS_OWNED(t) \
	(((uintptr_t)(t)) & CLIB_FFI_TYPE_OWNED)

#define CLIB_FFI_TYPE_MAKE_OWNED(t) \
	((clib_ffi_type*)(((uintptr_t)(t)) | CLIB_FFI_TYPE_OWNED))

#define CLIB_FFI_SIZEOF_ARG \
	MAX(FFI_SIZEOF_ARG, sizeof(double))

typedef struct _clib_ffi_cdata {
	b_obj_module          *std;
	clib_ffi_type         *type;
	void                  *ptr;
	void                  *ptr_holder;
	clib_ffi_flags         flags;
} clib_ffi_cdata;

typedef struct _clib_ffi_ctype {
  b_obj_dict         *std;
	clib_ffi_type         *type;
} clib_ffi_ctype;

/* forward declarations */
static void _clib_ffi_type_dtor(clib_ffi_type *type);
static void clib_ffi_finalize_type(clib_ffi_dcl *dcl);
static bool clib_ffi_is_same_type(clib_ffi_type *type1, clib_ffi_type *type2);
static clib_ffi_type *clib_ffi_remember_type(clib_ffi_type *type);
static char *clib_ffi_parse_directives(const char *filename, char *code_pos, char **scope_name, char **lib, bool preload);
DECLARE_MODULE_METHOD(ffi_trampoline);
static CLIB_COLD void clib_ffi_return_unsupported(clib_ffi_type *type);
static CLIB_COLD void clib_ffi_pass_unsupported(clib_ffi_type *type);
static CLIB_COLD void clib_ffi_assign_incompatible(b_value arg, clib_ffi_type *type);

#if FFI_CLOSURES
static void *clib_ffi_create_callback(clib_ffi_type *type, b_value *value);
#endif

static clib_always_inline void clib_ffi_type_dtor(clib_ffi_type *type) {
	if (UNEXPECTED(CLIB_FFI_TYPE_IS_OWNED(type))) {
		_clib_ffi_type_dtor(type);
		return;
	}
}

static bool clib_ffi_is_compatible_type(clib_ffi_type *dst_type, clib_ffi_type *src_type) {
	while (1) {
		if (dst_type == src_type) {
			return 1;
		} else if (dst_type->kind == src_type->kind) {
			if (dst_type->kind < CLIB_FFI_TYPE_POINTER) {
				return 1;
			} else if (dst_type->kind == CLIB_FFI_TYPE_POINTER) {
				dst_type = CLIB_FFI_TYPE(dst_type->pointer.type);
				src_type = CLIB_FFI_TYPE(src_type->pointer.type);
				if (dst_type->kind == CLIB_FFI_TYPE_VOID ||
				    src_type->kind == CLIB_FFI_TYPE_VOID) {
				    return 1;
				}
			} else if (dst_type->kind == CLIB_FFI_TYPE_ARRAY &&
			           (dst_type->array.length == src_type->array.length ||
			            dst_type->array.length == 0)) {
				dst_type = CLIB_FFI_TYPE(dst_type->array.type);
				src_type = CLIB_FFI_TYPE(src_type->array.type);
			} else {
				break;
			}
		} else if (dst_type->kind == CLIB_FFI_TYPE_POINTER &&
		           src_type->kind == CLIB_FFI_TYPE_ARRAY) {
			dst_type = CLIB_FFI_TYPE(dst_type->pointer.type);
			src_type = CLIB_FFI_TYPE(src_type->array.type);
			if (dst_type->kind == CLIB_FFI_TYPE_VOID) {
			    return 1;
			}
		} else {
			break;
		}
	}
	return 0;
}

static ffi_type* clib_ffi_face_struct_add_fields(ffi_type* t, clib_ffi_type *type, int *i, size_t size) {

  for(int j = 0; j < type->record.fields->names.count; j++) {
    b_value v;
    if(dict_get_entry(type->record.fields, type->record.fields->names.values[j], &v)) {
      clib_ffi_field *field = (clib_ffi_field *)AS_PTR(v)->pointer;
      switch (CLIB_FFI_TYPE(field->type)->kind) {
        case CLIB_FFI_TYPE_FLOAT:
          t->elements[(*i)++] = &ffi_type_float;
          break;
        case CLIB_FFI_TYPE_DOUBLE:
          t->elements[(*i)++] = &ffi_type_double;
          break;
#ifdef HAVE_LONG_DOUBLE
          case CLIB_FFI_TYPE_LONGDOUBLE:
				t->elements[(*i)++] = &ffi_type_longdouble;
				break;
#endif
        case CLIB_FFI_TYPE_SINT8:
        case CLIB_FFI_TYPE_UINT8:
        case CLIB_FFI_TYPE_BOOL:
        case CLIB_FFI_TYPE_CHAR:
          t->elements[(*i)++] = &ffi_type_uint8;
          break;
        case CLIB_FFI_TYPE_SINT16:
        case CLIB_FFI_TYPE_UINT16:
          t->elements[(*i)++] = &ffi_type_uint16;
          break;
        case CLIB_FFI_TYPE_SINT32:
        case CLIB_FFI_TYPE_UINT32:
          t->elements[(*i)++] = &ffi_type_uint32;
          break;
        case CLIB_FFI_TYPE_SINT64:
        case CLIB_FFI_TYPE_UINT64:
          t->elements[(*i)++] = &ffi_type_uint64;
          break;
        case CLIB_FFI_TYPE_POINTER:
          t->elements[(*i)++] = &ffi_type_pointer;
          break;
        case CLIB_FFI_TYPE_STRUCT: {
          clib_ffi_type *field_type = CLIB_FFI_TYPE(field->type);
          /* for unions we use only the first field */
          uint32_t num_fields = !(field_type->attr & CLIB_FFI_ATTR_UNION) ?
                                field_type->record.fields->names.count: 1;

          if (num_fields > 1) {
            size += sizeof(ffi_type*) * (num_fields - 1);
            t = realloc(t, size);
            t->elements = (ffi_type**)(t + 1);
          }
          t = clib_ffi_face_struct_add_fields(t, field_type, i, size);
          break;
        }
        default:
          t->elements[(*i)++] = &ffi_type_void;
          break;
      }
      if (type->attr & CLIB_FFI_ATTR_UNION) {
        /* for unions we use only the first field */
        break;
      }
    }
  }

	return t;
}

static ffi_type *clib_ffi_make_fake_struct_type(b_vm *vm, clib_ffi_type *type) {
	/* for unions we use only the first field */
	uint32_t num_fields = !(type->attr & CLIB_FFI_ATTR_UNION) ?
                        type->record.fields->names.count : 1;
	size_t size = sizeof(ffi_type) + sizeof(ffi_type*) * (num_fields + 1);
	ffi_type *t = N_ALLOCATE(ffi_type, size);
	int i;

	t->size = type->size;
	t->alignment = type->align;
	t->type = FFI_TYPE_STRUCT;
	t->elements = (ffi_type**)(t + 1);
	i = 0;
	t = clib_ffi_face_struct_add_fields(t, type, &i, size);
	t->elements[i] = NULL;
	return t;
}

static ffi_type *clib_ffi_get_type(b_vm *vm, clib_ffi_type *type) {
	clib_ffi_type_kind kind = type->kind;

again:
	switch (kind) {
		case CLIB_FFI_TYPE_FLOAT:
			return &ffi_type_float;
		case CLIB_FFI_TYPE_DOUBLE:
			return &ffi_type_double;
#ifdef HAVE_LONG_DOUBLE
		case CLIB_FFI_TYPE_LONGDOUBLE:
			return &ffi_type_longdouble;
#endif
		case CLIB_FFI_TYPE_UINT8:
			return &ffi_type_uint8;
		case CLIB_FFI_TYPE_SINT8:
			return &ffi_type_sint8;
		case CLIB_FFI_TYPE_UINT16:
			return &ffi_type_uint16;
		case CLIB_FFI_TYPE_SINT16:
			return &ffi_type_sint16;
		case CLIB_FFI_TYPE_UINT32:
			return &ffi_type_uint32;
		case CLIB_FFI_TYPE_SINT32:
			return &ffi_type_sint32;
		case CLIB_FFI_TYPE_UINT64:
			return &ffi_type_uint64;
		case CLIB_FFI_TYPE_SINT64:
			return &ffi_type_sint64;
		case CLIB_FFI_TYPE_POINTER:
			return &ffi_type_pointer;
		case CLIB_FFI_TYPE_VOID:
			return &ffi_type_void;
		case CLIB_FFI_TYPE_BOOL:
			return &ffi_type_uint8;
		case CLIB_FFI_TYPE_CHAR:
			return &ffi_type_sint8;
		case CLIB_FFI_TYPE_ENUM:
			kind = type->enumeration.kind;
			goto again;
		case CLIB_FFI_TYPE_STRUCT:
			return clib_ffi_make_fake_struct_type(vm, type);
		default:
			break;
	}
	return NULL;
}

static clib_never_inline clib_ffi_cdata *clib_ffi_cdata_to_b_value_slow(b_vm *vm, void *ptr, clib_ffi_type *type, clib_ffi_flags flags) {
	clib_ffi_cdata *cdata = ALLOCATE(clib_ffi_cdata, 1);
	cdata->type = type;
	cdata->flags = flags;
	cdata->ptr = ptr;
	return cdata;
}

static clib_never_inline clib_ffi_cdata *clib_ffi_cdata_to_b_value_slow_ptr(b_vm *vm, void *ptr, clib_ffi_type *type, clib_ffi_flags flags) {
	clib_ffi_cdata *cdata = ALLOCATE(clib_ffi_cdata, 1);
	cdata->type = type;
	cdata->flags = flags;
	cdata->ptr = (void*)&cdata->ptr_holder;
	*(void**)cdata->ptr = *(void**)ptr;
	return cdata;
}

static clib_never_inline clib_ffi_cdata *clib_ffi_cdata_to_b_value_slow_ret(b_vm *vm, void *ptr, clib_ffi_type *type, clib_ffi_flags flags) {
	clib_ffi_cdata *cdata = ALLOCATE(clib_ffi_cdata, 1);
	cdata->type = type;
	cdata->flags = flags;
	if (type->kind == CLIB_FFI_TYPE_POINTER) {
		cdata->ptr = (void*)&cdata->ptr_holder;
		*(void**)cdata->ptr = *(void**)ptr;
	} else if (type->kind == CLIB_FFI_TYPE_STRUCT) {
		cdata->ptr = malloc(type->size);
		cdata->flags |= CLIB_FFI_FLAG_OWNED;
		memcpy(cdata->ptr, ptr, type->size);
	} else {
		cdata->ptr = ptr;
	}
	return cdata;
}

static clib_always_inline void clib_ffi_cdata_to_b_value(b_vm *vm, clib_ffi_cdata *cdata, void *ptr, clib_ffi_type *type, b_value *rv, clib_ffi_flags flags, bool is_ret, bool debug_union) {
  clib_ffi_type_kind kind = type->kind;

again:
    switch (kind) {
    case CLIB_FFI_TYPE_FLOAT:
      *rv = NUMBER_VAL(*(float*)ptr);
      return;
    case CLIB_FFI_TYPE_DOUBLE:
      *rv = NUMBER_VAL(*(double*)ptr);
      return;
#ifdef HAVE_LONG_DOUBLE
    case CLIB_FFI_TYPE_LONGDOUBLE:
      *rv = NUMBER_VAL(*(long double*)ptr);
      return;
#endif
    case CLIB_FFI_TYPE_UINT8:
      *rv = NUMBER_VAL(*(uint8_t*)ptr);
      return;
    case CLIB_FFI_TYPE_SINT8:
      *rv = NUMBER_VAL(*(int8_t*)ptr);
      return;
    case CLIB_FFI_TYPE_UINT16:
      *rv = NUMBER_VAL(*(uint16_t*)ptr);
      return;
    case CLIB_FFI_TYPE_SINT16:
      *rv = NUMBER_VAL(*(int16_t*)ptr);
      return;
    case CLIB_FFI_TYPE_UINT32:
      *rv = NUMBER_VAL(*(uint32_t*)ptr);
      return;
    case CLIB_FFI_TYPE_SINT32:
      *rv = NUMBER_VAL(*(int32_t*)ptr);
      return;
    case CLIB_FFI_TYPE_UINT64:
      *rv = NUMBER_VAL(*(uint64_t*)ptr);
      return;
    case CLIB_FFI_TYPE_SINT64:
      *rv = NUMBER_VAL(*(int64_t*)ptr);
      return;
    case CLIB_FFI_TYPE_BOOL:
      *rv = NUMBER_VAL(*(uint8_t*)ptr);
      return;
    case CLIB_FFI_TYPE_CHAR:
      *rv = NUMBER_VAL(*(char*)ptr);
      return;
    case CLIB_FFI_TYPE_ENUM:
      kind = type->enumeration.kind;
      goto again;
    case CLIB_FFI_TYPE_POINTER:
      if (*(void**)ptr == NULL) {
        *rv = NIL_VAL;
        return;
      } else if (debug_union) {
        int l = snprintf(NULL, 0, "%p", *(void**)ptr);
        char *str = ALLOCATE(char, l + 1);
        sprintf(str, "%p", *(void**)ptr);
        str[l] = '\0';
        *rv = STRING_VAL(str);
        return;
      } else if ((type->attr & CLIB_FFI_ATTR_CONST) && CLIB_FFI_TYPE(type->pointer.type)->kind == CLIB_FFI_TYPE_CHAR) {
        *rv = STRING_VAL(*(char**)ptr);
        return;
      }
      if (!cdata) {
        if (is_ret) {
          cdata = clib_ffi_cdata_to_b_value_slow_ret(ptr, type, flags);
        } else {
          cdata = clib_ffi_cdata_to_b_value_slow_ptr(ptr, type, flags);
        }
      }
      *rv = PTR_VAL(&cdata->std);
      return;
    default:
      break;
  }

	if (!cdata) {
		if (is_ret) {
			cdata = clib_ffi_cdata_to_b_value_slow_ret(ptr, type, flags);
		} else {
			cdata = clib_ffi_cdata_to_b_value_slow(ptr, type, flags);
		}
	}
  *rv = PTR_VAL(&cdata->std);
}

static uint64_t clib_ffi_bit_field_read(void *ptr, clib_ffi_field *field) {
	size_t bit = field->first_bit;
	size_t last_bit = bit + field->bits - 1;
	uint8_t *p = (uint8_t *) ptr + bit / 8;
	uint8_t *last_p = (uint8_t *) ptr + last_bit / 8;
	size_t pos = bit % 8;
	size_t insert_pos = 0;
	uint8_t mask;
	uint64_t val = 0;

	/* Bitfield fits into a single byte */
	if (p == last_p) {
		mask = (1U << field->bits) - 1U;
		return (*p >> pos) & mask;
	}

	/* Read partial prefix byte */
	if (pos != 0) {
		size_t num_bits = 8 - pos;
		mask = ((1U << num_bits) - 1U) << pos;
		val = (*p++ >> pos) & mask;
		insert_pos += num_bits;
	}

	/* Read full bytes */
	while (p < last_p) {
		val |= *p++ << insert_pos;
		insert_pos += 8;
	}

	/* Read partial suffix byte */
	if (p == last_p) {
		size_t num_bits = last_bit % 8 + 1;
		mask = (1U << num_bits) - 1U;
		val |= (*p & mask) << insert_pos;
	}

	return val;
}

static void clib_ffi_bit_field_to_b_value(void *ptr, clib_ffi_field *field, b_value *rv) {
	uint64_t val = clib_ffi_bit_field_read(ptr, field);
	if (CLIB_FFI_TYPE(field->type)->kind == CLIB_FFI_TYPE_CHAR
	 || CLIB_FFI_TYPE(field->type)->kind == CLIB_FFI_TYPE_SINT8
	 || CLIB_FFI_TYPE(field->type)->kind == CLIB_FFI_TYPE_SINT16
	 || CLIB_FFI_TYPE(field->type)->kind == CLIB_FFI_TYPE_SINT32
	 || CLIB_FFI_TYPE(field->type)->kind == CLIB_FFI_TYPE_SINT64) {
		/* Sign extend */
		uint64_t shift = 64 - (field->bits % 64);
		if (shift != 0) {
			val = (int64_t)(val << shift) >> shift;
		}
	}
  *rv = val;
}

static void clib_ffi_b_value_to_bit_field(void *ptr, clib_ffi_field *field, b_value value) {
	uint64_t val = (uint64_t)AS_NUMBER(value);
	size_t bit = field->first_bit;
	size_t last_bit = bit + field->bits - 1;
	uint8_t *p = (uint8_t *) ptr + bit / 8;
	uint8_t *last_p = (uint8_t *) ptr + last_bit / 8;
	size_t pos = bit % 8;
	uint8_t mask;

	/* Bitfield fits into a single byte */
	if (p == last_p) {
		mask = ((1U << field->bits) - 1U) << pos;
		*p = (*p & ~mask) | ((val << pos) & mask);
		return;
	}

	/* Write partial prefix byte */
	if (pos != 0) {
		size_t num_bits = 8 - pos;
		mask = ((1U << num_bits) - 1U) << pos;
		*p = (*p & ~mask) | ((val << pos) & mask);
		p++;
		val >>= num_bits;
	}

	/* Write full bytes */
	while (p < last_p) {
		*p++ = val;
		val >>= 8;
	}

	/* Write partial suffix byte */
	if (p == last_p) {
		size_t num_bits = last_bit % 8 + 1;
		mask = (1U << num_bits) - 1U;
		*p = (*p & ~mask) | (val & mask);
	}
}

static clib_always_inline bool clib_ffi_b_value_to_cdata(void *ptr, clib_ffi_type *type, b_value value) {
	clib_ffi_type_kind kind = type->kind;

again:
	switch (kind) {
		case CLIB_FFI_TYPE_FLOAT:
			*(float*)ptr = (float)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_DOUBLE:
			*(double*)ptr = AS_NUMBER(value);
			break;
#ifdef HAVE_LONG_DOUBLE
		case CLIB_FFI_TYPE_LONGDOUBLE:
			*(long double*)ptr = (long double)AS_NUMBER(value);
			break;
#endif
		case CLIB_FFI_TYPE_UINT8:
			*(uint8_t*)ptr = (uint8_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_SINT8:
			*(int8_t*)ptr = (int8_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_UINT16:
			*(uint16_t*)ptr = (uint16_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_SINT16:
			*(int16_t*)ptr = (int16_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_UINT32:
			*(uint32_t*)ptr = (uint32_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_SINT32:
			*(int32_t*)ptr = (int32_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_UINT64:
			*(uint64_t*)ptr = (uint64_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_SINT64:
			*(int64_t*)ptr = (int64_t)AS_NUMBER(value);
			break;
		case CLIB_FFI_TYPE_BOOL:
			*(uint8_t*)ptr = (uint8_t)(AS_BOOL(value) ? 1 : 0);
			break;
		case CLIB_FFI_TYPE_CHAR: {
      b_obj_string *str = AS_STRING(value);
      if (str->length == 1) {
        *(char *) ptr = str->chars[0];
      } else {
        clib_ffi_assign_incompatible(value, type);
        return true;
      }
      break;
    }
		case CLIB_FFI_TYPE_ENUM:
			kind = type->enumeration.kind;
			goto again;
		case CLIB_FFI_TYPE_POINTER:
			if (IS_NIL(value)) {
				*(void**)ptr = NULL;
				break;
			} else if (IS_PTR(value)) {
				clib_ffi_cdata *cdata = (clib_ffi_cdata*)AS_PTR(value)->pointer;

				if (clib_ffi_is_compatible_type(type, CLIB_FFI_TYPE(cdata->type))) {
					if (CLIB_FFI_TYPE(cdata->type)->kind == CLIB_FFI_TYPE_POINTER) {
						*(void**)ptr = *(void**)cdata->ptr;
					} else {
						if (cdata->flags & CLIB_FFI_FLAG_OWNED) {
//							clib_throw_error(clib_ffi_exception_ce, "Attempt to perform assign of owned C pointer");
							return false;
						}
						*(void**)ptr = cdata->ptr;
					}
					return true;
				/* Allow transparent assignment of not-owned CData to compatible pointers */
				} else if (CLIB_FFI_TYPE(cdata->type)->kind != CLIB_FFI_TYPE_POINTER
				 && clib_ffi_is_compatible_type(CLIB_FFI_TYPE(type->pointer.type), CLIB_FFI_TYPE(cdata->type))) {
					if (cdata->flags & CLIB_FFI_FLAG_OWNED) {
//						clib_throw_error(clib_ffi_exception_ce, "Attempt to perform assign pointer to owned C data");
						return false;
					}
					*(void**)ptr = cdata->ptr;
					return true;
				}
#if FFI_CLOSURES
			} else if (CLIB_FFI_TYPE(type->pointer.type)->kind == CLIB_FFI_TYPE_FUNC) {
				void *callback = clib_ffi_create_callback(CLIB_FFI_TYPE(type->pointer.type), value);

				if (callback) {
					*(void**)ptr = callback;
					break;
				} else {
					return false;
				}
#endif
			}
			clib_ffi_assign_incompatible(value, type);
			return false;
		case CLIB_FFI_TYPE_STRUCT:
		case CLIB_FFI_TYPE_ARRAY:
		default:
			if (IS_PTR(value)) {
				clib_ffi_cdata *cdata = (clib_ffi_cdata*)AS_PTR(value)->pointer;
				if (clib_ffi_is_compatible_type(type, CLIB_FFI_TYPE(cdata->type)) &&
				    type->size == CLIB_FFI_TYPE(cdata->type)->size) {
					memcpy(ptr, cdata->ptr, type->size);
					return true;
				}
			}
			clib_ffi_assign_incompatible(value, type);
			return false;
	}
	return true;
}

#if defined(CLIB_WIN32) && (defined(HAVE_FFI_FASTCALL) || defined(HAVE_FFI_STDCALL) || defined(HAVE_FFI_VECTORCALL_PARTIAL))
static size_t clib_ffi_arg_size(clib_ffi_type *type) {
	clib_ffi_type *arg_type;
	size_t arg_size = 0;

  for(int i = 0; i < type->func.args->items.count; i++) {
    size_t n = CLIB_FFI_TYPE(arg_type)->size;
    size_t m = sizeof(size_t);
    arg_size += n > m ? n : m;
  }

	return arg_size;
}
#endif

static clib_always_inline b_obj_string *clib_ffi_mangled_func_name(b_vm *vm, b_obj_string *name, clib_ffi_type *type) {
#ifdef CLIB_WIN32
	switch (type->func.abi) {
# ifdef HAVE_FFI_FASTCALL
		case FFI_FASTCALL:
			return strpprintf(0, "@%s@%zu", ZSTR_VAL(name), clib_ffi_arg_size(type));
# endif
# ifdef HAVE_FFI_STDCALL
		case FFI_STDCALL:
			return strpprintf(0, "_%s@%zu", ZSTR_VAL(name), clib_ffi_arg_size(type));
# endif
# ifdef HAVE_FFI_VECTORCALL_PARTIAL
		case FFI_VECTORCALL_PARTIAL:
			return strpprintf(0, "%s@@%zu", ZSTR_VAL(name), clib_ffi_arg_size(type));
# endif
	}
#endif
	return copy_string(vm, name->chars, name->length);
}

#if FFI_CLOSURES
// TODO: closures are not yet supported.
/*typedef struct _clib_ffi_callback_data {
	clib_fcall_info_cache  fcc;
	clib_ffi_type         *type;
	void                  *code;
	void                  *callback;
	ffi_cif                cif;
	uint32_t               arg_count;
	ffi_type              *ret_type;
	ffi_type              *arg_types[0];
} clib_ffi_callback_data;

static void clib_ffi_callback_hash_dtor(b_value *zv)
{
	clib_ffi_callback_data *callback_data = Z_PTR_P(zv);

	ffi_closure_free(callback_data->callback);
	if (callback_data->fcc.function_handler->common.fn_flags & CLIB_ACC_CLOSURE) {
		OBJ_RELEASE(CLIB_CLOSURE_OBJECT(callback_data->fcc.function_handler));
	}
	for (int i = 0; i < callback_data->arg_count; ++i) {
		if (callback_data->arg_types[i]->type == FFI_TYPE_STRUCT) {
			efree(callback_data->arg_types[i]);
		}
	}
	if (callback_data->ret_type->type == FFI_TYPE_STRUCT) {
		efree(callback_data->ret_type);
	}
	efree(callback_data);
}

static void clib_ffi_callback_trampoline(ffi_cif* cif, void* ret, void** args, void* data)
{
	clib_ffi_callback_data *callback_data = (clib_ffi_callback_data*)data;
	clib_fcall_info fci;
	clib_ffi_type *ret_type;
	b_value retval;
	ALLOCA_FLAG(use_heap)

	fci.size = sizeof(clib_fcall_info);
	ZVAL_UNDEF(&fci.function_name);
	fci.retval = &retval;
	fci.params = do_alloca(sizeof(b_value) *callback_data->arg_count, use_heap);
	fci.object = NULL;
	fci.param_count = callback_data->arg_count;
	fci.named_params = NULL;

	if (callback_data->type->func.args) {
		int n = 0;
		clib_ffi_type *arg_type;

		CLIB_HASH_PACKED_FOREACH_PTR(callback_data->type->func.args, arg_type) {
			arg_type = CLIB_FFI_TYPE(arg_type);
			clib_ffi_cdata_to_b_value(NULL, args[n], arg_type, BP_VAR_R, &fci.params[n], (clib_ffi_flags)(arg_type->attr & CLIB_FFI_ATTR_CONST), 0, 0);
			n++;
		} CLIB_HASH_FOREACH_END();
	}

	ZVAL_UNDEF(&retval);
	if (clib_call_function(&fci, &callback_data->fcc) != SUCCESS) {
		clib_throw_error(clib_ffi_exception_ce, "Cannot call callback");
	}

	if (callback_data->arg_count) {
		int n = 0;

		for (n = 0; n < callback_data->arg_count; n++) {
			b_value_ptr_dtor(&fci.params[n]);
		}
	}
	free_alloca(fci.params, use_heap);

	if (EG(exception)) {
		clib_error(E_ERROR, "Throwing from FFI callbacks is not allowed");
	}

	ret_type = CLIB_FFI_TYPE(callback_data->type->func.ret_type);
	if (ret_type->kind != CLIB_FFI_TYPE_VOID) {
		clib_ffi_b_value_to_cdata(ret, ret_type, &retval);
	}

	b_value_ptr_dtor(&retval);
}

static void *clib_ffi_create_callback(clib_ffi_type *type, b_value *value)
{
	clib_fcall_info_cache fcc;
	char *error = NULL;
	uint32_t arg_count;
	void *code;
	void *callback;
	clib_ffi_callback_data *callback_data;

	if (type->attr & CLIB_FFI_ATTR_VARIADIC) {
		clib_throw_error(clib_ffi_exception_ce, "Variadic function closures are not supported");
		return NULL;
	}

	if (!clib_is_callable_ex(value, NULL, 0, NULL, &fcc, &error)) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign an invalid callback, %s", error);
		return NULL;
	}

	arg_count = type->func.args ? clib_hash_num_elements(type->func.args) : 0;
	if (arg_count < fcc.function_handler->common.required_num_args) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign an invalid callback, insufficient number of arguments");
		return NULL;
	}

	callback = ffi_closure_alloc(sizeof(ffi_closure), &code);
	if (!callback) {
		clib_throw_error(clib_ffi_exception_ce, "Cannot allocate callback");
		return NULL;
	}

	callback_data = emalloc(sizeof(clib_ffi_callback_data) + sizeof(ffi_type*) * arg_count);
	memcpy(&callback_data->fcc, &fcc, sizeof(clib_fcall_info_cache));
	callback_data->type = type;
	callback_data->callback = callback;
	callback_data->code = code;
	callback_data->arg_count = arg_count;

	if (type->func.args) {
		int n = 0;
		clib_ffi_type *arg_type;

		CLIB_HASH_PACKED_FOREACH_PTR(type->func.args, arg_type) {
			arg_type = CLIB_FFI_TYPE(arg_type);
			callback_data->arg_types[n] = clib_ffi_get_type(arg_type);
			if (!callback_data->arg_types[n]) {
				clib_ffi_pass_unsupported(arg_type);
				for (int i = 0; i < n; ++i) {
					if (callback_data->arg_types[i]->type == FFI_TYPE_STRUCT) {
						efree(callback_data->arg_types[i]);
					}
				}
				efree(callback_data);
				ffi_closure_free(callback);
				return NULL;
			}
			n++;
		} CLIB_HASH_FOREACH_END();
	}
	callback_data->ret_type = clib_ffi_get_type(CLIB_FFI_TYPE(type->func.ret_type));
	if (!callback_data->ret_type) {
		clib_ffi_return_unsupported(type->func.ret_type);
		for (int i = 0; i < callback_data->arg_count; ++i) {
			if (callback_data->arg_types[i]->type == FFI_TYPE_STRUCT) {
				efree(callback_data->arg_types[i]);
			}
		}
		efree(callback_data);
		ffi_closure_free(callback);
		return NULL;
	}

	if (ffi_prep_cif(&callback_data->cif, type->func.abi, callback_data->arg_count, callback_data->ret_type, callback_data->arg_types) != FFI_OK) {
		clib_throw_error(clib_ffi_exception_ce, "Cannot prepare callback CIF");
		goto free_on_failure;
	}

	if (ffi_prep_closure_loc(callback, &callback_data->cif, clib_ffi_callback_trampoline, callback_data, code) != FFI_OK) {
		clib_throw_error(clib_ffi_exception_ce, "Cannot prepare callback");
free_on_failure: ;
		for (int i = 0; i < callback_data->arg_count; ++i) {
			if (callback_data->arg_types[i]->type == FFI_TYPE_STRUCT) {
				efree(callback_data->arg_types[i]);
			}
		}
		if (callback_data->ret_type->type == FFI_TYPE_STRUCT) {
			efree(callback_data->ret_type);
		}
		efree(callback_data);
		ffi_closure_free(callback);
		return NULL;
	}

	if (!FFI_G(callbacks)) {
		FFI_G(callbacks) = emalloc(sizeof(b_obj_dict));
		clib_hash_init(FFI_G(callbacks), 0, NULL, clib_ffi_callback_hash_dtor, 0);
	}
	clib_hash_next_index_insert_ptr(FFI_G(callbacks), callback_data);

	if (fcc.function_handler->common.fn_flags & CLIB_ACC_CLOSURE) {
		GC_ADDREF(CLIB_CLOSURE_OBJECT(fcc.function_handler));
	}

	return code;
}*/
#endif

static b_value clib_ffi_cdata_get(b_vm *vm, clib_ffi_cdata *cdata, b_obj_string *member, int read_type, void **cache_slot, b_value rv) {
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);

#if 0
	if (UNEXPECTED(!cdata->ptr)) {
//		clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
		return NIL_VAL;
	}
#endif

	if (UNEXPECTED(member->length == 5 && memcmp(member->chars, "cdata", member->length) == 0)) {
//		clib_throw_error(clib_ffi_exception_ce, "Only 'cdata' property may be read");
		return NIL_VAL;
	}

	clib_ffi_cdata_to_b_value(vm, cdata, cdata->ptr, type, &rv, 0, 0, 0);
	return rv;
}

static b_value *clib_ffi_cdata_set(clib_ffi_cdata *cdata, b_obj_string *member, b_value *value, void **cache_slot) {
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);

#if 0
  if (UNEXPECTED(!cdata->ptr)) {
//		clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
		return NIL_VAL;
	}
#endif

  if (UNEXPECTED(member->length == 5 && memcmp(member->chars, "cdata", member->length) == 0)) {
//		clib_throw_error(clib_ffi_exception_ce, "Only 'cdata' property may be read");
    return NIL_VAL;
  }

	clib_ffi_b_value_to_cdata(cdata->ptr, type, value);

	return value;
}

static bool clib_ffi_cdata_cast_object(b_vm *vm, clib_ffi_cdata *cdata, b_value *writeobj) {
	if (IS_STRING(*writeobj)) {
		clib_ffi_type  *ctype = CLIB_FFI_TYPE(cdata->type);
		void           *ptr = cdata->ptr;
		clib_ffi_type_kind kind = ctype->kind;

again:
	    switch (kind) {
			case CLIB_FFI_TYPE_FLOAT:
        *writeobj = NUMBER_VAL(*(float*)ptr);
				break;
			case CLIB_FFI_TYPE_DOUBLE:
        *writeobj = NUMBER_VAL(*(double*)ptr);
				break;
#ifdef HAVE_LONG_DOUBLE
			case CLIB_FFI_TYPE_LONGDOUBLE:
				*writeobj = NUMBER_VAL(*(long double*)ptr);
				break;
#endif
			case CLIB_FFI_TYPE_UINT8:
        *writeobj = NUMBER_VAL(*(uint8_t*)ptr);
				break;
			case CLIB_FFI_TYPE_SINT8:
        *writeobj = NUMBER_VAL(*(int8_t*)ptr);
				break;
			case CLIB_FFI_TYPE_UINT16:
        *writeobj = NUMBER_VAL(*(uint16_t*)ptr);
				break;
			case CLIB_FFI_TYPE_SINT16:
        *writeobj = NUMBER_VAL(*(int16_t*)ptr);
				break;
			case CLIB_FFI_TYPE_UINT32:
        *writeobj = NUMBER_VAL(*(uint32_t*)ptr);
				break;
			case CLIB_FFI_TYPE_SINT32:
        *writeobj = NUMBER_VAL(*(int32_t*)ptr);
				break;
			case CLIB_FFI_TYPE_UINT64:
        *writeobj = NUMBER_VAL(*(uint64_t*)ptr);
				break;
			case CLIB_FFI_TYPE_SINT64:
        *writeobj = NUMBER_VAL(*(int64_t*)ptr);
				break;
			case CLIB_FFI_TYPE_BOOL:
        *writeobj = BOOL_VAL(*(uint8_t*)ptr);
				break;
			case CLIB_FFI_TYPE_CHAR: {
        char c = *(char *) ptr;
        *writeobj = OBJ_VAL(copy_string(vm, &c, 1));
        return true;
      }
			case CLIB_FFI_TYPE_ENUM:
				kind = ctype->enumeration.kind;
				goto again;
			case CLIB_FFI_TYPE_POINTER:
				if (*(void**)ptr == NULL) {
					*writeobj = NIL_VAL;
					break;
				} else if ((ctype->attr & CLIB_FFI_ATTR_CONST) && CLIB_FFI_TYPE(ctype->pointer.type)->kind == CLIB_FFI_TYPE_CHAR) {
					*writeobj = STRING_VAL(*(char**)ptr);
					return true;
				}
				return false;
			default:
				return false;
		}
    char *str = value_to_string(vm, *writeobj);
		*writeobj = STRING_VAL(str);
		return true;
	} else if (IS_BOOL(*writeobj)) {
		*writeobj = TRUE_VAL;
		return true;
	}

	return false;
}

static b_value clib_ffi_cdata_read_field(b_vm *vm, clib_ffi_cdata *cdata, b_obj_string *field_name, void **cache_slot, b_value *rv) {
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	void           *ptr = cdata->ptr;
	clib_ffi_field *field;

	if (cache_slot && *cache_slot == type) {
		field = *(cache_slot + 1);
	} else {
		if (UNEXPECTED(type->kind != CLIB_FFI_TYPE_STRUCT)) {
			if (type->kind == CLIB_FFI_TYPE_POINTER) {
				/* transparently dereference the pointer */
				if (UNEXPECTED(!ptr)) {
//					clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
					return NIL_VAL;
				}
				ptr = (void*)(*(char**)ptr);
				if (UNEXPECTED(!ptr)) {
//					clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
          return NIL_VAL;
				}
				type = CLIB_FFI_TYPE(type->pointer.type);
			}
			if (UNEXPECTED(type->kind != CLIB_FFI_TYPE_STRUCT)) {
//				clib_throw_error(clib_ffi_exception_ce, "Attempt to read field '%s' of non C struct/union", ZSTR_VAL(field_name));
				return NIL_VAL;
			}
		}

    b_value val;
    dict_get_entry(type->record.fields, OBJ_VAL(field_name), &val);
    if(!val || !IS_PTR(val)) return NIL_VAL;
    field = AS_PTR(val)->pointer;
		if (UNEXPECTED(!field)) {
//			clib_throw_error(clib_ffi_exception_ce, "Attempt to read undefined field '%s' of C struct/union", ZSTR_VAL(field_name));
			return NIL_VAL;
		}

		if (cache_slot) {
			*cache_slot = type;
			*(cache_slot + 1) = field;
		}
	}

#if 0
	if (UNEXPECTED(!ptr)) {
//		clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
		return NIL_VAL;
	}
#endif

	if (EXPECTED(!field->bits)) {
		clib_ffi_type *field_type = field->type;

		if (CLIB_FFI_TYPE_IS_OWNED(field_type)) {
			field_type = CLIB_FFI_TYPE(field_type);
			if (!(field_type->attr & CLIB_FFI_ATTR_STORED)
			 && field_type->kind == CLIB_FFI_TYPE_POINTER) {
				field->type = field_type = clib_ffi_remember_type(field_type);
			}
		}
		ptr = (void*)(((char*)ptr) + field->offset);
		clib_ffi_cdata_to_b_value(vm, NULL, ptr, field_type, rv, (cdata->flags & CLIB_FFI_FLAG_CONST) | (clib_ffi_flags)field->is_const, 0, 0);
	} else {
		clib_ffi_bit_field_to_b_value(ptr, field, rv);
	}

	return *rv;
}

static b_value *clib_ffi_cdata_write_field(clib_ffi_cdata *cdata, b_obj_string *field_name, b_value *value, void **cache_slot) {
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	void           *ptr = cdata->ptr;
	clib_ffi_field *field;

	if (cache_slot && *cache_slot == type) {
		field = *(cache_slot + 1);
	} else {
		if (UNEXPECTED(type->kind != CLIB_FFI_TYPE_STRUCT)) {
			if (type->kind == CLIB_FFI_TYPE_POINTER) {
				/* transparently dereference the pointer */
				if (UNEXPECTED(!ptr)) {
//					clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
					return value;
				}
				ptr = (void*)(*(char**)ptr);
				if (UNEXPECTED(!ptr)) {
//					clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
					return value;
				}
				type = CLIB_FFI_TYPE(type->pointer.type);
			}
			if (UNEXPECTED(type->kind != CLIB_FFI_TYPE_STRUCT)) {
//				clib_throw_error(clib_ffi_exception_ce, "Attempt to assign field '%s' of non C struct/union", ZSTR_VAL(field_name));
				return value;
			}
		}

    b_value val;
    dict_get_entry(type->record.fields, OBJ_VAL(field_name), &val);
    if(!val || !IS_PTR(val)) return value;
    field = AS_PTR(val)->pointer;
		if (UNEXPECTED(!field)) {
//			clib_throw_error(clib_ffi_exception_ce, "Attempt to assign undefined field '%s' of C struct/union", ZSTR_VAL(field_name));
			return value;
		}

		if (cache_slot) {
			*cache_slot = type;
			*(cache_slot + 1) = field;
		}
	}

#if 0
	if (UNEXPECTED(!ptr)) {
//		clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
		return value;
	}
#endif

	/*if (UNEXPECTED(cdata->flags & CLIB_FFI_FLAG_CONST)) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign read-only location");
		return value;
	} else if (UNEXPECTED(field->is_const)) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign read-only field '%s'", ZSTR_VAL(field_name));
		return value;
	}*/
  if (UNEXPECTED(cdata->flags & CLIB_FFI_FLAG_CONST) || UNEXPECTED(field->is_const)) {
    return value;
  }

	if (EXPECTED(!field->bits)) {
		ptr = (void*)(((char*)ptr) + field->offset);
		clib_ffi_b_value_to_cdata(ptr, CLIB_FFI_TYPE(field->type), *value);
	} else {
		clib_ffi_b_value_to_bit_field(ptr, field, *value);
	}
	return value;
}

static b_value clib_ffi_cdata_read_dim(b_vm *vm, clib_ffi_cdata *cdata, b_value *offset, b_value *rv) {
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	long       dim = (long)AS_NUMBER(*offset);
	clib_ffi_type  *dim_type;
	void           *ptr;
	clib_ffi_flags  is_const;

	if (EXPECTED(type->kind == CLIB_FFI_TYPE_ARRAY)) {
		if (UNEXPECTED((unsigned long)(dim) >= (unsigned long)type->array.length)
		 && (UNEXPECTED(dim < 0) || UNEXPECTED(type->array.length != 0))) {
//			clib_throw_error(clib_ffi_exception_ce, "C array index out of bounds");
			return NIL_VAL;
		}

		is_const = (cdata->flags & CLIB_FFI_FLAG_CONST) | (clib_ffi_flags)(type->attr & CLIB_FFI_ATTR_CONST);

		dim_type = type->array.type;
		if (CLIB_FFI_TYPE_IS_OWNED(dim_type)) {
			dim_type = CLIB_FFI_TYPE(dim_type);
			if (!(dim_type->attr & CLIB_FFI_ATTR_STORED)
			 && dim_type->kind == CLIB_FFI_TYPE_POINTER) {
				type->array.type = dim_type = clib_ffi_remember_type(dim_type);
			}
		}
#if 0
		if (UNEXPECTED(!cdata->ptr)) {
//			clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
			return NIL_VAL;
		}
#endif
		ptr = (void*)(((char*)cdata->ptr) + dim_type->size * dim);
	} else if (EXPECTED(type->kind == CLIB_FFI_TYPE_POINTER)) {
		is_const = (cdata->flags & CLIB_FFI_FLAG_CONST) | (clib_ffi_flags)(type->attr & CLIB_FFI_ATTR_CONST);
		dim_type = type->pointer.type;
		if (CLIB_FFI_TYPE_IS_OWNED(dim_type)) {
			dim_type = CLIB_FFI_TYPE(dim_type);
			if (!(dim_type->attr & CLIB_FFI_ATTR_STORED)
			 && dim_type->kind == CLIB_FFI_TYPE_POINTER) {
				type->pointer.type = dim_type = clib_ffi_remember_type(dim_type);
			}
		}
		if (UNEXPECTED(!cdata->ptr)) {
//			clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
      return &NIL_VAL;
		}
		ptr = (void*)((*(char**)cdata->ptr) + dim_type->size * dim);
	} else {
//		clib_throw_error(clib_ffi_exception_ce, "Attempt to read element of non C array");
    return NIL_VAL;
	}

	clib_ffi_cdata_to_b_value(vm, NULL, ptr, dim_type, rv, is_const, 0, 0);
	return rv;
}

static void clib_ffi_cdata_write_dim(clib_object *obj, b_value *offset, b_value *value) /* {{{ */
{
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)obj;
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	long       dim;
	void           *ptr;
	clib_ffi_flags  is_const;

	if (offset == NULL) {
		clib_throw_error(clib_ffi_exception_ce, "Cannot add next element to object of type FFI\\CData");
		return;
	}

	dim = b_value_get_long(offset);
	if (EXPECTED(type->kind == CLIB_FFI_TYPE_ARRAY)) {
		if (UNEXPECTED((clib_ulong)(dim) >= (clib_ulong)type->array.length)
		 && (UNEXPECTED(dim < 0) || UNEXPECTED(type->array.length != 0))) {
			clib_throw_error(clib_ffi_exception_ce, "C array index out of bounds");
			return;
		}

		is_const = (cdata->flags & CLIB_FFI_FLAG_CONST) | (clib_ffi_flags)(type->attr & CLIB_FFI_ATTR_CONST);
		type = CLIB_FFI_TYPE(type->array.type);
#if 0
		if (UNEXPECTED(!cdata->ptr)) {
			clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
			return;
		}
#endif
		ptr = (void*)(((char*)cdata->ptr) + type->size * dim);
	} else if (EXPECTED(type->kind == CLIB_FFI_TYPE_POINTER)) {
		is_const = (cdata->flags & CLIB_FFI_FLAG_CONST) | (clib_ffi_flags)(type->attr & CLIB_FFI_ATTR_CONST);
		type = CLIB_FFI_TYPE(type->pointer.type);
		if (UNEXPECTED(!cdata->ptr)) {
			clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
			return;
		}
		ptr = (void*)((*(char**)cdata->ptr) + type->size * dim);
	} else {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign element of non C array");
		return;
	}

	if (UNEXPECTED(is_const)) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign read-only location");
		return;
	}

	clib_ffi_b_value_to_cdata(ptr, type, value);
}
/* }}} */

#define MAX_TYPE_NAME_LEN 256

typedef struct _clib_ffi_ctype_name_buf {
	char *start;
	char *end;
	char buf[MAX_TYPE_NAME_LEN];
} clib_ffi_ctype_name_buf;

static bool clib_ffi_ctype_name_prepend(clib_ffi_ctype_name_buf *buf, const char *str, size_t len) /* {{{ */
{
	buf->start -= len;
	if (buf->start < buf->buf) {
		return 0;
	}
	memcpy(buf->start, str, len);
	return 1;
}
/* }}} */

static bool clib_ffi_ctype_name_append(clib_ffi_ctype_name_buf *buf, const char *str, size_t len) /* {{{ */
{
	if (buf->end + len > buf->buf + MAX_TYPE_NAME_LEN) {
		return 0;
	}
	memcpy(buf->end, str, len);
	buf->end += len;
	return 1;
}
/* }}} */

static bool clib_ffi_ctype_name(clib_ffi_ctype_name_buf *buf, const clib_ffi_type *type) /* {{{ */
{
	const char *name = NULL;
	bool is_ptr = 0;

	while (1) {
		switch (type->kind) {
			case CLIB_FFI_TYPE_VOID:
				name = "void";
				break;
			case CLIB_FFI_TYPE_FLOAT:
				name = "float";
				break;
			case CLIB_FFI_TYPE_DOUBLE:
				name = "double";
				break;
#ifdef HAVE_LONG_DOUBLE
			case CLIB_FFI_TYPE_LONGDOUBLE:
				name = "long double";
				break;
#endif
			case CLIB_FFI_TYPE_UINT8:
				name = "uint8_t";
				break;
			case CLIB_FFI_TYPE_SINT8:
				name = "int8_t";
				break;
			case CLIB_FFI_TYPE_UINT16:
				name = "uint16_t";
				break;
			case CLIB_FFI_TYPE_SINT16:
				name = "int16_t";
				break;
			case CLIB_FFI_TYPE_UINT32:
				name = "uint32_t";
				break;
			case CLIB_FFI_TYPE_SINT32:
				name = "int32_t";
				break;
			case CLIB_FFI_TYPE_UINT64:
				name = "uint64_t";
				break;
			case CLIB_FFI_TYPE_SINT64:
				name = "int64_t";
				break;
			case CLIB_FFI_TYPE_ENUM:
				if (type->enumeration.tag_name) {
					clib_ffi_ctype_name_prepend(buf, ZSTR_VAL(type->enumeration.tag_name), ZSTR_LEN(type->enumeration.tag_name));
				} else {
					clib_ffi_ctype_name_prepend(buf, "<anonymous>", sizeof("<anonymous>")-1);
				}
				name = "enum ";
				break;
			case CLIB_FFI_TYPE_BOOL:
				name = "bool";
				break;
			case CLIB_FFI_TYPE_CHAR:
				name = "char";
				break;
			case CLIB_FFI_TYPE_POINTER:
				if (!clib_ffi_ctype_name_prepend(buf, "*", 1)) {
					return 0;
				}
				is_ptr = 1;
				type = CLIB_FFI_TYPE(type->pointer.type);
				break;
			case CLIB_FFI_TYPE_FUNC:
				if (is_ptr) {
					is_ptr = 0;
					if (!clib_ffi_ctype_name_prepend(buf, "(", 1)
					 || !clib_ffi_ctype_name_append(buf, ")", 1)) {
						return 0;
					}
				}
				if (!clib_ffi_ctype_name_append(buf, "(", 1)
				 || !clib_ffi_ctype_name_append(buf, ")", 1)) {
					return 0;
				}
				type = CLIB_FFI_TYPE(type->func.ret_type);
				break;
			case CLIB_FFI_TYPE_ARRAY:
				if (is_ptr) {
					is_ptr = 0;
					if (!clib_ffi_ctype_name_prepend(buf, "(", 1)
					 || !clib_ffi_ctype_name_append(buf, ")", 1)) {
						return 0;
					}
				}
				if (!clib_ffi_ctype_name_append(buf, "[", 1)) {
					return 0;
				}
				if (type->attr & CLIB_FFI_ATTR_VLA) {
					if (!clib_ffi_ctype_name_append(buf, "*", 1)) {
						return 0;
					}
				} else if (!(type->attr & CLIB_FFI_ATTR_INCOMPLETE_ARRAY)) {
					char str[MAX_LENGTH_OF_LONG + 1];
					char *s = clib_print_long_to_buf(str + sizeof(str) - 1, type->array.length);

					if (!clib_ffi_ctype_name_append(buf, s, strlen(s))) {
						return 0;
					}
				}
				if (!clib_ffi_ctype_name_append(buf, "]", 1)) {
					return 0;
				}
				type = CLIB_FFI_TYPE(type->array.type);
				break;
			case CLIB_FFI_TYPE_STRUCT:
				if (type->attr & CLIB_FFI_ATTR_UNION) {
					if (type->record.tag_name) {
						clib_ffi_ctype_name_prepend(buf, ZSTR_VAL(type->record.tag_name), ZSTR_LEN(type->record.tag_name));
					} else {
						clib_ffi_ctype_name_prepend(buf, "<anonymous>", sizeof("<anonymous>")-1);
					}
					name = "union ";
				} else {
					if (type->record.tag_name) {
						clib_ffi_ctype_name_prepend(buf, ZSTR_VAL(type->record.tag_name), ZSTR_LEN(type->record.tag_name));
					} else {
						clib_ffi_ctype_name_prepend(buf, "<anonymous>", sizeof("<anonymous>")-1);
					}
					name = "struct ";
				}
				break;
			default:
				CLIB_UNREACHABLE();
		}
		if (name) {
			break;
		}
	}

//	if (buf->start != buf->end && *buf->start != '[') {
//		if (!clib_ffi_ctype_name_prepend(buf, " ", 1)) {
//			return 0;
//		}
//	}
	return clib_ffi_ctype_name_prepend(buf, name, strlen(name));
}
/* }}} */

static CLIB_COLD void clib_ffi_return_unsupported(clib_ffi_type *type) /* {{{ */
{
	type = CLIB_FFI_TYPE(type);
	if (type->kind == CLIB_FFI_TYPE_STRUCT) {
		clib_throw_error(clib_ffi_exception_ce, "FFI return struct/union is not implemented");
	} else if (type->kind == CLIB_FFI_TYPE_ARRAY) {
		clib_throw_error(clib_ffi_exception_ce, "FFI return array is not implemented");
	} else {
		clib_throw_error(clib_ffi_exception_ce, "FFI internal error. Unsupported return type");
	}
}
/* }}} */

static CLIB_COLD void clib_ffi_pass_unsupported(clib_ffi_type *type) /* {{{ */
{
	type = CLIB_FFI_TYPE(type);
	if (type->kind == CLIB_FFI_TYPE_STRUCT) {
		clib_throw_error(clib_ffi_exception_ce, "FFI passing struct/union is not implemented");
	} else if (type->kind == CLIB_FFI_TYPE_ARRAY) {
		clib_throw_error(clib_ffi_exception_ce, "FFI passing array is not implemented");
	} else {
		clib_throw_error(clib_ffi_exception_ce, "FFI internal error. Unsupported parameter type");
	}
}
/* }}} */

static CLIB_COLD void clib_ffi_pass_incompatible(b_value *arg, clib_ffi_type *type, uint32_t n, clib_execute_data *execute_data) /* {{{ */
{
	clib_ffi_ctype_name_buf buf1, buf2;

	buf1.start = buf1.end = buf1.buf + ((MAX_TYPE_NAME_LEN * 3) / 4);
	if (!clib_ffi_ctype_name(&buf1, type)) {
		clib_throw_error(clib_ffi_exception_ce, "Passing incompatible argument %d of C function '%s'", n + 1, ZSTR_VAL(EX(func)->internal_function.function_name));
	} else {
		*buf1.end = 0;
		if (Z_TYPE_P(arg) == IS_OBJECT && Z_OBJCE_P(arg) == clib_ffi_cdata_ce) {
			clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(arg);

			type = CLIB_FFI_TYPE(cdata->type);
			buf2.start = buf2.end = buf2.buf + ((MAX_TYPE_NAME_LEN * 3) / 4);
			if (!clib_ffi_ctype_name(&buf2, type)) {
				clib_throw_error(clib_ffi_exception_ce, "Passing incompatible argument %d of C function '%s', expecting '%s'", n + 1, ZSTR_VAL(EX(func)->internal_function.function_name), buf1.start);
			} else {
				*buf2.end = 0;
				clib_throw_error(clib_ffi_exception_ce, "Passing incompatible argument %d of C function '%s', expecting '%s', found '%s'", n + 1, ZSTR_VAL(EX(func)->internal_function.function_name), buf1.start, buf2.start);
			}
		} else {
			clib_throw_error(clib_ffi_exception_ce, "Passing incompatible argument %d of C function '%s', expecting '%s', found PHP '%s'", n + 1, ZSTR_VAL(EX(func)->internal_function.function_name), buf1.start, clib_b_value_type_name(arg));
		}
	}
}
/* }}} */

static CLIB_COLD void clib_ffi_assign_incompatible(b_value arg, clib_ffi_type *type) /* {{{ */
{
	clib_ffi_ctype_name_buf buf1, buf2;

	buf1.start = buf1.end = buf1.buf + ((MAX_TYPE_NAME_LEN * 3) / 4);
	if (!clib_ffi_ctype_name(&buf1, type)) {
		clib_throw_error(clib_ffi_exception_ce, "Incompatible types when assigning");
	} else {
		*buf1.end = 0;
		if (Z_TYPE_P(arg) == IS_OBJECT && Z_OBJCE_P(arg) == clib_ffi_cdata_ce) {
			clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(arg);

			type = CLIB_FFI_TYPE(cdata->type);
			buf2.start = buf2.end = buf2.buf + ((MAX_TYPE_NAME_LEN * 3) / 4);
			if (!clib_ffi_ctype_name(&buf2, type)) {
				clib_throw_error(clib_ffi_exception_ce, "Incompatible types when assigning to type '%s'", buf1.start);
			} else {
				*buf2.end = 0;
				clib_throw_error(clib_ffi_exception_ce, "Incompatible types when assigning to type '%s' from type '%s'", buf1.start, buf2.start);
			}
		} else {
			clib_throw_error(clib_ffi_exception_ce, "Incompatible types when assigning to type '%s' from PHP '%s'", buf1.start, clib_b_value_type_name(arg));
		}
	}
}
/* }}} */

static b_obj_string *clib_ffi_get_class_name(b_obj_string *prefix, const clib_ffi_type *type) /* {{{ */
{
	clib_ffi_ctype_name_buf buf;

	buf.start = buf.end = buf.buf + ((MAX_TYPE_NAME_LEN * 3) / 4);
	if (!clib_ffi_ctype_name(&buf, type)) {
		return b_obj_string_copy(prefix);
	} else {
		return b_obj_string_concat3(
			ZSTR_VAL(prefix), ZSTR_LEN(prefix), ":", 1, buf.start, buf.end - buf.start);
	}
}
/* }}} */

static b_obj_string *clib_ffi_cdata_get_class_name(const clib_object *zobj) /* {{{ */
{
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)zobj;

	return clib_ffi_get_class_name(zobj->ce->name, CLIB_FFI_TYPE(cdata->type));
}
/* }}} */

static int clib_ffi_cdata_compare_objects(b_value *o1, b_value *o2) /* {{{ */
{
	if (Z_TYPE_P(o1) == IS_OBJECT && Z_OBJCE_P(o1) == clib_ffi_cdata_ce &&
	    Z_TYPE_P(o2) == IS_OBJECT && Z_OBJCE_P(o2) == clib_ffi_cdata_ce) {
		clib_ffi_cdata *cdata1 = (clib_ffi_cdata*)Z_OBJ_P(o1);
		clib_ffi_cdata *cdata2 = (clib_ffi_cdata*)Z_OBJ_P(o2);
		clib_ffi_type *type1 = CLIB_FFI_TYPE(cdata1->type);
		clib_ffi_type *type2 = CLIB_FFI_TYPE(cdata2->type);

		if (type1->kind == CLIB_FFI_TYPE_POINTER && type2->kind == CLIB_FFI_TYPE_POINTER) {
			void *ptr1 = *(void**)cdata1->ptr;
			void *ptr2 = *(void**)cdata2->ptr;

			if (!ptr1 || !ptr2) {
				clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
				return 0;
			}
			return ptr1 == ptr2 ? 0 : (ptr1 < ptr2 ? -1 : 1);
		}
	}
	clib_throw_error(clib_ffi_exception_ce, "Comparison of incompatible C types");
	return 0;
}
/* }}} */

static clib_result clib_ffi_cdata_count_elements(clib_object *obj, long *count) /* {{{ */
{
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)obj;
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);

	if (type->kind != CLIB_FFI_TYPE_ARRAY) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to count() on non C array");
		return FAILURE;
	} else {
		*count = type->array.length;
		return SUCCESS;
	}
}
/* }}} */

static clib_object* clib_ffi_add(clib_ffi_cdata *base_cdata, clib_ffi_type *base_type, long offset) /* {{{ */
{
	char *ptr;
	clib_ffi_type *ptr_type;
	clib_ffi_cdata *cdata =
		(clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);

	if (base_type->kind == CLIB_FFI_TYPE_POINTER) {
		if (CLIB_FFI_TYPE_IS_OWNED(base_cdata->type)) {
			if (!(base_type->attr & CLIB_FFI_ATTR_STORED)) {
				if (GC_REFCOUNT(&base_cdata->std) == 1) {
					/* transfer type ownership */
					base_cdata->type = base_type;
					base_type = CLIB_FFI_TYPE_MAKE_OWNED(base_type);
				} else {
					base_cdata->type = base_type = clib_ffi_remember_type(base_type);
				}
			}
		}
		cdata->type = base_type;
		ptr = (char*)(*(void**)base_cdata->ptr);
		ptr_type = CLIB_FFI_TYPE(base_type)->pointer.type;
	} else {
		clib_ffi_type *new_type = emalloc(sizeof(clib_ffi_type));

		new_type->kind = CLIB_FFI_TYPE_POINTER;
		new_type->attr = 0;
		new_type->size = sizeof(void*);
		new_type->align = _Alignof(void*);

		ptr_type = base_type->array.type;
		if (CLIB_FFI_TYPE_IS_OWNED(ptr_type)) {
			ptr_type = CLIB_FFI_TYPE(ptr_type);
			if (!(ptr_type->attr & CLIB_FFI_ATTR_STORED)) {
				if (GC_REFCOUNT(&base_cdata->std) == 1) {
					/* transfer type ownership */
					base_type->array.type = ptr_type;
					ptr_type = CLIB_FFI_TYPE_MAKE_OWNED(ptr_type);
				} else {
					base_type->array.type = ptr_type = clib_ffi_remember_type(ptr_type);
				}
			}
		}
		new_type->pointer.type = ptr_type;

		cdata->type = CLIB_FFI_TYPE_MAKE_OWNED(new_type);
		ptr = (char*)base_cdata->ptr;
	}
	cdata->ptr = &cdata->ptr_holder;
	cdata->ptr_holder = ptr +
		(ptrdiff_t) (offset * CLIB_FFI_TYPE(ptr_type)->size);
	cdata->flags = base_cdata->flags & CLIB_FFI_FLAG_CONST;
	return &cdata->std;
}
/* }}} */

static clib_result clib_ffi_cdata_do_operation(clib_uchar opcode, b_value *result, b_value *op1, b_value *op2) /* {{{ */
{
	long offset;

	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	if (Z_TYPE_P(op1) == IS_OBJECT && Z_OBJCE_P(op1) == clib_ffi_cdata_ce) {
		clib_ffi_cdata *cdata1 = (clib_ffi_cdata*)Z_OBJ_P(op1);
		clib_ffi_type *type1 = CLIB_FFI_TYPE(cdata1->type);

		if (type1->kind == CLIB_FFI_TYPE_POINTER || type1->kind == CLIB_FFI_TYPE_ARRAY) {
			if (opcode == CLIB_ADD) {
				offset = b_value_get_long(op2);
				ZVAL_OBJ(result, clib_ffi_add(cdata1, type1, offset));
				if (result == op1) {
					OBJ_RELEASE(&cdata1->std);
				}
				return SUCCESS;
			} else if (opcode == CLIB_SUB) {
				if (Z_TYPE_P(op2) == IS_OBJECT && Z_OBJCE_P(op2) == clib_ffi_cdata_ce) {
					clib_ffi_cdata *cdata2 = (clib_ffi_cdata*)Z_OBJ_P(op2);
					clib_ffi_type *type2 = CLIB_FFI_TYPE(cdata2->type);

					if (type2->kind == CLIB_FFI_TYPE_POINTER || type2->kind == CLIB_FFI_TYPE_ARRAY) {
						clib_ffi_type *t1, *t2;
						char *p1, *p2;

						if (type1->kind == CLIB_FFI_TYPE_POINTER) {
							t1 = CLIB_FFI_TYPE(type1->pointer.type);
							p1 = (char*)(*(void**)cdata1->ptr);
						} else {
							t1 = CLIB_FFI_TYPE(type1->array.type);
							p1 = cdata1->ptr;
						}
						if (type2->kind == CLIB_FFI_TYPE_POINTER) {
							t2 = CLIB_FFI_TYPE(type2->pointer.type);
							p2 = (char*)(*(void**)cdata2->ptr);
						} else {
							t2 = CLIB_FFI_TYPE(type2->array.type);
							p2 = cdata2->ptr;
						}
						if (clib_ffi_is_same_type(t1, t2)) {
							ZVAL_LONG(result,
								(long)(p1 - p2) / (long)t1->size);
							return SUCCESS;
						}
					}
				}
				offset = b_value_get_long(op2);
				ZVAL_OBJ(result, clib_ffi_add(cdata1, type1, -offset));
				if (result == op1) {
					OBJ_RELEASE(&cdata1->std);
				}
				return SUCCESS;
			}
		}
	} else if (Z_TYPE_P(op2) == IS_OBJECT && Z_OBJCE_P(op2) == clib_ffi_cdata_ce) {
		clib_ffi_cdata *cdata2 = (clib_ffi_cdata*)Z_OBJ_P(op2);
		clib_ffi_type *type2 = CLIB_FFI_TYPE(cdata2->type);

		if (type2->kind == CLIB_FFI_TYPE_POINTER || type2->kind == CLIB_FFI_TYPE_ARRAY) {
			if (opcode == CLIB_ADD) {
				offset = b_value_get_long(op1);
				ZVAL_OBJ(result, clib_ffi_add(cdata2, type2, offset));
				return SUCCESS;
			}
		}
	}

	return FAILURE;
}
/* }}} */

typedef struct _clib_ffi_cdata_iterator {
	clib_object_iterator it;
	long key;
	b_value value;
	bool by_ref;
} clib_ffi_cdata_iterator;

static void clib_ffi_cdata_it_dtor(clib_object_iterator *iter) /* {{{ */
{
	b_value_ptr_dtor(&((clib_ffi_cdata_iterator*)iter)->value);
	b_value_ptr_dtor(&iter->data);
}
/* }}} */

static int clib_ffi_cdata_it_valid(clib_object_iterator *it) /* {{{ */
{
	clib_ffi_cdata_iterator *iter = (clib_ffi_cdata_iterator*)it;
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ(iter->it.data);
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);

	return (iter->key >= 0 && iter->key < type->array.length) ? SUCCESS : FAILURE;
}
/* }}} */

static b_value *clib_ffi_cdata_it_get_current_data(clib_object_iterator *it) /* {{{ */
{
	clib_ffi_cdata_iterator *iter = (clib_ffi_cdata_iterator*)it;
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ(iter->it.data);
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	clib_ffi_type  *dim_type;
	void *ptr;

	if (!cdata->ptr) {
		clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
		return &EG(uninitialized_b_value);
	}
	dim_type = type->array.type;
	if (CLIB_FFI_TYPE_IS_OWNED(dim_type)) {
		dim_type = CLIB_FFI_TYPE(dim_type);
		if (!(dim_type->attr & CLIB_FFI_ATTR_STORED)
		 && dim_type->kind == CLIB_FFI_TYPE_POINTER) {
			type->array.type = dim_type = clib_ffi_remember_type(dim_type);
		}
	}
	ptr = (void*)((char*)cdata->ptr + dim_type->size * iter->it.index);

	b_value_ptr_dtor(&iter->value);
	clib_ffi_cdata_to_b_value(NULL, ptr, dim_type, iter->by_ref ? BP_VAR_RW : BP_VAR_R, &iter->value, (cdata->flags & CLIB_FFI_FLAG_CONST) | (clib_ffi_flags)(type->attr & CLIB_FFI_ATTR_CONST), 0, 0);
	return &iter->value;
}
/* }}} */

static void clib_ffi_cdata_it_get_current_key(clib_object_iterator *it, b_value *key) /* {{{ */
{
	clib_ffi_cdata_iterator *iter = (clib_ffi_cdata_iterator*)it;
	ZVAL_LONG(key, iter->key);
}
/* }}} */

static void clib_ffi_cdata_it_move_forward(clib_object_iterator *it) /* {{{ */
{
	clib_ffi_cdata_iterator *iter = (clib_ffi_cdata_iterator*)it;
	iter->key++;
}
/* }}} */

static void clib_ffi_cdata_it_rewind(clib_object_iterator *it) /* {{{ */
{
	clib_ffi_cdata_iterator *iter = (clib_ffi_cdata_iterator*)it;
	iter->key = 0;
}
/* }}} */

static const clib_object_iterator_funcs clib_ffi_cdata_it_funcs = {
	clib_ffi_cdata_it_dtor,
	clib_ffi_cdata_it_valid,
	clib_ffi_cdata_it_get_current_data,
	clib_ffi_cdata_it_get_current_key,
	clib_ffi_cdata_it_move_forward,
	clib_ffi_cdata_it_rewind,
	NULL,
	NULL, /* get_gc */
};

static clib_object_iterator *clib_ffi_cdata_get_iterator(clib_class_entry *ce, b_value *object, int by_ref) /* {{{ */
{
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(object);
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	clib_ffi_cdata_iterator *iter;

	if (type->kind != CLIB_FFI_TYPE_ARRAY) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to iterate on non C array");
		return NULL;
	}

	iter = emalloc(sizeof(clib_ffi_cdata_iterator));

	clib_iterator_init(&iter->it);

	Z_ADDREF_P(object);
	ZVAL_OBJ(&iter->it.data, Z_OBJ_P(object));
	iter->it.funcs = &clib_ffi_cdata_it_funcs;
	iter->key = 0;
	iter->by_ref = by_ref;
	ZVAL_UNDEF(&iter->value);

	return &iter->it;
}
/* }}} */

static b_obj_dict *clib_ffi_cdata_get_debug_info(clib_object *obj, int *is_temp) /* {{{ */
{
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)obj;
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	void           *ptr = cdata->ptr;
	b_obj_dict      *ht = NULL;
	b_obj_string    *key;
	clib_ffi_field *f;
	long       n;
	b_value            tmp;

	if (!cdata->ptr) {
		clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
		return NULL;
	}

	switch (type->kind) {
		case CLIB_FFI_TYPE_BOOL:
		case CLIB_FFI_TYPE_CHAR:
		case CLIB_FFI_TYPE_ENUM:
		case CLIB_FFI_TYPE_FLOAT:
		case CLIB_FFI_TYPE_DOUBLE:
#ifdef HAVE_LONG_DOUBLE
		case CLIB_FFI_TYPE_LONGDOUBLE:
#endif
		case CLIB_FFI_TYPE_UINT8:
		case CLIB_FFI_TYPE_SINT8:
		case CLIB_FFI_TYPE_UINT16:
		case CLIB_FFI_TYPE_SINT16:
		case CLIB_FFI_TYPE_UINT32:
		case CLIB_FFI_TYPE_SINT32:
		case CLIB_FFI_TYPE_UINT64:
		case CLIB_FFI_TYPE_SINT64:
			clib_ffi_cdata_to_b_value(cdata, ptr, type, BP_VAR_R, &tmp, CLIB_FFI_FLAG_CONST, 0, 0);
			ht = clib_new_array(1);
			clib_hash_str_add(ht, "cdata", sizeof("cdata")-1, &tmp);
			*is_temp = 1;
			return ht;
			break;
		case CLIB_FFI_TYPE_POINTER:
			if (*(void**)ptr == NULL) {
				ZVAL_NULL(&tmp);
				ht = clib_new_array(1);
				clib_hash_index_add_new(ht, 0, &tmp);
				*is_temp = 1;
				return ht;
			} else if (CLIB_FFI_TYPE(type->pointer.type)->kind == CLIB_FFI_TYPE_VOID) {
				ZVAL_LONG(&tmp, (uintptr_t)*(void**)ptr);
				ht = clib_new_array(1);
				clib_hash_index_add_new(ht, 0, &tmp);
				*is_temp = 1;
				return ht;
			} else {
				clib_ffi_cdata_to_b_value(NULL, *(void**)ptr, CLIB_FFI_TYPE(type->pointer.type), BP_VAR_R, &tmp, CLIB_FFI_FLAG_CONST, 0, 0);
				ht = clib_new_array(1);
				clib_hash_index_add_new(ht, 0, &tmp);
				*is_temp = 1;
				return ht;
			}
			break;
		case CLIB_FFI_TYPE_STRUCT:
			ht = clib_new_array(clib_hash_num_elements(&type->record.fields));
			CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(&type->record.fields, key, f) {
				if (key) {
					if (!f->bits) {
						void *f_ptr = (void*)(((char*)ptr) + f->offset);
						clib_ffi_cdata_to_b_value(NULL, f_ptr, CLIB_FFI_TYPE(f->type), BP_VAR_R, &tmp, CLIB_FFI_FLAG_CONST, 0, type->attr & CLIB_FFI_ATTR_UNION);
						clib_hash_add(ht, key, &tmp);
					} else {
						clib_ffi_bit_field_to_b_value(ptr, f, &tmp);
						clib_hash_add(ht, key, &tmp);
					}
				}
			} CLIB_HASH_FOREACH_END();
			*is_temp = 1;
			return ht;
		case CLIB_FFI_TYPE_ARRAY:
			ht = clib_new_array(type->array.length);
			for (n = 0; n < type->array.length; n++) {
				clib_ffi_cdata_to_b_value(NULL, ptr, CLIB_FFI_TYPE(type->array.type), BP_VAR_R, &tmp, CLIB_FFI_FLAG_CONST, 0, 0);
				clib_hash_index_add(ht, n, &tmp);
				ptr = (void*)(((char*)ptr) + CLIB_FFI_TYPE(type->array.type)->size);
			}
			*is_temp = 1;
			return ht;
		case CLIB_FFI_TYPE_FUNC:
			ht = clib_new_array(0);
			// TODO: function name ???
			*is_temp = 1;
			return ht;
			break;
		default:
			CLIB_UNREACHABLE();
			break;
	}
	return NULL;
}
/* }}} */

static clib_result clib_ffi_cdata_get_closure(clib_object *obj, clib_class_entry **ce_ptr, clib_function **fptr_ptr, clib_object **obj_ptr, bool check_only) /* {{{ */
{
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)obj;
	clib_ffi_type  *type = CLIB_FFI_TYPE(cdata->type);
	clib_function  *func;

	if (type->kind != CLIB_FFI_TYPE_POINTER) {
		if (!check_only) {
			clib_throw_error(clib_ffi_exception_ce, "Attempt to call non C function pointer");
		}
		return FAILURE;
	}
	type = CLIB_FFI_TYPE(type->pointer.type);
	if (type->kind != CLIB_FFI_TYPE_FUNC) {
		if (!check_only) {
			clib_throw_error(clib_ffi_exception_ce, "Attempt to call non C function pointer");
		}
		return FAILURE;
	}
	if (!cdata->ptr) {
		if (!check_only) {
			clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
		}
		return FAILURE;
	}

	if (EXPECTED(EG(trampoline).common.function_name == NULL)) {
		func = &EG(trampoline);
	} else {
		func = ecalloc(sizeof(clib_internal_function), 1);
	}
	func->type = CLIB_INTERNAL_FUNCTION;
	func->common.arg_flags[0] = 0;
	func->common.arg_flags[1] = 0;
	func->common.arg_flags[2] = 0;
	func->common.fn_flags = CLIB_ACC_CALL_VIA_TRAMPOLINE;
	func->common.function_name = ZSTR_KNOWN(CLIB_STR_MAGIC_INVOKE);
	/* set to 0 to avoid arg_info[] allocation, because all values are passed by value anyway */
	func->common.num_args = 0;
	func->common.required_num_args = type->func.args ? clib_hash_num_elements(type->func.args) : 0;
	func->common.scope = NULL;
	func->common.prototype = NULL;
	func->common.arg_info = NULL;
	func->internal_function.handler = CLIB_FN(ffi_trampoline);
	func->internal_function.module = NULL;

	func->internal_function.reserved[0] = type;
	func->internal_function.reserved[1] = *(void**)cdata->ptr;

	*ce_ptr = NULL;
	*fptr_ptr= func;
	*obj_ptr = NULL;

	return SUCCESS;
}
/* }}} */

static clib_object *clib_ffi_ctype_new(clib_class_entry *class_type) /* {{{ */
{
	clib_ffi_ctype *ctype;

	ctype = emalloc(sizeof(clib_ffi_ctype));

	clib_ffi_object_init(&ctype->std, class_type);
	ctype->std.handlers = &clib_ffi_ctype_handlers;

	ctype->type = NULL;

	return &ctype->std;
}
/* }}} */

static void clib_ffi_ctype_free_obj(clib_object *object) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)object;

	clib_ffi_type_dtor(ctype->type);
}
/* }}} */

static bool clib_ffi_is_same_type(clib_ffi_type *type1, clib_ffi_type *type2) /* {{{ */
{
	while (1) {
		if (type1 == type2) {
			return 1;
		} else if (type1->kind == type2->kind) {
			if (type1->kind < CLIB_FFI_TYPE_POINTER) {
				return 1;
			} else if (type1->kind == CLIB_FFI_TYPE_POINTER) {
				type1 = CLIB_FFI_TYPE(type1->pointer.type);
				type2 = CLIB_FFI_TYPE(type2->pointer.type);
				if (type1->kind == CLIB_FFI_TYPE_VOID ||
				    type2->kind == CLIB_FFI_TYPE_VOID) {
				    return 1;
				}
			} else if (type1->kind == CLIB_FFI_TYPE_ARRAY &&
			           type1->array.length == type2->array.length) {
				type1 = CLIB_FFI_TYPE(type1->array.type);
				type2 = CLIB_FFI_TYPE(type2->array.type);
			} else {
				break;
			}
		} else {
			break;
		}
	}
	return 0;
}
/* }}} */

static b_obj_string *clib_ffi_ctype_get_class_name(const clib_object *zobj) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)zobj;

	return clib_ffi_get_class_name(zobj->ce->name, CLIB_FFI_TYPE(ctype->type));
}
/* }}} */

static int clib_ffi_ctype_compare_objects(b_value *o1, b_value *o2) /* {{{ */
{
	if (Z_TYPE_P(o1) == IS_OBJECT && Z_OBJCE_P(o1) == clib_ffi_ctype_ce &&
	    Z_TYPE_P(o2) == IS_OBJECT && Z_OBJCE_P(o2) == clib_ffi_ctype_ce) {
		clib_ffi_ctype *ctype1 = (clib_ffi_ctype*)Z_OBJ_P(o1);
		clib_ffi_ctype *ctype2 = (clib_ffi_ctype*)Z_OBJ_P(o2);
		clib_ffi_type *type1 = CLIB_FFI_TYPE(ctype1->type);
		clib_ffi_type *type2 = CLIB_FFI_TYPE(ctype2->type);

		if (clib_ffi_is_same_type(type1, type2)) {
			return 0;
		} else {
			return 1;
		}
	}
	clib_throw_error(clib_ffi_exception_ce, "Comparison of incompatible C types");
	return 0;
}
/* }}} */

static b_obj_dict *clib_ffi_ctype_get_debug_info(clib_object *obj, int *is_temp) /* {{{ */
{
	return NULL;
}
/* }}} */

static clib_object *clib_ffi_new(clib_class_entry *class_type) /* {{{ */
{
	clib_ffi *ffi;

	ffi = emalloc(sizeof(clib_ffi));

	clib_ffi_object_init(&ffi->std, class_type);
	ffi->std.handlers = &clib_ffi_handlers;

	ffi->lib = NULL;
	ffi->symbols = NULL;
	ffi->tags = NULL;
	ffi->persistent = 0;

	return &ffi->std;
}
/* }}} */

static void _clib_ffi_type_dtor(clib_ffi_type *type) /* {{{ */
{
	type = CLIB_FFI_TYPE(type);

	switch (type->kind) {
		case CLIB_FFI_TYPE_ENUM:
			if (type->enumeration.tag_name) {
				b_obj_string_release(type->enumeration.tag_name);
			}
			break;
		case CLIB_FFI_TYPE_STRUCT:
			if (type->record.tag_name) {
				b_obj_string_release(type->record.tag_name);
			}
			clib_hash_destroy(&type->record.fields);
			break;
		case CLIB_FFI_TYPE_POINTER:
			clib_ffi_type_dtor(type->pointer.type);
			break;
		case CLIB_FFI_TYPE_ARRAY:
			clib_ffi_type_dtor(type->array.type);
			break;
		case CLIB_FFI_TYPE_FUNC:
			if (type->func.args) {
				clib_hash_destroy(type->func.args);
				pefree(type->func.args, type->attr & CLIB_FFI_ATTR_PERSISTENT);
			}
			clib_ffi_type_dtor(type->func.ret_type);
			break;
		default:
			break;
	}
	pefree(type, type->attr & CLIB_FFI_ATTR_PERSISTENT);
}
/* }}} */

static void clib_ffi_type_hash_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_type *type = Z_PTR_P(zv);
	clib_ffi_type_dtor(type);
}
/* }}} */

static void clib_ffi_field_hash_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_field *field = Z_PTR_P(zv);
	clib_ffi_type_dtor(field->type);
	efree(field);
}
/* }}} */

static void clib_ffi_field_hash_persistent_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_field *field = Z_PTR_P(zv);
	clib_ffi_type_dtor(field->type);
	free(field);
}
/* }}} */

static void clib_ffi_symbol_hash_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_symbol *sym = Z_PTR_P(zv);
	clib_ffi_type_dtor(sym->type);
	efree(sym);
}
/* }}} */

static void clib_ffi_symbol_hash_persistent_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_symbol *sym = Z_PTR_P(zv);
	clib_ffi_type_dtor(sym->type);
	free(sym);
}
/* }}} */

static void clib_ffi_tag_hash_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_tag *tag = Z_PTR_P(zv);
	clib_ffi_type_dtor(tag->type);
	efree(tag);
}
/* }}} */

static void clib_ffi_tag_hash_persistent_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_tag *tag = Z_PTR_P(zv);
	clib_ffi_type_dtor(tag->type);
	free(tag);
}
/* }}} */

static void clib_ffi_cdata_dtor(clib_ffi_cdata *cdata) /* {{{ */
{
	clib_ffi_type_dtor(cdata->type);
	if (cdata->flags & CLIB_FFI_FLAG_OWNED) {
		if (cdata->ptr != (void*)&cdata->ptr_holder) {
			pefree(cdata->ptr, cdata->flags & CLIB_FFI_FLAG_PERSISTENT);
		} else {
			pefree(cdata->ptr_holder, cdata->flags & CLIB_FFI_FLAG_PERSISTENT);
		}
	}
}
/* }}} */

static void clib_ffi_scope_hash_dtor(b_value *zv) /* {{{ */
{
	clib_ffi_scope *scope = Z_PTR_P(zv);
	if (scope->symbols) {
		clib_hash_destroy(scope->symbols);
		free(scope->symbols);
	}
	if (scope->tags) {
		clib_hash_destroy(scope->tags);
		free(scope->tags);
	}
	free(scope);
}
/* }}} */

static void clib_ffi_free_obj(clib_object *object) /* {{{ */
{
	clib_ffi *ffi = (clib_ffi*)object;

	if (ffi->persistent) {
		return;
	}

	if (ffi->lib) {
		DL_UNLOAD(ffi->lib);
		ffi->lib = NULL;
	}

	if (ffi->symbols) {
		clib_hash_destroy(ffi->symbols);
		efree(ffi->symbols);
	}

	if (ffi->tags) {
		clib_hash_destroy(ffi->tags);
		efree(ffi->tags);
	}
}
/* }}} */

static void clib_ffi_cdata_free_obj(clib_object *object) /* {{{ */
{
	clib_ffi_cdata *cdata = (clib_ffi_cdata*)object;

	clib_ffi_cdata_dtor(cdata);
}
/* }}} */

static clib_object *clib_ffi_cdata_clone_obj(clib_object *obj) /* {{{ */
{
	clib_ffi_cdata *old_cdata = (clib_ffi_cdata*)obj;
	clib_ffi_type *type = CLIB_FFI_TYPE(old_cdata->type);
	clib_ffi_cdata *new_cdata;

	new_cdata = (clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);
	if (type->kind < CLIB_FFI_TYPE_POINTER) {
		new_cdata->std.handlers = &clib_ffi_cdata_value_handlers;
	}
	new_cdata->type = type;
	new_cdata->ptr = emalloc(type->size);
	memcpy(new_cdata->ptr, old_cdata->ptr, type->size);
	new_cdata->flags |= CLIB_FFI_FLAG_OWNED;

	return &new_cdata->std;
}
/* }}} */

static b_value *clib_ffi_read_var(clib_object *obj, b_obj_string *var_name, int read_type, void **cache_slot, b_value *rv) /* {{{ */
{
	clib_ffi        *ffi = (clib_ffi*)obj;
	clib_ffi_symbol *sym = NULL;

	if (ffi->symbols) {
		sym = clib_hash_find_ptr(ffi->symbols, var_name);
		if (sym && sym->kind != CLIB_FFI_SYM_VAR && sym->kind != CLIB_FFI_SYM_CONST && sym->kind != CLIB_FFI_SYM_FUNC) {
			sym = NULL;
		}
	}
	if (!sym) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to read undefined C variable '%s'", ZSTR_VAL(var_name));
		return &EG(uninitialized_b_value);
	}

	if (sym->kind == CLIB_FFI_SYM_VAR) {
		clib_ffi_cdata_to_b_value(NULL, sym->addr, CLIB_FFI_TYPE(sym->type), read_type, rv, (clib_ffi_flags)sym->is_const, 0, 0);
	} else if (sym->kind == CLIB_FFI_SYM_FUNC) {
		clib_ffi_cdata *cdata;
		clib_ffi_type *new_type = emalloc(sizeof(clib_ffi_type));

		new_type->kind = CLIB_FFI_TYPE_POINTER;
		new_type->attr = 0;
		new_type->size = sizeof(void*);
		new_type->align = _Alignof(void*);
		new_type->pointer.type = CLIB_FFI_TYPE(sym->type);

		cdata = emalloc(sizeof(clib_ffi_cdata));
		clib_ffi_object_init(&cdata->std, clib_ffi_cdata_ce);
		cdata->std.handlers = &clib_ffi_cdata_handlers;
		cdata->type = CLIB_FFI_TYPE_MAKE_OWNED(new_type);
		cdata->flags = CLIB_FFI_FLAG_CONST;
		cdata->ptr_holder = sym->addr;
		cdata->ptr = &cdata->ptr_holder;
		ZVAL_OBJ(rv, &cdata->std);
	} else {
		ZVAL_LONG(rv, sym->value);
	}

	return rv;
}
/* }}} */

static b_value *clib_ffi_write_var(clib_object *obj, b_obj_string *var_name, b_value *value, void **cache_slot) /* {{{ */
{
	clib_ffi        *ffi = (clib_ffi*)obj;
	clib_ffi_symbol *sym = NULL;

	if (ffi->symbols) {
		sym = clib_hash_find_ptr(ffi->symbols, var_name);
		if (sym && sym->kind != CLIB_FFI_SYM_VAR) {
			sym = NULL;
		}
	}
	if (!sym) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign undefined C variable '%s'", ZSTR_VAL(var_name));
		return value;
	}

	if (sym->is_const) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to assign read-only C variable '%s'", ZSTR_VAL(var_name));
		return value;
	}

	clib_ffi_b_value_to_cdata(sym->addr, CLIB_FFI_TYPE(sym->type), value);
	return value;
}
/* }}} */

static clib_result clib_ffi_pass_arg(b_value *arg, clib_ffi_type *type, ffi_type **pass_type, void **arg_values, uint32_t n, clib_execute_data *execute_data) /* {{{ */
{
	long lval;
	double dval;
	b_obj_string *str, *tmp_str;
	clib_ffi_type_kind kind = type->kind;

	ZVAL_DEREF(arg);

again:
	switch (kind) {
		case CLIB_FFI_TYPE_FLOAT:
			dval = b_value_get_double(arg);
			*pass_type = &ffi_type_float;
			*(float*)arg_values[n] = (float)dval;
			break;
		case CLIB_FFI_TYPE_DOUBLE:
			dval = b_value_get_double(arg);
			*pass_type = &ffi_type_double;
			*(double*)arg_values[n] = dval;
			break;
#ifdef HAVE_LONG_DOUBLE
		case CLIB_FFI_TYPE_LONGDOUBLE:
			dval = b_value_get_double(arg);
			*pass_type = &ffi_type_double;
			*(long double*)arg_values[n] = (long double)dval;
			break;
#endif
		case CLIB_FFI_TYPE_UINT8:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_uint8;
			*(uint8_t*)arg_values[n] = (uint8_t)lval;
			break;
		case CLIB_FFI_TYPE_SINT8:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_sint8;
			*(int8_t*)arg_values[n] = (int8_t)lval;
			break;
		case CLIB_FFI_TYPE_UINT16:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_uint16;
			*(uint16_t*)arg_values[n] = (uint16_t)lval;
			break;
		case CLIB_FFI_TYPE_SINT16:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_sint16;
			*(int16_t*)arg_values[n] = (int16_t)lval;
			break;
		case CLIB_FFI_TYPE_UINT32:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_uint32;
			*(uint32_t*)arg_values[n] = (uint32_t)lval;
			break;
		case CLIB_FFI_TYPE_SINT32:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_sint32;
			*(int32_t*)arg_values[n] = (int32_t)lval;
			break;
		case CLIB_FFI_TYPE_UINT64:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_uint64;
			*(uint64_t*)arg_values[n] = (uint64_t)lval;
			break;
		case CLIB_FFI_TYPE_SINT64:
			lval = b_value_get_long(arg);
			*pass_type = &ffi_type_sint64;
			*(int64_t*)arg_values[n] = (int64_t)lval;
			break;
		case CLIB_FFI_TYPE_POINTER:
			*pass_type = &ffi_type_pointer;
			if (Z_TYPE_P(arg) == IS_NULL) {
				*(void**)arg_values[n] = NULL;
				return SUCCESS;
			} else if (Z_TYPE_P(arg) == IS_STRING
			        && ((CLIB_FFI_TYPE(type->pointer.type)->kind == CLIB_FFI_TYPE_CHAR)
			         || (CLIB_FFI_TYPE(type->pointer.type)->kind == CLIB_FFI_TYPE_VOID))) {
				*(void**)arg_values[n] = Z_STRVAL_P(arg);
				return SUCCESS;
			} else if (Z_TYPE_P(arg) == IS_OBJECT && Z_OBJCE_P(arg) == clib_ffi_cdata_ce) {
				clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(arg);

				if (clib_ffi_is_compatible_type(type, CLIB_FFI_TYPE(cdata->type))) {
					if (CLIB_FFI_TYPE(cdata->type)->kind == CLIB_FFI_TYPE_POINTER) {
						if (!cdata->ptr) {
							clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
							return FAILURE;
						}
						*(void**)arg_values[n] = *(void**)cdata->ptr;
					} else {
						*(void**)arg_values[n] = cdata->ptr;
					}
					return SUCCESS;
				}
#if FFI_CLOSURES
			} else if (CLIB_FFI_TYPE(type->pointer.type)->kind == CLIB_FFI_TYPE_FUNC) {
				void *callback = clib_ffi_create_callback(CLIB_FFI_TYPE(type->pointer.type), arg);

				if (callback) {
					*(void**)arg_values[n] = callback;
					break;
				} else {
					return FAILURE;
				}
#endif
			}
			clib_ffi_pass_incompatible(arg, type, n, execute_data);
			return FAILURE;
		case CLIB_FFI_TYPE_BOOL:
			*pass_type = &ffi_type_uint8;
			*(uint8_t*)arg_values[n] = clib_is_true(arg);
			break;
		case CLIB_FFI_TYPE_CHAR:
			str = b_value_get_tmp_string(arg, &tmp_str);
			*pass_type = &ffi_type_sint8;
			*(char*)arg_values[n] = ZSTR_VAL(str)[0];
			if (ZSTR_LEN(str) != 1) {
				clib_ffi_pass_incompatible(arg, type, n, execute_data);
			}
			clib_tmp_string_release(tmp_str);
			break;
		case CLIB_FFI_TYPE_ENUM:
			kind = type->enumeration.kind;
			goto again;
		case CLIB_FFI_TYPE_STRUCT:
			if (Z_TYPE_P(arg) == IS_OBJECT && Z_OBJCE_P(arg) == clib_ffi_cdata_ce) {
				clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(arg);

				if (clib_ffi_is_compatible_type(type, CLIB_FFI_TYPE(cdata->type))) {
					*pass_type = clib_ffi_make_fake_struct_type(type);;
					arg_values[n] = cdata->ptr;
					break;
				}
			}
			clib_ffi_pass_incompatible(arg, type, n, execute_data);
			return FAILURE;
		default:
			clib_ffi_pass_unsupported(type);
			return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

static clib_result clib_ffi_pass_var_arg(b_value *arg, ffi_type **pass_type, void **arg_values, uint32_t n, clib_execute_data *execute_data) /* {{{ */
{
	ZVAL_DEREF(arg);
	switch (Z_TYPE_P(arg)) {
		case IS_NULL:
			*pass_type = &ffi_type_pointer;
			*(void**)arg_values[n] = NULL;
			break;
		case IS_FALSE:
			*pass_type = &ffi_type_uint8;
			*(uint8_t*)arg_values[n] = 0;
			break;
		case IS_TRUE:
			*pass_type = &ffi_type_uint8;
			*(uint8_t*)arg_values[n] = 1;
			break;
		case IS_LONG:
			if (sizeof(long) == 4) {
				*pass_type = &ffi_type_sint32;
				*(int32_t*)arg_values[n] = Z_LVAL_P(arg);
			} else {
				*pass_type = &ffi_type_sint64;
				*(int64_t*)arg_values[n] = Z_LVAL_P(arg);
			}
			break;
		case IS_DOUBLE:
			*pass_type = &ffi_type_double;
			*(double*)arg_values[n] = Z_DVAL_P(arg);
			break;
		case IS_STRING:
			*pass_type = &ffi_type_pointer;
			*(char**)arg_values[n] = Z_STRVAL_P(arg);
			break;
		case IS_OBJECT:
			if (Z_OBJCE_P(arg) == clib_ffi_cdata_ce) {
				clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(arg);
				clib_ffi_type *type = CLIB_FFI_TYPE(cdata->type);

				return clib_ffi_pass_arg(arg, type, pass_type, arg_values, n, execute_data);
			}
			CLIB_FALLTHROUGH;
		default:
			clib_throw_error(clib_ffi_exception_ce, "Unsupported argument type");
			return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

static CLIB_FUNCTION(ffi_trampoline) /* {{{ */
{
	clib_ffi_type *type = EX(func)->internal_function.reserved[0];
	void *addr = EX(func)->internal_function.reserved[1];
	ffi_cif cif;
	ffi_type *ret_type = NULL;
	ffi_type **arg_types = NULL;
	void **arg_values = NULL;
	uint32_t n, arg_count;
	void *ret;
	clib_ffi_type *arg_type;
	ALLOCA_FLAG(arg_types_use_heap = 0)
	ALLOCA_FLAG(arg_values_use_heap = 0)
	ALLOCA_FLAG(ret_use_heap = 0)

	CLIB_ASSERT(type->kind == CLIB_FFI_TYPE_FUNC);
	arg_count = type->func.args ? clib_hash_num_elements(type->func.args) : 0;
	if (type->attr & CLIB_FFI_ATTR_VARIADIC) {
		if (arg_count > EX_NUM_ARGS()) {
			clib_throw_error(clib_ffi_exception_ce, "Incorrect number of arguments for C function '%s', expecting at least %d parameter%s", ZSTR_VAL(EX(func)->internal_function.function_name), arg_count, (arg_count != 1) ? "s" : "");
			goto exit;
		}
		if (EX_NUM_ARGS()) {
			arg_types = do_alloca(
				sizeof(ffi_type*) * EX_NUM_ARGS(), arg_types_use_heap);
			arg_values = do_alloca(
				(sizeof(void*) + CLIB_FFI_SIZEOF_ARG) * EX_NUM_ARGS(), arg_values_use_heap);
			n = 0;
			if (type->func.args) {
				CLIB_HASH_PACKED_FOREACH_PTR(type->func.args, arg_type) {
					arg_type = CLIB_FFI_TYPE(arg_type);
					arg_values[n] = ((char*)arg_values) + (sizeof(void*) * EX_NUM_ARGS()) + (CLIB_FFI_SIZEOF_ARG * n);
					if (clib_ffi_pass_arg(EX_VAR_NUM(n), arg_type, &arg_types[n], arg_values, n, execute_data) == FAILURE) {
						free_alloca(arg_types, arg_types_use_heap);
						free_alloca(arg_values, arg_values_use_heap);
						goto exit;
					}
					n++;
				} CLIB_HASH_FOREACH_END();
			}
			for (; n < EX_NUM_ARGS(); n++) {
				arg_values[n] = ((char*)arg_values) + (sizeof(void*) * EX_NUM_ARGS()) + (CLIB_FFI_SIZEOF_ARG * n);
				if (clib_ffi_pass_var_arg(EX_VAR_NUM(n), &arg_types[n], arg_values, n, execute_data) == FAILURE) {
					free_alloca(arg_types, arg_types_use_heap);
					free_alloca(arg_values, arg_values_use_heap);
					goto exit;
				}
			}
		}
		ret_type = clib_ffi_get_type(CLIB_FFI_TYPE(type->func.ret_type));
		if (!ret_type) {
			clib_ffi_return_unsupported(type->func.ret_type);
			free_alloca(arg_types, arg_types_use_heap);
			free_alloca(arg_values, arg_values_use_heap);
			goto exit;
		}
		if (ffi_prep_cif_var(&cif, type->func.abi, arg_count, EX_NUM_ARGS(), ret_type, arg_types) != FFI_OK) {
			clib_throw_error(clib_ffi_exception_ce, "Cannot prepare callback CIF");
			free_alloca(arg_types, arg_types_use_heap);
			free_alloca(arg_values, arg_values_use_heap);
			goto exit;
		}
	} else {
		if (arg_count != EX_NUM_ARGS()) {
			clib_throw_error(clib_ffi_exception_ce, "Incorrect number of arguments for C function '%s', expecting exactly %d parameter%s", ZSTR_VAL(EX(func)->internal_function.function_name), arg_count, (arg_count != 1) ? "s" : "");
			goto exit;
		}
		if (EX_NUM_ARGS()) {
			arg_types = do_alloca(
				(sizeof(ffi_type*) + sizeof(ffi_type)) * EX_NUM_ARGS(), arg_types_use_heap);
			arg_values = do_alloca(
				(sizeof(void*) + CLIB_FFI_SIZEOF_ARG) * EX_NUM_ARGS(), arg_values_use_heap);
			n = 0;
			if (type->func.args) {
				CLIB_HASH_PACKED_FOREACH_PTR(type->func.args, arg_type) {
					arg_type = CLIB_FFI_TYPE(arg_type);
					arg_values[n] = ((char*)arg_values) + (sizeof(void*) * EX_NUM_ARGS()) + (CLIB_FFI_SIZEOF_ARG * n);
					if (clib_ffi_pass_arg(EX_VAR_NUM(n), arg_type, &arg_types[n], arg_values, n, execute_data) == FAILURE) {
						free_alloca(arg_types, arg_types_use_heap);
						free_alloca(arg_values, arg_values_use_heap);
						goto exit;
					}
					n++;
				} CLIB_HASH_FOREACH_END();
			}
		}
		ret_type = clib_ffi_get_type(CLIB_FFI_TYPE(type->func.ret_type));
		if (!ret_type) {
			clib_ffi_return_unsupported(type->func.ret_type);
			free_alloca(arg_types, arg_types_use_heap);
			free_alloca(arg_values, arg_values_use_heap);
			goto exit;
		}
		if (ffi_prep_cif(&cif, type->func.abi, arg_count, ret_type, arg_types) != FFI_OK) {
			clib_throw_error(clib_ffi_exception_ce, "Cannot prepare callback CIF");
			free_alloca(arg_types, arg_types_use_heap);
			free_alloca(arg_values, arg_values_use_heap);
			goto exit;
		}
	}

	ret = do_alloca(MAX(ret_type->size, sizeof(ffi_arg)), ret_use_heap);
	ffi_call(&cif, addr, ret, arg_values);

	for (n = 0; n < arg_count; n++) {
		if (arg_types[n]->type == FFI_TYPE_STRUCT) {
			efree(arg_types[n]);
		}
	}
	if (ret_type->type == FFI_TYPE_STRUCT) {
		efree(ret_type);
	}

	if (EX_NUM_ARGS()) {
		free_alloca(arg_types, arg_types_use_heap);
		free_alloca(arg_values, arg_values_use_heap);
	}

	clib_ffi_cdata_to_b_value(NULL, ret, CLIB_FFI_TYPE(type->func.ret_type), BP_VAR_R, return_value, 0, 1, 0);
	free_alloca(ret, ret_use_heap);

exit:
	b_obj_string_release(EX(func)->common.function_name);
	if (EX(func)->common.fn_flags & CLIB_ACC_CALL_VIA_TRAMPOLINE) {
		clib_free_trampoline(EX(func));
		EX(func) = NULL;
	}
}
/* }}} */

static clib_function *clib_ffi_get_func(clib_object **obj, b_obj_string *name, const b_value *key) /* {{{ */
{
	clib_ffi        *ffi = (clib_ffi*)*obj;
	clib_ffi_symbol *sym = NULL;
	clib_function   *func;
	clib_ffi_type   *type;

	if (ZSTR_LEN(name) == sizeof("new") -1
	 && (ZSTR_VAL(name)[0] == 'n' || ZSTR_VAL(name)[0] == 'N')
	 && (ZSTR_VAL(name)[1] == 'e' || ZSTR_VAL(name)[1] == 'E')
	 && (ZSTR_VAL(name)[2] == 'w' || ZSTR_VAL(name)[2] == 'W')) {
		return (clib_function*)&clib_ffi_new_fn;
	} else if (ZSTR_LEN(name) == sizeof("cast") -1
	 && (ZSTR_VAL(name)[0] == 'c' || ZSTR_VAL(name)[0] == 'C')
	 && (ZSTR_VAL(name)[1] == 'a' || ZSTR_VAL(name)[1] == 'A')
	 && (ZSTR_VAL(name)[2] == 's' || ZSTR_VAL(name)[2] == 'S')
	 && (ZSTR_VAL(name)[3] == 't' || ZSTR_VAL(name)[3] == 'T')) {
		return (clib_function*)&clib_ffi_cast_fn;
	} else if (ZSTR_LEN(name) == sizeof("type") -1
	 && (ZSTR_VAL(name)[0] == 't' || ZSTR_VAL(name)[0] == 'T')
	 && (ZSTR_VAL(name)[1] == 'y' || ZSTR_VAL(name)[1] == 'Y')
	 && (ZSTR_VAL(name)[2] == 'p' || ZSTR_VAL(name)[2] == 'P')
	 && (ZSTR_VAL(name)[3] == 'e' || ZSTR_VAL(name)[3] == 'E')) {
		return (clib_function*)&clib_ffi_type_fn;
	}

	if (ffi->symbols) {
		sym = clib_hash_find_ptr(ffi->symbols, name);
		if (sym && sym->kind != CLIB_FFI_SYM_FUNC) {
			sym = NULL;
		}
	}
	if (!sym) {
		clib_throw_error(clib_ffi_exception_ce, "Attempt to call undefined C function '%s'", ZSTR_VAL(name));
		return NULL;
	}

	type = CLIB_FFI_TYPE(sym->type);
	CLIB_ASSERT(type->kind == CLIB_FFI_TYPE_FUNC);

	if (EXPECTED(EG(trampoline).common.function_name == NULL)) {
		func = &EG(trampoline);
	} else {
		func = ecalloc(sizeof(clib_internal_function), 1);
	}
	func->common.type = CLIB_INTERNAL_FUNCTION;
	func->common.arg_flags[0] = 0;
	func->common.arg_flags[1] = 0;
	func->common.arg_flags[2] = 0;
	func->common.fn_flags = CLIB_ACC_CALL_VIA_TRAMPOLINE;
	func->common.function_name = b_obj_string_copy(name);
	/* set to 0 to avoid arg_info[] allocation, because all values are passed by value anyway */
	func->common.num_args = 0;
	func->common.required_num_args = type->func.args ? clib_hash_num_elements(type->func.args) : 0;
	func->common.scope = NULL;
	func->common.prototype = NULL;
	func->common.arg_info = NULL;
	func->internal_function.handler = CLIB_FN(ffi_trampoline);
	func->internal_function.module = NULL;

	func->internal_function.reserved[0] = type;
	func->internal_function.reserved[1] = sym->addr;

	return func;
}
/* }}} */

static clib_never_inline int clib_ffi_disabled(void) /* {{{ */
{
	clib_throw_error(clib_ffi_exception_ce, "FFI API is restricted by \"ffi.enable\" configuration directive");
	return 0;
}
/* }}} */

static clib_always_inline bool clib_ffi_validate_api_restriction(clib_execute_data *execute_data) /* {{{ */
{
	if (EXPECTED(FFI_G(restriction) > CLIB_FFI_ENABLED)) {
		CLIB_ASSERT(FFI_G(restriction) == CLIB_FFI_PRELOAD);
		if (FFI_G(is_cli)
		 || (execute_data->prev_execute_data
		  && (execute_data->prev_execute_data->func->common.fn_flags & CLIB_ACC_PRELOADED))
		 || (CG(compiler_options) & CLIB_COMPILE_PRELOAD)) {
			return 1;
		}
	} else if (EXPECTED(FFI_G(restriction) == CLIB_FFI_ENABLED)) {
		return 1;
	}
	return clib_ffi_disabled();
}
/* }}} */

#define CLIB_FFI_VALIDATE_API_RESTRICTION() do { \
		if (UNEXPECTED(!clib_ffi_validate_api_restriction(execute_data))) { \
			RETURN_THROWS(); \
		} \
	} while (0)

CLIB_METHOD(FFI, cdef) /* {{{ */
{
	b_obj_string *code = NULL;
	b_obj_string *lib = NULL;
	clib_ffi *ffi = NULL;
	DL_HANDLE handle = NULL;
	void *addr;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(0, 2)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR(code)
		Z_PARAM_STR_OR_NULL(lib)
	CLIB_PARSE_PARAMETERS_END();

	if (lib) {
		handle = DL_LOAD(ZSTR_VAL(lib));
		if (!handle) {
			clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s'", ZSTR_VAL(lib));
			RETURN_THROWS();
		}
#ifdef RTLD_DEFAULT
	} else if (1) {
		// TODO: this might need to be disabled or protected ???
		handle = RTLD_DEFAULT;
#endif
	}

	FFI_G(symbols) = NULL;
	FFI_G(tags) = NULL;

	if (code && ZSTR_LEN(code)) {
		/* Parse C definitions */
		FFI_G(default_type_attr) = CLIB_FFI_ATTR_STORED;

		if (clib_ffi_parse_decl(ZSTR_VAL(code), ZSTR_LEN(code)) == FAILURE) {
			if (FFI_G(symbols)) {
				clib_hash_destroy(FFI_G(symbols));
				efree(FFI_G(symbols));
				FFI_G(symbols) = NULL;
			}
			if (FFI_G(tags)) {
				clib_hash_destroy(FFI_G(tags));
				efree(FFI_G(tags));
				FFI_G(tags) = NULL;
			}
			RETURN_THROWS();
		}

		if (FFI_G(symbols)) {
			b_obj_string *name;
			clib_ffi_symbol *sym;

			CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(FFI_G(symbols), name, sym) {
				if (sym->kind == CLIB_FFI_SYM_VAR) {
					addr = DL_FETCH_SYMBOL(handle, ZSTR_VAL(name));
					if (!addr) {
						clib_throw_error(clib_ffi_exception_ce, "Failed resolving C variable '%s'", ZSTR_VAL(name));
						RETURN_THROWS();
					}
					sym->addr = addr;
				} else if (sym->kind == CLIB_FFI_SYM_FUNC) {
					b_obj_string *mangled_name = clib_ffi_mangled_func_name(name, CLIB_FFI_TYPE(sym->type));

					addr = DL_FETCH_SYMBOL(handle, ZSTR_VAL(mangled_name));
					b_obj_string_release(mangled_name);
					if (!addr) {
						clib_throw_error(clib_ffi_exception_ce, "Failed resolving C function '%s'", ZSTR_VAL(name));
						RETURN_THROWS();
					}
					sym->addr = addr;
				}
			} CLIB_HASH_FOREACH_END();
		}
	}

	ffi = (clib_ffi*)clib_ffi_new(clib_ffi_ce);
	ffi->lib = handle;
	ffi->symbols = FFI_G(symbols);
	ffi->tags = FFI_G(tags);

	FFI_G(symbols) = NULL;
	FFI_G(tags) = NULL;

	RETURN_OBJ(&ffi->std);
}
/* }}} */

static bool clib_ffi_same_types(clib_ffi_type *old, clib_ffi_type *type) /* {{{ */
{
	if (old == type) {
		return 1;
	}

	if (old->kind != type->kind
	 || old->size != type->size
	 || old->align != type->align
	 || old->attr != type->attr) {
		return 0;
	}

	switch (old->kind) {
		case CLIB_FFI_TYPE_ENUM:
			return old->enumeration.kind == type->enumeration.kind;
		case CLIB_FFI_TYPE_ARRAY:
			return old->array.length == type->array.length
			 &&	clib_ffi_same_types(CLIB_FFI_TYPE(old->array.type), CLIB_FFI_TYPE(type->array.type));
		case CLIB_FFI_TYPE_POINTER:
			return clib_ffi_same_types(CLIB_FFI_TYPE(old->pointer.type), CLIB_FFI_TYPE(type->pointer.type));
		case CLIB_FFI_TYPE_STRUCT:
			if (clib_hash_num_elements(&old->record.fields) != clib_hash_num_elements(&type->record.fields)) {
				return 0;
			} else {
				clib_ffi_field *old_field, *field;
				b_obj_string *key;
				Bucket *b = type->record.fields.arData;

				CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(&old->record.fields, key, old_field) {
					while (Z_TYPE(b->val) == IS_UNDEF) {
						b++;
					}
					if (key) {
						if (!b->key
						 || !b_obj_string_equals(key, b->key)) {
							return 0;
						}
					} else if (b->key) {
						return 0;
					}
					field = Z_PTR(b->val);
					if (old_field->offset != field->offset
					 || old_field->is_const != field->is_const
					 || old_field->is_nested != field->is_nested
					 || old_field->first_bit != field->first_bit
					 || old_field->bits != field->bits
					 || !clib_ffi_same_types(CLIB_FFI_TYPE(old_field->type), CLIB_FFI_TYPE(field->type))) {
						return 0;
					}
					b++;
				} CLIB_HASH_FOREACH_END();
			}
			break;
		case CLIB_FFI_TYPE_FUNC:
			if (old->func.abi != type->func.abi
			 || ((old->func.args ? clib_hash_num_elements(old->func.args) : 0) != (type->func.args ? clib_hash_num_elements(type->func.args) : 0))
			 || !clib_ffi_same_types(CLIB_FFI_TYPE(old->func.ret_type), CLIB_FFI_TYPE(type->func.ret_type))) {
				return 0;
			} else if (old->func.args) {
				clib_ffi_type *arg_type;
				b_value *zv = type->func.args->arPacked;

				CLIB_HASH_PACKED_FOREACH_PTR(old->func.args, arg_type) {
					while (Z_TYPE_P(zv) == IS_UNDEF) {
						zv++;
					}
					if (!clib_ffi_same_types(CLIB_FFI_TYPE(arg_type), CLIB_FFI_TYPE(Z_PTR_P(zv)))) {
						return 0;
					}
					zv++;
				} CLIB_HASH_FOREACH_END();
			}
			break;
		default:
			break;
	}

	return 1;
}
/* }}} */

static bool clib_ffi_same_symbols(clib_ffi_symbol *old, clib_ffi_symbol *sym) /* {{{ */
{
	if (old->kind != sym->kind || old->is_const != sym->is_const) {
		return 0;
	}

	if (old->kind == CLIB_FFI_SYM_CONST) {
		if (old->value != sym->value) {
			return 0;
		}
	}

	return clib_ffi_same_types(CLIB_FFI_TYPE(old->type), CLIB_FFI_TYPE(sym->type));
}
/* }}} */

static bool clib_ffi_same_tags(clib_ffi_tag *old, clib_ffi_tag *tag) /* {{{ */
{
	if (old->kind != tag->kind) {
		return 0;
	}

	return clib_ffi_same_types(CLIB_FFI_TYPE(old->type), CLIB_FFI_TYPE(tag->type));
}
/* }}} */

static bool clib_ffi_subst_old_type(clib_ffi_type **dcl, clib_ffi_type *old, clib_ffi_type *type) /* {{{ */
{
	clib_ffi_type *dcl_type;
	clib_ffi_field *field;

	if (CLIB_FFI_TYPE(*dcl) == type) {
		*dcl = old;
		return 1;
	}
	dcl_type = *dcl;
	switch (dcl_type->kind) {
		case CLIB_FFI_TYPE_POINTER:
			return clib_ffi_subst_old_type(&dcl_type->pointer.type, old, type);
		case CLIB_FFI_TYPE_ARRAY:
			return clib_ffi_subst_old_type(&dcl_type->array.type, old, type);
		case CLIB_FFI_TYPE_FUNC:
			if (clib_ffi_subst_old_type(&dcl_type->func.ret_type, old, type)) {
				return 1;
			}
			if (dcl_type->func.args) {
				b_value *zv;

				CLIB_HASH_PACKED_FOREACH_VAL(dcl_type->func.args, zv) {
					if (clib_ffi_subst_old_type((clib_ffi_type**)&Z_PTR_P(zv), old, type)) {
						return 1;
					}
				} CLIB_HASH_FOREACH_END();
			}
			break;
		case CLIB_FFI_TYPE_STRUCT:
			CLIB_HASH_MAP_FOREACH_PTR(&dcl_type->record.fields, field) {
				if (clib_ffi_subst_old_type(&field->type, old, type)) {
					return 1;
				}
			} CLIB_HASH_FOREACH_END();
			break;
		default:
			break;
	}
	return 0;
} /* }}} */

static void clib_ffi_cleanup_type(clib_ffi_type *old, clib_ffi_type *type) /* {{{ */
{
	clib_ffi_symbol *sym;
	clib_ffi_tag *tag;

	if (FFI_G(symbols)) {
		CLIB_HASH_MAP_FOREACH_PTR(FFI_G(symbols), sym) {
			clib_ffi_subst_old_type(&sym->type, old, type);
		} CLIB_HASH_FOREACH_END();
	}
	if (FFI_G(tags)) {
		CLIB_HASH_MAP_FOREACH_PTR(FFI_G(tags), tag) {
			clib_ffi_subst_old_type(&tag->type, old, type);
		} CLIB_HASH_FOREACH_END();
	}
}
/* }}} */

static clib_ffi_type *clib_ffi_remember_type(clib_ffi_type *type) /* {{{ */
{
	if (!FFI_G(weak_types)) {
		FFI_G(weak_types) = emalloc(sizeof(b_obj_dict));
		clib_hash_init(FFI_G(weak_types), 0, NULL, clib_ffi_type_hash_dtor, 0);
	}
	// TODO: avoid dups ???
	type->attr |= CLIB_FFI_ATTR_STORED;
	clib_hash_next_index_insert_ptr(FFI_G(weak_types), CLIB_FFI_TYPE_MAKE_OWNED(type));
	return type;
}
/* }}} */

static clib_ffi *clib_ffi_load(const char *filename, bool preload) /* {{{ */
{
	struct stat buf;
	int fd;
	char *code, *code_pos, *scope_name, *lib;
	size_t code_size, scope_name_len;
	clib_ffi *ffi;
	DL_HANDLE handle = NULL;
	clib_ffi_scope *scope = NULL;
	b_obj_string *name;
	clib_ffi_symbol *sym;
	clib_ffi_tag *tag;
	void *addr;

	if (stat(filename, &buf) != 0) {
		if (preload) {
			clib_error(E_WARNING, "FFI: failed pre-loading '%s', file doesn't exist", filename);
		} else {
			clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', file doesn't exist", filename);
		}
		return NULL;
	}

	if ((buf.st_mode & S_IFMT) != S_IFREG) {
		if (preload) {
			clib_error(E_WARNING, "FFI: failed pre-loading '%s', not a regular file", filename);
		} else {
			clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', not a regular file", filename);
		}
		return NULL;
	}

	code_size = buf.st_size;
	code = emalloc(code_size + 1);
	fd = open(filename, O_RDONLY, 0);
	if (fd < 0 || read(fd, code, code_size) != code_size) {
		if (preload) {
			clib_error(E_WARNING, "FFI: Failed pre-loading '%s', cannot read_file", filename);
		} else {
			clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', cannot read_file", filename);
		}
		efree(code);
		close(fd);
		return NULL;
	}
	close(fd);
	code[code_size] = 0;

	FFI_G(symbols) = NULL;
	FFI_G(tags) = NULL;
	FFI_G(persistent) = preload;
	FFI_G(default_type_attr) = preload ?
		CLIB_FFI_ATTR_STORED | CLIB_FFI_ATTR_PERSISTENT :
		CLIB_FFI_ATTR_STORED;

	scope_name = NULL;
	scope_name_len = 0;
	lib = NULL;
	code_pos = clib_ffi_parse_directives(filename, code, &scope_name, &lib, preload);
	if (!code_pos) {
		efree(code);
		FFI_G(persistent) = 0;
		return NULL;
	}
	code_size -= code_pos - code;

	if (clib_ffi_parse_decl(code_pos, code_size) == FAILURE) {
		if (preload) {
			clib_error(E_WARNING, "FFI: failed pre-loading '%s'", filename);
		} else {
			clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s'", filename);
		}
		goto cleanup;
	}

	if (lib) {
		handle = DL_LOAD(lib);
		if (!handle) {
			if (preload) {
				clib_error(E_WARNING, "FFI: Failed pre-loading '%s'", lib);
			} else {
				clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s'", lib);
			}
			goto cleanup;
		}
#ifdef RTLD_DEFAULT
	} else if (1) {
		// TODO: this might need to be disabled or protected ???
		handle = RTLD_DEFAULT;
#endif
	}

	if (preload) {
		if (!scope_name) {
			scope_name = "C";
		}
		scope_name_len = strlen(scope_name);
		if (FFI_G(scopes)) {
			scope = clib_hash_str_find_ptr(FFI_G(scopes), scope_name, scope_name_len);
		}
	}

	if (FFI_G(symbols)) {
		CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(FFI_G(symbols), name, sym) {
			if (sym->kind == CLIB_FFI_SYM_VAR) {
				addr = DL_FETCH_SYMBOL(handle, ZSTR_VAL(name));
				if (!addr) {
					if (preload) {
						clib_error(E_WARNING, "FFI: failed pre-loading '%s', cannot resolve C variable '%s'", filename, ZSTR_VAL(name));
					} else {
						clib_throw_error(clib_ffi_exception_ce, "Failed resolving C variable '%s'", ZSTR_VAL(name));
					}
					if (lib) {
						DL_UNLOAD(handle);
					}
					goto cleanup;
				}
				sym->addr = addr;
			} else if (sym->kind == CLIB_FFI_SYM_FUNC) {
				b_obj_string *mangled_name = clib_ffi_mangled_func_name(name, CLIB_FFI_TYPE(sym->type));

				addr = DL_FETCH_SYMBOL(handle, ZSTR_VAL(mangled_name));
				b_obj_string_release(mangled_name);
				if (!addr) {
					if (preload) {
						clib_error(E_WARNING, "failed pre-loading '%s', cannot resolve C function '%s'", filename, ZSTR_VAL(name));
					} else {
						clib_throw_error(clib_ffi_exception_ce, "Failed resolving C function '%s'", ZSTR_VAL(name));
					}
					if (lib) {
						DL_UNLOAD(handle);
					}
					goto cleanup;
				}
				sym->addr = addr;
			}
			if (scope && scope->symbols) {
				clib_ffi_symbol *old_sym = clib_hash_find_ptr(scope->symbols, name);

				if (old_sym) {
					if (clib_ffi_same_symbols(old_sym, sym)) {
						if (CLIB_FFI_TYPE_IS_OWNED(sym->type)
						 && CLIB_FFI_TYPE(old_sym->type) != CLIB_FFI_TYPE(sym->type)) {
							clib_ffi_type *type = CLIB_FFI_TYPE(sym->type);
							clib_ffi_cleanup_type(CLIB_FFI_TYPE(old_sym->type), CLIB_FFI_TYPE(type));
							clib_ffi_type_dtor(type);
						}
					} else {
						clib_error(E_WARNING, "FFI: failed pre-loading '%s', redefinition of '%s'", filename, ZSTR_VAL(name));
						if (lib) {
							DL_UNLOAD(handle);
						}
						goto cleanup;
					}
				}
			}
		} CLIB_HASH_FOREACH_END();
	}

	if (preload) {
		if (scope && scope->tags && FFI_G(tags)) {
			CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(FFI_G(tags), name, tag) {
				clib_ffi_tag *old_tag = clib_hash_find_ptr(scope->tags, name);

				if (old_tag) {
					if (clib_ffi_same_tags(old_tag, tag)) {
						if (CLIB_FFI_TYPE_IS_OWNED(tag->type)
						 && CLIB_FFI_TYPE(old_tag->type) != CLIB_FFI_TYPE(tag->type)) {
							clib_ffi_type *type = CLIB_FFI_TYPE(tag->type);
							clib_ffi_cleanup_type(CLIB_FFI_TYPE(old_tag->type), CLIB_FFI_TYPE(type));
							clib_ffi_type_dtor(type);
						}
					} else {
						clib_error(E_WARNING, "FFI: failed pre-loading '%s', redefinition of '%s %s'", filename, clib_ffi_tag_kind_name[tag->kind], ZSTR_VAL(name));
						if (lib) {
							DL_UNLOAD(handle);
						}
						goto cleanup;
					}
				}
			} CLIB_HASH_FOREACH_END();
		}

		if (!scope) {
			scope = malloc(sizeof(clib_ffi_scope));
			scope->symbols = FFI_G(symbols);
			scope->tags = FFI_G(tags);

			if (!FFI_G(scopes)) {
				FFI_G(scopes) = malloc(sizeof(b_obj_dict));
				clib_hash_init(FFI_G(scopes), 0, NULL, clib_ffi_scope_hash_dtor, 1);
			}

			clib_hash_str_add_ptr(FFI_G(scopes), scope_name, scope_name_len, scope);
		} else {
			if (FFI_G(symbols)) {
				if (!scope->symbols) {
					scope->symbols = FFI_G(symbols);
					FFI_G(symbols) = NULL;
				} else {
					CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(FFI_G(symbols), name, sym) {
						if (!clib_hash_add_ptr(scope->symbols, name, sym)) {
							clib_ffi_type_dtor(sym->type);
							free(sym);
						}
					} CLIB_HASH_FOREACH_END();
					FFI_G(symbols)->pDestructor = NULL;
					clib_hash_destroy(FFI_G(symbols));
				}
			}
			if (FFI_G(tags)) {
				if (!scope->tags) {
					scope->tags = FFI_G(tags);
					FFI_G(tags) = NULL;
				} else {
					CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(FFI_G(tags), name, tag) {
						if (!clib_hash_add_ptr(scope->tags, name, tag)) {
							clib_ffi_type_dtor(tag->type);
							free(tag);
						}
					} CLIB_HASH_FOREACH_END();
					FFI_G(tags)->pDestructor = NULL;
					clib_hash_destroy(FFI_G(tags));
				}
			}
		}

		if (EG(objects_store).object_buckets) {
			ffi = (clib_ffi*)clib_ffi_new(clib_ffi_ce);
		} else {
			ffi = ecalloc(1, sizeof(clib_ffi));
		}
		ffi->symbols = scope->symbols;
		ffi->tags = scope->tags;
		ffi->persistent = 1;
	} else {
		ffi = (clib_ffi*)clib_ffi_new(clib_ffi_ce);
		ffi->lib = handle;
		ffi->symbols = FFI_G(symbols);
		ffi->tags = FFI_G(tags);
	}

	efree(code);
	FFI_G(symbols) = NULL;
	FFI_G(tags) = NULL;
	FFI_G(persistent) = 0;

	return ffi;

cleanup:
	efree(code);
	if (FFI_G(symbols)) {
		clib_hash_destroy(FFI_G(symbols));
		pefree(FFI_G(symbols), preload);
		FFI_G(symbols) = NULL;
	}
	if (FFI_G(tags)) {
		clib_hash_destroy(FFI_G(tags));
		pefree(FFI_G(tags), preload);
		FFI_G(tags) = NULL;
	}
	FFI_G(persistent) = 0;
	return NULL;
}
/* }}} */

CLIB_METHOD(FFI, load) /* {{{ */
{
	b_obj_string *fn;
	clib_ffi *ffi;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(fn)
	CLIB_PARSE_PARAMETERS_END();

	if (CG(compiler_options) & CLIB_COMPILE_PRELOAD_IN_CHILD) {
		clib_throw_error(clib_ffi_exception_ce, "FFI::load() doesn't work in conjunction with \"opcache.preload_user\". Use \"ffi.preload\" instead.");
		RETURN_THROWS();
	}

	ffi = clib_ffi_load(ZSTR_VAL(fn), (CG(compiler_options) & CLIB_COMPILE_PRELOAD) != 0);

	if (ffi) {
		RETURN_OBJ(&ffi->std);
	}
}
/* }}} */

CLIB_METHOD(FFI, scope) /* {{{ */
{
	b_obj_string *scope_name;
	clib_ffi_scope *scope = NULL;
	clib_ffi *ffi;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(scope_name)
	CLIB_PARSE_PARAMETERS_END();

	if (FFI_G(scopes)) {
		scope = clib_hash_find_ptr(FFI_G(scopes), scope_name);
	}

	if (!scope) {
		clib_throw_error(clib_ffi_exception_ce, "Failed loading scope '%s'", ZSTR_VAL(scope_name));
		RETURN_THROWS();
	}

	ffi = (clib_ffi*)clib_ffi_new(clib_ffi_ce);

	ffi->symbols = scope->symbols;
	ffi->tags = scope->tags;
	ffi->persistent = 1;

	RETURN_OBJ(&ffi->std);
}
/* }}} */

static void clib_ffi_cleanup_dcl(clib_ffi_dcl *dcl) /* {{{ */
{
	if (dcl) {
		clib_ffi_type_dtor(dcl->type);
		dcl->type = NULL;
	}
}
/* }}} */

static void clib_ffi_throw_parser_error(const char *format, ...) /* {{{ */
{
	va_list va;
	char *message = NULL;

	va_start(va, format);
	clib_vspprintf(&message, 0, format, va);

	if (EG(current_execute_data)) {
		clib_throw_exception(clib_ffi_parser_exception_ce, message, 0);
	} else {
		clib_error(E_WARNING, "FFI Parser: %s", message);
	}

	efree(message);
	va_end(va);
}
/* }}} */

static clib_result clib_ffi_validate_vla(clib_ffi_type *type) /* {{{ */
{
	if (!FFI_G(allow_vla) && (type->attr & CLIB_FFI_ATTR_VLA)) {
		clib_ffi_throw_parser_error("\"[*]\" is not allowed in other than function prototype scope at line %d", FFI_G(line));
		return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

static clib_result clib_ffi_validate_incomplete_type(clib_ffi_type *type, bool allow_incomplete_tag, bool allow_incomplete_array) /* {{{ */
{
	if (!allow_incomplete_tag && (type->attr & CLIB_FFI_ATTR_INCOMPLETE_TAG)) {
		if (FFI_G(tags)) {
			b_obj_string *key;
			clib_ffi_tag *tag;

			CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(FFI_G(tags), key, tag) {
				if (CLIB_FFI_TYPE(tag->type) == type) {
					if (type->kind == CLIB_FFI_TYPE_ENUM) {
						clib_ffi_throw_parser_error("Incomplete enum \"%s\" at line %d", ZSTR_VAL(key), FFI_G(line));
					} else if (type->attr & CLIB_FFI_ATTR_UNION) {
						clib_ffi_throw_parser_error("Incomplete union \"%s\" at line %d", ZSTR_VAL(key), FFI_G(line));
					} else {
						clib_ffi_throw_parser_error("Incomplete struct \"%s\" at line %d", ZSTR_VAL(key), FFI_G(line));
					}
					return FAILURE;
				}
			} CLIB_HASH_FOREACH_END();
		}
		if (FFI_G(symbols)) {
			b_obj_string *key;
			clib_ffi_symbol *sym;

			CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(FFI_G(symbols), key, sym) {
				if (type == CLIB_FFI_TYPE(sym->type)) {
					clib_ffi_throw_parser_error("Incomplete C type %s at line %d", ZSTR_VAL(key), FFI_G(line));
					return FAILURE;
				}
			} CLIB_HASH_FOREACH_END();
		}
		clib_ffi_throw_parser_error("Incomplete type at line %d", FFI_G(line));
		return FAILURE;
	} else if (!allow_incomplete_array && (type->attr & CLIB_FFI_ATTR_INCOMPLETE_ARRAY)) {
		clib_ffi_throw_parser_error("\"[]\" is not allowed at line %d", FFI_G(line));
		return FAILURE;
	} else if (!FFI_G(allow_vla) && (type->attr & CLIB_FFI_ATTR_VLA)) {
		clib_ffi_throw_parser_error("\"[*]\" is not allowed in other than function prototype scope at line %d", FFI_G(line));
		return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

static clib_result clib_ffi_validate_type(clib_ffi_type *type, bool allow_incomplete_tag, bool allow_incomplete_array) /* {{{ */
{
	if (type->kind == CLIB_FFI_TYPE_VOID) {
		clib_ffi_throw_parser_error("void type is not allowed at line %d", FFI_G(line));
		return FAILURE;
	}
	return clib_ffi_validate_incomplete_type(type, allow_incomplete_tag, allow_incomplete_array);
}
/* }}} */

static clib_result clib_ffi_validate_var_type(clib_ffi_type *type, bool allow_incomplete_array) /* {{{ */
{
	if (type->kind == CLIB_FFI_TYPE_FUNC) {
		clib_ffi_throw_parser_error("function type is not allowed at line %d", FFI_G(line));
		return FAILURE;
	}
	return clib_ffi_validate_type(type, 0, allow_incomplete_array);
}
/* }}} */

void clib_ffi_validate_type_name(clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_finalize_type(dcl);
	if (clib_ffi_validate_var_type(CLIB_FFI_TYPE(dcl->type), 0) == FAILURE) {
		clib_ffi_cleanup_dcl(dcl);
		LONGJMP(FFI_G(bailout), FAILURE);
	}
}
/* }}} */

static bool clib_ffi_subst_type(clib_ffi_type **dcl, clib_ffi_type *type) /* {{{ */
{
	clib_ffi_type *dcl_type;
	clib_ffi_field *field;

	if (*dcl == type) {
		*dcl = CLIB_FFI_TYPE_MAKE_OWNED(type);
		return 1;
	}
	dcl_type = *dcl;
	switch (dcl_type->kind) {
		case CLIB_FFI_TYPE_POINTER:
			return clib_ffi_subst_type(&dcl_type->pointer.type, type);
		case CLIB_FFI_TYPE_ARRAY:
			return clib_ffi_subst_type(&dcl_type->array.type, type);
		case CLIB_FFI_TYPE_FUNC:
			if (clib_ffi_subst_type(&dcl_type->func.ret_type, type)) {
				return 1;
			}
			if (dcl_type->func.args) {
				b_value *zv;

				CLIB_HASH_PACKED_FOREACH_VAL(dcl_type->func.args, zv) {
					if (clib_ffi_subst_type((clib_ffi_type**)&Z_PTR_P(zv), type)) {
						return 1;
					}
				} CLIB_HASH_FOREACH_END();
			}
			break;
		case CLIB_FFI_TYPE_STRUCT:
			CLIB_HASH_MAP_FOREACH_PTR(&dcl_type->record.fields, field) {
				if (clib_ffi_subst_type(&field->type, type)) {
					return 1;
				}
			} CLIB_HASH_FOREACH_END();
			break;
		default:
			break;
	}
	return 0;
} /* }}} */

static void clib_ffi_tags_cleanup(clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_tag *tag;
	CLIB_HASH_MAP_FOREACH_PTR(FFI_G(tags), tag) {
		if (CLIB_FFI_TYPE_IS_OWNED(tag->type)) {
			clib_ffi_type *type = CLIB_FFI_TYPE(tag->type);
			clib_ffi_subst_type(&dcl->type, type);
			tag->type = type;
		}
	} CLIB_HASH_FOREACH_END();
	clib_hash_destroy(FFI_G(tags));
	efree(FFI_G(tags));
}
/* }}} */

CLIB_METHOD(FFI, new) /* {{{ */
{
	b_obj_string *type_def = NULL;
	clib_object *type_obj = NULL;
	clib_ffi_type *type, *type_ptr;
	clib_ffi_cdata *cdata;
	void *ptr;
	bool owned = 1;
	bool persistent = 0;
	bool is_const = 0;
	clib_ffi_flags flags = CLIB_FFI_FLAG_OWNED;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 3)
		Z_PARAM_OBJ_OF_CLASS_OR_STR(type_obj, clib_ffi_ctype_ce, type_def)
		Z_PARAM_OPTIONAL
		Z_PARAM_BOOL(owned)
		Z_PARAM_BOOL(persistent)
	CLIB_PARSE_PARAMETERS_END();

	if (!owned) {
		flags &= ~CLIB_FFI_FLAG_OWNED;
	}

	if (persistent) {
		flags |= CLIB_FFI_FLAG_PERSISTENT;
	}

	if (type_def) {
		clib_ffi_dcl dcl = CLIB_FFI_ATTR_INIT;

		if (Z_TYPE(EX(This)) == IS_OBJECT) {
			clib_ffi *ffi = (clib_ffi*)Z_OBJ(EX(This));
			FFI_G(symbols) = ffi->symbols;
			FFI_G(tags) = ffi->tags;
		} else {
			FFI_G(symbols) = NULL;
			FFI_G(tags) = NULL;
		}

		FFI_G(default_type_attr) = 0;

		if (clib_ffi_parse_type(ZSTR_VAL(type_def), ZSTR_LEN(type_def), &dcl) == FAILURE) {
			clib_ffi_type_dtor(dcl.type);
			if (Z_TYPE(EX(This)) != IS_OBJECT) {
				if (FFI_G(tags)) {
					clib_hash_destroy(FFI_G(tags));
					efree(FFI_G(tags));
					FFI_G(tags) = NULL;
				}
				if (FFI_G(symbols)) {
					clib_hash_destroy(FFI_G(symbols));
					efree(FFI_G(symbols));
					FFI_G(symbols) = NULL;
				}
			}
			return;
		}

		type = CLIB_FFI_TYPE(dcl.type);
		if (dcl.attr & CLIB_FFI_ATTR_CONST) {
			is_const = 1;
		}

		if (Z_TYPE(EX(This)) != IS_OBJECT) {
			if (FFI_G(tags)) {
				clib_ffi_tags_cleanup(&dcl);
			}
			if (FFI_G(symbols)) {
				clib_hash_destroy(FFI_G(symbols));
				efree(FFI_G(symbols));
				FFI_G(symbols) = NULL;
			}
		}
		FFI_G(symbols) = NULL;
		FFI_G(tags) = NULL;

		type_ptr = dcl.type;
	} else {
		clib_ffi_ctype *ctype = (clib_ffi_ctype*) type_obj;

		type_ptr = type = ctype->type;
		if (CLIB_FFI_TYPE_IS_OWNED(type)) {
			type = CLIB_FFI_TYPE(type);
			if (!(type->attr & CLIB_FFI_ATTR_STORED)) {
				if (GC_REFCOUNT(&ctype->std) == 1) {
					/* transfer type ownership */
					ctype->type = type;
				} else {
					ctype->type = type_ptr = type = clib_ffi_remember_type(type);
				}
			}
		}
	}

	if (type->size == 0) {
		clib_throw_error(clib_ffi_exception_ce, "Cannot instantiate FFI\\CData of zero size");
		clib_ffi_type_dtor(type_ptr);
		return;
	}

	ptr = pemalloc(type->size, flags & CLIB_FFI_FLAG_PERSISTENT);
	memset(ptr, 0, type->size);

	cdata = (clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);
	if (type->kind < CLIB_FFI_TYPE_POINTER) {
		cdata->std.handlers = &clib_ffi_cdata_value_handlers;
	}
	cdata->type = type_ptr;
	cdata->ptr = ptr;
	cdata->flags = flags;
	if (is_const) {
		cdata->flags |= CLIB_FFI_FLAG_CONST;
	}

	RETURN_OBJ(&cdata->std);
}
/* }}} */

CLIB_METHOD(FFI, free) /* {{{ */
{
	b_value *zv;
	clib_ffi_cdata *cdata;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(zv, clib_ffi_cdata_ce, 0, 1);
	CLIB_PARSE_PARAMETERS_END();

	cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);

	if (CLIB_FFI_TYPE(cdata->type)->kind == CLIB_FFI_TYPE_POINTER) {
		if (!cdata->ptr) {
			clib_throw_error(clib_ffi_exception_ce, "NULL pointer dereference");
			RETURN_THROWS();
		}
		if (cdata->ptr != (void*)&cdata->ptr_holder) {
			pefree(*(void**)cdata->ptr, cdata->flags & CLIB_FFI_FLAG_PERSISTENT);
		} else {
			pefree(cdata->ptr_holder, (cdata->flags & CLIB_FFI_FLAG_PERSISTENT) || !is_clib_ptr(cdata->ptr_holder));
		}
		*(void**)cdata->ptr = NULL;
	} else if (!(cdata->flags & CLIB_FFI_FLAG_OWNED)) {
		pefree(cdata->ptr, cdata->flags & CLIB_FFI_FLAG_PERSISTENT);
		cdata->ptr = NULL;
		cdata->flags &= ~(CLIB_FFI_FLAG_OWNED|CLIB_FFI_FLAG_PERSISTENT);
		cdata->std.handlers = &clib_ffi_cdata_free_handlers;
	} else {
		clib_throw_error(clib_ffi_exception_ce, "free() non a C pointer");
	}
}
/* }}} */

CLIB_METHOD(FFI, cast) /* {{{ */
{
	b_obj_string *type_def = NULL;
	clib_object *ztype = NULL;
	clib_ffi_type *old_type, *type, *type_ptr;
	clib_ffi_cdata *old_cdata, *cdata;
	bool is_const = 0;
	b_value *zv, *arg;
	void *ptr;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_OBJ_OF_CLASS_OR_STR(ztype, clib_ffi_ctype_ce, type_def)
		Z_PARAM_ZVAL(zv)
	CLIB_PARSE_PARAMETERS_END();

	arg = zv;
	ZVAL_DEREF(zv);

	if (type_def) {
		clib_ffi_dcl dcl = CLIB_FFI_ATTR_INIT;

		if (Z_TYPE(EX(This)) == IS_OBJECT) {
			clib_ffi *ffi = (clib_ffi*)Z_OBJ(EX(This));
			FFI_G(symbols) = ffi->symbols;
			FFI_G(tags) = ffi->tags;
		} else {
			FFI_G(symbols) = NULL;
			FFI_G(tags) = NULL;
		}

		FFI_G(default_type_attr) = 0;

		if (clib_ffi_parse_type(ZSTR_VAL(type_def), ZSTR_LEN(type_def), &dcl) == FAILURE) {
			clib_ffi_type_dtor(dcl.type);
			if (Z_TYPE(EX(This)) != IS_OBJECT) {
				if (FFI_G(tags)) {
					clib_hash_destroy(FFI_G(tags));
					efree(FFI_G(tags));
					FFI_G(tags) = NULL;
				}
				if (FFI_G(symbols)) {
					clib_hash_destroy(FFI_G(symbols));
					efree(FFI_G(symbols));
					FFI_G(symbols) = NULL;
				}
			}
			return;
		}

		type = CLIB_FFI_TYPE(dcl.type);
		if (dcl.attr & CLIB_FFI_ATTR_CONST) {
			is_const = 1;
		}

		if (Z_TYPE(EX(This)) != IS_OBJECT) {
			if (FFI_G(tags)) {
				clib_ffi_tags_cleanup(&dcl);
			}
			if (FFI_G(symbols)) {
				clib_hash_destroy(FFI_G(symbols));
				efree(FFI_G(symbols));
				FFI_G(symbols) = NULL;
			}
		}
		FFI_G(symbols) = NULL;
		FFI_G(tags) = NULL;

		type_ptr = dcl.type;
	} else {
		clib_ffi_ctype *ctype = (clib_ffi_ctype*) ztype;

		type_ptr = type = ctype->type;
		if (CLIB_FFI_TYPE_IS_OWNED(type)) {
			type = CLIB_FFI_TYPE(type);
			if (!(type->attr & CLIB_FFI_ATTR_STORED)) {
				if (GC_REFCOUNT(&ctype->std) == 1) {
					/* transfer type ownership */
					ctype->type = type;
				} else {
					ctype->type = type_ptr = type = clib_ffi_remember_type(type);
				}
			}
		}
	}

	if (Z_TYPE_P(zv) != IS_OBJECT || Z_OBJCE_P(zv) != clib_ffi_cdata_ce) {
		if (type->kind < CLIB_FFI_TYPE_POINTER && Z_TYPE_P(zv) < IS_STRING) {
			/* numeric conversion */
			cdata = (clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);
			cdata->std.handlers = &clib_ffi_cdata_value_handlers;
			cdata->type = type_ptr;
			cdata->ptr = emalloc(type->size);
			clib_ffi_b_value_to_cdata(cdata->ptr, type, zv);
			cdata->flags = CLIB_FFI_FLAG_OWNED;
			if (is_const) {
				cdata->flags |= CLIB_FFI_FLAG_CONST;
			}
			RETURN_OBJ(&cdata->std);
		} else if (type->kind == CLIB_FFI_TYPE_POINTER && Z_TYPE_P(zv) == IS_LONG) {
			/* number to pointer conversion */
			cdata = (clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);
			cdata->type = type_ptr;
			cdata->ptr = &cdata->ptr_holder;
			cdata->ptr_holder = (void*)(intptr_t)Z_LVAL_P(zv);
			if (is_const) {
				cdata->flags |= CLIB_FFI_FLAG_CONST;
			}
			RETURN_OBJ(&cdata->std);
		} else if (type->kind == CLIB_FFI_TYPE_POINTER && Z_TYPE_P(zv) == IS_NULL) {
			/* null -> pointer */
			cdata = (clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);
			cdata->type = type_ptr;
			cdata->ptr = &cdata->ptr_holder;
			cdata->ptr_holder = NULL;
			if (is_const) {
				cdata->flags |= CLIB_FFI_FLAG_CONST;
			}
			RETURN_OBJ(&cdata->std);
		} else {
			clib_wrong_parameter_class_error(2, "FFI\\CData", zv);
			RETURN_THROWS();
		}
	}

	old_cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);
	old_type = CLIB_FFI_TYPE(old_cdata->type);
	ptr = old_cdata->ptr;

	cdata = (clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);
	if (type->kind < CLIB_FFI_TYPE_POINTER) {
		cdata->std.handlers = &clib_ffi_cdata_value_handlers;
	}
	cdata->type = type_ptr;

	if (old_type->kind == CLIB_FFI_TYPE_POINTER
	 && type->kind != CLIB_FFI_TYPE_POINTER
	 && CLIB_FFI_TYPE(old_type->pointer.type)->kind == CLIB_FFI_TYPE_VOID) {
		/* automatically dereference void* pointers ??? */
		cdata->ptr = *(void**)ptr;
	} else if (old_type->kind == CLIB_FFI_TYPE_ARRAY
	 && type->kind == CLIB_FFI_TYPE_POINTER
	 && clib_ffi_is_compatible_type(CLIB_FFI_TYPE(old_type->array.type), CLIB_FFI_TYPE(type->pointer.type))) {		cdata->ptr = &cdata->ptr_holder;
 		cdata->ptr = &cdata->ptr_holder;
 		cdata->ptr_holder = old_cdata->ptr;
	} else if (old_type->kind == CLIB_FFI_TYPE_POINTER
	 && type->kind == CLIB_FFI_TYPE_ARRAY
	 && clib_ffi_is_compatible_type(CLIB_FFI_TYPE(old_type->pointer.type), CLIB_FFI_TYPE(type->array.type))) {
		cdata->ptr = old_cdata->ptr_holder;
	} else if (type->size > old_type->size) {
		clib_object_release(&cdata->std);
		clib_throw_error(clib_ffi_exception_ce, "attempt to cast to larger type");
		RETURN_THROWS();
	} else if (ptr != &old_cdata->ptr_holder) {
		cdata->ptr = ptr;
	} else {
		cdata->ptr = &cdata->ptr_holder;
		cdata->ptr_holder = old_cdata->ptr_holder;
	}
	if (is_const) {
		cdata->flags |= CLIB_FFI_FLAG_CONST;
	}

	if (old_cdata->flags & CLIB_FFI_FLAG_OWNED) {
		if (GC_REFCOUNT(&old_cdata->std) == 1 && Z_REFCOUNT_P(arg) == 1) {
			/* transfer ownership */
			old_cdata->flags &= ~CLIB_FFI_FLAG_OWNED;
			cdata->flags |= CLIB_FFI_FLAG_OWNED;
		} else {
			//???clib_throw_error(clib_ffi_exception_ce, "Attempt to cast owned C pointer");
		}
	}

	RETURN_OBJ(&cdata->std);
}
/* }}} */

CLIB_METHOD(FFI, type) /* {{{ */
{
	clib_ffi_ctype *ctype;
	clib_ffi_dcl dcl = CLIB_FFI_ATTR_INIT;
	b_obj_string *type_def;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(type_def);
	CLIB_PARSE_PARAMETERS_END();

	if (Z_TYPE(EX(This)) == IS_OBJECT) {
		clib_ffi *ffi = (clib_ffi*)Z_OBJ(EX(This));
		FFI_G(symbols) = ffi->symbols;
		FFI_G(tags) = ffi->tags;
	} else {
		FFI_G(symbols) = NULL;
		FFI_G(tags) = NULL;
	}

	FFI_G(default_type_attr) = 0;

	if (clib_ffi_parse_type(ZSTR_VAL(type_def), ZSTR_LEN(type_def), &dcl) == FAILURE) {
		clib_ffi_type_dtor(dcl.type);
		if (Z_TYPE(EX(This)) != IS_OBJECT) {
			if (FFI_G(tags)) {
				clib_hash_destroy(FFI_G(tags));
				efree(FFI_G(tags));
				FFI_G(tags) = NULL;
			}
			if (FFI_G(symbols)) {
				clib_hash_destroy(FFI_G(symbols));
				efree(FFI_G(symbols));
				FFI_G(symbols) = NULL;
			}
		}
		return;
	}

	if (Z_TYPE(EX(This)) != IS_OBJECT) {
		if (FFI_G(tags)) {
			clib_ffi_tags_cleanup(&dcl);
		}
		if (FFI_G(symbols)) {
			clib_hash_destroy(FFI_G(symbols));
			efree(FFI_G(symbols));
			FFI_G(symbols) = NULL;
		}
	}
	FFI_G(symbols) = NULL;
	FFI_G(tags) = NULL;

	ctype = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ctype->type = dcl.type;

	RETURN_OBJ(&ctype->std);
}
/* }}} */

CLIB_METHOD(FFI, typeof) /* {{{ */
{
	b_value *zv, *arg;
	clib_ffi_ctype *ctype;
	clib_ffi_type *type;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ZVAL(zv);
	CLIB_PARSE_PARAMETERS_END();

	arg = zv;
	ZVAL_DEREF(zv);
	if (Z_TYPE_P(zv) == IS_OBJECT && Z_OBJCE_P(zv) == clib_ffi_cdata_ce) {
		clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);

		type = cdata->type;
		if (CLIB_FFI_TYPE_IS_OWNED(type)) {
			type = CLIB_FFI_TYPE(type);
			if (!(type->attr & CLIB_FFI_ATTR_STORED)) {
				if (GC_REFCOUNT(&cdata->std) == 1 && Z_REFCOUNT_P(arg) == 1) {
					/* transfer type ownership */
					cdata->type = type;
					type = CLIB_FFI_TYPE_MAKE_OWNED(type);
				} else {
					cdata->type = type = clib_ffi_remember_type(type);
				}
			}
		}
	} else {
		clib_wrong_parameter_class_error(1, "FFI\\CData", zv);
		RETURN_THROWS();
	}

	ctype = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ctype->type = type;

	RETURN_OBJ(&ctype->std);
}
/* }}} */

CLIB_METHOD(FFI, arrayType) /* {{{ */
{
	b_value *ztype;
	clib_ffi_ctype *ctype;
	clib_ffi_type *type;
	b_obj_dict *dims;
	b_value *val;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_OBJECT_OF_CLASS(ztype, clib_ffi_ctype_ce)
		Z_PARAM_ARRAY_HT(dims)
	CLIB_PARSE_PARAMETERS_END();

	ctype = (clib_ffi_ctype*)Z_OBJ_P(ztype);
	type = CLIB_FFI_TYPE(ctype->type);

	if (type->kind == CLIB_FFI_TYPE_FUNC) {
		clib_throw_error(clib_ffi_exception_ce, "Array of functions is not allowed");
		RETURN_THROWS();
	} else if (type->kind == CLIB_FFI_TYPE_ARRAY && (type->attr & CLIB_FFI_ATTR_INCOMPLETE_ARRAY)) {
		clib_throw_error(clib_ffi_exception_ce, "Only the leftmost array can be undimensioned");
		RETURN_THROWS();
	} else if (type->kind == CLIB_FFI_TYPE_VOID) {
		clib_throw_error(clib_ffi_exception_ce, "Array of void type is not allowed");
		RETURN_THROWS();
	} else if (type->attr & CLIB_FFI_ATTR_INCOMPLETE_TAG) {
		clib_throw_error(clib_ffi_exception_ce, "Array of incomplete type is not allowed");
		RETURN_THROWS();
	}

	if (CLIB_FFI_TYPE_IS_OWNED(ctype->type)) {
		if (!(type->attr & CLIB_FFI_ATTR_STORED)) {
			if (GC_REFCOUNT(&ctype->std) == 1) {
				/* transfer type ownership */
				ctype->type = type;
				type = CLIB_FFI_TYPE_MAKE_OWNED(type);
			} else {
				ctype->type = type = clib_ffi_remember_type(type);
			}
		}
	}

	CLIB_HASH_REVERSE_FOREACH_VAL(dims, val) {
		long n = b_value_get_long(val);
		clib_ffi_type *new_type;

		if (n < 0) {
			clib_throw_error(clib_ffi_exception_ce, "negative array index");
			clib_ffi_type_dtor(type);
			RETURN_THROWS();
		} else if (CLIB_FFI_TYPE(type)->kind == CLIB_FFI_TYPE_ARRAY && (CLIB_FFI_TYPE(type)->attr & CLIB_FFI_ATTR_INCOMPLETE_ARRAY)) {
			clib_throw_error(clib_ffi_exception_ce, "only the leftmost array can be undimensioned");
			clib_ffi_type_dtor(type);
			RETURN_THROWS();
		}

		new_type = emalloc(sizeof(clib_ffi_type));
		new_type->kind = CLIB_FFI_TYPE_ARRAY;
		new_type->attr = 0;
		new_type->size = n * CLIB_FFI_TYPE(type)->size;
		new_type->align = CLIB_FFI_TYPE(type)->align;
		new_type->array.type = type;
		new_type->array.length = n;

		if (n == 0) {
			new_type->attr |= CLIB_FFI_ATTR_INCOMPLETE_ARRAY;
		}

		type = CLIB_FFI_TYPE_MAKE_OWNED(new_type);
	} CLIB_HASH_FOREACH_END();

	ctype = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ctype->type = type;

	RETURN_OBJ(&ctype->std);
}
/* }}} */

CLIB_METHOD(FFI, addr) /* {{{ */
{
	clib_ffi_type *type, *new_type;
	clib_ffi_cdata *cdata, *new_cdata;
	b_value *zv, *arg;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ZVAL(zv)
	CLIB_PARSE_PARAMETERS_END();

	arg = zv;
	ZVAL_DEREF(zv);
	if (Z_TYPE_P(zv) != IS_OBJECT || Z_OBJCE_P(zv) != clib_ffi_cdata_ce) {
		clib_wrong_parameter_class_error(1, "FFI\\CData", zv);
		RETURN_THROWS();
	}

	cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);
	type = CLIB_FFI_TYPE(cdata->type);

	new_type = emalloc(sizeof(clib_ffi_type));
	new_type->kind = CLIB_FFI_TYPE_POINTER;
	new_type->attr = 0;
	new_type->size = sizeof(void*);
	new_type->align = _Alignof(void*);
	/* life-time (source must relive the resulting pointer) ??? */
	new_type->pointer.type = type;

	new_cdata = (clib_ffi_cdata*)clib_ffi_cdata_new(clib_ffi_cdata_ce);
	new_cdata->type = CLIB_FFI_TYPE_MAKE_OWNED(new_type);
	new_cdata->ptr_holder = cdata->ptr;
	new_cdata->ptr = &new_cdata->ptr_holder;

	if (GC_REFCOUNT(&cdata->std) == 1 && Z_REFCOUNT_P(arg) == 1) {
		if (CLIB_FFI_TYPE_IS_OWNED(cdata->type)) {
			/* transfer type ownership */
			cdata->type = type;
			new_type->pointer.type = CLIB_FFI_TYPE_MAKE_OWNED(type);
		}
		if (cdata->flags & CLIB_FFI_FLAG_OWNED) {
			/* transfer ownership */
			cdata->flags &= ~CLIB_FFI_FLAG_OWNED;
			new_cdata->flags |= CLIB_FFI_FLAG_OWNED;
		}
	}

	RETURN_OBJ(&new_cdata->std);
}
/* }}} */

CLIB_METHOD(FFI, sizeof) /* {{{ */
{
	b_value *zv;
	clib_ffi_type *type;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ZVAL(zv);
	CLIB_PARSE_PARAMETERS_END();

	ZVAL_DEREF(zv);
	if (Z_TYPE_P(zv) == IS_OBJECT && Z_OBJCE_P(zv) == clib_ffi_cdata_ce) {
		clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);
		type = CLIB_FFI_TYPE(cdata->type);
	} else if (Z_TYPE_P(zv) == IS_OBJECT && Z_OBJCE_P(zv) == clib_ffi_ctype_ce) {
		clib_ffi_ctype *ctype = (clib_ffi_ctype*)Z_OBJ_P(zv);
		type = CLIB_FFI_TYPE(ctype->type);
	} else {
		clib_wrong_parameter_class_error(1, "FFI\\CData or FFI\\CType", zv);
		RETURN_THROWS();
	}

	RETURN_LONG(type->size);
}
/* }}} */

CLIB_METHOD(FFI, alignof) /* {{{ */
{
	b_value *zv;
	clib_ffi_type *type;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ZVAL(zv);
	CLIB_PARSE_PARAMETERS_END();

	ZVAL_DEREF(zv);
	if (Z_TYPE_P(zv) == IS_OBJECT && Z_OBJCE_P(zv) == clib_ffi_cdata_ce) {
		clib_ffi_cdata *cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);
		type = CLIB_FFI_TYPE(cdata->type);
	} else if (Z_TYPE_P(zv) == IS_OBJECT && Z_OBJCE_P(zv) == clib_ffi_ctype_ce) {
		clib_ffi_ctype *ctype = (clib_ffi_ctype*)Z_OBJ_P(zv);
		type = CLIB_FFI_TYPE(ctype->type);
	} else {
		clib_wrong_parameter_class_error(1, "FFI\\CData or FFI\\CType", zv);
		RETURN_THROWS();
	}

	RETURN_LONG(type->align);
}
/* }}} */

CLIB_METHOD(FFI, memcpy) /* {{{ */
{
	b_value *zv1, *zv2;
	clib_ffi_cdata *cdata1, *cdata2;
	clib_ffi_type *type1, *type2;
	void *ptr1, *ptr2;
	long size;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_OBJECT_OF_CLASS_EX(zv1, clib_ffi_cdata_ce, 0, 1);
		Z_PARAM_ZVAL(zv2)
		Z_PARAM_LONG(size)
	CLIB_PARSE_PARAMETERS_END();

	cdata1 = (clib_ffi_cdata*)Z_OBJ_P(zv1);
	type1 = CLIB_FFI_TYPE(cdata1->type);
	if (type1->kind == CLIB_FFI_TYPE_POINTER) {
		ptr1 = *(void**)cdata1->ptr;
	} else {
		ptr1 = cdata1->ptr;
		if (type1->kind != CLIB_FFI_TYPE_POINTER && size > type1->size) {
			clib_throw_error(clib_ffi_exception_ce, "Attempt to write over data boundary");
			RETURN_THROWS();
		}
	}

	ZVAL_DEREF(zv2);
	if (Z_TYPE_P(zv2) == IS_STRING) {
		ptr2 = Z_STRVAL_P(zv2);
		if (size > Z_STRLEN_P(zv2)) {
			clib_throw_error(clib_ffi_exception_ce, "Attempt to read over string boundary");
			RETURN_THROWS();
		}
	} else if (Z_TYPE_P(zv2) == IS_OBJECT && Z_OBJCE_P(zv2) == clib_ffi_cdata_ce) {
		cdata2 = (clib_ffi_cdata*)Z_OBJ_P(zv2);
		type2 = CLIB_FFI_TYPE(cdata2->type);
		if (type2->kind == CLIB_FFI_TYPE_POINTER) {
			ptr2 = *(void**)cdata2->ptr;
		} else {
			ptr2 = cdata2->ptr;
			if (type2->kind != CLIB_FFI_TYPE_POINTER && size > type2->size) {
				clib_throw_error(clib_ffi_exception_ce, "Attempt to read over data boundary");
				RETURN_THROWS();
			}
		}
	} else {
		clib_wrong_parameter_class_error(2, "FFI\\CData or string", zv2);
		RETURN_THROWS();
	}

	memcpy(ptr1, ptr2, size);
}
/* }}} */

CLIB_METHOD(FFI, memcmp) /* {{{ */
{
	b_value *zv1, *zv2;
	clib_ffi_cdata *cdata1, *cdata2;
	clib_ffi_type *type1, *type2;
	void *ptr1, *ptr2;
	long size;
	int ret;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_ZVAL(zv1);
		Z_PARAM_ZVAL(zv2);
		Z_PARAM_LONG(size)
	CLIB_PARSE_PARAMETERS_END();

	ZVAL_DEREF(zv1);
	if (Z_TYPE_P(zv1) == IS_STRING) {
		ptr1 = Z_STRVAL_P(zv1);
		if (size > Z_STRLEN_P(zv1)) {
			clib_throw_error(clib_ffi_exception_ce, "attempt to read over string boundary");
			RETURN_THROWS();
		}
	} else if (Z_TYPE_P(zv1) == IS_OBJECT && Z_OBJCE_P(zv1) == clib_ffi_cdata_ce) {
		cdata1 = (clib_ffi_cdata*)Z_OBJ_P(zv1);
		type1 = CLIB_FFI_TYPE(cdata1->type);
		if (type1->kind == CLIB_FFI_TYPE_POINTER) {
			ptr1 = *(void**)cdata1->ptr;
		} else {
			ptr1 = cdata1->ptr;
			if (type1->kind != CLIB_FFI_TYPE_POINTER && size > type1->size) {
				clib_throw_error(clib_ffi_exception_ce, "attempt to read over data boundary");
				RETURN_THROWS();
			}
		}
	} else {
		clib_wrong_parameter_class_error(1, "FFI\\CData or string", zv1);
		RETURN_THROWS();
	}

	ZVAL_DEREF(zv2);
	if (Z_TYPE_P(zv2) == IS_STRING) {
		ptr2 = Z_STRVAL_P(zv2);
		if (size > Z_STRLEN_P(zv2)) {
			clib_throw_error(clib_ffi_exception_ce, "Attempt to read over string boundary");
			RETURN_THROWS();
		}
	} else if (Z_TYPE_P(zv2) == IS_OBJECT && Z_OBJCE_P(zv2) == clib_ffi_cdata_ce) {
		cdata2 = (clib_ffi_cdata*)Z_OBJ_P(zv2);
		type2 = CLIB_FFI_TYPE(cdata2->type);
		if (type2->kind == CLIB_FFI_TYPE_POINTER) {
			ptr2 = *(void**)cdata2->ptr;
		} else {
			ptr2 = cdata2->ptr;
			if (type2->kind != CLIB_FFI_TYPE_POINTER && size > type2->size) {
				clib_throw_error(clib_ffi_exception_ce, "Attempt to read over data boundary");
				RETURN_THROWS();
			}
		}
	} else {
		clib_wrong_parameter_class_error(2, "FFI\\CData or string", zv2);
		RETURN_THROWS();
	}

	ret = memcmp(ptr1, ptr2, size);
	if (ret == 0) {
		RETVAL_LONG(0);
	} else if (ret < 0) {
		RETVAL_LONG(-1);
	} else {
		RETVAL_LONG(1);
	}
}
/* }}} */

CLIB_METHOD(FFI, memset) /* {{{ */
{
	b_value *zv;
	clib_ffi_cdata *cdata;
	clib_ffi_type *type;
	void *ptr;
	long ch, size;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_OBJECT_OF_CLASS_EX(zv, clib_ffi_cdata_ce, 0, 1);
		Z_PARAM_LONG(ch)
		Z_PARAM_LONG(size)
	CLIB_PARSE_PARAMETERS_END();

	cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);
	type = CLIB_FFI_TYPE(cdata->type);
	if (type->kind == CLIB_FFI_TYPE_POINTER) {
		ptr = *(void**)cdata->ptr;
	} else {
		ptr = cdata->ptr;
		if (type->kind != CLIB_FFI_TYPE_POINTER && size > type->size) {
			clib_throw_error(clib_ffi_exception_ce, "attempt to write over data boundary");
			RETURN_THROWS();
		}
	}

	memset(ptr, ch, size);
}
/* }}} */

CLIB_METHOD(FFI, string) /* {{{ */
{
	b_value *zv;
	clib_ffi_cdata *cdata;
	clib_ffi_type *type;
	void *ptr;
	long size;
	bool size_is_null = 1;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_OBJECT_OF_CLASS_EX(zv, clib_ffi_cdata_ce, 0, 1);
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(size, size_is_null)
	CLIB_PARSE_PARAMETERS_END();

	cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);
	type = CLIB_FFI_TYPE(cdata->type);
	if (!size_is_null) {
		if (type->kind == CLIB_FFI_TYPE_POINTER) {
			ptr = *(void**)cdata->ptr;
		} else {
			ptr = cdata->ptr;
			if (type->kind != CLIB_FFI_TYPE_POINTER && size > type->size) {
				clib_throw_error(clib_ffi_exception_ce, "attempt to read over data boundary");
				RETURN_THROWS();
			}
		}
		RETURN_STRINGL((char*)ptr, size);
	} else {
		if (type->kind == CLIB_FFI_TYPE_POINTER && CLIB_FFI_TYPE(type->pointer.type)->kind == CLIB_FFI_TYPE_CHAR) {
			ptr = *(void**)cdata->ptr;
		} else if (type->kind == CLIB_FFI_TYPE_ARRAY && CLIB_FFI_TYPE(type->array.type)->kind == CLIB_FFI_TYPE_CHAR) {
			ptr = cdata->ptr;
		} else {
			clib_throw_error(clib_ffi_exception_ce, "FFI\\Cdata is not a C string");
			RETURN_THROWS();
		}
		RETURN_STRING((char*)ptr);
	}
}
/* }}} */

CLIB_METHOD(FFI, isNull) /* {{{ */
{
	b_value *zv;
	clib_ffi_cdata *cdata;
	clib_ffi_type *type;

	CLIB_FFI_VALIDATE_API_RESTRICTION();
	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ZVAL(zv);
	CLIB_PARSE_PARAMETERS_END();

	ZVAL_DEREF(zv);
	if (Z_TYPE_P(zv) != IS_OBJECT || Z_OBJCE_P(zv) != clib_ffi_cdata_ce) {
		clib_wrong_parameter_class_error(1, "FFI\\CData", zv);
		RETURN_THROWS();
	}

	cdata = (clib_ffi_cdata*)Z_OBJ_P(zv);
	type = CLIB_FFI_TYPE(cdata->type);

	if (type->kind != CLIB_FFI_TYPE_POINTER){
		clib_throw_error(clib_ffi_exception_ce, "FFI\\Cdata is not a pointer");
		RETURN_THROWS();
	}

	RETURN_BOOL(*(void**)cdata->ptr == NULL);
}
/* }}} */


CLIB_METHOD(FFI_CType, getName) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	clib_ffi_ctype_name_buf buf;

	buf.start = buf.end = buf.buf + ((MAX_TYPE_NAME_LEN * 3) / 4);
	if (!clib_ffi_ctype_name(&buf, CLIB_FFI_TYPE(ctype->type))) {
		RETURN_STR_COPY(Z_OBJ_P(CLIB_THIS)->ce->name);
	} else {
		size_t len = buf.end - buf.start;
		b_obj_string *res = b_obj_string_init(buf.start, len, 0);
		RETURN_STR(res);
	}
}
/* }}} */

CLIB_METHOD(FFI_CType, getKind) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	RETURN_LONG(type->kind);
}
/* }}} */

CLIB_METHOD(FFI_CType, getSize) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	RETURN_LONG(type->size);
}
/* }}} */

CLIB_METHOD(FFI_CType, getAlignment) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	RETURN_LONG(type->align);
}
/* }}} */

CLIB_METHOD(FFI_CType, getAttributes) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	RETURN_LONG(type->attr);
}
/* }}} */

CLIB_METHOD(FFI_CType, getEnumKind) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_ENUM) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not an enumeration");
		RETURN_THROWS();
	}
	RETURN_LONG(type->enumeration.kind);
}
/* }}} */

CLIB_METHOD(FFI_CType, getArrayElementType) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;
	clib_ffi_ctype *ret;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_ARRAY) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not an array");
		RETURN_THROWS();
	}

	ret = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ret->type = CLIB_FFI_TYPE(type->array.type);
	RETURN_OBJ(&ret->std);
}
/* }}} */

CLIB_METHOD(FFI_CType, getArrayLength) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_ARRAY) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not an array");
		RETURN_THROWS();
	}
	RETURN_LONG(type->array.length);
}
/* }}} */

CLIB_METHOD(FFI_CType, getPointerType) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_ctype *ret;
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_POINTER) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a pointer");
		RETURN_THROWS();
	}

	ret = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ret->type = CLIB_FFI_TYPE(type->pointer.type);
	RETURN_OBJ(&ret->std);
}
/* }}} */

CLIB_METHOD(FFI_CType, getStructFieldNames) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;
	b_obj_dict *ht;
	b_obj_string* name;
	b_value zv;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_STRUCT) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a structure");
		RETURN_THROWS();
	}

	ht = clib_new_array(clib_hash_num_elements(&type->record.fields));
	RETVAL_ARR(ht);
	CLIB_HASH_MAP_FOREACH_STR_KEY(&type->record.fields, name) {
		ZVAL_STR_COPY(&zv, name);
		clib_hash_next_index_insert_new(ht, &zv);
	} CLIB_HASH_FOREACH_END();
}
/* }}} */

CLIB_METHOD(FFI_CType, getStructFieldOffset) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;
	b_obj_string *name;
	clib_ffi_field *ptr;

	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	CLIB_PARSE_PARAMETERS_END();

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_STRUCT) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a structure");
		RETURN_THROWS();
	}

	ptr = clib_hash_find_ptr(&type->record.fields, name);
	if (!ptr) {
		clib_throw_error(clib_ffi_exception_ce, "Wrong field name");
		RETURN_THROWS();
	}
	RETURN_LONG(ptr->offset);
}
/* }}} */

CLIB_METHOD(FFI_CType, getStructFieldType) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;
	b_obj_string *name;
	clib_ffi_field *ptr;
	clib_ffi_ctype *ret;

	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(name)
	CLIB_PARSE_PARAMETERS_END();

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_STRUCT) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a structure");
		RETURN_THROWS();
	}

	ptr = clib_hash_find_ptr(&type->record.fields, name);
	if (!ptr) {
		clib_throw_error(clib_ffi_exception_ce, "Wrong field name");
		RETURN_THROWS();
	}

	ret = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ret->type = CLIB_FFI_TYPE(ptr->type);
	RETURN_OBJ(&ret->std);
}
/* }}} */

CLIB_METHOD(FFI_CType, getFuncABI) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_FUNC) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a function");
		RETURN_THROWS();
	}
	RETURN_LONG(type->func.abi);
}
/* }}} */

CLIB_METHOD(FFI_CType, getFuncReturnType) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_ctype *ret;
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_FUNC) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a function");
		RETURN_THROWS();
	}

	ret = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ret->type = CLIB_FFI_TYPE(type->func.ret_type);
	RETURN_OBJ(&ret->std);
}
/* }}} */

CLIB_METHOD(FFI_CType, getFuncParameterCount) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type;

	if (clib_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_FUNC) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a function");
		RETURN_THROWS();
	}
	RETURN_LONG(type->func.args ? clib_hash_num_elements(type->func.args) : 0);
}
/* }}} */

CLIB_METHOD(FFI_CType, getFuncParameterType) /* {{{ */
{
	clib_ffi_ctype *ctype = (clib_ffi_ctype*)(Z_OBJ_P(CLIB_THIS));
	clib_ffi_type *type, *ptr;
	long n;
	clib_ffi_ctype *ret;

	CLIB_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(n)
	CLIB_PARSE_PARAMETERS_END();

	type = CLIB_FFI_TYPE(ctype->type);
	if (type->kind != CLIB_FFI_TYPE_FUNC) {
		clib_throw_error(clib_ffi_exception_ce, "FFI\\CType is not a function");
		RETURN_THROWS();
	}

	if (!type->func.args) {
		clib_throw_error(clib_ffi_exception_ce, "Wrong argument number");
		RETURN_THROWS();
	}

	ptr = clib_hash_index_find_ptr(type->func.args, n);
	if (!ptr) {
		clib_throw_error(clib_ffi_exception_ce, "Wrong argument number");
		RETURN_THROWS();
	}

	ret = (clib_ffi_ctype*)clib_ffi_ctype_new(clib_ffi_ctype_ce);
	ret->type = CLIB_FFI_TYPE(ptr);
	RETURN_OBJ(&ret->std);
}
/* }}} */

static char *clib_ffi_parse_directives(const char *filename, char *code_pos, char **scope_name, char **lib, bool preload) /* {{{ */
{
	char *p;

	*scope_name = NULL;
	*lib = NULL;
	while (*code_pos == '#') {
		if (strncmp(code_pos, "#define FFI_SCOPE", sizeof("#define FFI_SCOPE") - 1) == 0
		 && (code_pos[sizeof("#define FFI_SCOPE") - 1] == ' '
		  || code_pos[sizeof("#define FFI_SCOPE") - 1] == '\t')) {
			p = code_pos + sizeof("#define FFI_SCOPE");
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			if (*p != '"') {
				if (preload) {
					clib_error(E_WARNING, "FFI: failed pre-loading '%s', bad FFI_SCOPE define", filename);
				} else {
					clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', bad FFI_SCOPE define", filename);
				}
				return NULL;
			}
			p++;
			if (*scope_name) {
				if (preload) {
					clib_error(E_WARNING, "FFI: failed pre-loading '%s', FFI_SCOPE defined twice", filename);
				} else {
					clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', FFI_SCOPE defined twice", filename);
				}
				return NULL;
			}
			*scope_name = p;
			while (1) {
				if (*p == '\"') {
					*p = 0;
					p++;
					break;
				} else if (*p <= ' ') {
					if (preload) {
						clib_error(E_WARNING, "FFI: failed pre-loading '%s', bad FFI_SCOPE define", filename);
					} else {
						clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', bad FFI_SCOPE define", filename);
					}
					return NULL;
				}
				p++;
			}
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			while (*p == '\r' || *p == '\n') {
				p++;
			}
			code_pos = p;
		} else if (strncmp(code_pos, "#define FFI_LIB", sizeof("#define FFI_LIB") - 1) == 0
		 && (code_pos[sizeof("#define FFI_LIB") - 1] == ' '
		  || code_pos[sizeof("#define FFI_LIB") - 1] == '\t')) {
			p = code_pos + sizeof("#define FFI_LIB");
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			if (*p != '"') {
				if (preload) {
					clib_error(E_WARNING, "FFI: failed pre-loading '%s', bad FFI_LIB define", filename);
				} else {
					clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', bad FFI_LIB define", filename);
				}
				return NULL;
			}
			p++;
			if (*lib) {
				if (preload) {
					clib_error(E_WARNING, "FFI: failed pre-loading '%s', FFI_LIB defined twice", filename);
				} else {
					clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', FFI_LIB defined twice", filename);
				}
				return NULL;
			}
			*lib = p;
			while (1) {
				if (*p == '\"') {
					*p = 0;
					p++;
					break;
				} else if (*p <= ' ') {
					if (preload) {
						clib_error(E_WARNING, "FFI: failed pre-loading '%s', bad FFI_LIB define", filename);
					} else {
						clib_throw_error(clib_ffi_exception_ce, "Failed loading '%s', bad FFI_LIB define", filename);
					}
					return NULL;
				}
				p++;
			}
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			while (*p == '\r' || *p == '\n') {
				p++;
			}
			code_pos = p;
		} else {
			break;
		}
	}
	return code_pos;
}
/* }}} */

static CLIB_COLD clib_function *clib_fake_get_constructor(clib_object *object) /* {{{ */
{
	clib_throw_error(NULL, "Instantiation of %s is not allowed", ZSTR_VAL(object->ce->name));
	return NULL;
}
/* }}} */

static CLIB_COLD clib_never_inline void clib_bad_array_access(clib_class_entry *ce) /* {{{ */
{
	clib_throw_error(NULL, "Cannot use object of type %s as array", ZSTR_VAL(ce->name));
}
/* }}} */

static CLIB_COLD b_value *clib_fake_read_dimension(clib_object *obj, b_value *offset, int type, b_value *rv) /* {{{ */
{
	clib_bad_array_access(obj->ce);
	return NULL;
}
/* }}} */

static CLIB_COLD void clib_fake_write_dimension(clib_object *obj, b_value *offset, b_value *value) /* {{{ */
{
	clib_bad_array_access(obj->ce);
}
/* }}} */

static CLIB_COLD int clib_fake_has_dimension(clib_object *obj, b_value *offset, int check_empty) /* {{{ */
{
	clib_bad_array_access(obj->ce);
	return 0;
}
/* }}} */

static CLIB_COLD void clib_fake_unset_dimension(clib_object *obj, b_value *offset) /* {{{ */
{
	clib_bad_array_access(obj->ce);
}
/* }}} */

static CLIB_COLD clib_never_inline void clib_bad_property_access(clib_class_entry *ce) /* {{{ */
{
	clib_throw_error(NULL, "Cannot access property of object of type %s", ZSTR_VAL(ce->name));
}
/* }}} */

static CLIB_COLD b_value *clib_fake_read_property(clib_object *obj, b_obj_string *member, int type, void **cache_slot, b_value *rv) /* {{{ */
{
	clib_bad_property_access(obj->ce);
	return &EG(uninitialized_b_value);
}
/* }}} */

static CLIB_COLD b_value *clib_fake_write_property(clib_object *obj, b_obj_string *member, b_value *value, void **cache_slot) /* {{{ */
{
	clib_bad_array_access(obj->ce);
	return value;
}
/* }}} */

static CLIB_COLD int clib_fake_has_property(clib_object *obj, b_obj_string *member, int has_set_exists, void **cache_slot) /* {{{ */
{
	clib_bad_array_access(obj->ce);
	return 0;
}
/* }}} */

static CLIB_COLD void clib_fake_unset_property(clib_object *obj, b_obj_string *member, void **cache_slot) /* {{{ */
{
	clib_bad_array_access(obj->ce);
}
/* }}} */

static b_value *clib_fake_get_property_ptr_ptr(clib_object *obj, b_obj_string *member, int type, void **cache_slot) /* {{{ */
{
	return NULL;
}
/* }}} */

static CLIB_COLD clib_function *clib_fake_get_method(clib_object **obj_ptr, b_obj_string *method_name, const b_value *key) /* {{{ */
{
	clib_class_entry *ce = (*obj_ptr)->ce;
	clib_throw_error(NULL, "Object of type %s does not support method calls", ZSTR_VAL(ce->name));
	return NULL;
}
/* }}} */

static b_obj_dict *clib_fake_get_properties(clib_object *obj) /* {{{ */
{
	return (b_obj_dict*)&clib_empty_array;
}
/* }}} */

static b_obj_dict *clib_fake_get_gc(clib_object *ob, b_value **table, int *n) /* {{{ */
{
	*table = NULL;
	*n = 0;
	return NULL;
}
/* }}} */

static clib_result clib_fake_cast_object(clib_object *obj, b_value *result, int type)
{
	switch (type) {
		case _IS_BOOL:
			ZVAL_TRUE(result);
			return SUCCESS;
		default:
			return FAILURE;
	}
}

static CLIB_COLD clib_never_inline void clib_ffi_use_after_free(void) /* {{{ */
{
	clib_throw_error(clib_ffi_exception_ce, "Use after free()");
}
/* }}} */

static clib_object *clib_ffi_free_clone_obj(clib_object *obj) /* {{{ */
{
	clib_ffi_use_after_free();
	return NULL;
}
/* }}} */

static CLIB_COLD b_value *clib_ffi_free_read_dimension(clib_object *obj, b_value *offset, int type, b_value *rv) /* {{{ */
{
	clib_ffi_use_after_free();
	return NULL;
}
/* }}} */

static CLIB_COLD void clib_ffi_free_write_dimension(clib_object *obj, b_value *offset, b_value *value) /* {{{ */
{
	clib_ffi_use_after_free();
}
/* }}} */

static CLIB_COLD int clib_ffi_free_has_dimension(clib_object *obj, b_value *offset, int check_empty) /* {{{ */
{
	clib_ffi_use_after_free();
	return 0;
}
/* }}} */

static CLIB_COLD void clib_ffi_free_unset_dimension(clib_object *obj, b_value *offset) /* {{{ */
{
	clib_ffi_use_after_free();
}
/* }}} */

static CLIB_COLD b_value *clib_ffi_free_read_property(clib_object *obj, b_obj_string *member, int type, void **cache_slot, b_value *rv) /* {{{ */
{
	clib_ffi_use_after_free();
	return &EG(uninitialized_b_value);
}
/* }}} */

static CLIB_COLD b_value *clib_ffi_free_write_property(clib_object *obj, b_obj_string *member, b_value *value, void **cache_slot) /* {{{ */
{
	clib_ffi_use_after_free();
	return value;
}
/* }}} */

static CLIB_COLD int clib_ffi_free_has_property(clib_object *obj, b_obj_string *member, int has_set_exists, void **cache_slot) /* {{{ */
{
	clib_ffi_use_after_free();
	return 0;
}
/* }}} */

static CLIB_COLD void clib_ffi_free_unset_property(clib_object *obj, b_obj_string *member, void **cache_slot) /* {{{ */
{
	clib_ffi_use_after_free();
}
/* }}} */

static b_obj_dict *clib_ffi_free_get_debug_info(clib_object *obj, int *is_temp) /* {{{ */
{
	clib_ffi_use_after_free();
	return NULL;
}
/* }}} */

static CLIB_INI_MH(OnUpdateFFIEnable) /* {{{ */
{
	if (b_obj_string_equals_literal_ci(new_value, "preload")) {
		FFI_G(restriction) = CLIB_FFI_PRELOAD;
	} else {
		FFI_G(restriction) = (clib_ffi_api_restriction)clib_ini_parse_bool(new_value);
	}
	return SUCCESS;
}
/* }}} */

static CLIB_INI_DISP(clib_ffi_enable_displayer_cb) /* {{{ */
{
	if (FFI_G(restriction) == CLIB_FFI_PRELOAD) {
		CLIB_PUTS("preload");
	} else if (FFI_G(restriction) == CLIB_FFI_ENABLED) {
		CLIB_PUTS("On");
	} else {
		CLIB_PUTS("Off");
	}
}
/* }}} */

CLIB_INI_BEGIN()
	CLIB_INI_ENTRY3_EX("ffi.enable", "preload", CLIB_INI_SYSTEM, OnUpdateFFIEnable, NULL, NULL, NULL, clib_ffi_enable_displayer_cb)
	STD_CLIB_INI_ENTRY("ffi.preload", NULL, CLIB_INI_SYSTEM, OnUpdateString, preload, clib_ffi_globals, ffi_globals)
CLIB_INI_END()

static clib_result clib_ffi_preload_glob(const char *filename) /* {{{ */
{
#ifdef HAVE_GLOB
	glob_t globbuf;
	int    ret;
	unsigned int i;

	memset(&globbuf, 0, sizeof(glob_t));

	ret = glob(filename, 0, NULL, &globbuf);
#ifdef GLOB_NOMATCH
	if (ret == GLOB_NOMATCH || !globbuf.gl_pathc) {
#else
	if (!globbuf.gl_pathc) {
#endif
		/* pass */
	} else {
		for(i=0 ; i<globbuf.gl_pathc; i++) {
			clib_ffi *ffi = clib_ffi_load(globbuf.gl_pathv[i], 1);
			if (!ffi) {
				globfree(&globbuf);
				return FAILURE;
			}
			efree(ffi);
		}
		globfree(&globbuf);
	}
#else
	clib_ffi *ffi = clib_ffi_load(filename, 1);
	if (!ffi) {
		return FAILURE;
	}
	efree(ffi);
#endif

	return SUCCESS;
}
/* }}} */

static clib_result clib_ffi_preload(char *preload) /* {{{ */
{
	clib_ffi *ffi;
	char *s = NULL, *e, *filename;
	bool is_glob = 0;

	e = preload;
	while (*e) {
		switch (*e) {
			case CLIB_PATHS_SEPARATOR:
				if (s) {
					filename = estrndup(s, e-s);
					s = NULL;
					if (!is_glob) {
						ffi = clib_ffi_load(filename, 1);
						efree(filename);
						if (!ffi) {
							return FAILURE;
						}
						efree(ffi);
					} else {
						clib_result ret = clib_ffi_preload_glob(filename);

						efree(filename);
						if (ret == FAILURE) {
							return FAILURE;
						}
						is_glob = 0;
					}
				}
				break;
			case '*':
			case '?':
			case '[':
				is_glob = 1;
				break;
			default:
				if (!s) {
					s = e;
				}
				break;
		}
		e++;
	}
	if (s) {
		filename = estrndup(s, e-s);
		if (!is_glob) {
			ffi = clib_ffi_load(filename, 1);
			efree(filename);
			if (!ffi) {
				return FAILURE;
			}
			efree(ffi);
		} else {
			clib_result ret = clib_ffi_preload_glob(filename);
			efree(filename);
			if (ret == FAILURE) {
				return FAILURE;
			}
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ CLIB_MINIT_FUNCTION */
CLIB_MINIT_FUNCTION(ffi)
{
	REGISTER_INI_ENTRIES();

	FFI_G(is_cli) = strcmp(sapi_module.name, "cli") == 0;

	clib_ffi_exception_ce = register_class_FFI_Exception(clib_ce_error);

	clib_ffi_parser_exception_ce = register_class_FFI_ParserException(clib_ffi_exception_ce);

	clib_ffi_ce = register_class_FFI();
	clib_ffi_ce->create_object = clib_ffi_new;

	memcpy(&clib_ffi_new_fn, clib_hash_str_find_ptr(&clib_ffi_ce->function_table, "new", sizeof("new")-1), sizeof(clib_internal_function));
	clib_ffi_new_fn.fn_flags &= ~CLIB_ACC_STATIC;
	memcpy(&clib_ffi_cast_fn, clib_hash_str_find_ptr(&clib_ffi_ce->function_table, "cast", sizeof("cast")-1), sizeof(clib_internal_function));
	clib_ffi_cast_fn.fn_flags &= ~CLIB_ACC_STATIC;
	memcpy(&clib_ffi_type_fn, clib_hash_str_find_ptr(&clib_ffi_ce->function_table, "type", sizeof("type")-1), sizeof(clib_internal_function));
	clib_ffi_type_fn.fn_flags &= ~CLIB_ACC_STATIC;

	memcpy(&clib_ffi_handlers, clib_get_std_object_handlers(), sizeof(clib_object_handlers));
	clib_ffi_handlers.get_constructor      = clib_fake_get_constructor;
	clib_ffi_handlers.free_obj             = clib_ffi_free_obj;
	clib_ffi_handlers.clone_obj            = NULL;
	clib_ffi_handlers.read_property        = clib_ffi_read_var;
	clib_ffi_handlers.write_property       = clib_ffi_write_var;
	clib_ffi_handlers.read_dimension       = clib_fake_read_dimension;
	clib_ffi_handlers.write_dimension      = clib_fake_write_dimension;
	clib_ffi_handlers.get_property_ptr_ptr = clib_fake_get_property_ptr_ptr;
	clib_ffi_handlers.has_property         = clib_fake_has_property;
	clib_ffi_handlers.unset_property       = clib_fake_unset_property;
	clib_ffi_handlers.has_dimension        = clib_fake_has_dimension;
	clib_ffi_handlers.unset_dimension      = clib_fake_unset_dimension;
	clib_ffi_handlers.get_method           = clib_ffi_get_func;
	clib_ffi_handlers.compare              = NULL;
	clib_ffi_handlers.cast_object          = clib_fake_cast_object;
	clib_ffi_handlers.get_debug_info       = NULL;
	clib_ffi_handlers.get_closure          = NULL;
	clib_ffi_handlers.get_properties       = clib_fake_get_properties;
	clib_ffi_handlers.get_gc               = clib_fake_get_gc;

	clib_ffi_cdata_ce = register_class_FFI_CData();
	clib_ffi_cdata_ce->create_object = clib_ffi_cdata_new;
	clib_ffi_cdata_ce->get_iterator = clib_ffi_cdata_get_iterator;

	memcpy(&clib_ffi_cdata_handlers, clib_get_std_object_handlers(), sizeof(clib_object_handlers));
	clib_ffi_cdata_handlers.get_constructor      = clib_fake_get_constructor;
	clib_ffi_cdata_handlers.free_obj             = clib_ffi_cdata_free_obj;
	clib_ffi_cdata_handlers.clone_obj            = clib_ffi_cdata_clone_obj;
	clib_ffi_cdata_handlers.read_property        = clib_ffi_cdata_read_field;
	clib_ffi_cdata_handlers.write_property       = clib_ffi_cdata_write_field;
	clib_ffi_cdata_handlers.read_dimension       = clib_ffi_cdata_read_dim;
	clib_ffi_cdata_handlers.write_dimension      = clib_ffi_cdata_write_dim;
	clib_ffi_cdata_handlers.get_property_ptr_ptr = clib_fake_get_property_ptr_ptr;
	clib_ffi_cdata_handlers.has_property         = clib_fake_has_property;
	clib_ffi_cdata_handlers.unset_property       = clib_fake_unset_property;
	clib_ffi_cdata_handlers.has_dimension        = clib_fake_has_dimension;
	clib_ffi_cdata_handlers.unset_dimension      = clib_fake_unset_dimension;
	clib_ffi_cdata_handlers.get_method           = clib_fake_get_method;
	clib_ffi_cdata_handlers.get_class_name       = clib_ffi_cdata_get_class_name;
	clib_ffi_cdata_handlers.do_operation         = clib_ffi_cdata_do_operation;
	clib_ffi_cdata_handlers.compare              = clib_ffi_cdata_compare_objects;
	clib_ffi_cdata_handlers.cast_object          = clib_ffi_cdata_cast_object;
	clib_ffi_cdata_handlers.count_elements       = clib_ffi_cdata_count_elements;
	clib_ffi_cdata_handlers.get_debug_info       = clib_ffi_cdata_get_debug_info;
	clib_ffi_cdata_handlers.get_closure          = clib_ffi_cdata_get_closure;
	clib_ffi_cdata_handlers.get_properties       = clib_fake_get_properties;
	clib_ffi_cdata_handlers.get_gc               = clib_fake_get_gc;

	memcpy(&clib_ffi_cdata_value_handlers, clib_get_std_object_handlers(), sizeof(clib_object_handlers));
	clib_ffi_cdata_value_handlers.get_constructor      = clib_fake_get_constructor;
	clib_ffi_cdata_value_handlers.free_obj             = clib_ffi_cdata_free_obj;
	clib_ffi_cdata_value_handlers.clone_obj            = clib_ffi_cdata_clone_obj;
	clib_ffi_cdata_value_handlers.read_property        = clib_ffi_cdata_get;
	clib_ffi_cdata_value_handlers.write_property       = clib_ffi_cdata_set;
	clib_ffi_cdata_value_handlers.read_dimension       = clib_fake_read_dimension;
	clib_ffi_cdata_value_handlers.write_dimension      = clib_fake_write_dimension;
	clib_ffi_cdata_value_handlers.get_property_ptr_ptr = clib_fake_get_property_ptr_ptr;
	clib_ffi_cdata_value_handlers.has_property         = clib_fake_has_property;
	clib_ffi_cdata_value_handlers.unset_property       = clib_fake_unset_property;
	clib_ffi_cdata_value_handlers.has_dimension        = clib_fake_has_dimension;
	clib_ffi_cdata_value_handlers.unset_dimension      = clib_fake_unset_dimension;
	clib_ffi_cdata_value_handlers.get_method           = clib_fake_get_method;
	clib_ffi_cdata_value_handlers.get_class_name       = clib_ffi_cdata_get_class_name;
	clib_ffi_cdata_value_handlers.compare              = clib_ffi_cdata_compare_objects;
	clib_ffi_cdata_value_handlers.cast_object          = clib_ffi_cdata_cast_object;
	clib_ffi_cdata_value_handlers.count_elements       = NULL;
	clib_ffi_cdata_value_handlers.get_debug_info       = clib_ffi_cdata_get_debug_info;
	clib_ffi_cdata_value_handlers.get_closure          = NULL;
	clib_ffi_cdata_value_handlers.get_properties       = clib_fake_get_properties;
	clib_ffi_cdata_value_handlers.get_gc               = clib_fake_get_gc;

	memcpy(&clib_ffi_cdata_free_handlers, clib_get_std_object_handlers(), sizeof(clib_object_handlers));
	clib_ffi_cdata_free_handlers.get_constructor      = clib_fake_get_constructor;
	clib_ffi_cdata_free_handlers.free_obj             = clib_ffi_cdata_free_obj;
	clib_ffi_cdata_free_handlers.clone_obj            = clib_ffi_free_clone_obj;
	clib_ffi_cdata_free_handlers.read_property        = clib_ffi_free_read_property;
	clib_ffi_cdata_free_handlers.write_property       = clib_ffi_free_write_property;
	clib_ffi_cdata_free_handlers.read_dimension       = clib_ffi_free_read_dimension;
	clib_ffi_cdata_free_handlers.write_dimension      = clib_ffi_free_write_dimension;
	clib_ffi_cdata_free_handlers.get_property_ptr_ptr = clib_fake_get_property_ptr_ptr;
	clib_ffi_cdata_free_handlers.has_property         = clib_ffi_free_has_property;
	clib_ffi_cdata_free_handlers.unset_property       = clib_ffi_free_unset_property;
	clib_ffi_cdata_free_handlers.has_dimension        = clib_ffi_free_has_dimension;
	clib_ffi_cdata_free_handlers.unset_dimension      = clib_ffi_free_unset_dimension;
	clib_ffi_cdata_free_handlers.get_method           = clib_fake_get_method;
	clib_ffi_cdata_free_handlers.get_class_name       = clib_ffi_cdata_get_class_name;
	clib_ffi_cdata_free_handlers.compare              = clib_ffi_cdata_compare_objects;
	clib_ffi_cdata_free_handlers.cast_object          = clib_fake_cast_object;
	clib_ffi_cdata_free_handlers.count_elements       = NULL;
	clib_ffi_cdata_free_handlers.get_debug_info       = clib_ffi_free_get_debug_info;
	clib_ffi_cdata_free_handlers.get_closure          = NULL;
	clib_ffi_cdata_free_handlers.get_properties       = clib_fake_get_properties;
	clib_ffi_cdata_free_handlers.get_gc               = clib_fake_get_gc;

	clib_ffi_ctype_ce = register_class_FFI_CType();
	clib_ffi_ctype_ce->create_object = clib_ffi_ctype_new;

	memcpy(&clib_ffi_ctype_handlers, clib_get_std_object_handlers(), sizeof(clib_object_handlers));
	clib_ffi_ctype_handlers.get_constructor      = clib_fake_get_constructor;
	clib_ffi_ctype_handlers.free_obj             = clib_ffi_ctype_free_obj;
	clib_ffi_ctype_handlers.clone_obj            = NULL;
	clib_ffi_ctype_handlers.read_property        = clib_fake_read_property;
	clib_ffi_ctype_handlers.write_property       = clib_fake_write_property;
	clib_ffi_ctype_handlers.read_dimension       = clib_fake_read_dimension;
	clib_ffi_ctype_handlers.write_dimension      = clib_fake_write_dimension;
	clib_ffi_ctype_handlers.get_property_ptr_ptr = clib_fake_get_property_ptr_ptr;
	clib_ffi_ctype_handlers.has_property         = clib_fake_has_property;
	clib_ffi_ctype_handlers.unset_property       = clib_fake_unset_property;
	clib_ffi_ctype_handlers.has_dimension        = clib_fake_has_dimension;
	clib_ffi_ctype_handlers.unset_dimension      = clib_fake_unset_dimension;
	//clib_ffi_ctype_handlers.get_method           = clib_fake_get_method;
	clib_ffi_ctype_handlers.get_class_name       = clib_ffi_ctype_get_class_name;
	clib_ffi_ctype_handlers.compare              = clib_ffi_ctype_compare_objects;
	clib_ffi_ctype_handlers.cast_object          = clib_fake_cast_object;
	clib_ffi_ctype_handlers.count_elements       = NULL;
	clib_ffi_ctype_handlers.get_debug_info       = clib_ffi_ctype_get_debug_info;
	clib_ffi_ctype_handlers.get_closure          = NULL;
	clib_ffi_ctype_handlers.get_properties       = clib_fake_get_properties;
	clib_ffi_ctype_handlers.get_gc               = clib_fake_get_gc;

	if (FFI_G(preload)) {
		return clib_ffi_preload(FFI_G(preload));
	}

	return SUCCESS;
}
/* }}} */

/* {{{ CLIB_RSHUTDOWN_FUNCTION */
CLIB_RSHUTDOWN_FUNCTION(ffi)
{
	if (FFI_G(callbacks)) {
		clib_hash_destroy(FFI_G(callbacks));
		efree(FFI_G(callbacks));
		FFI_G(callbacks) = NULL;
	}
	if (FFI_G(weak_types)) {
#if 0
		fprintf(stderr, "WeakTypes: %d\n", clib_hash_num_elements(FFI_G(weak_types)));
#endif
		clib_hash_destroy(FFI_G(weak_types));
		efree(FFI_G(weak_types));
		FFI_G(weak_types) = NULL;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ CLIB_MINFO_FUNCTION */
CLIB_MINFO_FUNCTION(ffi)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "FFI support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

static const clib_ffi_type clib_ffi_type_void = {.kind=CLIB_FFI_TYPE_VOID, .size=1, .align=1};
static const clib_ffi_type clib_ffi_type_char = {.kind=CLIB_FFI_TYPE_CHAR, .size=1, .align=_Alignof(char)};
static const clib_ffi_type clib_ffi_type_bool = {.kind=CLIB_FFI_TYPE_BOOL, .size=1, .align=_Alignof(uint8_t)};
static const clib_ffi_type clib_ffi_type_sint8 = {.kind=CLIB_FFI_TYPE_SINT8, .size=1, .align=_Alignof(int8_t)};
static const clib_ffi_type clib_ffi_type_uint8 = {.kind=CLIB_FFI_TYPE_UINT8, .size=1, .align=_Alignof(uint8_t)};
static const clib_ffi_type clib_ffi_type_sint16 = {.kind=CLIB_FFI_TYPE_SINT16, .size=2, .align=_Alignof(int16_t)};
static const clib_ffi_type clib_ffi_type_uint16 = {.kind=CLIB_FFI_TYPE_UINT16, .size=2, .align=_Alignof(uint16_t)};
static const clib_ffi_type clib_ffi_type_sint32 = {.kind=CLIB_FFI_TYPE_SINT32, .size=4, .align=_Alignof(int32_t)};
static const clib_ffi_type clib_ffi_type_uint32 = {.kind=CLIB_FFI_TYPE_UINT32, .size=4, .align=_Alignof(uint32_t)};
static const clib_ffi_type clib_ffi_type_sint64 = {.kind=CLIB_FFI_TYPE_SINT64, .size=8, .align=_Alignof(int64_t)};
static const clib_ffi_type clib_ffi_type_uint64 = {.kind=CLIB_FFI_TYPE_UINT64, .size=8, .align=_Alignof(uint64_t)};
static const clib_ffi_type clib_ffi_type_float = {.kind=CLIB_FFI_TYPE_FLOAT, .size=sizeof(float), .align=_Alignof(float)};
static const clib_ffi_type clib_ffi_type_double = {.kind=CLIB_FFI_TYPE_DOUBLE, .size=sizeof(double), .align=_Alignof(double)};

#ifdef HAVE_LONG_DOUBLE
static const clib_ffi_type clib_ffi_type_long_double = {.kind=CLIB_FFI_TYPE_LONGDOUBLE, .size=sizeof(long double), .align=_Alignof(long double)};
#endif

static const clib_ffi_type clib_ffi_type_ptr = {.kind=CLIB_FFI_TYPE_POINTER, .size=sizeof(void*), .align=_Alignof(void*), .pointer.type = (clib_ffi_type*)&clib_ffi_type_void};

const struct {
	const char *name;
	const clib_ffi_type *type;
} clib_ffi_types[] = {
	{"void",        &clib_ffi_type_void},
	{"char",        &clib_ffi_type_char},
	{"bool",        &clib_ffi_type_bool},
	{"int8_t",      &clib_ffi_type_sint8},
	{"uint8_t",     &clib_ffi_type_uint8},
	{"int16_t",     &clib_ffi_type_sint16},
	{"uint16_t",    &clib_ffi_type_uint16},
	{"int32_t",     &clib_ffi_type_sint32},
	{"uint32_t",    &clib_ffi_type_uint32},
	{"int64_t",     &clib_ffi_type_sint64},
	{"uint64_t",    &clib_ffi_type_uint64},
	{"float",       &clib_ffi_type_float},
	{"double",      &clib_ffi_type_double},
#ifdef HAVE_LONG_DOUBLE
	{"long double", &clib_ffi_type_long_double},
#endif
#if SIZEOF_SIZE_T == 4
	{"uintptr_t",  &clib_ffi_type_uint32},
	{"intptr_t",   &clib_ffi_type_sint32},
	{"size_t",     &clib_ffi_type_uint32},
	{"ssize_t",    &clib_ffi_type_sint32},
	{"ptrdiff_t",  &clib_ffi_type_sint32},
#else
	{"uintptr_t",  &clib_ffi_type_uint64},
	{"intptr_t",   &clib_ffi_type_sint64},
	{"size_t",     &clib_ffi_type_uint64},
	{"ssize_t",    &clib_ffi_type_sint64},
	{"ptrdiff_t",  &clib_ffi_type_sint64},
#endif
#if SIZEOF_OFF_T == 4
	{"off_t",      &clib_ffi_type_sint32},
#else
	{"off_t",      &clib_ffi_type_sint64},
#endif

	{"va_list",           &clib_ffi_type_ptr},
	{"__builtin_va_list", &clib_ffi_type_ptr},
	{"__gnuc_va_list",    &clib_ffi_type_ptr},
};

/* {{{ CLIB_GINIT_FUNCTION */
static CLIB_GINIT_FUNCTION(ffi)
{
	size_t i;

#if defined(COMPILE_DL_FFI) && defined(ZTS)
	CLIB_TSRMLS_CACHE_UPDATE();
#endif
	memset(ffi_globals, 0, sizeof(*ffi_globals));
	clib_hash_init(&ffi_globals->types, 0, NULL, NULL, 1);
	for (i = 0; i < sizeof(clib_ffi_types)/sizeof(clib_ffi_types[0]); i++) {
		clib_hash_str_add_new_ptr(&ffi_globals->types, clib_ffi_types[i].name, strlen(clib_ffi_types[i].name), (void*)clib_ffi_types[i].type);
	}
}
/* }}} */

/* {{{ CLIB_GINIT_FUNCTION */
static CLIB_GSHUTDOWN_FUNCTION(ffi)
{
	if (ffi_globals->scopes) {
		clib_hash_destroy(ffi_globals->scopes);
		free(ffi_globals->scopes);
	}
	clib_hash_destroy(&ffi_globals->types);
}
/* }}} */

/* {{{ ffi_module_entry */
clib_module_entry ffi_module_entry = {
	STANDARD_MODULE_HEADER,
	"FFI",					/* Extension name */
	NULL,					/* clib_function_entry */
	CLIB_MINIT(ffi),		/* CLIB_MINIT - Module initialization */
	NULL,					/* CLIB_MSHUTDOWN - Module shutdown */
	NULL,					/* CLIB_RINIT - Request initialization */
	CLIB_RSHUTDOWN(ffi),	/* CLIB_RSHUTDOWN - Request shutdown */
	CLIB_MINFO(ffi),		/* CLIB_MINFO - Module info */
	PHP_VERSION,			/* Version */
	CLIB_MODULE_GLOBALS(ffi),
	CLIB_GINIT(ffi),
	CLIB_GSHUTDOWN(ffi),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_FFI
# ifdef ZTS
CLIB_TSRMLS_CACHE_DEFINE()
# endif
CLIB_GET_MODULE(ffi)
#endif

/* parser callbacks */
void clib_ffi_parser_error(const char *format, ...) /* {{{ */
{
	va_list va;
	char *message = NULL;

	va_start(va, format);
	clib_vspprintf(&message, 0, format, va);

	if (EG(current_execute_data)) {
		clib_throw_exception(clib_ffi_parser_exception_ce, message, 0);
	} else {
		clib_error(E_WARNING, "FFI Parser: %s", message);
	}

	efree(message);
	va_end(va);

	LONGJMP(FFI_G(bailout), FAILURE);
}
/* }}} */

static void clib_ffi_finalize_type(clib_ffi_dcl *dcl) /* {{{ */
{
	if (!dcl->type) {
		switch (dcl->flags & CLIB_FFI_DCL_TYPE_SPECIFIERS) {
			case CLIB_FFI_DCL_VOID:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_void;
				break;
			case CLIB_FFI_DCL_CHAR:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_char;
				break;
			case CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_SIGNED:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_sint8;
				break;
			case CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_UNSIGNED:
			case CLIB_FFI_DCL_BOOL:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_uint8;
				break;
			case CLIB_FFI_DCL_SHORT:
			case CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_SIGNED:
			case CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_INT:
			case CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_SIGNED|CLIB_FFI_DCL_INT:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_sint16;
				break;
			case CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_UNSIGNED:
			case CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_UNSIGNED|CLIB_FFI_DCL_INT:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_uint16;
				break;
			case CLIB_FFI_DCL_INT:
			case CLIB_FFI_DCL_SIGNED:
			case CLIB_FFI_DCL_SIGNED|CLIB_FFI_DCL_INT:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_sint32;
				break;
			case CLIB_FFI_DCL_UNSIGNED:
			case CLIB_FFI_DCL_UNSIGNED|CLIB_FFI_DCL_INT:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_uint32;
				break;
			case CLIB_FFI_DCL_LONG:
			case CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_SIGNED:
			case CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_INT:
			case CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_SIGNED|CLIB_FFI_DCL_INT:
				if (sizeof(long) == 4) {
					dcl->type = (clib_ffi_type*)&clib_ffi_type_sint32;
				} else {
					dcl->type = (clib_ffi_type*)&clib_ffi_type_sint64;
				}
				break;
			case CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_UNSIGNED:
			case CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_UNSIGNED|CLIB_FFI_DCL_INT:
				if (sizeof(long) == 4) {
					dcl->type = (clib_ffi_type*)&clib_ffi_type_uint32;
				} else {
					dcl->type = (clib_ffi_type*)&clib_ffi_type_uint64;
				}
				break;
			case CLIB_FFI_DCL_LONG_LONG|CLIB_FFI_DCL_LONG:
			case CLIB_FFI_DCL_LONG_LONG|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_SIGNED:
			case CLIB_FFI_DCL_LONG_LONG|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_INT:
			case CLIB_FFI_DCL_LONG_LONG|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_SIGNED|CLIB_FFI_DCL_INT:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_sint64;
				break;
			case CLIB_FFI_DCL_LONG_LONG|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_UNSIGNED:
			case CLIB_FFI_DCL_LONG_LONG|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_UNSIGNED|CLIB_FFI_DCL_INT:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_uint64;
				break;
			case CLIB_FFI_DCL_FLOAT:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_float;
				break;
			case CLIB_FFI_DCL_DOUBLE:
				dcl->type = (clib_ffi_type*)&clib_ffi_type_double;
				break;
			case CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_DOUBLE:
#ifdef _WIN32
				dcl->type = (clib_ffi_type*)&clib_ffi_type_double;
#else
				dcl->type = (clib_ffi_type*)&clib_ffi_type_long_double;
#endif
				break;
			case CLIB_FFI_DCL_FLOAT|CLIB_FFI_DCL_COMPLEX:
			case CLIB_FFI_DCL_DOUBLE|CLIB_FFI_DCL_COMPLEX:
			case CLIB_FFI_DCL_DOUBLE|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_COMPLEX:
				clib_ffi_parser_error("Unsupported type _Complex at line %d", FFI_G(line));
				break;
			default:
				clib_ffi_parser_error("Unsupported type specifier combination at line %d", FFI_G(line));
				break;
		}
		dcl->flags &= ~CLIB_FFI_DCL_TYPE_SPECIFIERS;
		dcl->flags |= CLIB_FFI_DCL_TYPEDEF_NAME;
	}
}
/* }}} */

bool clib_ffi_is_typedef_name(const char *name, size_t name_len) /* {{{ */
{
	clib_ffi_symbol *sym;
	clib_ffi_type *type;

	if (FFI_G(symbols)) {
		sym = clib_hash_str_find_ptr(FFI_G(symbols), name, name_len);
		if (sym) {
			return (sym->kind == CLIB_FFI_SYM_TYPE);
		}
	}
	type = clib_hash_str_find_ptr(&FFI_G(types), name, name_len);
	if (type) {
		return 1;
	}
	return 0;
}
/* }}} */

void clib_ffi_resolve_typedef(const char *name, size_t name_len, clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_symbol *sym;
	clib_ffi_type *type;

	if (FFI_G(symbols)) {
		sym = clib_hash_str_find_ptr(FFI_G(symbols), name, name_len);
		if (sym && sym->kind == CLIB_FFI_SYM_TYPE) {
			dcl->type = CLIB_FFI_TYPE(sym->type);;
			if (sym->is_const) {
				dcl->attr |= CLIB_FFI_ATTR_CONST;
			}
			return;
		}
	}
	type = clib_hash_str_find_ptr(&FFI_G(types), name, name_len);
	if (type) {
		dcl->type = type;
		return;
	}
	clib_ffi_parser_error("Undefined C type \"%.*s\" at line %d", name_len, name, FFI_G(line));
}
/* }}} */

void clib_ffi_resolve_const(const char *name, size_t name_len, clib_ffi_val *val) /* {{{ */
{
	clib_ffi_symbol *sym;

	if (UNEXPECTED(FFI_G(attribute_parsing))) {
		val->kind = CLIB_FFI_VAL_NAME;
		val->str = name;
		val->len = name_len;
		return;
	} else if (FFI_G(symbols)) {
		sym = clib_hash_str_find_ptr(FFI_G(symbols), name, name_len);
		if (sym && sym->kind == CLIB_FFI_SYM_CONST) {
			val->i64 = sym->value;
			switch (sym->type->kind) {
				case CLIB_FFI_TYPE_SINT8:
				case CLIB_FFI_TYPE_SINT16:
				case CLIB_FFI_TYPE_SINT32:
					val->kind = CLIB_FFI_VAL_INT32;
					break;
				case CLIB_FFI_TYPE_SINT64:
					val->kind = CLIB_FFI_VAL_INT64;
					break;
				case CLIB_FFI_TYPE_UINT8:
				case CLIB_FFI_TYPE_UINT16:
				case CLIB_FFI_TYPE_UINT32:
					val->kind = CLIB_FFI_VAL_UINT32;
					break;
				case CLIB_FFI_TYPE_UINT64:
					val->kind = CLIB_FFI_VAL_UINT64;
					break;
				default:
					CLIB_UNREACHABLE();
			}
			return;
		}
	}
	val->kind = CLIB_FFI_VAL_ERROR;
}
/* }}} */

void clib_ffi_make_enum_type(clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_type *type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));
	type->kind = CLIB_FFI_TYPE_ENUM;
	type->attr = FFI_G(default_type_attr) | (dcl->attr & CLIB_FFI_ENUM_ATTRS);
	type->enumeration.tag_name = NULL;
	if (type->attr & CLIB_FFI_ATTR_PACKED) {
		type->size = clib_ffi_type_uint8.size;
		type->align = clib_ffi_type_uint8.align;
		type->enumeration.kind = CLIB_FFI_TYPE_UINT8;
	} else {
		type->size = clib_ffi_type_uint32.size;
		type->align = clib_ffi_type_uint32.align;
		type->enumeration.kind = CLIB_FFI_TYPE_UINT32;
	}
	dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(type);
	dcl->attr &= ~CLIB_FFI_ENUM_ATTRS;
}
/* }}} */

void clib_ffi_add_enum_val(clib_ffi_dcl *enum_dcl, const char *name, size_t name_len, clib_ffi_val *val, int64_t *min, int64_t *max, int64_t *last) /* {{{ */
{
	clib_ffi_symbol *sym;
	const clib_ffi_type *sym_type;
	int64_t value;
	clib_ffi_type *enum_type = CLIB_FFI_TYPE(enum_dcl->type);
	bool overflow = 0;
	bool is_signed =
		(enum_type->enumeration.kind == CLIB_FFI_TYPE_SINT8 ||
		 enum_type->enumeration.kind == CLIB_FFI_TYPE_SINT16 ||
		 enum_type->enumeration.kind == CLIB_FFI_TYPE_SINT32 ||
		 enum_type->enumeration.kind == CLIB_FFI_TYPE_SINT64);

	CLIB_ASSERT(enum_type && enum_type->kind == CLIB_FFI_TYPE_ENUM);
	if (val->kind == CLIB_FFI_VAL_EMPTY) {
		if (is_signed) {
			if (*last == 0x7FFFFFFFFFFFFFFFLL) {
				overflow = 1;
			}
		} else {
			if ((*min != 0 || *max != 0)
			 && (uint64_t)*last == 0xFFFFFFFFFFFFFFFFULL) {
				overflow = 1;
			}
		}
		value = *last + 1;
	} else if (val->kind == CLIB_FFI_VAL_CHAR) {
		if (!is_signed && val->ch < 0) {
			if ((uint64_t)*max > 0x7FFFFFFFFFFFFFFFULL) {
				overflow = 1;
			} else {
				is_signed = 1;
			}
		}
		value = val->ch;
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
		if (!is_signed && val->i64 < 0) {
			if ((uint64_t)*max > 0x7FFFFFFFFFFFFFFFULL) {
				overflow = 1;
			} else {
				is_signed = 1;
			}
		}
		value = val->i64;
	} else if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
		if (is_signed && val->u64 > 0x7FFFFFFFFFFFFFFFULL) {
			overflow = 1;
		}
		value = val->u64;
	} else {
		clib_ffi_parser_error("Enumerator value \"%.*s\" must be an integer at line %d", name_len, name, FFI_G(line));
		return;
	}

	if (overflow) {
		clib_ffi_parser_error("Overflow in enumeration values \"%.*s\" at line %d", name_len, name, FFI_G(line));
		return;
	}

	if (is_signed) {
		*min = MIN(*min, value);
		*max = MAX(*max, value);
		if ((enum_type->attr & CLIB_FFI_ATTR_PACKED)
		 && *min >= -0x7FLL-1 && *max <= 0x7FLL) {
			sym_type = &clib_ffi_type_sint8;
		} else if ((enum_type->attr & CLIB_FFI_ATTR_PACKED)
		 && *min >= -0x7FFFLL-1 && *max <= 0x7FFFLL) {
			sym_type = &clib_ffi_type_sint16;
		} else if (*min >= -0x7FFFFFFFLL-1 && *max <= 0x7FFFFFFFLL) {
			sym_type = &clib_ffi_type_sint32;
		} else {
			sym_type = &clib_ffi_type_sint64;
		}
	} else {
		*min = MIN((uint64_t)*min, (uint64_t)value);
		*max = MAX((uint64_t)*max, (uint64_t)value);
		if ((enum_type->attr & CLIB_FFI_ATTR_PACKED)
		 && (uint64_t)*max <= 0xFFULL) {
			sym_type = &clib_ffi_type_uint8;
		} else if ((enum_type->attr & CLIB_FFI_ATTR_PACKED)
		 && (uint64_t)*max <= 0xFFFFULL) {
			sym_type = &clib_ffi_type_uint16;
		} else if ((uint64_t)*max <= 0xFFFFFFFFULL) {
			sym_type = &clib_ffi_type_uint32;
		} else {
			sym_type = &clib_ffi_type_uint64;
		}
	}
	enum_type->enumeration.kind = sym_type->kind;
	enum_type->size = sym_type->size;
	enum_type->align = sym_type->align;
	*last = value;

	if (!FFI_G(symbols)) {
		FFI_G(symbols) = pemalloc(sizeof(b_obj_dict), FFI_G(persistent));
		clib_hash_init(FFI_G(symbols), 0, NULL, FFI_G(persistent) ? clib_ffi_symbol_hash_persistent_dtor : clib_ffi_symbol_hash_dtor, FFI_G(persistent));
	}
	sym = clib_hash_str_find_ptr(FFI_G(symbols), name, name_len);
	if (sym) {
		clib_ffi_parser_error("Redeclaration of \"%.*s\" at line %d", name_len, name, FFI_G(line));
	} else {
		sym = pemalloc(sizeof(clib_ffi_symbol), FFI_G(persistent));
		sym->kind  = CLIB_FFI_SYM_CONST;
		sym->type  = (clib_ffi_type*)sym_type;
		sym->value = value;
		clib_hash_str_add_new_ptr(FFI_G(symbols), name, name_len, sym);
	}
}
/* }}} */

void clib_ffi_make_struct_type(clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_type *type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));
	type->kind = CLIB_FFI_TYPE_STRUCT;
	type->attr = FFI_G(default_type_attr) | (dcl->attr & CLIB_FFI_STRUCT_ATTRS);
	type->size = 0;
	type->align = dcl->align > 1 ? dcl->align : 1;
	if (dcl->flags & CLIB_FFI_DCL_UNION) {
		type->attr |= CLIB_FFI_ATTR_UNION;
	}
	dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(type);
	type->record.tag_name = NULL;
	clib_hash_init(&type->record.fields, 0, NULL, FFI_G(persistent) ? clib_ffi_field_hash_persistent_dtor :clib_ffi_field_hash_dtor, FFI_G(persistent));
	dcl->attr &= ~CLIB_FFI_STRUCT_ATTRS;
	dcl->align = 0;
}
/* }}} */

static clib_result clib_ffi_validate_prev_field_type(clib_ffi_type *struct_type) /* {{{ */
{
	if (clib_hash_num_elements(&struct_type->record.fields) > 0) {
		clib_ffi_field *field = NULL;

		CLIB_HASH_MAP_REVERSE_FOREACH_PTR(&struct_type->record.fields, field) {
			break;
		} CLIB_HASH_FOREACH_END();
		if (CLIB_FFI_TYPE(field->type)->attr & CLIB_FFI_ATTR_INCOMPLETE_ARRAY) {
			clib_ffi_throw_parser_error("Flexible array member not at end of struct at line %d", FFI_G(line));
			return FAILURE;
		}
	}
	return SUCCESS;
}
/* }}} */

static clib_result clib_ffi_validate_field_type(clib_ffi_type *type, clib_ffi_type *struct_type) /* {{{ */
{
	if (type == struct_type) {
		clib_ffi_throw_parser_error("Struct/union can't contain an instance of itself at line %d", FFI_G(line));
		return FAILURE;
	} else if (clib_ffi_validate_var_type(type, 1) == FAILURE) {
		return FAILURE;
	} else if (struct_type->attr & CLIB_FFI_ATTR_UNION) {
		if (type->attr & CLIB_FFI_ATTR_INCOMPLETE_ARRAY) {
			clib_ffi_throw_parser_error("Flexible array member in union at line %d", FFI_G(line));
			return FAILURE;
		}
	}
	return clib_ffi_validate_prev_field_type(struct_type);
}
/* }}} */

void clib_ffi_add_field(clib_ffi_dcl *struct_dcl, const char *name, size_t name_len, clib_ffi_dcl *field_dcl) /* {{{ */
{
	clib_ffi_field *field;
	clib_ffi_type *struct_type = CLIB_FFI_TYPE(struct_dcl->type);
	clib_ffi_type *field_type;

	CLIB_ASSERT(struct_type && struct_type->kind == CLIB_FFI_TYPE_STRUCT);
	clib_ffi_finalize_type(field_dcl);
	field_type = CLIB_FFI_TYPE(field_dcl->type);
	if (clib_ffi_validate_field_type(field_type, struct_type) == FAILURE) {
		clib_ffi_cleanup_dcl(field_dcl);
		LONGJMP(FFI_G(bailout), FAILURE);
	}

	field = pemalloc(sizeof(clib_ffi_field), FFI_G(persistent));
	if (!(struct_type->attr & CLIB_FFI_ATTR_PACKED) && !(field_dcl->attr & CLIB_FFI_ATTR_PACKED)) {
		struct_type->align = MAX(struct_type->align, MAX(field_type->align, field_dcl->align));
	}
	if (struct_type->attr & CLIB_FFI_ATTR_UNION) {
		field->offset = 0;
		struct_type->size = MAX(struct_type->size, field_type->size);
	} else {
		if (!(struct_type->attr & CLIB_FFI_ATTR_PACKED) && !(field_dcl->attr & CLIB_FFI_ATTR_PACKED)) {
			uint32_t field_align = MAX(field_type->align, field_dcl->align);
			struct_type->size = ((struct_type->size + (field_align - 1)) / field_align) * field_align;
		}
		field->offset = struct_type->size;
		struct_type->size += field_type->size;
	}
	field->type = field_dcl->type;
	field->is_const = (bool)(field_dcl->attr & CLIB_FFI_ATTR_CONST);
	field->is_nested = 0;
	field->first_bit = 0;
	field->bits = 0;
	field_dcl->type = field_type; /* reset "owned" flag */

	if (!clib_hash_str_add_ptr(&struct_type->record.fields, name, name_len, field)) {
		clib_ffi_type_dtor(field->type);
		pefree(field, FFI_G(persistent));
		clib_ffi_parser_error("Duplicate field name \"%.*s\" at line %d", name_len, name, FFI_G(line));
	}
}
/* }}} */

void clib_ffi_add_anonymous_field(clib_ffi_dcl *struct_dcl, clib_ffi_dcl *field_dcl) /* {{{ */
{
	clib_ffi_type *struct_type = CLIB_FFI_TYPE(struct_dcl->type);
	clib_ffi_type *field_type;
	clib_ffi_field *field;
	b_obj_string *key;

	CLIB_ASSERT(struct_type && struct_type->kind == CLIB_FFI_TYPE_STRUCT);
	clib_ffi_finalize_type(field_dcl);
	field_type = CLIB_FFI_TYPE(field_dcl->type);
	if (field_type->kind != CLIB_FFI_TYPE_STRUCT) {
		clib_ffi_cleanup_dcl(field_dcl);
		clib_ffi_parser_error("Declaration does not declare anything at line %d", FFI_G(line));
		return;
	}

	if (!(struct_type->attr & CLIB_FFI_ATTR_PACKED) && !(field_dcl->attr & CLIB_FFI_ATTR_PACKED)) {
		struct_type->align = MAX(struct_type->align, MAX(field_type->align, field_dcl->align));
	}
	if (!(struct_type->attr & CLIB_FFI_ATTR_UNION)) {
		if (clib_ffi_validate_prev_field_type(struct_type) == FAILURE) {
			clib_ffi_cleanup_dcl(field_dcl);
			LONGJMP(FFI_G(bailout), FAILURE);
		}
		if (!(struct_type->attr & CLIB_FFI_ATTR_PACKED) && !(field_dcl->attr & CLIB_FFI_ATTR_PACKED)) {
			uint32_t field_align = MAX(field_type->align, field_dcl->align);
			struct_type->size = ((struct_type->size + (field_align - 1)) / field_align) * field_align;
		}
	}

	CLIB_HASH_MAP_FOREACH_STR_KEY_PTR(&field_type->record.fields, key, field) {
		clib_ffi_field *new_field = pemalloc(sizeof(clib_ffi_field), FFI_G(persistent));

		if (struct_type->attr & CLIB_FFI_ATTR_UNION) {
			new_field->offset = field->offset;
		} else {
			new_field->offset = struct_type->size + field->offset;
		}
		new_field->type = field->type;
		new_field->is_const = field->is_const;
		new_field->is_nested = 1;
		new_field->first_bit = field->first_bit;
		new_field->bits = field->bits;
		field->type = CLIB_FFI_TYPE(field->type); /* reset "owned" flag */

		if (key) {
			if (!clib_hash_add_ptr(&struct_type->record.fields, key, new_field)) {
				clib_ffi_type_dtor(new_field->type);
				pefree(new_field, FFI_G(persistent));
				clib_ffi_parser_error("Duplicate field name \"%s\" at line %d", ZSTR_VAL(key), FFI_G(line));
				return;
			}
		} else {
			clib_hash_next_index_insert_ptr(&struct_type->record.fields, field);
		}
	} CLIB_HASH_FOREACH_END();

	if (struct_type->attr & CLIB_FFI_ATTR_UNION) {
		struct_type->size = MAX(struct_type->size, field_type->size);
	} else {
		struct_type->size += field_type->size;
	}

	clib_ffi_type_dtor(field_dcl->type);
	field_dcl->type = NULL;
}
/* }}} */

void clib_ffi_add_bit_field(clib_ffi_dcl *struct_dcl, const char *name, size_t name_len, clib_ffi_dcl *field_dcl, clib_ffi_val *bits) /* {{{ */
{
	clib_ffi_type *struct_type = CLIB_FFI_TYPE(struct_dcl->type);
	clib_ffi_type *field_type;
	clib_ffi_field *field;

	CLIB_ASSERT(struct_type && struct_type->kind == CLIB_FFI_TYPE_STRUCT);
	clib_ffi_finalize_type(field_dcl);
	field_type = CLIB_FFI_TYPE(field_dcl->type);
	if (clib_ffi_validate_field_type(field_type, struct_type) == FAILURE) {
		clib_ffi_cleanup_dcl(field_dcl);
		LONGJMP(FFI_G(bailout), FAILURE);
	}

	if (field_type->kind < CLIB_FFI_TYPE_UINT8 || field_type->kind > CLIB_FFI_TYPE_BOOL) {
		clib_ffi_cleanup_dcl(field_dcl);
		clib_ffi_parser_error("Wrong type of bit field \"%.*s\" at line %d", name ? name_len : sizeof("<anonymous>")-1, name ? name : "<anonymous>", FFI_G(line));
	}

	if (bits->kind == CLIB_FFI_VAL_INT32 || bits->kind == CLIB_FFI_VAL_INT64) {
		if (bits->i64 < 0) {
			clib_ffi_cleanup_dcl(field_dcl);
			clib_ffi_parser_error("Negative width in bit-field \"%.*s\" at line %d", name ? name_len : sizeof("<anonymous>")-1, name ? name : "<anonymous>", FFI_G(line));
		} else if (bits->i64 == 0) {
			clib_ffi_cleanup_dcl(field_dcl);
			if (name) {
				clib_ffi_parser_error("Zero width in bit-field \"%.*s\" at line %d", name ? name_len : sizeof("<anonymous>")-1, name ? name : "<anonymous>", FFI_G(line));
			}
			return;
		} else if (bits->i64 > field_type->size * 8) {
			clib_ffi_cleanup_dcl(field_dcl);
			clib_ffi_parser_error("Width of \"%.*s\" exceeds its type at line %d", name ? name_len : sizeof("<anonymous>")-1, name ? name : "<anonymous>", FFI_G(line));
		}
	} else if (bits->kind == CLIB_FFI_VAL_UINT32 || bits->kind == CLIB_FFI_VAL_UINT64) {
		if (bits->u64 == 0) {
			clib_ffi_cleanup_dcl(field_dcl);
			if (name) {
				clib_ffi_parser_error("Zero width in bit-field \"%.*s\" at line %d", name ? name_len : sizeof("<anonymous>")-1, name ? name : "<anonymous>", FFI_G(line));
			}
			return;
		} else if (bits->u64 > field_type->size * 8) {
			clib_ffi_cleanup_dcl(field_dcl);
			clib_ffi_parser_error("Width of \"%.*s\" exceeds its type at line %d", name ? name_len : sizeof("<anonymous>")-1, name ? name : "<anonymous>", FFI_G(line));
		}
	} else {
		clib_ffi_cleanup_dcl(field_dcl);
		clib_ffi_parser_error("Bit field \"%.*s\" width not an integer constant at line %d", name ? name_len : sizeof("<anonymous>")-1, name ? name : "<anonymous>", FFI_G(line));
	}

	field = pemalloc(sizeof(clib_ffi_field), FFI_G(persistent));
	if (!(struct_type->attr & CLIB_FFI_ATTR_PACKED)) {
		struct_type->align = MAX(struct_type->align, sizeof(uint32_t));
	}
	if (struct_type->attr & CLIB_FFI_ATTR_UNION) {
		field->offset = 0;
		field->first_bit = 0;
		field->bits = bits->u64;
		if (struct_type->attr & CLIB_FFI_ATTR_PACKED) {
			struct_type->size = MAX(struct_type->size, (bits->u64 + 7) / 8);
		} else {
			struct_type->size = MAX(struct_type->size, ((bits->u64 + 31) / 32) * 4);
		}
	} else {
		clib_ffi_field *prev_field = NULL;

		if (clib_hash_num_elements(&struct_type->record.fields) > 0) {
			CLIB_HASH_MAP_REVERSE_FOREACH_PTR(&struct_type->record.fields, prev_field) {
				break;
			} CLIB_HASH_FOREACH_END();
		}
		if (prev_field && prev_field->bits) {
			field->offset = prev_field->offset;
			field->first_bit = prev_field->first_bit + prev_field->bits;
			field->bits = bits->u64;
		} else {
			field->offset = struct_type->size;
			field->first_bit = 0;
			field->bits = bits->u64;
		}
		if (struct_type->attr & CLIB_FFI_ATTR_PACKED) {
			struct_type->size = field->offset + ((field->first_bit + field->bits) + 7) / 8;
		} else {
			struct_type->size = field->offset + (((field->first_bit + field->bits) + 31) / 32) * 4;
		}
	}
	field->type = field_dcl->type;
	field->is_const = (bool)(field_dcl->attr & CLIB_FFI_ATTR_CONST);
	field->is_nested = 0;
	field_dcl->type = field_type; /* reset "owned" flag */

	if (name) {
		if (!clib_hash_str_add_ptr(&struct_type->record.fields, name, name_len, field)) {
			clib_ffi_type_dtor(field->type);
			pefree(field, FFI_G(persistent));
			clib_ffi_parser_error("Duplicate field name \"%.*s\" at line %d", name_len, name, FFI_G(line));
		}
	} else {
		clib_hash_next_index_insert_ptr(&struct_type->record.fields, field);
	}
}
/* }}} */

void clib_ffi_adjust_struct_size(clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_type *struct_type = CLIB_FFI_TYPE(dcl->type);

	CLIB_ASSERT(struct_type->kind == CLIB_FFI_TYPE_STRUCT);
	if (dcl->align > struct_type->align) {
		struct_type->align = dcl->align;
	}
	if (!(struct_type->attr & CLIB_FFI_ATTR_PACKED)) {
		struct_type->size = ((struct_type->size + (struct_type->align - 1)) / struct_type->align) * struct_type->align;
	}
	dcl->align = 0;
}
/* }}} */

void clib_ffi_make_pointer_type(clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_type *type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));
	type->kind = CLIB_FFI_TYPE_POINTER;
	type->attr = FFI_G(default_type_attr) | (dcl->attr & CLIB_FFI_POINTER_ATTRS);
	type->size = sizeof(void*);
	type->align = _Alignof(void*);
	clib_ffi_finalize_type(dcl);
	if (clib_ffi_validate_vla(CLIB_FFI_TYPE(dcl->type)) == FAILURE) {
		clib_ffi_cleanup_dcl(dcl);
		LONGJMP(FFI_G(bailout), FAILURE);
	}
	type->pointer.type = dcl->type;
	dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(type);
	dcl->flags &= ~CLIB_FFI_DCL_TYPE_QUALIFIERS;
	dcl->attr &= ~CLIB_FFI_POINTER_ATTRS;
	dcl->align = 0;
}
/* }}} */

static clib_result clib_ffi_validate_array_element_type(clib_ffi_type *type) /* {{{ */
{
	if (type->kind == CLIB_FFI_TYPE_FUNC) {
		clib_ffi_throw_parser_error("Array of functions is not allowed at line %d", FFI_G(line));
		return FAILURE;
	} else if (type->kind == CLIB_FFI_TYPE_ARRAY && (type->attr & CLIB_FFI_ATTR_INCOMPLETE_ARRAY)) {
		clib_ffi_throw_parser_error("Only the leftmost array can be undimensioned at line %d", FFI_G(line));
		return FAILURE;
	}
	return clib_ffi_validate_type(type, 0, 1);
}
/* }}} */

void clib_ffi_make_array_type(clib_ffi_dcl *dcl, clib_ffi_val *len) /* {{{ */
{
	int length = 0;
	clib_ffi_type *element_type;
	clib_ffi_type *type;

	clib_ffi_finalize_type(dcl);
	element_type = CLIB_FFI_TYPE(dcl->type);

	if (len->kind == CLIB_FFI_VAL_EMPTY) {
		length = 0;
	} else if (len->kind == CLIB_FFI_VAL_UINT32 || len->kind == CLIB_FFI_VAL_UINT64) {
		length = len->u64;
	} else if (len->kind == CLIB_FFI_VAL_INT32 || len->kind == CLIB_FFI_VAL_INT64) {
		length = len->i64;
	} else if (len->kind == CLIB_FFI_VAL_CHAR) {
		length = len->ch;
	} else {
		clib_ffi_cleanup_dcl(dcl);
		clib_ffi_parser_error("Unsupported array index type at line %d", FFI_G(line));
		return;
	}
	if (length < 0) {
		clib_ffi_cleanup_dcl(dcl);
		clib_ffi_parser_error("Negative array index at line %d", FFI_G(line));
		return;
	}

	if (clib_ffi_validate_array_element_type(element_type) == FAILURE) {
		clib_ffi_cleanup_dcl(dcl);
		LONGJMP(FFI_G(bailout), FAILURE);
	}

	type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));
	type->kind = CLIB_FFI_TYPE_ARRAY;
	type->attr = FFI_G(default_type_attr) | (dcl->attr & CLIB_FFI_ARRAY_ATTRS);
	type->size = length * element_type->size;
	type->align = element_type->align;
	type->array.type = dcl->type;
	type->array.length = length;
	dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(type);
	dcl->flags &= ~CLIB_FFI_DCL_TYPE_QUALIFIERS;
	dcl->attr &= ~CLIB_FFI_ARRAY_ATTRS;
	dcl->align = 0;
}
/* }}} */

static clib_result clib_ffi_validate_func_ret_type(clib_ffi_type *type) /* {{{ */
{
	if (type->kind == CLIB_FFI_TYPE_FUNC) {
		clib_ffi_throw_parser_error("Function returning function is not allowed at line %d", FFI_G(line));
		return FAILURE;
	 } else if (type->kind == CLIB_FFI_TYPE_ARRAY) {
		clib_ffi_throw_parser_error("Function returning array is not allowed at line %d", FFI_G(line));
		return FAILURE;
	}
	return clib_ffi_validate_incomplete_type(type, 1, 0);
}
/* }}} */

void clib_ffi_make_func_type(clib_ffi_dcl *dcl, b_obj_dict *args, clib_ffi_dcl *nested_dcl) /* {{{ */
{
	clib_ffi_type *type;
	clib_ffi_type *ret_type;

	clib_ffi_finalize_type(dcl);
	ret_type = CLIB_FFI_TYPE(dcl->type);

	if (args) {
		int no_args = 0;
		clib_ffi_type *arg_type;

		CLIB_HASH_PACKED_FOREACH_PTR(args, arg_type) {
			arg_type = CLIB_FFI_TYPE(arg_type);
			if (arg_type->kind == CLIB_FFI_TYPE_VOID) {
				if (clib_hash_num_elements(args) != 1) {
					clib_ffi_cleanup_dcl(nested_dcl);
					clib_ffi_cleanup_dcl(dcl);
					clib_hash_destroy(args);
					pefree(args, FFI_G(persistent));
					clib_ffi_parser_error("void type is not allowed at line %d", FFI_G(line));
					return;
				} else {
					no_args = 1;
				}
			}
		} CLIB_HASH_FOREACH_END();
		if (no_args) {
			clib_hash_destroy(args);
			pefree(args, FFI_G(persistent));
			args = NULL;
		}
	}

#ifdef HAVE_FFI_VECTORCALL_PARTIAL
	if (dcl->abi == CLIB_FFI_ABI_VECTORCALL && args) {
		clib_ulong i;
		clib_ffi_type *arg_type;

		CLIB_HASH_PACKED_FOREACH_KEY_PTR(args, i, arg_type) {
			arg_type = CLIB_FFI_TYPE(arg_type);
# ifdef _WIN64
			if (i >= 4 && i <= 5 && (arg_type->kind == CLIB_FFI_TYPE_FLOAT || arg_type->kind == CLIB_FFI_TYPE_DOUBLE)) {
# else
			if (i < 6 && (arg_type->kind == CLIB_FFI_TYPE_FLOAT || arg_type->kind == CLIB_FFI_TYPE_DOUBLE)) {
# endif
				clib_ffi_cleanup_dcl(nested_dcl);
				clib_ffi_cleanup_dcl(dcl);
				clib_hash_destroy(args);
				pefree(args, FFI_G(persistent));
				clib_ffi_parser_error("Type float/double is not allowed at position " CLIB_ULONG_FMT " with __vectorcall at line %d", i+1, FFI_G(line));
				return;
			}
		} CLIB_HASH_FOREACH_END();
	}
#endif

	if (clib_ffi_validate_func_ret_type(ret_type) == FAILURE) {
		clib_ffi_cleanup_dcl(nested_dcl);
		clib_ffi_cleanup_dcl(dcl);
		if (args) {
			clib_hash_destroy(args);
			pefree(args, FFI_G(persistent));
		}
		LONGJMP(FFI_G(bailout), FAILURE);
	}

	type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));
	type->kind = CLIB_FFI_TYPE_FUNC;
	type->attr = FFI_G(default_type_attr) | (dcl->attr & CLIB_FFI_FUNC_ATTRS);
	type->size = sizeof(void*);
	type->align = 1;
	type->func.ret_type = dcl->type;
	switch (dcl->abi) {
		case CLIB_FFI_ABI_DEFAULT:
		case CLIB_FFI_ABI_CDECL:
			type->func.abi = FFI_DEFAULT_ABI;
			break;
#ifdef HAVE_FFI_FASTCALL
		case CLIB_FFI_ABI_FASTCALL:
			type->func.abi = FFI_FASTCALL;
			break;
#endif
#ifdef HAVE_FFI_THISCALL
		case CLIB_FFI_ABI_THISCALL:
			type->func.abi = FFI_THISCALL;
			break;
#endif
#ifdef HAVE_FFI_STDCALL
		case CLIB_FFI_ABI_STDCALL:
			type->func.abi = FFI_STDCALL;
			break;
#endif
#ifdef HAVE_FFI_PASCAL
		case CLIB_FFI_ABI_PASCAL:
			type->func.abi = FFI_PASCAL;
			break;
#endif
#ifdef HAVE_FFI_REGISTER
		case CLIB_FFI_ABI_REGISTER:
			type->func.abi = FFI_REGISTER;
			break;
#endif
#ifdef HAVE_FFI_MS_CDECL
		case CLIB_FFI_ABI_MS:
			type->func.abi = FFI_MS_CDECL;
			break;
#endif
#ifdef HAVE_FFI_SYSV
		case CLIB_FFI_ABI_SYSV:
			type->func.abi = FFI_SYSV;
			break;
#endif
#ifdef HAVE_FFI_VECTORCALL_PARTIAL
		case CLIB_FFI_ABI_VECTORCALL:
			type->func.abi = FFI_VECTORCALL_PARTIAL;
			break;
#endif
		default:
			type->func.abi = FFI_DEFAULT_ABI;
			clib_ffi_cleanup_dcl(nested_dcl);
			if (args) {
				clib_hash_destroy(args);
				pefree(args, FFI_G(persistent));
			}
			type->func.args = NULL;
			_clib_ffi_type_dtor(type);
			clib_ffi_parser_error("Unsupported calling convention line %d", FFI_G(line));
			break;
	}
	type->func.args = args;
	dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(type);
	dcl->attr &= ~CLIB_FFI_FUNC_ATTRS;
	dcl->align = 0;
	dcl->abi = 0;
}
/* }}} */

void clib_ffi_add_arg(b_obj_dict **args, const char *name, size_t name_len, clib_ffi_dcl *arg_dcl) /* {{{ */
{
	clib_ffi_type *type;

	if (!*args) {
		*args = pemalloc(sizeof(b_obj_dict), FFI_G(persistent));
		clib_hash_init(*args, 0, NULL, clib_ffi_type_hash_dtor, FFI_G(persistent));
	}
	clib_ffi_finalize_type(arg_dcl);
	type = CLIB_FFI_TYPE(arg_dcl->type);
	if (type->kind == CLIB_FFI_TYPE_ARRAY) {
		if (CLIB_FFI_TYPE_IS_OWNED(arg_dcl->type)) {
			type->kind = CLIB_FFI_TYPE_POINTER;
			type->size = sizeof(void*);
		} else {
			clib_ffi_type *new_type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));
			new_type->kind = CLIB_FFI_TYPE_POINTER;
			new_type->attr = FFI_G(default_type_attr) | (type->attr & CLIB_FFI_POINTER_ATTRS);
			new_type->size = sizeof(void*);
			new_type->align = _Alignof(void*);
			new_type->pointer.type = CLIB_FFI_TYPE(type->array.type);
			arg_dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(new_type);
		}
	} else if (type->kind == CLIB_FFI_TYPE_FUNC) {
		clib_ffi_type *new_type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));
		new_type->kind = CLIB_FFI_TYPE_POINTER;
		new_type->attr = FFI_G(default_type_attr);
		new_type->size = sizeof(void*);
		new_type->align = _Alignof(void*);
		new_type->pointer.type = arg_dcl->type;
		arg_dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(new_type);
	}
	if (clib_ffi_validate_incomplete_type(type, 1, 1) == FAILURE) {
		clib_ffi_cleanup_dcl(arg_dcl);
		clib_hash_destroy(*args);
		pefree(*args, FFI_G(persistent));
		*args = NULL;
		LONGJMP(FFI_G(bailout), FAILURE);
	}
	clib_hash_next_index_insert_ptr(*args, (void*)arg_dcl->type);
}
/* }}} */

void clib_ffi_declare(const char *name, size_t name_len, clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_symbol *sym;

	if (!FFI_G(symbols)) {
		FFI_G(symbols) = pemalloc(sizeof(b_obj_dict), FFI_G(persistent));
		clib_hash_init(FFI_G(symbols), 0, NULL, FFI_G(persistent) ? clib_ffi_symbol_hash_persistent_dtor : clib_ffi_symbol_hash_dtor, FFI_G(persistent));
	}
	clib_ffi_finalize_type(dcl);
	sym = clib_hash_str_find_ptr(FFI_G(symbols), name, name_len);
	if (sym) {
		if ((dcl->flags & CLIB_FFI_DCL_STORAGE_CLASS) == CLIB_FFI_DCL_TYPEDEF
		 && sym->kind == CLIB_FFI_SYM_TYPE
		 && clib_ffi_is_same_type(CLIB_FFI_TYPE(sym->type), CLIB_FFI_TYPE(dcl->type))
		 && sym->is_const == (bool)(dcl->attr & CLIB_FFI_ATTR_CONST)) {
			/* allowed redeclaration */
			clib_ffi_type_dtor(dcl->type);
			return;
		} else if ((dcl->flags & CLIB_FFI_DCL_STORAGE_CLASS) == 0
		 || (dcl->flags & CLIB_FFI_DCL_STORAGE_CLASS) == CLIB_FFI_DCL_EXTERN) {
			clib_ffi_type *type = CLIB_FFI_TYPE(dcl->type);

			if (type->kind == CLIB_FFI_TYPE_FUNC) {
				if (sym->kind == CLIB_FFI_SYM_FUNC
				 && clib_ffi_same_types(CLIB_FFI_TYPE(sym->type), type)) {
					/* allowed redeclaration */
					clib_ffi_type_dtor(dcl->type);
					return;
				}
			} else {
				if (sym->kind == CLIB_FFI_SYM_VAR
				 && clib_ffi_is_same_type(CLIB_FFI_TYPE(sym->type), type)
				 && sym->is_const == (bool)(dcl->attr & CLIB_FFI_ATTR_CONST)) {
					/* allowed redeclaration */
					clib_ffi_type_dtor(dcl->type);
					return;
				}
			}
		}
		clib_ffi_parser_error("Redeclaration of \"%.*s\" at line %d", name_len, name, FFI_G(line));
	} else {
		if ((dcl->flags & CLIB_FFI_DCL_STORAGE_CLASS) == CLIB_FFI_DCL_TYPEDEF) {
			if (clib_ffi_validate_vla(CLIB_FFI_TYPE(dcl->type)) == FAILURE) {
				clib_ffi_cleanup_dcl(dcl);
				LONGJMP(FFI_G(bailout), FAILURE);
			}
			if (dcl->align && dcl->align > CLIB_FFI_TYPE(dcl->type)->align) {
				if (CLIB_FFI_TYPE_IS_OWNED(dcl->type)) {
					CLIB_FFI_TYPE(dcl->type)->align = dcl->align;
				} else {
					clib_ffi_type *type = pemalloc(sizeof(clib_ffi_type), FFI_G(persistent));

					memcpy(type, CLIB_FFI_TYPE(dcl->type), sizeof(clib_ffi_type));
					type->attr |= FFI_G(default_type_attr);
					type->align = dcl->align;
					dcl->type = CLIB_FFI_TYPE_MAKE_OWNED(type);
				}
			}
			sym = pemalloc(sizeof(clib_ffi_symbol), FFI_G(persistent));
			sym->kind = CLIB_FFI_SYM_TYPE;
			sym->type = dcl->type;
			sym->is_const = (bool)(dcl->attr & CLIB_FFI_ATTR_CONST);
			dcl->type = CLIB_FFI_TYPE(dcl->type); /* reset "owned" flag */
			clib_hash_str_add_new_ptr(FFI_G(symbols), name, name_len, sym);
		} else {
			clib_ffi_type *type;

			type = CLIB_FFI_TYPE(dcl->type);
			if (clib_ffi_validate_type(type, (dcl->flags & CLIB_FFI_DCL_STORAGE_CLASS) == CLIB_FFI_DCL_EXTERN, 1) == FAILURE) {
				clib_ffi_cleanup_dcl(dcl);
				LONGJMP(FFI_G(bailout), FAILURE);
			}
			if ((dcl->flags & CLIB_FFI_DCL_STORAGE_CLASS) == 0 ||
			    (dcl->flags & CLIB_FFI_DCL_STORAGE_CLASS) == CLIB_FFI_DCL_EXTERN) {
				sym = pemalloc(sizeof(clib_ffi_symbol), FFI_G(persistent));
				sym->kind = (type->kind == CLIB_FFI_TYPE_FUNC) ? CLIB_FFI_SYM_FUNC : CLIB_FFI_SYM_VAR;
				sym->type = dcl->type;
				sym->is_const = (bool)(dcl->attr & CLIB_FFI_ATTR_CONST);
				dcl->type = type; /* reset "owned" flag */
				clib_hash_str_add_new_ptr(FFI_G(symbols), name, name_len, sym);
			} else {
				/* useless declarartion */
				clib_ffi_type_dtor(dcl->type);
			}
		}
	}
}
/* }}} */

void clib_ffi_declare_tag(const char *name, size_t name_len, clib_ffi_dcl *dcl, bool incomplete) /* {{{ */
{
	clib_ffi_tag *tag;
	clib_ffi_type *type;

	if (!FFI_G(tags)) {
		FFI_G(tags) = pemalloc(sizeof(b_obj_dict), FFI_G(persistent));
		clib_hash_init(FFI_G(tags), 0, NULL, FFI_G(persistent) ? clib_ffi_tag_hash_persistent_dtor : clib_ffi_tag_hash_dtor, FFI_G(persistent));
	}
	tag = clib_hash_str_find_ptr(FFI_G(tags), name, name_len);
	if (tag) {
		clib_ffi_type *type = CLIB_FFI_TYPE(tag->type);

		if (dcl->flags & CLIB_FFI_DCL_STRUCT) {
			if (tag->kind != CLIB_FFI_TAG_STRUCT) {
				clib_ffi_parser_error("\"%.*s\" defined as wrong kind of tag at line %d", name_len, name, FFI_G(line));
				return;
			} else if (!incomplete && !(type->attr & CLIB_FFI_ATTR_INCOMPLETE_TAG)) {
				clib_ffi_parser_error("Redefinition of \"struct %.*s\" at line %d", name_len, name, FFI_G(line));
				return;
			}
		} else if (dcl->flags & CLIB_FFI_DCL_UNION) {
			if (tag->kind != CLIB_FFI_TAG_UNION) {
				clib_ffi_parser_error("\"%.*s\" defined as wrong kind of tag at line %d", name_len, name, FFI_G(line));
				return;
			} else if (!incomplete && !(type->attr & CLIB_FFI_ATTR_INCOMPLETE_TAG)) {
				clib_ffi_parser_error("Redefinition of \"union %.*s\" at line %d", name_len, name, FFI_G(line));
				return;
			}
		} else if (dcl->flags & CLIB_FFI_DCL_ENUM) {
			if (tag->kind != CLIB_FFI_TAG_ENUM) {
				clib_ffi_parser_error("\"%.*s\" defined as wrong kind of tag at line %d", name_len, name, FFI_G(line));
				return;
			} else if (!incomplete && !(type->attr & CLIB_FFI_ATTR_INCOMPLETE_TAG)) {
				clib_ffi_parser_error("Redefinition of \"enum %.*s\" at line %d", name_len, name, FFI_G(line));
				return;
			}
		} else {
			CLIB_UNREACHABLE();
			return;
		}
		dcl->type = type;
		if (!incomplete) {
			type->attr &= ~CLIB_FFI_ATTR_INCOMPLETE_TAG;
		}
	} else {
		clib_ffi_tag *tag = pemalloc(sizeof(clib_ffi_tag), FFI_G(persistent));
		b_obj_string *tag_name = b_obj_string_init(name, name_len, FFI_G(persistent));

		if (dcl->flags & CLIB_FFI_DCL_STRUCT) {
			tag->kind = CLIB_FFI_TAG_STRUCT;
			clib_ffi_make_struct_type(dcl);
			type = CLIB_FFI_TYPE(dcl->type);
			type->record.tag_name = b_obj_string_copy(tag_name);
		} else if (dcl->flags & CLIB_FFI_DCL_UNION) {
			tag->kind = CLIB_FFI_TAG_UNION;
			clib_ffi_make_struct_type(dcl);
			type = CLIB_FFI_TYPE(dcl->type);
			type->record.tag_name = b_obj_string_copy(tag_name);
		} else if (dcl->flags & CLIB_FFI_DCL_ENUM) {
			tag->kind = CLIB_FFI_TAG_ENUM;
			clib_ffi_make_enum_type(dcl);
			type = CLIB_FFI_TYPE(dcl->type);
			type->enumeration.tag_name = b_obj_string_copy(tag_name);
		} else {
			CLIB_UNREACHABLE();
		}
		tag->type = CLIB_FFI_TYPE_MAKE_OWNED(dcl->type);
		dcl->type = CLIB_FFI_TYPE(dcl->type);
		if (incomplete) {
			dcl->type->attr |= CLIB_FFI_ATTR_INCOMPLETE_TAG;
		}
		clib_hash_add_new_ptr(FFI_G(tags), tag_name, tag);
		b_obj_string_release(tag_name);
	}
}
/* }}} */

void clib_ffi_set_abi(clib_ffi_dcl *dcl, uint16_t abi) /* {{{ */
{
	if (dcl->abi != CLIB_FFI_ABI_DEFAULT) {
		clib_ffi_parser_error("Multiple calling convention specifiers at line %d", FFI_G(line));
	} else {
		dcl->abi = abi;
	}
}
/* }}} */

#define SIMPLE_ATTRIBUTES(_) \
	_(cdecl) \
	_(fastcall) \
	_(thiscall) \
	_(stdcall) \
	_(ms_abi) \
	_(sysv_abi) \
	_(vectorcall) \
	_(aligned) \
	_(packed) \
	_(ms_struct) \
	_(gcc_struct) \
	_(const) \
	_(malloc) \
	_(deprecated) \
	_(nothrow) \
	_(leaf) \
	_(pure) \
	_(noreturn) \
	_(warn_unused_result)

#define ATTR_ID(name)   attr_ ## name,
#define ATTR_NAME(name) {sizeof(#name)-1, #name},

void clib_ffi_add_attribute(clib_ffi_dcl *dcl, const char *name, size_t name_len) /* {{{ */
{
	enum {
		SIMPLE_ATTRIBUTES(ATTR_ID)
		attr_unsupported
	};
	static const struct {
		size_t len;
		const char * const name;
	} names[] = {
		SIMPLE_ATTRIBUTES(ATTR_NAME)
		{0, NULL}
	};
	int id;

	if (name_len > 4
	 && name[0] == '_'
	 && name[1] == '_'
	 && name[name_len-2] == '_'
	 && name[name_len-1] == '_') {
		name += 2;
		name_len -= 4;
	}
	for (id = 0; names[id].len != 0; id++) {
		if (name_len == names[id].len) {
			if (memcmp(name, names[id].name, name_len) == 0) {
				break;
			}
		}
	}
	switch (id) {
		case attr_cdecl:
			clib_ffi_set_abi(dcl, CLIB_FFI_ABI_CDECL);
			break;
		case attr_fastcall:
			clib_ffi_set_abi(dcl, CLIB_FFI_ABI_FASTCALL);
			break;
		case attr_thiscall:
			clib_ffi_set_abi(dcl, CLIB_FFI_ABI_THISCALL);
			break;
		case attr_stdcall:
			clib_ffi_set_abi(dcl, CLIB_FFI_ABI_STDCALL);
			break;
		case attr_ms_abi:
			clib_ffi_set_abi(dcl, CLIB_FFI_ABI_MS);
			break;
		case attr_sysv_abi:
			clib_ffi_set_abi(dcl, CLIB_FFI_ABI_SYSV);
			break;
		case attr_vectorcall:
			clib_ffi_set_abi(dcl, CLIB_FFI_ABI_VECTORCALL);
			break;
		case attr_aligned:
			dcl->align = __BIGGEST_ALIGNMENT__;
			break;
		case attr_packed:
			dcl->attr |= CLIB_FFI_ATTR_PACKED;
			break;
		case attr_ms_struct:
			dcl->attr |= CLIB_FFI_ATTR_MS_STRUCT;
			break;
		case attr_gcc_struct:
			dcl->attr |= CLIB_FFI_ATTR_GCC_STRUCT;
			break;
		case attr_unsupported:
			clib_ffi_parser_error("Unsupported attribute \"%.*s\" at line %d", name_len, name, FFI_G(line));
			break;
		default:
			/* ignore */
			break;
	}
}
/* }}} */

#define VALUE_ATTRIBUTES(_) \
	_(regparam) \
	_(aligned) \
	_(mode) \
	_(nonnull) \
	_(alloc_size) \
	_(format) \
	_(deprecated)

void clib_ffi_add_attribute_value(clib_ffi_dcl *dcl, const char *name, size_t name_len, int n, clib_ffi_val *val) /* {{{ */
{
	enum {
		VALUE_ATTRIBUTES(ATTR_ID)
		attr_unsupported
	};
	static const struct {
		size_t len;
		const char * const name;
	} names[] = {
		VALUE_ATTRIBUTES(ATTR_NAME)
		{0, NULL}
	};
	int id;

	if (name_len > 4
	 && name[0] == '_'
	 && name[1] == '_'
	 && name[name_len-2] == '_'
	 && name[name_len-1] == '_') {
		name += 2;
		name_len -= 4;
	}
	for (id = 0; names[id].len != 0; id++) {
		if (name_len == names[id].len) {
			if (memcmp(name, names[id].name, name_len) == 0) {
				break;
			}
		}
	}
	switch (id) {
		case attr_regparam:
			if (n == 0
			 && (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_INT64 || val->kind == CLIB_FFI_VAL_UINT64)
			 && val->i64 == 3) {
				clib_ffi_set_abi(dcl, CLIB_FFI_ABI_REGISTER);
			} else {
				clib_ffi_parser_error("Incorrect \"regparam\" value at line %d", FFI_G(line));
			}
			break;
		case attr_aligned:
			if (n == 0
			 && (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_INT64 || val->kind == CLIB_FFI_VAL_UINT64)
			 && val->i64 > 0 && val->i64 <= 0x80000000 && (val->i64 & (val->i64 - 1)) == 0) {
				dcl->align = val->i64;
			} else {
				clib_ffi_parser_error("Incorrect \"alignment\" value at line %d", FFI_G(line));
			}
			break;
		case attr_mode:
			if (n == 0
			 && (val->kind == CLIB_FFI_VAL_NAME)) {
				const char *str = val->str;
				size_t len = val->len;
				if (len > 4
				 && str[0] == '_'
				 && str[1] == '_'
				 && str[len-2] == '_'
				 && str[len-1] == '_') {
					str += 2;
					len -= 4;
				}
				// TODO: Add support for vector type 'VnXX' ???
				if (len == 2) {
					if (str[1] == 'I') {
						if (dcl->flags & (CLIB_FFI_DCL_TYPE_SPECIFIERS-(CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_INT|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_LONG_LONG|CLIB_FFI_DCL_SIGNED|CLIB_FFI_DCL_UNSIGNED))) {
							/* inappropriate type */
						} else if (str[0] == 'Q') {
							dcl->flags &= ~(CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_INT|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_LONG_LONG);
							dcl->flags |= CLIB_FFI_DCL_CHAR;
							break;
						} else if (str[0] == 'H') {
							dcl->flags &= ~(CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_INT|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_LONG_LONG);
							dcl->flags |= CLIB_FFI_DCL_SHORT;
							break;
						} else if (str[0] == 'S') {
							dcl->flags &= ~(CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_INT|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_LONG_LONG);
							dcl->flags |= CLIB_FFI_DCL_INT;
							break;
						} else if (str[0] == 'D') {
							dcl->flags &= ~(CLIB_FFI_DCL_CHAR|CLIB_FFI_DCL_SHORT|CLIB_FFI_DCL_INT|CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_LONG_LONG);
							if (sizeof(long) == 8) {
								dcl->flags |= CLIB_FFI_DCL_LONG;
							} else {
								dcl->flags |= CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_LONG_LONG;
							}
							break;
						}
					} else if (str[1] == 'F') {
						if (dcl->flags & (CLIB_FFI_DCL_TYPE_SPECIFIERS-(CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_FLOAT|CLIB_FFI_DCL_DOUBLE))) {
							/* inappropriate type */
						} else if (str[0] == 'S') {
							dcl->flags &= ~(CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_FLOAT|CLIB_FFI_DCL_DOUBLE);
							dcl->flags |= CLIB_FFI_DCL_FLOAT;
							break;
						} else if (str[0] == 'D') {
							dcl->flags &= ~(CLIB_FFI_DCL_LONG|CLIB_FFI_DCL_FLOAT|CLIB_FFI_DCL_DOUBLE);
							dcl->flags |= CLIB_FFI_DCL_DOUBLE;
							break;
						}
					}
				}
			}
			clib_ffi_parser_error("Unsupported \"mode\" value at line %d", FFI_G(line));
			// TODO: ???
		case attr_unsupported:
			clib_ffi_parser_error("Unsupported attribute \"%.*s\" at line %d", name_len, name, FFI_G(line));
			break;
		default:
			/* ignore */
			break;
	}
}
/* }}} */

void clib_ffi_add_msvc_attribute_value(clib_ffi_dcl *dcl, const char *name, size_t name_len, clib_ffi_val *val) /* {{{ */
{
	if (name_len == sizeof("align")-1 && memcmp(name, "align", sizeof("align")-1) == 0) {
		if ((val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_INT64 || val->kind == CLIB_FFI_VAL_UINT64)
		 && val->i64 > 0 && val->i64 <= 0x80000000 && (val->i64 & (val->i64 - 1)) == 0) {
			dcl->align = val->i64;
		} else {
			clib_ffi_parser_error("Incorrect \"alignment\" value at line %d", FFI_G(line));
		}
	} else {
		/* ignore */
	}
}
/* }}} */

static clib_result clib_ffi_nested_type(clib_ffi_type *type, clib_ffi_type *nested_type) /* {{{ */
{
	nested_type = CLIB_FFI_TYPE(nested_type);
	switch (nested_type->kind) {
		case CLIB_FFI_TYPE_POINTER:
			/* "char" is used as a terminator of nested declaration */
			if (nested_type->pointer.type == &clib_ffi_type_char) {
				nested_type->pointer.type = type;
				return clib_ffi_validate_vla(CLIB_FFI_TYPE(type));
			} else {
				return clib_ffi_nested_type(type, nested_type->pointer.type);
			}
			break;
		case CLIB_FFI_TYPE_ARRAY:
			/* "char" is used as a terminator of nested declaration */
			if (nested_type->array.type == &clib_ffi_type_char) {
				nested_type->array.type = type;
				if (clib_ffi_validate_array_element_type(CLIB_FFI_TYPE(type)) == FAILURE) {
					return FAILURE;
				}
			} else {
				if (clib_ffi_nested_type(type, nested_type->array.type) != SUCCESS) {
					return FAILURE;
				}
			}
			nested_type->size = nested_type->array.length * CLIB_FFI_TYPE(nested_type->array.type)->size;
			nested_type->align = CLIB_FFI_TYPE(nested_type->array.type)->align;
			return SUCCESS;
			break;
		case CLIB_FFI_TYPE_FUNC:
			/* "char" is used as a terminator of nested declaration */
			if (nested_type->func.ret_type == &clib_ffi_type_char) {
				nested_type->func.ret_type = type;
				return clib_ffi_validate_func_ret_type(CLIB_FFI_TYPE(type));
			} else {
				return clib_ffi_nested_type(type, nested_type->func.ret_type);
			}
			break;
		default:
			CLIB_UNREACHABLE();
	}
}
/* }}} */

void clib_ffi_nested_declaration(clib_ffi_dcl *dcl, clib_ffi_dcl *nested_dcl) /* {{{ */
{
	/* "char" is used as a terminator of nested declaration */
	clib_ffi_finalize_type(dcl);
	if (!nested_dcl->type || nested_dcl->type == &clib_ffi_type_char) {
		nested_dcl->type = dcl->type;
	} else {
		if (clib_ffi_nested_type(dcl->type, nested_dcl->type) == FAILURE) {
			clib_ffi_cleanup_dcl(nested_dcl);
			LONGJMP(FFI_G(bailout), FAILURE);
		}
	}
	dcl->type = nested_dcl->type;
}
/* }}} */

void clib_ffi_align_as_type(clib_ffi_dcl *dcl, clib_ffi_dcl *align_dcl) /* {{{ */
{
	clib_ffi_finalize_type(align_dcl);
	dcl->align = MAX(align_dcl->align, CLIB_FFI_TYPE(align_dcl->type)->align);
}
/* }}} */

void clib_ffi_align_as_val(clib_ffi_dcl *dcl, clib_ffi_val *align_val) /* {{{ */
{
	switch (align_val->kind) {
		case CLIB_FFI_VAL_INT32:
		case CLIB_FFI_VAL_UINT32:
			dcl->align = clib_ffi_type_uint32.align;
			break;
		case CLIB_FFI_VAL_INT64:
		case CLIB_FFI_VAL_UINT64:
			dcl->align = clib_ffi_type_uint64.align;
			break;
		case CLIB_FFI_VAL_FLOAT:
			dcl->align = clib_ffi_type_float.align;
			break;
		case CLIB_FFI_VAL_DOUBLE:
			dcl->align = clib_ffi_type_double.align;
			break;
#ifdef HAVE_LONG_DOUBLE
		case CLIB_FFI_VAL_LONG_DOUBLE:
			dcl->align = clib_ffi_type_long_double.align;
			break;
#endif
		case CLIB_FFI_VAL_CHAR:
		case CLIB_FFI_VAL_STRING:
			dcl->align = clib_ffi_type_char.align;
			break;
		default:
			break;
	}
}
/* }}} */

#define clib_ffi_expr_bool(val) do { \
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) { \
		val->kind = CLIB_FFI_VAL_INT32; \
		val->i64 = !!val->u64; \
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) { \
		val->kind = CLIB_FFI_VAL_INT32; \
		val->i64 = !!val->i64; \
	} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
		val->kind = CLIB_FFI_VAL_INT32; \
		val->i64 = !!val->d; \
	} else if (val->kind == CLIB_FFI_VAL_CHAR) { \
		val->kind = CLIB_FFI_VAL_INT32; \
		val->i64 = !!val->ch; \
	} else { \
		val->kind = CLIB_FFI_VAL_ERROR; \
	} \
} while (0)

#define clib_ffi_expr_math(val, op2, OP) do { \
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = MAX(val->kind, op2->kind); \
			val->u64 = val->u64 OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32) { \
			val->u64 = val->u64 OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT64) { \
			val->u64 = val->u64 OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = op2->kind; \
			val->d = (clib_ffi_double)val->u64 OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->u64 = val->u64 OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32) { \
			val->i64 = val->i64 OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->i64 = val->i64 OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = MAX(val->kind, op2->kind); \
			val->i64 = val->i64 OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = op2->kind; \
			val->d = (clib_ffi_double)val->i64 OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->i64 = val->i64 OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->d = val->d OP (clib_ffi_double)op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 ||op2->kind == CLIB_FFI_VAL_INT64) { \
			val->d = val->d OP (clib_ffi_double)op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = MAX(val->kind, op2->kind); \
			val->d = val->d OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->d = val->d OP (clib_ffi_double)op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_CHAR) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = op2->kind; \
			val->u64 = val->ch OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = CLIB_FFI_VAL_INT64; \
			val->i64 = val->ch OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = op2->kind; \
			val->d = (clib_ffi_double)val->ch OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->ch = val->ch OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else { \
		val->kind = CLIB_FFI_VAL_ERROR; \
	} \
} while (0)

#define clib_ffi_expr_int_math(val, op2, OP) do { \
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = MAX(val->kind, op2->kind); \
			val->u64 = val->u64 OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32) { \
			val->u64 = val->u64 OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT64) { \
			val->u64 = val->u64 OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->u64 = val->u64 OP (uint64_t)op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->u64 = val->u64 OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32) { \
			val->i64 = val->i64 OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->i64 = val->i64 OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = MAX(val->kind, op2->kind); \
			val->i64 = val->i64 OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->u64 = val->u64 OP (int64_t)op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->i64 = val->i64 OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = op2->kind; \
			val->u64 = (uint64_t)val->d OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = op2->kind; \
			val->i64 = (int64_t)val->d OP op2->i64; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_CHAR) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = op2->kind; \
			val->u64 = (uint64_t)val->ch OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = op2->kind; \
			val->i64 = (int64_t)val->ch OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->ch = val->ch OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else { \
		val->kind = CLIB_FFI_VAL_ERROR; \
	} \
} while (0)

#define clib_ffi_expr_cmp(val, op2, OP) do { \
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->u64 OP op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->u64 OP op2->u64; /*signed/unsigned */ \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = (clib_ffi_double)val->u64 OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->u64 OP op2->d; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->i64 OP op2->i64; /* signed/unsigned */ \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->i64 OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = (clib_ffi_double)val->i64 OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->i64 OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->d OP (clib_ffi_double)op2->u64; \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 ||op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->d OP (clib_ffi_double)op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->d OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->d OP (clib_ffi_double)op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else if (val->kind == CLIB_FFI_VAL_CHAR) { \
		if (op2->kind == CLIB_FFI_VAL_UINT32 || op2->kind == CLIB_FFI_VAL_UINT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->ch OP op2->i64; /* signed/unsigned */ \
		} else if (op2->kind == CLIB_FFI_VAL_INT32 || op2->kind == CLIB_FFI_VAL_INT64) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->ch OP op2->i64; \
		} else if (op2->kind == CLIB_FFI_VAL_FLOAT || op2->kind == CLIB_FFI_VAL_DOUBLE || op2->kind == CLIB_FFI_VAL_LONG_DOUBLE) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = (clib_ffi_double)val->ch OP op2->d; \
		} else if (op2->kind == CLIB_FFI_VAL_CHAR) { \
			val->kind = CLIB_FFI_VAL_INT32; \
			val->i64 = val->ch OP op2->ch; \
		} else { \
			val->kind = CLIB_FFI_VAL_ERROR; \
		} \
	} else { \
		val->kind = CLIB_FFI_VAL_ERROR; \
	} \
} while (0)

void clib_ffi_expr_conditional(clib_ffi_val *val, clib_ffi_val *op2, clib_ffi_val *op3) /* {{{ */
{
	clib_ffi_expr_bool(val);
	if (val->kind == CLIB_FFI_VAL_INT32) {
		if (val->i64) {
			*val = *op2;
		} else {
			*val = *op3;
		}
	}
}
/* }}} */

void clib_ffi_expr_bool_or(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_bool(val);
	clib_ffi_expr_bool(op2);
	if (val->kind == CLIB_FFI_VAL_INT32 && op2->kind == CLIB_FFI_VAL_INT32) {
		val->i64 = val->i64 || op2->i64;
	} else {
		val->kind = CLIB_FFI_VAL_ERROR;
	}
}
/* }}} */

void clib_ffi_expr_bool_and(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_bool(val);
	clib_ffi_expr_bool(op2);
	if (val->kind == CLIB_FFI_VAL_INT32 && op2->kind == CLIB_FFI_VAL_INT32) {
		val->i64 = val->i64 && op2->i64;
	} else {
		val->kind = CLIB_FFI_VAL_ERROR;
	}
}
/* }}} */

void clib_ffi_expr_bw_or(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_int_math(val, op2, |);
}
/* }}} */

void clib_ffi_expr_bw_xor(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_int_math(val, op2, ^);
}
/* }}} */

void clib_ffi_expr_bw_and(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_int_math(val, op2, &);
}
/* }}} */

void clib_ffi_expr_is_equal(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_cmp(val, op2, ==);
}
/* }}} */

void clib_ffi_expr_is_not_equal(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_cmp(val, op2, !=);
}
/* }}} */

void clib_ffi_expr_is_less(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_cmp(val, op2, <);
}
/* }}} */

void clib_ffi_expr_is_greater(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_cmp(val, op2, >);
}
/* }}} */

void clib_ffi_expr_is_less_or_equal(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_cmp(val, op2, <=);
}
/* }}} */

void clib_ffi_expr_is_greater_or_equal(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_cmp(val, op2, >=);
}
/* }}} */

void clib_ffi_expr_shift_left(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_int_math(val, op2, <<);
}
/* }}} */

void clib_ffi_expr_shift_right(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_int_math(val, op2, >>);
}
/* }}} */

void clib_ffi_expr_add(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_math(val, op2, +);
}
/* }}} */

void clib_ffi_expr_sub(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_math(val, op2, -);
}
/* }}} */

void clib_ffi_expr_mul(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_math(val, op2, *);
}
/* }}} */

void clib_ffi_expr_div(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_math(val, op2, /);
}
/* }}} */

void clib_ffi_expr_mod(clib_ffi_val *val, clib_ffi_val *op2) /* {{{ */
{
	clib_ffi_expr_int_math(val, op2, %); // ???
}
/* }}} */

void clib_ffi_expr_cast(clib_ffi_val *val, clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_finalize_type(dcl);
	switch (CLIB_FFI_TYPE(dcl->type)->kind) {
		case CLIB_FFI_TYPE_FLOAT:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
				val->kind = CLIB_FFI_VAL_FLOAT;
				val->d = val->u64;
			} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_FLOAT;
				val->d = val->i64;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_FLOAT;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
				val->kind = CLIB_FFI_VAL_FLOAT;
				val->d = val->ch;
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
		case CLIB_FFI_TYPE_DOUBLE:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
				val->kind = CLIB_FFI_VAL_DOUBLE;
				val->d = val->u64;
			} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_DOUBLE;
				val->d = val->i64;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_DOUBLE;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
				val->kind = CLIB_FFI_VAL_DOUBLE;
				val->d = val->ch;
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
#ifdef HAVE_LONG_DOUBLE
		case CLIB_FFI_TYPE_LONGDOUBLE:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
				val->kind = CLIB_FFI_VAL_LONG_DOUBLE;
				val->d = val->u64;
			} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_LONG_DOUBLE;
				val->d = val->i64;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_LONG_DOUBLE;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
				val->kind = CLIB_FFI_VAL_LONG_DOUBLE;
				val->d = val->ch;
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
#endif
		case CLIB_FFI_TYPE_UINT8:
		case CLIB_FFI_TYPE_UINT16:
		case CLIB_FFI_TYPE_UINT32:
		case CLIB_FFI_TYPE_BOOL:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64 || val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_UINT32;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_UINT32;
				val->u64 = val->d;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
				val->kind = CLIB_FFI_VAL_UINT32;
				val->u64 = val->ch;
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
		case CLIB_FFI_TYPE_SINT8:
		case CLIB_FFI_TYPE_SINT16:
		case CLIB_FFI_TYPE_SINT32:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64 || val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_INT32;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_INT32;
				val->i64 = val->d;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
				val->kind = CLIB_FFI_VAL_INT32;
				val->i64 = val->ch;
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
		case CLIB_FFI_TYPE_UINT64:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64 || val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_UINT64;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_UINT64;
				val->u64 = val->d;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
				val->kind = CLIB_FFI_VAL_UINT64;
				val->u64 = val->ch;
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
		case CLIB_FFI_TYPE_SINT64:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
				val->kind = CLIB_FFI_VAL_CHAR;
				val->ch = val->u64;
			} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_CHAR;
				val->ch = val->i64;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_CHAR;
				val->ch = val->d;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
		case CLIB_FFI_TYPE_CHAR:
			if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64 || val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
				val->kind = CLIB_FFI_VAL_UINT32;
			} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
				val->kind = CLIB_FFI_VAL_UINT32;
				val->u64 = val->d;
			} else if (val->kind == CLIB_FFI_VAL_CHAR) {
				val->kind = CLIB_FFI_VAL_UINT32;
				val->u64 = val->ch;
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
			break;
		default:
			val->kind = CLIB_FFI_VAL_ERROR;
			break;
	}
	clib_ffi_type_dtor(dcl->type);
}
/* }}} */

void clib_ffi_expr_plus(clib_ffi_val *val) /* {{{ */
{
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
	} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
	} else if (val->kind == CLIB_FFI_VAL_CHAR) {
	} else {
		val->kind = CLIB_FFI_VAL_ERROR;
	}
}
/* }}} */

void clib_ffi_expr_neg(clib_ffi_val *val) /* {{{ */
{
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
		val->u64 = -val->u64;
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
		val->i64 = -val->i64;
	} else if (val->kind == CLIB_FFI_VAL_FLOAT || val->kind == CLIB_FFI_VAL_DOUBLE || val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
		val->d = -val->d;
	} else if (val->kind == CLIB_FFI_VAL_CHAR) {
		val->ch = -val->ch;
	} else {
		val->kind = CLIB_FFI_VAL_ERROR;
	}
}
/* }}} */

void clib_ffi_expr_bw_not(clib_ffi_val *val) /* {{{ */
{
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_UINT64) {
		val->u64 = ~val->u64;
	} else if (val->kind == CLIB_FFI_VAL_INT32 || val->kind == CLIB_FFI_VAL_INT64) {
		val->i64 = ~val->i64;
	} else if (val->kind == CLIB_FFI_VAL_CHAR) {
		val->ch = ~val->ch;
	} else {
		val->kind = CLIB_FFI_VAL_ERROR;
	}
}
/* }}} */

void clib_ffi_expr_bool_not(clib_ffi_val *val) /* {{{ */
{
	clib_ffi_expr_bool(val);
	if (val->kind == CLIB_FFI_VAL_INT32) {
		val->i64 = !val->i64;
	}
}
/* }}} */

void clib_ffi_expr_sizeof_val(clib_ffi_val *val) /* {{{ */
{
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_INT32) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_uint32.size;
	} else if (val->kind == CLIB_FFI_VAL_UINT64 || val->kind == CLIB_FFI_VAL_INT64) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_uint64.size;
	} else if (val->kind == CLIB_FFI_VAL_FLOAT) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_float.size;
	} else if (val->kind == CLIB_FFI_VAL_DOUBLE) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_double.size;
	} else if (val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
		val->kind = CLIB_FFI_VAL_UINT32;
#ifdef _WIN32
		val->u64 = clib_ffi_type_double.size;
#else
		val->u64 = clib_ffi_type_long_double.size;
#endif
	} else if (val->kind == CLIB_FFI_VAL_CHAR) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_char.size;
	} else if (val->kind == CLIB_FFI_VAL_STRING) {
		if (memchr(val->str, '\\', val->len)) {
			// TODO: support for escape sequences ???
			val->kind = CLIB_FFI_VAL_ERROR;
		} else {
			val->kind = CLIB_FFI_VAL_UINT32;
			val->u64 = val->len + 1;
		}
	} else {
		val->kind = CLIB_FFI_VAL_ERROR;
	}
}
/* }}} */

void clib_ffi_expr_sizeof_type(clib_ffi_val *val, clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_type *type;

	clib_ffi_finalize_type(dcl);
	type = CLIB_FFI_TYPE(dcl->type);
	val->kind = (type->size > 0xffffffff) ? CLIB_FFI_VAL_UINT64 : CLIB_FFI_VAL_UINT32;
	val->u64 = type->size;
	clib_ffi_type_dtor(dcl->type);
}
/* }}} */

void clib_ffi_expr_alignof_val(clib_ffi_val *val) /* {{{ */
{
	if (val->kind == CLIB_FFI_VAL_UINT32 || val->kind == CLIB_FFI_VAL_INT32) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_uint32.align;
	} else if (val->kind == CLIB_FFI_VAL_UINT64 || val->kind == CLIB_FFI_VAL_INT64) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_uint64.align;
	} else if (val->kind == CLIB_FFI_VAL_FLOAT) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_float.align;
	} else if (val->kind == CLIB_FFI_VAL_DOUBLE) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_double.align;
#ifdef HAVE_LONG_DOUBLE
	} else if (val->kind == CLIB_FFI_VAL_LONG_DOUBLE) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_long_double.align;
#endif
	} else if (val->kind == CLIB_FFI_VAL_CHAR) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = clib_ffi_type_char.size;
	} else if (val->kind == CLIB_FFI_VAL_STRING) {
		val->kind = CLIB_FFI_VAL_UINT32;
		val->u64 = _Alignof(char*);
	} else {
		val->kind = CLIB_FFI_VAL_ERROR;
	}
}
/* }}} */

void clib_ffi_expr_alignof_type(clib_ffi_val *val, clib_ffi_dcl *dcl) /* {{{ */
{
	clib_ffi_finalize_type(dcl);
	val->kind = CLIB_FFI_VAL_UINT32;
	val->u64 = CLIB_FFI_TYPE(dcl->type)->align;
	clib_ffi_type_dtor(dcl->type);
}
/* }}} */

void clib_ffi_val_number(clib_ffi_val *val, int base, const char *str, size_t str_len) /* {{{ */
{
	int u = 0;
	int l = 0;

	if (str[str_len-1] == 'u' || str[str_len-1] == 'U') {
		u = 1;
		if (str[str_len-2] == 'l' || str[str_len-2] == 'L') {
			l = 1;
			if (str[str_len-3] == 'l' || str[str_len-3] == 'L') {
				l = 2;
			}
		}
	} else if (str[str_len-1] == 'l' || str[str_len-1] == 'L') {
		l = 1;
		if (str[str_len-2] == 'l' || str[str_len-2] == 'L') {
			l = 2;
			if (str[str_len-3] == 'u' || str[str_len-3] == 'U') {
				u = 1;
			}
		} else if (str[str_len-2] == 'u' || str[str_len-2] == 'U') {
			u = 1;
		}
	}
	if (u) {
		val->u64 = strtoull(str, NULL, base);
		if (l == 0) {
			val->kind = CLIB_FFI_VAL_UINT32;
		} else if (l == 1) {
			val->kind = (sizeof(long) == 4) ? CLIB_FFI_VAL_UINT32 : CLIB_FFI_VAL_UINT64;
		} else if (l == 2) {
			val->kind = CLIB_FFI_VAL_UINT64;
		}
	} else {
		val->i64 = strtoll(str, NULL, base);
		if (l == 0) {
			val->kind = CLIB_FFI_VAL_INT32;
		} else if (l == 1) {
			val->kind = (sizeof(long) == 4) ? CLIB_FFI_VAL_INT32 : CLIB_FFI_VAL_INT64;
		} else if (l == 2) {
			val->kind = CLIB_FFI_VAL_INT64;
		}
	}
}
/* }}} */

void clib_ffi_val_float_number(clib_ffi_val *val, const char *str, size_t str_len) /* {{{ */
{
	val->d = strtold(str, NULL);
	if (str[str_len-1] == 'f' || str[str_len-1] == 'F') {
		val->kind = CLIB_FFI_VAL_FLOAT;
	} else if (str[str_len-1] == 'l' || str[str_len-1] == 'L') {
		val->kind = CLIB_FFI_VAL_LONG_DOUBLE;
	} else {
		val->kind = CLIB_FFI_VAL_DOUBLE;
	}
}
/* }}} */

void clib_ffi_val_string(clib_ffi_val *val, const char *str, size_t str_len) /* {{{ */
{
	if (str[0] != '\"') {
		val->kind = CLIB_FFI_VAL_ERROR;
	} else {
		val->kind = CLIB_FFI_VAL_STRING;
		val->str = str + 1;
		val->len = str_len - 2;
	}
}
/* }}} */

void clib_ffi_val_character(clib_ffi_val *val, const char *str, size_t str_len) /* {{{ */
{
	int n;

	if (str[0] != '\'') {
		val->kind = CLIB_FFI_VAL_ERROR;
	} else {
		val->kind = CLIB_FFI_VAL_CHAR;
		if (str_len == 3) {
			val->ch = str[1];
		} else if (str[1] == '\\') {
			if (str[2] == 'a') {
			} else if (str[2] == 'b' && str_len == 4) {
				val->ch = '\b';
			} else if (str[2] == 'f' && str_len == 4) {
				val->ch = '\f';
			} else if (str[2] == 'n' && str_len == 4) {
				val->ch = '\n';
			} else if (str[2] == 'r' && str_len == 4) {
				val->ch = '\r';
			} else if (str[2] == 't' && str_len == 4) {
				val->ch = '\t';
			} else if (str[2] == 'v' && str_len == 4) {
				val->ch = '\v';
			} else if (str[2] >= '0' && str[2] <= '7') {
				n = str[2] - '0';
				if (str[3] >= '0' && str[3] <= '7') {
					n = n * 8 + (str[3] - '0');
					if ((str[4] >= '0' && str[4] <= '7') && str_len == 6) {
						n = n * 8 + (str[4] - '0');
					} else if (str_len != 5) {
						val->kind = CLIB_FFI_VAL_ERROR;
					}
				} else if (str_len != 4) {
					val->kind = CLIB_FFI_VAL_ERROR;
				}
				if (n <= 0xff) {
					val->ch = n;
				} else {
					val->kind = CLIB_FFI_VAL_ERROR;
				}
			} else if (str[2] == 'x') {
				if (str[3] >= '0' && str[3] <= '9') {
					n = str[3] - '0';
				} else if (str[3] >= 'A' && str[3] <= 'F') {
					n = str[3] - 'A';
				} else if (str[3] >= 'a' && str[3] <= 'f') {
					n = str[3] - 'a';
				} else {
					val->kind = CLIB_FFI_VAL_ERROR;
					return;
				}
				if ((str[4] >= '0' && str[4] <= '9') && str_len == 6) {
					n = n * 16 + (str[4] - '0');
				} else if ((str[4] >= 'A' && str[4] <= 'F') && str_len == 6) {
					n = n * 16 + (str[4] - 'A');
				} else if ((str[4] >= 'a' && str[4] <= 'f') && str_len == 6) {
					n = n * 16 + (str[4] - 'a');
				} else if (str_len != 5) {
					val->kind = CLIB_FFI_VAL_ERROR;
					return;
				}
				val->ch = n;
			} else if (str_len == 4) {
				val->ch = str[2];
			} else {
				val->kind = CLIB_FFI_VAL_ERROR;
			}
		} else {
			val->kind = CLIB_FFI_VAL_ERROR;
		}
	}
}
/* }}} */
