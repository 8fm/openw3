
namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4()
			: Row0( RedVector4( 1.0f, 0.0f, 0.0f, 0.0f ) )
			, Row1( RedVector4( 0.0f, 1.0f, 0.0f, 0.0f ) )
			, Row2( RedVector4( 0.0f, 0.0f, 1.0f, 0.0f ) )
			, Row3( RedVector4( 0.0f, 0.0f, 0.0f, 1.0f ) )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const RedMatrix4x4& _m )
			: Row0( _m.Row0 )
			, Row1( _m.Row1 )
			, Row2( _m.Row2 )
			, Row3( _m.Row3 )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
			: Row0( _r0 )
			, Row1( _r1 )
			, Row2( _r2 )
			, Row3( _r3 )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
			: Row0( _r0, 0.0f )
			, Row1( _r1, 0.0f )
			, Row2( _r2, 0.0f )
			, Row3( RedVector4::EW )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const Red::System::Float* _f )
			: m00( _f[ 0] ), m01( _f[ 1] ), m02( _f[ 2] ), m03( _f[ 3] )
			, m10( _f[ 4] ), m11( _f[ 5] ), m12( _f[ 6] ), m13( _f[ 7] )
			, m20( _f[ 8] ), m21( _f[ 9] ), m22( _f[10] ), m23( _f[11] )
			, m30( _f[12] ), m31( _f[13] ), m32( _f[14] ), m33( _f[15] )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::~RedMatrix4x4()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4& RedMatrix4x4::operator=( const RedMatrix4x4& _m )
		{
			Row0 = _m.Row0;
			Row1 = _m.Row1;
			Row2 = _m.Row2;
			Row3 = _m.Row3;
			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const Red::System::Float* RedMatrix4x4::AsFloat() const
		{
			return &m00;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Set( const RedMatrix4x4& _m )
		{
			Row0 = _m.Row0;
			Row1 = _m.Row1;
			Row2 = _m.Row2;
			Row3 = _m.Row3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Set( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
		{
			Row0 = _r0;
			Row1 = _r1;
			Row2 = _r2;
			Row3 = _r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Set( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
		{
			Row0 = RedVector4( _r0, 0.0f );
			Row1 = RedVector4( _r1, 0.0f );
			Row2 = RedVector4( _r2, 0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Set33( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2 )
		{
			Row0 = RedVector4( _r0.X, _r0.Y, _r0.Z, 0.0f );
			Row1 = RedVector4( _r1.X, _r1.Y, _r1.Z, 0.0f );
			Row2 = RedVector4( _r2.X, _r2.Y, _r2.Z, 0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetZeros()
		{
			Row0 = RedVector4( 0.0f, 0.0f, 0.0f, 0.0f );
			Row1 = RedVector4( 0.0f, 0.0f, 0.0f, 0.0f );
			Row2 = RedVector4( 0.0f, 0.0f, 0.0f, 0.0f );
			Row3 = RedVector4( 0.0f, 0.0f, 0.0f, 0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetIdentity()
		{
			Row0 = RedVector4( 1.0f, 0.0f, 0.0f, 0.0f );
			Row1 = RedVector4( 0.0f, 1.0f, 0.0f, 0.0f );
			Row2 = RedVector4( 0.0f, 0.0f, 1.0f, 0.0f );
			Row3 = RedVector4( 0.0f, 0.0f, 0.0f, 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRows( const Red::System::Float* _f )
		{
			m00 = _f[0];
			m01 = _f[1];
			m02 = _f[2];
			m03 = _f[3];
			m10 = _f[4];
			m11 = _f[5];
			m12 = _f[6];
			m13 = _f[7];
			m20 = _f[8];
			m21 = _f[9];
			m22 = _f[10];
			m23 = _f[11];
			m30 = _f[12];
			m31 = _f[13];
			m32 = _f[14];
			m33 = _f[15];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetCols( const Red::System::Float* _f )
		{
			m00 = _f[0];
			m10 = _f[1];
			m20 = _f[2];
			m30 = _f[3];
			m01 = _f[4];
			m11 = _f[5];
			m21 = _f[6];
			m31 = _f[7];
			m02 = _f[8];
			m12 = _f[9];
			m22 = _f[10];
			m32 = _f[11];
			m03 = _f[12];
			m13 = _f[13];
			m23 = _f[14];
			m33 = _f[15];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRows( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
		{
			Row0 = _r0;
			Row1 = _r1;
			Row2 = _r2;
			Row3 = _r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetCols( const RedVector4& _c0, const RedVector4& _c1, const RedVector4& _c2, const RedVector4& _c3 )
		{
			Row0 = RedVector4( _c0.V[0], _c1.V[0], _c2.V[0], _c3.V[0] );
			Row1 = RedVector4( _c0.V[1], _c1.V[1], _c2.V[1], _c3.V[1] );
			Row2 = RedVector4( _c0.V[2], _c1.V[2], _c2.V[2], _c3.V[2] );
			Row3 = RedVector4( _c0.V[3], _c1.V[3], _c2.V[3], _c3.V[3] );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetColumn( Red::System::Uint32 _index, const RedVector4& _a )
		{
			Data[ _index ] = _a.V[0];
			Data[ _index + 4 ] = _a.V[1];
			Data[ _index + 8 ] = _a.V[2];
			Data[ _index + 12 ] = _a.V[3];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRow( Red::System::Uint32 _index, const RedVector4& _a )
		{
			Data[ ( _index * 4 ) ] = _a.V[0];
			Data[ ( _index * 4 ) + 1 ] = _a.V[1];
			Data[ ( _index * 4 ) + 2 ] = _a.V[2];
			Data[ ( _index * 4 ) + 3 ] = _a.V[3];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector4 RedMatrix4x4::GetColumn( Red::System::Uint32 _index ) const
		{
			return RedVector4( Row0.V[_index], Row1.V[_index], Row2.V[_index], Row3.V[_index] );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetRow( Red::System::Uint32 _index ) const
		{
			return (RedVector4&)Data[ _index * 4 ];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetAxisX() const
		{
			return (RedVector4&)Data[ 0 ];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetAxisY() const
		{
			return (RedVector4&)Data[ 4 ];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetAxisZ() const
		{
			return (RedVector4&)Data[ 8 ];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRotX( Red::System::Float _ccwRadians )
		{
			// Find the rotation round the X axis. Using standard circle calculation.
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

			m00 = 1.0f;
			m01 = 0.0f;
			m02 = 0.0f;
			m10 = 0.0f;
			m11 = rotC;
			m12 = rotS;
			m20 = 0.0f;
			m21 = -rotS;
			m22 = rotC;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRotY( Red::System::Float _ccwRadians )
		{
			// Find the rotation round the Y axis. Using standard circle calculation.
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
			m00 = rotC;
			m01 = 0.0f;
			m02 = -rotS;
			m10 = 0.0f;
			m11 = 1.0f;
			m12 = 0.0f;
			m20 = rotS;
			m21 = 0.0f;
			m22 = rotC;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRotZ( Red::System::Float _ccwRadians )
		{
			// Find the rotation round the Z axis. Using standard circle calculation.
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

			m00 = rotC;
			m01 = rotS;
			m02 = 0.0f;
			m10 = -rotS;
			m11 = rotC;
			m12 = 0.0f;
			m20 = 0.0f;
			m21 = 0.0f;
			m22 = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale( const RedScalar& _uniformScale )
		{
			m00 *= _uniformScale.X;
			m01 *= _uniformScale.X;
			m02 *= _uniformScale.X;
			m03 *= _uniformScale.X;
			m10 *= _uniformScale.X;
			m11 *= _uniformScale.X;
			m12 *= _uniformScale.X;
			m13 *= _uniformScale.X;
			m20 *= _uniformScale.X;
			m21 *= _uniformScale.X;
			m22 *= _uniformScale.X;
			m23 *= _uniformScale.X;
			m30 *= _uniformScale.X;
			m31 *= _uniformScale.X;
			m32 *= _uniformScale.X;
			m33 *= _uniformScale.X;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale( Red::System::Float _uniformScale )
		{
			m00 *= _uniformScale;
			m01 *= _uniformScale;
			m02 *= _uniformScale;
			m03 *= _uniformScale;
			m10 *= _uniformScale;
			m11 *= _uniformScale;
			m12 *= _uniformScale;
			m13 *= _uniformScale;
			m20 *= _uniformScale;
			m21 *= _uniformScale;
			m22 *= _uniformScale;
			m23 *= _uniformScale;
			m30 *= _uniformScale;
			m31 *= _uniformScale;
			m32 *= _uniformScale;
			m33 *= _uniformScale;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale( const RedVector3& _scale )
		{
			m00 *= _scale.V[0];
			m01 *= _scale.V[0];
			m02 *= _scale.V[0];
			m10 *= _scale.V[1];
			m11 *= _scale.V[1];
			m12 *= _scale.V[1];
			m20 *= _scale.V[2];
			m21 *= _scale.V[2];
			m22 *= _scale.V[2];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale( const RedVector4& _scale )
		{
			m00 *= _scale.V[0];
			m01 *= _scale.V[0];
			m02 *= _scale.V[0];
			m03 *= _scale.V[0];
			m10 *= _scale.V[1];
			m11 *= _scale.V[1];
			m12 *= _scale.V[1];
			m13 *= _scale.V[1];
			m20 *= _scale.V[2];
			m21 *= _scale.V[2];
			m22 *= _scale.V[2];
			m23 *= _scale.V[2];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale33( const RedVector4& _scale )
		{
			m00 *= _scale.V[0];
			m01 *= _scale.V[0];
			m02 *= _scale.V[0];
			m10 *= _scale.V[1];
			m11 *= _scale.V[1];
			m12 *= _scale.V[1];
			m20 *= _scale.V[2];
			m21 *= _scale.V[2];
			m22 *= _scale.V[2];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetPreScale44( const RedVector4& _scale )
		{
			m00 *= _scale.V[0];
			m01 *= _scale.V[1];
			m02 *= _scale.V[2];
			m10 *= _scale.V[0];
			m11 *= _scale.V[1];
			m12 *= _scale.V[2];
			m20 *= _scale.V[0];
			m21 *= _scale.V[1];
			m22 *= _scale.V[2];
			m30 *= _scale.V[0];
			m31 *= _scale.V[1];
			m32 *= _scale.V[2];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetPreScale33( const RedVector4& _scale )
		{
			m00 *= _scale.V[0];
			m01 *= _scale.V[1];
			m02 *= _scale.V[2];
			m10 *= _scale.V[0];
			m11 *= _scale.V[1];
			m12 *= _scale.V[2];
			m20 *= _scale.V[0];
			m21 *= _scale.V[1];
			m22 *= _scale.V[2];
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector4 RedMatrix4x4::GetScale() const
		{
			return RedVector4( Row0.Length3().X, Row1.Length3().X, Row2.Length3().X, 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector4 RedMatrix4x4::GetPreScale() const
		{
			RedVector4 v0( m00, m10, m20 );
			RedVector4 v1( m01, m11, m21 );
			RedVector4 v2( m02, m12, m22 );
			return RedVector4( v0.Length3().X, v1.Length3().X, v2.Length3().X, 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetTranslation( const RedVector4& _v )
		{
			Row3 = _v;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetTranslation( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w )
		{
			Row3 = RedVector4( _x, _y, _z, _w );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector4 RedMatrix4x4::GetTranslation() const
		{
			return RedVector4( Row3.AsVector3(), 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetTranslationRef() const
		{
			return Row3;		
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar RedMatrix4x4::Det() const
		{
			Red::System::Float det = 0.0f;
			det += m00 * CoFactor( 0, 0 ).X;
			det += m01 * CoFactor( 0, 1 ).X; 
			det += m02 * CoFactor( 0, 2 ).X; 
			det += m03 * CoFactor( 0, 3 ).X; 
			return det;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedScalar RedMatrix4x4::CoFactor( Red::System::Uint32 _i, Red::System::Uint32 _j ) const
		{
			//Matrix[ ( _i + dx ) & 3 ].V[ (_j + dy ) & 3 ]
			#define M( dx, dy ) Data[ ( ( ( _i + dx ) & 3 ) * 4 ) + ( ( _j + dy ) & 3 ) ] 
			Red::System::Float val = 0.0f;
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

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Invert()
		{
			RedMatrix4x4 n;

			// Get determinant
			RedScalar d = Det();
			if ( !d.Abs().IsAlmostZero() )
			{
				RedScalar id = 1.0f / d.X;

				// Invert matrix
				n.m00 = CoFactor( 0, 0 ).X * id.X;
				n.m01 = CoFactor( 1, 0 ).X * id.X;
				n.m02 = CoFactor( 2, 0 ).X * id.X;
				n.m03 = CoFactor( 3, 0 ).X * id.X;
				n.m10 = CoFactor( 0, 1 ).X * id.X;
				n.m11 = CoFactor( 1, 1 ).X * id.X;
				n.m12 = CoFactor( 2, 1 ).X * id.X;
				n.m13 = CoFactor( 3, 1 ).X * id.X;
				n.m20 = CoFactor( 0, 2 ).X * id.X;
				n.m21 = CoFactor( 1, 2 ).X * id.X;
				n.m22 = CoFactor( 2, 2 ).X * id.X;
				n.m23 = CoFactor( 3, 2 ).X * id.X;
				n.m30 = CoFactor( 0, 3 ).X * id.X;
				n.m31 = CoFactor( 1, 3 ).X * id.X;
				n.m32 = CoFactor( 2, 3 ).X * id.X;
				n.m33 = CoFactor( 3, 3 ).X * id.X;
			}

			Row0 = n.Row0;
			Row1 = n.Row1;
			Row2 = n.Row2;
			Row3 = n.Row3;
		}

		RED_INLINE RedMatrix4x4 RedMatrix4x4::Inverted() const
		{
			RedMatrix4x4 n;

			// Get determinant
			RedScalar d = Det();
			if ( !d.Abs().IsAlmostZero() )
			{
				RedScalar id = 1.0f / d.X;

				// Invert matrix
				n.m00 = CoFactor( 0, 0 ).X * id.X;
				n.m01 = CoFactor( 1, 0 ).X * id.X;
				n.m02 = CoFactor( 2, 0 ).X * id.X;
				n.m03 = CoFactor( 3, 0 ).X * id.X;
				n.m10 = CoFactor( 0, 1 ).X * id.X;
				n.m11 = CoFactor( 1, 1 ).X * id.X;
				n.m12 = CoFactor( 2, 1 ).X * id.X;
				n.m13 = CoFactor( 3, 1 ).X * id.X;
				n.m20 = CoFactor( 0, 2 ).X * id.X;
				n.m21 = CoFactor( 1, 2 ).X * id.X;
				n.m22 = CoFactor( 2, 2 ).X * id.X;
				n.m23 = CoFactor( 3, 2 ).X * id.X;
				n.m30 = CoFactor( 0, 3 ).X * id.X;
				n.m31 = CoFactor( 1, 3 ).X * id.X;
				n.m32 = CoFactor( 2, 3 ).X * id.X;
				n.m33 = CoFactor( 3, 3 ).X * id.X;
			}

			return n;
		}
		
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Transpose()
		{
			RedMatrix4x4 n;
			n.m00 = m00;
			n.m01 = m10;
			n.m02 = m20;
			n.m03 = m30;
			n.m10 = m01;
			n.m11 = m11;
			n.m12 = m21;
			n.m13 = m31;
			n.m20 = m02;
			n.m21 = m12;
			n.m22 = m22;
			n.m23 = m32;
			n.m30 = m03;
			n.m31 = m13;
			n.m32 = m23;
			n.m33 = m33;

			Row0 = n.Row0;
			Row1 = n.Row1;
			Row2 = n.Row2;
			Row3 = n.Row3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4 RedMatrix4x4::Transposed() const
		{
			RedMatrix4x4 n;
			n.m00 = m00;
			n.m01 = m10;
			n.m02 = m20;
			n.m03 = m30;
			n.m10 = m01;
			n.m11 = m11;
			n.m12 = m21;
			n.m13 = m31;
			n.m20 = m02;
			n.m21 = m12;
			n.m22 = m22;
			n.m23 = m32;
			n.m30 = m03;
			n.m31 = m13;
			n.m32 = m23;
			n.m33 = m33;
			return n;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::GetColumnMajor( Red::System::Float* _f ) const
		{
			//unrolled
			_f[0] = m00;
			_f[1] = m10;
			_f[2] = m20;
			_f[3] = m30;
			_f[4] = m01;
			_f[5] = m11;
			_f[6] = m21;
			_f[7] = m31;
			_f[8] = m02;
			_f[9] = m12;
			_f[10] = m22;
			_f[11] = m32;
			_f[12] = m03;
			_f[13] = m13;
			_f[14] = m23;
			_f[15] = m33;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Bool RedMatrix4x4::IsOk() const
		{
			return ( Row0.IsOk() && Row1.IsOk() && Row2.IsOk() && Row3.IsOk() );
		}
	}
}