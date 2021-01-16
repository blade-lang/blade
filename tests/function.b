def test() {
  echo 'It works!'
}

echo test
test()


# testing stack trace

def a() { 
  b()
}

def b() {
  c()
}

a()