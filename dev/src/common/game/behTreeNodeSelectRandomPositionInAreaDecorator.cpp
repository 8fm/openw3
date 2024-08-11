/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeSelectRandomPositionInAreaDecorator.h"

#include "../engine/clipMap.h"
#include "../engine/globalWater.h"

#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaDefinition )


///////////////////////////////////////////////////////////////////////////////
// SRandomPointInAreaAlgorithm
///////////////////////////////////////////////////////////////////////////////
RED_INLINE void SRandomPointInConvexAreaAlgorithm::GetTriangle( const CAreaComponent::TAreaPoints& points, Uint32 triangle, Vector2* verts )
{
	verts[ 0 ].Set( points[ 0 ].X, points[ 0 ].Y );
	verts[ 1 ].Set( points[ triangle+1 ].X, points[ triangle+1 ].Y );
	verts[ 2 ].Set( points[ triangle+2 ].X, points[ triangle+2 ].Y );

}
SRandomPointInConvexAreaAlgorithm::SRandomPointInConvexAreaAlgorithm()
{
}
SRandomPointInConvexAreaAlgorithm::~SRandomPointInConvexAreaAlgorithm()
{

}

Bool SRandomPointInConvexAreaAlgorithm::PrecomputeForArea( CAreaComponent* area )
{
	// check if we have already a precomputed data
	if ( m_area.Get() == area )
	{
		return true;
	}

	const CAreaComponent::TAreaPoints& worldPoints = area->GetWorldPoints();

	if ( worldPoints.Size() < 3 )
	{
		return false;
	}

	Uint32 triCount = worldPoints.Size() - 2;
	m_triangleAreas.ResizeFast( triCount );

	// compute triangle areas and its overal sum
	Float sumArea = 0.f;
	for ( Uint32 i = 0; i < triCount; ++i )
	{
		Vector2 verts[ 3 ];
		GetTriangle( worldPoints, i, verts );
		Float triangleArea = Abs( MathUtils::GeometryUtils::TriangleArea2D( verts[ 0 ], verts[ 1 ], verts[ 2 ] ) );
		sumArea += triangleArea;
		m_triangleAreas[ i ] = triangleArea;
	}

	if ( sumArea == 0.f )
	{
		return false;
	}

	// normalize all triangle areas so they all have sum of 1 (so we doesn't have to store it)
	Float mult = 1.f / sumArea;
	for ( Uint32 i = 0; i < triCount; ++i )
	{
		m_triangleAreas[ i ] *= mult;
	}
	
	m_area = area;

	return true;
}
void SRandomPointInConvexAreaAlgorithm::ComputeRandomPoint( CAreaComponent* area, Vector2& outPoint )
{
	Float r = GEngine->GetRandomNumberGenerator().Get< Float >();

	const CAreaComponent::TAreaPoints& worldPoints = area->GetWorldPoints();
	Uint32 triCount = worldPoints.Size() - 2;
	ASSERT( triCount == m_triangleAreas.Size() );
	Uint32 t = 0;
	for ( ; t < triCount-1; ++t )
	{
		r -= m_triangleAreas[ t ];
		if ( r<= 0.f )
		{
			break;
		}
	}

	Vector2 verts[ 3 ];
	GetTriangle( worldPoints, t, verts );

	auto& generator = GEngine->GetRandomNumberGenerator();
	Float u = generator.Get< Float >();
	Float v = generator.Get< Float >();

	if ( u + v > 1.f )
	{
		u = 1.f - u;
		v = 1.f - v;
	}

	outPoint = verts[ 0 ] + ( verts[ 1 ] - verts[ 0 ] ) * u + ( verts[ 2 ] - verts[ 0 ] ) * v;
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance::CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super ( def, owner, context, parent )
	, m_positionPtr( owner )
{
	def.m_areaSelection.InitInstance( owner, context, m_areaSelection );
}

Bool CBehTreeNodeDecoratorSelectRandomTerrainPositionInAreaInstance::Activate()
{
	CAreaComponent* area = m_areaSelection.GetArea();
	if ( !area )
	{
		return false;
	}

	if ( !m_pointSelectionAlgorithm.PrecomputeForArea( area ) )
	{
		return false;
	}

	CWorld* world = GGame->GetActiveWorld();

	const CClipMap* clipMap = world->GetTerrain();
	if ( !clipMap )
	{
		return false;
	}
	CGlobalWater* water = world->GetGlobalWater();

	const Box& areaBBox = area->GetBoundingBox();

	const Uint32 MAX_ITERATIONS = 6;
	Vector selectedSpot = Vector::ZEROS;
	for ( Uint32 i = 0; i < MAX_ITERATIONS; ++i )
	{
		m_pointSelectionAlgorithm.ComputeRandomPoint( area, selectedSpot.AsVector2() );

		Float height;
		if ( !clipMap->GetHeightForWorldPosition( selectedSpot, height ) )
		{
			continue;
		}
		if ( water )
		{
			Float waterHeight = water->GetWaterLevelBasic( selectedSpot.X, selectedSpot.Y );
			height = Max( waterHeight, height );
		}
		
		if ( height < areaBBox.Min.Z || height > areaBBox.Max.Z )
		{
			continue;
		}

		selectedSpot.Z = height;

		m_positionPtr->SetTarget( selectedSpot );

		return Super::Activate();
	}

	return false;
}