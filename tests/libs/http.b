import http

var total = 10000
var start = time()

for i in 1..total {
  echo to_string(http.get('localhost:3000'))
}

echo 'Made ${total} http get calls in ${time() - start} seconds'