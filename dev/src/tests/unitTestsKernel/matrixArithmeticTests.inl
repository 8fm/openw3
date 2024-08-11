
REDMATH_TEST( RedMath, Multiplication4x4 )
{
	RedMatrix4x4 matA( TestMatrix1 );
	RedMatrix4x4 matB( TestMatrix2 );
	RedMatrix4x4 expectedResult( TestMatrix3 );
	RedMatrix4x4 result;
	Mul( result, matA, matB );
	EXPECT_FLOAT_EQ( result.m00, expectedResult.m00 );
	EXPECT_FLOAT_EQ( result.m01, expectedResult.m01 );
	EXPECT_FLOAT_EQ( result.m02, expectedResult.m02 );
	EXPECT_FLOAT_EQ( result.m03, expectedResult.m03 );
	
	EXPECT_FLOAT_EQ( result.m10, expectedResult.m10 );
	EXPECT_FLOAT_EQ( result.m11, expectedResult.m11 );
	EXPECT_FLOAT_EQ( result.m12, expectedResult.m12 );
	EXPECT_FLOAT_EQ( result.m13, expectedResult.m13 );

	EXPECT_FLOAT_EQ( result.m20, expectedResult.m20 );
	EXPECT_FLOAT_EQ( result.m21, expectedResult.m21 );
	EXPECT_FLOAT_EQ( result.m22, expectedResult.m22 );
	EXPECT_FLOAT_EQ( result.m23, expectedResult.m23 );

	EXPECT_FLOAT_EQ( result.m30, expectedResult.m30 );
	EXPECT_FLOAT_EQ( result.m31, expectedResult.m31 );
	EXPECT_FLOAT_EQ( result.m32, expectedResult.m32 );
	EXPECT_FLOAT_EQ( result.m33, expectedResult.m33 );

	result.SetZeros();
	result = Mul( matA, matB );
	EXPECT_FLOAT_EQ( result.m00, expectedResult.m00 );
	EXPECT_FLOAT_EQ( result.m01, expectedResult.m01 );
	EXPECT_FLOAT_EQ( result.m02, expectedResult.m02 );
	EXPECT_FLOAT_EQ( result.m03, expectedResult.m03 );

	EXPECT_FLOAT_EQ( result.m10, expectedResult.m10 );
	EXPECT_FLOAT_EQ( result.m11, expectedResult.m11 );
	EXPECT_FLOAT_EQ( result.m12, expectedResult.m12 );
	EXPECT_FLOAT_EQ( result.m13, expectedResult.m13 );

	EXPECT_FLOAT_EQ( result.m20, expectedResult.m20 );
	EXPECT_FLOAT_EQ( result.m21, expectedResult.m21 );
	EXPECT_FLOAT_EQ( result.m22, expectedResult.m22 );
	EXPECT_FLOAT_EQ( result.m23, expectedResult.m23 );

	EXPECT_FLOAT_EQ( result.m30, expectedResult.m30 );
	EXPECT_FLOAT_EQ( result.m31, expectedResult.m31 );
	EXPECT_FLOAT_EQ( result.m32, expectedResult.m32 );
	EXPECT_FLOAT_EQ( result.m33, expectedResult.m33 );

	SetMul( matA, matB );
	EXPECT_FLOAT_EQ( matA.m00, expectedResult.m00 );
	EXPECT_FLOAT_EQ( matA.m01, expectedResult.m01 );
	EXPECT_FLOAT_EQ( matA.m02, expectedResult.m02 );
	EXPECT_FLOAT_EQ( matA.m03, expectedResult.m03 );

	EXPECT_FLOAT_EQ( matA.m10, expectedResult.m10 );
	EXPECT_FLOAT_EQ( matA.m11, expectedResult.m11 );
	EXPECT_FLOAT_EQ( matA.m12, expectedResult.m12 );
	EXPECT_FLOAT_EQ( matA.m13, expectedResult.m13 );

	EXPECT_FLOAT_EQ( matA.m20, expectedResult.m20 );
	EXPECT_FLOAT_EQ( matA.m21, expectedResult.m21 );
	EXPECT_FLOAT_EQ( matA.m22, expectedResult.m22 );
	EXPECT_FLOAT_EQ( matA.m23, expectedResult.m23 );

	EXPECT_FLOAT_EQ( matA.m30, expectedResult.m30 );
	EXPECT_FLOAT_EQ( matA.m31, expectedResult.m31 );
	EXPECT_FLOAT_EQ( matA.m32, expectedResult.m32 );
	EXPECT_FLOAT_EQ( matA.m33, expectedResult.m33 );
}

REDMATH_TEST( RedMath, Multiplication3x3 )
{
	RedMatrix3x3 matA( TestMatrix1 );
	RedMatrix3x3 matB( TestMatrix2 );
	RedMatrix3x3 expectedResult( TestMatrix3 );
	RedMatrix3x3 result;
	Mul( result, matA, matB );
	EXPECT_FLOAT_EQ( result.m00, expectedResult.m00 );
	EXPECT_FLOAT_EQ( result.m01, expectedResult.m01 );
	EXPECT_FLOAT_EQ( result.m02, expectedResult.m02 );

	EXPECT_FLOAT_EQ( result.m10, expectedResult.m10 );
	EXPECT_FLOAT_EQ( result.m11, expectedResult.m11 );
	EXPECT_FLOAT_EQ( result.m12, expectedResult.m12 );

	EXPECT_FLOAT_EQ( result.m20, expectedResult.m20 );
	EXPECT_FLOAT_EQ( result.m21, expectedResult.m21 );
	EXPECT_FLOAT_EQ( result.m22, expectedResult.m22 );

	result.SetZeros();
	result = Mul( matA, matB );
	EXPECT_FLOAT_EQ( result.m00, expectedResult.m00 );
	EXPECT_FLOAT_EQ( result.m01, expectedResult.m01 );
	EXPECT_FLOAT_EQ( result.m02, expectedResult.m02 );

	EXPECT_FLOAT_EQ( result.m10, expectedResult.m10 );
	EXPECT_FLOAT_EQ( result.m11, expectedResult.m11 );
	EXPECT_FLOAT_EQ( result.m12, expectedResult.m12 );

	EXPECT_FLOAT_EQ( result.m20, expectedResult.m20 );
	EXPECT_FLOAT_EQ( result.m21, expectedResult.m21 );
	EXPECT_FLOAT_EQ( result.m22, expectedResult.m22 );

	SetMul( matA, matB );
	EXPECT_FLOAT_EQ( matA.m00, expectedResult.m00 );
	EXPECT_FLOAT_EQ( matA.m01, expectedResult.m01 );
	EXPECT_FLOAT_EQ( matA.m02, expectedResult.m02 );

	EXPECT_FLOAT_EQ( matA.m10, expectedResult.m10 );
	EXPECT_FLOAT_EQ( matA.m11, expectedResult.m11 );
	EXPECT_FLOAT_EQ( matA.m12, expectedResult.m12 );

	EXPECT_FLOAT_EQ( matA.m20, expectedResult.m20 );
	EXPECT_FLOAT_EQ( matA.m21, expectedResult.m21 );
	EXPECT_FLOAT_EQ( matA.m22, expectedResult.m22 );
}