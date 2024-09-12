project(AkTimeStretchFX)

add_definitions(-DUSEALLBUTTERFLIES)
set(SRC_FILES
    "AkTimeStretchFX.cpp"
    "AkTimeStretchFXParams.cpp"
    "../../Common/AkPhaseVocoder.cpp"
    "../../Common/CircularBuffer.cpp"
    "../../Common/FreqWindow.cpp"
    "../../Common/LinearResampler.cpp"
    "../../Common/OLACircularBuffer.cpp"
    "../../Common/TimeWindow.cpp"
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
