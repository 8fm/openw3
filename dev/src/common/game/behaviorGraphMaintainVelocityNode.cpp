/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphOutput.h"
#include "behaviorGraphMaintainVelocityNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMaintainVelocityNode );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphMaintainVelocityNode::CBehaviorGraphMaintainVelocityNode()
: m_blendTime( 0.5f )
, m_stop( true )
{
}

void CBehaviorGraphMaintainVelocityNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_velocityWS;
}

void CBehaviorGraphMaintainVelocityNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeDelta ] = 0.0f;
	instance[ i_velocityWS ] = Vector::ZEROS;
}

void CBehaviorGraphMaintainVelocityNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_velocityWS );
}

void CBehaviorGraphMaintainVelocityNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MaintainVelocity );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorGraphMaintainVelocityNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( MaintainVelocity );

	TBaseClass::Sample( context, instance, output );
	
	Vector & velocityWS = instance[ i_velocityWS ];
	Vector velocityTargetWS = velocityWS;
	
	if ( m_stop )
	{
		velocityTargetWS = Vector::ZEROS;
	}

	velocityWS = BlendToWithBlendTime( velocityWS, velocityTargetWS, m_blendTime, instance[ i_timeDelta ] );

	Vector const addDeltaWS = velocityWS * instance[ i_timeDelta ];
	Matrix worldToLocal;
	instance.GetAnimatedComponent()->GetWorldToLocal( worldToLocal );
	Vector const addDeltaMS = worldToLocal.TransformVector( addDeltaWS );

	output.m_deltaReferenceFrameLocal.SetTranslation( Add( output.m_deltaReferenceFrameLocal.GetTranslation(), VectorToAnimVector( addDeltaMS ) ) );
}

void CBehaviorGraphMaintainVelocityNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	Vector & velocityWS = instance[ i_velocityWS ];
	velocityWS = Vector::ZEROS;

	if ( const CMovingAgentComponent * mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponent() ) )
	{
		velocityWS = mac->GetVelocity();
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
