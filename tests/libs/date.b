import 'date'

var total = 10000
var start = time()

iter var i = 0; i < total; i++ {
  echo Date().format('F d, Y g:i A')
}

echo 'Time taken for ${total} date allocation and formatting = ${time() - start}'