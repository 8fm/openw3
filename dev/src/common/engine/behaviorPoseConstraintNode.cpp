/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphValueNode.h"
#include "behaviorPoseConstraintNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPoseConstraintNode );

void CBehaviorGraphPoseConstraintNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_dt;
	compiler << i_weight;
}

void CBehaviorGraphPoseConstraintNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_dt ] = 0.f;
	instance[ i_weight ] = 0.f;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphPoseConstraintNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphPoseConstraintNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphPoseConstraintNode::GetCaption() const
{
	return TXT("Pose constraint");
}

#endif

void CBehaviorGraphPoseConstraintNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphPoseConstraintNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}
}

void CBehaviorGraphPoseConstraintNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphPoseConstraintNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_dt ] = timeDelta;

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );
		instance[ i_weight ] = m_cachedControlValueNode->GetValue( instance );
	}
}

void CBehaviorGraphPoseConstraintNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );
}

void CBehaviorGraphPoseConstraintNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );
}

Float CBehaviorGraphPoseConstraintNode::GetTimeDelta( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_dt ];
}

Float CBehaviorGraphPoseConstraintNode::GetWeight( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_weight ];
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPoseConstraintWithTargetNode );

void CBehaviorGraphPoseConstraintWithTargetNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_targetPos;
	compiler << i_targetRot;
}

void CBehaviorGraphPoseConstraintWithTargetNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_targetPos ] = Vector::ZERO_3D_POINT;
	instance[ i_targetRot ] = EulerAngles::ZEROS;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphPoseConstraintWithTargetNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );

	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetPos ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetRot ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphPoseConstraintWithTargetNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	m_cachedTargetPosValueNode = CacheVectorValueBlock( TXT("TargetPos") );
	m_cachedTargetRotValueNode = CacheVectorValueBlock( TXT("TargetRot") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphPoseConstraintWithTargetNode::GetCaption() const
{
	return TXT("Pose constraint with target");
}

#endif

void CBehaviorGraphPoseConstraintWithTargetNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedTargetPosValueNode )
	{
		m_cachedTargetPosValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetRotValueNode )
	{
		m_cachedTargetRotValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphPoseConstraintWithTargetNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedTargetPosValueNode )
	{
		m_cachedTargetPosValueNode->Activate( instance );
	}

	if ( m_cachedTargetRotValueNode )
	{
		m_cachedTargetRotValueNode->Activate( instance );
	}
}

void CBehaviorGraphPoseConstraintWithTargetNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedTargetPosValueNode )
	{
		m_cachedTargetPosValueNode->Deactivate( instance );
	}

	if ( m_cachedTargetRotValueNode )
	{
		m_cachedTargetRotValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphPoseConstraintWithTargetNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedTargetPosValueNode )
	{
		m_cachedTargetPosValueNode->Update( context, instance, timeDelta );
		instance[ i_targetPos ] = m_cachedTargetPosValueNode->GetVectorValue( instance );
	}

	if ( m_cachedTargetRotValueNode )
	{
		m_cachedTargetRotValueNode->Update( context, instance, timeDelta );
		instance[ i_targetRot ] = BehaviorUtils::VectorToAngles( m_cachedTargetRotValueNode->GetVectorValue( instance ) );
	}
}

void CBehaviorGraphPoseConstraintWithTargetNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );
}

void CBehaviorGraphPoseConstraintWithTargetNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );
}

Bool CBehaviorGraphPoseConstraintWithTargetNode::GetTarget( CBehaviorGraphInstance& instance, Vector& pos, EulerAngles& rot ) const
{
	pos = instance[ i_targetPos ];
	rot = instance[ i_targetRot ];

	return true;
}

const Vector& CBehaviorGraphPoseConstraintWithTargetNode::GetTargetPos( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_targetPos ];
}
