# Makefile for building a single configuration of the C interpreter. It expects
# variables to be passed in for:
#
# MODE         	"debug" or "release".
# NAME         	Name of the output executable (and object file directory).
# SOURCE_DIR   	Directory where source files are found.

# MinGW and Cygwin--or at least some versions of them--default CC to "cc" but then don't
# provide an executable named "cc". Manually point to "gcc" instead.
ifeq ($(OS),mingw32)
	CC = GCC
else ifeq ($(OS),cygwin)
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


ifeq ($(USE_SYSTEM_PCRE),1)
	LIB_PCRE2 := 
else
	LIB_PCRE2 := deps/pcre2/.libs/libpcre2-8.a
endif

ifneq (,$(findstring darwin,$(OS)))

# for OSX environments
	SHARED_EXT := dylib
else ifeq ($(OS),cygwin)

# for cygwin and mingw32 environments
	SHARED_LIB_FLAGS := -Wl,-soname,libbirdy.dll
	SHARED_EXT := dll
CFLAGS += -Wno-return-local-addr -Wno-implicit-fallthrough -Wno-maybe-uninitialized

else ifeq ($(OS),mingw32)

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

ifeq ($(OS),cygwin)
	CFLAGS += -Ideps/pcre2/src
else ifeq ($(OS),mingw32)
	CFLAGS += -Ideps/pcre2/src
endif

# Files.

SUB_DIRS := core builtin modules
SRC_DIR := $(addprefix $(SOURCE_DIR)/, $(SUB_DIRS))
BLD_DIRS := $(addprefix $(BUILD_DIR)/, $(SUB_DIRS))

SOURCES = $(foreach sdir, $(SRC_DIR), $(wildcard $(sdir)/*.c))
OBJECTS = $(patsubst $(SOURCE_DIR)/*.c, $(BUILD_DIR)/*.c, $(SOURCES))
# INCLUDES = $(addprefix -I, $(addprefix $(SOURCE_DIR)/, $(SUB_DIRS)))

vpath %.c $(SRC_DIR)

CFLAGS += -I$(SOURCE_DIR)/core -I$(SOURCE_DIR)

define make-goal
$1/*.o: %.c
	$(CC) -I$(INCLUDES) -c $$< -o $$@
endef

build/$(NAME): $(OBJECTS) $(LIB_PCRE2)
	@ printf "Building Bird in %s mode into %s...\n" $(MODE) $(NAME)
	@ printf "%s %s %s\n" $(CC) $@ "$(CFLAGS)"
	@ mkdir -p build
	@ $(CC) $(CFLAGS) $^ -o $@

.PHONY: default

$(foreach bdir,$(BLD_DIRS),$(eval $(call make-goal,$(bdir))))