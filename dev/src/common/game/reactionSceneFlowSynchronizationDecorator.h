/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeReactionSFlowSynchroDecoratorInstance;

	////////////////////////////////////////////////////////////////////////
	// Definition
	////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSFlowSynchroDecoratorDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeReactionSFlowSynchroDecoratorDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeReactionSFlowSynchroDecoratorInstance, ReactionSceneFlowSynchro );
protected:
	CName	m_allowedRole;
	Bool	m_isBlocikng;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	
};

BEGIN_CLASS_RTTI( CBehTreeNodeReactionSFlowSynchroDecoratorDefinition );
	PARENT_CLASS( CBehTreeNodeConditionDefinition );
	PROPERTY_EDIT( m_allowedRole, TXT("Role that can use this branch") );
	PROPERTY_EDIT( m_isBlocikng, TXT("If this branch will block next branches") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSFlowSynchroDecoratorInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CName	m_allowedRole;
	Bool	m_isBlocikng;
	Bool	m_complete;
	

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeReactionSFlowSynchroDecoratorDefinition Definition;

	CBehTreeNodeReactionSFlowSynchroDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
			
	RED_INLINE Bool IsBlocking(){ return m_isBlocikng; }

	CBehTreeNodeReactionSFlowSynchroDecoratorInstance* AsCBehTreeNodeReactionSFlowSynchroDecorator() override { return this; }
};