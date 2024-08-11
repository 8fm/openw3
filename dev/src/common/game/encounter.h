/**
 * Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../core/intrusiveList.h"

#include "actor.h"
#include "edSpawnTreeNode.h"
#include "encounterTypes.h"
#include "encounterCreatureDefinition.h"
#include "encounterCreaturePool.h"
#include "encounterSpawnPoints.h"
#include "spawnTree.h"
#include "spawnTreeDespawnConfiguration.h"
#include "spawnTreeDespawnerHandler.h"

struct SFastForwardExecutionContext;
class CBaseCreatureEntry;
class CWeatherManager;
class ISpawnTreeSpawnStrategy;

namespace DebugWindows
{
	class CDebugWindowGameWorld;
}

struct SEncounterSettings
{
	static Float GetSpawnPointInvalidDelay();
	static Float GetFailedWaypointTestLocationDelay();
	static Float GetSpawnLimitReachedDelay();
	static Float GetUpdateSpawnAnewDelay();
	static Bool m_enableDelays;
};

struct SEncounterGroupLimit
{
	DECLARE_RTTI_STRUCT( SEncounterGroupLimit )

	Int32	m_group;
	CName	m_groupName;
	Uint32	m_limit;
};

BEGIN_CLASS_RTTI( SEncounterGroupLimit )
	PROPERTY_RO( m_groupName, TXT("") )
	PROPERTY_EDIT( m_limit, TXT("") )
END_CLASS_RTTI();

class CEncounterGlobalSettings : public CObject
#ifndef NO_EDITOR_EVENT_SYSTEM
	,	public IEdEventListener
#endif
{
	DECLARE_ENGINE_CLASS( CEncounterGlobalSettings, CObject, 0 );

private:
	ISpawnTreeSpawnStrategy* m_defaultSpawnStrategy;
	TDynArray< SEncounterGroupLimit > m_groupLimits;

	static CEncounterGlobalSettings* m_instance;

public:
	static CEncounterGlobalSettings& GetInstance();

	RED_INLINE ISpawnTreeSpawnStrategy* GetSpawnStrategy() { return m_defaultSpawnStrategy; }

	RED_INLINE Int32 GetLimitForGroup( Int32 group ) {  return group < m_groupLimits.SizeInt() ? m_groupLimits[group].m_limit : -1; }

	void OnPropertyPostChange( IProperty* property ) override;

private:
	void LoadSpawnGroupConfig();
	void SaveSpawnGroupConfig();

#ifndef NO_EDITOR_EVENT_SYSTEM
protected:
	// Editor global envet
	void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;
#endif
};

BEGIN_CLASS_RTTI( CEncounterGlobalSettings )
	PARENT_CLASS( CObject )
	PROPERTY_INLINED( m_defaultSpawnStrategy, TXT("Default spawn strategy for encounters") )
	PROPERTY_EDIT( m_groupLimits, TXT("") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
class CEncounter : public CGameplayEntity, public IActorTerminationListener, public ICreatureDefinitionContainer
{
	DECLARE_ENGINE_CLASS( CEncounter, CGameplayEntity, 0 );

public:
	enum EActivationReason
	{
		REASON_TRIGGER														= FLAG( 0 ),
		REASON_FAST_FORWARD													= FLAG( 1 ),
	};

	typedef CEncounterCreaturePool::SCreature		SCreatureData;
	typedef Uint64									Hash;

	static const Hash INVALID_HASH					= 0;
	

	struct SActiveEntry : public Red::System::NonCopyable // IntrusiveList::Node
	{

		struct Sorter
		{
			static Bool Less( SActiveEntry* a, SActiveEntry* b );
		};

		typedef TSortedArray< SActiveEntry*, SActiveEntry::Sorter > List;

		SActiveEntry()	
			: m_branchHash( 0 )
			, m_isActive( false )
		{}		// almost empty default constructor (list initialization)

		SActiveEntry( CBaseCreatureEntry* entry, CSpawnTreeInstance* instance );	// full constructor

		SActiveEntry( SActiveEntry&& e )
			: m_entry( e.m_entry )
			, m_instanceBuffer( e.m_instanceBuffer )
			, m_spawnedActors( Move( e.m_spawnedActors ) )
			, m_branchHash( e.m_branchHash )
			, m_isActive( e.m_isActive )		{}

		SActiveEntry& operator=( SActiveEntry&& e )
		{
			m_entry = e.m_entry;
			m_instanceBuffer = e.m_instanceBuffer;
			m_spawnedActors = Move( e.m_spawnedActors );
			m_branchHash = e.m_branchHash;
			m_isActive = e.m_isActive;
			return *this;
		}

		struct Comperator
		{
			static Bool Less( const SActiveEntry& e1, const SActiveEntry& e2 );
			static Bool Less( Uint16 a, const SActiveEntry& e );
			static Bool Less( const SActiveEntry& e, Uint16 a );
		};
		
		CBaseCreatureEntry*						m_entry;
		CSpawnTreeInstance*						m_instanceBuffer;
		CEncounterCreaturePool::SCreatureList	m_spawnedActors;
		EngineTime								m_delayedUpdate;
		Uint64									m_branchHash;
		Bool									m_isActive;
	};

	struct FastForwardUpdateContext
	{
		FastForwardUpdateContext( SFastForwardExecutionContext* context = nullptr );

		void PreUpdate()																		{ m_runningSpawnJobsCount = 0; m_isLoadingTemplates = false; }

		Uint32									m_runningSpawnJobsCount;
		Bool									m_isLoadingTemplates;
		SFastForwardExecutionContext*			m_globalContext;
	};

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
	friend class DebugWindows::CDebugWindowGameWorld;
#endif
#endif

protected:
	struct SpawnJob
	{
		IJobEntitySpawn*												m_spawnJob;
		CEncounterCreaturePool::SCreatureList::ListId					m_spawningEntry;
	};
	typedef TDynArray< SpawnJob >									SpawnJobList;

	Bool															m_enabled;
	Bool															m_active;
	Bool															m_fullRespawnScheduled;
	Bool															m_ignoreAreaTrigger;
	Bool															m_wasRaining;
	Bool															m_isFullRespawnTimeInGameTime;
	Bool															m_ticked;
	Bool															m_tickRequested;
	Bool															m_tickSuppressed;
	Bool															m_isEncounterStateLoaded;
	Uint8															m_activationReason;
	Int16															m_runningSpawnJobsCount;							// NOTICE: 

	ISpawnTreeBranch*												m_spawnTree;
	TDynArray< CEncounterCreatureDefinition* >						m_creatureDefinition;
	CEncounterCreatureDefinition::CompiledCreatureList				m_compiledCreatureList;
	CEncounterSpawnPoints											m_spawnPoints;

	TSortedArray< SActiveEntry, SActiveEntry::Comperator >			m_entries;
	SActiveEntry::List												m_activeEntriesList;
	SActiveEntry*													m_lastUpdatedEntry;

	CSpawnTreeDespawnerHandler										m_despawnHandler;

	CSpawnTreeInstance												m_instanceBuffer;
	InstanceDataLayout												m_dataLayout;
	THandle < CTriggerAreaComponent >								m_triggerArea;
	THandle< CEncounterParameters >									m_encounterParameters;
	EntityHandle													m_spawnArea;

	SpawnJobList													m_spawnJobs;

	GameTime														m_fullRespawnDelay;
	
	GameTime														m_fullRespawnTime;
	
	Float															m_conditionRetestTimer;
	Float															m_conditionRetestTimeout;
	Float															m_deactivationTimeout;

	SSpawnTreeDespawnConfiguration									m_defaultImmediateDespawnConfiguration;

	CEncounterCreaturePool											m_creaturePool;

	void							ProcessPendingSpawnJobs();
	void							EnableTick();
	void							DisableTick();
	void							RequestTicks();
	void							DontRequestTicks();
	void							SuppressTicks();
	void							UnsuppressTicks();
	void							DeactivateInstantly();

	void							PostStateLoadInitialization();

public:
	CEncounter();
	~CEncounter();

	//////////////////////////////////////////////////////////////////////
	// CObject interface
	void							OnAttached( CWorld* world ) override;
	void							OnDetached( CWorld* world ) override;

	void							OnSerialize( IFile& file ) override;
	void							OnPostLoad() override;
	//////////////////////////////////////////////////////////////////////
	// CPersistentEntity interface
	void							OnSaveGameplayState( IGameSaver* saver ) override;
	void							OnLoadGameplayState( IGameLoader* loader ) override;

	//! Should save?
	Bool							CheckShouldSave() const override		{ return true; }

	//////////////////////////////////////////////////////////////////////
	Uint16							RegisterEntry( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer );			// registers entry on initialization, and returns unique entry id
	void							SaveEntryHash( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer, Uint64 entryParentalHash );

	void							RegisterForUpdates( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer );
	void							UnregisterForUpdates( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer );

	const TDynArray< Vector >*		GetWorldPoints() const;
	CTriggerAreaComponent*			GetTriggerArea() const					{ return m_triggerArea.Get(); }

	void							EnableMember( CName& name, Bool enable );
	RED_INLINE Bool					IsEnabled() const						{ return m_enabled; }
	CEncounterParameters*			GetEncounterParameters();

	void							SpawnAsync( IJobEntitySpawn* spawnJob, CBaseCreatureEntry* entry, CSpawnTreeInstance& instance );
	SCreatureData*					RegisterSpawnedCreature( CActor* actor, CEncounter::SActiveEntry& entryData, Int16 creatureDefinitionId );
	CSpawnTreeDespawnerHandler&		GetDespawner() { return m_despawnHandler; }

	void							ActivateEncounter( EActivationReason reason );
	void							DeactivateEncounter( EActivationReason reason, Bool performImmediately );
	void							EnterArea()								{ ActivateEncounter( REASON_TRIGGER ); }
	void							LeaveArea()								{ DeactivateEncounter( REASON_TRIGGER, false ); }
	void							EnableEncounter( Bool enable );
	void							OnTick( Float timeDelta ) override;

	Bool							BeginFastForward( const Vector& referencePosition, FastForwardUpdateContext& context );
	void							FastForwardTick( Float timeDelta, FastForwardUpdateContext& context );					// Special tick done when we are fast forwarding or preloading time.
	void							EndFastForward();

	Bool							IsGameplayLODable() override { return false; }


	//////////////////////////////////////////////////////////////////////
	// ICreatureDefinitionContainer interface (editor)
	CEncounterCreatureDefinition*	AddCreatureDefinition() override;
	void							RemoveCreatureDefinition( CEncounterCreatureDefinition* ) override;
	CEncounterCreatureDefinition*	GetCreatureDefinition( CName name ) override;
	// creature definitions runtime interface (fast)
	CEncounterCreatureDefinition*	GetCreatureDefinition( Int16 id )		{ return m_compiledCreatureList[ id ]; }
	Int16							GetCreatureDefinitionId( CName name );

	Bool							SetSpawnPhase( CName phaseName );
	void							GetSpawnPhases( TDynArray< CName >& outPhaseNames );

	ISpawnTreeBaseNode*				GetRootNode() const;
	CSpawnTreeInstance*				GetEncounterInstanceBuffer()			{ return &m_instanceBuffer; }

	
	CEncounterSpawnPoints&			GetSpawnPoints()						{ return m_spawnPoints; }

	GameTime			GetFullRespawnDelay() const							{ return m_fullRespawnDelay; }
	void				ForceLogicUpdate()									{ m_conditionRetestTimer = 0.f; }
	CEncounterCreaturePool& GetCreaturePool()								{ return m_creaturePool; }

	template< class T >
	T& operator[]( const TInstanceVar<T>& var )
	{
		return m_instanceBuffer.operator []( var );
	}

	// IActorTerminationListener interface
	void				OnDeath( CActor* actor ) override;
	void				OnDetach( CActor* actor ) override;
	void				OnPrePooling( CActor* actor ) override;
	CEncounter*			AsEncounter() override;
	// Stray actor interface ( See CStrayActorManager )
	Bool				 CanBeConvertedToStrayActor() const override;
	void				OnConvertToStrayActor( CActor *const actor ) override;


	// IEdSpawnTreeNode interface
	CObject*			AsCObject() override;
	IEdSpawnTreeNode*	GetParentNode() const override;
	Bool				CanAddChild() const override;
	void				AddChild( IEdSpawnTreeNode* node ) override;
	void				RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32				GetNumChildren() const override;
	IEdSpawnTreeNode*	GetChild( Int32 index ) const override;
	virtual Bool		GenerateIdsRecursively() override;
	Bool				CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const;
	void				GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	void				PreStructureModification() override;
	void				RemoveCreature( CActor* actor, ESpawnTreeCreatureRemovalReason removalReason );	// called when creature despawn by itself
	void				GetContextMenuSpecialOptions( SpecialOptionsList& outOptions ) override;
	void				RunSpecialOption( Int32 option ) override;
	EDebugState			GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const override;
	Bool				HoldsInstanceBuffer() const override;
	CSpawnTreeInstance* GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer = NULL ) override;
	ICreatureDefinitionContainer* AsCreatureDefinitionContainer() override;

	void				ForceDespawnDetached();
	void				ForceDespawnAll();
	void				DespawnCreature( SCreatureData& kriczer );
	Bool				ScheduleCreatureToDespawn( SCreatureData& kriczer );							// TODO: protected
	void				RemoveScheduledDespawn( SCreatureData& kriczer );								// TODO: protected

	SActiveEntry*		FindCreatureEntry( Uint64 entryId, Uint64 entryHash );
	SActiveEntry*		FindActiveCreatureEntry( CEncounterCreaturePool::SCreatureList::ListId listId );
	SActiveEntry*		FindActiveCreatureEntry( CActor* actor );

	static Bool			IsCreatureEntryActive( SActiveEntry* entry );
	static Bool			IsPointSeenByPlayer( const Vector& testPoint );

	void				ScheduleFullRespawn();
	Bool				CheckFullRespawn();
	void				DoFullRespawn();

	Uint64				ComputeHash();

	static void			ForceCleanupAllEncounters();

	// accessors
	const Bool			IsIgnoreAreaTrigger() const								{ return m_ignoreAreaTrigger; }
	const Bool			IsFullRespawnScheduled() const							{ return m_fullRespawnScheduled; }

#ifndef NO_EDITOR
	CWayPointsCollection::Input* ComputeWaypointCollectionCookingInput( CWorld* world, CWayPointCookingContext* waypointsData ) const;

	// cooking
	void				OnNavigationCook( CWorld* world, CNavigationCookingContext* context ) override;
#endif
	void				OnSpawnPointsCollectionLoaded();


protected:
	CWeatherManager* GetWeatherManager();
	void BroadcastEvent( CName eventName );
	void HandleRaining();
	void DetachCreatureInstance( CActor* actorToDetach );
	
	void LazyBindBuffer();
	void UnbindBuffer();

	Bool TryDeactivate();
	
	Bool IsWithinArea( const Vector &testPoint );
	
	ISpawnTreeBaseNode*							InternalGetRootTreeNode() const override;
	TDynArray< CEncounterCreatureDefinition* >& InternalGetCreatureDefinitions() override;				// ICreatureDefinitionContainer interface

	void DetermineCreatureDefinitionLock();
	// active entries
	void AddActiveEntry( SActiveEntry* entry );

	void funcEnableMember( CScriptStackFrame& stack, void* result );
	void funcGetPlayerDistFromArea( CScriptStackFrame& stack, void* result );
	void funcGetEncounterArea( CScriptStackFrame& stack, void* result );
	void funcIsPlayerInEncounterArea( CScriptStackFrame& stack, void* result );
	void funcIsEnabled( CScriptStackFrame& stack, void* result );
	void funcEnterArea( CScriptStackFrame& stack, void* result );
	void funcLeaveArea( CScriptStackFrame& stack, void* result );
	void funcEnableEncounter( CScriptStackFrame& stack, void* result );
	void funcForceDespawnDetached( CScriptStackFrame& stack, void* result );
	void funcSetSpawnPhase( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CEncounter );
	PARENT_CLASS( CGameplayEntity );
	PROPERTY_EDIT_SAVED( m_enabled,				TXT("Is encounter enabled for processing") );
	PROPERTY_EDIT( m_ignoreAreaTrigger,			TXT("Trigger is ignored, encounter only is quest controlled") );
	PROPERTY_SAVED( m_fullRespawnScheduled ); 
	PROPERTY_CUSTOM_EDIT( m_spawnTree,			TXT("Spawn tree"), TXT("TreeEditorButton") );
	PROPERTY( m_creatureDefinition )
	PROPERTY_INLINED( m_encounterParameters,	TXT("Encounter parameters to be imposed") );
	PROPERTY_EDIT( m_spawnArea,					TXT("Entity with area component to gather spawn points from. Encounter trigger is used if spawn area is unset.") );
	PROPERTY_CUSTOM_EDIT( m_fullRespawnDelay,	TXT("Timeout to restore spawn capabilities to an entry that was completely cleared"), TXT( "GameTimePropertyEditor" ));
	PROPERTY_EDIT( m_isFullRespawnTimeInGameTime, TXT("If set to true fullRespawnDelay is expressed in game time. If set to false it is in real time.") );
	PROPERTY_SAVED( m_fullRespawnTime );
	PROPERTY_SAVED( m_wasRaining );
	PROPERTY_EDIT( m_conditionRetestTimeout,	TXT("Time between condition retesting on encounter members") );
	PROPERTY_EDIT( m_defaultImmediateDespawnConfiguration, TXT("Default immediate despawn configuration") );
	PROPERTY_EDIT( m_spawnTreeType, TXT("Edit mode for spawn tree") );
	NATIVE_FUNCTION( "EnableMember", funcEnableMember );
	NATIVE_FUNCTION( "GetPlayerDistFromArea", funcGetPlayerDistFromArea );
	NATIVE_FUNCTION( "GetEncounterArea", funcGetEncounterArea );
	NATIVE_FUNCTION( "IsPlayerInEncounterArea", funcIsPlayerInEncounterArea );
	NATIVE_FUNCTION( "IsEnabled", funcIsEnabled );
	NATIVE_FUNCTION( "EnterArea", funcEnterArea );
	NATIVE_FUNCTION( "LeaveArea", funcLeaveArea );
	NATIVE_FUNCTION( "EnableEncounter", funcEnableEncounter );
	NATIVE_FUNCTION( "ForceDespawnDetached", funcForceDespawnDetached );
	NATIVE_FUNCTION( "SetSpawnPhase", funcSetSpawnPhase );
END_CLASS_RTTI();
