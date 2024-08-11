/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphVectorVariableBaseNode.h"

class CBehaviorGraphValueNode;

// --------------------------------------------------------------------------------------------------------------------
// class defining variable value
class CBehaviorGraphInternalVectorVariableNode : public CBehaviorGraphVectorVariableBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphInternalVectorVariableNode, CBehaviorGraphVectorVariableBaseNode, "Vector", "Internal vector variable" );

protected:
	CName m_internalVariableName;

public:
	CBehaviorGraphInternalVectorVariableNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual String GetCaption() const;

#endif

public:
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

public:
	virtual const CName& GetVariableName() const override { return m_internalVariableName; }

};

BEGIN_CLASS_RTTI( CBehaviorGraphInternalVectorVariableNode );
	PARENT_CLASS( CBehaviorGraphVectorVariableBaseNode );
	PROPERTY_CUSTOM_EDIT( m_internalVariableName, TXT("Variable name"), TXT("BehaviorInternalVectorVariableSelection") );
END_CLASS_RTTI();

