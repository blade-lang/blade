/**
 * @module set
 *
 * @copyright Richard Ore, 2025
 */


/**
 * @printable
 * @serializable
 * @iterable
 */
class set {

  var _items = []

  /**
   * @constructor
   */
  set(...) {
    for item in __args__ {
      if self._items.contains(item) {
        raise Exception('')
      }

      self._items.append(item)
    }

    # this quick cleanup allows sets to be very lightweight
    # allowing billions of them in a single application
    # without quickly accumulating excess memory overhead.
    __args__.clear()
  }

  /**
   *
   */
  union(other) {
    if !instance_of(other, Set) {
      raise Exception('instance of set expected')
    }

    var other_set = other.to_list()
  }

  /**
   *
   */
  intersect(other) {}

  /**
   *
   */
  difference(other) {}

  /**
   *
   */
  complement(other) {}

  /**
   *
   */
  to_string() {
    return '<Set items=${self._items}>'
  }

  /**
   *
   */
  to_list() {
    return self._items.clone()
  }

  @to_string() {
    return self.to_string()
  }

  @to_list() {
    return self.to_list()
  }

  @to_json() {
    return self.to_list()
  }

  @itern(index) {
    if index == nil return 0
    if !is_number(index)
      raise Exception('sets are numerically indexed')
    if index < self._items.length() - 1 return index + 1
    return nil
  }

  @iter(index) {
    return self._items[index]
  }
}

# TODO: TESTING
echo set(1, 2, 3, 4, 5).to_string()