/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "math.h"
#include "object.h"
#include "../redMath/redmathbase.h"
#include "mathUtils.h"

IMPLEMENT_RTTI_CLASS( Vector );
IMPLEMENT_RTTI_CLASS( Vector3 );
IMPLEMENT_RTTI_CLASS( Vector2 );
IMPLEMENT_RTTI_CLASS( Matrix );
IMPLEMENT_RTTI_CLASS( EulerAngles );
IMPLEMENT_RTTI_CLASS( Color );
IMPLEMENT_RTTI_CLASS( Rect );
IMPLEMENT_RTTI_CLASS( Sphere );
IMPLEMENT_RTTI_CLASS( Plane );

IMPLEMENT_RTTI_CLASS( ConvexHull );
IMPLEMENT_RTTI_CLASS( OrientedBox );
IMPLEMENT_RTTI_CLASS( Box );
IMPLEMENT_RTTI_CLASS( AACylinder );
IMPLEMENT_RTTI_CLASS( Cylinder );
IMPLEMENT_RTTI_CLASS( Tetrahedron );
IMPLEMENT_RTTI_CLASS( CutCone );
IMPLEMENT_RTTI_CLASS( FixedCapsule );
IMPLEMENT_RTTI_CLASS( Segment );
IMPLEMENT_RTTI_CLASS( Quad );

const Vector Vector::ZEROS( 0,0,0,0 );
const Vector Vector::ZERO_3D_POINT( 0,0,0,1);
const Vector Vector::ONES( 1,1,1,1 );
const Vector Vector::EX( 1,0,0,0 );
const Vector Vector::EY( 0,1,0,0 );
const Vector Vector::EZ( 0,0,1,0 );
const Vector Vector::EW( 0,0,0,1 );

const Matrix Matrix::ZEROS( Vector::ZEROS, Vector::ZEROS, Vector::ZEROS, Vector::ZEROS );
const Matrix Matrix::IDENTITY( Vector::EX, Vector::EY, Vector::EZ, Vector::EW );

static const Matrix IDENTITY;

const Vector3 Vector3::ZEROS( 0,0,0 );

const Color Color::BLACK( 0, 0, 0 );
const Color Color::WHITE( 255, 255, 255 );
const Color Color::RED( 255, 0, 0 );
const Color Color::GREEN( 0, 255, 0 );
const Color Color::BLUE( 0, 0, 255 );
const Color Color::YELLOW( 255, 255, 0 );
const Color Color::CYAN( 0, 255, 255 );
const Color Color::MAGENTA( 255, 0, 255 );
const Color Color::LIGHT_RED( 255, 127, 127 );
const Color Color::LIGHT_GREEN( 127, 255, 127 );
const Color Color::LIGHT_BLUE( 127, 127, 255 );
const Color Color::LIGHT_YELLOW( 255, 255, 127 );
const Color Color::LIGHT_CYAN( 127, 255, 255 );
const Color Color::LIGHT_MAGENTA( 255, 127, 255 );
const Color Color::DARK_RED( 127, 0, 0 );
const Color Color::DARK_GREEN( 0, 127, 0 );
const Color Color::DARK_BLUE( 0, 0, 127 );
const Color Color::DARK_YELLOW( 127, 127, 0 );
const Color Color::DARK_CYAN( 0, 127, 127 );
const Color Color::DARK_MAGENTA( 127, 0, 127 );
const Color Color::BROWN( 139, 69, 19 );

const Color Color::GRAY( 127, 127, 127 );
const Color Color::NORMAL( 127, 127, 255 );

const Color Color::CLEAR( 0, 0, 0, 0 );

const EulerAngles EulerAngles::ZEROS(0,0,0);

const RectF RectF::EMPTY( 0.f,0.f,0.f,0.f);
const Rect Rect::EMPTY( Rect::RESET_STATE );

RED_INLINE Float GetOXAngle( const Vector& p )
{
	return atan2( p.Y, p.X ) * 180.0f / 3.14159265f + 180.0f;
}

RED_INLINE void SwitchValues( Vector& v1, Vector& v2 )
{
	Vector temp;
	temp = v2;
	v2 = v1;
	v1 = temp;
}

void Compute2DConvexHull( TDynArray<Vector>& points )
{
	// Find the rightmost point with the lowest y coordinate
	Uint32 minY = 0;
	for ( Uint32 i=0; i<points.Size(); ++i )
	{
		if ( points[i].Y < points[minY].Y )
		{
			minY = i;
		}
		else if  ( points[i].Y == points[minY].Y && points[i].X > points[minY].X )
		{
			minY = i;
		}
	}

	// add it to the end so we will see if we are finished collecting convex hull
	Vector minYPoint = points[ minY ];
	points.PushBack( minYPoint );

	const Uint32 N = points.Size()-1;

	// package wrapping algorithm
	Uint32 min = minY;
	Float lastAngle;
	Float currentMinAngle = -1.0f;
	for ( Uint32 i=0; i<points.Size()-1; ++i )
	{
		lastAngle = currentMinAngle;
		currentMinAngle = 360.0f;
		SwitchValues( points[min], points[i] );
		if ( i > 0 && points[i].Y == points[0].Y ) 
		{
			min = N;
		}
		else for ( Uint32 j=i+1; j<points.Size(); ++j )
		{
			if( Abs( points[j].X - points[i].X ) < NumericLimits<Float>::Epsilon() && Abs( points[j].Y - points[i].Y ) < NumericLimits<Float>::Epsilon() )
			{
				continue;
			}
			Vector offsetPos = points[j] - points[i];
			offsetPos.X *= -1.0f;
			const Float angle = ( 360.0f - GetOXAngle( offsetPos ) ); 
			if ( ( ( angle < currentMinAngle ) || ( MAbs( angle - currentMinAngle ) < 0.01f ) ) && angle > lastAngle )
			{
				currentMinAngle = angle;
				min = j;
			}
		}
		if ( min == N )
		{
			points.Resize( i+1 );
			return;
		}
	}
}


Bool IsPointInsideConvexShape( const Vector & point, const TDynArray< Vector > & shape )
{
	ASSERT( shape.Size() > 2 );

	Uint32   lastVert  = shape.Size() - 1;
	Vector lastCross = Vector::Cross( shape[0]-shape[lastVert], point-shape[lastVert] );

	for ( Uint32 i = 0; i < lastVert; ++i )
	{
		const Vector & p0 = shape[ i ];
		const Vector & p1 = shape[ i + 1 ];

		Vector cross = Vector::Cross( p1-p0, point-p0 );

		if ( Vector::Dot3( cross, lastCross ) < 0 )
			return false;
	}

	return true;
}

namespace Red
{
	namespace Math
	{
		namespace Checks
		{
			RED_INLINE Bool IsFinite( Float x )
			{
				const Float inf = 1e8;
				return Red::Math::NumericalUtils::IsFinite(x) && (x > -inf) && (x < inf);
			}

#ifdef _DEBUG
			void ValidateQsTransform( const RedQsTransform& transform )
			{
				ASSERT(IsFinite(transform.Translation.X));
				ASSERT(IsFinite(transform.Translation.Y));
				ASSERT(IsFinite(transform.Translation.Z));
				ASSERT(IsFinite(transform.Rotation.Quat.X));
				ASSERT(IsFinite(transform.Rotation.Quat.Y));
				ASSERT(IsFinite(transform.Rotation.Quat.Z));
				ASSERT(IsFinite(transform.Rotation.Quat.W));
				ASSERT(IsFinite(transform.Scale.X));
				ASSERT(IsFinite(transform.Scale.Y));
				ASSERT(IsFinite(transform.Scale.Z));
				ASSERT(transform.Rotation.IsOk());
				RED_UNUSED( transform );
			}
#endif
		}
	}
}

// Generic implementation of the standard Box2 types
Box2 Box2::ZERO(0.0f, 0.0f, 0.0f, 0.0f);
Box2 Box2::IDENTITY(1.0f, 1.0f, 1.0f, 1.0f);


/** Implementation based on http://graphics.cs.kuleuven.be/publications/LD05ERQIT/LD05ERQIT.pdf */
Bool Quad::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const
{
	Vector e1 = m_points[1] - m_points[0];
	Vector e3 = m_points[3] - m_points[0];

	Vector t = origin - m_points[0];

	Vector p = Vector::Cross( direction, e3 );
	Float det = Vector::Dot3( e1, p );
	if ( Abs( det ) < NumericLimits<Float>::Epsilon() ) return false;
	Float alpha = Vector::Dot3( t, p ) / det;
	if ( alpha <= 0.0f ) return false;
	if ( alpha >= 1.0f ) return false;

	Vector q = Vector::Cross( t, e1 );
	Float beta = Vector::Dot3( direction, q ) / det;
	if ( beta <= 0.0f ) return false;
	if ( beta >= 1.0f ) return false;

	if ( alpha + beta > 1 )
	{
		Vector er1 = m_points[1] - m_points[2];
		Vector er3 = m_points[3] - m_points[2];
		Vector pp = Vector::Cross( direction, er1 );
		Float detp = Vector::Dot3( er3, pp );
		if ( Abs( detp ) < NumericLimits<Float>::Epsilon() ) return false;
		Vector tp = origin - m_points[2];
		Float alphap = Vector::Dot3( tp, pp ) / detp;
		if ( alphap < 0 ) return false;
		Vector qp = Vector::Cross( tp, er3 );
		Float betap = Vector::Dot3( direction, qp ) / detp;
		if ( betap < 0 ) return false;
	}

	enterDistFromOrigin = Vector::Dot3( e3, q ) / det;
	if ( enterDistFromOrigin < 0 ) return false;

	return true;
}
Bool Quad::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}

Quad::Quad( const Quad& other )
{
	m_points[0] = other.m_points[0];
	m_points[1] = other.m_points[1];
	m_points[2] = other.m_points[2];
	m_points[3] = other.m_points[3];
	ASSERT( ( Vector::Cross( m_points[2] - m_points[1], m_points[0] - m_points[1] ) - Vector::Cross( m_points[3] - m_points[2], m_points[1] - m_points[2] ) ).SquareMag3() < NumericLimits<Float>::Epsilon() );
}
Quad::Quad( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4 )
{
	m_points[0] = p1;
	m_points[1] = p2;
	m_points[2] = p3;
	m_points[3] = p4;
	ASSERT( ( Vector::Cross( m_points[2] - m_points[1], m_points[0] - m_points[1] ) - Vector::Cross( m_points[3] - m_points[2], m_points[1] - m_points[2] ) ).SquareMag3() < NumericLimits<Float>::Epsilon() );
}

// Return translated by a vector
Quad Quad::operator+( const Vector& dir ) const
{
	Quad c( *this );
	c.m_points[0] += dir;
	c.m_points[1] += dir;
	c.m_points[2] += dir;
	c.m_points[3] += dir;
	return c;
}

// Return translated by a -vector
Quad Quad::operator-( const Vector& dir ) const
{
	Quad c( *this );
	c.m_points[0] -= dir;
	c.m_points[1] -= dir;
	c.m_points[2] -= dir;
	c.m_points[3] -= dir;
	return c;
}

// Translate by a vector
Quad& Quad::operator+=( const Vector& dir )
{
	m_points[0] += dir;
	m_points[1] += dir;
	m_points[2] += dir;
	m_points[3] += dir;
	return *this;
}

// Translate by a -vector
Quad& Quad::operator-=( const Vector& dir )
{
	m_points[0] -= dir;
	m_points[1] -= dir;
	m_points[2] -= dir;
	m_points[3] -= dir;
	return *this;
}

// Return translated by a vector
Segment Segment::operator+( const Vector& dir ) const
{
	Segment c( *this );
	c.m_origin += dir;
	return c;
}

// Return translated by a -vector
Segment Segment::operator-( const Vector& dir ) const
{
	Segment c( *this );
	c.m_origin -= dir;
	return c;
}

// Translate by a vector
Segment& Segment::operator+=( const Vector& dir )
{
	m_origin += dir;
	return *this;
}

// Translate by a -vector
Segment& Segment::operator-=( const Vector& dir )
{
	m_origin -= dir;
	return *this;
}

const Box Box::UNIT( Vector::ZEROS, Vector::ONES );
const Box Box::EMPTY = Box ().Clear();
void Tetrahedron::CalculatePlanes( Vector* planes ) const
{
	planes[0] = Vector::Cross( m_points[2] - m_points[0], m_points[1] - m_points[0] ).Normalized3();
	planes[0].W = -Vector::Dot3( m_points[0], planes[0] );

	planes[1] = Vector::Cross( m_points[3] - m_points[1], m_points[2] - m_points[1] ).Normalized3();
	planes[1].W = -Vector::Dot3( m_points[1], planes[1] );

	planes[2] = Vector::Cross( m_points[0] - m_points[2], m_points[3] - m_points[2] ).Normalized3();
	planes[2].W = -Vector::Dot3( m_points[2], planes[2] );

	planes[3] = Vector::Cross( m_points[1] - m_points[3], m_points[0] - m_points[3] ).Normalized3();
	planes[3].W = -Vector::Dot3( m_points[3], planes[3] );
}

Bool Tetrahedron::Contains( const Vector& point ) const
{
	Vector planes[4];
	CalculatePlanes( planes );
	for ( const Vector* it = planes; it != planes + 4; ++it )
	{
		if ( Vector::Dot3( point, *it ) + it->W >= 0 ) return false;
	}
	return true;
}
Bool Tetrahedron::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const
{
	Vector planes[4];
	CalculatePlanes( planes );
	enterDistFromOrigin = -NumericLimits<Float>::Infinity();
	for ( const Vector* it = planes; it != planes + 4; ++it )
	{
		const Vector& plane = *it;
		Float proj = -Vector::Dot3( plane, direction );
		if ( proj > 0.0f )
		{
			Float intersectionDistance = Vector::Dot3( origin, plane );
			intersectionDistance += plane.A[3];
			intersectionDistance /= proj;
			if ( intersectionDistance > enterDistFromOrigin ) enterDistFromOrigin = intersectionDistance;
		}
	}
	Vector enterPoint = origin + direction * enterDistFromOrigin;
	for ( const Vector* it = planes; it != planes + 4; ++it )
	{
		const Vector& plane = *it;
		Float dist = Vector::Dot3( enterPoint, plane );
		dist += plane.W;
		if ( dist > NumericLimits<Float>::Epsilon() ) return false;
	}
	return enterDistFromOrigin > 0 ;
}
Bool Tetrahedron::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}
// Return translated by a vector
Tetrahedron Tetrahedron::operator+( const Vector& dir ) const
{
	Tetrahedron c( *this );
	c.m_points[0] += dir;
	c.m_points[1] += dir;
	c.m_points[2] += dir;
	c.m_points[3] += dir;
	return c;
}

// Return translated by a -vector
Tetrahedron Tetrahedron::operator-( const Vector& dir ) const
{
	Tetrahedron c( *this );
	c.m_points[0] -= dir;
	c.m_points[1] -= dir;
	c.m_points[2] -= dir;
	c.m_points[3] -= dir;
	return c;
}

// Translate by a vector
Tetrahedron& Tetrahedron::operator+=( const Vector& dir )
{
	m_points[0] += dir;
	m_points[1] += dir;
	m_points[2] += dir;
	m_points[3] += dir;
	return *this;
}

// Translate by a -vector
Tetrahedron& Tetrahedron::operator-=( const Vector& dir )
{
	m_points[0] -= dir;
	m_points[1] -= dir;
	m_points[2] -= dir;
	m_points[3] -= dir;
	return *this;
}

CutCone::CutCone( const Vector& pos1, const Vector& pos2, Float radius1, Float radius2 )
	: m_positionAndRadius1( pos1 )
{
	Vector d = ( pos2 - pos1 );
	m_height = d.Mag3();
	m_normalAndRadius2 = d / m_height;
	m_positionAndRadius1.W = radius1;
	m_normalAndRadius2.W = radius2;
}
CutCone::CutCone( const Vector& pos, const Vector& normal, Float radius1, Float radius2, Float height )
	: m_height( height )
	, m_positionAndRadius1( pos )
	, m_normalAndRadius2( normal )
{
	m_positionAndRadius1.W = radius1;
	m_normalAndRadius2.W = radius2;
}
EulerAngles CutCone::GetOrientation() const
{
	return EulerAngles(
		RAD2DEG( MATan2( m_normalAndRadius2.X, -m_normalAndRadius2.Y ) ),
		RAD2DEG( MATan2( m_normalAndRadius2.Z, MSqrt( m_normalAndRadius2.X * m_normalAndRadius2.X + m_normalAndRadius2.Y * m_normalAndRadius2.Y ) ) ),
		0.0f );
}

// Return translated by a vector
CutCone CutCone::operator+( const Vector& dir ) const
{
	CutCone c( *this );
	c.m_positionAndRadius1 += dir;
	c.m_positionAndRadius1.W -= dir.W;
	return c;
}

// Return translated by a -vector
CutCone CutCone::operator-( const Vector& dir ) const
{
	CutCone c( *this );
	c.m_positionAndRadius1 -= dir;
	c.m_positionAndRadius1.W += dir.W;
	return c;
}

// Translate by a vector
CutCone& CutCone::operator+=( const Vector& dir )
{
	m_positionAndRadius1 += dir;
	m_positionAndRadius1.W -= dir.W;
	return *this;
}

// Translate by a -vector
CutCone& CutCone::operator-=( const Vector& dir )
{
	m_positionAndRadius1 -= dir;
	m_positionAndRadius1.W += dir.W;
	return *this;
}


// Return translated by a vector
AACylinder AACylinder::operator+( const Vector& dir ) const
{
	AACylinder c( *this );
	c.m_positionAndRadius += dir;
	c.m_positionAndRadius.W -= dir.W;
	return c;
}
// Return translated by a -vector
AACylinder AACylinder:: operator-( const Vector& dir ) const
{
	AACylinder c( *this );
	c.m_positionAndRadius -= dir;
	c.m_positionAndRadius.W += dir.W;
	return c;
}
// Translate by a vector
AACylinder& AACylinder::operator+=( const Vector& dir )
{
	m_positionAndRadius += dir;
	m_positionAndRadius.W -= dir.W;
	return *this;
}
// Translate by a -vector
AACylinder& AACylinder::operator-=( const Vector& dir )
{
	m_positionAndRadius -= dir;
	m_positionAndRadius.W += dir.W;
	return *this;
}

// Return translated by a vector
Cylinder Cylinder::operator+( const Vector& dir ) const
{
	Cylinder c( *this );
	c.m_positionAndRadius += dir;
	c.m_positionAndRadius.W -= dir.W;
	return c;
}

// Return translated by a -vector
Cylinder Cylinder::operator-( const Vector& dir ) const
{
	Cylinder c( *this );
	c.m_positionAndRadius -= dir;
	c.m_positionAndRadius.W += dir.W;
	return c;
}
EulerAngles Cylinder::GetOrientation() const
{
	return EulerAngles(
		RAD2DEG( MATan2( m_normalAndHeight.X, -m_normalAndHeight.Y ) ),
		RAD2DEG( MATan2( m_normalAndHeight.Z, MSqrt( m_normalAndHeight.X * m_normalAndHeight.X + m_normalAndHeight.Y * m_normalAndHeight.Y ) ) ),
		0.0f );
}

// Translate by a vector
Cylinder& Cylinder::operator+=( const Vector& dir )
{
	m_positionAndRadius += dir;
	m_positionAndRadius.W -= dir.W;
	return *this;
}

// Translate by a -vector
Cylinder& Cylinder::operator-=( const Vector& dir )
{
	m_positionAndRadius -= dir;
	m_positionAndRadius.W += dir.W;
	return *this;
}

// Return translated by a vector
ConvexHull ConvexHull::operator+( const Vector& dir ) const
{
	ConvexHull convex( *this );
	for ( TDynArray<Vector>::iterator it = convex.m_planes.Begin(); it != convex.m_planes.End(); ++it )
	{
		MovePlane( *it, dir );
	}
	return convex;
}

// Return translated by a -vector
ConvexHull ConvexHull::operator-( const Vector& dir ) const
{
	ConvexHull convex( *this );
	for ( TDynArray<Vector>::iterator it = convex.m_planes.Begin(); it != convex.m_planes.End(); ++it )
	{
		MovePlane( *it, -dir );
	}
	return convex;
}

// Translate by a vector
ConvexHull& ConvexHull::operator+=( const Vector& dir )
{
	for ( TDynArray<Vector>::iterator it = m_planes.Begin(); it != m_planes.End(); ++it )
	{
		MovePlane( *it, dir );
	}
	return *this;
}

// Translate by a -vector
ConvexHull& ConvexHull::operator-=( const Vector& dir )
{
	for ( TDynArray<Vector>::iterator it = m_planes.Begin(); it != m_planes.End(); ++it )
	{
		MovePlane( *it, -dir );
	}
	return *this;
}

// Return translated by a vector
OrientedBox OrientedBox::operator+( const Vector& dir ) const
{
	OrientedBox box( *this );
	box.m_position += dir;
	box.m_position.W -= dir.W;
	return box;
}

// Return translated by a -vector
OrientedBox OrientedBox::operator-( const Vector& dir ) const
{
	OrientedBox box( *this );
	box.m_position -= dir;
	box.m_position.W += dir.W;
	return box;
}

// Translate by a vector
OrientedBox& OrientedBox::operator+=( const Vector& dir )
{
	m_position += dir;
	m_position.W -= dir.W;
	return *this;
}

// Translate by a -vector
OrientedBox& OrientedBox::operator-=( const Vector& dir )
{
	m_position -= dir;
	m_position.W += dir.W;
	return *this;
}

Bool ConvexHull::Contains( const Vector& point ) const
{
	for ( TDynArray<Vector>::const_iterator it = m_planes.Begin(); it != m_planes.End(); ++it )
	{
		if ( Vector::Dot3( point, *it ) + it->W >= 0 ) return false;
	}
	return true;
}
Bool ConvexHull::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const
{
	enterDistFromOrigin = -NumericLimits<Float>::Infinity();
	for ( TDynArray<Vector>::const_iterator it = m_planes.Begin(); it != m_planes.End(); ++it )
	{
		const Vector& plane = *it;
		Float proj = -Vector::Dot3( plane, direction );
		if ( proj > 0.0f )
		{
			Float intersectionDistance = Vector::Dot3( origin, plane );
			intersectionDistance += plane.A[3];
			intersectionDistance /= proj;
			if ( intersectionDistance > enterDistFromOrigin ) enterDistFromOrigin = intersectionDistance;
		}
	}
	Vector enterPoint = origin + ( direction * enterDistFromOrigin );
	for ( TDynArray<Vector>::const_iterator it = m_planes.Begin(); it != m_planes.End(); ++it )
	{
		const Vector& plane = *it;
		Float dist = Vector::Dot3( enterPoint, plane );
		dist += plane.W;
		if ( dist > NumericLimits<Float>::Epsilon() ) return false;
	}
	return enterDistFromOrigin > 0 ;
}
Bool ConvexHull::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}
Bool OrientedBox::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const
{
	Float tmin = -NumericLimits<Float>::Infinity(), tmax = NumericLimits<Float>::Infinity();

	Vector dir( Vector::Dot3( direction, m_edge1 ), Vector::Dot3( direction, m_edge2 ), Vector::Dot3( direction, Vector::Cross( m_edge1, m_edge2 ) ) );
	Vector ori( Vector::Dot3( origin, m_edge1 ), Vector::Dot3( origin, m_edge2 ), Vector::Dot3( origin, Vector::Cross( m_edge1, m_edge2 ) ) );

	if ( dir.X != 0.0f ) {
		Float reverse = 1.0f / dir.X;
		Float tx1 = ( m_position.X - ori.X ) * reverse;
		Float tx2 = ( m_position.X + m_edge1.W - ori.X ) * reverse;

		tmin = Red::Math::NumericalUtils::Min(tx1, tx2);
		tmax = Red::Math::NumericalUtils::Max(tx1, tx2);
	} else {
		if ( ori.X <= m_position.X || ori.X >= m_position.X + m_edge1.W ) return false;
	}
	if ( dir.Y != 0.0f ) {
		Float reverse = 1.0f / dir.Y;
		Float tx1 = ( m_position.Y - ori.Y ) * reverse;
		Float tx2 = ( m_position.Y + m_edge1.W - ori.Y ) * reverse;

		tmin = Red::Math::NumericalUtils::Max( tmin, Red::Math::NumericalUtils::Min(tx1, tx2) );
		tmax = Red::Math::NumericalUtils::Min( tmax, Red::Math::NumericalUtils::Max(tx1, tx2) );
	} else {
		if ( ori.Y <= m_position.Y || ori.Y >= m_position.Y + m_edge1.W ) return false;
	}
	if ( dir.Z != 0.0f ) {
		Float reverse = 1.0f / dir.Z;
		Float tx1 = ( m_position.Z - ori.Z ) * reverse;
		Float tx2 = ( m_position.Z + m_position.W - ori.Z ) * reverse;

		tmin = Red::Math::NumericalUtils::Max( tmin, Red::Math::NumericalUtils::Min(tx1, tx2) );
		tmax = Red::Math::NumericalUtils::Min( tmax, Red::Math::NumericalUtils::Max(tx1, tx2) );
	} else {
		if ( ori.X <= m_position.X || ori.X >= m_position.X + m_edge1.W ) return false;
	}
	if ( tmax < 0 ) return false;
	if ( tmin < 0 ) tmin = 0;
	enterDistFromOrigin = tmin;

	return tmax > tmin;
}
Bool OrientedBox::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}

EulerAngles OrientedBox::GetOrientation() const
{
	EulerAngles result;
	Vector alignedLeft( Vector::Cross( m_edge1, Vector( 0, 0, 1.0f ) ).Normalized3() );
	Float rollCos = Vector::Dot3( alignedLeft, Vector::Cross( m_edge1, m_edge2 ) );
	result.Roll = - RAD2DEG( MAsin( rollCos ) );

	Vector alignedUp( Vector::Cross( m_edge2, Vector( 0, 1.0f, 0 ) ).Normalized3() );
	Float pitchCos = Vector::Dot3( alignedUp, m_edge1 );
	result.Pitch = RAD2DEG( MAsin( pitchCos ) );

	Vector alignedForward( Vector::Cross( Vector::Cross( m_edge1, m_edge2 ), Vector( 1.0f, 0, 0 ) ).Normalized3() );
	Float yawCos = Vector::Dot3( alignedForward, m_edge2 );
	result.Yaw = - RAD2DEG( MAsin( yawCos ) );
	return result;
}

// Check if bounding box contains point
Bool OrientedBox::Contains( const Vector& point ) const
{
	Float t;
	Float d;

	// using SAT to check if point is inside
	t = Vector::Dot3( point, m_edge1 );
	d = Vector::Dot3( point, m_position );
	if ( t <= d || t >= d + m_edge1.W ) return false;

	t = Vector::Dot3( point, m_edge2 );
	d = Vector::Dot3( point, m_position );
	if ( t <= d || t >= d + m_edge2.W ) return false;

	t = Vector::Dot3( point, Vector::Cross( m_edge1, m_edge2 ) );
	d = Vector::Dot3( point, m_position );
	if ( t <= d || t >= d + m_position.W ) return false;
	return true;
}

OrientedBox::OrientedBox( const OrientedBox& cyl )
{
	m_position = cyl.m_position;
	m_edge1 = cyl.m_edge1;
	m_edge2 = cyl.m_edge2;
}
OrientedBox::OrientedBox( const Vector& pos, const Vector& edge1, const Vector& edge2, const Vector& edge3 )
{
	m_position = pos;
	Float len = edge1.Mag3();
	m_edge1 = edge1 / len;
	m_edge1.W = len;
	len = edge2.Mag3();
	m_edge2 = edge2 / len;
	m_edge2.W = len;

	Vector v = Vector::Cross( m_edge1, m_edge2 );
	Float d = Vector::Dot3( edge3, v );
	m_position.W = d;
	//if ( d <= 0 )
	/*{
		m_position.W = - m_position.W;
	}*/

}

AACylinder::AACylinder( const AACylinder& cyl ) : m_positionAndRadius( cyl.m_positionAndRadius ) {}
AACylinder::AACylinder( const Vector& pos, Float radius, Float height ) : m_positionAndRadius( pos ), m_height( height ) { m_positionAndRadius.W = radius; }
Bool AACylinder::Contains( const Vector& point ) const
{
	return point.Z > m_positionAndRadius.Z && point.Z < m_positionAndRadius.Z + m_height && ( point - m_positionAndRadius ).SquareMag2() < m_positionAndRadius.W * m_positionAndRadius.W;
};
Bool AACylinder::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const
{
	Float t1;
	Float t2;
	Vector k = origin - m_positionAndRadius;
	if ( !MathUtils::SolveQuadraticEquation( Vector::Dot2( direction, direction ), 2 * Vector::Dot2( direction, k ), k.SquareMag2() - m_positionAndRadius.W * m_positionAndRadius.W, t1, t2 ) || t1 == t2 ) return false;
	Float min = Min( t1, t2 );
	Float max = Max( t1, t2 );
	if ( direction.Z != 0.0f ) {
		Float reverse = 1.0f / direction.Z;
		Float tx1 = ( m_positionAndRadius.Z - origin.Z ) * reverse;
		Float tx2 = ( m_positionAndRadius.Z + m_height - origin.Z ) * reverse;

		min = Max( min, Min( tx1, tx2 ) );
		max = Min( max, Max( tx1, tx2 ) );
	} else {
		if ( origin.Z <= m_positionAndRadius.Z ||  origin.Z >= m_positionAndRadius.Z + m_height ) return false;
	}
	if ( min >= max || max < 0 ) return false;
	if ( min < 0 ) min = 0;
	enterDistFromOrigin = min;
	return true;
}
Bool AACylinder::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}

Cylinder::Cylinder( const Cylinder& cyl ) : m_positionAndRadius( cyl.m_positionAndRadius), m_normalAndHeight( cyl.m_normalAndHeight ) {}
Cylinder::Cylinder( const Vector& pos, const Vector& normal, Float radius, Float height )
	: m_positionAndRadius( pos )
	, m_normalAndHeight( normal )
{
	m_positionAndRadius.W = radius;
	m_normalAndHeight.W = height;
}
Cylinder::Cylinder( const Vector& pos1, const Vector& pos2, Float radius)
	: m_positionAndRadius( pos1 )
{
	m_positionAndRadius.W = radius;
	Vector v = pos2 - pos1;
	m_normalAndHeight.W = v.Mag3();
	Float t = 1 / m_normalAndHeight.W;
	m_normalAndHeight.X = v.X * t;
	m_normalAndHeight.Y = v.Y *t;
	m_normalAndHeight.Z = v.Z *t;
}

Bool Cylinder::Contains( const Vector& point ) const
{
	Float d = Vector::Dot3( point, m_normalAndHeight );
	Float p = Vector::Dot3( m_positionAndRadius, m_normalAndHeight );
	if ( d <= p || d >= p + GetHeight() ) return false;
	Vector t = m_positionAndRadius - point;
	return ( t - m_normalAndHeight * Vector::Dot3( t, m_normalAndHeight ) ).SquareMag3() < m_positionAndRadius.W * m_positionAndRadius.W;
}
Bool Cylinder::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const
{
	Vector t1 = Vector::Cross( origin - m_positionAndRadius, m_normalAndHeight );
	Vector t2 = Vector::Cross( direction, m_normalAndHeight );

	Float x1;
	Float x2;
	if ( !MathUtils::SolveQuadraticEquation( Vector::Dot3( t2, t2 ), 2 * Vector::Dot3( t1, t2 ), Vector::Dot3( t1, t1 ) - m_positionAndRadius.W * m_positionAndRadius.W, x1, x2 ) || t1 == t2 ) return false;
	Float min = Min( x1, x2 );
	Float max = Max( x1, x2 );

	Float o = Vector::Dot3( m_positionAndRadius, m_normalAndHeight );
	Float s = Vector::Dot3( origin, m_normalAndHeight );
	Float v = Vector::Dot3( direction, m_normalAndHeight );
	if ( v == 0 )
	{
		if ( s <= o || s >= o + m_normalAndHeight.W ) return false;
	}
	else
	{
		Float reverse = 1.0f / v;
		x1 = ( o - s ) * reverse;
		x2 = ( o + m_normalAndHeight.W - s ) * reverse;
		min = Max( min, Min( x1, x2 ) );
		max = Min( max, Max( x1, x2 ) );
	}
	if ( min >= max || max < 0 ) return false;
	enterDistFromOrigin = min;
	return true;
}
Bool Cylinder::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}
Bool CutCone::Contains( const Vector& point ) const
{
	Float d = Vector::Dot3( point, m_normalAndRadius2 );
	Float p = Vector::Dot3( m_positionAndRadius1, m_normalAndRadius2 );
	Float m = d - p;
	if ( m <= 0 || m >= GetHeight() ) return false;
	Vector t = m_positionAndRadius1 - point;
	Float sd = ( t - m_normalAndRadius2 * Vector::Dot3( t, m_normalAndRadius2 ) ).SquareMag3();
	Float r = m_positionAndRadius1.W + ( m_normalAndRadius2.W - m_positionAndRadius1.W ) * m / GetHeight();
	return sd < r * r;
}
Bool CutCone::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin ) const
{
	Vector d = origin - m_positionAndRadius1;
	Vector t1 = Vector::Cross( d, m_normalAndRadius2 );
	Vector t2 = Vector::Cross( direction, m_normalAndRadius2 );
	Float g = ( GetRadius2() - GetRadius1() ) / m_height;
	Float t3 = GetRadius1() + g * Vector::Dot3( m_normalAndRadius2, d );
	Float t4 = g * Vector::Dot3( direction, m_normalAndRadius2 );

	Float x1;
	Float x2;
	if (
		!MathUtils::SolveQuadraticEquation(
		Vector::Dot3( t2, t2 ) - t4 * t4,
		2 * ( Vector::Dot3( t1, t2 ) - t3 * t4 ),
		Vector::Dot3( t1, t1 ) - t3 * t3,
		x1, x2
		) || t1 == t2 ) return false;
	Float min = Min( x1, x2 );
	Float max = Max( x1, x2 );

	Float o = Vector::Dot3( m_positionAndRadius1, m_normalAndRadius2 );
	Float s = Vector::Dot3( origin, m_normalAndRadius2 );
	Float v = Vector::Dot3( direction, m_normalAndRadius2 );
	if ( v == 0 )
	{
		if ( s <= o || s >= o + m_height ) return false;
	}
	else
	{
		Float reverse = 1.0f / v;
		x1 = ( o - s ) * reverse;
		x2 = ( o + m_height - s ) * reverse;
		min = Max( min, Min( x1, x2 ) );
		max = Min( max, Max( x1, x2 ) );
	}
	if ( min >= max || max < 0 ) return false;
	enterDistFromOrigin = min;
	return true;
}
Bool CutCone::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}

void ConvexHull::MovePlane( Vector& plane, const Vector& translation )
{
	plane.W = - Vector::Dot3( translation + ( plane * ( - plane.W ) ), plane );
}

Bool Quad::IntersectSegment( const Segment& segment, Vector& enterPoint )
{
	Float t;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t );
	if ( ret && t * t <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t;
		return true;
	}
	return false;
}
Bool Tetrahedron::IntersectSegment( const Segment& segment, Vector& enterPoint )
{
	Float t;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t );
	if ( ret && t * t <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t;
		return true;
	}
	return false;
}
Bool CutCone::IntersectSegment( const Segment& segment, Vector& enterPoint )
{
	Float t;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t );
	if ( ret && t * t <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t;
		return true;
	}
	return false;
}
Bool OrientedBox::IntersectSegment( const Segment& segment, Vector& enterPoint )
{
	Float t;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t );
	if ( ret && t * t <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t;
		return true;
	}
	return false;
}
Bool Cylinder::IntersectSegment( const Segment& segment, Vector& enterPoint )
{
	Float t;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t );
	if ( ret && t * t <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t;
		return true;
	}
	return false;
}
Bool AACylinder::IntersectSegment( const Segment& segment, Vector& enterPoint )
{
	Float t;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t );
	if ( ret && t * t <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t;
		return true;
	}
	return false;
}

void MatrixDouble::Import( const Matrix &src )
{
	for ( Uint32 i=0; i<4; ++i )
	{
		for ( Uint32 j=0; j<4; ++j )
		{
			V[i][j] = src.V[i].A[j];
		}
	}
}

void MatrixDouble::Export( Matrix &dest ) const
{
	for ( Uint32 i=0; i<4; ++i )
	{
		for ( Uint32 j=0; j<4; ++j )
		{
			dest.V[i].A[j] = (Float)V[i][j];
		}
	}
}

Double MatrixDouble::Det() const
{
	Double det = 0.0;
	det += V[0][0] * CoFactor(0,0);
	det += V[0][1] * CoFactor(0,1); 
	det += V[0][2] * CoFactor(0,2); 
	det += V[0][3] * CoFactor(0,3); 
	return det;
}

Double MatrixDouble::CoFactor( Int32 i, Int32 j ) const
{
#define M( dx, dy ) V[ (i+dx)&3 ][ (j+dy) & 3 ]
	Double val = 0.0;
	val += M(1,1) * M(2,2) * M(3,3);
	val += M(1,2) * M(2,3) * M(3,1);
	val += M(1,3) * M(2,1) * M(3,2);
	val -= M(3,1) * M(2,2) * M(1,3);
	val -= M(3,2) * M(2,3) * M(1,1);
	val -= M(3,3) * M(2,1) * M(1,2);
	val *= ((i+j) & 1) ? -1.0f : 1.0f;
	return val; 
#undef M
}

MatrixDouble MatrixDouble::FullInverted() const
{
	MatrixDouble out;

	// Get determinant
	Double d = Det();
	if ( MAbs((Float)d) > 1e-12f )
	{
		Double id = 1.0 / d;

		// Invert matrix
		out.V[0][0] = CoFactor(0,0) * id;
		out.V[0][1] = CoFactor(1,0) * id;
		out.V[0][2] = CoFactor(2,0) * id;
		out.V[0][3] = CoFactor(3,0) * id;
		out.V[1][0] = CoFactor(0,1) * id;
		out.V[1][1] = CoFactor(1,1) * id;
		out.V[1][2] = CoFactor(2,1) * id;
		out.V[1][3] = CoFactor(3,1) * id;
		out.V[2][0] = CoFactor(0,2) * id;
		out.V[2][1] = CoFactor(1,2) * id;
		out.V[2][2] = CoFactor(2,2) * id;
		out.V[2][3] = CoFactor(3,2) * id;
		out.V[3][0] = CoFactor(0,3) * id;
		out.V[3][1] = CoFactor(1,3) * id;
		out.V[3][2] = CoFactor(2,3) * id;
		out.V[3][3] = CoFactor(3,3) * id;
	}
	else
	{
		out.Import( Matrix::IDENTITY );
	}

	return out;
}

MatrixDouble MatrixDouble::operator*( const MatrixDouble& other ) const
{
	return Mul( other, *this );
}

MatrixDouble MatrixDouble::Mul( const MatrixDouble& a, const MatrixDouble& b )
{
	MatrixDouble ret;
	ret.V[0][0] = b.V[0][0] * a.V[0][0] + b.V[0][1] * a.V[1][0] + b.V[0][2] * a.V[2][0] + b.V[0][3] * a.V[3][0];
	ret.V[0][1] = b.V[0][0] * a.V[0][1] + b.V[0][1] * a.V[1][1] + b.V[0][2] * a.V[2][1] + b.V[0][3] * a.V[3][1];
	ret.V[0][2] = b.V[0][0] * a.V[0][2] + b.V[0][1] * a.V[1][2] + b.V[0][2] * a.V[2][2] + b.V[0][3] * a.V[3][2];
	ret.V[0][3] = b.V[0][0] * a.V[0][3] + b.V[0][1] * a.V[1][3] + b.V[0][2] * a.V[2][3] + b.V[0][3] * a.V[3][3];
	ret.V[1][0] = b.V[1][0] * a.V[0][0] + b.V[1][1] * a.V[1][0] + b.V[1][2] * a.V[2][0] + b.V[1][3] * a.V[3][0];
	ret.V[1][1] = b.V[1][0] * a.V[0][1] + b.V[1][1] * a.V[1][1] + b.V[1][2] * a.V[2][1] + b.V[1][3] * a.V[3][1];
	ret.V[1][2] = b.V[1][0] * a.V[0][2] + b.V[1][1] * a.V[1][2] + b.V[1][2] * a.V[2][2] + b.V[1][3] * a.V[3][2];
	ret.V[1][3] = b.V[1][0] * a.V[0][3] + b.V[1][1] * a.V[1][3] + b.V[1][2] * a.V[2][3] + b.V[1][3] * a.V[3][3];
	ret.V[2][0] = b.V[2][0] * a.V[0][0] + b.V[2][1] * a.V[1][0] + b.V[2][2] * a.V[2][0] + b.V[2][3] * a.V[3][0];
	ret.V[2][1] = b.V[2][0] * a.V[0][1] + b.V[2][1] * a.V[1][1] + b.V[2][2] * a.V[2][1] + b.V[2][3] * a.V[3][1];
	ret.V[2][2] = b.V[2][0] * a.V[0][2] + b.V[2][1] * a.V[1][2] + b.V[2][2] * a.V[2][2] + b.V[2][3] * a.V[3][2];
	ret.V[2][3] = b.V[2][0] * a.V[0][3] + b.V[2][1] * a.V[1][3] + b.V[2][2] * a.V[2][3] + b.V[2][3] * a.V[3][3];
	ret.V[3][0] = b.V[3][0] * a.V[0][0] + b.V[3][1] * a.V[1][0] + b.V[3][2] * a.V[2][0] + b.V[3][3] * a.V[3][0];
	ret.V[3][1] = b.V[3][0] * a.V[0][1] + b.V[3][1] * a.V[1][1] + b.V[3][2] * a.V[2][1] + b.V[3][3] * a.V[3][1];
	ret.V[3][2] = b.V[3][0] * a.V[0][2] + b.V[3][1] * a.V[1][2] + b.V[3][2] * a.V[2][2] + b.V[3][3] * a.V[3][2];
	ret.V[3][3] = b.V[3][0] * a.V[0][3] + b.V[3][1] * a.V[1][3] + b.V[3][2] * a.V[2][3] + b.V[3][3] * a.V[3][3];  
	return ret;
}