def log(n) {
  var m = (n - 1) / (n + 1)
  var g = m
  iter var i = 3; i <= (n ** 5); i += 2 {
    g += (1 / i) * (m ** i)
  }
  return 2 * g
}

import 'math'

echo log(21)
echo Math.log(21)