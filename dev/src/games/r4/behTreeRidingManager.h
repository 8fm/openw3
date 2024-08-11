#pragma once
#include "../../common/game/behTreeNodeSpecial.h"
#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/createEntityHelper.h"

#include "r4CreateEntityManager.h"
#include "ridingAiStorage.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRidingManagerDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorRidingManagerInstance;
class CBehTreeDecoratorRidingManagerDefinition : public IBehTreeNodeSpecialDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorRidingManagerDefinition, IBehTreeNodeSpecialDefinition, CBehTreeDecoratorRidingManagerInstance, RidingManager );
protected:
	IBehTreeNodeDefinition*				m_child;
	IBehTreeNodeDefinition*				m_mountHorseChild;
	IBehTreeNodeDefinition*				m_dismountHorseChild;
	IBehTreeNodeDefinition*				m_mountBoatChild;
	IBehTreeNodeDefinition*				m_dismountBoatChild;

public:
	CBehTreeDecoratorRidingManagerDefinition();
	IBehTreeNodeInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

	Bool					IsTerminal() const override;
	Bool					IsValid() const override;

	Bool					CanAddChild() const override;
	void					RemoveChild( IBehTreeNodeDefinition* node ) override;
	Int32					GetNumChildren() const override;
	IBehTreeNodeDefinition*	GetChild( Int32 index ) const override;
	void					AddChild( IBehTreeNodeDefinition* node ) override;

	void					CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Correct children order to be coherent with positions, returns true if modification occurs
	Bool CorrectChildrenOrder() override;
#endif
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorRidingManagerDefinition );
	PARENT_CLASS( IBehTreeNodeSpecialDefinition );	
	PROPERTY( m_child );
	PROPERTY( m_mountHorseChild );
	PROPERTY( m_dismountHorseChild );
	PROPERTY( m_mountBoatChild );
	PROPERTY( m_dismountBoatChild );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRidingManagerInstance
// Takes care of executing mount and dismount node from messages it receives
class CBehTreeDecoratorRidingManagerInstance : public IBehTreeNodeInstance
{
	typedef IBehTreeNodeInstance Super;
	friend class CBehTreeDecoratorRidingManagerDefinition;
protected:
	IBehTreeNodeInstance*				m_child;
	IBehTreeNodeInstance*				m_mountHorseChild;
	IBehTreeNodeInstance*				m_dismountHorseChild;
	IBehTreeNodeInstance*				m_mountBoatChild;
	IBehTreeNodeInstance*				m_dismountBoatChild;

	CAIStorageRiderData::CStoragePtr	m_riderData;
	ERidingManagerTask					m_nextTask;
	EDismountType						m_nextTaskDismountType;
	Bool								m_nextTaskIsFromScript;

	// Player only saved data
	Bool								m_wasMountedWhenSaving;
	EVehicleSlot						m_vehicleSlot_save;
	Bool								m_wasMountedOnBoatWhenSaving;
	Float								m_loadFailTimeout;
	Float								m_tryFindVehicleTimeout;
public:
	typedef CBehTreeDecoratorRidingManagerDefinition Definition;

	CBehTreeDecoratorRidingManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	~CBehTreeDecoratorRidingManagerInstance();

	void OnDestruction() override;

	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	//! Handling children
	Int32 GetNumChildren() const override;
	IBehTreeNodeInstance* GetChild( Int32 index ) const override;

	Uint32 GetActiveChildCount() const override;
	IBehTreeNodeInstance* GetActiveChild( Uint32 activeChild ) const override;

	void UpdateSound();
	void UpdatePlayerLoadSave();
	void SaveState( IGameSaver* writer ) override;
	Bool LoadState( IGameLoader* reader ) override;
	Bool IsSavingState() const override;
};


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorRidingCheckDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorRidingCheckInstance;
class CBehTreeNodeDecoratorRidingCheckDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorRidingCheckDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorRidingCheckInstance, RidingCheck );
protected:

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorRidingCheckDefinition( ) {}
};

BEGIN_CLASS_RTTI(CBehTreeNodeDecoratorRidingCheckDefinition);
	PARENT_CLASS(IBehTreeNodeDecoratorDefinition);	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRidingCheckInstance
// Makes sure a branch is not executed before a mount or dismount order from another branch is in progress
class CBehTreeDecoratorRidingCheckInstance : public IBehTreeNodeDecoratorInstance
{
protected:
	typedef IBehTreeNodeDecoratorInstance Super;

	CAIStorageRiderData::CStoragePtr		m_riderData;
	Bool									m_riderActionFromMyBranch;

	Bool ConditionCheck();
public:
	typedef CBehTreeNodeDecoratorRidingCheckDefinition Definition;

	CBehTreeDecoratorRidingCheckInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	void Deactivate() override;

	Bool IsAvailable() override;
	Int32 Evaluate() override;
};


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableDefinitionHaha
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorDismountCheckInstance;
class CBehTreeDecoratorDismountCheckDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorDismountCheckDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorDismountCheckInstance, DismountCheck );
protected:
	EDismountType	m_dismountType;
public:
	CBehTreeDecoratorDismountCheckDefinition() 
		: m_dismountType( DT_normal )
	{
	}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorDismountCheckDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
	PROPERTY_EDIT( m_dismountType, TXT("Dismont type used when the node forces the rider to dismount") )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorDismountCheckInstance
// After the branch is activated
// this node blocks the rest of the branch is the rider is not dismounted and make him dismount if necessary
// Once the rider is dismounted the rest of the branch is activated
// Also if the rider suddenly becomes mounted ( mount from scripts ) while the branch is active it completes the branche.
class CBehTreeDecoratorDismountCheckInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CAIStorageRiderData::CStoragePtr		m_riderData;
	Bool									m_isDismounting;
	EDismountType							m_dismountType;
public:
	typedef CBehTreeDecoratorDismountCheckDefinition Definition;

	CBehTreeDecoratorDismountCheckInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	Bool Activate() override;
	void Update() override;

	Bool OnEvent( CBehTreeEvent& e ) override;
protected:
	Bool Check();
};


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairingLogicDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorRiderPairingLogicInstance;
class CBehTreeDecoratorRiderPairingLogicDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorRiderPairingLogicDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorRiderPairingLogicInstance, RiderPairingLogic );
protected:
	CBehTreeValCName	m_preferedHorseTag;
	Float				m_range;
	
public:
	CBehTreeDecoratorRiderPairingLogicDefinition() 
		: m_preferedHorseTag( CNAME( generic_horse ) )
		, m_range( 50.0f )
	{
	}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeDecoratorRiderPairingLogicDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
	PROPERTY_EDIT( m_preferedHorseTag, TXT( "entity Tag" )  );
	PROPERTY_EDIT( m_range, TXT( "range at which rider can detect its horse" ) )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairingLogicInstance
class CBehTreeDecoratorRiderPairingLogicInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CName									m_preferedHorseTag;
	Float									m_range;
	Bool									m_wasMountedWhenSaving;
	Float									m_loadFailTimeout;

	CAIStorageRiderData::CStoragePtr		m_riderData;
	THandle< CEncounter >					m_encounter; // pointer to the one and only encounter the node can be part of
	Float									m_timeToNextUpdate;
	Float									m_updateFreq;
	Bool									m_horseNeeded;
	THandle< CR4CreateEntityHelper >		m_createEntityHelperHandle;

public:
	typedef CBehTreeDecoratorRiderPairingLogicDefinition Definition;

	CBehTreeDecoratorRiderPairingLogicInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	void OnDestruction()override;

	Bool Activate() override;
	void Deactivate() override;
	void Update() override;
	Bool OnListenedEvent( CBehTreeEvent& e )override;

	void SaveState( IGameSaver* writer )override;
	Bool LoadState( IGameLoader* reader )override;
	Bool IsSavingState() const override;

private :
	void	SummonHorseForNPC( const CAIStorageRiderData & riderData, CName preferedHorseTag );	
	Bool	PairUsingEncounter( const CAIStorageRiderData & riderData, CEncounter *const encounter, CActor *const riderActor );	
};

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairWithHorseDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorRiderPairWithHorseInstance;
class CBehTreeDecoratorRiderPairWithHorseDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorRiderPairWithHorseDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorRiderPairWithHorseInstance, RiderPairWithHorse );
protected:
	
public:
	CBehTreeDecoratorRiderPairWithHorseDefinition() 
	{
	}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorRiderPairWithHorseDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairWithHorseInstance
class CBehTreeDecoratorRiderPairWithHorseInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CAIStorageRiderData::CStoragePtr		m_riderData;
	Bool									m_messageSend;
public:
	typedef CBehTreeDecoratorRiderPairWithHorseDefinition Definition;

	CBehTreeDecoratorRiderPairWithHorseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	Bool Activate() override;
	void Update() override;

	Bool OnEvent( CBehTreeEvent& e )override;
};


struct SBehTreePairHorseEventParam
{
	DECLARE_RTTI_STRUCT( SBehTreePairHorseEventParam )

	SBehTreePairHorseEventParam( CHorseRiderSharedParams* params = nullptr )
		: m_ptr( params )
		, m_outcome( false )											{}

	CHorseRiderSharedParams*			m_ptr;
	Bool								m_outcome;
};

BEGIN_NODEFAULT_CLASS_RTTI( SBehTreePairHorseEventParam )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHorsePairWithRiderInstance
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorHorsePairWithRiderInstance;
class CBehTreeDecoratorHorsePairWithRiderDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorHorsePairWithRiderDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorHorsePairWithRiderInstance, HorsePairWithRider );
protected:
	CName m_partyMemberName;
	Float m_range;

public:
	CBehTreeDecoratorHorsePairWithRiderDefinition() 
		: m_partyMemberName( CNAME(horse) )
		, m_range( 50.0f )
	{
	}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorHorsePairWithRiderDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderPairWithRiderInstance
class CBehTreeDecoratorHorsePairWithRiderInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CAIStorageHorseData::CStoragePtr		m_horseData;
public:
	typedef CBehTreeDecoratorHorsePairWithRiderDefinition Definition;

	CBehTreeDecoratorHorsePairWithRiderInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	void OnDestruction()override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
};



