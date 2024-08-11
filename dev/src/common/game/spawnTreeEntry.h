#pragma once

#include "spawnTreeBaseEntry.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CCreatureEntry with one creature
class CCreatureEntry : public CBaseCreatureEntry
{
	DECLARE_ENGINE_CLASS( CCreatureEntry, CBaseCreatureEntry, 0 );
protected:
	CName									m_creatureDefinition;
	TInstanceVar< Int16 >					i_creatureDefinition;									// cached id of current definition (based on creature definition name)
	TInstanceVar< CTemplateLoadRequest >	i_templateLoadRequest;


	Int16							GetCreatureDefinitionId( CSpawnTreeInstance& instance ) const	{ return instance[ i_creatureDefinition ]; }
	CEncounterCreatureDefinition*	GetCreatureDefinition( CSpawnTreeInstance& instance ) const;

	IJobEntitySpawn*				CreateSpawnJob( const Vector3& pos, Float yaw, Uint32 sp, CSpawnTreeInstance& instance, Int32 spawnGroupToUse  ) override;
	Bool							OnSpawnJobIsDoneInternal( CSpawnTreeInstance& instance, IJobEntitySpawn* spawnJob, CEncounter::SActiveEntry& encounterEntryData ) override;
	ELoadTemplateResult				UpdateLoading( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context ) override;
	Bool							IsCreatureElligible( CSpawnTreeInstance& instance, const CEncounterCreaturePool::SCreature &creature ) override;
	Bool							UpdateCollectCreaturesToSteal( CSpawnTreeInstance& instance, CEncounter::SActiveEntry& entry ) override;
	Bool							WasCreatureLimitReached( CSpawnTreeInstance& instance, Int32& groupToUse ) override;
public:

	CName							GetCreatureDefinitionName() const								{ return m_creatureDefinition; }

	// ISpawnTreeBaseNode interface
	void							Deactivate( CSpawnTreeInstance& instance ) override;
	Bool							UsesCreatureDefinition( CName creatureDefinition ) const override;
	void							CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames ) override;

	// CBaseCreatureEntry interface
	Bool							OnCreatureLoaded( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, CEncounter::SActiveEntry& encounterEntryData ) override;
	ISpawnTreeInitializer::EOutput	ActivateInitializers( CSpawnTreeInstance& instance, CActor* actor, ESpawnType spawnType, Int32 definitionCount = 0 ) override;
	Int32							GetCreatureDefinitionsCount() const override;
	Int16							GetCreatureDefinitionId( CSpawnTreeInstance& instance, Int32 i ) const override;


	// Instance buffer interface
	void							OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void							OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;
	void							OnDeinitData( CSpawnTreeInstance& instance ) override;

	// IEdSpawnTreeNode interface
	String							GetBlockCaption() const override;
	String							GetBlockDebugCaption( const CSpawnTreeInstance& instanceBuffer ) const override;
	String							GetEditorFriendlyName() const override;
	Bool							IsHiddenByDefault() const override;

	void							GatherBudgetingStats( ICreatureDefinitionContainer* container, SBudgetingStats& stats ) override;
	
};

BEGIN_CLASS_RTTI( CCreatureEntry );
	PARENT_CLASS( CBaseCreatureEntry );
	PROPERTY_CUSTOM_EDIT( m_creatureDefinition,	TXT("Creature definition"), TXT("CreatureDefinitionsEditor") );
END_CLASS_RTTI();

