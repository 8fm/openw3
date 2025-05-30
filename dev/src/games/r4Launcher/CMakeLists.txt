if (${GAME_CONFIG} STREQUAL "Final")
    set(BIN_NAME "witcher3")
elseif (${GAME_CONFIG} STREQUAL "ReleaseGame")
    set(BIN_NAME "witcher3Release")
elseif (${GAME_CONFIG} STREQUAL "Release")
    set(BIN_NAME "r4Launcher")
else()
    set(BIN_NAME "r4Launcher.${GAME_CONFIG}")
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
    "operatorNewDelete_orbis.cpp"
    "shutdownState.cpp"
    "utility.cpp"
    "r4Launcher.rc"
)

if (WIN32)
    list(APPEND SRC_FILES "mainRed.cpp")
    list(APPEND SRC_FILES "mainWindows.cpp")
    list(APPEND SRC_FILES "utilityWindows.cpp")
else()
    list(APPEND SRC_FILES "gameApplicationLinux.cpp")
    list(APPEND SRC_FILES "mainLinux.cpp")
    list(APPEND SRC_FILES "utilityLinux.cpp")
endif()

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
else()
    set(SYSTEM_DEPS
        Microsoft::DirectXTex
        libgfx_air
        libgfx_as2
        libgfx_as3
        libgfxexpat
        libgfx
    )
    set(WWISE_DEPS
        AkTremoloFX
        AkMemoryMgr
        AkMotionGenerator
        AkSineSource
        AkStereoDelayFX
        AkGuitarDistortionFX
        AkSoundEngine
        AkRoomVerbFX
        AkParametricEQFX
        AkToneSource
        AkFlangerFX
        AkCompressorFX
        AkAudioInputSource
        AkMusicEngine
        AkDelayFX
        AkPitchShifterFX
        AkSilenceSource
        AkGainFX
        AkVorbisDecoder
        AkTimeStretchFX
        AkMatrixReverbFX
        AkMeterFX
        AkStreamMgr
        AkHarmonizerFX
        AkPeakLimiterFX
        AkExpanderFX
    )
    if (NOT ${GAME_CONFIG} STREQUAL "Final") # CommunicationCentral
        list(APPEND WWISE_DEPS CommunicationCentral)
    endif()
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
    "$<LINK_GROUP:RESCAN,${WWISE_DEPS}>"
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    build.h
)
