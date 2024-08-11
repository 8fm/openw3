#include "build.h"

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix1[16] = {
	0.1f,	0.0f,	0.25f,	0.0f,
	0.0f,	0.01f,	0.3f,	0.0f,
	0.0f,	0.0f,	0.0f,	0.0f,
	100.0f,	10.0f,	0.0f,	1.0f 
};

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix2[16] = {
	0.1f,	1.0f,	2.0f,	0.0f,
	0.2f,	5.0f,	3.0f,	0.0f,
	0.0f,	4.0f,	6.0f,	0.0f,
	20.0f,	1.0f,	6.0f,	1.0f 
};

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix3[16] = {
	8.18181801f,	0.909090877f,	-3.18181801f,	0.0f,
	-0.545454562f,	0.272727281f,	0.0454545394f,	0.0f,
	0.363636345f,	-0.181818172f,	0.136363640f,	0.0f,
	-165.272705f,	-17.3636341f,	62.7727280f,	1.f
};

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix4[16] =  {
	0.1f,	0.2f,	0.0f,	20.0f,
	1.0f,	5.0f,	4.0f,	1.0f,
	2.0f,	3.0f,	6.0f,	6.0f,
	0.0f,	0.0f,	0.0f,	1.0f 
};

const Red::System::Float Deg90 = 1.57079633f;

//////////////////////////////////////////////////////////////////////////
// FLOAT
//////////////////////////////////////////////////////////////////////////


#include "../../common/redMath/redMatrix4x4_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		#define REDMATH_TEST(test_case_name, test_name) \
			TEST(test_case_name, RedMatrix4x4_FLOAT_##test_name)

		#define REDMATH_TEST_VERSION_FLOAT
		
		#include "matrix4x4Tests.inl"
	
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

		#include "matrix4x4Tests.inl"

		#undef REDMATH_TEST
		#undef REDMATH_TEST_VERSION_SIMD
	};
};
