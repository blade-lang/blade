import 'date'

var total = 1000000
var start = time()

/* iter var i = 0; i < total; i++ {
  echo Date().format('F d, Y g:i:s.u A')
} */

for i in 1..total {
  echo Date().format('F d, Y g:i:s.u A')
}

echo 'Time taken for ${total} date allocation, formatting and printing = ${time() - start}s'