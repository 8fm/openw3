project(SpeedTreeCore)

set(SRC_FILES
    "../Source/AllocatorStats.cpp"
    "../Source/CoordSys.cpp"
    "../Source/Core.cpp"
    "../Source/ErrorHandler.cpp"
    "../Source/FileSystem.cpp"
    "../Source/Memory.cpp"
    "../Source/Parser.cpp"
    "../Source/ScopeTrace.cpp"
    "../Source/Wind.cpp"
    "../Source/Fixed.cpp"
    "../Source/Evaluation/Key.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "../../../Include"
)

target_link_libraries(${PROJECT_NAME})
