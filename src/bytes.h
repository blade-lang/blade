#ifndef BLADE_BYTES_H
#define BLADE_BYTES_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_BYTES_METHOD(name) DECLARE_METHOD(bytes##name)

/**
 * bytes(value: number|list)
 *
 * creates a new array of bytes
 * - if a number is given, creates an array of size number
 * - if a list is given, converts the bytes list into an array of bytes
 */
DECLARE_NATIVE(bytes);

/**
 * bytes.length()
 *
 * returns the length of a bytes
 */
DECLARE_BYTES_METHOD(length);

/**
 * bytes.append(item: any)
 *
 * adds an item to the top of a bytes
 * @return nil
 */
DECLARE_BYTES_METHOD(append);

/**
 * bytes.clone()
 *
 * returns a deep clone of the bytes
 */
DECLARE_BYTES_METHOD(clone);

/**
 * bytes.extend(item: bytes2)
 *
 * adds the content of bytes2 into bytes1
 * @return nil
 */
DECLARE_BYTES_METHOD(extend);

/**
 * bytes.index_of(byte: number)
 *
 * returns the position of the first occurrence of byte in the bytes
 */
DECLARE_BYTES_METHOD(index_of);

/**
 * bytes.pop()
 *
 * removes the last item in a bytes and returns it
 */
DECLARE_BYTES_METHOD(pop);

/**
 * bytes.remove(index: number)
 *
 * removes the item at the specified index in the bytes
 */
DECLARE_BYTES_METHOD(remove);

/**
 * bytes.reverse()
 *
 * reverses the items in a bytes
 */
DECLARE_BYTES_METHOD(reverse);

/**
 * bytes.first()
 *
 * returns the first item in a bytes or nil if the bytes is empty
 */
DECLARE_BYTES_METHOD(first);

/**
 * bytes.last()
 *
 * returns the last item in a bytes or nil if the bytes is empty
 */
DECLARE_BYTES_METHOD(last);

/**
 * bytes.get(index: number)
 *
 * returns the value at index in the bytes
 */
DECLARE_BYTES_METHOD(get);

/**
 * bytes.split(delimiter: bytes)
 *
 * splits the content of a bytes based on the specified delimiter
 *
 * Notice
 * =======
 * bytes(0).split(bytes(0)) == []
 * # Result = true
 * echo 'test'.to_bytes().split(bytes(0))
 * # Result = [(74), (65), (73), (74)]
 */
DECLARE_BYTES_METHOD(split);

/**
 * bytes.is_alpha()
 *
 * returns true if the bytes contains only alphabets
 */
DECLARE_BYTES_METHOD(is_alpha);

/**
 * bytes.is_alnum()
 *
 * returns true if the bytes contains only number and alphabets
 */
DECLARE_BYTES_METHOD(is_alnum);

/**
 * bytes.is_number()
 *
 * returns true if the bytes contains only number
 */
DECLARE_BYTES_METHOD(is_number);

/**
 * bytes.is_lower()
 *
 * returns true if all the text in the bytes are lower case.
 */
DECLARE_BYTES_METHOD(is_lower);

/**
 * bytes.is_upper()
 *
 * returns true if all the text in the bytes are capital case.
 */
DECLARE_BYTES_METHOD(is_upper);

/**
 * bytes.is_space()
 *
 * returns true is the bytes contains only white space
 */
DECLARE_BYTES_METHOD(is_space);

/**
 * bytes.dispose()
 *
 * Due to the nature of bytes and their use-case (especially streaming data),
 * it is easy for the system memory to get filled up with data in the bytes.
 * The method allows users to reset a byte stream and empty it.
 *
 * This allows manual memory management of bytes.
 */
DECLARE_BYTES_METHOD(dispose);

/**
 * bytes.to_list()
 *
 * returns a list of every character in the bytes
 */
DECLARE_BYTES_METHOD(to_list);

/**
 * bytes.to_string()
 *
 * returns a string representation of the bytes
 */
DECLARE_BYTES_METHOD(to_string);

/**
 * See list.h
 */
DECLARE_BYTES_METHOD(__iter__);
DECLARE_BYTES_METHOD(__itern__);
DECLARE_BYTES_METHOD(each);

#endif