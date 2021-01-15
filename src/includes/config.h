#ifndef bird_config_h
#define bird_config_h

#include "common.h"

// global debug mode flag
#define DEBUG_MODE 0

// --> debug mode options starts here...
#if DEBUG_MODE == 1

#define DEBUG_TRACE_EXECUTION 0
#define DEBUG_PRINT_CODE 1
#define DEBUG_TABLE 0

#endif
// --> debug mode options ends here...

#define FRAMES_MAX 128
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define NUMBER_FORMAT "%.16g"
#define TABLE_MAX_LOAD 0.75

#endif