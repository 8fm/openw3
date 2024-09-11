project(SpeedTreeForest)

set(SRC_FILES

    "../Source/Culling.cpp"
    "../Source/Forest.cpp"
    "../Source/Terrain.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "../../../Include"
)

target_link_libraries(${PROJECT_NAME})
