project(AkMusicEngine)

add_definitions(-DPROXYCENTRAL_CONNECTED)
if (MSVC)
    add_compile_options(/wd4250 /wd4355)
endif()

set(SRC_FILES
    "Common/AkChildCtx.cpp"
    "Common/AkContextualMusicSequencer.cpp"
    "Common/AkMatrixAwareCtx.cpp"
    "Common/AkMatrixSequencer.cpp"
    "Common/AkMusicBank.cpp"
    "Common/AkMusicCtx.cpp"
    "Common/AkMusicNode.cpp"
    "Common/AkMusicPBI.cpp"
    "Common/AkMusicRanSeqCntr.cpp"
    "Common/AkMusicRenderer.cpp"
    "Common/AkMusicSegment.cpp"
    "Common/AkMusicSwitchCntr.cpp"
    "Common/AkMusicSwitchCtx.cpp"
    "Common/AkMusicTrack.cpp"
    "Common/AkMusicTransAware.cpp"
    "Common/AkRSIterator.cpp"
    "Common/AkScheduleWindow.cpp"
    "Common/AkSegmentChain.cpp"
    "Common/AkSegmentCtx.cpp"
    "Common/AkSegmentInfoRepository.cpp"
    "Common/AkSequencableSegmentCtx.cpp"
    "Common/AkSequenceCtx.cpp"
)
if (NOT ${GAME_CONFIG} STREQUAL "Final")
    list(APPEND SRC_FILES
        "../SoundEngineProxy/Common/TrackProxyLocal.cpp"
        "../SoundEngineProxy/Remote/CodeBase/TrackProxyConnected.cpp"
        "../SoundEngineProxy/Common/SegmentProxyLocal.cpp"
        "../SoundEngineProxy/Remote/CodeBase/SegmentProxyConnected.cpp"
        "../SoundEngineProxy/Common/MusicNodeProxyLocal.cpp"
        "../SoundEngineProxy/Common/MusicRanSeqProxyLocal.cpp"
        "../SoundEngineProxy/Common/MusicSwitchProxyLocal.cpp"
        "../SoundEngineProxy/Common/MusicTransAwareProxyLocal.cpp"
        "../SoundEngineProxy/Remote/CodeBase/MusicNodeProxyConnected.cpp"
        "../SoundEngineProxy/Remote/CodeBase/MusicRanSeqProxyConnected.cpp"
        "../SoundEngineProxy/Remote/CodeBase/MusicSwitchProxyConnected.cpp"
        "../SoundEngineProxy/Remote/CodeBase/MusicTransAwareProxyConnected.cpp"
        "../SoundEngineProxy/Remote/CodeBase/ProxyMusic.cpp"
    )
endif()
if (WIN32)
    set(STDAFX_H "../AkAudiolib/Win32/stdafx.h")
    set(SYSTEM_INC "../AkAudiolib/Win32")
    list(APPEND SRC_FILES
        "../AkAudiolib/Win32/stdafx.cpp"
    )
else()
    set(STDAFX_H "../AkAudiolib/Linux/stdafx.h")
    set(SYSTEM_INC "../AkAudiolib/Linux")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SYSTEM_INC}
    "Common"
    "../AkAudiolib/Common"
    "../../../include"
    "../SoundEngineProxy/Remote/CodeBase"
    "../SoundEngineProxy/Common"
    "../Communication/Remote/CodeBase"
)

target_link_libraries(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    ${STDAFX_H}
)
