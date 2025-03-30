project(PhysXVisualDebuggerSDK)

add_definitions(-DPX_PHYSX_STATIC_LIB)

set(SRC_FILES
    "${PHYSX_DIR}/Source/PhysXVisualDebuggerSDK/PvdCommStreamBufferedEventSink.cpp"
    "${PHYSX_DIR}/Source/PhysXVisualDebuggerSDK/PvdConnection.cpp"
    "${PHYSX_DIR}/Source/PhysXVisualDebuggerSDK/PvdConnectionManager.cpp"
    "${PHYSX_DIR}/Source/PhysXVisualDebuggerSDK/PvdDataStream.cpp"
    "${PHYSX_DIR}/Source/PhysXVisualDebuggerSDK/PvdNetworkStreams.cpp"
    "${PHYSX_DIR}/Source/PhysXVisualDebuggerSDK/PvdImmediateRenderer.cpp"
    "${PHYSX_DIR}/Source/PhysXVisualDebuggerSDK/PvdObjectModelMetaData.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PHYSX_DIR}/Source/PhysXProfileSDK"
    "${PHYSX_DIR}/Include/foundation"
    "${PHYSX_DIR}/Source/foundation/include"
    "${PHYSX_DIR}/Include/physxprofilesdk"
    "${PHYSX_DIR}/Include/physxvisualdebuggersdk"
    "${PHYSX_DIR}/Include"
)

target_link_libraries(${PROJECT_NAME})
