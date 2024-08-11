/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphValueNode.h"

#define BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE 15

// base class for nodes that refer to variables (normal and internal)
class CBehaviorGraphVariableBaseNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphVariableBaseNode, CBehaviorGraphValueNode );

public:
	CBehaviorGraphVariableBaseNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual void SetValue( CBehaviorGraphInstance& instance, const Float& value ) const = 0;
	virtual Float GetMin( CBehaviorGraphInstance& instance ) const = 0;
	virtual Float GetMax( CBehaviorGraphInstance& instance ) const = 0;
	virtual Float GetEditorValue( CBehaviorGraphInstance& instance ) const = 0;

	virtual Bool IsDraggedByClickOnInnerArea() const { return false; }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

public:
	virtual const CName& GetVariableName() const = 0;

};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphVariableBaseNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
END_CLASS_RTTI();
