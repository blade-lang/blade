add_library(pcre2 OBJECT
        config.h
        pcre2.h
        pcre2_auto_possess.c
        pcre2_chartables.c
        pcre2_compile.c
        pcre2_config.c
        pcre2_context.c
        pcre2_convert.c
        pcre2_dfa_match.c
        pcre2_error.c
        pcre2_extuni.c
        pcre2_find_bracket.c
        pcre2_fuzzsupport.c
        pcre2_internal.h
        pcre2_intmodedep.h
        pcre2_jit_compile.c
        pcre2_maketables.c
        pcre2_match.c
        pcre2_match_data.c
        pcre2_newline.c
        pcre2_ord2utf.c
        pcre2_pattern_info.c
        pcre2_script_run.c
        pcre2_serialize.c
        pcre2_string_utils.c
        pcre2_study.c
        pcre2_substitute.c
        pcre2_substring.c
        pcre2_tables.c
        pcre2_ucd.c
        pcre2_ucp.h
        pcre2_valid_utf.c
        pcre2_xclass.c)

add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DLINK_SIZE=4)

set_target_properties(pcre2 PROPERTIES
        COMPILE_DEFINITIONS PCRE2_CODE_UNIT_WIDTH=8
        MACHO_COMPATIBILITY_VERSION "10:0:10"
        MACHO_CURRENT_VERSION "10:0:10"
        VERSION 10
        SOVERSION 10)

target_include_directories(pcre2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
