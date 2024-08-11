/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCombatTargetSelectionBase.h"

class CBehTreeNodeClosestNonFriendlyTargetSelectionInstance;


////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition : public IBehTreeNodeCombatTargetSelectionBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition, IBehTreeNodeCombatTargetSelectionBaseDefinition, CBehTreeNodeClosestNonFriendlyTargetSelectionInstance, ClosestNonFriendlyTargetSelection );
protected:
	Float						m_testDelay;
public:
	CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition()
		: m_testDelay( 2.f )											{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition )
	PARENT_CLASS( IBehTreeNodeCombatTargetSelectionBaseDefinition )
	PROPERTY_EDIT( m_testDelay, TXT("Minimum test delay between 'closest target test'. Notice its performance heavy.") )
END_CLASS_RTTI()


////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeClosestNonFriendlyTargetSelectionInstance : public IBehTreeNodeCombatTargetSelectionBaseInstance
{
	typedef IBehTreeNodeCombatTargetSelectionBaseInstance Super;
protected:
	Float					m_testDelay;
	Float					m_choiceTimeout;
	THandle< CActor >		m_currentChoice;

	CActor*					FindTarget();
public:
	typedef CBehTreeNodeClosestNonFriendlyTargetSelectionDefinition Definition;

	CBehTreeNodeClosestNonFriendlyTargetSelectionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void					OnDestruction() override;

	Bool					IsAvailable() override;

	Bool					Activate() override;
	void					Update() override;

	Bool					OnListenedEvent( CBehTreeEvent& e ) override;

};

