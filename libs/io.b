#
# @module io
# 
# This module provides interfaces for working with to I/O stream and TTYs 
# as well as expose the operating system standard I/O for easy access.
# 
# Some I/O operations that should belong to this module have been merged as 
# core features and offered as built-in functions for Blade. Specifically 
# file I/O features that can be accessed via the built-in `file()` function. 
# 
# The standard I/O streams are also files and you can call almost all file 
# methods on them. Whenever a file method is not supported, you'll get an error 
# message telling you that such operation is not supported for standard streams.
# 
# ### Example
# 
# The following example shows how to use the `io` module for accepting user name 
# and printing the result.
# 
# ```blade
# import io
# 
# var name = io.readline('What is your name?')
# echo name
# ```
# 
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _io
import _os

/**
 * Set I/O position from the beginning
 */
var SEEK_SET = 0

/**
 * Set I/O position from the current position
 */
var SEEK_CUR = 1

/**
 * Set I/O position from the end
 */
var SEEK_END = 2


/**
 * class TTY is an interface to TTY terminals this class contains definitions 
 * to control TTY terminals
 */
class TTY {

  # TTY flags
  /**
   * TTY attribute for input flags
   * @static
   */
  static var TTY_IFLAG = 0

  /**
   * TTY attribute for output flags
   * @static
   */
  static var TTY_OFLAG = 1

  /**
   * TTY attribute for control flags
   * @static
   */
  static var TTY_CFLAG = 2

  /**
   * TTY attribute for local flags
   * @static
   */
  static var TTY_LFLAG = 3

  /**
   * TTY attribute for input speed
   * @static
   */
  static var TTY_ISPEED = 4

  /**
   * TTY attribute for output speed
   * @static
   */
  static var TTY_OSPEED = 5

  # input flags for input processing

  /**
   * ignore BREAK condition
   * @static
   */
  static var IGNBRK   = 0x00000001
  
  /**
   * map BREAK to SIGINTR 
   * @static
   */
  static var BRKINT   = 0x00000002      

  /**
   * ignore (discard) parity errors
   * @static
   */
  static var IGNPAR   = 0x00000004      

  /**
   * mark parity and framing errors 
   * @static
   */
  static var PARMRK   = 0x00000008      

  /**
   * enable checking of parity errors
   * @static
   */
  static var INPCK    = 0x00000010

  /**
   * strip 8th bit off chars 
   * @static
   */
  static var ISTRIP   = 0x00000020

  /**
   * map NL into CR
   * @static
   */
  static var INLCR    = 0x00000040

  /**
   * ignore CR 
   * @static
   */
  static var IGNCR    = 0x00000080

  /**
   * map CR to NL (ala CRMOD)
   * @static
   */
  static var ICRNL    = 0x00000100

  /**
   * enable output flow control 
   * @static
   */
  static var IXON     = 0x00000200

  /**
   * enable input flow control
   * @static
   */
  static var IXOFF    = 0x00000400

  /**
   * any char will restart after stop 
   * @static
   */
  static var IXANY    = 0x00000800

  /**
   * maintain state for UTF-8 VERASE
   * @static
   */
  static var IUTF8    = 0x00004000

  # output flags

  /**
   * enable following output processing
   * @static
   */
  static var OPOST    = 0x00000001

  /**
   * map NL to CR-NL (ala CRMOD)
   * @static
   */
  static var ONLCR    = 0x00000002

  # control flags

  /**
   * character size mask 
   * @static
   */
  static var CSIZE    = 0x00000300

  /**
   * 5 bits (pseudo)
   * @static
   */
  static var CS5      = 0x00000000

  /**
   * 6 bits 
   * @static
   */
  static var CS6      = 0x00000100

  /**
   * 7 bits 
   * @static
   */
  static var CS7      = 0x00000200

  /**
   * 8 bits
   * @static
   */
  static var CS8      = 0x00000300

  /**
   * send 2 stop bits 
   * @static
   */
  static var CSTOPB   = 0x00000400

  /**
   * enable receiver 
   * @static
   */
  static var CREAD    = 0x00000800

  /**
   * parity enable 
   * @static
   */
  static var PARENB   = 0x00001000

  /**
   * odd parity, else even 
   * @static
   */
  static var PARODD   = 0x00002000

  /**
   * hang up on last close 
   * @static
   */
  static var HUPCL    = 0x00004000

  /**
   * ignore modem status lines 
   * @static
   */
  static var CLOCAL   = 0x00008000

  # "Local" flags - dumping ground for other state
  # Warning: some flags in this structure begin with
  # the letter "I" and look like they belong in the
  # input flag.

  /**
   * visually erase chars 
   * @static
   */
  static var ECHOE    = 0x00000002

  /**
   * echo NL after line kill 
   * @static
   */
  static var ECHOK    = 0x00000004

  /**
   * enable echoing 
   * @static
   */
  static var ECHO     = 0x00000008

  /**
   * echo NL even if ECHO is off 
   * @static
   */
  static var ECHONL   = 0x00000010

  /**
   * enable signals INTR, QUIT, [D]SUSP 
   * @static
   */
  static var ISIG     = 0x00000080

  /**
   * canonicalize input lines 
   * @static
   */
  static var ICANON   = 0x00000100

  /**
   * enable DISCARD and LNEXT 
   * @static
   */
  static var IEXTEN   = 0x00000400

  /**
   * stop background jobs from output 
   * @static
   */
  static var TOSTOP   = 0x00400000

  /**
   * don't flush after interrupt 
   * @static
   */
  static var NOFLSH   = 0x80000000

  #-----------------------------------------------------------------------

  # Commands passed to set_attr() for setting the TTY attributes.

  /**
   * make change immediate 
   * @static
   */
  static var TCSANOW    = 0

  /**
   * drain output, then change 
   * @static
   */
  static var TCSADRAIN  = 1

  /**
   * drain output, flush input 
   * @static
   */
  static var TCSAFLUSH  = 2

  # Special Control Characters

  /**
   * ICANON
   * @static
   */
  static var VEOF       = 0

  /**
   * ICANON
   * @static
   */
  static var VEOL       = 1

  /**
   * ICANON
   * @static
   */
  static var VERASE     = 3

  /**
   * ICANON
   * @static
   */
  static var VKILL      = 5

  /**
   * ISIG
   * @static
   */
  static var VINTR      = 8

  /**
   * ISIG
   * @static
   */
  static var VQUIT      = 9

  /**
   * ISIG
   * @static
   */
  static var VSUSP      = 10

  /**
   * IXON, IXOFF 
   * @static
   */
  static var VSTART     = 12

  /**
   * IXON, IXOFF 
   * @static
   */
  static var VSTOP      = 13

  /**
   * !ICANON 
   * @static
   */
  static var VMIN       = 16

  /**
   * !ICANON
   * @static
   */
  static var VTIME      = 17

  /**
   * TTY(std: file)
   * @constructor 
   * @note file must be one of stdout and stderr
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
   * 
   * sets the attributes of the current tty session
   * 
   * - option: one ot the TCSA options above (see their description above)
   * - attrs a dictionary of the TTY_ flags listed above
   * - one can safely omit any of the TTY_ flags listed above and Blade will fill in the default values as it exists.
   * @note this flags will be merged and not overwritten
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
   * 
   * sets the current tty to raw mode
   * @return bool
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
   * @return bool
   */
  exit_raw() {
    _io.TTY.exit_raw()
  }

  /**
   * flush()
   * flushes the standard output and standard error interface
   */
  flush() {
    _io.TTY.flush(self.std);
  }
}

/** 
 * stdin is a file handle to the standard input file of the system
 */
var stdin = _io.stdin

/**
 * stdout is a file handle to the standard output file of the system
 */
var stdout = _io.stdout

/**
 * stderr is a file handle to the standard error file of the system
 */
var stderr = _io.stderr

/**
 * flush(file: file)
 *
 * flushes the content of the given file handle
 */
def flush(file) {
  _io.flush(file)
}

/**
 * putc(c: char | number)
 * writes character c to the screen
 * @return nil
 */
def putc(c) {
  _io.putc(c)
}

/**
 * getc()
 *
 * reads character(s) from standard input
 *
 * when length is given, gets `length` number of characters
 * else, gets a single character
 * @return char | string
 */
def getc() {
  return _io.getc()
}

/**
 * getch()
 *
 * reads character(s) from standard input without printing to standard output
 *
 * when length is given, gets `length` number of characters
 * else, gets a single character
 * @return char | string
 */
def getch() {
  return _io.getch()
}

/**
 * readline([message: string [, secure: bool = false [, obscure_text = '*']]])
 *
 * reads an entire line from standard input. If a _messagge_ is given, the 
 * message will be printed before it begins to wait for a user input. If 
 * _secure_ is `true`, the user's input will not be printing and _obscure_text_ 
 * will be printed instead.
 * 
 * @note newlines will not be added automatically for messages.
 * @return string
 */
def readline(message, secure, obscure_text) {

  if message != nil and !is_string(message)
    die Exception('string expected in argument 1 (message)')
  if secure != nil and !is_bool(secure)
    die Exception('boolean expected in argument 2 (secure)')
  if obscure_text != nil and !is_string(obscure_text)
    die Exception('string expected in argument 3 (obscure_text)')

  if secure == nil secure = false
  if obscure_text == nil obscure_text = '*'

  if message
    stdout.write('${message} ')

  var result = ''
  var input

  if !secure {
    while (input = stdin.read()) and input != '\n' and input != '\0'
      result += input
  } else {
    while (input = getch()) and input != '\n' and input != '\r' and input != '\0' {
    if ord(input) != 0x7f and input != '\b' {
        result += input
        stdout.write(obscure_text)
      } else {
        if result.length() > 0
          stdout.write('\b \b')
        result = result[,result.length() - 1]
      }
    }
  }

  return result
}

