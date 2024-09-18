#include <blade.h>
#include <errno.h>

// gd
#include <gd.h>

// fonts
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontg.h>
#include <gdfontmb.h>
#include <gdfontt.h>

#define CHECK_IMAGE_PTR(x) if((x) == NULL) \
    RETURN_ERROR("Invalid image pointer.") \

#define CHECK_FONT_PTR(x) if((x) == NULL) \
    RETURN_ERROR("Invalid font pointer.") \

#define CHECK_IMAGE(x) if((x) == NULL) \
    RETURN_ERROR(strerror(errno)) \

#define IMAGINE_CONST(v) \
  b_value __imagine_const_##v(b_vm *vm) { \
    return NUMBER_VAL(GD_##v); \
  }

#define IMAGINE_PTR(f, v, t) \
  b_value __imagine_ptr_##f(b_vm *vm) { \
    b_obj_ptr *ptr = (b_obj_ptr *)GC(new_ptr(vm, (v))); \
    ptr->name = "<void *imagine::type::" #t ">"; \
    return OBJ_VAL(ptr); \
  }

#define GET_IMAGINE_CONST(v) \
  {#v, true, __imagine_const_##v}

#define GET_IMAGINE_PTR(v) \
  {#v, true, __imagine_ptr_##v}

#define IMAGINE_IMAGE_PTR_NAME "<void *imagine::type::image>"

static void imagine_free_image_ptrs(void *data) {
  if(data != NULL) {
    gdImagePtr image = (gdImagePtr)data;
    gdImageDestroy(image);
    data = NULL;
  }
}

IMAGINE_PTR(tinyfont, gdFontGetTiny(), font);
IMAGINE_PTR(smallfont, gdFontGetSmall(), font);
IMAGINE_PTR(mediumfont, gdFontGetMediumBold(), font);
IMAGINE_PTR(largefont, gdFontGetLarge(), font);
IMAGINE_PTR(giantfont, gdFontGetGiant(), font);

IMAGINE_CONST(PIXELATE_UPPERLEFT);
IMAGINE_CONST(PIXELATE_AVERAGE);

IMAGINE_CONST(QUANT_DEFAULT);
IMAGINE_CONST(QUANT_JQUANT);
IMAGINE_CONST(QUANT_NEUQUANT);
IMAGINE_CONST(QUANT_LIQ);

IMAGINE_CONST(DEFAULT);
IMAGINE_CONST(BELL);
IMAGINE_CONST(BESSEL);
IMAGINE_CONST(BILINEAR_FIXED);
IMAGINE_CONST(BICUBIC);
IMAGINE_CONST(BICUBIC_FIXED);
IMAGINE_CONST(BLACKMAN);
IMAGINE_CONST(BOX);
IMAGINE_CONST(BSPLINE);
IMAGINE_CONST(CATMULLROM);
IMAGINE_CONST(GAUSSIAN);
IMAGINE_CONST(GENERALIZED_CUBIC);
IMAGINE_CONST(HERMITE);
IMAGINE_CONST(HAMMING);
IMAGINE_CONST(HANNING);
IMAGINE_CONST(MITCHELL);
IMAGINE_CONST(NEAREST_NEIGHBOUR);
IMAGINE_CONST(POWER);
IMAGINE_CONST(QUADRATIC);
IMAGINE_CONST(SINC);
IMAGINE_CONST(TRIANGLE);
IMAGINE_CONST(WEIGHTED4);
IMAGINE_CONST(LINEAR);
IMAGINE_CONST(LANCZOS3);
IMAGINE_CONST(LANCZOS8);
IMAGINE_CONST(BLACKMAN_BESSEL);
IMAGINE_CONST(BLACKMAN_SINC);
IMAGINE_CONST(QUADRATIC_BSPLINE);
IMAGINE_CONST(CUBIC_SPLINE);
IMAGINE_CONST(COSINE);
IMAGINE_CONST(WELSH);
IMAGINE_CONST(METHOD_COUNT);

IMAGINE_CONST(CROP_DEFAULT);
IMAGINE_CONST(CROP_TRANSPARENT);
IMAGINE_CONST(CROP_BLACK);
IMAGINE_CONST(CROP_WHITE);
IMAGINE_CONST(CROP_SIDES);
IMAGINE_CONST(CROP_THRESHOLD);

IMAGINE_CONST(CMP_IMAGE);
IMAGINE_CONST(CMP_NUM_COLORS);
IMAGINE_CONST(CMP_COLOR);
IMAGINE_CONST(CMP_SIZE_X);
IMAGINE_CONST(CMP_SIZE_Y);
IMAGINE_CONST(CMP_TRANSPARENT);
IMAGINE_CONST(CMP_BACKGROUND);
IMAGINE_CONST(CMP_INTERLACE);
IMAGINE_CONST(CMP_TRUECOLOR);

DECLARE_MODULE_METHOD(imagine__new) {
  ENFORCE_ARG_COUNT(new, 3);
  ENFORCE_ARG_TYPE(new, 0, IS_NUMBER);
  ENFORCE_ARG_TYPE(new, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(new, 2, IS_BOOL);

  bool create_true_colors = AS_BOOL(args[2]);

  gdImagePtr image;
  if(create_true_colors) {
    image = gdImageCreateTrueColor(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  } else {
    image = gdImageCreate(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  }

  if(NULL == image || image == 0) {
    if(create_true_colors) {
      RETURN_ERROR("Failed to create image with truecolors.");
    } else {
      RETURN_ERROR("Failed to create image.");
    }
  }

  RETURN_CLOSABLE_NAMED_PTR(image, IMAGINE_IMAGE_PTR_NAME, imagine_free_image_ptrs);
}

DECLARE_MODULE_METHOD(imagine__frompng) {
  ENFORCE_ARG_COUNT(frompng, 1);
  ENFORCE_ARG_TYPE(frompng, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromPng(AS_FILE(args[0])->file);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__fromjpeg) {
  ENFORCE_ARG_COUNT(fromjpeg, 1);
  ENFORCE_ARG_TYPE(fromjpeg, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromJpeg(AS_FILE(args[0])->file);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__fromgif) {
  ENFORCE_ARG_COUNT(fromgif, 1);
  ENFORCE_ARG_TYPE(fromgif, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromGif(AS_FILE(args[0])->file);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__frombmp) {
  ENFORCE_ARG_COUNT(frombmp, 1);
  ENFORCE_ARG_TYPE(frombmp, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromBmp(AS_FILE(args[0])->file);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__fromwbmp) {
  ENFORCE_ARG_COUNT(fromwbmp, 1);
  ENFORCE_ARG_TYPE(fromwbmp, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromWBMP(AS_FILE(args[0])->file);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__fromwxbm) {
  ENFORCE_ARG_COUNT(fromwxbm, 1);
  ENFORCE_ARG_TYPE(fromwxbm, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromXbm(AS_FILE(args[0])->file);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__fromwtga) {
  ENFORCE_ARG_COUNT(fromwtga, 1);
  ENFORCE_ARG_TYPE(fromwtga, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromTga(AS_FILE(args[0])->file);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__fromfile) {
  ENFORCE_ARG_COUNT(fromfile, 1);
  ENFORCE_ARG_TYPE(fromfile, 0, IS_STRING);

  gdImagePtr image = gdImageCreateFromFile(AS_C_STRING(args[0]));
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__close) {
  ENFORCE_ARG_COUNT(close, 1);
  ENFORCE_ARG_TYPE(close, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageDestroy(image);
  AS_PTR(args[0])->pointer = NULL;
  
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__setpixel) {
  ENFORCE_ARG_COUNT(setpixel, 4);
  ENFORCE_ARG_TYPE(setpixel, 0, IS_PTR);
  ENFORCE_ARG_TYPE(setpixel, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(setpixel, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(setpixel, 3, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageSetPixel(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__getpixel) {
  ENFORCE_ARG_COUNT(getpixel, 4);
  ENFORCE_ARG_TYPE(getpixel, 0, IS_PTR);
  ENFORCE_ARG_TYPE(getpixel, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(getpixel, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(getpixel, 3, IS_BOOL); // use true color

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  if(AS_BOOL(args[3])) {
    RETURN_NUMBER(gdImageGetTrueColorPixel(image, AS_NUMBER(args[1]), AS_NUMBER(args[2])));
  } else {
    RETURN_NUMBER(gdImageGetPixel(image, AS_NUMBER(args[1]), AS_NUMBER(args[2])));
  }
}

DECLARE_MODULE_METHOD(imagine__line) {
  ENFORCE_ARG_COUNT(line, 6);
  ENFORCE_ARG_TYPE(line, 0, IS_PTR);
  ENFORCE_ARG_TYPE(line, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(line, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(line, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(line, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(line, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageLine(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__dashedline) {
  ENFORCE_ARG_COUNT(dashedline, 6);
  ENFORCE_ARG_TYPE(dashedline, 0, IS_PTR);
  ENFORCE_ARG_TYPE(dashedline, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(dashedline, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(dashedline, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(dashedline, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(dashedline, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageDashedLine(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__rectangle) {
  ENFORCE_ARG_COUNT(rectangle, 6);
  ENFORCE_ARG_TYPE(rectangle, 0, IS_PTR);
  ENFORCE_ARG_TYPE(rectangle, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(rectangle, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(rectangle, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(rectangle, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(rectangle, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageRectangle(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__filledrectangle) {
  ENFORCE_ARG_COUNT(filledrectangle, 6);
  ENFORCE_ARG_TYPE(filledrectangle, 0, IS_PTR);
  ENFORCE_ARG_TYPE(filledrectangle, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledrectangle, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledrectangle, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledrectangle, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledrectangle, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageFilledRectangle(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__setclip) {
  ENFORCE_ARG_COUNT(setclip, 5);
  ENFORCE_ARG_TYPE(setclip, 0, IS_PTR);
  ENFORCE_ARG_TYPE(setclip, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(setclip, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(setclip, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(setclip, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageSetClip(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__getclip) {
  ENFORCE_ARG_COUNT(getclip, 1);
  ENFORCE_ARG_TYPE(getclip, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  int x1P = 0, y1P = 0, x2P = 0, y2P = 0;
  gdImageGetClip(image, &x1P, &y1P, &x2P, &y2P);

  b_obj_list *list = (b_obj_list*)GC(new_list(vm));
  write_list(vm, list, NUMBER_VAL(x1P));
  write_list(vm, list, NUMBER_VAL(y1P));
  write_list(vm, list, NUMBER_VAL(x2P));
  write_list(vm, list, NUMBER_VAL(y2P));

  RETURN_OBJ(list);
}

DECLARE_MODULE_METHOD(imagine__setresolution) {
  ENFORCE_ARG_COUNT(setresolution, 3);
  ENFORCE_ARG_TYPE(setresolution, 0, IS_PTR);
  ENFORCE_ARG_TYPE(setresolution, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(setresolution, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageSetResolution(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__boundsafe) {
  ENFORCE_ARG_COUNT(boundsafe, 3);
  ENFORCE_ARG_TYPE(boundsafe, 0, IS_PTR);
  ENFORCE_ARG_TYPE(boundsafe, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(boundsafe, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageBoundsSafe(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__char) {
  ENFORCE_ARG_COUNT(char, 6);
  ENFORCE_ARG_TYPE(char, 0, IS_PTR);
  ENFORCE_ARG_TYPE(char, 1, IS_PTR);
  ENFORCE_ARG_TYPE(char, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(char, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(char, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(char, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdFontPtr font = (gdFontPtr)AS_PTR(args[1])->pointer;
  CHECK_FONT_PTR(font);

  gdImageChar(
    image, 
    font, 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__charup) {
  ENFORCE_ARG_COUNT(charup, 6);
  ENFORCE_ARG_TYPE(charup, 0, IS_PTR);
  ENFORCE_ARG_TYPE(charup, 1, IS_PTR);
  ENFORCE_ARG_TYPE(charup, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(charup, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(charup, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(charup, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdFontPtr font = (gdFontPtr)AS_PTR(args[1])->pointer;
  CHECK_FONT_PTR(font);

  gdImageCharUp(
    image, 
    font, 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__string) {
  ENFORCE_ARG_COUNT(string, 6);
  ENFORCE_ARG_TYPE(string, 0, IS_PTR);
  ENFORCE_ARG_TYPE(string, 1, IS_PTR);
  ENFORCE_ARG_TYPE(string, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(string, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(string, 4, IS_STRING);
  ENFORCE_ARG_TYPE(string, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdFontPtr font = (gdFontPtr)AS_PTR(args[1])->pointer;
  CHECK_FONT_PTR(font);

  gdImageString(
    image, 
    font, 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    (unsigned char *)AS_C_STRING(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__stringup) {
  ENFORCE_ARG_COUNT(stringup, 6);
  ENFORCE_ARG_TYPE(stringup, 0, IS_PTR);
  ENFORCE_ARG_TYPE(stringup, 1, IS_PTR);
  ENFORCE_ARG_TYPE(stringup, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(stringup, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(stringup, 4, IS_STRING);
  ENFORCE_ARG_TYPE(stringup, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdFontPtr font = (gdFontPtr)AS_PTR(args[1])->pointer;
  CHECK_FONT_PTR(font);

  gdImageStringUp(
    image, 
    font, 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    (unsigned char *)AS_C_STRING(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__filledarc) {
  ENFORCE_ARG_COUNT(filledarc, 9);
  ENFORCE_ARG_TYPE(filledarc, 0, IS_PTR);
  ENFORCE_ARG_TYPE(filledarc, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledarc, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledarc, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledarc, 4, IS_STRING);
  ENFORCE_ARG_TYPE(filledarc, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledarc, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledarc, 7, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledarc, 8, IS_NUMBER); // style

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  // TODO: Define styles In Blade source
  // #define gdArc   0
  // #define gdPie   gdArc
  // #define gdChord 1
  // #define gdNoFill 2
  // #define gdEdged 4

  gdImageFilledArc(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7]),
    AS_NUMBER(args[8])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__arc) {
  ENFORCE_ARG_COUNT(arc, 8);
  ENFORCE_ARG_TYPE(arc, 0, IS_PTR);
  ENFORCE_ARG_TYPE(arc, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(arc, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(arc, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(arc, 4, IS_STRING);
  ENFORCE_ARG_TYPE(arc, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(arc, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(arc, 7, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageArc(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__ellipse) {
  ENFORCE_ARG_COUNT(ellipse, 6);
  ENFORCE_ARG_TYPE(ellipse, 0, IS_PTR);
  ENFORCE_ARG_TYPE(ellipse, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(ellipse, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(ellipse, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(ellipse, 4, IS_STRING);
  ENFORCE_ARG_TYPE(ellipse, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageEllipse(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__filledellipse) {
  ENFORCE_ARG_COUNT(filledellipse, 6);
  ENFORCE_ARG_TYPE(filledellipse, 0, IS_PTR);
  ENFORCE_ARG_TYPE(filledellipse, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledellipse, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledellipse, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(filledellipse, 4, IS_STRING);
  ENFORCE_ARG_TYPE(filledellipse, 5, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageFilledEllipse(
    image, 
    AS_NUMBER(args[1]), 
    AS_NUMBER(args[2]), 
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__polygon) {
  ENFORCE_ARG_COUNT(polygon, 3);
  ENFORCE_ARG_TYPE(polygon, 0, IS_PTR);
  ENFORCE_ARG_TYPE(polygon, 1, IS_LIST);
  ENFORCE_ARG_TYPE(polygon, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  b_obj_list *points_given = AS_LIST(args[1]);

  gdPointPtr points = ALLOCATE(gdPoint, points_given->items.count);

  for(int i = 0; i < points_given->items.count; i++) {
    if(!IS_LIST(points_given->items.values[i])) {
      FREE_ARRAY(gdPoint, points, points_given->items.count);
      RETURN_ERROR("invalid points data.");
    }

    b_obj_list *point = AS_LIST(points_given->items.values[i]);
    if(point->items.count != 2) {
      FREE_ARRAY(gdPoint, points, points_given->items.count);
      RETURN_ERROR("invalid points data.");
    }

    for(int j = 0; j < point->items.count; j++) {
      if(!IS_NUMBER(point->items.values[j])) {
        FREE_ARRAY(gdPoint, points, points_given->items.count);
        RETURN_ERROR("invalid points data.");
      }
    }

    points[i] = (gdPoint){ .x = point->items.values[0], .y = point->items.values[1] };
  }

  gdImagePolygon(image, points, points_given->items.count, AS_NUMBER(args[2]));
  FREE_ARRAY(gdPoint, points, points_given->items.count);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__openpolygon) {
  ENFORCE_ARG_COUNT(openpolygon, 3);
  ENFORCE_ARG_TYPE(openpolygon, 0, IS_PTR);
  ENFORCE_ARG_TYPE(openpolygon, 1, IS_LIST);
  ENFORCE_ARG_TYPE(openpolygon, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  b_obj_list *points_given = AS_LIST(args[1]);

  gdPointPtr points = ALLOCATE(gdPoint, points_given->items.count);

  for(int i = 0; i < points_given->items.count; i++) {
    if(!IS_LIST(points_given->items.values[i])) {
      FREE_ARRAY(gdPoint, points, points_given->items.count);
      RETURN_ERROR("invalid points data.");
    }

    b_obj_list *point = AS_LIST(points_given->items.values[i]);
    if(point->items.count != 2) {
      FREE_ARRAY(gdPoint, points, points_given->items.count);
      RETURN_ERROR("invalid points data.");
    }

    for(int j = 0; j < point->items.count; j++) {
      if(!IS_NUMBER(point->items.values[j])) {
        FREE_ARRAY(gdPoint, points, points_given->items.count);
        RETURN_ERROR("invalid points data.");
      }
    }

    points[i] = (gdPoint){ .x = point->items.values[0], .y = point->items.values[1] };
  }

  gdImageOpenPolygon(image, points, points_given->items.count, AS_NUMBER(args[2]));
  FREE_ARRAY(gdPoint, points, points_given->items.count);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__filledpolygon) {
  ENFORCE_ARG_COUNT(filledpolygon, 3);
  ENFORCE_ARG_TYPE(filledpolygon, 0, IS_PTR);
  ENFORCE_ARG_TYPE(filledpolygon, 1, IS_LIST);
  ENFORCE_ARG_TYPE(filledpolygon, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  b_obj_list *points_given = AS_LIST(args[1]);

  gdPointPtr points = ALLOCATE(gdPoint, points_given->items.count);

  for(int i = 0; i < points_given->items.count; i++) {
    if(!IS_LIST(points_given->items.values[i])) {
      FREE_ARRAY(gdPoint, points, points_given->items.count);
      RETURN_ERROR("invalid points data.");
    }

    b_obj_list *point = AS_LIST(points_given->items.values[i]);
    if(point->items.count != 2) {
      FREE_ARRAY(gdPoint, points, points_given->items.count);
      RETURN_ERROR("invalid points data.");
    }

    for(int j = 0; j < point->items.count; j++) {
      if(!IS_NUMBER(point->items.values[j])) {
        FREE_ARRAY(gdPoint, points, points_given->items.count);
        RETURN_ERROR("invalid points data.");
      }
    }

    points[i] = (gdPoint){ .x = point->items.values[0], .y = point->items.values[1] };
  }

  gdImageFilledPolygon(image, points, points_given->items.count, AS_NUMBER(args[2]));
  FREE_ARRAY(gdPoint, points, points_given->items.count);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__colorallocate) {
  ENFORCE_ARG_COUNT(colorallocate, 4);
  ENFORCE_ARG_TYPE(colorallocate, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorallocate, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorallocate, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorallocate, 3, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorAllocate(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3])));
}

DECLARE_MODULE_METHOD(imagine__colorallocatealpha) {
  ENFORCE_ARG_COUNT(colorallocatealpha, 5);
  ENFORCE_ARG_TYPE(colorallocatealpha, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorallocatealpha, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorallocatealpha, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorallocatealpha, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorallocatealpha, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorAllocateAlpha(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3]), AS_NUMBER(args[4])));
}

DECLARE_MODULE_METHOD(imagine__colorclosest) {
  ENFORCE_ARG_COUNT(colorclosest, 4);
  ENFORCE_ARG_TYPE(colorclosest, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorclosest, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorclosest, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorclosest, 3, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorClosest(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3])));
}

DECLARE_MODULE_METHOD(imagine__colorclosestalpha) {
  ENFORCE_ARG_COUNT(colorclosestalpha, 5);
  ENFORCE_ARG_TYPE(colorclosestalpha, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorclosestalpha, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorclosestalpha, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorclosestalpha, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorclosestalpha, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorClosestAlpha(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3]), AS_NUMBER(args[4])));
}

DECLARE_MODULE_METHOD(imagine__colorclosesthwb) {
  ENFORCE_ARG_COUNT(colorclosesthwb, 4);
  ENFORCE_ARG_TYPE(colorclosesthwb, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorclosesthwb, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorclosesthwb, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorclosesthwb, 3, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorClosestHWB(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3])));
}

DECLARE_MODULE_METHOD(imagine__colorexact) {
  ENFORCE_ARG_COUNT(colorexact, 4);
  ENFORCE_ARG_TYPE(colorexact, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorexact, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorexact, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorexact, 3, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorExact(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3])));
}

DECLARE_MODULE_METHOD(imagine__colorexactalpha) {
  ENFORCE_ARG_COUNT(colorexactalpha, 5);
  ENFORCE_ARG_TYPE(colorexactalpha, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorexactalpha, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorexactalpha, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorexactalpha, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorexactalpha, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorExactAlpha(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3]), AS_NUMBER(args[4])));
}

DECLARE_MODULE_METHOD(imagine__colorresolve) {
  ENFORCE_ARG_COUNT(colorresolve, 4);
  ENFORCE_ARG_TYPE(colorresolve, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorresolve, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorresolve, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorresolve, 3, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorResolve(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3])));
}

DECLARE_MODULE_METHOD(imagine__colorresolvealpha) {
  ENFORCE_ARG_COUNT(colorresolvealpha, 5);
  ENFORCE_ARG_TYPE(colorresolvealpha, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorresolvealpha, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorresolvealpha, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorresolvealpha, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorresolvealpha, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorResolveAlpha(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3]), AS_NUMBER(args[4])));
}

DECLARE_MODULE_METHOD(imagine__colordeallocate) {
  ENFORCE_ARG_COUNT(colorresolvealpha, 2);
  ENFORCE_ARG_TYPE(colorresolvealpha, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorresolvealpha, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  gdImageColorDeallocate(image, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__truecolortopalette) {
  ENFORCE_ARG_COUNT(truecolortopalette, 3);
  ENFORCE_ARG_TYPE(truecolortopalette, 0, IS_PTR);
  ENFORCE_ARG_TYPE(truecolortopalette, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(truecolortopalette, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageTrueColorToPalette(image, AS_NUMBER(args[1]), AS_NUMBER(args[2])));
}

DECLARE_MODULE_METHOD(imagine__palettetotruecolor) {
  ENFORCE_ARG_COUNT(palettetotruecolor, 1);
  ENFORCE_ARG_TYPE(truecolortopalette, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImagePaletteToTrueColor(image));
}

DECLARE_MODULE_METHOD(imagine__colortransparent) {
  ENFORCE_ARG_COUNT(colortransparent, 2);
  ENFORCE_ARG_TYPE(colortransparent, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colortransparent, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  gdImageColorTransparent(image, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__palettecopy) {
  ENFORCE_ARG_COUNT(palettecopy, 2);
  ENFORCE_ARG_TYPE(palettecopy, 0, IS_PTR);
  ENFORCE_ARG_TYPE(palettecopy, 1, IS_PTR);

  gdImagePtr image1 = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image1);

  gdImagePtr image2 = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(image2);
  
  gdImagePaletteCopy(image1, image2);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__colorreplace) {
  ENFORCE_ARG_COUNT(colorreplace, 3);
  ENFORCE_ARG_TYPE(colorreplace, 0, IS_PTR);
  ENFORCE_ARG_TYPE(colorreplace, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(colorreplace, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  RETURN_NUMBER(gdImageColorReplace(image, AS_NUMBER(args[1]), AS_NUMBER(args[2])));
}

DECLARE_MODULE_METHOD(imagine__gif) {
  ENFORCE_ARG_COUNT(gif, 2);
  ENFORCE_ARG_TYPE(gif, 0, IS_PTR);
  ENFORCE_ARG_TYPE(gif, 1, IS_FILE);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageGif(image, AS_FILE(args[1])->file);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__png) {
  ENFORCE_ARG_COUNT(png, 3);
  ENFORCE_ARG_TYPE(png, 0, IS_PTR);
  ENFORCE_ARG_TYPE(png, 1, IS_FILE);
  ENFORCE_ARG_TYPE(png, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImagePngEx(image, AS_FILE(args[1])->file, AS_NUMBER(args[2]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__jpeg) {
  ENFORCE_ARG_COUNT(jpeg, 3);
  ENFORCE_ARG_TYPE(jpeg, 0, IS_PTR);
  ENFORCE_ARG_TYPE(jpeg, 1, IS_FILE);
  ENFORCE_ARG_TYPE(jpeg, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageJpeg(image, AS_FILE(args[1])->file, AS_NUMBER(args[2]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__bmp) {
  ENFORCE_ARG_COUNT(bmp, 3);
  ENFORCE_ARG_TYPE(bmp, 0, IS_PTR);
  ENFORCE_ARG_TYPE(bmp, 1, IS_FILE);
  ENFORCE_ARG_TYPE(bmp, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageBmp(image, AS_FILE(args[1])->file, AS_NUMBER(args[2]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__wbmp) {
  ENFORCE_ARG_COUNT(wbmp, 3);
  ENFORCE_ARG_TYPE(wbmp, 0, IS_PTR);
  ENFORCE_ARG_TYPE(wbmp, 1, IS_FILE);
  ENFORCE_ARG_TYPE(wbmp, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageWBMP(image, AS_NUMBER(args[2]), AS_FILE(args[1])->file);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__filltoborder) {
  ENFORCE_ARG_COUNT(filltoborder, 5);
  ENFORCE_ARG_TYPE(filltoborder, 0, IS_PTR);
  ENFORCE_ARG_TYPE(filltoborder, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(filltoborder, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(filltoborder, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(filltoborder, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageFillToBorder(
    image, 
    AS_NUMBER(args[1]),
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__fill) {
  ENFORCE_ARG_COUNT(fill, 4);
  ENFORCE_ARG_TYPE(fill, 0, IS_PTR);
  ENFORCE_ARG_TYPE(fill, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(fill, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(fill, 3, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageFill(
    image, 
    AS_NUMBER(args[1]),
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__copy) {
  ENFORCE_ARG_COUNT(copy, 8);
  ENFORCE_ARG_TYPE(copy, 0, IS_PTR);
  ENFORCE_ARG_TYPE(copy, 1, IS_PTR);
  ENFORCE_ARG_TYPE(copy, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(copy, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(copy, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(copy, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(copy, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(copy, 7, IS_NUMBER);

  gdImagePtr dst = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(dst);

  gdImagePtr src = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(src);

  gdImageCopy(
    dst, 
    src, 
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__copymerge) {
  ENFORCE_ARG_COUNT(copymerge, 9);
  ENFORCE_ARG_TYPE(copymerge, 0, IS_PTR);
  ENFORCE_ARG_TYPE(copymerge, 1, IS_PTR);
  ENFORCE_ARG_TYPE(copymerge, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymerge, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymerge, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymerge, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymerge, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymerge, 7, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymerge, 8, IS_NUMBER);

  gdImagePtr dst = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(dst);

  gdImagePtr src = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(src);

  gdImageCopyMerge(
    dst, 
    src, 
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7]),
    AS_NUMBER(args[8])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__copymergegray) {
  ENFORCE_ARG_COUNT(copymergegray, 9);
  ENFORCE_ARG_TYPE(copymergegray, 0, IS_PTR);
  ENFORCE_ARG_TYPE(copymergegray, 1, IS_PTR);
  ENFORCE_ARG_TYPE(copymergegray, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymergegray, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymergegray, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymergegray, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymergegray, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymergegray, 7, IS_NUMBER);
  ENFORCE_ARG_TYPE(copymergegray, 8, IS_NUMBER);

  gdImagePtr dst = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(dst);

  gdImagePtr src = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(src);

  gdImageCopyMergeGray(
    dst, 
    src, 
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7]),
    AS_NUMBER(args[8])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__copyresized) {
  ENFORCE_ARG_COUNT(copyresized, 10);
  ENFORCE_ARG_TYPE(copyresized, 0, IS_PTR);
  ENFORCE_ARG_TYPE(copyresized, 1, IS_PTR);
  ENFORCE_ARG_TYPE(copyresized, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresized, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresized, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresized, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresized, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresized, 7, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresized, 8, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresized, 9, IS_NUMBER);

  gdImagePtr dst = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(dst);

  gdImagePtr src = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(src);

  gdImageCopyResized(
    dst, 
    src, 
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7]),
    AS_NUMBER(args[8]),
    AS_NUMBER(args[9])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__copyresampled) {
  ENFORCE_ARG_COUNT(copyresampled, 10);
  ENFORCE_ARG_TYPE(copyresampled, 0, IS_PTR);
  ENFORCE_ARG_TYPE(copyresampled, 1, IS_PTR);
  ENFORCE_ARG_TYPE(copyresampled, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresampled, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresampled, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresampled, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresampled, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresampled, 7, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresampled, 8, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyresampled, 9, IS_NUMBER);

  gdImagePtr dst = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(dst);

  gdImagePtr src = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(src);

  gdImageCopyResampled(
    dst, 
    src, 
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7]),
    AS_NUMBER(args[8]),
    AS_NUMBER(args[9])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__copyrotated) {
  ENFORCE_ARG_COUNT(copyrotated, 9);
  ENFORCE_ARG_TYPE(copyrotated, 0, IS_PTR);
  ENFORCE_ARG_TYPE(copyrotated, 1, IS_PTR);
  ENFORCE_ARG_TYPE(copyrotated, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyrotated, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyrotated, 4, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyrotated, 5, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyrotated, 6, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyrotated, 7, IS_NUMBER);
  ENFORCE_ARG_TYPE(copyrotated, 8, IS_NUMBER);

  gdImagePtr dst = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(dst);

  gdImagePtr src = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(src);

  gdImageCopyRotated(
    dst, 
    src, 
    AS_NUMBER(args[2]),
    AS_NUMBER(args[3]),
    AS_NUMBER(args[4]),
    AS_NUMBER(args[5]),
    AS_NUMBER(args[6]),
    AS_NUMBER(args[7]),
    AS_NUMBER(args[8])
  );
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__clone) {
  ENFORCE_ARG_COUNT(clone, 1);
  ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageClone(image);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__setbrush) {
  ENFORCE_ARG_COUNT(setbrush, 2);
  ENFORCE_ARG_TYPE(setbrush, 0, IS_PTR);
  ENFORCE_ARG_TYPE(setbrush, 1, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImagePtr brush = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(brush);

  gdImageSetBrush(image, brush);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__settile) {
  ENFORCE_ARG_COUNT(settile, 2);
  ENFORCE_ARG_TYPE(settile, 0, IS_PTR);
  ENFORCE_ARG_TYPE(settile, 1, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImagePtr tile = (gdImagePtr)AS_PTR(args[1])->pointer;
  CHECK_IMAGE_PTR(tile);

  gdImageSetTile(image, tile);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__setantialiased) {
  ENFORCE_ARG_RANGE(setantialiased, 2, 3);
  ENFORCE_ARG_TYPE(setantialiased, 0, IS_PTR);
  ENFORCE_ARG_TYPE(setantialiased, 1, IS_NUMBER);
  
  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

#ifndef _WIN32
  if(arg_count == 3) {
    ENFORCE_ARG_TYPE(setantialiased, 2, IS_NUMBER);
    gdImageSetAntiAliasedDontBlend(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]));
  } else {
    gdImageSetAntiAliased(image, AS_NUMBER(args[1]));
  }
#else
  gdImageSetAntiAliased(image, AS_NUMBER(args[1]));
#endif

  RETURN;
}

DECLARE_MODULE_METHOD(imagine__setthickness) {
  ENFORCE_ARG_COUNT(setthickness, 2);
  ENFORCE_ARG_TYPE(setthickness, 0, IS_PTR);
  ENFORCE_ARG_TYPE(setthickness, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageSetThickness(image, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__interlace) {
  ENFORCE_ARG_COUNT(interlace, 2);
  ENFORCE_ARG_TYPE(interlace, 0, IS_PTR);
  ENFORCE_ARG_TYPE(interlace, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageInterlace(image, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__alphablending) {
  ENFORCE_ARG_COUNT(alphablending, 2);
  ENFORCE_ARG_TYPE(alphablending, 0, IS_PTR);
  ENFORCE_ARG_TYPE(alphablending, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageAlphaBlending(image, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__savealpha) {
  ENFORCE_ARG_COUNT(savealpha, 2);
  ENFORCE_ARG_TYPE(savealpha, 0, IS_PTR);
  ENFORCE_ARG_TYPE(savealpha, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageSaveAlpha(image, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__pixelate) {
  ENFORCE_ARG_COUNT(pixelate, 3);
  ENFORCE_ARG_TYPE(pixelate, 0, IS_PTR);
  ENFORCE_ARG_TYPE(pixelate, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(pixelate, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImagePixelate(image, AS_NUMBER(args[1]), AS_NUMBER(args[2])));
}

DECLARE_MODULE_METHOD(imagine__scatter) {
  ENFORCE_ARG_RANGE(scatter, 3, 4);
  ENFORCE_ARG_TYPE(scatter, 0, IS_PTR);
  ENFORCE_ARG_TYPE(scatter, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(scatter, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  if(arg_count == 4 && !IS_NIL(args[3])) {
    ENFORCE_ARG_TYPE(scatter, 3, IS_LIST);

    b_obj_list *color_list = AS_LIST(args[3]);
    int *colors = ALLOCATE(int, color_list->items.count);
    for(int i = 0; i < color_list->items.count; i++) {
      if(!IS_NUMBER(color_list->items.values[i])) {
        FREE_ARRAY(int, colors, color_list->items.count);
        RETURN_ERROR("Invalid color in scatter profile.");
      }

      colors[i] = AS_NUMBER(color_list->items.values[i]);
    }

    RETURN_NUMBER(gdImageScatterColor(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), colors, color_list->items.count));
  } else {
    RETURN_NUMBER(gdImageScatter(image, AS_NUMBER(args[1]), AS_NUMBER(args[2])));
  }
}

DECLARE_MODULE_METHOD(imagine__smooth) {
  ENFORCE_ARG_COUNT(smooth, 2);
  ENFORCE_ARG_TYPE(smooth, 0, IS_PTR);
  ENFORCE_ARG_TYPE(smooth, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageSmooth(image, AS_NUMBER(args[1])));
}

DECLARE_MODULE_METHOD(imagine__meanremoval) {
  ENFORCE_ARG_COUNT(meanremoval, 1);
  ENFORCE_ARG_TYPE(meanremoval, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageMeanRemoval(image));
}

DECLARE_MODULE_METHOD(imagine__emboss) {
  ENFORCE_ARG_COUNT(emboss, 1);
  ENFORCE_ARG_TYPE(emboss, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageEmboss(image));
}

DECLARE_MODULE_METHOD(imagine__gaussianblur) {
  ENFORCE_ARG_COUNT(gaussianblur, 1);
  ENFORCE_ARG_TYPE(gaussianblur, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageGaussianBlur(image));
}

DECLARE_MODULE_METHOD(imagine__edgedetect) {
  ENFORCE_ARG_COUNT(edgedetect, 1);
  ENFORCE_ARG_TYPE(edgedetect, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageEdgeDetectQuick(image));
}

DECLARE_MODULE_METHOD(imagine__selectiveblur) {
  ENFORCE_ARG_COUNT(selectiveblur, 1);
  ENFORCE_ARG_TYPE(selectiveblur, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageSelectiveBlur(image));
}

DECLARE_MODULE_METHOD(imagine__grayscale) {
  ENFORCE_ARG_COUNT(grayscale, 1);
  ENFORCE_ARG_TYPE(grayscale, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageGrayScale(image));
}

DECLARE_MODULE_METHOD(imagine__negate) {
  ENFORCE_ARG_COUNT(negate, 1);
  ENFORCE_ARG_TYPE(negate, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageNegate(image));
}

DECLARE_MODULE_METHOD(imagine__color) {
  ENFORCE_ARG_COUNT(color, 5);
  ENFORCE_ARG_TYPE(color, 0, IS_PTR);
  ENFORCE_ARG_TYPE(color, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(color, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(color, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(color, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageColor(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]), AS_NUMBER(args[3]), AS_NUMBER(args[4])));
}

DECLARE_MODULE_METHOD(imagine__contrast) {
  ENFORCE_ARG_COUNT(contrast, 2);
  ENFORCE_ARG_TYPE(contrast, 0, IS_PTR);
  ENFORCE_ARG_TYPE(contrast, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageContrast(image, AS_NUMBER(args[1])));
}

DECLARE_MODULE_METHOD(imagine__brightness) {
  ENFORCE_ARG_COUNT(brightness, 2);
  ENFORCE_ARG_TYPE(brightness, 0, IS_PTR);
  ENFORCE_ARG_TYPE(brightness, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  RETURN_NUMBER(gdImageBrightness(image, AS_NUMBER(args[1])));
}

DECLARE_MODULE_METHOD(imagine__meta) {
  ENFORCE_ARG_COUNT(size, 1);
  ENFORCE_ARG_TYPE(size, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  int color_totals = gdImageColorsTotal(image);
  int true_colors = gdImageTrueColor(image);
  
  b_obj_dict *dict = (b_obj_dict*)GC(new_dict(vm));
  dict_set_entry(vm, dict, GC_L_STRING("width", 5), NUMBER_VAL(gdImageSX(image)));
  dict_set_entry(vm, dict, GC_L_STRING("height", 6), NUMBER_VAL(gdImageSY(image)));
  dict_set_entry(vm, dict, GC_L_STRING("colors", 6), NUMBER_VAL(color_totals > 0 ? color_totals : true_colors));
  dict_set_entry(vm, dict, GC_L_STRING("res_x", 5), NUMBER_VAL(gdImageResolutionX(image)));
  dict_set_entry(vm, dict, GC_L_STRING("res_y", 5), NUMBER_VAL(gdImageResolutionY(image)));
  dict_set_entry(vm, dict, GC_L_STRING("interpolation", 13), NUMBER_VAL((int)image->interpolation_id));
  dict_set_entry(vm, dict, GC_L_STRING("true_color", 10), BOOL_VAL(true_colors > 0));
  dict_set_entry(vm, dict, GC_L_STRING("interlaced", 10), BOOL_VAL(gdImageGetInterlaced(image) != 0));

  RETURN_OBJ(dict);
}

DECLARE_MODULE_METHOD(imagine__fliphorizontal) {
  ENFORCE_ARG_COUNT(fliphorizontal, 1);
  ENFORCE_ARG_TYPE(fliphorizontal, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageFlipHorizontal(image);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__flipvertical) {
  ENFORCE_ARG_COUNT(flipvertical, 1);
  ENFORCE_ARG_TYPE(flipvertical, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageFlipVertical(image);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__flip) {
  ENFORCE_ARG_COUNT(flip, 1);
  ENFORCE_ARG_TYPE(flip, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageFlipBoth(image);
  RETURN;
}

DECLARE_MODULE_METHOD(imagine__crop) {
  ENFORCE_ARG_COUNT(crop, 5);
  ENFORCE_ARG_TYPE(crop, 0, IS_PTR);
  ENFORCE_ARG_TYPE(crop, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(crop, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(crop, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(crop, 4, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);
  
  gdRect rect = { 
    .x = AS_NUMBER(args[1]), 
    .y = AS_NUMBER(args[2]), 
    .width = AS_NUMBER(args[3]), 
    .height = AS_NUMBER(args[4]) 
  };

  gdImagePtr new_image = gdImageCrop(image, &rect);
  if(NULL == new_image) {
    RETURN_ERROR("Failed to crop image to rectangle.");
  }

  RETURN_CLOSABLE_NAMED_PTR(new_image, IMAGINE_IMAGE_PTR_NAME, imagine_free_image_ptrs);
}

DECLARE_MODULE_METHOD(imagine__cropauto) {
  ENFORCE_ARG_COUNT(cropauto, 2);
  ENFORCE_ARG_TYPE(cropauto, 0, IS_PTR);
  ENFORCE_ARG_TYPE(cropauto, 1, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImagePtr new_image = gdImageCropAuto(image, AS_NUMBER(args[1]));
  if(NULL == new_image) {
    RETURN_ERROR("Failed to crop image to rectangle.");
  }

  RETURN_CLOSABLE_NAMED_PTR(new_image, IMAGINE_IMAGE_PTR_NAME, imagine_free_image_ptrs);
}

DECLARE_MODULE_METHOD(imagine__scale) {
  ENFORCE_ARG_COUNT(scale, 2);
  ENFORCE_ARG_TYPE(scale, 0, IS_PTR);
  ENFORCE_ARG_TYPE(scale, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(scale, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImagePtr new_image = gdImageScale(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]));
  if(NULL == new_image) {
    RETURN_ERROR("Failed to scale image to (%d, %d).", (int)AS_NUMBER(args[1]), (int)AS_NUMBER(args[2]));
  }

  RETURN_CLOSABLE_NAMED_PTR(new_image, IMAGINE_IMAGE_PTR_NAME, imagine_free_image_ptrs);
}

DECLARE_MODULE_METHOD(imagine__rotate) {
  ENFORCE_ARG_COUNT(rotate, 2);
  ENFORCE_ARG_TYPE(rotate, 0, IS_PTR);
  ENFORCE_ARG_TYPE(rotate, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(rotate, 2, IS_NUMBER);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImagePtr new_image = gdImageRotateInterpolated(image, AS_NUMBER(args[1]), AS_NUMBER(args[2]));
  if(NULL == new_image) {
    RETURN_ERROR("Failed to rotate image to angle %.16g.", AS_NUMBER(args[1]));
  }

  RETURN_CLOSABLE_NAMED_PTR(new_image, IMAGINE_IMAGE_PTR_NAME, imagine_free_image_ptrs);
}

CREATE_MODULE_LOADER(imagine) {
  static b_field_reg module_fields[] = {
      // Fonts
      GET_IMAGINE_PTR(tinyfont),
      GET_IMAGINE_PTR(smallfont),
      GET_IMAGINE_PTR(mediumfont),
      GET_IMAGINE_PTR(largefont),
      GET_IMAGINE_PTR(giantfont),

      // Color Quantization
      GET_IMAGINE_CONST(QUANT_DEFAULT),
      GET_IMAGINE_CONST(QUANT_JQUANT),
      GET_IMAGINE_CONST(QUANT_NEUQUANT),
      GET_IMAGINE_CONST(QUANT_LIQ),

      // Interpolation Methods
      GET_IMAGINE_CONST(DEFAULT),
      GET_IMAGINE_CONST(BELL),
      GET_IMAGINE_CONST(BESSEL),
      GET_IMAGINE_CONST(BILINEAR_FIXED),
      GET_IMAGINE_CONST(BICUBIC),
      GET_IMAGINE_CONST(BICUBIC_FIXED),
      GET_IMAGINE_CONST(BLACKMAN),
      GET_IMAGINE_CONST(BOX),
      GET_IMAGINE_CONST(BSPLINE),
      GET_IMAGINE_CONST(CATMULLROM),
      GET_IMAGINE_CONST(GAUSSIAN),
      GET_IMAGINE_CONST(GENERALIZED_CUBIC),
      GET_IMAGINE_CONST(HERMITE),
      GET_IMAGINE_CONST(HAMMING),
      GET_IMAGINE_CONST(HANNING),
      GET_IMAGINE_CONST(MITCHELL),
      GET_IMAGINE_CONST(NEAREST_NEIGHBOUR),
      GET_IMAGINE_CONST(POWER),
      GET_IMAGINE_CONST(QUADRATIC),
      GET_IMAGINE_CONST(SINC),
      GET_IMAGINE_CONST(TRIANGLE),
      GET_IMAGINE_CONST(WEIGHTED4),
      GET_IMAGINE_CONST(LINEAR),
      GET_IMAGINE_CONST(LANCZOS3),
      GET_IMAGINE_CONST(LANCZOS8),
      GET_IMAGINE_CONST(BLACKMAN_BESSEL),
      GET_IMAGINE_CONST(BLACKMAN_SINC),
      GET_IMAGINE_CONST(QUADRATIC_BSPLINE),
      GET_IMAGINE_CONST(CUBIC_SPLINE),
      GET_IMAGINE_CONST(COSINE),
      GET_IMAGINE_CONST(WELSH),
      GET_IMAGINE_CONST(METHOD_COUNT),

      // pixelate
      GET_IMAGINE_CONST(PIXELATE_UPPERLEFT),
      GET_IMAGINE_CONST(PIXELATE_AVERAGE),

      // Crop
      GET_IMAGINE_CONST(CROP_DEFAULT),
      GET_IMAGINE_CONST(CROP_TRANSPARENT),
      GET_IMAGINE_CONST(CROP_BLACK),
      GET_IMAGINE_CONST(CROP_WHITE),
      GET_IMAGINE_CONST(CROP_SIDES),
      GET_IMAGINE_CONST(CROP_THRESHOLD),

      // Image Comparison
      GET_IMAGINE_CONST(CMP_IMAGE),
      GET_IMAGINE_CONST(CMP_NUM_COLORS),
      GET_IMAGINE_CONST(CMP_COLOR),
      GET_IMAGINE_CONST(CMP_SIZE_X),
      GET_IMAGINE_CONST(CMP_SIZE_Y),
      GET_IMAGINE_CONST(CMP_TRANSPARENT),
      GET_IMAGINE_CONST(CMP_BACKGROUND),
      GET_IMAGINE_CONST(CMP_INTERLACE),
      GET_IMAGINE_CONST(CMP_TRUECOLOR),

      // finalized...
      {NULL, false, NULL}
  };

  static b_func_reg module_functions[] = {
      // create and destroy
      {"new",   true,  GET_MODULE_METHOD(imagine__new)},
      {"close",   true,  GET_MODULE_METHOD(imagine__close)},
      {"frompng",   true,  GET_MODULE_METHOD(imagine__frompng)},
      {"fromjpeg",   true,  GET_MODULE_METHOD(imagine__fromjpeg)},
      {"fromgif",   true,  GET_MODULE_METHOD(imagine__fromgif)},
      {"frombmp",   true,  GET_MODULE_METHOD(imagine__frombmp)},
      {"fromwbmp",   true,  GET_MODULE_METHOD(imagine__fromwbmp)},
      {"fromwtga",   true,  GET_MODULE_METHOD(imagine__fromwtga)},
      {"fromfile",   true,  GET_MODULE_METHOD(imagine__fromfile)},
      
      // pixels
      {"getpixel",   true,  GET_MODULE_METHOD(imagine__getpixel)},
      {"setpixel",   true,  GET_MODULE_METHOD(imagine__setpixel)},

      // drawing
      {"line",   true,  GET_MODULE_METHOD(imagine__line)},
      {"dashedline",   true,  GET_MODULE_METHOD(imagine__dashedline)},
      {"rectangle",   true,  GET_MODULE_METHOD(imagine__rectangle)},
      {"filledrectangle",   true,  GET_MODULE_METHOD(imagine__filledrectangle)},
      {"boundsafe",   true,  GET_MODULE_METHOD(imagine__boundsafe)},
      {"char",   true,  GET_MODULE_METHOD(imagine__char)},
      {"charup",   true,  GET_MODULE_METHOD(imagine__charup)},
      {"string",   true,  GET_MODULE_METHOD(imagine__string)},
      {"stringup",   true,  GET_MODULE_METHOD(imagine__stringup)},

      // drawing > polygons
      {"polygon",   true,  GET_MODULE_METHOD(imagine__polygon)},
      {"openpolygon",   true,  GET_MODULE_METHOD(imagine__openpolygon)},
      {"filledpolygon",   true,  GET_MODULE_METHOD(imagine__filledpolygon)},
      {"arc",   true,  GET_MODULE_METHOD(imagine__arc)},
      {"filledarc",   true,  GET_MODULE_METHOD(imagine__filledarc)},
      {"ellipse",   true,  GET_MODULE_METHOD(imagine__ellipse)},
      {"filledellipse",   true,  GET_MODULE_METHOD(imagine__filledellipse)},

      // color
      {"colorallocate",   true,  GET_MODULE_METHOD(imagine__colorallocate)},
      {"colorallocatealpha",   true,  GET_MODULE_METHOD(imagine__colorallocatealpha)},
      {"colorclosest",   true,  GET_MODULE_METHOD(imagine__colorclosest)},
      {"colorclosestalpha",   true,  GET_MODULE_METHOD(imagine__colorclosestalpha)},
      {"colorclosesthwb",   true,  GET_MODULE_METHOD(imagine__colorclosesthwb)},
      {"colorexact",   true,  GET_MODULE_METHOD(imagine__colorexact)},
      {"colorexactalpha",   true,  GET_MODULE_METHOD(imagine__colorexactalpha)},
      {"colorresolve",   true,  GET_MODULE_METHOD(imagine__colorresolve)},
      {"colorresolvealpha",   true,  GET_MODULE_METHOD(imagine__colorresolvealpha)},
      {"colordeallocate",   true,  GET_MODULE_METHOD(imagine__colordeallocate)},
      {"colortransparent",   true,  GET_MODULE_METHOD(imagine__colortransparent)},
      {"palettecopy",   true,  GET_MODULE_METHOD(imagine__palettecopy)},
      {"colorreplace",   true,  GET_MODULE_METHOD(imagine__colorreplace)},

      // export
      {"gif",   true,  GET_MODULE_METHOD(imagine__gif)},
      {"png",   true,  GET_MODULE_METHOD(imagine__png)},
      {"jpeg",   true,  GET_MODULE_METHOD(imagine__jpeg)},
      {"bmp",   true,  GET_MODULE_METHOD(imagine__bmp)},
      {"wbmp",   true,  GET_MODULE_METHOD(imagine__wbmp)},

      // processing
      {"filltoborder",   true,  GET_MODULE_METHOD(imagine__filltoborder)},
      {"fill",   true,  GET_MODULE_METHOD(imagine__fill)},
      {"copy",   true,  GET_MODULE_METHOD(imagine__copy)},
      {"copymerge",   true,  GET_MODULE_METHOD(imagine__copymerge)},
      {"copymergegray",   true,  GET_MODULE_METHOD(imagine__copymergegray)},
      {"copyresized",   true,  GET_MODULE_METHOD(imagine__copyresized)},
      {"copyresampled",   true,  GET_MODULE_METHOD(imagine__copyresampled)},
      {"copyrotated",   true,  GET_MODULE_METHOD(imagine__copyrotated)},
      {"clone",   true,  GET_MODULE_METHOD(imagine__clone)},
      {"setbrush",   true,  GET_MODULE_METHOD(imagine__setbrush)},
      {"settile",   true,  GET_MODULE_METHOD(imagine__settile)},
      {"setantialiased",   true,  GET_MODULE_METHOD(imagine__setantialiased)},
      {"setthickness",   true,  GET_MODULE_METHOD(imagine__setthickness)},
      {"interlace",   true,  GET_MODULE_METHOD(imagine__interlace)},
      {"alphablending",   true,  GET_MODULE_METHOD(imagine__alphablending)},
      {"savealpha",   true,  GET_MODULE_METHOD(imagine__savealpha)},
      {"fliphorizontal",   true,  GET_MODULE_METHOD(imagine__fliphorizontal)},
      {"flipvertical",   true,  GET_MODULE_METHOD(imagine__flipvertical)},
      {"flip",   true,  GET_MODULE_METHOD(imagine__flip)},
      {"crop",   true,  GET_MODULE_METHOD(imagine__crop)},
      {"cropauto",   true,  GET_MODULE_METHOD(imagine__cropauto)},
      {"scale",   true,  GET_MODULE_METHOD(imagine__scale)},
      {"rotate",   true,  GET_MODULE_METHOD(imagine__rotate)},

      // filters
      {"pixelate",   true,  GET_MODULE_METHOD(imagine__pixelate)},
      {"scatter",   true,  GET_MODULE_METHOD(imagine__scatter)},
      {"smooth",   true,  GET_MODULE_METHOD(imagine__smooth)},
      {"meanremoval",   true,  GET_MODULE_METHOD(imagine__meanremoval)},
      {"emboss",   true,  GET_MODULE_METHOD(imagine__emboss)},
      {"gaussianblur",   true,  GET_MODULE_METHOD(imagine__gaussianblur)},
      {"edgedetect",   true,  GET_MODULE_METHOD(imagine__edgedetect)},
      {"selectiveblur",   true,  GET_MODULE_METHOD(imagine__selectiveblur)},
      {"color",   true,  GET_MODULE_METHOD(imagine__color)},
      {"contrast",   true,  GET_MODULE_METHOD(imagine__contrast)},
      {"brightness",   true,  GET_MODULE_METHOD(imagine__brightness)},
      {"grayscale",   true,  GET_MODULE_METHOD(imagine__grayscale)},
      {"negate",   true,  GET_MODULE_METHOD(imagine__negate)},

      // misc
      {"setclip",   true,  GET_MODULE_METHOD(imagine__setclip)},
      {"getclip",   true,  GET_MODULE_METHOD(imagine__getclip)},
      {"setresolution",   true,  GET_MODULE_METHOD(imagine__setresolution)},
      {"truecolortopalette",   true,  GET_MODULE_METHOD(imagine__truecolortopalette)},
      {"palettetotruecolor",   true,  GET_MODULE_METHOD(imagine__palettetotruecolor)},

      // Blade extras
      {"meta",   true,  GET_MODULE_METHOD(imagine__meta)},

      // Finalized...
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_imagine",
      .fields = module_fields,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}

#undef IMAGINE_IMAGE_PTR_NAME
#undef GET_IMAGINE_PTR
#undef IMAGINE_PTR
#undef IMAGINE_CONST
#undef GET_IMAGINE_CONST
#undef CHECK_IMAGE
#undef CHECK_FONT_PTR
#undef CHECK_IMAGE_PTR
