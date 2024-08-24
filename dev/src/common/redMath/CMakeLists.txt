project(redMath)

set(TARGET_TYPE "StaticLibrary")
include(${CMAKE_SOURCE_DIR}/cmake/BuildOptions.cmake)

set(SRC_FILES
    "constants_fpu.cpp"
    "mathTypes.cpp"
    "matrixArithmetic_float.cpp"
    "matrixArithmetic_simd.cpp"
    "matrixFunctions_float.cpp"
    "matrixFunctions_simd.cpp"
    "random/fastRand.cpp"
    "random/mersenneTwister.cpp"
    "random/noise.cpp"
    "random/random.cpp"
    "random/standardRand.cpp"
    "random/standardRand16.cpp"
    "random/standardRand32.cpp"
    "random/xorshift.cpp"
    "redMatrix3x3_float.cpp"
    "redMatrix3x3_simd.cpp"
    "redMatrix4x4_float.cpp"
    "redMatrix4x4_simd.cpp"
    "redquaternion_simd.cpp"
    "redVector2_float.cpp"
    "redVector2_simd.cpp"
    "redVector3_float.cpp"
    "redVector3_simd.cpp"
    "redVector4_float.cpp"
    "redVector4_simd.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME})
