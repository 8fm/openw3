/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red 
{ 
	namespace Math
	{
		namespace Fpu
		{
			// *** RedMatrix3x3
			RedMatrix3x3::RedMatrix3x3()
			{
			}

			RedMatrix3x3::RedMatrix3x3( const float* _f )
			{
				Matrix[0] = RedVector3( &_f[0] );
				Matrix[1] = RedVector3( &_f[4] );
				Matrix[2] = RedVector3( &_f[8] );
			}

			RedMatrix3x3::RedMatrix3x3( const RedMatrix3x3& _m )
			{
				Matrix[0] = _m.Matrix[0];
				Matrix[1] = _m.Matrix[1];
				Matrix[2] = _m.Matrix[2];
			}

			RedMatrix3x3::RedMatrix3x3( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 )
			{
				Matrix[0] = _v0;
				Matrix[1] = _v1;
				Matrix[2] = _v2;
			}

			RedMatrix3x3 RedMatrix3x3::operator+( const RedMatrix3x3& _m ) const
			{
				return RedMatrix3x3( Matrix[0] + _m.Matrix[0], 
									 Matrix[1] + _m.Matrix[1], 
									 Matrix[2] + _m.Matrix[2] );
			}

			bool RedMatrix3x3::operator==( const RedMatrix3x3& _m ) const
			{
				return ( Matrix[0] == _m.Matrix[0] ) && ( Matrix[1] == _m.Matrix[1] ) && ( Matrix[2] == _m.Matrix[2] );
			}

			bool RedMatrix3x3::operator!=( const RedMatrix3x3& _m ) const
			{
				return ( Matrix[0] != _m.Matrix[0] ) && ( Matrix[1] != _m.Matrix[1] ) && ( Matrix[2] != _m.Matrix[2] );
			}

			bool RedMatrix3x3::IsAlmostEqual( const RedMatrix3x3& _m, float _epsilon ) const
			{
				return Matrix[0].IsAlmostEqual( _m.Matrix[0], _epsilon ) && Matrix[1].IsAlmostEqual( _m.Matrix[1], _epsilon ) && Matrix[2].IsAlmostEqual( _m.Matrix[2], _epsilon );
			}

			RedMatrix3x3& RedMatrix3x3::operator=( const RedMatrix3x3& _m )
			{
				Matrix[0] = _m.Matrix[0];
				Matrix[1] = _m.Matrix[1];
				Matrix[2] = _m.Matrix[2];
				return *this;
			}

			RedMatrix3x3 RedMatrix3x3::operator*( const RedMatrix3x3& _m )
			{
				return RedMatrix3x3::Mul( *this, _m );
			}

			RedMatrix3x3 RedMatrix3x3::Mul( const RedMatrix3x3& _a, const RedMatrix3x3& _b )
			{
				RedMatrix3x3 result;
				result.Matrix[0].V[0] = ( _a.Matrix[0].V[0] * _b.Matrix[0].V[0] ) +
										( _a.Matrix[0].V[1] * _b.Matrix[1].V[0] ) +
										( _a.Matrix[0].V[2] * _b.Matrix[2].V[0] );

				result.Matrix[0].V[1] = ( _a.Matrix[0].V[0] * _b.Matrix[0].V[1] ) +
										( _a.Matrix[0].V[1] * _b.Matrix[1].V[1] ) +
										( _a.Matrix[0].V[2] * _b.Matrix[2].V[1] );

				result.Matrix[0].V[2] = ( _a.Matrix[0].V[0] * _b.Matrix[0].V[2] ) +
										( _a.Matrix[0].V[1] * _b.Matrix[1].V[2] ) +
										( _a.Matrix[0].V[2] * _b.Matrix[2].V[2] );


				result.Matrix[1].V[0] = ( _a.Matrix[1].V[0] * _b.Matrix[0].V[0] ) +
										( _a.Matrix[1].V[1] * _b.Matrix[1].V[0] ) +
										( _a.Matrix[1].V[2] * _b.Matrix[2].V[0] );

				result.Matrix[1].V[1] = ( _a.Matrix[1].V[0] * _b.Matrix[0].V[1] ) +
										( _a.Matrix[1].V[1] * _b.Matrix[1].V[1] ) +
										( _a.Matrix[1].V[2] * _b.Matrix[2].V[1] );

				result.Matrix[1].V[2] = ( _a.Matrix[1].V[0] * _b.Matrix[0].V[2] ) +
										( _a.Matrix[1].V[1] * _b.Matrix[1].V[2] ) +
										( _a.Matrix[1].V[2] * _b.Matrix[2].V[2] );


				result.Matrix[2].V[0] = ( _a.Matrix[2].V[0] * _b.Matrix[0].V[0] ) +
										( _a.Matrix[2].V[1] * _b.Matrix[1].V[0] ) +
										( _a.Matrix[2].V[2] * _b.Matrix[2].V[0] );

				result.Matrix[2].V[1] = ( _a.Matrix[2].V[0] * _b.Matrix[0].V[1] ) +
										( _a.Matrix[2].V[1] * _b.Matrix[1].V[1] ) +
										( _a.Matrix[2].V[2] * _b.Matrix[2].V[1] );

				result.Matrix[2].V[2] = ( _a.Matrix[2].V[0] * _b.Matrix[0].V[2] ) +
										( _a.Matrix[2].V[1] * _b.Matrix[1].V[2] ) +
										( _a.Matrix[2].V[2] * _b.Matrix[2].V[2] );

				return result;
			}

			RedMatrix3x3& RedMatrix3x3::Set( const RedMatrix3x3& _m )
			{
				Matrix[0] = _m.Matrix[0];
				Matrix[1] = _m.Matrix[1];
				Matrix[2] = _m.Matrix[2];
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::Set( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 )
			{
				Matrix[0] = _v0;
				Matrix[1] = _v1;
				Matrix[2] = _v2;
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetRows( const float* _f )
			{
				Matrix[0] = RedVector3( &_f[0] );
				Matrix[1] = RedVector3( &_f[4] );
				Matrix[2] = RedVector3( &_f[8] );
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetCols( const float* _f )
			{
				// This may not be correct - will correct when tested.
				Matrix[0].V[0] = _f[0];
				Matrix[0].V[1] = _f[4];
				Matrix[0].V[2] = _f[8];
				Matrix[1].V[0] = _f[1];
				Matrix[1].V[1] = _f[5];
				Matrix[1].V[2] = _f[9];
				Matrix[2].V[0] = _f[2];
				Matrix[2].V[1] = _f[6];
				Matrix[2].V[2] = _f[10];
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetRows( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
			{
				Matrix[0] = _r0;
				Matrix[1] = _r1;
				Matrix[2] = _r2;
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetCols( const RedVector3& _c0, const RedVector3& _c1, const RedVector3& _c2 )
			{
				Matrix[0].V[0] = _c0.V[0];
				Matrix[1].V[0] = _c0.V[1];
				Matrix[2].V[0] = _c0.V[2];
				Matrix[0].V[1] = _c1.V[0];
				Matrix[1].V[1] = _c1.V[1];
				Matrix[2].V[1] = _c1.V[2];	
				Matrix[0].V[2] = _c2.V[0];
				Matrix[1].V[2] = _c2.V[1];
				Matrix[2].V[2] = _c2.V[2];	
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetZeros()
			{
				Matrix[0].SetZeros();
				Matrix[1].SetZeros();
				Matrix[2].SetZeros();
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetIdentity()
			{
				SetZeros();
				Matrix[0].V[0] = 1.0f;
				Matrix[1].V[1] = 1.0f;
				Matrix[2].V[2] = 1.0f;
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetColumn( int _index, const RedVector3& _v )
			{
				Matrix[0].V[_index] = _v.V[0];
				Matrix[1].V[_index] = _v.V[1];
				Matrix[2].V[_index] = _v.V[2];
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetRow( int _index, const RedVector3& _v )
			{
				Matrix[_index] = _v;
				return *this;
			}

			RedVector3 RedMatrix3x3::GetColumn( int _index ) const
			{
				return RedVector3( Matrix[0].V[_index],
									Matrix[1].V[_index],
									Matrix[2].V[_index] );
			}

			const RedVector3& RedMatrix3x3::GetRow( int _index ) const
			{
				return Matrix[_index];
			}

			RedVector3 RedMatrix3x3::GetAxisX() const
			{
				return RedVector3( Matrix[0] );
			}

			RedVector3 RedMatrix3x3::GetAxisY() const
			{
				return RedVector3( Matrix[1] );
			}

			RedVector3 RedMatrix3x3::GetAxisZ() const
			{
				return RedVector3( Matrix[2] );
			}

			RedMatrix3x3& RedMatrix3x3::SetRotX( float _ccwRadians )
			{
				float rotC = ::cosf( _ccwRadians );
				float rotS = ::sinf( _ccwRadians );
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

				Matrix[0].V[0] = 1.0f;
				Matrix[0].V[1] = 0.0f;
				Matrix[0].V[2] = 0.0f;
				Matrix[1].V[0] = 0.0f;
				Matrix[1].V[1] = rotC;
				Matrix[1].V[2] = rotS;
				Matrix[2].V[0] = 0.0f;
				Matrix[2].V[1] = -rotS;
				Matrix[2].V[2] = rotC;

				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetRotY( float _ccwRadians )
			{
				float rotC = ::cosf( _ccwRadians );
				float rotS = ::sinf( _ccwRadians );

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

				Matrix[0].V[0] = rotC;
				Matrix[0].V[1] = 0.0f;
				Matrix[0].V[2] = -rotS;
				Matrix[1].V[0] = 0.0f;
				Matrix[1].V[1] = 1.0f;
				Matrix[1].V[2] = 0.0f;
				Matrix[2].V[0] = rotS;
				Matrix[2].V[1] = 0.0f;
				Matrix[2].V[2] = rotC;
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetRotZ( float _ccwRadians )
			{
				float rotC = ::cosf( _ccwRadians );
				float rotS = ::sinf( _ccwRadians );

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

				Matrix[0].V[0] = rotC;
				Matrix[0].V[1] = rotS;
				Matrix[0].V[2] = 0.0f;
				Matrix[1].V[0] = -rotS;
				Matrix[1].V[1] = rotC;
				Matrix[1].V[2] = 0.0f;
				Matrix[2].V[0] = 0.0f;
				Matrix[2].V[1] = 0.0f;
				Matrix[2].V[2] = 1.0f;
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetScale( const RedVector3& _scale )
			{
				Matrix[0].V[0] *= _scale.V[0];
				Matrix[0].V[1] *= _scale.V[0];
				Matrix[0].V[2] *= _scale.V[0];
				Matrix[1].V[0] *= _scale.V[1];
				Matrix[1].V[1] *= _scale.V[1];
				Matrix[1].V[2] *= _scale.V[1];
				Matrix[2].V[0] *= _scale.V[2];
				Matrix[2].V[1] *= _scale.V[2];
				Matrix[2].V[2] *= _scale.V[2];
				return *this;	
			}

			RedMatrix3x3& RedMatrix3x3::SetPreScale( const RedVector3& _scale )
			{
				Matrix[0].V[0] *= _scale.V[0];
				Matrix[0].V[1] *= _scale.V[1];
				Matrix[0].V[2] *= _scale.V[2];
				Matrix[1].V[0] *= _scale.V[0];
				Matrix[1].V[1] *= _scale.V[1];
				Matrix[1].V[2] *= _scale.V[2];
				Matrix[2].V[0] *= _scale.V[0];
				Matrix[2].V[1] *= _scale.V[1];
				Matrix[2].V[2] *= _scale.V[2];
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetScale( RedVector1& _uniformScale )
			{
				Matrix[0] *= _uniformScale;
				Matrix[1] *= _uniformScale;
				Matrix[2] *= _uniformScale;
				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::SetScale( float _uniformScale )
			{
				Matrix[0].V[0] *= _uniformScale;
				Matrix[0].V[1] *= _uniformScale;
				Matrix[0].V[2] *= _uniformScale;
				Matrix[1].V[0] *= _uniformScale;
				Matrix[1].V[1] *= _uniformScale;
				Matrix[1].V[2] *= _uniformScale;
				Matrix[2].V[0] *= _uniformScale;
				Matrix[2].V[1] *= _uniformScale;
				Matrix[2].V[2] *= _uniformScale;
				return *this;
			}

			RedVector3 RedMatrix3x3::GetScale() const
			{
				return RedVector3( Matrix[0].Length(), Matrix[1].Length(), Matrix[2].Length() );
			}

			RedVector3 RedMatrix3x3::GetPreScale() const
			{
				RedVector3 v0( Matrix[0].V[0], Matrix[1].V[0], Matrix[2].V[0] );
				RedVector3 v1( Matrix[0].V[1], Matrix[1].V[1], Matrix[2].V[1] );
				RedVector3 v2( Matrix[0].V[2], Matrix[1].V[2], Matrix[2].V[2] );
				return RedVector3( v0.Length(), v1.Length(), v2.Length() );
			}

			RedMatrix3x3& RedMatrix3x3::BuildFromDirectionVector( const RedVector3& _dirVec )
			{
				// EY from direction, EZ pointing up
				Matrix[1] = _dirVec.Normalized();
				Matrix[2] = RedVector3( 0, 0, 1 );

				// EX as cross product of EY and EZ
				Matrix[0] = RedVector3::Cross( Matrix[1], Matrix[2] ).Normalized();
				// EZ as cross product of EX and EY ( to orthogonalize );
				Matrix[2] = RedVector3::Cross( Matrix[0], Matrix[1] ).Normalized();

				return *this;
			}

			RedMatrix3x3& RedMatrix3x3::BuildFromQuaternion( const RedVector4& _quaternion )
			{
				float xx = _quaternion.X * _quaternion.X;
				float xy = _quaternion.X * _quaternion.Y;
				float xz = _quaternion.X * _quaternion.Z;
				float xw = _quaternion.X * _quaternion.W;
				float yy = _quaternion.Y * _quaternion.Y;
				float yz = _quaternion.Y * _quaternion.Z;
				float yw = _quaternion.Y * _quaternion.W;
				float zz = _quaternion.Z * _quaternion.Z;
				float zw = _quaternion.Z * _quaternion.W;

				Matrix[0].Set(
					1.0f - 2.0f * (yy + zz),
					2.0f * (xy - zw),
					2.0f * (xz + yw)
					);
				Matrix[1].Set(
					2.0f * (xy + zw),
					1.0f - 2.0f * (xx + zz),
					2.0f * (yz - xw)
					);
				Matrix[2].Set(
					2.0f * (xz - yw),
					2.0f * (yz + xw),
					1.0f - 2.0f * (xx + yy)
					);
				return *this;
			}

			float RedMatrix3x3::Det() const
			{
				float det = 0.0f; 
				det += Matrix[0].V[0] * Matrix[1].V[1] * Matrix[2].V[2]; // 123
				det += Matrix[0].V[1] * Matrix[1].V[2] * Matrix[2].V[0]; // 231
				det += Matrix[0].V[2] * Matrix[1].V[0] * Matrix[2].V[1]; // 312
				det -= Matrix[0].V[2] * Matrix[1].V[1] * Matrix[2].V[0]; // 321
				det -= Matrix[0].V[1] * Matrix[1].V[0] * Matrix[2].V[2]; // 213
				det -= Matrix[0].V[0] * Matrix[1].V[2] * Matrix[2].V[1]; // 132 
				return det;
			}

			RedMatrix3x3& RedMatrix3x3::Invert()
			{
				*this = Inverted();
				return *this;
			}

			RedMatrix3x3 RedMatrix3x3::Inverted() const
			{
				RedMatrix3x3 out;

				// Transpose the 3x3 inner matrix
				out.Matrix[0].V[0] = Matrix[0].V[0];
				out.Matrix[0].V[1] = Matrix[1].V[0];
				out.Matrix[0].V[2] = Matrix[2].V[0];
				out.Matrix[1].V[0] = Matrix[0].V[1];
				out.Matrix[1].V[1] = Matrix[1].V[1];
				out.Matrix[1].V[2] = Matrix[2].V[1];
				out.Matrix[2].V[0] = Matrix[0].V[2];
				out.Matrix[2].V[1] = Matrix[1].V[2];
				out.Matrix[2].V[2] = Matrix[2].V[2];

				return out;
			}

			RedMatrix3x3& RedMatrix3x3::Transpose()
			{
				*this = Transposed();
				return *this;
			}

			RedMatrix3x3 RedMatrix3x3::Transposed() const
			{
				RedMatrix3x3 out;

				// Transpose the 3x3 inner matrix
				out.Matrix[0].V[0] = Matrix[0].V[0];
				out.Matrix[0].V[1] = Matrix[1].V[0];
				out.Matrix[0].V[2] = Matrix[2].V[0];
				out.Matrix[1].V[0] = Matrix[0].V[1];
				out.Matrix[1].V[1] = Matrix[1].V[1];
				out.Matrix[1].V[2] = Matrix[2].V[1];
				out.Matrix[2].V[0] = Matrix[0].V[2];
				out.Matrix[2].V[1] = Matrix[1].V[2];
				out.Matrix[2].V[2] = Matrix[2].V[2];

				return out;
			}

			RedVector3 RedMatrix3x3::TransformVector( const RedVector3& _v ) const
			{
				RedVector3 result;

				result.X = ( Matrix[0].V[0] * _v.V[0] ) + 
							( Matrix[1].V[0] * _v.V[1] ) + 
							( Matrix[2].V[0] * _v.V[2] );

				result.Y = ( Matrix[0].V[1] * _v.V[0] ) +
							( Matrix[1].V[1] * _v.V[1] ) + 
							( Matrix[2].V[1] * _v.V[2] );

				result.Z = ( Matrix[0].V[2] * _v.V[0] ) + 
							( Matrix[1].V[2] * _v.V[1] ) + 
							( Matrix[2].V[2] * _v.V[2] );
				return result;
			}

			void RedMatrix3x3::ExtractScale( RedMatrix3x3& _trMatrix, RedVector3& _scale ) const
			{
				// Just copy the matrix
				_trMatrix = *this;

				RedVector3 xAxis = _trMatrix.GetRow(0);
				RedVector3 yAxis = _trMatrix.GetRow(1);
				RedVector3 zAxis = _trMatrix.GetRow(2);
					   
				// Extract magnitude of the vectors on normalize axes
				_scale.Set( xAxis.Length(), yAxis.Length(), zAxis.Length() );

				// Set the rows to normalized version of the axes.
				_trMatrix.SetRows( xAxis.Normalize(), yAxis.Normalize(), zAxis.Normalize() );
			}

			bool RedMatrix3x3::IsOk() const
			{
				return ( Matrix[0].IsOk() && Matrix[1].IsOk() && Matrix[2].IsOk() );
			}

			// *** RedMatrix4x4
			RedMatrix4x4::RedMatrix4x4()
			{
			}

			RedMatrix4x4::RedMatrix4x4( const float* _f )
			{
				Matrix[0] = RedVector4( &_f[0] );
				Matrix[1] = RedVector4( &_f[4] );
				Matrix[2] = RedVector4( &_f[8] );
				Matrix[3] = RedVector4( &_f[12] );
			}

			RedMatrix4x4::RedMatrix4x4( const RedMatrix4x4& _m )
			{
				Matrix[0] = _m.Matrix[0];
				Matrix[1] = _m.Matrix[1];
				Matrix[2] = _m.Matrix[2];
				Matrix[3] = _m.Matrix[3];
			}

			RedMatrix4x4::RedMatrix4x4( const RedMatrix3x3& _m )
			{
				Matrix[0] = RedVector4( _m.Matrix[0], 0.0f );
				Matrix[1] = RedVector4( _m.Matrix[1], 0.0f );
				Matrix[2] = RedVector4( _m.Matrix[2], 0.0f );
				Matrix[3] = RedVector4( 0.0f, 0.0f, 0.0f, 1.0f );
			}

			RedMatrix4x4::RedMatrix4x4( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
			{
				Matrix[0] = _r0;
				Matrix[1] = _r1;
				Matrix[2] = _r2;
				Matrix[3] = _r3;
			}

			RedMatrix4x4::RedMatrix4x4( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
			{
				Matrix[0] = RedVector4(_r0, 0.0f );
				Matrix[1] = RedVector4(_r1, 0.0f );
				Matrix[2] = RedVector4(_r2, 0.0f );
				Matrix[3] = RedVector4(0.0f, 0.0f, 0.0f, 1.0f );
			}

			bool RedMatrix4x4::operator==( const RedMatrix4x4& _m ) const
			{
				return ( Matrix[0] == _m.Matrix[0] ) && ( Matrix[1] == _m.Matrix[1] ) && ( Matrix[2] == _m.Matrix[2] ) && ( Matrix[3] == _m.Matrix[3] );
			}

			bool RedMatrix4x4::operator==( const RedMatrix3x3& _m ) const
			{
				return ( Matrix[0] == _m.Matrix[0] ) && ( Matrix[1] == _m.Matrix[1] ) && ( Matrix[2] == _m.Matrix[2] );
			}

			bool RedMatrix4x4::operator!=( const RedMatrix4x4& _m ) const
			{
				return ( Matrix[0] != _m.Matrix[0] ) && ( Matrix[1] != _m.Matrix[1] ) && ( Matrix[2] != _m.Matrix[2] ) && ( Matrix[3] != _m.Matrix[3] );
			}

			bool RedMatrix4x4::operator!=( const RedMatrix3x3& _m ) const
			{
				return ( Matrix[0] != _m.Matrix[0] ) && ( Matrix[1] != _m.Matrix[1] ) && ( Matrix[2] != _m.Matrix[2] );
			}

			RedMatrix4x4& RedMatrix4x4::operator=( const RedMatrix4x4& _m )
			{
				Matrix[0] = _m.Matrix[0];
				Matrix[1] = _m.Matrix[1];
				Matrix[2] = _m.Matrix[2];
				Matrix[3] = _m.Matrix[3];
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::operator=( const RedMatrix3x3& _m )
			{
				Matrix[0] = RedVector4( _m.Matrix[0], 0.0f );
				Matrix[1] = RedVector4( _m.Matrix[0], 0.0f );
				Matrix[2] = RedVector4( _m.Matrix[0], 0.0f );
				Matrix[3] = RedVector4( 0.0f, 0.0f, 0.0f, 1.0f );
				return *this;
			}

			RedMatrix4x4 RedMatrix4x4::operator*( const RedMatrix4x4& _m ) const
			{
				return RedMatrix4x4::Mul( _m, *this );
			}

			RedMatrix4x4 RedMatrix4x4::Mul( const RedMatrix4x4& _a, const RedMatrix4x4& _b )
			{
				RedMatrix4x4 result;
				result.Matrix[0].V[0] = ( _b.Matrix[0].V[0] * _a.Matrix[0].V[0] )+
										( _b.Matrix[0].V[1] * _a.Matrix[1].V[0] )+
										( _b.Matrix[0].V[2] * _a.Matrix[2].V[0] )+
										( _b.Matrix[0].V[3] * _a.Matrix[3].V[0] );

				result.Matrix[0].V[1] = ( _b.Matrix[0].V[0] * _a.Matrix[0].V[1] ) + 
										( _b.Matrix[0].V[1] * _a.Matrix[1].V[1] ) +
										( _b.Matrix[0].V[2] * _a.Matrix[2].V[1] ) + 
										( _b.Matrix[0].V[3] * _a.Matrix[3].V[1] );

				result.Matrix[0].V[2] = ( _b.Matrix[0].V[0] * _a.Matrix[0].V[2] ) +
										( _b.Matrix[0].V[1] * _a.Matrix[1].V[2] ) +
										( _b.Matrix[0].V[2] * _a.Matrix[2].V[2] ) +
										( _b.Matrix[0].V[3] * _a.Matrix[3].V[2] );

				result.Matrix[0].V[3] = ( _b.Matrix[0].V[0] * _a.Matrix[0].V[3] ) +
										( _b.Matrix[0].V[1] * _a.Matrix[1].V[3] ) +
										( _b.Matrix[0].V[2] * _a.Matrix[2].V[3] ) +
										( _b.Matrix[0].V[3] * _a.Matrix[3].V[3] );


				result.Matrix[1].V[0] = ( _b.Matrix[1].V[0] * _a.Matrix[0].V[0] ) +
										( _b.Matrix[1].V[1] * _a.Matrix[1].V[0] ) + 
										( _b.Matrix[1].V[2] * _a.Matrix[2].V[0] ) + 
										( _b.Matrix[1].V[3] * _a.Matrix[3].V[0] );

				result.Matrix[1].V[1] = ( _b.Matrix[1].V[0] * _a.Matrix[0].V[1] ) +
										( _b.Matrix[1].V[1] * _a.Matrix[1].V[1] ) +
										( _b.Matrix[1].V[2] * _a.Matrix[2].V[1] ) +
										( _b.Matrix[1].V[3] * _a.Matrix[3].V[1] );

				result.Matrix[1].V[2] = ( _b.Matrix[1].V[0] * _a.Matrix[0].V[2] ) +
										( _b.Matrix[1].V[1] * _a.Matrix[1].V[2] ) +
										( _b.Matrix[1].V[2] * _a.Matrix[2].V[2] ) +
										( _b.Matrix[1].V[3] * _a.Matrix[3].V[2] );

				result.Matrix[1].V[3] = ( _b.Matrix[1].V[0] * _a.Matrix[0].V[3] ) + 
										( _b.Matrix[1].V[1] * _a.Matrix[1].V[3] ) + 
										( _b.Matrix[1].V[2] * _a.Matrix[2].V[3] ) + 
										( _b.Matrix[1].V[3] * _a.Matrix[3].V[3] );


				result.Matrix[2].V[0] = ( _b.Matrix[2].V[0] * _a.Matrix[0].V[0] ) +
										( _b.Matrix[2].V[1] * _a.Matrix[1].V[0] ) +
										( _b.Matrix[2].V[2] * _a.Matrix[2].V[0] ) +
										( _b.Matrix[2].V[3] * _a.Matrix[3].V[0] );

				result.Matrix[2].V[1] = ( _b.Matrix[2].V[0] * _a.Matrix[0].V[1] ) +
										( _b.Matrix[2].V[1] * _a.Matrix[1].V[1] ) +
										( _b.Matrix[2].V[2] * _a.Matrix[2].V[1] ) +
										( _b.Matrix[2].V[3] * _a.Matrix[3].V[1] );

				result.Matrix[2].V[2] = ( _b.Matrix[2].V[0] * _a.Matrix[0].V[2] ) +
										( _b.Matrix[2].V[1] * _a.Matrix[1].V[2] ) +
										( _b.Matrix[2].V[2] * _a.Matrix[2].V[2] ) +
										( _b.Matrix[2].V[3] * _a.Matrix[3].V[2] );

				result.Matrix[2].V[3] = ( _b.Matrix[2].V[0] * _a.Matrix[0].V[3] ) +
										( _b.Matrix[2].V[1] * _a.Matrix[1].V[3] ) +
										( _b.Matrix[2].V[2] * _a.Matrix[2].V[3] ) +
										( _b.Matrix[2].V[3] * _a.Matrix[3].V[3] );


				result.Matrix[3].V[0] = ( _b.Matrix[3].V[0] * _a.Matrix[0].V[0] ) +
										( _b.Matrix[3].V[1] * _a.Matrix[1].V[0] ) +
										( _b.Matrix[3].V[2] * _a.Matrix[2].V[0] ) +
										( _b.Matrix[3].V[3] * _a.Matrix[3].V[0] );

				result.Matrix[3].V[1] = ( _b.Matrix[3].V[0] * _a.Matrix[0].V[1] ) +
										( _b.Matrix[3].V[1] * _a.Matrix[1].V[1] ) + 
										( _b.Matrix[3].V[2] * _a.Matrix[2].V[1] ) + 
										( _b.Matrix[3].V[3] * _a.Matrix[3].V[1] );

				result.Matrix[3].V[2] = ( _b.Matrix[3].V[0] * _a.Matrix[0].V[2] ) + 
										( _b.Matrix[3].V[1] * _a.Matrix[1].V[2] ) + 
										( _b.Matrix[3].V[2] * _a.Matrix[2].V[2] ) + 
										( _b.Matrix[3].V[3] * _a.Matrix[3].V[2] );

				result.Matrix[3].V[3] = ( _b.Matrix[3].V[0] * _a.Matrix[0].V[3] ) + 
										( _b.Matrix[3].V[1] * _a.Matrix[1].V[3] ) + 
										( _b.Matrix[3].V[2] * _a.Matrix[2].V[3] ) + 
										( _b.Matrix[3].V[3] * _a.Matrix[3].V[3] );  
				return result;
			}

			RedMatrix4x4& RedMatrix4x4::Set( const RedMatrix4x4& _m )
			{
				Matrix[0] = _m.Matrix[0];
				Matrix[1] = _m.Matrix[1];
				Matrix[2] = _m.Matrix[2];
				Matrix[3] = _m.Matrix[3];
				return (*this);
			}

			RedMatrix4x4& RedMatrix4x4::Set( const RedMatrix3x3& _m )
			{
				Matrix[0] = RedVector4( _m.Matrix[0], 0.0f );
				Matrix[1] = RedVector4( _m.Matrix[1], 0.0f );
				Matrix[2] = RedVector4( _m.Matrix[2], 0.0f );
				Matrix[3] = RedVector4( 0.0f, 0.0f, 0.0f, 1.0f );
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::Set( const RedVector3& _x, const RedVector3& _y, const RedVector3& _z )
			{
				Matrix[0] = RedVector4( _x, 0.0f );
				Matrix[1] = RedVector4( _y, 0.0f );
				Matrix[2] = RedVector4( _z, 0.0f );
				Matrix[3] = RedVector4( 0.0f, 0.0f, 0.0f, 1.0f );
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::Set( const RedVector4& _x, const RedVector4& _y, const RedVector4& _z, const RedVector4& _w )
			{
				Matrix[0] = _x;
				Matrix[1] = _y;
				Matrix[2] = _z;
				Matrix[3] = _w;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::Set33( const RedVector4& _x, const RedVector4& _y, const RedVector4& _z )
			{
				Matrix[0] = RedVector4( _x.AsVector3() );
				Matrix[1] = RedVector4( _y.AsVector3() );
				Matrix[2] = RedVector4( _z.AsVector3() );
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetRows( const float* _f )
			{
				Matrix[0] = RedVector3( &_f[0] );
				Matrix[1] = RedVector3( &_f[4] );
				Matrix[2] = RedVector3( &_f[8] );
				Matrix[3] = RedVector3( &_f[12] );
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetCols( const float* _f )
			{
				Matrix[0].V[0] = _f[0];
				Matrix[0].V[1] = _f[4];
				Matrix[0].V[2] = _f[8];
				Matrix[0].V[3] = _f[12];
				Matrix[1].V[0] = _f[1];
				Matrix[1].V[1] = _f[5];
				Matrix[1].V[2] = _f[9];
				Matrix[1].V[3] = _f[13];
				Matrix[2].V[0] = _f[2];
				Matrix[2].V[1] = _f[6];
				Matrix[2].V[2] = _f[10];
				Matrix[2].V[3] = _f[14];
				Matrix[3].V[0] = _f[3];
				Matrix[3].V[1] = _f[6];
				Matrix[3].V[2] = _f[11];
				Matrix[3].V[3] = _f[15];
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetRows( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
			{
				Matrix[0] = _r0;
				Matrix[1] = _r1;
				Matrix[2] = _r2;
				Matrix[3] = _r3;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetCols( const RedVector4& _c0, const RedVector4& _c1, const RedVector4& _c2, const RedVector4& _c3 )
			{
				Matrix[0].V[0] = _c0.V[0];
				Matrix[1].V[0] = _c0.V[1];
				Matrix[2].V[0] = _c0.V[2];
				Matrix[3].V[0] = _c0.V[3];	
				Matrix[0].V[1] = _c1.V[0];
				Matrix[1].V[1] = _c1.V[1];
				Matrix[2].V[1] = _c1.V[2];
				Matrix[3].V[1] = _c1.V[3];	
				Matrix[0].V[2] = _c2.V[0];
				Matrix[1].V[2] = _c2.V[1];
				Matrix[2].V[2] = _c2.V[2];
				Matrix[3].V[2] = _c2.V[3];	
				Matrix[0].V[3] = _c3.V[0];
				Matrix[1].V[3] = _c3.V[1];
				Matrix[2].V[3] = _c3.V[2];
				Matrix[3].V[3] = _c3.V[3];
				return *this;
			}

			// Column/Row access
			RedMatrix4x4& RedMatrix4x4::SetColumn( int _index, const RedVector4& _a )
			{
				Matrix[0].V[ _index ] = _a.V[0];
				Matrix[1].V[ _index ] = _a.V[1];
				Matrix[2].V[ _index ] = _a.V[2];
				Matrix[3].V[ _index ] = _a.V[3];
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetRow( int _index, const RedVector4& _a )
			{
				Matrix[_index] = _a;
				return *this;	
			}

			RedVector4 RedMatrix4x4::GetColumn( int _index ) const
			{
				return RedVector4( Matrix[0].V[_index],
									Matrix[0].V[_index],
									Matrix[0].V[_index] );
			}

			const RedVector4& RedMatrix4x4::GetRow( int _index ) const
			{
				return Matrix[_index];
			}

			RedVector4 RedMatrix4x4::GetAxisX() const
			{
				return RedVector4( Matrix[0] );
			}

			RedVector4 RedMatrix4x4::GetAxisY() const
			{
				return RedVector4( Matrix[1] );
			}

			RedVector4 RedMatrix4x4::GetAxisZ() const
			{
				return RedVector4( Matrix[2] );
			}

			RedMatrix4x4& RedMatrix4x4::SetZeros()
			{
				Matrix[0].SetZeros();
				Matrix[1].SetZeros();
				Matrix[2].SetZeros();
				Matrix[3].SetZeros();
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetIdentity()
			{
				SetZeros();
				Matrix[0].V[0] = 1.0f;
				Matrix[1].V[1] = 1.0f;
				Matrix[2].V[2] = 1.0f;
				Matrix[3].V[3] = 1.0f;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetRotX( float _ccwRadians )
			{
				float rotC = ::cosf( _ccwRadians );
				float rotS = ::sinf( _ccwRadians );

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

				Matrix[0].V[0] = 1.0f;
				Matrix[0].V[1] = 0.0f;
				Matrix[0].V[2] = 0.0f;
				Matrix[1].V[0] = 0.0f;
				Matrix[1].V[1] = rotC;
				Matrix[1].V[2] = rotS;
				Matrix[2].V[0] = 0.0f;
				Matrix[2].V[1] = -rotS;
				Matrix[2].V[2] = rotC;

				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetRotY( float _ccwRadians )
			{
				float rotC = ::cosf( _ccwRadians );
				float rotS = ::sinf( _ccwRadians );

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

				Matrix[0].V[0] = rotC;
				Matrix[0].V[1] = 0.0f;
				Matrix[0].V[2] = -rotS;
				Matrix[1].V[0] = 0.0f;
				Matrix[1].V[1] = 1.0f;
				Matrix[1].V[2] = 0.0f;
				Matrix[2].V[0] = rotS;
				Matrix[2].V[1] = 0.0f;
				Matrix[2].V[2] = rotC;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetRotZ( float _ccwRadians )
			{
				float rotC = ::cosf( _ccwRadians );
				float rotS = ::sinf( _ccwRadians );

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

				Matrix[0].V[0] = rotC;
				Matrix[0].V[1] = rotS;
				Matrix[0].V[2] = 0.0f;
				Matrix[1].V[0] = -rotS;
				Matrix[1].V[1] = rotC;
				Matrix[1].V[2] = 0.0f;
				Matrix[2].V[0] = 0.0f;
				Matrix[2].V[1] = 0.0f;
				Matrix[2].V[2] = 1.0f;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetScale( const RedVector1& _uniformScale )
			{
				Matrix[0] *= _uniformScale;
				Matrix[1] *= _uniformScale;
				Matrix[2] *= _uniformScale;
				Matrix[3] *= _uniformScale;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetScale( float _uniformScale )
			{
				Matrix[0] *= _uniformScale;
				Matrix[1] *= _uniformScale;
				Matrix[2] *= _uniformScale;
				Matrix[3] *= _uniformScale;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetScale( const RedVector3& _scale )
			{
				Matrix[0].V[0] *= _scale.V[0];
				Matrix[0].V[1] *= _scale.V[0];
				Matrix[0].V[2] *= _scale.V[0];
				Matrix[1].V[0] *= _scale.V[1];
				Matrix[1].V[1] *= _scale.V[1];
				Matrix[1].V[2] *= _scale.V[1];
				Matrix[2].V[0] *= _scale.V[2];
				Matrix[2].V[1] *= _scale.V[2];
				Matrix[2].V[2] *= _scale.V[2];
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetScale( const RedVector4& _scale )
			{
				Matrix[0].V[0] *= _scale.V[0];
				Matrix[0].V[1] *= _scale.V[0];
				Matrix[0].V[2] *= _scale.V[0];
				Matrix[0].V[3] *= _scale.V[0];
				Matrix[1].V[0] *= _scale.V[1];
				Matrix[1].V[1] *= _scale.V[1];
				Matrix[1].V[2] *= _scale.V[1];
				Matrix[1].V[3] *= _scale.V[1];
				Matrix[2].V[0] *= _scale.V[2];
				Matrix[2].V[1] *= _scale.V[2];
				Matrix[2].V[2] *= _scale.V[2];
				Matrix[2].V[3] *= _scale.V[2];
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetScale33( const RedVector4& _scale )
			{
				Matrix[0].V[0] *= _scale.V[0];
				Matrix[0].V[1] *= _scale.V[0];
				Matrix[0].V[2] *= _scale.V[0];
				Matrix[1].V[0] *= _scale.V[1];
				Matrix[1].V[1] *= _scale.V[1];
				Matrix[1].V[2] *= _scale.V[1];
				Matrix[2].V[0] *= _scale.V[2];
				Matrix[2].V[1] *= _scale.V[2];
				Matrix[2].V[2] *= _scale.V[2];
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetPreScale44( const RedVector4& _scale )
			{
				Matrix[0].V[0] *= _scale.V[0];
				Matrix[0].V[1] *= _scale.V[1];
				Matrix[0].V[2] *= _scale.V[2];
				Matrix[1].V[0] *= _scale.V[0];
				Matrix[1].V[1] *= _scale.V[1];
				Matrix[1].V[2] *= _scale.V[2];
				Matrix[2].V[0] *= _scale.V[0];
				Matrix[2].V[1] *= _scale.V[1];
				Matrix[2].V[2] *= _scale.V[2];
				Matrix[3].V[0] *= _scale.V[0];
				Matrix[3].V[1] *= _scale.V[1];
				Matrix[3].V[2] *= _scale.V[2];
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetPreScale33( const RedVector4& _scale )
			{
				Matrix[0].V[0] *= _scale.V[0];
				Matrix[0].V[1] *= _scale.V[1];
				Matrix[0].V[2] *= _scale.V[2];
				Matrix[1].V[0] *= _scale.V[0];
				Matrix[1].V[1] *= _scale.V[1];
				Matrix[1].V[2] *= _scale.V[2];
				Matrix[2].V[0] *= _scale.V[0];
				Matrix[2].V[1] *= _scale.V[1];
				Matrix[2].V[2] *= _scale.V[2];
				return *this;
			}

			RedVector4 RedMatrix4x4::GetScale() const
			{
				return RedVector4( Matrix[0].Length3(), Matrix[1].Length3(), Matrix[2].Length3() );
			}

			RedVector4 RedMatrix4x4::GetPreScale() const
			{
				RedVector4 v0( Matrix[0].V[0], Matrix[1].V[0], Matrix[2].V[0] );
				RedVector4 v1( Matrix[0].V[1], Matrix[1].V[1], Matrix[2].V[1] );
				RedVector4 v2( Matrix[0].V[2], Matrix[1].V[2], Matrix[2].V[2] );
				return RedVector4( v0.Length3(), v1.Length3(), v2.Length3() );
			}

			RedMatrix4x4& RedMatrix4x4::SetTranslation( const RedVector4& _v )
			{
				Matrix[3].V[0] = _v.V[0];
				Matrix[3].V[1] = _v.V[1];
				Matrix[3].V[2] = _v.V[2];
				Matrix[3].V[3] = 1.0f;
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::SetTranslation( float _x, float _y, float _z )
			{
				Matrix[3].V[0] = _x;
				Matrix[3].V[1] = _y;
				Matrix[3].V[2] = _z;
				Matrix[3].V[3] = 1.0f;
				return *this;
			}

			RedVector4 RedMatrix4x4::GetTranslation() const
			{
				return RedVector4( Matrix[3].AsVector3(), 1.0f );
			}

			const RedVector4& RedMatrix4x4::GetTranslationRef() const
			{
				return Matrix[3];
			}

			// Convert rotation to euler angles
			RedEulerAngles RedMatrix4x4::ToEulerAngles() const
			{
				RedEulerAngles ret;

				if ( Matrix[1].V[2] > 0.995f )
				{
					ret.Roll =  RAD2DEG( MATan2( Matrix[2].V[0], Matrix[0].V[0] ) );
					ret.Pitch =  90.0f;
					ret.Yaw =  0.0f;
				}
				else if ( Matrix[1].V[2] < -0.995f )
				{
					ret.Roll =  RAD2DEG( MATan2( Matrix[2].V[0], Matrix[0].V[0] ) );
					ret.Pitch = -90.0f;
					ret.Yaw =  0.0f;
				}
				else
				{
					ret.Roll = -RAD2DEG( MATan2( Matrix[0].V[2], Matrix[2].V[2] ) );
					ret.Pitch =  RAD2DEG( MAsin(  Matrix[1].V[2] ) );
					ret.Yaw	= -RAD2DEG( MATan2( Matrix[1].V[0],  Matrix[1].V[1] ) );
				}

				return ret;
			}

			float RedMatrix4x4::GetYaw() const
			{
				return -RAD2DEG( MATan2( Matrix[1].V[0], Matrix[1].V[1] ) ); // unscaling is not needed, both arguments has same scale
			}

			// Convert rotation to euler angles, works properly also for scaled matrices
			RedEulerAngles RedMatrix4x4::ToEulerAnglesFull() const
			{
				RedEulerAngles ret;

				float row1Mag = Matrix[1].Length3();
				float row2MagSqr = Matrix[2].SquareLength3();

				float unscaleR1     = 1.f / row1Mag;
				float rescaleR2toR0 = MSqrt( ( Matrix[0].SquareLength3() / row2MagSqr ) );

				float cell12 = Matrix[1].V[2] * unscaleR1;
				if ( cell12 > 0.995f )
				{
					ret.Roll = RAD2DEG( MATan2( Matrix[2].V[0] * rescaleR2toR0, Matrix[0].V[0] ) );
					ret.Pitch =  90.0f;
					ret.Yaw =  0.0f;
				}
				else if ( cell12 < -0.995f )
				{
					ret.Roll =  RAD2DEG( MATan2( Matrix[2].V[0] * rescaleR2toR0, Matrix[0].V[0] ) );
					ret.Pitch = -90.0f;
					ret.Yaw	=  0.0f;
				}
				else
				{
					ret.Roll = -RAD2DEG( MATan2( Matrix[0].V[2], Matrix[2].V[2] * rescaleR2toR0 ) );
					ret.Pitch =  RAD2DEG( asin( cell12 ) );
					ret.Yaw	= -RAD2DEG( MATan2( Matrix[1].V[0], Matrix[1].V[1] ) ); // unscaling is not needed, both arguments has same scale
				}

				return ret;
			}

			void RedMatrix4x4::ToAngleVectors( RedVector4* _forward, RedVector4* _right, RedVector4* _up ) const
			{
				RedVector4 f( 0, 1, 0 );
				RedVector4 r( 1, 0, 0 );
				RedVector4 u( 0, 0, 1 );

				if ( _forward )
				{
					_forward->Set( TransformVector( f ) );
				}

				if ( _right )
				{
					_right->Set( TransformVector( r ) );
				}

				if ( _up )
				{
					_up->Set( TransformVector( u ) );
				}
			}

			RedQuaternion RedMatrix4x4::ToQuaternion() const
			{
				float tr = Matrix[0].V[0] + Matrix[1].V[1] + Matrix[2].V[2] + 1.0f;
				float qw;
				float qx;
				float qy;
				float qz;

				if ( tr > 0.0f ) 
				{
					float S = MSqrt( tr ) * 2.0f; // S=4*qw 
					qw = 0.25f * S;
					qx = (Matrix[1].V[2] - Matrix[2].V[1]) / S;
					qy = (Matrix[2].V[0] - Matrix[0].V[2]) / S; 
					qz = (Matrix[0].V[1] - Matrix[1].V[0]) / S; 
				} 
				else if ( ( Matrix[0].V[0] > Matrix[1].V[1] ) && ( Matrix[0].V[0] > Matrix[2].V[2] ) ) 
				{ 
					float S = MSqrt( 1.0f + Matrix[0].V[0] - Matrix[1].V[1] - Matrix[2].V[2] ) * 2.0f; // S=4*qx 
					qw = (Matrix[1].V[2] - Matrix[2].V[1]) / S;
					qx = 0.25f * S;
					qy = (Matrix[1].V[0] + Matrix[0].V[1]) / S; 
					qz = (Matrix[2].V[0] + Matrix[0].V[2]) / S; 
				} 
				else if (Matrix[1].V[1] > Matrix[2].V[2]) 
				{ 
					float S = MSqrt( 1.0f + Matrix[1].V[1] - Matrix[0].V[0] - Matrix[2].V[2] ) * 2.f; // S=4*qy
					qw = ( Matrix[2].V[0] - Matrix[0].V[2] ) / S;
					qx = ( Matrix[1].V[0] + Matrix[0].V[1] ) / S; 
					qy = 0.25f * S;
					qz = ( Matrix[2].V[1] + Matrix[1].V[2] ) / S;
				} 
				else 
				{ 
					float S = MSqrt( 1.0f + Matrix[2].V[2] - Matrix[0].V[0] - Matrix[1].V[1] ) * 2.f; // S=4*qz
					qw = (Matrix[0].V[1] - Matrix[1].V[0]) / S;
					qx = (Matrix[2].V[0] + Matrix[0].V[2]) / S;
					qy = (Matrix[2].V[1] + Matrix[1].V[2]) / S;
					qz = 0.25f * S;
				}

				return RedQuaternion( qx, qy, qz, qw );
			}

			RedMatrix4x4& RedMatrix4x4::BuildPerspectiveLH( float _fovy, float _aspect, float _zn, float _zf )
			{
				const float ys = 1.0f / ::tanf( _fovy * 0.5f );
				const float xs = ys / _aspect;
				const float zs = _zf/(_zf-_zn);

				Matrix[0].Set( xs, 0.0f, 0.0f, 0.0f );
				Matrix[1].Set( 0.0f, ys, 0.0f, 0.0f );
				Matrix[2].Set( 0.0f, 0.0f, zs, 1.0f );
				Matrix[3].Set( 0.0f, 0.0f, ( ( -_zn ) * zs ), 0.0f );

				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::BuildOrthoLH( float _w, float _h, float _zn, float _zf )
			{
				Matrix[0].Set( ( 2.0f / _w ), 0.0f, 0.0f, 0.0f );
				Matrix[1].Set( 0.0f, ( 2.0f / _h ), 0.0f, 0.0f );
				Matrix[2].Set( 0.0f, 0.0f, ( 1.0f / ( _zf - _zn ) ), 0.0f );
				Matrix[3].Set( 0.0f, 0.0f, -_zn/(_zf-_zn), 1.0f );

				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::BuildFromDirectionVector( const RedVector4& _dirVec )
			{
				// EY from direction, EZ pointing up
				Matrix[1] = _dirVec.Normalized3();
				Matrix[2] = RedVector4::EZ;

				// EX as cross product of EY and EZ
				Matrix[0] = RedVector4::Cross( Matrix[1], Matrix[2] ).Normalized3();
				// EZ as cross product of EX and EY ( to orthogonalize );
				Matrix[2] = RedVector4::Cross( Matrix[0], Matrix[1] ).Normalized3();

				Matrix[0].W = 0.0f;	
				Matrix[1].W = 0.0f;
				Matrix[2].W = 0.0f;
				Matrix[3] = RedVector4::EW;

				return (*this);
			}

			RedMatrix4x4& RedMatrix4x4::BuildFromQuaternion( const RedVector4& _quaternion )
			{
				float x2 = _quaternion.X + _quaternion.X;
				float xx = _quaternion.X * x2;
				float wx = _quaternion.W * x2;

				float y2 = _quaternion.Y + _quaternion.Y;
				float xy = _quaternion.X * y2;
				float yy = _quaternion.Y * y2;
				float wy = _quaternion.W * y2;
				
				float z2 = _quaternion.Z + _quaternion.Z;
				float xz = _quaternion.X * z2;
				float yz = _quaternion.Y * z2;
				float zz = _quaternion.Z * z2;
				float wz = _quaternion.W * z2;


				Matrix[0].Set( 1.0f - ( yy + zz ), ( xy + wz ), ( xz - wy ), 0.0f );

				Matrix[1].Set( ( xy - wz ), 1.0f - ( xx + zz ), ( yz + wx ), 0.0f );

				Matrix[2].Set( ( xz + wy ), ( yz - wx ), 1.0f - ( xx + yy ), 0.0f );

				Matrix[3].Set( 0.0f, 0.0f, 0.0f, 1.0f );

				return *this;
			}

			float RedMatrix4x4::Det() const
			{
				float det = 0.0f;
				det += Matrix[0].V[0] * CoFactor( 0, 0 );
				det += Matrix[0].V[1] * CoFactor( 0, 1 ); 
				det += Matrix[0].V[2] * CoFactor( 0, 2 ); 
				det += Matrix[0].V[3] * CoFactor( 0, 3 ); 
				return det;
			}

			float RedMatrix4x4::CoFactor( int _i, int _j ) const
			{
				#define M( dx, dy ) Matrix[ ( _i + dx ) & 3 ].V[ (_j + dy ) & 3 ]
				float val = 0.0f;
				val += M( 1, 1 ) * M( 2, 2 ) * M( 3, 3 );
				val += M( 1, 2 ) * M( 2, 3 ) * M( 3, 1 );
				val += M( 1, 3 ) * M( 2, 1 ) * M( 3, 2 );
				val -= M( 3, 1 ) * M( 2, 2 ) * M( 1, 3 );
				val -= M( 3, 2 ) * M( 2, 3 ) * M( 1, 1 );
				val -= M( 3, 3 ) * M( 2, 1 ) * M( 1, 2 );
				val *= ( ( _i + _j ) & 1 ) ? -1.0f : 1.0f;
				return val; 
				#undef M
			}


			
			RedMatrix4x4& RedMatrix4x4::Invert()
			{
				*this = Inverted();
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::FullInvert()
			{
				*this = FullInverted();
				return *this;
			}

			RedMatrix4x4& RedMatrix4x4::Transpose()
			{
				*this = Transposed();
				return *this;
			}

			RedMatrix4x4 RedMatrix4x4::Inverted() const
			{
				RedMatrix4x4 out;

				// Transpose the 3x3 inner matrix
				out.Matrix[0].V[0] = Matrix[0].V[0];
				out.Matrix[0].V[1] = Matrix[1].V[0];
				out.Matrix[0].V[2] = Matrix[2].V[0];
				out.Matrix[1].V[0] = Matrix[0].V[1];
				out.Matrix[1].V[1] = Matrix[1].V[1];
				out.Matrix[1].V[2] = Matrix[2].V[1];
				out.Matrix[2].V[0] = Matrix[0].V[2];
				out.Matrix[2].V[1] = Matrix[1].V[2];
				out.Matrix[2].V[2] = Matrix[2].V[2];

				// Calculate inverted translation
				out.Matrix[3].V[0] = -( Matrix[3].V[0] * Matrix[0].V[0] + Matrix[3].V[1] * Matrix[0].V[1] + Matrix[3].V[2] * Matrix[0].V[2] );
				out.Matrix[3].V[1] = -( Matrix[3].V[0] * Matrix[1].V[0] + Matrix[3].V[1] * Matrix[1].V[1] + Matrix[3].V[2] * Matrix[1].V[2] );
				out.Matrix[3].V[2] = -( Matrix[3].V[0] * Matrix[2].V[0] + Matrix[3].V[1] * Matrix[2].V[1] + Matrix[3].V[2] * Matrix[2].V[2] );

				// Leave last row as in the identity matrix
				out.Matrix[0].V[3] = 0.0f;
				out.Matrix[1].V[3] = 0.0f;
				out.Matrix[2].V[3] = 0.0f;
				out.Matrix[3].V[3] = 1.0f;
				return out;
			}

			RedMatrix4x4 RedMatrix4x4::FullInverted() const
			{
				RedMatrix4x4 out;

				// Get determinant
				float d = Det();
				if ( MAbs(d) > 1e-12f )
				{
					float id = 1.0f / d;

					// Invert matrix
					out.Matrix[0].V[0] = CoFactor( 0, 0 ) * id;
					out.Matrix[0].V[1] = CoFactor( 1, 0 ) * id;
					out.Matrix[0].V[2] = CoFactor( 2, 0 ) * id;
					out.Matrix[0].V[3] = CoFactor( 3, 0 ) * id;
					out.Matrix[1].V[0] = CoFactor( 0, 1 ) * id;
					out.Matrix[1].V[1] = CoFactor( 1, 1 ) * id;
					out.Matrix[1].V[2] = CoFactor( 2, 1 ) * id;
					out.Matrix[1].V[3] = CoFactor( 3, 1 ) * id;
					out.Matrix[2].V[0] = CoFactor( 0, 2 ) * id;
					out.Matrix[2].V[1] = CoFactor( 1, 2 ) * id;
					out.Matrix[2].V[2] = CoFactor( 2, 2 ) * id;
					out.Matrix[2].V[3] = CoFactor( 3, 2 ) * id;
					out.Matrix[3].V[0] = CoFactor( 0, 3 ) * id;
					out.Matrix[3].V[1] = CoFactor( 1, 3 ) * id;
					out.Matrix[3].V[2] = CoFactor( 2, 3 ) * id;
					out.Matrix[3].V[3] = CoFactor( 3, 3 ) * id;
				}

				return out;
			}

			RedMatrix4x4 RedMatrix4x4::Transposed() const
			{
				RedMatrix4x4 out;
				out.Matrix[0].V[0] = Matrix[0].V[0];
				out.Matrix[0].V[1] = Matrix[1].V[0];
				out.Matrix[0].V[2] = Matrix[2].V[0];
				out.Matrix[0].V[3] = Matrix[3].V[0];
				out.Matrix[1].V[0] = Matrix[0].V[1];
				out.Matrix[1].V[1] = Matrix[1].V[1];
				out.Matrix[1].V[2] = Matrix[2].V[1];
				out.Matrix[1].V[3] = Matrix[3].V[1];
				out.Matrix[2].V[0] = Matrix[0].V[2];
				out.Matrix[2].V[1] = Matrix[1].V[2];
				out.Matrix[2].V[2] = Matrix[2].V[2];
				out.Matrix[2].V[3] = Matrix[3].V[2];
				out.Matrix[3].V[0] = Matrix[0].V[3];
				out.Matrix[3].V[1] = Matrix[1].V[3];
				out.Matrix[3].V[2] = Matrix[2].V[3];
				out.Matrix[3].V[3] = Matrix[3].V[3];
				return out;
			}

			void RedMatrix4x4::GetColumnMajor( float* _data ) const
			{
				//unrolled
				_data[0] = Matrix[0].V[0];
				_data[1] = Matrix[1].V[0];
				_data[2] = Matrix[2].V[0];
				_data[3] = Matrix[3].V[0];
				_data[4] = Matrix[0].V[1];
				_data[5] = Matrix[1].V[1];
				_data[6] = Matrix[2].V[1];
				_data[7] = Matrix[3].V[1];
				_data[8] = Matrix[0].V[2];
				_data[9] = Matrix[1].V[2];
				_data[10] = Matrix[2].V[2];
				_data[11] = Matrix[3].V[2];
				_data[12] = Matrix[0].V[3];
				_data[13] = Matrix[1].V[3];
				_data[14] = Matrix[2].V[3];
				_data[15] = Matrix[3].V[3];	
			}

			RedVector4 RedMatrix4x4::TransformVector( const RedVector4& _v ) const
			{
				RedVector4 out;
				out.V[0] = ( Matrix[0].V[0] * _v.X ) +
							( Matrix[1].V[0] * _v.Y ) +
							( Matrix[2].V[0] * _v.Z );

				out.V[1] = ( Matrix[0].V[1] * _v.X ) +
							( Matrix[1].V[1] * _v.Y ) + 
							( Matrix[2].V[1] * _v.Z );

				out.V[2] = ( Matrix[0].V[2] * _v.X ) +
							( Matrix[1].V[2] * _v.Y ) +
							( Matrix[2].V[2] * _v.Z );
	
				out.V[3] = 0.0f;
				return out;
			}

			RedVector4 RedMatrix4x4::TransformVectorWithW( const RedVector4& _v ) const
			{
				RedVector4 out;
				out.V[0] = ( Matrix[0].V[0] * _v.X ) +
							( Matrix[1].V[0] * _v.Y ) +
							( Matrix[2].V[0] * _v.Z ) +
							( Matrix[3].V[0] * _v.W );

				out.V[1] = ( Matrix[0].V[1] * _v.X ) +
							( Matrix[1].V[1] * _v.Y ) +
							( Matrix[2].V[1] * _v.Z ) + 
							( Matrix[3].V[1] * _v.W );

				out.V[2] = ( Matrix[0].V[2] * _v.X ) +
							( Matrix[1].V[2] * _v.Y ) +
							( Matrix[2].V[2] * _v.Z ) + 
							( Matrix[3].V[2] * _v.W );

				out.V[3] = ( Matrix[0].V[3] * _v.X ) +
							( Matrix[1].V[3] * _v.Y ) +
							( Matrix[2].V[3] * _v.Z ) + 
							( Matrix[3].V[3] * _v.W );

				return out;
			}

			RedVector4 RedMatrix4x4::TransformVectorAsPoint( const RedVector4& _v ) const
			{
				RedVector4 out;
				out.V[0] = ( Matrix[0].V[0] * _v.X ) +
							( Matrix[1].V[0] * _v.Y ) +
							( Matrix[2].V[0] * _v.Z ) +
							Matrix[3].V[0];

				out.V[1] = ( Matrix[0].V[1] * _v.X ) +
							( Matrix[1].V[1] * _v.Y ) +
							( Matrix[2].V[1] * _v.Z ) +
							Matrix[3].V[1];

				out.V[2] = ( Matrix[0].V[2] * _v.X ) +
							( Matrix[1].V[2] * _v.Y ) +
							( Matrix[2].V[2] * _v.Z ) +
							Matrix[3].V[2];

				out.V[3] = 1.0f;
				return out;
			}

			RedVector4 RedMatrix4x4::TransformPoint( const RedVector4& _v ) const
			{
				RedVector4 out;
				out.V[0] = ( Matrix[0].V[0] * _v.X ) +
							( Matrix[1].V[0] * _v.Y ) +
							( Matrix[2].V[0] * _v.Z ) +
							Matrix[3].V[0];

				out.V[1] = ( Matrix[0].V[1] * _v.X ) +
							( Matrix[1].V[1] * _v.Y ) + 
							( Matrix[2].V[1] * _v.Z ) + 
							Matrix[3].V[1];

				out.V[2] = ( Matrix[0].V[2] * _v.X ) + 
							( Matrix[1].V[2] * _v.Y ) + 
							( Matrix[2].V[2] * _v.Z ) + 
							Matrix[3].V[2];

				out.V[3] = 1.0f;
				return out;
			}

			void RedMatrix4x4::ExtractScale( RedMatrix4x4& _trMatrix, RedVector4& _scale ) const
			{
				// Just copy the matrix
				_trMatrix = *this;

				RedVector4 xAxis = _trMatrix.GetRow(0);
				RedVector4 yAxis = _trMatrix.GetRow(1);
				RedVector4 zAxis = _trMatrix.GetRow(2);

				// Extract scale and normalize axes
				_scale.V[0] = xAxis.Length();
				_scale.V[1] = yAxis.Length();
				_scale.V[2] = zAxis.Length();
	
				_trMatrix.SetRows( xAxis.Normalize(), yAxis.Normalize(), zAxis.Normalize(),  RedVector4::EW );
			}

			bool RedMatrix4x4::IsOk() const
			{
				return ( Matrix[0].IsOk() && Matrix[1].IsOk() && Matrix[2].IsOk() && Matrix[3].IsOk());
			}
		}
	}
}
