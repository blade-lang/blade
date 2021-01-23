echo [1, 2, 3, 4, 5]

/* var start = time()
var c = [1, 2] * 500000000
print(time() - start) */

var list = [1, 'Ink', 3.5, false, 'A', nil]
print(list)

print(list[1,3])

list[4] = 'Maggie'
print(list)

list += [
  'Apple', 
  'Peach'
]
print(list)

list *= 3
print(list)