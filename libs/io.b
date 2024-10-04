/**
 * @module io
 * 
 * This module provides interfaces for working with to I/O stream and TTYs 
 * as well as expose the operating system standard I/O for easy access.
 * 
 * Some I/O operations that should belong to this module have been merged as 
 * core features and offered as built-in functions for Blade. Specifically 
 * file I/O features that can be accessed via the built-in `file()` function. 
 * 
 * The standard I/O streams are also files and you can call almost all file 
 * methods on them. Whenever a file method is not supported, you'll get an error 
 * message telling you that such operation is not supported for standard streams.
 * 
 * ### Example
 * 
 * The following example shows how to use the `io` module for accepting user name 
 * and printing the result.
 * 
 * ```blade
 * import io
 * 
 * var name = io.readline('What is your name?')
 * echo name
 * ```
 * 
 * @copyright 2021, Ore Richard Muyiwa and Blade contributors
 */

import _io
import _os

/**
 * Set I/O position from the beginning.
 * 
 * @type int
 */
var SEEK_SET = 0

/**
 * Set I/O position from the current position.
 * 
 * @type int
 */
var SEEK_CUR = 1

/**
 * Set I/O position from the end.
 * 
 * @type int
 */
var SEEK_END = 2


/**
 * class TTY is an interface to TTY terminals this class contains definitions 
 * to control TTY terminals
 */
class TTY {

  # TTY flags

  /**
   * TTY attribute for input flags.
   * 
   * @type int
   * @static
   */
  static var TTY_IFLAG = 0

  /**
   * TTY attribute for output flags.
   * 
   * @type int
   * @static
   */
  static var TTY_OFLAG = 1

  /**
   * TTY attribute for control flags.
   * 
   * @type int
   * @static
   */
  static var TTY_CFLAG = 2

  /**
   * TTY attribute for local flags.
   * 
   * @type int
   * @static
   */
  static var TTY_LFLAG = 3

  /**
   * TTY attribute for input speed.
   * 
   * @type int
   * @static
   */
  static var TTY_ISPEED = 4

  /**
   * TTY attribute for output speed.
   * 
   * @type int
   * @static
   */
  static var TTY_OSPEED = 5

  # input flags for input processing

  /**
   * Ignore BREAK condition.
   * 
   * @type int
   * @static
   */
  static var IGNBRK   = 0x00000001
  
  /**
   * Map BREAK to SIGINTR.
   * 
   * @type int
   * @static
   */
  static var BRKINT   = 0x00000002      

  /**
   * Ignore (discard) parity errors.
   * 
   * @type int
   * @static
   */
  static var IGNPAR   = 0x00000004      

  /**
   * Mark parity and framing errors.
   * 
   * @type int
   * @static
   */
  static var PARMRK   = 0x00000008      

  /**
   * Enable checking of parity errors.
   * 
   * @type int
   * @static
   */
  static var INPCK    = 0x00000010

  /**
   * Strip 8th bit off chars.
   * 
   * @type int
   * @static
   */
  static var ISTRIP   = 0x00000020

  /**
   * Map NL into CR.
   * 
   * @type int
   * @static
   */
  static var INLCR    = 0x00000040

  /**
   * Ignore CR.
   * 
   * @type int
   * @static
   */
  static var IGNCR    = 0x00000080

  /**
   * Map CR to NL (ala CRMOD).
   * 
   * @type int
   * @static
   */
  static var ICRNL    = 0x00000100

  /**
   * Enable output flow control.
   * 
   * @type int
   * @static
   */
  static var IXON     = 0x00000200

  /**
   * Enable input flow control.
   * 
   * @type int
   * @static
   */
  static var IXOFF    = 0x00000400

  /**
   * Any char will restart after stop.
   * 
   * @type int
   * @static
   */
  static var IXANY    = 0x00000800

  /**
   * Maintain state for UTF-8 VERASE.
   * 
   * @type int
   * @static
   */
  static var IUTF8    = 0x00004000

  # output flags

  /**
   * Enable following output processing.
   * 
   * @type int
   * @static
   */
  static var OPOST    = 0x00000001

  /**
   * Map NL to CR-NL (ala CRMOD).
   * 
   * @type int
   * @static
   */
  static var ONLCR    = 0x00000002

  # control flags

  /**
   * Character size mask .
   * 
   * @type int
   * @static
   */
  static var CSIZE    = 0x00000300

  /**
   * 5 bits (pseudo).
   * 
   * @type int
   * @static
   */
  static var CS5      = 0x00000000

  /**
   * 6 bits.
   * 
   * @type int
   * @static
   */
  static var CS6      = 0x00000100

  /**
   * 7 bits.
   * 
   * @type int
   * @static
   */
  static var CS7      = 0x00000200

  /**
   * 8 bits.
   * 
   * @type int
   * @static
   */
  static var CS8      = 0x00000300

  /**
   * Send 2 stop bits.
   * 
   * @type int
   * @static
   */
  static var CSTOPB   = 0x00000400

  /**
   * Enable receiver.
   * 
   * @type int
   * @static
   */
  static var CREAD    = 0x00000800

  /**
   * Parity enable.
   * 
   * @type int
   * @static
   */
  static var PARENB   = 0x00001000

  /**
   * Odd parity, else even.
   * 
   * @type int
   * @static
   */
  static var PARODD   = 0x00002000

  /**
   * Hang up on last close.
   * 
   * @type int
   * @static
   */
  static var HUPCL    = 0x00004000

  /**
   * Ignore modem status lines.
   * 
   * @type int
   * @static
   */
  static var CLOCAL   = 0x00008000

  # "Local" flags - dumping ground for other state
  # Warning: some flags in this structure begin with
  # the letter "I" and look like they belong in the
  # input flag.

  /**
   * Visually erase chars.
   * 
   * @type int
   * @static
   */
  static var ECHOE    = 0x00000002

  /**
   * Echo NL after line kill 
   * @static
   */
  static var ECHOK    = 0x00000004

  /**
   * Enable echoing.
   * 
   * @type int
   * @static
   */
  static var ECHO     = 0x00000008

  /**
   * Echo NL even if ECHO is off.
   * 
   * @type int
   * @static
   */
  static var ECHONL   = 0x00000010

  /**
   * Enable signals INTR, QUIT, [D]SUSP.
   * 
   * @type int
   * @static
   */
  static var ISIG     = 0x00000080

  /**
   * Canonicalize input lines.
   * 
   * @type int
   * @static
   */
  static var ICANON   = 0x00000100

  /**
   * Enable DISCARD and LNEXT.
   * 
   * @type int
   * @static
   */
  static var IEXTEN   = 0x00000400

  /**
   * Stop background jobs from output.
   * 
   * @type int
   * @static
   */
  static var TOSTOP   = 0x00400000

  /**
   * Don't flush after interrupt.
   * 
   * @type int
   * @static
   */
  static var NOFLSH   = 0x80000000

  #-----------------------------------------------------------------------

  # Commands passed to set_attr() for setting the TTY attributes.

  /**
   * Make change immediate.
   * 
   * @type int
   * @static
   */
  static var TCSANOW    = 0

  /**
   * Drain output, then change.
   * 
   * @type int
   * @static
   */
  static var TCSADRAIN  = 1

  /**
   * Drain output, flush input.
   * 
   * @type int
   * @static
   */
  static var TCSAFLUSH  = 2

  # Special Control Characters

  /**
   * ICANON.
   * 
   * @type int
   * @static
   */
  static var VEOF       = 0

  /**
   * ICANON.
   * 
   * @type int
   * @static
   */
  static var VEOL       = 1

  /**
   * ICANON.
   * 
   * @type int
   * @static
   */
  static var VERASE     = 3

  /**
   * ICANON.
   * 
   * @type int
   * @static
   */
  static var VKILL      = 5

  /**
   * ISIG.
   * 
   * @type int
   * @static
   */
  static var VINTR      = 8

  /**
   * ISIG.
   * 
   * @type int
   * @static
   */
  static var VQUIT      = 9

  /**
   * ISIG.
   * 
   * @type int
   * @static
   */
  static var VSUSP      = 10

  /**
   * IXON, IXOFF.
   * 
   * @type int
   * @static
   */
  static var VSTART     = 12

  /**
   * IXON, IXOFF.
   * 
   * @type int
   * @static
   */
  static var VSTOP      = 13

  /**
   * !ICANON.
   * 
   * @type int
   * @static
   */
  static var VMIN       = 16

  /**
   * !ICANON.
   * 
   * @type int
   * @static
   */
  static var VTIME      = 17

  /**
   * TTY(std: file)
   * 
   * @note _file_ must be one of stdout and stderr
   * @param file std
   * @constructor 
   */
  TTY(std) {
    if !is_file(std) {
      raise Exception('TTY expects a standard file as argument, ${typeof(std)} given')
    }

    self.std = std
  }

  /**
   * Returns the attribute of the current tty session
   * The returned attributes is a dict containing the TTY_ flags
   * 
   * @returns dict
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
   * 
   * @note This flags will be merged and not overwritten
   * @param number option
   * @param dict attr
   * @returns bool
   */
  set_attr(option, attrs) {
    if !is_int(option) 
      raise Exception('integer expected as first argument, ${typeof(option)} given')
    if !is_dict(attrs) 
      raise Exception('dictionary expected as second argument, ${typeof(attrs)} given')
    return _io.TTY.tcsetattr(self.std, option, attrs)
  }

  /**
   * Sets the current tty to raw mode.
   * 
   * @returns bool
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
   * Disables the raw mode flags on the current tty.
   * 
   * @returns bool
   */
  exit_raw() {
    _io.TTY.exit_raw()
  }

  /**
   * Flushes the standard output and standard error interface
   */
  flush() {
    _io.TTY.flush(self.std);
  }
}

/** 
 * Stdin is a file handle to the standard input file of the system.
 * @type file
 */
var stdin = _io.stdin

/**
 * Stdout is a file handle to the standard output file of the system.
 * @type file
 */
var stdout = _io.stdout

/**
 * Stderr is a file handle to the standard error file of the system.
 * @type file
 */
var stderr = _io.stderr

/**
 * Flushes the content of the given file handle
 */
def flush(file) {
  _io.flush(file)
}

/**
 * Writes character c to the screen.
 * 
 * @param char|number c
 */
def putc(c) {
  _io.putc(c)
}

/**
 * Reads character(s) from standard input
 *
 * When length is given, gets `length` number of characters
 * else, gets a single character
 * 
 * @returns char|string
 */
def getc() {
  return _io.getc()
}

/**
 * Reads character(s) from standard input without printing to standard output
 *
 * When length is given, gets `length` number of characters
 * else, gets a single character.
 * 
 * @returns char|string
 */
def getch() {
  return _io.getch()
}

/**
 * Reads an entire line from standard input. If a _messagge_ is given, the 
 * message will be printed before it begins to wait for a user input. If 
 * _secure_ is `true`, the user's input will not be printing and _obscure_text_ 
 * will be printed instead.
 * 
 * @note Newlines will not be added automatically for messages.
 * @param string? message
 * @param bool? secure
 * @param string? obscure_text: Default value is `*`.
 * @returns string
 */
def readline(message, secure, obscure_text) {

  if message != nil and !is_string(message)
    raise Exception('string expected in argument 1 (message)')
  if secure != nil and !is_bool(secure)
    raise Exception('boolean expected in argument 2 (secure)')
  if obscure_text != nil and !is_string(obscure_text)
    raise Exception('string expected in argument 3 (obscure_text)')

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

