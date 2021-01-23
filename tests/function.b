def test() {
  echo 'It works!\n'
}

print(test)
test()

def test2(name, age, ...) {
  print(name)
  print(age)
  print(__args__)
}

test2('Richard', 20, 'James')