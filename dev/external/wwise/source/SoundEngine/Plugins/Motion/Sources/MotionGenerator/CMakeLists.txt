project(AkMotionGenerator)

set(SRC_FILES
    "AkMotionGenerator.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../../../../../include"
)

target_link_libraries(${PROJECT_NAME})
