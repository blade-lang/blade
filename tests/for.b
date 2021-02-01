for i in [1,2,3,4,5] {
  echo i
}

var details = {name: 'Richard', age: 27, address: 'Nigeria'}

for x, y in details {
  echo x + ' = ' + y

  for i, j in [6,7,8,9,10] {
    echo i + ' = ' + j

    for i in [11,12,13,14,15] {
      echo i
    }
  }
}