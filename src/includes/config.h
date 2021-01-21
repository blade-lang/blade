#ifndef bird_config_h
#define bird_config_h

// global debug mode flag
#define DEBUG_MODE 1

// --> debug mode options starts here...
#if DEBUG_MODE == 1

#define DEBUG_TRACE_EXECUTION 0
#define DEBUG_PRINT_CODE 1
#define DEBUG_TABLE 0
#define DEBUG_STRESS_GC 1
#define DEBUG_LOG_GC 0

#endif
// --> debug mode options ends here...

#define MAX_USING_CASES 256
#define MAX_FUNCTION_PARAMETERS 255
#define FRAMES_MAX 128
#define NUMBER_FORMAT "%.16g"
#define TABLE_MAX_LOAD 0.75
#define GC_HEAP_GROWTH_FACTOR 2

#define USE_NAN_BOXING 1

#endif