project(platformCommon)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "build.cpp"
    "platform.cpp"
    "secondScreenDevicePC.cpp"
    "secondScreenDeviceWatcherPC.cpp"
    "secondScreenManager.cpp"
)

if (WIN32)
    list(APPEND SRC_FILES "clipboardWin32.cpp")
    list(APPEND SRC_FILES "platformWindows.cpp")
    list(APPEND SRC_FILES "win32Viewport.cpp")
else()
    list(APPEND SRC_FILES "platformLinux.cpp")
endif()

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
