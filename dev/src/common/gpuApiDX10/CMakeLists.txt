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

if (WIN32)
    target_link_libraries(${PROJECT_NAME})
else()
    add_dependencies(${PROJECT_NAME} dxvk vkd3d)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${DIRECTX_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${DIRECTX_MATH_LIBS}
        ${DIRECTX_LIBS}
    )
endif()

