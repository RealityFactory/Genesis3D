set(OGLDRV_SOURCES

    GLX.c
    OglDrv.c
    OglMisc.c
    Render.c
    THandle.c
)

add_library(OglDrv SHARED
    ${OGLDRV_SOURCES}
)

target_include_directories(OglDrv PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}

    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/Engine
    ${PROJECT_SOURCE_DIR}/Engine/Drivers
    ${PROJECT_SOURCE_DIR}/Support
    ${PROJECT_SOURCE_DIR}/Math
    ${PROJECT_SOURCE_DIR}/Bitmap
)

target_compile_definitions(OglDrv PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

target_link_libraries(OglDrv PRIVATE

    OpenGL::GL
    X11::X11
    X11::Xrandr
)

set_target_properties(OglDrv PROPERTIES

    OUTPUT_NAME OglDrv

    LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/Drivers"

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/Drivers"
)

if(GENESIS_ENABLE_PROFILING)

    target_compile_options(OglDrv PRIVATE -pg)
    target_link_options(OglDrv PRIVATE -pg)

endif()

if(GENESIS_ENABLE_COVERAGE)

    target_compile_options(OglDrv PRIVATE --coverage)
    target_link_options(OglDrv PRIVATE --coverage)

endif()

# Ensure the driver is built when building Genesis3D.
# This is a build dependency, NOT a linker dependency.

add_dependencies(Genesis3D OglDrv)

