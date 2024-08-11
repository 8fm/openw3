
namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3::RedMatrix3x3()
		{
			SetIdentity(); // Make it consistent with the 4x4 version
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3::RedMatrix3x3( const Red::System::Float* _f )
		{
			Row0 = RedVector3( &_f[0] );
			Row1 = RedVector3( &_f[4] );
			Row2 = RedVector3( &_f[8] );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3::RedMatrix3x3( const RedMatrix3x3& _m )
		{
			Row0 = _m.Row0;
			Row1 = _m.Row1;
			Row2 = _m.Row2;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3::RedMatrix3x3( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 )
		{
			Row0 = _v0;
			Row1 = _v1;
			Row2 = _v2;
		}
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3::~RedMatrix3x3()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3& RedMatrix3x3::operator=( const RedMatrix3x3& _m )
		{
			Row0 = _m.Row0;
			Row1 = _m.Row1;
			Row2 = _m.Row2;
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const Red::System::Float* RedMatrix3x3::AsFloat() const
		{
			return Data;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::Set( const RedMatrix3x3& _m )
		{
			Row0 = _m.Row0;
			Row1 = _m.Row1;
			Row2 = _m.Row2;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::Set( const RedVector3& _x, const RedVector3& _y, const RedVector3& _z )
		{
			Row0 = _x;
			Row1 = _y;
			Row2 = _z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRows( const Red::System::Float* _f )
		{
			Row0 = RedVector3( &_f[0] );
			Row1 = RedVector3( &_f[4] );
			Row2 = RedVector3( &_f[8] );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetCols( const Red::System::Float* _f )
		{
			Row0.X = _f[0];
			Row0.Y = _f[4];
			Row0.Z = _f[8];
			Row1.X = _f[1];
			Row1.Y = _f[5];
			Row1.Z = _f[9];
			Row2.X = _f[2];
			Row2.Y = _f[6];
			Row2.Z = _f[10];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRows( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
		{
			Row0 = _r0;
			Row1 = _r1;
			Row2 = _r2;
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetCols( const RedVector3& _c0, const RedVector3& _c1, const RedVector3& _c2 )
		{
			Row0.X = _c0.X;
			Row1.X = _c0.Y;
			Row2.X = _c0.Z;
			Row0.Y = _c1.X;
			Row1.Y = _c1.Y;
			Row2.Y = _c1.Z;	
			Row0.Z = _c2.X;
			Row1.Z = _c2.Y;
			Row2.Z = _c2.Z;	
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRotX( Red::System::Float _ccwRadians )
		{
			Red::System::Float rotC = Red::Math::MCos( _ccwRadians );
			Red::System::Float rotS = Red::Math::MSin( _ccwRadians );
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

			Row0.X = 1.0f;
			Row0.Y = 0.0f;
			Row0.Z = 0.0f;
			Row1.X = 0.0f;
			Row1.Y = rotC;
			Row1.Z = rotS;
			Row2.X = 0.0f;
			Row2.Y = -rotS;
			Row2.Z = rotC;
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRotY( Red::System::Float _ccwRadians )
		{
			Red::System::Float rotC = Red::Math::MCos( _ccwRadians );
			Red::System::Float rotS = Red::Math::MSin( _ccwRadians );

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

			Row0.X = rotC;
			Row0.Y = 0.0f;
			Row0.Z = -rotS;
			Row1.X = 0.0f;
			Row1.Y = 1.0f;
			Row1.Z = 0.0f;
			Row2.X = rotS;
			Row2.Y = 0.0f;
			Row2.Z = rotC;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRotZ( Red::System::Float _ccwRadians )
		{
			Red::System::Float rotC = Red::Math::MCos( _ccwRadians );
			Red::System::Float rotS = Red::Math::MSin( _ccwRadians );

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

			Row0.X = rotC;
			Row0.Y = rotS;
			Row0.Z = 0.0f;
			Row1.X = -rotS;
			Row1.Y = rotC;
			Row1.Z = 0.0f;
			Row2.X = 0.0f;
			Row2.Y = 0.0f;
			Row2.Z = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetScale( const RedVector3& _scale )
		{
			RedScalar rowA( _scale.X );
			RedScalar rowB( _scale.Y );
			RedScalar rowC( _scale.Z );

			Row0.Set( Row0.X * rowA.X, Row0.Y * rowA.X, Row0.Z * rowA.X );
			Row1.Set( Row1.X * rowB.X, Row1.Y * rowB.X, Row1.Z * rowB.X );
			Row2.Set( Row2.X * rowC.X, Row2.Y * rowC.X, Row2.Z * rowC.X );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetPreScale( const RedVector3& _scale )
		{
			Row0.Set( Row0.X * _scale.X, Row0.Y * _scale.Y, Row0.Z * _scale.Z );
			Row1.Set( Row1.X * _scale.X, Row1.Y * _scale.Y, Row1.Z * _scale.Z );
			Row2.Set( Row2.X * _scale.X, Row2.Y * _scale.Y, Row2.Z * _scale.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetScale( const RedScalar& _uniformScale )
		{
			Row0.Set( Row0.X * _uniformScale.X, Row0.Y * _uniformScale.X, Row0.Z * _uniformScale.X );
			Row1.Set( Row1.X * _uniformScale.X, Row1.Y * _uniformScale.X, Row1.Z * _uniformScale.X );
			Row2.Set( Row2.X * _uniformScale.X, Row2.Y * _uniformScale.X, Row2.Z * _uniformScale.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetScale( Red::System::Float _uniformScale )
		{
			Row0.Set( Row0.X * _uniformScale, Row0.Y * _uniformScale, Row0.Z * _uniformScale );
			Row1.Set( Row1.X * _uniformScale, Row1.Y * _uniformScale, Row1.Z * _uniformScale );
			Row2.Set( Row2.X * _uniformScale, Row2.Y * _uniformScale, Row2.Z * _uniformScale );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetScale() const
		{
			return RedVector3( Row0.Length().X, Row1.Length().X, Row2.Length().X );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetPreScale() const
		{
			RedVector3 v0( Row0.X, Row1.X, Row2.X );
			RedVector3 v1( Row0.Y, Row1.Y, Row2.Y );
			RedVector3 v2( Row0.Z, Row1.Z, Row2.Z );
			return RedVector3( v0.Length().X, v1.Length().X, v2.Length().X );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetColumn( Red::System::Uint32 _index, const RedVector3& _v )
		{
			Row0.V[_index] = _v.X;
			Row1.V[_index] = _v.Y;
			Row2.V[_index] = _v.Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRow( Red::System::Uint32 _index, const RedVector3& _v )
		{
			Rows[_index] = _v;
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetColumn( Red::System::Uint32 _index ) const
		{
			return RedVector3( Row0.V[_index], Row1.V[_index], Row2.V[_index] );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector3& RedMatrix3x3::GetRow( Red::System::Uint32 _index ) const
		{
			return Rows[_index];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetAxisX() const
		{
			return Row0;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetAxisY() const
		{
			return Row1;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetAxisZ() const
		{
			return Row2;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::BuildFromDirectionVector( const RedVector3& _dirVec )
		{
			// EY from direction, EZ pointing up
			Row1 = _dirVec.Normalized();
			Row2 = RedVector3::EZ;

			// EX as cross product of EY and EZ
			Row0.Set( ( Row1.Y * Row2.Z ) + ( Row1.Z * Row2.Y ), ( Row1.Z * Row2.X ) + ( Row1.X * Row2.Z ), ( Row1.X * Row2.Y ) + ( Row1.Y * Row2.X ) );
			Row0.Normalize();
			// EZ as cross product of EX and EY ( to orthogonalize );
			Row2.Set( ( Row0.Y * Row1.Z ) + ( Row0.Z * Row1.Y ), ( Row0.Z * Row1.X ) + ( Row0.X * Row1.Z ), ( Row0.X * Row1.Y ) + ( Row0.Y * Row1.X ) );
			Row2.Normalize();
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::BuildFromQuaternion( const RedVector4& _quaternion )
		{			
			RedVector4 quatX_XYZW( _quaternion.X * _quaternion.X, _quaternion.X * _quaternion.Y, _quaternion.X * _quaternion.Z, _quaternion.X * _quaternion.W );
			RedVector3 quatY_YZW( _quaternion.Y * _quaternion.Y, _quaternion.Y * _quaternion.Z, _quaternion.Y * _quaternion.W );
			RedVector2 quatZ_ZW( _quaternion.Z * _quaternion.Z, _quaternion.Z * _quaternion.W );
			
			Row0.Set( 1.0f - 2.0f * ( quatY_YZW.X + quatZ_ZW.X ), 2.0f * ( quatX_XYZW.Y - quatZ_ZW.Y ), 2.0f * ( quatX_XYZW.Z + quatY_YZW.Z ) );
			Row1.Set( 2.0f * ( quatX_XYZW.Y + quatZ_ZW.Y ), 1.0f - 2.0f * ( quatX_XYZW.X + quatZ_ZW.X ), 2.0f * ( quatY_YZW.Y - quatX_XYZW.V[3] ) );
			Row2.Set( 2.0f * ( quatX_XYZW.Z - quatY_YZW.Z ), 2.0f * ( quatY_YZW.Y + quatX_XYZW.V[3] ), 1.0f - 2.0f * ( quatX_XYZW.X + quatY_YZW.X ) );
		}

		//RED_INLINE void BuildFromQuaternion( const RedQuaternion& _quaternion );
		//RED_INLINE void BuildFromEularAngle( const RedEularAngles& _eular );

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar RedMatrix3x3::Det() const
		{
			Red::System::Float det = 0.0f; 
			det += Row0.X * Row1.Y * Row2.Z; // 123
			det += Row0.Y * Row1.Z * Row2.X; // 231
			det += Row0.Z * Row1.X * Row2.Y; // 312
			det -= Row0.Z * Row1.Y * Row2.X; // 321
			det -= Row0.Y * Row1.X * Row2.Z; // 213
			det -= Row0.X * Row1.Z * Row2.Y; // 132 
			return det;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::Invert()
		{
			RedScalar det = Det();
			RedMatrix3x3 n;
			Red::System::Float m11 = ( Row1.Y * Row2.Z ) - ( Row1.Z * Row2.Y );
			Red::System::Float m12 = ( Row1.X * Row2.Z ) - ( Row1.Z * Row2.X );
			Red::System::Float m13 = ( Row1.X * Row2.Y ) - ( Row1.Y * Row2.X );

			Red::System::Float m21 = ( Row0.Y * Row2.Z ) - ( Row0.Z * Row2.Y );
			Red::System::Float m22 = ( Row0.X * Row2.Z ) - ( Row0.Z * Row2.X );
			Red::System::Float m23 = ( Row0.X * Row2.Y ) - ( Row0.Y * Row2.X );

			Red::System::Float m31 = ( Row0.Y * Row1.Z ) - ( Row0.Z * Row1.Y );
			Red::System::Float m32 = ( Row0.X * Row1.Z ) - ( Row0.Z * Row1.X );
			Red::System::Float m33 = ( Row0.X * Row1.Y ) - ( Row0.Y * Row1.X );

			n.Row0.Set( m11, ( -1.0f * m12 ) , m13 );
			n.Row1.Set( ( -1.0f * m21 ), m22, ( -1.0f * m23 ) );
			n.Row2.Set( m31, ( -1.0f * m32 ), m33 );

			Red::System::Float invDet = ( 1.0f / det.X );
			Row0.X = n.Row0.X * invDet;
			Row0.Y = n.Row1.X * invDet;
			Row0.Z = n.Row2.X * invDet;

			Row1.X = n.Row0.Y * invDet;
			Row1.Y = n.Row1.Y * invDet;
			Row1.Z = n.Row2.Y * invDet;

			Row2.X = n.Row0.Z * invDet;
			Row2.Y = n.Row1.Z * invDet;
			Row2.Z = n.Row2.Z * invDet;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3 RedMatrix3x3::Inverted() const
		{
			RedScalar det = Det();
			RedMatrix3x3 n;
			// 2x2 Determinants.
			Red::System::Float m11 = ( Row1.Y * Row2.Z ) - ( Row1.Z * Row2.Y );
			Red::System::Float m12 = ( Row1.X * Row2.Z ) - ( Row1.Z * Row2.X );
			Red::System::Float m13 = ( Row1.X * Row2.Y ) - ( Row1.Y * Row2.X );

			Red::System::Float m21 = ( Row0.Y * Row2.Z ) - ( Row0.Z * Row2.Y );
			Red::System::Float m22 = ( Row0.X * Row2.Z ) - ( Row0.Z * Row2.X );
			Red::System::Float m23 = ( Row0.X * Row2.Y ) - ( Row0.Y * Row2.X );

			Red::System::Float m31 = ( Row0.Y * Row1.Z ) - ( Row0.Z * Row1.Y );
			Red::System::Float m32 = ( Row0.X * Row1.Z ) - ( Row0.Z * Row1.X );
			Red::System::Float m33 = ( Row0.X * Row1.Y ) - ( Row0.Y * Row1.X );

			n.Row0.Set( m11, ( -1.0f * m12 ) , m13 );
			n.Row1.Set( ( -1.0f * m21 ), m22, ( -1.0f * m23 ) );
			n.Row2.Set( m31, ( -1.0f * m32 ), m33 );

			Red::System::Float invDet = ( 1.0f / det.X );
			return RedMatrix3x3( 
					RedVector3( n.Row0.X * invDet, n.Row1.X * invDet, n.Row2.X * invDet ),
					RedVector3( n.Row0.Y * invDet, n.Row1.Y * invDet, n.Row2.Y * invDet ),
					RedVector3( n.Row0.Z * invDet, n.Row1.Z * invDet, n.Row2.Z * invDet ) 
					);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::Transpose()
		{
			RedVector3 colA( Row0.X, Row1.X, Row2.X );
			RedVector3 colB( Row0.Y, Row1.Y, Row2.Y );
			RedVector3 colC( Row0.Z, Row1.Z, Row2.Z );

			Row0 = colA;
			Row1 = colB;
			Row2 = colC;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3 RedMatrix3x3::Transposed() const
		{
			RedVector3 colA( Row0.X, Row1.X, Row2.X );
			RedVector3 colB( Row0.Y, Row1.Y, Row2.Y );
			RedVector3 colC( Row0.Z, Row1.Z, Row2.Z );

			return RedMatrix3x3( colA, colB, colC );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::ExtractScale( RedMatrix3x3& _trMatrix, RedVector3& _scale ) const
		{
			RedVector3 xAxis = GetRow( 0 );
			RedVector3 yAxis = GetRow( 1 );
			RedVector3 zAxis = GetRow( 2 );

			_scale.Set( xAxis.Length().X, yAxis.Length().X, zAxis.Length().X );
			_trMatrix.SetRows( xAxis.Normalize(), yAxis.Normalize(), zAxis.Normalize() );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Bool RedMatrix3x3::IsAlmostEqual( const RedMatrix3x3& _m, Red::System::Float _epsilon ) const
		{
			return Row0.IsAlmostEqual( _m.Row0, _epsilon ) &&
			Row1.IsAlmostEqual( _m.Row1, _epsilon ) &&
			Row2.IsAlmostEqual( _m.Row2, _epsilon );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Bool RedMatrix3x3::IsZero() const
		{
			return Row0.IsZero() && Row1.IsZero() && Row2.IsZero();
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetIdentity()
		{
			Row0.Set(RedVector3::EX);
			Row1.Set(RedVector3::EY);
			Row2.Set(RedVector3::EZ);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Bool RedMatrix3x3::IsIdentity() const
		{
			return Row0.IsAlmostEqual( RedMatrix3x3::IDENTITY.Row0 ) &&
				   Row1.IsAlmostEqual( RedMatrix3x3::IDENTITY.Row1 ) &&
				   Row2.IsAlmostEqual( RedMatrix3x3::IDENTITY.Row2 );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetZeros()
		{
			Row0.SetZeros();
			Row1.SetZeros();
			Row2.SetZeros();
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Bool RedMatrix3x3::IsOk() const
		{
			return Row0.IsOk() && Row1.IsOk() && Row2.IsOk();
		}
	}
}