def fib(n) {
  if n < 2 return n
  return fib(n - 2) + fib(n - 1)
}

class A {
  to_abs() {
    return 300
  }

  to_string() {
    return 'A class called A'
  }
}

echo abs(-10)
echo abs(A())
echo to_string([1, 2, 3])
echo to_string({name: 'Richard', age: 28})
echo to_string(A())

var start = time()
echo fib(35)
echo 'Time taken: ${time() - start}s'