/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

Sphere::Sphere()
	: CenterRadius2( Vector::ZEROS )
{
}

Sphere::Sphere( const Vector& center, Float radius )
{
	CenterRadius2 = center;
	CenterRadius2.SetW( radius );
}

// Return translated by a vector
Sphere Sphere::operator+( const Vector& dir ) const
{
	return Sphere( CenterRadius2 + dir, CenterRadius2.W );
}

// Return translated by a -vector
Sphere Sphere::operator-( const Vector& dir ) const
{
	return Sphere( CenterRadius2 - dir, CenterRadius2.W );
}

// Translate by a vector
Sphere& Sphere::operator+=( const Vector& dir )
{
	CenterRadius2.X += dir.X;
	CenterRadius2.Y += dir.Y;
	CenterRadius2.Z += dir.Z;
	return *this;
}

// Translate by a -vector
Sphere& Sphere::operator-=( const Vector& dir )
{
	CenterRadius2.X -= dir.X;
	CenterRadius2.Y -= dir.Y;
	CenterRadius2.Z -= dir.Z;
	return *this;
}

const Vector& Sphere::GetCenter() const
{
	return CenterRadius2;
}

Float Sphere::GetSquareRadius() const
{
	return CenterRadius2.W * CenterRadius2.W;
}


Float Sphere::GetRadius() const
{
	return CenterRadius2.W;
}

Float Sphere::GetDistance( const Vector& point ) const
{
	return Vector::Sub3( CenterRadius2, point ).Mag3() - CenterRadius2.W;
}

Float Sphere::GetDistance( const Sphere& sphere ) const
{
	return GetDistance( sphere.GetCenter() ) - sphere.GetRadius();
}

Bool Sphere::Contains( const Vector& point ) const
{
	return Vector::Sub3( CenterRadius2, point ).SquareMag3() <= CenterRadius2.W * CenterRadius2.W;
}

Bool Sphere::Contains( const Sphere& sphere ) const
{
	// math: dist^2 <= (r1 + r2)^2 = r1^2 + 2*r1*r2 + r2^2
	Float distSq = Vector::Sub3( CenterRadius2, sphere.GetCenter() ).SquareMag3();
	Float r1 = CenterRadius2.W;
	Float r2 = sphere.CenterRadius2.W;
	Float rSq = r1 * ( r1 + 2.f*r2 ) + r2*r2;

	return distSq <= rSq;
}

Bool Sphere::Touches( const Sphere& sphere ) const
{
	return GetDistance(sphere) <= 0;
}

Int32 Sphere::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint, Vector& exitPoint ) const
{
	Float sqrDelta = 0.0f;

	Vector rayDir = direction.Normalized3();

	Float A = Vector::Dot3( rayDir, rayDir );
	Vector vec = origin - CenterRadius2;
	vec.W = 0.0f;

	Float B = 2.0f* Vector::Dot3( vec, rayDir );
	Float C = Vector::Dot3( vec, vec ) - CenterRadius2.W;

	const Float EPSILON = 1.0e-8f;

	//Check if there were an intersection
	if (A == 0 || ( sqrDelta = (B * B - 4 * A * C) ) < 0)
	{
		//there were no intersection
		return 0;
	}
	else
	{
		//There is intersection. We've got two results. 
		Float delta = MSqrt( sqrDelta );
		Float A2 = 2.0f * A;
		Float t1 = ( -B + delta ) / A2;
		Float t2 = ( -B - delta ) / A2;

		if (t1 > t2) 
		{
			Float t = t1; 
			t1 = t2; 
			t2 = t;
		}

		if ( t2 <= EPSILON ) 
			return 0;

		if (t1 > EPSILON && t2 > EPSILON )
		{
			enterPoint = rayDir * t1 + origin;
			enterPoint.W = 1.0f;
			exitPoint = rayDir * t2 + origin;
			exitPoint.W = 1.0f;

			return 2;
		}
		else
		{
			enterPoint = origin;
			enterPoint.W = 1.0f;

			if (t1 > EPSILON )
			{
				exitPoint = rayDir * t1 + origin;
				exitPoint.W = 1.0f;
				return 1;
			}

			if (t2 > EPSILON )
			{
				exitPoint = rayDir * t2 + origin;
				exitPoint.W = 1.0f;
				return 1;
			}
		}
	}

	return 0;
}

RED_INLINE Int32 Sphere::IntersectEdge( const Vector& a,  const Vector& b, Vector& intersectionPoint0, Vector& intersectionPoint1 ) const
{
	Vector enterPoint;
	Vector exitPoint;

	Int32 res = IntersectRay( a, b - a, enterPoint, exitPoint );
	
	Vector minVec( ::Min( a.X, b.X ), ::Min( a.Y, b.Y ), ::Min( a.Z, b.Z ) );
	Vector maxVec( ::Max( a.X, b.X ), ::Max( a.Y, b.Y ), ::Max( a.Z, b.Z ) );
	Box box( minVec, maxVec );

	if( res == 0 )
	{
		return 0;
	}
	else if( res == 1 )
	{
		Bool exitInside = box.Contains( exitPoint );
		if( exitInside )
		{
			intersectionPoint0 = exitPoint;
			return 1;
		}

		return 0;
	}
	else
	{
		Bool enterInside = box.Contains( enterPoint );
		Bool exitInside = box.Contains( exitPoint );

		if( enterInside && exitInside )
		{
			intersectionPoint0 = enterPoint;
			intersectionPoint1 = exitPoint;
			return 2;
		}
		else if( enterInside )
		{
			intersectionPoint0 = enterPoint;
			return 1;
		}
		else if( exitInside )
		{
			intersectionPoint0 =  exitPoint;
			return 1;
		}		
	}

	return 0;
};