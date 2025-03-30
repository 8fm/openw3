project(PhysX3CharacterKinematic)

add_definitions(-DPX_PHYSX_CHARACTER_EXPORTS -D_LIB -DPX_PHYSX_STATIC_LIB)

set(SRC_FILES
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctBoxController.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctCapsuleController.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctCharacterController.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctCharacterControllerCallbacks.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctCharacterControllerManager.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctController.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctObstacleContext.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctSweptBox.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctSweptCapsule.cpp"
    "${PHYSX_DIR}/Source/PhysXCharacterKinematic/src/CctSweptVolume.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PHYSX_DIR}/Include/foundation"
    "${PHYSX_DIR}/Source/foundation/include"
    "${PHYSX_DIR}/Include/physxprofilesdk"
    "${PHYSX_DIR}/Include/physxvisualdebuggersdk"
    "${PHYSX_DIR}/Include/characterkinematic"
    "${PHYSX_DIR}/Include/common"
    "${PHYSX_DIR}/Include/geometry"
    "${PHYSX_DIR}/Include/extensions"
    "${PHYSX_DIR}/Include/pxtask"
    "${PHYSX_DIR}/Include"
    "${PHYSX_DIR}/Source/GeomUtils/headers"
    "${PHYSX_DIR}/Source/Common/src"
)

target_link_libraries(${PROJECT_NAME})
