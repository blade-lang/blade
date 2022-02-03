#!-- part of the json module

import reflect

/**
 * @class Encoder
 * 
 * Blade to JSON encoding class
 */
class Encoder {
  var _max_depth = 1024
  var _depth = 0
  var _item_spacing = ' '
  var _merge_strip_start = 2

  /**
   * @constructor Encoder
   * 
   * Encoder([compact: boolean = false, [max_depth: number = 1024]])
   * @note that depth starts from zero
   */
  Encoder(compact, max_depth) {
    if max_depth {
      if !is_number(max_depth)
        die Exception('max_depth must be number. ${typeof(max_depth)} given')
      self._max_depth = max_depth
    }

    if compact {
      if !is_bool(compact) die Exception('compact expects boolean. ${typeof(compact)} given')
      self._item_spacing = ''
      self._merge_strip_start = 1
    }
  }

  /**
   * _encode(value: any)
   * 
   * encode helper method.
   * @note this function calls the parent encode() method whenever
   * the depth of the encoding increases
   */
  _encode(value) {
    using typeof(value) {
      when 'nil' return 'null'
      when 'boolean' return to_string(value)
      when 'number' return to_string(value)
      when 'string' {
        if value.index_of('"') > -1 or value.index_of('\\') > -1 {
          var esc_value = value.replace('/"/', '\\"').  # replace " with \"
              replace('/\\\\/', '\\\\')   # replace \ with \\
          return '"${esc_value}"'
        }
        return '"${value}"'
      }
      when 'list' {
        var result = ''
        for val in value {
          # inner lists will increase the depth
          if is_list(val) or is_dict(val) {
            result += ',${self._item_spacing}${self.encode(val)}'
          } else {
            result += ',${self._item_spacing}${self._encode(val)}'
          }
        }
        if result return '[${result[self._merge_strip_start,]}]'
        return '[]'
      }
      when 'dictionary' {
        var result = ''
        for key, val in value {
          # inner dictionaries will increase the depth
          if is_dict(val) or is_list(val) {
            result += ',${self._item_spacing}"${to_string(key)}":${self._item_spacing}${self.encode(val)}'
          } else {
            result += ',${self._item_spacing}"${to_string(key)}":${self._item_spacing}${self._encode(val)}'
          }
        }
        if result return '{${result[self._merge_strip_start,]}}'
        return '{}'
      }
      default {
        
        if is_instance(value) {
          if reflect.has_method(value, '@to_json')  { # get the @to_json decorator
            return self.encode(reflect.call_method(value, '@to_json'))
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

    if self._depth > self._max_depth {
      die Exception('maximum recursive depth of ${self._max_depth} exceeded')
    }
    
    # depth increment is done here so that we only
    # increment depths when needed
    self._depth++

    return self._encode(value)
  }
}
