# io.BytesIO implementation


class BytesIO {
  # trackers
  var _open = true
  var _number = 0
  var _max_length = 0
  var _position = 0

  BytesIO(source, mode) {
    if !mode mode = 'r'

    if !is_bytes(source) {
      raise Exception('BytesIO must take source from a bytearray')
    } else if !is_string(mode) {
      raise Exception('invalid I/O mode')
    }

    self.source = source
    self._mode = mode
    self._number = rand(0, 65535)
    self._max_length = self.source.length()

    self._ctime = time()
    self._atime = 0
    self._mtime = 0
  }

  exists() {
    return true
  }

  close() {
    self._do_close()
  }

  open() {
    return self._do_open()
  }

  read(length) {
    if !length {
      length = self._max_length - self._position
    }

    if !is_number(length) {
      raise Exception('length must be a number')
    }

    if !self._open {
      self._do_open()
    }

    var result = self._read(length)
    self._do_close()
    return result
  }

  gets(length) {
    if !length {
      length = self._max_length - self._position
    }

    if !is_number(length) {
      raise Exception('length must be a number')
    }

    return self._read(length)
  }

  write(data) {
    if !is_bytes(data) and !is_string(data) {
      raise Exception('string or bytes expected')
    }

    if is_string(data) data = data.to_bytes()

    if !self._open {
      self._do_open()
    }

    return self._write(data)
  }

  puts(data) {
    if !is_bytes(data) and !is_string(data) {
      raise Exception('string or bytes expected')
    }

    if is_string(data) data = data.to_bytes()

    return self._write(data)
  }

  number() {
    return self._number
  }

  is_tty() {
    return false
  }

  is_open() {
    return self._open
  }

  is_closed() {
    return !self._open
  }

  flush() {
    # do nothing...
  }

  stats() {
    return {
      is_readable: self._can_read(),
      is_writable: self._can_write(),
      is_executable: self._can_read() and !self._can_write(),
      is_symbolic: false,
      size: self.source.length(),
      mode: 0,
      dev: 0,
      ino: 0,
      nlink: 0,
      uid: 0,
      gid: 0,
      mtime: self._mtime,
      atime: self._atime,
      ctime: self._ctime,
      blocks: max(self.source.length() // 4096, 1),
      blksize: 4096,
    }
  }

  copy() {
    return BytesIO(self.source, self._mode)
  }

  truncate(length) {
    if !length length = 0
    if !is_number(length) {
      raise Exception('invalid I/O data truncation length')
    }

    self._max_length = length
  }

  seek(position, seek_type) {
    if !is_number(position) {
      raise Exception('invalid I/O seek position')
    }

    if seek_type == nil seek_type = 1
    if !is_number(seek_type) or seek_type < 0 or seek_type > 2 {
      raise Exception('invalid seek type')
    }

    using seek_type {
      when 0 self._position = position
      when 1 self._position += position
      when 2 self._position = self.source.length() - position
    }
  }

  tell() {
    return self._position
  }

  mode() {
    return self._mode
  }

  _can_read() {
    return self._open and to_bool(self._mode.index_of('r') or self._mode.index_of('+') or self._mode.index_of('a'))
  }

  _can_write() {
    return self._open and to_bool(self._mode.index_of('w') or self._mode.index_of('+') or self._mode.index_of('a'))
  }

  _do_open() {
    self._open = true
    self._max_length = self.source.length()
    self._position = 0
  }

  _do_close() {
    self._open = false
    self._max_length = 0
    self._position = self.source.length() + 1
  }

  _read(length) {
    if !self._can_read() {
      raise Exception('I/O not open for reading')
    }

    self._atime = time()

    var max_readable = self._max_length - self._position
    if max_readable < 1 {
      return bytes(0)
    }

    var result
    if length >= max_readable {
      result = self.source[self._position,]
    } else {
      result = self.source[self._position, self._position + length]
    }

    self._position += length
    return result
  }

  _write(data) {
    if !self._can_write() {
      raise Exception('I/O not open for writing')
    }

    if self._max_length - self._position < 0 {
      raise Exception('cannot write beyond I/O offsets')
    }

    self.source += data

    self._max_length += data.length()
    self._mtime = time()

    return data.length()
  }
}
