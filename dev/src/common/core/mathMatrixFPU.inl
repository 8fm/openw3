/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

Matrix::Matrix( const Float* f )
{
	SetRows( f );
}

Matrix::Matrix( const Matrix& a )
{
	Set( a );
}

Matrix::Matrix( const Vector& x, const Vector& y, const Vector& z, const Vector& w )
{
	SetRows( x, y, z, w );
}

Matrix& Matrix::Set( const Matrix& a )
{
	V[0] = a.V[0];
	V[1] = a.V[1];
	V[2] = a.V[2];
	V[3] = a.V[3];
	return *this;
}

Bool Matrix::operator==( const Matrix& a ) const
{
	return ( V[0] == a.V[0] ) && ( V[1] == a.V[1] ) && ( V[2] == a.V[2] ) && ( V[3] == a.V[3] );
}

Matrix& Matrix::operator=( const Matrix& a )
{
	V[0].Set4( a.V[0] );
	V[1].Set4( a.V[1] );
	V[2].Set4( a.V[2] );
	V[3].Set4( a.V[3] );
	return *this;
}

Matrix& Matrix::SetRows( const Float* f )
{
	V[0].Set4( f + 0 );
	V[1].Set4( f + 4 );
	V[2].Set4( f + 8 );
	V[3].Set4( f + 12 );
	return *this;
}

Matrix& Matrix::SetCols( const Float* f )
{
	V[0].A[0] = f[0];
	V[0].A[1] = f[4];
	V[0].A[2] = f[8];
	V[0].A[3] = f[12];
	V[1].A[0] = f[1];
	V[1].A[1] = f[5];
	V[1].A[2] = f[9];
	V[1].A[3] = f[13];
	V[2].A[0] = f[2];
	V[2].A[1] = f[6];
	V[2].A[2] = f[10];
	V[2].A[3] = f[14];
	V[3].A[0] = f[3];
	V[3].A[1] = f[6];
	V[3].A[2] = f[11];
	V[3].A[3] = f[15];
	return *this;
}

Matrix& Matrix::SetRows( const Vector& x, const Vector& y, const Vector& z, const Vector& w )
{
	V[0] = x;
	V[1] = y;
	V[2] = z;
	V[3] = w;
	return *this;
}

Matrix& Matrix::SetCols( const Vector& x, const Vector& y, const Vector& z, const Vector& w )
{
	V[0].A[0] = x.X;
	V[1].A[0] = x.Y;
	V[2].A[0] = x.Z;
	V[3].A[0] = x.W;	
	V[0].A[1] = y.X;
	V[1].A[1] = y.Y;
	V[2].A[1] = y.Z;
	V[3].A[1] = y.W;	
	V[0].A[2] = z.X;
	V[1].A[2] = z.Y;
	V[2].A[2] = z.Z;
	V[3].A[2] = z.W;	
	V[0].A[3] = w.X;
	V[1].A[3] = w.Y;
	V[2].A[3] = w.Z;
	V[3].A[3] = w.W;
	return *this;
}

Matrix& Matrix::SetZeros()
{
	V[0].SetZeros();
	V[1].SetZeros();
	V[2].SetZeros();
	V[3].SetZeros();
	return *this;
}

Matrix& Matrix::SetIdentity()
{
	SetZeros();
	V[0].A[0] = 1.0f;
	V[1].A[1] = 1.0f;
	V[2].A[2] = 1.0f;
	V[3].A[3] = 1.0f;
	return *this;
}

Matrix& Matrix::Set33( const Matrix& a )
{
	V[0].A[0] = a.V[0].A[0];
	V[0].A[1] = a.V[0].A[1];
	V[0].A[2] = a.V[0].A[2];
	V[1].A[0] = a.V[1].A[0];
	V[1].A[1] = a.V[1].A[1];
	V[1].A[2] = a.V[1].A[2];
	V[2].A[0] = a.V[2].A[0];
	V[2].A[1] = a.V[2].A[1];
	V[2].A[2] = a.V[2].A[2];
	return *this;
}

Matrix& Matrix::Set33( const Vector& x, const Vector& y, const Vector& z )
{
	V[0].A[0] = x.X;
	V[0].A[1] = x.Y;
	V[0].A[2] = x.Z;
	V[1].A[0] = y.X;
	V[1].A[1] = y.Y;
	V[1].A[2] = y.Z;
	V[2].A[0] = z.X;
	V[2].A[1] = z.Y;
	V[2].A[2] = z.Z;
	return *this;
}

Matrix& Matrix::SetRotX33( Float ccwRadians )
{
	Float rotC = MCos( ccwRadians );
	Float rotS = MSin( ccwRadians );

	// CCW rotation around X axis
	// [ 1   0       0       ]
	// [ 0   cos(a)  sin(a)  ]
	// [ 0   -sin(a) cos(a)  ]
	//
	// Given [0,1,0] rotation order is
	//  0:   Y+
	//  90   Z+
	//  180: Y-
	//  270: Z-
	//  360: Y+

	V[0].A[0] = 1.0f;
	V[0].A[1] = 0.0f;
	V[0].A[2] = 0.0f;
	V[1].A[0] = 0.0f;
	V[1].A[1] = rotC;
	V[1].A[2] = rotS;
	V[2].A[0] = 0.0f;
	V[2].A[1] = -rotS;
	V[2].A[2] = rotC;

	return *this;
}

Matrix& Matrix::SetRotY33( Float ccRadians )
{
	Float rotC = MCos( ccRadians );
	Float rotS = MSin( ccRadians );

	// CCW rotation around Y axis
	// [ cos(a)   0   -sin(a) ]
	// [ 0        1    0      ]
	// [ sin(a)  0    cos(a)  ]
	//
	// Given [1,0,0] rotation order is
	//  0:   X+
	//  90   Z-
	//  180: X-
	//  270: Z+
	//  360: X+

	V[0].A[0] = rotC;
	V[0].A[1] = 0.0f;
	V[0].A[2] = -rotS;
	V[1].A[0] = 0.0f;
	V[1].A[1] = 1.0f;
	V[1].A[2] = 0.0f;
	V[2].A[0] = rotS;
	V[2].A[1] = 0.0f;
	V[2].A[2] = rotC;
	return *this;
}

Matrix& Matrix::SetRotZ33( Float ccRadians )
{
	Float rotC = MCos( ccRadians );
	Float rotS = MSin( ccRadians );

	// CCW rotation around Z axis
	// [ cos(a)   sin(a)  0 ]
	// [ -sin(a)  cos(a)  0 ]
	// [ 0        0       1 ]
	//
	// Given [1,0,0] rotation order is
	//  0:   X+
	//  90   Y+
	//  180: X-
	//  270: Y-
	//  360: X+

	V[0].A[0] = rotC;
	V[0].A[1] = rotS;
	V[0].A[2] = 0.0f;
	V[1].A[0] = -rotS;
	V[1].A[1] = rotC;
	V[1].A[2] = 0.0f;
	V[2].A[0] = 0.0f;
	V[2].A[1] = 0.0f;
	V[2].A[2] = 1.0f;
	return *this;
}

Matrix& Matrix::SetScale33( const Vector& scale )
{
	V[0].A[0] *= scale.X;
	V[0].A[1] *= scale.X;
	V[0].A[2] *= scale.X;
	V[1].A[0] *= scale.Y;
	V[1].A[1] *= scale.Y;
	V[1].A[2] *= scale.Y;
	V[2].A[0] *= scale.Z;
	V[2].A[1] *= scale.Z;
	V[2].A[2] *= scale.Z;
	return *this;
}

Matrix& Matrix::SetScale44( const Vector& scale )
{
	V[0].A[0] *= scale.X;
	V[0].A[1] *= scale.X;
	V[0].A[2] *= scale.X;
	V[0].A[3] *= scale.X;
	V[1].A[0] *= scale.Y;
	V[1].A[1] *= scale.Y;
	V[1].A[2] *= scale.Y;
	V[1].A[3] *= scale.Y;
	V[2].A[0] *= scale.Z;
	V[2].A[1] *= scale.Z;
	V[2].A[2] *= scale.Z;
	V[2].A[3] *= scale.Z;
	return *this;
}

Vector Matrix::GetScale33() const
{
	return Vector( V[0].Mag3(), V[1].Mag3(), V[2].Mag3() );
}

Matrix& Matrix::SetPreScale33( const Vector& scale )
{
	V[0].A[0] *= scale.X;
	V[0].A[1] *= scale.Y;
	V[0].A[2] *= scale.Z;
	V[1].A[0] *= scale.X;
	V[1].A[1] *= scale.Y;
	V[1].A[2] *= scale.Z;
	V[2].A[0] *= scale.X;
	V[2].A[1] *= scale.Y;
	V[2].A[2] *= scale.Z;
	return *this;
}

Matrix& Matrix::SetPreScale44( const Vector& scale )
{
	V[0].A[0] *= scale.X;
	V[0].A[1] *= scale.Y;
	V[0].A[2] *= scale.Z;
	V[1].A[0] *= scale.X;
	V[1].A[1] *= scale.Y;
	V[1].A[2] *= scale.Z;
	V[2].A[0] *= scale.X;
	V[2].A[1] *= scale.Y;
	V[2].A[2] *= scale.Z;
	V[3].A[0] *= scale.X;
	V[3].A[1] *= scale.Y;
	V[3].A[2] *= scale.Z;
	return *this;
}

Vector Matrix::GetPreScale33() const
{
	Vector v0( V[0].A[0], V[1].A[0], V[2].A[0] );
	Vector v1( V[0].A[1], V[1].A[1], V[2].A[1] );
	Vector v2( V[0].A[2], V[1].A[2], V[2].A[2] );
	return Vector( v0.Mag3(), v1.Mag3(), v2.Mag3() );
}

Matrix& Matrix::SetScale33( Float uniformScale )
{
	V[0].A[0] *= uniformScale;
	V[0].A[1] *= uniformScale;
	V[0].A[2] *= uniformScale;
	V[1].A[0] *= uniformScale;
	V[1].A[1] *= uniformScale;
	V[1].A[2] *= uniformScale;
	V[2].A[0] *= uniformScale;
	V[2].A[1] *= uniformScale;
	V[2].A[2] *= uniformScale;
	return *this;
}

Matrix& Matrix::SetTranslation( const Vector& a )
{
	V[3].A[0] = a.X;
	V[3].A[1] = a.Y;
	V[3].A[2] = a.Z;
	V[3].A[3] = 1.0f;
	return *this;
}

Matrix& Matrix::SetTranslation( Float x, Float y, Float z )
{
	V[3].A[0] = x;
	V[3].A[1] = y;
	V[3].A[2] = z;
	V[3].A[3] = 1.0f;
	return *this;
}

Vector Matrix::GetTranslation() const
{
	return Vector( V[3] ).SetW( 1.0f );
}

const Vector& Matrix::GetTranslationRef() const
{
	return V[3];
}

RED_INLINE EulerAngles& FromMatrix( const Matrix& matrix );

Float Matrix::Det() const
{
	Float det = 0.0f;
	det += V[0].A[0] * CoFactor(0,0);
	det += V[0].A[1] * CoFactor(0,1); 
	det += V[0].A[2] * CoFactor(0,2); 
	det += V[0].A[3] * CoFactor(0,3); 
	return det;
}

Float Matrix::CoFactor( Int32 i, Int32 j ) const
{
	#define M( dx, dy ) V[ (i+dx)&3 ].A[ (j+dy) & 3 ]
	Float val = 0.0f;
	val += M(1,1) * M(2,2) * M(3,3);
	val += M(1,2) * M(2,3) * M(3,1);
	val += M(1,3) * M(2,1) * M(3,2);
	val -= M(3,1) * M(2,2) * M(1,3);
	val -= M(3,2) * M(2,3) * M(1,1);
	val -= M(3,3) * M(2,1) * M(1,2);
	val *= ((i+j) & 1) ? -1.0f : 1.0f;
	return val; 
	#undef M
}

Matrix& Matrix::Invert()
{
	*this = Inverted();
	return *this;
}

Matrix& Matrix::FullInvert()
{
	*this = FullInverted();
	return *this;
}

Matrix& Matrix::Transpose()
{
	*this = Transposed();
	return *this;
}

Matrix Matrix::Inverted() const
{
	Matrix out;

	// Transpose the 3x3 inner matrix
	out.V[0].A[0] = V[0].A[0];
	out.V[0].A[1] = V[1].A[0];
	out.V[0].A[2] = V[2].A[0];
	out.V[1].A[0] = V[0].A[1];
	out.V[1].A[1] = V[1].A[1];
	out.V[1].A[2] = V[2].A[1];
	out.V[2].A[0] = V[0].A[2];
	out.V[2].A[1] = V[1].A[2];
	out.V[2].A[2] = V[2].A[2];

	// Calculate inverted translation
	out.V[3].A[0] = -( V[3].A[0] * V[0].A[0] + V[3].A[1] * V[0].A[1] + V[3].A[2] * V[0].A[2] );
	out.V[3].A[1] = -( V[3].A[0] * V[1].A[0] + V[3].A[1] * V[1].A[1] + V[3].A[2] * V[1].A[2] );
	out.V[3].A[2] = -( V[3].A[0] * V[2].A[0] + V[3].A[1] * V[2].A[1] + V[3].A[2] * V[2].A[2] );

	// Leave last row as in the identity matrix
	out.V[0].A[3] = 0.0f;
	out.V[1].A[3] = 0.0f;
	out.V[2].A[3] = 0.0f;
	out.V[3].A[3] = 1.0f;
	return out;
}

Matrix Matrix::Transposed() const
{
	Matrix out;
	out.V[0].A[0] = V[0].A[0];
	out.V[0].A[1] = V[1].A[0];
	out.V[0].A[2] = V[2].A[0];
	out.V[0].A[3] = V[3].A[0];
	out.V[1].A[0] = V[0].A[1];
	out.V[1].A[1] = V[1].A[1];
	out.V[1].A[2] = V[2].A[1];
	out.V[1].A[3] = V[3].A[1];
	out.V[2].A[0] = V[0].A[2];
	out.V[2].A[1] = V[1].A[2];
	out.V[2].A[2] = V[2].A[2];
	out.V[2].A[3] = V[3].A[2];
	out.V[3].A[0] = V[0].A[3];
	out.V[3].A[1] = V[1].A[3];
	out.V[3].A[2] = V[2].A[3];
	out.V[3].A[3] = V[3].A[3];
	return out;
}

Matrix Matrix::FullInverted() const
{
	Matrix out;

	// Get determinant
	Float d = Det();
	if ( MAbs(d) > 1e-12f )
	{
		Float id = 1.0f / d;

		// Invert matrix
		out.V[0].A[0] = CoFactor(0,0) * id;
		out.V[0].A[1] = CoFactor(1,0) * id;
		out.V[0].A[2] = CoFactor(2,0) * id;
		out.V[0].A[3] = CoFactor(3,0) * id;
		out.V[1].A[0] = CoFactor(0,1) * id;
		out.V[1].A[1] = CoFactor(1,1) * id;
		out.V[1].A[2] = CoFactor(2,1) * id;
		out.V[1].A[3] = CoFactor(3,1) * id;
		out.V[2].A[0] = CoFactor(0,2) * id;
		out.V[2].A[1] = CoFactor(1,2) * id;
		out.V[2].A[2] = CoFactor(2,2) * id;
		out.V[2].A[3] = CoFactor(3,2) * id;
		out.V[3].A[0] = CoFactor(0,3) * id;
		out.V[3].A[1] = CoFactor(1,3) * id;
		out.V[3].A[2] = CoFactor(2,3) * id;
		out.V[3].A[3] = CoFactor(3,3) * id;
	}

	return out;
}

Vector Matrix::GetColumn( Int32 index ) const
{
	return Vector( V[0].A[ index ], V[1].A[ index ], V[2].A[ index ], V[3].A[ index ] );
}

Matrix& Matrix::SetColumn( Int32 index, const Vector& a )
{
	V[0].A[ index ] = a.X;
	V[1].A[ index ] = a.Y;
	V[2].A[ index ] = a.Z;
	V[3].A[ index ] = a.W;
	return *this;
}

const Vector& Matrix::GetRow( Int32 index ) const
{
	return V[ index ];
}

Matrix& Matrix::SetRow( Int32 index, const Vector& a )
{
	V[ index ] = a;
	return *this;
}

Vector Matrix::GetAxisX() const
{
    return Vector( V[0].A[0], V[0].A[1], V[0].A[2], 0.f );
}
    
Vector Matrix::GetAxisY() const
{
    return Vector( V[1].A[0], V[1].A[1], V[1].A[2], 0.f );
} 

Vector Matrix::GetAxisZ() const
{
    return Vector( V[2].A[0], V[2].A[1], V[2].A[2], 0.f );
}
    
void Matrix::GetColumnMajor( Float* data ) const
{
	//unrolled
	data[0]=V[0].A[0];
	data[1]=V[1].A[0];
	data[2]=V[2].A[0];
	data[3]=V[3].A[0];
	data[4]=V[0].A[1];
	data[5]=V[1].A[1];
	data[6]=V[2].A[1];
	data[7]=V[3].A[1];
	data[8]=V[0].A[2];
	data[9]=V[1].A[2];
	data[10]=V[2].A[2];
	data[11]=V[3].A[2];
	data[12]=V[0].A[3];
	data[13]=V[1].A[3];
	data[14]=V[2].A[3];
	data[15]=V[3].A[3];	
}

void Matrix::GetColumnMajor3x4( Float* data ) const
{
	//unrolled
	data[0]=V[0].A[0];
	data[1]=V[1].A[0];
	data[2]=V[2].A[0];
	data[3]=V[3].A[0];
	data[4]=V[0].A[1];
	data[5]=V[1].A[1];
	data[6]=V[2].A[1];
	data[7]=V[3].A[1];
	data[8]=V[0].A[2];
	data[9]=V[1].A[2];
	data[10]=V[2].A[2];
	data[11]=V[3].A[2];
}

Matrix Matrix::Mul( const Matrix& a, const Matrix& b )
{
	Matrix ret;
	ret.V[0].A[0] = b.V[0].A[0] * a.V[0].A[0] + b.V[0].A[1] * a.V[1].A[0] + b.V[0].A[2] * a.V[2].A[0] + b.V[0].A[3] * a.V[3].A[0];
	ret.V[0].A[1] = b.V[0].A[0] * a.V[0].A[1] + b.V[0].A[1] * a.V[1].A[1] + b.V[0].A[2] * a.V[2].A[1] + b.V[0].A[3] * a.V[3].A[1];
	ret.V[0].A[2] = b.V[0].A[0] * a.V[0].A[2] + b.V[0].A[1] * a.V[1].A[2] + b.V[0].A[2] * a.V[2].A[2] + b.V[0].A[3] * a.V[3].A[2];
	ret.V[0].A[3] = b.V[0].A[0] * a.V[0].A[3] + b.V[0].A[1] * a.V[1].A[3] + b.V[0].A[2] * a.V[2].A[3] + b.V[0].A[3] * a.V[3].A[3];
	ret.V[1].A[0] = b.V[1].A[0] * a.V[0].A[0] + b.V[1].A[1] * a.V[1].A[0] + b.V[1].A[2] * a.V[2].A[0] + b.V[1].A[3] * a.V[3].A[0];
	ret.V[1].A[1] = b.V[1].A[0] * a.V[0].A[1] + b.V[1].A[1] * a.V[1].A[1] + b.V[1].A[2] * a.V[2].A[1] + b.V[1].A[3] * a.V[3].A[1];
	ret.V[1].A[2] = b.V[1].A[0] * a.V[0].A[2] + b.V[1].A[1] * a.V[1].A[2] + b.V[1].A[2] * a.V[2].A[2] + b.V[1].A[3] * a.V[3].A[2];
	ret.V[1].A[3] = b.V[1].A[0] * a.V[0].A[3] + b.V[1].A[1] * a.V[1].A[3] + b.V[1].A[2] * a.V[2].A[3] + b.V[1].A[3] * a.V[3].A[3];
	ret.V[2].A[0] = b.V[2].A[0] * a.V[0].A[0] + b.V[2].A[1] * a.V[1].A[0] + b.V[2].A[2] * a.V[2].A[0] + b.V[2].A[3] * a.V[3].A[0];
	ret.V[2].A[1] = b.V[2].A[0] * a.V[0].A[1] + b.V[2].A[1] * a.V[1].A[1] + b.V[2].A[2] * a.V[2].A[1] + b.V[2].A[3] * a.V[3].A[1];
	ret.V[2].A[2] = b.V[2].A[0] * a.V[0].A[2] + b.V[2].A[1] * a.V[1].A[2] + b.V[2].A[2] * a.V[2].A[2] + b.V[2].A[3] * a.V[3].A[2];
	ret.V[2].A[3] = b.V[2].A[0] * a.V[0].A[3] + b.V[2].A[1] * a.V[1].A[3] + b.V[2].A[2] * a.V[2].A[3] + b.V[2].A[3] * a.V[3].A[3];
	ret.V[3].A[0] = b.V[3].A[0] * a.V[0].A[0] + b.V[3].A[1] * a.V[1].A[0] + b.V[3].A[2] * a.V[2].A[0] + b.V[3].A[3] * a.V[3].A[0];
	ret.V[3].A[1] = b.V[3].A[0] * a.V[0].A[1] + b.V[3].A[1] * a.V[1].A[1] + b.V[3].A[2] * a.V[2].A[1] + b.V[3].A[3] * a.V[3].A[1];
	ret.V[3].A[2] = b.V[3].A[0] * a.V[0].A[2] + b.V[3].A[1] * a.V[1].A[2] + b.V[3].A[2] * a.V[2].A[2] + b.V[3].A[3] * a.V[3].A[2];
	ret.V[3].A[3] = b.V[3].A[0] * a.V[0].A[3] + b.V[3].A[1] * a.V[1].A[3] + b.V[3].A[2] * a.V[2].A[3] + b.V[3].A[3] * a.V[3].A[3];  
	return ret;
}

Matrix Matrix::operator*( const Matrix& other ) const
{
	return Mul( other, *this );
}

Vector Matrix::TransformVector( const Vector& a ) const
{
	Vector out;
	out.X = (V[0].A[0] * a.X) + (V[1].A[0] * a.Y) + (V[2].A[0] * a.Z);
	out.Y = (V[0].A[1] * a.X) + (V[1].A[1] * a.Y) + (V[2].A[1] * a.Z);
	out.Z = (V[0].A[2] * a.X) + (V[1].A[2] * a.Y) + (V[2].A[2] * a.Z);
	out.W = 0.0f;
	return out;
}

Vector Matrix::TransformVectorWithW( const Vector& a ) const
{
	Vector out;
	out.X = (V[0].A[0] * a.X) + (V[1].A[0] * a.Y) + (V[2].A[0] * a.Z) + (V[3].A[0] * a.W);
	out.Y = (V[0].A[1] * a.X) + (V[1].A[1] * a.Y) + (V[2].A[1] * a.Z) + (V[3].A[1] * a.W);
	out.Z = (V[0].A[2] * a.X) + (V[1].A[2] * a.Y) + (V[2].A[2] * a.Z) + (V[3].A[2] * a.W);
	out.W = (V[0].A[3] * a.X) + (V[1].A[3] * a.Y) + (V[2].A[3] * a.Z) + (V[3].A[3] * a.W);
	return out;
}

Vector Matrix::TransformVectorAsPoint( const Vector& a ) const
{
	Vector out;
	out.X = (V[0].A[0] * a.X) + (V[1].A[0] * a.Y) + (V[2].A[0] * a.Z) + V[3].A[0];
	out.Y = (V[0].A[1] * a.X) + (V[1].A[1] * a.Y) + (V[2].A[1] * a.Z) + V[3].A[1];
	out.Z = (V[0].A[2] * a.X) + (V[1].A[2] * a.Y) + (V[2].A[2] * a.Z) + V[3].A[2];
	out.W = 1.0f;
	return out;
}

Vector Matrix::TransformPoint( const Vector& a ) const
{
	Vector out;
	out.X = (V[0].A[0] * a.X) + (V[1].A[0] * a.Y) + (V[2].A[0] * a.Z) + V[3].A[0];
	out.Y = (V[0].A[1] * a.X) + (V[1].A[1] * a.Y) + (V[2].A[1] * a.Z) + V[3].A[1];
	out.Z = (V[0].A[2] * a.X) + (V[1].A[2] * a.Y) + (V[2].A[2] * a.Z) + V[3].A[2];
	out.W = 1.0f;
	return out;
}

Box Matrix::TransformBox( const Box& box ) const
{
	Vector corners[ 8 ];
	corners[0] = Vector::Permute( box.Min, box.Max, 0, 0, 0, 0 );
	corners[1] = Vector::Permute( box.Min, box.Max, 0, 0, 1, 0 );
	corners[2] = Vector::Permute( box.Min, box.Max, 0, 1, 0, 0 );
	corners[3] = Vector::Permute( box.Min, box.Max, 0, 1, 1, 0 );
	corners[4] = Vector::Permute( box.Min, box.Max, 1, 0, 0, 0 );
	corners[5] = Vector::Permute( box.Min, box.Max, 1, 0, 1, 0 );
	corners[6] = Vector::Permute( box.Min, box.Max, 1, 1, 0, 0 );
	corners[7] = Vector::Permute( box.Min, box.Max, 1, 1, 1, 0 );

	Box newBox;
	newBox.Clear();

	for ( Uint32 i=0; i<8; i++ )
	{
		newBox.AddPoint( TransformPoint( corners[i] ) );
	}

	return newBox;
}

Matrix& Matrix::BuildPerspectiveLH( Float fovy, Float aspect, Float zn, Float zf )
{
	const Float ys = 1.0f / MTan( fovy * 0.5f );
	const Float xs = ys / aspect;
	const Float zs = zf/(zf-zn);

	V[0].Set4( xs, 0.0f, 0.0f, 0.0f );
	V[1].Set4( 0.0f, ys, 0.0f, 0.0f );
	V[2].Set4( 0.0f, 0.0f, zs, 1.0f );
	V[3].Set4( 0.0f, 0.0f, -zn*zs, 0.0f );

	return (*this);
}

Matrix& Matrix::BuildOrthoLH( Float w, Float h, Float zn, Float zf )
{
	V[0].Set4( 2.0f/w, 0.0f, 0.0f, 0.0f );
	V[1].Set4( 0.0f, 2.0f/h, 0.0f, 0.0f );
	V[2].Set4( 0.0f, 0.0f, 1.0f/(zf-zn), 0.0f );
	V[3].Set4( 0.0f, 0.0f, -zn/(zf-zn), 1.0f );

	return (*this);
}

Matrix& Matrix::BuildFromDirectionVector( const Vector& dirVec )
{
	// EY from direction, EZ pointing up
	V[1] = dirVec.Normalized3();
	V[2] = Vector::EZ;

	// EX as cross product of EY and EZ
	V[0] = Vector::Cross( V[1], V[2] );
	if ( V[0].Normalize3() > NumericLimits< Float >::Epsilon() )
	{
		// EZ as cross product of EX and EY ( to orthogonalize );
		V[2] = Vector::Cross( V[0], V[1] ).Normalized3();
		
	}
	else
	{
		SetRotX33( (dirVec.Z < 0.f) ? -M_PI / 2.f : M_PI / 2.f );
	}

	V[0].W = 0.0f;
	V[1].W = 0.0f;
	V[2].W = 0.0f;
	V[3] = Vector::EW;
	

	return (*this);
}

Matrix& Matrix::BuildFromQuaternion( const Vector& quaternion )
{
	float xx = quaternion.X * quaternion.X;
	float xy = quaternion.X * quaternion.Y;
	float xz = quaternion.X * quaternion.Z;
	float xw = quaternion.X * quaternion.W;
	float yy = quaternion.Y * quaternion.Y;
	float yz = quaternion.Y * quaternion.Z;
	float yw = quaternion.Y * quaternion.W;
	float zz = quaternion.Z * quaternion.Z;
	float zw = quaternion.Z * quaternion.W;

	V[0].Set4(
		1.f - 2.f * (yy + zz),
		2.f * (xy - zw),
		2.f * (xz + yw),
		0.f);
	V[1].Set4(
		2.f * (xy + zw),
		1.f - 2.f * (xx + zz),
		2.f * (yz - xw),
		0.f);
	V[2].Set4(
		2.f * (xz - yw),
		2.f * (yz + xw),
		1.f - 2.f * (xx + yy),
		0.f);
	V[3].Set4(0.f, 0.f, 0.f, 1.f);

	return (*this);
}

EulerAngles Matrix::ToEulerAngles( ) const
{
	EulerAngles ret;

	if ( GetRow(1).A[2] > 0.995f )
	{
		ret.Roll  =  RAD2DEG( atan2f( GetRow(2).A[0], GetRow(0).A[0] ) );
		ret.Pitch =  90.0f;
		ret.Yaw	  =  0.0f;
	}
	else if ( GetRow(1).A[2] < -0.995f )
	{
		ret.Roll  =  RAD2DEG( atan2f( GetRow(2).A[0], GetRow(0).A[0] ) );
		ret.Pitch = -90.0f;
		ret.Yaw	  =  0.0f;
	}
	else
	{
		ret.Roll  = -RAD2DEG( atan2f( GetRow(0).A[2], GetRow(2).A[2] ) );
		ret.Pitch =  RAD2DEG( asin( GetRow(1).A[2] ) );
		ret.Yaw	  = -RAD2DEG( atan2f( GetRow(1).A[0], GetRow(1).A[1] ) );
	}

	return ret;
}

EulerAngles Matrix::ToEulerAnglesFull( ) const
{
	EulerAngles ret;

	Float row1Mag = GetRow(1).Mag3();
	RED_ASSERT( row1Mag );
	Float row2MagSqr = GetRow(2).SquareMag3();
	RED_ASSERT( row2MagSqr );

	Float unscaleR1     = 1.f / row1Mag;
	Float rescaleR2toR0 = MSqrt( ( GetRow(0).SquareMag3() / row2MagSqr ) );

	Float cell12 = GetRow(1).A[2] * unscaleR1;
	if ( cell12 > 0.99999f )
	{
		ret.Roll  =  RAD2DEG( atan2f( GetRow(2).A[0] * rescaleR2toR0, GetRow(0).A[0] ) );
		ret.Pitch =  90.0f;
		ret.Yaw	  =  0.0f;
	}
	else if ( cell12 < -0.99999f )
	{
		ret.Roll  =  RAD2DEG( atan2f( GetRow(2).A[0] * rescaleR2toR0, GetRow(0).A[0] ) );
		ret.Pitch = -90.0f;
		ret.Yaw	  =  0.0f;
	}
	else
	{
		ret.Roll  = -RAD2DEG( atan2f( GetRow(0).A[2], GetRow(2).A[2] * rescaleR2toR0 ) );
		ret.Pitch =  RAD2DEG( asin( cell12 ) );
		ret.Yaw	  = -RAD2DEG( atan2f( GetRow(1).A[0], GetRow(1).A[1] ) ); // unscaling is not needed, both arguments has same scale
	}

	return ret;
}

Float Matrix::GetYaw( ) const
{
	// This was originally here but fucked up the camera while going through streaming doors
	//Float a2 = MAbs( GetRow(1).A[2] ) * 0.001f;
	//if ( a2 > MAbs( GetRow(1).A[0] ) || a2 > MAbs( GetRow(1).A[1] ) )
	//{
	//	return 0.f;
	//}

	const Vector& forward = GetRow(1);
	return -RAD2DEG( atan2f( forward.A[0], forward.A[1] ) ); // unscaling is not needed, both arguments has same scale
}

void Matrix::ExtractScale( Matrix &trMatrix, Vector& scale ) const
{
	// Just copy the matrix
	trMatrix = *this;

    Vector xAxis = trMatrix.GetRow(0);
    Vector yAxis = trMatrix.GetRow(1);
    Vector zAxis = trMatrix.GetRow(2);

    // Extract scale and normalize axes
    scale.Set3( xAxis.Normalize3(), yAxis.Normalize3(), zAxis.Normalize3() );
    trMatrix.SetRows( xAxis, yAxis, zAxis, Vector::EW );
}

void Matrix::ToAngleVectors( Vector* forward, Vector* right, Vector* up ) const
{
	Vector f( 0, 1, 0 );
	Vector r( 1, 0, 0 );
	Vector u( 0, 0, 1 );

	if ( forward )
	{
		forward->Set3( TransformVector( f ) );
	}

	if ( right )
	{
		right->Set3( TransformVector( r ) );
	}

	if ( up )
	{
		up->Set3( TransformVector( u ) );
	}
}

Vector Matrix::ToQuat() const
{
	float tr = GetRow(0).A[0] + GetRow(1).A[1] + GetRow(2).A[2] + 1.f;
	float qw;
	float qx;
	float qy;
	float qz;

	if (tr > 0.f) 
	{ 
		float S = sqrtf(tr) * 2.f; // S=4*qw 
		qw = 0.25f * S;
		qx = (GetRow(1).A[2] - GetRow(2).A[1]) / S;
		qy = (GetRow(2).A[0] - GetRow(0).A[2]) / S; 
		qz = (GetRow(0).A[1] - GetRow(1).A[0]) / S; 
	} 
	else if ( (GetRow(0).A[0] > GetRow(1).A[1]) && (GetRow(0).A[0] > GetRow(2).A[2]) ) 
	{ 
		float S = sqrtf(1.0f + GetRow(0).A[0] - GetRow(1).A[1] - GetRow(2).A[2]) * 2.f; // S=4*qx 
		qw = (GetRow(1).A[2] - GetRow(2).A[1]) / S;
		qx = 0.25f * S;
		qy = (GetRow(1).A[0] + GetRow(0).A[1]) / S; 
		qz = (GetRow(2).A[0] + GetRow(0).A[2]) / S; 
	} 
	else if (GetRow(1).A[1] > GetRow(2).A[2]) 
	{ 
		float S = sqrtf(1.0f + GetRow(1).A[1] - GetRow(0).A[0] - GetRow(2).A[2]) * 2.f; // S=4*qy
		qw = (GetRow(2).A[0] - GetRow(0).A[2]) / S;
		qx = (GetRow(1).A[0] + GetRow(0).A[1]) / S; 
		qy = 0.25f * S;
		qz = (GetRow(2).A[1] + GetRow(1).A[2]) / S;
	} 
	else 
	{ 
		float S = sqrtf(1.0f + GetRow(2).A[2] - GetRow(0).A[0] - GetRow(1).A[1]) * 2.f; // S=4*qz
		qw = (GetRow(0).A[1] - GetRow(1).A[0]) / S;
		qx = (GetRow(2).A[0] + GetRow(0).A[2]) / S;
		qy = (GetRow(2).A[1] + GetRow(1).A[2]) / S;
		qz = 0.25f * S;
	}

	Vector q;
	q.W = qw;
	q.X = qx;
	q.Y = qy;
	q.Z = qz;
	return q;
}

Bool Matrix::IsOk() const
{
	return V[ 0 ].IsOk() && V[ 1 ].IsOk() && V[ 2 ].IsOk() && V[ 3 ].IsOk();
}

Bool Matrix::Equal( const Matrix& a, const Matrix& b )
{
	return Vector::Equal4( a.V[3], b.V[3] ) // Translation first
		&& Vector::Equal4( a.V[0], b.V[0] )
		&& Vector::Equal4( a.V[1], b.V[1] )
		&& Vector::Equal4( a.V[2], b.V[2] );
}

Bool Matrix::Near( const Matrix& a, const Matrix& b, Float eps )
{
	return Vector::Near4( a.V[3], b.V[3], eps ) // Translation first
		&& Vector::Near4( a.V[0], b.V[0], eps )
		&& Vector::Near4( a.V[1], b.V[1], eps )
		&& Vector::Near4( a.V[2], b.V[2], eps );
}
