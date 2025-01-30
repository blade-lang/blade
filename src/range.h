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
 * See list.h
 */
DECLARE_RANGE_METHOD(__iter__);
DECLARE_RANGE_METHOD(__itern__);

/**
 * range.loop()
 *
 * calls a callback function for the range times.
 */
DECLARE_RANGE_METHOD(loop);

/**
 * range.range()
 *
 * returns the range value.
 */
DECLARE_RANGE_METHOD(range);

DECLARE_RANGE_METHOD(within);

#endif