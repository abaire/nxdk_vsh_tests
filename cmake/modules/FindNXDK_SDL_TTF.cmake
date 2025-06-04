if (NOT TARGET NXDK::SDL_TTF)

    add_library(nxdk_sdl_ttf STATIC IMPORTED)

    set (sdl2_ttf_LIBRARY "${NXDK_DIR}/lib/libSDL_ttf.lib")
    set_target_properties(
            nxdk_sdl_ttf
            PROPERTIES
            IMPORTED_LOCATION "${sdl2_ttf_LIBRARY}"
    )

    add_library(nxdk_freetype STATIC IMPORTED)
    set (freetype_LIBRARY "${NXDK_DIR}/lib/libfreetype.lib")
    set_target_properties(
            nxdk_freetype
            PROPERTIES
            IMPORTED_LOCATION "${freetype_LIBRARY}"
    )

    add_library(NXDK::SDL_TTF INTERFACE IMPORTED)
    target_link_libraries(
            NXDK::SDL_TTF
            INTERFACE
            nxdk_freetype
            nxdk_sdl_ttf
    )

    set (sdl2_ttf_INCLUDE_DIR "${NXDK_DIR}/lib/sdl/SDL_ttf")
    target_include_directories(
            NXDK::SDL_TTF
            SYSTEM INTERFACE
            "${sdl2_ttf_INCLUDE_DIR}"
    )

endif ()
