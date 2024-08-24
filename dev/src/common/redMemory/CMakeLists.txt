project(redMemory)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "src/allocatorMetrics.cpp"
    "src/block.cpp"
    "src/build.cpp"
    "src/debugAllocator.cpp"
    "src/defaultAllocator.cpp"
    "src/documentation.cpp"
    "src/dynamicFixedSizeAllocator.cpp"
    "src/dynamicTlsfAllocator.cpp"
    "src/fixedSizeAllocator.cpp"
    "src/hook.cpp"
    "src/hookMarkBlock.cpp"
    "src/hookOverrunDetector.cpp"
    "src/hookPool.cpp"
    "src/hookTypes.cpp"
    "src/hookUtils.cpp"
    "src/intrusiveList.cpp"
    "src/legacyGpuAllocator.cpp"
    "src/linearAllocator.cpp"
    "src/lockingDynamicTlsfAllocator.cpp"
    "src/locklessSlabAllocator.cpp"
    "src/log.cpp"
    "src/redMemoryInit.cpp"
    "src/metricsRegistry.cpp"
    "src/metricsUtils.cpp"
    "src/nullAllocator.cpp"
    "src/oomHandler.cpp"
    "src/oomHandlerBreak.cpp"
    "src/oomHandlerIgnore.cpp"
    "src/operatorsLegacy.cpp"
    "src/pageAllocator.cpp"
    "src/pageAllocatorWin.cpp"
    "src/pool.cpp"
    "src/poolStorage.cpp"
    "src/poolMetrics.cpp"
    "src/poolRegistry.cpp"
    "src/hookHandler.cpp"
    "src/poolUnitTest.cpp"
    "src/poolUtils.cpp"
    "src/reportUtils.cpp"
    "src/reporter.cpp"
    "src/slabAllocator.cpp"
    "src/slabChunk.cpp"
    "src/slabHeader.cpp"
    "src/staticFixedSizeAllocator.cpp"
    "src/staticTlsfAllocator.cpp"
    "src/systemAllocator.cpp"
    "src/systemAllocatorType.cpp"
    "src/systemAllocatorWin.cpp"
    "src/systemBlock.cpp"
    "src/threadIdProvider.cpp"
    "src/threadMonitor.cpp"
    "src/tlsfAllocator.cpp"
    "src/tlsfBlock.cpp"
    "src/utils.cpp"
    "src/vault.cpp"
    "src/virtualRange.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "include"
)

target_link_libraries(${PROJECT_NAME})
