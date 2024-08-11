/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphVectorVariableBaseNode.h"

class CBehaviorGraphValueNode;

// --------------------------------------------------------------------------------------------------------------------
// class defining variable value
class CBehaviorGraphVectorVariableNode : public CBehaviorGraphVectorVariableBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphVectorVariableNode, CBehaviorGraphVectorVariableBaseNode, "Vector", "Vector variable" );

protected:
	CName m_variableName;

public:
	CBehaviorGraphVectorVariableNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual String GetCaption() const;

#endif

public:
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

public:
	virtual const CName& GetVariableName() const override { return m_variableName; }

};

BEGIN_CLASS_RTTI( CBehaviorGraphVectorVariableNode );
	PARENT_CLASS( CBehaviorGraphVectorVariableBaseNode );
	PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorVectorVariableSelection") );
END_CLASS_RTTI();
