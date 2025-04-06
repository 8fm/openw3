cmake_minimum_required(VERSION 3.18)
project(LavaEngine)

#set(CMAKE_VERBOSE_MAKEFILE ON)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

# GAME_CONFIG
if (NOT GAME_CONFIG)
    set(GAME_CONFIG "Final" CACHE STRING "" FORCE)
endif()
message("GAME_CONFIG=${GAME_CONFIG}")

# GAME_PLATFORM: x64 or arm64
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(GAME_PLATFORM "arm64" CACHE STRING "" FORCE)
else()
    set(GAME_PLATFORM "x64" CACHE STRING "" FORCE)
endif()
message("GAME_PLATFORM=${GAME_PLATFORM}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/${GAME_CONFIG}-${GAME_PLATFORM})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/${GAME_CONFIG}-${GAME_PLATFORM})

if (NOT WIN32)
    message("Using DXVK 2.4 for D3D11 API")
    include(ExternalProject)
    set(DXVK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/dxvk-src")
    set(DXVK_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/dxvk-bin")
    set(DIRECTX_INCLUDE_DIRS
        "${DXVK_SOURCE_DIR}/include/native/directx"
        "${DXVK_SOURCE_DIR}/include/native/windows"
    )
    set(DIRECTX_LIBS
        "${DXVK_BINARY_DIR}/src/d3d11/libdxvk_d3d11.so"
        "${DXVK_BINARY_DIR}/src/dxgi/libdxvk_dxgi.so"
    )
    ExternalProject_Add(dxvk
        GIT_REPOSITORY    https://github.com/doitsujin/dxvk
        GIT_TAG           ad253b8a7e20b7cf16fce7d1c505928a434eac29 # 2.6
        GIT_SHALLOW       OFF
        BUILD_ALWAYS      OFF
        SOURCE_DIR        ${DXVK_SOURCE_DIR}
        BINARY_DIR        ${DXVK_BINARY_DIR}
        CONFIGURE_HANDLED_BY_BUILD ON
        CONFIGURE_COMMAND meson setup ../dxvk-src --buildtype=release -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false -Denable_d3d11=true -Denable_dxgi=true
        BUILD_COMMAND     ninja
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  ${DIRECTX_LIBS}
    )

    message("Getting DirectXMath 3.1.9")
    include(FetchContent)
    FetchContent_Declare(dxmath
      GIT_REPOSITORY https://github.com/microsoft/DirectXMath
      GIT_TAG        d837578297c6c93849573858182350ede04987dc # 3.1.9
      GIT_SHALLOW    OFF
    )
    FetchContent_MakeAvailable(dxmath)
    FetchContent_GetProperties(dxmath)
    if(NOT EXISTS "${dxmath_SOURCE_DIR}/Inc/sal.h")
        file(DOWNLOAD https://raw.githubusercontent.com/dotnet/runtime/v8.0.8/src/coreclr/pal/inc/rt/sal.h "${dxmath_SOURCE_DIR}/Inc/sal.h")
    endif()
    list(APPEND DIRECTX_INCLUDE_DIRS
        "${dxmath_SOURCE_DIR}/Inc"
    )
    message("DIRECTX_INCLUDE_DIRS=${DIRECTX_INCLUDE_DIRS}")

    ### dev/external ###
    # Apex
    set(APEX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dev/external/Apex")
    set(PHYSX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dev/external/PhysX3")
    add_subdirectory(dev/external/Apex)
    # gfx4 - expat
    option(BUILD_tools "" OFF)
    option(BUILD_examples "" OFF)
    option(BUILD_tests "" OFF)
    option(BUILD_shared "" OFF)
    add_subdirectory(dev/external/gfx4/3rdParty/expat-2.1.0)
    # gfx4 - all other
    add_subdirectory(dev/external/gfx4/Projects/cmake)
    # PhysX3
    add_subdirectory(dev/external/PhysX3)
    # speedtree
    add_subdirectory(dev/external/speedtree/Source)
    # wwise
    add_subdirectory(dev/external/wwise)
endif()

add_subdirectory(dev/src/common/core)
add_subdirectory(dev/src/common/engine)
add_subdirectory(dev/src/common/game)
add_subdirectory(dev/src/common/gpuApiDX10)
if (WIN32)
add_subdirectory(dev/src/common/gpuApiOpenGL)
endif()
add_subdirectory(dev/src/common/gpuApiUtils)
add_subdirectory(dev/src/common/matcompiler)
add_subdirectory(dev/src/common/nullrender)
add_subdirectory(dev/src/common/physics)
add_subdirectory(dev/src/common/platformCommon)
#add_subdirectory(dev/src/common/redContainers) # only headers
add_subdirectory(dev/src/common/redIO)
add_subdirectory(dev/src/common/redMath)
add_subdirectory(dev/src/common/redMemory)
add_subdirectory(dev/src/common/redMemoryFramework)
add_subdirectory(dev/src/common/redNetwork)
add_subdirectory(dev/src/common/redSystem)
add_subdirectory(dev/src/common/redThreads)
add_subdirectory(dev/src/common/renderer)
add_subdirectory(dev/src/common/scripts)
add_subdirectory(dev/src/games/r4)
if (WIN32)
add_subdirectory(dev/src/win32/platform)
else()
add_subdirectory(dev/src/linux/platform)
endif()

add_subdirectory(dev/src/games/r4Launcher)
