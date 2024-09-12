project(AkMemoryMgr)

set(SRC_FILES
    "../3rdParty/Common/tlsf/tlsf.cpp"
    "Common/AkMemoryMgrBase.cpp"
)
if (WIN32)
    set(STDAFX_H "Win32/stdafx.h")
    set(SYSTEM_INC "Win32" "../AkAudiolib/Win32")
    list(APPEND SRC_FILES
        "Win32/stdafx.cpp"
        "Win32/AkMemoryMgr.cpp"
    )
else()
    set(STDAFX_H "Linux/stdafx.h")
    set(SYSTEM_INC "Linux" "../AkAudiolib/Linux")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SYSTEM_INC}
    "Common"
    "../3rdParty/Common"
    "../../../include"
    "../AkAudiolib/Common"
)

target_link_libraries(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    ${STDAFX_H}
)
