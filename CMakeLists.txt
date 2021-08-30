cmake_minimum_required(VERSION 3.18)

project(blade C)

# required modules for our task
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(ExternalProject)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_VERBOSE_MAKEFILE on)

set(cfsp " ")
set(SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/..)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})

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
		src/standard/hash.c)

set(PCRE2_SOURCES
		src/pcre/pcre2_auto_possess.c
		src/pcre/pcre2_chartables.c
		src/pcre/pcre2_compile.c
		src/pcre/pcre2_config.c
		src/pcre/pcre2_context.c
		src/pcre/pcre2_convert.c
		src/pcre/pcre2_dfa_match.c
		src/pcre/pcre2_error.c
		src/pcre/pcre2_extuni.c
		src/pcre/pcre2_find_bracket.c
		src/pcre/pcre2_jit_compile.c
		src/pcre/pcre2_maketables.c
		src/pcre/pcre2_match.c
		src/pcre/pcre2_match_data.c
		src/pcre/pcre2_newline.c
		src/pcre/pcre2_ord2utf.c
		src/pcre/pcre2_pattern_info.c
		src/pcre/pcre2_script_run.c
		src/pcre/pcre2_serialize.c
		src/pcre/pcre2_string_utils.c
		src/pcre/pcre2_study.c
		src/pcre/pcre2_substitute.c
		src/pcre/pcre2_substring.c
		src/pcre/pcre2_tables.c
		src/pcre/pcre2_ucd.c
		src/pcre/pcre2_valid_utf.c
		src/pcre/pcre2_xclass.c
)

add_definitions(-DHAVE_CONFIG_H)

include_directories(src)

add_library(libblade ${BLADE_SOURCES})
add_library(libpcre src/pcre/pcre2.h ${PCRE2_SOURCES} src/pcre/config.h)
add_executable(blade src/blade.c)

set_target_properties(libpcre PROPERTIES
		COMPILE_DEFINITIONS PCRE2_CODE_UNIT_WIDTH=8
		MACHO_COMPATIBILITY_VERSION "10:0:10"
		MACHO_CURRENT_VERSION "10:0:10"
		VERSION 10
		SOVERSION 10)

if(MSVC)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_DEBUG_POSTFIX "d")
	else()
		set(CMAKE_DEBUG_POSTFIX "")
	endif()
else()
	set(CMAKE_DEBUG_POSTFIX "")
endif(MSVC)

set(LIBRARY_NAME_END "${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(SHARED_LIBRARY_NAME_END "${CMAKE_DEBUG_POSTFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}")

add_custom_target(blade_files)

# add dependencies
add_dependencies(libblade libpcre blade_files)

set_target_properties(blade
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
		ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
		)

set_target_properties(libblade
		PROPERTIES
		OUTPUT_NAME blade
		ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
		)

#if(MSVC)
#  set_target_properties(blade PROPERTIES COMPILE_FLAGS "/GL")
#  set_target_properties(libblade PROPERTIES COMPILE_FLAGS "/GL")
#  set_target_properties(blade PROPERTIES LINK_FLAGS "/LTCG")
#  set_target_properties(libblade PROPERTIES LINK_FLAGS "/LTCG")
#endif(MSVC)

target_link_libraries(libblade libpcre)

if(UNIX)
	target_link_libraries(libblade readline)
	target_link_libraries(libblade m)
endif(UNIX)

target_link_libraries(blade libblade)

add_custom_command(TARGET blade_files POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/libs ${CMAKE_CURRENT_BINARY_DIR}/bin/libs
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/bin/tests
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/benchmarks ${CMAKE_CURRENT_BINARY_DIR}/bin/benchmarks

)

string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)

string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
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

enable_testing()

# define a function to simplify adding tests
function(add_blade_test target arg index result)
	#  message(STATUS "setting up test ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME} tests/${arg}.b")
	add_test(NAME ${arg}_${index} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME} bin/tests/${arg}.b)
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
