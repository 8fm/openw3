/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibConst.h"

class CPathLibSettings
{
	DECLARE_RTTI_STRUCT( CPathLibSettings );
protected:
	Uint32											m_agentCategoriesCount;
	union {
		Float										m_agentCategoriesPersonalSpaces[ PathLib::MAX_ACTOR_CATEGORIES ];
		struct 
		{
			Float									m_agentCategoryRadius1;
			Float									m_agentCategoryRadius2;
			Float									m_agentCategoryRadius3;
			Float									m_agentCategoryRadius4;
		};
	};
	Float											m_roadsNavcostMultiplier;
	Float											m_maxTerrainSlope;
	Float											m_seaLevel;
	Float											m_desiredStreamingRange;

	Float											m_terrainWalkableRegionMinSize;
	Float											m_terrainUnwalkableRegionMinSize;
	Float											m_terrainNavmeshSurroundedRegionMinSize;

	Float											m_terrainHeightApproximationRange;

public:
	CPathLibSettings();																// load defaults

	Uint32		GetCategoriesCount() const											{ return m_agentCategoriesCount; }
	Float		GetCategoryPersonalSpace( Uint32 category ) const					{ return m_agentCategoriesPersonalSpaces[ category ]; }

	Float		GetMaxTerrainSlope() const											{ return m_maxTerrainSlope; }
	Float		GetSeaLevel() const													{ return m_seaLevel; }
	Float		GetDesiredStreamingRange() const									{ return m_desiredStreamingRange; }

	Float		GetTerrainWalkableRegionMinSize() const								{ return m_terrainWalkableRegionMinSize; }
	Float		GetTerrainUnwalkableRegionMinSize() const							{ return m_terrainUnwalkableRegionMinSize; }
	Float		GetTerrainNavmeshSurroundedRegionMinSize() const					{ return m_terrainNavmeshSurroundedRegionMinSize; }

	Float		GetTerrainHeightApproximationRange() const							{ return m_terrainHeightApproximationRange; }

	Uint32		ComputePSCategory( Float personalSpace ) const;
};

BEGIN_CLASS_RTTI( CPathLibSettings )
	PROPERTY_EDIT_RANGE( m_agentCategoriesCount, TXT("How many agent categories we use in game"), 0.f, Float(PathLib::MAX_ACTOR_CATEGORIES) )
	PROPERTY_EDIT( m_agentCategoryRadius1, TXT("Personal spaces of agent category 1") )
	PROPERTY_EDIT( m_agentCategoryRadius2, TXT("Personal spaces of agent category 2") )
	PROPERTY_EDIT( m_agentCategoryRadius3, TXT("Personal spaces of agent category 3") )
	PROPERTY_EDIT( m_agentCategoryRadius4, TXT("Personal spaces of agent category 4") )
	PROPERTY_EDIT( m_roadsNavcostMultiplier, TXT("Global navigation cost multiplier for roads") )
	PROPERTY_EDIT_RANGE( m_maxTerrainSlope, TXT("Maximal slope for terrain"), 30.f, 80.f )
	PROPERTY_EDIT( m_seaLevel, TXT("Sea level (any terrain below is unwalkable)") )
	PROPERTY_EDIT( m_desiredStreamingRange, TXT("Range at which we want navigation data to have all navdata streamed in") )
	PROPERTY_EDIT( m_terrainWalkableRegionMinSize, TXT("Terrain walkable region minimal size (in m^2)") )
	PROPERTY_EDIT( m_terrainUnwalkableRegionMinSize, TXT("Terrain un-walkable region minimal size (in m^2)") )
	PROPERTY_EDIT( m_terrainNavmeshSurroundedRegionMinSize, TXT("Terrain un-walkable region minimal size (in m^2)") )
	PROPERTY_EDIT( m_terrainHeightApproximationRange, TXT("Height approximation max inaccuracy. Lesser accuracy - moar problems, specially with npc movement when there is no physics loaed. Moar accuracy - larger .navtile data."))
END_CLASS_RTTI()

