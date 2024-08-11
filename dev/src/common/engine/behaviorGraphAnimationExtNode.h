/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorGraphAnimationNode.h"

class CBehaviorGraphAnimationExtNode : public CBehaviorGraphAnimationNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationExtNode, CBehaviorGraphAnimationNode, "Animation", "Animation ext" );	

protected:
	Float		m_animStartOffset;
	Float		m_fireLoopEventBackOffset;

public:
	CBehaviorGraphAnimationExtNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationExtNode );
	PARENT_CLASS( CBehaviorGraphAnimationNode );
	PROPERTY_EDIT( m_animStartOffset, TXT("") );
	PROPERTY_EDIT( m_fireLoopEventBackOffset, TXT("") );
END_CLASS_RTTI();
