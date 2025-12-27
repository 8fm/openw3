cmake_minimum_required(VERSION 3.18)
project(LavaEngine)

#set(CMAKE_VERBOSE_MAKEFILE ON)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

# GAME_CONFIG
if (NOT GAME_CONFIG)
    set(GAME_CONFIG "Debug" CACHE STRING "" FORCE)
endif()
message("GAME_CONFIG=${GAME_CONFIG}")

# GAME_PLATFORM: x64 or arm64
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(GAME_PLATFORM "arm64" CACHE STRING "" FORCE)
else()
    set(GAME_PLATFORM "x64" CACHE STRING "" FORCE)
endif()
message("GAME_PLATFORM=${GAME_PLATFORM}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin/${GAME_PLATFORM})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin/${GAME_PLATFORM})

if (NOT WIN32)
    option(USE_OLD_DXVK "Use old DXVK Native for native D3D11 API" OFF)
    if (USE_OLD_DXVK)
        message("Using DXVK Native for D3D11 API")
        set(DXVK_TYPE "native")
        set(DXVK_GIT_REPOSITORY "https://github.com/q4a/dxvk-native")
        set(DXVK_GIT_TAG "master")
        set(DXVK_EXTRA_PARAM "")
    else()
        message("Using upstream DXVK for D3D11 API")
        set(DXVK_TYPE "upstream")
        set(DXVK_GIT_REPOSITORY "https://github.com/doitsujin/dxvk")
        set(DXVK_GIT_TAG "v2.7.1")
        set(DXVK_EXTRA_PARAM "-Denable_d3d8=false")
    endif()
    include(ExternalProject)
    set(DXVK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/dxvk-src-${DXVK_TYPE}")
    set(DXVK_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/dxvk-bin-${DXVK_TYPE}")
    set(DIRECTX_INCLUDE_DIRS
        "${DXVK_SOURCE_DIR}/include/native/directx"
        "${DXVK_SOURCE_DIR}/include/native/windows"
    )
    set(DIRECTX_LIBS
        "${DXVK_BINARY_DIR}/src/d3d11/libdxvk_d3d11.so"
        "${DXVK_BINARY_DIR}/src/dxgi/libdxvk_dxgi.so"
    )
    ExternalProject_Add(dxvk
        GIT_REPOSITORY    ${DXVK_GIT_REPOSITORY}
        GIT_TAG           ${DXVK_GIT_TAG}
        GIT_SHALLOW       OFF
        BUILD_ALWAYS      OFF
        SOURCE_DIR        ${DXVK_SOURCE_DIR}
        BINARY_DIR        ${DXVK_BINARY_DIR}
        CONFIGURE_HANDLED_BY_BUILD ON
        CONFIGURE_COMMAND meson setup ../dxvk-src-${DXVK_TYPE} --buildtype=release ${DXVK_EXTRA_PARAM} -Denable_d3d9=false -Denable_d3d10=false -Denable_d3d11=true -Denable_dxgi=true
        BUILD_COMMAND     ninja
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  ${DIRECTX_LIBS}
    )

    include(ProcessorCount)
    ProcessorCount(N)
    message("ProcessorCount = ${N}")
    set(VKD3D_CONFIG "./configure")
    list(APPEND VKD3D_CONFIG "CXX=clang++")
    list(APPEND VKD3D_CONFIG  "CC=clang")
    if (${GAME_CONFIG} STREQUAL "Debug")
        list(APPEND VKD3D_CONFIG "CPPFLAGS=-O0")
        list(APPEND VKD3D_CONFIG   "CFLAGS=-O0")
        set(VKD3D_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/vkd3d-debug")
    else()
        list(APPEND VKD3D_CONFIG "CPPFLAGS=-O3 -DNDEBUG -DVKD3D_NO_TRACE_MESSAGES")
        list(APPEND VKD3D_CONFIG   "CFLAGS=-O3 -DNDEBUG -DVKD3D_NO_TRACE_MESSAGES")
        set(VKD3D_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/vkd3d-release")
    endif()
    set(DIRECTX_INCLUDE_DIRS ${DIRECTX_INCLUDE_DIRS}
        "${VKD3D_SOURCE_DIR}/include"
    )
    set(VKD3D_LIBS
        "${VKD3D_SOURCE_DIR}/.libs/libvkd3d-utils.a"
        "${VKD3D_SOURCE_DIR}/.libs/libvkd3d-shader.a"
    )
    ExternalProject_Add(vkd3d
        GIT_REPOSITORY    https://github.com/q4a/vkd3d
        GIT_TAG           dxvk
        GIT_SHALLOW       ON
        BUILD_ALWAYS      OFF
        SOURCE_DIR        ${VKD3D_SOURCE_DIR}
        BUILD_IN_SOURCE   ON
        CONFIGURE_HANDLED_BY_BUILD ON
        # result comman will be: ./configure "CPPFLAGS=-O3 -DNDEBUG -DVKD3D_NO_TRACE_MESSAGES" "CFLAGS=-O3 -DNDEBUG -DVKD3D_NO_TRACE_MESSAGES"
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ${VKD3D_CONFIG}
        BUILD_COMMAND     make -j${N}
        BUILD_BYPRODUCTS  ${VKD3D_LIBS}
        INSTALL_COMMAND   ""
    )

    find_package(SDL2 CONFIG REQUIRED)
    find_package(directxtex CONFIG REQUIRED)
    find_package(directxmath CONFIG REQUIRED)
    set(DIRECTX_MATH_LIBS Microsoft::DirectXMath)

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
