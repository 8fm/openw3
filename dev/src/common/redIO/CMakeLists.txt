project(redIO)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "redIO.cpp"
    "redIOAsyncFileHandleCache.cpp"
    "redIOAsyncIO.cpp"
    "redIOFile.cpp"
    "redIOFiosFileOrbisAPI.cpp"
    "redIOProactorGenericAPI.cpp"
    "redIOProactorOrbisAPI.cpp"
    "redIOSystemFileLinuxAPI.cpp"
    "redIOSystemFileWinAPI.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
