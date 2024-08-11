#pragma once

#include "spawnTreeBaseEntry.h"

class CSpawnTreeEntrySubDefinition : public CObject, public IEdSpawnTreeNode
{
	DECLARE_ENGINE_CLASS( CSpawnTreeEntrySubDefinition, CObject, 0 );
protected:
	typedef ISpawnTreeBaseNode::TListOperations< TDynArray< ISpawnTreeInitializer* >, ISpawnTreeInitializer > ListOperations;

	CName											m_creatureDefinition;
	CName											m_partyMemberId;
	Uint32											m_creatureCount;
	TDynArray< ISpawnTreeInitializer* >				m_initializers;

	TInstanceVar< Int16 >							i_creatureDefinition;
	TInstanceVar< CTemplateLoadRequest >			i_templateLoadRequest;
public:
	CSpawnTreeEntrySubDefinition()
		: m_creatureCount( 1 )																				{}

	CEncounterCreatureDefinition*	GetCreatureDefinition( CSpawnTreeInstance& instance ) const;
	Int16							GetCreatureDefinitionId( CSpawnTreeInstance& instance ) const			{ return instance[ i_creatureDefinition ]; }
	CName							GetCreatureDefinitionName() const										{ return m_creatureDefinition; }
	Uint32							GetSpawnedCreaturesCount() const										{ return m_creatureCount; }
	CName							GetPartyMemberName() const												{ return m_partyMemberId; }
	void							Block( CSpawnTreeInstance& instance ) const								{ instance[ i_creatureDefinition ] = -1; }
	CTemplateLoadRequest&			GetTemplateLoadRequest( CSpawnTreeInstance& instance ) const			{ return instance[ i_templateLoadRequest ]; }
	void							OnFullRespawn( CSpawnTreeInstance& instance ) const;

	void							OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	void							OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) const;
	void							OnDeinitData( CSpawnTreeInstance& instance ) const;

	void							Deactivate( CSpawnTreeInstance& instance ) const;

	virtual Bool					GenerateIdsRecursively() override;

	const TDynArray< ISpawnTreeInitializer* >& GetInitializers() const										{ return m_initializers; }

	// CObject interface
	void							OnPostLoad() override;

	// IEdSpawnTreeNode interface
	CObject*						AsCObject() override;
	IEdSpawnTreeNode*				GetParentNode() const override;
	Bool							CanAddChild() const override;
	void							AddChild( IEdSpawnTreeNode* node ) override;
	void							RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32							GetNumChildren() const override;
	IEdSpawnTreeNode*				GetChild( Int32 index ) const override;
	Bool							UpdateChildrenOrder() override;
	void							GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool							CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	Bool							CanBeHidden() const override;
	Bool							IsHiddenByDefault() const override;
	Color							GetBlockColor() const override;
	String							GetBlockCaption() const override;
	String							GetEditorFriendlyName() const override;
	void							GatherBudgetingStats( ICreatureDefinitionContainer* container, SBudgetingStats& stats );
};

BEGIN_CLASS_RTTI( CSpawnTreeEntrySubDefinition );
	PARENT_CLASS( CObject );
	PROPERTY_RO( m_id, TXT("Randomly generated node id") );
	PROPERTY_CUSTOM_EDIT( m_creatureDefinition,	TXT("Creature definition"), TXT("CreatureDefinitionsEditor") );
	PROPERTY_EDIT( m_partyMemberId, TXT("Name of given party member that can be referenced from external systems.") );
	PROPERTY_EDIT( m_creatureCount, TXT("Number of creatures created from this definition") );
	PROPERTY( m_initializers );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_graphPosX );
	PROPERTY_NOT_COOKED( m_graphPosY );
	PROPERTY_NOT_COOKED( m_comment );
#endif
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// CPartySpawnOrganizer
class CPartySpawnOrganizer : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CPartySpawnOrganizer );
public:
	virtual void PrePartySpawn( CCreaturePartyEntry* entry, TDynArray< EntitySpawnInfo >& spawnInfo, SEncounterSpawnPoint* sp );		// NOTICE: empty implementation are provided
	virtual void PostPartySpawn( const CEncounterCreaturePool::Party &party );															// so user can override any of those two.
};

BEGIN_ABSTRACT_CLASS_RTTI( CPartySpawnOrganizer )
	PARENT_CLASS( IScriptable )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
// CCreaturePartyEntry
class CCreaturePartyEntry : public CBaseCreatureEntry
{
	DECLARE_ENGINE_CLASS( CCreaturePartyEntry, CBaseCreatureEntry, 0 );

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
	friend class DebugWindows::CDebugWindowGameWorld;
#endif
#endif

protected:
	typedef TListOperations< TDynArray< CSpawnTreeEntrySubDefinition* >, CSpawnTreeEntrySubDefinition > DefListOperations;
	struct InitializersIterator : public TBaseClass::InitializersIterator
	{
		typedef TBaseClass::InitializersIterator Super;
	protected:
		TDynArray< CSpawnTreeEntrySubDefinition* >::const_iterator			m_subDefIt;
		TDynArray< CSpawnTreeEntrySubDefinition* >::const_iterator			m_subDefEnd;
		Uint32																m_initializerIdx;

	public:
		InitializersIterator( const CCreaturePartyEntry& entry, CSpawnTreeInstance& instance );

		Bool							Next( const ISpawnTreeInitializer*& outInitializer, CSpawnTreeInstance*& instanceBuffer ) override;
		void							Reset() override;
	};

	TDynArray< CSpawnTreeEntrySubDefinition* >		m_subDefinitions;
	THandle< CPartySpawnOrganizer >					m_partySpawnOrganizer;
	Bool											m_blockChats;
	Bool											m_synchronizeWork;
	// This hash allows for working out which party matches in different phases
	// matching parties have the same number of subdefinitions the same creature definitions and 
	// the same count in subdefinitions
	TInstanceVar< Uint32 >							i_matchingEntryHashValue;
	TInstanceVar< Uint16 >							i_spawnCount;

	Uint32							GetMatchingEntryHashValue( CSpawnTreeInstance& instance );
	Int32							AcceptActorByInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionIndex );
	
	IJobEntitySpawn*				CreateSpawnJob( const Vector3& pos, Float yaw, Uint32 sp, CSpawnTreeInstance& instance, Int32 spawnGroupToUse  ) override;
	Bool							OnSpawnJobIsDoneInternal( CSpawnTreeInstance& instance, IJobEntitySpawn* spawnJob, CEncounter::SActiveEntry& encounterEntryData ) override;
	ELoadTemplateResult				UpdateLoading( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context ) override;
	Bool							IsCreatureElligible( CSpawnTreeInstance& instance, const CEncounterCreaturePool::SCreature &creature ) override;
	Bool							UpdateCollectCreaturesToSteal( CSpawnTreeInstance& instance, CEncounter::SActiveEntry& entry ) override;
	Bool							WasCreatureLimitReached( CSpawnTreeInstance& instance, Int32& groupToUse ) override;

	Bool							AreInitializersSaving( CSpawnTreeInstance& instance ) const override;
	Bool							SaveInitializers( CSpawnTreeInstance& instance, IGameSaver* writer ) const override;
	Bool							LoadInitializers( CSpawnTreeInstance& instance, IGameLoader* reader ) const override;
public:
	// used both internally and externally by spawn point organizers to address EntitySpawnInfo list
	struct SpawnInfoIterator
	{
	private:
		CCreaturePartyEntry*			m_owner;
		CSpawnTreeEntrySubDefinition*	m_subDef;
		Uint32							m_subDefIndex;
		Uint32							m_subDefNextIndex;
		Int32							m_guysToSpawnLeft;

		void							Progress();
	public:
		SpawnInfoIterator( CCreaturePartyEntry* entry );

		void							FetchNext();

		CSpawnTreeEntrySubDefinition*	GetSubdefinition() const	{ return m_subDef; }
		Int32							GetSubdefinitionIdx() const	{ return m_subDefIndex; }
	};


	CCreaturePartyEntry();
	~CCreaturePartyEntry();

	// Base creature entry interface
	Int32							GetCreatureDefinitionsCount() const override;
	Int16							GetCreatureDefinitionId( CSpawnTreeInstance& instance, Int32 i ) const override;
	Bool							UsesCreatureDefinition( CName creatureDefinition ) const override;
	void							OnCreatureRemoval( CSpawnTreeInstance& instance, CEncounterCreaturePool::SCreature& creature, ESpawnTreeCreatureRemovalReason removalReason, Bool isCreatureDetached = false ) override;
	ISpawnTreeInitializer::EOutput	ActivateInitializers( CSpawnTreeInstance& instance, CActor* actor, ESpawnType spawnType, Int32 definitionCount = 0 ) override;
	ISpawnTreeInitializer::EOutput	ActivateInitializersOnSteal( CSpawnTreeInstance& instance, CActor* actor, SSpawnTreeEntryStealCreatureState& stealState, Bool initial ) override;
	void							DeactivateInitializers( CSpawnTreeInstance& instance, CActor* actor, Int32 definitionIndex = -1 ) override;
	void							OnFullRespawn( CSpawnTreeInstance& instance ) const override;
	void							Deactivate( CSpawnTreeInstance& instance ) override;

	Bool							CanSpawnInitializerClass( ISpawnTreeInitializer* defObject ) const override;

	// Instance buffer interface
	void							OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void							OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;
	void							OnDeinitData( CSpawnTreeInstance& instance ) override;

	virtual Bool					GenerateIdsRecursively() override;

	// IEdSpawnTreeNode interface
	void							AddChild( IEdSpawnTreeNode* node ) override;
	void							RemoveChild( IEdSpawnTreeNode* node ) override;
	IEdSpawnTreeNode*				GetChild( Int32 index ) const override;
	Int32							GetNumChildren() const override;
	void							GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool							UpdateChildrenOrder() override;
	String							GetBlockCaption() const override;
	String							GetBlockDebugCaption( const CSpawnTreeInstance& instanceBuffer ) const override;
	String							GetEditorFriendlyName() const override;
	Bool							CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	//void							GatherBudgetingStats( CSpawnTreeInstance& instance, SBudgetingStats& stats ) {};
	void							GatherBudgetingStats( ICreatureDefinitionContainer* container, SBudgetingStats& stats ) override;
	RED_INLINE Int32				GetQuantityMax() const											{ return m_quantityMax; }

	CSpawnTreeEntrySubDefinition*	AddPartyMember( bool inEditor );


private:
	void funcAddPartyMember( CScriptStackFrame& stack, void* result );
};



BEGIN_CLASS_RTTI( CCreaturePartyEntry );
	PARENT_CLASS( CBaseCreatureEntry );
	PROPERTY( m_subDefinitions );
	PROPERTY_INLINED( m_partySpawnOrganizer, TXT("Object that define the organisation of the party at spawn time") )
	PROPERTY_EDIT( m_blockChats, TXT("Bloack chats") )
	PROPERTY_EDIT( m_synchronizeWork, TXT("If true, all npcs work or none of them") )	

	NATIVE_FUNCTION( "AddPartyMember", funcAddPartyMember );
END_CLASS_RTTI();