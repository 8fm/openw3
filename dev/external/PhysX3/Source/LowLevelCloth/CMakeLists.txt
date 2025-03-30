project(LowLevelCloth)

add_definitions(-DPX_PHYSX_STATIC_LIB)

set(SRC_FILES
    "${PHYSX_DIR}/Source/LowLevelCloth/src/Allocator.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/Factory.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/PhaseConfig.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwCloth.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwClothData.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwCollision.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwFabric.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwFactory.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwInterCollision.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwSelfCollision.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwSolver.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/SwSolverKernel.cpp"
    "${PHYSX_DIR}/Source/LowLevelCloth/src/TripletScheduler.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PHYSX_DIR}/Include/foundation"
    "${PHYSX_DIR}/Source/foundation/include"
    "${PHYSX_DIR}/Include/physxprofilesdk"
    "${PHYSX_DIR}/Include/physxvisualdebuggersdk"
    "${PHYSX_DIR}/Include"
    "${PHYSX_DIR}/Include/pxtask"
    "${PHYSX_DIR}/Source/LowLevelCloth/include"
    "${PHYSX_DIR}/Source/LowLevelCloth/src"
)

target_link_libraries(${PROJECT_NAME})
