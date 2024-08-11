
namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3()
			: X( 0.0f )
			, Y( 0.0f )
			, Z( 0.0f )
			, W( 1.0f )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( const RedScalar& _v )
			: X( _v.X )
			, Y( _v.X )
			, Z( _v.X )
			, W( 1.0f )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( const RedVector3& _v )
			: X( _v.X )
			, Y( _v.Y )
			, Z( _v.Z )
			, W( 1.0f )
		{
		}
		
		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z )
			: X( _x )
			, Y( _y )
			, Z( _z )
			, W( 1.0f )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::RedVector3( const Red::System::Float* _f )
			: X( _f[0] )
			, Y( _f[1] )
			, Z( _f[2] )
			, W( 1.0f )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3::~RedVector3()
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::operator = ( const RedScalar& _v )
		{
			X = _v.X;
			Y = _v.X;
			Z = _v.X;
			W = 1.0f;
			return ( *this );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::operator = ( const RedVector3& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = 1.0f;
			return ( *this );
		}

		//////////////////////////////////////////////////////////////////////////
		const Red::System::Float* RedVector3::AsFloat() const
		{
			return &V[0];
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( Red::System::Float _x, Red::System::Float _y, Red::System::Float _z )
		{
			X = _x;
			Y = _y;
			Z = _z;
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( const RedScalar& _v )
		{
			X = _v.X;
			Y = _v.X;
			Z = _v.X;
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( const RedVector3& _v )
		{
			X = _v.X;
			Y = _v.Y;
			Z = _v.Z;
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::Set( const Red::System::Float* _f )
		{
			X = _f[0];
			Y = _f[1];
			Z = _f[2];
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::SetZeros()
		{
			X = 0.0f;
			Y = 0.0f;
			Z = 0.0f;
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		void RedVector3::SetOnes()
		{
			X = 1.0f;
			Y = 1.0f;
			Z = 1.0f;
			W = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::Negate()
		{
			X = -X;
			Y = -Y;
			Z = -Z;
			return (*this);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector3::Negated() const
		{
			return RedVector3( -X, -Y, -Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector3::Length() const
		{
			return Red::Math::MSqrt( Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) + Red::Math::MSqr( Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedVector3::SquareLength() const
		{
			return Red::Math::MSqr( X ) + Red::Math::MSqr( Y ) + Red::Math::MSqr( Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3& RedVector3::Normalize()
		{
			RedScalar length = Length();
			if( !length.IsZero() )
			{
				Red::System::Float unitLength = 1.0f / length.X;
				X *= unitLength;
				Y *= unitLength;
				Z *= unitLength;
			}
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RedVector3::Normalized() const
		{
			RedScalar length = Length();
			if( length.IsZero() )
			{
				return *this;
			}
			Red::System::Float unitLength = 1.0f / length.X;
			return RedVector3( X * unitLength, Y * unitLength, Z * unitLength );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsAlmostEqual( const RedVector3& _v, Red::System::Float _epsilon ) const
		{
			return ( Red::Math::NumericalUtils::Abs< Red::System::Float >( X - _v.X ) < _epsilon ) &&
				   ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Y - _v.Y ) < _epsilon ) &&
				   ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Z - _v.Z ) < _epsilon );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsZero() const
		{
			return ( X == 0.0f && Y == 0.0f && Z == 0.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsAlmostZero( Red::System::Float _epsilon ) const
		{
			return ( Red::Math::NumericalUtils::Abs< Red::System::Float >( X ) <= _epsilon ) &&
				   ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Y ) <= _epsilon ) &&
				   ( Red::Math::NumericalUtils::Abs< Red::System::Float >( Z ) <= _epsilon );
		}

		//////////////////////////////////////////////////////////////////////////
		Red::System::Bool RedVector3::IsOk() const
		{
			return ( Red::Math::NumericalUtils::IsFinite( X ) &&
					 Red::Math::NumericalUtils::IsFinite( Y ) &&
					 Red::Math::NumericalUtils::IsFinite( Z ) );
		}
	}
}