cmake_minimum_required(VERSION 3.18...3.26)

project(blade C)

# required modules for our task
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(ExternalProject)
include(GenerateExportHeader)

subdirs(thirdparty packages)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# set build type to release if not specified.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/blade")

# Just for debugging availability in tools like CLion
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY OR CMAKE_RUNTIME_OUTPUT_DIRECTORY STREQUAL "")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
endif()

if(UNIX)
	if(APPLE)
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};@loader_path")
	else()
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN")
	endif()
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

set(BLADE_SOURCES
		src/dict.c
		src/file.c
		src/list.c
		src/bstring.c
		src/range.c
		src/blob.c
		src/bytes.c
		src/compiler.c
		src/debug.c
		src/memory.c
		src/module.c
		src/native.c
		src/object.c
		src/pathinfo.c
		src/scanner.c
		src/table.c
		src/util.c
		src/utf8.c
		src/value.c
		src/vm.c
		# standard module
		src/standard/base64.c
		src/standard/date.c
		src/standard/io.c
		src/standard/math.c
		src/standard/os.c
		src/standard/socket.c
		src/standard/hash.c
		src/standard/process.c
		src/standard/reflect.c
		src/standard/struct.c
    src/standard/thread.c)

add_definitions(-DEXPORT_LIBS)

if(UNIX AND NOT APPLE AND MUSL)
	add_definitions(-D__MUSL__)
endif()

if(UNIX)
	add_compile_options(-Wno-deprecated-declarations)
endif()

include_directories(src thirdparty)

if(WIN32)
	add_library(libblade ${BLADE_SOURCES})
else()
	add_library(libblade SHARED ${BLADE_SOURCES})
endif()

add_executable(blade src/blade.c)

if(WIN32)
	target_link_libraries(libblade PRIVATE wsock32 ws2_32)

	if(MSVC)
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			set(CMAKE_DEBUG_POSTFIX "d")
		else()
			set(CMAKE_DEBUG_POSTFIX "")
		endif()
	else()
		set(CMAKE_DEBUG_POSTFIX "")
	endif(MSVC)
endif()

# Header checks
check_include_file("getopt.h" HAVE_GETOPT_H)
if(NOT "${HAVE_GETOPT_H}")
	target_sources(libblade PRIVATE src/bgetopt.c)
endif()
check_include_file("termios.h" HAVE_TERMIOS_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/utsname.h" HAVE_SYS_UTSNAME_H)
check_include_file("sys/errno.h" HAVE_SYS_ERRNO_H)
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("dlfcn.h" HAVE_DLFCN_H)

# Function checks
check_function_exists("asprintf" HAVE_ASPRINTF)
if(NOT "${HAVE_ASPRINTF}")
	target_link_libraries(libblade PRIVATE asprintf)
endif()
check_function_exists("strsep" HAVE_STRSEP)
if(NOT "${HAVE_STRSEP}")
	target_link_libraries(libblade PRIVATE strsep)
endif()
check_function_exists("gettimeofday" HAVE_GETTIMEOFDAY)
if(NOT "${HAVE_GETTIMEOFDAY}")
	target_link_libraries(libblade PRIVATE gettimeofday)
endif()
check_function_exists("timespec_get" HAVE_TIMESPEC_GET)

check_function_exists("utime" HAVE_UTIME)
check_function_exists("basename" HAVE_BASENAME)
check_function_exists("dirname" HAVE_DIRNAME)
check_function_exists("sysconf" HAVE_SYSCONF)
check_function_exists("sysctlbyname" HAVE_SYSCTLBYNAME)
check_function_exists("setjmp" HAVE_SETJMP)

# Generate config.h
set(INCLUDE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/include-gen")
file(MAKE_DIRECTORY "${INCLUDE_GEN_DIR}")
configure_file(src/config.h.in "${INCLUDE_GEN_DIR}/config.h")
target_include_directories(libblade PUBLIC "${INCLUDE_GEN_DIR}")

set(LIBRARY_NAME_END "${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(SHARED_LIBRARY_NAME_END "${CMAKE_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}")

set_target_properties(libblade
		PROPERTIES
		OUTPUT_NAME blade
		)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
if(MINGW OR MUSL)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-stringop-overread")
endif()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PCRE2_USE_STATIC_LIBS ON)
set(PCRE2_BUILD_TESTS OFF)
set(PCRE2_BUILD_PCRE2GREP OFF)
include(FetchContent)
FetchContent_Declare(pcre2
		URL      https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.43/pcre2-10.43.tar.gz
		URL_HASH SHA256=889d16be5abb8d05400b33c25e151638b8d4bac0e2d9c76e9d6923118ae8a34e
		)
FetchContent_MakeAvailable(pcre2)

if(NOT pcre2_POPULATED)
	FetchContent_Populate(pcre2)
	add_subdirectory(${pcre2_SOURCE_DIR} ${pcre2_BUILD_DIR})
endif()

# link pcre2
target_link_libraries(libblade PRIVATE pcre2-8-static)
target_include_directories(libblade PUBLIC "${pcre2_BINARY_DIR}")

if(UNIX)
	#	Add linenoise as dependency and link it.
	add_dependencies(libblade linenoise)
	target_link_libraries(libblade PRIVATE linenoise)

	target_link_libraries(libblade PRIVATE m)
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		target_link_libraries(libblade PRIVATE dl)
	endif()

	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	target_link_libraries(libblade PRIVATE Threads::Threads)
	#	target_link_libraries(libblade PRIVATE pthread)
endif(UNIX)

if(WIN32)
	add_dependencies(libblade mman)
	target_link_libraries(libblade PRIVATE mman)

	find_package(PThreads4W REQUIRED)
	target_link_libraries(libblade PRIVATE PThreads4W::PThreads4W)

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pragmas")
endif()

target_link_libraries(blade PRIVATE libblade)

add_custom_target(blade_lib_files ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/libs" "${OUTPUT_DIR}/libs"
#		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/tests" "${OUTPUT_DIR}/tests"
#		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/benchmarks" "${OUTPUT_DIR}/benchmarks"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/apps" "${OUTPUT_DIR}/apps"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/bins" "${OUTPUT_DIR}"
		COMMENT "Exporting libs, tests, and benchmark scripts..."
)

add_dependencies(blade blade_lib_files)

set(EXE_FILE "${OUTPUT_DIR}/$<TARGET_NAME:blade>${CMAKE_EXECUTABLE_SUFFIX}")
set(LIBRARY_FILE "${OUTPUT_DIR}/$<TARGET_NAME:libblade>${CMAKE_SHARED_LIBRARY_SUFFIX}")

add_custom_command(TARGET blade POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:blade> ${EXE_FILE}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libblade> ${LIBRARY_FILE}
		COMMAND ${EXE_FILE} "${PROJECT_SOURCE_DIR}/scripts/make_blade.b" "${PROJECT_SOURCE_DIR}"
		# The first one is useful for development purposes but doesn't in any way harm release.
		COMMAND ${EXE_FILE} "${PROJECT_SOURCE_DIR}/scripts/ast.b" "${PROJECT_SOURCE_DIR}/libs/ast"
		COMMAND ${EXE_FILE} "${PROJECT_SOURCE_DIR}/scripts/ast.b" "${OUTPUT_DIR}/libs/ast"
		COMMENT "Generating Blade header and copy output..."
)

string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)

string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
message(STATUS "")
message(STATUS "Blade configuration summary")
message(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
message(STATUS "  Build type ...................... : ${buildtype}")
message(STATUS "  C compiler flags ................ : ${${CMAKE_C_FLAGS}${cfsp}}${CMAKE_C_FLAGS_${buildtype}}")

# for LTO
if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	include(CheckIPOSupported)
	check_ipo_supported(RESULT LTOSupported OUTPUT LTOSupportError)
	if(LTOSupported)
		message(STATUS "  IPO/LTO ......................... : enabled")
		set_property(TARGET libblade PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(STATUS "  IPO/LTO ......................... : not supported: <${LTOSupportError}>")
	endif()
endif()

# Generating single header file...
generate_export_header(libblade           # generates the export header shared_EXPORTS.h automatically
		EXPORT_MACRO_NAME EXPORT
		EXPORT_FILE_NAME export.h
		STATIC_DEFINE SHARED_EXPORTS_BUILT_AS_STATIC
)

enable_testing()
add_subdirectory(tests)

# Replace install() with conditional installation.
macro(install)
	_install(${ARGN})
endmacro()
