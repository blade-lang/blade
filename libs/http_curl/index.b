/**
 * @module http
 * 
 * > **IMPORTANT NOTICE:**
 * >
 * > THIS MODULE IS DEPRECIATED AND WILL BE REMOVED FROM THE CORE
 * > LIBRARY AS SOON AS THE PURE BLADE IMPLEMENTATION IS STABLE.
 * > IT IS ONLY HERE FOR HISTORICAL REASONS AND TO SERVE AS A BASE
 * > BENCHMARK FOR THE DEVELOPMENT OF THE `http` MODULE.
 * >
 * > YOU SHOULD USE THE `http` MODULE INSTEAD AS ITS MORE SUPPORTED,
 * > AND ALL FURTHER DEVELOPMENTS TOWARDS HTTP WILL BE DONE THERE.
 * >
 * > BUG REPORTS AND ISSUES FOR THIS MODULE WILL NOT BE TREATED AS
 * > PRIORITY.
 *
 * The `chttp` module provides a rich library to help in building HTTP
 * clients and servers. The module also provides a few generic abstractions
 * for simple HTTP operations such as a GET request.
 *
 * ### Examples
 *
 * The example below shows making a GET request to fetch a webpage.
 *
 * ```blade
 * import chttp
 *
 * echo chttp.get('http://example.com')
 * # <class HttpResponse instance at 0x600002adacd0>
 * ```
 *
 * There is a `post()` and `put()` alternative to the `get()` method called
 * above and they are documented below.
 *
 * For a more controlled HTTP request, you should use the HttpClient class.
 * Below is an example of such implementation that sets the timeout for
 * receiving response back from the server to 30 seconds.
 *
 * ```blade
 * import chttp
 *
 * var client = chttp.HttpClient()
 * client.receive_timeout = 30000 # Optional
 * var res = client.send_request('http://example.com/endpoint?query=1', 'GET')
 * echo res.body.to_string()
 * ```
 *
 * Creating a server with the `chttp` module is also a breeze.
 * The example below shows an implementation of an HTTP API server listening on port
 * 3000 and simple returns the JSON of the request object itself.
 *
 * ```blade
 * import chttp
 * import json
 *
 * var server = chttp.server(3000)
 * server.handle('GET', '/', @(request, response) {
 *   response.json(request)
 * })
 * server.listen()
 * ```
 *
 * The `chttp` module does not make any assumption as to the type of data to be sent
 * in request bodies and for this reason, it should not be expected to automatically
 * convert dictionaries into JSON objects or create multipart/form-data request for you.
 * Rather, it gives the tools required to craft any request body of your choice.
 * 
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

import .response { HttpResponse }
import .status { * }
import .client { HttpClient }
import .server { HttpServer }

# single HttpClient for all requests lifetime
var _client = HttpClient()

/**
 * Sets the request headers for the current module instance.
 *  
 * This function returns HttpClient in order to allow for idiomatic 
 * chaining such as:
 * 
 * ```blade
 * import chttp
 * echo chttp.set_headers({
 *   'Authorization': 'Bearer SomeAPIBearerToken',
 *   'Host': 'example.com',
 * }).get('http://example.com/current-user').body.to_string()
 * ```
 * 
 * @param dict headers
 * @returns HttpClient
 * @raises  Exception
 */
def set_headers(headers) {
  if !is_dict(headers)
    raise Exception('headers must be a dictionary')
  _client.headers = headers
  return _client
}

/**
 * Sends an Http GET request and returns an HttpResponse
 * or throws one of SocketException or Exception if it fails.
 * 
 * @param string url
 * @returns HttpResponse
 * @raises  Exception
 * @raises  SocketException
 * @raises  HttpException
 */
def get(url) {
  return _client.get(url)
}

/**
 * Sends an Http POST request and returns an HttpResponse.
 * 
 * @param string url
 * @param string|bytes|nil data
 * @returns HttpResponse
 * @raises  Exception
 * @raises  SocketException
 * @raises  HttpException
 */
def post(url, data) {
  return _client.post(url, data)
}

/**
 * Sends an Http PUT request and returns an HttpResponse.
 * 
 * @param string url
 * @param string|bytes|nil data
 * @returns HttpResponse
 * @raises  Exception
 * @raises  SocketException
 * @raises  HttpException
 */
def put(url, data) {
  return _client.put(url, data)
}

/**
 * Sends an Http DELETE request and returns an HttpResponse.
 * 
 * @param string url
 * @returns HttpResponse
 * @raises  Exception
 * @raises  SocketException
 * @raises  HttpException
 */
def delete(url) {
  return _client.send_request(url, 'DELETE', nil)
}

/**
 * Creates an new HttpServer instance.
 * 
 * @param int port
 * @param string address
 * @returns HttpServer
 * @raises  Exception
 * @raises  SocketException
 * @raises  HttpException
 */
def server(port, address) {
  return HttpServer(port, address)
}

/**
 * Returns the default client.
 * 
 * @returns HttpClient
 */
def client() {
  return _client
}
