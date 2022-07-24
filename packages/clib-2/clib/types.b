import _clib
import reflect

var void = _clib.void

var bool = _clib.bool

var uint8_t = _clib.uint8
var int8_t = _clib.sint8
var byte = int8_t
var ubyte = uint8_t

var uint16_t = _clib.uint16
var int16_t = _clib.sint16

var uint32_t = _clib.uint32
var int32_t = _clib.sint32

var uint64_t = _clib.uint64
var int64_t = _clib.sint64
var ssize_t = int64_t

var float = _clib.float

var double = _clib.double

var uchar = _clib.uchar
var char = _clib.schar

var ushort = _clib.ushort
var short = _clib.sshort

var uint = _clib.uint
var int = _clib.sint

var ulong = _clib.ulong
var long = _clib.slong
var size_t = ulong

var long_double = _clib.longdouble

var char_ptr = _clib.char_ptr
var uchar_ptr = _clib.uchar_ptr

var ptr = _clib.pointer

/**
 * struct(...type)
 * 
 * Returns a type that can be used to declare structs.
 * @return type
 */
def struct(...) {
  if __args__.length() == 0
    die Exception('canot have an empty struct')

  for arg in __args__ {
    # Ensure a valid clib pointer.
    if !(reflect.is_ptr(arg) and to_string(arg).match('/clib/'))
      die Exception('invalid type in struct delaration')
  }

  return _clib.new_struct(__args__)
}

/**
 * create_struct(type: type, ...values: type)
 * 
 * Creates a new struct instance based on the given previously 
 * declared struct type and sets it's value based on the given 
 * values. When no value is set, a new instance of the struct is 
 * created without any initialized fields.
 * 
 * For example,
 * 
 * ```blade
 * var my_struct = create_struct(mytype.tm)
 * ```
 * 
 * The following call to `create_struct()` translates into C code.
 * 
 * ```c
 * struct tm my_struct;
 * ```
 * 
 * While this the following,
 * 
 * ```blade
 * var my_struct = create_struct(mytype.tm, 0, 1.5)
 * ```
 * 
 * translates into the following C code.
 * 
 * ```c
 * struct tm my_struct = {
 *    .sec = 0,
 *    .usec = 1.5
 * };
 * ```
 * @return ptr
 */
def create_struct(type, ...) {
  if !(reflect.is_ptr(type) and to_string(type).match('/clib/'))
    die Exception('canot have an empty struct')

  return _clib.create_struct(type, __args__)
}

var array = struct
