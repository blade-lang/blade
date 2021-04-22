import 'http'
import 'os'

for i in 1..1000 {
  echo 'Attempt ${i}'
  echo HttpClient().get('localhost:8000')
}

# echo Os.exec('pwd')

# echo HttpClient().
#         headers({
#           'Authorization': 'Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiI2MDY0ZDM1NjA1OTUzZDcyNzYwY2NkOTAiLCJpYXQiOjE2MTg5NjQwMDQsImV4cCI6MTYyMTM4MzIwNH0.HmwochIX8BfW0v7PyxEB4X2YjC9r5SJDbqR4PmzfdkY'
#         }).
#         post('localhost:8000/images/upload/dp', {
#           file: file('me.png')
#         })
        # post('localhost:8000/ads/create', {
        #   username: 'eqliqandfriends@gmail.com',
        #   password: 'password'
        # })