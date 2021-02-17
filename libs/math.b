/**
 * Math
 *
 * Bird's mathematical library
 * @copyright Ore Richard */


class Math {
  # Math.PI
  # represents the ratio of the circumference of a circle 
  # to its diameter
  static var PI = 3.141592653589793

  # Mathematical infinity
  static var Infinity = 1/0

  # Mathematical NaN
  static var NaN = 0/0

  # factorial(n) 
  # calculates the product of all positive 
  # numbers less than or equal to a given positive number n
  # @return number
  static factorial(n) {
    var result = 1
    iter var i = 1; i <= n; i++ result *= i
    return result
  }
  

  # Core trigonometric functions
  # implemented internally in C to leverage precision
  # and machine level instructions

  # returns a numeric value between -1 and 1, which 
  # represents the sine of the angle given in radians
  static sin(n) {}

  # returns a numeric value between -1 and 1, which 
  # represents the cosine of the angle
  static cos(n) {}

  # returns a numeric value that represents the tangent 
  # of the angle given
  static tan(n) {}

  # returns the hyperbolic sine (in radians) of number n
  static sinh(n) {}

  # returns the hyperbolic cosine (in radians) of number n
  static cosh(n) {}

  # returns the hyperbolic tangent (in radians) of number n
  static tanh(n) {}

  # returns a numeric value between -(π/2) and π/2 radians 
  # for x between -1 and 1. 
  # If the value of x is outside this range, it returns Math.NaN
  static asin(n) {}

  # returns a numeric value between 0 and π radians for x 
  # between -1 and 1. 
  # If the value of x is outside this range, it returns Math.NaN
  static acos(n) {}

  # returns a numeric value between -(π/2) and π/2 radians.
  static atan(n) {}

  # returns a numeric value between -π and π representing the 
  # angle theta of an (x, y) point. 
  # This is the counterclockwise angle, measured in radians, 
  # between the positive X axis, and the point (x, y). 
  # Note that the arguments to this function pass the y-coordinate 
  # first and the x-coordinate second
  static atan2(x, y) {}

  # returns the hyperbolic arcsine (in radians) of number n
  static asinh(n) {}

  # returns the hyperbolic arccosine (in radians) of number n
  static acosh(n) {}

  # returns the hyperbolic arctangent (in radians) of number n
  static atanh(n) {}

  # returns e ** x, where x is the argument, and e is Euler's 
  # number (also known as Napier's constant), the base of the 
  # natural logarithms
  static exp(n) {}

  # returns (e ** x) - 1, where x is the argument, and e the base of 
  # the natural logarithms
  static expm1(n) {}

  # returns number n rounded up to the next largest integer
  static ceil(n) {}

  # returns the value of a number rounded to the nearest integer
  static round(n) {}

  # returns the natural logarithm (base e) of a number (mathematical ln(x))
  # NOTE:
  # - If the value of x is 0, the return value is always -inf
  # - If the value of x is negative, the return value is always Math.NaN
  static log(n) {}

  # returns the base 2 logarithm of the given number. 
  # If the number is negative, Math.NaN is returned
  static log2(n) {}

  # returns the base 10 logarithm of the given number. 
  # If the number is negative, Math.NaN is returned
  static log10(n) {}

  /**
  For very small values of x, adding 1 can reduce or eliminate precision.  
  The double floats used in JS give you about 15 digits of precision.  
  1 + 1e-15 = 1.000000000000001, but 1 + 1e-16 = 1.000000000000000 and therefore 
  exactly 1.0 in that arithmetic, because digits past 15 are rounded off.  

  When you calculate log(1 + x), you should get an answer very close to x, 
  if x is small (that's why these are called 'natural' logarithms).  
  If you calculate Math.log(1 + 1.1111111111e-15) you should get an answer 
  close to 1.1111111111e-15.  
  Instead, you will end up taking the logarithm of 1.00000000000000111022 
  (the roundoff is in binary so sometimes it gets ugly), so you get the answer 
  1.11022...e-15, with only  3 correct digits.  
  If, instead, you calculate Math.log1p(1.1111111111e-15) you will get a much 
  more accurate answer 1.1111111110999995e-15 with 15 correct digits of precision 
  (actually 16 in this case).
  
  returns the natural logarithm (base e) of 1 + a number
  If the value of x is less than -1, the return value is always Math.NaN.
  */
  static log1p(n) {}

  # returns the cube root of a number n
  static cbrt(n) {
    if n == nil return 0
    else if n < 0 return -(-n ** (1/3))
    return n ** (1/3)
  }

  static sign(n) {
    if !is_number(n) n = to_number(n)

    if n > 0 return 1
    else if n < 0 return -1
    return n
  }

  # A number representing the largest integer less than or 
  # equal to the specified number
  static floor(n) {}
}