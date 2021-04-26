# Makefile for building a single configuration of the C interpreter. It expects
# variables to be passed in for:
#
# MODE         	"debug" or "release".
# NAME         	Name of the output executable (and object file directory).
# SOURCE_DIR   	Directory where source files are found.

# Some platform-specific workarounds. Note that we use "gcc" explicitly in the
# call to get the machine name because one of these workarounds deals with $(CC)
# itself not working.
# OS := $(lastword $(subst -, ,$(shell gcc -dumpmachine)))

_OS :=
_ARCH :=

# detect OS and architecture (for future use)
ifeq ($(OS),Windows_NT)
  _OS = windows
	ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
		_ARCH = amd64
	else
		ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			_ARCH = amd64
		endif
		ifeq ($(PROCESSOR_ARCHITECTURE),x86)
			_ARCH = ia32
		endif
	endif
else ifeq ($(OS),mingw32)
	_OS = mingw
	ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
    _ARCH = amd64
	else
		ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			_ARCH = amd64
		endif
		ifeq ($(PROCESSOR_ARCHITECTURE),x86)
			_ARCH = ia32
		endif
	endif
else ifeq ($(OS),cygwin)
	_OS = cygwin
	ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
    _ARCH = amd64
	else
		ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			_ARCH = amd64
		endif
		ifeq ($(PROCESSOR_ARCHITECTURE),x86)
			_ARCH = ia32
		endif
	endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
			_OS = linux
    endif
    ifeq ($(UNAME_S),Darwin)
      _OS = osx
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
      _ARCH += amd64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
      _ARCH += ia32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
      _ARCH += arm
    endif
endif

# MinGW and Cygwin--or at least some versions of them--default CC to "cc" but then don't
# provide an executable named "cc". Manually point to "gcc" instead.
ifeq ($(_OS),$(filter $(_OS), cygwin mingw))
	CC = GCC
endif

CFLAGS := -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter

# If we're building at a point in the middle of an edit, don't fail if there
# are functions that aren't used yet.
ifeq ($(MODE),debug)
	CFLAGS += -Wno-unused-function -Wno-unused-variable
endif

# Don't add -fPIC on Windows since it generates a warning which gets promoted
# to an error by -Werror.
ifeq ($(_OS),mingw)
else ifeq ($(_OS),cygwin)
else ifeq ($(_OS),windows)
	# Do nothing.
else
	CFLAGS += -fPIC
endif

# Mode configuration.
ifeq ($(MODE),debug)
	CFLAGS += -O0 -DDEBUG -g
	BUILD_DIR := build/debug
else
# CFLAGS += -O3 -flto
	CFLAGS += -Ofast -flto
	BUILD_DIR := build/release
endif


LIB_STATICS:=

ifeq ($(_OS),osx)
# for OSX environments
	SHARED_EXT := dylib
	LIB_STATICS	= deps/lib/darwin/libpcre2-8.a
	CFLAGS += -Ldeps/lib/darwin -lreadline
endif
ifeq ($(_OS),$(filter $(_OS), cygwin mingw))
# for cygwin and mingw32 environments
	SHARED_LIB_FLAGS := -Wl,-soname,libbirdy.dll
	SHARED_EXT := dll
	CFLAGS += -Wno-return-local-addr -Wno-maybe-uninitialized -Wno-sequence-point
endif
ifeq ($(_OS),linux)
# for linux environments
	SHARED_LIB_FLAGS := -Wl,-soname,libbirdy.so
	SHARED_EXT := so	
	CFLAGS += -Wno-return-local-addr -Wno-implicit-fallthrough -Wno-maybe-uninitialized -Wno-unused-result -Wno-sequence-point -lreadline
# we want to make sure GCC compilers enable asprintf and vasprintf
# We also need to link the math lib here
	CFLAGS += -D_GNU_SOURCE -lm
endif

LIB_WIN32 :=

# for mingw and cygwin
ifeq ($(_OS),cygwin)
	LIB_WIN32 = deps/lib/mingw/libpcre2-8.dll.a /usr/lib/libreadline.dll.a
else ifeq ($(_OS),mingw)
	CFLAGS += -lshlwapi
	LIB_WIN32 = deps/lib/mingw/libpcre2-8.dll.a /mingw64/lib/libreadline.dll.a /mingw64/x86_64-w64-mingw32/lib/libshlwapi.a /mingw64/x86_64-w64-mingw32/lib/libws2_32.a /mingw64/x86_64-w64-mingw32/lib/libnetapi32.a
endif

# Files.
SUB_DIRS := core modules
SRC_DIR := $(addprefix $(SOURCE_DIR)/, $(SUB_DIRS))
BLD_DIRS := $(addprefix $(BUILD_DIR)/, $(SUB_DIRS))

SOURCES = $(foreach sdir, $(SRC_DIR), $(wildcard $(sdir)/*.c))
OBJECTS = $(patsubst $(SOURCE_DIR)/*.c, $(BUILD_DIR)/*.c, $(SOURCES))
# INCLUDES = $(addprefix -I, $(addprefix $(SOURCE_DIR)/, $(SUB_DIRS)))

# Adding the include paths
# 
# Curl needs to be available somehow on the device
# and even though we try to supply the library for the build,
# I have detected that those libraries may not work without
# the -lcurl flag.
CFLAGS += -I$(SOURCE_DIR)/core -I$(SOURCE_DIR) -Ideps/includes -lcurl

# Main build...
build/$(NAME): $(OBJECTS) $(LIB_STATICS) $(LIB_WIN32)
	@ printf "Building Bird in %s mode into %s for %s %s...\n" $(MODE) $(NAME) $(_OS) $(_ARCH)
	@ printf "%s %s %s\n" $(CC) $@ "$(CFLAGS) $(LIB_STATICS) $(LIB_WIN32)"
	@ mkdir -p build
	@ $(CC) $(CFLAGS) $^ -o $@

.PHONY: default