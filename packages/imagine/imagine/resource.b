import _imagine
import _reflect
import .quants

class ImageResource {
  # cache for the image meta
  var _meta
  # true color meta state
  var _true_color

  ImageResource(ptr) {
    # self._ptr is the raw image pointer
    self._ptr = ptr
    self._true_color = self.meta().true_color
  }

  /**
   * Invokes the given callback with the image as a parameter and 
   * automatically closes the image once the callback returns. 
   * Leaving images in open can quickly lead to resource exhaustion 
   * especially when working with multiple images. The `use()` 
   * method is recommended over manually closing images as it 
   * ensures that an image is always closed and not forgotten in 
   * memory.
   * 
   * @param {function(1)} callback
   */
  use(callback) {
    if !is_function(callback)
      die Exception('function(1) expected as callback')
    
    var fn_arity = _reflect.getfunctionmetadata(callback).arity
    if fn_arity != 1 
      die Exception('function must accept exactly one argument (callback)')

    callback(self)
    _imagine.close(self._ptr)
  }

  /**
   * Closes an image and frees all associated resources.
   * 
   * @note an image can no longer be used once it is closed.
   */
  close() {
    _imagine.close(self._ptr)
  }

  # ------------------------- EXTRAS ------------------------------

  /**
   * Returns metadata information about the image.
   * 
   * Metadata contains:
   * - `width`: The width of the image (in pixels).
   * - `height`: The height of the image (in pixels).
   * - `colors`: The number of colors in the image.
   * - `res_x`: The horizontal resolution in DPI.
   * - `res_y`: The vertical resolution in DPI.
   * - `interpolation`: The method of interpolation used on the image.
   * - `true_color`: True if the image uses true colors, false otherwise.
   * - `interlaced`: True if the image is interlaced, false otherwise.
   * 
   * @returns {dict}
   */
  meta() {
    if !self._meta {
      self._meta = _imagine.meta(self._ptr)
    }

    return self._meta
  }

  # ------------------------- PIXELS ------------------------------

  /**
   * Sets the pixel indicated by _x_ and _y_ coordinate in the image to 
   * the given _color_.
   * 
   * @param {number} x
   * @param {number} y
   * @param {number} color
   */
  set_pixel(x, y, color) {
    if !is_number(x) or !is_number(y) or !is_number(color) {
      die Exception('number expected')
    }

    _imagine.setpixel(self._ptr, x, y, color)
  }

  /**
   * Returns the color at the give pixel indicated by _x_ and _y_ 
   * coordinate in the image.
   * 
   * @param {number} x
   * @param {number} y
   * @returns {number}
   */
  get_pixel(x, y) {
    if !is_number(x) or !is_number(y) {
      die Exception('number expected')
    }

    return _imagine.getpixel(self._ptr, x, y, self._true_color)
  }

  # ------------------------- DRAWINGS ------------------------------

  /**
   * Draws a character string.
   * 
   * @param {number} x - The x coordinate of the upper left pixel.
   * @param {number} y - The y coordinate of the upper left pixel.
   * @param {string} text - The character string.
   * @param {font} font - The raster font.
   * @param {number} color - The color.
   */
  string(x, y, text, font, color) {
    if !font font = _imagine.smallfont
    if !color color = self.allocate_color(0, 0, 0, 0)

    if !is_number(x) or !is_number(y) {
      die Exception('number expected for x and y coordinate')
    }

    if !is_string(text) {
      die Exception('string expected for text, ${typeof(text)} given')
    }

    if !is_number(color) {
      die Exception('number expected color, ${typeof(color)} given')
    }

    if !_reflect.isptr(font) or to_string(font).index_of('imagine::type::font') == -1 {
      die Exception('imagine::font expected font')
    }

    _imagine.string(self._ptr, font, x, y, text, color)
  }

  # ------------------------- COLOR ------------------------------

  /**
   * Returns the given color allocated from the image palette. 
   * Any of R, G, B, or A can be omitted or set to nil in which case 
   * they'll default to zero.
   * 
   * @param {number?} r
   * @param {number?} g
   * @param {number?} b
   * @param {number?} a
   * @returns {number}
   */
  allocate_color(r, g, b, a) {
    if r == nil r = 0
    if g == nil g = 0
    if b == nil b = 0
    if a == nil a = 0

    if !is_number(r) or !is_number(g) or !is_number(b) or !is_number(a) {
      die Exception('number expected')
    }

    if self._true_color {
      return _imagine.colorallocatealpha(self._ptr, r, g, b, a)
    }

    return _imagine.colorallocate(self._ptr, r, g, b)
  }

  # ------------------------- PROCESSING ------------------------------

  /**
   * Flood fills the image with the given _color_ starting are 
   * the coordinates given by _x_ and _y_.
   * 
   * @param {number} x
   * @param {number} y
   * @param {number} color
   */
  fill(x, y, color) {
    if !is_number(x) or !is_number(y) {
      die Exception('number expected for x and y coordinate')
    }

    if !is_number(color) {
      die Exception('number expected color, ${typeof(color)} given')
    }

    _imagine.fill(self._ptr, x, y, color)
  }

  /**
   * Sets the save alpha flag
   * 
   * The save alpha flag specifies whether the alpha channel of the pixels should
   * be saved. This is supported only for image formats that support full alpha
   * transparency, e.g. PNG.
   * 
   * @param {bool} save
   */
  save_alpha(save) {
    if save == nil save = true
    if !is_bool(save)
      die Exception('boolean expected')

    _imagine.savealpha(self._ptr, save ? 1 : 0)
  }

  # ------------------------- FILTERS ------------------------------

  # ------------------------- MISC ------------------------------

  # ------------------------- EXPORT ------------------------------

  /**
   * Saves the image to file with the PNG format.
   * 
   * Quality level: 0-10, where 9 is NO COMPRESSION at all,
   * 9 is FASTEST but produces larger files, 0 provides the best
   * compression (smallest files) but takes a long time to compress, and
   * 10 selects the default compiled into the zlib library.
   * 
   * @param {string|file} dest
   * @param {number} quality
   */
  export_png(dest, quality) {
    if quality == nil quality = 10

    if !is_string(dest) and !is_file(dest) 
      die Exception('string path or file expected, ${typeof(dest)} given')
    if !is_number(quality)
      die Exception('number expected at quality, ${typeof(quality)} given')

    if quality < 1 or quality > 10 {
      die Exception('quality must be between 1 and 10')
    }

    if is_string(dest) {
      dest = file(dest, 'wb')
    }

    if dest.mode().index_of('w') == -1{
      die Exception('file not writable')
    }

    _imagine.png(self._ptr, dest, 9 - quality)
  }

  /**
   * Saves the image to file with the JPEG format.
   * 
   * Quality level: 100 is highest quality (there is always 
   * a little loss with JPEG). 0 is lowest. 10 is about the 
   * lowest useful setting.
   * 
   * @param {string|file} dest
   * @param {number} quality
   */
  export_jpeg(dest, quality) {
    if quality == nil quality = 100

    if !is_string(dest) and !is_file(dest) 
      die Exception('string path or file expected, ${typeof(dest)} given')
    if !is_number(quality)
      die Exception('number expected at quality, ${typeof(quality)} given')

    if quality < 0 or quality > 100 {
      die Exception('quality must be between 0 and 100')
    }

    if is_string(dest) {
      dest = file(dest, 'wb')
    }

    if dest.mode().index_of('w') == -1{
      die Exception('file not writable')
    }

    _imagine.jpeg(self._ptr, dest, quality)
  }

  /**
   * Saves the image to file with the BMP format.
   * 
   * Quality level: 100 is highest quality (there is always 
   * a little loss with BMP). 0 is lowest. 10 is about the 
   * lowest useful setting.
   * 
   * @param {string|file} dest
   * @param {number} quality
   */
  export_bmp(dest, quality) {
    if quality == nil quality = 100

    if !is_string(dest) and !is_file(dest) 
      die Exception('string path or file expected, ${typeof(dest)} given')
    if !is_number(quality)
      die Exception('number expected at quality, ${typeof(quality)} given')

    if quality < 0 or quality > 100 {
      die Exception('quality must be between 0 and 100')
    }

    if is_string(dest) {
      dest = file(dest, 'wb')
    }

    if dest.mode().index_of('w') == -1{
      die Exception('file not writable')
    }

    _imagine.bmp(self._ptr, dest, 100 - quality)
  }

  /**
   * Saves the image to file with the WBMP format using the 
   * given foreground color.
   * 
   * @param {string|file} dest
   * @param {number} foreground
   */
  export_wbmp(dest, foreground) {
    if foreground == nil foreground = self.allocate_color(0, 0, 0)

    if !is_string(dest) and !is_file(dest) 
      die Exception('string path or file expected, ${typeof(dest)} given')
    if !is_number(foreground)
      die Exception('number expected at quality, ${typeof(foreground)} given')

    if is_string(dest) {
      dest = file(dest, 'wb')
    }

    if dest.mode().index_of('w') == -1{
      die Exception('file not writable')
    }

    _imagine.wbmp(self._ptr, dest, foreground)
  }

  /**
   * Saves the image to file with the WEBP format using the 
   * given quantization.
   * 
   * @param {string|file} dest
   * @param {number} quantization
   */
  export_webp(dest, quantization) {
    if quantization == nil quantization = quants.QUANT_DEFAULT

    if !is_string(dest) and !is_file(dest) 
      die Exception('string path or file expected, ${typeof(dest)} given')
    if !is_number(quantization)
      die Exception('number expected at quality, ${typeof(quantization)} given')

    if is_string(dest) {
      dest = file(dest, 'wb')
    }

    if dest.mode().index_of('w') == -1{
      die Exception('file not writable')
    }

    _imagine.webp(self._ptr, dest, quantization)
  }

  /**
   * Saves the image to file with the TIFF format.
   * 
   * @param {string|file} dest
   */
  export_tiff(dest) {
    if !is_string(dest) and !is_file(dest) 
      die Exception('string path or file expected, ${typeof(dest)} given')

    if is_string(dest) {
      dest = file(dest, 'wb')
    }

    if dest.mode().index_of('w') == -1{
      die Exception('file not writable')
    }

    _imagine.tiff(self._ptr, dest)
  }

  /**
   * Saves the image to file with the JPEG format.
   * 
   * Quality level: 100 is highest quality (there is always 
   * a little loss with JPEG). 0 is lowest. 10 is about the 
   * lowest useful setting.
   * 
   * @param {string|file} dest
   * @param {number} quality
   * @param {number} speed - Default = 1
   */
  export_avif(dest, quality, speed) {
    if quality == nil quality = 100
    if speed == nil speed = 1

    if !is_string(dest) and !is_file(dest) 
      die Exception('string path or file expected, ${typeof(dest)} given')
    if !is_number(quality)
      die Exception('number expected at quality, ${typeof(quality)} given')
    if !is_number(speed)
      die Exception('number expected at speed, ${typeof(speed)} given')

    if quality < 0 or quality > 100 {
      die Exception('quality must be between 0 and 100')
    }

    if is_string(dest) {
      dest = file(dest, 'wb')
    }

    if dest.mode().index_of('w') == -1{
      die Exception('file not writable')
    }

    _imagine.avif(self._ptr, dest, quality, speed)
  }
}