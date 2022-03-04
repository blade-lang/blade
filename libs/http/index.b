#
# @module http
#
# Provides interfaces for working with Http client requests.
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
