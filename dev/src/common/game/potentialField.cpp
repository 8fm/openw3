#include "build.h"
#include "potentialField.h"
#include "../engine/renderer.h"
#include "../engine/renderFragment.h"


IMPLEMENT_ENGINE_CLASS( IPotentialField );

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCircularPotentialField );

CCircularPotentialField::CCircularPotentialField()
: m_radius( 0 )
, m_rangeTop( 1000.0f )
, m_rangeBottom( -1000.0f )
, m_solid( true )
{
}

Bool CCircularPotentialField::RangeTest( const Vector& pos, Float strengthMultiplier ) const
{
	Vector dir = pos - m_origin;
	Float dist = dir.Mag2();

	if ( dist < m_radius )
	{
		if( dir.Z <= m_rangeTop && dir.Z >= m_rangeBottom )
		{
			return true;
		}
	}		

	return false;
}

Float CCircularPotentialField::GetPotentialValue( const Vector& pos, Float strengthMultiplier ) const 
{ 
	Vector dir = pos - m_origin;
	Float dist = dir.Mag2();

	if ( m_radius > 0 && dist < m_radius )
	{
		if( dir.Z <= m_rangeTop && dir.Z >= m_rangeBottom )
		{
			if ( m_solid )
			{
				return true;
			}
			else
			{
				return ( m_radius - dist ) / m_radius;
			}
		}
	}
	return 0.0f;
}

static IRenderResource* CreateDebugMesh( Float radius, Float bottom, Float top, const Color& color )
{
	TDynArray< DebugVertex > vertices;
	TDynArray< Uint32 > wireIndices;
	Uint32 sides = 16;

	// Generate vertices of the lower ring at z=0
	Uint32 lowerRing = vertices.Size();
	for ( Uint32 i=0; i<sides; i++ )
	{
		const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
		const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
		new ( vertices ) DebugVertex( Vector( c * radius, s * radius, bottom ), color );
	}

	// Generate vertices of the higher ring at z=height
	const Uint32 color32 = color.ToUint32();
	Uint32 higherRing = vertices.Size();
	for ( Uint32 i=0; i<sides; i++ )
	{
		const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
		const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
		new ( vertices ) DebugVertex( Vector( c * radius, s * radius, top ), color );
	}

	// Generate sides
	for ( Uint32 i=0; i<sides; i++ )
	{
		Uint32 a = lowerRing + i;
		Uint32 b = lowerRing + ((i+1) % sides);
		Uint32 c = higherRing + ((i+1) % sides);
		Uint32 d = higherRing + i;

		// Wire indices uses only lines
		wireIndices.PushBack( a );
		wireIndices.PushBack( b );
		wireIndices.PushBack( c );
		wireIndices.PushBack( d );
		wireIndices.PushBack( a );
		wireIndices.PushBack( d );
		wireIndices.PushBack( b );
		wireIndices.PushBack( c );
	}

	// Generate upper cap vertices
	Uint32 upperCap = vertices.Size();
	for ( Uint32 i=0; i<sides; i++ )
	{
		const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
		const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
		new ( vertices ) DebugVertex( Vector( c * radius, s * radius, top ), color );
	}

	// Generate lower cap vertices
	Uint32 lowerCap = vertices.Size();
	for ( Uint32 i=0; i<sides; i++ )
	{
		const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
		const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
		new ( vertices ) DebugVertex( Vector( c * radius, s * radius, bottom ), color );
	}

	return GRender->UploadDebugMesh( vertices, wireIndices );
}

void CCircularPotentialField::OnGenerateDebugFragments( CRenderFrame* frame, const Matrix& localToWorld )
{
	static IRenderResource* mesh = CreateDebugMesh( 1.0f, 0.0f, 1.0f, Color::GREEN );

	Matrix mtx = localToWorld;
	mtx.SetScale33( Vector( m_radius, m_radius, 1 ) );
	new ( frame ) CRenderFragmentDebugWireMesh( frame, mtx, mesh );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CSoundPotentialField );

CSoundPotentialField::CSoundPotentialField()
	: m_radius( 30.0f )
{
}

Bool CSoundPotentialField::RangeTest( const Vector& pos, Float strengthMultiplier ) const
{
	Vector dir = pos - Vector::ZEROS;
	Float dist = dir.Mag3();

	if ( dist < m_radius )
	{
		return CalcFieldStrength( strengthMultiplier, dist ) > 0.5f;
	}		

	return false;
}

Float CSoundPotentialField::GetPotentialValue( const Vector& pos, Float strengthMultiplier ) const 
{ 
	Vector dir = pos - Vector::ZEROS;
	Float dist = dir.Mag3();

	if ( dist < m_radius )
	{
		return CalcFieldStrength( strengthMultiplier, dist );
	}	

	return 0.0f;
}

Float CSoundPotentialField::CalcFieldStrength( Float strengthMultiplier, Float distance ) const
{
	return strengthMultiplier * powf( 2.0f, -distance * 0.15f );
}