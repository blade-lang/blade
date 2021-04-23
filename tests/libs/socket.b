import 'socket'
import 'io'

# # Server
# var server = Socket()
# server.bind(Socket.IP_ANY, 3000)
# server.listen()
# echo 'Listening on port ${Socket.IP_ANY}:3000'

# var client = server.accept()
# echo 'Client connected: ${client.host}'

# while true {
#   try {
#     var data = client.receive()
#     if data {
#       if data.trim() == '.bye' {
#         client.close()
#         break
#       }
#       echo data
#     }
#   } catch e {
#     echo 'Client disconnected! ${e.message}'
#     client.shutdown()
#     break
#   }
# }





# Client
var client = Socket()
client.connect(nil, 3000)
client.set_option(Socket.SO_SNDTIMEO, 1000) # 10 milliseconds

while true {

  var message = ''
  var input
  while (input = stdin().read()) != '\n' {
    message += input
  }

  try {
    client.send(message)
  } catch e {
    echo 'Connection closed ${e.message}'
    break
  }

  if message.trim() == '.bye' {
    client.close()
    break
  }
}