a = 'outer'

def test() {
  a = 'inner'
  echo 'It works! ' + a
}

echo a

echo test
test()

def test2(name, age, ...) {
  echo name
  echo age
  echo __args__
}

test2('Richard', 20, 'James')