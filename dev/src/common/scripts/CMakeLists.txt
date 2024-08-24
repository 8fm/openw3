project(scripts)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "Lexer/definition.cpp"
    "Lexer/lexer.cpp"
    "Memory/allocatorProxy.cpp"
    "Memory/simpleDynArray.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
