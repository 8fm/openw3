project(AkParametricEQFX)

set(SRC_FILES
    "Win32/AkParametricEQFX.cpp"
    "AkParametricEQFXParams.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../include"
    "../Common/Win32"
    "../Common"
    "../../Common"
)

target_link_libraries(${PROJECT_NAME})
