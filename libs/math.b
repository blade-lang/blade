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

  # factorial(n) 
  # calculates the product of all positive 
  # numbers less than or equal to a given positive number n
  # @return number
  static factorial(n) {
    var result = 1
    iter var i = 1; i <= n; i++ result *= i
    return result
  }

  /* static sin(n) {
    var t = n, sine = t

    iter var a = 1; a < 24; a++ {
      var mult = -n * n / ((2 * a + 1) * (2 * a))
      t *= mult
      sine += t
    }

    return sine
  } */
  static sin(n) {}
}