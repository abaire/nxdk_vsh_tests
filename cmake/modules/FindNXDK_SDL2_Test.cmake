if (NOT TARGET NXDK::SDL2_Test)

    add_library(nxdk_sdl2_test STATIC IMPORTED)

    set (sdl2_test_LIBRARY "${NXDK_DIR}/lib/SDL2_test.lib")
    set_target_properties(
            nxdk_sdl2_test
            PROPERTIES
            IMPORTED_LOCATION "${sdl2_test_LIBRARY}"
    )

    add_library(NXDK::SDL2_Test INTERFACE IMPORTED)
    target_link_libraries(
            NXDK::SDL2_Test
            INTERFACE
            nxdk_sdl2_test
    )

    set (sdl2_test_INCLUDE_DIR "${NXDK_DIR}/lib/sdl/SDL_ttf")
    target_include_directories(
            NXDK::SDL2_Test
            SYSTEM INTERFACE
            "${sdl2_test_INCLUDE_DIR}"
    )

endif ()
