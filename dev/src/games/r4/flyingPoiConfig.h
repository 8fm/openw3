#pragma once
#include "r4SwarmUtils.h"

class CFlyingCritterLairParams;

class CGravity
{
public:
	Float m_radius;
	Float m_gravity;

	CGravity( )
		: m_radius( 0.0f )
		, m_gravity( 0.0f )
	{
	}
	Bool	ParseXML( const SCustomNode & parentNode );
};
typedef TDynArray< CGravity >	CGravity_Array;

class CPoiConfigByGroup
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Boids );

public:
	Uint32			m_groupStateId;
	Bool			m_closestOnly;
	Float			m_gravity;
	CGravity_Array	m_gravityArray;

	CPoiConfigByGroup()
		: m_groupStateId( (Uint32)-1 ) 
		, m_closestOnly( false )
		, m_gravity( 0.0f )			{}

	Bool	ParseXML( const SCustomNode & parentNode, CFlyingCritterLairParams *const params);
	Bool	HasGravity()const { return m_gravity != 0.0f || m_gravityArray.Size() != 0; }
	Float	GetGravityFromDistance( Float distance, Float distanceMult )const;
};


class CFlyingPoiConfig 
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Boids );

public:
	CName							m_poiType;
	CPoiConfigByGroup_CPointerArray	m_poiConfigByGroupArray;
	Vector							m_positionOffest;

	CFlyingPoiConfig()
		: m_poiType( CName::NONE )
		, m_positionOffest( 0.0f, 0.0f, 0.0f )	{}
	Bool	ParseXML( const SCustomNode & poiConfigNode, CBoidSpecies *const boidSpecies, CFlyingCritterLairParams *const params);
	Uint32	GetPoiConfigByGroupIndex( Uint32 groupStateId )const;
};

typedef TArrayMap< CName, CFlyingPoiConfig * >	CFlyingPoiConfig_Map;