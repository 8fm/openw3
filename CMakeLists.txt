cmake_minimum_required(VERSION 3.18)
project(LavaEngine)

#set(CMAKE_VERBOSE_MAKEFILE ON)
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("GAME_CONFIG=${GAME_CONFIG}")
message("GAME_PLATFORM=${GAME_PLATFORM}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/${GAME_CONFIG}-${GAME_PLATFORM})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/${GAME_CONFIG}-${GAME_PLATFORM})

if (NOT WIN32)
    message("Using DXVK 2.4 for D3D11 API")
    include(ExternalProject)
    ExternalProject_Add(dxvk
        GIT_REPOSITORY    https://github.com/doitsujin/dxvk
        GIT_TAG           008afc1c5f287e82a553108790da6e10b1159200 # 2.4
        GIT_SHALLOW       OFF
        BUILD_ALWAYS      OFF
        CONFIGURE_HANDLED_BY_BUILD ON
        CONFIGURE_COMMAND meson setup ../dxvk --buildtype=release -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false -Denable_d3d11=true -Denable_dxgi=true
        BUILD_COMMAND     ninja
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  <BINARY_DIR>/src/d3d11/libdxvk_d3d11.so
    )
    ExternalProject_Get_property(dxvk SOURCE_DIR BINARY_DIR)

    set(DIRECTX_INCLUDE_DIRS
        "${SOURCE_DIR}/include/native/directx"
        "${SOURCE_DIR}/include/native/windows"
    )
    set(DIRECTX_LIBS ${BINARY_DIR}/src/d3d11/libdxvk_d3d11.so)

    message("Getting DirectXMath 3.1.9")
    include(FetchContent)
    FetchContent_Declare(dxmath
      googletest
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
