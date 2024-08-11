/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red 
{ 
	namespace Math
	{
		namespace Fpu
		{
				// *** RedVector1
				RedVector1::RedVector1()
				{

				}

				RedVector1::RedVector1( const float _f ) 
					: X(_f), Y(_f), Z(_f), W(_f)
				{
				}

				RedVector1::RedVector1( const RedVector1& _v )
					: X(_v.X), Y(_v.X), Z(_v.X), W(_v.X)
				{
				}

				RedVector1::~RedVector1()
				{
				}

				RedVector1 RedVector1::operator-() const
				{
					return RedVector1::Negated();
				}

				RedVector1 RedVector1::operator+( const RedVector1& _v ) const
				{
					return RedVector1( ( *this ).X + _v.X );
				}

				RedVector1 RedVector1::operator-( const RedVector1& _v ) const
				{
					return RedVector1( X - _v.X );
				}

				RedVector1 RedVector1::operator*( const RedVector1& _v ) const
				{
					return RedVector1( X * _v.X );
				}

				RedVector1 RedVector1::operator/( const RedVector1& _v ) const
				{
					return RedVector1( X / _v.X );
				}

				RedVector1 RedVector1::operator+( float _f ) const
				{
					return RedVector1( X + _f );
				}

				RedVector1 RedVector1::operator-( float _f ) const
				{
					return RedVector1( X - _f );
				}

				RedVector1 RedVector1::operator*( float _f ) const
				{
					return RedVector1( X * _f );
				}

				RedVector1 RedVector1::operator/( float _f ) const
				{
					return RedVector1( X / _f );
				}

				RedVector1& RedVector1::operator=( const RedVector1& _v )
				{
					X = _v.X;
					Y = _v.X;
					Z = _v.X;
					W = _v.X;
					return *this;
				}

				RedVector1& RedVector1::operator+=( const RedVector1& _v )
				{
					X += _v.X;
					Y += _v.X;
					Z += _v.X;
					W += _v.X;
					return *this;
				}

				RedVector1& RedVector1::operator-=( const RedVector1& _v )
				{
					X -= _v.X;
					Y -= _v.X;
					Z -= _v.X;
					W -= _v.X;
					return *this;
				}

				RedVector1& RedVector1::operator*=( const RedVector1& _v )
				{
					X *= _v.X;
					Y *= _v.X;
					Z *= _v.X;
					W *= _v.X;
					return *this;
				}

				RedVector1& RedVector1::operator/=( const RedVector1& _v )
				{
					X /= _v.X;
					Y /= _v.X;
					Z /= _v.X;
					W /= _v.X;
					return *this;
				}

				RedVector1& RedVector1::operator+=( float _f )
				{
					X += _f;
					Y += _f;
					Z += _f;
					W += _f;
					return *this;
				}

				RedVector1& RedVector1::operator-=( float _f )
				{
					X -= _f;
					Y -= _f;
					Z -= _f;
					W -= _f;
					return *this;
				}

				RedVector1& RedVector1::operator*=( float _f )
				{
					X *= _f;
					Y *= _f;
					Z *= _f;
					W *= _f;
					return *this;
				}

				RedVector1& RedVector1::operator/=( float _f )
				{
					X /= _f;
					Y /= _f;
					Z /= _f;
					W /= _f;
					return *this;
				}

				bool RedVector1::operator==( const RedVector1& _v ) const
				{
					return ( X == _v.X );
				}

				bool RedVector1::operator!=( const RedVector1& _v ) const
				{
					return (X != _v.X );
				}

				RedVector1 RedVector1::Set( float _f )
				{
					X = _f;
					Y = _f;
					Z = _f;
					W = _f;
					return *this; 
				}

				RedVector1 RedVector1::Set( const RedVector1& _v )
				{
					X = _v.X;
					Y = _v.X;
					Z = _v.X;
					W = _v.X;
					return *this;
				}

				RedVector1& RedVector1::SetZeros()
				{
					X = 0.0f;
					Y = 0.0f;
					Z = 0.0f;
					W = 0.0f;
					return *this;
				}

				RedVector1& RedVector1::SetOnes()
				{
					X = 1.0f;
					Y = 1.0f;
					Z = 1.0f;
					W = 1.0f;
					return *this;
				}

				RedVector1& RedVector1::Negate()
				{
					X = -X;
					Y = -Y;
					Z = -Z;
					W = -W;
					return *this;
				}

				RedVector1 RedVector1::Negated() const
				{
					return RedVector1( -X );
				}

				RedVector1 RedVector1::Abs() const
				{
					return RedVector1( NumericalUtils::Abs( X ) );
				}

				bool RedVector1::IsZero() const
				{
					return ( X == 0.0f );
				}

				bool RedVector1::IsAlmostZero( float _epsilon ) const
				{	
					return ( NumericalUtils::Abs( X ) <= _epsilon );
				}

				// *** RedVector2
				RedVector2::RedVector2()
				{

				}

				RedVector2::RedVector2( const RedVector1& _v ) 
					: X( _v.X ), Y( _v.Y ), Z(_v.Z), W(_v.W)
				{

				}

				RedVector2::RedVector2( const RedVector2& _v ) 
					: X( _v.X ), Y( _v.Y ), Z(_v.Z), W(_v.W)
				{

				}

				RedVector2::RedVector2( float _x, float _y )
					: X( _x ), Y( _y ), Z( _x ), W( _y )
				{
				}

				RedVector2::RedVector2( const float* _f )
					: X( _f[0] ), Y( _f[1] ), Z( _f[0] ), W( _f[1] )
				{

				}

				RedVector2::~RedVector2()
				{
				}

				RedVector2& RedVector2::operator=( const RedVector1& _v )
				{
					X = _v.X;
					Y = _v.Y;
					return *this;
				}

				RedVector2& RedVector2::operator=( const RedVector2& _v )
				{
					X = _v.X;
					Y = _v.Y;
					return *this;
				}

				RedVector2 RedVector2::operator+( float _f ) const
				{	
					return RedVector2( X + _f, Y + _f );
				}

				RedVector2 RedVector2::operator-( float _f ) const
				{
					return RedVector2( X - _f, Y - _f );
				}

				RedVector2 RedVector2::operator*( float _f ) const
				{
					return RedVector2( X * _f, Y * _f );
				}

				RedVector2 RedVector2::operator/( float _f ) const
				{
					return RedVector2( X / _f, Y / _f );
				}

				RedVector2 RedVector2::operator+( const RedVector1& _v ) const
				{
					return RedVector2( X + _v.X, Y + _v.Y );
				}
		
				RedVector2 RedVector2::operator-( const RedVector1& _v ) const
				{
					return RedVector2( X - _v.X, Y - _v.Y );
				}
		
				RedVector2 RedVector2::operator*( const RedVector1& _v ) const
				{
					return RedVector2( X * _v.X, Y * _v.Y );
				}
		
				RedVector2 RedVector2::operator/( const RedVector1& _v ) const
				{
					return RedVector2( X / _v.X, Y / _v.Y );
				}

				RedVector2 RedVector2::operator+( const RedVector2& _v ) const
				{
					return RedVector2( X + _v.X, Y + _v.Y );
				}

				RedVector2 RedVector2::operator-( const RedVector2& _v ) const
				{
					return RedVector2( X - _v.X, Y - _v.Y );
				}

				RedVector2 RedVector2::operator*( const RedVector2& _v ) const
				{
					return RedVector2( X * _v.X, Y * _v.Y );
				}

				RedVector2 RedVector2::operator/( const RedVector2& _v ) const
				{
					return RedVector2( X / _v.X, Y / _v.Y );
				}

				RedVector2& RedVector2::operator+=( const RedVector1& _v) 
				{
					X += _v.X;
					Y += _v.X;
					Z += _v.X;
					W += _v.X;
					return *this;
				}

				RedVector2& RedVector2::operator-=( const RedVector1& _v)
				{
					X -= _v.X;
					Y -= _v.X;
					Z -= _v.X;
					W -= _v.X;
					return *this;
				}

				RedVector2& RedVector2::operator*=( const RedVector1& _v) 
				{
					X *= _v.X;
					Y *= _v.X;
					Z *= _v.X;
					W *= _v.X;
					return *this;
				}

				RedVector2& RedVector2::operator/=( const RedVector1& _v)
				{
					X /= _v.X;
					Y /= _v.X;
					Z /= _v.X;
					W /= _v.X;
					return *this;
				}

				RedVector2& RedVector2::operator+=( const RedVector2& _v)
				{
					X += _v.X;
					Y += _v.Y;
					Z += _v.X;
					W += _v.Y;
					return *this;
				}

				RedVector2& RedVector2::operator-=( const RedVector2& _v) 
				{
					X -= _v.X;
					Y -= _v.Y;
					Z -= _v.X;
					W -= _v.Y;
					return *this;
				}

				RedVector2& RedVector2::operator*=( const RedVector2& _v)
				{
					X *= _v.X;
					Y *= _v.Y;
					Z *= _v.X;
					W *= _v.Y;
					return *this;
				}

				RedVector2& RedVector2::operator/=( const RedVector2& _v) 
				{
					X /= _v.X;
					Y /= _v.Y;
					Z /= _v.X;
					W /= _v.Y;
					return *this;
				}
			
				bool RedVector2::operator==(const RedVector1& _v) const
				{
					return ( X == _v.X && Y == _v.Y );
				}
		
				bool RedVector2::operator!=(const RedVector1& _v) const
				{
					return ( X != _v.X && Y != _v.Y );
				}

				bool RedVector2::operator==(const RedVector2& _v) const
				{
					return ( X == _v.X && Y == _v.Y );
				}

				bool RedVector2::operator!=(const RedVector2& _v) const
				{
					return ( X != _v.X && Y != _v.Y );
				}

				float RedVector2::Length() const
				{
					return ::sqrtf( ( X * X ) + ( Y * Y ) );
				}

				float RedVector2::SquareLength() const
				{
					return ( X * X ) + ( Y * Y );
				}

				RedVector2& RedVector2::Normalize()
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) );
					if( length != 0.0f )
					{
						float unitLength = 1.0f / length;
						X *= unitLength;
						Y *= unitLength;
					}
					return *this;
				}

				RedVector2 RedVector2::Normalized() const
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) );
					if( length == 0.0f )
					{
						return *this;
					}
					float unitLength = 1.0f / length;
					return RedVector2( X * unitLength, Y * unitLength );
				}

				float RedVector2::Dot( const RedVector2& _v ) const
				{
					return ( ( X * _v.X ) + ( Y * _v.Y ) );
				}

				void RedVector2::Set( float _x, float _y )
				{
					X = _x;
					Y = _y;
					Z = _x;
					W = _y;
				}

				float RedVector2::CrossZ( const RedVector2& _v ) const
				{
					return ( X * _v.Y ) - ( Y * _v.X );
				}

				// *** RedVector3
				RedVector3::RedVector3()
				{
				}

				RedVector3::RedVector3( const RedVector1& _v ) 
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( _v.W )
				{
				}

				RedVector3::RedVector3( const RedVector2& _v )
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( _v.W )
				{
				}

				RedVector3::RedVector3( const RedVector3& _v )
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( 0.0f )
				{
				}

				RedVector3::RedVector3( float _f )
					: X( _f ), Y( _f ), Z( _f ), W( 0.0f )
				{
				}

				RedVector3::RedVector3( const float* _f )
					: X( _f[0] ), Y( _f[1] ), Z( _f[2] ), W( 0.0f )
				{
				}

				RedVector3::RedVector3( float _x, float _y, float _z )
					: X( _x ), Y( _y ), Z( _z ), W( 0.0f )
				{
				}

		
				RedVector3::~RedVector3()
				{
				}

				RedVector3& RedVector3::operator=( const RedVector1& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = 1.0f;
					return *this;
				}

				RedVector3& RedVector3::operator=( const RedVector2& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = 1.0f;
					return *this;
				}

				RedVector3& RedVector3::operator=( const RedVector3& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = 1.0f;
					return *this;
				}

				RedVector3 RedVector3::operator+( float _f ) const
				{
					return RedVector3( X + _f, Y + _f, Z + _f );
				}

				RedVector3 RedVector3::operator-( float _f ) const
				{
					return RedVector3( X - _f, Y - _f, Z - _f );
				}

				RedVector3 RedVector3::operator*( float _f ) const
				{
					return RedVector3( X * _f, Y * _f, Z * _f );
				}

				RedVector3 RedVector3::operator/( float _f ) const
				{
					return RedVector3( X / _f, Y / _f, Z / _f );
				}

				RedVector3 RedVector3::operator+( const RedVector1& _v ) const
				{
					return RedVector3( X + _v.X, Y + _v.Y, Z + _v.Z );
				}

				RedVector3 RedVector3::operator-( const RedVector1& _v ) const
				{
					return RedVector3( X - _v.X, Y - _v.Y, Z - _v.Z );
				}

				RedVector3 RedVector3::operator*( const RedVector1& _v ) const
				{
					return RedVector3( X * _v.X, Y * _v.Y, Z * _v.Z );
				}

				RedVector3 RedVector3::operator/( const RedVector1& _v ) const
				{
					return RedVector3( X / _v.X, Y / _v.Y, Z / _v.Z );
				}

				RedVector3 RedVector3::operator+( const RedVector2& _v ) const
				{
					return RedVector3( X + _v.X, Y + _v.Y, Z );
				}

				RedVector3 RedVector3::operator-( const RedVector2& _v ) const
				{
					return RedVector3( X - _v.X, Y - _v.Y, Z );
				}

				RedVector3 RedVector3::operator*( const RedVector2& _v ) const
				{
					return RedVector3( X * _v.X, Y * _v.Y, Z );
				}

				RedVector3 RedVector3::operator/( const RedVector2& _v ) const
				{
					return RedVector3( X / _v.X, Y / _v.Y, Z );
				}

				RedVector3 RedVector3::operator+( const RedVector3& _v ) const
				{
					return RedVector3( X + _v.X, Y + _v.Y, Z + _v.Z );
				}

				RedVector3 RedVector3::operator-( const RedVector3& _v ) const
				{
					return RedVector3( X - _v.X, Y - _v.Y, Z - _v.Z );
				}

				RedVector3 RedVector3::operator*( const RedVector3& _v ) const
				{
					return RedVector3( X * _v.X, Y * _v.Y, Z * _v.Z );
				}

				RED_INLINE RedVector3 RedVector3::operator/( const RedVector3& _v ) const
				{
					return RedVector3( X / _v.X, Y / _v.Y, Z / _v.Z );
				}

				RedVector3& RedVector3::operator+=( const RedVector1& _v)
				{
					X += _v.X;
					Y += _v.Y;
					Z += _v.Z;
					return *this;
				}

				RedVector3& RedVector3::operator-=( const RedVector1& _v)
				{
					X -= _v.X;
					Y -= _v.Y;
					Z -= _v.Z;
					return *this;
				}

				RedVector3& RedVector3::operator*=( const RedVector1& _v)
				{
					X *= _v.X;
					Y *= _v.Y;
					Z *= _v.Z;
					return *this;
				}

				RedVector3& RedVector3::operator/=( const RedVector1& _v)
				{
					X /= _v.X;
					Y /= _v.Y;
					Z /= _v.Z;
					return *this;
				}

				RedVector3& RedVector3::operator+=( const RedVector2& _v)
				{
					X += _v.X;
					Y += _v.Y;
					return *this;
				}

				RedVector3& RedVector3::operator-=( const RedVector2& _v)
				{
					X -= _v.X;
					Y -= _v.Y;
					return *this;
				}

				RedVector3& RedVector3::operator*=( const RedVector2& _v)
				{
					X *= _v.X;
					Y *= _v.Y;
					return *this;
				}

				RedVector3& RedVector3::operator/=( const RedVector2& _v)
				{
					X /= _v.X;
					Y /= _v.Y;
					return *this;
				}

				RedVector3& RedVector3::operator+=( const RedVector3& _v)
				{
					X += _v.X;
					Y += _v.Y;
					Z += _v.Z;
					return *this;
				}

				RedVector3& RedVector3::operator-=( const RedVector3& _v)
				{
					X -= _v.X;
					Y -= _v.Y;
					Z -= _v.Z;
					return *this;
				}

				RedVector3& RedVector3::operator*=( const RedVector3& _v)
				{
					X *= _v.X;
					Y *= _v.Y;
					Z *= _v.Z;
					return *this;
				}

				RedVector3& RedVector3::operator/=( const RedVector3& _v)
				{
					X /= _v.X;
					Y /= _v.Y;
					Z /= _v.Z;
					return *this;
				}

				bool RedVector3::operator==( const RedVector3& _v ) const
				{
					return ( X == _v.X ) && ( Y == _v.Y ) && ( Z == _v.Z );
				}

				bool RedVector3::operator!=( const RedVector3& _v ) const
				{
					return ( X != _v.X ) && ( Y != _v.Y ) && ( Z != _v.Z );
				}

				RedVector3& RedVector3::SetZeros()
				{
					X = 0.0f;
					Y = 0.0f;
					Z = 0.0f;
					return *this;
				}

				RedVector3& RedVector3::SetOnes()
				{
					X = 1.0f;
					Y = 1.0f;
					Z = 1.0f;
					return *this;
				}

				bool RedVector3::IsAlmostEqual( const RedVector3& _v, float _epsilon ) const
				{
					return ( Red::Math::NumericalUtils::Abs< float >( X - _v.X ) < _epsilon ) &&
						   ( Red::Math::NumericalUtils::Abs< float >( Y - _v.Y ) < _epsilon ) &&
						   ( Red::Math::NumericalUtils::Abs< float >( Z - _v.Z ) < _epsilon );
				}

				bool RedVector3::IsZero() const
				{
					return ( X == 0.0f && Y == 0.0f && Z == 0.0f );
				}

				bool RedVector3::IsAlmostZero( float _epsilon ) const
				{
					return  ( ( ( X >= 0.0f ) ? X : -X ) <= _epsilon ) &&
						( ( ( Y >= 0.0f ) ? Y : -Y ) <= _epsilon ) &&
						( ( ( Z >= 0.0f ) ? Z : -Z ) <= _epsilon );
				}

				float RedVector3::Length() const
				{
					return ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
				}

				float RedVector3::SquareLength() const
				{
					return ( X * X ) + ( Y * Y ) + ( Z * Z );
				}

				RedVector3& RedVector3::Normalize()
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
					if( length != 0.0f )
					{
						float unitLength = 1.0f / length;
						X *= unitLength;
						Y *= unitLength;
						Z *= unitLength;
					}
					return *this;
				}

				RedVector3 RedVector3::Normalized() const
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
					if( length == 0.0f )
					{
						return *this;
					}
					return RedVector3( X / length, Y / length, Z / length );
				}

				float RedVector3::Dot( const RedVector3& _v ) const
				{
					return ( X * _v.X ) + ( Y * _v.Y ) + ( Z * _v.Z );
				}

				RedVector3 RedVector3::Cross( const RedVector3& _v ) const
				{
					return RedVector3( Y * _v.Z - _v.Y * Z,
									   _v.X * Z - X * _v.Z,
									   X * _v.Y - _v.X * Y );
				}

				RedVector3 RedVector3::Cross( const RedVector3& _a, const RedVector3& _b )
				{
					return RedVector3( _a.Y * _b.Z - _b.Y * _a.Z,
								   _b.X * _a.Z - _a.X * _b.Z,
								   _a.X * _b.Y - _b.X * _a.Y );
				}
		
				void RedVector3::Set( float _x, float _y, float _z )
				{
					X = _x;
					Y = _y;
					Z = _z;
				}

				bool RedVector3::IsOk() const
				{
					return ( Red::Math::NumericalUtils::IsFinite( X ) && 
							 Red::Math::NumericalUtils::IsFinite( Y ) && 
							 Red::Math::NumericalUtils::IsFinite( Z ) );
				}

				// *** RedVector4
				RedVector4::RedVector4()
				{
				}

				RedVector4::RedVector4( const RedVector1& _v )
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( _v.W )
				{
				}

				RedVector4::RedVector4( const RedVector2& _v )
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( _v.W )
				{
				}

				RedVector4::RedVector4( const RedVector3& _v )
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( _v.W )
				{
				}

				RedVector4::RedVector4( const RedVector3& _v, float _w )
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( _w )
				{
				}

				RedVector4::RedVector4( const RedVector4& _v )
					: X( _v.X ), Y( _v.Y ), Z( _v.Z ), W( _v.W )
				{
				}

				RedVector4::RedVector4( float _f )
					:  X( _f ), Y( _f ), Z( _f ), W( _f )
				{
				}

				RedVector4::RedVector4( const float* _f )
					: X( _f[0] ), Y( _f[1] ), Z( _f[2] ), W( _f[3] )
				{
				}

				RedVector4::RedVector4( float _x, float _y, float _z, float _w )
					: X( _x ), Y( _y ), Z( _z ), W( _w )
				{
				}

				RedVector4::~RedVector4()
				{
				}


				RedVector4& RedVector4::operator=( const RedVector1& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = _v.W;
					return *this;
				}

				RedVector4& RedVector4::operator=( const RedVector2& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = _v.W;
					return *this;
				}


				RedVector4& RedVector4::operator=( const RedVector3& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = _v.W;
					return *this;
				}

				RedVector4& RedVector4::operator=( const RedVector4& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = _v.W;
					return *this;
				}

				RedVector4 RedVector4::operator+( float _f ) const
				{
					return RedVector4( X + _f, Y + _f, Z + _f, W + _f );
				}

				RedVector4 RedVector4::operator-( float _f ) const
				{
					return RedVector4( X - _f, Y - _f, Z - _f, W - _f );
				}

				RedVector4 RedVector4::operator*( float _f ) const
				{
					return RedVector4( X * _f, Y * _f, Z * _f, W * _f );
				}

				RedVector4 RedVector4::operator/( float _f ) const
				{
					return RedVector4( X / _f, Y / _f, Z / _f, W / _f );
				}
		
				RedVector4& RedVector4::operator+=( const float _f )
				{
					X += _f;
					Y += _f;
					Z += _f;
					W += _f;
					return *this;
				}

				RedVector4& RedVector4::operator-=( const float _f )
				{
					X -= _f;
					Y -= _f;
					Z -= _f;
					W -= _f;
					return *this;
				}

				RedVector4& RedVector4::operator*=( const float _f )
				{
					X *= _f;
					Y *= _f;
					Z *= _f;
					W *= _f;
					return *this;
				}

				RedVector4& RedVector4::operator/=( const float _f )
				{
					X /= _f;
					Y /= _f;
					Z /= _f;
					W /= _f;
					return *this;
				}

				RedVector4 RedVector4::operator+( const RedVector1& _v ) const
				{
					return RedVector4( X + _v.X, Y + _v.Y, Z + _v.Z, W + _v.W );
				}

				RedVector4 RedVector4::operator-( const RedVector1& _v ) const
				{
					return RedVector4( X - _v.X, Y - _v.Y, Z - _v.Z, W - _v.W );
				}

				RedVector4 RedVector4::operator*( const RedVector1& _v ) const
				{
					return RedVector4( X * _v.X, Y * _v.Y, Z * _v.Z, W * _v.W );
				}

				RedVector4 RedVector4::operator/( const RedVector1& _v ) const
				{
					return RedVector4( X / _v.X, Y / _v.Y, Z / _v.Z, W / _v.W );
				}

				RedVector4 RedVector4::operator+( const RedVector2& _v ) const
				{
					return RedVector4( X + _v.X, Y + _v.Y, Z, W );
				}

				RedVector4 RedVector4::operator-( const RedVector2& _v ) const
				{
					return RedVector4( X - _v.X, Y - _v.Y, Z, W );
				}

				RedVector4 RedVector4::operator*( const RedVector2& _v ) const
				{
					return RedVector4( X * _v.X, Y * _v.Y, Z, W );
				}

				RedVector4 RedVector4::operator/( const RedVector2& _v ) const
				{
					return RedVector4( X / _v.X, Y / _v.Y, Z, W );
				}

				RedVector4 RedVector4::operator+( const RedVector3& _v ) const
				{
					return RedVector4( X + _v.X, Y + _v.Y, Z + _v.Z, W );
				}

				RedVector4 RedVector4::operator-( const RedVector3& _v ) const
				{
					return RedVector4( X - _v.X, Y - _v.Y, Z - _v.Z, W );
				}

				RedVector4 RedVector4::operator*( const RedVector3& _v ) const
				{
					return RedVector4( X * _v.X, Y * _v.Y, Z * _v.Z, W );
				}

				RedVector4 RedVector4::operator/( const RedVector3& _v ) const
				{
					return RedVector4( X / _v.X, Y / _v.Y, Z / _v.Z, W );
				}

				RedVector4 RedVector4::operator+( const RedVector4& _v ) const
				{
					return RedVector4( X + _v.X, Y + _v.Y, Z + _v.Z, W + _v.W);
				}

				RedVector4 RedVector4::operator-( const RedVector4& _v ) const
				{
					return RedVector4( X - _v.X, Y - _v.Y, Z - _v.Z, W - _v.W);
				}

				RedVector4 RedVector4::operator*( const RedVector4& _v ) const
				{
					return RedVector4( X * _v.X, Y * _v.Y, Z * _v.Z, W * _v.W);
				}

				RedVector4 RedVector4::operator/( const RedVector4& _v ) const
				{
					return RedVector4( X / _v.X, Y / _v.Y, Z / _v.Z, W / _v.W);
				}

				RedVector4& RedVector4::operator+=( const RedVector1& _v )
				{
					X += _v.X;
					Y += _v.X;
					Z += _v.X;
					W += _v.X;
					return *this;
				}

				RedVector4& RedVector4::operator-=( const RedVector1& _v )
				{
					X -= _v.X;
					Y -= _v.X;
					Z -= _v.X;
					W -= _v.X;
					return *this;
				}

				RedVector4& RedVector4::operator*=( const RedVector1& _v )
				{
					X *= _v.X;
					Y *= _v.X;
					Z *= _v.X;
					W *= _v.X;
					return *this;
				}

				RedVector4& RedVector4::operator/=( const RedVector1& _v )
				{
					X /= _v.X;
					Y /= _v.X;
					Z /= _v.X;
					W /= _v.X;
					return *this;
				}

				RedVector4& RedVector4::operator+=( const RedVector2& _v )
				{
					X += _v.X;
					Y += _v.Y;
					return *this;			
				}

				RedVector4& RedVector4::operator-=( const RedVector2& _v )
				{
					X -= _v.X;
					Y -= _v.Y;
					return *this;	
				}

				RedVector4& RedVector4::operator*=( const RedVector2& _v )
				{
					X *= _v.X;
					Y *= _v.Y;
					return *this;	
				}

				RedVector4& RedVector4::operator/=( const RedVector2& _v )
				{
					X /= _v.X;
					Y /= _v.Y;
					return *this;	
				}

				RedVector4& RedVector4::operator+=( const RedVector3& _v )
				{
					X += _v.X;
					Y += _v.Y;
					Z += _v.Z;
					return *this;
				}

				RedVector4& RedVector4::operator-=( const RedVector3& _v )
				{
					X -= _v.X;
					Y -= _v.Y;
					Z -= _v.Z;
					return *this;
				}

				RedVector4& RedVector4::operator*=( const RedVector3& _v )
				{
					X *= _v.X;
					Y *= _v.Y;
					Z *= _v.Z;
					return *this;
				}

				RedVector4& RedVector4::operator/=( const RedVector3& _v )
				{
					X /= _v.X;
					Y /= _v.Y;
					Z /= _v.Z;
					return *this;			
				}

				RedVector4& RedVector4::operator+=( const RedVector4& _v )
				{
					X += _v.X;
					Y += _v.Y;
					Z += _v.Z;
					W += _v.W;
					return *this;
				}

				RedVector4& RedVector4::operator-=( const RedVector4& _v )
				{
					X -= _v.X;
					Y -= _v.Y;
					Z -= _v.Z;
					W -= _v.W;
					return *this;
				}

				RedVector4& RedVector4::operator*=( const RedVector4& _v )
				{
					X *= _v.X;
					Y *= _v.Y;
					Z *= _v.Z;
					W *= _v.W;
					return *this;
				}

				RedVector4& RedVector4::operator/=( const RedVector4& _v )
				{
					X /= _v.X;
					Y /= _v.Y;
					Z /= _v.Z;
					W /= _v.W;
					return *this;
				}

				bool RedVector4::operator==( const RedVector4& _v ) const
				{
					return ( X == _v.X ) && ( Y == _v.Y ) && ( Z == _v.Z ) && ( W == _v.W );
				}

				bool RedVector4::operator==( const RedVector3& _v ) const
				{
					return ( X == _v.X ) && ( Y == _v.Y ) && ( Z == _v.Z );
				}

				bool RedVector4::operator!=( const RedVector4& _v ) const
				{
					return ( X != _v.X ) && ( Y != _v.Y ) && ( Z != _v.Z ) && ( W != _v.W );
				}
		
				bool RedVector4::operator!=( const RedVector3& _v ) const
				{
					return ( X != _v.X ) && ( Y != _v.Y ) && ( Z != _v.Z );
				}

				bool RedVector4::IsAlmostEqual( const RedVector4& _v, float _epsilon ) const
				{
					return ( Red::Math::NumericalUtils::Abs< float >( X - _v.X ) < _epsilon ) &&
						   ( Red::Math::NumericalUtils::Abs< float >( Y - _v.Y ) < _epsilon ) &&
						   ( Red::Math::NumericalUtils::Abs< float >( Z - _v.Z ) < _epsilon ) &&
						   ( Red::Math::NumericalUtils::Abs< float >( W - _v.W ) < _epsilon );
				}


				RedVector4& RedVector4::Set( float _x )
				{
					X = _x;
					Y = _x;
					Z = _x;
					W = _x;
					return *this;
				}

				RedVector4& RedVector4::Set( const RedVector1& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = _v.W;
					return *this;
				}

				RedVector4& RedVector4::Set( const RedVector4& _v )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = _v.W;
					return *this;
				}
				RedVector4& RedVector4::Set( float _x, float _y, float _z, float _w )
				{
					X = _x;
					Y = _y;
					Z = _z;
					W = _w;
					return *this;
				}

				RedVector4& RedVector4::Set( const RedVector3& _v, float _w )
				{
					X = _v.X;
					Y = _v.Y;
					Z = _v.Z;
					W = _w;
					return *this;
				}

				RedVector4& RedVector4::SetTransformedPos( const RedQsTransform& _trans, const RedVector4& _v )
				{
					RedVector4 scaled;
					RedVector4 rotated; 					
					scaled = _v * _trans.Scale;
					rotated.RotateDirection( _trans.Rotation, scaled );
					X = rotated.X + _trans.Translation.X;
					Y = rotated.Y + _trans.Translation.Y;
					Z = rotated.Z + _trans.Translation.Z;
					W = rotated.W + _trans.Translation.W;
					return *this;
				}

				RedVector4& RedVector4::SetTransformedInversePos(const RedQsTransform& _t, const RedVector4& _v )
				{
					RedVector4 temp( _v );
					temp -= _t.Translation;
					RedMatrix3x3 tempMat;
					tempMat.BuildFromQuaternion( _t.Rotation.Quat );

					float v0 = temp.V[0];
					float v1 = temp.V[1];
					float v2 = temp.V[2];
					RedVector4& t = *this;
					t.V[0] = ( tempMat.Matrix[0].V[0] * v0 ) + ( tempMat.Matrix[1].V[0] * v1 ) + ( tempMat.Matrix[2].V[0] * v2 );
					t.V[1] = ( tempMat.Matrix[0].V[1] * v0 ) + ( tempMat.Matrix[1].V[1] * v1 ) + ( tempMat.Matrix[2].V[1] * v2 );
					t.V[2] = ( tempMat.Matrix[0].V[2] * v0 ) + ( tempMat.Matrix[1].V[2] * v1 ) + ( tempMat.Matrix[2].V[2] * v2 );
					t.V[3] = 0.0f;

					X *= 1.0f / _t.Scale.V[0];
					Y *= 1.0f / _t.Scale.V[1];
					Z *= 1.0f / _t.Scale.V[2];
					return *this;
				}

				RedVector4& RedVector4::SetZeros()
				{
					X = 0.0f;
					Y = 0.0f;
					Z = 0.0f;
					W = 0.0f;
					return *this;
				}

				RedVector4& RedVector4::SetOnes()
				{
					X = 1.0f;
					Y = 1.0f;
					Z = 1.0f;
					W = 1.0f;
					return *this;
				}

		
				RedVector4& RedVector4::Negate()
				{
					X = -X;
					Y = -Y;
					Z = -Z;
					W = -W;
					return *this;
				}

				RedVector4 RedVector4::Negated() const
				{
					return RedVector4( -X, -Y, -Z, -W );
				}

				RedVector4 RedVector4::Abs() const
				{
					return RedVector4( ( X >= 0.0f ) ? X : -X,
						( Y >= 0.0f ) ? Y : -Y,
						( Z >= 0.0f ) ? Z : -Z,
						( W >= 0.0f ) ? W : -W );
				}

				float RedVector4::Sum() const
				{
					return X + Y + Z + W;
				}

				float RedVector4::Length() const
				{
					return ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) + ( W * W ) );
				}

				float RedVector4::SquareLength() const
				{
					return ( X * X ) + ( Y * Y ) + ( Z * Z ) + ( W * W );
				}

				float RedVector4::Sum2() const
				{
					return X + Y;
				}

				float RedVector4::Length2() const
				{
					return ::sqrtf( ( X * X ) + ( Y * Y ) );
				}

				float RedVector4::SquareLength2() const
				{
					return ( X * X ) + ( Y * Y );
				}

				float RedVector4::Sum3() const
				{
					return X + Y + Z;
				}

				float RedVector4::Length3() const
				{
					return ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
				}

				float RedVector4::SquareLength3() const
				{
					return ( X * X ) + ( Y * Y ) + ( Z * Z );
				}

				float RedVector4::DistanceTo( const RedVector4& _t ) const
				{
					RedVector4 delta(X - _t.X, Y - _t.Y, Z - _t.Z, W - _t.W);
					return delta.Length3();
				}

				float RedVector4::DistanceSquaredTo( const RedVector4& _t ) const
				{
					RedVector4 delta(X - _t.X, Y - _t.Y, Z - _t.Z, W - _t.W);
					return delta.SquareLength3();
				}

				float RedVector4::DistanceTo2D( const RedVector4& _t ) const
				{
					RedVector2 delta( X - _t.X, Y - _t.Y );
					return delta.Length();
				}

				float RedVector4::DistanceSquaredTo2D( const RedVector4& _t ) const
				{
					RedVector2 delta( X - _t.X, Y - _t.Y );
					return delta.SquareLength();
				}
		
				float RedVector4::DistanceToEdge( const RedVector4& _a, const RedVector4& _b ) const
				{
					RedVector4 edge = (_b - _a).Normalized3();
					float ta = RedVector4::Dot3( edge, _a );
					float tb = RedVector4::Dot3( edge, _b );
					float p = RedVector4::Dot3( edge, *this );
					if ( p >= ta && p <= tb )
					{
						RedVector4 projected = _a + edge * (p-ta);
						return DistanceTo( projected );
					}
					else if ( p < ta )
					{
						return DistanceTo( _a );
					}
					else
					{
						return DistanceTo( _b );
					}
				}

				RedVector4 RedVector4::NearestPointOnEdge( const RedVector4& _a, const RedVector4& _b ) const
				{
					RedVector4 delta = _b - _a;
					float length = delta.Length();

					RedVector4 v( X - _a.X, Y - _a.Y, Z - _a.Z );

					float projection = RedVector4::Dot3( v, delta ) / length;

					if ( projection <= 0 )
					{
						return _a;
					}
					else if ( projection >= 1 )
					{
						return _b;
					}
					else
					{
						return RedVector4( _b * projection + _a * ( 1.0f - projection ) );
					}
				}

				RedVector4& RedVector4::Normalize()
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) + ( W * W ));
					if( length != 0.0f )
					{
						float unitLength = 1.0f / length;
						X *= unitLength;
						Y *= unitLength;
						Z *= unitLength;
						W *= unitLength;
					}
					return *this;
				}

				RedVector4& RedVector4::Normalize2()
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) );
					if( length != 0.0f )
					{
						float unitLength = 1.0f / length;
						X *= unitLength;
						Y *= unitLength;
					}
					return *this;
				}

				RedVector4& RedVector4::Normalize3()
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
					if( length != 0.0f )
					{
						float unitLength = 1.0f / length;
						X *= unitLength;
						Y *= unitLength;
						Z *= unitLength;
					}
					return *this;
				}

				RedVector4 RedVector4::Normalized() const
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) + ( W * W ) );
					if( length == 0.0f )
					{
						return *this;
					}
					float unitLength = 1.0f / length;
					return RedVector4( X * unitLength, Y * unitLength, Z * unitLength, W * unitLength );
				}

				RedVector4 RedVector4::Normalized2() const
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) );
					if( length == 0.0f )
					{
						return *this;
					}
					float unitLength = 1.0f / length;
					return RedVector4( X * unitLength, Y * unitLength, Z , W );
				}

				RedVector4 RedVector4::Normalized3() const
				{
					float length = ::sqrtf( ( X * X ) + ( Y * Y ) + ( Z * Z ) );
					if( length == 0.0f )
					{
						return *this;
					}
					float unitLength = 1.0f / length;
					return RedVector4( X * unitLength, Y * unitLength, Z * unitLength, W );
				}
		
				bool RedVector4::IsNormalized( float _epsilon ) const
				{
					float length = SquareLength();
					length -= 1.0f;
					length = ( length >= 0.0f ) ? length : -length;
					return ( length < _epsilon );
				}

				bool RedVector4::IsNormalized2( float _epsilon ) const
				{
					float length = SquareLength2();
					length -= 1.0f;
					length = ( length >= 0.0f ) ? length : -length;
					return ( length < _epsilon );
				}

				bool RedVector4::IsNormalized3( float _epsilon ) const
				{
					float length = SquareLength3();
					length -= 1.0f;
					length = ( length >= 0.0f ) ? length : -length;
					return ( length < _epsilon );
				}

				RedVector4 RedVector4::Min( const RedVector4& _a, const RedVector4& _b )
				{
					return RedVector4( NumericalUtils::Min( _a.X, _b.X ), NumericalUtils::Min( _a.Y, _b.Y ), NumericalUtils::Min( _a.Z, _b.Z ), NumericalUtils::Min( _a.W, _b.W ) );
				}

				RedVector4 RedVector4::Max( const RedVector4& _a, const RedVector4& _b )
				{
					return RedVector4( NumericalUtils::Max( _a.X, _b.X ), NumericalUtils::Max( _a.Y, _b.Y ), NumericalUtils::Max( _a.Z, _b.Z ), NumericalUtils::Max( _a.W, _b.W ) );
				}
		
				RedVector4 RedVector4::Clamp( const RedVector4& _a, float _Min, float _Max )
				{
					RedVector4 vMin = RedVector4( _Min, _Min, _Min, _Min );
					RedVector4 vMax = RedVector4( _Max, _Max, _Max, _Max );
					RedVector4 result = Max( _a, vMin );
					result = Min( result, vMax );
					return result;
				}
		
				float RedVector4::Upper3() const
				{
					return NumericalUtils::Max(X, NumericalUtils::Max( Y, Z ));
				}

				float RedVector4::Upper4() const
				{
					return NumericalUtils::Max( NumericalUtils::Max( X, Y ), NumericalUtils::Max( Z, W ) );
				}

				float RedVector4::Lower3() const
				{
					return NumericalUtils::Min(X, NumericalUtils::Min( Y, Z ));
				}

				float RedVector4::Lower4() const
				{
					return NumericalUtils::Min( NumericalUtils::Min( X, Y ), NumericalUtils::Min( Z, W ) );
				}

				RedVector4 RedVector4::ZeroElement( unsigned int _i ) const
				{
					RedVector4 result = *this;
					result.V[_i] = 0.0f;
					return result;
				}

				bool RedVector4::Equal( const RedVector4& _a, const RedVector2& _b )
				{
					return (_a.X == _b.X) && (_a.Y == _b.Y);
				}

				bool RedVector4::Equal( const RedVector4& _a, const RedVector3& _b )
				{
					return (_a.X == _b.X) && (_a.Y == _b.Y) && (_a.Z == _b.Z);
				}

				bool RedVector4::Equal( const RedVector4& _a, const RedVector4& _b )
				{
					return (_a.X == _b.X) && (_a.Y == _b.Y) && (_a.Z == _b.Z) && (_a.W == _b.W);
				}

				bool RedVector4::Near( const RedVector4& _a, const RedVector2& _b, float _epsilon )
				{
					return ( NumericalUtils::Abs( _a.X - _b.X ) < _epsilon &&
						NumericalUtils::Abs( _a.Y - _b.Y ) < _epsilon &&
						NumericalUtils::Abs( _a.Z - _b.Z ) < _epsilon &&
						NumericalUtils::Abs( _a.W - _b.W ) < _epsilon );
				}

				bool RedVector4::Near( const RedVector4& _a, const RedVector3& _b, float _epsilon )
				{
					return ( NumericalUtils::Abs( _a.X - _b.X ) < _epsilon &&
						NumericalUtils::Abs( _a.Y - _b.Y ) < _epsilon &&
						NumericalUtils::Abs( _a.Z - _b.Z ) < _epsilon );
				}

				bool RedVector4::Near( const RedVector4& _a, const RedVector4& _b, float _epsilon )
				{
					return ( NumericalUtils::Abs( _a.X - _b.X ) < _epsilon &&
						NumericalUtils::Abs( _a.Y - _b.Y ) < _epsilon );
				}

				float RedVector4::Dot( const RedVector4& _a, const RedVector2& _b )
				{
					return ( ( _a.X * _b.X ) + ( _a.Y * _b.Y ) );
				}

				float RedVector4::Dot( const RedVector4& _a, const RedVector3& _b )
				{
					return ( _a.X * _b.X ) + ( _a.Y * _b.Y ) + ( _a.Z * _b.Z );
				}

				float RedVector4::Dot( const RedVector4& _a, const RedVector4& _b )
				{
					return ( _a.X * _b.X ) + ( _a.Y * _b.Y ) + ( _a.Z * _b.Z ) + ( _a.W * _b.W );
				}

				float RedVector4::Dot2( const RedVector4& _a, const RedVector4& _b )
				{
					return ( ( _a.X * _b.X ) + ( _a.Y * _b.Y ) );
				}

				float RedVector4::Dot3( const RedVector4& _a, const RedVector4& _b )
				{
					return ( _a.X * _b.X ) + ( _a.Y * _b.Y ) + ( _a.Z * _b.Z );
				}

				RedVector4 RedVector4::Cross( const RedVector4& _a, const RedVector4& _b, float w )
				{
					return RedVector4(
						_a.Y * _b.Z - _a.Z * _b.Y, 
						_a.Z * _b.X - _a.X * _b.Z,
						_a.X * _b.Y - _a.Y * _b.X,
						w
						);
				}

				float RedVector4::Cross2( const RedVector4& _a, const RedVector4& _b )
				{
					return _a.X * _b.Y - _b.X * _a.Y;
				}

				RedVector4 RedVector4::Permute( const RedVector4& _a, const RedVector4& _b, unsigned int _x, unsigned int _y, unsigned int _z, unsigned int _w)
				{
					return RedVector4(
						_x ? _b.X : _a.X,
						_y ? _b.Y : _a.Y,
						_z ? _b.Z : _a.Z,
						_w ? _b.W : _a.W
					);
				}

				const RedVector1& RedVector4::AsVector1() const
				{
					return reinterpret_cast< const RedVector1& >( *this );
				}

				RedVector1& RedVector4::AsVector1()
				{
					return reinterpret_cast< RedVector1& >( *this );
				}

				const RedVector2& RedVector4::AsVector2() const
				{
					return reinterpret_cast< const RedVector2& >( *this );
				}

				RedVector2& RedVector4::AsVector2()
				{
					return reinterpret_cast< RedVector2& >( *this );
				}

				const RedVector3& RedVector4::AsVector3() const
				{
					return reinterpret_cast< const RedVector3& >( *this );
				}

				RedVector3& RedVector4::AsVector3()
				{
					return reinterpret_cast< RedVector3& >( *this );
				}

				RedVector4 RedVector4::Position()
				{
					return *this;
				}

				const RedVector4 RedVector4::Position() const
				{
					return *this;
				}

				RedVector4 RedVector4::Lerp( const RedVector4& _a, const RedVector4& _b, const float _weight )
				{
					const float s = 1.0f - _weight;

					return RedVector4(
						s * _a.X + _weight * _b.X,
						s * _a.Y + _weight * _b.Y,
						s * _a.Z + _weight * _b.Z,
						s * _a.W + _weight * _b.W );
				}

				void RedVector4::Lerp( const RedVector4& _a, const float _weight )
				{
					const float s = 1.0f - _weight;

					X = s * X + _weight * _a.X;
					Y = s * Y + _weight * _a.Y;
					Z = s * Z + _weight * _a.Z;
					W = s * W + _weight * _a.W;
				}

				void RedVector4::RotateDirection( const RedQuaternion& _quat, const RedVector4& _direction )
				{
					RedVector1 qreal( _quat.Quat.V[3] );
					RedVector1 q2minus1( qreal * qreal - 0.5f );

					RedVector4 ret;
					ret = _direction * q2minus1;
					
					RedVector1 imagDotDir = RedVector4::Dot3(_quat.GetImaginary(), _direction );
					ret += _quat.GetImaginary() * imagDotDir; 

					RedVector4 imagCrossDir = RedVector4::Cross( _quat.GetImaginary(), _direction );
					ret +=  imagCrossDir * qreal;

					ret += ret;
					Set(ret);
				}

				void RedVector4::AxisRotateVector( RedVector4& vec, const RedVector4& normAxis, float angle )
				{
					RedVector4 redAxis;
					redAxis.Set( normAxis.X, normAxis.Y, normAxis.Z, 0.0f );
					RedQuaternion rotQuat;
					rotQuat.SetAxisAngle( redAxis, angle );

					RedVector4 redVec;
					RedVector4 redRotated;

					redVec.Set( vec.X, vec.Y, vec.Z, 0.0f );
					redRotated.RotateDirection( rotQuat, redVec );
					vec = redRotated;
				}

				void RedVector4::InverseRotateDirection( const RedMatrix3x3& _m, const RedVector4& _v )
				{
					float v0 = _v.V[0];
					float v1 = _v.V[1];
					float v2 = _v.V[2];

					V[0] = _m.Matrix[0].V[0] * v0 + _m.Matrix[1].V[0] * v1 + _m.Matrix[2].V[0] * v2;
					V[1] = _m.Matrix[0].V[1] * v0 + _m.Matrix[1].V[1] * v1 + _m.Matrix[2].V[1] * v2;
					V[2] = _m.Matrix[0].V[2] * v0 + _m.Matrix[1].V[2] * v1 + _m.Matrix[2].V[2] * v2;
					V[3] = 0;
				}

				void RedVector4::InverseRotateDirection( const RedQuaternion& _quat, const RedVector4& _v )
				{
					RedVector1 qReal( _quat.Quat.V[3] );
					RedVector1 q2minus1( ( qReal * qReal ) - 0.5f );

					RedVector4 ret;
					ret = _v * q2minus1;

					RedVector1 imagDotDir( RedVector4::Dot3(_quat.GetImaginary(), _v ) );
					ret += _quat.GetImaginary() * imagDotDir;

					RedVector4 imagCrossDir;
					imagCrossDir = RedVector4::Cross( _v, _quat.GetImaginary() );
					ret += imagCrossDir * qReal;

					ret += ret;
					Set(ret);
				}

				RedVector4 RedVector4::Project( const RedVector4& _v, const RedVector4& _onNormal )
				{
					float num = RedVector4::Dot(_onNormal, _onNormal);
					if( num < FLT_EPSILON )
					{
						return RedVector4::ZERO_3D_POINT;
					}

					return ( _onNormal * RedVector4::Dot3(_v, _onNormal ) ) / num;
				}

				bool RedVector4::IsOk() const
				{
					return ( Red::Math::NumericalUtils::IsFinite( X ) && 
							 Red::Math::NumericalUtils::IsFinite( Y ) && 
							 Red::Math::NumericalUtils::IsFinite( Z ) &&
							 Red::Math::NumericalUtils::IsFinite( W ) );
				}

				unsigned int RedVector4::GetConvertedToUByte4Color() const
				{
					// This should actually make use of a clamp function - it's here for compilation and completeness.
					unsigned char red = (unsigned char) NumericalUtils::Min( NumericalUtils::Max(  V[0] * 255.0f, 0.0f ), 255.0f );
					unsigned char green = (unsigned char) NumericalUtils::Min( NumericalUtils::Max(  V[1] * 255.0f, 0.0f ), 255.0f );
					unsigned char blue = (unsigned char) NumericalUtils::Min( NumericalUtils::Max(  V[2] * 255.0f, 0.0f ), 255.0f );
					unsigned char alpha = (unsigned char) NumericalUtils::Min( NumericalUtils::Max(  V[3] * 255.0f, 0.0f ), 255.0f );
					return ( alpha << 24 ) | ( red << 16 ) | ( green << 8 ) | blue;
				}

				void RedVector4::CalculatePerpendicularVector( const RedVector4& _in, RedVector4& _out )
				{
					int min = 0;
					int eleA = 1;
					int eleB = 2;

					float vA = MAbs( _in.V[0] );
					float vB = MAbs( _in.V[1] );
					float vC = MAbs( _in.V[2] );

					if( vB < vA )
					{
						eleA = 0;
						min = 1;
						vA = vB;
					}

					if( vC < vA )
					{
						eleB = min;
						min = 2;
					}

					_out.SetZeros();
					_out.V[eleA] = _in.V[eleB];
					_out.V[eleB] = _in.V[eleA];
				}
		};
	};
};