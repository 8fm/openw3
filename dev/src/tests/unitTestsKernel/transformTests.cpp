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
		};
	};
};

#include "../../common/redMath/redmathbase.h"

#include "../../common/redMath/redvector_fpu.h"
#include "../../common/redMath/redmatrix_fpu.h"
#include "../../common/redMath/redquaternion_fpu.h"
#include "../../common/redMath/redqstransform_fpu.h"
#include "../../common/redMath/redtransform_fpu.h"
#include "../../common/redMath/redeulerangles_fpu.h"

#include "../../common/redMath/redmatrix_fpu.inl"
#include "../../common/redMath/redvector_fpu.inl"
#include "../../common/redMath/redtransform_fpu.inl"
#include "../../common/redMath/redquaternion_fpu.inl"
#include "../../common/redMath/redeulerangles_fpu.inl"
#include "../../common/redMath/redqstransform_fpu.inl"

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix[12] =  {
	0.1f,	0.2f,	0.0f, 0.f,
	1.0f,	5.0f,	4.0f, 0.f,
	2.0f,	3.0f,	6.0f, 0.f,
};

static void TestIdentical(RedMath::SIMD::RedTransform &a, Red::Math::Fpu::RedTransform &b)
{
	RedMath::SIMD::RedVector4 a_trans = a.GetTranslation();
	RedMath::SIMD::RedMatrix3x3 a_rot = a.GetRotation();

	Red::Math::Fpu::RedVector4 b_trans = b.GetTranslation();
	Red::Math::Fpu::RedMatrix3x3 b_rot = b.GetRotation();

	EXPECT_FLOAT_EQ( a_trans.X, b_trans.X );
	EXPECT_FLOAT_EQ( a_trans.Y, b_trans.Y );
	EXPECT_FLOAT_EQ( a_trans.Z, b_trans.Z );
	EXPECT_FLOAT_EQ( a_trans.W, b_trans.W );
	
	EXPECT_FLOAT_EQ( a_rot.Row0.X, b_rot.Matrix[0].X );
	EXPECT_FLOAT_EQ( a_rot.Row0.Y, b_rot.Matrix[0].Y );
	EXPECT_FLOAT_EQ( a_rot.Row0.Z, b_rot.Matrix[0].Z );

	EXPECT_FLOAT_EQ( a_rot.Row1.X, b_rot.Matrix[1].X );
	EXPECT_FLOAT_EQ( a_rot.Row1.Y, b_rot.Matrix[1].Y );
	EXPECT_FLOAT_EQ( a_rot.Row1.Z, b_rot.Matrix[1].Z );

	EXPECT_FLOAT_EQ( a_rot.Row2.X, b_rot.Matrix[2].X );
	EXPECT_FLOAT_EQ( a_rot.Row2.Y, b_rot.Matrix[2].Y );
	EXPECT_FLOAT_EQ( a_rot.Row2.Z, b_rot.Matrix[2].Z );
}


TEST( RedMath, RedTransform_construction )
{
	// RedTransform( const RedMatrix3x3& _r, const RedVector4& _t );

	RedMath::SIMD::RedQuaternion simdRot(3.f, 2.f, -1.f, -2.f);
	RedMath::SIMD::RedVector4 simdTrans(1.f, 2.f, 3.f, 4.f);
	RedMath::SIMD::RedMatrix3x3 simdMat(TestMatrix);

	Red::Math::Fpu::RedQuaternion fpuRot(3.f, 2.f, -1.f, -2.f);
	Red::Math::Fpu::RedVector4 fpuTrans(1.f, 2.f, 3.f, 4.f);
	Red::Math::Fpu::RedMatrix3x3 fpuMat(TestMatrix);
	
	RedMath::SIMD::RedTransform simdTransform(simdMat, simdTrans);
	Red::Math::Fpu::RedTransform fpuTransform(fpuMat, fpuTrans);

	TestIdentical(simdTransform, fpuTransform);

	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().X, 1.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Y, 2.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Z, 3.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().W, 4.f );

	RedMath::SIMD::RedMatrix3x3 rot = simdTransform.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, 0.1f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 0.2f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, 0.0f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 1.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, 5.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 4.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 2.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, 3.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, 6.f );

	// RedTransform( const RedQuaternion& _q, const RedVector4& _v );

	RedMath::SIMD::RedTransform simdTransform2(simdRot, simdTrans);
	Red::Math::Fpu::RedTransform fpuTransform2(fpuRot, fpuTrans);

	TestIdentical(simdTransform2, fpuTransform2);

	EXPECT_FLOAT_EQ( simdTransform2.GetTranslation().X, 1.f );
	EXPECT_FLOAT_EQ( simdTransform2.GetTranslation().Y, 2.f );
	EXPECT_FLOAT_EQ( simdTransform2.GetTranslation().Z, 3.f );
	EXPECT_FLOAT_EQ( simdTransform2.GetTranslation().W, 4.f );

	rot = simdTransform2.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, -9.f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 8.f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, -14.f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 16.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, -19.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 8.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 2.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, -16.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, -25.f );

	// RedTransform( const RedTransform& _r );

	RedMath::SIMD::RedTransform simdTransform3(simdTransform);
	Red::Math::Fpu::RedTransform fpuTransform3(fpuTransform);

	TestIdentical(simdTransform3, fpuTransform3);

	EXPECT_FLOAT_EQ( simdTransform3.GetTranslation().X, 1.f );
	EXPECT_FLOAT_EQ( simdTransform3.GetTranslation().Y, 2.f );
	EXPECT_FLOAT_EQ( simdTransform3.GetTranslation().Z, 3.f );
	EXPECT_FLOAT_EQ( simdTransform3.GetTranslation().W, 4.f );

	rot = simdTransform3.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, 0.1f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 0.2f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, 0.0f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 1.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, 5.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 4.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 2.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, 3.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, 6.f );
}

TEST( RedMath, RedTransform_operators )
{
	// float& operator()
	// const float& operator()

	RedMath::SIMD::RedQuaternion simdRot(3.f, 2.f, -1.f, -2.f);
	RedMath::SIMD::RedVector4 simdTrans(1.f, 2.f, 3.f, 4.f);
	
	Red::Math::Fpu::RedQuaternion fpuRot(3.f, 2.f, -1.f, -2.f);
	Red::Math::Fpu::RedVector4 fpuTrans(1.f, 2.f, 3.f, 4.f);
	
	RedMath::SIMD::RedTransform simdTransform(simdRot, simdTrans);
	Red::Math::Fpu::RedTransform fpuTransform(fpuRot, fpuTrans);

	const RedMath::SIMD::RedTransform simdTransformC(simdRot, simdTrans);
	const Red::Math::Fpu::RedTransform fpuTransformC(fpuRot, fpuTrans);

	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			EXPECT_FLOAT_EQ(simdTransform(i, j), fpuTransform(i, j));
			EXPECT_FLOAT_EQ(simdTransformC(i, j), fpuTransformC(i, j));
		}
	}
}

TEST( RedMath, RedTransform_misc )
{
	RedMath::SIMD::RedQuaternion simdRot(3.f, 2.f, -1.f, -2.f);
	RedMath::SIMD::RedVector4 simdTrans(1.f, 2.f, 3.f, 4.f);

	Red::Math::Fpu::RedQuaternion fpuRot(3.f, 2.f, -1.f, -2.f);
	Red::Math::Fpu::RedVector4 fpuTrans(1.f, 2.f, 3.f, 4.f);

	RedMath::SIMD::RedTransform simdTransform;
	Red::Math::Fpu::RedTransform fpuTransform;

	// Set()

	simdTransform.Set(simdRot, simdTrans);
	fpuTransform.Set(fpuRot, fpuTrans);

	TestIdentical(simdTransform, fpuTransform);

	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().X, 1.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Y, 2.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Z, 3.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().W, 4.f );

	RedMath::SIMD::RedMatrix3x3 rot = simdTransform.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, -9.f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 8.f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, -14.f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 16.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, -19.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 8.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 2.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, -16.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, -25.f );

	// SetIdentity()

	simdTransform.SetIdentity();
	fpuTransform.SetIdentity();

	TestIdentical(simdTransform, fpuTransform);

	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().X, 0.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Y, 0.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Z, 0.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().W, 0.f );

	rot = simdTransform.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, 1.f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 0.f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, 0.f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 0.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, 1.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 0.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 0.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, 0.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, 1.f );

	// IsAlmostEqual()

	RedMath::SIMD::RedQuaternion simdRot2(3.1f, 2.1f, -1.1f, -2.1f);
	RedMath::SIMD::RedVector4 simdTrans2(1.1f, 2.1f, 3.1f, 4.1f);

	Red::Math::Fpu::RedQuaternion fpuRot2(3.1f, 2.1f, -1.1f, -2.1f);
	Red::Math::Fpu::RedVector4 fpuTrans2(1.1f, 2.1f, 3.1f, 4.1f);

	RedMath::SIMD::RedTransform simdTransform1(simdRot, simdTrans);
	Red::Math::Fpu::RedTransform fpuTransform1(fpuRot, fpuTrans);

	RedMath::SIMD::RedTransform simdTransform2(simdRot2, simdTrans2);
	Red::Math::Fpu::RedTransform fpuTransform2(fpuRot2, fpuTrans2);
	
	EXPECT_TRUE(simdTransform1.IsAlmostEqual(simdTransform2, 5.f));
	EXPECT_TRUE(fpuTransform1.IsAlmostEqual(fpuTransform2, 5.f));

	EXPECT_FALSE(simdTransform1.IsAlmostEqual(simdTransform2, 0.05f));
	EXPECT_FALSE(fpuTransform1.IsAlmostEqual(fpuTransform2, 0.05f));

	// SetTranslation()

	simdTransform.SetTranslation(RedMath::SIMD::RedVector4(4.f, 3.f, 2.f, 1.f));
	fpuTransform.SetTranslation(Red::Math::Fpu::RedVector4(4.f, 3.f, 2.f, 1.f));

	TestIdentical(simdTransform, fpuTransform);

	// GetTranslation()
	
	RedMath::SIMD::RedVector4 trans1 = simdTransform.GetTranslation();
	Red::Math::Fpu::RedVector4 trans2 = fpuTransform.GetTranslation();

	EXPECT_FLOAT_EQ( trans1.X, trans2.X );
	EXPECT_FLOAT_EQ( trans1.Y, trans2.Y );
	EXPECT_FLOAT_EQ( trans1.Z, trans2.Z );
	EXPECT_FLOAT_EQ( trans1.W, trans2.W );

	EXPECT_FLOAT_EQ( trans1.X, 4.f );
	EXPECT_FLOAT_EQ( trans1.Y, 3.f );
	EXPECT_FLOAT_EQ( trans1.Z, 2.f );
	EXPECT_FLOAT_EQ( trans1.W, 1.f );

	// const GetTranslation()

	const RedMath::SIMD::RedTransform simdTransformC(simdRot, simdTrans);
	const Red::Math::Fpu::RedTransform fpuTransformC(fpuRot, fpuTrans);

	trans1 = simdTransformC.GetTranslation();
	trans2 = fpuTransformC.GetTranslation();

	EXPECT_FLOAT_EQ( trans1.X, trans2.X );
	EXPECT_FLOAT_EQ( trans1.Y, trans2.Y );
	EXPECT_FLOAT_EQ( trans1.Z, trans2.Z );
	EXPECT_FLOAT_EQ( trans1.W, trans2.W );

	EXPECT_FLOAT_EQ( trans1.X, 1.f );
	EXPECT_FLOAT_EQ( trans1.Y, 2.f );
	EXPECT_FLOAT_EQ( trans1.Z, 3.f );
	EXPECT_FLOAT_EQ( trans1.W, 4.f );

	// SetRotation(const RedMatrix3x3&)

	RedMath::SIMD::RedMatrix3x3 simdMat(TestMatrix);
	Red::Math::Fpu::RedMatrix3x3 fpuMat(TestMatrix);

	simdTransform.SetRotation(simdMat);
	fpuTransform.SetRotation(fpuMat);

	TestIdentical(simdTransform, fpuTransform);

	rot = simdTransform.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, 0.1f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 0.2f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, 0.0f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 1.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, 5.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 4.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 2.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, 3.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, 6.f );

	// SetRotation(const RedQuaternion&)

	simdTransform.SetRotation(simdRot);
	fpuTransform.SetRotation(fpuRot);

	TestIdentical(simdTransform, fpuTransform);

	rot = simdTransform.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, -9.f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 8.f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, -14.f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 16.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, -19.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 8.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 2.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, -16.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, -25.f );
	
	// const GetRotation()

	RedMath::SIMD::RedMatrix3x3 rot1 = simdTransformC.GetRotation();
	Red::Math::Fpu::RedMatrix3x3 rot2 = fpuTransformC.GetRotation();

	EXPECT_FLOAT_EQ( rot1.Row0.X, -9.f );
	EXPECT_FLOAT_EQ( rot1.Row0.Y, 8.f );
	EXPECT_FLOAT_EQ( rot1.Row0.Z, -14.f );
	EXPECT_FLOAT_EQ( rot1.Row1.X, 16.f );
	EXPECT_FLOAT_EQ( rot1.Row1.Y, -19.f );
	EXPECT_FLOAT_EQ( rot1.Row1.Z, 8.f );
	EXPECT_FLOAT_EQ( rot1.Row2.X, 2.f );
	EXPECT_FLOAT_EQ( rot1.Row2.Y, -16.f );
	EXPECT_FLOAT_EQ( rot1.Row2.Z, -25.f );

	EXPECT_FLOAT_EQ( rot2.Matrix[0].X, rot1.Row0.X );
	EXPECT_FLOAT_EQ( rot2.Matrix[0].Y, rot1.Row0.Y );
	EXPECT_FLOAT_EQ( rot2.Matrix[0].Z, rot1.Row0.Z );
	EXPECT_FLOAT_EQ( rot2.Matrix[1].X, rot1.Row1.X );
	EXPECT_FLOAT_EQ( rot2.Matrix[1].Y, rot1.Row1.Y );
	EXPECT_FLOAT_EQ( rot2.Matrix[1].Z, rot1.Row1.Z );
	EXPECT_FLOAT_EQ( rot2.Matrix[2].X, rot1.Row2.X );
	EXPECT_FLOAT_EQ( rot2.Matrix[2].Y, rot1.Row2.Y );
	EXPECT_FLOAT_EQ( rot2.Matrix[2].Z, rot1.Row2.Z );
	
	// SetInverse()

	simdTransform.SetInverse(simdMat);
	fpuTransform.SetInverse(fpuMat);

	TestIdentical(simdTransform, fpuTransform);

	// SetMul( const RedTransform& _a, const RedTransform& _b );

	simdTransform.SetMul(simdTransform1, simdTransform2);
	fpuTransform.SetMul(fpuTransform1, fpuTransform2);

	TestIdentical(simdTransform, fpuTransform);

	// SetMul( const RedQsTransform& _a, const RedTransform& _b );

	RedMath::SIMD::RedVector4 simdScale(4.f, 3.f, 2.f, 1.f);
	Red::Math::Fpu::RedVector4 fpuScale(4.f, 3.f, 2.f, 1.f);

	RedMath::SIMD::RedQsTransform simdQST(simdTrans, simdRot, simdScale);
	Red::Math::Fpu::RedQsTransform fpuQST(fpuTrans, fpuRot, fpuScale);

	simdTransform.SetMul(simdQST, simdTransform1);
	fpuTransform.SetMul(fpuQST, fpuTransform1);

	TestIdentical(simdTransform, fpuTransform);

	// SetMulInverseMul()

	simdTransform.SetMulInverseMul(simdTransform2, simdTransform1);
	fpuTransform.SetMulInverseMul(fpuTransform2, fpuTransform1);

	TestIdentical(simdTransform, fpuTransform);

	// SetMulEq()

	simdTransform.SetMulEq(simdTransform2);
	fpuTransform.SetMulEq(fpuTransform2);

	TestIdentical(simdTransform, fpuTransform);

	// Get4x4ColumnMajor()

	float f1[16], f2[16];
	simdTransform.Get4x4ColumnMajor(f1);
	fpuTransform.Get4x4ColumnMajor(f2);

	EXPECT_TRUE( memcmp(f1, f2, sizeof(float) * 16) == 0 );

	// Set4x4ColumnMajor()

	for (int i = 0; i < 16; ++i)
		f1[i] = 1.f / (i + 1);

	simdTransform.Set4x4ColumnMajor(f1);
	fpuTransform.Set4x4ColumnMajor(f1);

	TestIdentical(simdTransform, fpuTransform);

	// SetRows()

	simdTransform.SetRows(
		RedMath::SIMD::RedVector4(0.f, 1.f, 2.f, 3.f),
		RedMath::SIMD::RedVector4(4.f, 5.f, 6.f, 7.f),
		RedMath::SIMD::RedVector4(8.f, 9.f, 10.f, 11.f),
		RedMath::SIMD::RedVector4(12.f, 13.f, 14.f, 15.f)
		);

	fpuTransform.SetRows(
		Red::Math::Fpu::RedVector4(0.f, 1.f, 2.f, 3.f),
		Red::Math::Fpu::RedVector4(4.f, 5.f, 6.f, 7.f),
		Red::Math::Fpu::RedVector4(8.f, 9.f, 10.f, 11.f),
		Red::Math::Fpu::RedVector4(12.f, 13.f, 14.f, 15.f)
		);

	TestIdentical(simdTransform, fpuTransform);

	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().X, 12.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Y, 13.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().Z, 14.f );
	EXPECT_FLOAT_EQ( simdTransform.GetTranslation().W, 15.f );

	rot = simdTransform.GetRotation();

	EXPECT_FLOAT_EQ( rot.Row0.X, 0.f );
	EXPECT_FLOAT_EQ( rot.Row0.Y, 1.f );
	EXPECT_FLOAT_EQ( rot.Row0.Z, 2.f );

	EXPECT_FLOAT_EQ( rot.Row1.X, 4.f );
	EXPECT_FLOAT_EQ( rot.Row1.Y, 5.f );
	EXPECT_FLOAT_EQ( rot.Row1.Z, 6.f );

	EXPECT_FLOAT_EQ( rot.Row2.X, 8.f );
	EXPECT_FLOAT_EQ( rot.Row2.Y, 9.f );
	EXPECT_FLOAT_EQ( rot.Row2.Z, 10.f );
	
	// IsOk() const;

	EXPECT_TRUE( simdTransform.IsOk() == fpuTransform.IsOk() );
	EXPECT_TRUE( simdTransform1.IsOk() == fpuTransform1.IsOk() );
	EXPECT_TRUE( simdTransform2.IsOk() == fpuTransform2.IsOk() );

	// RedVector4 GetColumn();

	for (int i = 0; i < 4; i++)
	{
		RedMath::SIMD::RedVector4 col1 = simdTransform.GetColumn(i);
		Red::Math::Fpu::RedVector4 col2 = fpuTransform.GetColumn(i);

		EXPECT_FLOAT_EQ( col1.X, col2.X );
		EXPECT_FLOAT_EQ( col1.Y, col2.Y );
		EXPECT_FLOAT_EQ( col1.Z, col2.Z );
		EXPECT_FLOAT_EQ( col1.W, col2.W );
	}
}