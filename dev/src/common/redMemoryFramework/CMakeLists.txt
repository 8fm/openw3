project(redMemoryFramework)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "redMemoryAllocator.cpp"
    "redMemoryAllocatorRegistration.cpp"
    "redMemoryArrayMetrics.cpp"
    "redMemoryCallstackCachedWriter.cpp"
    "redMemoryCrashReporter.cpp"
    "redMemoryDebugAllocator.cpp"
    "redMemoryFileWriterWinApi.cpp"
    "redMemoryFramework.cpp"
    "redMemoryLeakTracker.cpp"
    "redMemoryManager.cpp"
    "redMemoryMetricsCallstack.cpp"
    "redMemoryMetricsCallstackWinAPI.cpp"
    "redMemoryMetricsCollector.cpp"
    "redMemoryMetricsHistogramData.cpp"
    "redMemoryMetricsSerialiser.cpp"
    "redMemoryMetricsSerialiserWinAPI.cpp"
    "redMemoryMultiAllocator.cpp"
    "redMemoryPageAllocatorWin.cpp"
    "redMemoryRegionAllocator.cpp"
    "redMemorySystemMemoryStatsWinAPI.cpp"
    "redMemoryTlsfImpl.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
