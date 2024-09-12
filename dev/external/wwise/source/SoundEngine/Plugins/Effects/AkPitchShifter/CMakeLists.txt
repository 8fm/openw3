project(AkPitchShifterFX)

set(SRC_FILES
    "AkPitchShifterDSPProcess.cpp"
    "AkPitchShifterFX.cpp"
    "AkPitchShifterFXParams.cpp"
    "../../Common/AkDelayPitchShift.cpp"
    "../Common/DelayLineLight.cpp"
    "../Common/Mix2Interp.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
    "../Common/Win32"
    "../Common"
    "../../Common"
    "../../../AkAudiolib/Common/ak_fft"
    "../../../AkAudiolib/Common/ak_fft/tools"
)

target_link_libraries(${PROJECT_NAME})
