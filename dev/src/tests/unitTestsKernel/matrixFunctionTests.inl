
REDMATH_TEST( RedMath, BuildPerspectiveLH )
{
	RedMatrix4x4 perspective = BuildPerspectiveLH( deg90, aspect, nearPlane, farPlane );
	EXPECT_FLOAT_EQ( perspective.m00, TestPerspective[0] );
	EXPECT_FLOAT_EQ( perspective.m01, TestPerspective[1] );
	EXPECT_FLOAT_EQ( perspective.m02, TestPerspective[2] );
	EXPECT_FLOAT_EQ( perspective.m03, TestPerspective[3] );
	EXPECT_FLOAT_EQ( perspective.m10, TestPerspective[4] );
	EXPECT_FLOAT_EQ( perspective.m11, TestPerspective[5] );
	EXPECT_FLOAT_EQ( perspective.m12, TestPerspective[6] );
	EXPECT_FLOAT_EQ( perspective.m13, TestPerspective[7] );
	EXPECT_FLOAT_EQ( perspective.m20, TestPerspective[8] );
	EXPECT_FLOAT_EQ( perspective.m21, TestPerspective[9] );
	EXPECT_FLOAT_EQ( perspective.m22, TestPerspective[10] );
	EXPECT_FLOAT_EQ( perspective.m23, TestPerspective[11] );
	EXPECT_FLOAT_EQ( perspective.m30, TestPerspective[12] );
	EXPECT_FLOAT_EQ( perspective.m31, TestPerspective[13] );
	EXPECT_FLOAT_EQ( perspective.m32, TestPerspective[14] );
	EXPECT_FLOAT_EQ( perspective.m33, TestPerspective[15] );
}

REDMATH_TEST( RedMath, BuildOrthoLH )
{
	RedMatrix4x4 ortho = BuildOrthoLH( width, height, nearPlane, farPlane );
	EXPECT_FLOAT_EQ( ortho.m00, TestOrthographic[0] );
	EXPECT_FLOAT_EQ( ortho.m01, TestOrthographic[1] );
	EXPECT_FLOAT_EQ( ortho.m02, TestOrthographic[2] );
	EXPECT_FLOAT_EQ( ortho.m03, TestOrthographic[3] );
	EXPECT_FLOAT_EQ( ortho.m10, TestOrthographic[4] );
	EXPECT_FLOAT_EQ( ortho.m11, TestOrthographic[5] );
	EXPECT_FLOAT_EQ( ortho.m12, TestOrthographic[6] );
	EXPECT_FLOAT_EQ( ortho.m13, TestOrthographic[7] );
	EXPECT_FLOAT_EQ( ortho.m20, TestOrthographic[8] );
	EXPECT_FLOAT_EQ( ortho.m21, TestOrthographic[9] );
	EXPECT_FLOAT_EQ( ortho.m22, TestOrthographic[10] );
	EXPECT_FLOAT_EQ( ortho.m23, TestOrthographic[11] );
	EXPECT_FLOAT_EQ( ortho.m30, TestOrthographic[12] );
	EXPECT_FLOAT_EQ( ortho.m31, TestOrthographic[13] );
	EXPECT_FLOAT_EQ( ortho.m32, TestOrthographic[14] );
	EXPECT_FLOAT_EQ( ortho.m33, TestOrthographic[15] );
}

REDMATH_TEST( RedMath, ToAngleVectors )
{
	RedMatrix4x4 mat( TestMatrix1 );
	RedVector4 forwards;
	RedVector4 right;
	RedVector4 up;
	ToAngleVectors( mat, forwards, right, up );

	EXPECT_FLOAT_EQ( right.X, mat.m00 );
	EXPECT_FLOAT_EQ( right.Y, mat.m01 );
	EXPECT_FLOAT_EQ( right.Z, mat.m02 );

	EXPECT_FLOAT_EQ( forwards.X, mat.m10 );
	EXPECT_FLOAT_EQ( forwards.Y, mat.m11 );
	EXPECT_FLOAT_EQ( forwards.Z, mat.m12 );

	EXPECT_FLOAT_EQ( up.X, mat.m20 );
	EXPECT_FLOAT_EQ( up.Y, mat.m21 );
	EXPECT_FLOAT_EQ( up.Z, mat.m22 );
}

REDMATH_TEST( RedMath, TransformVector )
{
	RedMatrix4x4 ortho = BuildOrthoLH( 1000.f, 1000.f, 0.f, 1000.f );
	RedVector4 out = TransformVector(ortho, RedVector4(1000.f, 2000.f, 3000.f, 4000.f));

	EXPECT_FLOAT_EQ( out.X, 2.f );
	EXPECT_FLOAT_EQ( out.Y, 4.f );
	EXPECT_FLOAT_EQ( out.Z, 3.f );
	EXPECT_FLOAT_EQ( out.W, 0.f );

	Red::Math::Fpu::RedMatrix4x4 mat;
	mat.BuildOrthoLH( 1000.f, 1000.f, 0.f, 1000.f );
	Red::Math::Fpu::RedVector4 out2 = mat.TransformVector(Red::Math::Fpu::RedVector4(1000.f, 2000.f, 3000.f, 4000.f));

	EXPECT_FLOAT_EQ( out.X, out2.X );
	EXPECT_FLOAT_EQ( out.Y, out2.Y );
	EXPECT_FLOAT_EQ( out.Z, out2.Z );
	EXPECT_FLOAT_EQ( out.W, out2.W );

	RedMatrix4x4 mat2(TestMatrix1);
	RedVector4 out3 = TransformVector(mat2, RedVector4(100.f, 200.f, 300.f, 400.f));
	Red::Math::Fpu::RedMatrix4x4 mat3(TestMatrix1);
	Red::Math::Fpu::RedVector4 out4 = mat3.TransformVector(Red::Math::Fpu::RedVector4(100.f, 200.f, 300.f, 400.f));

	EXPECT_FLOAT_EQ( out3.X, out4.X );
	EXPECT_FLOAT_EQ( out3.Y, out4.Y );
	EXPECT_FLOAT_EQ( out3.Z, out4.Z );
	EXPECT_FLOAT_EQ( out3.W, out4.W );
}

REDMATH_TEST( RedMath, TransformVectorWithW )
{
	RedMatrix4x4 ortho = BuildOrthoLH( 1000.f, 1000.f, 0.f, 1000.f );
	RedVector4 out = TransformVectorWithW(ortho, RedVector4(1000.f, 2000.f, 3000.f, 4000.f));

	EXPECT_FLOAT_EQ( out.X, 2.f );
	EXPECT_FLOAT_EQ( out.Y, 4.f );
	EXPECT_FLOAT_EQ( out.Z, 3.f );
	EXPECT_FLOAT_EQ( out.W, 4000.f );

	Red::Math::Fpu::RedMatrix4x4 mat;
	mat.BuildOrthoLH( 1000.f, 1000.f, 0.f, 1000.f );
	Red::Math::Fpu::RedVector4 out2 = mat.TransformVectorWithW(Red::Math::Fpu::RedVector4(1000.f, 2000.f, 3000.f, 4000.f));

	EXPECT_FLOAT_EQ( out.X, out2.X );
	EXPECT_FLOAT_EQ( out.Y, out2.Y );
	EXPECT_FLOAT_EQ( out.Z, out2.Z );
	EXPECT_FLOAT_EQ( out.W, out2.W );

	RedMatrix4x4 mat2(TestMatrix1);
	RedVector4 out3 = TransformVectorWithW(mat2, RedVector4(100.f, 200.f, 300.f, 400.f));
	Red::Math::Fpu::RedMatrix4x4 mat3(TestMatrix1);
	Red::Math::Fpu::RedVector4 out4 = mat3.TransformVectorWithW(Red::Math::Fpu::RedVector4(100.f, 200.f, 300.f, 400.f));

	EXPECT_FLOAT_EQ( out3.X, out4.X );
	EXPECT_FLOAT_EQ( out3.Y, out4.Y );
	EXPECT_FLOAT_EQ( out3.Z, out4.Z );
	EXPECT_FLOAT_EQ( out3.W, out4.W );
}

REDMATH_TEST( RedMath, TransformPoint )
{
	RedMatrix4x4 ortho = BuildOrthoLH( 1000.f, 1000.f, 0.f, 1000.f );
	RedVector4 out = TransformPoint(ortho, RedVector4(1000.f, 2000.f, 3000.f, 4000.f));

	EXPECT_FLOAT_EQ( out.X, 2.f );
	EXPECT_FLOAT_EQ( out.Y, 4.f );
	EXPECT_FLOAT_EQ( out.Z, 3.f );
	EXPECT_FLOAT_EQ( out.W, 1.f );

	Red::Math::Fpu::RedMatrix4x4 mat;
	mat.BuildOrthoLH( 1000.f, 1000.f, 0.f, 1000.f );
	Red::Math::Fpu::RedVector4 out2 = mat.TransformPoint(Red::Math::Fpu::RedVector4(1000.f, 2000.f, 3000.f, 4000.f));

	EXPECT_FLOAT_EQ( out.X, out2.X );
	EXPECT_FLOAT_EQ( out.Y, out2.Y );
	EXPECT_FLOAT_EQ( out.Z, out2.Z );
	EXPECT_FLOAT_EQ( out.W, out2.W );

	RedMatrix4x4 mat2(TestMatrix1);
	RedVector4 out3 = TransformPoint(mat2, RedVector4(100.f, 200.f, 300.f, 400.f));
	Red::Math::Fpu::RedMatrix4x4 mat3(TestMatrix1);
	Red::Math::Fpu::RedVector4 out4 = mat3.TransformPoint(Red::Math::Fpu::RedVector4(100.f, 200.f, 300.f, 400.f));

	EXPECT_FLOAT_EQ( out3.X, out4.X );
	EXPECT_FLOAT_EQ( out3.Y, out4.Y );
	EXPECT_FLOAT_EQ( out3.Z, out4.Z );
	EXPECT_FLOAT_EQ( out3.W, out4.W );
}

REDMATH_TEST( RedMath, BuildFromQuaternion )
{
	RedVector4 Quat1(1.f, 2.f, 3.f, 4.f);
	Red::Math::Fpu::RedVector4 Quat2(1.f, 2.f, 3.f, 4.f);

	RedMatrix4x4 mat1 = BuildFromQuaternion(Quat1);
	Red::Math::Fpu::RedMatrix4x4 mat2; 
	mat2.BuildFromQuaternion(Quat2);

	EXPECT_TRUE( memcmp(&mat1, &mat2, sizeof(mat1)) == 0 );
}