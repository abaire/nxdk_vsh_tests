include(NV2A_VSH REQUIRED)
include(XBEUtils REQUIRED)

find_package(NXDK REQUIRED)
find_package(NXDK_SDL2 REQUIRED)
find_package(NXDK_SDL2_Image REQUIRED)
find_package(NXDK_SDL2_Test REQUIRED)
find_package(NXDK_SDL_TTF REQUIRED)
find_package(Threads REQUIRED)

configure_file(configure.h.in configure.h)

include(FetchContent)
FetchContent_Declare(
        nxdkftplib
        GIT_REPOSITORY https://github.com/abaire/nxdk_ftp_client_lib.git
        GIT_TAG a6c469260b5b1dc17bad7ea68f4ed9f7d3940853
)
FetchContent_MakeAvailable(nxdkftplib)

FetchContent_Declare(
        xbox_math3d
        GIT_REPOSITORY https://github.com/abaire/xbox_math3d.git
        GIT_TAG ec4256026136c89d530ee1cd8fc0e45ea7786ced
)
FetchContent_MakeAvailable(xbox_math3d)

# Vertex shaders.
generate_nv2a_vshinc_files(
        generated_nv2a_vertex_shaders
        SOURCES
        shaders/americas_army_shader.vsh
        shaders/clear_state.vsh
        shaders/compute_footer.vsh
        shaders/exceptional_float_passthrough.vsh
        shaders/ilu_exp_passthrough.vsh
        shaders/ilu_lit_passthrough.vsh
        shaders/ilu_log_passthrough.vsh
        shaders/ilu_rcc_passthrough.vsh
        shaders/ilu_rcp.vsh
        shaders/ilu_rcp_passthrough.vsh
        shaders/ilu_rsq_passthrough.vsh
        shaders/mac_add.vsh
        shaders/mac_add_passthrough.vsh
        shaders/mac_arl_passthrough.vsh
        shaders/mac_dp3_passthrough.vsh
        shaders/mac_dp4_passthrough.vsh
        shaders/mac_dph_passthrough.vsh
        shaders/mac_dst_passthrough.vsh
        shaders/mac_mad_passthrough.vsh
        shaders/mac_max_passthrough.vsh
        shaders/mac_min_passthrough.vsh
        shaders/mac_mov.vsh
        shaders/mac_mov_passthrough.vsh
        shaders/mac_mul_passthrough.vsh
        shaders/mac_sge_passthrough.vsh
        shaders/mac_slt_passthrough.vsh
        shaders/paired_ilu_non_r1_temp_out.vsh
        shaders/spyvsspymenu.vsh
        shaders/vertex_data_array_format_passthrough.vsh
)

# Sources that should be optimized regardless of standard debug settings.
add_library(
        optimized_sources
        STATIC
        debug_output.cpp
        debug_output.h
        logger.cpp
        logger.h
        main.cpp
        menu_item.cpp
        menu_item.h
        nxdk_ext.h
        pbkit_ext.cpp
        pbkit_ext.h
        pgraph_diff_token.cpp
        pgraph_diff_token.h
        pushbuffer.cpp
        pushbuffer.h
        test_driver.cpp
        test_driver.h
        test_host.cpp
        test_host.h
        text_overlay.cpp
        text_overlay.h
        shaders/vertex_shader_program.cpp
        shaders/vertex_shader_program.h
)

if (NOT NO_OPT)
    set_opt_compile_and_link_options(optimized_sources)
else ()
    set_compile_and_link_options(optimized_sources)
endif ()

target_include_directories(
        optimized_sources
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(
        optimized_sources
        PUBLIC
        generated_nv2a_vertex_shaders
        xbox_math3d
        PRIVATE
        fpng
        nv2a_vsh_cpu
        pbkit_sdl_gpu
        sdl_fontcache
        printf
        NXDK::NXDK
        NXDK::NXDK_CXX
        NXDK::SDL2
        NXDK::SDL2_Image
        NXDK::SDL2_Test
        NXDK::SDL_TTF
)

add_executable(
        nxdk_vsh_tests
        tests/americasarmyshader.cpp
        tests/americasarmyshader.h
        tests/cpu_shader_tests.cpp
        tests/cpu_shader_tests.h
        tests/exceptional_float_tests.cpp
        tests/exceptional_float_tests.h
        tests/ilu_rcp_tests.cpp
        tests/ilu_rcp_tests.h
        tests/mac_add_tests.cpp
        tests/mac_add_tests.h
        tests/mac_mov_tests.cpp
        tests/mac_mov_tests.h
        tests/paired_ilu_tests.cpp
        tests/paired_ilu_tests.h
        tests/spyvsspymenu.cpp
        tests/spyvsspymenu.h
        tests/test_suite.cpp
        tests/test_suite.h
        tests/vertex_data_array_format_tests.cpp
        tests/vertex_data_array_format_tests.h
)

# Pull debug info out of the binary into a host-side linked binary.
split_debug(nxdk_vsh_tests)

set(EXECUTABLE_BINARY "${CMAKE_CURRENT_BINARY_DIR}/nxdk_vsh_tests.exe")
set_compile_and_link_options(nxdk_vsh_tests)
target_include_directories(
        nxdk_vsh_tests
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(
        nxdk_vsh_tests
        PUBLIC
        compare_as_int
        fpng
        nv2a_vsh_cpu
        optimized_sources
        pbkit_sdl_gpu
        printf
        sdl_fontcache
        NXDK::NXDK
        NXDK::NXDK_CXX
        NXDK::SDL2
        NXDK::SDL2_Image
)

add_xbe(
        xbe_file "${EXECUTABLE_BINARY}"
        TITLE "VSH Tests"
        RESOURCE_ROOTS
        "${CMAKE_SOURCE_DIR}/resources"
        RESOURCE_DIRS
        "${CMAKE_SOURCE_DIR}/resources"
)
add_xiso(nxdk_vsh_tests_xiso xbe_file)


# Deployment via xbdm_gdb_bridge - https://github.com/abaire/xbdm_gdb_bridge
#set(XBDM_GDB_BRIDGE $ENV{XBDM_GDB_BRIDGE})
#set(XBOX $ENV{XBOX})
#if (DEFINED ENV{REMOTE_PATH})
#    set(REMOTE_PATH $ENV{REMOTE_PATH})
#else ()
#    set(REMOTE_PATH "e:\\\\cmake_pgraph")
#endif ()
#
#add_custom_target(
#        deploy_with_xbdm
#        COMMAND "${XBDM_GDB_BRIDGE}" "${XBOX}" -- %syncdir "${XBE_STAGING_DIR}" "${REMOTE_PATH}" -d
#        DEPENDS "${XBE_OUTPUT_PATH}"
#)
