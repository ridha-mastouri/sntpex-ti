cmake_minimum_required(VERSION 3.13)

project(sntpex_ti
    VERSION 1.0.0
    DESCRIPTION "Extended SNTP library for TI SimpleLink"
    LANGUAGES C
)

add_library(sntpex_ti STATIC
    src/sntp_ex_lib_ti.c
)

target_include_directories(sntpex_ti
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sntpex_ti PUBLIC c99)

target_compile_options(sntpex_ti PRIVATE
    -Wall
    -Wextra
)

# NOTE:
# SimpleLink SDK headers and libraries must be provided
# by the application project using this library.
