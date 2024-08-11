/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibSettings.h"

IMPLEMENT_ENGINE_CLASS( CPathLibSettings );

CPathLibSettings::CPathLibSettings()
	: m_agentCategoriesCount( 3 )
	, m_roadsNavcostMultiplier( 0.25f )
	, m_maxTerrainSlope( 50.f )
	, m_seaLevel( -0.5f )
	, m_desiredStreamingRange( 256.f )
	, m_terrainWalkableRegionMinSize( 25.f )
	, m_terrainUnwalkableRegionMinSize( 0.5f )
	, m_terrainNavmeshSurroundedRegionMinSize( 10000.f ) // 100m2
	, m_terrainHeightApproximationRange( 0.5f )
{
	m_agentCategoriesPersonalSpaces[ 0 ] = 0.405f;
	m_agentCategoriesPersonalSpaces[ 1 ] = 1.01f;
	m_agentCategoriesPersonalSpaces[ 2 ] = 2.31f;
	m_agentCategoriesPersonalSpaces[ 3 ] = 4.00f;

}

Uint32 CPathLibSettings::ComputePSCategory( Float personalSpace ) const
{
	for ( Uint32 category = 0; category < m_agentCategoriesCount; ++category )
	{
		if ( personalSpace <= GetCategoryPersonalSpace( category ) )
		{
			return category;
		}
	}
	return m_agentCategoriesCount-1;
}