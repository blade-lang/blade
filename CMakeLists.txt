cmake_minimum_required(VERSION 3.18)

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

set(cfsp " ")

# set build type to release if not specified.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/blade")

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG OR CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG STREQUAL "")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}")
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG OR CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG STREQUAL "")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG OR CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG STREQUAL "")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE OR CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE STREQUAL "")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE OR CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE STREQUAL "")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE OR CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE STREQUAL "")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
endif()

set(BLADE_SOURCES
		src/blade_dict.c
		src/blade_file.c
		src/blade_list.c
		src/blade_string.c
		src/blade_range.c
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
		src/standard/reflect.c
		)

add_definitions(-DEXPORT_LIBS)

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
	target_sources(libblade PRIVATE src/blade_getopt.c)
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
check_function_exists("utime" HAVE_UTIME)
check_function_exists("basename" HAVE_BASENAME)
check_function_exists("dirname" HAVE_DIRNAME)

# Generate config.h
set(INCLUDE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/include-gen")
file(MAKE_DIRECTORY "${INCLUDE_GEN_DIR}")
configure_file(src/config.h.in "${INCLUDE_GEN_DIR}/config.h")
target_include_directories(libblade PUBLIC "${INCLUDE_GEN_DIR}")

set(LIBRARY_NAME_END "${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(SHARED_LIBRARY_NAME_END "${CMAKE_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}")

add_custom_target(blade_files)

# add dependencies
add_dependencies(libblade pcre2 blade_files)

set_target_properties(libblade
		PROPERTIES
		OUTPUT_NAME blade
		)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
endif()

# link pcre2
target_link_libraries(libblade PRIVATE pcre2)

if(UNIX)
	find_library(READLINE_LIBRARY readline REQUIRED)
	target_link_libraries(blade PRIVATE "${READLINE_LIBRARY}")
	target_link_libraries(libblade PRIVATE m)
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		target_link_libraries(libblade PRIVATE pthread)
		target_link_libraries(libblade PRIVATE dl)
	endif()
endif(UNIX)

target_link_libraries(blade PRIVATE libblade)

add_custom_command(TARGET blade_files POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/libs ${CMAKE_CURRENT_BINARY_DIR}/blade/libs
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/blade/tests
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/benchmarks ${CMAKE_CURRENT_BINARY_DIR}/blade/benchmarks
		COMMENT "Exporting libs, tests, and benchmark scripts..."
)

set(EXE_FILE "${OUTPUT_DIR}/blade${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_command(TARGET blade POST_BUILD
		COMMAND	"${EXE_FILE}" "${PROJECT_SOURCE_DIR}/scripts/make_blade.b" "${PROJECT_SOURCE_DIR}"
		COMMAND	"${EXE_FILE}" "${PROJECT_SOURCE_DIR}/scripts/ast.b" "${OUTPUT_DIR}/libs/ast"
		COMMENT "Generating Blade header and folders and package dependencies..."
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
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(supported)
	message(STATUS "  IPO/LTO ......................... : enabled")
	set_property(TARGET libblade PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
	message(STATUS "  IPO/LTO ......................... : not supported: <${error}>")
endif()

message(STATUS "")

# Generating single header file...
generate_export_header(libblade           # generates the export header shared_EXPORTS.h automatically
		EXPORT_MACRO_NAME EXPORT
		EXPORT_FILE_NAME export.h
		STATIC_DEFINE SHARED_EXPORTS_BUILT_AS_STATIC
)

if(UNIX OR APPLE)
	if(APPLE)
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};@executable_path")
	else()
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN")
	endif()
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

install(TARGETS blade DESTINATION ${OUTPUT_DIR})

enable_testing()

# define a function to simplify adding tests
function(add_blade_test target arg index result)
	#  message(STATUS "setting up test ${CMAKE_CURRENT_BINARY_DIR}/blade/${PROJECT_NAME} tests/${arg}.b")
	add_test(NAME ${arg}_${index} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/blade/${PROJECT_NAME} blade/tests/${arg}.b)
	set_tests_properties(${arg}_${index}
			PROPERTIES PASS_REGULAR_EXPRESSION ${result}
	)
endfunction(add_blade_test)

# do a bunch of result based tests
add_blade_test(blade anonymous 0 "works")
add_blade_test(blade anonymous 1 "is the best")
add_blade_test(blade assert 0 "AssertionError")
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
add_blade_test(blade die 0 "Exception")
add_blade_test(blade for 0 "address = Nigeria")
add_blade_test(blade for 1 "1 = 7")
add_blade_test(blade for 2 "n\na\nm\ne")
add_blade_test(blade for 3 "12\n13\n14\n15")
add_blade_test(blade for 4 "Richard\nAlex\nJustina")
add_blade_test(blade function 0 "outer")
add_blade_test(blade function 1 "<function test at 0")
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
add_blade_test(blade native 0 "10")
add_blade_test(blade native 1 "300")
add_blade_test(blade native 2 "\\[1, 2, 3\\]")
add_blade_test(blade native 3 "{name: Richard, age: 28}")
add_blade_test(blade native 4 "A class called A")
add_blade_test(blade native 5 "9227465\nTime taken")
add_blade_test(blade native 6 "1548008755920\nTime taken")
add_blade_test(blade pi 0 "3.141592653589734")
add_blade_test(blade scope 1 "inner\nouter")
add_blade_test(blade string 0 "25, This is john's LAST 20")
add_blade_test(blade try 0 "list index 10 out of range")
add_blade_test(blade using 0 "ten\nafter")
add_blade_test(blade var 0 "it works\n20\ntrue")
add_blade_test(blade while 0 "x = 51")
