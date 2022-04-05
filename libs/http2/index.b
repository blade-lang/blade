#
# @module http
#
# The `http` module provides a rich library to help in building HTTP 
# clients and servers. The module also provides a few generic abstractions 
# for simple HTTP operations such as a GET request.
# 
# ### Examples
# 
# The example below shows making a GET request to fetch a webpage.
# 
# ```blade
# import http
# 
# echo http.get('http://example.com')
# # <class HttpResponse instance at 0x600002adacd0>
# ```
# 
# There is a `post()` and `put()` alternative to the `get()` method called 
# above and they are documented below.
# 
# For a more controlled HTTP request, you should use the HttpClient class. 
# Below is an example of such implementation that sets the timeout for 
# receiving response back from the server to 30 seconds.
# 
# ```blade
# import http
# 
# var client = http.HttpClient()
# client.receive_timeout = 30000
# var res = client.send_request('http://example/endpoint?query=1', 'GET')
# echo res.body
# ```
# 
# Creating a server with the `http` module is also a breeze. 
# The example below shows an implementation of an HTTP API server listening on port 
# 3000 and simple returns the JSON of the request object itself.
# 
# ```blade
# import http
# import json
# 
# var server = http.server(3000)
# server.on_receive(|request, response| {
#   echo 'Request from ${request.ip} to ${request.path}.'
#   response.headers['Content-Type'] = 'application/json'
#   response.write(json.encode(request))
# })
# 
# echo 'Listening on Port 3000...'
# server.listen()
# ```
# 
# The `http` module does not make any assumption as to the type of data to be sent 
# in request bodies and for this reason, it should not be expected to automatically 
# convert dictionaries into JSON objects or create multipart/form-data request for you. 
# Rather, it gives the tools required to craft any request body of your choice.
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
#

import .response { HttpResponse }
import .status { * }
import .client { HttpClient }
import .server { HttpServer }

# single HttpClient for all requests lifetime
var _client = HttpClient()

/**
 * get(url: string)
 *
 * sends an Http GET request and returns an HttpResponse
 * or throws one of SocketException or Exception if it fails
 */
def get(url) {
  return _client.send_request(url, 'GET')
}

/**
 * post(url: string, [data: string])
 *
 * sends an Http POST request and returns an HttpResponse
 * or throws one of SocketException or Exception if it fails
 */
def post(url, data) {
  return _client.send_request(url, 'POST', data)
}

/**
 * put(url: string, [data: string])
 *
 * sends an Http PUT request and returns an HttpResponse
 * or throws one of SocketException or Exception if it fails
 */
def put(url, data) {
  return _client.send_request(url, 'PUT', data)
}

/**
 * server(port: int, address: string, is_secure: bool)
 * 
 * Creates an new HttpServer instance.
 */
def server(port, address, is_secure) {
  return HttpServer(port, address, is_secure)
}
