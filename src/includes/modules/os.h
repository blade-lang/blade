#ifndef bird_module_os_h
#define bird_module_os_h

#include "module.h"
#include "native.h"

/**
 * Os.exec(command: string)
 *
 * exec executes shell commands
 */
// DECLARE_MODULE_METHOD(os, exec);

/**
 * Os.info()
 *
 * returns the system information
 */
// DECLARE_MODULE_METHOD(os, info);

/**
 * Os.sleep(time: number)
 *
 * sleeps for the specifified number of seconds
 */
// DECLARE_MODULE_METHOD(os, sleep);

CREATE_MODULE_LOADER(os);

#endif