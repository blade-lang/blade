var x = "global\n"
def outer() {
  var x = "outer\n"
  def inner() {
    echo x
  }
  inner()
}
outer()



# test 2

def outer() {
  var x = "value\n"
  def middle() {
    def inner() {
      echo x
    }

    echo "create inner closure\n"
    return inner
  }

  echo "return from outer\n"
  return middle
}

var mid = outer()
var _in = mid()
_in()


iter var i = 0; i < 10; i++ {
  var start = time()
  var sum = 0
  iter var j = 0; j < 1000000; j++ {
    def outer(a, b, c) {
      def inner() {
        return a + b + c
      }
      return inner
    }

    var closure = outer(j, j, j)
    sum = sum + closure()
  }

  print(sum)
  print(time() - start)
}