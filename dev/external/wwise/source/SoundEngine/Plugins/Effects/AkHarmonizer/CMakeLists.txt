project(AkHarmonizerFX)

add_definitions(-DUSEALLBUTTERFLIES)
set(SRC_FILES
    "AkHarmonizerDSPProcess.cpp"
    "AkHarmonizerFX.cpp"
    "AkHarmonizerFXParams.cpp"
    "../../Common/AkPhaseVocoder.cpp"
    "../../Common/AkResamplingPhaseVocoder.cpp"
    "../../Common/CircularBuffer.cpp"
    "../../Common/ResamplingCircularBuffer.cpp"
    "../../Common/FreqWindow.cpp"
    "../../Common/LinearResampler.cpp"
    "../../Common/OLACircularBuffer.cpp"
    "../../Common/TimeWindow.cpp"
    "../Common/DelayLineLight.cpp"
    "../Common/Mix2Interp.cpp"
    "../../../AkAudiolib/Common/ak_fft/ak_fft.cpp"
    "../../../AkAudiolib/Common/ak_fft/tools/ak_fftr.cpp"
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
