/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphInternalVectorVariableNode.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphInternalVectorVariableNode );

CBehaviorGraphInternalVectorVariableNode::CBehaviorGraphInternalVectorVariableNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphInternalVectorVariableNode::GetCaption() const
{
	if ( m_internalVariableName != CName::NONE )
	{
		return String::Printf( TXT("Internal vector variable [ %s ]"), m_internalVariableName.AsChar() );
	}

	return TXT("Internal vector variable");
}

#endif

Vector CBehaviorGraphInternalVectorVariableNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{	
	DEBUG_ANIM_VECTOR( instance.GetInternalVectorValue( m_internalVariableName ) );
	return instance.GetInternalVectorValue( m_internalVariableName );
}

void CBehaviorGraphInternalVectorVariableNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	intVecVar.PushBack( m_internalVariableName );
}
