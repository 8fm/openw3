project(physics)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "build.cpp"
    "compiledCollision.cpp"
    "physicalCollision.cpp"
    "physicsChainedRagdollWrapperPhysx.cpp"
    "physicsDebuggerPhysx.cpp"
    "physicsEngine.cpp"
    "physicsJointedRagdollWrapperPhysx.cpp"
    "physicsLogger.cpp"
    "physicsParticleWrapperPhysx.cpp"
    "physicsRagdollWrapperPhysx.cpp"
    "physicsSettings.cpp"
    "physicsSimpleBodyWrapperPhysx.cpp"
    "physicsWrapperPhysx.cpp"
    "physicsWorld.cpp"
    "physicsWorldInvalidAreaCache.cpp"
    "physicsWorldPhysx.cpp"
    "physicsWorldBatchTracePhysx.cpp"
    "physicsWorldTrace.cpp"
    "physXCpuDispatcher.cpp"
    "physXEngine.cpp"
    "PhysXStreams.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/characterkinematic
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/common
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/cooking
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/extensions
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/foundation
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/geometry
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/gpu
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/physxvisualdebuggersdk
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Include/pxtask
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/framework/public/
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/framework/public/PhysX3/
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/module/destructible/public
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/NxParameterized/public
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/shared/external/include
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/public
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/module/clothing/public
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Source
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Source/foundation/include
    ${CMAKE_SOURCE_DIR}/dev/external/PhysX3/Source/common/src
    ${CMAKE_SOURCE_DIR}/dev/external/Apex/module/legacy/public
)

target_link_libraries(${PROJECT_NAME})
