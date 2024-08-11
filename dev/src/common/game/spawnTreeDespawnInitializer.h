#pragma once

#include "spawnTreeInitializer.h"
#include "spawnTreeDespawnConfiguration.h"
#include "behTreeArbitratorPriorities.h"

class CSpawnTreeDespawnerHandler;

class CSpawnTreeDespawnInitializer : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeDespawnInitializer, ISpawnTreeInitializer, 0 );
protected:
	SSpawnTreeDespawnConfiguration			m_instantDespawnConfiguration;

public:
	CSpawnTreeDespawnInitializer()
		: m_instantDespawnConfiguration()											{}

	Bool					CreateDespawners( CSpawnTreeDespawnerHandler& handler, CActor* actor, SpawnTreeDespawnerId id ) const override;

	String					GetEditorFriendlyName() const override;
	Bool					IsConflicting( const ISpawnTreeInitializer* initializer ) const override;
	Bool					IsSpawnableOnPartyMembers() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeDespawnInitializer )
	PARENT_CLASS( ISpawnTreeInitializer )
	PROPERTY_EDIT( m_instantDespawnConfiguration, TXT("Despawn configuration") )
END_CLASS_RTTI()


class CSpawnTreeAIDespawnInitializer : public CSpawnTreeDespawnInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeAIDespawnInitializer, CSpawnTreeDespawnInitializer, 0 );
protected:
	SSpawnTreeAIDespawnConfiguration		m_aiDespawnConfiguration;
	THandle< CAIDespawnTree >				m_ai;
	ETopLevelAIPriorities					m_aiPriority;

public:
	CSpawnTreeAIDespawnInitializer()
		: m_aiDespawnConfiguration()
		, m_aiPriority( AIP_AboveEmergency )										{}

	Bool					CreateDespawners( CSpawnTreeDespawnerHandler& handler, CActor* actor, SpawnTreeDespawnerId id ) const override;

#ifndef NO_EDITOR
	void					OnCreatedInEditor() override;
#endif
	String					GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeAIDespawnInitializer );
	PARENT_CLASS( CSpawnTreeDespawnInitializer );
	PROPERTY_EDIT( m_aiDespawnConfiguration, TXT("AI behavior conditions configuration") );
	PROPERTY_INLINED( m_ai, TXT("AI definition") );
	PROPERTY_EDIT( m_aiPriority, TXT("AI despawn action priority") );
END_CLASS_RTTI();
