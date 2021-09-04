class Mandlebrot {
  var BUFFER_SIZE = 8192

  Mandlebrot(size) {
    self.size = size
    self.fac = 2.0 / size

    if size % 8 == 0 self.shift = 0
    else self.shift = 8 - size % 8

    self.buf = '' * BUFFER_SIZE
  }

  compute() {
    echo 'P4\n' + self.size + ' ' + self.size
  }
}
