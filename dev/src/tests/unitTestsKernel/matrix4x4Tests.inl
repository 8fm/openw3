const RedVector3 TestRowA3( 1.0f, 2.0f, 3.0f );
const RedVector3 TestRowB3( 4.0f, 5.0f, 6.0f );
const RedVector3 TestRowC3( 7.0f, 8.0f, 9.0f );

const RedVector4 TestRowA(  1.0f,  2.0f,  3.0f,  4.0f );
const RedVector4 TestRowB(  5.0f,  6.0f,  7.0f,  8.0f );
const RedVector4 TestRowC(  9.0f, 10.0f, 11.0f, 12.0f );
const RedVector4 TestRowD( 13.0f, 14.0f, 15.0f, 16.0f );

REDMATH_TEST( RedMath, Construction )
{
	RedMatrix4x4 noParams;
	EXPECT_FLOAT_EQ( noParams.m00, RedMatrix4x4::IDENTITY.m00 );
	EXPECT_FLOAT_EQ( noParams.m01, RedMatrix4x4::IDENTITY.m01 );
	EXPECT_FLOAT_EQ( noParams.m02, RedMatrix4x4::IDENTITY.m02 );
	EXPECT_FLOAT_EQ( noParams.m03, RedMatrix4x4::IDENTITY.m03 );
	EXPECT_FLOAT_EQ( noParams.m10, RedMatrix4x4::IDENTITY.m10 );
	EXPECT_FLOAT_EQ( noParams.m11, RedMatrix4x4::IDENTITY.m11 );
	EXPECT_FLOAT_EQ( noParams.m12, RedMatrix4x4::IDENTITY.m12 );
	EXPECT_FLOAT_EQ( noParams.m13, RedMatrix4x4::IDENTITY.m13 );
	EXPECT_FLOAT_EQ( noParams.m20, RedMatrix4x4::IDENTITY.m20 );
	EXPECT_FLOAT_EQ( noParams.m21, RedMatrix4x4::IDENTITY.m21 );
	EXPECT_FLOAT_EQ( noParams.m22, RedMatrix4x4::IDENTITY.m22 );
	EXPECT_FLOAT_EQ( noParams.m23, RedMatrix4x4::IDENTITY.m23 );
	EXPECT_FLOAT_EQ( noParams.m30, RedMatrix4x4::IDENTITY.m30 );
	EXPECT_FLOAT_EQ( noParams.m31, RedMatrix4x4::IDENTITY.m31 );
	EXPECT_FLOAT_EQ( noParams.m32, RedMatrix4x4::IDENTITY.m32 );
	EXPECT_FLOAT_EQ( noParams.m33, RedMatrix4x4::IDENTITY.m33 );

	RedMatrix4x4 fromVector3s( TestRowA3, TestRowB3, TestRowC3 );
	EXPECT_FLOAT_EQ( fromVector3s.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( fromVector3s.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( fromVector3s.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( fromVector3s.m03, 0.0f );
	EXPECT_FLOAT_EQ( fromVector3s.m10, TestRowB3.X );
	EXPECT_FLOAT_EQ( fromVector3s.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( fromVector3s.m12, TestRowB3.Z );
	EXPECT_FLOAT_EQ( fromVector3s.m13, 0.0f );
	EXPECT_FLOAT_EQ( fromVector3s.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( fromVector3s.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( fromVector3s.m22, TestRowC3.Z );
	EXPECT_FLOAT_EQ( fromVector3s.m23, 0.0f );
	EXPECT_FLOAT_EQ( fromVector3s.m30, RedMatrix4x4::IDENTITY.m30 );
	EXPECT_FLOAT_EQ( fromVector3s.m31, RedMatrix4x4::IDENTITY.m31 );
	EXPECT_FLOAT_EQ( fromVector3s.m32, RedMatrix4x4::IDENTITY.m32 );
	EXPECT_FLOAT_EQ( fromVector3s.m33, RedMatrix4x4::IDENTITY.m33 );

	RedMatrix4x4 fromVector4s( TestRowA, TestRowB, TestRowC, TestRowD );
	EXPECT_FLOAT_EQ( fromVector4s.m00, TestRowA.X );
	EXPECT_FLOAT_EQ( fromVector4s.m01, TestRowA.Y );
	EXPECT_FLOAT_EQ( fromVector4s.m02, TestRowA.Z );
	EXPECT_FLOAT_EQ( fromVector4s.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( fromVector4s.m10, TestRowB.X );
	EXPECT_FLOAT_EQ( fromVector4s.m11, TestRowB.Y );
	EXPECT_FLOAT_EQ( fromVector4s.m12, TestRowB.Z );
	EXPECT_FLOAT_EQ( fromVector4s.m13, TestRowB.W );
	EXPECT_FLOAT_EQ( fromVector4s.m20, TestRowC.X );
	EXPECT_FLOAT_EQ( fromVector4s.m21, TestRowC.Y );
	EXPECT_FLOAT_EQ( fromVector4s.m22, TestRowC.Z );
	EXPECT_FLOAT_EQ( fromVector4s.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( fromVector4s.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( fromVector4s.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( fromVector4s.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( fromVector4s.m33, TestRowD.W );

	RedMatrix4x4 copy( fromVector3s );
	EXPECT_FLOAT_EQ( copy.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( copy.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( copy.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( copy.m03, 0.0f );
	EXPECT_FLOAT_EQ( copy.m10, TestRowB3.X );
	EXPECT_FLOAT_EQ( copy.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( copy.m12, TestRowB3.Z );
	EXPECT_FLOAT_EQ( copy.m13, 0.0f );
	EXPECT_FLOAT_EQ( copy.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( copy.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( copy.m22, TestRowC3.Z );
	EXPECT_FLOAT_EQ( copy.m23, 0.0f );
	EXPECT_FLOAT_EQ( copy.m30, RedMatrix4x4::IDENTITY.m30 );
	EXPECT_FLOAT_EQ( copy.m31, RedMatrix4x4::IDENTITY.m31 );
	EXPECT_FLOAT_EQ( copy.m32, RedMatrix4x4::IDENTITY.m32 );
	EXPECT_FLOAT_EQ( copy.m33, RedMatrix4x4::IDENTITY.m33 );

	RedMatrix4x4 fromFloats( TestMatrix1 );
	EXPECT_FLOAT_EQ( fromFloats.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( fromFloats.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( fromFloats.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( fromFloats.m03, TestMatrix1[3] );
	EXPECT_FLOAT_EQ( fromFloats.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( fromFloats.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( fromFloats.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( fromFloats.m13, TestMatrix1[7] );
	EXPECT_FLOAT_EQ( fromFloats.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( fromFloats.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( fromFloats.m22, TestMatrix1[10] );
	EXPECT_FLOAT_EQ( fromFloats.m23, TestMatrix1[11] );
	EXPECT_FLOAT_EQ( fromFloats.m30, TestMatrix1[12] );
	EXPECT_FLOAT_EQ( fromFloats.m31, TestMatrix1[13] );
	EXPECT_FLOAT_EQ( fromFloats.m32, TestMatrix1[14] );
	EXPECT_FLOAT_EQ( fromFloats.m33, TestMatrix1[15] );
}

REDMATH_TEST( RedMath, Operators )
{
	RedMatrix4x4 matA( TestMatrix1 );
	RedMatrix4x4 matB = matA;
	EXPECT_FLOAT_EQ( matB.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( matB.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( matB.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( matB.m03, TestMatrix1[3] );
	EXPECT_FLOAT_EQ( matB.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( matB.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( matB.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( matB.m13, TestMatrix1[7] );
	EXPECT_FLOAT_EQ( matB.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( matB.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( matB.m22, TestMatrix1[10] );
	EXPECT_FLOAT_EQ( matB.m23, TestMatrix1[11] );
	EXPECT_FLOAT_EQ( matB.m30, TestMatrix1[12] );
	EXPECT_FLOAT_EQ( matB.m31, TestMatrix1[13] );
	EXPECT_FLOAT_EQ( matB.m32, TestMatrix1[14] );
	EXPECT_FLOAT_EQ( matB.m33, TestMatrix1[15] );
}

REDMATH_TEST( RedMath, AsFloat )
{
	RedMatrix4x4 mat( TestMatrix1 );
	const Red::System::Float* data = mat.AsFloat();

	EXPECT_FLOAT_EQ( data[0], TestMatrix1[0] );
	EXPECT_FLOAT_EQ( data[1], TestMatrix1[1] );
	EXPECT_FLOAT_EQ( data[2], TestMatrix1[2] );
	EXPECT_FLOAT_EQ( data[3], TestMatrix1[3] );
	EXPECT_FLOAT_EQ( data[4], TestMatrix1[4] );
	EXPECT_FLOAT_EQ( data[5], TestMatrix1[5] );
	EXPECT_FLOAT_EQ( data[6], TestMatrix1[6] );
	EXPECT_FLOAT_EQ( data[7], TestMatrix1[7] );
	EXPECT_FLOAT_EQ( data[8], TestMatrix1[8] );
	EXPECT_FLOAT_EQ( data[9], TestMatrix1[9] );
	EXPECT_FLOAT_EQ( data[10], TestMatrix1[10] );
	EXPECT_FLOAT_EQ( data[11], TestMatrix1[11] );
	EXPECT_FLOAT_EQ( data[12], TestMatrix1[12] );
	EXPECT_FLOAT_EQ( data[13], TestMatrix1[13] );
	EXPECT_FLOAT_EQ( data[14], TestMatrix1[14] );
	EXPECT_FLOAT_EQ( data[15], TestMatrix1[15] );
}

REDMATH_TEST( RedMath, SetFunctions )
{
	RedMatrix4x4 mat;
	mat.Set( TestMatrix1 );
	EXPECT_FLOAT_EQ( mat.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( mat.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( mat.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( mat.m03, TestMatrix1[3] );
	EXPECT_FLOAT_EQ( mat.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( mat.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( mat.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( mat.m13, TestMatrix1[7] );
	EXPECT_FLOAT_EQ( mat.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( mat.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( mat.m22, TestMatrix1[10] );
	EXPECT_FLOAT_EQ( mat.m23, TestMatrix1[11] );
	EXPECT_FLOAT_EQ( mat.m30, TestMatrix1[12] );
	EXPECT_FLOAT_EQ( mat.m31, TestMatrix1[13] );
	EXPECT_FLOAT_EQ( mat.m32, TestMatrix1[14] );
	EXPECT_FLOAT_EQ( mat.m33, TestMatrix1[15] );

	mat.Set( TestRowA, TestRowB, TestRowC, TestRowD );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );

	mat.Set( TestRowA3, TestRowB3, TestRowC3 );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( mat.m03, 0.0f );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB3.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB3.Z );
	EXPECT_FLOAT_EQ( mat.m13, 0.0f );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z );
	EXPECT_FLOAT_EQ( mat.m23, 0.0f );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );

	mat.Set33( TestRowA, TestRowB, TestRowC );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z );
	EXPECT_FLOAT_EQ( mat.m03, 0.0f );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z );
	EXPECT_FLOAT_EQ( mat.m13, 0.0f );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z );
	EXPECT_FLOAT_EQ( mat.m23, 0.0f );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );
}

REDMATH_TEST( RedMath, SetZeros )
{
	RedMatrix4x4 mat;
	mat.SetZeros();
	EXPECT_FLOAT_EQ( mat.m00, 0.0f );
	EXPECT_FLOAT_EQ( mat.m01, 0.0f );
	EXPECT_FLOAT_EQ( mat.m02, 0.0f );
	EXPECT_FLOAT_EQ( mat.m03, 0.0f );
	EXPECT_FLOAT_EQ( mat.m10, 0.0f );
	EXPECT_FLOAT_EQ( mat.m11, 0.0f );
	EXPECT_FLOAT_EQ( mat.m12, 0.0f );
	EXPECT_FLOAT_EQ( mat.m13, 0.0f );
	EXPECT_FLOAT_EQ( mat.m20, 0.0f );
	EXPECT_FLOAT_EQ( mat.m21, 0.0f );
	EXPECT_FLOAT_EQ( mat.m22, 0.0f );
	EXPECT_FLOAT_EQ( mat.m23, 0.0f );
	EXPECT_FLOAT_EQ( mat.m30, 0.0f );
	EXPECT_FLOAT_EQ( mat.m31, 0.0f );
	EXPECT_FLOAT_EQ( mat.m32, 0.0f );
	EXPECT_FLOAT_EQ( mat.m33, 0.0f );
}

REDMATH_TEST( RedMath, SetIdentity )
{
	RedMatrix4x4 mat( RedMatrix4x4::ZEROS );
	mat.SetIdentity();

	EXPECT_FLOAT_EQ( mat.m00, RedMatrix4x4::IDENTITY.m00 );
	EXPECT_FLOAT_EQ( mat.m01, RedMatrix4x4::IDENTITY.m01 );
	EXPECT_FLOAT_EQ( mat.m02, RedMatrix4x4::IDENTITY.m02 );
	EXPECT_FLOAT_EQ( mat.m03, RedMatrix4x4::IDENTITY.m03 );
	EXPECT_FLOAT_EQ( mat.m10, RedMatrix4x4::IDENTITY.m10 );
	EXPECT_FLOAT_EQ( mat.m11, RedMatrix4x4::IDENTITY.m11 );
	EXPECT_FLOAT_EQ( mat.m12, RedMatrix4x4::IDENTITY.m12 );
	EXPECT_FLOAT_EQ( mat.m13, RedMatrix4x4::IDENTITY.m13 );
	EXPECT_FLOAT_EQ( mat.m20, RedMatrix4x4::IDENTITY.m20 );
	EXPECT_FLOAT_EQ( mat.m21, RedMatrix4x4::IDENTITY.m21 );
	EXPECT_FLOAT_EQ( mat.m22, RedMatrix4x4::IDENTITY.m22 );
	EXPECT_FLOAT_EQ( mat.m23, RedMatrix4x4::IDENTITY.m23 );
	EXPECT_FLOAT_EQ( mat.m30, RedMatrix4x4::IDENTITY.m30 );
	EXPECT_FLOAT_EQ( mat.m31, RedMatrix4x4::IDENTITY.m31 );
	EXPECT_FLOAT_EQ( mat.m32, RedMatrix4x4::IDENTITY.m32 );
	EXPECT_FLOAT_EQ( mat.m33, RedMatrix4x4::IDENTITY.m33 );
}

REDMATH_TEST( RedMath, SetRowFunctions )
{
	RedMatrix4x4 mat;
	mat.SetRows( TestMatrix1 );
	EXPECT_FLOAT_EQ( mat.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( mat.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( mat.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( mat.m03, TestMatrix1[3] );
	EXPECT_FLOAT_EQ( mat.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( mat.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( mat.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( mat.m13, TestMatrix1[7] );
	EXPECT_FLOAT_EQ( mat.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( mat.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( mat.m22, TestMatrix1[10] );
	EXPECT_FLOAT_EQ( mat.m23, TestMatrix1[11] );
	EXPECT_FLOAT_EQ( mat.m30, TestMatrix1[12] );
	EXPECT_FLOAT_EQ( mat.m31, TestMatrix1[13] );
	EXPECT_FLOAT_EQ( mat.m32, TestMatrix1[14] );
	EXPECT_FLOAT_EQ( mat.m33, TestMatrix1[15] );

	mat.SetRows( TestRowA, TestRowB, TestRowC, TestRowD );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );

	mat.SetRow( 1, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m10, 1.0f );
	EXPECT_FLOAT_EQ( mat.m11, 1.0f );
	EXPECT_FLOAT_EQ( mat.m12, 1.0f );
	EXPECT_FLOAT_EQ( mat.m13, 1.0f );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );
}

REDMATH_TEST( RedMath, GetRowFunctions )
{
	RedMatrix4x4 mat;
	const RedVector4& row = mat.GetRow( 2 );

	EXPECT_FLOAT_EQ( row.X, 0.0f );
	EXPECT_FLOAT_EQ( row.Y, 0.0f );
	EXPECT_FLOAT_EQ( row.Z, 1.0f );
	EXPECT_FLOAT_EQ( row.W, 0.0f );
}

REDMATH_TEST( RedMath, SetColFunctions )
{
	RedMatrix4x4 mat;
	mat.SetCols( TestMatrix1 );
	EXPECT_FLOAT_EQ( mat.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( mat.m10, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( mat.m20, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( mat.m30, TestMatrix1[3] );
	EXPECT_FLOAT_EQ( mat.m01, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( mat.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( mat.m21, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( mat.m31, TestMatrix1[7] );
	EXPECT_FLOAT_EQ( mat.m02, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( mat.m12, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( mat.m22, TestMatrix1[10] );
	EXPECT_FLOAT_EQ( mat.m32, TestMatrix1[11] );
	EXPECT_FLOAT_EQ( mat.m03, TestMatrix1[12] );
	EXPECT_FLOAT_EQ( mat.m13, TestMatrix1[13] );
	EXPECT_FLOAT_EQ( mat.m23, TestMatrix1[14] );
	EXPECT_FLOAT_EQ( mat.m33, TestMatrix1[15] );

	mat.SetCols( TestRowA, TestRowB, TestRowC, TestRowD );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowB.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowC.X );
	EXPECT_FLOAT_EQ( mat.m03, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m10, TestRowA.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowC.Y );
	EXPECT_FLOAT_EQ( mat.m13, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m20, TestRowA.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowB.Z );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m30, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m31, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m32, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );

	mat.SetColumn( 3, RedVector4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowB.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowC.X );
	EXPECT_FLOAT_EQ( mat.m03, 1.0f );
	EXPECT_FLOAT_EQ( mat.m10, TestRowA.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowC.Y );
	EXPECT_FLOAT_EQ( mat.m13, 1.0f );
	EXPECT_FLOAT_EQ( mat.m20, TestRowA.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowB.Z );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z );
	EXPECT_FLOAT_EQ( mat.m23, 1.0f );
	EXPECT_FLOAT_EQ( mat.m30, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m31, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m32, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m33, 1.0f );
}

REDMATH_TEST( RedMath, GetColFunctions )
{
	RedMatrix4x4 mat;
	RedVector4 col = mat.GetColumn( 3 );
	EXPECT_FLOAT_EQ( col.X, 0.0f );
	EXPECT_FLOAT_EQ( col.Y, 0.0f );
	EXPECT_FLOAT_EQ( col.Z, 0.0f );
	EXPECT_FLOAT_EQ( col.W, 1.0f );
}

REDMATH_TEST( RedMath, GetAxis )
{
	RedMatrix4x4 mat;
	const RedVector4& xAxis = mat.GetAxisX();
	EXPECT_FLOAT_EQ( xAxis.X, 1.0f );
	EXPECT_FLOAT_EQ( xAxis.Y, 0.0f );
	EXPECT_FLOAT_EQ( xAxis.Z, 0.0f );
	EXPECT_FLOAT_EQ( xAxis.W, 0.0f );
	const RedVector4& yAxis = mat.GetAxisY();
	EXPECT_FLOAT_EQ( yAxis.X, 0.0f );
	EXPECT_FLOAT_EQ( yAxis.Y, 1.0f );
	EXPECT_FLOAT_EQ( yAxis.Z, 0.0f );
	EXPECT_FLOAT_EQ( yAxis.W, 0.0f );
	const RedVector4& zAxis = mat.GetAxisZ();
	EXPECT_FLOAT_EQ( zAxis.X, 0.0f );
	EXPECT_FLOAT_EQ( zAxis.Y, 0.0f );
	EXPECT_FLOAT_EQ( zAxis.Z, 1.0f );
	EXPECT_FLOAT_EQ( zAxis.W, 0.0f );
}

REDMATH_TEST( RedMath, SetRot )
{
	Red::System::Float expectedCosine = Red::Math::MCos( Deg90 );
	Red::System::Float expectedSine = Red::Math::MSin( Deg90 );

	RedMatrix4x4 mat;
	mat.SetRotX( Deg90 );
	EXPECT_FLOAT_EQ( mat.m00, 1.0f );
	EXPECT_FLOAT_EQ( mat.m01, 0.0f );
	EXPECT_FLOAT_EQ( mat.m02, 0.0f );
	EXPECT_FLOAT_EQ( mat.m10, 0.0f );
	EXPECT_FLOAT_EQ( mat.m11, expectedCosine );
	EXPECT_FLOAT_EQ( mat.m12, expectedSine );
	EXPECT_FLOAT_EQ( mat.m20, 0.0f );
	EXPECT_FLOAT_EQ( mat.m21, -expectedSine );
	EXPECT_FLOAT_EQ( mat.m22, expectedCosine );

	mat.SetRotY( Deg90 );
	EXPECT_FLOAT_EQ( mat.m00, expectedCosine );
	EXPECT_FLOAT_EQ( mat.m01, 0.0f );
	EXPECT_FLOAT_EQ( mat.m02, -expectedSine );
	EXPECT_FLOAT_EQ( mat.m10, 0.0f );
	EXPECT_FLOAT_EQ( mat.m11, 1.0f );
	EXPECT_FLOAT_EQ( mat.m12, 0.0f );
	EXPECT_FLOAT_EQ( mat.m20, expectedSine );
	EXPECT_FLOAT_EQ( mat.m21, 0.0f );
	EXPECT_FLOAT_EQ( mat.m22, expectedCosine );

	mat.SetRotZ( Deg90 );
	EXPECT_FLOAT_EQ( mat.m00, expectedCosine );
	EXPECT_FLOAT_EQ( mat.m01, expectedSine );
	EXPECT_FLOAT_EQ( mat.m02, 0.0f );
	EXPECT_FLOAT_EQ( mat.m10, -expectedSine );
	EXPECT_FLOAT_EQ( mat.m11, expectedCosine );
	EXPECT_FLOAT_EQ( mat.m12, 0.0f );
	EXPECT_FLOAT_EQ( mat.m20, 0.0f );
	EXPECT_FLOAT_EQ( mat.m21, 0.0f );
	EXPECT_FLOAT_EQ( mat.m22, 1.0f );
}

REDMATH_TEST( RedMath, SetScale )
{
	RedMatrix4x4 mat( TestRowA, TestRowB, TestRowC, TestRowD );
	const RedScalar scaleA( 2.0f );
	const Red::System::Float scaleB( 2.5f );
	const RedVector3 scaleC( 3.0f, 4.0f, 5.0f );
	const RedVector4 scaleD( 6.0f, 7.0f, 8.0f, 9.0f );

	mat.SetScale( scaleA );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W * scaleA.X );

	mat.Set( TestRowA, TestRowB, TestRowC, TestRowD );
	mat.SetScale( scaleB );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X * scaleB );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y * scaleB );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z * scaleB );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W * scaleB );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X * scaleB );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y * scaleB );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z * scaleB );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W * scaleB );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X * scaleB );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y * scaleB );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z * scaleB );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W * scaleB );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X * scaleB );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y * scaleB );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z * scaleB );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W * scaleB );

	mat.Set( TestRowA, TestRowB, TestRowC, TestRowD );
	mat.SetScale( scaleC );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X * scaleC.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y * scaleC.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z * scaleC.X );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X * scaleC.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y * scaleC.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z * scaleC.Y );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X * scaleC.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y * scaleC.Z );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z * scaleC.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );

	mat.Set( TestRowA, TestRowB, TestRowC, TestRowD );
	mat.SetScale( scaleD );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X * scaleD.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y * scaleD.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z * scaleD.X );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W * scaleD.X );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X * scaleD.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y * scaleD.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z * scaleD.Y );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W * scaleD.Y );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X * scaleD.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y * scaleD.Z );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z * scaleD.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W * scaleD.Z );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );

	mat.Set( TestRowA, TestRowB, TestRowC, TestRowD );
	mat.SetScale33( scaleD );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X * scaleD.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y * scaleD.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z * scaleD.X );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X * scaleD.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y * scaleD.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z * scaleD.Y );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X * scaleD.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y * scaleD.Z );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z * scaleD.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );
}

REDMATH_TEST( RedMath, SetPreScale )
{
	RedMatrix4x4 mat( TestRowA, TestRowB, TestRowC, TestRowD );
	RedVector4 scaleVal( 1.0f, 2.0f, 3.0f, 4.0f );
	mat.SetPreScale44( scaleVal );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y * scaleVal.Y);
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z * scaleVal.Z);
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );

	mat.Set( TestRowA, TestRowB, TestRowC, TestRowD );
	mat.SetPreScale33( scaleVal );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m03, TestRowA.W );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m13, TestRowB.W );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m23, TestRowC.W );
	EXPECT_FLOAT_EQ( mat.m30, TestRowD.X );
	EXPECT_FLOAT_EQ( mat.m31, TestRowD.Y );
	EXPECT_FLOAT_EQ( mat.m32, TestRowD.Z );
	EXPECT_FLOAT_EQ( mat.m33, TestRowD.W );
}

REDMATH_TEST( RedMath, GetScale )
{
	RedMatrix4x4 mat;
	RedVector4 scale = mat.GetScale();
	EXPECT_FLOAT_EQ( scale.X, 1.0f );
	EXPECT_FLOAT_EQ( scale.Y, 1.0f );
	EXPECT_FLOAT_EQ( scale.Z, 1.0f );
	EXPECT_FLOAT_EQ( scale.W, 1.0f );
}

REDMATH_TEST( RedMath, GetPreScale )
{
	RedMatrix4x4 mat;
	RedVector4 scale = mat.GetPreScale();
	EXPECT_FLOAT_EQ( scale.X, 1.0f );
	EXPECT_FLOAT_EQ( scale.Y, 1.0f );
	EXPECT_FLOAT_EQ( scale.Z, 1.0f );
	EXPECT_FLOAT_EQ( scale.W, 1.0f );
}

REDMATH_TEST( RedMath, SetGetTranslation )
{
	RedMatrix4x4 mat;
	mat.SetTranslation( TestRowA );
	EXPECT_FLOAT_EQ( mat.Row3.X, TestRowA.X );
	EXPECT_FLOAT_EQ( mat.Row3.Y, TestRowA.Y );
	EXPECT_FLOAT_EQ( mat.Row3.Z, TestRowA.Z );
	EXPECT_FLOAT_EQ( mat.Row3.W, TestRowA.W );

	mat.SetTranslation( 2.0f, 3.0f, 4.0f );
	EXPECT_FLOAT_EQ( mat.Row3.X, 2.0f );
	EXPECT_FLOAT_EQ( mat.Row3.Y, 3.0f );
	EXPECT_FLOAT_EQ( mat.Row3.Z, 4.0f );
	EXPECT_FLOAT_EQ( mat.Row3.W, 1.0f );

	RedVector4 trans = mat.GetTranslation();
	EXPECT_FLOAT_EQ( trans.X, 2.0f );
	EXPECT_FLOAT_EQ( trans.Y, 3.0f );
	EXPECT_FLOAT_EQ( trans.Z, 4.0f );
	EXPECT_FLOAT_EQ( trans.W, 1.0f );

	const RedVector4& transRef = mat.GetTranslationRef();
	EXPECT_FLOAT_EQ( transRef.X, 2.0f );
	EXPECT_FLOAT_EQ( transRef.Y, 3.0f );
	EXPECT_FLOAT_EQ( transRef.Z, 4.0f );
	EXPECT_FLOAT_EQ( transRef.W, 1.0f );
}

REDMATH_TEST(RedMath, Determinant )
{
	RedMatrix4x4 mat( TestMatrix2 );
	// Determinant calculated using: http://matrix.reshish.com/determinant.php
	// should be 2.2f
	const Red::System::Float expectedDet = 2.2f;
	RedScalar det = mat.Det();
	EXPECT_FLOAT_EQ( det.X, expectedDet );
	EXPECT_FLOAT_EQ( det.Y, expectedDet );
	EXPECT_FLOAT_EQ( det.Z, expectedDet );
	EXPECT_FLOAT_EQ( det.W, expectedDet );
}

REDMATH_TEST(RedMath, Cofactor )
{
	RedMatrix4x4 mat( TestMatrix2 );
	const Red::System::Float expectedMinor = 18.0f;
	RedScalar cofactorMinor = mat.CoFactor( 0, 0 );
	EXPECT_FLOAT_EQ( cofactorMinor.X, expectedMinor );
	EXPECT_FLOAT_EQ( cofactorMinor.Y, expectedMinor );
	EXPECT_FLOAT_EQ( cofactorMinor.Z, expectedMinor );
	EXPECT_FLOAT_EQ( cofactorMinor.W, expectedMinor );
}

REDMATH_TEST( RedMath, Inverse )
{
	RedMatrix4x4 mat( TestMatrix2 );
	RedMatrix4x4 expectedInverseMatrix( TestMatrix3 );
	// Inverse calculated using: http://matrix.reshish.com/inverse.php
	RedMatrix4x4 invertedMat = mat.Inverted();
	EXPECT_FLOAT_EQ( invertedMat.m00, expectedInverseMatrix.m00 );
	EXPECT_FLOAT_EQ( invertedMat.m01, expectedInverseMatrix.m01 );
	EXPECT_FLOAT_EQ( invertedMat.m02, expectedInverseMatrix.m02 );
	EXPECT_FLOAT_EQ( invertedMat.m03, expectedInverseMatrix.m03 );
	EXPECT_FLOAT_EQ( invertedMat.m10, expectedInverseMatrix.m10 );
	EXPECT_FLOAT_EQ( invertedMat.m11, expectedInverseMatrix.m11 );
	EXPECT_FLOAT_EQ( invertedMat.m12, expectedInverseMatrix.m12 );
	EXPECT_FLOAT_EQ( invertedMat.m13, expectedInverseMatrix.m13 );
	EXPECT_FLOAT_EQ( invertedMat.m20, expectedInverseMatrix.m20 );
	EXPECT_FLOAT_EQ( invertedMat.m21, expectedInverseMatrix.m21 );
	EXPECT_FLOAT_EQ( invertedMat.m22, expectedInverseMatrix.m22 );
	EXPECT_FLOAT_EQ( invertedMat.m23, expectedInverseMatrix.m23 );
	EXPECT_FLOAT_EQ( invertedMat.m30, expectedInverseMatrix.m30 );
	EXPECT_FLOAT_EQ( invertedMat.m31, expectedInverseMatrix.m31 );
	EXPECT_FLOAT_EQ( invertedMat.m32, expectedInverseMatrix.m32 );
	EXPECT_FLOAT_EQ( invertedMat.m33, expectedInverseMatrix.m33 );

	mat.Invert();
	EXPECT_FLOAT_EQ( mat.m00, expectedInverseMatrix.m00 );
	EXPECT_FLOAT_EQ( mat.m01, expectedInverseMatrix.m01 );
	EXPECT_FLOAT_EQ( mat.m02, expectedInverseMatrix.m02 );
	EXPECT_FLOAT_EQ( mat.m03, expectedInverseMatrix.m03 );
	EXPECT_FLOAT_EQ( mat.m10, expectedInverseMatrix.m10 );
	EXPECT_FLOAT_EQ( mat.m11, expectedInverseMatrix.m11 );
	EXPECT_FLOAT_EQ( mat.m12, expectedInverseMatrix.m12 );
	EXPECT_FLOAT_EQ( mat.m13, expectedInverseMatrix.m13 );
	EXPECT_FLOAT_EQ( mat.m20, expectedInverseMatrix.m20 );
	EXPECT_FLOAT_EQ( mat.m21, expectedInverseMatrix.m21 );
	EXPECT_FLOAT_EQ( mat.m22, expectedInverseMatrix.m22 );
	EXPECT_FLOAT_EQ( mat.m23, expectedInverseMatrix.m23 );
	EXPECT_FLOAT_EQ( mat.m30, expectedInverseMatrix.m30 );
	EXPECT_FLOAT_EQ( mat.m31, expectedInverseMatrix.m31 );
	EXPECT_FLOAT_EQ( mat.m32, expectedInverseMatrix.m32 );
	EXPECT_FLOAT_EQ( mat.m33, expectedInverseMatrix.m33 );
}

REDMATH_TEST( RedMath, Transpose )
{
	RedMatrix4x4 mat( TestMatrix2 );
	RedMatrix4x4 expectedTranspose( TestMatrix4 );

	RedMatrix4x4 transposedMatrix = mat.Transposed();
	EXPECT_FLOAT_EQ( transposedMatrix.m00, expectedTranspose.m00 );
	EXPECT_FLOAT_EQ( transposedMatrix.m01, expectedTranspose.m01 );
	EXPECT_FLOAT_EQ( transposedMatrix.m02, expectedTranspose.m02 );
	EXPECT_FLOAT_EQ( transposedMatrix.m03, expectedTranspose.m03 );
	EXPECT_FLOAT_EQ( transposedMatrix.m10, expectedTranspose.m10 );
	EXPECT_FLOAT_EQ( transposedMatrix.m11, expectedTranspose.m11 );
	EXPECT_FLOAT_EQ( transposedMatrix.m12, expectedTranspose.m12 );
	EXPECT_FLOAT_EQ( transposedMatrix.m13, expectedTranspose.m13 );
	EXPECT_FLOAT_EQ( transposedMatrix.m20, expectedTranspose.m20 );
	EXPECT_FLOAT_EQ( transposedMatrix.m21, expectedTranspose.m21 );
	EXPECT_FLOAT_EQ( transposedMatrix.m22, expectedTranspose.m22 );
	EXPECT_FLOAT_EQ( transposedMatrix.m23, expectedTranspose.m23 );
	EXPECT_FLOAT_EQ( transposedMatrix.m30, expectedTranspose.m30 );
	EXPECT_FLOAT_EQ( transposedMatrix.m31, expectedTranspose.m31 );
	EXPECT_FLOAT_EQ( transposedMatrix.m32, expectedTranspose.m32 );
	EXPECT_FLOAT_EQ( transposedMatrix.m33, expectedTranspose.m33 );

	mat.Transpose();
	EXPECT_FLOAT_EQ( mat.m00, expectedTranspose.m00 );
	EXPECT_FLOAT_EQ( mat.m01, expectedTranspose.m01 );
	EXPECT_FLOAT_EQ( mat.m02, expectedTranspose.m02 );
	EXPECT_FLOAT_EQ( mat.m03, expectedTranspose.m03 );
	EXPECT_FLOAT_EQ( mat.m10, expectedTranspose.m10 );
	EXPECT_FLOAT_EQ( mat.m11, expectedTranspose.m11 );
	EXPECT_FLOAT_EQ( mat.m12, expectedTranspose.m12 );
	EXPECT_FLOAT_EQ( mat.m13, expectedTranspose.m13 );
	EXPECT_FLOAT_EQ( mat.m20, expectedTranspose.m20 );
	EXPECT_FLOAT_EQ( mat.m21, expectedTranspose.m21 );
	EXPECT_FLOAT_EQ( mat.m22, expectedTranspose.m22 );
	EXPECT_FLOAT_EQ( mat.m23, expectedTranspose.m23 );
	EXPECT_FLOAT_EQ( mat.m30, expectedTranspose.m30 );
	EXPECT_FLOAT_EQ( mat.m31, expectedTranspose.m31 );
	EXPECT_FLOAT_EQ( mat.m32, expectedTranspose.m32 );
	EXPECT_FLOAT_EQ( mat.m33, expectedTranspose.m33 );
}

REDMATH_TEST( RedMath, IsOk )
{
	RedMatrix4x4 mat;
	EXPECT_TRUE( mat.IsOk() );
	mat.m03 = RED_INFINITY;
	mat.m13 = RED_NAN;
	mat.m23 = RED_NEG_NAN;
	mat.m33 = RED_NEG_INFINITY;

	EXPECT_FALSE( mat.IsOk() );
	mat.m03 = 0.0f;
	EXPECT_FALSE( mat.IsOk() );
	mat.m13 = 0.0f;
	EXPECT_FALSE( mat.IsOk() );
	mat.m23 = 0.0f;
	EXPECT_FALSE( mat.IsOk() );
	mat.m33 = 0.0f;
	EXPECT_TRUE( mat.IsOk() );
}