#pragma once
#include "questCondition.h"
#include "inventoryComponent.h"
#include "../../common/engine/globalEventsManager.h"
#include "entityWatcher.h"

class IActorConditionType : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IActorConditionType, CObject )
protected:
	Bool	m_inverted;

public:
	IActorConditionType()
		: m_inverted( false ){}
	virtual ~IActorConditionType(){}

	// actor param is nullptr for CQuestManyActorsCondition, so one needs to write 
	// different OnActivate, OnDeactivate and OnActorLost for such case
	virtual void OnActivate( CActor* actor ) { }
	virtual void OnDeactivate( CActor* actor ) { }
	virtual void OnActorLost() { } // for example : actor was despawned
	Bool Evaluate( CActor* actor );
	virtual Bool EvaluateNullActor() { return false; }
protected :
	virtual Bool EvaluateImpl( CActor* actor ) { return true; }
public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return String::EMPTY; }
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( IActorConditionType )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT( m_inverted, TXT("Invert Condition") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestActorCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestActorCondition, IQuestCondition, 0 )

private:
	CName					m_actorTag;
	IActorConditionType*	m_checkType;

protected:
	typedef TEntityWatcher< CActor > TActorWatcher;
	TActorWatcher*			m_actorWatcher;

public:
	CQuestActorCondition();
	virtual ~CQuestActorCondition();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Actor tag: %s, Actor Condition: %s" ), 
			m_actorTag.AsString().AsChar(), ( m_checkType ) ? m_checkType->GetDescription().AsChar() : TXT( "None" ) ); 
	}
#endif

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestActorCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_actorTag, TXT( "Tag of an actor we want to check for life signs." ) )
	PROPERTY_INLINED( m_checkType, TXT( "Condition" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

enum EQuestActorConditionLogicOperation
{
	ACTORS_All,
	ACTORS_AtLeastOne,
	ACTORS_OneAndOnly,
};

BEGIN_ENUM_RTTI( EQuestActorConditionLogicOperation )
	ENUM_OPTION( ACTORS_All )
	ENUM_OPTION( ACTORS_AtLeastOne )
	ENUM_OPTION( ACTORS_OneAndOnly )
END_ENUM_RTTI();

class CQuestManyActorsCondition : public IQuestCondition, IGlobalEventsListener
{
	DECLARE_ENGINE_CLASS( CQuestManyActorsCondition, IQuestCondition, 0 )

private:
	TagList								m_actorTags;
	EQuestActorConditionLogicOperation	m_logicOperation;
	IActorConditionType*				m_condition;
	Bool								m_wasRegistered;
	Bool								m_findActors;
	

protected:
	TDynArray< THandle< CActor > >	m_actors;

public:
	CQuestManyActorsCondition();
	virtual ~CQuestManyActorsCondition();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Actor tags: %s, Actor Condition: %s" ), 
			m_actorTags.ToString().AsChar(), ( m_condition ) ? m_condition->GetDescription().AsChar() : TXT( "None" ) ); 
	}
#endif

protected:
	//! IQuestCondition implementation
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	//! IGlobalEventsListener
	void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;

private:
	Bool RegisterCallback( Bool reg );
	void FindActors();
};

BEGIN_CLASS_RTTI( CQuestManyActorsCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_actorTags, TXT( "Tag list that defines which actors to find." ) )
	PROPERTY_EDIT( m_logicOperation, TXT( "How many actors must fulfill the condition." ) );
	PROPERTY_INLINED( m_condition, TXT( "Condition to be checked on each actor." ) )
	END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestNoLivingActorsCondition : public IQuestCondition, public IGlobalEventsListener
{
	DECLARE_ENGINE_CLASS( CQuestNoLivingActorsCondition, IQuestCondition, 0 )

private:
	TagList								m_actorTags;
	TDynArray< THandle< CActor > >		m_actors;
	Bool								m_wasRegistered;
	Bool								m_findActors;

public:
	CQuestNoLivingActorsCondition();
	virtual ~CQuestNoLivingActorsCondition();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Actor tags: %s" ), m_actorTags.ToString().AsChar() ); 
	}
#endif

protected:
	//! IQuestCondition implementation
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	//! IGlobalEventsListener
	void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;

private:
	Bool RegisterCallback( Bool reg );
	void FindActors();
};

BEGIN_CLASS_RTTI( CQuestNoLivingActorsCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_actorTags, TXT( "Tag list that defines which actors to find." ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQCIsAlive : public IActorConditionType
{
	DECLARE_ENGINE_CLASS( CQCIsAlive, IActorConditionType, 0 )

protected:
	//! IActorConditionType implementation
	virtual Bool EvaluateImpl( CActor* actor ) override;
	virtual Bool EvaluateNullActor() override;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Is Alive" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCIsAlive )
	PARENT_CLASS( IActorConditionType )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQCHasAbility : public IActorConditionType
{
	DECLARE_ENGINE_CLASS( CQCHasAbility, IActorConditionType, 0 )

private:
	CName						m_ability;

protected:
	//! IActorConditionType implementation
	virtual Bool EvaluateImpl( CActor* actor ) override;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Has ability - %s" ), m_ability.AsString().AsChar() ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQCHasAbility )
	PARENT_CLASS( IActorConditionType )
	PROPERTY_EDIT( m_ability, TXT( "An ability the actor should have" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQCActorInventory : public IActorConditionType, public IInventoryListener
{
	DECLARE_ENGINE_CLASS( CQCActorInventory, IActorConditionType, 0 )

private:

	Bool						m_isFulfilled;
	Bool						m_wasRegistered;
	Bool						m_multipleActorsCondition;

protected:

	CQCActorInventory();

	//! IActorConditionType implementation
	virtual void OnActivate( CActor* actor ) override;
	virtual void OnDeactivate( CActor* actor ) override;
	virtual void OnActorLost() override;
	virtual Bool EvaluateImpl( CActor* actor ) override;

	Bool RegisterCallback( CActor* actor, Bool reg );	
	virtual Bool EvaluateImpl( CInventoryComponent* inventory );

	//! IInventoryListener
	virtual void OnInventoryEvent( CInventoryComponent* inventory, EInventoryEventType eventType, SItemUniqueId itemId, Int32 quantity, Bool fromAssociatedInventory ) override;
};

BEGIN_CLASS_RTTI( CQCActorInventory )
	PARENT_CLASS( IActorConditionType )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQCHasItem : public CQCActorInventory
{
	DECLARE_ENGINE_CLASS( CQCHasItem, CQCActorInventory, 0 )

private:
	CName						m_item;
	CName						m_itemCategory;
	CName						m_itemTag;
	Uint32						m_quantity;
	ECompareFunc				m_compareFunc;

protected:
	CQCHasItem();

	virtual Bool EvaluateImpl( CInventoryComponent* inventory ) override;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Has Item - %s, %d" ), m_item.AsString().AsChar(), m_quantity ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQCHasItem )
	PARENT_CLASS( CQCActorInventory )
	PROPERTY_EDIT( m_item, TXT( "An item the actor should have" ) )
	PROPERTY_EDIT( m_itemCategory, TXT( "Category of item the actor should have" ) );
	PROPERTY_EDIT( m_itemTag, TXT( "Tag of item the actor should have" ) );
	PROPERTY_EDIT( m_quantity, TXT( "How much of an item the actor should have" ) )
	PROPERTY_EDIT( m_compareFunc, TXT("How to compare values.") );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQCItemQuantity : public CQCActorInventory
{
	DECLARE_ENGINE_CLASS( CQCItemQuantity, CQCActorInventory, 0 )

private:
	CName						m_item;
	CName						m_itemCategory;
	CName						m_itemTag;
	Uint32						m_quantity;
	ECompareFunc				m_compareFunc;		//!< How to compare values

protected:
	CQCItemQuantity();

	virtual Bool EvaluateImpl( CInventoryComponent* inventory ) override;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Item Quantity - %s, %d" ), m_item.AsString().AsChar(), m_quantity ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQCItemQuantity );
	PARENT_CLASS( CQCActorInventory );
	PROPERTY_EDIT( m_item, TXT( "An item the actor should have" ) );
	PROPERTY_EDIT( m_itemCategory, TXT( "Category of item the actor should have" ) );
	PROPERTY_EDIT( m_itemTag, TXT( "Tag of item the actor should have" ) );
	PROPERTY_EDIT( m_quantity, TXT( "How much of an item the actor should have" ) );
	PROPERTY_EDIT( m_compareFunc, TXT("How to compare values.") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CQCActorScriptedCondition : public IActorConditionType
{
	DECLARE_ENGINE_CLASS( CQCActorScriptedCondition, IActorConditionType, 0 )

	const CFunction*	m_evaluateFunction;

protected:

	CQCActorScriptedCondition();

	//! IActorConditionType implementation
	virtual void OnActivate( CActor* actor ) override;
	virtual void OnDeactivate( CActor* actor ) override;
	virtual void OnActorLost() override;
	virtual Bool EvaluateImpl( CActor* actor ) override;
	virtual void OnScriptReloaded() override;
	void CacheScriptedFunctions();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Actor scripted condition" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCActorScriptedCondition )
	PARENT_CLASS( IActorConditionType )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
class CQCAttitude : public IActorConditionType
{
	DECLARE_ENGINE_CLASS( CQCAttitude, IActorConditionType, 0 )

private:
	EAIAttitude			m_attitude;

public:
	CQCAttitude();

protected:
	//! IActorConditionType implementation
	virtual Bool EvaluateImpl( CActor* actor ) override;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Attitude" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCAttitude )
	PARENT_CLASS( IActorConditionType )
	PROPERTY_EDIT( m_attitude, TXT( "Attitude the actor should have" ) );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
class CQCDistanceTo : public IActorConditionType, public IGlobalEventsListener
{
	DECLARE_ENGINE_CLASS( CQCDistanceTo, IActorConditionType, 0 )

private:
	CName				m_targetNodeTag;
	ECompareFunc		m_compareFunc;
	Float				m_distance;
	THandle< CNode >	m_cachedNode;
	Bool				m_wasRegistered;

public:
	CQCDistanceTo();
	virtual ~CQCDistanceTo();

protected:
	
	//! IActorConditionType implementation
	virtual void OnActivate( CActor* actor ) override;
	virtual void OnDeactivate( CActor* actor ) override;
	virtual Bool EvaluateImpl( CActor* actor ) override;

	//! IGlobalEventListener
	virtual void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;
	
	Bool RegisterCallback( Bool reg );
	void FindTargetNode();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return TXT( "Attitude" ); }
#endif
};

BEGIN_CLASS_RTTI( CQCDistanceTo )
	PARENT_CLASS( IActorConditionType )
	PROPERTY_EDIT( m_targetNodeTag, TXT( "Required tags of a target node" ) );
	PROPERTY_EDIT( m_compareFunc, TXT( "The way how the distance should be compared" ) );
	PROPERTY_EDIT_RANGE( m_distance, TXT( "Distance to compare against" ), FLT_EPSILON, FLT_MAX );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
