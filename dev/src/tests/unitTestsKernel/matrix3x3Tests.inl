const RedVector3 TestRowA3( 1.0f, 2.0f, 3.0f );
const RedVector3 TestRowB3( 4.0f, 5.0f, 6.0f );
const RedVector3 TestRowC3( 7.0f, 8.0f, 9.0f );

REDMATH_TEST( RedMath, Construction )
{
	RedMatrix3x3 noParams;
	EXPECT_FLOAT_EQ( noParams.m00, RedMatrix3x3::IDENTITY.m00 );
	EXPECT_FLOAT_EQ( noParams.m01, RedMatrix3x3::IDENTITY.m01 );
	EXPECT_FLOAT_EQ( noParams.m02, RedMatrix3x3::IDENTITY.m02 );
	EXPECT_FLOAT_EQ( noParams.m10, RedMatrix3x3::IDENTITY.m10 );
	EXPECT_FLOAT_EQ( noParams.m11, RedMatrix3x3::IDENTITY.m11 );
	EXPECT_FLOAT_EQ( noParams.m12, RedMatrix3x3::IDENTITY.m12 );
	EXPECT_FLOAT_EQ( noParams.m20, RedMatrix3x3::IDENTITY.m20 );
	EXPECT_FLOAT_EQ( noParams.m21, RedMatrix3x3::IDENTITY.m21 );
	EXPECT_FLOAT_EQ( noParams.m22, RedMatrix3x3::IDENTITY.m22 );

	RedMatrix3x3 fromVector3s( TestRowA3, TestRowB3, TestRowC3 );
	EXPECT_FLOAT_EQ( fromVector3s.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( fromVector3s.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( fromVector3s.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( fromVector3s.m10, TestRowB3.X );
	EXPECT_FLOAT_EQ( fromVector3s.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( fromVector3s.m12, TestRowB3.Z );
	EXPECT_FLOAT_EQ( fromVector3s.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( fromVector3s.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( fromVector3s.m22, TestRowC3.Z );

	RedMatrix3x3 copy( fromVector3s );
	EXPECT_FLOAT_EQ( copy.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( copy.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( copy.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( copy.m10, TestRowB3.X );
	EXPECT_FLOAT_EQ( copy.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( copy.m12, TestRowB3.Z );
	EXPECT_FLOAT_EQ( copy.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( copy.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( copy.m22, TestRowC3.Z );

	RedMatrix3x3 fromFloats( TestMatrix1 );
	EXPECT_FLOAT_EQ( fromFloats.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( fromFloats.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( fromFloats.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( fromFloats.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( fromFloats.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( fromFloats.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( fromFloats.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( fromFloats.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( fromFloats.m22, TestMatrix1[10] );
}

REDMATH_TEST( RedMath, Operators )
{
	RedMatrix3x3 matA( TestMatrix1 );
	RedMatrix3x3 matB = matA;
	EXPECT_FLOAT_EQ( matB.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( matB.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( matB.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( matB.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( matB.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( matB.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( matB.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( matB.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( matB.m22, TestMatrix1[10] );
}

REDMATH_TEST( RedMath, AsFloat )
{
	RedMatrix3x3 mat( TestMatrix1 );
	const Red::System::Float* data = mat.AsFloat();

	EXPECT_FLOAT_EQ( data[0], TestMatrix1[0] );
	EXPECT_FLOAT_EQ( data[1], TestMatrix1[1] );
	EXPECT_FLOAT_EQ( data[2], TestMatrix1[2] );
	EXPECT_FLOAT_EQ( data[4], TestMatrix1[4] );
	EXPECT_FLOAT_EQ( data[5], TestMatrix1[5] );
	EXPECT_FLOAT_EQ( data[6], TestMatrix1[6] );
	EXPECT_FLOAT_EQ( data[8], TestMatrix1[8] );
	EXPECT_FLOAT_EQ( data[9], TestMatrix1[9] );
	EXPECT_FLOAT_EQ( data[10], TestMatrix1[10] );
}

REDMATH_TEST( RedMath, SetFunctions )
{
	RedMatrix3x3 mat;
	mat.Set( TestMatrix1 );
	EXPECT_FLOAT_EQ( mat.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( mat.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( mat.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( mat.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( mat.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( mat.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( mat.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( mat.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( mat.m22, TestMatrix1[10] );

	mat.Set( TestRowA3, TestRowB3, TestRowC3 );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB3.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB3.Z );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z );
}

REDMATH_TEST( RedMath, SetZeros )
{
	RedMatrix3x3 mat;
	mat.SetZeros();
	EXPECT_FLOAT_EQ( mat.m00, 0.0f );
	EXPECT_FLOAT_EQ( mat.m01, 0.0f );
	EXPECT_FLOAT_EQ( mat.m02, 0.0f );
	EXPECT_FLOAT_EQ( mat.m10, 0.0f );
	EXPECT_FLOAT_EQ( mat.m11, 0.0f );
	EXPECT_FLOAT_EQ( mat.m12, 0.0f );
	EXPECT_FLOAT_EQ( mat.m20, 0.0f );
	EXPECT_FLOAT_EQ( mat.m21, 0.0f );
	EXPECT_FLOAT_EQ( mat.m22, 0.0f );
}

REDMATH_TEST( RedMath, SetIdentity )
{
	RedMatrix3x3 mat( RedMatrix3x3::ZEROS );
	mat.SetIdentity();

	EXPECT_FLOAT_EQ( mat.m00, RedMatrix3x3::IDENTITY.m00 );
	EXPECT_FLOAT_EQ( mat.m01, RedMatrix3x3::IDENTITY.m01 );
	EXPECT_FLOAT_EQ( mat.m02, RedMatrix3x3::IDENTITY.m02 );
	EXPECT_FLOAT_EQ( mat.m10, RedMatrix3x3::IDENTITY.m10 );
	EXPECT_FLOAT_EQ( mat.m11, RedMatrix3x3::IDENTITY.m11 );
	EXPECT_FLOAT_EQ( mat.m12, RedMatrix3x3::IDENTITY.m12 );
	EXPECT_FLOAT_EQ( mat.m20, RedMatrix3x3::IDENTITY.m20 );
	EXPECT_FLOAT_EQ( mat.m21, RedMatrix3x3::IDENTITY.m21 );
	EXPECT_FLOAT_EQ( mat.m22, RedMatrix3x3::IDENTITY.m22 );
}

REDMATH_TEST( RedMath, SetRowFunctions )
{
	RedMatrix3x3 mat;
	mat.SetRows( TestMatrix1 );
	EXPECT_FLOAT_EQ( mat.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( mat.m01, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( mat.m02, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( mat.m10, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( mat.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( mat.m12, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( mat.m20, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( mat.m21, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( mat.m22, TestMatrix1[10] );
	
	mat.SetRows( TestRowA3, TestRowB3, TestRowC3 );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB3.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB3.Z );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z );

	mat.SetRow( 1, RedVector3(1.0f, 1.0f, 1.0f ) );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z );
	EXPECT_FLOAT_EQ( mat.m10, 1.0f );
	EXPECT_FLOAT_EQ( mat.m11, 1.0f );
	EXPECT_FLOAT_EQ( mat.m12, 1.0f );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z );
}

REDMATH_TEST( RedMath, GetRowFunctions )
{
	RedMatrix3x3 mat;
	const RedVector3& row = mat.GetRow( 2 );

	EXPECT_FLOAT_EQ( row.X, 0.0f );
	EXPECT_FLOAT_EQ( row.Y, 0.0f );
	EXPECT_FLOAT_EQ( row.Z, 1.0f );
}

REDMATH_TEST( RedMath, SetColFunctions )
{
	RedMatrix3x3 mat;
	mat.SetCols( TestMatrix1 );
	EXPECT_FLOAT_EQ( mat.m00, TestMatrix1[0] );
	EXPECT_FLOAT_EQ( mat.m10, TestMatrix1[1] );
	EXPECT_FLOAT_EQ( mat.m20, TestMatrix1[2] );
	EXPECT_FLOAT_EQ( mat.m01, TestMatrix1[4] );
	EXPECT_FLOAT_EQ( mat.m11, TestMatrix1[5] );
	EXPECT_FLOAT_EQ( mat.m21, TestMatrix1[6] );
	EXPECT_FLOAT_EQ( mat.m02, TestMatrix1[8] );
	EXPECT_FLOAT_EQ( mat.m12, TestMatrix1[9] );
	EXPECT_FLOAT_EQ( mat.m22, TestMatrix1[10] );
	
	mat.SetCols( TestRowA3, TestRowB3, TestRowC3 );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowB3.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowC3.X );
	EXPECT_FLOAT_EQ( mat.m10, TestRowA3.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowC3.Y );
	EXPECT_FLOAT_EQ( mat.m20, TestRowA3.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowB3.Z );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z );
	
	mat.SetColumn( 2, RedVector3( 1.0f, 1.0f, 1.0f ) );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowB3.X );
	EXPECT_FLOAT_EQ( mat.m02, 1.0f );
	EXPECT_FLOAT_EQ( mat.m10, TestRowA3.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y );
	EXPECT_FLOAT_EQ( mat.m12, 1.0f );
	EXPECT_FLOAT_EQ( mat.m20, TestRowA3.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowB3.Z );
	EXPECT_FLOAT_EQ( mat.m22, 1.0f );
}

REDMATH_TEST( RedMath, GetColFunctions )
{
	RedMatrix3x3 mat;
	RedVector3 col = mat.GetColumn( 2 );
	EXPECT_FLOAT_EQ( col.X, 0.0f );
	EXPECT_FLOAT_EQ( col.Y, 0.0f );
	EXPECT_FLOAT_EQ( col.Z, 1.0f );
}

REDMATH_TEST( RedMath, GetAxis )
{
	RedMatrix3x3 mat;
	const RedVector3& xAxis = mat.GetAxisX();
	EXPECT_FLOAT_EQ( xAxis.X, 1.0f );
	EXPECT_FLOAT_EQ( xAxis.Y, 0.0f );
	EXPECT_FLOAT_EQ( xAxis.Z, 0.0f );

	const RedVector3& yAxis = mat.GetAxisY();
	EXPECT_FLOAT_EQ( yAxis.X, 0.0f );
	EXPECT_FLOAT_EQ( yAxis.Y, 1.0f );
	EXPECT_FLOAT_EQ( yAxis.Z, 0.0f );
	
	const RedVector3& zAxis = mat.GetAxisZ();
	EXPECT_FLOAT_EQ( zAxis.X, 0.0f );
	EXPECT_FLOAT_EQ( zAxis.Y, 0.0f );
	EXPECT_FLOAT_EQ( zAxis.Z, 1.0f );
}

REDMATH_TEST( RedMath, SetRot )
{
	Red::System::Float expectedCosine = Red::Math::MCos( Deg90 );
	Red::System::Float expectedSine = Red::Math::MSin( Deg90 );

	RedMatrix3x3 mat;
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
	RedMatrix3x3 mat( TestRowA3, TestRowB3, TestRowC3 );
	const RedScalar scaleA( 2.0f );
	const Red::System::Float scaleB( 2.5f );
	const RedVector3 scaleC( 3.0f, 4.0f, 5.0f );

	mat.SetScale( scaleA );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB3.X * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB3.Z * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y * scaleA.X );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z * scaleA.X );
	
	mat.Set( TestRowA3, TestRowB3, TestRowC3 );
	mat.SetScale( scaleB );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X * scaleB );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y * scaleB );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z * scaleB );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB3.X * scaleB );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y * scaleB );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB3.Z * scaleB );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X * scaleB );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y * scaleB );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z * scaleB );
	
	mat.Set( TestRowA3, TestRowB3, TestRowC3 );
	mat.SetScale( scaleC );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X * scaleC.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y * scaleC.X );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z * scaleC.X );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB3.X * scaleC.Y );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y * scaleC.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB3.Z * scaleC.Y );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X * scaleC.Z );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y * scaleC.Z );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z * scaleC.Z );
}

REDMATH_TEST( RedMath, SetPreScale )
{
	RedMatrix3x3 mat( TestRowA3, TestRowB3, TestRowC3 );
	RedVector3 scaleVal( 1.0f, 2.0f, 3.0f );
	mat.SetPreScale( scaleVal );
	EXPECT_FLOAT_EQ( mat.m00, TestRowA3.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m01, TestRowA3.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m02, TestRowA3.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m10, TestRowB3.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m11, TestRowB3.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m12, TestRowB3.Z * scaleVal.Z );
	EXPECT_FLOAT_EQ( mat.m20, TestRowC3.X * scaleVal.X );
	EXPECT_FLOAT_EQ( mat.m21, TestRowC3.Y * scaleVal.Y );
	EXPECT_FLOAT_EQ( mat.m22, TestRowC3.Z * scaleVal.Z );
}

REDMATH_TEST( RedMath, GetScale )
{
	RedMatrix3x3 mat;
	RedVector3 scale = mat.GetScale();
	EXPECT_FLOAT_EQ( scale.X, 1.0f );
	EXPECT_FLOAT_EQ( scale.Y, 1.0f );
	EXPECT_FLOAT_EQ( scale.Z, 1.0f );
}

REDMATH_TEST( RedMath, GetPreScale )
{
	RedMatrix3x3 mat;
	RedVector3 scale = mat.GetPreScale();
	EXPECT_FLOAT_EQ( scale.X, 1.0f );
	EXPECT_FLOAT_EQ( scale.Y, 1.0f );
	EXPECT_FLOAT_EQ( scale.Z, 1.0f );
}

REDMATH_TEST(RedMath, Determinant )
{
	RedMatrix3x3 mat( TestMatrix2 );
	// Determinant calculated using: http://matrix.reshish.com/determinant.php
	// should be 2.2f
	const Red::System::Float expectedDet = 2.2f;
	RedScalar det = mat.Det();
	EXPECT_FLOAT_EQ( det.X, expectedDet );
	EXPECT_FLOAT_EQ( det.Y, expectedDet );
	EXPECT_FLOAT_EQ( det.Z, expectedDet );
	EXPECT_FLOAT_EQ( det.W, expectedDet );
}

REDMATH_TEST( RedMath, Inverse )
{
	RedMatrix3x3 mat( TestMatrix2 );
	RedMatrix3x3 expectedInverseMatrix( TestMatrix3 );
	// Inverse calculated using: http://matrix.reshish.com/inverse.php
	RedMatrix3x3 invertedMat = mat.Inverted();
	EXPECT_FLOAT_EQ( invertedMat.m00, expectedInverseMatrix.m00 );
	EXPECT_FLOAT_EQ( invertedMat.m01, expectedInverseMatrix.m01 );
	EXPECT_FLOAT_EQ( invertedMat.m02, expectedInverseMatrix.m02 );
	EXPECT_FLOAT_EQ( invertedMat.m10, expectedInverseMatrix.m10 );
	EXPECT_FLOAT_EQ( invertedMat.m11, expectedInverseMatrix.m11 );
	EXPECT_FLOAT_EQ( invertedMat.m12, expectedInverseMatrix.m12 );
	EXPECT_FLOAT_EQ( invertedMat.m20, expectedInverseMatrix.m20 );
	EXPECT_FLOAT_EQ( invertedMat.m21, expectedInverseMatrix.m21 );
	EXPECT_FLOAT_EQ( invertedMat.m22, expectedInverseMatrix.m22 );
	
	mat.Invert();
	EXPECT_FLOAT_EQ( mat.m00, expectedInverseMatrix.m00 );
	EXPECT_FLOAT_EQ( mat.m01, expectedInverseMatrix.m01 );
	EXPECT_FLOAT_EQ( mat.m02, expectedInverseMatrix.m02 );
	EXPECT_FLOAT_EQ( mat.m10, expectedInverseMatrix.m10 );
	EXPECT_FLOAT_EQ( mat.m11, expectedInverseMatrix.m11 );
	EXPECT_FLOAT_EQ( mat.m12, expectedInverseMatrix.m12 );
	EXPECT_FLOAT_EQ( mat.m20, expectedInverseMatrix.m20 );
	EXPECT_FLOAT_EQ( mat.m21, expectedInverseMatrix.m21 );
	EXPECT_FLOAT_EQ( mat.m22, expectedInverseMatrix.m22 );
}

REDMATH_TEST( RedMath, Transpose )
{
	RedMatrix3x3 mat( TestMatrix2 );
	RedMatrix3x3 expectedTranspose( TestMatrix4 );

	RedMatrix3x3 transposedMatrix = mat.Transposed();
	EXPECT_FLOAT_EQ( transposedMatrix.m00, expectedTranspose.m00 );
	EXPECT_FLOAT_EQ( transposedMatrix.m01, expectedTranspose.m01 );
	EXPECT_FLOAT_EQ( transposedMatrix.m02, expectedTranspose.m02 );
	EXPECT_FLOAT_EQ( transposedMatrix.m10, expectedTranspose.m10 );
	EXPECT_FLOAT_EQ( transposedMatrix.m11, expectedTranspose.m11 );
	EXPECT_FLOAT_EQ( transposedMatrix.m12, expectedTranspose.m12 );
	EXPECT_FLOAT_EQ( transposedMatrix.m20, expectedTranspose.m20 );
	EXPECT_FLOAT_EQ( transposedMatrix.m21, expectedTranspose.m21 );
	EXPECT_FLOAT_EQ( transposedMatrix.m22, expectedTranspose.m22 );
	
	mat.Transpose();
	EXPECT_FLOAT_EQ( mat.m00, expectedTranspose.m00 );
	EXPECT_FLOAT_EQ( mat.m01, expectedTranspose.m01 );
	EXPECT_FLOAT_EQ( mat.m02, expectedTranspose.m02 );
	EXPECT_FLOAT_EQ( mat.m10, expectedTranspose.m10 );
	EXPECT_FLOAT_EQ( mat.m11, expectedTranspose.m11 );
	EXPECT_FLOAT_EQ( mat.m12, expectedTranspose.m12 );
	EXPECT_FLOAT_EQ( mat.m20, expectedTranspose.m20 );
	EXPECT_FLOAT_EQ( mat.m21, expectedTranspose.m21 );
	EXPECT_FLOAT_EQ( mat.m22, expectedTranspose.m22 );
}

REDMATH_TEST( RedMath, IsOk )
{
	RedMatrix3x3 mat;
	EXPECT_TRUE( mat.IsOk() );
	mat.m02 = RED_INFINITY;
	mat.m12 = RED_NAN;
	mat.m22 = RED_NEG_NAN;

	EXPECT_FALSE( mat.IsOk() );
	mat.m02 = 0.0f;
	EXPECT_FALSE( mat.IsOk() );
	mat.m12 = 0.0f;
	EXPECT_FALSE( mat.IsOk() );
	mat.m22 = 0.0f;
	EXPECT_TRUE( mat.IsOk() );
}