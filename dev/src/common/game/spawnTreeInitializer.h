#pragma once

#include "edSpawnTreeNode.h"
#include "encounterCreaturePool.h"
#include "encounterTypes.h"
#include "spawnTree.h"
#include "spawnTreeInitializersIterator.h"

class CSpawnTreeDespawnerHandler;
struct SSpawnTreeUpdateSpawnContext;

struct SCompiledInitializer
{
	DECLARE_RTTI_STRUCT( SCompiledInitializer );

	ISpawnTreeInitializer*	m_initializer;
	CSpawnTreeInstance*		m_instance;

	SCompiledInitializer( ISpawnTreeInitializer* initializer, CSpawnTreeInstance* instance )
		: m_initializer( initializer )
		, m_instance( instance )													{}

	SCompiledInitializer()
		: m_initializer( nullptr )
		, m_instance( nullptr )														{}

	RED_INLINE Bool operator==( const SCompiledInitializer& other ) const			{ return m_initializer == other.m_initializer; }
};

BEGIN_CLASS_RTTI( SCompiledInitializer );
END_CLASS_RTTI();

class ISpawnTreeInitializer : public CObject, public IEdSpawnTreeNode
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeInitializer, CObject );

protected:
	Bool					m_overrideDeepInitializers;

	// utility functions
	CEntityTemplate*		GetCreatureEntityTemplate() const;

public:
	enum EActivationReason
	{
		EAR_NormalSpawn		= EST_NormalSpawn,
		EAR_PoolSpawn		= EST_PoolSpawn,
		EAR_GameIsRestored	= EST_GameIsRestored,
		EAR_Steal
	};

	enum EOutput
	{
		OUTPUT_SUCCESS,
		OUTPUT_FAILED,
		OUTPUT_POSTPONED
	};

	ISpawnTreeInitializer()
		: m_overrideDeepInitializers( false )										{}

	virtual Bool			Accept( CActor* actor ) const;
	virtual EOutput			Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const;
	virtual void			Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const;
	virtual void			Tick( CEncounterCreaturePool::SCreatureList& creatures, CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context ) const;

	virtual void			OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const;
	virtual Bool			CreateDespawners( CSpawnTreeDespawnerHandler& handler, CActor* actor, SpawnTreeDespawnerId id ) const;

	virtual void			OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const;

	// event handling
	virtual void			OnCreatureRemoval( CSpawnTreeInstance& instance, CActor* actor, ESpawnTreeCreatureRemovalReason removalReason, CBaseCreatureEntry* entry ) const;
	virtual void			OnFullRespawn( CSpawnTreeInstance& instance ) const;
	virtual void			OnEvent( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, CName eventName, CSpawnTreeInstance* entryBuffer ) const;
	virtual void			UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const;

	virtual Bool			CallActivateOnRestore() const;
	virtual Bool			CallActivateOnSpawn() const;
	virtual Bool			CallActivateOnPoolSpawn() const;
	virtual Bool			CallActivateWhenStealing() const;
	virtual Bool			IsTickable() const;
	virtual Bool			IsSpawner() const;

	virtual Bool					HasSubInitializer() const;
	virtual ISpawnTreeInitializer*	GetSubInitializer() const;

	virtual Bool				IsSpawnable() const;								// tmpshit needed to bypass abstract native<->script classes problem
	virtual Bool				IsConflicting( const ISpawnTreeInitializer* initializer ) const;
	virtual Bool				IsSpawnableOnPartyMembers() const;
	virtual EFindSpawnResult	FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP ) const;
	virtual EFindSpawnResult	FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const;
	virtual void				CollectSpawnTags( TagList& tagList ) const;
	Bool						IsOverridingDeepInitializers() const				{ return m_overrideDeepInitializers; }

	// Instance buffer interface
	virtual void			OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void			OnInitData( CSpawnTreeInstance& instance );
	virtual void			OnDeinitData( CSpawnTreeInstance& instance );

	// IEdSpawnTreeNode interface
	CObject*				AsCObject() override;
	IEdSpawnTreeNode*		GetParentNode() const override;
	Bool					CanAddChild() const override;
	Color					GetBlockColor() const override;
	String					GetEditorFriendlyName() const override;

	// CObject interace
	Bool					OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;

	// saving interface
	static Bool				AreInitializersStateSaving( ISpawnTreeInitializersIterator& iterator );
	static Bool				SaveInitializersState( ISpawnTreeInitializersIterator& iterator, IGameSaver* writer );
	static Bool				LoadInitializersState( ISpawnTreeInitializersIterator& iterator, IGameLoader* reader );

	virtual Bool			IsStateSaving( CSpawnTreeInstance& instance ) const;
	virtual void			SaveState( CSpawnTreeInstance& instance, IGameSaver* writer ) const;
	virtual void			LoadState( CSpawnTreeInstance& instance, IGameLoader* reader ) const;


	static EActivationReason ActivationReason( ESpawnType spawnType )				{ return EActivationReason( spawnType ); }
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeInitializer )
	PARENT_CLASS( CObject )
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_graphPosX )
	PROPERTY_NOT_COOKED( m_graphPosY )
	PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("Comment") )
#endif
	PROPERTY_RO( m_id, TXT("Randomly generated node id") );
	PROPERTY_EDIT( m_overrideDeepInitializers, TXT("Overrides conflicting initializers defined deeper in tree") )
END_CLASS_RTTI()


class ISpawnTreeScriptedInitializer : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_CLASS( ISpawnTreeScriptedInitializer , ISpawnTreeInitializer, 0 );
protected:
	Bool							ScriptActivate( CActor* actor ) const;
	Bool							ScriptDeactivate( CActor* actor ) const;

public:
	ISpawnTreeInitializer::EOutput	Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const override;
	void							Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const override;
	String							GetEditorFriendlyName() const override;
	
	Bool							IsSpawnable() const override;
};

BEGIN_CLASS_RTTI( ISpawnTreeScriptedInitializer );
	PARENT_CLASS( ISpawnTreeInitializer );
END_CLASS_RTTI();

