/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphFillMovementVariablesUsingSteeringNode;

///////////////////////////////////////////////////////////////////////////////

/**
 *	Setup movement variables using steering output
 */
class CBehaviorGraphFillMovementVariablesUsingSteeringNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphFillMovementVariablesUsingSteeringNode, CBehaviorGraphBaseNode, "Movement", "Fill movement variables using steering output" );

protected:
	// in pairs - should we fill it? name of variable to fill
	Bool m_fillRequestedMovementDirectionWSVariable;
	CName m_requestedMovementDirectionWSVariableName;
	Bool m_fillRequestedFacingDirectionWSVariable;
	CName m_requestedFacingDirectionWSVariableName;

public:
	CBehaviorGraphFillMovementVariablesUsingSteeringNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Fill movement variables using steering output"); }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

private:
	RED_INLINE void FillVariables( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphFillMovementVariablesUsingSteeringNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_fillRequestedMovementDirectionWSVariable, TXT( "Should we fill it?" ) )
	PROPERTY_CUSTOM_EDIT( m_requestedMovementDirectionWSVariableName, TXT("Requested movement direction variable name"), TXT("BehaviorVariableSelection") );
	PROPERTY_EDIT( m_fillRequestedFacingDirectionWSVariable, TXT( "Should we fill it?" ) )
	PROPERTY_CUSTOM_EDIT( m_requestedFacingDirectionWSVariableName, TXT("Requested facing direction variable name"), TXT("BehaviorVariableSelection") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
