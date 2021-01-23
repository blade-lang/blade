var start = time()

var prime = [false] * 100000001

prime[1] = true
prime[0] = true

iter var i = 2; i <= 100000000; i++ {
  if prime[i] == false {
    iter var j = i * i; j <= 100000000; j = j + 1 {
      prime[j] = true
    }
  }
}

echo time() - start