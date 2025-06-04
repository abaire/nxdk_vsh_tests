cmake_minimum_required(VERSION 3.30)
project(nxdk_vsh_tests)

set(
        CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/"
)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Generate verbose Makefile")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_CMAKE_PROCESSING_LANGUAGE "CXX")

include(CheckSymbolExists)
include(FindPkgConfig)

# Determine if this build is for the Xbox or the host system.
if (CMAKE_TOOLCHAIN_FILE MATCHES "toolchain-nxdk.cmake")
    set(IS_TARGET_BUILD ON)
else ()
    message (FATAL_ERROR "This project must be built with the nxdk toolchain")
endif ()


option(
        NO_OPT
        "Suppress optimization on core files that are typically optimized regardless of release/debug configuration"
        OFF
)

option(
        DUMP_CONFIG_FILE
        "Write a default config file and exit instead of running tests."
        OFF
)

option(
        DISABLE_AUTORUN
        "Prevents the tests from being executed without user input."
        OFF
)

option(
        AUTORUN_IMMEDIATELY
        "Remove the delay for input before starting automated testing."
        OFF
)

option(
        ENABLE_SHUTDOWN
        "Cause the program to shut down the xbox on completion instead of rebooting."
        OFF
)

set(
        DEFAULT_OUTPUT_DIRECTORY_PATH
        "e:/nxdk_vsh_tests"
        CACHE STRING
        "Absolute XBOX-path to a directory into which test output will be written."
)

macro(set_opt_compile_and_link_options TARGET_NAME)
    target_compile_options(
            "${TARGET_NAME}"
            PRIVATE
            -O3
            -fno-strict-aliasing
            -Wall
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-macro-redefined>   # Suppress warning from NXDK undef of __STDC_NO_THREADS__
            -D_USE_MATH_DEFINES
    )
    target_link_options(
            "${TARGET_NAME}"
            PRIVATE
            "/debug:none"
    )
endmacro()

macro(set_compile_and_link_options TARGET_NAME)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        target_compile_options(
                "${TARGET_NAME}"
                PRIVATE
                -g
                -gdwarf-4
                -O0
                -Wall
                -fstandalone-debug
                $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-macro-redefined>   # Suppress warning from NXDK undef of __STDC_NO_THREADS__
                -D_USE_MATH_DEFINES
        )
        target_link_options(
                "${TARGET_NAME}"
                PRIVATE
                "/debug:full"
        )
    else ()
        set_opt_compile_and_link_options("${TARGET_NAME}")
    endif ()
endmacro()

add_subdirectory(src)
add_subdirectory(third_party)
