var x = "global"
def outer() {
  var x = "outer"
  def inner() {
    echo x
  }
  inner()
}
outer()



# test 2

def outer() {
  var x = "value"
  def middle() {
    def inner() {
      echo x
    }

    echo "create inner closure"
    return inner
  }

  echo "return from outer"
  return middle
}

var mid = outer()
var _in = mid()
_in()