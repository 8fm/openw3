project(AkDelayFX)

set(SRC_FILES
    "AkDelayFX.cpp"
    "AkDelayFXDSP.cpp"
    "AkDelayFXParams.cpp"
)
add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
)

target_link_libraries(${PROJECT_NAME})
