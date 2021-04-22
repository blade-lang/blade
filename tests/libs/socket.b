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
#   var data = client.receive()
#   if data {
#     if data.trim() == '.bye' {
#       client.close()
#       break
#     }
#     echo data
#   }
# }





# Client
var client = Socket()
client.connect(nil, 3000)
client.set_option(Socket.SO_SNDTIMEO, 10) # 10 milliseconds

while true {

  var message = ''
  var input
  while (input = stdin().read()) != '\n' {
    message += input
  }

  client.send(message)

  if message.trim() == '.bye' {
    client.close()
    break
  }
}