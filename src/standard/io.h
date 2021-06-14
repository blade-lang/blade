#ifndef BIRD_MODULE_IO_H
#define BIRD_MODULE_IO_H

#include "module.h"
#include "native.h"
#include "value.h"

extern bool is_std_file(b_obj_file *file);

CREATE_MODULE_LOADER(io);

#endif