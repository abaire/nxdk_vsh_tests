include(ExternalProject)
include(FetchContent)

find_package(NXDK REQUIRED)
find_package(NXDK_SDL2 REQUIRED)
find_package(NXDK_SDL_TTF REQUIRED)

ExternalProject_Add(
        _nv2a_vsh_cpu
        PREFIX _nv2a_vsh_cpu
        GIT_REPOSITORY https://github.com/abaire/nv2a_vsh_cpu.git
        GIT_TAG 1115255708c10c4841b65dcd2223262e7a316598
        INSTALL_COMMAND ""
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -Dnv2a_vsh_cpu_UNIT_TEST=OFF
        BUILD_BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/_nv2a_vsh_cpu/src/_nv2a_vsh_cpu-build/libnv2a_vsh_cpu.lib
)
ExternalProject_Get_Property(_nv2a_vsh_cpu INSTALL_DIR)
set(nv2a_vsh_cpu_lib_dir ${INSTALL_DIR}/src/_nv2a_vsh_cpu-build)
set(nv2a_vsh_cpu_include_dir ${INSTALL_DIR}/src/_nv2a_vsh_cpu/src)

message ("Foo ${nv2a_vsh_cpu_lib_dir}  ${CMAKE_CURRENT_BINARY_DIR}")

add_library(
        nv2a_vsh_cpu
        INTERFACE
        _nv2a_vsh_cpu
)

target_include_directories(
        nv2a_vsh_cpu
        INTERFACE
        "${nv2a_vsh_cpu_include_dir}"
)

target_link_libraries(
        nv2a_vsh_cpu
        INTERFACE
        "${nv2a_vsh_cpu_lib_dir}/libnv2a_vsh_cpu.lib"
)

add_dependencies(nv2a_vsh_cpu _nv2a_vsh_cpu)


ExternalProject_Add(
        _pbkit_sdl_gpu
        PREFIX _pbkit_sdl_gpu
        GIT_REPOSITORY https://github.com/abaire/pbkit-sdl-gpu.git
        GIT_TAG 0503a8d5ab2f21ca1121ac03afd6eaef1729bb22
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        BUILD_BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/_pbkit_sdl_gpu/lib/libpbkit_sdl_gpu.lib
)
ExternalProject_Get_Property(_pbkit_sdl_gpu INSTALL_DIR)
set(pbkit_sdl_gpu_lib_dir ${INSTALL_DIR}/lib)
set(pbkit_sdl_gpu_include_dir ${INSTALL_DIR}/pbkit_sdl_gpu)

add_library(
        pbkit_sdl_gpu
        INTERFACE
        _pbkit_sdl_gpu
)

target_include_directories(
        pbkit_sdl_gpu
        INTERFACE
        "${pbkit_sdl_gpu_include_dir}"
)

target_link_libraries(
        pbkit_sdl_gpu
        INTERFACE
        "${pbkit_sdl_gpu_lib_dir}/libpbkit_sdl_gpu.lib"
)

add_dependencies(pbkit_sdl_gpu _pbkit_sdl_gpu)


FetchContent_Declare(
        _sdl_fontcache
        GIT_REPOSITORY https://github.com/grimfang4/SDL_FontCache.git
        GIT_TAG c37a4030e1a1ad22131acdf62e093001e67905d6
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        SOURCE_SUBDIR __do_not_build
)
FetchContent_MakeAvailable(_sdl_fontcache)
FetchContent_GetProperties(_sdl_fontcache SOURCE_DIR sdl_fontcache_SOURCE_DIR)

add_library(
        sdl_fontcache
        "${sdl_fontcache_SOURCE_DIR}/SDL_FontCache.c"
        "${sdl_fontcache_SOURCE_DIR}/SDL_FontCache.h"
)

target_link_libraries(
        sdl_fontcache
        PRIVATE
        pbkit_sdl_gpu
        NXDK::SDL2
        NXDK::SDL_TTF
)

target_include_directories(
        sdl_fontcache
        PUBLIC
        "${sdl_fontcache_SOURCE_DIR}"
        "${sdl2_ttf_INCLUDE_DIR}"
)

target_compile_definitions(
        sdl_fontcache
        PUBLIC
        FC_USE_SDL_GPU
)

if (NOT NO_OPT)
    set_opt_compile_and_link_options(sdl_fontcache)
else ()
    set_compile_and_link_options(sdl_fontcache)
endif ()


# Fast png compression.
add_library(
        fpng
        EXCLUDE_FROM_ALL
        fpng/src/fpng.cpp
        fpng/src/fpng.h
)
target_compile_options(
        fpng
        PRIVATE
        -O3
        -Wno-everything
        -DFPNG_NO_STDIO=1
        -DFPNG_NO_SSE=1
)
target_link_options(fpng PRIVATE "/debug:none")

# Full featured printf.
add_library(
        printf
        EXCLUDE_FROM_ALL
        printf/printf.c
        printf/printf.h
)
target_compile_options(
        printf
        PRIVATE
        -O3
        -Wno-everything
)
target_link_options(printf PRIVATE "/debug:none")

# Floating point comparison functions
add_library(
        compare_as_int
        EXCLUDE_FROM_ALL
        compareasint/compare_as_int.cpp
        compareasint/compare_as_int.h
)
target_compile_options(
        compare_as_int
        PRIVATE
        -O3
        -Wno-everything
)
target_link_options(compare_as_int PRIVATE "/debug:none")


## 3D math routines
#add_library(
#        xbox_math3d
#        EXCLUDE_FROM_ALL
#        xbox_math3d/src/xbox_math_d3d.cpp
#        xbox_math3d/src/xbox_math_d3d.h
#        xbox_math3d/src/xbox_math_matrix.cpp
#        xbox_math3d/src/xbox_math_matrix.h
#        xbox_math3d/src/xbox_math_types.cpp
#        xbox_math3d/src/xbox_math_types.h
#        xbox_math3d/src/xbox_math_util.cpp
#        xbox_math3d/src/xbox_math_util.h
#        xbox_math3d/src/xbox_math_vector.cpp
#        xbox_math3d/src/xbox_math_vector.h
#)
#target_include_directories(
#        xbox_math3d
#        INTERFACE
#        xbox_math3d/src
#)
#if (NOT NO_OPT)
#    target_compile_options(
#            xbox_math3d
#            PRIVATE
#            -O3
#            -Wno-everything
#    )
#    target_link_options(xbox_math3d PRIVATE "/debug:none")
#else ()
#    if (CMAKE_BUILD_TYPE MATCHES Debug)
#        target_compile_options(
#                xbox_math3d
#                PRIVATE
#                -g
#                -gdwarf-4
#                -O0
#                -Wall
#                -fstandalone-debug
#        )
#        target_link_options(xbox_math3d PRIVATE "/debug:full")
#    else ()
#        target_compile_options(
#                xbox_math3d
#                PRIVATE
#                -O3
#                -fno-strict-aliasing
#                -Wall
#        )
#        target_link_options(xbox_math3d PRIVATE "/debug:none")
#    endif ()
#endif ()
