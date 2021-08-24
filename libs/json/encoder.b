/**
 * class Encoder
 * 
 * JSON encoding class
 */
class Encoder {
  var _max_depth = 1024
  var _depth = 0

  Encoder(max_depth) {
    if max_depth {
      if !is_number(max_depth)
        die Exception('max_depth must be number. ${typeof(max_depth)} given')
      self._max_depth = max_depth
    }
  }

  encode(value) {
    self._depth++

    if self._depth > self._max_depth {
      die Exception('maximum recursive depth of ${self._max_depth} exceeded')
    }

    using typeof(value) {
      when 'nil' return 'null'
      when 'boolean' return to_string(value)
      when 'number' return to_string(value)
      when 'string' return '"${value.replace('"', '\\"')}"'
      when 'list' {
        var result = ''
        for val in value {
          result += ', ${self.encode(val)}'
        }
        if result return '[${result[2,]}]'
        return '[]'
      }
      when 'dictionary' {
        var result = ''
        for key, val in value {
          result += ', "${to_string(key)}": ${self.encode(val)}'
        }
        if result return '{${result[2,]}}'
        return '{}'
      }
      default {
        /* 
        if is_instance(value) && hasprop(value, '@to_json') {
          # @TODO: when native method `call` is implemented, simply call the @to_json()
          # method on the instance.
        } 
        */
        die Exception('object of type ${typeof(value)} is not a JSON serializable')
      }
    }
  }
}