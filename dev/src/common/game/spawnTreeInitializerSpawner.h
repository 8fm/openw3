#pragma once

#include "2daProperties.h"
#include "spawnTreeSpawner.h"

class ISpawnTreeSpawnerInitializer : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeSpawnerInitializer, ISpawnTreeInitializer );
public:
	Bool			IsConflicting( const ISpawnTreeInitializer* initializer ) const override;
	Bool			IsSpawner() const override;
	Bool			IsSpawnableOnPartyMembers() const override;

	virtual const ISpawnTreeBaseSpawner* GetSpawner() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeSpawnerInitializer );
	PARENT_CLASS( ISpawnTreeInitializer );
END_CLASS_RTTI()


class CSpawnTreeInitializerWaypointSpawner : public ISpawnTreeSpawnerInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerWaypointSpawner, ISpawnTreeSpawnerInitializer, 0 );

protected:
	CSpawnTreeWaypointSpawner				m_spawner;

public:
	CSpawnTreeInitializerWaypointSpawner()
		: m_spawner()																{}

	void			OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void			OnInitData( CSpawnTreeInstance& instance ) override;
	void			OnDeinitData( CSpawnTreeInstance& instance ) override;

	EFindSpawnResult				FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP ) const override;
	EFindSpawnResult				FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const override;
	void							CollectSpawnTags( TagList& tagList ) const override;

	void							OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const override;

	const ISpawnTreeBaseSpawner*	GetSpawner() const override;

	// edSpawnTreeNode interface
	String			GetBlockCaption() const override;
	String			GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerWaypointSpawner );
	PARENT_CLASS( ISpawnTreeSpawnerInitializer );
	PROPERTY_EDIT( m_spawner, TXT("Configuration") );
END_CLASS_RTTI()

class CSpawnTreeInitializerActionpointSpawner : public ISpawnTreeSpawnerInitializer
#ifndef NO_EDITOR
	, public CActionPointCategories2dPropertyOwner
#endif
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerActionpointSpawner, ISpawnTreeSpawnerInitializer, 0 );

	static const Float MIN_SPAWN_DELAY;

protected:
	CSpawnTreeActionPointSpawner			m_spawner;

public:
	CSpawnTreeInitializerActionpointSpawner()
		: m_spawner()																{}

	EOutput			Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;

	void			OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void			OnInitData( CSpawnTreeInstance& instance ) override;
	void			OnDeinitData( CSpawnTreeInstance& instance ) override;

	EFindSpawnResult				FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP ) const override;
	EFindSpawnResult				FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const override;
	void							CollectSpawnTags( TagList& tagList ) const override;

	void							OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const override;

	const ISpawnTreeBaseSpawner*	GetSpawner() const override;
	
#ifndef RED_FINAL_BUILD
	virtual void OnPostLoad() override;
#endif

#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif
	// edSpawnTreeNode interface
	String			GetBlockCaption() const override;
	String			GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerActionpointSpawner );
	PARENT_CLASS( ISpawnTreeSpawnerInitializer );
	PROPERTY_EDIT( m_spawner, TXT("Configuration") );
END_CLASS_RTTI()
