#include "build.h"

// Forwards
namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			class RedMatrix3x3;
			class RedEulerAngles;
			class RedQuaternion;
			class RedQsTransform;
		};
	};
};


#include "../../common/redMath/redmathbase.h"

#include "../../common/redMath/redvector_fpu.h"
#include "../../common/redMath/redmatrix_fpu.h"
#include "../../common/redMath/redquaternion_fpu.h"
#include "../../common/redMath/redqstransform_fpu.h"
#include "../../common/redMath/redeulerangles_fpu.h"

#include "../../common/redMath/redmatrix_fpu.inl"
#include "../../common/redMath/redvector_fpu.inl"
#include "../../common/redMath/redeulerangles_fpu.inl"
#include "../../common/redMath/redquaternion_fpu.inl"

static const Red::System::Float width = 640.0f;
static const Red::System::Float height = 480.0f;
static const Red::System::Float aspect = 1.333333333333333f;
static const Red::System::Float deg90 = 1.57079633f;
static const Red::System::Float nearPlane = 10.0f;
static const Red::System::Float farPlane = 1000.0f;
//ys = 0.999999997
//xs = 0.749999998
//zs = 1.01010096

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestPerspective[16] = { 0.749999998f, 0.0f, 0.0f, 0.0f,
																		   0.0f, 0.999999997f, 0.0f, 0.0f, 
																		   0.0f, 0.0f, 1.01010096f, 1.0f,
																		   0.0f, 0.0f, -10.1010096f, 0.0f };

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestOrthographic[16] = { 0.003125f, 0.0f, 0.0f, 0.0f,
																				0.0f, 0.00416666667f, 0.0f, 0.0f, 
																				0.0f, 0.0f, 0.0010101011f, 0.0f, 
																				0.0f, 0.0f, -0.01010101096f, 1.0f };

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix1[16] = {     0.7f,  0.2f, 0.05f, 0.0f,
																			   0.0f, 0.01f,  1.6f, 0.0f,
																			   0.0f,  0.0f,  3.0f, 0.0f,
																			   200.0f, 10.0f,  0.0f, 1.0f };



//////////////////////////////////////////////////////////////////////////
// FLOAT
//////////////////////////////////////////////////////////////////////////


#include "../../common/redMath/redMatrix4x4_float.h"
#include "../../common/redMath/matrixFunctions_float.h"

namespace RedMath
{
	namespace FLOAT
	{
#define REDMATH_TEST(test_case_name, test_name) \
	TEST(test_case_name, RedMatrix4x4_FLOAT_##test_name)

#define REDMATH_TEST_VERSION_FLOAT

#include "matrixFunctionTests.inl"

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

#include "matrixFunctionTests.inl"

#undef REDMATH_TEST
#undef REDMATH_TEST_VERSION_SIMD
	};
};