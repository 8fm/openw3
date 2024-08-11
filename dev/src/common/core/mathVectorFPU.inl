/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

Vector::Vector( const Vector& v )
	: X( v.X )
	, Y( v.Y )
	, Z( v.Z )
	, W( v.W )
{
}
Vector::Vector( const Float* f )
	: X( f[0] )
	, Y( f[1] )
	, Z( f[2] )
	, W( f[3] )
{
}

Vector::Vector( Float x, Float y, Float z, Float w )
	: X( x )
	, Y( y )
	, Z( z )
	, W( w )
{
}

Vector& Vector::Set3( Float x, Float y, Float z )
{
	X = x;
	Y = y;
	Z = z;
	return *this;
}

Vector& Vector::Set4( Float x, Float y, Float z, Float w )
{
	X = x;
	Y = y;
	Z = z;
	W = w;
	return *this;
}

Vector& Vector::Set3( const Float* value )
{
	X = value[0];
	Y = value[1];
	Z = value[2];
	return *this;
}

Vector& Vector::Set4( const Float* value )
{
	X = value[0];
	Y = value[1];
	Z = value[2];
	W = value[3];
	return *this;
}

Vector& Vector::Set3( Float value )
{
	X = value;
	Y = value;
	Z = value;
	return *this;
}

Vector& Vector::Set4( Float value )
{
	X = value;
	Y = value;
	Z = value;
	W = value;
	return *this;
}

Vector& Vector::Set3( const Vector& a )
{
	X = a.X;
	Y = a.Y;
	Z = a.Z;
	return *this;
}

Vector& Vector::Set4( const Vector& a )
{
	X = a.X;
	Y = a.Y;
	Z = a.Z;
	W = a.W;
	return *this;
}

Vector& Vector::SetX( Float value )
{
	X = value;
	return *this;
}

Vector& Vector::SetY( Float value )
{
	Y = value;
	return *this;
}

Vector& Vector::SetZ( Float value )
{
	Z = value;
	return *this;
}

Vector& Vector::SetW( Float value )
{
	W = value;
	return *this;
}

Vector& Vector::SetZeros()
{
	X = Y = Z = W = 0.0f;
	return *this;
}

Vector& Vector::SetOnes()
{
	X = Y = Z = W = 1.0f;
	return *this;
}

Vector& Vector::Negate()
{
	X = -X;
	Y = -Y;
	Z = -Z;
	W = -W;
	return *this;
}

Vector Vector::Negated() const
{
	return Vector( -X, -Y, -Z, -W );
}

Vector Vector::Abs() const
{
	return Vector( ::Abs( X ), ::Abs( Y ), ::Abs( Z ), ::Abs( W ) );
}

Float Vector::Sum3() const
{
	return X + Y + Z;
}

Float Vector::Sum4() const
{
	return X + Y + Z + W;
}

Float Vector::SquareMag2() const
{
	return X * X + Y * Y;
}

Float Vector::SquareMag3() const
{
	return X * X + Y * Y + Z * Z;
}

Float Vector::SquareMag4() const
{
	return X * X + Y * Y + Z * Z + W * W;
}

Float Vector::Mag2() const
{
	return MSqrt( X * X + Y * Y );
}

Float Vector::Mag3() const
{
	return MSqrt( X * X + Y * Y + Z * Z );
}

Float Vector::Mag4() const
{
	return MSqrt( X * X + Y * Y + Z * Z + W * W );
}

Float Vector::Normalize3()
{
	float len = Mag3();
	if ( len != 0 )
	{
		Div3( len );
	}
	return len;
}

Float Vector::Normalize2()
{
	float len = Mag2();
	if ( len != 0 )
	{
		Div3( len );
	}
	return len;
}

Float Vector::Normalize4()
{
	float len = Mag4();
	if ( len != 0 )
	{
		Div4( len );
	}
	return len;
}

Vector Vector::Normalized2() const
{
	float len = Mag2();
	if ( len == 0 )
	{
		return *this;
	}
	return Div3( *this, len );
}

Vector Vector::Normalized3() const
{
	float len = Mag3();
	if ( len == 0 )
	{
		return *this;
	}
	return Div3( *this, len );
}

Vector Vector::Normalized4() const
{
	float len = Mag4();
	if ( len == 0 )
	{
		return *this;
	}
	return Div4( *this, len );
}

Bool Vector::IsNormalized3( Float eps ) const
{
	const Float len = SquareMag3();
	return MAbs( len - 1.0f ) < eps;
}

Bool Vector::IsNormalized4( Float eps ) const
{
	const Float len = SquareMag4();
	return MAbs( len - 1.0f ) < eps;
}

Vector Vector::Min4( const Vector& a, const Vector& b )
{
	return Vector( Min( a.X, b.X ), Min( a.Y, b.Y ), Min( a.Z, b.Z ) , Min( a.W, b.W ) );
}

Vector Vector::Max4( const Vector& a, const Vector& b )
{
	return Vector( Max( a.X, b.X ), Max( a.Y, b.Y ), Max( a.Z, b.Z ) , Max( a.W, b.W ) );
}

Vector Vector::Clamp4( const Vector& a, Float min, Float max )
{
	Vector vmin = Vector( min, min, min, min );
	Vector vmax = Vector( max, max, max, max );
	Vector result = Max4( a, vmin );
	result = Min4( result, vmax );
	return result;
}

Float Vector::Upper3() const
{
	return Max( X, Max( Y, Z ) );
}

Float Vector::Upper4() const
{
	return Max( Max( X, Y ), Max( Z, W ) );
}

Float Vector::Lower3() const
{
	return Min( X, Min( Y, Z ) );
}

Float Vector::Lower4() const
{
	return Min( Min( X, Y ), Min( Z, W ) );
}

Vector Vector::Add4( const Vector& a, const Vector& b )
{
	return Vector( a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W );
}

Vector Vector::Sub4( const Vector& a, const Vector& b )
{
	return Vector( a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W - b.W );
}

Vector Vector::Sub3( const Vector& a, const Vector& b )
{
	return Vector( a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W );
}

Vector Vector::Mul4( const Vector& a, const Vector& b )
{
	return Vector( a.X * b.X, a.Y * b.Y, a.Z * b.Z, a.W * b.W );
}

Vector Vector::Div4( const Vector& a, const Vector& b )
{
	return Vector( a.X / b.X, a.Y / b.Y, a.Z / b.Z, a.W / b.W );
}

Vector Vector::Add3( const Vector& a, const Vector& b )
{
	return Vector( a.X + b.X, a.Y + b.Y, a.Z + b.Z );
}

Vector Vector::Mul3( const Vector& a, const Vector& b )
{
	return Vector( a.X * b.X, a.Y * b.Y, a.Z * b.Z );
}

Vector Vector::Div3( const Vector& a, const Vector& b )
{
	return Vector( a.X / b.X, a.Y / b.Y, a.Z / b.Z );
}



Vector Vector::Add4( const Vector& a, Float b )
{
	return Vector( a.X + b, a.Y + b, a.Z + b, a.W + b );
}

Vector Vector::Sub4( const Vector& a, Float b )
{
	return Vector( a.X - b, a.Y - b, a.Z - b, a.W - b );
}

Vector Vector::Mul4( const Vector& a, Float b )
{
	return Vector( a.X * b, a.Y * b, a.Z * b, a.W * b );
}

Vector Vector::Div4( const Vector& a, Float b )
{
	const Float invB = Float(1) / b;
	return Vector( a.X * invB, a.Y * invB, a.Z * invB, a.W * invB );
}

Vector Vector::Div3( const Vector& a, Float b )
{
	const Float invB = Float(1) / b;
	return Vector( a.X * invB, a.Y * invB, a.Z * invB );
}

Vector& Vector::Add4( const Vector& a )
{
	X += a.X;
	Y += a.Y;
	Z += a.Z;
	W += a.W;
	return *this;
}

Vector& Vector::Add3( const Vector& a )
{
	X += a.X;
	Y += a.Y;
	Z += a.Z;
	return *this;
}

Vector& Vector::Sub4( const Vector& a )
{
	X -= a.X;
	Y -= a.Y;
	Z -= a.Z;
	W -= a.W;
	return *this;
}

Vector& Vector::Sub3( const Vector& a )
{
	X -= a.X;
	Y -= a.Y;
	Z -= a.Z;
	return *this;
}

Vector& Vector::Mul4( const Vector& a )
{
	X *= a.X;
	Y *= a.Y;
	Z *= a.Z;
	W *= a.W;
	return *this;
}

Vector& Vector::Div4( const Vector& a )
{
	X /= a.X;
	Y /= a.Y;
	Z /= a.Z;
	W /= a.W;
	return *this;
}

Vector& Vector::Div3( const Vector& a )
{
	X /= a.X;
	Y /= a.Y;
	Z /= a.Z;
	return *this;
}

Vector& Vector::Add4( Float a )
{
	X += a;
	Y += a;
	Z += a;
	W += a;
	return *this;
}

Vector& Vector::Sub4( Float a )
{
	X -= a;
	Y -= a;
	Z -= a;
	W -= a;
	return *this;
}

Vector& Vector::Mul4( Float a )
{
	X *= a;
	Y *= a;
	Z *= a;
	W *= a;
	return *this;
}

Vector& Vector::Mul3( Float a )
{
	X *= a;
	Y *= a;
	Z *= a;
	return *this;
}

Vector& Vector::Div4( Float a )
{
	X /= a;
	Y /= a;
	Z /= a;
	W /= a;
	return *this;
}

Vector& Vector::Div3( Float a )
{
	X /= a;
	Y /= a;
	Z /= a;
	return *this;
}

Bool Vector::Equal2( const Vector& a, const Vector& b )
{
	return a.X == b.X && a.Y == b.Y;
}

Bool Vector::Equal3( const Vector& a, const Vector& b )
{
	return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
}

Bool Vector::Equal4( const Vector& a, const Vector& b )
{
	return a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
}

Bool Vector::Near2( const Vector& a, const Vector& b, Float eps/*=1e-3f*/ )
{
	return MAbs( a.X - b.X) < eps && MAbs( a.Y - b.Y ) < eps;
}

Bool Vector::Near3( const Vector& a, const Vector& b, Float eps/*=1e-3f*/ )
{
	return MAbs( a.X - b.X) < eps && MAbs( a.Y - b.Y ) < eps && MAbs( a.Z - b.Z ) < eps;
}

Bool Vector::Near4( const Vector& a, const Vector& b, Float eps/*=1e-3f*/ )
{
	return MAbs( a.X - b.X) < eps && MAbs( a.Y - b.Y ) < eps && MAbs( a.Z - b.Z ) < eps && MAbs( a.W - b.W ) < eps;
}

Float Vector::Dot2( const Vector& a, const Vector& b )
{
	return ( a.X * b.X ) + ( a.Y * b.Y );
}

Float Vector::Dot3( const Vector& a, const Vector& b )
{
	return ( a.X * b.X ) + ( a.Y * b.Y ) + ( a.Z * b.Z );
}

Float Vector::Dot4( const Vector& a, const Vector& b )
{
	return ( a.X * b.X ) + ( a.Y * b.Y ) + ( a.Z * b.Z ) + ( a.W * b.W );
}

Float Vector::Dot2( const Vector& b ) const
{
	return ( X * b.X ) + ( Y * b.Y );
}

Float Vector::Dot3( const Vector& b ) const
{
	return ( X * b.X ) + ( Y * b.Y ) + ( Z * b.Z );
}

Float Vector::Dot4( const Vector& b ) const
{
	return ( X * b.X ) + ( Y * b.Y ) + ( Z * b.Z ) + ( W * b.W );
}

Vector Vector::ZeroElement( Uint32 i ) const
{
	RED_ASSERT( i < 3 );
	Vector vec = *this;
	vec.A[ i ] = 0.f;
	return vec;
}

Float Vector::DistanceTo( const Vector& other ) const
{
	Vector delta = Sub4( *this, other );
	return delta.Mag3();
}

Float Vector::DistanceSquaredTo( const Vector& other ) const
{
	Vector delta = Sub4( *this, other );
	return delta.SquareMag3();
}

Float Vector::DistanceTo2D( const Vector& other ) const
{
	Vector delta = Sub4( *this, other );
	return delta.Mag2();
}

Float Vector::DistanceSquaredTo2D( const Vector& other ) const
{
	Vector2 delta = this->AsVector2() - other.AsVector2();
	return delta.SquareMag();
}

Float Vector::DistanceToEdge( const Vector& a, const Vector &b ) const
{
	Vector edge = (b - a).Normalized3();
	Float ta = Vector::Dot3( edge, a );
	Float tb = Vector::Dot3( edge, b );
	Float p = Vector::Dot3( edge, *this );
	if ( p >= ta && p <= tb )
	{
		Vector projected = a + edge * (p-ta);
		return DistanceTo( projected );
	}
	else if ( p < ta )
	{
		return DistanceTo( a );
	}
	else
	{
		return DistanceTo( b );
	}
}

Float Vector::DistanceToEdge2D( const Vector& a, const Vector &b ) const
{
	Vector edgeXY = b - a;

	// almost vertical edge
	if ( edgeXY.SquareMag2() < 0.0001f )
	{
		return DistanceTo2D( a );
	}

	const Vector edge = edgeXY.Normalized2();
	const Float ta = Vector::Dot2( edge, a );
	const Float tb = Vector::Dot2( edge, b );
	const Float p = Vector::Dot2( edge, *this );
	if ( p >= ta && p <= tb )
	{
		Vector projected = a + edge * (p-ta);
		return DistanceTo2D( projected );
	}
	else if ( p < ta )
	{
		return DistanceTo2D( a );
	}
	else
	{
		return DistanceTo2D( b );
	}
}

Vector Vector::NearestPointOnEdge(const Vector& a, const Vector& b) const
{
	Vector d = b - a;
	Float len = d.Normalize3();

	Vector v( X - a.X, Y - a.Y, Z - a.Z );

	Float proj = len != 0.0f? Vector::Dot3( v, d ) / len : 0.0f;

	if ( proj <= 0 )
	{
		return a;
	}
	else if ( proj >= 1 )
	{
		return b;
	}
	else
	{
		return Vector( b * proj + a * ( 1 - proj ) );
	}
}

Vector Vector::Cross( const Vector& a, const Vector& b, Float w/*=1.0f*/ )
{
	return Vector( 
		a.Y * b.Z - b.Y * a.Z, 
		b.X * a.Z - a.X * b.Z,
		a.X * b.Y - b.X * a.Y,
		w
		);		
}

Float Vector::Cross2( const Vector& a, const Vector& b )
{
	return a.X * b.Y - b.X * a.Y;
}

Vector Vector::Permute( const Vector& a, const Vector&b, Uint32 x, Uint32 y, Uint32 z, Uint32 w )
{
	return Vector(
		x ? b.X : a.X,
		y ? b.Y : a.Y,
		z ? b.Z : a.Z,
		w ? b.W : a.W
	);
}

Vector Vector::operator-() const
{
	return Vector::Negated();
}

Vector Vector::operator+( const Vector& a ) const
{
	return Vector::Add4( *this, a );
}

Vector Vector::operator-( const Vector& a ) const
{
	return Vector::Sub4( *this, a );
}

Vector Vector::operator*( const Vector& a ) const
{
	return Vector::Mul4( *this, a );
}

Vector Vector::operator/( const Vector& a ) const
{
	return Vector::Div4( *this, a );
}

Vector Vector::operator+( Float a ) const
{
	return Vector::Add4( *this, a );
}

Vector Vector::operator-( Float a ) const
{
	return Vector::Sub4( *this, a );
}

Vector Vector::operator*( Float a ) const
{
	return Vector::Mul4( *this, a );
}

Vector Vector::operator/( Float a ) const
{
	return Vector::Div4( *this, a );
}

Vector& Vector::operator=( const Vector& a )
{
	X = a.X;
	Y = a.Y;
	Z = a.Z;
	W = a.W;
	return *this;
}

Vector& Vector::operator+=( const Vector& a )
{
	Vector::Add4( a );
	return *this;
}

Vector& Vector::operator-=( const Vector& a )
{
	Vector::Sub4( a );
	return *this;
}

Vector& Vector::operator*=( const Vector& a )
{
	Vector::Mul4( a );
	return *this;
}

Vector& Vector::operator/=( const Vector& a )
{
	Vector::Div4( a );
	return *this;
}

Vector& Vector::operator+=( Float a )
{
	Vector::Add4( a );
	return *this;
}

Vector& Vector::operator-=( Float a )
{
	Vector::Sub4( a );
	return *this;
}

Vector& Vector::operator*=( Float a )
{
	Vector::Mul4( a );
	return *this;
}

Vector& Vector::operator/=( Float a )
{
	Vector::Div4( a );
	return *this;
}

Bool Vector::operator==( const Vector& a ) const
{
	return Vector::Equal4( *this, a );
}

Bool Vector::operator!=( const Vector& a ) const
{
	return !Vector::Equal4( *this, a );
}

const Vector2& Vector::AsVector2() const
{
	return reinterpret_cast< const Vector2& >( *this );
}
Vector2& Vector::AsVector2()
{
	return reinterpret_cast< Vector2& >( *this );
}
const Vector3& Vector::AsVector3() const
{
	return reinterpret_cast< const Vector3& >( *this );
}
Vector3& Vector::AsVector3()
{
	return reinterpret_cast< Vector3& >( *this );
}

Vector Vector::Interpolate( const Vector& a, const Vector& b, const Float weight )
{
	const Float s = 1.0f - weight;

	Vector out;
	out.X = s * a.X + weight * b.X;
	out.Y = s * a.Y + weight * b.Y;
	out.Z = s * a.Z + weight * b.Z;
	out.W = s * a.W + weight * b.W;
	
	return out;
}

void Vector::Interpolate( const Vector& a, const Float weight )
{
	const Float s = 1.0f - weight;

	X = s * X + weight * a.X;
	Y = s * Y + weight * a.Y;
	Z = s * Z + weight * a.Z;
	W = s * W + weight * a.W;
}

Vector Vector::Project( const Vector& vector, const Vector& onNormal )
{
	Float num = Dot3(onNormal, onNormal);

	if ( num < NumericLimits< Float >::Epsilon() )
	{
		return Vector::ZERO_3D_POINT;
	}

	return ( onNormal * Dot3( vector, onNormal ) ) / num;
}

Bool Vector::IsOk() const
{
	return IsFinite( X ) && IsFinite( Y ) && IsFinite( Z );
}
