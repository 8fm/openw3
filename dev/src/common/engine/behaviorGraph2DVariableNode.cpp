/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraph2DVariableNode.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraph2DVectorVariableNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraph2DVariableNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraph2DMultiVariablesNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraph2DVectorVariableNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT("variableName") )
	{
		CBehaviorGraph *graph = GetGraph();

		if ( !graph->GetVariables().GetVariable( m_variableName ) && !m_variableName.Empty() )
		{
			graph->GetVariables().AddVariable( m_variableName );
		}
	}
}

#endif

Vector CBehaviorGraph2DVectorVariableNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{	
	return instance.GetVectorValue( m_variableName );
}
void CBehaviorGraph2DVectorVariableNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	var.PushBack( m_variableName );
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
Vector CBehaviorGraph2DVectorVariableNode::GetMinVal( CBehaviorGraphInstance& instance ) const
{
	return instance.GetVectorValueMin( m_variableName );
}
Vector CBehaviorGraph2DVectorVariableNode::GetMaxVal( CBehaviorGraphInstance& instance ) const
{
	return instance.GetVectorValueMax( m_variableName );
}

void CBehaviorGraph2DVectorVariableNode::SetVectorValue( CBehaviorGraphInstance& instance, const Vector& val )
{
	instance.SetVectorValue( m_variableName, val );
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraph2DMultiVariablesNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT("variableName1") )
	{
		CBehaviorGraph *graph = GetGraph();
		if ( !graph->GetVariables().GetVariable( m_variableName1 ) && !m_variableName1.Empty() )
		{
			graph->GetVariables().AddVariable( m_variableName1 );
		}
	}
	if ( prop->GetName() == TXT("variableName2") )
	{
		CBehaviorGraph *graph = GetGraph();
		if ( graph->GetVariables().GetVariable( m_variableName2 ) && !m_variableName2.Empty() )
		{
			graph->GetVariables().AddVariable( m_variableName2 );
		}
	}
}

#endif

Vector CBehaviorGraph2DMultiVariablesNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{	
	return Vector(
		instance.GetFloatValue( m_variableName1 ),
		instance.GetFloatValue( m_variableName2 ),
		0.0f );
}

void CBehaviorGraph2DMultiVariablesNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	var.PushBack( m_variableName1 );
	var.PushBack( m_variableName2 );
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
Vector CBehaviorGraph2DMultiVariablesNode::GetMinVal( CBehaviorGraphInstance& instance ) const
{
	return Vector(
		instance.GetFloatValueMin( m_variableName1 ),
		instance.GetFloatValueMin( m_variableName2 ),
		0.0f );
}
Vector CBehaviorGraph2DMultiVariablesNode::GetMaxVal( CBehaviorGraphInstance& instance ) const
{
	return Vector(
		instance.GetFloatValueMax( m_variableName1 ),
		instance.GetFloatValueMax( m_variableName2 ),
		0.0f );
}

void CBehaviorGraph2DMultiVariablesNode::SetVectorValue( CBehaviorGraphInstance& instance, const Vector& val )
{
	instance.SetFloatValue( m_variableName1, val.X );
	instance.SetFloatValue( m_variableName2, val.Y );
}
#endif
