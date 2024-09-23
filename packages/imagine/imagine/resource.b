import _imagine
import _reflect

import types

import .quants
import .arcs

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
   * Draws a line between x1,y1 and x2, y2.The line is drawn using 
   * the color index specified. Note that color index can be a color 
   * returned by `allocate_color()` or one of `set_style()`, or
   * `set_brush()`.
   * 
   * @param {number} x1
   * @param {number} y1
   * @param {number} x2
   * @param {number} y2
   * @param {number} color
   */
  line(x1, y1, x2, y2, color) {
    if !is_number(x1) or !is_number(y1) or !is_number(x2) or !is_number(y2) or !is_number(color) {
      die Exception('number expected')
    }

    _imagine.line(self._ptr, x1, y1, x2, y2, color)
  }

  /**
   * Draws a dashed line between x1,y1 and x2, y2.The line is drawn using 
   * the color specified. Note that color index can be a color returned 
   * by `allocate_color()` or one of `set_style()`, or `set_brush()`.
   * 
   * @param {number} x1
   * @param {number} y1
   * @param {number} x2
   * @param {number} y2
   * @param {number} color
   */
  dashed_line(x1, y1, x2, y2, color) {
    if !is_number(x1) or !is_number(y1) or !is_number(x2) or !is_number(y2) or !is_number(color) {
      die Exception('number expected')
    }

    _imagine.dashedline(self._ptr, x1, y1, x2, y2, color)
  }

  /**
   * Draws a rectangle with the upper left (x1, y1) then lower right (y1,y2) 
   * corners specified, using the color specified.
   * 
   * @param {number} x1
   * @param {number} y1
   * @param {number} x2
   * @param {number} y2
   * @param {number} color
   */
  rectangle(x1, y1, x2, y2, color) {
    if !is_number(x1) or !is_number(y1) or !is_number(x2) or !is_number(y2) or !is_number(color) {
      die Exception('number expected')
    }

    _imagine.rectangle(self._ptr, x1, y1, x2, y2, color)
  }

  /**
   * Draws a solid rectangle with the upper left (x1, y1) then lower 
   * right (y1,y2) corners specified, using the color specified.
   * 
   * @param {number} x1
   * @param {number} y1
   * @param {number} x2
   * @param {number} y2
   * @param {number} color
   */
  filled_rectangle(x1, y1, x2, y2, color) {
    if !is_number(x1) or !is_number(y1) or !is_number(x2) or !is_number(y2) or !is_number(color) {
      die Exception('number expected')
    }

    _imagine.filledrectangle(self._ptr, x1, y1, x2, y2, color)
  }

  /**
   * Returns true if the coordinate represented by _x_ and _y_ 
   * is within the bounds of the image.
   * 
   * @param {number} x
   * @param {number} y
   */
  safe_bound(x, y) {
    if !is_number(x) or !is_number(y) {
      die Exception('number expected')
    }

    return _imagine.boundsafe(self._ptr, x, y)
  }

  /**
   * Draws a single character.
   * 
   * @param {number} x - The x coordinate of the upper left pixel.
   * @param {number} y - The y coordinate of the upper left pixel.
   * @param {char} text - The character.
   * @param {font} font - The raster font.
   * @param {number} color - The color.
   */
  char(x, y, char, font, color) {
    if !font font = _imagine.smallfont
    if !color color = self.allocate_color(0, 0, 0, 0)

    if !is_number(x) or !is_number(y) {
      die Exception('number expected for x and y coordinate')
    }

    if !types.char(char) {
      die Exception('char expected for char, ${typeof(char)} given')
    }

    if !is_number(color) {
      die Exception('number expected color, ${typeof(color)} given')
    }

    if !_reflect.isptr(font) or to_string(font).index_of('imagine::type::font') == -1 {
      die Exception('imagine::font expected font')
    }

    _imagine.char(self._ptr, font, x, y, ord(char), color)
  }

  /**
   * Draws a single character vertically.
   * 
   * @param {number} x - The x coordinate of the upper left pixel.
   * @param {number} y - The y coordinate of the upper left pixel.
   * @param {char} text - The character.
   * @param {font} font - The raster font.
   * @param {number} color - The color.
   */
  char_vert(x, y, char, font, color) {
    if !font font = _imagine.smallfont
    if !color color = self.allocate_color(0, 0, 0, 0)

    if !is_number(x) or !is_number(y) {
      die Exception('number expected for x and y coordinate')
    }

    if !types.char(char) {
      die Exception('char expected for char, ${typeof(char)} given')
    }

    if !is_number(color) {
      die Exception('number expected color, ${typeof(color)} given')
    }

    if !_reflect.isptr(font) or to_string(font).index_of('imagine::type::font') == -1 {
      die Exception('imagine::font expected font')
    }

    _imagine.charup(self._ptr, font, x, y, ord(char), color)
  }

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

  /**
   * Draws a character string vertically.
   * 
   * @param {number} x - The x coordinate of the upper left pixel.
   * @param {number} y - The y coordinate of the upper left pixel.
   * @param {string} text - The character string.
   * @param {font} font - The raster font.
   * @param {number} color - The color.
   */
  string_vert(x, y, text, font, color) {
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

    _imagine.stringup(self._ptr, font, x, y, text, color)
  }

  /**
   * Draws a polygon with the vertices specified by _points_, in the 
   * specified by _color_. There must be at least three points.
   * 
   * Point must be a list of lists where each list contains two numbers 
   * for the x and y coordinates. It is required that there must be at 
   * least three points.
   * 
   * @param {list[list]} points
   * @param {number} color
   */
  polygon(points, color) {
    if !is_list(points)
      die Exception('list expected at points, ${typeof(points)} given')
    if !is_number(color)
      die Exception('number expected color, ${typeof(color)} given')

    # validate the point
    for point in points {
      if !is_list(point) or point.length() != 2
        die Exception('invalid points data')

      for item in point {
        if !is_number(item)
          die Exception('invalid points data')
      }
    }

    if points.length() < 3
      die Exception('a minimum of three points expected')

    _imagine.polygon(self._ptr, points, color)
  }

  /**
   * Draws an open polygon with the vertices specified by _points_, in 
   * the specified by _color_. There must be at least three points.
   * 
   * Point must be a list of lists where each list contains two numbers 
   * for the x and y coordinates. It is required that there must be at 
   * least three points.
   * 
   * @param {list[list]} points
   * @param {number} color
   */
  open_polygon(points, color) {
    if !is_list(points)
      die Exception('list expected at points, ${typeof(points)} given')
    if !is_number(color)
      die Exception('number expected color, ${typeof(color)} given')

    # validate the point
    for point in points {
      if !is_list(point) or point.length() != 2
        die Exception('invalid points data')

      for item in point {
        if !is_number(item)
          die Exception('invalid points data')
      }
    }

    if points.length() < 3
      die Exception('a minimum of three points expected')

    _imagine.openpolygon(self._ptr, points, color)
  }

  /**
   * Fills a polygon with the vertices specified by _points_, in the 
   * specified by _color_. There must be at least three points.
   * 
   * Point must be a list of lists where each list contains two numbers 
   * for the x and y coordinates. It is required that there must be at 
   * least three points.
   * 
   * @param {list[list]} points
   * @param {number} color
   */
  filled_polygon(points, color) {
    if !is_list(points)
      die Exception('list expected at points, ${typeof(points)} given')
    if !is_number(color)
      die Exception('number expected color, ${typeof(color)} given')

    # validate the point
    for point in points {
      if !is_list(point) or point.length() != 2
        die Exception('invalid points data')

      for item in point {
        if !is_number(item)
          die Exception('invalid points data')
      }
    }

    if points.length() < 3
      die Exception('a minimum of three points expected')

    _imagine.filledpolygon(self._ptr, points, color)
  }

  /**
   * Draws a partial ellipse centered at the given point, with the 
   * specified width and height in pixels. The arc begins at the 
   * position in degrees specified by _start_ and ends at the 
   * position specified by _end_. The arc is drawn in the color 
   * specified by the last argument. A circle can be drawn by 
   * beginning from 0 degrees and ending at 360 degrees, with width 
   * and height being equal. `end` must be greater than `start`. 
   * Values greater than 360 are interpreted modulo 360. 
   * 
   * @param {number} x
   * @param {number} y
   * @param {number} width
   * @param {number} height
   * @param {number} start
   * @param {number} end
   * @param {number} color
   */
  arc(x, y, width, height, start, end, color) {
    if !is_number(x) or !is_number(y) or !is_number(width) or 
      !is_number(height) or !is_number(start) or !is_number(end) or !is_number(color) {
        die Exception('number expected')
    }

    _imagine.arc(self._ptr, x, y, width, height, start, end, color)
  }

  /**
   * Fills a partial ellipse centered at the given point, with the 
   * specified width and height in pixels using the specified style. 
   * The arc begins at the position in degrees specified by _start_ 
   * and ends at the position specified by _end_. The arc is drawn 
   * in the color specified by the last argument. A circle can be 
   * drawn by beginning from 0 degrees and ending at 360 degrees, 
   * with width and height being equal. `end` must be greater than 
   * `start`. Values greater than 360 are interpreted modulo 360. 
   * 
   * Style must be one or more of ARC_* constants or'ed together.
   * E.g. `ARC_NO_FILL | ARC_NO_EDGE`.
   * 
   * When style is not given, it defaults to `ARC_PIE`.
   * 
   * @param {number} x
   * @param {number} y
   * @param {number} width
   * @param {number} height
   * @param {number} start
   * @param {number} end
   * @param {number} color
   * @param {number} style
   */
  filled_arc(x, y, width, height, start, end, color, style) {
    if !style style = arcs.ARC_PIE
    
    if !is_number(x) or !is_number(y) or !is_number(width) or 
      !is_number(height) or !is_number(start) or !is_number(end) or 
      !is_number(color) or !is_number(style) {
        die Exception('number expected')
    }

    _imagine.filledarc(self._ptr, x, y, width, height, start, end, color, style)
  }

  /**
   * Draws a full ellipse centered at the given point, with the 
   * specified width, height, and color.
   * 
   * @param {number} x
   * @param {number} y
   * @param {number} width
   * @param {number} height
   * @param {number} color
   */
  ellipse(x, y, width, height, color) {
    if !is_number(x) or !is_number(y) or !is_number(width) or 
      !is_number(height) or !is_number(color) {
        die Exception('number expected')
    }

    _imagine.ellipse(self._ptr, x, y, width, height, color)
  }

  /**
   * Fills a full ellipse centered at the given point, with the 
   * specified width, height, and color.
   * 
   * @param {number} x
   * @param {number} y
   * @param {number} width
   * @param {number} height
   * @param {number} color
   */
  filled_ellipse(x, y, width, height, color) {
    if !is_number(x) or !is_number(y) or !is_number(width) or 
      !is_number(height) or !is_number(color) {
        die Exception('number expected')
    }

    _imagine.filledellipse(self._ptr, x, y, width, height, color)
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

  /**
   * Returns the closes color based on the image to the color specified by 
   * `r`, `g`, `b`, and `a`. A slightly different color with the same 
   * transparency beats the exact same color with radically different 
   * transparency.
   * 
   * @param {number} r
   * @param {number} g
   * @param {number} b
   * @param {number} a
   * @returns {number}
   */
  closest_color(r, g, b, a) {
    if r == nil r = 0
    if g == nil g = 0
    if b == nil b = 0
    if a == nil a = 0

    if !is_number(r) or !is_number(g) or !is_number(b) or !is_number(a) {
      die Exception('number expected')
    }

    if self._true_color and a {
      return _imagine.colorclosestalpha(self._ptr, r, g, b, a)
    }

    return _imagine.colorclosest(self._ptr, r, g, b)
  }

  /**
   * Same as `closes_color()` but uses an alternative algorithm and does 
   * not account for transparency.
   * 
   * @param {number} r
   * @param {number} g
   * @param {number} b
   * @returns {number}
   */
  closest_color_hwb(r, g, b) {
    if r == nil r = 0
    if g == nil g = 0
    if b == nil b = 0

    if !is_number(r) or !is_number(g) or !is_number(b) {
      die Exception('number expected')
    }

    return _imagine.colorclosesthwb(self._ptr, r, g, b)
  }

  /**
   * Returns an exact match only, including alpha when specified.
   * 
   * @param {number} r
   * @param {number} g
   * @param {number} b
   * @param {number} a
   * @returns {number}
   */
  exact_color(r, g, b, a) {
    if r == nil r = 0
    if g == nil g = 0
    if b == nil b = 0
    if a == nil a = 0

    if !is_number(r) or !is_number(g) or !is_number(b) or !is_number(a) {
      die Exception('number expected')
    }

    if self._true_color and a {
      return _imagine.colorexactalpha(self._ptr, r, g, b, a)
    }

    return _imagine.colorexact(self._ptr, r, g, b)
  }
  
  /**
   * Resolves color in the image based on `exact_color()` and `closest_color()` 
   * and return the one that matches the image best.
   * 
   * @param {number} r
   * @param {number} g
   * @param {number} b
   * @param {number} a
   * @returns {number}
   */
  resolve_color(r, g, b, a) {
    if r == nil r = 0
    if g == nil g = 0
    if b == nil b = 0
    if a == nil a = 0

    if !is_number(r) or !is_number(g) or !is_number(b) or !is_number(a) {
      die Exception('number expected')
    }

    if self._true_color and a {
      return _imagine.colorresolvealpha(self._ptr, r, g, b, a)
    }

    return _imagine.colorresolve(self._ptr, r, g, b)
  }

  /**
   * Deallocates a color previously allocated from the image.
   * 
   * @param {number} color
   */
  deallocate_color(color) {
    if !is_number(color) {
      die Exception('number expected')
    }

    _imagine.colordeallocate(self._ptr, color)
  }

  /**
   * Specifies a color index (if a palette image) or an RGB color (if a 
   * truecolor image) which should be considered 100% transparent. FOR 
   * TRUECOLOR IMAGES, THIS IS IGNORED IF AN ALPHA CHANNEL IS BEING SAVED. 
   * Use `save_apha(false)` to turn off the saving of a full alpha 
   * channel in a truecolor image. Note that this function is usually 
   * compatible with older browsers that do not understand full alpha 
   * channels well.
   * 
   * @param {number} color
   */
  color_transparent(color) {
    if !is_number(color) {
      die Exception('number expected')
    }

    _imagine.colortransparent(self._ptr, color)
  }

  /**
   * Copies the palatte from a paletted image to this image.
   * 
   * @param {ImageResource} image
   */
  palette_copy(image) {
    if !instance_of(image, ImageResource) {
      die Exception('ImageResource expected')
    }

    _imagine.palettecopy(self._ptr, image._ptr)
  }

  /**
   * Replaces every occurrence of color _src_ in the image with the 
   * color _dest_.
   * 
   * @param {number} src
   * @param {number} dest
   * @returns {bool}
   */
  color_replace(src, dest) {
    if !is_number(src) or !is_number(dest) {
      die Exception('number expected')
    }

    return _imagine.colorreplace(self._ptr, src, dest) == 0
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
   * Flood fills the image with the given _color_ starting are 
   * the coordinates given by _x_ and _y_ and using the color 
   * specified by border to fill its borders.
   * 
   * @param {number} x
   * @param {number} y
   * @param {number} color
   */
  fill_to_border(x, y, border, color) {
    if !is_number(x) or !is_number(y) {
      die Exception('number expected for x and y coordinate')
    } else if !is_number(border) {
      die Exception('number expected border, ${typeof(border)} given')
    } else if !is_number(color) {
      die Exception('number expected color, ${typeof(color)} given')
    }

    _imagine.filltoborder(self._ptr, x, y, border, color)
  }
  
  /**
   * @param {ImageResource} src
   * @param {number} dst_x
   * @param {number} dst_y
   * @param {number} src_x
   * @param {number} src_y
   * @param {number} width
   * @param {number} height
   */
  copy(src, dst_x, dst_y, src_x, src_y, width, height) {
    if !instance_of(src, ImageResource) {
      die Exception('image resource expected in argument 1, ${typeof(src)} given')
    } else if !is_number(dst_x) or !is_number(dst_y) or !is_number(src_x) or
        !is_number(src_y) or !is_number(width) or !is_number(height) {
      die Exception('number expected')
    }
    
    _imagine.copy(self._ptr, src._ptr, dst_x, dst_y, src_x, src_y, width, height)
  }
  
  /**
   * @param {ImageResource} src
   * @param {number} dst_x
   * @param {number} dst_y
   * @param {number} src_x
   * @param {number} src_y
   * @param {number} width
   * @param {number} height
   * @param {number} pct
   */
  copy_merge(src, dst_x, dst_y, src_x, src_y, width, height, pct) {
    if !instance_of(src, ImageResource) {
      die Exception('image resource expected in argument 1, ${typeof(src)} given')
    } else if !is_number(dst_x) or !is_number(dst_y) or !is_number(src_x) or
        !is_number(src_y) or !is_number(width) or !is_number(height) or
            !is_number(pct) {
      die Exception('number expected')
    }

    _imagine.copymerge(self._ptr, src._ptr, dst_x, dst_y, src_x, src_y, width, height, pct)
  }
  
  /**
   * @param {ImageResource} src
   * @param {number} dst_x
   * @param {number} dst_y
   * @param {number} src_x
   * @param {number} src_y
   * @param {number} width
   * @param {number} height
   * @param {number} pct
   */
  copy_merge_gray(src, dst_x, dst_y, src_x, src_y, width, height, pct) {
    if !instance_of(src, ImageResource) {
      die Exception('image resource expected in argument 1, ${typeof(src)} given')
    } else if !is_number(dst_x) or !is_number(dst_y) or !is_number(src_x) or
        !is_number(src_y) or !is_number(width) or !is_number(height) or
            !is_number(pct) {
      die Exception('number expected')
    }
    
    _imagine.copymergegray(self._ptr, src._ptr, dst_x, dst_y, src_x, src_y, width, height, pct)
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