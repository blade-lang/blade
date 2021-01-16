def fib(n) {
  if n < 2 return n
  return fib(n - 2) + fib(n - 1)
}

var start = time()
echo fib(35);
echo time() - start