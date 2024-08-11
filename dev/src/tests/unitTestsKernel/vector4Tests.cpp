#include "build.h"

#include "../../common/redMath/redmathbase.h"

#include "../../common/redMath/redVector4_float.h"
#include "../../common/redMath/random/random.h"
#include "../../common/redMath/random/standardRand.h"
#include "../../common/redMath/vectorArithmetic_float.h"

const Red::System::Float constVals[7] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
//////////////////////////////////////////////////////////////////////////
// float
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedVector4_float_construction )
{
	RedMath::FLOAT::RedVector4 noParams;
	EXPECT_FLOAT_EQ( noParams.X, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.W, constVals[0] );

	RedMath::FLOAT::RedScalar scalarSet( constVals[3] );
	RedMath::FLOAT::RedVector4 fromScalar( scalarSet );
	EXPECT_FLOAT_EQ( fromScalar.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Y, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( fromScalar.W, constVals[3] );

	RedMath::FLOAT::RedVector3 vector3NoW( constVals[3], constVals[2], constVals[1] );
	RedMath::FLOAT::RedVector4 fromVec3NoW( vector3NoW );
	EXPECT_FLOAT_EQ( fromVec3NoW.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVec3NoW.Y, constVals[2] );
	EXPECT_FLOAT_EQ( fromVec3NoW.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( fromVec3NoW.W, constVals[0] );

	RedMath::FLOAT::RedVector4 fromVec3W( vector3NoW, constVals[4] );
	EXPECT_FLOAT_EQ( fromVec3W.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVec3W.Y, constVals[2] );
	EXPECT_FLOAT_EQ( fromVec3W.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( fromVec3W.W, constVals[4] );

	RedMath::FLOAT::RedVector4 fromVector4( fromVec3W );
	EXPECT_FLOAT_EQ( fromVector4.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVector4.Y, constVals[2] );
	EXPECT_FLOAT_EQ( fromVector4.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( fromVector4.W, constVals[4] );

	Red::System::Float fltPtrs[4] = { 3.0f, 4.0f, 5.0f, 6.0f };
	RedMath::FLOAT::RedVector4 fromFlt( fltPtrs );
	EXPECT_FLOAT_EQ( fromFlt.X, fltPtrs[0] );
	EXPECT_FLOAT_EQ( fromFlt.Y, fltPtrs[1] );
	EXPECT_FLOAT_EQ( fromFlt.Z, fltPtrs[2] ); 
	EXPECT_FLOAT_EQ( fromFlt.W, fltPtrs[3] );

	RedMath::FLOAT::RedVector4 singleFloats( constVals[1], constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( singleFloats.X, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloats.Y, constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( singleFloats.W, constVals[4] );
}

TEST( RedMath, RedVector4_float_Operator_Assignment )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[1] );
	RedMath::FLOAT::RedVector4 result = scalarVal;
	EXPECT_FLOAT_EQ( result.X, scalarVal.X );
	EXPECT_FLOAT_EQ( result.Y, scalarVal.Y );
	EXPECT_FLOAT_EQ( result.Z, scalarVal.Z );
	EXPECT_FLOAT_EQ( result.W, scalarVal.W );

	result = RedMath::FLOAT::RedVector4( constVals[1], constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( result.X, constVals[1] );
	EXPECT_FLOAT_EQ( result.Y, constVals[2] );
	EXPECT_FLOAT_EQ( result.Z, constVals[3] );
	EXPECT_FLOAT_EQ( result.W, constVals[4] );

	result = RedMath::FLOAT::RedVector3( constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( result.X, constVals[2] );
	EXPECT_FLOAT_EQ( result.Y, constVals[3] );
	EXPECT_FLOAT_EQ( result.Z, constVals[4] );
	EXPECT_FLOAT_EQ( result.W, 0.0f );
}

TEST( RedMath, RedVector4_float_AsFloat )
{
	RedMath::FLOAT::RedVector4 vectorVal( constVals[1], constVals[2], constVals[3], constVals[4] );
	const Red::System::Float* fltPtr = vectorVal.AsFloat();

	EXPECT_FLOAT_EQ( fltPtr[0], vectorVal.X ); 
	EXPECT_FLOAT_EQ( fltPtr[1], vectorVal.Y ); 
	EXPECT_FLOAT_EQ( fltPtr[2], vectorVal.Z ); 
	EXPECT_FLOAT_EQ( fltPtr[3], vectorVal.W );
}

TEST( RedMath, RedVector4_float_Set_Functions )
{
	RedMath::FLOAT::RedVector4 vectorVal;
	const RedMath::FLOAT::RedVector4 copySetVectorVal( constVals[1], constVals[2], constVals[3], constVals[4] );
	const RedMath::FLOAT::RedVector3 copySetVec3Val( constVals[4], constVals[3], constVals[2] );
	const RedMath::FLOAT::RedScalar copySetScalarVal( constVals[5] );
	Red::System::Float fltPtr[4] = { 4.0f, 5.0f, 6.0f, 7.0f };

	vectorVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[5] );

	vectorVal.Set( copySetVec3Val );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[4] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[0] );

	vectorVal.Set( copySetVec3Val, constVals[6] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[4] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[6] );


	vectorVal.Set( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[0] );
	
	vectorVal.Set( constVals[1], constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[4] );

	vectorVal.Set( copySetVectorVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[4] );

	vectorVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( vectorVal.X, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, fltPtr[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, fltPtr[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, fltPtr[3] );

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

TEST( RedMath, RedVector4_float_Negate_Functions )
{
	RedMath::FLOAT::RedVector4 vectorVal( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::FLOAT::RedVector4 vectorValInv( -constVals[1], -constVals[2], -constVals[3], -constVals[4] );
	RedMath::FLOAT::RedVector4 vectorValNegate( vectorVal );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, -constVals[4] );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[4] );

	RedMath::FLOAT::RedVector4 negatedVectorVal( vectorVal.Negated() );
	EXPECT_FLOAT_EQ( negatedVectorVal.X, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( negatedVectorVal.W, -constVals[4] );

	RedMath::FLOAT::RedVector4 NegatedVectorValInv( vectorValInv.Negated() );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.X, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Y, constVals[2] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Z, constVals[3] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.W, constVals[4] );
}

TEST( RedMath, RedVector4_float_Sum )
{
	RedMath::FLOAT::RedVector4 vectorVal( constVals[2], constVals[3], constVals[4], constVals[5] );
	
	Red::System::Float sum3 = constVals[2] + constVals[3] + constVals[4];
	RedMath::FLOAT::RedScalar val = vectorVal.Sum3();
	EXPECT_FLOAT_EQ( val.X, sum3 );

	Red::System::Float sum4 = constVals[2] + constVals[3] + constVals[4] + constVals[5];
	val = vectorVal.Sum4();
	EXPECT_FLOAT_EQ( val.X, sum4 );
}

TEST( RedMath, RedVector4_float_Length )
{
	RedMath::FLOAT::RedVector4 vectorVal( constVals[2], constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedScalar length3 = vectorVal.Length3();
	Red::System::Float estimatedLength3 = Red::Math::MSqrt( Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) );
	EXPECT_FLOAT_EQ( length3.X, estimatedLength3 );

	RedMath::FLOAT::RedScalar length4 = vectorVal.Length4();
	Red::System::Float estimatedLength4 = Red::Math::MSqrt( Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	EXPECT_FLOAT_EQ( length4.X, estimatedLength4 );
}

TEST( RedMath, RedVector4_float_SquareLength )
{
	RedMath::FLOAT::RedVector4 vectorVal( constVals[2], constVals[3], constVals[4], constVals[5] );
	RedMath::FLOAT::RedScalar length3 = vectorVal.SquareLength3();
	Red::System::Float estimatedLength3 = Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] );
	EXPECT_FLOAT_EQ( length3.X, estimatedLength3 );

	RedMath::FLOAT::RedScalar length4 = vectorVal.SquareLength4();
	Red::System::Float estimatedLength4 = Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );
	EXPECT_FLOAT_EQ( length4.X, estimatedLength4 );
}

TEST( RedMath, RedVector4_float_Normalize3_Functions )
{
	RedMath::FLOAT::RedVector4 vectorValNormalized( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::FLOAT::RedVector4 normalizeVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::FLOAT::RedVector4 normalizeFastVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::FLOAT::RedVector4 zeroVec( constVals[0], constVals[0], constVals[0], constVals[0] );
	RedMath::FLOAT::RedVector4 zeroVecFast( constVals[0], constVals[0], constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[3] * unitVal;
	Red::System::Float yVal = constVals[4] * unitVal;
	Red::System::Float zVal = constVals[5] * unitVal;
	
	normalizeVectorVal.Normalize3();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, constVals[0] );

	RedMath::FLOAT::RedVector4 normal = vectorValNormalized.Normalized3();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, constVals[0] );

	normal.SetZeros();

	normal = zeroVec.Normalized3();
	EXPECT_FLOAT_EQ( normal.X, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( normal.W, constVals[0] );

	zeroVec.Normalize3();
	EXPECT_FLOAT_EQ( zeroVec.X, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Y, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Z, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.W, constVals[0] );
}

TEST( RedMath, RedVector4_float_Normalize4_Functions )
{
	RedMath::FLOAT::RedVector4 vectorValNormalized( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::FLOAT::RedVector4 normalizeVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::FLOAT::RedVector4 normalizeFastVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::FLOAT::RedVector4 zeroVec( constVals[0], constVals[0], constVals[0], constVals[0] );
	RedMath::FLOAT::RedVector4 zeroVecFast( constVals[0], constVals[0], constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) + Red::Math::MSqr( constVals[6] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[3] * unitVal;
	Red::System::Float yVal = constVals[4] * unitVal;
	Red::System::Float zVal = constVals[5] * unitVal;
	Red::System::Float wVal = constVals[6] * unitVal;

	normalizeVectorVal.Normalize4();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, wVal );

	RedMath::FLOAT::RedVector4 normal = vectorValNormalized.Normalized4();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, wVal );

	normal.SetZeros();

	normal = zeroVec.Normalized4();
	EXPECT_FLOAT_EQ( normal.X, 0.0f );
	EXPECT_FLOAT_EQ( normal.Y, 0.0f );
	EXPECT_FLOAT_EQ( normal.Z, 0.0f );
	EXPECT_FLOAT_EQ( normal.W, 0.0f );

	zeroVec.Normalize4();
	EXPECT_FLOAT_EQ( zeroVec.X, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Y, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Z, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.W, 0.0f );
}

TEST( RedMath, RedVector4_float_IsNormalized )
{
	RedMath::FLOAT::RedVector4 resVecNorm4( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::FLOAT::RedVector4 resVecNorm3( constVals[1], constVals[2], constVals[3] );
	RedMath::FLOAT::RedVector4 notNormalized( constVals[3], constVals[4], constVals[5], constVals[6] );
	resVecNorm3.Normalize3();
	EXPECT_TRUE( resVecNorm3.IsNormalized3() );
	resVecNorm4.Normalize4();
	EXPECT_TRUE( resVecNorm4.IsNormalized4() );
	EXPECT_FALSE( notNormalized.IsNormalized3() );
	EXPECT_FALSE( notNormalized.IsNormalized4() );
}

TEST( RedMath, RedVector4_float_AlmostEqual )
{
	const Red::System::Float eps = 0.00001f;
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = eps + adj;
	RedMath::FLOAT::RedVector4 vA( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::FLOAT::RedVector4 vB( constVals[1] + adj, constVals[2] + adj, constVals[3] + adj, constVals[4] + adj );
	RedMath::FLOAT::RedVector4 vC( constVals[1] + adj2, constVals[2] + adj2, constVals[3] + adj2, constVals[4] + adj2 );

	EXPECT_TRUE( vA.IsAlmostEqual( vB, eps ) );
	EXPECT_FALSE( vA.IsAlmostEqual( vC, eps ) );
}

TEST( RedMath, RedVector4_float_IsZero_or_AlmostZero )
{
	const Red::System::Float eps = 0.00001f;
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = eps + adj;
	RedMath::FLOAT::RedVector4 zero;
	RedMath::FLOAT::RedVector4 epsZero( eps, eps, eps );
	RedMath::FLOAT::RedVector4 notZero( adj2, adj2, adj2 );

	EXPECT_TRUE( zero.IsZero() );
	EXPECT_FALSE( epsZero.IsZero() );
	EXPECT_FALSE( notZero.IsZero() );

	EXPECT_TRUE( zero.IsAlmostZero( eps ) );
	EXPECT_TRUE( epsZero.IsAlmostZero( eps ) );
	EXPECT_FALSE( notZero.IsAlmostZero( eps ) );
}

TEST( RedMath, RedVector4_float_Upper )
{
	RedMath::FLOAT::RedVector4 upper3Test( constVals[3], constVals[2], constVals[1] );
	RedMath::FLOAT::RedVector4 upper4Test( constVals[5], constVals[6], constVals[2], constVals[1] );
	
	RedMath::FLOAT::RedScalar upper3Res = upper3Test.Upper3();
	EXPECT_FLOAT_EQ( upper3Res.X, constVals[3] );
	RedMath::FLOAT::RedScalar upper4Res = upper4Test.Upper4();
	EXPECT_FLOAT_EQ( upper4Res.X, constVals[6] );
}

TEST( RedMath, RedVector4_float_Lower )
{
	RedMath::FLOAT::RedVector4 lower3Test( constVals[3], constVals[2], constVals[1] );
	RedMath::FLOAT::RedVector4 lower4Test( constVals[5], constVals[6], constVals[2], constVals[1] );

	RedMath::FLOAT::RedScalar lower3Res = lower3Test.Lower3();
	EXPECT_FLOAT_EQ( lower3Res.X, constVals[1] );
	RedMath::FLOAT::RedScalar lower4Res = lower4Test.Lower4();
	EXPECT_FLOAT_EQ( lower4Res.X, constVals[1] );
}

TEST( RedMath, RedVector4_float_ZeroElement )
{
	RedMath::FLOAT::RedVector4 fullOnes( RedMath::FLOAT::RedVector4::ONES );
	RedMath::FLOAT::RedVector4 xZero( fullOnes.ZeroElement( 0 ) );
	EXPECT_FLOAT_EQ( xZero.X, constVals[0] );
	EXPECT_FLOAT_EQ( xZero.Y, constVals[1] );
	EXPECT_FLOAT_EQ( xZero.Z, constVals[1] );
	EXPECT_FLOAT_EQ( xZero.W, constVals[1] );
	
	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );

	RedMath::FLOAT::RedVector4 yZero( fullOnes.ZeroElement( 1 ) );
	EXPECT_FLOAT_EQ( yZero.X, constVals[1] );
	EXPECT_FLOAT_EQ( yZero.Y, constVals[0] );
	EXPECT_FLOAT_EQ( yZero.Z, constVals[1] );
	EXPECT_FLOAT_EQ( yZero.W, constVals[1] );

	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );

	RedMath::FLOAT::RedVector4 zZero( fullOnes.ZeroElement( 2 ) );
	EXPECT_FLOAT_EQ( zZero.X, constVals[1] );
	EXPECT_FLOAT_EQ( zZero.Y, constVals[1] );
	EXPECT_FLOAT_EQ( zZero.Z, constVals[0] );
	EXPECT_FLOAT_EQ( zZero.W, constVals[1] );

	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );

	RedMath::FLOAT::RedVector4 wZero( fullOnes.ZeroElement( 3 ) );
	EXPECT_FLOAT_EQ( wZero.X, constVals[1] );
	EXPECT_FLOAT_EQ( wZero.Y, constVals[1] );
	EXPECT_FLOAT_EQ( wZero.Z, constVals[1] );
	EXPECT_FLOAT_EQ( wZero.W, constVals[0] );

	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );
}

TEST( RedMath, RedVector4_float_AsScalar )
{
	RedMath::FLOAT::RedVector4 sourceVec( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::FLOAT::RedScalar result( sourceVec.AsScalar( 0 ) );
	EXPECT_FLOAT_EQ( result.X, constVals[1] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );

	result = sourceVec.AsScalar( 1 );
	EXPECT_FLOAT_EQ( result.X, constVals[2] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );

	result = sourceVec.AsScalar( 2 );
	EXPECT_FLOAT_EQ( result.X, constVals[3] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );

	result = sourceVec.AsScalar( 3 );
	EXPECT_FLOAT_EQ( result.X, constVals[4] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );
}

TEST( RedMath, RedVector4_float_AsVector3 )
{
	RedMath::FLOAT::RedVector4 sourceVec( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::FLOAT::RedVector3 result( sourceVec.AsVector3() );

	EXPECT_FLOAT_EQ( result.X, constVals[1] );
	EXPECT_FLOAT_EQ( result.Y, constVals[2] );
	EXPECT_FLOAT_EQ( result.Z, constVals[3] );
	EXPECT_FLOAT_EQ( result.W, constVals[1] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );
}

TEST( RedMath, RedVector4_float_IsOK )
{
	RedMath::FLOAT::RedVector4 notOk( RED_NAN, RED_NEG_NAN, RED_INFINITY, RED_NEG_INFINITY );
	RedMath::FLOAT::RedVector4 isOk( constVals[0], constVals[1], constVals[0], constVals[2] );

	EXPECT_FALSE( notOk.IsOk() );
	EXPECT_TRUE( isOk.IsOk() );

	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.X ) );
	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.Y ) );
	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.Z ) );
	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.W ) );
}

//////////////////////////////////////////////////////////////////////////
// simd
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedVector4_simd_construction )
{
	RedMath::SIMD::RedVector4 noParams;
	EXPECT_FLOAT_EQ( noParams.X, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.W, constVals[0] );

	RedMath::SIMD::RedScalar scalarSet( constVals[3] );
	RedMath::SIMD::RedVector4 fromScalar( scalarSet );
	EXPECT_FLOAT_EQ( fromScalar.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Y, constVals[3] );
	EXPECT_FLOAT_EQ( fromScalar.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( fromScalar.W, constVals[3] );

	RedMath::SIMD::RedVector3 vector3NoW( constVals[3], constVals[2], constVals[1] );
	RedMath::SIMD::RedVector4 fromVec3NoW( vector3NoW );
	EXPECT_FLOAT_EQ( fromVec3NoW.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVec3NoW.Y, constVals[2] );
	EXPECT_FLOAT_EQ( fromVec3NoW.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( fromVec3NoW.W, constVals[1] );

	RedMath::SIMD::RedVector4 fromVec3W( vector3NoW, constVals[4] );
	EXPECT_FLOAT_EQ( fromVec3W.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVec3W.Y, constVals[2] );
	EXPECT_FLOAT_EQ( fromVec3W.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( fromVec3W.W, constVals[4] );

	RedMath::SIMD::RedVector4 fromVector4( fromVec3W );
	EXPECT_FLOAT_EQ( fromVector4.X, constVals[3] );
	EXPECT_FLOAT_EQ( fromVector4.Y, constVals[2] );
	EXPECT_FLOAT_EQ( fromVector4.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( fromVector4.W, constVals[4] );

	RED_ALIGNED_VAR( Red::System::Float, 16 ) fltPtrs[4] = { 3.0f, 4.0f, 5.0f, 6.0f };
	RedMath::SIMD::RedVector4 fromFlt( fltPtrs );
	EXPECT_FLOAT_EQ( fromFlt.X, fltPtrs[0] );
	EXPECT_FLOAT_EQ( fromFlt.Y, fltPtrs[1] );
	EXPECT_FLOAT_EQ( fromFlt.Z, fltPtrs[2] ); 
	EXPECT_FLOAT_EQ( fromFlt.W, fltPtrs[3] );

	RedMath::SIMD::RedVector4 singleFloats( constVals[1], constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( singleFloats.X, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloats.Y, constVals[2] );
	EXPECT_FLOAT_EQ( singleFloats.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( singleFloats.W, constVals[4] );
}

TEST( RedMath, RedVector4_simd_Operator_Assignment )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[1] );
	RedMath::SIMD::RedVector4 result = scalarVal;
	EXPECT_FLOAT_EQ( result.X, scalarVal.X );
	EXPECT_FLOAT_EQ( result.Y, scalarVal.Y );
	EXPECT_FLOAT_EQ( result.Z, scalarVal.Z );
	EXPECT_FLOAT_EQ( result.W, scalarVal.W );

	result = RedMath::SIMD::RedVector4( constVals[1], constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( result.X, constVals[1] );
	EXPECT_FLOAT_EQ( result.Y, constVals[2] );
	EXPECT_FLOAT_EQ( result.Z, constVals[3] );
	EXPECT_FLOAT_EQ( result.W, constVals[4] );

	result = RedMath::SIMD::RedVector3( constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( result.X, constVals[2] );
	EXPECT_FLOAT_EQ( result.Y, constVals[3] );
	EXPECT_FLOAT_EQ( result.Z, constVals[4] );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_AsFloat )
{
	RedMath::SIMD::RedVector4 vectorVal( constVals[1], constVals[2], constVals[3], constVals[4] );
	const Red::System::Float* fltPtr = vectorVal.AsFloat();

	EXPECT_FLOAT_EQ( fltPtr[0], vectorVal.X ); 
	EXPECT_FLOAT_EQ( fltPtr[1], vectorVal.Y ); 
	EXPECT_FLOAT_EQ( fltPtr[2], vectorVal.Z ); 
	EXPECT_FLOAT_EQ( fltPtr[3], vectorVal.W );
}

TEST( RedMath, RedVector4_simd_Set_Functions )
{
	RedMath::SIMD::RedVector4 vectorVal;
	const RedMath::SIMD::RedVector4 copySetVectorVal( constVals[1], constVals[2], constVals[3], constVals[4] );
	const RedMath::SIMD::RedVector3 copySetVec3Val( constVals[4], constVals[3], constVals[2] );
	const RedMath::SIMD::RedScalar copySetScalarVal( constVals[5] );
	RED_ALIGNED_VAR( Red::System::Float, 16 ) fltPtr[4] = { 4.0f, 5.0f, 6.0f, 7.0f };

	vectorVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[5] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[5] );

	vectorVal.Set( copySetVec3Val );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[4] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[1] );

	vectorVal.Set( copySetVec3Val, constVals[6] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[4] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[6] );


	vectorVal.Set( constVals[1], constVals[2], constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[0] );

	vectorVal.Set( constVals[1], constVals[2], constVals[3], constVals[4] );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[4] );

	vectorVal.Set( copySetVectorVal );
	EXPECT_FLOAT_EQ( vectorVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorVal.W, constVals[4] );

	vectorVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( vectorVal.X, fltPtr[0] );
	EXPECT_FLOAT_EQ( vectorVal.Y, fltPtr[1] );
	EXPECT_FLOAT_EQ( vectorVal.Z, fltPtr[2] );
	EXPECT_FLOAT_EQ( vectorVal.W, fltPtr[3] );

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

TEST( RedMath, RedVector4_simd_Negate_Functions )
{
	RedMath::SIMD::RedVector4 vectorVal( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::SIMD::RedVector4 vectorValInv( -constVals[1], -constVals[2], -constVals[3], -constVals[4] );
	RedMath::SIMD::RedVector4 vectorValNegate( vectorVal );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, -constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, -constVals[4] );

	vectorValNegate.Negate();
	EXPECT_FLOAT_EQ( vectorValNegate.X, constVals[1] );
	EXPECT_FLOAT_EQ( vectorValNegate.Y, constVals[2] );
	EXPECT_FLOAT_EQ( vectorValNegate.Z, constVals[3] );
	EXPECT_FLOAT_EQ( vectorValNegate.W, constVals[4] );

	RedMath::SIMD::RedVector4 negatedVectorVal( vectorVal.Negated() );
	EXPECT_FLOAT_EQ( negatedVectorVal.X, -constVals[1] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( negatedVectorVal.Z, -constVals[3] );
	EXPECT_FLOAT_EQ( negatedVectorVal.W, -constVals[4] );

	RedMath::SIMD::RedVector4 NegatedVectorValInv( vectorValInv.Negated() );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.X, constVals[1] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Y, constVals[2] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.Z, constVals[3] );
	EXPECT_FLOAT_EQ( NegatedVectorValInv.W, constVals[4] );
}


TEST( RedMath, RedVector4_simd_Sum )
{
	RedMath::SIMD::RedVector4 vectorVal( constVals[2], constVals[3], constVals[4], constVals[5] );

	Red::System::Float sum3 = constVals[2] + constVals[3] + constVals[4];
	RedMath::SIMD::RedScalar val = vectorVal.Sum3();
	EXPECT_FLOAT_EQ( val.X, sum3 );

	Red::System::Float sum4 = constVals[2] + constVals[3] + constVals[4] + constVals[5];
	val = vectorVal.Sum4();
	EXPECT_FLOAT_EQ( val.X, sum4 );
}

TEST( RedMath, RedVector4_simd_Length )
{
	RedMath::SIMD::RedVector4 vectorVal( constVals[2], constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedScalar length3 = vectorVal.Length3();
	Red::System::Float estimatedLength3 = Red::Math::MSqrt( Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) );
	EXPECT_FLOAT_EQ( length3.X, estimatedLength3 );

	RedMath::SIMD::RedScalar length4 = vectorVal.Length4();
	Red::System::Float estimatedLength4 = Red::Math::MSqrt( Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	EXPECT_FLOAT_EQ( length4.X, estimatedLength4 );
}

TEST( RedMath, RedVector4_simd_SquareLength )
{
	RedMath::SIMD::RedVector4 vectorVal( constVals[2], constVals[3], constVals[4], constVals[5] );
	RedMath::SIMD::RedScalar length3 = vectorVal.SquareLength3();
	Red::System::Float estimatedLength3 = Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] );
	EXPECT_FLOAT_EQ( length3.X, estimatedLength3 );

	RedMath::SIMD::RedScalar length4 = vectorVal.SquareLength4();
	Red::System::Float estimatedLength4 = Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );
	EXPECT_FLOAT_EQ( length4.X, estimatedLength4 );

	RedMath::SIMD::RedScalar length2 = vectorVal.SquareLength2();
	EXPECT_FLOAT_EQ( length2, Red::Math::MSqr( constVals[2] ) + Red::Math::MSqr( constVals[3] ) );
}


TEST( RedMath, RedVector4_simd_Normalize3_Functions )
{
	RedMath::SIMD::RedVector4 vectorValNormalized( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::SIMD::RedVector4 normalizeVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::SIMD::RedVector4 normalizeFastVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::SIMD::RedVector4 normalizeFastVectorVal3( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::SIMD::RedVector4 zeroVec( constVals[0], constVals[0], constVals[0], constVals[0] );
	RedMath::SIMD::RedVector4 zeroVecFast( constVals[0], constVals[0], constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[3] * unitVal;
	Red::System::Float yVal = constVals[4] * unitVal;
	Red::System::Float zVal = constVals[5] * unitVal;

	normalizeVectorVal.Normalize3();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, constVals[0] );

	RedMath::SIMD::RedVector4 normal = vectorValNormalized.Normalized3();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, constVals[0] );

	normal.SetZeros();
	normal = zeroVec.Normalized3();
	EXPECT_FLOAT_EQ( normal.X, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Y, constVals[0] );
	EXPECT_FLOAT_EQ( normal.Z, constVals[0] );
	EXPECT_FLOAT_EQ( normal.W, constVals[0] );

	zeroVec.Normalize3();
	EXPECT_FLOAT_EQ( zeroVec.X, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Y, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.Z, constVals[0] );
	EXPECT_FLOAT_EQ( zeroVec.W, constVals[0] );
	//////////////////////////////////////////////////////////////////////////
	// NOTE:
	// Accuracy between instructions is a problem - hence this nasty intrinsic
	// here for testing. As we'll get different results from different methods
	//////////////////////////////////////////////////////////////////////////
	Red::System::Float sqr = Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] );
	
	RED_ALIGNED_VAR( Red::System::Float, 16 ) expectedApprox = 0.0f;
	_mm_store_ss( &expectedApprox, _mm_rsqrt_ss( _mm_set1_ps( sqr ) ) ) ;

	xVal = constVals[3] * expectedApprox;
	yVal = constVals[4] * expectedApprox;
	zVal = constVals[5] * expectedApprox;

	normal.SetZeros();
	normal = normalizeFastVectorVal.NormalizedFast3();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, 0.0f );

	normalizeFastVectorVal.NormalizeFast3();
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.W, 0.0f );
}

TEST( RedMath, RedVector4_simd_Normalize4_Functions )
{
	RedMath::SIMD::RedVector4 vectorValNormalized( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::SIMD::RedVector4 normalizeVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::SIMD::RedVector4 normalizeFastVectorVal( constVals[3], constVals[4], constVals[5], constVals[6] );
	RedMath::SIMD::RedVector4 zeroVec( constVals[0], constVals[0], constVals[0], constVals[0] );
	RedMath::SIMD::RedVector4 zeroVecFast( constVals[0], constVals[0], constVals[0], constVals[0] );

	Red::System::Float expectedLength = Red::Math::MSqrt( Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) + Red::Math::MSqr( constVals[6] ) );
	Red::System::Float unitVal = 1.0f / expectedLength;
	Red::System::Float xVal = constVals[3] * unitVal;
	Red::System::Float yVal = constVals[4] * unitVal;
	Red::System::Float zVal = constVals[5] * unitVal;
	Red::System::Float wVal = constVals[6] * unitVal;

	normalizeVectorVal.Normalize4();
	EXPECT_FLOAT_EQ( normalizeVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeVectorVal.W, wVal );

	RedMath::SIMD::RedVector4 normal = vectorValNormalized.Normalized4();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, wVal );

	normal.SetZeros();
	normal = zeroVec.Normalized4();
	EXPECT_FLOAT_EQ( normal.X, 0.0f );
	EXPECT_FLOAT_EQ( normal.Y, 0.0f );
	EXPECT_FLOAT_EQ( normal.Z, 0.0f );
	EXPECT_FLOAT_EQ( normal.W, 0.0f );

	zeroVec.Normalize4();
	EXPECT_FLOAT_EQ( zeroVec.X, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Y, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.Z, 0.0f );
	EXPECT_FLOAT_EQ( zeroVec.W, 0.0f );

	//////////////////////////////////////////////////////////////////////////
	// NOTE:
	// Accuracy between instructions is a problem - hence this nasty intrinsic
	// here for testing. As we'll get different results from different methods
	//////////////////////////////////////////////////////////////////////////
	Red::System::Float sqr4 = Red::Math::MSqr( constVals[3] ) + Red::Math::MSqr( constVals[4] ) + Red::Math::MSqr( constVals[5] ) + Red::Math::MSqr( constVals[6] );

	RED_ALIGNED_VAR( Red::System::Float, 16 ) expectedApprox4 = 0.0f;
	_mm_store_ss( &expectedApprox4, _mm_rsqrt_ss( _mm_set1_ps( sqr4 ) ) );
	xVal = constVals[3] * expectedApprox4;
	yVal = constVals[4] * expectedApprox4;
	zVal = constVals[5] * expectedApprox4;
	wVal = constVals[6] * expectedApprox4;

	normal.SetZeros();
	normal = normalizeFastVectorVal.NormalizedFast4();
	EXPECT_FLOAT_EQ( normal.X, xVal );
	EXPECT_FLOAT_EQ( normal.Y, yVal );
	EXPECT_FLOAT_EQ( normal.Z, zVal );
	EXPECT_FLOAT_EQ( normal.W, wVal );

	normalizeFastVectorVal.NormalizeFast4();
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.X, xVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Y, yVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.Z, zVal );
	EXPECT_FLOAT_EQ( normalizeFastVectorVal.W, wVal );
}


TEST( RedMath, RedVector4_simd_IsNormalized )
{
	RedMath::SIMD::RedVector4 resVecNorm4( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::SIMD::RedVector4 resVecNorm3( constVals[1], constVals[2], constVals[3] );
	RedMath::SIMD::RedVector4 notNormalized( constVals[3], constVals[4], constVals[5], constVals[6] );
	resVecNorm3.Normalize3();
	EXPECT_TRUE( resVecNorm3.IsNormalized3() );
	resVecNorm4.Normalize4();
	EXPECT_TRUE( resVecNorm4.IsNormalized4() );
	EXPECT_FALSE( notNormalized.IsNormalized3() );
	EXPECT_FALSE( notNormalized.IsNormalized4() );
}

TEST( RedMath, RedVector4_simd_AlmostEqual )
{
	const SIMDVector eps = _mm_set1_ps( 0.00001f );
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = 0.00001f + adj;
	RedMath::SIMD::RedVector4 vA( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::SIMD::RedVector4 vB( constVals[1] + adj, constVals[2] + adj, constVals[3] + adj, constVals[4] + adj );
	RedMath::SIMD::RedVector4 vC( constVals[1] + adj2, constVals[2] + adj2, constVals[3] + adj2, constVals[4] + adj2 );

	EXPECT_TRUE( vA.IsAlmostEqual( vB, eps ) );
	EXPECT_FALSE( vA.IsAlmostEqual( vC, eps ) );
	
	float feps = 0.00001f;
	EXPECT_TRUE( vA.IsAlmostEqual( vB, feps ) );
	EXPECT_FALSE( vA.IsAlmostEqual( vC, feps ) );
}

TEST( RedMath, RedVector4_simd_IsZero_or_AlmostZero )
{
	const SIMDVector eps = _mm_set1_ps( 0.00001f );
	const Red::System::Float adj = 0.000001f;
	const Red::System::Float adj2 = 0.00001f + adj;
	RedMath::SIMD::RedVector4 zero;
	RedMath::SIMD::RedVector4 epsZero( eps );
	RedMath::SIMD::RedVector4 notZero( adj2, adj2, adj2 );

	EXPECT_TRUE( zero.IsZero() );
	EXPECT_FALSE( epsZero.IsZero() );
	EXPECT_FALSE( notZero.IsZero() );

	EXPECT_TRUE( zero.IsAlmostZero( eps ) );
	EXPECT_TRUE( epsZero.IsAlmostZero( eps ) );
	EXPECT_FALSE( notZero.IsAlmostZero( eps ) );
}

TEST( RedMath, RedVector4_simd_Upper )
{
	RedMath::SIMD::RedVector4 upper3Test( constVals[3], constVals[2], constVals[1] );
	RedMath::SIMD::RedVector4 upper4Test( constVals[5], constVals[6], constVals[2], constVals[1] );

	RedMath::SIMD::RedScalar upper3Res = upper3Test.Upper3();
	EXPECT_FLOAT_EQ( upper3Res.X, constVals[3] );
	RedMath::SIMD::RedScalar upper4Res = upper4Test.Upper4();
	EXPECT_FLOAT_EQ( upper4Res.X, constVals[6] );
}

TEST( RedMath, RedVector4_simd_Lower )
{
	RedMath::SIMD::RedVector4 lower3Test( constVals[3], constVals[2], constVals[1] );
	RedMath::SIMD::RedVector4 lower4Test( constVals[5], constVals[6], constVals[2], constVals[1] );

	RedMath::SIMD::RedScalar lower3Res = lower3Test.Lower3();
	EXPECT_FLOAT_EQ( lower3Res.X, constVals[1] );
	RedMath::SIMD::RedScalar lower4Res = lower4Test.Lower4();
	EXPECT_FLOAT_EQ( lower4Res.X, constVals[1] );
}

TEST( RedMath, RedVector4_simd_ZeroElement )
{
	RedMath::SIMD::RedVector4 fullOnes( RedMath::SIMD::RedVector4::ONES );
	RedMath::SIMD::RedVector4 xZero( fullOnes.ZeroElement( 0 ) );
	EXPECT_FLOAT_EQ( xZero.X, constVals[0] );
	EXPECT_FLOAT_EQ( xZero.Y, constVals[1] );
	EXPECT_FLOAT_EQ( xZero.Z, constVals[1] );
	EXPECT_FLOAT_EQ( xZero.W, constVals[1] );

	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );

	RedMath::SIMD::RedVector4 yZero( fullOnes.ZeroElement( 1 ) );
	EXPECT_FLOAT_EQ( yZero.X, constVals[1] );
	EXPECT_FLOAT_EQ( yZero.Y, constVals[0] );
	EXPECT_FLOAT_EQ( yZero.Z, constVals[1] );
	EXPECT_FLOAT_EQ( yZero.W, constVals[1] );

	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );

	RedMath::SIMD::RedVector4 zZero( fullOnes.ZeroElement( 2 ) );
	EXPECT_FLOAT_EQ( zZero.X, constVals[1] );
	EXPECT_FLOAT_EQ( zZero.Y, constVals[1] );
	EXPECT_FLOAT_EQ( zZero.Z, constVals[0] );
	EXPECT_FLOAT_EQ( zZero.W, constVals[1] );

	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );

	RedMath::SIMD::RedVector4 wZero( fullOnes.ZeroElement( 3 ) );
	EXPECT_FLOAT_EQ( wZero.X, constVals[1] );
	EXPECT_FLOAT_EQ( wZero.Y, constVals[1] );
	EXPECT_FLOAT_EQ( wZero.Z, constVals[1] );
	EXPECT_FLOAT_EQ( wZero.W, constVals[0] );

	EXPECT_FLOAT_EQ( fullOnes.X, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Y, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.Z, constVals[1] );
	EXPECT_FLOAT_EQ( fullOnes.W, constVals[1] );
}

TEST( RedMath, RedVector4_simd_AsScalar )
{
	RedMath::SIMD::RedVector4 sourceVec( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::SIMD::RedScalar result( sourceVec.AsScalar( 0 ) );
	EXPECT_FLOAT_EQ( result.X, constVals[1] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );

	result = sourceVec.AsScalar( 1 );
	EXPECT_FLOAT_EQ( result.X, constVals[2] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );

	result = sourceVec.AsScalar( 2 );
	EXPECT_FLOAT_EQ( result.X, constVals[3] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );

	result = sourceVec.AsScalar( 3 );
	EXPECT_FLOAT_EQ( result.X, constVals[4] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );
}

TEST( RedMath, RedVector4_simd_AsVector3 )
{
	RedMath::SIMD::RedVector4 sourceVec( constVals[1], constVals[2], constVals[3], constVals[4] );
	RedMath::SIMD::RedVector3 result( sourceVec.AsVector3() );

	EXPECT_FLOAT_EQ( result.X, constVals[1] );
	EXPECT_FLOAT_EQ( result.Y, constVals[2] );
	EXPECT_FLOAT_EQ( result.Z, constVals[3] );
	EXPECT_FLOAT_EQ( result.W, constVals[1] );

	EXPECT_FLOAT_EQ( sourceVec.X, constVals[1] );
	EXPECT_FLOAT_EQ( sourceVec.Y, constVals[2] );
	EXPECT_FLOAT_EQ( sourceVec.Z, constVals[3] );
	EXPECT_FLOAT_EQ( sourceVec.W, constVals[4] );
}

TEST( RedMath, RedVector4_simd_IsOK )
{
	RedMath::SIMD::RedVector4 notOk( RED_NAN, RED_NEG_NAN, RED_INFINITY, RED_NEG_INFINITY );
	RedMath::SIMD::RedVector4 isOk( constVals[0], constVals[1], constVals[0], constVals[2] );

	EXPECT_FALSE( notOk.IsOk() );
	EXPECT_TRUE( isOk.IsOk() );

	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.X ) );
	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.Y ) );
	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.Z ) );
	EXPECT_FALSE( Red::Math::NumericalUtils::IsFinite( notOk.W ) );
}

TEST( RedMath, RedVector4_simd_Lerp )
{
	RedMath::SIMD::RedVector4 v1(1.f, 2.f, 3.f, -1.f);
	RedMath::SIMD::RedVector4 v2(1.f, 0.f, 4.f, 1.f);

	RedMath::SIMD::RedVector4 v3 = RedVector4::Lerp(v1, v2, 0.5f);

	EXPECT_FLOAT_EQ( v3.X, 1.f );
	EXPECT_FLOAT_EQ( v3.Y, 1.f );
	EXPECT_FLOAT_EQ( v3.Z, 3.5f );
	EXPECT_FLOAT_EQ( v3.W, 0.f );

	v1.Lerp(v2, 0.5f);

	EXPECT_FLOAT_EQ( v1.X, 1.f );
	EXPECT_FLOAT_EQ( v1.Y, 1.f );
	EXPECT_FLOAT_EQ( v1.Z, 3.5f );
	EXPECT_FLOAT_EQ( v1.W, 0.f );
}


TEST( RedMath, RedVector4_simd_Distance )
{
	RedMath::SIMD::RedVector4 v1(constVals[1], constVals[2], constVals[3], constVals[4]);
	RedMath::SIMD::RedVector4 v2(-constVals[1], -constVals[2], -constVals[3], -constVals[4]);

	RedMath::SIMD::RedScalar res = v1.DistanceSquaredTo(v2);
	EXPECT_FLOAT_EQ( res, 4.f * (constVals[1] * constVals[1] + constVals[2] * constVals[2] + constVals[3] * constVals[3]));

	res = v1.DistanceTo2D(v2);
	EXPECT_FLOAT_EQ( res, 2.f * Red::Math::MSqrt(constVals[1] * constVals[1] + constVals[2] * constVals[2]));

	res = v1.DistanceTo(v2);
	EXPECT_FLOAT_EQ( res, 2.f * Red::Math::MSqrt(constVals[1] * constVals[1] + constVals[2] * constVals[2] + constVals[3] * constVals[3]));
}

TEST( RedMath, RedVector4_simd_Rotate )
{
	// void RedVector4::RotateDirection( const RedQuaternion& _quat, const RedVector4& _direction )
	// void RedVector4::InverseRotateDirection( const RedQuaternion& _quat, const RedVector4& _v )
	// void RedVector4::InverseRotateDirection( const RedMatrix3x3& _m, const RedVector4& _v )
}

TEST( RedMath, RedVector4_simd_TransformPos )
{
	// RedVector4& RedVector4::SetTransformedPos( const RedQsTransform& _trans, const RedVector4& _v )
	// void RedVector4::SetTransformedInversePos(const RedQsTransform& _t, const RedVector4& _v )
}
