#!-- part of the imagine module

import _imagine
import _reflect

import types

import .quants
import .arcs
import .flips
import .crops
import .blurs
import .interpolations


/**
 * The ImageResource class represents a loaded image and exposes all 
 * the image processing, metadata and manipulation functions.
 */
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
   * @param function(1) callback
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
   * @returns dict
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
   * @param number x
   * @param number y
   * @param number color
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
   * @param number x
   * @param number y
   * @returns number
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
   * @param number x1
   * @param number y1
   * @param number x2
   * @param number y2
   * @param number color
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
   * @param number x1
   * @param number y1
   * @param number x2
   * @param number y2
   * @param number color
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
   * @param number x1
   * @param number y1
   * @param number x2
   * @param number y2
   * @param number color
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
   * @param number x1
   * @param number y1
   * @param number x2
   * @param number y2
   * @param number color
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
   * @param number x
   * @param number y
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
   * @param number x - The x coordinate of the upper left pixel.
   * @param number y - The y coordinate of the upper left pixel.
   * @param char text - The character.
   * @param font font - The raster font.
   * @param number color - The color.
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
   * @param number x - The x coordinate of the upper left pixel.
   * @param number y - The y coordinate of the upper left pixel.
   * @param char text - The character.
   * @param font font - The raster font.
   * @param number color - The color.
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
   * @param number x - The x coordinate of the upper left pixel.
   * @param number y - The y coordinate of the upper left pixel.
   * @param string text - The character string.
   * @param font font - The raster font.
   * @param number color - The color.
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
   * @param number x - The x coordinate of the upper left pixel.
   * @param number y - The y coordinate of the upper left pixel.
   * @param string text - The character string.
   * @param font font - The raster font.
   * @param number color - The color.
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
   * @param list[list] points
   * @param number color
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
   * @param list[list] points
   * @param number color
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
   * @param list[list] points
   * @param number color
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
   * @param number x
   * @param number y
   * @param number width
   * @param number height
   * @param number start
   * @param number end
   * @param number color
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
   * @param number x
   * @param number y
   * @param number width
   * @param number height
   * @param number start
   * @param number end
   * @param number color
   * @param number style
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
   * @param number x
   * @param number y
   * @param number width
   * @param number height
   * @param number color
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
   * @param number x
   * @param number y
   * @param number width
   * @param number height
   * @param number color
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
   * @param number? r
   * @param number? g
   * @param number? b
   * @param number? a
   * @returns number
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
   * @param number r
   * @param number g
   * @param number b
   * @param number a
   * @returns number
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
   * @param number r
   * @param number g
   * @param number b
   * @returns number
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
   * @param number r
   * @param number g
   * @param number b
   * @param number a
   * @returns number
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
   * @param number r
   * @param number g
   * @param number b
   * @param number a
   * @returns number
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
   * @param number color
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
   * @param number color
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
   * @param ImageResource image
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
   * @param number src
   * @param number dest
   * @returns bool
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
   * @param number x
   * @param number y
   * @param number color
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
   * @param number x
   * @param number y
   * @param number color
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
   * Copy a part of image _src_ onto this image starting at the x,y c
   * oordinates src_x, src_y with the source width and height. The 
   * portion defined will be copied onto the x,y coordinates, dst_x 
   * and dst_y.
   * 
   * @param ImageResource src
   * @param number dst_x
   * @param number dst_y
   * @param number src_x
   * @param number src_y
   * @param number width
   * @param number height
   */
  copy(src, dst_x, dst_y, src_x, src_y, width, height) {
    if !instance_of(src, ImageResource) {
      die Exception('image resource expected in argument 1, ${typeof(src)} given')
    } else if !is_number(dst_x) or !is_number(dst_y) or !is_number(src_x) or
        !is_number(src_y) or !is_number(width) or !is_number(height) {
      die Exception('number expected')
    }
    
    _imagine.copy(self._ptr, src.get_pointer(), dst_x, dst_y, src_x, src_y, width, height)
  }
  
  /**
   * Copy and merge a part of image _src_ onto this image starting 
   * at the x,y coordinates src_x, src_y with the source width and 
   * height. The portion defined will be copied onto the x,y 
   * coordinates, dst_x and dst_y.
   * 
   * The two images will be merged according to pct which can range 
   * from 0 to 100. When pct = 0, no action is taken, when 100 this 
   * function behaves identically to `copy()` for pallete images, 
   * except for ignoring alpha components, while it implements 
   * alpha transparency for true colour images.
   * 
   * @param ImageResource src
   * @param number dst_x
   * @param number dst_y
   * @param number src_x
   * @param number src_y
   * @param number width
   * @param number height
   * @param number pct
   */
  copy_merge(src, dst_x, dst_y, src_x, src_y, width, height, pct) {
    if !instance_of(src, ImageResource) {
      die Exception('image resource expected in argument 1, ${typeof(src)} given')
    } else if !is_number(dst_x) or !is_number(dst_y) or !is_number(src_x) or
        !is_number(src_y) or !is_number(width) or !is_number(height) or
            !is_number(pct) {
      die Exception('number expected')
    }

    _imagine.copymerge(self._ptr, src.get_pointer(), dst_x, dst_y, src_x, src_y, width, height, pct)
  }
  
  /**
   * Same as `copy_merge()` except that when merging it preserves the 
   * hue of the source by converting the destination pixels to gray scale 
   * before the copy operation.
   * 
   * @param ImageResource src
   * @param number dst_x
   * @param number dst_y
   * @param number src_x
   * @param number src_y
   * @param number width
   * @param number height
   * @param number pct
   */
  copy_merge_gray(src, dst_x, dst_y, src_x, src_y, width, height, pct) {
    if !instance_of(src, ImageResource) {
      die Exception('image resource expected in argument 1, ${typeof(src)} given')
    } else if !is_number(dst_x) or !is_number(dst_y) or !is_number(src_x) or
        !is_number(src_y) or !is_number(width) or !is_number(height) or
            !is_number(pct) {
      die Exception('number expected')
    }
    
    _imagine.copymergegray(self._ptr, src.get_pointer(), dst_x, dst_y, src_x, src_y, width, height, pct)
  }

  /**
   * Copy a resized area defined by src_x, src_y, src_width, and 
   * src_height from the image _src_ to the area defined by x, y, 
   * width, height on this image.
   * 
   * If the source and destination coordinates and width and heights 
   * differ, appropriate stretching or shrinking of the image fragment
   * will be performed. 
   * 
   * The coordinates refer to the upper left corner. 
   * 
   * This function can be used to copy regions within the same image 
   * (if this image is the same as _src_) but if the regions overlap 
   * the results will be unpredictable.
   * 
   * @param ImageResource src
   * @param number x
   * @param number y
   * @param number src_x
   * @param number src_y
   * @param number width
   * @param number height
   * @param number src_width
   * @param number src_height
   */
  copy_resized(src, x, y, src_x, src_y, width, height, src_width, src_height) {
    if !instance_of(src, ImageResource) {
      die Exception('ImageResource expected in argument 1')
    } else if !is_number(x) or !is_number(y) or !is_number(src_x) or !is_number(src_y) or
        !is_number(width) or !is_number(height) or !is_number(src_width) or !is_number(src_height) {
      die Exception('number expected')
    }
    
    _imagine.copyresized(self._ptr, src.get_pointer(), x, y, src_x, src_y, width, height, src_width, src_height)
  }

  /**
   * Copy a resized area defined by src_x, src_y, src_width, and 
   * src_height from the image _src_ to the area defined by x, y, 
   * width, height on this image. Unlike `copy_resized()`, it 
   * smoothly interpolates pixel values so that, in particular, 
   * reducing the size of an image still retains a great deal of 
   * clarity.
   * 
   * If the source and destination coordinates and width and heights 
   * differ, appropriate stretching or shrinking of the image fragment
   * will be performed. 
   * 
   * The coordinates refer to the upper left corner. 
   * 
   * This function can be used to copy regions within the same image 
   * (if this image is the same as _src_) but if the regions overlap 
   * the results will be unpredictable.
   * 
   * @param ImageResource src
   * @param number x
   * @param number y
   * @param number src_x
   * @param number src_y
   * @param number width
   * @param number height
   * @param number src_width
   * @param number src_height
   */
  copy_resampled(src, x, y, src_x, src_y, width, height, src_width, src_height) {
    if !instance_of(src, ImageResource) {
      die Exception('ImageResource expected in argument 1')
    } else if !is_number(x) or !is_number(y) or !is_number(src_x) or !is_number(src_y) or
        !is_number(width) or !is_number(height) or !is_number(src_width) or !is_number(src_height) {
      die Exception('number expected')
    }
    
    _imagine.copyresampled(self._ptr, src.get_pointer(), x, y, src_x, src_y, width, height, src_width, src_height)
  }

  /**
   * Similar to `copy_resized()` with an added rotation to the copied image. 
   * Destination is the _center_ of the rotated copy. Angle is in degrees, 
   * same as `arc()`. 
   * 
   * Floating point destination center coordinates allow accurate rotation of 
   * objects of odd-numbered width or height.
   * 
   * The rotation angle is interpreted as the number of degrees to rotate the 
   * image anticlockwise.
   * 
   * @param ImageResource src
   * @param number x
   * @param number y
   * @param number src_x
   * @param number src_y
   * @param number src_width
   * @param number src_height
   * @param number angle
   */
  copy_rotated(src, x, y, src_x, src_y, src_width, src_height, angle) {
    if !instance_of(src, ImageResource) {
      die Exception('ImageResource expected in argument 1')
    } else if !is_number(x) or !is_number(y) or !is_number(src_x) or !is_number(src_y) or
        !is_number(src_width) or !is_number(src_height) or !is_number(angle) {
      die Exception('number expected')
    }
    
    _imagine.copyrotated(self._ptr, src.get_pointer(), x, y, src_x, src_y, src_width, src_height, angle)
  }

  /**
   * Clones this image resource.
   * 
   * @returns ImageResource
   */
  clone() {
    return ImageResource(_imagine.clone(self._ptr))
  }

  /**
   * Sets the brush image to be used by all line drawing functions for 
   * this image.
   * 
   * A "brush" is an image used to draw wide, shaped strokes in another image. 
   * Just as a paintbrush is not a single point, a brush image need not be a 
   * single pixel. Any image resource can be used as a brush, and by setting 
   * the transparent color index of the brush image with `color_transparent()`, 
   * a brush of any shape can be created. 
   * 
   * All line-drawing functions, such as gdImageLine and `polygon()`, will use 
   * the current brush if the special "color" `COLOR_BRUSHED` or 
   * `COLOR_STYLED_BRUSHED` is used when calling them.
   * 
   * > **NOTE:** 
   * > 
   * > You need not take special action when you are finished with a 
   * > brush, but if you close the brush image (or let the GC close it), 
   * > you must not use the `COLOR_BRUSHED` or `COLOR_STYLED_BRUSHED` colors 
   * > until you have set a new brush image.
   * 
   * @param ImageResource brush
   */
  set_brush(brush) {
    if !instance_of(brush, ImageResource) {
      die Exception('ImageResource expected')
    }
    
    _imagine.setbrush(self._ptr, brush.get_pointer())
  }

  /**
   * Sets the tile image to be used by all region filling functions.
   * 
   * A tile is an image used to fill an area with a repeated pattern. Any image 
   * resource can be used as a tile, and by setting the transparent color index 
   * of the tile image with `color_transparent()`, a tile that allows certain 
   * parts of the underlying area to shine through can be created. All 
   * region-filling functions, such as `fill()` and `filled_polygon()`, will use 
   * the current tile if the special "color" `COLOR_TILED` is used when calling 
   * them.
   * 
   * You can set any image resource to be the tile. If the tile image does not have 
   * the same color map as the first image, any colors missing from the first image 
   * will be allocated. If not enough colors can be allocated, the closest colors 
   * already available will be used. This allows arbitrary GIFs to be used as tile 
   * images. It also means, however, that you should not set a tile unless you will 
   * actually use it; if you set a rapid succession of different tile images, you can 
   * quickly fill your color map, and the results will not be optimal.
   * 
   * You need not take any special action when you are finished with a tile. As for 
   * any other image, if you will not be using the tile image for any further purpose, 
   * you should call `close()`. You must not use the color `COLOR_TILED` if the current 
   * tile has been closed; you can of course set a new tile to replace it.
   * 
   * @param ImageResource tile
   */
  set_tile(tile) {
    if !instance_of(tile, ImageResource) {
      die Exception('ImageResource expected')
    }
    
    _imagine.setbrush(self._ptr, tile.get_pointer())
  }

  /**
   * Set the color for subsequent anti-aliased drawing and whether to blend the 
   * color or not.
   * 
   * @param number color
   * @param bool dont_blend
   */
  set_antialiased(color, dont_blend) {
    if !is_number(color) {
      die Exception('color must be a number')
    }

    if dont_blend == nil dont_blend = false
    if !is_bool(dont_blend) {
      die Exception('dont_blend must be boolean')
    }
    
    _imagine.setantialiased(self._ptr, color, dont_blend ? 1 : 0)
  }

  /**
   * Sets the thickness in pixels for following lines drawn when drawing lines, 
   * ellipses, rectangles, polygons and so forth.
   * 
   * @param number thickness
   */
  set_thickness(thickness) {
    if !is_number(thickness) {
      die Exception('number expected')
    }

    _imagine.setthickness(self._ptr, thickness)
  }

  /**
   * Sets whether an image is interlaced. If the `enabled` parameter is not 
   * given, it defaults to true.
   * 
   * @param bool? enable
   */
  interlace(enable) {
    if enable == nil enable = true

    if !is_bool(enable) {
      die Exception('boolean expected')
    }

    _imagine.interlace(self._ptr, enable ? 1 : 0)
  }

  /**
   * Toggles between two different blending modes of drawing on truecolor images. 
   * 
   * In blending mode, the alpha channel component of the color supplied to all 
   * drawing function, such as `set_pixel()` determines how much of the underlying 
   * color should be allowed to shine through. As a result, the module 
   * automatically blends the existing color at that point with the drawing color, 
   * and stores the result in the image. The resulting pixel is opaque. 
   * 
   * In non-blending mode, the drawing color is copied literally with its alpha 
   * channel information, replacing the destination pixel. Blending mode is not 
   * available when drawing on palette images.
   * 
   * If the `enabled` parameter is not given, it defaults to true.
   * 
   * @param bool enable
   */
  alpha_blending(enable) {
    if enable == nil enable = true

    if !is_bool(enable) {
      die Exception('boolean expected')
    }
    
    _imagine.alphablending(self._ptr, enable ? 1 : 0)
  }

  /**
   * Flips the image horizontally, vertically, or in both direction as specified 
   * in mode. `mode` must be one of the `FLIP_*` constants. When no mode is set, 
   * mode defaults to `FLIP_BOTH`.
   * 
   * @param number? mode
   */
  flip(mode) {
    if !mode mode = flips.FLIP_BOTH

    if !is_number(mode) {
      die Exception('FLIP_* mode constant expected')
    }

    if mode < flips.FLIP_BOTH or mode > flips.FLIP_VERTICAL {
      die Exception('invalid flip mode')
    }

    if mode == flips.FLIP_BOTH {
      _imagine.flip(self._ptr)
    } else if mode == flips.FLIP_HORIZONTAL {
      _imagine.fliphorizontal(self._ptr)
    } else {
      _imagine.flipvertical(self._ptr)
    }
  }

  /**
   * Returns a new imaged cropped from the rectangular area specified by x, y, 
   * width, and height in this image.
   * 
   * @param number x
   * @param number y
   * @param number width
   * @param number height
   * @returns ImageResource
   */
  crop(x, y, width, height) {
    if !is_number(x) or !is_number(y) or !is_number(width) or !is_number(height) {
      die Exception('number expected')
    }

    return ImageResource(_imagine.crop(self._ptr, x, y, width, height))
  }

  /**
   * Crop an image automatically using one of the `CROP_*` modes. If `mode` 
   * is not give, it defaults to `CROP_DEFAULT`.
   * 
   * @param number? mode
   * @returns ImageResource
   */
  auto_crop(mode) {
    if mode == nil mode = crops.CROP_DEFAULT

    if !is_number(mode) {
      die Exception('CROP_* mode constant expected')
    }

    if mode < crops.CROP_DEFAULT or mode > crops.CROP_SIDES {
      die Exception('invalid crop mode')
    }

    return ImageResource(_imagine.cropauto(self._ptr, mode))
  }

  /**
   * Scale an image using the given new width and height with the 
   * interpolation algorithm. If height is not given, the height 
   * will be automatcially calculated from the new width to maitain 
   * aspect ratio. 
   * 
   * If the interpolation method is not given, it defaults to 
   * `INTERP_BILINEAR_FIXED`.
   * 
   * This method returns a new image rather than modify this image.
   * 
   * @param number width
   * @param number? height
   * @param number? method
   * @returns ImageResource
   */
  scale(width, height, method) {
    if height == nil or height == -1 {
      # automatically set height based on width to maintain aspect ratio.
      height = (width / self.meta().width) * self.meta().height
    }

    if !is_number(width) or !is_number(height) {
      die Exception('width and height must be numbers')
    }

    if method == nil method = interpolations.INTERP_BILINEAR_FIXED

    if !is_number(method) or method < interpolations.INTERP_DEFAULT or
        method > interpolations.WELSH {
      die Exception('interpolation must be an INTERP_* constant')
    }

    return ImageResource(_imagine.scale(self._ptr, width, height, method))
  }

  /**
   * Creates a new image rotated counter-clockwise by the requested angle using 
   * the given interpolation method.  Non-square angles will add a border with 
   * bgcolor.
   * 
   * @param number angle
   * @param number bg_color
   * @param number? method
   * @returns ImageResource
   */
  rotate(angle, bg_color, method) {
    if !is_number(angle) or !is_number(bg_color) {
      die Exception('angle and bg_color must be numbers')
    }

    if method == nil method = interpolations.INTERP_BILINEAR_FIXED

    if !is_number(method) or method < interpolations.INTERP_DEFAULT or
        method > interpolations.WELSH {
      die Exception('interpolation must be an INTERP_* constant')
    }

    return ImageResource(_imagine.rotate(self._ptr, angle, bg_color, method))
  }

  /**
   * Sets the save alpha flag
   * 
   * The save alpha flag specifies whether the alpha channel of the pixels should
   * be saved. This is supported only for image formats that support full alpha
   * transparency, e.g. PNG.
   * 
   * @param bool save
   */
  save_alpha(save) {
    if save == nil save = true
    if !is_bool(save)
      die Exception('boolean expected')

    _imagine.savealpha(self._ptr, save ? 1 : 0)
  }

  # ------------------------- FILTERS ------------------------------

  /**
   * Applies pixelation effect to the image based on the block 
   * size and given effect mode.
   * 
   * @param number block_size
   * @param number mode
   */
  pixelate(block_size, mode) {
    if !is_number(block_size) or !is_number(mode)
      die Exception('number expected')

    _imagine.pixelate(self._ptr, block_size, mode)
  }

  /**
   * Applies scatter effect to an image using the _sub_ and _plus_ to 
   * control the strength of the scatter and colors to indicate the 
   * colors it should be restricted to.
   * 
   * @param number sub
   * @param number plus
   * @param list<number> colors
   */
  scatter(sub, plus, colors) {
    if !is_number(sub) or !is_number(plus)
      die Exception('number expected')
    
    if colors != nil {
      if !is_list(colors)
        die Exception('list of color expected in argument 3')
      
      for color in colors {
        if !is_number(color)
          die Exception('invalid color in color list')
      }
    }

    _imagine.scatter(self._ptr, sub, plus, colors)
  }

  /**
   * Makes an image smooter based on the specified weight. If 
   * weight is not given, it defaults to `1`.
   * 
   * @param number weight
   */
  smooth(weight) {
    if weight == nil weight = 1
    if !is_number(weight)
      die Exception('number expected')

    _imagine.smooth(self._ptr, weight)
  }

  /**
   * Uses mean removal to achieve a "sketchy" effect.
   */
  mean_removal() {
    _imagine.meanremoval(self._ptr)
  }

  /**
   * Embosses the image.
   */
  emboss() {
    _imagine.emboss(self._ptr)
  }

  /**
   * Applies a blur to the image. If the type is not given, a 
   * Guassian blur will be applied.
   * 
   * @param number type
   */
  blur(type) {
    if type == nil type = blurs.BLUR_GAUSSIAN
    if !is_number(type) or !(type != blurs.BLUR_GAUSSIAN and type != blurs.BLUR_SELECTIVE) {
      die Exception('BLUR_* constant expected')
    }

    if type == blurs.BLUR_GAUSSIAN {
      _imagine.gaussianblur(self._ptr)
    } else {
      _imagine.selectiveblur(self._ptr)
    }
  }

  /**
   * Uses edge detection to highlight the edges in the image.
   */
  detect_edge() {
    _imagine.edgedetect(self._ptr)
  }

  /**
   * Converts the image into grayscale by changing the red, green 
   * and blue components to their weighted sum using the same 
   * coefficients as the REC.601 luma (Y') calculation. The alpha 
   * components are retained. For palette images the result may 
   * differ due to palette limitations.
   */
  grayscale() {
    _imagine.grayscale(self._ptr)
  }

  /**
   * Reverses all colors of the image to create a negative image.
   */
  negate() {
    _imagine.negate(self._ptr)
  }

  /**
   * Same as `grayscale()` except this allows you to specify the 
   * output color.
   * 
   * @param number r
   * @param number g
   * @param number b
   * @param number a
   */
  color(r, g, b, a) {
    if !is_number(r) or !is_number(g) or !is_number(b) or !is_number(a) {
      die Exception('number expected')
    }

    _imagine.color(self._ptr, r, g, b, a)
  }

  /**
   * Changes the contrast of the image based on the level set 
   * in _contrast_.
   * 
   * @param number contrast
   */
  contrast(contrast) {
    if !is_number(contrast)
      die Exception('number expected')

    _imagine.contrast(self._ptr, contrast)
  }

  /**
   * Changes the brightness of the image based on the level set 
   * in _brightness_.
   * 
   * @param number brightness
   */
  brightness(brightness) {
    if !is_number(brightness)
      die Exception('number expected')

    _imagine.brightness(self._ptr, brightness)
  }

  # ------------------------- MISC ------------------------------

  /**
   * Sets the rectangular clipping region beyond which no pixels 
   * will be drawn in the image.
   * 
   * @param number x1
   * @param number y1
   * @param number x2
   * @param number y2
   */
  set_clip(x1, y1, x2, y2) {
    if !is_number(x1) or !is_number(y1) or !is_number(x2) or !is_number(y2) {
      die Exception('number expected')
    }

    _imagine.setclip(self._ptr, x1, y1, x2, y2)
  }

  /**
   * Returns the clipping region in the image. See `set_clip()`.
   * 
   * The function returns a list containing four numbers that 
   * indicates the x1, y1, x2, and y2 of the clipping region in 
   * the image.
   * 
   * @returns list<number>
   */
  get_clip() {
    return _imagine.getclip(self._ptr)
  }

  /**
   * Sets the resolution of the the image across both axis.
   * 
   * @param number res_x
   * @param number res_y
   */
  set_resolution(res_x, res_y) {
    if !is_number(res_x) or !is_number(res_y)
      die Exception('number expected')
    
    _imagine.setresolution(self._ptr, res_x, res_y)

    # Update the meta here since part of the information in the 
    # meta includes res_x and res_y. If we do not do this, the 
    # image metadata will be invalid and out of sync.
    # 
    # Why we didn't just update self._meta?
    # Because it could be nil. Which will essentially make us 
    # re-implement meta() except that function uses a cache so 
    # it can't be updated directly without first updating the 
    # underlying data. So, we opted to just update the 
    # underlying data.
    self._meta = _imagine.meta(self._ptr)
  }

  /**
   * Convert a true color image to a palette image. 
   * 
   * The first parameter `dither` controls whether the image 
   * should be dithered which results in a more speckled image but 
   * with better color approximation. 
   * 
   * The second argument `colors_wanted` controls the number of 
   * colors that should be kept in the palette.
   * 
   * @param bool dither
   * @param number colors_wanted
   * @returns bool - `true` if successful, otherwise `false`.
   */
  true_color_to_palette(dither, colors_wanted) {
    if !is_bool(dither_flag)
      die Exception('boolean expected as first argument')
    else if !is_number(colors_wanted)
      die Exception('number expected as second argument')
    
    var result = _imagine.truecolortopalette(self._ptr, dither_flag ? 1 : 0, colors_wanted)

    # Update the meta here since this function affects its true_color status.
    self._meta = _imagine.meta(self._ptr)
    return result == 1
  }

  /**
   * Converts a palette based image to true color.
   * 
   * @returns bool - `true` if successful, otherwise `false`.
   */
  palette_to_true_color() {
    var result = _imagine.palettetotruecolor(self._ptr)

    # Update the meta here since this function affects its true_color status.
    self._meta = _imagine.meta(self._ptr)
    return result == 1
  }

  /**
   * Makes the colors of the palette version of an image more closely 
   * match the true color version. This function should be given a 
   * true color image as the function will attempt to make the color 
   * of the image given if the current image is a paletted image.
   * 
   * @param ImageResource image
   * @returns bool - `true` if successful, otherwise `false`.
   */
  match_color(image) {
    if !instance_of(image, ImageResource) {
      die Exception('ImageResource expected')
    }

    if !image.meta().true_color {
      die Exception('true color image expected')
    }

    return _imagine.colormatch(image.get_pointer(), self._ptr) == 1
  }

  /**
   * Check whether two images are idential.
   * 
   * This check includes a size, transparency, interlace, color profile, 
   * and a pixel by pixel check.
   * 
   * If the images are completely identical, the method returns a zero 
   * (`0`). Otherwise, it returns a number greater than 0. The number 
   * returned can be tested againt the various `CMP_*` constants to test 
   * for any of the conditions.
   * 
   * For example,
   * 
   * ```blade
   * var result = image1.compare(image2)
   * 
   * var both_transparent = !(result & CMP_TRANSPARENT)
   * var same_width = !(result & CMP_SIZE_X)
   * ```
   * 
   * @param ImageResource image
   * @returns number
   */
  compare(image) {
    if !instance_of(image, ImageResource) {
      die Exception('ImageResource expected')
    }

    return _imagine.imagecompare(self._ptr, image.get_pointer())
  }

  # ------------------------- EXPORT ------------------------------

  /**
   * Saves the image to file with the PNG format.
   * 
   * Quality level: 0-10, where 9 is NO COMPRESSION at all,
   * 9 is FASTEST but produces larger files, 0 provides the best
   * compression (smallest files) but takes a long time to compress, and
   * 10 selects the default compiled into the zlib library.
   * 
   * @param string|file dest
   * @param number quality
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
   * @param string|file dest
   * @param number quality
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
   * @param string|file dest
   * @param number quality
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
   * @param string|file dest
   * @param number foreground
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
   * @param string|file dest
   * @param number quantization
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
   * @param string|file dest
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
   * @param string|file dest
   * @param number quality
   * @param number speed - Default = 1
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

  # ----------- POINTER ---------------

  /**
   * Returns the raw image resource pointer.
   * 
   * @returns ptr
   */
  get_pointer() {
    return self._ptr
  }
}
