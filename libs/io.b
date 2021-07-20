/**
 * I/O
 *
 * provides Bladey's interface to I/O stream handling and operations
 * @copyright 2021, Ore Richard Muyiwa 
 */

import _io

 # for file seek
var SEEK_SET = 0
var SEEK_CUR = 1
var SEEK_END = 2


/**
 * an interface to TTY terminals
 * this class contains definitions to control TTY terminals
 */
class TTY {

  # TTY flags
  static var TTY_IFLAG = 0
  static var TTY_OFLAG = 1
  static var TTY_CFLAG = 2
  static var TTY_LFLAG = 3
  static var TTY_ISPEED = 4
  static var TTY_OSPEED = 5

  # input flags for input processing
  static var IGNBRK   = 0x00000001      # ignore BREAK condition 
  static var BRKINT   = 0x00000002      # map BREAK to SIGINTR 
  static var IGNPAR   = 0x00000004      # ignore (discard) parity errors 
  static var PARMRK   = 0x00000008      # mark parity and framing errors 
  static var INPCK    = 0x00000010      # enable checking of parity errors 
  static var ISTRIP   = 0x00000020      # strip 8th bit off chars 
  static var INLCR    = 0x00000040      # map NL into CR 
  static var IGNCR    = 0x00000080      # ignore CR 
  static var ICRNL    = 0x00000100      # map CR to NL (ala CRMOD) 
  static var IXON     = 0x00000200      # enable output flow control 
  static var IXOFF    = 0x00000400      # enable input flow control 
  static var IXANY    = 0x00000800      # any char will restart after stop 
  static var IUTF8    = 0x00004000      # maintain state for UTF-8 VERASE

  # output flags
  static var OPOST    = 0x00000001      # enable following output processing
  static var ONLCR    = 0x00000002      # map NL to CR-NL (ala CRMOD)

  # control flags
  static var CSIZE    = 0x00000300      # character size mask 
  static var CS5      = 0x00000000      # 5 bits (pseudo) 
  static var CS6      = 0x00000100      # 6 bits 
  static var CS7      = 0x00000200      # 7 bits 
  static var CS8      = 0x00000300      # 8 bits 
  static var CSTOPB   = 0x00000400      # send 2 stop bits 
  static var CREAD    = 0x00000800      # enable receiver 
  static var PARENB   = 0x00001000      # parity enable 
  static var PARODD   = 0x00002000      # odd parity, else even 
  static var HUPCL    = 0x00004000      # hang up on last close 
  static var CLOCAL   = 0x00008000      # ignore modem status lines 

  # "Local" flags - dumping ground for other state
  # Warning: some flags in this structure begin with
  # the letter "I" and look like they belong in the
  # input flag.
  static var ECHOE    = 0x00000002      # visually erase chars 
  static var ECHOK    = 0x00000004      # echo NL after line kill 
  static var ECHO     = 0x00000008      # enable echoing 
  static var ECHONL   = 0x00000010      # echo NL even if ECHO is off 
  static var ISIG     = 0x00000080      # enable signals INTR, QUIT, [D]SUSP 
  static var ICANON   = 0x00000100      # canonicalize input lines 
  static var IEXTEN   = 0x00000400      # enable DISCARD and LNEXT 
  static var TOSTOP   = 0x00400000      # stop background jobs from output 
  static var NOFLSH   = 0x80000000      # don't flush after interrupt 

  #-----------------------------------------------------------------------

  # Commands passed to set_attr() for setting the TTY attributes.
  static var TCSANOW    = 0               # make change immediate 
  static var TCSADRAIN  = 1               # drain output, then change 
  static var TCSAFLUSH  = 2               # drain output, flush input 

  # Special Control Characters
  static var VEOF       = 0       # ICANON 
  static var VEOL       = 1       # ICANON 
  static var VERASE     = 3       # ICANON 
  static var VKILL      = 5       # ICANON 
  static var VINTR      = 8       # ISIG 
  static var VQUIT      = 9       # ISIG 
  static var VSUSP      = 10      # ISIG 
  static var VSTART     = 12      # IXON, IXOFF 
  static var VSTOP      = 13      # IXON, IXOFF 
  static var VMIN       = 16      # !ICANON 
  static var VTIME      = 17      # !ICANON

  /**
   * The constructor of the TTY class. 
   */
  TTY(std) {
    if !is_file(std) {
      die Exception('TTY expects a standard file as argument, ${typeof(std)} given')
    }

    self.std = std
  }

  /**
   * get_attr()
   * Returns the attribute of the current tty session
   * The returned a attributes is a dict containing the TTY_ flags 
   */
  get_attr() {
    return _io.TTY.tcgetattr(self.std)
  }

  /**
   * set_attr(option: number, attrs: dict)
   * sets the attributes of the current tty session
   * - option: one ot the TCSA options above (see their description above)
   * - attrs a dictionary of the TTY_ flags listed above
   * 
   * one can safely omit any of the TTY_ flags listed above and
   * Blade will fill in the default values as it exists.
   * - @note this flags will be merged and not overwritten
   */
  set_attr(option, attrs) {
    if !is_int(option) 
      die Exception('integer expected as first argument, ${typeof(option)} given')
    if !is_dict(attrs) 
      die Exception('dictionary expected as second argument, ${typeof(attrs)} given')
    return _io.TTY.tcsetattr(self.std, option, attrs)
  }

  /**
   * set_raw()
   * sets the current tty to raw mode
   */
  set_raw() {
    var new_attr = _io.TTY.tcgetattr(self.std)

    new_attr[TTY.TTY_IFLAG] &= ~(TTY.IGNBRK | TTY.BRKINT | TTY.PARMRK | TTY.ISTRIP | TTY.INLCR | TTY.IGNCR | TTY.ICRNL | TTY.IXON)
    new_attr[TTY.TTY_OFLAG] &= ~TTY.OPOST
    new_attr[TTY.TTY_LFLAG] &= ~(TTY.ECHO | TTY.ECHONL | TTY.ICANON | TTY.ISIG | TTY.IEXTEN)
    new_attr[TTY.TTY_CFLAG] &= ~(TTY.CSIZE | TTY.PARENB)
    new_attr[TTY.TTY_CFLAG] |= TTY.CS8
    
    return self.set_attr(TTY.TCSAFLUSH, new_attr)
  }

  /**
   * exit_raw()
   * disables the raw mode flags on the current tty
   */
  exit_raw() {
    return _io.TTY.exit_raw()
  }

  /**
   * flush()
   * flushes the standard output and standard error interface
   * @return nil
   */
  flush() {
    _io.TTY.flush(self.std);
  }
}

/** 
 * stdin
 * returns an handle to the standard input file of the system
 * 
 * This method is a stub for stdin() method which was declared in
 * native C.
 * @return file<std>
 */
var stdin = _io.stdin

/**
 * stdout
 * returns an handle to the standard output file of the system
 * 
 * This method is a stub for stdout() method which was declared in
 * native C.
 * @return file<std>
 */
var stdout = _io.stdout

/**
 * stderr
 * returns an handle to the standard error file of the system
 * 
 * This method is a stub for stderr() method which was declared in
 * native C.
 * @return file<std>
 */
var stderr = _io.stderr

/**
 * flush(file: file)
 *
 * flushes the content of the given file handle
 * @returns nil
 */
def flush(file) {
  return _io.flush(file)
}

/**
 * putc(c: char)
 * writes character c to the screen
 * @return nil
 */
def putc(c) {
  return _io.putc(c)
}

/**
 * getc()
 *
 * reads character(s) from standard input
 *
 * when length is given, gets `length` number of characters
 * else, gets a single character
 * @returns char | string
 */
def getc() {
  return _io.getc()
}

/**
 * readline()
 *
 * reads an entire line from standard input
 * @returns string
 */
def readline() {
  var result = ''
  var input

  while (input = stdin.read()) and input != '\n' and input != '\0'
    result += input

  return result
}

