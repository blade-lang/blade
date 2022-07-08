# 
# @module zlib
# 
#   The 'zlib' compression library provides in-memory compression and
# uncompression functions, including integrity checks of the uncompressed data.
# This version of the library supports only one compression method (deflation)
# but other algorithms will be added later and will have the same stream
# interface.
# 
#   Compression can be done in a single step if the buffers are large enough,
# or can be done by repeated calls of the compression function.  In the latter
# case, the application must provide more input and/or consume the output
# (providing more output space) before each call.
# 
#   The compressed data format used by default by the in-memory functions is
# the zlib format, which is a zlib wrapper documented in RFC 1950, wrapped
# around a compress stream, which is itself documented in RFC 1951.
# 
#   The library also supports reading and writing files in gzip (.gz) format
# with an interface similar to that of stdio using the functions that start
# with "gz".  The gzip format is different from the zlib format.  gzip is a
# gzip wrapper, documented in RFC 1952, wrapped around a compress stream.
# 
#   This library can optionally read and write gzip and raw compress streams in
# memory as well.
# 
#   The zlib format was designed to be compact and fast for use in memory
# and on communications channels.  The gzip format was designed for single-
# file compression on file systems, has a larger header than zlib to maintain
# directory information, and uses a different, slower check method than zlib.
# 
#   The library does not install any signal handler.  The decoder checks
# the consistency of the compressed data, so the library should never crash
# even in the case of corrupted input.
# 
# @copyright 1995-2017 Jean-loup Gailly and Mark Adler
# @copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import _zlib


# version
/**
 * zLib version string 
 */
var version = _zlib.Z_VERSION


# compression levels
/**
 * no compression level
 */
var NO_COMPRESSION = _zlib.Z_NO_COMPRESSION

/**
 * best speed compression
 */
var BEST_SPEED = _zlib.Z_BEST_SPEED

/**
 * best compression level
 */
var BEST_COMPRESSION = _zlib.Z_BEST_COMPRESSION

/**
 * default compression level
 */
var DEFAULT_COMPRESSION = _zlib.Z_DEFAULT_COMPRESSION


# compression strategy; see compress() for details
/**
 * filtered compression strategy
 */
var FILTERED = _zlib.Z_FILTERED

/**
 * huffman only compression strategy
 */
var HUFFMAN_ONLY = _zlib.Z_HUFFMAN_ONLY

/**
 * rle compression strategy
 */
var RLE = _zlib.Z_RLE

/**
 * fixed compression strategy
 */
var FIXED = _zlib.Z_FIXED

/**
 * default compression strategy
 */
var DEFAULT_STRATEGY = _zlib.Z_DEFAULT_STRATEGY


# others
/**
 * default memory level
 */
var DEFAULT_MEMORY_LEVEL = 8

/**
 * maximum windows bit
 */
var MAX_WBITS = _zlib.MAX_WBITS


/**
 * adler32(value: bytes | string [, initial: number])
 * 
 * Updates a running Adler-32 checksum with the bytes buf[0..len-1] and
 * return the updated checksum.
 * 
 * @note An Adler-32 checksum is almost as reliable as a CRC-32 but can be computed much faster.
 * @return number
 */
def adler32(value, initial) {
  if !is_string(value) and !is_bytes(value)
    die Exception('string or bytes expected in arg 1 (value)')
  if initial != nil and !is_number(initial)
    die Exception('number expected in arg 2 (initial)')

  if is_string(data) data = data.to_bytes()
  if initial == nil initial = 0

  return _zlib.adler32(value, initial)
}

/**
 * crc32(value: bytes | string [, initial: number])
 * 
 * Update a running CRC-32 cheksum with the bytes buf[0..len-1] and return the
 * updated CRC-32 checksum.
 * @return number
 */
def crc32(value, initial) {
  if !is_string(value) and !is_bytes(value)
    die Exception('string or bytes expected in arg 1 (value)')
  if initial != nil and !is_number(initial)
    die Exception('number expected in arg 2 (initial)')

  if is_string(data) data = data.to_bytes()
  if initial == nil initial = 0

  return _zlib.crc32(value, initial)
}

/**
 * compress(data: bytes | string [, level: int = Z_DEFAULT_COMPRESSION [, strategy: int = Z_DEFAULT_STRATEGY [, wbits: int = MAX_WBITS [, memory_level: int = Z_DEFAULT_MEMORY_LEVEL]]]])
 * 
 * compress compresses as much data as possible, and stops when the input
 * buffer becomes empty or the output buffer becomes full.
 * 
 * -  The compression `level` must be DEFAULT_COMPRESSION, or between 0 and 9:
 *    1 gives best speed, 9 gives best compression, 0 gives no compression at all
 *    (the input data is simply copied a block at a time).  DEFAULT_COMPRESSION
 *    requests a default compromise between speed and compression (currently
 *    equivalent to level 6)
 * 
 * -  The `wbits` parameter is the base two logarithm of the window size
 *    (the size of the history buffer).  It should be in the range 8..15 for this
 *    version of the library.  Larger values of this parameter result in better
 *    compression at the expense of memory usage.  The default value is 15.
 * 
 *    For the current implementation of compress(), a `wbits` value of 8 (a
 *    window size of 256 bytes) is not supported.  As a result, a request for 8
 *    will result in 9 (a 512-byte window).
 * 
 *    `wbits` can also be -8..-15 for raw compress.  In this case, `-wbits`
 *    determines the window size.  compress() will then generate raw compress data
 *    with no zlib header or trailer, and will not compute a check value.
 * 
 *    `wbits` can also be greater than 15 for optional gzip encoding.  Add
 *    16 to `wbits` to write a simple gzip header and trailer around the
 *    compressed data instead of a zlib wrapper.  The gzip header will have no
 *    file name, no extra data, no comment, no modification time (set to zero), no
 *    header crc, and the operating system will be set to the appropriate value,
 *    if the operating system can be determined by the runtime.
 * 
 *    For raw compress or gzip encoding, a request for a 256-byte window is
 *    rejected as invalid, since only the zlib header provides a means of
 *    transmitting the window size to the uncompressor.
 * 
 * -  The `strategy` parameter is used to tune the compression algorithm.  Use the
 *    value DEFAULT_STRATEGY for normal data, FILTERED for data produced by a
 *    filter (or predictor), HUFFMAN_ONLY to force Huffman encoding only (no
 *    string match), or RLE to limit match distances to one (run-length
 *    encoding).  Filtered data consists mostly of small values with a somewhat
 *    random distribution.  In this case, the compression algorithm is tuned to
 *    compress them better.  The effect of FILTERED is to force more Huffman
 *    coding and less string matching; it is somewhat intermediate between
 *    DEFAULT_STRATEGY and HUFFMAN_ONLY.  RLE is designed to be almost as
 *    fast as HUFFMAN_ONLY, but give better compression for PNG image data.  The
 *    strategy parameter only affects the compression ratio but not the
 *    correctness of the compressed output even if it is not set appropriately.
 *    FIXED prevents the use of dynamic Huffman codes, allowing for a simpler
 *    decoder for special applications.
 * 
 * -  The `memory_level` parameter specifies how much memory should be allocated
 *    for the internal compression state.  memory_level 1 uses minimum memory but is
 *    slow and reduces compression ratio; memory_level 9 uses maximum memory for
 *    optimal speed.  The default value is 8.
 * 
 * @return bytes
 */
def compress(data, level, strategy, wbits, memory_level) {

  # validations.
  if !is_string(data) and !is_bytes(data)
    die Exception('string or bytes expected in arg 1 (data)')
  if level != nil and !is_number(level) and !is_int(level)
    die Exception('integer expected in arg 2 (level)')
  if strategy != nil and !is_number(strategy) and !is_int(strategy)
    die Exception('integer expected in arg 3 (strategy)')
  if wbits != nil and !is_number(wbits) and !is_int(wbits)
    die Exception('integer expected in arg 4 (wbits)')
  if memory_level != nil and !is_number(memory_level) and !is_int(memory_level)
    die Exception('integer expected in arg 5 (memory_level)')

  # defaults.
  if !level level = DEFAULT_COMPRESSION
  if !strategy strategy = DEFAULT_STRATEGY
  if wbits == nil wbits = MAX_WBITS
  if !memory_level memory_level = DEFAULT_MEMORY_LEVEL
  
  if is_string(data) data = data.to_bytes()

  return _zlib.deflate(data, level, strategy, wbits, memory_level)
}

/**
 * uncompress(data: bytes | string [, wbits: int = MAX_WBITS])
 * 
 * uncompress decompresses as much data as possible, and stops when the input
 * buffer becomes empty or the output buffer becomes full.
 * 
 * -  In this implementation, uncompress() always flushes as much output as
 *    possible to the output buffer, and always uses the faster approach on the
 *    first call.
 * 
 * -  The `wbits` parameter is the base two logarithm of the maximum window
 *    size (the size of the history buffer).  It should be in the range 8..15 for
 *    this version of the library.  The default value is 15.  `wbits` must be greater than or equal to the `wbits` value
 *    provided to compress() while compressing, or it must be equal to 15 if
 *    compress() is used with the default values.  If a compressed stream with a 
 *    larger window size is given as input, uncompress() will return with the error 
 *    code data error instead of trying to allocate a larger window.
 * 
 *    `wbits` can also be zero to request that uncompress use the window size in
 *    the zlib header of the compressed stream.
 * 
 *    `wbits` can also be -8..-15 for raw uncompress.  In this case, `-wbits`
 *    determines the window size.  uncompress() will then process raw compress data,
 *    not looking for a zlib or gzip header, not generating a check value, and not
 *    looking for any check values for comparison at the end of the stream.  This
 *    is for use with other formats that use the compress compressed data format
 *    such as zip.  Those formats provide their own check values.  If a custom
 *    format is developed using the raw compress format for compressed data, it is
 *    recommended that a check value such as an Adler-32 or a CRC-32 be applied to
 *    the uncompressed data as is done in the zlib, gzip, and zip formats.  For
 *    most applications, the zlib format should be used as is.  Note that comments
 *    on the use in compress() applies to the magnitude of `wbits`.
 * 
 *    `wbits` can also be greater than 15 for optional gzip decoding.  Add
 *    32 to `wbits` to enable zlib and gzip decoding with automatic header
 *    detection, or add 16 to decode only the gzip format (the zlib format will
 *    return a data error).  uncompress() will not automatically decode concatenated 
 *    gzip streams.
 * 
 * -  uncompress() can uncompress either zlib-wrapped or gzip-wrapped compress data.
 *    If the compression uses gzip-wrapper, the correct `wbits` may need to be set.
 * @return bytes
 */
def uncompress(data, wbits) {
  if !is_string(data) and !is_bytes(data)
    die Exception('string or bytes expected in arg 1 (data)')
  if wbits != nil and !is_number(wbits) and !is_int(wbits)
    die Exception('integer expected in arg 2 (wbits)')
  
  if is_string(data) data = data.to_bytes()
  if wbits == nil wbits = MAX_WBITS

  return _zlib.inflate(data, wbits)
}

/**
 * deflate(data: bytes | string)
 * 
 * Compress data using the default options for Deflate
 * @return bytes
 */
def deflate(data) {
  return compress(data, DEFAULT_COMPRESSION, DEFAULT_STRATEGY, -MAX_WBITS)
}

/**
 * undeflate(data: bytes | string)
 * 
 * uncompress a deflated data using default options
 * @return bytes
 */
def undeflate(data) {
  return uncompress(data, -MAX_WBITS)
}

/**
 * gzip(data: bytes | string)
 * 
 * Compress data using the default options for GZip
 * @return bytes
 */
def gzip(data) {
  return compress(data, DEFAULT_COMPRESSION, DEFAULT_STRATEGY, MAX_WBITS | 16)
}

/**
 * ungzip(data: bytes | string)
 * 
 * uncompress a GZipped data using default options
 * @return bytes
 */
def ungzip(data) {
  return uncompress(data, MAX_WBITS | 16)
}
