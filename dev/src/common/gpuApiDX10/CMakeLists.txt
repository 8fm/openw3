project(gpuApiDX10)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "gpuApi.cpp"
    "gpuApiBuffer.cpp"
    "gpuApiDDSLoader.cpp"
    "gpuApiDevice.cpp"
    "gpuApiDeviceState.cpp"
    "gpuApiDrawContext.cpp"
    "gpuApiGamma.cpp"
    "gpuApiQuery.cpp"
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
