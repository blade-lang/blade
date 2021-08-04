#ifndef BLADE_RANGE_H
#define BLADE_RANGE_H

#include "common.h"
#include "native.h"
#include "vm.h"

#define DECLARE_RANGE_METHOD(name) DECLARE_METHOD(range##name)

/**
 * range.lower()
 *
 * returns the lower limit of the range
 */
DECLARE_RANGE_METHOD(lower);

/**
 * range.upper()
 *
 * returns the upper limit of the range
 */
DECLARE_RANGE_METHOD(upper);
/**
 * range.@iter()
 *
 * implementing the iterable interface
 */
DECLARE_RANGE_METHOD(__iter__);

/**
 * range.@itern()
 *
 * implementing the iterable interface
 */
DECLARE_RANGE_METHOD(__itern__);

#endif