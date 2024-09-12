project(AkStreamMgr)

set(SRC_FILES
    "Common/AkDeviceBase.cpp"
    "Common/AkDeviceBlocking.cpp"
    "Common/AkDeviceDeferredLinedUp.cpp"
    "Common/AkIOMemMgr.cpp"
    "Common/AkStreamMgr.cpp"
    "Common/AkTransferDeferred.cpp"
)
if (WIN32)
    set(STDAFX_H "Win32/stdafx.h")
    set(SYSTEM_INC "Win32")
    list(APPEND SRC_FILES
        "Win32/stdafx.cpp"
        "Win32/AkIOThread.cpp"
    )
else()
    set(STDAFX_H "Linux/stdafx.h")
    set(SYSTEM_INC "Linux")
    list(APPEND SRC_FILES
        "Linux/stdafx.cpp"
    )
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SYSTEM_INC}
    "include"
    "Common"
)

target_link_libraries(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    ${STDAFX_H}
)
