#include "build.h"
#include "../../common/redMath/redVector2_float.h"

#include "../../common/redMath/redVector2_simd.h"
#include "../../common/redMath/redscalar_simd.inl"
#include "../../common/redMath/redvector2_simd.inl"

const Red::System::Float constVals[6] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

//////////////////////////////////////////////////////////////////////////
// float
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedVector2_float_construction )
{
	RedMath::FLOAT::RedVector2 noParams;
	EXPECT_FLOAT_EQ( noParams.X, noParams.Z );
	EXPECT_FLOAT_EQ( noParams.Y, noParams.W );

	EXPECT_FLOAT_EQ( noParams.X, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.W, constVals[0] );

	RedMath::FLOAT::RedVector2 singleFloats( constVals[1], constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.X, singleFloats.Z );
	EXPECT_FLOAT_EQ( singleFloats.Y, singleFloats.W );

	EXPECT_FLOAT_EQ( singleFloats.X, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloats.Y, constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( singleFloats.W, constVals[2] );

	RedMath::FLOAT::RedScalar scalarSet( constVals[3] );
	RedMath::FLOAT::RedVector2 fromScalar( scalarSet );
	EXPECT_FLOAT_EQ( fromScalar.X, fromScalar.Z );
	EXPECT_FLOAT_EQ( fromScalar.Y, fromScalar.W );

	EXPECT_FLOAT_EQ( fromScalar.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Y, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( fromScalar.W, constVals[3] );

	RedMath::FLOAT::RedVector2 vectorSet( constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector2 fromVector2( vectorSet );
	EXPECT_FLOAT_EQ( fromVector2.X, fromVector2.Z );
	EXPECT_FLOAT_EQ( fromVector2.Y, fromVector2.W );

	EXPECT_FLOAT_EQ( fromVector2.X, constVals[4] );
	EXPECT_FLOAT_EQ( fromVector2.Y, constVals[5] );
	EXPECT_FLOAT_EQ( fromVector2.Z, constVals[4] ); 
	EXPECT_FLOAT_EQ( fromVector2.W, constVals[5] );

	Red::System::Float fltPtrs[2] = { 4.0f, 5.0f };
	RedMath::FLOAT::RedVector2 fromFlt( fltPtrs );
	EXPECT_FLOAT_EQ( fromFlt.X, fromFlt.Z );
	EXPECT_FLOAT_EQ( fromFlt.Y, fromFlt.W );

	EXPECT_FLOAT_EQ( fromFlt.X, fltPtrs[0] );
	EXPECT_FLOAT_EQ( fromFlt.Y, fltPtrs[1] );
	EXPECT_FLOAT_EQ( fromFlt.Z, fltPtrs[0] ); 
	EXPECT_FLOAT_EQ( fromFlt.W, fltPtrs[1] );
}

TEST( RedMath, RedVector2_float_Operator_Assignment )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[1] );
	RedMath::FLOAT::RedVector2 result = scalarVal;
	EXPECT_FLOAT_EQ( result.X, scalarVal.X );
	EXPECT_FLOAT_EQ( result.Y, scalarVal.Y );
	EXPECT_FLOAT_EQ( result.Z, scalarVal.Z );
	EXPECT_FLOAT_EQ( result.W, scalarVal.W );

	result = RedMath::FLOAT::RedVector2( constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( result.X, constVals[2] );
	EXPECT_FLOAT_EQ( result.Y, constVals[3] );
	EXPECT_FLOAT_EQ( result.Z, constVals[2] );
	EXPECT_FLOAT_EQ( result.W, constVals[3] );
}

TEST( RedMath, RedVector2_float_AsFloat )
{
	RedMath::FLOAT::RedVector2 vectorVal( constVals[3], constVals[4] );
	const Red::System::Float* fltPtr = vectorVal.AsFloat();

	EXPECT_FLOAT_EQ( fltPtr[0], vectorVal.X ); 
	EXPECT_FLOAT_EQ( fltPtr[1], vectorVal.Y ); 
	EXPECT_FLOAT_EQ( fltPtr[2], vectorVal.Z ); 
	EXPECT_FLOAT_EQ( fltPtr[3], vectorVal.W );
}

TEST( RedMath, RedVector2_float_Set_Functions )
{
	RedMath::FLOAT::RedVector2 vectorVal;
	const RedMath::FLOAT::RedVector2 copySetVectorVal( constVals[2], constVals[3] );
	const RedMath::FLOAT::RedScalar copySetScalarVal( constVals[5] );
	Red::System::Float fltPtr[2] = { 4.0f, 5.0f };

	vectorVal.Set( constVals[1], constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[2] );

	vectorVal.Set( copySetVectorVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[3] );

	vectorVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[5] );

	vectorVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( vectorVal.X, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, fltPtr[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.W, fltPtr[1] );

	vectorVal.SetZeros();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[0] );

	vectorVal.SetOnes();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );
}

TEST( RedMath, RedVector2_float_Negate_Functions )
{
	RedMath::FLOAT::RedVector2 vectorVal( constVals[1], constVals[2] );
	RedMath::FLOAT::RedVector2 vectorValInv( -constVals[1], -constVals[2] );
	RedMath::FLOAT::RedVector2 vectorValNegate( vectorVal );
	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, -constVals[2] );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[2] );

	RedMath::FLOAT::RedVector2 negatedVectorVal( vectorVal.Negated() );
	EXPECT_FLOAT_EQ( negatedVectorVal.X, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Z, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.W, -constVals[2] );

	RedMath::FLOAT::RedVector2 NegatedVectorValInv( vectorValInv.Negated() );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.X, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Y, constVals[2] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Z, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.W, constVals[2] );
}

TEST( RedMath, RedVector2_float_Length )
{
	RedMath::FLOAT::RedVector2 vectorVal( constVals[4], constVals[5] );
	RedMath::FLOAT::RedScalar length = vectorVal.Length();
	Red::System::Float estimatedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	EXPECT_FLOAT_EQ( length.X, estimatedLength );
}

TEST( RedMath, RedVector2_float_SquareLength )
{
	RedMath::FLOAT::RedVector2 vectorVal( constVals[4], constVals[5] );
	RedMath::FLOAT::RedScalar length = vectorVal.SquareLength();
	Red::System::Float expectedLength = Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );
	EXPECT_FLOAT_EQ( length.X, expectedLength );
}

TEST( RedMath, RedVector2_float_Normalize_Functions)
{
	RedMath::FLOAT::RedVector2 vectorValNormalized( constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector2 normalizeVectorVal( constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector2 normalizeFastVectorVal( constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector2 zeroVec( constVals[0], constVals[0] );
	RedMath::FLOAT::RedVector2 zeroVecFast( constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[4] * unitVal;
	Red::System::Float yVal = constVals[5] * unitVal;

	normalizeVectorVal.Normalize();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, yVal );
	
	RedMath::FLOAT::RedVector2 normal = vectorValNormalized.Normalized();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, xVal );
	EXPECT_FLOAT_EQ( normal.W, yVal );

	normal.SetZeros();

	normal = zeroVec.Normalized();
	EXPECT_FLOAT_EQ( normal.X, 0.0f );
	EXPECT_FLOAT_EQ( normal.Y, 0.0f );
	EXPECT_FLOAT_EQ( normal.Z, 0.0f );
	EXPECT_FLOAT_EQ( normal.W, 0.0f );

	zeroVec.Normalize();
	EXPECT_FLOAT_EQ( zeroVec.X, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Y, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Z, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.W, 0.0f );
}

TEST( RedMath, RedVector2_float_AlmostEqual )
{
	const Red::System::Float eps = 0.00001f;
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = eps + adj;
	RedMath::FLOAT::RedVector2 vA( constVals[2], constVals[3] );
	RedMath::FLOAT::RedVector2 vB( constVals[2] + adj, constVals[3] + adj );
	RedMath::FLOAT::RedVector2 vC( constVals[2] + adj2, constVals[3] + adj2 );

	EXPECT_TRUE( vA.IsAlmostEqual( vB, eps ) );
	EXPECT_FALSE( vA.IsAlmostEqual( vC, eps ) );
}

TEST( RedMath, RedVector2_float_IsZero_or_AlmostZero )
{
	const Red::System::Float eps = 0.00001f;
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = eps + adj;
	RedMath::FLOAT::RedVector2 zero;
	RedMath::FLOAT::RedVector2 epsZero( eps, eps );
	RedMath::FLOAT::RedVector2 notZero( adj2, adj2 );

	EXPECT_TRUE( zero.IsZero() );
	EXPECT_FALSE( epsZero.IsZero() );
	EXPECT_FALSE( notZero.IsZero() );

	EXPECT_TRUE( zero.IsAlmostZero( eps ) );
	EXPECT_TRUE( epsZero.IsAlmostZero( eps ) );
	EXPECT_FALSE( notZero.IsAlmostZero( eps ) );
}

//////////////////////////////////////////////////////////////////////////
// simd
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedVector2_simd_construction )
{
	RedMath::SIMD::RedVector2 noParams;
	EXPECT_FLOAT_EQ( noParams.X, noParams.Z );
	EXPECT_FLOAT_EQ( noParams.Y, noParams.W );

	EXPECT_FLOAT_EQ( noParams.X, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] ); 
	EXPECT_FLOAT_EQ( noParams.W, constVals[0] );

	RedMath::SIMD::RedVector2 singleFloats( constVals[1], constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.X, singleFloats.Z );
	EXPECT_FLOAT_EQ( singleFloats.Y, singleFloats.W );

	EXPECT_FLOAT_EQ( singleFloats.X, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloats.Y, constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( singleFloats.W, constVals[2] );

	RedMath::SIMD::RedScalar scalarSet( constVals[3] );
	RedMath::SIMD::RedVector2 fromScalar( scalarSet );
	EXPECT_FLOAT_EQ( fromScalar.X, fromScalar.Z );
	EXPECT_FLOAT_EQ( fromScalar.Y, fromScalar.W );

	EXPECT_FLOAT_EQ( fromScalar.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Y, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( fromScalar.W, constVals[3] );

	RedMath::SIMD::RedVector2 vectorSet( constVals[4], constVals[5] );
	RedMath::SIMD::RedVector2 fromVector2( vectorSet );
	EXPECT_FLOAT_EQ( fromVector2.X, fromVector2.Z );
	EXPECT_FLOAT_EQ( fromVector2.Y, fromVector2.W );

	EXPECT_FLOAT_EQ( fromVector2.X, constVals[4] );
	EXPECT_FLOAT_EQ( fromVector2.Y, constVals[5] );
	EXPECT_FLOAT_EQ( fromVector2.Z, constVals[4] ); 
	EXPECT_FLOAT_EQ( fromVector2.W, constVals[5] );

	RED_ALIGNED_VAR( Red::System::Float, 16 ) fltPtrs[4] = { 4.0f, 5.0f, 4.0f, 5.0f };
	RedMath::SIMD::RedVector2 fromFlt( fltPtrs );
	EXPECT_FLOAT_EQ( fromFlt.X, fromFlt.Z );
	EXPECT_FLOAT_EQ( fromFlt.Y, fromFlt.W );

	EXPECT_FLOAT_EQ( fromFlt.X, fltPtrs[0] );
	EXPECT_FLOAT_EQ( fromFlt.Y, fltPtrs[1] );
	EXPECT_FLOAT_EQ( fromFlt.Z, fltPtrs[2] ); 
	EXPECT_FLOAT_EQ( fromFlt.W, fltPtrs[3] );
}

TEST( RedMath, RedVector2_simd_Operator_Assignment )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[1] );
	RedMath::SIMD::RedVector2 result = scalarVal;
	EXPECT_FLOAT_EQ( result.X, scalarVal.X );
	EXPECT_FLOAT_EQ( result.Y, scalarVal.Y );
	EXPECT_FLOAT_EQ( result.Z, scalarVal.Z );
	EXPECT_FLOAT_EQ( result.W, scalarVal.W );

	result = RedMath::SIMD::RedVector2( constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( result.X, constVals[2] );
	EXPECT_FLOAT_EQ( result.Y, constVals[3] );
	EXPECT_FLOAT_EQ( result.Z, constVals[2] );
	EXPECT_FLOAT_EQ( result.W, constVals[3] );
}

TEST( RedMath, RedVector2_simd_AsFloat )
{
	RedMath::SIMD::RedVector2 vectorVal( constVals[3], constVals[4] );
	const Red::System::Float* fltPtr = vectorVal.AsFloat();

	EXPECT_FLOAT_EQ( fltPtr[0], vectorVal.X ); 
	EXPECT_FLOAT_EQ( fltPtr[1], vectorVal.Y ); 
	EXPECT_FLOAT_EQ( fltPtr[2], vectorVal.Z ); 
	EXPECT_FLOAT_EQ( fltPtr[3], vectorVal.W );
}


TEST( RedMath, RedVector2_simd_Set_Functions )
{
	RedMath::SIMD::RedVector2 vectorVal;
	const RedMath::SIMD::RedVector2 copySetVectorVal( constVals[2], constVals[3] );
	const RedMath::SIMD::RedScalar copySetScalarVal( constVals[5] );
	RED_ALIGNED_VAR( Red::System::Float, 16) fltPtr[4] = { 4.0f, 5.0f, 4.0f, 5.0f };

	vectorVal.Set( constVals[1], constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[2] );

	vectorVal.Set( copySetVectorVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[3] );

	vectorVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[5] );

	vectorVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( vectorVal.X, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, fltPtr[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.W, fltPtr[1] );

	vectorVal.SetZeros();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[0] );

	vectorVal.SetOnes();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );
}

TEST( RedMath, RedVector2_simd_Negate_Functions )
{
	RedMath::SIMD::RedVector2 vectorVal( constVals[1], constVals[2] );
	RedMath::SIMD::RedVector2 vectorValInv( -constVals[1], -constVals[2] );
	RedMath::SIMD::RedVector2 vectorValNegate( vectorVal );
	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, -constVals[2] );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[2] );

	RedMath::SIMD::RedVector2 negatedVectorVal( vectorVal.Negated() );
	EXPECT_FLOAT_EQ( negatedVectorVal.X, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Z, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.W, -constVals[2] );

	RedMath::SIMD::RedVector2 NegatedVectorValInv( vectorValInv.Negated() );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.X, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Y, constVals[2] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Z, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.W, constVals[2] );
}

TEST( RedMath, RedVector2_simd_Length )
{
	RedMath::SIMD::RedVector2 vectorVal( constVals[4], constVals[5] );
	RedMath::SIMD::RedScalar length = vectorVal.Length();
	Red::System::Float estimatedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	EXPECT_FLOAT_EQ( length.X, estimatedLength );
}

TEST( RedMath, RedVector2_simd_SquareLength )
{
	RedMath::SIMD::RedVector2 vectorVal( constVals[4], constVals[5] );
	RedMath::SIMD::RedScalar length = vectorVal.SquareLength();
	Red::System::Float expectedLength = Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );
	EXPECT_FLOAT_EQ( length.X, expectedLength );
}

TEST( RedMath, RedVector2_simd_Normalize_Functions)
{
	RedMath::SIMD::RedVector2 vectorValNormalized( constVals[4], constVals[5] );
	RedMath::SIMD::RedVector2 normalizeVectorVal( constVals[4], constVals[5] );
	RedMath::SIMD::RedVector2 normalizeFastVectorVal( constVals[4], constVals[5] );
	RedMath::SIMD::RedVector2 zeroVec( constVals[0], constVals[0] );
	RedMath::SIMD::RedVector2 zeroVecFast( constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[4] * unitVal;
	Red::System::Float yVal = constVals[5] * unitVal;

	normalizeVectorVal.Normalize();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, yVal );

	RedMath::SIMD::RedVector2 normal = vectorValNormalized.Normalized();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, xVal );
	EXPECT_FLOAT_EQ( normal.W, yVal );

	
	//////////////////////////////////////////////////////////////////////////
	// NOTE:
	// Accuracy between instructions is a problem - hence this nasty intrinsic
	// here for testing. As we'll get different results from different methods
	//////////////////////////////////////////////////////////////////////////
	Red::System::Float sqr = Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );

	RED_ALIGNED_VAR( Red::System::Float, 16 ) expectedApprox = 0.0f;
	_mm_store_ss( &expectedApprox, _mm_rsqrt_ss( _mm_set1_ps( sqr ) ) );
	xVal = constVals[4] * expectedApprox;
	yVal = constVals[5] * expectedApprox;
	normalizeFastVectorVal.NormalizeFast();
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Z, xVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.W, yVal );

	normal.SetZeros();

	normal = vectorValNormalized.NormalizedFast();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, xVal );
	EXPECT_FLOAT_EQ( normal.W, yVal );

	normal.SetZeros();
	normal = zeroVec.Normalized();
	EXPECT_FLOAT_EQ( normal.X, 0.0f );
	EXPECT_FLOAT_EQ( normal.Y, 0.0f );
	EXPECT_FLOAT_EQ( normal.Z, 0.0f );
	EXPECT_FLOAT_EQ( normal.W, 0.0f );

	zeroVec.Normalize();
	EXPECT_FLOAT_EQ( zeroVec.X, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Y, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Z, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.W, 0.0f );

	normal.SetZeros();
	normal = zeroVecFast.NormalizedFast();
	EXPECT_FLOAT_EQ( normal.X, 0.0f );
	EXPECT_FLOAT_EQ( normal.Y, 0.0f );
	EXPECT_FLOAT_EQ( normal.Z, 0.0f );
	EXPECT_FLOAT_EQ( normal.W, 0.0f );

	zeroVecFast.NormalizeFast();
	EXPECT_FLOAT_EQ( zeroVecFast.X, 0.0f );
	EXPECT_FLOAT_EQ( zeroVecFast.Y, 0.0f );
	EXPECT_FLOAT_EQ( zeroVecFast.Z, 0.0f );
	EXPECT_FLOAT_EQ( zeroVecFast.W, 0.0f );
}

TEST( RedMath, RedVector2_simd_AlmostEqual )
{
	const SIMDVector eps = _mm_set1_ps( 0.00001f );
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = 0.00001f + adj;
	RedMath::SIMD::RedVector2 vA( constVals[2], constVals[3] );
	RedMath::SIMD::RedVector2 vB( constVals[2] + adj, constVals[3] + adj );
	RedMath::SIMD::RedVector2 vC( constVals[2] + adj2, constVals[3] + adj2 );

	EXPECT_TRUE( vA.IsAlmostEqual( vB, eps ) );
	EXPECT_FALSE( vA.IsAlmostEqual( vC, eps ) );
}

TEST( RedMath, RedVector2_simd_IsZero_or_AlmostZero )
{
	const SIMDVector eps = _mm_set1_ps( 0.00001f );
	const Red::System::Float adj = 0.0000001f;
	const Red::System::Float adj2 = 0.00001f + adj;
	RedMath::SIMD::RedVector2 zero;
	RedMath::SIMD::RedVector2 epsZero( eps );
	RedMath::SIMD::RedVector2 notZero( adj2, adj2 );

	EXPECT_TRUE( zero.IsZero() );
	EXPECT_FALSE( epsZero.IsZero() );
	EXPECT_FALSE( notZero.IsZero() );

	EXPECT_TRUE( zero.IsAlmostZero( eps ) );
	EXPECT_TRUE( epsZero.IsAlmostZero( eps ) );
	EXPECT_FALSE( notZero.IsAlmostZero( eps ) );
}
