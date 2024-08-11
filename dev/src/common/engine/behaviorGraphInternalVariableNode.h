/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphVariableBaseNode.h"



// class defining internal variable value
class CBehaviorGraphInternalVariableNode : public CBehaviorGraphVariableBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphInternalVariableNode, CBehaviorGraphVariableBaseNode, "Float", "Internal Variable" );

protected:
	CName m_internalVariableName;		//!< Name of the variable

public:
	CBehaviorGraphInternalVariableNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *prop );

	virtual String GetCaption() const;
	virtual void SetValue( CBehaviorGraphInstance& instance, const Float& value ) const;
	virtual Float GetMin( CBehaviorGraphInstance& instance ) const;
	virtual Float GetMax( CBehaviorGraphInstance& instance ) const;
	virtual Float GetEditorValue( CBehaviorGraphInstance& instance ) const;
#endif

public:
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

public:
	virtual const CName& GetVariableName() const { return m_internalVariableName; }

};

BEGIN_CLASS_RTTI( CBehaviorGraphInternalVariableNode );
	PARENT_CLASS( CBehaviorGraphVariableBaseNode );
	PROPERTY_CUSTOM_EDIT( m_internalVariableName, TXT("Internal variable name"), TXT("BehaviorInternalVariableSelection") );
END_CLASS_RTTI();
