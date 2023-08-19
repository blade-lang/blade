/**
 * @module json
 *
 * Provides APIs for encoding and decoding JSON data.
 *
 * JavaScript Object Notation (JSON) is a lightweight, text-based,
 * language-independent data interchange format.  It was derived from
 * the ECMAScript Programming Language Standard.  JSON defines a small
 * set of formatting rules for the portable representation of structured
 * data.
 *
 * This implementation complies with [RFC 8259](https://datatracker.ietf.org/doc/html/rfc8259).
 *
 * ### JSON to Blade value mapping
 *
 * | JSON | Blade |
 * |------|-------|
 * | Null | Nil |
 * | String | String |
 * | Number | Number |
 * | Boolean | Boolean |
 * | Array | List |
 * | Object | Dict |
 *
 *
 * ### Blade to JSON object mapping
 *
 * | Blade | JSON |
 * |-------|------|
 * | `nil` | Null |
 * | Integer | Number |
 * | Number | Number |
 * | Char | String |
 * | String | String |
 * | List | Array |
 * | Dict | Object |
 * | Instance of class implementing `to_json()` decorator | Any |
 *
 *
 * Example,
 *
 * ```blade-repl
 * %> import json
 * %> json.encode([1, 2, 3])
 * '[1,2,3]'
 * %>
 * %> json.encode({name: 'Blade', version: '0.0.7'})
 * '{"name":"Blade","version":"0.0.7"}'
 * %>
 * %> json.encode({name: 'Blade', version: '0.0.7'}, false)
 * '{
 *   "name": "Blade",
 *   "version": "0.0.7"
 * }'
 * ```
 *
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

import .encoder { * }
import _json { _decode }


/**
 * encode(value: any [, compact: boolean = true [, max_depth: number = 1024]])
 * 
 * JSON encodes the given value with a recursive depth up to `max_depth`.
 * 
 * If _compact_ is `false`, the resulting json string will be 
 * tightly packed. i.e. spaces will be trimmed from objects and arrays. Otherwise, 
 * the JSON output will be pretty formatted.
 * 
 * @param max_depth is the maximum recursive depth for encoding, default = 1024.
 * @note pretty formatting use 2 spaces instead of tabs.
 * @return string
 */
def encode(value, compact, max_depth) {
  if compact == nil compact = true
  return Encoder(compact, max_depth).encode(value)
}

/**
 * decode(value: string [, allow_comments: boolean = true])
 * 
 * decodes the input JSON string into Blade objects
 * 
 * @param value is the string to decode
 * @param allow_comments can be set to enable/disable C-style comments in json [default = true]
 * @return object
 */
def decode(value, allow_comments) {
  if allow_comments == nil allow_comments = true
  return _decode(value, allow_comments)
}

/**
 * parse(path: string)
 * 
 * parses a file containing json data.
 * @return object
 */
def parse(path) {
  if !is_string(path)
    die Exception('file path expected, ${typeof(path)} given')

  var f = file(path)
  if !f.exists()
    die Exception('could not open file ${path}')

  return decode(f.read())
}

