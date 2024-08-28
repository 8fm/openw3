project(redNetwork)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "address.cpp"
    "channel.cpp"
    "manager.cpp"
    "memory.cpp"
    "packet.cpp"
    "ping.cpp"
    "rawManager.cpp"
    "simpleDynArray.cpp"
    "socket.cpp"
)

if (WIN32)
    list(APPEND SRC_FILES "platform/platformWindows.cpp")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
