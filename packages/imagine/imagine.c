#include <blade.h>
#include <errno.h>
#include <gd.h>

#define CHECK_IMAGE_PTR(x) if((x) == NULL) \
    RETURN_ERROR("Invalid image pointer.") \

#define CHECK_IMAGE(x) if((x) == NULL) \
    RETURN_ERROR(strerror(errno)) \

void imagine_free_image_ptrs(void *data) {
  gdImagePtr image = (gdImagePtr)data;
  gdImageDestroy(image);
}

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

  RETURN_CLOSABLE_NAMED_PTR(image, "<void *imagine::type::image>", imagine_free_image_ptrs);
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
  ENFORCE_ARG_TYPE(fromfile, 0, IS_FILE);

  gdImagePtr image = gdImageCreateFromFile(AS_FILE(args[0])->path->chars);
  CHECK_IMAGE(image);
  RETURN_PTR(image);
}

DECLARE_MODULE_METHOD(imagine__close) {
  ENFORCE_ARG_COUNT(close, 1);
  ENFORCE_ARG_TYPE(close, 0, IS_PTR);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageDestroy(image);
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

DECLARE_MODULE_METHOD(imagine__aablend) {
  ENFORCE_ARG_COUNT(aablend, 1);

  gdImagePtr image = (gdImagePtr)AS_PTR(args[0])->pointer;
  CHECK_IMAGE_PTR(image);

  gdImageAABlend(image);
  RETURN;
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

CREATE_MODULE_LOADER(imagine) {
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
      {"aablend",   true,  GET_MODULE_METHOD(imagine__aablend)},
      {"line",   true,  GET_MODULE_METHOD(imagine__line)},
      {"dashedline",   true,  GET_MODULE_METHOD(imagine__dashedline)},
      {"rectangle",   true,  GET_MODULE_METHOD(imagine__rectangle)},
      {"filledrectangle",   true,  GET_MODULE_METHOD(imagine__filledrectangle)},
      {"boundsafe",   true,  GET_MODULE_METHOD(imagine__boundsafe)},
      // misc
      {"setclip",   true,  GET_MODULE_METHOD(imagine__setclip)},
      {"getclip",   true,  GET_MODULE_METHOD(imagine__getclip)},
      {"setresolution",   true,  GET_MODULE_METHOD(imagine__setresolution)},
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_imagine",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}

#undef CHECK_IMAGE
#undef CHECK_IMAGE_PTR
