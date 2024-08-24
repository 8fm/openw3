if (${GAME_CONFIG} STREQUAL "Final")
    set(BIN_NAME "witcher3")
elseif (${GAME_CONFIG} STREQUAL "ReleaseGame")
    set(BIN_NAME "witcher3Release")
else()
    set(BIN_NAME "r4Launcher")
endif()
project(${BIN_NAME})

set(TARGET_TYPE "Application")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "activateState.cpp"
    "build.cpp"
    "externDefinitions.cpp"
    "gameConstrainedState.cpp"
    "gameRunningState.cpp"
    "initializationState.cpp"
    "mainRed.cpp"
    "mainWindows.cpp"
    "operatorNewDelete_orbis.cpp"
    "shutdownState.cpp"
    "utility.cpp"
    "utilityWindows.cpp"
    "r4Launcher.rc"
)

add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/dev/external/gfx4/Src
    ${CMAKE_SOURCE_DIR}/dev/external/gfx4/Include
)

target_link_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/dev"
)

if (WIN32)
    set(SYSTEM_DEPS "legacy_stdio_definitions")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    core
    engine
    game
    gpuApiDX10
    gpuApiUtils
    matcompiler
    physics
    platformCommon
    redIO
    redMath
    redMemory
    redMemoryFramework
    redNetwork
    redSystem
    redThreads
    renderer
    scripts
    r4
    platform
    ${SYSTEM_DEPS}
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    build.h
)
