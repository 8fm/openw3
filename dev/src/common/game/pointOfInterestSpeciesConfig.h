#pragma once

class CBoidLairParams;
class CPointOfInterestSpeciesConfig
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Boids );

public:
	CPointOfInterestSpeciesConfig();
	Bool ParseXML( const SCustomNode & POIConfigNode, const CBoidLairParams *const params );

	CName	m_type;
	Float	m_gravity;
	Bool	m_action;
	Uint32  m_boidStateIndex;
	Float	m_actionTimeOut;
	Float	m_wanderTimeOut;
	Float	m_timeVariation;
};

typedef TArrayMap< CName, CPointOfInterestSpeciesConfig * >  CPointOfInterestSpeciesConfig_Map;