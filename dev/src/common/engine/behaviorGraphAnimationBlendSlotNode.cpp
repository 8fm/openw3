/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationBlendSlotNode.h"
#include "behaviorGraphAnimationSlotNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/entity.h"
#include "../engine/curve.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "skeletalAnimationEntry.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//#define SLOT_DEBUG

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationSlotNode );

CBehaviorGraphAnimationSlotNode::CBehaviorGraphAnimationSlotNode()
{
	
}

void CBehaviorGraphAnimationSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_slotActive;
	compiler << i_hasCachedPose;
	compiler << i_blendIn;
	compiler << i_blendOut;
	compiler << i_blendInDuration;
	compiler << i_blendOutDuration;
	compiler << i_blendInType;
	compiler << i_blendOutType;
	compiler << i_mergeBlendedSlotEvents;
	compiler << i_slotAction;
	compiler << i_isInTick;
	compiler << i_pose;
	compiler << i_firstLoop;
	compiler << i_finishBlendingDurationLeft;
}

void CBehaviorGraphAnimationSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_hasCachedPose ] = false;
	instance[ i_blendInType ] = BTBM_Blending;
	instance[ i_blendOutType ] = BTBM_Blending;

	instance[ i_isInTick ] = false;

	SlotReset( instance );
	SetSlotInactive( instance );
}

void CBehaviorGraphAnimationSlotNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	if ( HasCachedPose( instance ) )
	{
		DestroyPose( instance );
	}

	TBaseClass::OnReleaseInstance( instance );
}

void CBehaviorGraphAnimationSlotNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_slotActive );
	INST_PROP( i_hasCachedPose );
	INST_PROP( i_blendIn );
	INST_PROP( i_firstUpdate );
	INST_PROP( i_blendOut );
	INST_PROP( i_blendInDuration );
	INST_PROP( i_blendOutDuration );
	INST_PROP( i_mergeBlendedSlotEvents );
	INST_PROP( i_slotAction );
	INST_PROP( i_firstLoop );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimationSlotNode::GetCaption() const
{
	return String::Printf( TXT("Animation blend slot [ %s ]"), m_name.AsChar() );
}

void CBehaviorGraphAnimationSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( ForcedTime ), false ) );
}

#endif

void CBehaviorGraphAnimationSlotNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	m_cachedBaseInputNode = CacheBlock( TXT("In") );
	m_cachedForceTimeNode = CacheValueBlock( TXT("ForcedTime") );
}

void CBehaviorGraphAnimationSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimationSlot );
	if ( !IsValid( instance ) )
	{
		// Update base input
		UpdateInputAnimation( context, instance, timeDelta );
		return;
	}

	instance[ i_isInTick ] = true;

	if ( IsBlendingOnFinish( instance ) ) // if we are blending on finish
	{
		// then update the blend time left
		instance[ i_finishBlendingDurationLeft ] -= timeDelta;

		// and validate if we are already fully blended.
		if ( !IsBlendingOnFinish( instance) )
		{
			StopAndDeactivate( instance );
		}
	}

	if ( !IsPlayingSlotAnimation( instance ) && IsSlotActive( instance ) )
	{
		SetSlotInactive( instance );
	}

	if ( !IsSlotActive( instance ) )
	{
		// Slot is not active
		if ( HasCachedPose( instance ) )
		{
			DestroyPose( instance );
		}

		CheckInputActivation( instance );

		// Update base input
		UpdateInputAnimation( context, instance, timeDelta );
	}
	else
	{
		// Update slot animation
		UpdateSlotAnimation( context, instance, timeDelta );

		// Slot is active
		UpdateSlotLogic( instance, timeDelta );

		if ( IsBlendingWithInput( instance ) )
		{
			CheckInputActivation( instance );

			// Update base input
			UpdateInputAnimation( context, instance, timeDelta );

#ifdef SLOT_DEBUG
			BEH_LOG(TXT("Update slot eith input: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif
		}
		else
		{
			CheckInputDeactivation( instance );

#ifdef SLOT_DEBUG
			BEH_LOG(TXT("Update slot: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif
		}
	}

	instance[ i_isInTick ] = false;
}

void CBehaviorGraphAnimationSlotNode::UpdateSlotLogic( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	ASSERT( IsSlotActive( instance ) );
	ASSERT( IsPlayingSlotAnimation( instance ) );

	if ( IsBlendIn( instance ) )
	{
		ASSERT( HasBlendIn( instance ) );
		ASSERT( !IsBlendOut( instance ) );
	}
	else if ( IsBlendOut( instance ) )
	{
		ASSERT( HasBlendOut( instance ) );
		ASSERT( !IsBlendIn( instance ) );
	}

	/*if ( HasBlendIn( instance ) && IsBlendInStarted( instance ) )
	{
		ASSERT( !logicStep );
		logicStep = true;

		//...
	}*/

	if ( HasBlendOut( instance ) && IsBlendOutStarted( instance ) )
	{
		// Inform 
		SetSlotLogicAction( instance, SA_BlendOut );

#ifdef SLOT_DEBUG
		BEH_LOG(TXT("Slot logic BlendOut: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif
	}

	if ( IsAnimationSlotFinished( instance ) && !IsBlendingOnFinish(instance) )
	{
		// Inform
		SetSlotLogicAction( instance, SA_Finish );

#ifdef SLOT_DEBUG
		BEH_LOG(TXT("Slot logic Finish: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif
	}
}

void CBehaviorGraphAnimationSlotNode::UpdateSlotAnimation( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphAnimationSlotNode::UpdateInputAnimation( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedBaseInputNode )
	{
		if ( !m_cachedBaseInputNode->IsActive( instance ) )
		{
			ASSERT( m_cachedBaseInputNode->IsActive( instance ) );
			m_cachedBaseInputNode->Activate( instance );
		}

		// Update base input
		m_cachedBaseInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphAnimationSlotNode::CheckInputActivation( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedBaseInputNode && !m_cachedBaseInputNode->IsActive( instance ) )
	{
		m_cachedBaseInputNode->Activate( instance );
	}
}

void CBehaviorGraphAnimationSlotNode::CheckInputDeactivation( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedBaseInputNode && m_cachedBaseInputNode->IsActive( instance ) )
	{
		m_cachedBaseInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphAnimationSlotNode::SetSlotLogicAction( CBehaviorGraphInstance& instance, ESlotAction action ) const
{
	instance[ i_slotAction ] = action;
}

void CBehaviorGraphAnimationSlotNode::ProcessSlotLogicAction( CBehaviorGraphInstance& instance ) const
{
	Int32& action = instance[ i_slotAction ];

	if ( action == SA_BlendOut )
	{
		AnimationSlotBlendOutStarted( instance );
	}
	else if ( action == SA_Finish )
	{
		AnimationSlotFinished( instance );
	}

	action = SA_None;
}

Bool CBehaviorGraphAnimationSlotNode::IsSlotPoseMimic() const
{
	return false;
}

void CBehaviorGraphAnimationSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	if ( IsSlotActive( instance ) && IsValid( instance ) )
	{
		ASSERT( IsPlayingSlotAnimation( instance ) );

		// Blending
		if ( IsBlending( instance ) )
		{
			// Blend weight
			Float weight = GetBlendWeight( instance );
			ASSERT( weight >= 0.f && weight <= 1.f );

			if ( HasCachedPose( instance ) )
			{
				ASSERT( HasCachedPose( instance ) );

				// Get pose
				CCacheBehaviorGraphOutput cacheSlotPose( context, IsSlotPoseMimic() );
				SBehaviorGraphOutput* slotPose = cacheSlotPose.GetPose();

				// Get cached pose
				SBehaviorGraphOutput* cachedPose = instance[ i_pose ].GetPose();
				ASSERT( cachedPose );

				if ( cachedPose && slotPose )
				{
					// Sample animations
					SampleSlotAnimation( context, instance, *slotPose );

					// Blend
					BlendPoses( instance, context, output, *cachedPose, *slotPose, weight );
				}
				else
				{
					return;
				}

#ifdef SLOT_DEBUG
				BEH_LOG(TXT("Blend with cached pose: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif
			}
			else
			{
				// Get poses
				CCacheBehaviorGraphOutput cacheSlotPose( context, IsSlotPoseMimic() );
				CCacheBehaviorGraphOutput cacheInputPose( context, IsSlotPoseMimic() );

				SBehaviorGraphOutput* slotPose = cacheSlotPose.GetPose();
				SBehaviorGraphOutput* inputPose = cacheInputPose.GetPose();

				if ( slotPose && inputPose )
				{
					// Sample animations
					SampleSlotAnimation( context, instance, *slotPose );
					SampleInputAnimation( context, instance, *inputPose );

					// Blend
					BlendPoses( instance, context, output, *inputPose, *slotPose, weight );
				}

#ifdef SLOT_DEBUG
				BEH_LOG(TXT("Blend with input: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif
			}
		}
		else
		{
			// Sample only slot animation
			SampleSlotAnimation( context, instance, output );
		}

		// Add animation shift to translation
		SSlotAnimationShift* shifts = GetSlotShift( instance );

		if ( shifts->m_animationShift != Vector::ZEROS )
		{
#ifdef USE_HAVOK_ANIMATION
			hkVector4 hkTranslation  = output.m_deltaReferenceFrameLocal.getTranslation();
			const hkVector4& hkDelta = TO_CONST_HK_VECTOR_REF( shifts->m_animationShift );
			hkTranslation.add3clobberW( hkDelta );
			output.m_deltaReferenceFrameLocal.setTranslation( hkTranslation );
#else
			RedVector4 translation  = output.m_deltaReferenceFrameLocal.GetTranslation();
			const RedVector4& delta = reinterpret_cast< const RedVector4& >( shifts->m_animationShift );
			translation.X += delta.X;
			translation.Y += delta.Y;
			translation.Z += delta.Z;

			output.m_deltaReferenceFrameLocal.SetTranslation( translation );
#endif
			shifts->m_animationShift = Vector::ZEROS;
		}

		// Process slot logic action after sampled last pose
		ProcessSlotLogicAction( instance );
	}
	else
	{
		// Sample base input
		SampleInputAnimation( context, instance, output );
	}
}

void CBehaviorGraphAnimationSlotNode::BlendPoses( CBehaviorGraphInstance& instance, SBehaviorSampleContext& context, SBehaviorGraphOutput &output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float inputWeight ) const
{
	const Float weight = BehaviorUtils::BezierInterpolation( inputWeight );

	// Poses
#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() <= BL_Lod2 )
	{
		output.SetInterpolate( poseA, poseB, weight );
	}
	else
	{
		output.SetInterpolateME( poseA, poseB, weight );
	}
#else
	output.SetInterpolate( poseA, poseB, weight );
#endif

	output.MergeUsedAnims( poseA, poseB, 1.f - weight, weight );

	// Merge events
	if ( instance[ i_mergeBlendedSlotEvents ] )
	{
		output.MergeEvents( poseA, poseB, 1.f - weight, weight );
	}
	else
	{
		// assuming poseB is a slot pose
		output.MergeEvents( poseA, 1.0f - weight );
	}

	// Check motion
	if ( IsBlendIn( instance ) )
	{
		if ( instance[ i_blendInType ] == BTBM_Source )
		{
#ifdef USE_HAVOK_ANIMATION
			output.m_deltaReferenceFrameLocal.setIdentity();
#else
			output.m_deltaReferenceFrameLocal.SetIdentity();
#endif
		}
		else if ( instance[ i_blendInType ] == BTBM_Destination )
		{
			output.m_deltaReferenceFrameLocal = poseB.m_deltaReferenceFrameLocal;
		}
	}
	else if ( IsBlendOut( instance ) || IsBlendingOnFinish(instance) )
	{
		if ( instance[ i_blendOutType ] == BTBM_Source )
		{
			output.m_deltaReferenceFrameLocal = poseA.m_deltaReferenceFrameLocal;
		}
		else if ( instance[ i_blendOutType ] == BTBM_Destination )
		{
			output.m_deltaReferenceFrameLocal = poseB.m_deltaReferenceFrameLocal;
		}
	}
}

void CBehaviorGraphAnimationSlotNode::SampleSlotAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CBehaviorGraphAnimationNode::Sample( context, instance, output );

#ifdef TPOSE_DETECTOR
	if ( instance.GetAnimatedComponent()->GetEntity()->QueryActorInterface() )
	{
		ASSERT( !output.IsTPose() );
	}
#endif
}

void CBehaviorGraphAnimationSlotNode::SampleInputAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( m_cachedBaseInputNode )
	{
		ASSERT( m_cachedBaseInputNode->IsActive( instance ) );

		// Sample base input
		m_cachedBaseInputNode->Sample( context, instance, output );
	}

#ifdef TPOSE_DETECTOR
	if ( instance.GetAnimatedComponent()->GetEntity()->QueryActorInterface() )
	{
		ASSERT( !output.IsTPose() );
	}
#endif
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphAnimationSlotNode::CollectAnimationUsageData( const CBehaviorGraphInstance& constInstance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const
{
	// TODO
	CBehaviorGraphInstance& instance = const_cast< CBehaviorGraphInstance& >( constInstance );
	if ( IsSlotActive( instance ) && IsValid( instance ) )
	{
		Float weight = IsBlending( instance ) ? GetBlendWeight( instance ) : 1.f;
		ASSERT( weight >= 0.f && weight <= 1.f );

		if ( const CSkeletalAnimationSetEntry* anim = instance[ i_animation ] )
		{
			SBehaviorUsedAnimationData usageInfo( anim, instance[ i_localTime ], weight );

			collectorArray.PushBack( usageInfo );
		}
	}
}
#endif

Float CBehaviorGraphAnimationSlotNode::GetBlendWeight( CBehaviorGraphInstance& instance ) const
{
	Float ret = 0.f;

	if ( IsBlendIn( instance ) )
	{
		ret = GetLocalTime( instance ) / instance[ i_blendInDuration ];
	}
	else if ( IsBlendingOnFinish(instance) )
	{
		ret = instance[ i_finishBlendingDurationLeft ] / instance[ i_blendOutDuration ];
	}
	else if ( IsBlendOut( instance ) )
	{
		ret = ( GetAnimDuration( instance ) - GetLocalTime( instance ) ) / instance[ i_blendOutDuration ];
	}
	else
	{
		ASSERT( 0 );
	};

	ASSERT( ret >= 0.f && ret <= 1.f );

	return ret;
}

Bool CBehaviorGraphAnimationSlotNode::IsBlendIn( CBehaviorGraphInstance& instance ) const
{
	const Int32 fistLoop = instance[ i_firstLoop ];
	//ASSERT( fistLoop != 2 );
	if ( fistLoop == 0 )
	{
		return false;
	}

	Float animTime = GetLocalTime( instance );
	Float blendInDur = instance[ i_blendInDuration ];
	return animTime < blendInDur;
}

Bool CBehaviorGraphAnimationSlotNode::IsBlendOut( CBehaviorGraphInstance& instance ) const
{
	if ( IsLooped( instance ) )
	{
		return false;
	}

	Float animTime = GetLocalTime( instance );
	Float animDur = GetAnimDuration( instance );
	Float blendOutDur = instance[ i_blendOutDuration ];

	return ( !m_cachedForceTimeNode && animTime > animDur - blendOutDur ) ? true : false;
}

Float CBehaviorGraphAnimationSlotNode::GetBlendTimer( CBehaviorGraphInstance& instance ) const
{
	return 0.f;
}

Bool CBehaviorGraphAnimationSlotNode::IsBlendInStarted( CBehaviorGraphInstance& instance ) const
{
	const Int32 fistLoop = instance[ i_firstLoop ];
	ASSERT( fistLoop != 2 );
	if ( fistLoop == 0 )
	{
		return false;
	}

	Float currTime = GetLocalTime( instance );
	Float prevTime = instance[ i_prevTime ];
	Float blendInTime = instance[ i_blendInDuration ];

	return blendInTime >= prevTime && blendInTime < currTime;
}

Bool CBehaviorGraphAnimationSlotNode::IsBlendOutStarted( CBehaviorGraphInstance& instance ) const
{
	if ( IsLooped( instance ) )
	{
		return false;
	}

	Float currTime = GetLocalTime( instance );
	Float prevTime = instance[ i_prevTime ];
	Float animDur = GetAnimDuration( instance );
	Float blendOutTime = animDur - instance[ i_blendOutDuration ];

	return blendOutTime >= prevTime && blendOutTime < currTime;
}

Bool CBehaviorGraphAnimationSlotNode::IsBlending( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_slotAction ] != SA_BlendOut && ( IsBlendIn( instance ) || IsBlendOut( instance ) || IsBlendingOnFinish(instance) );
}

Bool CBehaviorGraphAnimationSlotNode::IsBlendingWithCachedPose( CBehaviorGraphInstance& instance ) const
{
	return IsBlending( instance ) && HasCachedPose( instance );
}

Bool CBehaviorGraphAnimationSlotNode::IsBlendingWithInput( CBehaviorGraphInstance& instance ) const
{
	return IsBlending( instance ) && !HasCachedPose( instance );
}

Bool CBehaviorGraphAnimationSlotNode::IsAnimationSlotFinished( CBehaviorGraphInstance& instance ) const
{
	Float animTime = GetLocalTime( instance );
	Float animDur = GetAnimDuration( instance );
	return animTime >= animDur;
}

Bool CBehaviorGraphAnimationSlotNode::HasBlendOut( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_blendOutDuration ] > 0.f;
}

Bool CBehaviorGraphAnimationSlotNode::HasBlendIn( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_blendInDuration ] > 0.f;
}

Bool CBehaviorGraphAnimationSlotNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( IsSlotActive( instance ) )
	{
		Bool ret = TBaseClass::ProcessEvent( instance, event );

		if ( m_cachedBaseInputNode && IsBlending( instance ) )
		{
			ret |= m_cachedBaseInputNode->ProcessEvent( instance, event );
		}

		return ret;
	}
	else if ( m_cachedBaseInputNode ) 
	{
		return m_cachedBaseInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphAnimationSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedBaseInputNode )
	{
		m_cachedBaseInputNode->Activate( instance );
	}
}

void CBehaviorGraphAnimationSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( HasCachedPose( instance ) )
	{
		DestroyPose( instance );
	}

	TBaseClass::OnDeactivated( instance );

	if ( m_cachedBaseInputNode )
	{
		m_cachedBaseInputNode->Deactivate( instance );
	}

	SetSlotInactive( instance );
	SetSlotLogicAction( instance, SA_None );
}

void CBehaviorGraphAnimationSlotNode::SetupSlot( CBehaviorGraphInstance& instance, const SBehaviorSlotSetup* setup ) const
{
	TBaseClass::SetupSlot( instance, setup );

	// Set blending durations
	instance[ i_blendInDuration ] = setup->m_blendIn;
	instance[ i_blendOutDuration ] = setup->m_blendOut;
	instance[ i_finishBlendingDurationLeft ] = 0.0f; // If we were blending on finish, then stop it immediately.

	// Set blending types
	instance[ i_blendInType ] = setup->m_blendInType;
	instance[ i_blendOutType ] = setup->m_blendOutType;

	instance[ i_mergeBlendedSlotEvents ] = setup->m_mergeBlendedSlotEvents;
}

void CBehaviorGraphAnimationSlotNode::SetSlotActive( CBehaviorGraphInstance& instance ) const
{
	instance[ i_slotActive ] = true;
}

void CBehaviorGraphAnimationSlotNode::SetSlotInactive( CBehaviorGraphInstance& instance ) const
{
	instance[ i_slotActive ] = false;
}

void CBehaviorGraphAnimationSlotNode::StartAnimation( CBehaviorGraphInstance& instance ) const
{
	ASSERT( !instance[ i_isInTick ] );

#ifdef SLOT_DEBUG
	BEH_LOG(TXT("StartAnimation: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif

	if ( IsSlotActive( instance ) )
	{
#ifdef SLOT_DEBUG
		BEH_LOG(TXT("StartAnimation - slot active") );
#endif

		if ( !HasCachedPose( instance ) )
		{
#ifdef SLOT_DEBUG
			BEH_LOG(TXT("StartAnimation - create pose") );
#endif

			CreatePose( instance );
		}

		CachePose( instance );
	}

	// Set slot active - only here!
	SetSlotActive( instance );

	// Reset times, loops, flags etc. in base animation node logic
	InternalReset( instance );

	// Reset loop flag
	instance[ i_firstLoop ] = 1;

	Float duration = GetAnimDuration( instance );
	ASSERT( duration > 0.f );

	// Check input params
	Float& blendInDuration = instance[ i_blendInDuration ];
	Float& blendOutDuration = instance[ i_blendOutDuration ];

	if ( blendOutDuration + blendInDuration >= duration )
	{
		Float weight = blendInDuration / ( blendOutDuration + blendInDuration );
		blendInDuration  = duration * weight * 0.99f;
		blendOutDuration = duration * ( 1.f - weight ) * 0.99f;
	}
}

Bool CBehaviorGraphAnimationSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, 
													 const CName& animation, 
													 const SBehaviorSlotSetup* slotSetup /* = NULL  */) const
{
#ifndef NO_SLOT_ANIM
	CName animationName = GetAnimationFullName( animation );

	if ( SetRuntimeAnimationByName( instance, animationName ) && IsValid( instance ) )
	{
		if ( slotSetup )
		{
			SetupSlot( instance, slotSetup );
		}
		
		StartAnimation( instance );

		if ( m_startEvtName )
		{
			instance.GenerateEvent( m_startEvtName );
		}

		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

Bool CBehaviorGraphAnimationSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, 
													 CSkeletalAnimationSetEntry* skeletalAnimation, 
													 const SBehaviorSlotSetup* slotSetup /* = NULL  */) const
{
#ifndef NO_SLOT_ANIM
	if ( SetRuntimeAnimation( instance, skeletalAnimation ) && IsValid( instance ) )
	{
		if ( slotSetup )
		{
			SetupSlot( instance, slotSetup );
		}

		StartAnimation( instance );

		if ( m_startEvtName )
		{
			instance.GenerateEvent( m_startEvtName );
		}

		return true;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

Bool CBehaviorGraphAnimationSlotNode::StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime /* = 0.0f*/ ) const
{
	CSkeletalAnimationSetEntry* anim = instance[ i_animation ];
	if ( anim )
	{
		ASSERT( IsSlotActive( instance ) );

		if ( m_stopEvtName )
		{
			instance.GenerateEvent( m_stopEvtName );
		}

		// Stop with blend.
		if ( blendOutTime > 0.001f ) // Don't bother with small blending times.
		{
			instance[ i_finishBlendingDurationLeft ] = blendOutTime;
			instance[ i_blendOutDuration ] = blendOutTime;

			// Destroy stored pose, cuz we would like to blend with input.
			if ( HasCachedPose( instance ) )
			{
				DestroyPose( instance );
			}

			// Ok, so here is the tricky question: we should inform listener about ending
			// right now (logically animation is stopped here), or when blending is finished?
			// Currently second option is used.
		}
		else  // Stop playing anim at once(without blending).
		{
			StopAndDeactivate( instance );
		}
	}

	return instance[ i_animation ] == nullptr;
}

void CBehaviorGraphAnimationSlotNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	SlotReset( instance );

	instance[ i_slotActive ] = false;

	if ( HasCachedPose( instance ) )
	{
		DestroyPose( instance );
	}
}

Bool CBehaviorGraphAnimationSlotNode::IsLooped( CBehaviorGraphInstance& instance ) const
{
	return TBaseClass::IsLooped( instance );
}

void CBehaviorGraphAnimationSlotNode::SlotReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::SlotReset( instance );

	// Reset blend params
	instance[ i_blendInDuration ] = 0.f;
	instance[ i_blendOutDuration ] = 0.f;
	instance[ i_finishBlendingDurationLeft ] = 0.f;
	instance[ i_blendInType ] = 0;
	instance[ i_blendOutType ] = 0;
	instance[ i_mergeBlendedSlotEvents ] = true;
	instance[ i_firstLoop ] = 2;

	SetSlotLogicAction( instance, SA_None );
}

void CBehaviorGraphAnimationSlotNode::OnAnimationFinished( CBehaviorGraphInstance& instance ) const
{
	Int32& fistLoop = instance[ i_firstLoop ];
	ASSERT( fistLoop != 2 );
	fistLoop = 0;
}

void CBehaviorGraphAnimationSlotNode::AnimationSlotFinished( CBehaviorGraphInstance& instance ) const
{
	ASSERT( IsSlotActive( instance ) );

#ifdef SLOT_DEBUG
	BEH_LOG(TXT("AnimationSlotFinished: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif

	CSkeletalAnimationSetEntry* anim = instance[ i_animation ];
	if ( anim )
	{
		SlotReset( instance );

		if ( HasCachedPose( instance ) )
		{
			DestroyPose( instance );
		}

		// Listener
		if( instance[ i_slotAnimationListener ] != 0 )
		{
			// Listener copied to temporary variable so it can be set again in callback
			ISlotAnimationListener* listener = reinterpret_cast< ISlotAnimationListener* >( instance[ i_slotAnimationListener ] );

#ifdef DEBUG_SLOT_LISTENERS
			LOG_ENGINE( TXT("Anim slot OnDeactivated with listener '%ls', 0x%X, '%ls'"), listener->GetListenerName().AsChar(), listener, instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
#endif

			instance[ i_slotAnimationListener ] = 0;
			listener->OnSlotAnimationEnd( this, instance, ISlotAnimationListener::S_Finished );						
		}
	}
	else
	{
		ASSERT( anim );
	}
}

void CBehaviorGraphAnimationSlotNode::AnimationSlotBlendOutStarted( CBehaviorGraphInstance& instance ) const
{
	ASSERT( IsSlotActive( instance ) );

#ifdef SLOT_DEBUG
	BEH_LOG(TXT("AnimationSlotBlendOutStarted: prev %f, curr %f"), instance[ i_prevTime ], GetLocalTime( instance ) );
#endif

	CSkeletalAnimationSetEntry* anim = instance[ i_animation ];
	if ( anim )
	{
		if ( HasCachedPose( instance ) )
		{
			DestroyPose( instance );
		}

		// Listener
		if( instance[ i_slotAnimationListener ] != 0 )
		{
			// Listener copied to temporary variable so it can be set again in callback
			ISlotAnimationListener* listener = reinterpret_cast< ISlotAnimationListener* >( instance[ i_slotAnimationListener ] );

#ifdef DEBUG_SLOT_LISTENERS
			LOG_ENGINE( TXT("Anim slot OnDeactivated with listener '%ls', 0x%X, '%ls'"), listener->GetListenerName().AsChar(), listener, instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
#endif

			instance[ i_slotAnimationListener ] = 0;
			listener->OnSlotAnimationEnd( this, instance, ISlotAnimationListener::S_BlendOutStarted );
		}
	}
	else
	{
		ASSERT( anim );
	}
}

void CBehaviorGraphAnimationSlotNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( IsSlotActive( instance ) )
	{
		TBaseClass::GetSyncInfo( instance, info );
	}
	else if ( m_cachedBaseInputNode ) 
	{
		return m_cachedBaseInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphAnimationSlotNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( !IsSlotActive( instance ) && m_cachedBaseInputNode )
	{
		m_cachedBaseInputNode->SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphAnimationSlotNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedBaseInputNode )
	{
		if ( IsSlotActive( instance ) )
		{
			if ( ( IsBlendingOnFinish( instance) || IsBlendIn( instance ) ) && !HasCachedPose( instance ) )
			{
				m_cachedBaseInputNode->ProcessActivationAlpha( instance, alpha * ( 1.f - GetBlendWeight( instance ) ) );
			}
			else if ( IsBlendOut( instance ) && !HasCachedPose( instance ) )
			{
				m_cachedBaseInputNode->ProcessActivationAlpha( instance, alpha * GetBlendWeight( instance ) );
			}
		}
		else
		{
			m_cachedBaseInputNode->ProcessActivationAlpha( instance, alpha );
		}
	}
}

void CBehaviorGraphAnimationSlotNode::CreatePose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( !instance[ i_hasCachedPose ] );

	if ( !instance[ i_hasCachedPose ] )
	{
		instance[ i_pose ].Create( instance, IsSlotPoseMimic() );
		instance[ i_hasCachedPose ] = true;
	}
}

Bool CBehaviorGraphAnimationSlotNode::HasCachedPose( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_hasCachedPose ];
}

void CBehaviorGraphAnimationSlotNode::CachePose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_pose ].Cache( instance );
}

void CBehaviorGraphAnimationSlotNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_pose ].Free( instance );
	instance[ i_hasCachedPose ] = false;
}

Bool CBehaviorGraphAnimationSlotNode::IsSlotActive( const CBehaviorGraphInstance& instace ) const
{
	return instace[ i_slotActive ];
}

Float CBehaviorGraphAnimationSlotNode::GetLocalTime( CBehaviorGraphInstance& instace ) const
{
	return instace[ i_localTime ];
}

Bool CBehaviorGraphAnimationSlotNode::IsBlendingOnFinish(CBehaviorGraphInstance& instance) const
{
	 return instance[ i_finishBlendingDurationLeft ] > 0.0f;
}

void CBehaviorGraphAnimationSlotNode::StopAndDeactivate(CBehaviorGraphInstance& instance) const
{
	SlotReset( instance );

	if( instance[ i_slotAnimationListener ] != 0 )
	{
		// listener copied to temporary variable so it can be set again in callback
		ISlotAnimationListener* listener = reinterpret_cast< ISlotAnimationListener* >( instance[ i_slotAnimationListener ] );

#ifdef DEBUG_SLOT_LISTENERS
		LOG_ENGINE( TXT("Anim slot OnDeactivated with listener '%ls', 0x%X, '%ls'"), listener->GetListenerName().AsChar(), listener, instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
#endif

		instance[ i_slotAnimationListener ] = 0;
		listener->OnSlotAnimationEnd( this, instance, ISlotAnimationListener::S_Stopped );		
	}

	if ( !IsPlayingSlotAnimation( instance ) )
	{
		SetSlotInactive( instance );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicSlotNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( ForcedTime ), false ) );
}

String CBehaviorGraphMimicSlotNode::GetCaption() const
{
	return String::Printf( TXT("Blend mimic slot [ %s ]"), m_name.AsChar() );
}

#endif

void CBehaviorGraphMimicSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_useCachePose;
}

void CBehaviorGraphMimicSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_useCachePose ] = false;
}

void CBehaviorGraphMimicSlotNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	if ( TBaseClass::HasCachedPose( instance ) )
	{
		TBaseClass::DestroyPose( instance );
	}

	TBaseClass::OnReleaseInstance( instance );

	ASSERT( !instance[ i_pose ].m_pose );
}

void CBehaviorGraphMimicSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	ASSERT( !TBaseClass::HasCachedPose( instance ) );

	TBaseClass::CreatePose( instance );
}

void CBehaviorGraphMimicSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	ASSERT( TBaseClass::HasCachedPose( instance ) );

	TBaseClass::DestroyPose( instance );

	ASSERT( !TBaseClass::HasCachedPose( instance ) );
}

Bool CBehaviorGraphMimicSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, const CName& animation, const SBehaviorSlotSetup* slotSetup ) const
{
	BEH_LOG( TXT("Play mimic slot animation '%ls' on slot '%ls' for entity '%ls'"), animation.AsString().AsChar(), GetName().AsChar(), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	return TBaseClass::PlayAnimation( instance, animation, slotSetup );
}

Bool CBehaviorGraphMimicSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup ) const
{
	BEH_LOG( TXT("Play mimic slot animation '%ls' on slot '%ls' for entity '%ls'"), skeletalAnimation->GetName().AsString().AsChar(), GetName().AsChar(), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	return TBaseClass::PlayAnimation( instance, skeletalAnimation, slotSetup );
}

Bool CBehaviorGraphMimicSlotNode::StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime /*0.0f*/ ) const
{
	BEH_LOG( TXT("Stop mimic slot animation on slot '%ls' for entity '%ls'"), GetName().AsChar(), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	return TBaseClass::StopAnimation( instance, blendOutTime );
}

void CBehaviorGraphMimicSlotNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedBaseInputNode = CacheMimicBlock( TXT("In") );
}

Bool CBehaviorGraphMimicSlotNode::IsSlotPoseMimic() const
{
	return true;
}

Bool CBehaviorGraphMimicSlotNode::IsValid( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	ASSERT( ac );

	if ( !ac->GetMimicSkeleton() )
	{
		return false;
	}

	return true;
}

void CBehaviorGraphMimicSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( context.HasMimic() == false )
	{
		return;
	}

	TBaseClass::Sample( context, instance, output );

	ASSERT( TBaseClass::HasCachedPose( instance ) );

	if ( TBaseClass::HasCachedPose( instance ) )
	{
		CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
		ASSERT( pose.m_pose );

		if ( pose.m_pose )
		{
			*pose.m_pose = output;
		}
	}
}

void CBehaviorGraphMimicSlotNode::SampleSlotAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CBehaviorGraphMimicsAnimationNode::Sample( context, instance, output );
}

Bool CBehaviorGraphMimicSlotNode::IsLooped( CBehaviorGraphInstance& instance ) const
{
	return CBehaviorGraphAnimationBaseSlotNode::IsLooped( instance );
}

Bool CBehaviorGraphMimicSlotNode::HasCachedPose( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_useCachePose ];
}

void CBehaviorGraphMimicSlotNode::CachePose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( instance[ i_useCachePose ] );
}

void CBehaviorGraphMimicSlotNode::CreatePose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_useCachePose ] = true;
}

void CBehaviorGraphMimicSlotNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_useCachePose ] = false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationSlotWithCurveNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void CBehaviorGraphAnimationSlotWithCurveNode::OnSpawned( const GraphBlockSpawnInfo& info )
	{
		TBaseClass::OnSpawned( info );

		m_curve = CreateObject< CCurve >( this );
	}
#endif

Float CBehaviorGraphAnimationSlotWithCurveNode::GetBlendWeight( CBehaviorGraphInstance& instance ) const
{
	Float progress = TBaseClass::GetBlendWeight( instance );

	ASSERT( progress <= 1.f && progress >= 0.f );

	return Clamp( m_curve->GetFloatValue( progress ), 0.f, 1.f );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicSlotWithCurveNode );


#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphMimicSlotWithCurveNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_curve = CreateObject< CCurve >( this );
}
#endif

Float CBehaviorGraphMimicSlotWithCurveNode::GetBlendWeight( CBehaviorGraphInstance& instance ) const
{
	Float progress = TBaseClass::GetBlendWeight( instance );

	ASSERT( progress <= 1.f && progress >= 0.f );

	return Clamp( m_curve->GetFloatValue( progress ), 0.f, 1.f );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicSlotWithSwapingNode );

CBehaviorGraphMimicSlotWithSwapingNode::CBehaviorGraphMimicSlotWithSwapingNode()
	: m_from( 0 )
	, m_to( 0 )
{

}

void CBehaviorGraphMimicSlotWithSwapingNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_from < output.m_numFloatTracks && m_to < output.m_numFloatTracks )
	{
		Float& a = output.m_floatTracks[ m_from ];
		Float& b = output.m_floatTracks[ m_to ];

		Float temp = a;
		a = b;
		b = temp;
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SSlotEventAnim );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicEventSlotNode );

void CBehaviorGraphMimicEventSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_events;
}

void CBehaviorGraphMimicEventSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const Uint32 numAnims = m_animations.Size();

	TDynArray< Uint32 >& events = instance[ i_events ];
	events.Resize( numAnims );

	for ( Uint32 i=0; i<numAnims; ++i )
	{
		const SSlotEventAnim& a = m_animations[ i ];
		events[ i ] = instance.GetEventId( a.m_event );
	}
}

Bool CBehaviorGraphMimicEventSlotNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret( false );

	const TDynArray< Uint32 >& events = instance[ i_events ];
	const Uint32 numAnims = m_animations.Size();

	RED_ASSERT( numAnims == events.Size() );

	for ( Uint32 i=0; i<numAnims; ++i )
	{
		const Uint32 eventId = events[ i ];
		if ( eventId == event.GetEventID() )
		{
			const SSlotEventAnim& slotAnim = m_animations[ i ];
			const CName animationToPlay = slotAnim.m_animation;
			if ( animationToPlay )
			{
				SBehaviorSlotSetup ss;
				ss.m_blendIn = slotAnim.m_blendIn;
				ss.m_blendOut = slotAnim.m_blendOut;
				ss.m_looped = slotAnim.m_looped;
				PlayAnimation( instance, animationToPlay, &ss );
			}

			ret = true;
		}
	}

	ret |= TBaseClass::ProcessEvent( instance, event );
	
	return ret;
}

void CBehaviorGraphMimicEventSlotNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	for ( const SSlotEventAnim& a : m_animations )
	{
		if ( a.m_event )
		{
			events.PushBack( a.m_event );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
