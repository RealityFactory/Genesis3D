cmake_minimum_required(VERSION 3.20)

project(Genesis3D
    VERSION 1.1
    LANGUAGES C CXX
)

include(GNUInstallDirs)

# ==================================================
# Build options
# ==================================================

option(GENESIS_ENABLE_PROFILING
    "Enable gprof profiling"
    OFF
)

option(GENESIS_ENABLE_COVERAGE
    "Enable code coverage instrumentation"
    OFF
)

# ==================================================
# Compiler configuration
# ==================================================

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT MSVC)
    add_compile_options(
        $<$<CONFIG:Debug>:-g3>
    )
endif()

# ==================================================
# Dependencies (Linux)
# ==================================================

if(UNIX AND NOT APPLE)

    find_package(X11 REQUIRED COMPONENTS Xext Xrandr)
    find_package(OpenGL REQUIRED)
    find_package(Freetype REQUIRED)

endif()

# ==================================================
# Genesis3D engine sources
# ==================================================

set(GENESIS3D_SOURCES

    # Root
    Camera.c
    DrawBBox.c
    Ge.c
    List.c
    Sound.c
    Sound3d.c
    Sprite.c
    TClip.c
    Timer.c
    TSC.c

    # Actor
    Actor/Actor.c
    Actor/Body.c
    Actor/BodyInst.c
    Actor/Motion.c
    Actor/Path.c
    Actor/Pose.c
    Actor/Puppet.c
    Actor/QKFrame.c
    Actor/StrBlock.c
    Actor/TKArray.c
    Actor/TKEvents.c
    Actor/VKFrame.c
    Actor/XFArray.c

    # Bitmap
    Bitmap/Bitmap.c
    Bitmap/Bitmap_BlitData.c
    Bitmap/Bitmap_Gamma.c
    Bitmap/PixelFormat.c
    Bitmap/Compression/PalCreate.c
    Bitmap/Compression/Palettize.c
    Bitmap/Compression/PalOptimize.c
    Bitmap/Compression/Yuv.c

    # Engine
    Engine/BitmapList.c
    Engine/Engine.c
    Engine/FontBmp.c
    Engine/Genesis3d.c
    Engine/System.c

    # Engine Logo
    Engine/Logo/A_Corona.c
    Engine/Logo/A_Streak.c
    Engine/Logo/Corona.c
    Engine/Logo/Electric.c
    Engine/Logo/Logo.c
    Engine/Logo/LogoActor.c
    Engine/Logo/Poweredby.c
    Engine/Logo/Streak.c
    Engine/Logo/WebUrl.c

    # Engine drivers
    Engine/Drivers/Bmp.c

    # Entities
    Entities/Entities.c

    # Font
    Font/Font.c
    Font/wgClip.c

    # Math
    Math/Box.c
    Math/CRC32.c
    Math/ExtBox.c
    Math/Quatern.c
    Math/Vec3d.c
    Math/XForm3d.c

    # Physics
    Physics/Matrix33.c
    Physics/PhysicsJoint.c
    Physics/PhysicsObject.c
    Physics/PhysicsSystem.c

    # Support
    Support/ErrorLog.c
    Support/geAssert.c
    Support/Log.c
    Support/MemPool.c
    Support/Ram.c

    # VFile
    VFile/DirTree.c
    VFile/DirTreeCommon.c
    VFile/FSDos.c
    VFile/FSMemory.c
    VFile/FSVfs.c
    VFile/VFile.c

    # World
    World/Fog.c
    World/Frustum.c
    World/GBSPFile.c
    World/Light.c
    World/Plane.c
    World/Surface.c
    World/Trace.c
    World/User.c
    World/Vis.c
    World/WBitmap.c
    World/World.c
)

# ==================================================
# Genesis3D shared library
# ==================================================

if(UNIX AND NOT APPLE)

    add_library(Genesis3D SHARED
        ${GENESIS3D_SOURCES}
    )

    target_include_directories(Genesis3D PRIVATE

        ${CMAKE_CURRENT_SOURCE_DIR}

        Actor
        Bitmap
        Bitmap/Compression
        Engine
        Engine/Drivers
        Engine/Logo
        Entities
        Font
        Math
        Physics
        Support
        VFile
        World
    )

    target_compile_definitions(Genesis3D PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
    )

    target_link_libraries(Genesis3D PRIVATE

        X11::X11
        X11::Xext

        Freetype::Freetype

        ${CMAKE_DL_LIBS}
        m
    )

    set_target_properties(Genesis3D PROPERTIES
        OUTPUT_NAME Genesis3D
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

endif()

# ==================================================
# Graphics drivers
# ==================================================

add_subdirectory(Engine/Drivers)

# ==================================================
# Optional profiling / coverage
# ==================================================

if(GENESIS_ENABLE_PROFILING AND NOT MSVC)

    target_compile_options(Genesis3D PRIVATE -pg)
    target_link_options(Genesis3D PRIVATE -pg)

endif()

if(GENESIS_ENABLE_COVERAGE AND NOT MSVC)

    target_compile_options(Genesis3D PRIVATE --coverage)
    target_link_options(Genesis3D PRIVATE --coverage)

endif()

