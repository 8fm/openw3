/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeSelector.h"
#include "behTreeNodeConditionReactionEvent.h"

class CBehTreeReactionEventData;
class CBehTreeNodeReactionSceneDefinitionDecoratorInstance;
	////////////////////////////////////////////////////////////////////////
	// Definition
	////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSceneDefinitionDecorator : public CBehTreeNodeConditionReactionEventDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeReactionSceneDefinitionDecorator, CBehTreeNodeConditionReactionEventDefinition, CBehTreeNodeReactionSceneDefinitionDecoratorInstance, ReactionSceneDefinitionDecorator );
protected:
	TDynArray< CName >		m_roles;		
	CBehTreeValBool			m_inInWorkBranch;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeReactionSceneDefinitionDecorator() : m_inInWorkBranch( false ) {}
};

BEGIN_CLASS_RTTI( CBehTreeNodeReactionSceneDefinitionDecorator );
	PARENT_CLASS( CBehTreeNodeConditionReactionEventDefinition );	
	PROPERTY_EDIT( m_roles, TXT("Roles needed to this scene") );	
	PROPERTY_EDIT( m_inInWorkBranch, TXT( "Accept poke event only if node is active") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeReactionSceneDefinitionDecoratorInstance : public CBehTreeNodeConditionReactionEventInstance
{
	typedef CBehTreeNodeConditionReactionEventInstance Super;
protected:
	TDynArray< CName >				m_roles;	
	CBehTreeCounterDataPtr			m_workBeingPerformed;
	CBehTreeCounterDataPtr			m_softReactionLocker;
	bool							m_inInWorkBranch;

public:
	typedef CBehTreeNodeReactionSceneDefinitionDecorator Definition;

	CBehTreeNodeReactionSceneDefinitionDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	void OnGenerateDebugFragments( CRenderFrame* frame ) override;
	void Deactivate() override;
	Bool Activate() override;
	Bool ConditionCheck() override;

	virtual Bool CanBeAssignedToScene( CBehTreeReactionEventData* reactionData );
};