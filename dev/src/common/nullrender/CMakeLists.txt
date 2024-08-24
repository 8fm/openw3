project(nullrender)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "nullCommands.cpp"
    "nullFragments.cpp"
    "nullScaleformCommands.cpp"
    "nullRender.cpp"
    "build.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/dev/external/gfx4/Src
    ${CMAKE_SOURCE_DIR}/dev/external/gfx4/Include
)

target_link_libraries(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    build.h
)
