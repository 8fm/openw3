project(platform)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "build.cpp"
    "clipboardWin32.cpp"
    "inputDeviceGamepadPS4ForPC.cpp"
    "inputDeviceGamepadXInput.cpp"
    "inputDeviceKeyboardDInput.cpp"
    "inputDeviceKeyboardRawInput.cpp"
    "inputDeviceManagerWin32.cpp"
    "inputDeviceMouseDInput.cpp"
    "inputDeviceTabletWintab.cpp"
    "inputLibWintab.cpp"
    "os.cpp"
    "userProfileManagerWindows.cpp"
    "userProfileManagerWindowsSaving.cpp"
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
