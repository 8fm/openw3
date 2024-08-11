/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphValueNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphValueNode );

CBehaviorGraphValueNode::CBehaviorGraphValueNode()
{
}

Color CBehaviorGraphValueNode::GetTitleColor() const
{
	return Color( 64, 64, 255 );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphValueBaseNode );

void CBehaviorGraphValueBaseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
}

void CBehaviorGraphValueBaseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_value ] = 0.f;
}

void CBehaviorGraphValueBaseNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_value );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphValueBaseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );
}

#endif

void CBehaviorGraphValueBaseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

Float CBehaviorGraphValueBaseNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	RED_ASSERT( IsActive( instance ) );

	return instance[ i_value ];
}

void CBehaviorGraphValueBaseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphValueBaseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphValueBaseNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphValueBaseNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_value ] = 0.f;
}

void CBehaviorGraphValueBaseNode::CacheConnections()
{
	TBaseClass::CacheConnections();
	m_cachedInputNode = CacheValueBlock( TXT("Input") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVectorValueNode );

Color CBehaviorGraphVectorValueNode::GetTitleColor() const
{
	return Color( 64, 200, 255 );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVectorValueBaseNode );

void CBehaviorGraphVectorValueBaseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
}

void CBehaviorGraphVectorValueBaseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_value ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphVectorValueBaseNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_value );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphVectorValueBaseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Output ) ) );
}

#endif

void CBehaviorGraphVectorValueBaseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

Float CBehaviorGraphVectorValueBaseNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	RED_ASSERT( IsActive( instance ) );

	return 0.f;
}

Vector CBehaviorGraphVectorValueBaseNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	RED_ASSERT( IsActive( instance ) );

	return instance[ i_value ];
}

void CBehaviorGraphVectorValueBaseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphVectorValueBaseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphVectorValueBaseNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphVectorValueBaseNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_value ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphVectorValueBaseNode::CacheConnections()
{
	TBaseClass::CacheConnections();
	m_cachedInputNode = CacheVectorValueBlock( TXT("Input") );
}
