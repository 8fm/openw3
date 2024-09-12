project(AkGuitarDistortionFX)

set(SRC_FILES
    "AkGuitarDistortionFX.cpp"
    "AkGuitarDistortionFXParams.cpp"
    "../../Common/AkDistortion.cpp"
    "../../Common/AkMultiBandEQ.cpp"
    "../../Common/AkRectifier.cpp"
    "../Common/Mix2Interp.cpp"
    "../Common/OnePoleZeroHPFilter.cpp"
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
