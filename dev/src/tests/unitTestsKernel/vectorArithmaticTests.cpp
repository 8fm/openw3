#include "build.h"

#include "../../common/redMath/vectorArithmetic_float.h"

#include "../../common/redMath/redmathbase.h"

//////////////////////////////////////////////////////////////////////////
// float
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedScalar_float_Add )
{
	RedMath::FLOAT::RedScalar result;
	RedMath::FLOAT::RedScalar valA( 1.0f );
	RedMath::FLOAT::RedScalar valB( 2.0f );
	
	RedMath::FLOAT::Add( result, valA, valB );
	
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );
}

TEST( RedMath, RedVector2_float_Add )
{
	RedMath::FLOAT::RedVector2 result;
	RedMath::FLOAT::RedVector2 valA( 1.0f, 2.0f );
	RedMath::FLOAT::RedVector2 valB( 2.0f, 1.0f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::Add( result, valA, scalar );
	
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 4.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	RedMath::FLOAT::Add( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );
}

TEST( RedMath, RedVector3_float_Add )
{
	RedMath::FLOAT::RedVector3 result;
	RedMath::FLOAT::RedVector3 valA( 1.0f, 2.0f, 3.0f );
	RedMath::FLOAT::RedVector3 valB( 2.0f, 1.0f, 4.0f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::Add( result, valA, scalar );

	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 4.0f );
	EXPECT_FLOAT_EQ( result.Z, 5.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	RedMath::FLOAT::Add( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector4_float_Add )
{
	RedMath::FLOAT::RedVector4 result;
	RedMath::FLOAT::RedVector4 valA( 1.0f, 2.0f, 3.0f, 4.0f );
	RedMath::FLOAT::RedVector4 valB( 2.0f, 1.0f, 4.0f, 2.0f );
	RedMath::FLOAT::RedVector3 vec3( 2.0f, 1.0f, 4.0f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::Add( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 4.0f );
	EXPECT_FLOAT_EQ( result.Z, 5.0f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	RedMath::FLOAT::Add( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	RedMath::FLOAT::Add( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	RedMath::FLOAT::Add3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Add3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );
}

TEST( RedMath, RedScalar_float_SetAdd )
{
	RedMath::FLOAT::RedScalar valA( 1.0f );
	RedMath::FLOAT::RedScalar valB( 2.0f );
	RedMath::FLOAT::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.0f ); 
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 3.0f );
}

TEST( RedMath, RedVector2_float_SetAdd )
{
	RedMath::FLOAT::RedVector2 valA( 1.0f, 2.0f );
	RedMath::FLOAT::RedVector2 valB( 2.0f, 1.5f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );
	RedMath::FLOAT::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.5f ); 
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 3.5f );

	RedMath::FLOAT::SetAdd( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.0f );
	EXPECT_FLOAT_EQ( valA.Y, 4.5f );
	EXPECT_FLOAT_EQ( valA.Z, 4.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.5f );
}

TEST( RedMath, RedVector3_float_SetAdd )
{
	RedMath::FLOAT::RedVector3 valA( 1.0f, 2.0f, 3.0f );
	RedMath::FLOAT::RedVector3 valB( 2.0f, 1.5f, 2.5f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );
	RedMath::FLOAT::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.5f ); 
	EXPECT_FLOAT_EQ( valA.Z, 5.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );

	RedMath::FLOAT::SetAdd( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.0f );
	EXPECT_FLOAT_EQ( valA.Y, 4.5f );
	EXPECT_FLOAT_EQ( valA.Z, 6.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );
}

TEST( RedMath, RedVector4_float_SetAdd )
{
	RedMath::FLOAT::RedVector4 valA( 1.0f, 2.0f, 3.0f, 1.0f );
	RedMath::FLOAT::RedVector4 valB( 2.0f, 1.5f, 2.5f, 1.0f );
	RedMath::FLOAT::RedVector3 vec3( 3.0f, 4.0f, 5.0f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );

	RedMath::FLOAT::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.5f ); 
	EXPECT_FLOAT_EQ( valA.Z, 5.5f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );

	RedMath::FLOAT::SetAdd( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.0f );
	EXPECT_FLOAT_EQ( valA.Y, 4.5f );
	EXPECT_FLOAT_EQ( valA.Z, 6.5f );
	EXPECT_FLOAT_EQ( valA.W, 3.0f );

	RedMath::FLOAT::SetAdd( valA, vec3 );
	EXPECT_FLOAT_EQ( valA.X, 7.0f );
	EXPECT_FLOAT_EQ( valA.Y, 8.5f );
	EXPECT_FLOAT_EQ( valA.Z, 11.5f );
	EXPECT_FLOAT_EQ( valA.W, 0.0f );

	RedMath::FLOAT::SetAdd3( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 10.0f );
	EXPECT_FLOAT_EQ( valA.Z, 14.0f );
	EXPECT_FLOAT_EQ( valA.W, 0.0f );
}

TEST( RedMath, RedScalar_float_Sub )
{
	RedMath::FLOAT::RedScalar result;
	RedMath::FLOAT::RedScalar valA( 2.0f );
	RedMath::FLOAT::RedScalar valB( 1.0f );
	RedMath::FLOAT::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();

	result = RedMath::FLOAT::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector2_float_Sub )
{
	RedMath::FLOAT::RedVector2 result;
	RedMath::FLOAT::RedVector2 valA( 2.0f, 2.0f );
	RedMath::FLOAT::RedVector2 valB( 1.0f, 1.5f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );

	RedMath::FLOAT::Sub( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::FLOAT::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 0.5f );
	
	result.SetZeros();
	result = RedMath::FLOAT::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 0.5f );
}

TEST( RedMath, RedVector3_float_Sub )
{
	RedMath::FLOAT::RedVector3 result;
	RedMath::FLOAT::RedVector3 valA( 2.0f, 2.0f, 2.2f );
	RedMath::FLOAT::RedVector3 valB( 1.0f, 1.5f, 3.3f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );

	RedMath::FLOAT::Sub( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.2f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::FLOAT::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector4_float_Sub )
{
	RedMath::FLOAT::RedVector4 result;
	RedMath::FLOAT::RedVector4 valA( 2.0f, 2.0f, 2.2f, 8.0f );
	RedMath::FLOAT::RedVector4 valB( 1.0f, 1.5f, 3.3f, 2.0f );
	RedMath::FLOAT::RedVector3 vec3( 0.5f, 0.25f, 0.1f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );

	RedMath::FLOAT::Sub( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.2f );
	EXPECT_FLOAT_EQ( result.W, 7.0f );

	RedMath::FLOAT::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );
	
	result.SetZeros();
	RedMath::FLOAT::Sub( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 1.75f );
	EXPECT_FLOAT_EQ( result.Z, 2.1f );
	EXPECT_FLOAT_EQ( result.W, 8.0f );

	result.SetZeros();
	RedMath::FLOAT::Sub3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 8.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );
	
	result.SetZeros();
	result = RedMath::FLOAT::Sub3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 8.0f );
}

TEST( RedMath, RedScalar_float_SetSub )
{
	RedMath::FLOAT::RedScalar valA( 2.0f );
	RedMath::FLOAT::RedScalar valB( 1.0f );

	RedMath::FLOAT::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 1.0f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );
}

TEST( RedMath, RedVector2_float_SetSub )
{
	RedMath::FLOAT::RedVector2 valA( 3.0f, 3.0f );
	RedMath::FLOAT::RedVector2 valB( 1.0f, 1.5f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );

	RedMath::FLOAT::SetSub( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.0f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );

	RedMath::FLOAT::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 0.5f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 0.5f );
}

TEST( RedMath, RedVector3_float_SetSub )
{
	RedMath::FLOAT::RedVector3 valA( 3.0f, 3.0f, 4.0f );
	RedMath::FLOAT::RedVector3 valB( 1.0f, 1.5f, 2.0f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );

	RedMath::FLOAT::SetSub( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );

	RedMath::FLOAT::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 0.5f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );
}

TEST( RedMath, RedVector4_float_SetSub )
{
	RedMath::FLOAT::RedVector4 valA( 3.0f, 3.0f, 4.0f, 6.0f );
	RedMath::FLOAT::RedVector4 valB( 1.0f, 1.5f, 2.0f, 1.0f );
	RedMath::FLOAT::RedVector3 vec3( 0.6f, 0.75f, 1.0f );
	RedMath::FLOAT::RedScalar scalar( 1.0f );

	RedMath::FLOAT::SetSub( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 5.0f );

	RedMath::FLOAT::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 0.5f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.0f );

	RedMath::FLOAT::SetSub( valA, vec3 );
	EXPECT_FLOAT_EQ( valA.X, 0.4f );
	EXPECT_FLOAT_EQ( valA.Y, -0.25f );
	EXPECT_FLOAT_EQ( valA.Z, 0.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.0f );

	RedMath::FLOAT::SetSub3( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, -0.6f );
	EXPECT_FLOAT_EQ( valA.Y, -1.75f );
	EXPECT_FLOAT_EQ( valA.Z, -2.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.0f );
}

TEST( RedMath, RedScalar_float_Mul )
{
	RedMath::FLOAT::RedScalar result;
	RedMath::FLOAT::RedScalar valA( 2.0f );
	RedMath::FLOAT::RedScalar valB( 1.5f );

	RedMath::FLOAT::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );

	result.SetZeros();

	result = RedMath::FLOAT::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );
}

TEST( RedMath, Redvector2_float_Mul )
{
	RedMath::FLOAT::RedVector2 result;
	RedMath::FLOAT::RedVector2 valA( 1.5f, 2.5f );
	RedMath::FLOAT::RedVector2 valB( 2.0f, 4.0f );
	RedMath::FLOAT::RedScalar scalar( 3.0f );

	RedMath::FLOAT::Mul( result, valB, scalar );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 12.0f );
	EXPECT_FLOAT_EQ( result.Z, 6.0f );
	EXPECT_FLOAT_EQ( result.W, 12.0f );

	RedMath::FLOAT::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 10.0f );

	result.SetZeros();

	result = RedMath::FLOAT::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 10.0f );
}

TEST( RedMath, RedVector3_float_Mul )
{
	RedMath::FLOAT::RedVector3 result;
	RedMath::FLOAT::RedVector3 valA( 1.5f, 2.5f, 0.25f );
	RedMath::FLOAT::RedVector3 valB( 2.0f, 4.0f, 5.0f );
	RedMath::FLOAT::RedScalar scalar( 3.0f );

	RedMath::FLOAT::Mul( result, valB, scalar );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 12.0f );
	EXPECT_FLOAT_EQ( result.Z, 15.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::FLOAT::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();

	result = RedMath::FLOAT::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, Redvector4_float_Mul )
{
	RedMath::FLOAT::RedVector4 result;
	RedMath::FLOAT::RedVector4 valA( 1.5f, 2.5f, 0.25f, 5.0f );
	RedMath::FLOAT::RedVector4 valB( 2.0f, 4.0f, 5.0f, 0.5f );
	RedMath::FLOAT::RedVector3 vec3( 2.0f, 4.0f, 5.0f );
	RedMath::FLOAT::RedScalar scalar( 3.0f );

	RedMath::FLOAT::Mul( result, valB, scalar );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 12.0f );
	EXPECT_FLOAT_EQ( result.Z, 15.0f );
	EXPECT_FLOAT_EQ( result.W, 1.5f );

	RedMath::FLOAT::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 2.5f );

	RedMath::FLOAT::Mul( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );

	RedMath::FLOAT::Mul3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 2.5f );

	result.SetZeros();
	result = RedMath::FLOAT::Mul( valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Mul3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );
}

TEST( RedMath, RedScalar_float_SetMul )
{
	RedMath::FLOAT::RedScalar valA( 2.0f );
	RedMath::FLOAT::RedScalar valB( 1.5f );
	
	RedMath::FLOAT::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 3.0f );
}

TEST( RedMath, RedVector2_float_SetMul )
{
	RedMath::FLOAT::RedVector2 valA( 1.5f, 2.5f );
	RedMath::FLOAT::RedVector2 valB( 2.0f, 4.0f );
	RedMath::FLOAT::RedScalar scalar( 3.0f );

	RedMath::FLOAT::SetMul( valB, scalar );
	EXPECT_FLOAT_EQ( valB.X, 6.0f );
	EXPECT_FLOAT_EQ( valB.Y, 12.0f );
	EXPECT_FLOAT_EQ( valB.Z, 6.0f );
	EXPECT_FLOAT_EQ( valB.W, 12.0f );

	RedMath::FLOAT::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 30.0f );
	EXPECT_FLOAT_EQ( valA.Z, 9.0f );
	EXPECT_FLOAT_EQ( valA.W, 30.0f );
}

TEST( RedMath, RedVector3_float_SetMul )
{
	RedMath::FLOAT::RedVector3 valA( 1.5f, 2.5f, 0.25f );
	RedMath::FLOAT::RedVector3 valB( 2.0f, 4.0f, 5.0f );
	RedMath::FLOAT::RedScalar scalar( 3.0f );

	RedMath::FLOAT::SetMul( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.5f );
	EXPECT_FLOAT_EQ( valA.Y, 7.5f );
	EXPECT_FLOAT_EQ( valA.Z, 0.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );

	RedMath::FLOAT::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 30.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );
}

TEST( RedMath, RedVector4_float_SetMul )
{
	RedMath::FLOAT::RedVector4 valA( 1.5f, 2.5f, 0.25f, 0.33f );
	RedMath::FLOAT::RedVector4 valB( 2.0f, 4.0f, 5.0f, 2.0f );
	RedMath::FLOAT::RedVector3 vec3( 2.0f, 4.0f, 5.0f );
	RedMath::FLOAT::RedScalar scalar( 3.0f );

	RedMath::FLOAT::SetMul( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.5f );
	EXPECT_FLOAT_EQ( valA.Y, 7.5f );
	EXPECT_FLOAT_EQ( valA.Z, 0.75f );
	EXPECT_FLOAT_EQ( valA.W, 0.99f );

	RedMath::FLOAT::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 30.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.98f );

	RedMath::FLOAT::SetMul( valA, vec3 );
	EXPECT_FLOAT_EQ( valA.X, 18.0f );
	EXPECT_FLOAT_EQ( valA.Y, 120.0f );
	EXPECT_FLOAT_EQ( valA.Z, 18.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.98f );

	RedMath::FLOAT::SetMul3( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 36.0f );
	EXPECT_FLOAT_EQ( valA.Y, 480.0f );
	EXPECT_FLOAT_EQ( valA.Z, 93.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.98f );
}

TEST( RedMath, RedScalar_float_Div )
{
	RedMath::FLOAT::RedScalar result;
	RedMath::FLOAT::RedScalar valA( 3.0f );
	RedMath::FLOAT::RedScalar valB( 1.5f );

	RedMath::FLOAT::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );	
}

TEST( RedMath, RedVector2_float_Div )
{
	RedMath::FLOAT::RedVector2 result;
	RedMath::FLOAT::RedVector2 valA( 3.0f, 4.0f );
	RedMath::FLOAT::RedVector2 valB( 1.5f, 2.0f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::Div( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.5f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );

	RedMath::FLOAT::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );
}

TEST( RedMath, RedVector3_float_Div )
{
	RedMath::FLOAT::RedVector3 result;
	RedMath::FLOAT::RedVector3 valA( 3.0f, 4.0f, 1.0f );
	RedMath::FLOAT::RedVector3 valB( 1.5f, 2.0f, 0.5f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::Div( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 0.5f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::FLOAT::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector4_float_Div )
{
	RedMath::FLOAT::RedVector4 result;
	RedMath::FLOAT::RedVector4 valA( 3.0f, 4.0f, 1.0f, 1.0f );
	RedMath::FLOAT::RedVector4 valB( 1.5f, 2.0f, 0.5f, 0.25f );
	RedMath::FLOAT::RedVector3 vec3( 0.5f, 0.25f, 0.1f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::Div( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 0.5f );
	EXPECT_FLOAT_EQ( result.W, 0.5f );

	RedMath::FLOAT::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	RedMath::FLOAT::Div( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 16.0f );
	EXPECT_FLOAT_EQ( result.Z, 10.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::FLOAT::Div3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	result = RedMath::FLOAT::Div( valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 16.0f );
	EXPECT_FLOAT_EQ( result.Z, 10.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result = RedMath::FLOAT::Div3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedScalar_float_SetDiv )
{
	RedMath::FLOAT::RedScalar valA( 3.0f );
	RedMath::FLOAT::RedScalar valB( 1.5f );

	RedMath::FLOAT::SetDiv( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.0f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );
}

TEST( RedMath, RedVector2_float_SetDiv )
{
	RedMath::FLOAT::RedVector2 valA( 3.0f, 4.0f );
	RedMath::FLOAT::RedVector2 valB( 5.0f, 6.0f );
	RedMath::FLOAT::RedVector2 valC( 2.5f, 2.0f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::SetDiv( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 1.5f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 1.5f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );

	RedMath::FLOAT::SetDiv( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 2.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 2.0f );
	EXPECT_FLOAT_EQ( valB.W, 3.0f );
}

TEST( RedMath, RedVector3_float_SetDiv )
{
	RedMath::FLOAT::RedVector3 valA( 3.0f, 4.0f, 5.0f );
	RedMath::FLOAT::RedVector3 valB( 5.0f, 6.0f, 2.5f );
	RedMath::FLOAT::RedVector3 valC( 2.5f, 2.0f, 1.25f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::SetDiv( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 1.5f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );

	RedMath::FLOAT::SetDiv( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 2.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 2.0f );
	EXPECT_FLOAT_EQ( valB.W, 1.0f );
}

TEST( RedMath, RedVector4_float_SetDiv )
{
	RedMath::FLOAT::RedVector4 valA( 3.0f, 4.0f, 5.0f, 3.0f );
	RedMath::FLOAT::RedVector4 valB( 5.0f, 6.0f, 2.5f, 5.0f );
	RedMath::FLOAT::RedVector4 valC( 2.5f, 2.0f, 1.25f, 2.5f );
	RedMath::FLOAT::RedVector3 vec3( 0.1f, 0.5f, 0.2f );
	RedMath::FLOAT::RedScalar scalar( 2.0f );

	RedMath::FLOAT::SetDiv( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 1.5f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.5f );

	RedMath::FLOAT::SetDiv( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 2.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 2.0f );
	EXPECT_FLOAT_EQ( valB.W, 2.0f );

	RedMath::FLOAT::SetDiv( valB, vec3 );
	EXPECT_FLOAT_EQ( valB.X, 20.0f );
	EXPECT_FLOAT_EQ( valB.Y, 6.0f );
	EXPECT_FLOAT_EQ( valB.Z, 10.0f );
	EXPECT_FLOAT_EQ( valB.W, 2.0f );

	RedMath::FLOAT::SetDiv3( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 8.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 8.0f );
	EXPECT_FLOAT_EQ( valB.W, 2.0f );

}

//////////////////////////////////////////////////////////////////////////
// simd
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedScalar_simd_Add )
{
	RedMath::SIMD::RedScalar result;
	RedMath::SIMD::RedScalar valA( 1.0f );
	RedMath::SIMD::RedScalar valB( 2.0f );

	RedMath::SIMD::Add( result, valA, valB );

	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );

	result.SetZeros();
	result = RedMath::SIMD::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );
}

TEST( RedMath, RedVector2_simd_Add )
{
	RedMath::SIMD::RedVector2 result;
	RedMath::SIMD::RedVector2 valA( 1.0f, 2.0f );
	RedMath::SIMD::RedVector2 valB( 2.0f, 1.0f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::Add( result, valA, scalar );

	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 4.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	RedMath::SIMD::Add( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );

	result.SetZeros();
	result = RedMath::SIMD::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );
}

TEST( RedMath, RedVector3_simd_Add )
{
	RedMath::SIMD::RedVector3 result;
	RedMath::SIMD::RedVector3 valA( 1.0f, 2.0f, 3.0f );
	RedMath::SIMD::RedVector3 valB( 2.0f, 1.0f, 4.0f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::Add( result, valA, scalar );

	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 4.0f );
	EXPECT_FLOAT_EQ( result.Z, 5.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	RedMath::SIMD::Add( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::SIMD::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_Add )
{
	RedMath::SIMD::RedVector4 result;
	RedMath::SIMD::RedVector4 valA( 1.0f, 2.0f, 3.0f, 4.0f );
	RedMath::SIMD::RedVector4 valB( 2.0f, 1.0f, 4.0f, 2.0f );
	RedMath::SIMD::RedVector3 vec3( 2.0f, 1.0f, 4.0f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::Add( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 4.0f );
	EXPECT_FLOAT_EQ( result.Z, 5.0f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	RedMath::SIMD::Add( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	RedMath::SIMD::Add( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	result = RedMath::SIMD::Add( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	RedMath::SIMD::Add3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	result = RedMath::SIMD::Add3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 7.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );
}

TEST( RedMath, RedScalar_simd_SetAdd )
{
	RedMath::SIMD::RedScalar valA( 1.0f );
	RedMath::SIMD::RedScalar valB( 2.0f );
	RedMath::SIMD::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.0f ); 
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 3.0f );
}

TEST( RedMath, RedVector2_simd_SetAdd )
{
	RedMath::SIMD::RedVector2 valA( 1.0f, 2.0f );
	RedMath::SIMD::RedVector2 valB( 2.0f, 1.5f );
	RedMath::SIMD::RedScalar scalar( 1.0f );
	RedMath::SIMD::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.5f ); 
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 3.5f );

	RedMath::SIMD::SetAdd( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.0f );
	EXPECT_FLOAT_EQ( valA.Y, 4.5f );
	EXPECT_FLOAT_EQ( valA.Z, 4.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.5f );
}

TEST( RedMath, RedVector3_simd_SetAdd )
{
	RedMath::SIMD::RedVector3 valA( 1.0f, 2.0f, 3.0f );
	RedMath::SIMD::RedVector3 valB( 2.0f, 1.5f, 2.5f );
	RedMath::SIMD::RedScalar scalar( 1.0f );
	RedMath::SIMD::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.5f ); 
	EXPECT_FLOAT_EQ( valA.Z, 5.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );

	RedMath::SIMD::SetAdd( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.0f );
	EXPECT_FLOAT_EQ( valA.Y, 4.5f );
	EXPECT_FLOAT_EQ( valA.Z, 6.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_SetAdd )
{
	RedMath::SIMD::RedVector4 valA( 1.0f, 2.0f, 3.0f, 1.0f );
	RedMath::SIMD::RedVector4 valB( 2.0f, 1.5f, 2.5f, 1.0f );
	RedMath::SIMD::RedVector3 vec3( 3.0f, 4.0f, 5.0f );
	RedMath::SIMD::RedScalar scalar( 1.0f );

	RedMath::SIMD::SetAdd( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.5f ); 
	EXPECT_FLOAT_EQ( valA.Z, 5.5f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );

	RedMath::SIMD::SetAdd( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.0f );
	EXPECT_FLOAT_EQ( valA.Y, 4.5f );
	EXPECT_FLOAT_EQ( valA.Z, 6.5f );
	EXPECT_FLOAT_EQ( valA.W, 3.0f );

	RedMath::SIMD::SetAdd( valA, vec3 );
	EXPECT_FLOAT_EQ( valA.X, 7.0f );
	EXPECT_FLOAT_EQ( valA.Y, 8.5f );
	EXPECT_FLOAT_EQ( valA.Z, 11.5f );
	EXPECT_FLOAT_EQ( valA.W, 3.0f );

	RedMath::SIMD::SetAdd3( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 10.0f );
	EXPECT_FLOAT_EQ( valA.Z, 14.0f );
	EXPECT_FLOAT_EQ( valA.W, 0.0f );
}


TEST( RedMath, RedScalar_simd_Sub )
{
	RedMath::SIMD::RedScalar result;
	RedMath::SIMD::RedScalar valA( 2.0f );
	RedMath::SIMD::RedScalar valB( 1.0f );
	RedMath::SIMD::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();

	result = RedMath::SIMD::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f ); 
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector2_simd_Sub )
{
	RedMath::SIMD::RedVector2 result;
	RedMath::SIMD::RedVector2 valA( 2.0f, 2.0f );
	RedMath::SIMD::RedVector2 valB( 1.0f, 1.5f );
	RedMath::SIMD::RedScalar scalar( 1.0f );

	RedMath::SIMD::Sub( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::SIMD::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 0.5f );

	result.SetZeros();
	result = RedMath::SIMD::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, 1.0f );
	EXPECT_FLOAT_EQ( result.W, 0.5f );
}

TEST( RedMath, RedVector3_simd_Sub )
{
	RedMath::SIMD::RedVector3 result;
	RedMath::SIMD::RedVector3 valA( 2.0f, 2.0f, 2.2f );
	RedMath::SIMD::RedVector3 valB( 1.0f, 1.5f, 3.3f );
	RedMath::SIMD::RedScalar scalar( 1.0f );

	RedMath::SIMD::Sub( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.2f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::SIMD::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::SIMD::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_Sub )
{
	RedMath::SIMD::RedVector4 result;
	RedMath::SIMD::RedVector4 valA( 2.0f, 2.0f, 2.2f, 8.0f );
	RedMath::SIMD::RedVector4 valB( 1.0f, 1.5f, 3.3f, 2.0f );
	RedMath::SIMD::RedVector3 vec3( 0.5f, 0.25f, 0.1f );
	RedMath::SIMD::RedScalar scalar( 1.0f );

	RedMath::SIMD::Sub( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 1.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.2f );
	EXPECT_FLOAT_EQ( result.W, 7.0f );

	RedMath::SIMD::Sub( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	RedMath::SIMD::Sub( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 1.75f );
	EXPECT_FLOAT_EQ( result.Z, 2.1f );
	EXPECT_FLOAT_EQ( result.W, 8.0f );

	result.SetZeros();
	RedMath::SIMD::Sub3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 8.0f );

	result.SetZeros();
	result = RedMath::SIMD::Sub( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 6.0f );

	result.SetZeros();
	result = RedMath::SIMD::Sub3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 1.0f );
	EXPECT_FLOAT_EQ( result.Y, 0.5f );
	EXPECT_FLOAT_EQ( result.Z, -1.1f );
	EXPECT_FLOAT_EQ( result.W, 8.0f );
}

TEST( RedMath, RedScalar_simd_SetSub )
{
	RedMath::SIMD::RedScalar valA( 2.0f );
	RedMath::SIMD::RedScalar valB( 1.0f );

	RedMath::SIMD::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 1.0f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );
}

TEST( RedMath, RedVector2_simd_SetSub )
{
	RedMath::SIMD::RedVector2 valA( 3.0f, 3.0f );
	RedMath::SIMD::RedVector2 valB( 1.0f, 1.5f );
	RedMath::SIMD::RedScalar scalar( 1.0f );

	RedMath::SIMD::SetSub( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.0f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );

	RedMath::SIMD::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 0.5f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 0.5f );
}

TEST( RedMath, RedVector3_simd_SetSub )
{
	RedMath::SIMD::RedVector3 valA( 3.0f, 3.0f, 4.0f );
	RedMath::SIMD::RedVector3 valB( 1.0f, 1.5f, 2.0f );
	RedMath::SIMD::RedScalar scalar( 1.0f );

	RedMath::SIMD::SetSub( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );

	RedMath::SIMD::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 0.5f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_SetSub )
{
	RedMath::SIMD::RedVector4 valA( 3.0f, 3.0f, 4.0f, 6.0f );
	RedMath::SIMD::RedVector4 valB( 1.0f, 1.5f, 2.0f, 1.0f );
	RedMath::SIMD::RedVector3 vec3( 0.6f, 0.75f, 1.0f );
	RedMath::SIMD::RedScalar scalar( 1.0f );

	RedMath::SIMD::SetSub( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 5.0f );

	RedMath::SIMD::SetSub( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 1.0f );
	EXPECT_FLOAT_EQ( valA.Y, 0.5f );
	EXPECT_FLOAT_EQ( valA.Z, 1.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.0f );

	RedMath::SIMD::SetSub( valA, vec3 );
	EXPECT_FLOAT_EQ( valA.X, 0.4f );
	EXPECT_FLOAT_EQ( valA.Y, -0.25f );
	EXPECT_FLOAT_EQ( valA.Z, 0.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.0f );

	RedMath::SIMD::SetSub3( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, -0.6f );
	EXPECT_FLOAT_EQ( valA.Y, -1.75f );
	EXPECT_FLOAT_EQ( valA.Z, -2.0f );
	EXPECT_FLOAT_EQ( valA.W, 4.0f );
}

TEST( RedMath, RedScalar_simd_Mul )
{
	RedMath::SIMD::RedScalar result;
	RedMath::SIMD::RedScalar valA( 2.0f );
	RedMath::SIMD::RedScalar valB( 1.5f );

	RedMath::SIMD::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );

	result.SetZeros();

	result = RedMath::SIMD::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 3.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 3.0f );
}

TEST( RedMath, Redvector2_simd_Mul )
{
	RedMath::SIMD::RedVector2 result;
	RedMath::SIMD::RedVector2 valA( 1.5f, 2.5f );
	RedMath::SIMD::RedVector2 valB( 2.0f, 4.0f );
	RedMath::SIMD::RedScalar scalar( 3.0f );

	RedMath::SIMD::Mul( result, valB, scalar );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 12.0f );
	EXPECT_FLOAT_EQ( result.Z, 6.0f );
	EXPECT_FLOAT_EQ( result.W, 12.0f );

	RedMath::SIMD::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 10.0f );

	result.SetZeros();

	result = RedMath::SIMD::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 3.0f );
	EXPECT_FLOAT_EQ( result.W, 10.0f );
}

TEST( RedMath, RedVector3_simd_Mul )
{
	RedMath::SIMD::RedVector3 result;
	RedMath::SIMD::RedVector3 valA( 1.5f, 2.5f, 0.25f );
	RedMath::SIMD::RedVector3 valB( 2.0f, 4.0f, 5.0f );
	RedMath::SIMD::RedScalar scalar( 3.0f );

	RedMath::SIMD::Mul( result, valB, scalar );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 12.0f );
	EXPECT_FLOAT_EQ( result.Z, 15.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::SIMD::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();

	result = RedMath::SIMD::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, Redvector4_simd_Mul )
{
	RedMath::SIMD::RedVector4 result;
	RedMath::SIMD::RedVector4 valA( 1.5f, 2.5f, 0.25f, 5.0f );
	RedMath::SIMD::RedVector4 valB( 2.0f, 4.0f, 5.0f, 0.5f );
	RedMath::SIMD::RedVector3 vec3( 2.0f, 4.0f, 5.0f );
	RedMath::SIMD::RedScalar scalar( 3.0f );

	RedMath::SIMD::Mul( result, valB, scalar );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 12.0f );
	EXPECT_FLOAT_EQ( result.Z, 15.0f );
	EXPECT_FLOAT_EQ( result.W, 1.5f );

	RedMath::SIMD::Mul( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 2.5f );

	RedMath::SIMD::Mul( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );

	RedMath::SIMD::Mul3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );

	result.SetZeros();
	result = RedMath::SIMD::Mul( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 2.5f );

	result.SetZeros();
	result = RedMath::SIMD::Mul( valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );

	result.SetZeros();
	result = RedMath::SIMD::Mul3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 3.0f );
	EXPECT_FLOAT_EQ( result.Y, 10.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.25f );
	EXPECT_FLOAT_EQ( result.W, 5.0f );
}

TEST( RedMath, RedScalar_simd_SetMul )
{
	RedMath::SIMD::RedScalar valA( 2.0f );
	RedMath::SIMD::RedScalar valB( 1.5f );

	RedMath::SIMD::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 3.0f );
	EXPECT_FLOAT_EQ( valA.Y, 3.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.0f );
	EXPECT_FLOAT_EQ( valA.W, 3.0f );
}

TEST( RedMath, RedVector2_simd_SetMul )
{
	RedMath::SIMD::RedVector2 valA( 1.5f, 2.5f );
	RedMath::SIMD::RedVector2 valB( 2.0f, 4.0f );
	RedMath::SIMD::RedScalar scalar( 3.0f );

	RedMath::SIMD::SetMul( valB, scalar );
	EXPECT_FLOAT_EQ( valB.X, 6.0f );
	EXPECT_FLOAT_EQ( valB.Y, 12.0f );
	EXPECT_FLOAT_EQ( valB.Z, 6.0f );
	EXPECT_FLOAT_EQ( valB.W, 12.0f );

	RedMath::SIMD::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 30.0f );
	EXPECT_FLOAT_EQ( valA.Z, 9.0f );
	EXPECT_FLOAT_EQ( valA.W, 30.0f );
}

TEST( RedMath, RedVector3_simd_SetMul )
{
	RedMath::SIMD::RedVector3 valA( 1.5f, 2.5f, 0.25f );
	RedMath::SIMD::RedVector3 valB( 2.0f, 4.0f, 5.0f );
	RedMath::SIMD::RedScalar scalar( 3.0f );

	RedMath::SIMD::SetMul( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.5f );
	EXPECT_FLOAT_EQ( valA.Y, 7.5f );
	EXPECT_FLOAT_EQ( valA.Z, 0.75f );
	EXPECT_FLOAT_EQ( valA.W, 0.0f );

	RedMath::SIMD::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 30.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.75f );
	EXPECT_FLOAT_EQ( valA.W, 0.0f );
}

TEST( RedMath, RedVector4_simd_SetMul )
{
	RedMath::SIMD::RedVector4 valA( 1.5f, 2.5f, 0.25f, 0.33f );
	RedMath::SIMD::RedVector4 valB( 2.0f, 4.0f, 5.0f, 2.0f );
	RedMath::SIMD::RedVector3 vec3( 2.0f, 4.0f, 5.0f );
	RedMath::SIMD::RedScalar scalar( 3.0f );

	RedMath::SIMD::SetMul( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 4.5f );
	EXPECT_FLOAT_EQ( valA.Y, 7.5f );
	EXPECT_FLOAT_EQ( valA.Z, 0.75f );
	EXPECT_FLOAT_EQ( valA.W, 0.99f );

	RedMath::SIMD::SetMul( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 9.0f );
	EXPECT_FLOAT_EQ( valA.Y, 30.0f );
	EXPECT_FLOAT_EQ( valA.Z, 3.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.98f );

	RedMath::SIMD::SetMul( valA, vec3 );
	EXPECT_FLOAT_EQ( valA.X, 18.0f );
	EXPECT_FLOAT_EQ( valA.Y, 120.0f );
	EXPECT_FLOAT_EQ( valA.Z, 18.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.98f );

	RedMath::SIMD::SetMul3( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 36.0f );
	EXPECT_FLOAT_EQ( valA.Y, 480.0f );
	EXPECT_FLOAT_EQ( valA.Z, 93.75f );
	EXPECT_FLOAT_EQ( valA.W, 1.98f );
}

TEST( RedMath, RedScalar_simd_Div )
{
	RedMath::SIMD::RedScalar result;
	RedMath::SIMD::RedScalar valA( 3.0f );
	RedMath::SIMD::RedScalar valB( 1.5f );

	RedMath::SIMD::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );

	result.SetZeros();
	result = RedMath::SIMD::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );	
}

TEST( RedMath, RedVector2_simd_Div )
{
	RedMath::SIMD::RedVector2 result;
	RedMath::SIMD::RedVector2 valA( 3.0f, 4.0f );
	RedMath::SIMD::RedVector2 valB( 1.5f, 2.0f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::Div( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 1.5f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );

	RedMath::SIMD::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );

	result.SetZeros();
	result = RedMath::SIMD::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 2.0f );
}

TEST( RedMath, RedVector3_simd_Div )
{
	RedMath::SIMD::RedVector3 result;
	RedMath::SIMD::RedVector3 valA( 3.0f, 4.0f, 1.0f );
	RedMath::SIMD::RedVector3 valB( 1.5f, 2.0f, 0.5f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::Div( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 0.5f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::SIMD::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::SIMD::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_Div )
{
	RedMath::SIMD::RedVector4 result;
	RedMath::SIMD::RedVector4 valA( 3.0f, 4.0f, 1.0f, 1.0f );
	RedMath::SIMD::RedVector4 valB( 1.5f, 2.0f, 0.5f, 0.25f );
	RedMath::SIMD::RedVector3 vec3( 0.5f, 0.25f, 0.1f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::Div( result, valA, scalar );
	EXPECT_FLOAT_EQ( result.X, 1.5f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 0.5f );
	EXPECT_FLOAT_EQ( result.W, 0.5f );

	RedMath::SIMD::Div( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	RedMath::SIMD::Div( result, valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 16.0f );
	EXPECT_FLOAT_EQ( result.Z, 10.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	RedMath::SIMD::Div3( result, valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result.SetZeros();
	result = RedMath::SIMD::Div( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 4.0f );

	result.SetZeros();
	result = RedMath::SIMD::Div( valA, vec3 );
	EXPECT_FLOAT_EQ( result.X, 6.0f );
	EXPECT_FLOAT_EQ( result.Y, 16.0f );
	EXPECT_FLOAT_EQ( result.Z, 10.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );

	result = RedMath::SIMD::Div3( valA, valB );
	EXPECT_FLOAT_EQ( result.X, 2.0f );
	EXPECT_FLOAT_EQ( result.Y, 2.0f );
	EXPECT_FLOAT_EQ( result.Z, 2.0f );
	EXPECT_FLOAT_EQ( result.W, 1.0f );
}

TEST( RedMath, RedScalar_simd_SetDiv )
{
	RedMath::SIMD::RedScalar valA( 3.0f );
	RedMath::SIMD::RedScalar valB( 1.5f );

	RedMath::SIMD::SetDiv( valA, valB );
	EXPECT_FLOAT_EQ( valA.X, 2.0f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.0f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );
}

TEST( RedMath, RedVector2_simd_SetDiv )
{
	RedMath::SIMD::RedVector2 valA( 3.0f, 4.0f );
	RedMath::SIMD::RedVector2 valB( 5.0f, 6.0f );
	RedMath::SIMD::RedVector2 valC( 2.5f, 2.0f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::SetDiv( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 1.5f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 1.5f );
	EXPECT_FLOAT_EQ( valA.W, 2.0f );

	RedMath::SIMD::SetDiv( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 2.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 2.0f );
	EXPECT_FLOAT_EQ( valB.W, 3.0f );
}

TEST( RedMath, RedVector3_simd_SetDiv )
{
	RedMath::SIMD::RedVector3 valA( 3.0f, 4.0f, 5.0f );
	RedMath::SIMD::RedVector3 valB( 5.0f, 6.0f, 2.5f );
	RedMath::SIMD::RedVector3 valC( 2.5f, 2.0f, 1.25f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::SetDiv( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 1.5f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.0f );

	RedMath::SIMD::SetDiv( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 2.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 2.0f );
	EXPECT_FLOAT_EQ( valB.W, 1.0f );
}

TEST( RedMath, RedVector4_simd_SetDiv )
{
	RedMath::SIMD::RedVector4 valA( 3.0f, 4.0f, 5.0f, 3.0f );
	RedMath::SIMD::RedVector4 valB( 5.0f, 6.0f, 2.5f, 5.0f );
	RedMath::SIMD::RedVector4 valC( 2.5f, 2.0f, 1.25f, 2.5f );
	RedMath::SIMD::RedVector3 vec3( 0.1f, 0.5f, 0.2f );
	RedMath::SIMD::RedScalar scalar( 2.0f );

	RedMath::SIMD::SetDiv( valA, scalar );
	EXPECT_FLOAT_EQ( valA.X, 1.5f );
	EXPECT_FLOAT_EQ( valA.Y, 2.0f );
	EXPECT_FLOAT_EQ( valA.Z, 2.5f );
	EXPECT_FLOAT_EQ( valA.W, 1.5f );

	RedMath::SIMD::SetDiv( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 2.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 2.0f );
	EXPECT_FLOAT_EQ( valB.W, 2.0f );

	RedMath::SIMD::SetDiv( valB, vec3 );
	EXPECT_FLOAT_EQ( valB.X, 20.0f );
	EXPECT_FLOAT_EQ( valB.Y, 6.0f );
	EXPECT_FLOAT_EQ( valB.Z, 10.0f );
	EXPECT_FLOAT_EQ( valB.W, 2.0f );

	RedMath::SIMD::SetDiv3( valB, valC );
	EXPECT_FLOAT_EQ( valB.X, 8.0f );
	EXPECT_FLOAT_EQ( valB.Y, 3.0f );
	EXPECT_FLOAT_EQ( valB.Z, 8.0f );
	EXPECT_FLOAT_EQ( valB.W, 2.0f );
}
