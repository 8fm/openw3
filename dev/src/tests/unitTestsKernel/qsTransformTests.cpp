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

#include "../../common/redMath/redqstransform_fpu.inl"
#include "../../common/redMath/redmatrix_fpu.inl"
#include "../../common/redMath/redvector_fpu.inl"
#include "../../common/redMath/redquaternion_fpu.inl"
#include "../../common/redMath/redtransform_fpu.inl"
#include "../../common/redMath/redeulerangles_fpu.inl"

RED_ALIGNED_VAR( static const Red::System::Float, 16 ) TestMatrix[12] =  {
	0.1f,	0.2f,	0.0f, 0.f,
	1.0f,	5.0f,	4.0f, 0.f,
	2.0f,	3.0f,	6.0f, 0.f,
};

static void TestIdentical(RedMath::SIMD::RedQsTransform &a, Red::Math::Fpu::RedQsTransform &b)
{
	EXPECT_FLOAT_EQ( a.Translation.X, b.Translation.X );
	EXPECT_FLOAT_EQ( a.Translation.Y, b.Translation.Y );
	EXPECT_FLOAT_EQ( a.Translation.Z, b.Translation.Z );
	EXPECT_FLOAT_EQ( a.Translation.W, b.Translation.W );

	EXPECT_FLOAT_EQ( a.Rotation.Quat.X, b.Rotation.Quat.X );
	EXPECT_FLOAT_EQ( a.Rotation.Quat.Y, b.Rotation.Quat.Y );
	EXPECT_FLOAT_EQ( a.Rotation.Quat.Z, b.Rotation.Quat.Z );
	EXPECT_FLOAT_EQ( a.Rotation.Quat.W, b.Rotation.Quat.W );

	EXPECT_FLOAT_EQ( a.Scale.X, b.Scale.X );
	EXPECT_FLOAT_EQ( a.Scale.Y, b.Scale.Y );
	EXPECT_FLOAT_EQ( a.Scale.Z, b.Scale.Z );
	EXPECT_FLOAT_EQ( a.Scale.W, b.Scale.W );
}

TEST( RedMath, RedQsTransform_construction )
{
	RedMath::SIMD::RedQuaternion simdRot(3.f, 2.f, -1.f, -2.f);
	RedMath::SIMD::RedVector4 simdTrans(1.f, 2.f, 3.f, 4.f);
	RedMath::SIMD::RedVector4 simdScale(4.f, 3.f, 2.f, 1.f);

	Red::Math::Fpu::RedQuaternion fpuRot(3.f, 2.f, -1.f, -2.f);
	Red::Math::Fpu::RedVector4 fpuTrans(1.f, 2.f, 3.f, 4.f);
	Red::Math::Fpu::RedVector4 fpuScale(4.f, 3.f, 2.f, 1.f);

	RedMath::SIMD::RedQsTransform simdQST(simdTrans, simdRot, simdScale);
	Red::Math::Fpu::RedQsTransform fpuQST(fpuTrans, fpuRot, fpuScale);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 4.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, -1.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, -2.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 4.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 1.f );

	RedMath::SIMD::RedQsTransform simdQST2(simdTrans, simdRot);
	Red::Math::Fpu::RedQsTransform fpuQST2(fpuTrans, fpuRot);

	TestIdentical(simdQST2, fpuQST2);

	EXPECT_FLOAT_EQ( simdQST2.Translation.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST2.Translation.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST2.Translation.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST2.Translation.W, 4.f );
							
	EXPECT_FLOAT_EQ( simdQST2.Rotation.Quat.X, 3.f );
	EXPECT_FLOAT_EQ( simdQST2.Rotation.Quat.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST2.Rotation.Quat.Z, -1.f );
	EXPECT_FLOAT_EQ( simdQST2.Rotation.Quat.W, -2.f );
							
	EXPECT_FLOAT_EQ( simdQST2.Scale.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST2.Scale.Y, 1.f );
	EXPECT_FLOAT_EQ( simdQST2.Scale.Z, 1.f );
	EXPECT_FLOAT_EQ( simdQST2.Scale.W, 1.f );
}

TEST( RedMath, RedQsTransform_misc )
{
	RedMath::SIMD::RedQuaternion simdRot(1.f, 2.f, -3.f, -1.f);
	RedMath::SIMD::RedVector4 simdTrans(1.f, 2.f, 3.f, 4.f);
	RedMath::SIMD::RedVector4 simdScale(4.f, 3.f, 2.f, 1.f);
	RedMath::SIMD::RedMatrix4x4 simdMat(TestMatrix);

	Red::Math::Fpu::RedQuaternion fpuRot(1.f, 2.f, -3.f, -1.f);
	Red::Math::Fpu::RedVector4 fpuTrans(1.f, 2.f, 3.f, 4.f);
	Red::Math::Fpu::RedVector4 fpuScale(4.f, 3.f, 2.f, 1.f);
	Red::Math::Fpu::RedMatrix4x4 fpuMat(TestMatrix);

	RedMath::SIMD::RedQsTransform simdQST;
	Red::Math::Fpu::RedQsTransform fpuQST;

	// Set()

	simdQST.Set(simdTrans, simdRot, simdScale);
	fpuQST.Set(fpuTrans, fpuRot, fpuScale);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 4.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, -3.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, -1.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 4.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 1.f );

	simdQST.Set(simdMat);
	fpuQST.Set(fpuMat);

	TestIdentical(simdQST, fpuQST);

	// SetIdentity();

	simdQST.SetIdentity();
	fpuQST.SetIdentity();

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 0.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, 1.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 1.f );

	// SetZero();

	simdQST.SetZero();
	fpuQST.SetZero();

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 0.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, 0.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 0.f );

	// SetTranslation()

	simdQST.SetTranslation(simdTrans);
	fpuQST.SetTranslation(fpuTrans);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 4.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, 0.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 0.f );

	// SetRotation()

	simdQST.SetRotation(simdRot);
	fpuQST.SetRotation(fpuRot);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 4.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, -3.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, -1.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 0.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 0.f );

	// SetScale()

	simdQST.SetScale(simdScale);
	fpuQST.SetScale(fpuScale);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 4.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, -3.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, -1.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 4.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 1.f );

	// GetTranslation()

	RedMath::SIMD::RedVector4 sVec = simdQST.GetTranslation();
	Red::Math::Fpu::RedVector4 fVec = fpuQST.GetTranslation();

	EXPECT_FLOAT_EQ( sVec.X, fVec.X );
	EXPECT_FLOAT_EQ( sVec.Y, fVec.Y );
	EXPECT_FLOAT_EQ( sVec.Z, fVec.Z );
	EXPECT_FLOAT_EQ( sVec.W, fVec.W );
	EXPECT_FLOAT_EQ( sVec.X, 1.f );
	EXPECT_FLOAT_EQ( sVec.Y, 2.f );
	EXPECT_FLOAT_EQ( sVec.Z, 3.f );
	EXPECT_FLOAT_EQ( sVec.W, 4.f );

	// GetRotation()

	RedMath::SIMD::RedQuaternion sQ = simdQST.GetRotation();
	Red::Math::Fpu::RedQuaternion fQ = fpuQST.GetRotation();

	EXPECT_FLOAT_EQ( sQ.Quat.X, fQ.Quat.X );
	EXPECT_FLOAT_EQ( sQ.Quat.Y, fQ.Quat.Y );
	EXPECT_FLOAT_EQ( sQ.Quat.Z, fQ.Quat.Z );
	EXPECT_FLOAT_EQ( sQ.Quat.W, fQ.Quat.W );
	EXPECT_FLOAT_EQ( sQ.Quat.X, 1.f );
	EXPECT_FLOAT_EQ( sQ.Quat.Y, 2.f );
	EXPECT_FLOAT_EQ( sQ.Quat.Z, -3.f );
	EXPECT_FLOAT_EQ( sQ.Quat.W, -1.f );

	// GetScale()

	sVec = simdQST.GetScale();
	fVec = fpuQST.GetScale();

	EXPECT_FLOAT_EQ( sVec.X, fVec.X );
	EXPECT_FLOAT_EQ( sVec.Y, fVec.Y );
	EXPECT_FLOAT_EQ( sVec.Z, fVec.Z );
	EXPECT_FLOAT_EQ( sVec.W, fVec.W );
	EXPECT_FLOAT_EQ( sVec.X, 4.f );
	EXPECT_FLOAT_EQ( sVec.Y, 3.f );
	EXPECT_FLOAT_EQ( sVec.Z, 2.f );
	EXPECT_FLOAT_EQ( sVec.W, 1.f );

	// Leps()

	RedMath::SIMD::RedQuaternion simdRot1(4.f, 4.f, 4.f, 4.f);
	RedMath::SIMD::RedVector4 simdTrans1(6.f, 6.f, 6.f, 6.f);
	RedMath::SIMD::RedVector4 simdScale1(8.f, 8.f, 8.f, 8.f);

	RedMath::SIMD::RedQuaternion simdRot2(3.f, 3.f, 3.f, 3.f);
	RedMath::SIMD::RedVector4 simdTrans2(5.f, 5.f, 5.f, 5.f);
	RedMath::SIMD::RedVector4 simdScale2(7.f, 7.f, 7.f, 7.f);
	
	Red::Math::Fpu::RedQuaternion fpuRot1(4.f, 4.f, 4.f, 4.f);
	Red::Math::Fpu::RedVector4 fpuTrans1(6.f, 6.f, 6.f, 6.f);
	Red::Math::Fpu::RedVector4 fpuScale1(8.f, 8.f, 8.f, 8.f);
	
	Red::Math::Fpu::RedQuaternion fpuRot2(3.f, 3.f, 3.f, 3.f);
	Red::Math::Fpu::RedVector4 fpuTrans2(5.f, 5.f, 5.f, 5.f);
	Red::Math::Fpu::RedVector4 fpuScale2(7.f, 7.f, 7.f, 7.f);

	RedMath::SIMD::RedQsTransform simdQST1(simdTrans1, simdRot1, simdScale1);
	RedMath::SIMD::RedQsTransform simdQST2(simdTrans2, simdRot2, simdScale2);

	Red::Math::Fpu::RedQsTransform fpuQST1(fpuTrans1, fpuRot1, fpuScale1);
	Red::Math::Fpu::RedQsTransform fpuQST2(fpuTrans2, fpuRot2, fpuScale2);

	simdQST.Lerp(simdQST1, simdQST2, 0.5f);
	fpuQST.Lerp(fpuQST1, fpuQST2, 0.5f);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 5.5f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 5.5f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 5.5f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 5.5f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 0.5f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 0.5f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, 0.5f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, 0.5f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 7.5f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 7.5f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 7.5f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 7.5f );

	// Slerp()

	simdQST.Slerp(simdQST1, simdQST2, 0.5f);
	fpuQST.Slerp(fpuQST1, fpuQST2, 0.5f);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 5.5f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 5.5f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 5.5f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 5.5f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, 0.5f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, 0.5f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, 0.5f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, 0.5f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 7.5f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 7.5f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 7.5f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 7.5f );

	// ConvertToMatrix()

	simdQST.ConvertToMatrix();
	fpuQST.ConvertToMatrix();

	TestIdentical(simdQST, fpuQST);

	// ConvertToMatrixNormalized()

	simdQST.ConvertToMatrixNormalized();
	fpuQST.ConvertToMatrixNormalized();

	TestIdentical(simdQST, fpuQST);

	// SetFromTransformNoScale()

	RedMath::SIMD::RedTransform simdT(simdRot, simdTrans);
	Red::Math::Fpu::RedTransform fpuT(fpuRot, fpuTrans);

	simdQST.SetFromTransformNoScale(simdT);
	fpuQST.SetFromTransformNoScale(fpuT);

	TestIdentical(simdQST, fpuQST);

	EXPECT_FLOAT_EQ( simdQST.Translation.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Y, 2.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Translation.W, 4.f );

	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.X, -1.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Y, -2.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.Z, 3.f );
	EXPECT_FLOAT_EQ( simdQST.Rotation.Quat.W, 1.f );

	EXPECT_FLOAT_EQ( simdQST.Scale.X, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Y, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.Z, 1.f );
	EXPECT_FLOAT_EQ( simdQST.Scale.W, 1.f );

	// CopyToTransformNoScale()

	RedMath::SIMD::RedTransform simdT2;
	simdQST.CopyToTransformNoScale(simdT2);

	Red::Math::Fpu::RedTransform fpuT2;
	fpuQST.CopyToTransformNoScale(fpuT2);

	EXPECT_FLOAT_EQ( simdT2.GetTranslation().X, fpuT2.GetTranslation().X );
	EXPECT_FLOAT_EQ( simdT2.GetTranslation().Y, fpuT2.GetTranslation().Y );
	EXPECT_FLOAT_EQ( simdT2.GetTranslation().Z, fpuT2.GetTranslation().Z );
	EXPECT_FLOAT_EQ( simdT2.GetTranslation().W, fpuT2.GetTranslation().W );

	EXPECT_FLOAT_EQ( simdT2.GetColumn(0).X, fpuT2.GetColumn(0).X );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(0).Y, fpuT2.GetColumn(0).Y );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(0).Z, fpuT2.GetColumn(0).Z );

	EXPECT_FLOAT_EQ( simdT2.GetColumn(1).X, fpuT2.GetColumn(1).X );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(1).Y, fpuT2.GetColumn(1).Y );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(1).Z, fpuT2.GetColumn(1).Z );

	EXPECT_FLOAT_EQ( simdT2.GetColumn(2).X, fpuT2.GetColumn(2).X );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(2).Y, fpuT2.GetColumn(2).Y );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(2).Z, fpuT2.GetColumn(2).Z );

	// SetFromTransform()

	simdQST2.SetFromTransform(simdT2);
	fpuQST2.SetFromTransform(fpuT2);

	TestIdentical(simdQST2, fpuQST2);

	// CopyToTransform()

	simdQST.CopyToTransform(simdT2);
	fpuQST.CopyToTransform(fpuT2);

	EXPECT_FLOAT_EQ( simdT2.GetTranslation().X, fpuT2.GetTranslation().X );
	EXPECT_FLOAT_EQ( simdT2.GetTranslation().Y, fpuT2.GetTranslation().Y );
	EXPECT_FLOAT_EQ( simdT2.GetTranslation().Z, fpuT2.GetTranslation().Z );
	EXPECT_FLOAT_EQ( simdT2.GetTranslation().W, fpuT2.GetTranslation().W );

	EXPECT_FLOAT_EQ( simdT2.GetColumn(0).X, fpuT2.GetColumn(0).X );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(0).Y, fpuT2.GetColumn(0).Y );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(0).Z, fpuT2.GetColumn(0).Z );

	EXPECT_FLOAT_EQ( simdT2.GetColumn(1).X, fpuT2.GetColumn(1).X );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(1).Y, fpuT2.GetColumn(1).Y );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(1).Z, fpuT2.GetColumn(1).Z );

	EXPECT_FLOAT_EQ( simdT2.GetColumn(2).X, fpuT2.GetColumn(2).X );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(2).Y, fpuT2.GetColumn(2).Y );
	EXPECT_FLOAT_EQ( simdT2.GetColumn(2).Z, fpuT2.GetColumn(2).Z );

	// SetInverse()

	simdQST.SetInverse(simdQST2);
	fpuQST.SetInverse(fpuQST2);

	TestIdentical(simdQST, fpuQST);

	// SetMul()

	simdQST.SetMul(simdQST1, simdQST2);
	fpuQST.SetMul(fpuQST1, fpuQST2);

	TestIdentical(simdQST, fpuQST);

	// SetMulInverseMul()

	simdQST2.SetMulInverseMul(simdQST1, simdQST);
	fpuQST2.SetMulInverseMul(fpuQST1, fpuQST);

	TestIdentical(simdQST2, fpuQST2);

	// SetMulMulInverse()

	simdQST1.SetMulMulInverse(simdQST2, simdQST);
	fpuQST1.SetMulMulInverse(fpuQST2, fpuQST);

	TestIdentical(simdQST1, fpuQST1);

	// SetMulEq()

	simdQST.SetMulEq(simdQST1);
	fpuQST.SetMulEq(fpuQST1);

	TestIdentical(simdQST, fpuQST);

	// Set4x4ColumnMajor()

	simdQST.Set4x4ColumnMajor(TestMatrix);
	fpuQST.Set4x4ColumnMajor(TestMatrix);
	
	TestIdentical(simdQST, fpuQST);

	// Get4x4ColumnMajor()

	 Red::System::Float floats1[16];
	 Red::System::Float floats2[16];

	 simdQST.Get4x4ColumnMajor(floats1);
	 fpuQST.Get4x4ColumnMajor(floats2);

	EXPECT_TRUE( memcmp(floats1, floats2, sizeof(float) * 16) == 0 );

	// IsOk()

	EXPECT_TRUE( simdQST.IsOk() == fpuQST.IsOk() );
	EXPECT_TRUE( simdQST1.IsOk() == fpuQST1.IsOk() );
	EXPECT_TRUE( simdQST2.IsOk() == fpuQST2.IsOk() );
	
	// IsAlmostEqual()

	EXPECT_TRUE( simdQST.IsAlmostEqual(simdQST) );
	EXPECT_TRUE( simdQST1.IsAlmostEqual(simdQST1) );
	EXPECT_TRUE( simdQST2.IsAlmostEqual(simdQST2) );

	EXPECT_TRUE( fpuQST.IsAlmostEqual( fpuQST) );
	EXPECT_TRUE( fpuQST1.IsAlmostEqual(fpuQST1) );
	EXPECT_TRUE( fpuQST2.IsAlmostEqual(fpuQST2) );

	EXPECT_FALSE( simdQST.IsAlmostEqual(simdQST1) );
	EXPECT_FALSE( fpuQST.IsAlmostEqual(fpuQST1) );


	// BlendAddMul()

	simdQST.BlendAddMul(simdQST1, 0.5f);
	fpuQST.BlendAddMul(fpuQST1, 0.5f);

	TestIdentical(simdQST, fpuQST);

	// BlendNormalize()

	simdQST.BlendNormalize(0.2f);
	fpuQST.BlendNormalize(0.2f);

	TestIdentical(simdQST, fpuQST);

	// FastRenormalize()

	simdQST.FastRenormalize();
	fpuQST.FastRenormalize();

	TestIdentical(simdQST, fpuQST);


	// FastRenormalizeBatch()

	const int array_size = 10;

	RedMath::SIMD::RedQsTransform simdTArray[array_size];
	Red::Math::Fpu::RedQsTransform fpuTArray[array_size];
	float weight1[array_size], weight2[array_size];

	for (int i = 0; i < array_size; ++i)
	{
		float data[16];
		for (int j = 0; j < 16; ++j)
			data[j] = 1.f / (i + 1);

		simdTArray[i].Set(RedMath::SIMD::RedMatrix4x4(data));
		fpuTArray[i].Set(Red::Math::Fpu::RedMatrix4x4(data));
		weight1[i] = weight2[i] = 1.f / (i + 1);
	}

	RedMath::SIMD::RedQsTransform::FastRenormalizeBatch(simdTArray, weight1, array_size);
	Red::Math::Fpu::RedQsTransform::FastRenormalizeBatch(fpuTArray, weight2, array_size);

	EXPECT_TRUE( memcmp(simdTArray, fpuTArray, sizeof(RedMath::SIMD::RedQsTransform) * array_size) == 0 );
}