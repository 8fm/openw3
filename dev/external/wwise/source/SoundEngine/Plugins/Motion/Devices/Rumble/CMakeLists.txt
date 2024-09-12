project(AkRumble)

set(SRC_FILES
    "stdafx.cpp"
    "RumbleBusBase.cpp"
    "Factory.cpp"
    "RumbleBusWindowsXBox.cpp"
    "AkRumbleControllerDirectInput.cpp"
    "AkRumbleControllerXInput.cpp"
    "RumbleDeviceHelper.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "include"
    "."
    "../../../../../../include"
    "../../../../AkAudiolib/Win32"
    "../../../../AkAudiolib/Common"
)

target_link_libraries(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    stdafx.cpp
)
