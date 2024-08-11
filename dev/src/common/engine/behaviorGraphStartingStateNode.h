/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionBlend.h"

class CBehaviorGraphAnimationNode;

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphFlowTransitionNode : public CBehaviorGraphStateTransitionBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphFlowTransitionNode, CBehaviorGraphStateTransitionBlendNode, "State machine", "Flow transition" );

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual Color GetClientColor() const;
#endif

public:
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool IsManualCreationAllowed() const { return false; }

public:
	Bool GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphFlowTransitionNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphFlowConnectionNode : public CBehaviorGraphStateNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphFlowConnectionNode, CBehaviorGraphStateNode, "State machine", "Flow" );

protected:
	CName							m_stateID;

protected:
	CBehaviorGraphAnimationNode*	m_animNode;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
	virtual Bool CanBeExpanded() const;

	Bool HasAnimation() const;
#endif

public:
	CBehaviorGraphFlowConnectionNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	Bool GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const;
	Bool CanSynchronizeIntanceTo( CBehaviorGraphInstance& instance, const CBehaviorSyncInfo& info ) const;
	Bool SynchronizeInstanceTo( CBehaviorGraphInstance& instance, const CBehaviorSyncInfo& info ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphFlowConnectionNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
	PROPERTY_EDIT( m_stateID, TXT("State ID from previous instance") );
	PROPERTY_INLINED( m_animNode, TXT("Animation node") );
END_CLASS_RTTI();
