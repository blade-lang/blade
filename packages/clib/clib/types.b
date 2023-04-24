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


/**
 * struct(...type)
 * 
 * Returns a type that can be used to declare structs. 
 * To create or read value for the struct, you need to use the `pack()` 
 * and `unpack()` function in the `struct` module respectively.
 * 
 * @note This function can also be used to define a C union or array.
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
 * new(type: type, ...any)
 * 
 * Creates a new C value for the specified clib type with the given values.
 * @returns bytes
 */
def new(type, ...) {
  if __args__.length() == 0
    die Exception('canot have an empty struct')

  # Ensure a valid and non void clib pointer.
  if !(reflect.is_ptr(type) and to_string(type).match('/clib/')) and type != void
    die Exception('invalid type for new')

  return _clib.new(type, __args__)
}

/**
 * get(type: type, data: string | bytes)
 * 
 * Returns the data contained in a C type _type_ encoded in the data.
 * The data should either be an output of `clib.new()` or a call to a 
 * function returning one of struct, union or array.
 * @returns any
 */
def get(type, data) {
  # Ensure a valid and non void clib pointer.
  if !(reflect.is_ptr(type) and to_string(type).match('/clib/')) and type != void
    die Exception('invalid type for new')
  if is_string(data) data = data.to_bytes()

  return _clib.get(type, data)
}
