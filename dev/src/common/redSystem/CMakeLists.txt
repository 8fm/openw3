project(redSystem)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "clock.cpp"
    "cpuid.cpp"
    "crashReportDataBufferWindows.cpp"
    "crc.cpp"
    "crtWindows.cpp"
    "error.cpp"
    "guid.cpp"
    "logCommonOutputDevice.cpp"
    "logFile.cpp"
    "nameHash.cpp"
    "ttyWriter.cpp"
    "unitTestMode.cpp"
    "windowsDebuggerWriter.cpp"
    "log.cpp"
)

if (WIN32)
    list(APPEND SRC_FILES "errorWindows.cpp")
    list(APPEND SRC_FILES "timerWindows.cpp")
else()
    list(APPEND SRC_FILES "errorLinux.cpp")
    list(APPEND SRC_FILES "timerLinux.cpp")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
