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
 * bytes.is_alpha()
 *
 * returns true if the string contains only alphabets
 */
DECLARE_BYTES_METHOD(is_alpha);

/**
 * bytes.is_alnum()
 *
 * returns true if the string contains only number and alphabets
 */
DECLARE_BYTES_METHOD(is_alnum);

/**
 * bytes.is_number()
 *
 * returns true if the string contains only number
 */
DECLARE_BYTES_METHOD(is_number);

/**
 * bytes.is_lower()
 *
 * returns true if all the text in the string are lower case.
 */
DECLARE_BYTES_METHOD(is_lower);

/**
 * bytes.is_upper()
 *
 * returns true if all the text in the string are capital case.
 */
DECLARE_BYTES_METHOD(is_upper);

/**
 * bytes.is_space()
 *
 * returns true is the string contains only white space
 */
DECLARE_BYTES_METHOD(is_space);

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
 * bytes.@iter()
 *
 * implementing the iterable interface
 */
DECLARE_BYTES_METHOD(__iter__);

/**
 * bytes.@itern()
 *
 * implementing the iterable interface
 */
DECLARE_BYTES_METHOD(__itern__);

#endif