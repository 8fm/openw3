project(AkRoomVerbFX)

set(SRC_FILES
    "AkRoomVerbFX.cpp"
    "AkRoomVerbFXParams.cpp"
    "ERUnitDual.cpp"
    "FDN4.cpp"
    "../Common/AllpassFilter.cpp"
    "../Common/ConstantPowerChannelMixdown.cpp"
    "../Common/DelayLine.cpp"
    "../Common/Mix3Interp.cpp"
    "../Common/OnePoleFilter.cpp"
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
