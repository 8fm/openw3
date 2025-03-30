project(PhysX3Cooking)

add_definitions(-DPX_PHYSX_STATIC_LIB -DPX_COOKING)

set(SRC_FILES
    "${PHYSX_DIR}/Source/PhysXCooking/src/BigConvexDataBuilder.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/ConvexHullBuilder.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/ConvexMeshBuilder.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/Cooking.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/CookingUtils.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/GuAdjacencies.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/GuEdgeList.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/GuHeightFieldCooking.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/GuRTreeCooking.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/hulllib.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/IceMeshBuilder2.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/InternalTriangleMeshBuilder.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/TriangleMeshBuilder.cpp"
    "${PHYSX_DIR}/Source/PhysXCooking/src/VolumeIntegration.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PHYSX_DIR}/Include/foundation"
    "${PHYSX_DIR}/Source/foundation/include"
    "${PHYSX_DIR}/Include/physxprofilesdk"
    "${PHYSX_DIR}/Include/physxvisualdebuggersdk"
    "${PHYSX_DIR}/Include/common"
    "${PHYSX_DIR}/Include/geometry"
    "${PHYSX_DIR}/Include/cloth"
    "${PHYSX_DIR}/Include/cooking"
    "${PHYSX_DIR}/Include"
    "${PHYSX_DIR}/Source/Common/src"
    "${PHYSX_DIR}/Source/GeomUtils/headers"
    "${PHYSX_DIR}/Source/GeomUtils/src"
    "${PHYSX_DIR}/Source/GeomUtils/Opcode"
    "${PHYSX_DIR}/Source/PhysXCooking/src"
    "${PHYSX_DIR}/Source/PhysXCooking/include"
    "${PHYSX_DIR}/Source/PhysXExtensions/src"
)

target_link_libraries(${PROJECT_NAME})
