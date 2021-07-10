/**
 * CLib
 *
 * Provides interface for interacting with the C libraries
 * @copyright 2021, Ore Richard Muyiwa
 */

/**
 * CTypes
 * enumeration of C standard/basic data types and their pointers
 */
class CType {
  # basic types
  static var Void           = 0
  static var Bool           = 1
  static var Char           = 2
  static var UChar          = 3
  static var Short          = 4
  static var UShort         = 5
  static var Int            = 6
  static var UInt           = 7
  static var UInt8          = 8
  static var UInt16         = 9
  static var UInt32         = 10
  static var UInt64         = 11
  static var Long           = 12
  static var LongLong       = 13
  static var ULong          = 14
  static var ULongLong      = 15
  static var Float          = 16
  static var Double         = 17
  static var LongDouble     = 18

  # basic type pointers
  static var VoidPtr        = 19
  static var BoolPtr        = 20
  static var CharPtr        = 21
  static var UCharPtr       = 22
  static var ShortPtr       = 23
  static var UShortPtr      = 24
  static var IntPtr         = 25
  static var UIntPtr        = 26
  static var UInt8Ptr       = 27
  static var UInt16Ptr      = 28
  static var UInt32Ptr      = 29
  static var UInt64Ptr      = 30
  static var LongPtr        = 31
  static var LongLongPtr    = 32
  static var ULongPtr       = 33
  static var ULongLongPtr   = 34
  static var FloatPtr       = 35
  static var DoublePtr      = 36
  static var LongDoublePtr  = 37

  # derived pointers
  static var UnionPtr       = 38
  static var StructPtr      = 39
}

/**
 * CDeclaration
 * representation class for a C variable declaration (non-assignment) 
 */
class CDeclaration {
  CDeclaration(name, type) {
    if !is_string(name) or !is_number(type) or 
        type < CType.Void or type > CType.StructPtr {
      die Exception('expects valid C name (string) and type (CType)')
    }

    self.name = name
    self.type = type
  }
}

/**
 * CValue
 * representation class for static typed C value
 */
class CValue {
  CValue(type, value) {
    if !is_number(type) or type < CType.Void or type > CType.StructPtr
      die Exception('expected CType as first parameter')

    self.type = type
    self.value = value
  }
}


/**
 * CUnion
 * representation class for a C union
 */
class CUnion {
  CUnion(...) {
    for x in __args__ {
      if !is_instance(x, CDeclaration)
        die Exception('expected 0 or more CDeclaration')
    }

    self.union = __args__
  }
}

/**
 * CStruct
 * representation class for a C struct object
 */
class CStruct {
  CStruct(...) {
    for x in __args__ {
      if !is_instance(x, CDeclaration)
        die Exception('expected 0 or more CDeclaration')
    }

    self.struct = __args__
  }
}

/**
 * Main library class
 * this class is responsible for interacting with C shared libraries
 * 
 * it provides interfaces for loading libraries, calling functions, 
 * returning value, declarating types between C and Bird
 */
class CLib {
  /**
   * CLib(library_file)
   *
   * this constructor accepts a file object pointing
   * to the shared C library.
   */
  CLib(library_file) {
    self.library_file = library_file
  }

  /**
   * loads the library to memory and prepares it for
   * calling.
   */
  open() {
    self._open(self.library_file)
  }

  call_int(name, ...) {
    if !is_string(name) 
      die Exception('method name must be string')
    if __args__.length() > 0 {
      for x in __args__ {
        if !is_instance(x, CValue) 
          die Exception('call parameters must be CValue instance(s)')
      }
    }
  }
}