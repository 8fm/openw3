#pragma once
#include "questcondition.h"
#include "inventoryComponent.h"
#include "entityWatcher.h"

class IGameplayEntConditionType : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IGameplayEntConditionType, CObject )

public:
	IGameplayEntConditionType() {}
	virtual ~IGameplayEntConditionType() {}

	virtual void OnActivate( CGameplayEntity& entity ) { }
	virtual void OnDeactivate( CGameplayEntity& entity ) { }
	virtual void OnEntityLost() { } // for example: entity was despawned
	virtual Bool Evaluate( CGameplayEntity& entity ) { return true; }

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const { return String::EMPTY; }
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( IGameplayEntConditionType )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()


class CQuestGameplayEntCondition : public IQuestCondition
{
public:
	DECLARE_ENGINE_CLASS( CQuestGameplayEntCondition, IQuestCondition, 0 )

	CQuestGameplayEntCondition();
	~CQuestGameplayEntCondition();

	CName									m_entityTag;
	IGameplayEntConditionType *				m_checkType;

protected:
	typedef TEntityWatcher< CGameplayEntity >	TGameplayEnityWatcher;
	TGameplayEnityWatcher*					m_entityWatcher;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Entity tag: %s, Gameplay ent Condition: %s" ), 
			m_entityTag.AsString().AsChar(), ( m_checkType ) ? m_checkType->GetDescription().AsChar() : TXT( "None" ) ); 
	}
#endif

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual void OnDeactivate();
	virtual Bool OnIsFulfilled();	
};

BEGIN_CLASS_RTTI( CQuestGameplayEntCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_entityTag, TXT( "Tag of an gameplay entity" ) )
	PROPERTY_INLINED( m_checkType, TXT( "Condition" ) )
END_CLASS_RTTI()



class CQCHasItemGE : public IGameplayEntConditionType, public IInventoryListener
{
	DECLARE_ENGINE_CLASS( CQCHasItemGE, IGameplayEntConditionType, 0 )

private:
	CName						m_item;
	CName						m_itemCategory;
	CName						m_itemTag;
	Uint32						m_quantity;
	ECompareFunc				m_compareFunc;

	Bool						m_wasRegistered;
	Bool						m_isFulfilled;

protected:
	CQCHasItemGE();

	//! IGameplayEntConditionType
	virtual void OnActivate( CGameplayEntity& entity ) override;
	virtual void OnDeactivate( CGameplayEntity& entity ) override;
	virtual void OnEntityLost() override;
	virtual Bool Evaluate( CGameplayEntity& entity ) override;

	Bool RegisterCallback( CGameplayEntity& entity, Bool reg );
	Bool EvaluateImpl( CInventoryComponent* inventory );

	//! IInventoryListener
	virtual void OnInventoryEvent( CInventoryComponent* inventory, EInventoryEventType eventType, SItemUniqueId itemId, Int32 quantity, Bool fromAssociatedInventory ) override;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Has Item - %s, %d" ),
			m_item.AsString().AsChar(), m_quantity ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQCHasItemGE )
	PARENT_CLASS( IGameplayEntConditionType )
	PROPERTY_EDIT( m_item, TXT( "An item in the inventory of entity" ) )
	PROPERTY_EDIT( m_itemCategory, TXT( "Category of item in the inventory of entity" ) )
	PROPERTY_EDIT( m_itemTag, TXT( "Tag of item in the inventory of entity" ) )
	PROPERTY_EDIT( m_quantity, TXT( "How many of those items" ) )
	PROPERTY_EDIT( m_compareFunc, TXT("How to compare values.") );
END_CLASS_RTTI()

/////////////////////////////////////////////////////////////////////////////// 

class CQCAnimationState : public IGameplayEntConditionType, public IAnimationSetListener
{
	DECLARE_ENGINE_CLASS( CQCAnimationState, IGameplayEntConditionType, 0 )

private:
	CName						m_animationName;
	Bool						m_isPlaying;

	Bool						m_wasRegistered;
	Bool						m_isFulfilled;

protected:
	CQCAnimationState();

	//! IGameplayEntConditionType
	virtual void OnActivate( CGameplayEntity& entity ) override;
	virtual void OnDeactivate( CGameplayEntity& entity ) override;
	virtual void OnEntityLost() override;
	virtual Bool Evaluate( CGameplayEntity& entity ) override;

	Bool RegisterCallback( CGameplayEntity& entity, Bool reg );
	Bool EvaluateImpl( CPropertyAnimationSet* );

	//! IAnimationSetListener
	virtual void OnStateChanged( CPropertyAnimationSet* animationSet ) override;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Animation state - %s, %d" ), m_animationName.AsString().AsChar(), m_isPlaying ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQCAnimationState )
	PARENT_CLASS( IGameplayEntConditionType )
	PROPERTY_EDIT( m_animationName, TXT( "Played animation" ) )
	PROPERTY_EDIT( m_isPlaying, TXT( "State to check" ) )
END_CLASS_RTTI()
