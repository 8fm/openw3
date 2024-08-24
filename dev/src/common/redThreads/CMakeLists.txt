project(redThreads)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "redThreadsThread.cpp"
    "redThreadsThreadOrbisAPI.cpp"
    "redThreadsThreadWinAPI.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
