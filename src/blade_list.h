#ifndef BLADE_LIST_H
#define BLADE_LIST_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_LIST_METHOD(name) DECLARE_METHOD(list##name)

/**
 * list.length()
 *
 * returns the length of a list
 */
DECLARE_LIST_METHOD(length);

/**
 * list.append(item: any)
 *
 * adds an item to the top of a list
 * @return nil
 */
DECLARE_LIST_METHOD(append);

/**
 * list.clear()
 *
 * removes all items in a list
 */
DECLARE_LIST_METHOD(clear);

/**
 * list.clone()
 *
 * returns a deep clone of the list
 */
DECLARE_LIST_METHOD(clone);

/**
 * list.count(item: any)
 *
 * returns the number ot occurrence of item in list
 */
DECLARE_LIST_METHOD(count);

/**
 * list.extend(item: list2)
 *
 * adds the content of list2 into list1
 * @return nil
 */
DECLARE_LIST_METHOD(extend);

/**
 * list.index_of(item: any)
 *
 * returns the index at which the item first occurred in the list
 * @returns -1 if not found
 */
DECLARE_LIST_METHOD(index_of);

/**
 * list.insert(item: any, index: number)
 *
 * inserts the item into the specified index in the list.
 * if the index is far greater than the current max index + 1,
 * it fills the intermediate indices with nil values
 */
DECLARE_LIST_METHOD(insert);

/**
 * list.pop()
 *
 * removes the last item in a list and returns it
 */
DECLARE_LIST_METHOD(pop);

/**
 * list.shift([count: number])
 *
 * removes items from the beginning of a list.
 * - if count is not specified or 1, returns the item removed
 * - else, returns a list containing the removed items
 */
DECLARE_LIST_METHOD(shift);

/**
 * list.remove_at(index: number)
 *
 * removes the item at the specified index in the list
 */
DECLARE_LIST_METHOD(remove_at);

/**
 * list.remove(item: any)
 *
 * removes the first occurrence of the specified item from the list
 */
DECLARE_LIST_METHOD(remove);

/**
 * list.reverse()
 *
 * returns a list containing the reverse of the  the items in the list
 */
DECLARE_LIST_METHOD(reverse);

/**
 * list.sort()
 *
 * sorts the entries in a list
 */
DECLARE_LIST_METHOD(sort);

/**
 * list.contains(item: any)
 *
 * returns true if item exists in list or false otherwise
 */
DECLARE_LIST_METHOD(contains);

/**
 * list.delete(lower: number, upper: number)
 *
 * deletes a range of items from a list starting from lower to
 * upper.
 * upper limit must be greater than lower limit
 */
DECLARE_LIST_METHOD(delete);

/**
 * list.first()
 *
 * returns the first item in a list or nil if the list is empty
 */
DECLARE_LIST_METHOD(first);

/**
 * list.last()
 *
 * returns the last item in a list or nil if the list is empty
 */
DECLARE_LIST_METHOD(last);

/**
 * list.is_empty()
 *
 * returns true if a list is empty, false otherwise
 */
DECLARE_LIST_METHOD(is_empty);

/**
 * list.take(n: number)
 *
 * returns the first n number of items in the list,
 * or a copy of the list if n >= list.length
 * - if n is a negative number, returns list.take(list.length - n)
 */
DECLARE_LIST_METHOD(take);

/**
 * list.get(index: number)
 *
 * returns the value at index in the list
 */
DECLARE_LIST_METHOD(get);

/**
 * list.compact()
 *
 * removes all nil values from a list
 */
DECLARE_LIST_METHOD(compact);

/**
 * list.unique()
 *
 * removes all new list containing unique values
 */
DECLARE_LIST_METHOD(unique);

/**
 * list.zip(items... : list...)
 * Converts any arguments to lists, then merges elements of itself with
 * corresponding elements from each argument.
 *
 * This generates a sequence of list.length n-element lists, where n is one
 * more than the count of arguments.
 *
 * If the size of any argument is less than the size of the initial list,
 * nil values are supplied.
 */
DECLARE_LIST_METHOD(zip);

/**
 * list.to_dict()
 *
 * converts a list into a dictionary
 */
DECLARE_LIST_METHOD(to_dict);

/**
 * list.@iter()
 *
 * implementing the iterable interface
 */
DECLARE_LIST_METHOD(__iter__);

/**
 * list.@itern()
 *
 * implementing the iterable interface
 */
DECLARE_LIST_METHOD(__itern__);

#endif