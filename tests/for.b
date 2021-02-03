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

for n in 'name' {
  echo n
}

for g in bytes([10, 21, 13, 47]) {
  echo g
}

class Iterable {
  var index = -1
  var items = ['Richard', 'Alex', 'Justina']

  __iter__() {
    return self.items[self.index]
  }

  __itern__() {
    if self.index < self.items.length() - 1
      return self.index++
    return empty
  }
}

for it in Iterable() {
  echo it
}