#ifndef BIRD_B_STRING_H
#define BIRD_B_STRING_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_STRING_METHOD(name) DECLARE_METHOD(string##name)

/**
 * string.length()
 *
 * returns the length of a string
 */
DECLARE_STRING_METHOD(length);

/**
 * string.upper()
 *
 * returns a copy of the string in capital case
 */
DECLARE_STRING_METHOD(upper);

/**
 * string.lower()
 *
 * returns a copy of the string in lower case
 */
DECLARE_STRING_METHOD(lower);

/**
 * string.is_alpha()
 *
 * returns true if the string contains only alphabets
 */
DECLARE_STRING_METHOD(is_alpha);

/**
 * string.is_alnum()
 *
 * returns true if the string contains only number and alphabets
 */
DECLARE_STRING_METHOD(is_alnum);

/**
 * string.is_number()
 *
 * returns true if the string contains only number
 */
DECLARE_STRING_METHOD(is_number);

/**
 * string.is_lower()
 *
 * returns true if all the text in the string are lower case.
 */
DECLARE_STRING_METHOD(is_lower);

/**
 * string.is_upper()
 *
 * returns true if all the text in the string are capital case.
 */
DECLARE_STRING_METHOD(is_upper);

/**
 * string.is_space()
 *
 * returns true is the string contains only white space
 */
DECLARE_STRING_METHOD(is_space);

/**
 * string.trim([trim character: string])
 *
 * removes all existence whitespace (or
 * the trim character) around a string.
 */
DECLARE_STRING_METHOD(trim);

/**
 * string.ltrim([trim character: string])
 *
 * removes all existence whitespace (or
 * the trim character) from the beginning of a string.
 */
DECLARE_STRING_METHOD(ltrim);

/**
 * string.rtrim([trim character: string])
 *
 * removes all existence whitespace (or
 * the trim character) from the end of a string.
 */
DECLARE_STRING_METHOD(rtrim);

/**
 * string.join(item: string|list|dict)
 *
 * concatenates the items in a list using the value string
 */
DECLARE_STRING_METHOD(join);

/**
 * string.split(delimiter: string)
 *
 * splits the content of a string based on the specified delimiter
 *
 * Notice
 * =======
 * ''.split('') == []
 * 'test'.split('') == ['t', 'e', 's', 't']
 */
DECLARE_STRING_METHOD(split);

/**
 * string.index_of(str: string)
 *
 * returns the position of the first occurrence of str in the string
 */
DECLARE_STRING_METHOD(index_of);

/**
 * string.starts_with(str: string)
 *
 * returns true if the string starts with str or false otherwise
 */
DECLARE_STRING_METHOD(starts_with);

/**
 * string.ends_with(str: string)
 *
 * returns true if the string ends with str or false otherwise
 */
DECLARE_STRING_METHOD(ends_with);

/**
 * string.count(str: string)
 *
 * returns the number of occurrence of a str in string
 */
DECLARE_STRING_METHOD(count);

/**
 * string.to_number()
 *
 * returns the number value of a string
 */
DECLARE_STRING_METHOD(to_number);

/**
 * string.to_list()
 *
 * returns a list of every character in a string
 */
DECLARE_STRING_METHOD(to_list);

/**
 * string.lpad(width: number [, fill_character: char])
 *
 * returns the string left justified in a string of length width.
 * padding is done using the specified fill_character (default is a space).
 * The original string is returned if width is less than string.length
 */
DECLARE_STRING_METHOD(lpad);

/**
 * string.rpad(width: number [, fill_character: char])
 *
 * returns the string right justified in a string of length width.
 * padding is done using the specified fill_character (default is a space).
 * The original string is returned if width is less than string.length
 */
DECLARE_STRING_METHOD(rpad);

/**
 * string.match(str: string|regex)
 *
 * if str is not regex:
 *  - returns true if string contains str
 *  - returns false otherwise
 * else:
 *  - returns boolean false if there is no match
 *  - returns the first matching data in a list
 */
DECLARE_STRING_METHOD(match);

/**
 * string.matches(str: regex)
 *
 * returns a list of matches of str in string
 * NOTE: str must be a regular expression
 */
DECLARE_STRING_METHOD(matches);

/**
 * string.replace(str: regex|string, replacement: string)
 *
 * replaces a substring matched by the regex|string str with replacement string
 *
 * in the replacement string,
 * - you can reference capture groups using the $(index) notation.
 * e.g.
 *
 * 'name 123'.replace('/(\d+)/', '$0 name') -> 'name 123 name'
 */
DECLARE_STRING_METHOD(replace);

/**
 * string.to_bytes()
 *
 * returns the bytes making up the string
 */
DECLARE_STRING_METHOD(to_bytes);

/**
 * string.__iter__()
 *
 * implementing the iterable interface
 */
DECLARE_STRING_METHOD(__iter__);

/**
 * string.__itern__()
 *
 * implementing the iterable interface
 */
DECLARE_STRING_METHOD(__itern__);

#endif