#pragma once

#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/behTreeVars.h"
#include "../../common/game/behTreeNodeCondition.h"

#include "behTreeCarryingItemData.h"

#include "../../common/game/behTreeCustomMoveData.h"

class CBehTreeNodeDecoratorCarryingItemManagerInstance;
class CBehTreeDecoratorCarryingItemsBaseInstance;
class CBehTreeNodeConditionIsCarryingItemInstance;

////////////////////////////////////////////////////////////////////////
// Carry item manager
class CBehTreeNodeDecoratorCarryingItemManagerDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorCarryingItemManagerDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorCarryingItemManagerInstance, CarryingItemManager );

	CBehTreeNodeDecoratorCarryingItemManagerDefinition() : m_dropOnDeactivation( true ){}
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

	CBehTreeValBool	m_dropOnDeactivation;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorCarryingItemManagerDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_dropOnDeactivation, TXT("If item should be dropped") );
END_CLASS_RTTI();


class CBehTreeNodeDecoratorCarryingItemManagerInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCarryingItemDataPtr		m_carryingItemsData;
	Bool							m_dropOnDeactivation;

public:
	typedef CBehTreeNodeDecoratorCarryingItemManagerDefinition Definition;

	CBehTreeNodeDecoratorCarryingItemManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	void OnDestruction() override;

	Bool Activate() override;
	void Deactivate() override;	
	Bool Interrupt() override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	Bool OnEvent( CBehTreeEvent& e );
private:
	void DropImmediate();
	
};

////////////////////////////////////////////////////////////////////////


class CBehTreeDecoratorCarryingItemsBaseDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorCarryingItemsBaseDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorCarryingItemsBaseInstance, BaseCarryingItemDecorator );
protected:
	CName	m_carryingAreaName_var;
	CBehTreeValCName m_storeTag;

public:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorCarryingItemsBaseDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
	PROPERTY_EDIT( m_carryingAreaName_var, TXT("") );
	PROPERTY_EDIT( m_storeTag,	TXT("") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
class CBehTreeDecoratorCarryingItemsBaseInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCustomMoveDataPtr	m_customMoveData;
	THandle< CAreaComponent >	m_carryingArea;
	CName							m_storeTag;
	CBehTreeCarryingItemDataPtr		m_carryingItemsData;

	virtual Bool IfStorePointValid( CCarryableItemStorePointComponent* storePoint, CBehTreeCarryingItemData* carryingData ) { return false; }
	virtual void SetTargetMoveData( CCarryableItemStorePointComponent* storePoint ){}

public:
	typedef CBehTreeDecoratorCarryingItemsBaseDefinition Definition;

	CBehTreeDecoratorCarryingItemsBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );			

	Bool Activate() override;
	Bool IsAvailable() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

private:
	Bool FindNewPointToStoreItem();
};


////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsCarryingItemDefinition: public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsCarryingItemDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionIsCarryingItemInstance, ConditionIsCarryingItem );

protected:	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionIsCarryingItemDefinition() 
		: CBehTreeNodeConditionDefinition()		
	{
	}	
};


BEGIN_CLASS_RTTI(CBehTreeNodeConditionIsCarryingItemDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
END_CLASS_RTTI();

class CBehTreeNodeConditionIsCarryingItemInstance : public CBehTreeNodeConditionInstance
{
protected:	
	CBehTreeCarryingItemDataPtr		m_carryingItemsData;

	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeConditionIsCarryingItemDefinition Definition;

	CBehTreeNodeConditionIsCarryingItemInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );	
};