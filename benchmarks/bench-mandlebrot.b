import struct
import io { stdout }

def mandlebrot(h, w) {
  if !w w = h

  echo 'P4\n${w} ${h}'

  var bit_num = 128, bytes_acc = 0,
      yfac = 2/h, xfac = 2/w

  for y in 0..h {
    var result = []
    var ci = y * yfac - 1

    for x in 0..w {
      var zr = 0, zi = 0, tr = 0, ti = 0
      var cr = x * xfac - 1.5

      var should_break = false
      do {
        for i in 0..50 {
          zi = 2 * zr * zi + ci
          zr = tr - ti + cr
          tr = zr * zr
          if tr + (ti = zi * zi) > 4 {
            should_break = true
            break
          }
        }
        if should_break break
        bytes_acc += bit_num
      } while false

      if bit_num == 1 {
        result.append(bytes_acc)
        bit_num = 128
        bytes_acc = 0
      } else {
        bit_num >>= 1
      }
    }

    if bit_num != 128 {
      result.append(bytes_acc)
      bit_num = 128
      bytes_acc = 0
    }
    
    stdout.write(struct.pack('c*', result).to_string().ascii())
  }
}

var start = microtime()
mandlebrot(16000)
echo 'Total time taken = ${(microtime() - start) / 1000000}'