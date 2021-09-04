var test = |a| {
  echo a
}
test('It works')

def main() {
  var g = 'Coke'
  echo g
  var concat = || {
    g += ' is the best'
  }
  concat()
  echo g
}

main()
