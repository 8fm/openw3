/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphVectorVariableNode.h"
#include "behaviorGraphOutput.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVectorVariableNode );

CBehaviorGraphVectorVariableNode::CBehaviorGraphVectorVariableNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphVectorVariableNode::GetCaption() const
{
	if ( m_variableName != CName::NONE )
	{
		return String::Printf( TXT("Vector variable [ %s ]"), m_variableName.AsChar() );
	}

	return TXT("Vector variable");
}

#endif

Vector CBehaviorGraphVectorVariableNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{	
	DEBUG_ANIM_VECTOR( instance.GetVectorValue( m_variableName ) );
	return instance.GetVectorValue( m_variableName );
}

void CBehaviorGraphVectorVariableNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	vecVar.PushBack( m_variableName );
}

