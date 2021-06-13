#ifndef BIRD_DICTIONARY_H
#define BIRD_DICTIONARY_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_DICT_METHOD(name) DECLARE_METHOD(dict##name)

/**
 * dictionary.length()
 *
 * returns the length of a dictionary
 */
DECLARE_DICT_METHOD(length);

/**
 * dict.add(key: any, value: any)
 *
 * adds the key-value pair to the dictionary
 */
DECLARE_DICT_METHOD(add);

/**
 * dict.set(key: any, value: any)
 *
 * set the value of the named key to the give value in 
 * the dictionary. 
 * creates a new entry if the key cannot be found.
 */
DECLARE_DICT_METHOD(set);

/**
 * dict.clear()
 *
 * clears the contents of the dictionary
 */
DECLARE_DICT_METHOD(clear);

/**
 * dict.clone()
 *
 * returns a deep copy of the current dictionary
 */
DECLARE_DICT_METHOD(clone);

/**
 * dict.compact()
 *
 * removes all nil entries from a the dictionary
 */
DECLARE_DICT_METHOD(compact);

/**
 * dict.contains(key: any)
 *
 * returns true if the dictionary contains the key or false otherwise
 */
DECLARE_DICT_METHOD(contains);

/**
 * dict.extend(item: dict)
 *
 * adds all key-value pair in item into the dict
 */
DECLARE_DICT_METHOD(extend);

/**
 * dict.get(key: any [, default: any])
 *
 * returns the value of the key or the default if the key is not present
 *
 * this is one thing that separates dict[invalid_key] from
 * dict.get(invalid_key) when using get(), if the default options
 * is not set, invalid keys throw errors instead of returning nil.
 *
 * so if you want to that dict MUST contain the key, use get()
 */
DECLARE_DICT_METHOD(get);

/**
 * dict.keys()
 *
 * returns a list of keys in the dictionary
 */
DECLARE_DICT_METHOD(keys);

/**
 * dict.values()
 *
 * returns a list of values in the dictionary
 */
DECLARE_DICT_METHOD(values);

/**
 * dict.remove(key: any)
 *
 * removes a key and it's value from the dictionary and returns
 * the removed key
 */
DECLARE_DICT_METHOD(remove);

/**
 * dict.assign(key: any, value: any)
 *
 * sets the value of a key in a dictionary or adds a new key-value
 * pair if the key doesn't exist
 */
DECLARE_DICT_METHOD(assign);

/**
 * dict.is_empty()
 *
 * returns true if the dict is empty or false otherwise
 */
DECLARE_DICT_METHOD(is_empty);

/**
 * dict.find_key(value: any)
 *
 * returns the key to the value in the list or nil if it does not exist
 */
DECLARE_DICT_METHOD(find_key);

/**
 * dict.to_list()
 *
 * returns the current dictionary into a list
 */
DECLARE_DICT_METHOD(to_list);

/**
 * dict.has_attr(attr: any)
 *
 * returns true if a dict has an attribute or false otherwise
 */
DECLARE_DICT_METHOD(has_attr);

/**
 * dict.__iter__()
 *
 * implementing the iterable interface
 */
DECLARE_DICT_METHOD(__iter__);

/**
 * dict.__itern__()
 *
 * implementing the iterable interface
 */
DECLARE_DICT_METHOD(__itern__);

#endif