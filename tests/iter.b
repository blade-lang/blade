iter var i = 0; i < 10; i = i + 1 echo i

iter var i = 0; i < 10; i = i + 1 {
  echo i + ' iteration'
}

var j = 20

iter ; j >= 1; j = j - 1 {
  echo 'J = ' + j
}

iter var i = 1; i <= 10; i = i + 1 {
  echo 'hi = ' + i
  iter var j = 10; j >= 1; j = j - 1 {
    if j > 4 and j < 7 {
      continue
    }
    echo 'jay = ' + j
  }
}

iter var x = 0; x < 10; x = x + 1 {
  if x == 3 break
  echo 'The new x = ' + x
}