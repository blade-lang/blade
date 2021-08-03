# 
# @module math
#
# Blade's mathematical library
# @copyright 2021, Ore Richard Muyiwa
#

import _math

/**
 * PI
 *
 * represents the ratio of the circumference of a circle 
 * to its diameter
 */
var PI = 3.141592653589793

/**
 * E
 *
 * represents Euler's number, the base of natural logarithms
 */
var E = 2.718281828459045

/**
 * LOG_10
 *
 * represents the natural logarithm of 10
 */
var LOG_10 = 2.302585092994046

/**
 * LOG_10_E
 *
 * represents the base 10 logarithm of e
 */
var LOG_10_E = 0.4342944819032518

/**
 * LOG_2
 *
 * represents the natural logarithm of 2
 */
var LOG_2 = 0.6931471805599453

/**
 * LOG_2_E
 *
 * represents the base 2 logarithm of e
 */
var LOG_2_E = 1.4426950408889634

/**
 * ROOT_2
 *
 * represents the square root of 2
 */
var ROOT_2 = 1.4142135623730951

/**
 * ROOT_3
 *
 * represents the square root of 3
 */
var ROOT_3 = 1.732050807568877

/**
 * ROOT_HALF
 *
 * represents the square root of 1/2
 */
var ROOT_HALF = 0.7071067811865476

/**
 * Infinity
 *
 * Mathematical infinity
 */
var Infinity = 1/0

/**
 * NaN
 *
 * Mathematical NaN
 */
var NaN = 0/0

/**
 * factorial(n: number)
 *
 * calculates the product of all positive 
 * numbers less than or equal to a given positive number n
 * @return number
 */
def factorial(n) {
  if !is_number(n) {
    die Exception('number expected')
  } else if n < 0 {
    die Exception('Math error')
  }

  var result = 1
  iter var i = 1; i <= n; i++ result *= i
  return result
}


# Core trigonometric functions
# implemented internally in C to leverage precision
# and machine level instructions

/**
 * sin(n: number)
 *
 * returns a numeric value between -1 and 1, which 
 * represents the sine of the angle given in radians
 * @return number
 */
def sin(n) {
  return _math.sin(n)
}

/**
 * cos(n: number)
 * 
 * returns a numeric value between -1 and 1, which 
 * represents the cosine of the angle
 * @return number
 */
def cos(n) {
  return _math.cos(n)
}

/**
 * tan(n: number)
 * 
 * returns a numeric value that represents the tangent 
 * of the angle given
 * @return number
 */
def tan(n) {
  return _math.tan(n)
}

/**
 * sinh(n: number)
 * 
 * returns the hyperbolic sine (in radians) of number n
 * @return number
 */
def sinh(n) {
  return _math.sinh(n)
}

/**
 * cosh(n: number)
 * 
 * returns the hyperbolic cosine (in radians) of number n
 * @return number
 */
def cosh(n) {
  return _math.cosh(n)
}

/**
 * tanh(n: number)
 * 
 * returns the hyperbolic tangent (in radians) of number n
 * @return number
 */
def tanh(n) {
  return _math.tanh(n)
}

/**
 * returns a numeric value between -(π/2) and π/2 radians 
 * for x between -1 and 1. 
 * If the value of x is outside this range, it returns NaN
 * @return number
 */
def asin(n) {
  return _math.asin(n)
}

/**
 * acos(n: number)
 * 
 * returns a numeric value between 0 and π radians for x 
 * between -1 and 1. 
 * If the value of x is outside this range, it returns NaN
 * @return number
 */
def acos(n) {
  return _math.acos(n)
}

/**
 * returns a numeric value between -(π/2) and π/2 radians.
 * @return number
 */
def atan(n) {
  return _math.atan(n)
}

/**
 * atan2(n: number)
 * 
 * returns a numeric value between -π and π representing the 
 * angle theta of an (x, y) point. 
 * This is the counterclockwise angle, measured in radians, 
 * between the positive X axis, and the point (x, y). 
 * @note the arguments to this function pass the y-coordinate 
 * first and the x-coordinate second
 * @return number
 */
def atan2(x, y) {
  return _math.atan2(x, y)
}

/**
 * asinh(n: number)
 * 
 * returns the hyperbolic arcsine (in radians) of number n
 * @return number
 */
def asinh(n) {
  return _math.asinh(n)
}

/**
 * acosh(n: number)
 * 
 * returns the hyperbolic arccosine (in radians) of number n
 * @return number
 */
def acosh(n) {
  return _math.acosh(n)
}

/**
 * atanh(n: number)
 * 
 * returns the hyperbolic arctangent (in radians) of number n
 * @return number
 */
def atanh(n) {
  return _math.atanh(n)
}

/**
 * exp(n: number)
 * 
 * returns e ** x, where x is the argument, and e is Euler's 
 * number (also known as Napier's constant), the base of the 
 * natural logarithms
 * @return number
 */
def exp(n) {
  return _math.exp(n)
}

/**
 * expm1(n: number)
 * 
 * returns (e ** x) - 1, where x is the argument, and e the base of 
 * the natural logarithms
 * @return number
 */
def expm1(n) {
  return _math.expm1(n)
}

/**
 * ceil(n: number)
 * 
 * returns number n rounded up to the next largest integer
 * @return number
 */
def ceil(n) {
  return _math.ceil(n)
}

/**
 * round(n: number)
 * 
 * returns the value of a number rounded to the nearest integer
 * @return number
 */
def round(n) {
  return _math.round(n)
}

/**
 * log(n: number)
 * 
 * returns the natural logarithm (base e) of a number (mathematical ln(x))
 * @note
 * - If the value of x is 0, the return value is always -inf
 * - If the value of x is negative, the return value is always NaN
 * @return number
 */
def log(n) {
  return _math.log(n)
}

/**
 * log2(n: number)
 * 
 * returns the base 2 logarithm of the given number. 
 * If the number is negative, NaN is returned
 * @return number
 */
def log2(n) {
  return _math.log2(n)
}

/**
 * log10(n: number)
 * 
 * returns the base 10 logarithm of the given number. 
 * If the number is negative, NaN is returned
 * @return number
 */
def log10(n) {
  return _math.log10(n)
}

/**
 * log1p(n: number)
 * 
 * For very small values of x, adding 1 can reduce or eliminate precision.  
 * The double floats used in JS give you about 15 digits of precision.  
 * 1 + 1e-15 = 1.000000000000001, but 1 + 1e-16 = 1.000000000000000 and therefore 
 * exactly 1.0 in that arithmetic, because digits past 15 are rounded off.  
 * 
 * When you calculate log(1 + x), you should get an answer very close to x, 
 * if x is small (that's why these are called 'natural' logarithms).  
 * If you calculate log(1 + 1.1111111111e-15) you should get an answer 
 * close to 1.1111111111e-15.  
 * Instead, you will end up taking the logarithm of 1.00000000000000111022 
 * (the roundoff is in binary so sometimes it gets ugly), so you get the answer 
 * 1.11022...e-15, with only  3 correct digits.  
 * If, instead, you calculate log1p(1.1111111111e-15) you will get a much 
 * more accurate answer 1.1111111110999995e-15 with 15 correct digits of precision 
 * (actually 16 in this case).
 * 
 * returns the natural logarithm (base e) of 1 + a number
 * If the value of x is less than -1, the return value is always NaN.
 * @return number
 */
def log1p(n) {
  return _math.log1p(n)
}

/**
 * cbrt(n: number)
 * 
 * returns the cube root of a number n
 * @return number
 */
def cbrt(n) {
  if !is_number(n) {
    die Exception('number expected')
  }

  if n == nil return 0
  else if n < 0 return -(-n ** (1/3))
  return n ** (1/3)
}

/**
 * sign(n: number)
 *
 * returns either a positive or negative +/- 1, indicating the sign of 
 * a number passed into the argument. 
 * If the number passed into sign() is 0, it will return a 0.
 * @return number
 */
def sign(n) {
  if !is_number(n) n = to_number(n)

  if n > 0 return 1
  else if n < 0 return -1
  return n
}

/**
 * floor(n: number)
 * 
 * A number representing the largest integer less than or 
 * equal to the specified number
 * @return number
 */
def floor(n) {
  return _math.floor(n)
}

/**
 * is_nan(n: number)
 * 
 * returns true if the given number is equal to NaN or false otherwise
 * @return bool
 */
def is_nan(n) {
  return n == NaN
}

/**
 * is_inf(n: number)
 * 
 * returns true if the given number is equal to Infinity or -Infinity 
 * or false otherwise
 * @return bool
 */
def is_inf(n) {
  return n == Infinity or n == -Infinity
}

/**
 * is_finite(n: number)
 * 
 * return true if x is neither an Infinity nor a NaN, and false otherwise
 * @return bool
 */
def is_finite(n) {
  return !is_inf(n) and !is_nan(n)
}

/**
 * trunc(n: number)
 * 
 * returns the integer part of a number by removing any fractional
 * @return number
 */
def trunc(n) {
  if !is_number(n) {
    die Exception('number expected')
  }

  return n < 0 ?  ceil(n) : floor(n)
}

/**
 * sqrt(n: number)
 * 
 * returns the square root of a nunmber
 * @return number
 */
def sqrt(n) {
  if !is_number(n) {
    die Exception('number expected')
  }

  return n ** 0.5
}

/**
 * sum(arg: iterable)
 *
 * calculate the sum of all the elements in the input iterable
 * the default start value for the product is 1.
 * when the iterable is empty, it returns 1
 * @return number
 */
def sum(arg) {
  if !is_iterable(arg) {
    die Exception('iterable expected')
  }

  var result = 0

  for i in arg {
    if is_list(i) or is_dict(i) 
      result += product(i)
    else result += i
  }

  return result
}

/**
 * product(arg: iterable)
 *
 * calculate the product of all the elements in the input iterable
 * the default start value for the product is 1.
 * when the iterable is empty, it returns 1
 * @return number
 */
def product(arg) {
  if !is_iterable(arg) {
    die Exception('iterable expected')
  }

  var result = 1

  for i in arg {
    if is_list(i) or is_dict(i) 
      result *= product(i)
    else result *= i
  }

  return result
}


/**
 * fraction(n: number)
 *
 * returns the fractional part of a number as a whole number 
 * by removing any integer
 * @return number
 */
def fraction(n) {
  if !is_number(n) {
    die Exception('number expected')
  }

  var str = to_string(n).split('.')
  return str.length() == 1 ? 0 : to_number(str[1])
}

