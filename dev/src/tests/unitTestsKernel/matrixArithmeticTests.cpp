#include "build.h"

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix1[16] = {     0.7f,  0.2f, 0.05f, 0.0f,
																  0.0f, 0.01f,  1.6f, 0.0f,
																  0.0f,  0.0f,  3.0f, 0.0f,
																200.0f, 10.0f,  0.0f, 1.0f };

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix2[16] = { 0.56f, 1.0f, 2.0f, 0.0f, 
															  0.0f,  2.0f, 3.0f, 0.0f,
															  2.0f,  0.0f, 1.0f, 0.0f,
															  2.0f,  0.0f, 3.0f, 1.0 };

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix3[16] = { 0.492f, 1.1f, 2.05f, 0.0f, 
															  3.2f, 0.02f, 1.6300000000000001f, 0.0f,
															  6.0f, 0.0f, 3.0f, 0.0f,
															  114.00000000000001f, 220.0f, 433.0f, 1.0f };

//////////////////////////////////////////////////////////////////////////
// FLOAT
//////////////////////////////////////////////////////////////////////////


#include "../../common/redMath/redMatrix4x4_float.h"
#include "../../common/redMath/matrixArithmetic_float.h"

namespace RedMath
{
	namespace FLOAT
	{
#define REDMATH_TEST(test_case_name, test_name) \
	TEST(test_case_name, RedMatrix4x4_FLOAT_##test_name)

#define REDMATH_TEST_VERSION_FLOAT

#include "matrixArithmeticTests.inl"

#undef REDMATH_TEST
#undef REDMATH_TEST_VERSION_FLOAT
	};
};


//////////////////////////////////////////////////////////////////////////
// SIMD
//////////////////////////////////////////////////////////////////////////

#include "../../common/redMath/redmathbase.h"

namespace RedMath
{
	namespace SIMD
	{
#define REDMATH_TEST(test_case_name, test_name) \
	TEST(test_case_name, RedMatrix4x4_SIMD_##test_name)

#define REDMATH_TEST_VERSION_SIMD

#include "matrixArithmeticTests.inl"

#undef REDMATH_TEST
#undef REDMATH_TEST_VERSION_SIMD
	};
};