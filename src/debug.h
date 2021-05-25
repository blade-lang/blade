#ifndef bird_debug_h
#define bird_debug_h

#include "blob.h"

void disassemble_blob(b_blob *blob, const char *name);

int disassemble_instruction(b_blob *blob, int offset);

#endif