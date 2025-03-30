project(SceneQuery)

add_definitions(-DPX_PHYSX_STATIC_LIB)

set(SRC_FILES
    "${PHYSX_DIR}/Source/SceneQuery/SqAABBPruner.cpp"
    "${PHYSX_DIR}/Source/SceneQuery/SqAABBTreeUpdateMap.cpp"
    "${PHYSX_DIR}/Source/SceneQuery/SqBucketPruner.cpp"
    "${PHYSX_DIR}/Source/SceneQuery/SqPruningPool.cpp"
    "${PHYSX_DIR}/Source/SceneQuery/SqSceneQueryManager.cpp"
    "${PHYSX_DIR}/Source/SceneQuery/SqUtilities.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PHYSX_DIR}/Include/foundation"
    "${PHYSX_DIR}/Source/foundation/include"
    "${PHYSX_DIR}/Include/physxprofilesdk"
    "${PHYSX_DIR}/Include/physxvisualdebuggersdk"
    "${PHYSX_DIR}/Include/common"
    "${PHYSX_DIR}/Include/geometry"
    "${PHYSX_DIR}/Include/pvd"
    "${PHYSX_DIR}/Include/pxtask"
    "${PHYSX_DIR}/Include"
    "${PHYSX_DIR}/Source/Common/src"
    "${PHYSX_DIR}/Source/Common/src/windows"
    "${PHYSX_DIR}/Source/GeomUtils/headers"
    "${PHYSX_DIR}/Source/GeomUtils/src"
    "${PHYSX_DIR}/Source/GeomUtils/Opcode"
    "${PHYSX_DIR}/Source/SceneQuery"
    "${PHYSX_DIR}/Source/SimulationController/include"
    "${PHYSX_DIR}/Source/LowLevel/API/include"
    "${PHYSX_DIR}/Source/LowLevel/ps3/include"
    "${PHYSX_DIR}/Source/PhysX/src"
    "${PHYSX_DIR}/Source/PhysX/src/buffering"
    "${PHYSX_DIR}/Source/PvdRuntime/src"
)

target_link_libraries(${PROJECT_NAME})
