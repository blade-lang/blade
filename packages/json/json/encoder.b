#!-- part of the json module

import reflect

def _get_string(value) {
  return '"' + value.replace('"', '\"', false).  # replace " with \"
#     replace('/', '\/', false). # replace /
    replace('\b', '\\b', false). # replace \b
    replace('\f', '\\f', false). # replace \f
    replace('\n', '\\n', false). # replace \n
    replace('\r', '\\r', false). # replace \r
    replace('\t', '\\t', false). # replace \t
    replace('/\\(?!\\)/', '\\\\', false) + '"' # replace \
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
   * Encoder([compact: boolean = false, [max_depth: number = 1024]])
   * @constructor
   * @note that depth starts from zero
   * @note set max_depth to `0` to disable max depth
   */
  Encoder(compact, max_depth) {
    if max_depth {
      if !is_number(max_depth)
        die Exception('max_depth must be number. ${typeof(max_depth)} given')
      self._max_depth = max_depth
    }

    if compact != nil {
      if !is_bool(compact)
        die Exception('compact expects boolean. ${typeof(compact)} given')
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

  /**
   * _encode(value: any)
   *
   * encode helper method.
   * @note this function calls the parent encode() method whenever the depth of the encoding increases
   */
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
            result += ',${self._start_alignment()}${self._encode(val)}'
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
          result += ',${self._start_alignment()}"${to_string(key)}":${spacing}${self._encode(val)}'
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
 
        die Exception('object of type ${typeof(value)} is not a JSON serializable')
      }
    }
  }

  /**
   * encode(value: any)
   *
   * main encode method
   */
  encode(value) {

    self._is_list = is_list(value)
    self._is_object = is_dict(value)

    if self._depth > self._max_depth and self._max_depth != 0 {
      die Exception('maximum recursive depth of ${self._max_depth} exceeded')
    }

    return self._encode(value)
  }
}
