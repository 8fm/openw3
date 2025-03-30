project(PvdRuntime)

add_definitions(-DPX_PHYSX_CORE_EXPORTS -DPX_PHYSX_STATIC_LIB)

set(SRC_FILES
    "${PHYSX_DIR}/Source/PvdRuntime/src/PvdSceneVisualDebugger.cpp"
    "${PHYSX_DIR}/Source/PvdRuntime/src/PvdVisualDebugger.cpp"
    "${PHYSX_DIR}/Source/PvdRuntime/src/PxMetaDataPvdBinding.cpp"
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
    "${PHYSX_DIR}/Include/particles"
    "${PHYSX_DIR}/Include/cloth"
    "${PHYSX_DIR}/Include"
    "${PHYSX_DIR}/Source/Common/src"
    "${PHYSX_DIR}/Source/GeomUtils/headers"
    "${PHYSX_DIR}/Source/GeomUtils/src"
    "${PHYSX_DIR}/Source/GeomUtils/Opcode"
    "${PHYSX_DIR}/Source/PhysX/src"
    "${PHYSX_DIR}/Source/PhysX/src/cloth"
    "${PHYSX_DIR}/Source/PhysX/src/buffering"
    "${PHYSX_DIR}/Source/PhysX/src/particles"
    "${PHYSX_DIR}/Source/LowLevel/API/include"
    "${PHYSX_DIR}/Source/LowLevel/software/include"
    "${PHYSX_DIR}/Source/LowLevel/common/include/utils"
    "${PHYSX_DIR}/Source/LowLevel/common/include/math"
    "${PHYSX_DIR}/Source/LowLevel/common/include/pipeline"
    "${PHYSX_DIR}/Source/SceneQuery"
    "${PHYSX_DIR}/Source/SimulationController/include"
    "${PHYSX_DIR}/Source/SimulationController/src"
    "${PHYSX_DIR}/Source/SimulationController/src/framework"
    "${PHYSX_DIR}/Source/SimulationController/src/particles"
    "${PHYSX_DIR}/Source/PvdRuntime/src"
    "${PHYSX_DIR}/Source/PhysXMetaData/core/include"
)

target_link_libraries(${PROJECT_NAME})
