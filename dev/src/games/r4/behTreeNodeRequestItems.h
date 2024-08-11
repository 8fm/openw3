/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeDecorator.h"

class CBehTreeNodeRequestItemsInstance;

class CBehTreeNodeRequestItemsDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeRequestItemsDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeRequestItemsInstance, RequestItems )
protected:
	CBehTreeValCName m_LeftItemType;
	CBehTreeValCName m_RightItemType;
	CBehTreeValBool m_chooseSilverIfPossible;
	CName m_behaviorGraphVarName;
	

	virtual IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeRequestItemsDefinition()																		{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeRequestItemsDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_LeftItemType, TXT("Left item category") )
	PROPERTY_EDIT( m_RightItemType, TXT("Right item category") )
	PROPERTY_EDIT( m_chooseSilverIfPossible, TXT("Try choosing silver sword") )
	PROPERTY_EDIT( m_behaviorGraphVarName, TXT("Combat style id variable name") )
END_CLASS_RTTI()


class CBehTreeNodeRequestItemsInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	enum EExecutionState
	{
		STATE_OK,
		STATE_INITIAL,
		STATE_PROCESS_REQUESTS,
		STATE_SHEATHE,
		STATE_SHIELD_START,
		STATE_SHIELD,
		STATE_DRAW_START,
		STATE_DRAW
	};

	CName								m_LeftItemType;
	CName								m_RightItemType;

	Bool								m_chooseSilverIfPossible;
	Bool								m_witcherSwordButTakenOutFromHip;

	Bool								m_noSheatheAnimation;
	Bool								m_witcherSwordSelection;

	Bool								m_processLeftItem;
	Bool								m_processRightItem;

	Bool								m_itemsAreAvailable;

	Bool								m_takeBowArrow;
	Bool								m_takeBolt;

	Bool								m_flaggedItemsProcessing;
	Bool								m_flaggedItemsIdleProcessing;

	EExecutionState						m_executionState;

	Uint32								m_inventoryStateHash;

	void SelectWitcherWeapon( Float weaponType );

	void BeginItemProcessingAction( CName leftItem, CName rightItem );

	Bool DetermineRequiredItemsInternal();
	Bool DetermineRequiredItems();
	Bool CheckForRequiredItems();

	void StartItemsProcessing();
	void StopItemsProcessing();

	void RequestIdleProcessing();
	void CancelRequestIdleProcessing();

public:
	typedef CBehTreeNodeRequestItemsDefinition Definition;

	CBehTreeNodeRequestItemsInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	Bool IsAvailable() override;
	Int32 Evaluate() override;

	////////////////////////////////////////////////////////////////////
	Bool Activate() override;
	void Deactivate() override;
	void Update() override;
	Bool OnEvent( CBehTreeEvent& e ) override;

};

