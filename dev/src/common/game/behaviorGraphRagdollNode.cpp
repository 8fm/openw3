/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphOutput.h"
#include "behaviorGraphRagdollNode.h"
#include "../engine/behaviorGraphContext.h"
#include "../engine/allocatedBehaviorGraphOutput.h"
#include "../engine/behaviorGraphSocket.h"
#include "../engine/behaviorGraphValueNode.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "movingPhysicalAgentComponent.h"
#include "../engine/behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRagdollNode );
IMPLEMENT_RTTI_ENUM( EBehaviorGraphRagdollNodeState );

///////////////////////////////////////////////////////////////////////////////

// RED_DEFINE_STATIC_NAME( pelvis ); <- declared in name registry

RED_DEFINE_STATIC_NAME( torso3 );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphRagdollNode::CBehaviorGraphRagdollNode()
: m_allowToProvidePreRagdollPose( false )
, m_updateAndSampleInputIfPreRagdollWeightIsNonZero( false )
, m_keepInFrozenRagdollPose( false )
{
}

void CBehaviorGraphRagdollNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_ragdollExists;
	compiler << i_controlValue;
	compiler << i_prevControlValue;
	compiler << i_startedToSwitchAtControlValue;
	compiler << i_impulse;
	compiler << i_state;
	compiler << i_ragdolledPose;
}

void CBehaviorGraphRagdollNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_ragdollExists ] = false;
	instance[ i_controlValue ] = 0.f;
	instance[ i_prevControlValue ] = 0.f;
	instance[ i_startedToSwitchAtControlValue ] = 0.f;
	instance[ i_impulse ] = Vector::ZERO_3D_POINT;
	instance[ i_state ] = BGRN_Inactive;

	if ( CAnimatedComponent *animComponent = instance.GetAnimatedComponentUnsafe() )
	{
		animComponent->SetHasBehaviorRagdollNode(true);
	}
}

void CBehaviorGraphRagdollNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_ragdollExists );
	INST_PROP( i_controlValue );
	INST_PROP( i_prevControlValue );
	INST_PROP( i_startedToSwitchAtControlValue );
	INST_PROP( i_impulse );
	INST_PROP( i_state );
	INST_PROP( i_ragdolledPose );
}

void CBehaviorGraphRagdollNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	ChangeState( instance, BGRN_Inactive );

	TBaseClass::OnReleaseInstance( instance );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRagdollNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( RootBoneImpulse ) ) );
}

#endif

void CBehaviorGraphRagdollNode::FastPoseReset( SBehaviorGraphOutput* pose ) const
{
	// clear list of events before reusing m_pose...
	pose->ClearEventsAndUsedAnims();

	// clear float tracks
	for ( Uint32 i=0; i<pose->m_numFloatTracks; i++ )
	{
		pose->m_floatTracks[i] = 0;
	}
#ifdef USE_HAVOK_ANIMATION
	// clear delta reference
	pose->m_deltaReferenceFrameLocal.setIdentity();
#else
	// clear delta reference
	pose->m_deltaReferenceFrameLocal.SetIdentity();
#endif
}

void CBehaviorGraphRagdollNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Ragdoll );

	// update variable 
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedRootBoneImpulseVariable )
	{
		m_cachedRootBoneImpulseVariable->Update( context, instance, timeDelta );
	}

	// copy variable value (so it's constant across update and sample)
	UpdateControlValues( instance );

	EBehaviorGraphRagdollNodeState& state = instance[ i_state ];

	if ( timeDelta > 0 && instance[ i_ragdollExists ] )
	{
		Float const & controlValue = instance[ i_controlValue ];
		Float const & prevControlValue = instance[ i_prevControlValue ];
		if ( state == BGRN_Kinematic || state == BGRN_SwitchingFromRagdoll )
		{
			// switch to ragdoll
			if ( prevControlValue < controlValue )
			{
				ChangeState( instance, BGRN_Ragdoll );
			}
			if ( controlValue == 0.0f && state == BGRN_SwitchingFromRagdoll && ! m_keepInFrozenRagdollPose )
			{
				// finished switching from ragdoll
				ChangeState( instance, BGRN_Kinematic );
			}
		}
		else if ( state == BGRN_Ragdoll )
		{
			if ( prevControlValue > controlValue )
			{
				// switch back to kinematic (skip switching if requested)
				ChangeState( instance, controlValue != 0.0f
#ifdef DISABLE_SAMPLING_AT_LOD3
					// not actually disabling sampling but it is special behavior, isn't it?
					&& context.GetLodLevel() <= BL_Lod2
#endif
					? BGRN_SwitchingFromRagdoll : BGRN_Ragdoll );
			}
		}
	}

	if ( m_cachedInputNode && ( state != BGRN_Ragdoll || ( m_allowToProvidePreRagdollPose && m_updateAndSampleInputIfPreRagdollWeightIsNonZero && instance.GetAnimatedComponent() && instance.GetAnimatedComponent()->GetSkeletonPreRagdollWeight() > 0.0f ) ) )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphRagdollNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( Ragdoll );

	EBehaviorGraphRagdollNodeState const & state = instance[ i_state ];

	if ( m_cachedInputNode && ! m_keepInFrozenRagdollPose && ( state != BGRN_Ragdoll || ( m_allowToProvidePreRagdollPose && m_updateAndSampleInputIfPreRagdollWeightIsNonZero && instance.GetAnimatedComponent() && instance.GetAnimatedComponent()->GetSkeletonPreRagdollWeight() > 0.0f ) ) )
	{
		m_cachedInputNode->Sample( context, instance, output );
		if ( m_allowToProvidePreRagdollPose )
		{
			instance.GetAnimatedComponentUnsafe()->SetRagdollNodeProvidesValidPose();
		}
	}

	Bool & ragdollExists = instance[ i_ragdollExists ];
	if ( ! ragdollExists )
	{
		// ragdoll appeared, hey! and it appeared after OnActivated/Update :(
		ChangeState( instance, BGRN_Inactive );
		if ( CAnimatedComponent *animComponent = instance.GetAnimatedComponentUnsafe() )
		{
			if ( CPhysicsRagdollWrapper* wrapper = animComponent->GetRagdollPhysicsWrapper() )
			{
				Float const & controlValue = instance[ i_controlValue ];
				ChangeState( instance, controlValue == 0.0f? BGRN_Kinematic : BGRN_Ragdoll );
				ragdollExists = true;
			}
		}
	}

	if ( state == BGRN_SwitchingFromRagdoll )
	{
		// get ragdolled pose
		SBehaviorGraphOutput * ragdolledPose = instance[ i_ragdolledPose ].GetPose();

		// rotate ragdolled pose by what we were rotated from output (inverse actually, so the ragdolled pose remains still in world space)
		{
			RedQuaternion deltaRotInverted;
			deltaRotInverted.SetInverse( output.m_deltaReferenceFrameLocal.GetRotation() );
			RedQuaternion currentPoseRotation = ragdolledPose->m_outputPose[0].GetRotation();
			RedQuaternion useRotation = RedQuaternion::Mul( currentPoseRotation, deltaRotInverted );
			useRotation.Normalize();
			ragdolledPose->m_outputPose[0].SetRotation( useRotation );
		}

		// blend
		Float const & controlValue = instance[ i_controlValue ];
		Float const & startedToSwitchAtControlValue = instance[ i_startedToSwitchAtControlValue ];
		RED_ASSERT( startedToSwitchAtControlValue != 0.0f, TXT("startedToSwitchAtControlValue should never be 0, it is set only when entering SwitchingFromRagdoll state and this happens only when control value is non zero") );
		// if we want to keep in ragdoll pose, this weight should be always 1.0 - we won't switch from SwitchingFromRagdoll to Kinematic
		Float ragdollWeight = m_keepInFrozenRagdollPose? 1.0f : Clamp( controlValue / startedToSwitchAtControlValue, 0.0f, 1.0f ); 
		output.SetInterpolateWithoutME( output, *ragdolledPose, ragdollWeight );

		// don't merge events and used anims (from ragdoll it doesn't make sense)
	}
}

void CBehaviorGraphRagdollNode::ChangeState( CBehaviorGraphInstance& instance, EBehaviorGraphRagdollNodeState newState ) const
{
	EBehaviorGraphRagdollNodeState& state = instance[ i_state ];

	if ( state == newState )
	{
		return;
	}

	if ( newState == BGRN_SwitchingFromRagdoll )
	{
		instance[ i_startedToSwitchAtControlValue ] = instance[ i_controlValue ];
		CreateAndCacheRagdolledPose( instance );
	}
	else if ( state == BGRN_SwitchingFromRagdoll )
	{
		DestroyRagdolledPose( instance );
	}

	if ( newState != BGRN_Inactive )
	{
		if ( CAnimatedComponent *animComponent = instance.GetAnimatedComponentUnsafe() )
		{
			if ( CPhysicsRagdollWrapper* wrapper = animComponent->GetRagdollPhysicsWrapper() )
			{
				// no need to check if we're kinematic or not, it is done by those functions
				if ( newState == BGRN_Ragdoll )
				{
					wrapper->SwitchToKinematic( false );
				}
				else
				{
					// make sure we will switch to swimming
					if ( m_switchToSwimming )
					{
						if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( animComponent ) )
						{
							mpac->SetRagdollToSwimming( true );
						}
					}
					wrapper->SwitchToKinematic( true );
				}
			}
		}
	}

	state = newState;
}

void CBehaviorGraphRagdollNode::CreateAndCacheRagdolledPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& ragdolledPose = instance[ i_ragdolledPose ];
	// cache output but we might be missing ragdoll pose
	ragdolledPose.CreateAndCache( instance );
	// get ragdoll pose from skeleton model space (that is used for skinning and ragdoll sync)
	if ( CAnimatedComponent *animComponent = instance.GetAnimatedComponentUnsafe() )
	{
		ragdolledPose.GetPose()->SetPoseFromBonesModelSpace( animComponent->GetSkeleton(), animComponent->GetSkeletonModelSpace() );
	}
}

void CBehaviorGraphRagdollNode::DestroyRagdolledPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& ragdolledPose = instance[ i_ragdolledPose ];
	ragdolledPose.Free( instance );
}

void CBehaviorGraphRagdollNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
	Bool & ragdollExists = instance[ i_ragdollExists ];
	ragdollExists = false;
	EBehaviorGraphRagdollNodeState newState = BGRN_Inactive;
	if ( CAnimatedComponent *animComponent = instance.GetAnimatedComponentUnsafe() )
	{
		if ( CPhysicsRagdollWrapper* wrapper = animComponent->GetRagdollPhysicsWrapper() )
		{
			newState = wrapper->IsKinematic()? BGRN_Kinematic : BGRN_Ragdoll;
			ragdollExists = true;
		}
		if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( animComponent ) )
		{
			// get from animated ragdoll straight to ragdoll
			if ( mac->GetAnimationProxy().GetInAnimatedRagdoll() )
			{
				newState = BGRN_Ragdoll;
			}
		}
		if ( m_switchToSwimming )
		{
			if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( animComponent ) )
			{
				mpac->SetRagdollToSwimming( true );
			}
		}
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	if ( m_cachedRootBoneImpulseVariable )
	{
		m_cachedRootBoneImpulseVariable->Activate( instance );
	}

	UpdateControlValues( instance );
	instance[ i_prevControlValue ] = instance[ i_controlValue ];
	if ( instance[ i_controlValue ] > 0.0f )
	{
		// force to start in ragdoll
		newState = BGRN_Ragdoll;
	}

	ChangeState( instance, newState );
}

void CBehaviorGraphRagdollNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( CAnimatedComponent* ac = instance.GetAnimatedComponentUnsafe() )
	{
		ac->SetRagdollNodeProvidesValidPose( false );
		if ( m_switchToSwimming )
		{
			if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( ac ) )
			{
				mpac->SetRagdollToSwimming( false );
			}
		}
	}

	ChangeState( instance, BGRN_Inactive );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}

	if ( m_cachedRootBoneImpulseVariable )
	{
		m_cachedRootBoneImpulseVariable->Deactivate( instance );
	}
}

void CBehaviorGraphRagdollNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	// don't call process activation alpha of TBaseClass as it calls cached input node always!
	// just go straight to base's base class
	CBehaviorGraphNode::ProcessActivationAlpha( instance, alpha );
	if ( m_cachedInputNode && instance[ i_state ] != BGRN_Ragdoll )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedRootBoneImpulseVariable )
	{
		m_cachedRootBoneImpulseVariable->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphRagdollNode::UpdateControlValues( CBehaviorGraphInstance& instance ) const
{
	Float& controlValue = instance[ i_controlValue ];

	instance[ i_prevControlValue ] = controlValue;
	controlValue = m_cachedControlVariableNode ? m_cachedControlVariableNode->GetValue( instance ) : 0.0f;
	controlValue = Clamp< Float >( controlValue, 0.0f, 1.0f );

	if ( m_cachedRootBoneImpulseVariable )
	{
		instance[ i_impulse ] = m_cachedRootBoneImpulseVariable->GetVectorValue( instance );
	}
}

void CBehaviorGraphRagdollNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
	m_cachedRootBoneImpulseVariable = CacheVectorValueBlock( TXT("RootBoneImpulse") );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
