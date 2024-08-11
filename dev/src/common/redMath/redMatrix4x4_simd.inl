namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4()
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);
			
			SetIdentity();
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const RedMatrix4x4& _m )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Set(_m);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Set(_r0, _r1, _r2, _r3);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Set(_r0, _r1, _r2);
			Row3.Set(RedVector4::EW);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::RedMatrix4x4( const Red::System::Float* _f )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(Data, RED_MATH_SSE_MEMORY_ALIGNMENT);

			SetRows(_f);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4::~RedMatrix4x4()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4& RedMatrix4x4::operator=( const RedMatrix4x4& _m )
		{
			Set(_m);

			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const Red::System::Float* RedMatrix4x4::AsFloat() const
		{
			return Data;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Set( const RedMatrix4x4& _m )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(&_m, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Row0 = _m.Row0;
			Row1 = _m.Row1;
			Row2 = _m.Row2;
			Row3 = _m.Row3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Set( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(&_r0, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_r1, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_r2, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_r3, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Row0 = _r0;
			Row1 = _r1;
			Row2 = _r2;
			Row3 = _r3;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Set( const RedVector3& _r0, const RedVector3& _r1, const RedVector3& _r2 )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(&_r0, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_r1, RED_MATH_SSE_MEMORY_ALIGNMENT);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_r2, RED_MATH_SSE_MEMORY_ALIGNMENT);

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
			Row0.SetZeros();
			Row1.SetZeros();
			Row2.SetZeros();
			Row3.SetZeros();
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetIdentity()
		{
			Row0.Set(RedVector4::EX);
			Row1.Set(RedVector4::EY);
			Row2.Set(RedVector4::EZ);
			Row3.Set(RedVector4::EW);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRows( const Red::System::Float* _f )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(_f, RED_MATH_SSE_MEMORY_ALIGNMENT);

			Row0.Set(&_f[0]);
			Row1.Set(&_f[4]);
			Row2.Set(&_f[8]);
			Row3.Set(&_f[12]);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetCols( const Red::System::Float* _f )
		{
			Row0.Set(_f[0], _f[4],  _f[8], _f[12]);
			Row1.Set(_f[1], _f[5],  _f[9], _f[13]);
			Row2.Set(_f[2], _f[6], _f[10], _f[14]);
			Row3.Set(_f[3], _f[7], _f[11], _f[15]);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRows( const RedVector4& _r0, const RedVector4& _r1, const RedVector4& _r2, const RedVector4& _r3 )
		{
			Set(_r0, _r1, _r2, _r3);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetCols( const RedVector4& _c0, const RedVector4& _c1, const RedVector4& _c2, const RedVector4& _c3 )
		{
			Row0.Set( _c0.X, _c1.X, _c2.X, _c3.X );
			Row1.Set( _c0.Y, _c1.Y, _c2.Y, _c3.Y );
			Row2.Set( _c0.Z, _c1.Z, _c2.Z, _c3.Z );
			Row3.Set( _c0.W, _c1.W, _c2.W, _c3.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetColumn( Red::System::Uint32 _index, const RedVector4& _a )
		{
			Data[ _index ] = _a.X;
			Data[ _index + 4 ] = _a.Y;
			Data[ _index + 8 ] = _a.Z;
			Data[ _index + 12 ] = _a.W;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRow( Red::System::Uint32 _index, const RedVector4& _a )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(&_a, RED_MATH_SSE_MEMORY_ALIGNMENT);

			RedVector4 *row = reinterpret_cast<RedVector4*>(&Data[_index * 4]);
			*row = _a;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector4 RedMatrix4x4::GetColumn( Red::System::Uint32 _index ) const
		{
			return RedVector4( Data[_index], Data[_index + 4], Data[_index + 8], Data[_index + 12] );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetRow( Red::System::Uint32 _index ) const
		{
			return (RedVector4&)(Data[ _index * 4 ]);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetAxisX() const
		{
			return Row0;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetAxisY() const
		{
			return Row1;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE const RedVector4& RedMatrix4x4::GetAxisZ() const
		{
			return Row2;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetRotX( Red::System::Float _ccwRadians )
		{
			//reinterpret_cast<RedMath::FLOAT::RedMatrix4x4*>(this)->SetRotX();

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
			Row0.V = _mm_mul_ps(Row0.V, _uniformScale.V);
			Row1.V = _mm_mul_ps(Row1.V, _uniformScale.V);
			Row2.V = _mm_mul_ps(Row2.V, _uniformScale.V);
			Row3.V = _mm_mul_ps(Row3.V, _uniformScale.V);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale( Red::System::Float _uniformScale )
		{
			RedScalar scale(_uniformScale);
			SetScale(scale);
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale( const RedVector3& _scale )
		{
			m00 *= _scale.X;
			m01 *= _scale.X;
			m02 *= _scale.X;
			m10 *= _scale.Y;
			m11 *= _scale.Y;
			m12 *= _scale.Y;
			m20 *= _scale.Z;
			m21 *= _scale.Z;
			m22 *= _scale.Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale( const RedVector4& _scale )
		{
			SetMul(Row0, RedScalar(_scale.X));
			SetMul(Row1, RedScalar(_scale.Y));
			SetMul(Row2, RedScalar(_scale.Z));
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetScale33( const RedVector4& _scale )
		{
			m00 *= _scale.X;
			m01 *= _scale.X;
			m02 *= _scale.X;
			m10 *= _scale.Y;
			m11 *= _scale.Y;
			m12 *= _scale.Y;
			m20 *= _scale.Z;
			m21 *= _scale.Z;
			m22 *= _scale.Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetPreScale44( const RedVector4& _scale )
		{
			m00 *= _scale.X;
			m01 *= _scale.Y;
			m02 *= _scale.Z;
			m10 *= _scale.X;
			m11 *= _scale.Y;
			m12 *= _scale.Z;
			m20 *= _scale.X;
			m21 *= _scale.Y;
			m22 *= _scale.Z;
			m30 *= _scale.X;
			m31 *= _scale.Y;
			m32 *= _scale.Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::SetPreScale33( const RedVector4& _scale )
		{
			m00 *= _scale.X;
			m01 *= _scale.Y;
			m02 *= _scale.Z;
			m10 *= _scale.X;
			m11 *= _scale.Y;
			m12 *= _scale.Z;
			m20 *= _scale.X;
			m21 *= _scale.Y;
			m22 *= _scale.Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector4 RedMatrix4x4::GetScale() const
		{
			return RedVector4( Row0.Length3(), Row1.Length3(), Row2.Length3(), 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedVector4 RedMatrix4x4::GetPreScale() const
		{
			RedVector4 v0( m00, m10, m20 );
			RedVector4 v1( m01, m11, m21 );
			RedVector4 v2( m02, m12, m22 );

			return RedVector4( v0.Length3(), v1.Length3(), v2.Length3(), 1.0f );
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
		RED_INLINE void CoFactor(RedScalar &v0, RedScalar &v1, RedScalar &v2, RedScalar &v3, RedScalar &ret)
		{
			ret = _mm_sub_ps(_mm_mul_ps(v0.V, v1.V), _mm_mul_ps(v2.V, v3.V));
		}

		//////////////////////////////////////////////////////////////////////////
		// This function is not used for the inverse or determinant calculation anymore
		// It remains in place for compatibility reasons with the FLOAT version API
		// but it is not a critical function anymore
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
		RED_INLINE RedMatrix4x4 RedMatrix4x4::Inverted() const
		{
			RedMatrix4x4 n = *this;
			n.Invert();
			return n;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void RedMatrix4x4::Transpose()
		{
			__m128 a = _mm_shuffle_ps((Row0.V), (Row1.V), _MM_SHUFFLE(1, 0, 1, 0));
			__m128 b = _mm_shuffle_ps((Row0.V), (Row1.V), _MM_SHUFFLE(3, 2, 3, 2));
			__m128 c = _mm_shuffle_ps((Row2.V), (Row3.V), _MM_SHUFFLE(1, 0, 1, 0));
			__m128 d = _mm_shuffle_ps((Row2.V), (Row3.V), _MM_SHUFFLE(3, 2, 3, 2));
				
			Row0.V = _mm_shuffle_ps(a, c, _MM_SHUFFLE(2, 0, 2, 0));
			Row1.V = _mm_shuffle_ps(a, c, _MM_SHUFFLE(3, 1, 3, 1));
			Row2.V = _mm_shuffle_ps(b, d, _MM_SHUFFLE(2, 0, 2, 0));
			Row3.V = _mm_shuffle_ps(b, d, _MM_SHUFFLE(3, 1, 3, 1));
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedMatrix4x4 RedMatrix4x4::Transposed() const
		{
			RedMatrix4x4 n = *this;
			n.Transpose();
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

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedQuaternion RedMatrix4x4::ToQuaternion() const
		{
			float tr = Row0.f[0] + Row1.f[1] + Row2.f[2] + 1.0f;
			float qw;
			float qx;
			float qy;
			float qz;

			if ( tr > 0.0f ) 
			{
				float S = Red::Math::MSqrt( tr ) * 2.0f; // S=4*qw 
				qw = 0.25f * S;
				qx = (Row1.Z - Row2.Y) / S;
				qy = (Row2.X - Row0.Z) / S; 
				qz = (Row0.Y - Row1.X) / S; 
			} 
			else if ( ( Row0.X > Row1.Y ) && ( Row0.X > Row2.Z ) ) 
			{ 
				float S = Red::Math::MSqrt( 1.0f + Row0.X - Row1.Y - Row2.Z ) * 2.0f; // S=4*qx 
				qw = (Row1.Z - Row2.Y) / S;
				qx = 0.25f * S;
				qy = (Row1.X + Row0.Y) / S; 
				qz = (Row2.X + Row0.Z) / S; 
			} 
			else if (Row1.Y > Row2.Z) 
			{ 
				float S = Red::Math::MSqrt( 1.0f + Row1.Y - Row0.X - Row2.Z ) * 2.f; // S=4*qy
				qw = ( Row2.X - Row0.Z ) / S;
				qx = ( Row1.X + Row0.Y ) / S; 
				qy = 0.25f * S;
				qz = ( Row2.Y + Row1.Z ) / S;
			} 
			else 
			{ 
				float S = Red::Math::MSqrt( 1.0f + Row2.Z - Row0.X - Row1.Y ) * 2.f; // S=4*qz
				qw = (Row0.Y - Row1.X) / S;
				qx = (Row2.X + Row0.Z) / S;
				qy = (Row2.Y + Row1.Z) / S;
				qz = 0.25f * S;
			}

			return RedQuaternion( qx, qy, qz, qw );
		}

		// Convert rotation to euler angles
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedEulerAngles RedMatrix4x4::ToEulerAngles() const
		{
			RedEulerAngles ret;

			if ( Row1.Z > 0.995f )
			{
				ret.Roll =  RAD2DEG( Red::Math::MATan2( Row2.X, Row0.X ) );
				ret.Pitch =  90.0f;
				ret.Yaw =  0.0f;
			}
			else if ( Row1.Z < -0.995f )
			{
				ret.Roll =  RAD2DEG( Red::Math::MATan2( Row2.X, Row0.X ) );
				ret.Pitch = -90.0f;
				ret.Yaw =  0.0f;
			}
			else
			{
				ret.Roll = -RAD2DEG( Red::Math::MATan2( Row0.Z, Row2.Z ) );
				ret.Pitch =  RAD2DEG( Red::Math::MAsin(  Row1.Z ) );
				ret.Yaw	= -RAD2DEG( Red::Math::MATan2( Row1.X,  Row1.Y ) );
			}

			return ret;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE RedEulerAngles RedMatrix4x4::ToEulerAnglesFull() const
		{
			RedEulerAngles ret;

			float row1Mag = Row1.Length3();
			float row2MagSqr = Row2.SquareLength3();

			float unscaleR1     = 1.f / row1Mag;
			float rescaleR2toR0 = Red::Math::MSqrt( ( Row0.SquareLength3() / row2MagSqr ) );

			float cell12 = Row1.Z * unscaleR1;
			if ( cell12 > 0.995f )
			{
				ret.Roll = RAD2DEG( Red::Math::MATan2( Row2.X * rescaleR2toR0, Row0.X ) );
				ret.Pitch =  90.0f;
				ret.Yaw =  0.0f;
			}
			else if ( cell12 < -0.995f )
			{
				ret.Roll =  RAD2DEG( Red::Math::MATan2( Row2.X * rescaleR2toR0, Row0.X ) );
				ret.Pitch = -90.0f;
				ret.Yaw	=  0.0f;
			}
			else
			{
				ret.Roll = -RAD2DEG( Red::Math::MATan2( Row0.Z, Row2.Z * rescaleR2toR0 ) );
				ret.Pitch =  RAD2DEG( asin( cell12 ) );
				ret.Yaw	= -RAD2DEG( Red::Math::MATan2( Row1.X, Row1.Y ) ); // unscaling is not needed, both arguments has same scale
			}

			return ret;
		}
	}
}