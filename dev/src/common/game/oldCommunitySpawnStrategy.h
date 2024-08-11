/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "spawnStrategy.h"


///////////////////////////////////////////////////////////////////////////////

struct SAgentStub;
enum ELODWeight : CEnum::TValueType;

///////////////////////////////////////////////////////////////////////////////

/// Obsolete functionality the old community system used to determine
/// who should be spawned
class COldCommunitySpawnStrategy : public IEngineSpawnStrategy
{
	DECLARE_ENGINE_CLASS( COldCommunitySpawnStrategy, IEngineSpawnStrategy, 0 );

private:
	Float		m_spawnRadius;
	Float		m_despawnRadius;

public:
	COldCommunitySpawnStrategy();

	//! Sets a new spawn distance
	RED_INLINE void SetSpawnRadius( Float val ) { m_spawnRadius = val; }

	//! Sets a new despawn distance
	RED_INLINE void SetDespawnRadius( Float val ) { m_despawnRadius = val; }

	// ------------------------------------------------------------------------
	// ISpawnStrategy implementation
	// ------------------------------------------------------------------------
	void SetLOD( const CAgentsWorld& world, IAgent& agent ) const;

	virtual void OnPostLoad() override;

	static Float CalculateCameraRespectiveWeight( const Vector& camForward, const Vector& camPos, const Vector& stubPos, Float spawnRadius );

private:
	Bool ShouldBeSpawned( SAgentStub *agentStub, const CAgentsWorld& world ) const;
	Bool ShouldBeDespawned( const SAgentStub *agentStub, const CAgentsWorld& world ) const;
};
BEGIN_CLASS_RTTI( COldCommunitySpawnStrategy )
	PARENT_CLASS( IEngineSpawnStrategy )
	PROPERTY_EDIT( m_spawnRadius, TXT( "Agent's spawn distance" ) );
	PROPERTY_EDIT( m_despawnRadius, TXT( "Agent's despawn distance ( should be about 10% larger than the spawn radius to introduce histeresis )" ) );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
