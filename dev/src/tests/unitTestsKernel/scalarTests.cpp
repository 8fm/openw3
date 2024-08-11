#include "build.h"

#include "../../common/redMath/redScalar_float.h"

#include "../../common/redMath/redScalar_simd.h"
#include "../../common/redMath/redScalar_simd.inl"


// [0] must be 0.0f.
// [5] must be 0.001f.
// [6] must be 0.0011f.
const Red::System::Float constVals[ 7 ] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 0.001f, 0.0011f };

//////////////////////////////////////////////////////////////////////////
// FLOAT
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedScalar_float_construction )
{
	RedMath::FLOAT::RedScalar noParams;
	EXPECT_FLOAT_EQ( noParams.X, noParams.Y );
	EXPECT_FLOAT_EQ( noParams.X, noParams.Z );
	EXPECT_FLOAT_EQ( noParams.X, noParams.W );
	EXPECT_FLOAT_EQ( noParams.Y, noParams.Z );
	EXPECT_FLOAT_EQ( noParams.Y, noParams.W );
	EXPECT_FLOAT_EQ( noParams.Z, noParams.W );

	EXPECT_FLOAT_EQ( noParams.X, constVals[0] ); 
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] ); 
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] ); 
	EXPECT_FLOAT_EQ( noParams.W, constVals[0] );

	RedMath::FLOAT::RedScalar singleFloat( constVals[1] );
	EXPECT_FLOAT_EQ( singleFloat.X, singleFloat.Y );
	EXPECT_FLOAT_EQ( singleFloat.X, singleFloat.Z );
	EXPECT_FLOAT_EQ( singleFloat.X, singleFloat.W );
	EXPECT_FLOAT_EQ( singleFloat.Y, singleFloat.Z );
	EXPECT_FLOAT_EQ( singleFloat.Y, singleFloat.W );
	EXPECT_FLOAT_EQ( singleFloat.Z, singleFloat.W );

	EXPECT_FLOAT_EQ( singleFloat.X, constVals[1] ); 
	EXPECT_FLOAT_EQ( singleFloat.Y, constVals[1] ); 
	EXPECT_FLOAT_EQ( singleFloat.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( singleFloat.W, constVals[1] );

	// Just in case...
	Red::System::Float fltPtr = constVals[3];
	const RedMath::FLOAT::RedScalar constScalarVal( constVals[2] );
	RedMath::FLOAT::RedScalar movScalarVal( constVals[4] );

	RedMath::FLOAT::RedScalar fromRedScalar( constScalarVal );
	EXPECT_FLOAT_EQ( fromRedScalar.X, fromRedScalar.Y );
	EXPECT_FLOAT_EQ( fromRedScalar.X, fromRedScalar.Z );
	EXPECT_FLOAT_EQ( fromRedScalar.X, fromRedScalar.W );
	EXPECT_FLOAT_EQ( fromRedScalar.Y, fromRedScalar.Z );
	EXPECT_FLOAT_EQ( fromRedScalar.Y, fromRedScalar.W );
	EXPECT_FLOAT_EQ( fromRedScalar.Z, fromRedScalar.W );

	EXPECT_FLOAT_EQ( fromRedScalar.X, constScalarVal.X ); 
	EXPECT_FLOAT_EQ( fromRedScalar.Y, constScalarVal.Y ); 
	EXPECT_FLOAT_EQ( fromRedScalar.Z, constScalarVal.Z ); 
	EXPECT_FLOAT_EQ( fromRedScalar.W, constScalarVal.W );

	RedMath::FLOAT::RedScalar fromFltPtr( &fltPtr );
	EXPECT_FLOAT_EQ( fromFltPtr.X, fromFltPtr.Y );
	EXPECT_FLOAT_EQ( fromFltPtr.X, fromFltPtr.Z );
	EXPECT_FLOAT_EQ( fromFltPtr.X, fromFltPtr.W );
	EXPECT_FLOAT_EQ( fromFltPtr.Y, fromFltPtr.Z );
	EXPECT_FLOAT_EQ( fromFltPtr.Y, fromFltPtr.W );
	EXPECT_FLOAT_EQ( fromFltPtr.Z, fromFltPtr.W );

	EXPECT_FLOAT_EQ( fromFltPtr.X, fltPtr );
	EXPECT_FLOAT_EQ( fromFltPtr.Y, fltPtr );
	EXPECT_FLOAT_EQ( fromFltPtr.Z, fltPtr );
	EXPECT_FLOAT_EQ( fromFltPtr.W, fltPtr );
}

TEST( RedMath, RedScalar_float_Operator_Assignment )
{
	const RedMath::FLOAT::RedScalar constScalarVal( constVals[2] );
	RedMath::FLOAT::RedScalar valA = constScalarVal;
	EXPECT_FLOAT_EQ( valA.X, constScalarVal.X );
	EXPECT_FLOAT_EQ( valA.Y, constScalarVal.Y );
	EXPECT_FLOAT_EQ( valA.Z, constScalarVal.Z );
	EXPECT_FLOAT_EQ( valA.W, constScalarVal.W );

	RedMath::FLOAT::RedScalar movScalarVal( constVals[4] );
	RedMath::FLOAT::RedScalar valB = movScalarVal;
	EXPECT_FLOAT_EQ( valB.X, constVals[4] );
	EXPECT_FLOAT_EQ( valB.Y, constVals[4] );
	EXPECT_FLOAT_EQ( valB.Z, constVals[4] );
	EXPECT_FLOAT_EQ( valB.W, constVals[4] );
}

TEST( RedMath, RedScalar_float_AsFloat )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[4] );
	const Red::System::Float* fltPtr = scalarVal.AsFloat();

	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.X );
	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.Y ); 
	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.Z ); 
	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.W );
}

TEST( RedMath, RedScalar_float_Set_Functions )
{
	RedMath::FLOAT::RedScalar scalarVal;
	const RedMath::FLOAT::RedScalar copySetScalarVal( constVals[2] );
	Red::System::Float fltPtr = constVals[3];
	
	scalarVal.Set( constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[1] );

	scalarVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( scalarVal.X, copySetScalarVal.X );
	EXPECT_FLOAT_EQ( scalarVal.Y, copySetScalarVal.Y ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, copySetScalarVal.Z );
	EXPECT_FLOAT_EQ( scalarVal.W, copySetScalarVal.W );

	scalarVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[3] ); 
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[3] ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[3] ); 
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[3] );

	scalarVal.SetZeros();
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[0] ); 
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[0] ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[0] ); 
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[0] );

	scalarVal.SetOnes();
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[1] );

}

TEST( RedMath, RedScalar_float_Negate_Functions )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[1] );
	RedMath::FLOAT::RedScalar scalarValInv( -constVals[1] );
	RedMath::FLOAT::RedScalar scalarValNegate( scalarVal );
	scalarValNegate.Negate();
	EXPECT_FLOAT_EQ( scalarValNegate.X, -constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.Y, -constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.Z, -constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.W, -constVals[1] );

	scalarValNegate.Negate();
	EXPECT_FLOAT_EQ( scalarValNegate.X, constVals[1] );
	EXPECT_FLOAT_EQ( scalarValNegate.Y, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.W, constVals[1] );

	RedMath::FLOAT::RedScalar negatedScalarVal( scalarVal.Negated() );
	EXPECT_FLOAT_EQ( negatedScalarVal.X, -constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarVal.Y, -constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarVal.Z, -constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarVal.W, -constVals[1] );

	RedMath::FLOAT::RedScalar negatedScalarValInv( scalarValInv.Negated() );
	EXPECT_FLOAT_EQ( negatedScalarValInv.X, constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarValInv.Y, constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarValInv.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarValInv.W, constVals[1] );
}

TEST( RedMath, RedScalar_float_Abs )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[2] );
	scalarVal.Negate();
	EXPECT_FLOAT_EQ( scalarVal.X, -constVals[2] );
	EXPECT_FLOAT_EQ( scalarVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( scalarVal.Z, -constVals[2] );
	EXPECT_FLOAT_EQ( scalarVal.W, -constVals[2] );

	RedMath::FLOAT::RedScalar absVal( scalarVal.Abs() );
	EXPECT_FLOAT_EQ( absVal.X, constVals[2] );
	EXPECT_FLOAT_EQ( absVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( absVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( absVal.W, constVals[2] );
}

TEST( RedMath, RedScalar_float_IsZero )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[0] );
	Red::System::Bool result = scalarVal.IsZero();
	EXPECT_TRUE( result );

	scalarVal.SetOnes();
	result = scalarVal.IsZero();
	EXPECT_FALSE( result );
}

TEST( RedMath, RedScalar_float_IsAlmostZero )
{
	RedMath::FLOAT::RedScalar scalarVal( constVals[5] );
	Red::System::Bool result = scalarVal.IsAlmostZero( constVals[6] );
	EXPECT_TRUE( result );

	scalarVal.Negate();
	result = scalarVal.IsAlmostZero( constVals[6] );
	EXPECT_TRUE( result );

	scalarVal.SetOnes();
	result = scalarVal.IsAlmostZero( constVals[6] );
	EXPECT_FALSE( result );
}

//////////////////////////////////////////////////////////////////////////
// SIMD
//////////////////////////////////////////////////////////////////////////
TEST( RedMath, RedScalar_SIMD_construction )
{
	RedMath::SIMD::RedScalar noParams;
	EXPECT_FLOAT_EQ( noParams.X, noParams.Y );
	EXPECT_FLOAT_EQ( noParams.X, noParams.Z );
	EXPECT_FLOAT_EQ( noParams.X, noParams.W );
	EXPECT_FLOAT_EQ( noParams.Y, noParams.Z );
	EXPECT_FLOAT_EQ( noParams.Y, noParams.W );
	EXPECT_FLOAT_EQ( noParams.Z, noParams.W );

	EXPECT_FLOAT_EQ( noParams.X, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Y, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.Z, constVals[0] );
	EXPECT_FLOAT_EQ( noParams.W, constVals[0] );

	RedMath::SIMD::RedScalar singleFloat( constVals[1] );
	EXPECT_FLOAT_EQ( singleFloat.X, singleFloat.Y );
	EXPECT_FLOAT_EQ( singleFloat.X, singleFloat.Z );
	EXPECT_FLOAT_EQ( singleFloat.X, singleFloat.W );
	EXPECT_FLOAT_EQ( singleFloat.Y, singleFloat.Z );
	EXPECT_FLOAT_EQ( singleFloat.Y, singleFloat.W );
	EXPECT_FLOAT_EQ( singleFloat.Z, singleFloat.W );
	
	EXPECT_FLOAT_EQ( singleFloat.X, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloat.Y, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloat.Z, constVals[1] );
	EXPECT_FLOAT_EQ( singleFloat.W, constVals[1] );

	RED_ALIGNED_VAR(Red::System::Float, 16) fltPtr[4] = { 3.0f, 2.0f, 1.0f, 4.0f };
	const RedMath::SIMD::RedScalar constScalarVal( constVals[2] );
	RedMath::SIMD::RedScalar movScalarVal( constVals[4] );

	RedMath::SIMD::RedScalar fromRedScalar( constScalarVal );
	EXPECT_FLOAT_EQ( fromRedScalar.X, fromRedScalar.Y );
	EXPECT_FLOAT_EQ( fromRedScalar.X, fromRedScalar.Z );
	EXPECT_FLOAT_EQ( fromRedScalar.X, fromRedScalar.W );
	EXPECT_FLOAT_EQ( fromRedScalar.Y, fromRedScalar.Z );
	EXPECT_FLOAT_EQ( fromRedScalar.Y, fromRedScalar.W );
	EXPECT_FLOAT_EQ( fromRedScalar.Z, fromRedScalar.W );

	EXPECT_FLOAT_EQ( fromRedScalar.X, constVals[2] );
	EXPECT_FLOAT_EQ( fromRedScalar.Y, constVals[2] );
	EXPECT_FLOAT_EQ( fromRedScalar.Z, constVals[2] );
	EXPECT_FLOAT_EQ( fromRedScalar.W, constVals[2] );

	RedMath::SIMD::RedScalar fromFltPtr( fltPtr );
	EXPECT_FLOAT_EQ( fromFltPtr.X, fromFltPtr.Y );
	EXPECT_FLOAT_EQ( fromFltPtr.X, fromFltPtr.Z );
	EXPECT_FLOAT_EQ( fromFltPtr.X, fromFltPtr.W );
	EXPECT_FLOAT_EQ( fromFltPtr.Y, fromFltPtr.Z );
	EXPECT_FLOAT_EQ( fromFltPtr.Y, fromFltPtr.W );
	EXPECT_FLOAT_EQ( fromFltPtr.Z, fromFltPtr.W );

	EXPECT_FLOAT_EQ( fromFltPtr.X, fltPtr[0] );
	EXPECT_FLOAT_EQ( fromFltPtr.Y, fltPtr[0] );
	EXPECT_FLOAT_EQ( fromFltPtr.Z, fltPtr[0] );
	EXPECT_FLOAT_EQ( fromFltPtr.W, fltPtr[0] );
}

TEST( RedMath, RedScalar_SIMD_Operator_Assignment )
{
	const RedMath::SIMD::RedScalar constScalarVal( constVals[2] );
	RedMath::SIMD::RedScalar valA = constScalarVal;
	EXPECT_FLOAT_EQ( valA.X, constScalarVal.X );
	EXPECT_FLOAT_EQ( valA.Y, constScalarVal.Y );
	EXPECT_FLOAT_EQ( valA.Z, constScalarVal.Z );
	EXPECT_FLOAT_EQ( valA.W, constScalarVal.W );

	RedMath::SIMD::RedScalar movScalarVal( constVals[4] );
	RedMath::SIMD::RedScalar valB = movScalarVal;
	EXPECT_FLOAT_EQ( valB.X, constVals[4] );
	EXPECT_FLOAT_EQ( valB.Y, constVals[4] );
	EXPECT_FLOAT_EQ( valB.Z, constVals[4] );
	EXPECT_FLOAT_EQ( valB.W, constVals[4] );
}

TEST( RedMath, RedScalar_SIMD_AsFloat )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[4] );
	const Red::System::Float* fltPtr = scalarVal.AsFloat();

	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.X ); 
	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.Y ); 
	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.Z ); 
	EXPECT_FLOAT_EQ( (*fltPtr), scalarVal.W );
}

TEST( RedMath, RedScalar_SIMD_Set_Functions )
{
	RedMath::SIMD::RedScalar scalarVal;
	const RedMath::SIMD::RedScalar copySetScalarVal( constVals[2] );
	RedMath::SIMD::RedScalar movSetScalarVal( constVals[4] ); 
	Red::System::Float fltPtr = constVals[3];

	scalarVal.Set( constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[1] );
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[1] );

	scalarVal.Set( copySetScalarVal );
	EXPECT_FLOAT_EQ( scalarVal.X, copySetScalarVal.X );
	EXPECT_FLOAT_EQ( scalarVal.Y, copySetScalarVal.Y ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, copySetScalarVal.Z );
	EXPECT_FLOAT_EQ( scalarVal.W, copySetScalarVal.W );

	scalarVal.Set( fltPtr );
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[3] );
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[3] );
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[3] );
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[3] );

	scalarVal.Set( std::move( movSetScalarVal ) );
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[4] ); 
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[4] ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[4] ); 
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[4] );

	scalarVal.SetZeros();
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[0] ); 
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[0] ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[0] ); 
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[0] );

	scalarVal.SetOnes();
	EXPECT_FLOAT_EQ( scalarVal.X, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarVal.Y, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarVal.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarVal.W, constVals[1] );

}

TEST( RedMath, RedScalar_SIMD_Negate_Functions )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[1] );
	RedMath::SIMD::RedScalar scalarValInv( -constVals[1] );
	RedMath::SIMD::RedScalar scalarValNegate( scalarVal );
	scalarValNegate.Negate();
	EXPECT_FLOAT_EQ( scalarValNegate.X, -constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.Y, -constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.Z, -constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.W, -constVals[1] );

	scalarValNegate.Negate();
	EXPECT_FLOAT_EQ( scalarValNegate.X, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.Y, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( scalarValNegate.W, constVals[1] );


	RedMath::SIMD::RedScalar negatedScalarVal( scalarVal.Negated() );
	EXPECT_FLOAT_EQ( negatedScalarVal.X, -constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarVal.Y, -constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarVal.Z, -constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarVal.W, -constVals[1] );

	RedMath::SIMD::RedScalar negatedScalarValInv( scalarValInv.Negated() );
	EXPECT_FLOAT_EQ( negatedScalarValInv.X, constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarValInv.Y, constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarValInv.Z, constVals[1] ); 
	EXPECT_FLOAT_EQ( negatedScalarValInv.W, constVals[1] );
}

TEST( RedMath, RedScalar_SIMD_Abs )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[2] );
	scalarVal.Negate();
	EXPECT_FLOAT_EQ( scalarVal.X, -constVals[2] );
	EXPECT_FLOAT_EQ( scalarVal.Y, -constVals[2] );
	EXPECT_FLOAT_EQ( scalarVal.Z, -constVals[2] );
	EXPECT_FLOAT_EQ( scalarVal.W, -constVals[2] );

	RedMath::SIMD::RedScalar absVal( scalarVal.Abs() );
	EXPECT_FLOAT_EQ( absVal.X, constVals[2] );
	EXPECT_FLOAT_EQ( absVal.Y, constVals[2] );
	EXPECT_FLOAT_EQ( absVal.Z, constVals[2] );
	EXPECT_FLOAT_EQ( absVal.W, constVals[2] );
}

TEST( RedMath, RedScalar_SIMD_IsZero )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[0] );
	Red::System::Bool result = scalarVal.IsZero();
	EXPECT_TRUE( result );
	
	scalarVal.SetOnes();
	result = scalarVal.IsZero();
	EXPECT_FALSE( result );
}

TEST( RedMath, RedScalar_SIMD_IsAlmostZero )
{
	RedMath::SIMD::RedScalar scalarVal( constVals[5] );
	__m128 eps = _mm_set1_ps( constVals[6] );
	Red::System::Bool result = scalarVal.IsAlmostZero( eps );
	EXPECT_TRUE( true );

	scalarVal.Negate();
	result = scalarVal.IsAlmostZero( eps );
	EXPECT_TRUE( result );

	scalarVal.SetOnes();
	result = scalarVal.IsAlmostZero( eps );
	EXPECT_FALSE( result );
}