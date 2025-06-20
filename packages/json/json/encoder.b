#!-- part of the json module

import reflect

def _get_string(value) {
  var string_data = '"' + 
    value.replace('\\', '\\\\', false). # replace \
      replace('\f', '\\f', false). # replace \f
      replace('\n', '\\n', false). # replace \n
      replace('\r', '\\r', false). # replace \r
      replace('\t', '\\t', false). # replace \t
      replace('\b', '\\b', false). # replace \b
      replace('"', '\\"', false). # replace " with \"

      # This part is essential to handle JSON data 
      # coming from corrupted sources.
      replace('\0', '\\\\0', false) + # replace NULL characters
    '"' 

  # We want to try to escape non-ASCII characters to their UTF-8 encoding 
  # string
  # 
  # While this may increase the size of the final JSON string, it is more
  # widely supported as not all JSON decoders will decode UTF-8 characters
  # correctly due to different charset supports across different tools.
  catch {
    var unicode_character_matches = string_data.matches('/[^\\x00-\\x7F]/u')
    if unicode_character_matches {
      for match in unicode_character_matches[0] {
        string_data = string_data.replace(match, '\\u' + hex(ord(match)).lpad(4, '0'))
      }
    }
  }

  return string_data
}

/**
 * Blade to JSON encoding class
 */
class Encoder {
  var _max_depth = 1024
  var _depth = 0
  var _item_spacing = ' '
  var _merge_strip_start = 2
  var _is_object = false
  var _is_list = false

  /**
   * @param bool? compact: Default value is `false`.
   * @param number? max_depth: Default value is `1024`.
   * @note Depth starts from zero
   * @note Set max_depth to `0` to disable max depth
   * @constructor
   */
  Encoder(compact, max_depth) {
    if max_depth {
      if !is_number(max_depth)
        raise TypeError('max_depth must be number. ${typeof(max_depth)} given')
      self._max_depth = max_depth
    }

    if compact != nil {
      if !is_bool(compact)
        raise TypeError('compact expects boolean. ${typeof(compact)} given')
      self._item_spacing = compact ? '' : ' '
      self._merge_strip_start = compact ? 1 : 2
    }
    self.compact = compact
  }

  _start_alignment() {
    if !self.compact return '${self._item_spacing}\n${''.lpad(self._depth * 2)}'
    return self._item_spacing
  }

  _end_alignment() {
    if !self.compact return '\n${''.lpad((self._depth - 1) * 2)}'
    return ''
  }

  
  # Encode helper method.
  # @note this function calls the parent encode() method whenever the depth of the encoding increases.
  _encode(value) {
    var spacing = self._item_spacing
    using typeof(value) {
      when 'nil' return 'null'
      when 'boolean' return to_string(value)
      when 'number' return to_string(value)
      when 'bytes' return _get_string(value.to_string())
      when 'string' return _get_string(value)
      when 'list' {
        var result = ''
        self._depth++
        for val in value {
          # inner lists will increase the depth
          #
          # We're wrapping this line in a catch to allow lists encoding
          # to skip values that are unencodable. This let's the result or
          # json.encode to be directly compatible with the original definition
          # of JSON by JavaScript.
          catch {
            result += ',${self._start_alignment()}${self._encode(val)}'
          }
        }
        if result {
          result = '[${result[self._merge_strip_start,]}${self._end_alignment()}]'
        } else result = '[]'
        self._depth--
        return result
      }
      when 'dictionary' {
        var result = ''
        self._depth++
        for key, val in value {
          # inner dictionaries will increase the depth
          #
          # We're wrapping this line in a catch to allow dictionary encoding
          # to skip values that are unencodable. This let's the result or
          # json.encode to be directly compatible with the original definition
          # of JSON by JavaScript.
          catch {
            result += ',${self._start_alignment()}"${to_string(key)}":${spacing}${self._encode(val)}'
          }
        }
        if result {
          result = '{${result[self._merge_strip_start,]}${self._end_alignment()}}'
        } else result = '{}'
        self._depth--
        return result
      }
      default {

        if is_instance(value) {
          if reflect.has_decorator(value, 'to_json')  { # check the @to_json decorator
            return self._encode(reflect.get_decorator(value, 'to_json')())
          }
        }
 
        raise Exception('object of type ${typeof(value)} is not a JSON serializable')
      }
    }
  }

  /**
   * Encodes a value to it's corresponding JSON string.
   * 
   * @param any value
   * @returns string
   */
  encode(value) {

    self._is_list = is_list(value)
    self._is_object = is_dict(value)

    if self._depth > self._max_depth and self._max_depth != 0 {
      raise RangeError('maximum recursive depth of ${self._max_depth} exceeded')
    }

    return self._encode(value)
  }
}
