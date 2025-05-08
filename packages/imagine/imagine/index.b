/**
 * @module imagine
 * 
 * This module provide classes and functions for dynamic image creation 
 * and manipulation. `Imagine` supports different image formats and can 
 * be used to generate thumbnails, charts, graphics, and many other 
 * kinds of images on the fly.
 * 
 * The following image formats are currently supported by `imagine`:
 * 
 * - JPEG
 * - PNG
 * - GIF
 * - BMP
 * - AVIF
 * - WebP
 * - HEIF
 * - TIFF
 * - WBMP
 * - TGA
 * 
 * ## Features
 * 
 * The module supports transparency, blending, images and image text 
 * transformations and various filters.
 * 
 * - ⁠Image loading and saving in various formats (JPEG, PNG, GIF, etc.)
 * - ⁠Image resizing, cropping, and rotation
 * - Color manipulation (brightness, contrast, saturation, etc.)
 * - Image filtering (blur, sharpen, edge detection, etc.)
 * - Text rendering and drawing
 * - Support for layers, masks, and alpha channels
 * 
 * ## Examples
 * 
 * The following create a PNG image filled with color red.
 * 
 * ```blade
 * import imagine { * }
 * 
 * # create empty image handle
 * var img = Image.new(100, 100, true)
 * 
 * # allocate color red and use it to fill the image
 * var bg_color = img.allocate_color(255, 0, 0)
 * img.fill(0, 0, bg_color)
 * 
 * # export image to png
 * img.export_png('image.png')
 * 
 * # close the image handle
 * img.close()
 * ```
 * 
 * While the above example works very fine, it is a very common thing 
 * from experience for people to forget to close file and image handles 
 * and this have real implications on the system. For this reason, the 
 * coventionally advice way to use `Image` instances is via 
 * .[[imagine.ImageResource.use()]]. 
 * 
 * The example below demonstartes the former example with `.use` pattern.
 * 
 * ```blade
 * import imagine { * }
 * 
 * Image.new(100, 100, true).use(@(img) {
 *   # allocate color red and use it to fill the image
 *   var bg_color = img.allocate_color(255, 0, 0)
 *   img.fill(0, 0, bg_color)
 * 
 *   # export image to png
 *   img.export_png('image.png')
 * })
 * ```
 * 
 * Imagine is great at converting images as well as generating images. 
 * The example below loads a PNG image and saves a copy as a JPEG file 
 * (for sake of continuity, we're using the image we just created but feel 
 * free to play around with your own images).
 * 
 * ```blade
 * import imagine { * }
 * 
 * Image.from_png('image.png').use(@(img) {
 *   img.export_jpeg('image.jpg')
 * })
 * ```
 * 
 * Image can create transparent images as well as images containig texts. 
 * The example below shows creates a simple transparent PNG image with the 
 * text `A simple text string` written in it. 
 * 
 * ```blade
 * import imagine { * }
 * 
 * Image.new(130, 20, true).use(@(im) {
 *   im.save_alpha()
 * 
 *   var bg_color = im.allocate_color(0, 0, 0, 127)
 *   var fore_color = im.allocate_color(233, 14, 91)
 * 
 *   im.fill(0, 0, bg_color)
 *   im.string(5, 2, 'A simple text string', FONT_REGULAR, fore_color)
 * 
 *   im.export_png('simple.png')
 * })
 * ```
 * 
 * @copyright 2021, Richard Ore and Blade contributors
 */


import .fonts { * }
import .comparisons { * }
import .interpolations { * }
import .quants { * }
import .crops { * }
import .arcs { * }
import .colors { * }
import .image { * }


/**
 * Compose a truecolor value from its components.
 * 
 * @param number? r: The red channel (0-255) - Default: 0
 * @param number? g: The green channel (0-255) - Default: 0
 * @param number? b: The blue channel (0-255) - Default: 0
 * @param number? a: The alpha channel (0-127, where 127 is fully transparent, and 0 is completely opaque) - Default: 0.
 * @returns number
 */
def true_color(r, g, b, a) {
  if r == nil r = 0
  if g == nil g = 0
  if b == nil b = 0
  if a == nil a = 0

  if !is_number(r) or !is_number(g) or !is_number(b) or !is_number(a) {
    raise TypeError('number expected')
  }

  if a == 0 {
    return (a << 24) + (r << 16) + (g << 8) + b
  } else {
    return (r << 16) + (g << 8) + b
  }
}


/**
 * Decomposes an Image true color number into it's respective 
 * RGBA components.
 * 
 * The function returns a dictionary that contains the following 
 * decomposed items:
 * 
 * - `r` - The red channel value
 * - `g` - The green channel value
 * - `b` - The blue channel value
 * - `a` - The alpha channel value
 * 
 * @param number color
 * @returns dict
 */
def decompose(color) {
  var r = (c & 0xFF0000) >> 16
  var g = (c & 0x00FF00) >> 8
  var b = (c & 0x0000FF)
  var a = (c & 0x7F000000) >> 24

  return { r, g, b, a}
}

/**
 * Creates an image from any supported image file.
 * 
 * As long as the file type is supported by Imagine, the file type 
 * will automatically be detected.
 * 
 * This function is a shorthand for [[imagine.Image.from_file]].
 * 
 * @param string|file src
 * @returns [[imagine.ImageResource]]
 */
def load(path) {
  return Image.from_file(path)
}
