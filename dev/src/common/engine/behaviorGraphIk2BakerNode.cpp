/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphIk2BakerNode.h"
#include "controlRigAlgorithms.h"
#include "behaviorProfiler.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphUtils.inl"
#include "../engine/animatedComponent.h"
#include "../engine/skeleton.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphIk2BakerNode );

CBehaviorGraphIk2BakerNode::CBehaviorGraphIk2BakerNode()
	: m_endBoneName( String::EMPTY )
	, m_blendInDuration( 0.1f )
	, m_blendOutDuration( 0.1f )
	, m_animEventName( CName::NONE )
	, m_defaultEventStartTime( 0.0f )
	, m_defaultEventEndTime( 0.0f )
	, m_hingeAxis( EAxis::A_Y )
	, m_enforceEndPosition( true )
	, m_bonePositionInWorldSpace( true )
	, m_enforceEndRotation( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIk2BakerNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Target ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetRot ) ) );
}

#endif

void CBehaviorGraphIk2BakerNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_zeroBoneIdx;
	compiler << i_firstBoneIdx;
	compiler << i_secondBoneIdx;
	compiler << i_endBoneIdx;

	compiler << i_blendInDuration;
	compiler << i_blendOutDuration;

	compiler << i_isAnimEventActive;
	compiler << i_currentAnimEventTime;
	compiler << i_animEventDuration;

	compiler << i_needToRecalculateAdditiveTransforms;

	compiler << i_weight;

	compiler << i_targetPos;
	compiler << i_targetRot;

	compiler << i_firstBoneAdditiveTransform;
	compiler << i_secondBoneAdditiveTransform;
	compiler << i_endBoneAdditiveTransform;
}

void CBehaviorGraphIk2BakerNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	Int32& endBoneIdx = instance[ i_endBoneIdx ];
	Int32& secondBoneIdx = instance[ i_secondBoneIdx ];
	Int32& firstBoneIdx = instance[ i_firstBoneIdx ];
	Int32& zeroBoneIdx = instance[ i_zeroBoneIdx ];

	endBoneIdx = FindBoneIndex( m_endBoneName, instance );
	if ( const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton() )
	{
		secondBoneIdx = skeleton->GetParentBoneIndex( endBoneIdx );
		firstBoneIdx = skeleton->GetParentBoneIndex( secondBoneIdx );
		zeroBoneIdx = skeleton->GetParentBoneIndex( firstBoneIdx );
	}
	else
	{
		secondBoneIdx = -1;
		firstBoneIdx = -1;
		zeroBoneIdx = -1;
	}

	instance[ i_blendInDuration ] = m_blendInDuration;
	instance[ i_blendOutDuration ] = m_blendOutDuration;

	instance[ i_isAnimEventActive ] = false;
	instance[ i_currentAnimEventTime ] = 0.f;
	instance[ i_animEventDuration ] = 0.f;

	instance[ i_needToRecalculateAdditiveTransforms ] = false;

	instance[ i_weight ] = 0.f;

	instance[ i_targetPos ] = Vector::ZERO_3D_POINT;
	instance[ i_targetRot ] = EulerAngles::ZEROS;

	instance[ i_firstBoneAdditiveTransform ] = EngineQsTransform::IDENTITY;
	instance[ i_secondBoneAdditiveTransform ] = EngineQsTransform::IDENTITY;
	instance[ i_endBoneAdditiveTransform ] = EngineQsTransform::IDENTITY;

	CheckBones( instance );
}

void CBehaviorGraphIk2BakerNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_zeroBoneIdx );
	INST_PROP( i_firstBoneIdx );
	INST_PROP( i_secondBoneIdx );
	INST_PROP( i_endBoneIdx );

	INST_PROP( i_blendInDuration );
	INST_PROP( i_blendOutDuration );

	INST_PROP( i_isAnimEventActive );
	INST_PROP( i_currentAnimEventTime );
	INST_PROP( i_animEventDuration );

	INST_PROP( i_needToRecalculateAdditiveTransforms );

	INST_PROP( i_weight );

	INST_PROP( i_targetPos );
	INST_PROP( i_targetRot );

	INST_PROP( i_firstBoneAdditiveTransform );
	INST_PROP( i_secondBoneAdditiveTransform );
	INST_PROP( i_endBoneAdditiveTransform );
}

void CBehaviorGraphIk2BakerNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Ik2Baker );

	TBaseClass::OnUpdate( context, instance, timeDelta );
	
	Bool& isAnimEventActive = instance[ i_isAnimEventActive ];
	Float& currentAnimEventTime = instance[ i_currentAnimEventTime ];

	if ( isAnimEventActive )
	{
		currentAnimEventTime += timeDelta;
	}

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );

		Float& weight = instance[ i_weight ];
		Float prevWeight = weight;
		weight = m_cachedValueNode->GetValue( instance );
		if ( AreEqual( weight, 1.f ) && !AreEqual( prevWeight, 1.f ) )
		{
			instance[ i_needToRecalculateAdditiveTransforms ] = true;
		}
	}
	else
	{
		instance[ i_weight ] = 0.f;
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Update( context, instance, timeDelta );

		instance[ i_targetPos ] = m_cachedTargetPosNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_targetPos ] = Vector::ZERO_3D_POINT;
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->Update( context, instance, timeDelta );

		instance[ i_targetRot ] = BehaviorUtils::VectorToAngles( m_cachedTargetRotNode->GetVectorValue( instance ) );
	}
	else
	{
		instance[ i_targetRot ] = EulerAngles::ZEROS;
	}

	if ( isAnimEventActive && currentAnimEventTime >= instance[ i_animEventDuration ] )
	{
		isAnimEventActive = false;
	}
}

void CBehaviorGraphIk2BakerNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( Ik2Baker );

	TBaseClass::Sample( context, instance, output );

	Int32 firstBoneIdx = instance[ i_firstBoneIdx ];
	Int32 secondBoneIdx = instance[ i_secondBoneIdx ];
	Int32 endBoneIdx = instance[ i_endBoneIdx ];
	Float weight = instance[ i_weight ];
	Bool& isAnimEventActive = instance[ i_isAnimEventActive ];

	if ( firstBoneIdx == -1 || secondBoneIdx == -1 || endBoneIdx == -1 )
	{
		return;
	}

	Bool isWeightEqualOne = AreEqual( weight, 1.f );
	if ( isWeightEqualOne && !isAnimEventActive )
	{
		Bool animEventFound = false;
		Float currentAnimEventTime;
		Float animEventDuration;

		// Try to find event with specified name
		if ( m_animEventName != CName::NONE )
		{
			for ( Uint32 i = 0; i < output.m_numEventsFired; ++i )
			{
				const CAnimationEventFired& event = output.m_eventsFired[ i ];
				if ( event.GetEventName() == m_animEventName && ( event.m_type == EAnimationEventType::AET_DurationStart ||  event.m_type == EAnimationEventType::AET_Duration ) )
				{
					animEventFound = true;
					currentAnimEventTime = event.m_animInfo.m_localTime - event.m_extEvent->GetStartTime();
					animEventDuration = event.m_animInfo.m_eventDuration;
					break;
				}
			}
		}
		// Get default artificial event when no name is provided
		else
		{
			CSyncInfo info;
			GetSyncInfo( instance, info );
			if ( m_defaultEventStartTime <= info.m_currTime && info.m_currTime <= m_defaultEventEndTime )
			{
				animEventFound = true;
				currentAnimEventTime = info.m_currTime - m_defaultEventStartTime;
				animEventDuration = m_defaultEventEndTime - m_defaultEventStartTime;
			}
		}

		if ( animEventFound )
		{
			if ( Bool& needToRecalculateAdditiveTransforms = instance[ i_needToRecalculateAdditiveTransforms ] )
			{
				UpdateAdditiveTransforms( instance, output );
				needToRecalculateAdditiveTransforms = false;
			}

			isAnimEventActive = true;
			instance[ i_currentAnimEventTime ] = currentAnimEventTime;
			instance[ i_animEventDuration ] = animEventDuration;

			CheckBlendTimes( instance );
		}
	}
		
	if ( isWeightEqualOne && isAnimEventActive )
	{
		AnimQsTransform transformWithAdditive;
		Float blendWeight = CalculateBlendWeight( instance );

		if ( blendWeight > 0.f )
		{
			transformWithAdditive.SetMul( ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( instance[ i_firstBoneAdditiveTransform ] ), output.m_outputPose[ firstBoneIdx ] );
			output.m_outputPose[ firstBoneIdx ].Lerp( output.m_outputPose[ firstBoneIdx ], transformWithAdditive, blendWeight );

			transformWithAdditive.SetMul( ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( instance[ i_secondBoneAdditiveTransform ] ), output.m_outputPose[ secondBoneIdx ] );
			output.m_outputPose[ secondBoneIdx ].Lerp( output.m_outputPose[ secondBoneIdx ], transformWithAdditive, blendWeight );

			transformWithAdditive.SetMul( ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( instance[ i_endBoneAdditiveTransform ] ), output.m_outputPose[ endBoneIdx ] );
			output.m_outputPose[ endBoneIdx ].Lerp( output.m_outputPose[ endBoneIdx ], transformWithAdditive, blendWeight );
		}
	}
}

void CBehaviorGraphIk2BakerNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_isAnimEventActive ] = false;
	instance[ i_weight ] = 0.f;
}

void CBehaviorGraphIk2BakerNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_isAnimEventActive ] = false;
	instance[ i_weight ] = 0.f;

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Activate( instance );
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->Activate( instance );
	}
}

void CBehaviorGraphIk2BakerNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Deactivate( instance );
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->Deactivate( instance );
	}
}

void CBehaviorGraphIk2BakerNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedValueNode = CacheValueBlock( TXT( "Weight" ) );
	m_cachedTargetPosNode = CacheVectorValueBlock( TXT( "Target" ) );
	m_cachedTargetRotNode = CacheVectorValueBlock( TXT( "TargetRot" ) );
}

void CBehaviorGraphIk2BakerNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphIk2BakerNode::CheckBones( CBehaviorGraphInstance& instance ) const
{
	Int32& zeroBoneIdx = instance[ i_zeroBoneIdx ];
	Int32& firstBoneIdx = instance[ i_firstBoneIdx ];
	Int32& secondBoneIdx = instance[ i_secondBoneIdx ];
	Int32& endBoneIdx = instance[ i_endBoneIdx ];

	if ( zeroBoneIdx == -1 || firstBoneIdx == -1 || secondBoneIdx == -1 || endBoneIdx == -1 )
	{
		zeroBoneIdx = -1;
		firstBoneIdx = -1;
		secondBoneIdx = -1;
		endBoneIdx = -1;
	}
}

void CBehaviorGraphIk2BakerNode::CheckBlendTimes( CBehaviorGraphInstance& instance ) const
{
	Float& blendInDuration = instance[ i_blendInDuration ];
	Float& blendOutDuration = instance[ i_blendOutDuration ];
	Float animEventDuration = instance[ i_animEventDuration ];

	blendInDuration = m_blendInDuration; // Reset blend durations
	blendOutDuration = m_blendOutDuration;

	if ( blendInDuration + blendOutDuration > animEventDuration )
	{
		Float adjustingFactor = animEventDuration / ( blendInDuration + blendOutDuration );
		blendInDuration *= adjustingFactor;
		blendOutDuration *= adjustingFactor;
	}
}

void CBehaviorGraphIk2BakerNode::UpdateAdditiveTransforms( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	Ik2Baker::Input bakerInput;
	Ik2Baker::Output bakerOutput;

	bakerInput.m_firstJointParentTransformMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), instance[ i_zeroBoneIdx ] );
	bakerInput.m_firstJointLS = output.m_outputPose[ instance[ i_firstBoneIdx ] ];
	bakerInput.m_secondJointLS = output.m_outputPose[ instance[ i_secondBoneIdx ] ];
	bakerInput.m_endBoneLS = output.m_outputPose[ instance[ i_endBoneIdx ] ];
	bakerInput.m_hingeAxisLS =  VectorToAnimVector( BehaviorUtils::VectorFromAxis( m_hingeAxis ) );
	bakerInput.m_endTargetMS = VectorToAnimVector( instance[ i_targetPos ] );
	if ( m_bonePositionInWorldSpace )
	{
		bakerInput.m_endTargetMS = VectorToAnimVector( instance.GetAnimatedComponent()->GetLocalToWorld().FullInverted().TransformPoint( ANIM_VECTOR_TO_CONST_VECTOR_REF( bakerInput.m_endTargetMS ) ) );
	}
	bakerInput.m_endTargetRotationMS.Quat = VectorToAnimVector( instance[ i_targetRot ].ToQuat() );
	bakerInput.m_enforceEndPosition = m_enforceEndPosition;
	bakerInput.m_enforceEndRotation = m_enforceEndRotation;

	Ik2Baker::Bake( bakerInput, bakerOutput );

	instance[ i_firstBoneAdditiveTransform ] = ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( bakerOutput.m_firstBoneAdditive ); 
	instance[ i_secondBoneAdditiveTransform ] = ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( bakerOutput.m_secondBoneAdditive );
	instance[ i_endBoneAdditiveTransform ] = ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( bakerOutput.m_endBoneAdditive );
}

Float CBehaviorGraphIk2BakerNode::CalculateBlendWeight( CBehaviorGraphInstance& instance ) const
{
	Float currentTime = instance[ i_currentAnimEventTime ];
	Float animEventDuration = instance[ i_animEventDuration ];
	Float blendInTime = instance[ i_blendInDuration ];
	Float blendOutTime = instance[ i_blendOutDuration ];

	Float blendWeight;
	if ( currentTime < blendInTime )
	{
		blendWeight = currentTime / blendInTime;
	}
	else if ( currentTime > animEventDuration -  blendOutTime )
	{
		blendWeight = ( animEventDuration - currentTime ) / blendOutTime; 
	}
	else
	{
		blendWeight = 1.f;
	}

	return blendWeight;
}

Bool CBehaviorGraphIk2BakerNode::AreEqual( Float arg1, Float arg2, Float epsilon ) const
{
	return Red::Math::MAbs( arg1 - arg2 ) < epsilon;
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
