project(AkToneSource)

set(SRC_FILES
    "AkLpFilter.cpp"
    "AkToneGen.cpp"
    "AkToneGenDSP.cpp"
    "AkToneGenParams.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
)

target_link_libraries(${PROJECT_NAME})
