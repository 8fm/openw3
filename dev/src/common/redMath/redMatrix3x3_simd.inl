
namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		RedMatrix3x3::RedMatrix3x3()
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);
			SetIdentity(); // Make it consistent with the 4x4 version
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix3x3::RedMatrix3x3( const Red::System::Float* _f )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);

			SetRows( _f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix3x3::RedMatrix3x3( const RedMatrix3x3& _m )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Set( _m );
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix3x3::RedMatrix3x3( const RedVector3& _v0, const RedVector3& _v1, const RedVector3& _v2 )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Set( _v0, _v1, _v2 );
		}
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix3x3::~RedMatrix3x3()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix3x3& RedMatrix3x3::operator=( const RedMatrix3x3& _m )
		{
			Set( _m );

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
			RED_MATH_MEM_ALIGNMENT_CHECK(&_m, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Set( _m.Row0, _m.Row1,_m.Row2 );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::Set( const RedVector3& _x, const RedVector3& _y, const RedVector3& _z )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(&_x, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_y, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_z, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Row0 = _x;
			Row1 = _y;
			Row2 = _z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRows( const Red::System::Float* _f )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(_f, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Row0.Set( &_f[0] );
			Row1.Set( &_f[4] );
			Row2.Set( &_f[8] );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetCols( const Red::System::Float* _f )
		{
			Row0.Set( _f[0], _f[4], _f[8] );
			Row1.Set( _f[1], _f[5], _f[9] );
			Row2.Set( _f[2], _f[6], _f[10] );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRows( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
		{
			Set( _r0, _r1, _r2 );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetCols( const RedVector3& _c0, const RedVector3& _c1, const RedVector3& _c2 )
		{
			Row0.Set( _c0.X, _c1.X, _c2.X );
			Row1.Set( _c0.Y, _c1.Y, _c2.Y );
			Row2.Set( _c0.Z, _c1.Z, _c2.Z );
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

			Row0.Set( 1.0f, 0.0f, 0.0f );
			Row1.Set( 0.0f, rotC, rotS );
			Row2.Set( 0.0f, -rotS, rotC );
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

			Row0.Set( rotC, 0.0f, -rotS );
			Row1.Set( 0.0f, 1.0f, 0.0f );
			Row2.Set( rotS, 0.0f, rotC );
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

			Row0.Set( rotC, rotS, 0.0f );
			Row1.Set( -rotS, rotC, 0.0f );
			Row2.Set( 0.0f, 0.0f, 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetScale( const RedVector3& _scale )
		{
			SetMul( Row0, RedScalar( _scale.X ) );
			SetMul( Row1, RedScalar( _scale.Y ) );
			SetMul( Row2, RedScalar( _scale.Z ) );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetPreScale( const RedVector3& _scale )
		{
			SetMul( Row0, _scale );
			SetMul( Row1, _scale );
			SetMul( Row2, _scale );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetScale( const RedScalar& _uniformScale )
		{
			SetMul( Row0, _uniformScale );
			SetMul( Row1, _uniformScale );
			SetMul( Row2, _uniformScale );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetScale( Red::System::Float _uniformScale )
		{
			SetScale( RedScalar(_uniformScale) );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetScale() const
		{
			return RedVector3( Row0.Length(), Row1.Length(), Row2.Length() );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetPreScale() const
		{
			RedVector3 v0( Row0.X, Row1.X, Row2.X );
			RedVector3 v1( Row0.Y, Row1.Y, Row2.Y );
			RedVector3 v2( Row0.Z, Row1.Z, Row2.Z );
			return RedVector3( v0.Length(), v1.Length(), v2.Length() );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetColumn( Red::System::Uint32 _index, const RedVector3& _v )
		{
			Row0.f[_index] = _v.X;
			Row1.f[_index] = _v.Y;
			Row2.f[_index] = _v.Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::SetRow( Red::System::Uint32 _index, const RedVector3& _v )
		{
			Rows[_index] = _v;
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector3 RedMatrix3x3::GetColumn( Red::System::Uint32 _index ) const
		{
			return RedVector3( Row0.f[_index], Row1.f[_index], Row2.f[_index] );
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
			Row1.Set( 2.0f * ( quatX_XYZW.Y + quatZ_ZW.Y ), 1.0f - 2.0f * ( quatX_XYZW.X + quatZ_ZW.X ), 2.0f * ( quatY_YZW.Y - quatX_XYZW.W ) );
			Row2.Set( 2.0f * ( quatX_XYZW.Z - quatY_YZW.Z ), 2.0f * ( quatY_YZW.Y + quatX_XYZW.W ), 1.0f - 2.0f * ( quatX_XYZW.X + quatY_YZW.X ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar RedMatrix3x3::Det() const
		{
			RedVector3 cross = Cross(Row0, Row1);
			return Dot(cross, Row2);
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

			RedScalar invDet( 1.0f / det.X );
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
			RedMatrix3x3 n = *this;
			n.Invert();

			return n;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix3x3::Transpose()
		{
			Set(RedVector3( Row0.X, Row1.X, Row2.X ),
				RedVector3( Row0.Y, Row1.Y, Row2.Y ),
				RedVector3( Row0.Z, Row1.Z, Row2.Z ) );
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