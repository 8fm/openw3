/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphVariableBaseNode.h"

#define BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE 15

// class defining variable value
class CBehaviorGraphVariableNode : public CBehaviorGraphVariableBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphVariableNode, CBehaviorGraphVariableBaseNode, "Float", "Variable" );

protected:
	CName m_variableName;		//!< Name of the variable

public:
	CBehaviorGraphVariableNode();

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
	virtual const CName& GetVariableName() const override { return m_variableName; }

};

BEGIN_CLASS_RTTI( CBehaviorGraphVariableNode );
	PARENT_CLASS( CBehaviorGraphVariableBaseNode );
	PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorVariableSelection") );
END_CLASS_RTTI();
