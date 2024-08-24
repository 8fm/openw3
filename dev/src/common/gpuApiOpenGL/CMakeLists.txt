project(gpuApiOpenGL)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "gpuApiBuffer.cpp"
    "gpuApiDevice.cpp"
    "gpuApiDrawContext.cpp"
    "gpuApiRender.cpp"
    "gpuApiRenderState.cpp"
    "gpuApiRenderTarget.cpp"
    "gpuApiSamplerStates.cpp"
    "gpuApiShader.cpp"
    "gpuApiTexture.cpp"
    "gpuApiVertexLayouts.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
