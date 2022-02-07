import socket { * }

def serve(port, on_client_receive) {
  if !is_number(port)
    die Exception('number expected at parameter 1')
  if on_client_receive and !is_function(on_client_receive)
    die Exception('function expected at parameter 2')

  var id = 1

  var soc = Socket()
  soc.set_option(SO_REUSEADDR, true)
  soc.bind(port, IP_ANY)
  echo 'Listening on ${IP_ANY}:${port}...'
  soc.listen()

  while true {
    var client = soc.accept()
    echo 'Client connected...'

    # timeout if nothing is received after 10 second
    client.set_option(SO_RCVTIMEO, 2000)
    # timeout if we can't send after 10 second
    client.set_option(SO_SNDTIMEO, 2000)

    try {
      var data = client.receive()
      if data {
        echo 'Request received:\n${data}\n'
        on_client_receive(client, id, data)
      }
    } catch Exception e {
      echo 'Client error: ${e.message}'
    } finally {
      id++
      client.close()
      echo 'Client disconnected...'
    }
  }
}

serve(3000, |client, id, data| {

  var message = 'Hello to client ${id} from simple multi-client server implemented with Blade socket module'

  var response = 'HTTP/1.1 200 OK
X-Powered-By: Blade
Access-Control-Allow-Origin: *
Content-Type: application/json; charset=utf-8
Content-Length: ${message.length()}
ETag: W/"20-kpKo63uv4n6XEGgQeIwK7WAi6Ls"
Date: Sun, 18 Apr 2021 03:52:16 GMT

${message}'

  echo 'Response sent:\n${response}\n'

  client.send(response)
})
