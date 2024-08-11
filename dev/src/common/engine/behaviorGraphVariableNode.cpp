/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphVariableNode.h"
#include "behaviorGraphOutput.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVariableNode );

CBehaviorGraphVariableNode::CBehaviorGraphVariableNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphVariableNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT("variableName") )
	{		
		CBehaviorGraph *graph = GetGraph();

		if ( !graph->GetVariables().GetVariable( m_variableName ) && m_variableName != CName::NONE )
		{
			graph->GetVariables().AddVariable( m_variableName );
		}
	}
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphVariableNode::GetCaption() const
{
	if ( !m_variableName.Empty() )
	{
		return String::Printf( TXT("Variable [ %s ]"), m_variableName.AsChar() );
	}

	return TXT("Variable");
}

void CBehaviorGraphVariableNode::SetValue( CBehaviorGraphInstance& instance, const Float& value ) const
{
	instance.SetFloatValue( m_variableName, value );
}

Float CBehaviorGraphVariableNode::GetMin( CBehaviorGraphInstance& instance ) const
{
	return instance.GetFloatValueMin( m_variableName );
}

Float CBehaviorGraphVariableNode::GetMax( CBehaviorGraphInstance& instance ) const
{
	return instance.GetFloatValueMax( m_variableName );
}

Float CBehaviorGraphVariableNode::GetEditorValue( CBehaviorGraphInstance& instance ) const
{
	return instance.GetFloatValue( m_variableName );
}

#endif

Float CBehaviorGraphVariableNode::GetValue( CBehaviorGraphInstance& instance ) const
{	
	DEBUG_ANIM_FLOAT( instance.GetFloatValue( m_variableName ) );
	return instance.GetFloatValue( m_variableName );
}


void CBehaviorGraphVariableNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	var.PushBack( m_variableName );
}
