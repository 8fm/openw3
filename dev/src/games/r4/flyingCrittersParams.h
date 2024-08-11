#pragma once

#include "../../common/game/swarmLairEntity.h"
#include "r4SwarmUtils.h"
#include "flyingPoiConfig.h"

class CR4BoidSpecies;


typedef TArrayMap< CName, const CPoiConfigByGroup* >	CPoiConfigByGroup_Map;
typedef TDynArray< CPoiConfigByGroup_Map >				CGroupStateToPoiConfig;

class CFlyingCritterLairParams : public CSwarmLairParams
{
public:
	Float					m_wallsDistance;
	Float					m_wallsRepulsion;
	Bool					m_livesUnderWater;
	Float					m_waterSurfaceCollisionForce;

	CGroupState_Array			m_groupStateArray;
	CFlyingPoiConfig_Map		m_flyingPoiConfigMap;
	CGroupStateToPoiConfig		m_groupStateToPoiConfig; // rearanged structure to make acces of data faster (from m_flyingPoiConfigMap)

	enum ETypes
	{
		TYPE_FLYING_CRITTER_LAIR_PARAMS	= TYPE_SWARM_LAIR_PARAMS | FLAG( 3 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_FLYING_CRITTER_LAIR_PARAMS, 
	};
	CFlyingCritterLairParams(Bool isValid = false );
	CFlyingCritterLairParams( const CFlyingCritterLairParams & params );
	virtual ~CFlyingCritterLairParams();

	Bool						ParseXmlAttribute(const SCustomNodeAttribute & att)override;
	Bool						ParseXmlNode( const SCustomNode & node, CBoidSpecies *const boidSpecies )override;
	Bool						OnParsePoiConfig( const SCustomNode & node, CBoidSpecies *const boidSpecies )override;
	Bool						ParseXML( const SCustomNode & paramsNode, CBoidSpecies *const boidSpecies)override;
	CSwarmSoundConfig *const	CreateSoundConfig()override;

	Uint32  GetGroupStateIndexFromName( CName stateName )const;

	static const CFlyingCritterLairParams sm_defaultParams;
private:
	Bool VirtualCopyTo(CBoidLairParams* const params)const override;
};

class CGroupState
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Boids );

public:
	CName  m_stateName;
	Bool   m_ignorePoiAfterReach;

	Float  m_cozyFlyDist;
	Float  m_maxVelocity;
	Float  m_minVelocity;
	Float  m_frictionMultiplier;
	Float  m_tooCloseMultiplier;
	Float  m_tooFarMultiplier;
	Float  m_randomDirMultiplier;
	Float  m_thinnessMultiplier;
	Float  m_collisionMultiplier;
	Float  m_randomNeighboursRatio;
	Float  m_randomDirRatio;
	Uint32 m_neighbourCount;
	Float  m_glidingPercentage;

	Float  m_velocityVariation;			// The higher the more the cloud will look like a string
	Float  m_tooCloseVariation;
	Float  m_tooFarVariation;			// In ring mode will make the ring thicker and smaller
	Float  m_genericVariation;

	CGroupState();
	Bool	ParseXML( const SCustomNode & poiStateNode, CR4BoidSpecies *const boidSpecies );
};
