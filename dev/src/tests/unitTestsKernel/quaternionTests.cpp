#include "build.h"

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
#include "../../common/redMath/redquaternion_fpu.inl"
#include "../../common/redMath/redqstransform_fpu.h"
#include "../../common/redMath/redeulerangles_fpu.h"

#include "../../common/redMath/redvector_fpu.inl"
#include "../../common/redMath/redmatrix_fpu.inl"
#include "../../common/redMath/redeulerangles_fpu.inl"


RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestFloats[4] = {
	1.0f,	2.0f,	3.0f,	4.0f
};

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix[12] =  {
	0.1f,	0.2f,	0.0f, 0.f,
	1.0f,	5.0f,	4.0f, 0.f,
	2.0f,	3.0f,	6.0f, 0.f,
};

static void TestIdentical(RedMath::SIMD::RedQuaternion &a, Red::Math::Fpu::RedQuaternion &b)
{
	EXPECT_FLOAT_EQ( a.Quat.X, b.Quat.X );
	EXPECT_FLOAT_EQ( a.Quat.Y, b.Quat.Y );
	EXPECT_FLOAT_EQ( a.Quat.Z, b.Quat.Z );
	EXPECT_FLOAT_EQ( a.Quat.W, b.Quat.W );
}


TEST( RedMath, RedQuaternion_construction )
{
	RedMath::SIMD::RedQuaternion simdQuaternion(TestFloats[0], TestFloats[1], TestFloats[2], TestFloats[3]);
	Red::Math::Fpu::RedQuaternion fpuQuaternion(TestFloats[0], TestFloats[1], TestFloats[2], TestFloats[3]);

	TestIdentical(simdQuaternion, fpuQuaternion);
	EXPECT_FLOAT_EQ( simdQuaternion.Quat.X, TestFloats[0] );
	EXPECT_FLOAT_EQ( simdQuaternion.Quat.Y, TestFloats[1] );
	EXPECT_FLOAT_EQ( simdQuaternion.Quat.Z, TestFloats[2] );
	EXPECT_FLOAT_EQ( simdQuaternion.Quat.W, TestFloats[3] );


	RedMath::SIMD::RedVector4 simdVector(TestFloats[0], TestFloats[1], TestFloats[2], TestFloats[3]);
	RedMath::SIMD::RedQuaternion simdQuaternion2(simdVector, 5.f);

	Red::Math::Fpu::RedVector4 FpuVector(TestFloats[0], TestFloats[1], TestFloats[2], TestFloats[3]);
	Red::Math::Fpu::RedQuaternion fpuQuaternion2(FpuVector, 5.f);
	
	TestIdentical(simdQuaternion2, fpuQuaternion2);

	RedMath::SIMD::RedQuaternion simdQuaternion3(simdQuaternion);
	Red::Math::Fpu::RedQuaternion fpuQuaternion3(fpuQuaternion);

	TestIdentical(simdQuaternion3, fpuQuaternion3);
	EXPECT_FLOAT_EQ( simdQuaternion3.Quat.X, TestFloats[0] );
	EXPECT_FLOAT_EQ( simdQuaternion3.Quat.Y, TestFloats[1] );
	EXPECT_FLOAT_EQ( simdQuaternion3.Quat.Z, TestFloats[2] );
	EXPECT_FLOAT_EQ( simdQuaternion3.Quat.W, TestFloats[3] );
}

TEST( RedMath, RedQuaternion_operators )
{
	// Assignment from quat

	RedMath::SIMD::RedQuaternion simdQuaternion(TestFloats[0], TestFloats[1], TestFloats[2], TestFloats[3]);
	Red::Math::Fpu::RedQuaternion fpuQuaternion(TestFloats[0], TestFloats[1], TestFloats[2], TestFloats[3]);

	RedMath::SIMD::RedQuaternion simdQuaternion2;
	Red::Math::Fpu::RedQuaternion fpuQuaternion2;

	simdQuaternion2 = simdQuaternion;
	fpuQuaternion2 = fpuQuaternion;

	TestIdentical(simdQuaternion2, fpuQuaternion2);
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.X, TestFloats[0] );
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.Y, TestFloats[1] );
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.Z, TestFloats[2] );
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.W, TestFloats[3] );

	// Assignment from vector

	RedMath::SIMD::RedVector4 simdVector(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);
	Red::Math::Fpu::RedVector4 FpuVector(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);

	simdQuaternion2 = simdVector;
	fpuQuaternion2 = FpuVector;

	TestIdentical(simdQuaternion2, fpuQuaternion2);
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.X, TestFloats[3] );
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.Y, TestFloats[2] );
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.Z, TestFloats[1] );
	EXPECT_FLOAT_EQ( simdQuaternion2.Quat.W, TestFloats[0] );
}

TEST( RedMath, RedQuaternion_miscFunctions )
{
	// ConstructFromMatrix()

	RedMath::SIMD::RedMatrix3x3 simdMat(TestMatrix);
	Red::Math::Fpu::RedMatrix3x3 fpuMat(TestMatrix);

	RedMath::SIMD::RedQuaternion simdQuat;
	Red::Math::Fpu::RedQuaternion fpuQuat;

	simdQuat.ConstructFromMatrix(simdMat);
	fpuQuat.ConstructFromMatrix(fpuMat);

	TestIdentical(simdQuat, fpuQuat);

	// Set(float _x, float _y, float _z, float _w)

	simdQuat.Set(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);
	fpuQuat.Set(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);

	TestIdentical(simdQuat, fpuQuat);
	EXPECT_FLOAT_EQ( simdQuat.Quat.X, TestFloats[3] );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Y, TestFloats[2] );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Z, TestFloats[1] );
	EXPECT_FLOAT_EQ( simdQuat.Quat.W, TestFloats[0] );

	// Set( const RedMatrix3x3& _rotMat )

	simdQuat.Set(simdMat);
	fpuQuat.Set(fpuMat);
	TestIdentical(simdQuat, fpuQuat);

	// SetIdentity()

	simdQuat.SetIdentity();
	fpuQuat.SetIdentity();

	TestIdentical(simdQuat, fpuQuat);
	EXPECT_FLOAT_EQ( simdQuat.Quat.X, 0.f );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQuat.Quat.W, 1.f );

	// SetInverse()

	RedMath::SIMD::RedQuaternion simdQuat2(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);
	simdQuat.SetInverse(simdQuat2);

	Red::Math::Fpu::RedQuaternion fpuQuat2(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);
	fpuQuat.SetInverse(fpuQuat2);

	TestIdentical(simdQuat, fpuQuat);
	EXPECT_FLOAT_EQ( simdQuat.Quat.X, -TestFloats[3] );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Y, -TestFloats[2] );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Z, -TestFloats[1] );
	EXPECT_FLOAT_EQ( simdQuat.Quat.W, TestFloats[0] );

	// Normalize()

	simdQuat.Normalize();
	fpuQuat.Normalize();

	TestIdentical(simdQuat, fpuQuat);

	// Mul()

	RedMath::SIMD::RedQuaternion simdQuat3 = RedMath::SIMD::RedQuaternion::Mul(simdQuat, simdQuat2);
	Red::Math::Fpu::RedQuaternion fpuQuat3 = Red::Math::Fpu::RedQuaternion::Mul(fpuQuat, fpuQuat2);

	TestIdentical(simdQuat3, fpuQuat3);

	// Add()

	simdQuat3 = RedMath::SIMD::RedQuaternion::Add(simdQuat, simdQuat2);
	fpuQuat3 = Red::Math::Fpu::RedQuaternion::Add(fpuQuat, fpuQuat2);

	TestIdentical(simdQuat3, fpuQuat3);
	
	// SetMul()

	simdQuat3.SetMul(simdQuat, simdQuat2);
	fpuQuat3.SetMul(fpuQuat, fpuQuat2);

	TestIdentical(simdQuat3, fpuQuat3);

	// SetMulInverse()

	simdQuat3.SetMulInverse(simdQuat, simdQuat2);
	fpuQuat3.SetMulInverse(fpuQuat, fpuQuat2);

	TestIdentical(simdQuat3, fpuQuat3);

	// SetInverseMul()

	simdQuat3.SetMulInverse(simdQuat, simdQuat2);
	fpuQuat3.SetMulInverse(fpuQuat, fpuQuat2);

	TestIdentical(simdQuat3, fpuQuat3);

	// EstimateAngleTo()

	RedMath::SIMD::RedVector4 simdVec = simdQuat3.EstimateAngleTo(simdQuat2);
	Red::Math::Fpu::RedVector4 fpuVec = fpuQuat3.EstimateAngleTo(fpuQuat2);

	EXPECT_FLOAT_EQ( simdVec.X, fpuVec.X );
	EXPECT_FLOAT_EQ( simdVec.Y, fpuVec.Y );
	EXPECT_FLOAT_EQ( simdVec.Z, fpuVec.Z );
	EXPECT_FLOAT_EQ( simdVec.W, fpuVec.W );

	// SetShortestRotation()

	RedMath::SIMD::RedVector4 simdVec2(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);
	Red::Math::Fpu::RedVector4 fpuVec2(TestFloats[3], TestFloats[2], TestFloats[1], TestFloats[0]);
	
	simdQuat.SetShortestRotation(simdVec2, simdVec);
	fpuQuat.SetShortestRotation(fpuVec2, fpuVec);

	TestIdentical(simdQuat, fpuQuat);

	// SetShortestRotationDamped()

	simdQuat.SetShortestRotationDamped(0.1f, simdVec, simdVec2);
	fpuQuat.SetShortestRotationDamped(0.1f, fpuVec, fpuVec2);

	TestIdentical(simdQuat, fpuQuat);

	// SetAxisAngle()

	simdQuat.SetAxisAngle(simdVec2, 2.f);
	fpuQuat.SetAxisAngle(fpuVec2, 2.f);

	TestIdentical(simdQuat, fpuQuat);

	// SetAndNormalize()

	simdQuat.SetAndNormalize(simdMat);
	fpuQuat.SetAndNormalize(fpuMat);

	TestIdentical(simdQuat, fpuQuat);

	// RemoveAxisComponent()

	simdQuat.RemoveAxisComponent(simdVec2);
	fpuQuat.RemoveAxisComponent(fpuVec2);

	TestIdentical(simdQuat, fpuQuat);

	// DecomposeRestAxis()

	float angle1, angle2;
	simdQuat.DecomposeRestAxis(simdVec2, simdQuat2, angle1);
	fpuQuat.DecomposeRestAxis(fpuVec2, fpuQuat2, angle2);

	TestIdentical(simdQuat2, fpuQuat2);
	EXPECT_FLOAT_EQ( simdVec2.X, fpuVec2.X );
	EXPECT_FLOAT_EQ( simdVec2.Y, fpuVec2.Y );
	EXPECT_FLOAT_EQ( simdVec2.Z, fpuVec2.Z );
	EXPECT_FLOAT_EQ( simdVec2.W, fpuVec2.W );
	EXPECT_FLOAT_EQ( angle1, angle2 );

	// SetLerp()

	simdQuat2.SetLerp(simdQuat, simdQuat3, 0.5f);
	fpuQuat2.SetLerp(fpuQuat, fpuQuat3, 0.5f);

	TestIdentical(simdQuat2, fpuQuat2);

	// SetSlerp()

	simdQuat.SetSlerp(simdQuat3, simdQuat2, 0.5f);
	fpuQuat.SetSlerp(fpuQuat3, fpuQuat2, 0.5f);

	TestIdentical(simdQuat, fpuQuat);

	// SetReal()

	simdQuat.SetReal(0.95f);
	fpuQuat.SetReal(0.95f);

	TestIdentical(simdQuat, fpuQuat);

	// GetReal()

	EXPECT_FLOAT_EQ( simdQuat2.GetReal(), fpuQuat2.GetReal() );

	// SetImaginary()

	simdQuat.SetImaginary(RedMath::SIMD::RedVector4(3.f, 2.f, 1.f, 2.f));
	fpuQuat.SetImaginary(Red::Math::Fpu::RedVector4(3.f, 2.f, 1.f, 2.f));

	TestIdentical(simdQuat, fpuQuat);

	EXPECT_FLOAT_EQ( simdQuat.Quat.X, 3.f );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQuat.Quat.Z, 1.f );

	// GetImaginary()

	simdVec = simdQuat.GetImaginary();
	fpuVec = fpuQuat.GetImaginary();

	// GetAngle()

	EXPECT_FLOAT_EQ( simdQuat.GetAngle(), fpuQuat.GetAngle() );

	// GetYaw()

	EXPECT_FLOAT_EQ( simdQuat.GetYaw(), fpuQuat.GetYaw() );
	
	// GetAxis()

	simdVec = simdQuat.GetAxis();
	fpuVec = fpuQuat.GetAxis();

	EXPECT_FLOAT_EQ( simdVec.X, fpuVec.X );
	EXPECT_FLOAT_EQ( simdVec.Y, fpuVec.Y );
	EXPECT_FLOAT_EQ( simdVec.Z, fpuVec.Z );

	// HasValidAxis()

	EXPECT_TRUE(simdQuat.HasValidAxis() == fpuQuat.HasValidAxis());
	EXPECT_TRUE(simdQuat2.HasValidAxis() == fpuQuat2.HasValidAxis());
	EXPECT_TRUE(simdQuat3.HasValidAxis() == fpuQuat3.HasValidAxis());

	// IsOk()

	EXPECT_TRUE(simdQuat.IsOk() == fpuQuat.IsOk());
	EXPECT_TRUE(simdQuat2.IsOk() == fpuQuat2.IsOk());
	EXPECT_TRUE(simdQuat3.IsOk() == fpuQuat3.IsOk());

	// SetFlippedRotation()

	simdQuat2.SetFlippedRotation(simdVec2);
	fpuQuat2.SetFlippedRotation(fpuVec2);

	TestIdentical(simdQuat2, fpuQuat2);
}