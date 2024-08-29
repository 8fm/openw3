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
    "redMemoryFramework.cpp"
    "redMemoryLeakTracker.cpp"
    "redMemoryManager.cpp"
    "redMemoryMetricsCallstack.cpp"
    "redMemoryMetricsCollector.cpp"
    "redMemoryMetricsHistogramData.cpp"
    "redMemoryMetricsSerialiser.cpp"
    "redMemoryMultiAllocator.cpp"
    "redMemoryRegionAllocator.cpp"
    "redMemoryTlsfImpl.cpp"
)

if (WIN32)
    list(APPEND SRC_FILES "redMemoryFileWriterWinApi.cpp")
    list(APPEND SRC_FILES "redMemoryMetricsCallstackWinAPI.cpp")
    list(APPEND SRC_FILES "redMemoryMetricsSerialiserWinAPI.cpp")
    list(APPEND SRC_FILES "redMemoryPageAllocatorWin.cpp")
    list(APPEND SRC_FILES "redMemorySystemMemoryStatsWinAPI.cpp")
else()
    list(APPEND SRC_FILES "redMemoryFileWriterLinux.cpp")
    list(APPEND SRC_FILES "redMemoryMetricsCallstackLinux.cpp")
    list(APPEND SRC_FILES "redMemoryMetricsSerialiserLinux.cpp")
    list(APPEND SRC_FILES "redMemoryPageAllocatorLinux.cpp")
    list(APPEND SRC_FILES "redMemorySystemMemoryStatsLinuxAPI.cpp")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
