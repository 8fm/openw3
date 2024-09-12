project(AkTremoloFX)

set(SRC_FILES
    "AkTremoloFX.cpp"
    "AkTremoloFXParams.cpp"
    "../Common/LFO.cpp"
    "../Common/OnePoleFilter.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
    "../Common/Win32"
    "../Common"
    "../../Common"
    "../../../AkAudiolib/Common"
)

target_link_libraries(${PROJECT_NAME})
