project(PxTask)

add_definitions(-DPX_PHYSX_STATIC_LIB)

set(SRC_FILES
    "${PHYSX_DIR}/Source/pxtask/src/CudaWrapper.cpp"
    "${PHYSX_DIR}/Source/pxtask/src/TaskManager.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PHYSX_DIR}/Include/foundation"
    "${PHYSX_DIR}/Source/foundation/include"
    "${PHYSX_DIR}/Include/physxprofilesdk"
    "${PHYSX_DIR}/Include/physxvisualdebuggersdk"
    "${PHYSX_DIR}/Include"
    "${PHYSX_DIR}/Include/pxtask"
)

target_link_libraries(${PROJECT_NAME})
