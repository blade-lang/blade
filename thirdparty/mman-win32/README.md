# mmain-win32

[![Build Status](https://travis-ci.org/boldowa/mman-win32.svg?branch=master)](https://travis-ci.org/boldowa/mman-win32)
[![Coverage Status](https://coveralls.io/repos/github/boldowa/mman-win32/badge.svg?branch=master)](https://coveralls.io/github/boldowa/mman-win32?branch=master)

A light implementation of the mmap functions for MinGW(forked from https://code.google.com/archive/p/mman-win32/).

The mmap-win32 library implements a wrapper for mmap functions around the memory mapping Windows API.

License: [MIT License](https://opensource.org/licenses/mit-license.php)


## Build / Install steps

1. Install [CMake](https://cmake.org)
2. Move to **build** directory
3. Run `cmake .. && make -j2` command.
4. Run `sudo make install` command.