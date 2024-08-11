/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphHeadingNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphHeadingNode );

CBehaviorGraphHeadingNode::CBehaviorGraphHeadingNode()
	: m_heading( 180.0f )
{
}

void CBehaviorGraphHeadingNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	// ------------- TO REFACTOR ----------------
	TBaseClass::Sample( context, instance, output );

	ASSERT( !TXT("DO NOT USE THIS NODE!") );

	const CAnimatedComponent *charComponent = instance.GetAnimatedComponent();
	if ( charComponent && charComponent->GetSkeleton() && charComponent->GetSkeleton()->IsValid() )
	{
		// Apply heading
		const AnimQuaternion rotationQuat( AnimVector4( 0, 0, 1, 1 ), DEG2RAD( m_heading ) );
		const AnimQsTransform rotation( AnimVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, AnimVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

		for ( Uint32 i=0; i<output.m_numBones; i++ )
		{
			if ( charComponent->GetSkeleton()->GetParentBoneIndex(i) == -1 )
			{
#ifdef USE_HAVOK_ANIMATION
				output.m_outputPose[ i ].setMul( rotation, output.m_outputPose[ i ] );
#else
				output.m_outputPose[ i ].SetMul( rotation, output.m_outputPose[ i ] );
#endif
			}
			else if ( charComponent->GetTrajectoryBone() == (Int32)i )
			{
#ifdef USE_HAVOK_ANIMATION
				output.m_outputPose[ i ].setMul( output.m_outputPose[ i ], rotation );
#else
				output.m_outputPose[ i ].SetMul( rotation, output.m_outputPose[ i ] );
#endif
			}
		}

#ifdef USE_HAVOK_ANIMATION
		output.m_deltaReferenceFrameLocal.m_translation.setRotatedDir(rotationQuat, output.m_deltaReferenceFrameLocal.m_translation);
#else
		RedMatrix4x4 matrix = BuildFromQuaternion(rotationQuat.Quat);

		const RedVector4 trans = output.m_deltaReferenceFrameLocal.Translation;
		output.m_deltaReferenceFrameLocal.Translation = TransformVector(matrix, trans);
#endif
	}

	// ------------- TO REFACTOR ----------------
}

String CBehaviorGraphHeadingNode::GetCaption() const
{
	return String::Printf( TXT("Heading [%.2f]"), m_heading );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMotionExRotAngleNode );

CBehaviorGraphMotionExRotAngleNode::CBehaviorGraphMotionExRotAngleNode()
	: m_worldSpace( false )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMotionExRotAngleNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );		
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Value ) ) );
}

#endif

void CBehaviorGraphMotionExRotAngleNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->OnUpdate( context, instance, timeDelta );
	}
}

void CBehaviorGraphMotionExRotAngleNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Sample( context, instance, output );
	}

#ifdef USE_HAVOK_ANIMATION
	Float angle;
	hkQuaternion rest;
	output.m_deltaReferenceFrameLocal.m_rotation.decomposeRestAxis( hkVector4( 0.f, 0.f, 1.f ), rest, angle );
	angle = RAD2DEG( angle );

	if ( m_worldSpace )
	{
		angle = EulerAngles::NormalizeAngle180( instance.GetAnimatedComponent()->GetWorldYaw() + angle );
	}

	instance[ i_value ] = angle;
#else
	Float angle = output.m_deltaReferenceFrameLocal.GetRotation().GetYaw();

	if ( m_worldSpace )
	{
		angle = EulerAngles::NormalizeAngle180( instance.GetAnimatedComponent()->GetWorldYaw() + angle );
	}
	
	instance[ i_value ] = angle;
#endif
}

void CBehaviorGraphMotionExRotAngleNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = 0.f;
}

void CBehaviorGraphMotionExRotAngleNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedAnimInputNode = CacheBlock( TXT("Input") );
}

void CBehaviorGraphMotionExRotAngleNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Activate( instance );
	}
}

void CBehaviorGraphMotionExRotAngleNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphMotionExRotAngleNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMotionExRotAngleNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphMotionExRotAngleNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphMotionExRotAngleNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedAnimInputNode )
	{
		return m_cachedAnimInputNode->ProcessEvent( instance, event );
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMotionExToAngleNode );

CBehaviorGraphMotionExToAngleNode::CBehaviorGraphMotionExToAngleNode()
	: m_worldSpace( false )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMotionExToAngleNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );		
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Value ) ) );
}

#endif

void CBehaviorGraphMotionExToAngleNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->OnUpdate( context, instance, timeDelta );
	}
}

void CBehaviorGraphMotionExToAngleNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Sample( context, instance, output );
	}

#ifdef USE_HAVOK_ANIMATION
	if ( output.m_deltaReferenceFrameLocal.m_translation.lengthSquared3() > 0.0001f )
	{
		hkVector4 vec( output.m_deltaReferenceFrameLocal.m_translation );
		vec.normalize3();
		const Float cosAngle = Clamp( (Float)vec.dot3( hkVector4( 0.f, 1.f, 0.f ) ), -1.f, 1.f );
		Float angle = RAD2DEG( MAcos( cosAngle ) );

		if ( m_worldSpace )
		{
			angle = EulerAngles::NormalizeAngle180( instance.GetAnimatedComponent()->GetWorldYaw() + angle );
		}

		instance[ i_value ] = angle;
	}
#endif
}

void CBehaviorGraphMotionExToAngleNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = 0.f;
}

void CBehaviorGraphMotionExToAngleNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedAnimInputNode = CacheBlock( TXT("Input") );
}

void CBehaviorGraphMotionExToAngleNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Activate( instance );
	}
}

void CBehaviorGraphMotionExToAngleNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphMotionExToAngleNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMotionExToAngleNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphMotionExToAngleNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphMotionExToAngleNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedAnimInputNode )
	{
		return m_cachedAnimInputNode->ProcessEvent( instance, event );
	}

	return false;
}
