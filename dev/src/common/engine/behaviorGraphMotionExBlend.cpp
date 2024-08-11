/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphMotionExBlend.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animSyncInfo.h"
#include "animatedComponent.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMotionExBlendNode );

CBehaviorGraphMotionExBlendNode::CBehaviorGraphMotionExBlendNode()
	: m_threshold( 0.03f ) // 3%
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMotionExBlendNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( A ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( B ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Speed ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphMotionExBlendNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_control;
	compiler << i_speed;
	compiler << i_weight;
}

void CBehaviorGraphMotionExBlendNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_control ] = 0.f;
	instance[ i_speed ] = 0.f;
	instance[ i_weight ] = 0.f;
}

void CBehaviorGraphMotionExBlendNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_control );
	INST_PROP( i_speed );
	INST_PROP( i_weight );
}

void CBehaviorGraphMotionExBlendNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedSpeedVariableNode = CacheValueBlock( TXT("Speed") );
	m_cachedFirstInputNode = CacheBlock( TXT("A") );
	m_cachedSecondInputNode = CacheBlock( TXT("B") );
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphMotionExBlendNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	instance[ i_speed ] = 0.f;
	instance[ i_control ] = 1.f;

	if ( m_cachedSpeedVariableNode )
	{
		m_cachedSpeedVariableNode->Update( context, instance, timeDelta );
		instance[ i_speed ] = m_cachedSpeedVariableNode->GetValue( instance ) * timeDelta;
	}
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
		instance[ i_control ] = Clamp( m_cachedControlVariableNode->GetValue( instance ), 0.f, 1.f );
	}

	if ( m_cachedFirstInputNode && m_cachedSecondInputNode ) 
	{
		m_cachedFirstInputNode->Update( context, instance, timeDelta );
		m_cachedSecondInputNode->Update( context, instance, timeDelta );
	}
}

Float CBehaviorGraphMotionExBlendNode::GetWeightFromSpeed( const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, const Float speed ) const
{
#ifdef USE_HAVOK_ANIMATION
	Float distA = poseA.m_deltaReferenceFrameLocal.m_translation.length3();
	Float distB = poseB.m_deltaReferenceFrameLocal.m_translation.length3();
#else
	Float distA = poseA.m_deltaReferenceFrameLocal.Translation.Length3();
	Float distB = poseB.m_deltaReferenceFrameLocal.Translation.Length3();
#endif

	if ( distA > distB )
	{
		Float temp = distA;
		distA = distB;
		distB = temp;
	}

	if ( speed <= distA )
	{
		return 0.f;
	}
	else if ( speed >= distB )
	{
		return 1.f;
	}
	else
	{
		Float weight = ( speed - distA ) / ( distB - distA ); 
		ASSERT( weight > 0.f && weight < 1.f );
		return weight;
	}
}

void CBehaviorGraphMotionExBlendNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	if ( m_cachedSecondInputNode && m_cachedFirstInputNode && instance[ i_control ] > 0.f )
	{
		CCacheBehaviorGraphOutput cachePose1( context );
		CCacheBehaviorGraphOutput cachePose2( context );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			m_cachedFirstInputNode->Sample( context, instance, *temp1 );
			m_cachedSecondInputNode->Sample( context, instance, *temp2 );

			Float newWeight = instance[ i_control ] * GetWeightFromSpeed( *temp1, *temp2, instance[ i_speed ] );
			if ( MAbs( instance[ i_weight ] - newWeight ) > m_threshold )
			{
				instance[ i_weight ] = newWeight;
			}

			// Interpolate poses
#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				output.SetInterpolate( *temp1, *temp2, instance[ i_weight ] );
			}
			else
			{
				output.SetInterpolateME( *temp1, *temp2, instance[ i_weight ] );
			}
#else
			output.SetInterpolate( *temp1, *temp2, instance[ i_weight ] );
#endif

			// Merge events
			output.MergeEventsAndUsedAnims(*temp1, *temp2, 1.0f - instance[ i_weight ], instance[ i_weight ] );
		}
	}
}

void CBehaviorGraphMotionExBlendNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	if ( m_cachedFirstInputNode && m_cachedSecondInputNode )
	{
		CSyncInfo firstSyncInfo;
		CSyncInfo secondSyncInfo;

		m_cachedFirstInputNode->GetSyncInfo( instance, firstSyncInfo );
		m_cachedSecondInputNode->GetSyncInfo( instance, secondSyncInfo );

		info.SetInterpolate( firstSyncInfo, secondSyncInfo, instance[ i_weight ] );
	}
}

void CBehaviorGraphMotionExBlendNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedFirstInputNode && m_cachedSecondInputNode )
	{
		m_cachedFirstInputNode->SynchronizeTo( instance, info );
		m_cachedSecondInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphMotionExBlendNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedFirstInputNode && m_cachedSecondInputNode )
	{
		retVal = m_cachedFirstInputNode->ProcessEvent( instance, event ); 
		retVal |= m_cachedSecondInputNode->ProcessEvent( instance, event );
	}

	return retVal;
}

void CBehaviorGraphMotionExBlendNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedFirstInputNode && m_cachedSecondInputNode )
	{
		m_cachedFirstInputNode->Activate( instance );
		m_cachedSecondInputNode->Activate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	if ( m_cachedSpeedVariableNode )
	{
		m_cachedSpeedVariableNode->Activate( instance );
	}
}

void CBehaviorGraphMotionExBlendNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedFirstInputNode && m_cachedSecondInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
		m_cachedSecondInputNode->Deactivate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}

	if ( m_cachedSpeedVariableNode )
	{
		m_cachedSpeedVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphMotionExBlendNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedFirstInputNode && m_cachedSecondInputNode )
	{
		m_cachedFirstInputNode->ProcessActivationAlpha( instance, instance[ i_weight ] );
		m_cachedSecondInputNode->ProcessActivationAlpha( instance, 1.f - instance[ i_weight ] );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedSpeedVariableNode )
	{
		m_cachedSpeedVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMotionExValueNode );

void CBehaviorGraphMotionExValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
}

void CBehaviorGraphMotionExValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const CSkeletalAnimationSetEntry* anim = instance.GetAnimatedComponent()->GetAnimationContainer() ? 
		instance.GetAnimatedComponent()->GetAnimationContainer()->FindAnimation( m_animation ) : NULL;

	if ( anim && anim->GetAnimation() )
	{
#ifdef USE_HAVOK_ANIMATION
		instance[ i_value ] = anim->GetAnimation()->GetMovementBetweenTime( 0.f, 0.f, 1 ).m_translation.length3();
#else
		instance[ i_value ] = anim->GetAnimation()->GetMovementBetweenTime( 0.0f, 0.0f, 1 ).Translation.Length3();
#endif
	}
	else
	{
		instance[ i_value ] = 0.f;
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMotionExValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );
}

String CBehaviorGraphMotionExValueNode::GetCaption() const
{
	return m_animation.Empty() ? TXT("Motion ex value") : String::Printf( TXT("Motion ex value - %s"), m_animation.AsString().AsChar() );
}

#endif

CBehaviorGraph* CBehaviorGraphMotionExValueNode::GetParentGraph()
{
	CBehaviorGraph* graph = GetGraph();
	return graph;
}

Float CBehaviorGraphMotionExValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_value ];
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMotionRotChangeValueNode );

CBehaviorGraphMotionRotChangeValueNode::CBehaviorGraphMotionRotChangeValueNode()
	: m_analizeMotionEx( true )
	, m_radOrDeg( false )
{

}

void CBehaviorGraphMotionRotChangeValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
	compiler << i_prevMotion;
	compiler << i_currMotion;
}

void CBehaviorGraphMotionRotChangeValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_value ] = 0.f;
	instance[ i_prevMotion ] = Vector::ZERO_3D_POINT;
	instance[ i_currMotion ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphMotionRotChangeValueNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_value );
	INST_PROP( i_prevMotion );
	INST_PROP( i_currMotion );
}

Float CBehaviorGraphMotionRotChangeValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_value ];
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMotionRotChangeValueNode::GetCaption() const
{
	return String( TXT("Motion rot change") );
}

#endif

void CBehaviorGraphMotionRotChangeValueNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

#ifdef USE_HAVOK_ANIMATION
	Float& value = instance[ i_value ];
	Vector& prevMotion = instance[ i_prevMotion ];
	Vector& currMotion = instance[ i_currMotion ];

	if ( !m_analizeMotionEx )
	{
		// TODO
	}
	else
	{
		currMotion = TO_CONST_VECTOR_REF( output.m_deltaReferenceFrameLocal.m_translation );
		currMotion.Normalize3();

		const Float cosAngle = Clamp( Vector::Dot3( prevMotion, currMotion ), -1.f, 1.f );
		const Float angle = MAcos( cosAngle );

		value = m_radOrDeg ? angle : RAD2DEG( angle );

		ASSERT( !Red::Math::NumericalUtils::IsNan( value ) );
	}
#endif
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMotionRotChangeValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );	
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Value ) ) );		
}

#endif

void CBehaviorGraphMotionRotChangeValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	Vector& prevMotion = instance[ i_prevMotion ];
	const Vector currMotion = instance[ i_currMotion ];

	prevMotion = currMotion;
}

void CBehaviorGraphMotionRotChangeValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = 0.0f;
	instance[ i_prevMotion ] = Vector::ZERO_3D_POINT;
	instance[ i_currMotion ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphMotionRotChangeValueNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphMotionRotChangeValueNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphMotionRotChangeValueNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphMotionRotChangeValueNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMotionRotChangeValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphMotionRotChangeValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}

	instance[ i_value ] = 0.f;
	instance[ i_prevMotion ] = Vector::ZERO_3D_POINT;
	instance[ i_currMotion ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphMotionRotChangeValueNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputNode = CacheBlock( TXT("Input") );
}

//////////////////////////////////////////////////////////////////////////
