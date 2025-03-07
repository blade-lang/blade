/** 
 * @module math
 *
 * This module contains functions and constants to make trigonometric and 
 * non-trigonometric mathematics a breeze. The module also defines a couple
 * of commonly used scientific and mathematical constants such as `PI`.
 * 
 * @copyright 2021, Richard Ore and Blade contributors
 */

import _math

/**
 * represents the ratio of the circumference of a circle 
 * to its diameter
 */
var PI = 3.141592653589793

/**
 * represents Euler's number, the base of natural logarithms
 */
var E = 2.718281828459045

/**
 * represents the natural logarithm of 10
 */
var LOG_10 = 2.302585092994046

/**
 * represents the base 10 logarithm of e
 */
var LOG_10_E = 0.4342944819032518

/**
 * represents the natural logarithm of 2
 */
var LOG_2 = 0.6931471805599453

/**
 * represents the base 2 logarithm of e
 */
var LOG_2_E = 1.4426950408889634

/**
 * represents the square root of 2
 */
var ROOT_2 = 1.4142135623730951

/**
 * represents the square root of 3
 */
var ROOT_3 = 1.732050807568877

/**
 * represents the square root of 1/2
 */
var ROOT_HALF = 0.7071067811865476

/**
 * Mathematical infinity
 */
var Infinity = 1/0

/**
 * Mathematical NaN
 */
var NaN = 0/0

/**
 * factorial(n: number)
 *
 * calculates the product of all positive 
 * numbers less than or equal to a given positive number n
 * @returns number
 * 
 * Example:
 * 
 * ```blade-repl
 * %> import math
 * %> math.factorial(60)
 * 8.320987112741392e+81
 * ```
 */
def factorial(n) {
  if !is_number(n) {
    raise Exception('number expected')
  } else if n < 0 {
    raise Exception('Math error')
  }

  var result = 1
  iter var i = 1; i <= n; i++ result *= i
  return result
}


# Core trigonometric functions
# implemented internally in C to leverage precision, machine instructions and speed.

/**
 * Returns a numeric value between -1 and 1, which 
 * represents the sine of the angle given in radians.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.sin(46)
 * 0.9017883476488092
 * ```
 * 
 * @param number n
 * @returns number
 */
def sin(n) {
  return _math.sin(n)
}

/**
 * Returns a numeric value between -1 and 1, which 
 * represents the cosine of the angle.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.cos(93)
 * 0.3174287015197017
 * ```
 * 
 * @param number n
 * @returns number
 */
def cos(n) {
  return _math.cos(n)
}

/**
 * Returns a numeric value that represents the tangent 
 * of the angle given.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.tan(11.43)
 * -2.155225644164932
 * ```
 * 
 * @param number n
 * @returns number
 */
def tan(n) {
  return _math.tan(n)
}

/**
 * Returns the hyperbolic sine (in radians) of number n.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.sinh(1.4)
 * 1.904301501451534
 * ```
 * 
 * @param number n
 * @returns number
 */
def sinh(n) {
  return _math.sinh(n)
}

/**
 * Returns the hyperbolic cosine (in radians) of number n.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.cosh(1.91)
 * 3.450584592563374
 * ```
 * 
 * @param number n
 * @returns number
 */
def cosh(n) {
  return _math.cosh(n)
}

/**
 * Returns the hyperbolic tangent (in radians) of number n.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.tanh(2.19)
 * 0.975
 * 
 * @param number n
 * @returns number2591705196751
 * ```
 */
def tanh(n) {
  return _math.tanh(n)
}

/**
 * Returns a numeric value between -(π/2) and π/2 radians 
 * for x between -1 and 1. 
 * If the value of x is outside this range, it returns NaN.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.asin(0.123)
 * 0.123312275191872
 * ```
 * 
 * @param number n
 * @returns number
 */
def asin(n) {
  return _math.asin(n)
}

/**
 * Returns a numeric value between 0 and π radians for x 
 * between -1 and 1. If the value of x is outside this range, it 
 * returns NaN.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.acos(0.471)
 * 1.080372275769021
 * ```
 * 
 * @param number n
 * @returns number
 */
def acos(n) {
  return _math.acos(n)
}

/**
 * Returns a numeric value between -(π/2) and π/2 radians.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.atan(math.Infinity)
 * 1.570796326794897
 * ```
 * 
 * @param number n
 * @returns number
 */
def atan(n) {
  return _math.atan(n)
}

/**
 * Returns a numeric value between -π and π representing the 
 * angle theta of an (x, y) point. This is the counterclockwise angle, 
 * measured in radians, between the positive X axis, and the point (x, y). 
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.atan2(math.Infinity, -math.Infinity)
 * 2.356194490192345
 * %> math.atan2(1, 2)
 * 0.4636476090008061
 * %> math.atan2(-1.5, 2.4)
 * -0.5585993153435624
 * ```
 * 
 * @note the arguments to this function pass the y-coordinate first and the x-coordinate second.
 * @param number n
 * @returns number
 */
def atan2(x, y) {
  return _math.atan2(x, y)
}

/**
 * Returns the hyperbolic arc-sine (in radians) of number n.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.asinh(3.42)
 * 1.943507380182802
 * ```
 * 
 * @param number n
 * @returns number
 */
def asinh(n) {
  return _math.asinh(n)
}

/**
 * Returns the hyperbolic arc-cosine (in radians) of number n.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.acosh(1.21)
 * 0.637237379754108
 * ```
 * 
 * @param number n
 * @returns number
 */
def acosh(n) {
  return _math.acosh(n)
}

/**
 * Returns the hyperbolic arc-tangent (in radians) of number n.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.atanh(0.11)
 * 0.1104469157900971
 * ```
 * 
 * @param number n
 * @returns number
 */
def atanh(n) {
  return _math.atanh(n)
}

/**
 * Returns e ** x, where x is the argument, and e is Euler's 
 * number (also known as Napier's constant), the base of the 
 * natural logarithms.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.exp(4)
 * 54.59815003314424
 * ```
 * 
 * @param number n
 * @returns number
 */
def exp(n) {
  return _math.exp(n)
}

/**
 * Returns (e ** x) - 1, where x is the argument, and e the base of 
 * the natural logarithms.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.expm1(1)
 * 1.718281828459045
 * ```
 * 
 * @param number n
 * @returns number
 */
def expm1(n) {
  return _math.expm1(n)
}

/**
 * Returns number n rounded up to the next largest integer.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.ceil(1.65)
 * 2
 * %> math.ceil(1.01)
 * 2
 * ```
 * 
 * @param number n
 * @returns number
 */
def ceil(n) {
  return _math.ceil(n)
}

/**
 * Returns the value of a number rounded to the nearest integer.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.round(103.51)
 * 104
 * %> math.round(103.49)
 * 103
 * ```
 * 
 * @param number n
 * @returns number
 */
def round(n) {
  return _math.round(n)
}

/**
 * Returns the natural logarithm (base e) of a number (mathematical ln(x)).
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.log(45)
 * 3.80666248977032
 * ```
 * 
 * @note If the value of x is 0, the return value is always -inf.
 * @note If the value of x is negative, the return value is always NaN.
 * @param number n
 * @returns number
 */
def log(n) {
  return _math.log(n)
}

/**
 * Returns the base 2 logarithm of the given number. 
 * If the number is negative, NaN is returned
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.log2(45)
 * 5.491853096329675
 * ```
 * 
 * @param number n
 * @returns number
 */
def log2(n) {
  return _math.log2(n)
}

/**
 * Returns the base 10 logarithm of the given number. 
 * If the number is negative, NaN is returned.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.log10(45)
 * 1.653212513775344
 * ```
 * 
 * @param number n
 * @returns number
 */
def log10(n) {
  return _math.log10(n)
}

/**
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
 * (the round-off is in binary so sometimes it gets ugly), so you get the answer
 * 1.11022...e-15, with only  3 correct digits.  
 * If, instead, you calculate log1p(1.1111111111e-15) you will get a much 
 * more accurate answer 1.1111111110999995e-15 with 15 correct digits of precision 
 * (actually 16 in this case).
 * 
 * returns the natural logarithm (base e) of 1 + a number
 * If the value of x is less than -1, the return value is always NaN.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.log1p(45)
 * 3.828641396489095
 * ```
 * 
 * @param number n
 * @returns number
 */
def log1p(n) {
  return _math.log1p(n)
}

/**
 * Returns the cube root of a number n.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.cbrt(64)
 * 4
 * ```
 * 
 * @param number n
 * @returns number
 */
def cbrt(n) {
  if !is_number(n) {
    raise Exception('number expected')
  }

  if n == nil return 0
  else if n < 0 return -(-n ** (1/3))
  return n ** (1/3)
}

/**
 * Returns either a positive or negative +/- 1, indicating the sign of 
 * a number passed into the argument. 
 * If the number passed into sign() is 0, it will return a 0.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.sign(10)
 * 1
 * %> math.sign(-20)
 * -1
 * %> math.sign(-0)
 * -0
 * %> math.sign(0)
 * 0
 * ```
 * 
 * @param number n
 * @returns number
 */
def sign(n) {
  if !is_number(n) n = to_number(n)

  if n > 0 return 1
  else if n < 0 return -1
  return n
}

/**
 * A number representing the largest integer less than or 
 * equal to the specified number
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.floor(1.92)
 * 1
 * ```
 * 
 * @param number n
 * @returns number
 */
def floor(n) {
  return _math.floor(n)
}

/**
 * is_nan(n: number)
 * 
 * returns true if the given number is equal to NaN or false otherwise.
 * 
 * @param number n
 * @returns bool
 */
def is_nan(n) {
  return n == NaN
}

/**
 * Returns `true` if the given number is equal to Infinity or -Infinity 
 * or `false` otherwise.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.is_inf(math.Infinity)
 * true
 * %> math.is_inf(-math.Infinity)
 * true
 * %> math.is_inf(0)
 * false
 * ```
 * 
 * @param number n
 * @returns bool
 */
def is_inf(n) {
  return n == Infinity or n == -Infinity
}

/**
 * Return `true` if x is neither an Infinity nor a NaN, and `false` otherwise.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.is_finite(0)
 * true
 * %> math.is_finite(math.NaN)
 * true
 * %> math.is_finite(-math.Infinity)
 * false
 * ```
 * 
 * @param number n
 * @returns bool
 */
def is_finite(n) {
  return !is_inf(n) and !is_nan(n)
}

/**
 * Returns the integer part of a number by removing any fractional.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.trunc(1.92)
 * 1
 * %> math.trunc(1.0)
 * 1
 * %> math.trunc(1.01)
 * 1
 * %> math.trunc(-1.01)
 * -1
 * ```
 * 
 * @param number n
 * @returns number
 */
def trunc(n) {
  if !is_number(n) {
    raise Exception('number expected')
  }

  return n < 0 ?  ceil(n) : floor(n)
}

/**
 * Returns the square root of a number.
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.sqrt(100)
 * 10
 * ```
 * 
 * @param number n
 * @returns number
 */
def sqrt(n) {
  if !is_number(n) {
    raise Exception('number expected')
  }

  return n ** 0.5
}

/**
 * Calculates the sum of all the elements in the input iterable
 * the default start value for the product is 1.
 * when the iterable is empty, it returns 1
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.sum([1, 2, [3, 4, [5, 6]]])
 * 21
 * ```
 * 
 * @param iterable arg
 * @returns number
 */
def sum(arg) {
  if !is_iterable(arg) {
    raise Exception('iterable expected')
  }

  var result = 0

  for i in arg {
    if !is_number(i) and !is_list(i) and !is_dict(i)
      raise Exception('invalid item in sumation iterable')

    if is_list(i) or is_dict(i) 
      result += sum(i)
    else result += i
  }

  return result
}

/**
 * Calculates the product of all the elements in the input iterable
 * the default start value for the product is 1.
 * when the iterable is empty, it returns 1
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.product([1, 2, [3, 4, [5, 6]]])
 * 720
 * ```
 * 
 * @param iterable arg
 * @returns number
 */
def product(arg) {
  if !is_iterable(arg) {
    raise Exception('iterable expected')
  }

  var result = 1

  for i in arg {
    if !is_number(i) and !is_list(i) and !is_dict(i)
      raise Exception('invalid item in product iterable')

    if is_list(i) or is_dict(i) 
      result *= product(i)
    else result *= i
  }

  return result
}


/**
 * Returns the fractional part of a number as a whole number 
 * by removing any integer
 * 
 * Example:
 * 
 * ```blade-repl
 * %> math.fraction(1.92)
 * 92
 * ```
 * 
 * @param number n
 * @returns number
 */
def fraction(n) {
  if !is_number(n) {
    raise Exception('number expected')
  }

  var str = to_string(n).split('.')
  return str.length() == 1 ? 0 : to_number(str[1])
}

