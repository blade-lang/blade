#ifndef bird_module_io_h
#define bird_module_io_h

#include "module.h"
#include "native.h"
#include "value.h"

extern bool is_std_file(b_obj_file *file);

CREATE_MODULE_LOADER(io);

#endif