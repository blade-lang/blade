#!-- part of the json module

/**
 * class Encoder
 * 
 * Blade to JSON encoding class
 */
class Encoder {
  var _max_depth = 1024
  var _depth = 0
  var _item_spacing = ' '
  var _merge_strip_start = 2

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

  encode(value) {
    self._depth++

    if self._depth > self._max_depth {
      die Exception('maximum recursive depth of ${self._max_depth} exceeded')
    }

    using typeof(value) {
      when 'nil' return 'null'
      when 'boolean' return to_string(value)
      when 'number' return to_string(value)
      when 'string' {
        if value.index_of('"') > -1 return '"${value.replace('"', '\\"')}"'
        return '"${value}"'
      }
      when 'list' {
        var result = ''
        for val in value {
          result += ',' + self._item_spacing + '${self.encode(val)}'
        }
        if result return '[${result[self._merge_strip_start,]}]'
        return '[]'
      }
      when 'dictionary' {
        var result = ''
        for key, val in value {
          result += ', "${to_string(key)}":' + self._item_spacing + '${self.encode(val)}'
        }
        if result return '{${result[self._merge_strip_start,]}}'
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