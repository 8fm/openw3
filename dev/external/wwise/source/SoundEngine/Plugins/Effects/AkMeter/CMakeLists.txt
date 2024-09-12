project(AkMeterFX)

set(SRC_FILES
    "AkMeterFX.cpp"
    "AkMeterFXParams.cpp"
    "AkMeterManager.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
    "../Common/Win32"
    "../Common"
    "../../Common"
    "../../../AkAudiolib/Win32"
    "../../../AkAudiolib/Common"
)

target_link_libraries(${PROJECT_NAME})
