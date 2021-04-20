import 'http'

# for i in 1..1000 {
#   echo 'Attempt ${i}'
#   echo HttpClient().get('localhost:3000')
# }

echo HttpClient().
        headers({
          'Authorization': 'Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiI2MDY0ZDM1NjA1OTUzZDcyNzYwY2NkOTAiLCJpYXQiOjE2MTg4NDA0ODgsImV4cCI6MTYyMTI1OTY4OH0.25ZTTjF1Sw1k7vlefw5Y74g9o9FfaJr69oXaaLFefpI'
        }).
        get('localhost:8000/ads')