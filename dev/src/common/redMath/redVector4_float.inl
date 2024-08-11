
namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4()
			: X( 0.0f )
			, Y( 0.0f )
			, Z( 0.0f )
			, W( 0.0f )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedScalar& _v )
			: X( _v.X )
			, Y( _v.X )
			, Z( _v.X )
			, W( _v.X )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedVector3& _v )
			: X( _v.X )
			, Y( _v.Y )
			, Z( _v.Z )
			, W( 0.0f )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedVector3& _v, Red::System::Float _w )
			: X( _v.X )
			, Y( _v.Y )
			, Z( _v.Z )
			, W( _w )
		{
		}
		
		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const RedVector4& _v )
			: X( _v.X )
			, Y( _v.Y )
			, Z( _v.Z )
			, W( _v.W )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( const Red::System::Float* _f )
			: X( _f[0] )
			, Y( _f[1] )
			, Z( _f[2] )
			, W( _f[3] )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::RedVector4( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w )
			: X( _x )
			, Y( _y )
			, Z( _z )
			, W( _w )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4::~RedVector4()
		{
		}

		//////////////////////////////////////////////////////////////////////////
		const Red::System::Float* RedVector4::AsFloat() const
		{
			return V;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::operator = ( const RedScalar& _v )
		{
			X = _v.X;
			Y = _v.X;
			Z = _v.X;
			W = _v.X;
			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::operator = ( const RedVector3& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = 0.0f;
			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::operator = ( const RedVector4& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = _v.W;
			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const RedScalar& _v )
		{
			X = _v.X;
			Y = _v.X;
			Z = _v.X;
			W = _v.X;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const RedVector3& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = 0.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const RedVector3& _v, Red::System::Float _w )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = _w;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const RedVector4& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = _v.W;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z, Red::System::Float _w )
		{
			X = _x;
			Y = _y;
			Z = _z;
			W = _w;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::Set( const Red::System::Float* _f )
		{
			X = _f[0];
			Y = _f[1];
			Z = _f[2];
			W = _f[3];
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::SetZeros()
		{
			X = 0.0f;
			Y = 0.0f;
			Z = 0.0f;
			W = 0.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector4::SetOnes()
		{
			X = 1.0f;
			Y = 1.0f;
			Z = 1.0f;
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::Negate()
		{
			X = -X;
			Y = -Y;
			Z = -Z;
			W = -W;
			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Negated() const
		{
			return RedVector4( -X, -Y, -Z, -W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Abs() const
		{
			return RedVector4( Red::Math::NumericalUtils::Abs< Red::System::Float >( X ),
							   Red::Math::NumericalUtils::Abs< Red::System::Float >( Y ),
							   Red::Math::NumericalUtils::Abs< Red::System::Float >( Z ),
							   Red::Math::NumericalUtils::Abs< Red::System::Float >( W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Sum3() const
		{
			return X + Y + Z;
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Length3() const
		{
			return Red::Math::MSqrt( Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) + Red::Math::MSqr( Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::SquareLength3() const
		{
			return Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) + Red::Math::MSqr( Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Sum4() const
		{
			return X + Y + Z + W;
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Length4() const
		{
			return Red::Math::MSqrt( Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) + Red::Math::MSqr( Z ) + Red::Math::MSqr( W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::SquareLength4() const
		{
			return Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) + Red::Math::MSqr( Z ) + Red::Math::MSqr( W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::Normalize4()
		{
			RedScalar length = Length4();
			if( !length.IsZero() )
			{
				Red::System::Float unitLength = 1.0f / length.X;
				X *= unitLength;
				Y *= unitLength;
				Z *= unitLength;
				W *= unitLength;
			}
			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4& RedVector4::Normalize3()
		{
			RedScalar length = Length3();
			if( !length.IsZero() )
			{
				Red::System::Float unitLength = 1.0f / length.X;
				X *= unitLength;
				Y *= unitLength;
				Z *= unitLength;
				W = 0.0f;
			}
			return ( *this );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Normalized4() const
		{
			RedScalar length = Length4();
			if( length.IsZero() )
			{
				return *this;
			}
			Red::System::Float unitLength = 1.0f / length.X;
			return RedVector4( X * unitLength, Y * unitLength, Z * unitLength, W * unitLength );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::Normalized3() const
		{
			RedScalar length = Length3();
			if( length.IsZero() )
			{
				return *this;
			}
			Red::System::Float unitLength = 1.0f / length.X;
			return RedVector4( X * unitLength, Y * unitLength, Z * unitLength );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsNormalized4( Red::System::Float _epsilon ) const
		{
			Red::System::Float sqrLength = SquareLength4().X;
			sqrLength -= 1.0f;
			sqrLength = ( sqrLength >= 0.0f ) ? sqrLength : -sqrLength;
			return ( sqrLength <= _epsilon );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsNormalized3( Red::System::Float _epsilon ) const
		{
			Red::System::Float sqrLength = SquareLength3().X;
			sqrLength -= 1.0f;
			sqrLength = ( sqrLength >= 0.0f ) ? sqrLength : -sqrLength;
			return ( sqrLength <= _epsilon );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsAlmostEqual( const RedVector4& _v, Red::System::Float _epsilon ) const
		{
			return ( ( Red::Math::NumericalUtils::Abs< Red::System::Float >( X - _v.X ) < _epsilon ) &&
				 ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Y - _v.Y ) < _epsilon ) &&
				 ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Z - _v.Z ) < _epsilon ) &&
				 ( Red::Math::NumericalUtils::Abs< Red::System::Float >( W - _v.W ) < _epsilon ) );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsAlmostZero( Red::System::Float _epsilon ) const
		{
			return ( ( Red::Math::NumericalUtils::Abs< Red::System::Float >( X ) <= _epsilon ) &&
					( Red::Math::NumericalUtils::Abs< Red::System::Float >( Y ) <= _epsilon ) &&
					( Red::Math::NumericalUtils::Abs< Red::System::Float >( Z ) <= _epsilon ) &&
					( Red::Math::NumericalUtils::Abs< Red::System::Float >( W ) <= _epsilon ) );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsZero() const
		{
			return ( X == 0.0f && Y == 0.0f && Z == 0.0f && W == 0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Upper3() const
		{
			return Red::Math::NumericalUtils::Max( X, Red::Math::NumericalUtils::Max( Y, Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Upper4() const
		{
			return Red::Math::NumericalUtils::Max( Red::Math::NumericalUtils::Max( X, Y ), Red::Math::NumericalUtils::Max( Z, W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Lower3() const
		{
			return Red::Math::NumericalUtils::Min( X, Red::Math::NumericalUtils::Min( Y, Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::Lower4() const
		{
			return Red::Math::NumericalUtils::Min( Red::Math::NumericalUtils::Min( X, Y ), Red::Math::NumericalUtils::Min( Z, W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RedVector4::ZeroElement( Red::System::Uint32 _i ) const
		{
			RedVector4 result( *this );
			result.V[_i] = 0.0f;
			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector4::AsScalar( Red::System::Uint32 _i ) const
		{
			return RedScalar( V[_i] );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector4::AsVector3() const
		{
			return RedVector3( V );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector4::IsOk() const
		{
			return ( Red::Math::NumericalUtils::IsFinite( X ) && 
				Red::Math::NumericalUtils::IsFinite( Y ) && 
				Red::Math::NumericalUtils::IsFinite( Z ) &&
				Red::Math::NumericalUtils::IsFinite( W ) );
		}
	}
}