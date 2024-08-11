/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

class CBehaviorGraphValueNode;
class CBehaviorGraphValueBaseNode;

#include "behaviorGraphValueNode.h"

// this is identical to CBehaviorGraphSetInternalVariableNode except for the fact it has different input and output
// but because of that it has to derive from CBehaviorGraphValueBaseNode - this might be only case so it's fine
// but in general maybe we should have interface IBehaviorGraphValueNode?
class CBehaviorGraphSetInternalVariableFloatInOutNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSetInternalVariableFloatInOutNode, CBehaviorGraphValueBaseNode, "Float.Set Internal Variable", "Float value in/out" );

protected:
	CName						m_variableName;		//!< Name of the variable
	Bool						m_setWhenActivatedOnly;

protected:
	CBehaviorGraphValueNode*	m_cachedValueNode;

public:
	CBehaviorGraphSetInternalVariableFloatInOutNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

public:
	const CName& GetVariableName() const { return m_variableName; }

};

BEGIN_CLASS_RTTI( CBehaviorGraphSetInternalVariableFloatInOutNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorInternalVariableSelection") );
	PROPERTY( m_cachedValueNode );
END_CLASS_RTTI();
