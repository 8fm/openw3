#include "build.h"

#include "../../common/redMath/vectorFunctions_float.h"

#include "../../common/redMath/redmathbase.h"

//////////////////////////////////////////////////////////////////////////
// float
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedScalar_float_SquareRoot_Function )
{
	RedMath::FLOAT::RedScalar vA( 4.0f );
	RedMath::FLOAT::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 2.0f );
	EXPECT_FLOAT_EQ( vA.Z, 2.0f );
	EXPECT_FLOAT_EQ( vA.W, 2.0f );

	const RedMath::FLOAT::RedScalar vB( 9.0f );
	RedMath::FLOAT::RedScalar res = RedMath::FLOAT::SqrRoot( vB );
	EXPECT_FLOAT_EQ( res.X, 3.0f );
	EXPECT_FLOAT_EQ( res.Y, 3.0f );
	EXPECT_FLOAT_EQ( res.Z, 3.0f );
	EXPECT_FLOAT_EQ( res.W, 3.0f );
}

TEST( RedMath, RedVector2_float_SquareRoot_Function )
{
	RedMath::FLOAT::RedVector2 vA( 4.0f, 9.0f );
	RedMath::FLOAT::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 3.0f );
	EXPECT_FLOAT_EQ( vA.Z, 2.0f );
	EXPECT_FLOAT_EQ( vA.W, 3.0f );

	const RedMath::FLOAT::RedVector2 vB( 16.0f, 25.0f );
	RedMath::FLOAT::RedVector2 res = RedMath::FLOAT::SqrRoot( vB );
	EXPECT_FLOAT_EQ( res.X, 4.0f );
	EXPECT_FLOAT_EQ( res.Y, 5.0f );
	EXPECT_FLOAT_EQ( res.Z, 4.0f );
	EXPECT_FLOAT_EQ( res.W, 5.0f );
}

TEST( RedMath, RedVector3_float_SquareRoot_Function )
{
	RedMath::FLOAT::RedVector3 vA( 4.0f, 9.0f, 16.0f );
	RedMath::FLOAT::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 3.0f );
	EXPECT_FLOAT_EQ( vA.Z, 4.0f );
	EXPECT_FLOAT_EQ( vA.W, 1.0f );

	const RedMath::FLOAT::RedVector3 vB( 25.0f, 36.0f, 49.0f );
	RedMath::FLOAT::RedVector3 res = RedMath::FLOAT::SqrRoot( vB );
	
	EXPECT_FLOAT_EQ( res.X, 5.0f );
	EXPECT_FLOAT_EQ( res.Y, 6.0f );
	EXPECT_FLOAT_EQ( res.Z, 7.0f );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}

TEST( RedMath, RedVector4_float_SquareRoot_Function )
{
	RedMath::FLOAT::RedVector4 vA( 4.0f, 9.0f, 16.0f, 25.0f );
	RedMath::FLOAT::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 3.0f );
	EXPECT_FLOAT_EQ( vA.Z, 4.0f );
	EXPECT_FLOAT_EQ( vA.W, 5.0f );

	const RedMath::FLOAT::RedVector4 vB( 36.0f, 49.0f, 64.0f, 81.0f );
	RedMath::FLOAT::RedVector4 res = RedMath::FLOAT::SqrRoot( vB );

	EXPECT_FLOAT_EQ( res.X, 6.0f );
	EXPECT_FLOAT_EQ( res.Y, 7.0f );
	EXPECT_FLOAT_EQ( res.Z, 8.0f );
	EXPECT_FLOAT_EQ( res.W, 9.0f );

	RedMath::FLOAT::RedVector4 vC( 4.0f, 9.0f, 16.0f, 2.0f );
	RedMath::FLOAT::SqrRoot3( vC );
	EXPECT_FLOAT_EQ( vC.X, 2.0f );
	EXPECT_FLOAT_EQ( vC.Y, 3.0f );
	EXPECT_FLOAT_EQ( vC.Z, 4.0f );
	EXPECT_FLOAT_EQ( vC.W, 2.0f );

	const RedMath::FLOAT::RedVector4 vD( 25.0f, 36.0f, 49.0f, 2.0f );
	res = RedMath::FLOAT::SqrRoot3( vD );
	EXPECT_FLOAT_EQ( res.X, 5.0f );
	EXPECT_FLOAT_EQ( res.Y, 6.0f );
	EXPECT_FLOAT_EQ( res.Z, 7.0f );
	EXPECT_FLOAT_EQ( res.W, 2.0f );
}

TEST( RedMath, RedScalar_float_Reciprocal_SquareRoot_Function )
{
	RedMath::FLOAT::RedScalar vA( 4.0f );
	RedMath::FLOAT::RSqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 0.5f );
	EXPECT_FLOAT_EQ( vA.Y, 0.5f );
	EXPECT_FLOAT_EQ( vA.Z, 0.5f );
	EXPECT_FLOAT_EQ( vA.W, 0.5f );

	const RedMath::FLOAT::RedScalar vB( 9.0f );
	RedMath::FLOAT::RedScalar res = RedMath::FLOAT::RSqrRoot( vB );
	EXPECT_FLOAT_EQ( res.X, 0.333333333f );
	EXPECT_FLOAT_EQ( res.Y, 0.333333333f );
	EXPECT_FLOAT_EQ( res.Z, 0.333333333f );
	EXPECT_FLOAT_EQ( res.W, 0.333333333f );
}

TEST( RedMath, RedVector2_float_Reciprocal_SquareRoot_Function )
{
	RedMath::FLOAT::RedVector2 vA( 4.0f, 9.0f );
	RedMath::FLOAT::RSqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 0.5f );
	EXPECT_FLOAT_EQ( vA.Y, 0.333333333f );
	EXPECT_FLOAT_EQ( vA.Z, 0.5f );
	EXPECT_FLOAT_EQ( vA.W, 0.333333333f );

	const RedMath::FLOAT::RedVector2 vB( 16.0f, 25.0f );
	RedMath::FLOAT::RedVector2 res = RedMath::FLOAT::RSqrRoot( vB );
	EXPECT_FLOAT_EQ( res.X, 0.25f );
	EXPECT_FLOAT_EQ( res.Y, 0.2f );
	EXPECT_FLOAT_EQ( res.Z, 0.25f );
	EXPECT_FLOAT_EQ( res.W, 0.2f );
}

TEST( RedMath, RedVector3_float_Reciprocal_SquareRoot_Function )
{
	RedMath::FLOAT::RedVector3 vA( 4.0f, 9.0f, 16.0f );
	RedMath::FLOAT::RSqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 0.5f );
	EXPECT_FLOAT_EQ( vA.Y, 0.333333333f );
	EXPECT_FLOAT_EQ( vA.Z, 0.25f );
	EXPECT_FLOAT_EQ( vA.W, 1.0f );

	const RedMath::FLOAT::RedVector3 vB( 25.0f, 36.0f, 49.0f );
	RedMath::FLOAT::RedVector3 res = RedMath::FLOAT::RSqrRoot( vB );

	EXPECT_FLOAT_EQ( res.X, 0.2f );
	EXPECT_FLOAT_EQ( res.Y, 0.166666666f );
	EXPECT_FLOAT_EQ( res.Z, 0.142857149f );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}

TEST( RedMath, RedVector4_float_Reciprocal_SquareRoot_Function )
{
	RedMath::FLOAT::RedVector4 vA( 4.0f, 9.0f, 16.0f, 25.0f );
	RedMath::FLOAT::RSqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 0.5f );
	EXPECT_FLOAT_EQ( vA.Y, 0.333333333f );
	EXPECT_FLOAT_EQ( vA.Z, 0.25f );
	EXPECT_FLOAT_EQ( vA.W, 0.2f );

	const RedMath::FLOAT::RedVector4 vB( 36.0f, 49.0f, 64.0f, 81.0f );
	RedMath::FLOAT::RedVector4 res = RedMath::FLOAT::RSqrRoot( vB );

	EXPECT_FLOAT_EQ( res.X, 0.166666666f );
	EXPECT_FLOAT_EQ( res.Y, 0.142857149f );
	EXPECT_FLOAT_EQ( res.Z, 0.125f );
	EXPECT_FLOAT_EQ( res.W, 0.111111111f );

	RedMath::FLOAT::RedVector4 vC( 4.0f, 9.0f, 16.0f, 2.0f );
	RedMath::FLOAT::RSqrRoot3( vC );
	EXPECT_FLOAT_EQ( vC.X, 0.5f );
	EXPECT_FLOAT_EQ( vC.Y, 0.333333333f );
	EXPECT_FLOAT_EQ( vC.Z, 0.25f );
	EXPECT_FLOAT_EQ( vC.W, 2.0f );

	const RedMath::FLOAT::RedVector4 vD( 25.0f, 36.0f, 49.0f, 2.0f );
	res = RedMath::FLOAT::RSqrRoot3( vD );
	EXPECT_FLOAT_EQ( res.X, 0.2f );
	EXPECT_FLOAT_EQ( res.Y, 0.166666666f );
	EXPECT_FLOAT_EQ( res.Z, 0.142857149f );
	EXPECT_FLOAT_EQ( res.W, 2.0f );
}

TEST( RedMath, RedVector2_float_Dot_Product_Function )
{
	RedMath::FLOAT::RedVector2 valA( 2.0f, 3.0f );
	RedMath::FLOAT::RedVector2 valB( 4.0f, 5.0f );
	RedMath::FLOAT::RedVector2 valZero;
	RedMath::FLOAT::RedScalar res;
	const Red::System::Float expectedRes = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f );
	
	RedMath::FLOAT::Dot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::Dot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::FLOAT::Dot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
	
	res.SetOnes();
	res = RedMath::FLOAT::Dot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

}

TEST( RedMath, RedVector2_float_UnitDot_Product_Function )
{
	RedMath::FLOAT::RedVector2 valA( 2.0f, 3.0f );
	RedMath::FLOAT::RedVector2 valB( 4.0f, 5.0f );
	RedMath::FLOAT::RedVector2 valZero;
	RedMath::FLOAT::RedScalar res;

	RedMath::FLOAT::RedScalar dotProd = RedMath::FLOAT::Dot( valA, valB );
	RedMath::FLOAT::RedScalar length = ( ( 2.0f * 2.0f ) + ( 3.0f * 3.0f ) ) * ( ( 4.0f * 4.0f ) + ( 5.0f * 5.0f ) );
	SqrRoot( length );
	const Red::System::Float expectedRes = RedMath::FLOAT::Div( dotProd, length ).X;

	RedMath::FLOAT::UnitDot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::UnitDot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::FLOAT::UnitDot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector3_float_Dot_Product_Function )
{
	RedMath::FLOAT::RedVector3 valA( 2.0f, 3.0f, 6.0f );
	RedMath::FLOAT::RedVector3 valB( 4.0f, 5.0f, 7.0f );
	RedMath::FLOAT::RedVector3 valZero;
	RedMath::FLOAT::RedScalar res;
	const Red::System::Float expectedRes = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f ) + ( 6.0f * 7.0f );

	RedMath::FLOAT::Dot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::Dot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::FLOAT::Dot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector3_float_UnitDot_Product_Function )
{
	RedMath::FLOAT::RedVector3 valA( 1.0f, 2.0f, 3.0f );
	RedMath::FLOAT::RedVector3 valB( -1.0f, 1.5f, -2.0f );
	RedMath::FLOAT::RedVector3 valZero;
	RedMath::FLOAT::RedScalar res;

	//////////////////////////////////////////////////////////////////////////
	// Ensure that our fast assumption is close to the longer calculation.
	//////////////////////////////////////////////////////////////////////////
	RedMath::FLOAT::RedVector3 normalA( valA.Normalized() );
	RedMath::FLOAT::RedVector3 normalB( valB.Normalized() );
	RedMath::FLOAT::RedScalar dotTest( RedMath::FLOAT::Dot( normalA, normalB ) );

	RedMath::FLOAT::RedScalar dotProd = RedMath::FLOAT::Dot( valA, valB );
	RedMath::FLOAT::RedScalar length = RedMath::FLOAT::Mul( valA.SquareLength(), valB.SquareLength() );
	SqrRoot( length );
	const Red::System::Float expectedRes = RedMath::FLOAT::Div( dotProd, length ).X;
	EXPECT_FLOAT_EQ( dotTest.X, expectedRes );
	//////////////////////////////////////////////////////////////////////////

	RedMath::FLOAT::UnitDot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::UnitDot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::FLOAT::UnitDot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector4_float_Dot_Product_Function )
{
	RedMath::FLOAT::RedVector4 valA( 2.0f, 3.0f, 6.0f, 8.0f );
	RedMath::FLOAT::RedVector4 valB( 4.0f, 5.0f, 7.0f, 9.0f );
	RedMath::FLOAT::RedVector3 vec3( 4.0f, 5.0f, 7.0f );
	RedMath::FLOAT::RedVector3 valZero3;
	RedMath::FLOAT::RedVector4 valZero;
	RedMath::FLOAT::RedScalar res;
	Red::System::Float expectedRes = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f ) + ( 6.0f * 7.0f ) + ( 8.0f * 9.0f );
	Red::System::Float expectVec3Res = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f ) + ( 6.0f * 7.0f );
	
	RedMath::FLOAT::Dot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::FLOAT::Dot( res, valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );

	RedMath::FLOAT::Dot3( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );

	res.SetZeros();
	res = RedMath::FLOAT::Dot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::Dot( valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );

	res.SetZeros();
	res = RedMath::FLOAT::Dot3( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );

	RedMath::FLOAT::Dot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::Dot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::FLOAT::Dot( res, valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::Dot( valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::FLOAT::Dot3( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::Dot3( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector4_float_UnitDot_Product_Function )
{
	RedMath::FLOAT::RedVector4 valA( 2.0f, 3.0f, 4.0f, 7.0f );
	RedMath::FLOAT::RedVector4 valB( 4.0f, 5.0f, 6.0f, 8.0f );
	RedMath::FLOAT::RedVector3 vec3( 4.0f, 5.0f, 6.0f );
	RedMath::FLOAT::RedVector3 valZero3;
	RedMath::FLOAT::RedVector4 valZero;
	RedMath::FLOAT::RedScalar res;

	//////////////////////////////////////////////////////////////////////////
	// Ensure that our fast assumption is close to the longer calculation.
	//////////////////////////////////////////////////////////////////////////
	RedMath::FLOAT::RedVector4 normalA( valA.Normalized4() );
	RedMath::FLOAT::RedVector4 normalB( valB.Normalized4() );
	RedMath::FLOAT::RedVector4 normalAVec3( valA.Normalized3() );
	RedMath::FLOAT::RedVector3 normalVec3( vec3.Normalized() );
	RedMath::FLOAT::RedScalar dotTest( RedMath::FLOAT::Dot( normalA, normalB ) );
	RedMath::FLOAT::RedScalar dotTestVec3( RedMath::FLOAT::Dot( normalAVec3, normalVec3 ) );

	RedMath::FLOAT::RedScalar dotProd = RedMath::FLOAT::Dot( valA, valB );
	RedMath::FLOAT::RedScalar dotProdVec3 = RedMath::FLOAT::Dot( valA, vec3 );
	RedMath::FLOAT::RedScalar length = RedMath::FLOAT::Mul( valA.SquareLength4(), valB.SquareLength4() );
	RedMath::FLOAT::RedScalar lengthVec3 = RedMath::FLOAT::Mul( valA.SquareLength3(), vec3.SquareLength() );
	SqrRoot( length );
	SqrRoot( lengthVec3 );
	const Red::System::Float expectedRes = RedMath::FLOAT::Div( dotProd, length ).X;
	const Red::System::Float expectedResVec3 = RedMath::FLOAT::Div( dotProdVec3, lengthVec3 ).X;
	EXPECT_FLOAT_EQ( dotTest.X, expectedRes );
	EXPECT_FLOAT_EQ( dotTestVec3.X, expectedResVec3 );
	//////////////////////////////////////////////////////////////////////////

	RedMath::FLOAT::UnitDot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::FLOAT::UnitDot( res, valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	RedMath::FLOAT::UnitDot3( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	res.SetZeros();
	res = RedMath::FLOAT::UnitDot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::UnitDot( valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	res.SetZeros();
	res = RedMath::FLOAT::UnitDot3( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	RedMath::FLOAT::UnitDot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::FLOAT::UnitDot( res, valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::UnitDot( valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::FLOAT::UnitDot3( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::UnitDot3( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector2_float_Cross_Product_Function )
{
	RedMath::FLOAT::RedVector2 valA( 0.9f, 0.1f );
	RedMath::FLOAT::RedVector2 valB( 0.1f, -0.9f );
	RedMath::FLOAT::RedScalar res;

	Red::System::Float expectedRes = ( 0.9f * -0.9f ) - ( 0.1f * 0.1f );

	RedMath::FLOAT::Cross( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::Cross( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );
}

TEST( RedMath, RedVector3_float_Cross_Product_Function )
{
	RedMath::FLOAT::RedVector3 valA( 0.9f, 0.1f, 3.0f );
	RedMath::FLOAT::RedVector3 valB( 0.1f, -0.9f, 2.45f );
	RedMath::FLOAT::RedVector3 res;

	Red::System::Float xVal = ( 0.1f * 2.45f ) - ( 3.0f * -0.9f );
	Red::System::Float yVal = ( 3.0f * 0.1f ) - ( 0.9f * 2.45f );
	Red::System::Float zVal = ( 0.9f * -0.9f ) - ( 0.1f * 0.1f );

	RedMath::FLOAT::Cross( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	res.SetZeros();
	res = RedMath::FLOAT::Cross( valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}

TEST( RedMath, RedVector4_float_Cross_Product_Function )
{
	RedMath::FLOAT::RedVector4 valA( 0.9f, 0.1f, 3.0f, 3.0f );
	RedMath::FLOAT::RedVector4 valB( 0.1f, -0.9f, 2.45f, 4.0f );
	RedMath::FLOAT::RedVector3 vec3( 0.1f, -0.9f, 2.45f );
	RedMath::FLOAT::RedVector4 res;

	Red::System::Float xVal = ( 0.1f * 2.45f ) - ( 3.0f * -0.9f );
	Red::System::Float yVal = ( 3.0f * 0.1f ) - ( 0.9f * 2.45f );
	Red::System::Float zVal = ( 0.9f * -0.9f ) - ( 0.1f * 0.1f );

	RedMath::FLOAT::Cross( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	RedMath::FLOAT::Cross( res, valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	res.SetZeros();
	res = RedMath::FLOAT::Cross( valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	res.SetZeros();
	res = RedMath::FLOAT::Cross( valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}

//////////////////////////////////////////////////////////////////////////
// simd
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedScalar_simd_SquareRoot_Function )
{
	RedMath::SIMD::RedScalar vA( 4.0f );
	RedMath::SIMD::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 2.0f );
	EXPECT_FLOAT_EQ( vA.Z, 2.0f );
	EXPECT_FLOAT_EQ( vA.W, 2.0f );

	const RedMath::SIMD::RedScalar vB( 9.0f );
	RedMath::SIMD::RedScalar res = RedMath::SIMD::SqrRoot( vB );
	EXPECT_FLOAT_EQ( res.X, 3.0f );
	EXPECT_FLOAT_EQ( res.Y, 3.0f );
	EXPECT_FLOAT_EQ( res.Z, 3.0f );
	EXPECT_FLOAT_EQ( res.W, 3.0f );
}

TEST( RedMath, RedVector2_simd_SquareRoot_Function )
{
	RedMath::SIMD::RedVector2 vA( 4.0f, 9.0f );
	RedMath::SIMD::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 3.0f );
	EXPECT_FLOAT_EQ( vA.Z, 2.0f );
	EXPECT_FLOAT_EQ( vA.W, 3.0f );

	const RedMath::SIMD::RedVector2 vB( 16.0f, 25.0f );
	RedMath::SIMD::RedVector2 res = RedMath::SIMD::SqrRoot( vB );
	EXPECT_FLOAT_EQ( res.X, 4.0f );
	EXPECT_FLOAT_EQ( res.Y, 5.0f );
	EXPECT_FLOAT_EQ( res.Z, 4.0f );
	EXPECT_FLOAT_EQ( res.W, 5.0f );
}

TEST( RedMath, RedVector3_simd_SquareRoot_Function )
{
	RedMath::SIMD::RedVector3 vA( 4.0f, 9.0f, 16.0f );
	RedMath::SIMD::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 3.0f );
	EXPECT_FLOAT_EQ( vA.Z, 4.0f );
	EXPECT_FLOAT_EQ( vA.W, 1.0f );

	const RedMath::SIMD::RedVector3 vB( 25.0f, 36.0f, 49.0f );
	RedMath::SIMD::RedVector3 res = RedMath::SIMD::SqrRoot( vB );

	EXPECT_FLOAT_EQ( res.X, 5.0f );
	EXPECT_FLOAT_EQ( res.Y, 6.0f );
	EXPECT_FLOAT_EQ( res.Z, 7.0f );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_SquareRoot_Function )
{
	RedMath::SIMD::RedVector4 vA( 4.0f, 9.0f, 16.0f, 25.0f );
	RedMath::SIMD::SqrRoot( vA );
	EXPECT_FLOAT_EQ( vA.X, 2.0f );
	EXPECT_FLOAT_EQ( vA.Y, 3.0f );
	EXPECT_FLOAT_EQ( vA.Z, 4.0f );
	EXPECT_FLOAT_EQ( vA.W, 5.0f );

	const RedMath::SIMD::RedVector4 vB( 36.0f, 49.0f, 64.0f, 81.0f );
	RedMath::SIMD::RedVector4 res = RedMath::SIMD::SqrRoot( vB );

	EXPECT_FLOAT_EQ( res.X, 6.0f );
	EXPECT_FLOAT_EQ( res.Y, 7.0f );
	EXPECT_FLOAT_EQ( res.Z, 8.0f );
	EXPECT_FLOAT_EQ( res.W, 9.0f );

	RedMath::SIMD::RedVector4 vC( 4.0f, 9.0f, 16.0f, 2.0f );
	RedMath::SIMD::SqrRoot3( vC );
	EXPECT_FLOAT_EQ( vC.X, 2.0f );
	EXPECT_FLOAT_EQ( vC.Y, 3.0f );
	EXPECT_FLOAT_EQ( vC.Z, 4.0f );
	EXPECT_FLOAT_EQ( vC.W, 2.0f );

	const RedMath::SIMD::RedVector4 vD( 25.0f, 36.0f, 49.0f, 2.0f );
	res = RedMath::SIMD::SqrRoot3( vD );
	EXPECT_FLOAT_EQ( res.X, 5.0f );
	EXPECT_FLOAT_EQ( res.Y, 6.0f );
	EXPECT_FLOAT_EQ( res.Z, 7.0f );
	EXPECT_FLOAT_EQ( res.W, 2.0f );
}

TEST( RedMath, RedScalar_simd_Reciprocal_SquareRoot_Function )
{
	RedMath::SIMD::RedScalar vA( 4.0f );
	RedMath::SIMD::RSqrRoot( vA );
	EXPECT_NEAR( vA.X, 0.5f, 0.001f );
	EXPECT_NEAR( vA.Y, 0.5f, 0.001f );
	EXPECT_NEAR( vA.Z, 0.5f, 0.001f );
	EXPECT_NEAR( vA.W, 0.5f, 0.001f );

	const RedMath::SIMD::RedScalar vB( 9.0f );
	RedMath::SIMD::RedScalar res = RedMath::SIMD::RSqrRoot( vB );
	EXPECT_NEAR( res.X, 0.333333333f, 0.001f );
	EXPECT_NEAR( res.Y, 0.333333333f, 0.001f );
	EXPECT_NEAR( res.Z, 0.333333333f, 0.001f );
	EXPECT_NEAR( res.W, 0.333333333f, 0.001f );
}

TEST( RedMath, RedVector2_simd_Reciprocal_SquareRoot_Function )
{
	RedMath::SIMD::RedVector2 vA( 4.0f, 9.0f );
	RedMath::SIMD::RSqrRoot( vA );
	EXPECT_NEAR( vA.X, 0.5f, 0.001f );
	EXPECT_NEAR( vA.Y, 0.333333333f, 0.001f );
	EXPECT_NEAR( vA.Z, 0.5f, 0.001f );
	EXPECT_NEAR( vA.W, 0.333333333f, 0.001f );

	const RedMath::SIMD::RedVector2 vB( 16.0f, 25.0f );
	RedMath::SIMD::RedVector2 res = RedMath::SIMD::RSqrRoot( vB );
	EXPECT_NEAR( res.X, 0.25f, 0.001f );
	EXPECT_NEAR( res.Y, 0.2f, 0.001f );
	EXPECT_NEAR( res.Z, 0.25f, 0.001f );
	EXPECT_NEAR( res.W, 0.2f, 0.001f );
}

TEST( RedMath, RedVector3_simd_Reciprocal_SquareRoot_Function )
{
	RedMath::SIMD::RedVector3 vA( 4.0f, 9.0f, 16.0f );
	RedMath::SIMD::RSqrRoot( vA );
	EXPECT_NEAR( vA.X, 0.5f, 0.001f );
	EXPECT_NEAR( vA.Y, 0.333333333f, 0.001f );
	EXPECT_NEAR( vA.Z, 0.25f, 0.001f );
	EXPECT_FLOAT_EQ( vA.W, 1.0f );

	const RedMath::SIMD::RedVector3 vB( 25.0f, 36.0f, 49.0f );
	RedMath::SIMD::RedVector3 res = RedMath::SIMD::RSqrRoot( vB );

	EXPECT_NEAR( res.X, 0.2f, 0.001f );
	EXPECT_NEAR( res.Y, 0.166666666f, 0.001f );
	EXPECT_NEAR( res.Z, 0.142857149f, 0.001f );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_Reciprocal_SquareRoot_Function )
{
	RedMath::SIMD::RedVector4 vA( 4.0f, 9.0f, 16.0f, 25.0f );
	RedMath::SIMD::RSqrRoot( vA );
	EXPECT_NEAR( vA.X, 0.5f, 0.001f );
	EXPECT_NEAR( vA.Y, 0.333333333f, 0.001f );
	EXPECT_NEAR( vA.Z, 0.25f, 0.001f );
	EXPECT_NEAR( vA.W, 0.2f, 0.001f );

	const RedMath::SIMD::RedVector4 vB( 36.0f, 49.0f, 64.0f, 81.0f );
	RedMath::SIMD::RedVector4 res = RedMath::SIMD::RSqrRoot( vB );

	EXPECT_NEAR( res.X, 0.166666666f, 0.001f );
	EXPECT_NEAR( res.Y, 0.142857149f, 0.001f );
	EXPECT_NEAR( res.Z, 0.125f, 0.001f );
	EXPECT_NEAR( res.W, 0.111111111f, 0.001f );

	RedMath::SIMD::RedVector4 vC( 4.0f, 9.0f, 16.0f, 2.0f );
	RedMath::SIMD::RSqrRoot3( vC );
	EXPECT_NEAR( vC.X, 0.5f, 0.001f );
	EXPECT_NEAR( vC.Y, 0.333333333f, 0.001f );
	EXPECT_NEAR( vC.Z, 0.25f, 0.001f );
	EXPECT_FLOAT_EQ( vC.W, 2.0f );

	const RedMath::SIMD::RedVector4 vD( 25.0f, 36.0f, 49.0f, 2.0f );
	res = RedMath::SIMD::RSqrRoot3( vD );
	EXPECT_NEAR( res.X, 0.2f, 0.001f );
	EXPECT_NEAR( res.Y, 0.166666666f, 0.001f );
	EXPECT_NEAR( res.Z, 0.142857149f, 0.001f );
	EXPECT_FLOAT_EQ( res.W, 2.0f );
}

TEST( RedMath, RedVector2_simd_Dot_Product_Function )
{
	RedMath::SIMD::RedVector2 valA( 2.0f, 3.0f );
	RedMath::SIMD::RedVector2 valB( 4.0f, 5.0f );
	RedMath::SIMD::RedScalar res;
	const Red::System::Float expectedRes = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f );

	RedMath::SIMD::Dot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::SIMD::Dot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

}

TEST( RedMath, RedVector2_simd_UnitDot_Product_Function )
{
	RedMath::SIMD::RedVector2 valA( 2.0f, 3.0f );
	RedMath::SIMD::RedVector2 valB( 4.0f, 5.0f );
	RedMath::SIMD::RedVector2 valZero;
	RedMath::SIMD::RedScalar res;

	RedMath::SIMD::RedScalar dotProd = RedMath::SIMD::Dot( valA, valB );
	RedMath::SIMD::RedScalar length = ( ( 2.0f * 2.0f ) + ( 3.0f * 3.0f ) ) * ( ( 4.0f * 4.0f ) + ( 5.0f * 5.0f ) );
	SqrRoot( length );
	const Red::System::Float expectedRes = RedMath::SIMD::Div( dotProd, length );

	RedMath::SIMD::UnitDot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::SIMD::UnitDot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::SIMD::UnitDot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector3_simd_Dot_Product_Function )
{
	RedMath::SIMD::RedVector3 valA( 2.0f, 3.0f, 6.0f );
	RedMath::SIMD::RedVector3 valB( 4.0f, 5.0f, 7.0f );
	RedMath::SIMD::RedVector3 valZero;
	RedMath::SIMD::RedScalar res;
	const Red::System::Float expectedRes = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f ) + ( 6.0f * 7.0f );

	RedMath::SIMD::Dot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::SIMD::Dot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::SIMD::Dot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector3_simd_UnitDot_Product_Function )
{
	RedMath::FLOAT::RedVector3 valA( 1.0f, 2.0f, 3.0f );
	RedMath::FLOAT::RedVector3 valB( -1.0f, 1.5f, -2.0f );
	RedMath::FLOAT::RedVector3 valZero;
	RedMath::FLOAT::RedScalar res;

	//////////////////////////////////////////////////////////////////////////
	// Ensure that our fast assumption is close to the longer calculation.
	//////////////////////////////////////////////////////////////////////////
	RedMath::FLOAT::RedVector3 normalA( valA.Normalized() );
	RedMath::FLOAT::RedVector3 normalB( valB.Normalized() );
	RedMath::FLOAT::RedScalar dotTest( RedMath::FLOAT::Dot( normalA, normalB ) );

	RedMath::FLOAT::RedScalar dotProd = RedMath::FLOAT::Dot( valA, valB );
	RedMath::FLOAT::RedScalar length = RedMath::FLOAT::Mul( valA.SquareLength(), valB.SquareLength() );
	SqrRoot( length );
	const Red::System::Float expectedRes = RedMath::FLOAT::Div( dotProd, length ).X;
	EXPECT_FLOAT_EQ( dotTest.X, expectedRes );
	//////////////////////////////////////////////////////////////////////////

	RedMath::FLOAT::UnitDot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::FLOAT::UnitDot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::FLOAT::UnitDot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::FLOAT::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}


TEST( RedMath, RedVector4_simd_Dot_Product_Function )
{
	RedMath::SIMD::RedVector4 valA( 2.0f, 3.0f, 6.0f, 8.0f );
	RedMath::SIMD::RedVector4 valB( 4.0f, 5.0f, 7.0f, 9.0f );
	RedMath::SIMD::RedVector3 vec3( 4.0f, 5.0f, 7.0f );
	RedMath::SIMD::RedVector4 valZero;
	RedMath::SIMD::RedVector3 valZero3;
	RedMath::SIMD::RedScalar res;
	Red::System::Float expectedRes = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f ) + ( 6.0f * 7.0f ) + ( 8.0f * 9.0f );
	Red::System::Float expectVec3Res = ( 2.0f * 4.0f ) + ( 3.0f * 5.0f ) + ( 6.0f * 7.0f );

	RedMath::SIMD::Dot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::SIMD::Dot( res, valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );

	RedMath::SIMD::Dot3( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );

	res.SetZeros();
	res = RedMath::SIMD::Dot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::SIMD::Dot( valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );


	res.SetZeros();
	res = RedMath::SIMD::Dot3( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Y, expectVec3Res );
	EXPECT_FLOAT_EQ( res.Z, expectVec3Res );
	EXPECT_FLOAT_EQ( res.W, expectVec3Res );


	RedMath::SIMD::Dot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::Dot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::SIMD::Dot( res, valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::Dot( valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::SIMD::Dot3( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::Dot3( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}


TEST( RedMath, RedVector4_simd_UnitDot_Product_Function )
{
	RedMath::SIMD::RedVector4 valA( 2.0f, 3.0f, 4.0f, 7.0f );
	RedMath::SIMD::RedVector4 valB( 4.0f, 5.0f, 6.0f, 8.0f );
	RedMath::SIMD::RedVector3 vec3( 4.0f, 5.0f, 6.0f );
	RedMath::SIMD::RedVector3 valZero3;
	RedMath::SIMD::RedVector4 valZero;
	RedMath::SIMD::RedScalar res;

	//////////////////////////////////////////////////////////////////////////
	// Ensure that our fast assumption is close to the longer calculation.
	//////////////////////////////////////////////////////////////////////////
	RedMath::SIMD::RedVector4 normalA( valA.Normalized4() );
	RedMath::SIMD::RedVector4 normalB( valB.Normalized4() );
	RedMath::SIMD::RedVector4 normalAVec3( valA.Normalized3() );
	RedMath::SIMD::RedVector3 normalVec3( vec3.Normalized() );
	RedMath::SIMD::RedScalar dotTest( RedMath::SIMD::Dot( normalA, normalB ) );
	RedMath::SIMD::RedScalar dotTestVec3( RedMath::SIMD::Dot( normalAVec3, normalVec3 ) );

	RedMath::SIMD::RedScalar dotProd = RedMath::SIMD::Dot( valA, valB );
	RedMath::SIMD::RedScalar dotProdVec3 = RedMath::SIMD::Dot( valA, vec3 );
	RedMath::SIMD::RedScalar length = RedMath::SIMD::Mul( valA.SquareLength4(), valB.SquareLength4() );
	RedMath::SIMD::RedScalar lengthVec3 = RedMath::SIMD::Mul( valA.SquareLength3(), vec3.SquareLength() );
	SqrRoot( length );
	SqrRoot( lengthVec3 );
	const Red::System::Float expectedRes = RedMath::SIMD::Div( dotProd, length );
	const Red::System::Float expectedResVec3 = RedMath::SIMD::Div( dotProdVec3, lengthVec3 );
	EXPECT_FLOAT_EQ( dotTest.X, expectedRes );
	EXPECT_FLOAT_EQ( dotTestVec3.X, expectedResVec3 );
	//////////////////////////////////////////////////////////////////////////

	RedMath::SIMD::UnitDot( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	RedMath::SIMD::UnitDot( res, valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	RedMath::SIMD::UnitDot3( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	res.SetZeros();
	res = RedMath::SIMD::UnitDot( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::SIMD::UnitDot( valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	res.SetZeros();
	res = RedMath::SIMD::UnitDot3( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Y, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.Z, expectedResVec3 );
	EXPECT_FLOAT_EQ( res.W, expectedResVec3 );

	RedMath::SIMD::UnitDot( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::UnitDot( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::SIMD::UnitDot( res, valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::UnitDot( valZero, valZero3 );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	RedMath::SIMD::UnitDot3( res, valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );

	res.SetOnes();
	res = RedMath::SIMD::UnitDot3( valZero, valZero );
	EXPECT_FLOAT_EQ( res.X, 0.0f );
	EXPECT_FLOAT_EQ( res.Y, 0.0f );
	EXPECT_FLOAT_EQ( res.Z, 0.0f );
	EXPECT_FLOAT_EQ( res.W, 0.0f );
}

TEST( RedMath, RedVector2_simd_Cross_Product_Function )
{
	RedMath::SIMD::RedVector2 valA( 0.9f, 0.1f );
	RedMath::SIMD::RedVector2 valB( 0.1f, -0.9f );
	RedMath::SIMD::RedScalar res;

	Red::System::Float expectedRes = ( 0.9f * -0.9f ) - ( 0.1f * 0.1f );

	RedMath::SIMD::Cross( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );

	res.SetZeros();
	res = RedMath::SIMD::Cross( valA, valB );
	EXPECT_FLOAT_EQ( res.X, expectedRes );
	EXPECT_FLOAT_EQ( res.Y, expectedRes );
	EXPECT_FLOAT_EQ( res.Z, expectedRes );
	EXPECT_FLOAT_EQ( res.W, expectedRes );
}

TEST( RedMath, RedVector3_simd_Cross_Product_Function )
{
	RedMath::SIMD::RedVector3 valA( 0.9f, 0.1f, 3.0f );
	RedMath::SIMD::RedVector3 valB( 0.1f, -0.9f, 2.45f );
	RedMath::SIMD::RedVector3 res;

	Red::System::Float xVal = ( 0.1f * 2.45f ) - ( 3.0f * -0.9f );
	Red::System::Float yVal = ( 3.0f * 0.1f ) - ( 0.9f * 2.45f );
	Red::System::Float zVal = ( 0.9f * -0.9f ) - ( 0.1f * 0.1f );

	RedMath::SIMD::Cross( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	res.SetZeros();
	res = RedMath::SIMD::Cross( valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_Cross_Product_Function )
{
	RedMath::SIMD::RedVector4 valA( 0.9f, 0.1f, 3.0f, 3.0f );
	RedMath::SIMD::RedVector4 valB( 0.1f, -0.9f, 2.45f, 4.0f );
	RedMath::SIMD::RedVector3 vec3( 0.1f, -0.9f, 2.45f );
	RedMath::SIMD::RedVector4 res;

	Red::System::Float xVal = ( 0.1f * 2.45f ) - ( 3.0f * -0.9f );
	Red::System::Float yVal = ( 3.0f * 0.1f ) - ( 0.9f * 2.45f );
	Red::System::Float zVal = ( 0.9f * -0.9f ) - ( 0.1f * 0.1f );

	RedMath::SIMD::Cross( res, valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	RedMath::SIMD::Cross( res, valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	res.SetZeros();
	res = RedMath::SIMD::Cross( valA, valB );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );

	res.SetZeros();
	res = RedMath::SIMD::Cross( valA, vec3 );
	EXPECT_FLOAT_EQ( res.X, xVal );
	EXPECT_FLOAT_EQ( res.Y, yVal );
	EXPECT_FLOAT_EQ( res.Z, zVal );
	EXPECT_FLOAT_EQ( res.W, 1.0f );
}