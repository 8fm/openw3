/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

class CBehaviorGraphValueNode;
class CBehaviorGraphBaseNode;

#include "behaviorGraphNode.h"

class CBehaviorGraphSetInternalVariableNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSetInternalVariableNode, CBehaviorGraphBaseNode, "Float.Set Internal Variable", "Animation in/out" );

protected:
	CName						m_variableName;					//!< Name of the variable
	Bool						m_setValueOnActivationAsWell;
	Bool						m_setValueBeforeInputIsUpdated;

protected:
	CBehaviorGraphValueNode*	m_cachedValueNode;

public:
	CBehaviorGraphSetInternalVariableNode();

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

BEGIN_CLASS_RTTI( CBehaviorGraphSetInternalVariableNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorInternalVariableSelection") );
	PROPERTY_EDIT( m_setValueOnActivationAsWell, String::EMPTY );
	PROPERTY_EDIT( m_setValueBeforeInputIsUpdated, String::EMPTY );
	PROPERTY( m_cachedValueNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphInternalVariableCounterNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphInternalVariableCounterNode, CBehaviorGraphBaseNode, "Float.Set Internal Variable", "Counter" );

protected:
	CName		m_variableName;
	Bool		m_countOnActivation;
	Bool		m_countOnDeactivation;
	Float		m_stepValue;

public:
	CBehaviorGraphInternalVariableCounterNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

public:
	const CName& GetVariableName() const { return m_variableName; }

};

BEGIN_CLASS_RTTI( CBehaviorGraphInternalVariableCounterNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorInternalVariableSelection") );
	PROPERTY_EDIT( m_countOnActivation, String::EMPTY );
	PROPERTY_EDIT( m_countOnDeactivation, String::EMPTY );
	PROPERTY_EDIT( m_stepValue, String::EMPTY );
END_CLASS_RTTI();
