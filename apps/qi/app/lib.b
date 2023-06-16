import reflect
import colors
import iters
import io


var _before_eachs = []
var _before_alls = []
var _after_eachs = []
var _after_alls = []
var _total_tests = 0
var _total_suites = 0
var _passed_tests = 0
var _failed_tests = 0
var _file

def file() {
  return _file
}

def set_file(f) {
  _file = f
}

var _stats = []
var _curr_desc = {
  it: [],
  file: file(),
  time: 0,
}
var _curr_it = {}

def before_each(fn) {
  _before_eachs.append(fn)
}

def after_each(fn) {
  _after_eachs.append(fn)
}

def before_all(fn) {
  _before_alls.append(fn)
}

def after_all(fn) {
  _after_alls.append(fn)
}

class expect {
  var value
  var _is_not = false

  expect(value) {
    self.value = value
  }

  _run(name, expected, fn) {
    if self._is_not name = 'not ${name}'
    if !fn fn = |x, y| { return x == y }

    var v = to_string(self.value).replace('\r', '\\r').replace('\n', '\\n')
    var w = expected != nil ? to_string(expected).replace('\r', '\\r').replace('\n', '\\n') : nil
    #if v.length() > 20 v = v[,17] + '...'
    #if w.length() > 20 w = v[,17] + '...'

    var state = {
      name: w != nil ? 'expect "${v}" ${name} "${w}"' : 'expect "${v}" ${name}', 
      status: false,
    }
    
    try {
      if !self._is_not and fn(self.value, expected) {
        _passed_tests++
        state.status = true
      } else if self._is_not and !fn(self.value, expected) {
        _passed_tests++
        state.status = true
      } else {
        _failed_tests++
      }
    } catch Exception e {
      _failed_tests++
      io.stderr.write(e.message + '\r\n')
      io.stderr.write(e.stacktrace + '\r\n')
    } finally {
      _curr_it.expects.append(state)
    }
  }

  not() {
    self._is_not = true
    return self
  }

  to_be(e) {
    self._run('to be', e)
    return self
  }

  to_be_nil() {
    self._run('to be nil', nil)
    return self
  }

  to_be_defined() {
    self._run('to be defined', nil, |x, y| { return x != nil })
    return self
  }

  to_be_truthy() {
    self._run('to be truthy', nil, |x, y| { return !!x })
    return self
  }

  to_be_falsy() {
    self._run('to be falsy', nil, |x, y| { return !x })
    return self
  }

  to_be_greater_than(e) {
    self._run('to be greather than', e, |x, y| { 
      if is_string(x) return x.length() > y
      return x > y 
    })
    return self
  }

  to_be_greater_than_or_equal(e) {
    self._run('to be greather than or equal to', e, |x, y| { 
      if is_string(x) return x.length() >= y
      return x >= y 
    })
    return self
  }

  to_be_less_than(e) {
    self._run('to be less than', e, |x, y| { 
      if is_string(x) return x.length() < y
      return x < y 
    })
    return self
  }

  to_be_less_than_or_equal(e) {
    self._run('to be less than or equal to', e, |x, y| { 
      if is_string(x) return x.length() <= y
      return x <= y 
    })
    return self
  }

  to_match(e) {
    self._run('to match', e, |x, y| { return x.match(y) })
    return self
  }

  to_contain(e) {
    self._run('to contain', e, |x, y| {
      if is_dict(x) return x.contains(y)
      return x.count(y) > 0
    })
    return self
  }

  to_throw(e) {
    if !e e = Exception

    if is_function(self.value) {
      self._run('to throw', e, |x, y| {
        try {
          x()
          return false
        } catch Exception ex {
          if is_string(e) return ex.message.match(e)
          if is_class(e) return instance_of(ex, e)
          if instance_of(e, Exception) and ex == e return true
          return true
        }
      })
    } else {
      self._run('to throw', e, @(x, y) { return false })
    }

    return self
  }

  to_have_length(e) {
    self._run('to have length', e, |x, y| { return x.length() == y })
    return self
  }

  to_be_instance_of(e) {
    self._run('to be an instance of', e, |x, y| { return instance_of(x, y) })
    return self
  }

  to_have_property(e, value) {
    var name = 'to have a property'
    if value != nil name = 'to have value "${to_string(value)}" in property'

    self._run(name, e, |x, y| {
      var res = is_instance(x) and reflect.has_prop(x, y)
      if res and value != nil {
        return reflect.get_prop(x, y) == value
      }
      return res
    })

    return self
  }

  to_have_method(e) {
    self._run('to have a method', e, |x, y| {
      return is_instance(x) and reflect.has_method(x, y)
    })
    return self
  }

  to_have_decorator(e) {
    self._run('to have a decorator', e, |x, y| {
      return is_instance(x) and reflect.has_decorator(x, y)
    })
    return self
  }

  to_be_boolean() {
    self._run('to be a boolean', nil, |x, y| { return is_bool(x) })
    return self
  }

  to_be_number() {
    self._run('to be a number', nil, |x, y| { return is_number(x) })
    return self
  }

  to_be_string() {
    self._run('to be a string', nil, |x, y| { return is_string(x) })
    return self
  }

  to_be_list() {
    self._run('to be a list', nil, |x, y| { return is_list(x) })
    return self
  }

  to_be_dict() {
    self._run('to be a dict', nil, |x, y| { return is_dict(x) })
    return self
  }

  to_be_function() {
    self._run('to be a function', nil, |x, y| { return is_function(x) })
    return self
  }

  to_be_class() {
    self._run('to be a class', nil, |x, y| { return is_class(x) })
    return self
  }

  to_be_iterable() {
    self._run('to be an iterable', nil, |x, y| { return is_iterable(x) })
    return self
  }

  to_be_file() {
    self._run('to be a file', nil, |x, y| { return is_file(x) })
    return self
  }

  to_be_bytes() {
    self._run('to be bytes', nil, |x, y| { return is_bytes(x) })
    return self
  }
}

def it(desc, fn) {
  _total_tests++
  var start = microtime()
  for be in _before_eachs {
    be()
  }

  _curr_it = {
    name: desc,
    expects: [],
    time: 0,
  }

  try {
    fn()
  } catch Exception e {
    io.stderr.write(e.message + '\r\n')
    io.stderr.write(e.stacktrace + '\r\n')
  }

  for ae in _after_eachs {
    ae()
  }

  _curr_it.time = microtime() - start

  _curr_desc.it.append(_curr_it)
}

def describe(desc, fn) {
  try {
    _curr_desc = {
      it: [],
      file: file(),
      time: 0,
    }

    var start = microtime()

    for ba in _before_alls {
      ba()
    }

    _curr_desc.name = desc
    fn()

    for aa in _after_alls {
      aa()
    }

    _curr_desc.time = microtime() - start
    _stats.append(_curr_desc)
  } catch Exception e {
    io.stderr.write(e.message + '\r\n')
    io.stderr.write(e.stacktrace + '\r\n')
  }
}

def _get_mark(state) {
  return state ? 
  colors.text('\u2714', colors.text_color.green) :
  colors.text('\u2715', colors.text_color.red)
}

def _print(text, state) {
  return state ? 
  colors.text('\u2714 ' + text, colors.text_color.green) :
  colors.text('\u2715 ' + text, colors.text_color.red)
}

def _gray(txt) {
  return colors.text(txt, colors.text_color.dark_grey)
}

def _report(text, state) {
  return state ? 
  colors.text(text, colors.text_color.green) :
  colors.text(text, colors.text_color.red)
}

def _time(time) {
  if time < 1000 {
    return time + 'µs'
  } else if time < 1000000 {
    return (time / 1000) + 'ms'
  }

  return (time / 1000000) + 's'
}

def run(f) {
  set_file(f)
  reflect.run_script(f)
}

def show_tests_results() {
  echo ''

  var failed_suites = 0
  var failed_tests = 0
  var total_time = 0

  iter var index = 0; index < _stats.length(); index++ {
    var e = _stats[index]

    total_time += e.time

    var fails = iters.filter(e.it, |x| {
      return iters.filter(x.expects, |y|{ return !y.status }).length() > 0
    }).length() > 0
    if fails failed_suites++

    echo colors.text(
      !fails ? ' PASS ' : ' FAIL ', 
      fails ? 
        colors.background.red :
        colors.background.green
    ) + ' ' + e.file

    echo '  ${e.name}'
    iter var i = 0; i < e.it.length(); i++ {
      var _e = e.it[i]
      var it_fails = iters.filter(_e.expects, |x|{ return !x.status }).length() > 0
      if it_fails failed_tests++

      echo '    ' + _print('${_e.name} (${_time(_e.time)})', !it_fails)

      iter var j = 0; j < _e.expects.length(); j++ {
        var expect = _e.expects[j]
        echo '      ${_get_mark(expect.status)} ${_gray(expect.name)}'
      }
    }
    echo ''
  }

  var passed_suites = _report((_stats.length() - failed_suites) + ' passed', true)
  var passed_tests = _report((_total_tests - failed_tests) + ' passed', true)
  var passes = _passed_tests > 0 ? _report(_passed_tests + ' passed', true) : '0 passed'
  var suite_fails = failed_suites > 0 ? _report(failed_suites + ' failed', false) : '0 failed'
  var test_fails = failed_tests > 0 ? _report(failed_tests + ' failed', false) : '0 failed'
  var assert_fails = _failed_tests > 0 ? _report(_failed_tests + ' failed', false) : '0 failed'

  echo colors.text('Test suites:  ${passed_suites}, ${suite_fails}, ${_stats.length()} total', colors.style.bold)
  echo colors.text('Tests:        ${passed_tests}, ${test_fails}, ${_total_tests} total', colors.style.bold)
  echo colors.text('Assertions:   ${passes}, ${assert_fails}, ${_passed_tests + _failed_tests} total', colors.style.bold)
  echo colors.text('Time:         ${_time(total_time)}', colors.style.bold)
  echo colors.text(_gray('Ran all test suites.'), colors.style.bold)

  return _failed_tests == 0
}
