namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2()
			: X( 0.0f )
			, Y( 0.0f )
			, Z( 0.0f )
			, W( 0.0f )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( const RedScalar& _v )
			: X( _v.X )
			, Y( _v.X )
			, Z( _v.X )
			, W( _v.X )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( const RedVector2& _v )
			: X( _v.X )
			, Y( _v.Y )
			, Z( _v.Z )
			, W( _v.W )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( Red::System::Float _x, Red::System::Float _y )
			: X( _x )
			, Y( _y )
			, Z( _x )
			, W( _y )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::RedVector2( const Red::System::Float* _f )
			: X( _f[0] )
			, Y( _f[1] )
			, Z( _f[0] )
			, W( _f[1] )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2::~RedVector2()
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::operator = ( const RedScalar& _v )
		{
			X = _v.X;
			Y = _v.X;
			Z = _v.X;
			W = _v.X;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::operator = ( const RedVector2& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = _v.W;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		const Red::System::Float* RedVector2::AsFloat() const
		{
			return &V[0];
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( Red::System::Float _x, Red::System::Float _y )
		{
			X = _x;
			Y = _y;
			Z = _x;
			W = _y;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( const RedScalar& _v )
		{
			X = _v.X;
			Y = _v.X;
			Z = _v.X;
			W = _v.X;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( const RedVector2& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = _v.W;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::Set( const Red::System::Float* _f )
		{
			X = _f[0];
			Y = _f[1];
			Z = _f[0];
			W = _f[1];
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::SetZeros()
		{
			X = 0.0f;
			Y = 0.0f;
			Z = 0.0f;
			W = 0.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector2::SetOnes()
		{
			X = 1.0f;
			Y = 1.0f;
			Z = 1.0f;
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::Negate()
		{
			X = -X;
			Y = -Y;
			Z = -Z;
			W = -W;
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 RedVector2::Negated() const
		{
			return RedVector2( -X, -Y );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector2::Length() const
		{
			return Red::Math::MSqrt( Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector2::SquareLength() const
		{
			return Red::Math::MSqr( X ) + Red::Math::MSqr( Y );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2& RedVector2::Normalize()
		{
			Red::System::Float length = Red::Math::MSqrt( Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) );
			if( length != 0.0f )
			{
				Red::System::Float unitLength = 1.0f / length;
				X *= unitLength;
				Y *= unitLength;
				Z = X;
				W = Y;
			}
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 RedVector2::Normalized() const
		{
			Red::System::Float length = Red::Math::MSqrt( Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) );
			if( length == 0.0f )
			{
				return RedVector2::ZEROS;
			}
			Red::System::Float unitLength = 1.0f / length;
			return RedVector2( X * unitLength, Y * unitLength );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector2::IsAlmostEqual( const RedVector2& _v, Red::System::Float _epsilon ) const
		{
			return ( Red::Math::NumericalUtils::Abs< Red::System::Float >( X - _v.X ) < _epsilon ) &&
				   ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Y - _v.Y ) < _epsilon );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector2::IsZero() const
		{
			return ( X == 0.0f && Y == 0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector2::IsAlmostZero( Red::System::Float _epsilon ) const
		{
			return ( Red::Math::NumericalUtils::Abs< Red::System::Float >( X ) <= _epsilon ) &&
				   ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Y ) <= _epsilon ); 
		}
	}
}