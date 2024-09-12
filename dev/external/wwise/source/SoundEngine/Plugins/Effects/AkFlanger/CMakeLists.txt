project(AkFlangerFX)

set(SRC_FILES
    "AkFlangerFX.cpp"
    "AkFlangerFXParams.cpp"
    "../Common/LFO.cpp"
    "../Common/Mix2Interp.cpp"
    "../Common/OnePoleFilter.cpp"
    "../Common/UniComb.cpp"
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
