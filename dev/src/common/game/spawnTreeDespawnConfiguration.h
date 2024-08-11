#pragma once

#include "encounterTypes.h"
#include "aiActionParameters.h"

struct SSpawnTreeDespawnConfiguration
{
	DECLARE_RTTI_STRUCT( SSpawnTreeDespawnConfiguration );

	SSpawnTreeDespawnConfiguration();

	Bool					m_canDespawnOnSight;
	Float					m_minDespawnRange;
	Float					m_maxDespawnRange;
	Float					m_forceDespawnRange;

	Float					m_despawnDelayMin;
	Float					m_despawnDelayMax;
	Float					m_despawnTime;
};

BEGIN_CLASS_RTTI( SSpawnTreeDespawnConfiguration )
	PROPERTY_EDIT( m_canDespawnOnSight, TXT("Can creature despawn while being visible.") )
	PROPERTY_EDIT( m_minDespawnRange, TXT("Minimal range from player for creature to despawn.") )
	PROPERTY_EDIT( m_forceDespawnRange, TXT("Range on which visibility limitation is not important") )
	PROPERTY_EDIT( m_despawnDelayMin, TXT("Minimal delay to start despawning countdown.") )
	PROPERTY_EDIT( m_despawnDelayMax, TXT("Maximal delay to start despawning countdown.") )
	PROPERTY_EDIT( m_despawnTime, TXT("Time countdown to despawn when visibility and range conditions are met.") )
END_CLASS_RTTI()

struct SSpawnTreeAIDespawnConfiguration : public SSpawnTreeDespawnConfiguration
{
	DECLARE_RTTI_STRUCT( SSpawnTreeAIDespawnConfiguration );

	SSpawnTreeAIDespawnConfiguration();
};

BEGIN_CLASS_RTTI( SSpawnTreeAIDespawnConfiguration )
	PROPERTY_EDIT( m_canDespawnOnSight, TXT("Can creature despawn witout AI while being visible.") )
	PROPERTY_EDIT( m_minDespawnRange, TXT("Minimal range from player for creature to despawn.") )
	PROPERTY_EDIT( m_maxDespawnRange, TXT("Maximal range from player for creature to despawn.") )
	PROPERTY_EDIT( m_forceDespawnRange, TXT("Range on which visibility limitation is not important") )
	PROPERTY_EDIT( m_despawnDelayMin, TXT("Minimal delay to start despawning countdown.") )
	PROPERTY_EDIT( m_despawnDelayMax, TXT("Maximal delay to start despawning countdown.") )
	PROPERTY_EDIT( m_despawnTime, TXT("Time countdown to despawn when visibility and range conditions are met.") )
END_CLASS_RTTI()


class CSpawnTreeDespawnAction
{
protected:
	SSpawnTreeDespawnConfiguration				m_configuration;

	SpawnTreeDespawnerId						m_id;
	Int8										m_aiPriority;
	Bool										m_initialDelay			: 1;
	Bool										m_aiActivated			: 1;

	THandle< CActor >							m_actor;
	
	Float										m_delay;
	THandle< IAIActionTree >					m_ai;
	

	Bool CheckConditions( const Vector& referencePosition, CActor* actor );
public:
	CSpawnTreeDespawnAction( const SSpawnTreeDespawnConfiguration& configuration, CActor* actor, SpawnTreeDespawnerId id );
	CSpawnTreeDespawnAction( const SSpawnTreeDespawnConfiguration& configuration, IAIActionTree* ai, Int8 aiPriority, CActor* actor, SpawnTreeDespawnerId id );

	Bool UpdateDespawn( const Vector& referencePosition );		// return true on despawn attempt
	SpawnTreeDespawnerId GetDespawnerId() const										{ return m_id; }
};