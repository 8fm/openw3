project(AkSineSource)

set(SRC_FILES
    "AkFXSrcSine.cpp"
    "AkFXSrcSineParams.cpp"
    "../../../../DSP/AkSineOsc.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
    "../../../../DSP"
)

target_link_libraries(${PROJECT_NAME})
