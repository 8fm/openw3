#include "build.h"
#include "../../common/redMath/redVector3_float.h"

#include "../../common/redMath/redVector3_simd.h"
#include "../../common/redMath/redscalar_simd.inl"
#include "../../common/redMath/redvector3_simd.inl"

const Red::System::Float constVals[6] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
//////////////////////////////////////////////////////////////////////////
// float
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedVector3_float_construction )
{
	RedMath::FLOAT::RedVector3 noParams;
	EXPECT_FLOAT_EQ( noParams.X, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.W, constVals[1] );

	RedMath::FLOAT::RedVector3 singleFloats( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( singleFloats.X, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloats.Y, constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( singleFloats.W, constVals[1] );

	RedMath::FLOAT::RedScalar scalarSet( constVals[3] );
	RedMath::FLOAT::RedVector3 fromScalar( scalarSet );
	EXPECT_FLOAT_EQ( fromScalar.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Y, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( fromScalar.W, constVals[1] );

	RedMath::FLOAT::RedVector3 vectorSet( constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector3 fromVector( vectorSet );
	EXPECT_FLOAT_EQ( fromVector.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVector.Y, constVals[4] );
	EXPECT_FLOAT_EQ( fromVector.Z, constVals[5] ); 
	EXPECT_FLOAT_EQ( fromVector.W, constVals[1] );

	Red::System::Float fltPtrs[3] = { 4.0f, 5.0f, 6.0f };
	RedMath::FLOAT::RedVector3 fromFlt( fltPtrs );
	EXPECT_FLOAT_EQ( fromFlt.X, fltPtrs[0] );
	EXPECT_FLOAT_EQ( fromFlt.Y, fltPtrs[1] );
	EXPECT_FLOAT_EQ( fromFlt.Z, fltPtrs[2] ); 
	EXPECT_FLOAT_EQ( fromFlt.W, constVals[1] );
}

TEST( RedMath, RedVector3_float_Operator_Assignment )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[1] );
	RedMath::FLOAT::RedVector3 result = scalarVal;
	EXPECT_FLOAT_EQ( result.X, scalarVal.X );
	EXPECT_FLOAT_EQ( result.Y, scalarVal.Y );
	EXPECT_FLOAT_EQ( result.Z, scalarVal.Z );
	EXPECT_FLOAT_EQ( result.W, constVals[1] );

	result = RedMath::FLOAT::RedVector3( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( result.X, constVals[1] );
	EXPECT_FLOAT_EQ( result.Y, constVals[2] );
	EXPECT_FLOAT_EQ( result.Z, constVals[3] );
	EXPECT_FLOAT_EQ( result.W, constVals[1] );
}

TEST( RedMath, RedVector3_float_AsFloat )
{
	RedMath::FLOAT::RedVector3 vectorVal( constVals[2], constVals[3], constVals[4] );
	const Red::System::Float* fltPtr = vectorVal.AsFloat();

	EXPECT_FLOAT_EQ( fltPtr[0], vectorVal.X ); 
	EXPECT_FLOAT_EQ( fltPtr[1], vectorVal.Y ); 
	EXPECT_FLOAT_EQ( fltPtr[2], vectorVal.Z ); 
	EXPECT_FLOAT_EQ( fltPtr[3], vectorVal.W );
}

TEST( RedMath, RedVector3_float_Set_Functions )
{
	RedMath::FLOAT::RedVector3 vectorVal;
	const RedMath::FLOAT::RedVector3 copySetVectorVal( constVals[1], constVals[2], constVals[3] );
	const RedMath::FLOAT::RedScalar copySetScalarVal( constVals[5] );
	Red::System::Float fltPtr[3] = { 4.0f, 5.0f, 6.0f };

	vectorVal.Set( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.Set( copySetVectorVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( vectorVal.X, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, fltPtr[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, fltPtr[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.SetZeros();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.SetOnes();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );
}

TEST( RedMath, RedVector3_float_Negate_Functions )
{
	RedMath::FLOAT::RedVector3 vectorVal( constVals[1], constVals[2], constVals[3] );
	RedMath::FLOAT::RedVector3 vectorValInv( -constVals[1], -constVals[2], -constVals[3] );
	RedMath::FLOAT::RedVector3 vectorValNegate( vectorVal );
	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[1] );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[1] );

	RedMath::FLOAT::RedVector3 negatedVectorVal( vectorVal.Negated() );
	EXPECT_FLOAT_EQ( negatedVectorVal.X, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( negatedVectorVal.W, constVals[1] );

	RedMath::FLOAT::RedVector3 NegatedVectorValInv( vectorValInv.Negated() );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.X, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Y, constVals[2] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Z, constVals[3] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.W, constVals[1] );
}

TEST( RedMath, RedVector3_float_Length )
{
	RedMath::FLOAT::RedVector3 vectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedScalar length = vectorVal.Length();
	Red::System::Float estimatedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	EXPECT_FLOAT_EQ( length.X, estimatedLength );
}

TEST( RedMath, RedVector3_float_SquareLength )
{
	RedMath::FLOAT::RedVector3 vectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedScalar length = vectorVal.SquareLength();
	Red::System::Float expectedLength = Red::Math::MSqr( constVals[3] )+ Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );
	EXPECT_FLOAT_EQ( length.X, expectedLength );
}

TEST( RedMath, RedVector3_float_Normalize_Functions)
{
	RedMath::FLOAT::RedVector3 vectorValNormalized( constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector3 normalizeVectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector3 normalizeFastVectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedVector3 zeroVec( constVals[0], constVals[0], constVals[0] );
	RedMath::FLOAT::RedVector3 zeroVecFast( constVals[0], constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[3] * unitVal;
	Red::System::Float yVal = constVals[4] * unitVal;
	Red::System::Float zVal = constVals[5] * unitVal;

	normalizeVectorVal.Normalize();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, constVals[1] );

	RedMath::FLOAT::RedVector3 normal = vectorValNormalized.Normalized();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, 1.0f );

	normal.SetZeros();

	normal = zeroVec.Normalized();
	EXPECT_FLOAT_EQ( normal.X, 0.0f );
	EXPECT_FLOAT_EQ( normal.Y, 0.0f );
	EXPECT_FLOAT_EQ( normal.Z, 0.0f );
	EXPECT_FLOAT_EQ( normal.W, 1.0f );

	zeroVec.Normalize();
	EXPECT_FLOAT_EQ( zeroVec.X, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Y, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Z, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.W, 1.0f );
}

TEST( RedMath, RedVector3_float_AlmostEqual )
{
	const Red::System::Float eps = 0.00001f;
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = eps + adj;
	RedMath::FLOAT::RedVector3 vA( constVals[1], constVals[2], constVals[3] );
	RedMath::FLOAT::RedVector3 vB( constVals[1] + adj, constVals[2] + adj, constVals[3] + adj );
	RedMath::FLOAT::RedVector3 vC( constVals[1] + adj2, constVals[2] + adj2, constVals[3] + adj2 );

	EXPECT_TRUE( vA.IsAlmostEqual( vB, eps ) );
	EXPECT_FALSE( vA.IsAlmostEqual( vC, eps ) );
}

TEST( RedMath, RedVector3_float_IsZero_or_AlmostZero )
{
	const Red::System::Float eps = 0.00001f;
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = eps + adj;
	RedMath::FLOAT::RedVector3 zero;
	RedMath::FLOAT::RedVector3 epsZero( eps, eps, eps );
	RedMath::FLOAT::RedVector3 notZero( adj2, adj2, adj2 );

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

TEST( RedMath, RedVector3_simd_construction) 
{
	RedMath::SIMD::RedVector3 noParams;
	EXPECT_FLOAT_EQ( noParams.X, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.W, constVals[1] );

	RedMath::SIMD::RedVector3 singleFloats( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( singleFloats.X, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloats.Y, constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( singleFloats.W, constVals[1] );

	RedMath::SIMD::RedScalar scalarSet( constVals[3] );
	RedMath::SIMD::RedVector3 fromScalar( scalarSet );
	EXPECT_FLOAT_EQ( fromScalar.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Y, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( fromScalar.W, constVals[1] );

	RedMath::SIMD::RedVector3 vectorSet( constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedVector3 fromVector( vectorSet );
	EXPECT_FLOAT_EQ( fromVector.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVector.Y, constVals[4] );
	EXPECT_FLOAT_EQ( fromVector.Z, constVals[5] ); 
	EXPECT_FLOAT_EQ( fromVector.W, constVals[1] );

	RED_ALIGNED_VAR( Red::System::Float, 16 ) fltPtrs[3] = { 4.0f, 5.0f, 6.0f };
	RedMath::SIMD::RedVector3 fromFlt( fltPtrs );
	EXPECT_FLOAT_EQ( fromFlt.X, fltPtrs[0] );
	EXPECT_FLOAT_EQ( fromFlt.Y, fltPtrs[1] );
	EXPECT_FLOAT_EQ( fromFlt.Z, fltPtrs[2] ); 
	EXPECT_FLOAT_EQ( fromFlt.W, constVals[1] );
}


TEST( RedMath, RedVector3_simd_Operator_Assignment )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[1] );
	RedMath::SIMD::RedVector3 result = scalarVal;
	EXPECT_FLOAT_EQ( result.X, scalarVal.X );
	EXPECT_FLOAT_EQ( result.Y, scalarVal.Y );
	EXPECT_FLOAT_EQ( result.Z, scalarVal.Z );
	EXPECT_FLOAT_EQ( result.W, constVals[1] );

	result = RedMath::SIMD::RedVector3( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( result.X, constVals[1] );
	EXPECT_FLOAT_EQ( result.Y, constVals[2] );
	EXPECT_FLOAT_EQ( result.Z, constVals[3] );
	EXPECT_FLOAT_EQ( result.W, constVals[1] );
}

TEST( RedMath, RedVector3_simd_AsFloat )
{
	RedMath::SIMD::RedVector3 vectorVal( constVals[2], constVals[3], constVals[4] );
	const Red::System::Float* fltPtr = vectorVal.AsFloat();

	EXPECT_FLOAT_EQ( fltPtr[0], vectorVal.X ); 
	EXPECT_FLOAT_EQ( fltPtr[1], vectorVal.Y ); 
	EXPECT_FLOAT_EQ( fltPtr[2], vectorVal.Z ); 
	EXPECT_FLOAT_EQ( fltPtr[3], vectorVal.W );
}


TEST( RedMath, RedVector3_simd_Set_Functions )
{
	RedMath::SIMD::RedVector3 vectorVal;
	const RedMath::SIMD::RedVector3 copySetVectorVal( constVals[1], constVals[2], constVals[3] );
	const RedMath::SIMD::RedScalar copySetScalarVal( constVals[5] );
	RED_ALIGNED_VAR( Red::System::Float, 16 ) fltPtr[3] = { 4.0f, 5.0f, 6.0f };

	vectorVal.Set( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.Set( copySetVectorVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( vectorVal.X, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, fltPtr[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, fltPtr[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.SetZeros();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.SetOnes();
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );
}

TEST( RedMath, RedVector3_simd_Negate_Functions )
{
	RedMath::SIMD::RedVector3 vectorVal( constVals[1], constVals[2], constVals[3] );
	RedMath::SIMD::RedVector3 vectorValInv( -constVals[1], -constVals[2], -constVals[3] );
	RedMath::SIMD::RedVector3 vectorValNegate( vectorVal );
	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[1] );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[1] );

	RedMath::SIMD::RedVector3 negatedVectorVal( vectorVal.Negated() );
	EXPECT_FLOAT_EQ( negatedVectorVal.X, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( negatedVectorVal.W, constVals[1] );

	RedMath::SIMD::RedVector3 NegatedVectorValInv( vectorValInv.Negated() );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.X, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Y, constVals[2] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Z, constVals[3] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.W, constVals[1] );
}


TEST( RedMath, RedVector3_simd_Length )
{
	RedMath::SIMD::RedVector3 vectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedScalar length = vectorVal.Length();
	Red::System::Float estimatedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	EXPECT_FLOAT_EQ( length.X, estimatedLength );
}

TEST( RedMath, RedVector3_simd_SquareLength )
{
	RedMath::SIMD::RedVector3 vectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedScalar length = vectorVal.SquareLength();
	Red::System::Float expectedLength = Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );
	EXPECT_FLOAT_EQ( length.X, expectedLength );
}

TEST( RedMath, RedVector3_simd_Normalize_Functions)
{
	RedMath::SIMD::RedVector3 vectorValNormalized( constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedVector3 normalizeVectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedVector3 normalizeFastVectorVal( constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedVector3 zeroVec( constVals[0], constVals[0], constVals[0] );
	RedMath::SIMD::RedVector3 zeroVecFast( constVals[0], constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[3] * unitVal;
	Red::System::Float yVal = constVals[4] * unitVal;
	Red::System::Float zVal = constVals[5] * unitVal;

	normalizeVectorVal.Normalize();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, constVals[1] );

	RedMath::SIMD::RedVector3 normal = vectorValNormalized.Normalized();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, constVals[1] );

	normal.SetZeros();
	normal = zeroVec.Normalized();
	EXPECT_FLOAT_EQ( normal.X, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( normal.W, constVals[1] );

	zeroVec.Normalize();
	EXPECT_FLOAT_EQ( zeroVec.X, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Y, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Z, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.W, constVals[1] );
	
	zeroVec.SetZeros();
	normal.SetZeros();
	normal = zeroVec.NormalizedFast();
	EXPECT_FLOAT_EQ( normal.X, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( normal.W, constVals[1] );

	zeroVec.NormalizeFast();
	EXPECT_FLOAT_EQ( zeroVec.X, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Y, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Z, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.W, constVals[1] );
	//////////////////////////////////////////////////////////////////////////
	// NOTE:
	// Accuracy between instructions is a problem - hence this nasty intrinsic
	// here for testing. As we'll get different results from different methods
	//////////////////////////////////////////////////////////////////////////
	Red::System::Float sqr = Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );

	RED_ALIGNED_VAR( Red::System::Float, 16 ) expectedApprox = 0.0f;
	_mm_store_ss( &expectedApprox, _mm_rsqrt_ss( _mm_set1_ps( sqr ) ) );
	xVal = constVals[3] * expectedApprox;
	yVal = constVals[4] * expectedApprox;
	zVal = constVals[5] * expectedApprox;

	normal.SetZeros();
	normal = normalizeFastVectorVal.NormalizedFast();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, constVals[1] );

	normalizeFastVectorVal.NormalizeFast();
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.W, constVals[1] );
}


TEST( RedMath, RedVector3_simd_AlmostEqual )
{
	const Red::System::Float eps = 0.00001f;
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = eps + adj;
	RedMath::FLOAT::RedVector3 vA( constVals[1], constVals[2], constVals[3] );
	RedMath::FLOAT::RedVector3 vB( constVals[1] + adj, constVals[2] + adj, constVals[3] + adj );
	RedMath::FLOAT::RedVector3 vC( constVals[1] + adj2, constVals[2] + adj2, constVals[3] + adj2 );

	EXPECT_TRUE( vA.IsAlmostEqual( vB, eps ) );
	EXPECT_FALSE( vA.IsAlmostEqual( vC, eps ) );
}

TEST( RedMath, RedVector3_simd_IsZero_or_AlmostZero )
{
	const SIMDVector eps = _mm_set1_ps( 0.00001f );
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = 0.00001f + adj;
	RedMath::SIMD::RedVector3 zero;
	RedMath::SIMD::RedVector3 epsZero( eps );
	RedMath::SIMD::RedVector3 notZero( adj2, adj2, adj2 );

	EXPECT_TRUE( zero.IsZero() );
	EXPECT_FALSE( epsZero.IsZero() );
	EXPECT_FALSE( notZero.IsZero() );

	EXPECT_TRUE( zero.IsAlmostZero( eps ) );
	EXPECT_TRUE( epsZero.IsAlmostZero( eps ) );
	EXPECT_FALSE( notZero.IsAlmostZero( eps ) );
}
