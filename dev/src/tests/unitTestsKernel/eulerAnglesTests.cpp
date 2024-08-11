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
			class RedTransform;
			class RedQsTransform;
			class RedAABB;
		};
	};
};

#include "../../common/redMath/redmathbase.h"

#include "../../common/redMath/redvector_fpu.h"
#include "../../common/redMath/redmatrix_fpu.h"
#include "../../common/redMath/redquaternion_fpu.h"
#include "../../common/redMath/redqstransform_fpu.h"
#include "../../common/redMath/redeulerangles_fpu.h"

#include "../../common/redMath/redeulerangles_fpu.inl"
#include "../../common/redMath/redvector_fpu.inl"
#include "../../common/redMath/redquaternion_fpu.inl"
#include "../../common/redMath/redmatrix_fpu.inl"


static void TestIdentical(RedMath::SIMD::RedEulerAngles &a, Red::Math::Fpu::RedEulerAngles &b)
{
	EXPECT_FLOAT_EQ( a.Pitch, b.Pitch );
	EXPECT_FLOAT_EQ( a.Roll, b.Roll );
	EXPECT_FLOAT_EQ( a.Yaw, b.Yaw );
}


TEST( RedMath, RedEulerAngles_construction )
{
	// RedEulerAngles( float _roll, float _pitch, float _yaw )

	RedMath::SIMD::RedEulerAngles simdEA(1.f, 2.f, 3.f);
	Red::Math::Fpu::RedEulerAngles fpuEA(1.f, 2.f, 3.f);

	TestIdentical(simdEA, fpuEA);

	EXPECT_FLOAT_EQ( simdEA.Roll, 1.f );
	EXPECT_FLOAT_EQ( simdEA.Pitch, 2.f );
	EXPECT_FLOAT_EQ( simdEA.Yaw, 3.f );

	// RedEulerAngles( const RedEulerAngles& _e )

	RedMath::SIMD::RedEulerAngles simdEA2(simdEA);
	Red::Math::Fpu::RedEulerAngles fpuEA2(fpuEA);

	TestIdentical(simdEA2, fpuEA2);

	EXPECT_FLOAT_EQ( simdEA.Roll, 1.f );
	EXPECT_FLOAT_EQ( simdEA.Pitch, 2.f );
	EXPECT_FLOAT_EQ( simdEA.Yaw, 3.f );

	// RedEulerAngles( const RedVector3& _v )

	RedMath::SIMD::RedEulerAngles simdEA3(RedMath::SIMD::RedVector3(3.f, 2.f, 1.f));
	Red::Math::Fpu::RedEulerAngles fpuEA3(Red::Math::Fpu::RedVector3(3.f, 2.f, 1.f));

	TestIdentical(simdEA3, fpuEA3);

	// RedEulerAngles( const RedVector4& _v );

	RedMath::SIMD::RedEulerAngles simdEA4(RedMath::SIMD::RedVector4(3.f, 4.f, 5.f, 6.f));
	Red::Math::Fpu::RedEulerAngles fpuEA4(Red::Math::Fpu::RedVector4(3.f, 4.f, 5.f, 6.f));

	TestIdentical(simdEA4, fpuEA4);
}

TEST( RedMath, RedEulerAngles_misc )
{
	// IsAlmostEqual()

	RedMath::SIMD::RedEulerAngles simdEA(1.f, 2.f, 3.f);
	Red::Math::Fpu::RedEulerAngles fpuEA(1.f, 2.f, 3.f);

	RedMath::SIMD::RedEulerAngles simdEA2(1.1f, 2.1f, 3.1f);
	Red::Math::Fpu::RedEulerAngles fpuEA2(1.1f, 2.1f, 3.1f);

	EXPECT_TRUE(simdEA.IsAlmostEqual(simdEA2, 0.5f));
	EXPECT_TRUE(fpuEA.IsAlmostEqual(fpuEA2, 0.5f));

	EXPECT_FALSE(simdEA.IsAlmostEqual(simdEA2, 0.05f));
	EXPECT_FALSE(fpuEA.IsAlmostEqual(fpuEA2, 0.05f));

	// ToMatrix4()

	RedMath::SIMD::RedMatrix4x4 simdMat = simdEA.ToMatrix4();
	Red::Math::Fpu::RedMatrix4x4 fpuMat = fpuEA.ToMatrix4();

	EXPECT_TRUE(memcmp(&simdMat, &fpuMat, sizeof(simdMat)) == 0);

	// ToMatrix4()

	RedMath::SIMD::RedMatrix4x4 simdMat2;
	Red::Math::Fpu::RedMatrix4x4 fpuMat2;
	simdEA.ToMatrix4(simdMat2);
	fpuEA.ToMatrix4(fpuMat2);

	EXPECT_TRUE(memcmp(&simdMat2, &fpuMat2, sizeof(simdMat2)) == 0);
	EXPECT_TRUE(memcmp(&simdMat2, &simdMat, sizeof(simdMat2)) == 0);

	// ToAngleVectors()

	RedMath::SIMD::RedVector4 forward1, right1, up1;
	Red::Math::Fpu::RedVector4 forward2, right2, up2;

	simdEA.ToAngleVectors(&forward1, &right1, &up1);
	fpuEA.ToAngleVectors(&forward2, &right2, &up2);

	EXPECT_FLOAT_EQ(forward1.X, forward2.X);
	EXPECT_FLOAT_EQ(forward1.Y, forward2.Y);
	EXPECT_FLOAT_EQ(forward1.Z, forward2.Z);
	EXPECT_FLOAT_EQ(forward1.W, forward2.W);

	EXPECT_FLOAT_EQ(right1.X, right2.X);
	EXPECT_FLOAT_EQ(right1.Y, right2.Y);
	EXPECT_FLOAT_EQ(right1.Z, right2.Z);
	EXPECT_FLOAT_EQ(right1.W, right2.W);

	EXPECT_FLOAT_EQ(up1.X, up2.X);
	EXPECT_FLOAT_EQ(up1.Y, up2.Y);
	EXPECT_FLOAT_EQ(up1.Z, up2.Z);
	EXPECT_FLOAT_EQ(up1.W, up2.W);

	// ToQuaternion()

	RedMath::SIMD::RedQuaternion simdQuat = simdEA.ToQuaternion();
	Red::Math::Fpu::RedQuaternion fpuQuat = fpuEA.ToQuaternion();

	EXPECT_FLOAT_EQ(simdQuat.Quat.X, fpuQuat.Quat.X);
	EXPECT_FLOAT_EQ(simdQuat.Quat.Y, fpuQuat.Quat.Y);
	EXPECT_FLOAT_EQ(simdQuat.Quat.Z, fpuQuat.Quat.Z);
	EXPECT_FLOAT_EQ(simdQuat.Quat.W, fpuQuat.Quat.W);

	// TransformPoint()

	RedMath::SIMD::RedVector4 out1 = simdEA.TransformPoint(RedMath::SIMD::RedVector4(4.f, 3.f, 2.f, 1.f));
	Red::Math::Fpu::RedVector4 out2 = fpuEA.TransformPoint(Red::Math::Fpu::RedVector4(4.f, 3.f, 2.f, 1.f));

	EXPECT_FLOAT_EQ(out1.X, out2.X);
	EXPECT_FLOAT_EQ(out1.Y, out2.Y);
	EXPECT_FLOAT_EQ(out1.Z, out2.Z);
	EXPECT_FLOAT_EQ(out1.W, out2.W);

	// TransformVector()

	RedMath::SIMD::RedVector4 out3 = simdEA.TransformVector(RedMath::SIMD::RedVector4(4.f, 5.f, 6.f, 7.f));
	Red::Math::Fpu::RedVector4 out4 = fpuEA.TransformVector(Red::Math::Fpu::RedVector4(4.f, 5.f, 6.f, 7.f));

	EXPECT_FLOAT_EQ(out3.X, out4.X);
	EXPECT_FLOAT_EQ(out3.Y, out4.Y);
	EXPECT_FLOAT_EQ(out3.Z, out4.Z);
	EXPECT_FLOAT_EQ(out3.W, out4.W);

	// NormalizeAngle()

	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::NormalizeAngle(100.f), Red::Math::Fpu::RedEulerAngles::NormalizeAngle(100.f));
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::NormalizeAngle(100.f), 100.f);
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::NormalizeAngle(-100.f), Red::Math::Fpu::RedEulerAngles::NormalizeAngle(-100.f));
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::NormalizeAngle(-100.f), 260.f);
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::NormalizeAngle(1000.f), Red::Math::Fpu::RedEulerAngles::NormalizeAngle(1000.f));
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::NormalizeAngle(1000.f), 280.f);

	// RedEulerAngles& Normalize();

	RedMath::SIMD::RedEulerAngles simdEA3(100.f, -100.f, 1000.f);
	Red::Math::Fpu::RedEulerAngles fpuEA3(100.f, -100.f, 1000.f);

	simdEA3.Normalize();
	fpuEA3.Normalize();

	TestIdentical(simdEA3, fpuEA3);

	EXPECT_FLOAT_EQ(simdEA3.Roll, 100.f);
	EXPECT_FLOAT_EQ(simdEA3.Pitch, 260.f);
	EXPECT_FLOAT_EQ(simdEA3.Yaw, 280.f);

	// YawFromXY()

	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::YawFromXY(10.f, 20.f), Red::Math::Fpu::RedEulerAngles::YawFromXY(10.f, 20.f));
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::YawFromXY(100.f, -200.f), Red::Math::Fpu::RedEulerAngles::YawFromXY(100.f, -200.f));
	
	// YawToVector4()

	RedMath::SIMD::RedVector4 v1 = RedMath::SIMD::RedEulerAngles::YawToVector4(-100.f);
	Red::Math::Fpu::RedVector4 v2 = Red::Math::Fpu::RedEulerAngles::YawToVector4(-100.f);

	EXPECT_FLOAT_EQ(v1.X, v2.X);
	EXPECT_FLOAT_EQ(v1.Y, v2.Y);
	EXPECT_FLOAT_EQ(v1.Z, v2.Z);
	EXPECT_FLOAT_EQ(v1.W, v2.W);

	v1 = RedMath::SIMD::RedEulerAngles::YawToVector4(1000.f);
	v2 = Red::Math::Fpu::RedEulerAngles::YawToVector4(1000.f);

	EXPECT_FLOAT_EQ(v1.X, v2.X);
	EXPECT_FLOAT_EQ(v1.Y, v2.Y);
	EXPECT_FLOAT_EQ(v1.Z, v2.Z);
	EXPECT_FLOAT_EQ(v1.W, v2.W);

	// YawToVector2()

	RedMath::SIMD::RedVector2 v3 = RedMath::SIMD::RedEulerAngles::YawToVector2(-100.f);
	Red::Math::Fpu::RedVector2 v4 = Red::Math::Fpu::RedEulerAngles::YawToVector2(-100.f);

	EXPECT_FLOAT_EQ(v3.X, v4.X);
	EXPECT_FLOAT_EQ(v3.Y, v4.Y);

	v3 = RedMath::SIMD::RedEulerAngles::YawToVector2(1000.f);
	v4 = Red::Math::Fpu::RedEulerAngles::YawToVector2(1000.f);

	EXPECT_FLOAT_EQ(v3.X, v4.X);
	EXPECT_FLOAT_EQ(v3.Y, v4.Y);

	// AngleDistance()

	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::AngleDistance(100.f, -100.f), Red::Math::Fpu::RedEulerAngles::AngleDistance(100.f, -100.f));
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::AngleDistance(100.f, -100.f), 160.f);
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::AngleDistance(1000.f, 0.f), Red::Math::Fpu::RedEulerAngles::AngleDistance(1000.f, 0.f));
	EXPECT_FLOAT_EQ(RedMath::SIMD::RedEulerAngles::AngleDistance(1000.f, 0.f), 80.f);

	// AngleDistance( const RedEulerAngles& _a, const RedEulerAngles& _b );

	RedMath::SIMD::RedEulerAngles ea1 = RedMath::SIMD::RedEulerAngles::AngleDistance(simdEA, simdEA2);
	Red::Math::Fpu::RedEulerAngles ea2 = Red::Math::Fpu::RedEulerAngles::AngleDistance(fpuEA, fpuEA2);

	TestIdentical(ea1, ea2);

	// InterpolateEulerAngles();

	ea1 = RedMath::SIMD::RedEulerAngles::InterpolateEulerAngles(simdEA, simdEA2, 0.5);
	ea2 = Red::Math::Fpu::RedEulerAngles::InterpolateEulerAngles(fpuEA, fpuEA2, 0.5);

	TestIdentical(ea1, ea2);
}