import 'date'

var total = 10000
var start = time()

for i in 1..total {
  echo Date().format('F d, Y g:i:s.u A')
}

echo 'Time taken for ${total} date allocation and formatting = ${time() - start}s'