# Makefile for building a single configuration of the C interpreter. It expects
# variables to be passed in for:
#
# MODE         "debug" or "release".
# NAME         Name of the output executable (and object file directory).
# SOURCE_DIR   Directory where source files and headers are found.

# MinGW--or at least some versions of it--default CC to "cc" but then don't
# provide an executable named "cc". Manually point to "gcc" instead.
ifeq ($(OS),mingw32)
	CC = GCC
endif

CFLAGS := -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter 
USE_SYSTEM_PCRE := 0

# # For Windows OS, you may need to uncomment this section
# CFLAGS += -Wno-implicit-fallthrough -Wno-format-zero-length -Wno-maybe-uninitialized

# If we're building at a point in the middle of an edit, don't fail if there
# are functions that aren't used yet.
ifeq ($(MODE),debug)
	CFLAGS += -Wno-unused-function -Wno-unused-variable
endif

# Some platform-specific workarounds. Note that we use "gcc" explicitly in the
# call to get the machine name because one of these workarounds deals with $(CC)
# itself not working.
OS := $(lastword $(subst -, ,$(shell gcc -dumpmachine)))

# Don't add -fPIC on Windows since it generates a warning which gets promoted
# to an error by -Werror.
ifeq      ($(OS),mingw32)
else ifeq ($(OS),cygwin)
	# Do nothing.
else
	CFLAGS += -fPIC
endif

# Mode configuration.
ifeq ($(MODE),debug)
	CFLAGS += -O0 -DDEBUG -g
	BUILD_DIR := build/debug
else
	CFLAGS += -O3 -flto
	BUILD_DIR := build/release
endif

# Files.

SOURCES := 						$(wildcard $(SOURCE_DIR)/*.c)

SOURCES_OBJECTS := 		$(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))

ifeq ($(USE_SYSTEM_PCRE),1)
	LIB_PCRE2 := 
else
	LIB_PCRE2 := deps/pcre2/.libs/libpcre2-8.a
endif

CFLAGS += -I$(HEADERS_DIR)

ifneq (,$(findstring darwin,$(OS)))

# for OSX environments
	SHARED_EXT := dylib
else ifeq ($(OS),cygwin) || ifeq ($(OS),mingw32)

# for cygwin and mingw32 environments
	SHARED_LIB_FLAGS := -Wl,-soname,libbirdy.dll
	SHARED_EXT := dll
CFLAGS += -Wno-return-local-addr -Wno-implicit-fallthrough -Wno-maybe-uninitialized

else

# for linux environments
	SHARED_LIB_FLAGS := -Wl,-soname,libbirdy.so
	SHARED_EXT := so

CFLAGS += -Wno-return-local-addr -Wno-implicit-fallthrough -lreadline 
endif

# Targets ---------------------------------------------------------------------

# Link the interpreter.
# build/$(NAME): $(SOURCES_OBJECTS)
build/$(NAME): $(SOURCES_OBJECTS) $(LIB_PCRE2)
	@ printf "%8s %s %s\n" $(CC) $@ "$(CFLAGS)"
	@ mkdir -p build
	@ $(CC) $(CFLAGS) $^ -o $@

# Compile source object files.
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@ printf "%8s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) -c $(CFLAGS) -o $@ $<

.PHONY: default
