#include "build.h"

// Forwards
namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			class RedMatrix3x3;
			class RedQuaternion;
			class RedQsTransform;
		};
	};
};

#include "../../common/redMath/redmathbase.h"

#include "../../common/redMath/redvector_fpu.h"
#include "../../common/redMath/redaabb_fpu.h"
#include "../../common/redMath/redquaternion_fpu.h"
#include "../../common/redMath/redqstransform_fpu.h"
#include "../../common/redMath/redmatrix_fpu.h"

#include "../../common/redMath/redmatrix_fpu.inl"
#include "../../common/redMath/redaabb_fpu.inl"
#include "../../common/redMath/redvector_fpu.inl"
#include "../../common/redMath/redquaternion_fpu.inl"

static void TestIdentical(RedMath::SIMD::RedAABB &a, Red::Math::Fpu::RedAABB &b)
{
	EXPECT_FLOAT_EQ(a.Min.X, a.Min.X);
	EXPECT_FLOAT_EQ(a.Min.Y, a.Min.Y);
	EXPECT_FLOAT_EQ(a.Min.Z, a.Min.Z);
	EXPECT_FLOAT_EQ(a.Min.W, a.Min.W);

	EXPECT_FLOAT_EQ(a.Max.X, a.Max.X);
	EXPECT_FLOAT_EQ(a.Max.Y, a.Max.Y);
	EXPECT_FLOAT_EQ(a.Max.Z, a.Max.Z);
	EXPECT_FLOAT_EQ(a.Max.W, a.Max.W);
}

TEST( RedMath, RedAABB_construction )
{
	// RedAABB( const RedVector4& _min, const RedVector4& _max )

	RedMath::SIMD::RedVector4 v1(-1.f, -2.f, -3.f, -4.f);
	RedMath::SIMD::RedVector4 v2(1.f, 2.f, 3.f, 4.f);
	RedMath::SIMD::RedAABB simdAABB(v1, v2);

	Red::Math::Fpu::RedVector4 v3(-1.f, -2.f, -3.f, -4.f);
	Red::Math::Fpu::RedVector4 v4(1.f, 2.f, 3.f, 4.f);
	Red::Math::Fpu::RedAABB fpuAABB(v3, v4);

	TestIdentical(simdAABB, fpuAABB);

	EXPECT_FLOAT_EQ(simdAABB.Min.X, -1.f);
	EXPECT_FLOAT_EQ(simdAABB.Min.Y, -2.f);
	EXPECT_FLOAT_EQ(simdAABB.Min.Z, -3.f);
	EXPECT_FLOAT_EQ(simdAABB.Min.W, -4.f);

	EXPECT_FLOAT_EQ(simdAABB.Max.X, 1.f);
	EXPECT_FLOAT_EQ(simdAABB.Max.Y, 2.f);
	EXPECT_FLOAT_EQ(simdAABB.Max.Z, 3.f);
	EXPECT_FLOAT_EQ(simdAABB.Max.W, 4.f);

	// RedAABB( const RedVector3& _min, const RedVector3& _max )

	RedMath::SIMD::RedVector3 v31(-1.f, -2.f, -3.f);
	RedMath::SIMD::RedVector3 v32(1.f, 2.f, 3.f);
	RedMath::SIMD::RedAABB simdAABB2(v31, v32);

	Red::Math::Fpu::RedVector3 v33(-1.f, -2.f, -3.f);
	Red::Math::Fpu::RedVector3 v34(1.f, 2.f, 3.f);
	Red::Math::Fpu::RedAABB fpuAABB2(v33, v34);

	TestIdentical(simdAABB2, fpuAABB2);

	EXPECT_FLOAT_EQ(simdAABB2.Min.X, -1.f);
	EXPECT_FLOAT_EQ(simdAABB2.Min.Y, -2.f);
	EXPECT_FLOAT_EQ(simdAABB2.Min.Z, -3.f);
	
	EXPECT_FLOAT_EQ(simdAABB2.Max.X, 1.f);
	EXPECT_FLOAT_EQ(simdAABB2.Max.Y, 2.f);
	EXPECT_FLOAT_EQ(simdAABB2.Max.Z, 3.f);
}

TEST( RedMath, RedAABB_misc )
{
	// OverlapsWith( const RedAABB& _box )

	RedMath::SIMD::RedVector3 vs1a(-2.f, -2.f, -2.f);
	RedMath::SIMD::RedVector3 vs1b(-1.f, -1.f, -1.f);
	RedMath::SIMD::RedAABB simdAABB1(vs1a, vs1b);

	RedMath::SIMD::RedVector3 vs2a(1.f, 1.f, 1.f);
	RedMath::SIMD::RedVector3 vs2b(2.f, 2.f, 2.f);
	RedMath::SIMD::RedAABB simdAABB2(vs2a, vs2b);

	RedMath::SIMD::RedVector3 vs3a(-1.f, -1.f, -1.f);
	RedMath::SIMD::RedVector3 vs3b(3.f, 3.f, 3.f);
	RedMath::SIMD::RedAABB simdAABB3(vs3a, vs3b);

	Red::Math::Fpu::RedVector3 vf1a(-2.f, -2.f, -2.f);
	Red::Math::Fpu::RedVector3 vf1b(-1.f, -1.f, -1.f);
	Red::Math::Fpu::RedAABB fpuAABB1(vf1a, vf1b);

	Red::Math::Fpu::RedVector3 vf2a(1.f, 1.f, 1.f);
	Red::Math::Fpu::RedVector3 vf2b(2.f, 2.f, 2.f);
	Red::Math::Fpu::RedAABB fpuAABB2(vf2a, vf2b);

	Red::Math::Fpu::RedVector3 vf3a(-1.f, -1.f, -1.f);
	Red::Math::Fpu::RedVector3 vf3b(3.f, 3.f, 3.f);
	Red::Math::Fpu::RedAABB fpuAABB3(vf3a, vf3b);

	EXPECT_FALSE(simdAABB1.OverlapsWith(simdAABB2));
	EXPECT_TRUE(simdAABB1.OverlapsWith(simdAABB3));
	EXPECT_TRUE(simdAABB3.OverlapsWith(simdAABB1));

	EXPECT_FALSE(fpuAABB1.OverlapsWith(fpuAABB2));
	EXPECT_TRUE(fpuAABB1.OverlapsWith(fpuAABB3));
	EXPECT_TRUE(fpuAABB3.OverlapsWith(fpuAABB1));
 
	// Contains()

	EXPECT_FALSE(simdAABB3.Contains(simdAABB1));
	EXPECT_TRUE(simdAABB3.Contains(simdAABB2));
	
	EXPECT_FALSE(fpuAABB3.Contains(fpuAABB1));
	EXPECT_TRUE(fpuAABB3.Contains(fpuAABB2));
	
	// ContainsPoint()

	EXPECT_FALSE(simdAABB1.ContainsPoint(RedMath::SIMD::RedVector4(0.f, 0.f, 0.f, 0.f)));
	EXPECT_TRUE(simdAABB3.ContainsPoint(RedMath::SIMD::RedVector4(0.f, 0.f, 0.f, 0.f)));
	EXPECT_TRUE(simdAABB2.ContainsPoint(RedMath::SIMD::RedVector4(1.f, 1.f, 1.f, 0.f)));
	
	EXPECT_FALSE(fpuAABB1.ContainsPoint(Red::Math::Fpu::RedVector4(0.f, 0.f, 0.f, 0.f)));
	EXPECT_TRUE( fpuAABB3.ContainsPoint(Red::Math::Fpu::RedVector4(0.f, 0.f, 0.f, 0.f)));
	EXPECT_TRUE( fpuAABB2.ContainsPoint(Red::Math::Fpu::RedVector4(1.f, 1.f, 1.f, 0.f)));

	// AddPoint()
	
	simdAABB1.AddPoint(RedMath::SIMD::RedVector4(0.f, 0.f, 0.f, 0.f));
	simdAABB1.AddPoint(RedMath::SIMD::RedVector4(-3.f, -3.f, -3.f, 0.f));

	fpuAABB1.AddPoint(Red::Math::Fpu::RedVector4(0.f, 0.f, 0.f, 0.f));
	fpuAABB1.AddPoint(Red::Math::Fpu::RedVector4(-3.f, -3.f, -3.f, 0.f));

	TestIdentical(simdAABB1, fpuAABB1);

	EXPECT_FLOAT_EQ(simdAABB1.Min.X, -3.f);
	EXPECT_FLOAT_EQ(simdAABB1.Min.Y, -3.f);
	EXPECT_FLOAT_EQ(simdAABB1.Min.Z, -3.f);
							
	EXPECT_FLOAT_EQ(simdAABB1.Max.X, 0.f);
	EXPECT_FLOAT_EQ(simdAABB1.Max.Y, 0.f);
	EXPECT_FLOAT_EQ(simdAABB1.Max.Z, 0.f);

	// AddAABB()

	simdAABB2.AddAABB(simdAABB1);
	fpuAABB2.AddAABB(fpuAABB1);

	TestIdentical(simdAABB2, fpuAABB2);

	EXPECT_FLOAT_EQ(simdAABB2.Min.X, -3.f);
	EXPECT_FLOAT_EQ(simdAABB2.Min.Y, -3.f);
	EXPECT_FLOAT_EQ(simdAABB2.Min.Z, -3.f);

	EXPECT_FLOAT_EQ(simdAABB2.Max.X, 2.f);
	EXPECT_FLOAT_EQ(simdAABB2.Max.Y, 2.f);
	EXPECT_FLOAT_EQ(simdAABB2.Max.Z, 2.f);
	
	// IsOk()

	EXPECT_TRUE(fpuAABB1.IsOk());
	EXPECT_TRUE(fpuAABB2.IsOk());
	EXPECT_TRUE(fpuAABB3.IsOk());
	EXPECT_TRUE(simdAABB1.IsOk());
	EXPECT_TRUE(simdAABB2.IsOk());
	EXPECT_TRUE(simdAABB3.IsOk());

	simdAABB3.Max.X = simdAABB3.Min.X - 1.f;
	fpuAABB3.Max.X = fpuAABB3.Min.X - 1.f;

	EXPECT_FALSE(fpuAABB3.IsOk());
	EXPECT_FALSE(simdAABB3.IsOk());

	// SetEmpty()
	// IsEmpty()

	EXPECT_FALSE(simdAABB1.IsEmpty());
	EXPECT_FALSE(fpuAABB1.IsEmpty());

	simdAABB1.SetEmpty();
	fpuAABB1.SetEmpty();

	EXPECT_TRUE(simdAABB1.IsEmpty());
	EXPECT_TRUE(fpuAABB1.IsEmpty());

	TestIdentical(simdAABB1, fpuAABB1);
}