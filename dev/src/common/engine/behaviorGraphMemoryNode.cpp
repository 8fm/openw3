/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphMemoryNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMemoryNode );

CBehaviorGraphMemoryNode::CBehaviorGraphMemoryNode()
	: m_resetOnActivation( true )
{
}

void CBehaviorGraphMemoryNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currValue;
	compiler << i_prevValue;
}

void CBehaviorGraphMemoryNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_currValue ] = 0.f;
	instance[ i_prevValue ] = 0.f;
}

void CBehaviorGraphMemoryNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_currValue );
	INST_PROP( i_prevValue );
}

void CBehaviorGraphMemoryNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Memory );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_prevValue ] = instance[ i_currValue ];
	instance[ i_currValue ] = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;
}

Float CBehaviorGraphMemoryNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_prevValue ];
}

void CBehaviorGraphMemoryNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_currValue ] = 0.f;
	instance[ i_prevValue ] = 0.f;
}

void CBehaviorGraphMemoryNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_resetOnActivation )
	{
		instance[ i_currValue ] = 0.f;
		instance[ i_prevValue ] = 0.f;
	}
}

String CBehaviorGraphMemoryNode::GetCaption() const
{
	return TXT("Memory");
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphValueAccNode );

CBehaviorGraphValueAccNode::CBehaviorGraphValueAccNode()
	: m_resetOnActivation( false )
	, m_initValue( 0.f )
	, m_wrapValue( false )
	, m_wrapValueThrMin( -180.f )
	, m_wrapValueThrMax( 180.f )
{
}

void CBehaviorGraphValueAccNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_accValue;
}

void CBehaviorGraphValueAccNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_accValue ] = m_initValue;
}

void CBehaviorGraphValueAccNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_accValue );
}

void CBehaviorGraphValueAccNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( ValueAcc );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float& accValue = instance[ i_accValue ];
	accValue += m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;

	if ( m_wrapValue )
	{
		// TODO
		// accValue...
	}
}

Float CBehaviorGraphValueAccNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_accValue ];
}

String CBehaviorGraphValueAccNode::GetCaption() const
{
	return TXT("Acc");
}

void CBehaviorGraphValueAccNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_accValue ] = m_initValue;
}

void CBehaviorGraphValueAccNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_resetOnActivation )
	{
		instance[ i_accValue ] = m_initValue;
	}
}
