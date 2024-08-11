#pragma once

#include "encounterTypes.h"
#include "encounter.h"
#include "spawnTreeNode.h"
#include "spawnTreeSpawner.h"
#include "spawnTreeEntryListElement.h"
#include "spawnTreeInitializersIterator.h"
#include "templateLoadBalancer.h"
#include "wayPointsCollection.h"

enum EEntryState
{ 
	EES_ACTIVATED,
	EES_DEACTIVATED,
	EES_LOADING,
	EES_UNLOADING,
	EES_PRESTART,
};

BEGIN_ENUM_RTTI( EEntryState );
	ENUM_OPTION( EES_ACTIVATED );
	ENUM_OPTION( EES_DEACTIVATED );
	ENUM_OPTION( EES_LOADING );
	ENUM_OPTION( EES_UNLOADING );
	ENUM_OPTION( EES_PRESTART );
END_ENUM_RTTI();


struct SCompiledInitializer;
class ISpawnTreeInitializer;

struct SSpawnTreeEntryStealCreatureState
{
	DECLARE_RTTI_STRUCT( SSpawnTreeEntryStealCreatureState )

	typedef TDynArray< Uint16 > PostponedInitializers;

	Bool									m_isDelayed;
	Uint8									m_subdefIndex;
	Uint16									m_previousOwnerEntry;
	THandle< CActor >						m_actor;
	EngineTime								m_delayTimeout;
	PostponedInitializers					m_postponedInitializers;
};

BEGIN_NODEFAULT_CLASS_RTTI( SSpawnTreeEntryStealCreatureState )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
// Spawn tree entry properties, that are being setup by its initializers.
struct SSpawnTreeEntrySetup
{
	DECLARE_RTTI_STRUCT( SSpawnTreeEntrySetup )

	SSpawnTreeEntrySetup()
		: m_spawnRatio( 1.f )
		, m_stealingDelay( 15.f )
		, m_greedyStealing( false )																{}

	Float									m_spawnRatio;
	Float									m_stealingDelay;
	Bool									m_greedyStealing;
};

BEGIN_NODEFAULT_CLASS_RTTI( SSpawnTreeEntrySetup )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
struct SSpawnTreeUpdateSpawnContext
{
	SSpawnTreeUpdateSpawnContext(
		EngineTime currentTime,
		const Matrix& referenceTransform,
		Uint32 spawnJobLimit )
		: m_runtimeData( nullptr )
		, m_currentTime( currentTime )
		, m_referenceTransform( referenceTransform )
		, m_spawnJobLimit( spawnJobLimit )
		, m_optimizeTimeouts( true )
		, m_ignoreVision( false )
		, m_ignoreMinDistance( false )			
		, m_ignoreSpawnerTimeout( false )														{}

	CEncounter::SActiveEntry*	m_runtimeData;
	EngineTime					m_currentTime;
	Matrix						m_referenceTransform;
	Int32						m_spawnJobLimit;
	Bool						m_optimizeTimeouts;
	Bool						m_ignoreVision;
	Bool						m_ignoreMinDistance;
	Bool						m_ignoreSpawnerTimeout;

	void						SetTickIn( Float time );
};


//////////////////////////////////////////////////////////////////////////
// Base creature entry - encounter member that is responsible for
// managing characters spawned from same set of spawnpoints, same template
// and same ai.
class CBaseCreatureEntry : public ISpawnTreeLeafNode
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CBaseCreatureEntry, ISpawnTreeLeafNode );
	friend CEncounter;

protected:
	struct InitializersIterator : public ISpawnTreeInitializersIterator
	{
	protected:
		const CBaseCreatureEntry&										m_entry;
		CSpawnTreeInstance&												m_instance;
		TDynArray< THandle< ISpawnTreeInitializer> >::const_iterator	m_it;
		TDynArray< THandle< ISpawnTreeInitializer> >::const_iterator	m_end;
		
	public:
		InitializersIterator( const CBaseCreatureEntry& entry, CSpawnTreeInstance& instance )
			: m_entry( entry )
			, m_instance( instance )
			, m_it( entry.m_initializers.Begin() )
			, m_end( entry.m_initializers.End() )												{}

		Bool							Next( const ISpawnTreeInitializer*& outInitializer, CSpawnTreeInstance*& instanceBuffer ) override;
		void							Reset() override;
	};

	enum ELoadTemplateResult
	{
		LOAD_FAILED,
		LOAD_SUCCESS,
		LOAD_PENDING,
	};

	typedef TListOperations< TDynArray< THandle< ISpawnTreeInitializer> >, ISpawnTreeInitializer > ListOperations;
	typedef TDynArray< SCompiledInitializer > InitializersList;
	typedef TDynArray< SSpawnTreeEntryStealCreatureState > StealingInProgress;

	TDynArray< THandle< ISpawnTreeInitializer> >		m_initializers;

	Int32									m_quantityMin;
	Int32									m_quantityMax;

	Float									m_spawnInterval;

	Float									m_waveDelay;
	Float									m_waveCounterHitAtDeathRatio;

	Bool									m_randomizeRotation;

	Int32									m_group;

	CSpawnTreeWaypointSpawner				m_baseSpawner;
	GameTime								m_recalculateDelay;



	// instance vars
	TInstanceVar< InitializersList >		i_initializers;
	TInstanceVar< InitializersList >		i_tickable;
	TInstanceVar< StealingInProgress >		i_stealingInProgress;
	TInstanceVar< EngineTime >				i_spawnTimeout;
	TInstanceVar< EngineTime >				i_waveTimeout;
	TInstanceVar< EngineTime >				i_loadPriorityRecalculationTimeout;
	TInstanceVar< GameTime >				i_recalculateTimeout;
	TInstanceVar< SCompiledSpawnStrategyInitializer > i_spawnStrategy;
	TInstanceVar< SSpawnTreeEntrySetup >	i_setup;
	TInstanceVar< Uint16 >					i_entryUniqueId;
	TInstanceVar< Int16 >					i_numCreatureDead;
	TInstanceVar< Int16 >					i_numCreaturesToSpawn;	
	TInstanceVar< Int16	>					i_numCreaturesSpawned;
	TInstanceVar< Uint16 >					i_listId;												// unique id for active entries list
	TInstanceVar< Bool >					i_baseSpawnerValid;										// if set 'm_baseSpawner' is being omitted totally
	TInstanceVar< Bool >					i_isUpdated;											// is entry scheduled for updates by encounter
	TInstanceVar< Bool >					i_isCreatureDefinitionInitialized;						// is creature definition ready to spawn
	TInstanceVar< Bool >					i_waveDelayIsOn;
	TInstanceVar< Bool >					i_noSpawnPointDefined;
	TInstanceVar< Bool >					i_isSpawnJobRunning;
	TInstanceVar< Float >					i_timeToNextTick;
	
	Bool							CheckEntityTemplate( CEntityTemplate* entityTemplate );

	void							PreCreatureSpawn( CSpawnTreeInstance& instance );				// function is run when spawning job is started
	CEntityTemplate*				CheckIsCreatureDefinitionReady( CEncounterCreatureDefinition* creatureDefinition, CSpawnTreeInstance& instance );
	void							HandleWaveRespawn( CSpawnTreeInstance& instance ) const;
	Int16							RecalculateNumCreaturesToSpawn(  CSpawnTreeInstance& instance ) const;	

	RED_INLINE Bool					WasSpawnLimitReachedCommon( CSpawnTreeInstance& instance ) const{ return GetNumCreaturesToSpawn( instance ) <= instance[ i_numCreaturesSpawned ]+instance[ i_numCreatureDead ]; }
	RED_INLINE Bool					TestSpawnInterval( CSpawnTreeInstance& instance )				{ EngineTime& t = instance[ i_spawnTimeout ]; if ( t.IsValid() ) { if ( t > GGame->GetEngineTime() ) return false; t = EngineTime::ZERO; } return true; }

	void							UpdateSpawnAnew( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context );
	void							StealCreature( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature );

	virtual ELoadTemplateResult		UpdateLoading( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context ) = 0;
	virtual Bool					UpdateCollectCreaturesToSteal( CSpawnTreeInstance& instance, CEncounter::SActiveEntry& entry ) = 0;
	Bool							UpdateProcessCreatureStealing( CSpawnTreeInstance& instance, CEncounter::SActiveEntry& entry );

	virtual Bool					IsCreatureElligible( CSpawnTreeInstance& instance, const CEncounterCreaturePool::SCreature &creature ) = 0;
	virtual Int32					AcceptActorByInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionCount = 0 );
	virtual Bool					WasCreatureLimitReached( CSpawnTreeInstance& instance, Int32& groupToUse ) = 0;
	virtual IJobEntitySpawn*		CreateSpawnJob( const Vector3& pos, Float yaw, Uint32 sp, CSpawnTreeInstance& instance, Int32 spawnGroupToUse ) = 0;
	virtual Bool					OnSpawnJobIsDoneInternal( CSpawnTreeInstance& instance, IJobEntitySpawn* spawnJob, CEncounter::SActiveEntry& encounterEntryData ) = 0;

	virtual Bool					AreInitializersSaving( CSpawnTreeInstance& instance ) const;
	virtual Bool					SaveInitializers( CSpawnTreeInstance& instance, IGameSaver* writer ) const;
	virtual Bool					LoadInitializers( CSpawnTreeInstance& instance, IGameLoader* reader ) const;

	void							ReserveAPForSpawn( CSpawnTreeInstance& instance, EntitySpawnInfo& data, Uint32 sp );

public:
	CBaseCreatureEntry();
	~CBaseCreatureEntry();

	RED_INLINE Int16				GetNumCreaturesSpawned( CSpawnTreeInstance& instance ) const				{ return instance[ i_numCreaturesSpawned ]; }
	RED_INLINE Int16				GetNumCreaturesToSpawn( const CSpawnTreeInstance& instance ) const			{ return instance[ i_numCreaturesToSpawn ]; }
	RED_INLINE Int16				GetNumCreaturesDead( CSpawnTreeInstance& instance ) const					{ return instance[ i_numCreatureDead ]; }
	RED_INLINE Bool					IsPerformingSpawnJob( CSpawnTreeInstance& instance ) const					{ return instance[ i_isSpawnJobRunning ]; }

	RED_INLINE TDynArray< THandle< ISpawnTreeInitializer > >& GetInitializers()									{ return m_initializers; }
	RED_INLINE const InitializersList& GetRuntimeInitializers( CSpawnTreeInstance& instance )					{ return instance[ i_initializers ]; }
	RED_INLINE const InitializersList& GetRuntimeInitializers( const CSpawnTreeInstance& instance ) const		{ return instance[ i_initializers ]; }

	RED_INLINE void					ForceRecalculateNumCreaturesToSpawn( CSpawnTreeInstance& instance ) const	{ instance[ i_numCreaturesToSpawn ] = RecalculateNumCreaturesToSpawn( instance ); }
	void							Activate( CSpawnTreeInstance& instance ) override;
	void							Deactivate( CSpawnTreeInstance& instance ) override;
	Bool							UpdateSpawn( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context );
	Bool							FastForwardUpdateSpawn( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, CEncounter::FastForwardUpdateContext& fastForwardContext );
	void							OnFullRespawn(  CSpawnTreeInstance& instance ) const override;
	void							CollectSpawnTags( TagList& tagList ) override;
	void							TickInitializers( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreatureList& creatures, SSpawnTreeUpdateSpawnContext& context );
	void							OnEvent( CSpawnTreeInstance& instance, CName eventName ) const override;
	void							UpdateEntriesSetup( CSpawnTreeInstance& instance ) const override;
	Bool							OnSpawnJobIsDone( CSpawnTreeInstance& instance, IJobEntitySpawn* spawnJob, CEncounter::SActiveEntry& encounterEntryData );

	void							UpdateSetup( CSpawnTreeInstance& instance ) const;
	
	void							OnPropertyPostChange( IProperty* property ) override;
	Bool							OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	virtual Bool					UsesCreatureDefinition( CName creatureDefinition ) const					= 0;
	virtual Int32					GetCreatureDefinitionsCount() const											= 0;
	virtual Int16					GetCreatureDefinitionId( CSpawnTreeInstance& instance, Int32 i ) const		= 0;
	Uint16							GetUniqueListId( CSpawnTreeInstance& instance ) const						{ return instance[ i_listId ]; }

	virtual Bool							OnCreatureLoaded( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, CEncounter::SActiveEntry& encounterEntryData );
	virtual void							OnCreatureRemoval( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, ESpawnTreeCreatureRemovalReason removalReason, Bool isCreatureDetached = false );
	virtual ISpawnTreeInitializer::EOutput	ActivateInitializers( CSpawnTreeInstance& instance, CActor* actor, ESpawnType spawnType, Int32 definitionCount = 0 );
	virtual ISpawnTreeInitializer::EOutput	ActivateInitializersOnSteal( CSpawnTreeInstance& instance, CActor* actor, SSpawnTreeEntryStealCreatureState& stealState, Bool initial );
	virtual void							DeactivateInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionIndex = -1 );

	void							SetIsUpdated( CSpawnTreeInstance& instance, Bool b )			{ instance[ i_isUpdated ] = b; }
	Bool							DoesRandomizeRotation() const									{ return m_randomizeRotation; }
	Bool							CreateDespawners( CSpawnTreeInstance& instance, CActor* actor, SpawnTreeDespawnerId despawnerId );

	EFindSpawnResult				FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP );
	EFindSpawnResult				FindSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP );
	void							OnSpawnPointsCollectionLoaded( CSpawnTreeInstance& instance );
#ifndef NO_EDITOR
	void							CookSpawnPoints( CSpawnTreeInstance& instance, CWayPointsCollection::Input& waypointsInput );
#endif

	// new function used in CanSpawnChildClass to test initializers type conflicts (to limit editor options)
	virtual Bool					CanSpawnInitializerClass( ISpawnTreeInitializer* defObject ) const;
	Bool							IsInitializerClassConflicting( ISpawnTreeInitializer* defObject ) const;

	// Instance buffer interface
	void							OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void							OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;
	void							OnDeinitData( CSpawnTreeInstance& instance ) override;

	virtual Bool					GenerateIdsRecursively() override;
	virtual void					GenerateHashRecursively( Uint64 parentHash, CSpawnTreeInstance* parentBuffer = nullptr ) override;

	// IEdSpawnTreeNode interface
	Bool							CanAddChild() const override;
	void							AddChild( IEdSpawnTreeNode* node ) override;
	void							RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32							GetNumChildren() const override;
	IEdSpawnTreeNode*				GetChild( Int32 index ) const override;
	Bool							UpdateChildrenOrder() override;
	void							GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool							CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	Bool							CanBeHidden() const override;
	Color							GetBlockColor() const override;
	String							GetBitmapName() const override;
	String							GetEditorFriendlyName() const override;

	// Saving state
	Bool							IsNodeStateSaving( CSpawnTreeInstance& instance ) const override;
	void							SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const override;
	Bool							LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const override;
	void							PostLoadGameplayState( CSpawnTreeInstance& instance ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBaseCreatureEntry )
	PARENT_CLASS( ISpawnTreeLeafNode )
	PROPERTY( m_initializers )
	PROPERTY_EDIT( m_quantityMin,						TXT("Minimum random number of creatures per spawn") )
	PROPERTY_EDIT( m_quantityMax,						TXT("Maximum random number of creatures per spawn") )
	PROPERTY_EDIT( m_spawnInterval,						TXT("If non zero, set (randomized) interval between spawns.") )
	PROPERTY_EDIT( m_waveDelay,							TXT("Delay between enemy waves") )
	PROPERTY_EDIT_RANGE( m_waveCounterHitAtDeathRatio,	TXT("Death ratio of current wave when delay counter to start new wave will hit"), 0.f, 1.f )
	PROPERTY_EDIT( m_randomizeRotation,					TXT("Use custom yaw or spawn point's default yaw"))
	PROPERTY_CUSTOM_EDIT( m_group,						TXT("Group that the creature belongs to, 0 is critical and will spawn regardless of the global spawn limits"), TXT("ScriptedEnum_EEncounterSpawnGroup") )
	PROPERTY_EDIT( m_baseSpawner,						TXT("Base spawner") )
	PROPERTY_CUSTOM_EDIT( m_recalculateDelay,			TXT("Timeout to recalculate number of creatures to spawn on activate"), TXT( "GameTimePropertyEditor" ) )
END_CLASS_RTTI()