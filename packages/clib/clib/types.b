import _clib
import reflect

/**
 * C void type
 * @type ptr
 */
var void = _clib.void

/**
 * C bool type
 * @type ptr
 */
var bool = _clib.bool


/**
 * C uint8_t type
 * @type ptr
 */
var uint8_t = _clib.uint8

/**
 * C int8_t type
 * @type ptr
 */
var int8_t = _clib.sint8

/**
 * C byte type
 * @type ptr
 */
var byte = int8_t

/**
 * C ubyte type
 * @type ptr
 */
var ubyte = uint8_t


/**
 * C uint16_t type
 * @type ptr
 */
var uint16_t = _clib.uint16

/**
 * C int16_t type
 * @type ptr
 */
var int16_t = _clib.sint16


/**
 * C uint32_t type
 * @type ptr
 */
var uint32_t = _clib.uint32

/**
 * C int32_t type
 * @type ptr
 */
var int32_t = _clib.sint32


/**
 * C uint64_t type
 * @type ptr
 */
var uint64_t = _clib.uint64

/**
 * C int64_t type
 * @type ptr
 */
var int64_t = _clib.sint64

/**
 * C ssize_t type
 * @type ptr
 */
var ssize_t = int64_t


/**
 * C float type
 * @type ptr
 */
var float = _clib.float


/**
 * C double type
 * @type ptr
 */
var double = _clib.double


/**
 * C uchar type
 * @type ptr
 */
var uchar = _clib.uchar

/**
 * C char type
 * @type ptr
 */
var char = _clib.schar


/**
 * C ushort type
 * @type ptr
 */
var ushort = _clib.ushort

/**
 * C short type
 * @type ptr
 */
var short = _clib.sshort


/**
 * C uint type
 * @type ptr
 */
var uint = _clib.uint

/**
 * C int type
 * @type ptr
 */
var int = _clib.sint


/**
 * C ulong type
 * @type ptr
 */
var ulong = _clib.ulong

/**
 * C long type
 * @type ptr
 */
var long = _clib.slong

/**
 * C size_t type
 * @type ptr
 */
var size_t = ulong


/**
 * C long_double type
 * @type ptr
 */
var long_double = _clib.longdouble


/**
 * C char_ptr type
 * @type ptr
 */
var char_ptr = _clib.char_ptr

/**
 * C uchar_ptr type
 * @type ptr
 */
var uchar_ptr = _clib.uchar_ptr


/**
 * C ptr type
 * @type ptr
 */
var ptr = _clib.pointer


/*
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
*/
