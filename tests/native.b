def fib(n) {
  if n < 2 return n
  return fib(n - 2) + fib(n - 1)
}

var start = time()
print(fib(35))
print(time() - start)