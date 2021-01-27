#ifndef bird_file_h
#define bird_file_h

#include "native.h"
#include "object.h"

#define DECLARE_FILE_METHOD(name) DECLARE_METHOD(file##name)

/**
 * file(path: string [, mode: string])
 *
 * opens a new file handle to the file specified
 * available modes are as exists in C, with the following exception
 * - w+ does not tructuate the file, but will still create it
 *   if it does not exist
 * - default mode is 'r' i.e. read only
 */
DECLARE_NATIVE(file);

#endif