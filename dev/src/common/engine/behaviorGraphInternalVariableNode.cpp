/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphInternalVariableNode.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphInternalVariableNode );

CBehaviorGraphInternalVariableNode::CBehaviorGraphInternalVariableNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphInternalVariableNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT("internalVariableName") )
	{		
		CBehaviorGraph *graph = GetGraph();

		if ( !graph->GetInternalVariables().GetVariable( m_internalVariableName ) && m_internalVariableName != CName::NONE )
		{
			graph->GetInternalVariables().AddVariable( m_internalVariableName );
		}
	}
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphInternalVariableNode::GetCaption() const
{
	if ( m_internalVariableName != CName::NONE )
	{
		return String::Printf( TXT("Internal variable [ %s ]"), m_internalVariableName.AsChar() );
	}

	return TXT("Internal variable");
}

void CBehaviorGraphInternalVariableNode::SetValue( CBehaviorGraphInstance& instance, const Float& value ) const
{
	instance.SetInternalFloatValue( m_internalVariableName, value );
}

Float CBehaviorGraphInternalVariableNode::GetMin( CBehaviorGraphInstance& instance ) const
{
	return instance.GetInternalFloatValueMin( m_internalVariableName );
}

Float CBehaviorGraphInternalVariableNode::GetMax( CBehaviorGraphInstance& instance ) const
{
	return instance.GetInternalFloatValueMax( m_internalVariableName );
}

Float CBehaviorGraphInternalVariableNode::GetEditorValue( CBehaviorGraphInstance& instance ) const
{
	return instance.GetInternalFloatValue( m_internalVariableName );
}

#endif

Float CBehaviorGraphInternalVariableNode::GetValue( CBehaviorGraphInstance& instance ) const
{	
	return instance.GetInternalFloatValue( m_internalVariableName );
}


void CBehaviorGraphInternalVariableNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	intVar.PushBack( m_internalVariableName );
}
