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
		src/standard/array.c
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
		)

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
target_link_libraries(libblade PRIVATE threads)

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
		target_link_libraries(libblade PRIVATE pthread)
		target_link_libraries(libblade PRIVATE dl)
	endif()
endif(UNIX)

if(WIN32)
	add_dependencies(libblade mman)
	target_link_libraries(libblade PRIVATE mman)
endif()

target_link_libraries(blade PRIVATE libblade)

add_custom_target(blade_lib_files ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/libs" "${OUTPUT_DIR}/libs"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/tests" "${OUTPUT_DIR}/tests"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/benchmarks" "${OUTPUT_DIR}/benchmarks"
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

# define a function to simplify adding tests
function(add_blade_test target arg index result)
	  message(STATUS "setting up test ${arg}_${index} -> tests/${arg}.b")
	add_test(NAME ${arg}_${index} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/blade/${PROJECT_NAME} blade/tests/${arg}.b)
	set_tests_properties(${arg}_${index}
			PROPERTIES PASS_REGULAR_EXPRESSION ${result}
	)
endfunction(add_blade_test)

# do a bunch of result based tests
add_blade_test(blade anonymous 0 "works")
add_blade_test(blade anonymous 1 "is the best")
add_blade_test(blade assert 0 "Illegal State:")
add_blade_test(blade assert 1 "empty list expected")
add_blade_test(blade bytes 0 "\\(0 0 0 0 0\\)")
add_blade_test(blade bytes 1 "HELLO")
add_blade_test(blade class 0 "3")
add_blade_test(blade class 1 "10")
add_blade_test(blade class 2 "scone with berries and cream")
add_blade_test(blade class 3 "Person is shouting")
add_blade_test(blade class 4 "2001")
add_blade_test(blade class 5 "Richard. You are 15")
add_blade_test(blade class 6 "Jane. You are 25")
add_blade_test(blade class 7 "A")
add_blade_test(blade class 8 "Name is set")
add_blade_test(blade class 9 "cannot call private method '_echo'")
add_blade_test(blade closure 0 "outer\nreturn from outer\ncreate inner closure\nvalue\n1499998500000")
add_blade_test(blade condition 0 "Test passed\nTest passed")
add_blade_test(blade dictionary 0 "age: 28")
add_blade_test(blade dictionary 1 "Plot 10,")
add_blade_test(blade dictionary 2 "30")
add_blade_test(blade dictionary 3 "children: 2")
add_blade_test(blade dictionary 4 "{name: Richard, age: 30}")
add_blade_test(blade dictionary 5 "{name: Richard, age: 53}")
add_blade_test(blade dictionary 6 "{name: Alexander, age: 30}")
add_blade_test(blade do 0 "10\n9")
add_blade_test(blade do 1 "2\n1")
add_blade_test(blade die 0 "Exception")
add_blade_test(blade for 0 "address = Nigeria")
add_blade_test(blade for 1 "1 = 7")
add_blade_test(blade for 2 "n\na\nm\ne")
add_blade_test(blade for 3 "12\n13\n14\n15")
add_blade_test(blade for 4 "Richard\nAlex\nJustina")
add_blade_test(blade function 0 "outer")
add_blade_test(blade function 1 "<function test\\(0\\) at 0")
add_blade_test(blade function 2 "It works! inner")
add_blade_test(blade function 3 "Richard")
add_blade_test(blade function 4 "\\[James\\]")
add_blade_test(blade function 5 "Sin 10 = -0.5440211108893656")
add_blade_test(blade if 0 "It works")
add_blade_test(blade if 1 "Nope")
add_blade_test(blade if 2 "2 is less than 5")
add_blade_test(blade if 3 "Ok")
add_blade_test(blade import 0 "Richard,")
add_blade_test(blade import 1 "Alagbaa Estate")
add_blade_test(blade import 2 "It works! inner")
add_blade_test(blade import 3 "Sin 10 =")
add_blade_test(blade import 4 "3.141592653589734")
add_blade_test(blade iter 0 "The new x = 0")
add_blade_test(blade list 0 "\\[\\[1, 2, 4], \\[4, 5, 6\\], \\[7, 8, 9\\]\\]")
add_blade_test(blade logarithm 0 "3.044522437723423\n3.044522437723423")
add_blade_test(blade native 0 "10\n300\n\\[1, 2, 3\\]\n{name: Richard, age: 28}\nA class called A\n9227465\nTime taken")
add_blade_test(blade native 1 "1548008755920\nTime taken")
add_blade_test(blade pi 0 "3.141592653589734")
add_blade_test(blade scope 1 "inner\nouter")
add_blade_test(blade string 0 "25, This is john's LAST 20")
add_blade_test(blade try 0 "Second exception thrown")
add_blade_test(blade try 1 "Despite the error, I run because I am in finally")
add_blade_test(blade try 2 "I am a thrown exception")
add_blade_test(blade try 3 "Try block called")
add_blade_test(blade try 4 "Final block called")
add_blade_test(blade try 5 "Error occurred, but I will still run")
add_blade_test(blade try 6 "message: I am a thrown exception")
add_blade_test(blade try 7 "list index 8 out of range")
add_blade_test(blade try 8 "list index 10 out of range")
add_blade_test(blade using 0 "ten\nafter")
add_blade_test(blade var 0 "it works\n20\ntrue")
add_blade_test(blade while 0 "x = 51")

# Replace install() with conditional installation.
macro(install)
	_install(${ARGN})
endmacro()
