/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibRoughTerrainMap.h"

#include "../core/mathUtils.h"

#include "pathlibRoughtTerrainComponent.h"

namespace PathLib
{

CRoughtTerrainMap::CRoughtTerrainMap()
{

}
CRoughtTerrainMap::~CRoughtTerrainMap()
{

}

void CRoughtTerrainMap::Collect( CPathLibRoughtTerrainComponent* component )
{
	RoughtTerrainZone zone;

	zone.m_isRoughtTerrain = component->IsRoughtTerrain();
	zone.m_bbox = component->GetBoundingBox();
	const auto& verts = component->GetWorldPoints();
	zone.m_verts.Resize( verts.Size() );
	for ( Uint32 i = 0, n = verts.Size(); i < n; ++i )
	{
		zone.m_verts[ i ] = verts[ i ].AsVector2();
	}

	m_zones.PushBack( Move( zone ) );
}

CRoughtTerrainMap::ERoughtOverride CRoughtTerrainMap::IsLocationRought( const Vector& pos ) const
{
	for ( auto it = m_zones.Begin(), end = m_zones.End(); it != end; ++it )
	{
		const RoughtTerrainZone& zone = *it;
		if ( zone.m_bbox.Contains( pos ) )
		{
			if ( MathUtils::GeometryUtils::IsPointInPolygon2D( zone.m_verts, pos.AsVector2() ) )
			{
				return zone.m_isRoughtTerrain ? ROUGH : EASY;
			}
		}
	}
	return NOPE;
}
CRoughtTerrainMap::ERoughtOverride CRoughtTerrainMap::IsLineRought( const Vector& v0, const Vector& v1 ) const
{
	Box bbox( Box::RESET_STATE );
	bbox.AddPoint( v0 );
	bbox.AddPoint( v1 );
	for ( auto it = m_zones.Begin(), end = m_zones.End(); it != end; ++it )
	{
		const RoughtTerrainZone& zone = *it;
		if ( zone.m_bbox.Touches( bbox ) )
		{
			if ( MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( zone.m_verts, v0.AsVector2(), v1.AsVector2() ) )
			{
				return zone.m_isRoughtTerrain ? ROUGH : EASY;
			}
		}
	}
	return NOPE;
}

};				// namespace PathLib