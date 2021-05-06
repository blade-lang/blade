import 'socket'
import 'io'

# # Server
# var server = Socket()
# server.set_option(Socket.SO_REUSEADDR, true)
# server.bind(Socket.IP_ANY, 3000)
# server.listen()
# echo 'Listening on port ${Socket.IP_ANY}:3000'

# while true {
#   var client = server.accept()
#   # client.set_option(Socket.SO_RCVTIMEO, 100);
#   echo 'Client connected: ${client.host}'

#   while true {
#     try {
#       var data = client.receive()
#       if data {
#         echo data.trim()
#         if data.trim() == '.bye' {
#           client.close()
#           echo 'Client disconnected!'
#           break
#         }
#       }
#     } catch e {
#       echo 'Client disconnected with error -> ${e.message}'
#       break
#     }
#   }
# }





# # Client
# var client = Socket()
# client.connect(nil, 3000, 1000) # 1 seconds
# # client.set_option(Socket.SO_SNDTIMEO, 100) # 100 milliseconds

# while true {
#   var message = ''
#   var input
#   while (input = stdin().read()) != '\n' {
#     message += input
#   }

#   try {
#     client.send('${message}\n')
#   } catch e {
#     echo 'Connection closed ${e.message}'
#     break
#   }

#   if message.trim() == '.bye' {
#     client.close()
#     break
#   }
# }


var start = time()
var total = 100000

for i in 1..total {
  echo 'Attempt ${i}'
  var client = Socket()
  client.connect(nil, 3000, 1000)
  client.send('GET / HTTP/1.1\r\n\r\n')
  var data = client.receive()

  if data.index_of('Content-Length') {
    var m = data.matches('/Content\-Length:\s*\d+/')
    if m {
      var length = to_number(m[0].replace('/[^0-9]/', ''))
      while data.length() < length {
        data += client.receive()
      }
    }
  }

  echo data
  client.close()
}

echo 'Made ${total} http get calls in ${time() - start} seconds'