iter var i = 0; i < 10; i++ echo i + '\n'

iter var i = 0; i < 10; i++ {
  echo i + ' iteration' + '\n'
}

var j = 20

iter ; j >= 1; j-- {
  echo 'J = ' + j + '\n'
}

iter var i = 1; i <= 10; i++ {
  echo 'hi = ' + i + '\n'
  iter var j = 10; j >= 1; j-- {
    if j > 4 and j < 7 {
      continue
    }
    echo 'jay = ' + j + '\n'
  }
}

 iter var x = 0; x < 10; x++ {
  if x == 3 break
  echo 'The new x = ' + x + '\n'
}