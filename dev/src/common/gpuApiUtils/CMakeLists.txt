project(gpuApiUtils)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "gpuApiCommon.cpp"
    "gpuApiDDSLoader.cpp"
    "gpuApiMemory.cpp"
    "gpuApiShaderParsing.cpp"
    "gpuApiUtils.cpp"
    "gpuApiVertexFormats.cpp"
    "gpuApiVertexLayoutsCommon.cpp"
    "gpuApiVertexPacking.cpp"
)

set(MOJO_SRC
    "mojoshader/mojoshader.c"
    "mojoshader/mojoshader_assembler.c"
    "mojoshader/mojoshader_common.c"
    "mojoshader/mojoshader_compiler.c"
    "mojoshader/mojoshader_effects.c"
    "mojoshader/mojoshader_lexer.c"
    "mojoshader/mojoshader_opengl.c"
    "mojoshader/mojoshader_preprocessor.c"
)
set_source_files_properties(${MOJO_SRC} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
set_source_files_properties(${MOJO_SRC} PROPERTIES LANGUAGE CXX)
list(APPEND SRC_FILES ${MOJO_SRC})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    gpuApiUtils.h
)
