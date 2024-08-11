/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "behaviorGraphLookAtUsingAnimationsNode.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphUtils.inl"
#include "behaviorGraphSocket.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "animSyncInfo.h"
#include "behaviorIncludes.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLookAtUsingAnimationsProcessingNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLookAtUsingAnimationsCommonBaseNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLookAtUsingAnimationsNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLookAtUsingEmbeddedAnimationsNode );
IMPLEMENT_ENGINE_CLASS( SLookAtAnimationPairDefinition );
IMPLEMENT_ENGINE_CLASS( SLookAtAnimationPairInstance );
IMPLEMENT_ENGINE_CLASS( SLookAtAnimationPairInputBasedDefinition );
IMPLEMENT_ENGINE_CLASS( SLookAtAnimationPairInputBasedInstance );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( AddedVertical );
RED_DEFINE_STATIC_NAME( AddedHorizontal );
RED_DEFINE_STATIC_NAME( LookAtTarget );
RED_DEFINE_STATIC_NAME( LookAtBlendTime );
RED_DEFINE_STATIC_NAME( LookAtTargetDirMS );
RED_DEFINE_STATIC_NAME( OutLookAtTarget );
RED_DEFINE_STATIC_NAME( OnOff );
RED_DEFINE_STATIC_NAME( DisallowLookAt );

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphLookAtUsingAnimationsProcessingNode::CBehaviorGraphLookAtUsingAnimationsProcessingNode()
	: m_angleLimit( 90.0f, 90.0f )
	, m_animationAngleRange( 90.0f, 90.0f )
	, m_angleToStopLooking( 135.0f )
	, m_lookAtSpeed( 180.0f )
	, m_lookAtBlendTime( 0.2f )
	, m_controlValueBlendTime( 0.2f )
	, m_defaultControlValue( 1.0f )
	, m_bone( CNAME( head ) )
	, m_useCharacterRot( false )
{
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIdx;
	compiler << i_fwdAxis;
	compiler << i_controlValue;
	compiler << i_lookAtTargetDirMS;
	compiler << i_lookAt;
	compiler << i_storedTimeDelta;
	compiler << i_isLooking;
	compiler << i_justActivated;
	compiler << i_eventBlockingControlValue;
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();

	instance[ i_boneIdx ] = skeleton->FindBoneByName( m_bone );
	instance[ i_controlValue ] = 0.f;
	instance[ i_lookAtTargetDirMS ] = Vector::ZEROS;
	instance[ i_lookAt ] = Vector2( 0.0f, 0.0f );
	instance[ i_storedTimeDelta ] = 0.0f;
	instance[ i_isLooking ] = false;
	instance[ i_justActivated ] = false;
	instance[ i_eventBlockingControlValue ] = 0.0f;

	const Int32& boneIdx = instance[ i_boneIdx ];
	EAxis& fwdAxis = instance[ i_fwdAxis ];
	if ( boneIdx != -1 )
	{
		// find axis that points in looking dir (assume it is along Y axis)
		const Matrix boneMS = skeleton->GetBoneMatrixMS( boneIdx );
		const Vector xAxis = boneMS.GetAxisX();
		const Vector yAxis = boneMS.GetAxisY();
		const Vector zAxis = boneMS.GetAxisZ();
		const Float xAxisYabs = Abs( xAxis.Y );
		const Float yAxisYabs = Abs( yAxis.Y );
		const Float zAxisYabs = Abs( zAxis.Y );
		if ( xAxisYabs > yAxisYabs )
		{
			if ( xAxisYabs > zAxisYabs )
			{
				fwdAxis = xAxis.Y > 0.0f? A_X : A_NX;
			}
			else
			{
				fwdAxis = zAxis.Y > 0.0f? A_Z : A_NZ;
			}
		}
		else
		{
			if ( yAxisYabs > zAxisYabs )
			{
				fwdAxis = yAxis.Y > 0.0f? A_Y : A_NY;
			}
			else
			{
				fwdAxis = zAxis.Y > 0.0f? A_Z : A_NZ;
			}
		}
	}
	else
	{
		fwdAxis = A_X;
	}
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIdx );
	INST_PROP( i_controlValue );
	INST_PROP( i_lookAtTargetDirMS );
	INST_PROP( i_lookAt );
	INST_PROP( i_storedTimeDelta );
	INST_PROP( i_isLooking );
	INST_PROP( i_justActivated );
	INST_PROP( i_eventBlockingControlValue );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( OutLookAtTarget ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( LookAtTarget ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( OnOff ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( LookAtBlendTime ), false ) );
}

#endif

Float CBehaviorGraphLookAtUsingAnimationsProcessingNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return 0.0f;
}

Vector CBehaviorGraphLookAtUsingAnimationsProcessingNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_lookAtTargetDirMS ];
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Base") );
	m_cachedLookAtVariableNode = CacheVectorValueBlock( TXT("LookAtTarget") );
	m_cachedControlVariableNode = CacheValueBlock( TXT("OnOff") );
	m_cachedLookAtBlendTimeNode = CacheValueBlock( TXT("LookAtBlendTime") );
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// update base
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	// update variables
	if ( m_cachedLookAtVariableNode )
	{
		m_cachedLookAtVariableNode->Update( context, instance, timeDelta );
	}
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
	}

	// copy variable value (so it's constant across update and sample)
	UpdateControlValue( instance, timeDelta, instance[ i_justActivated ] );

	instance[ i_storedTimeDelta ] = timeDelta;
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::UpdateControlValue( CBehaviorGraphInstance& instance, Float timeDelta, Bool dontBlend ) const
{
	Float desiredControlValue = m_cachedControlVariableNode ? m_cachedControlVariableNode->GetValue( instance ) : m_defaultControlValue;
	desiredControlValue *= Clamp( 1.0f - instance[ i_eventBlockingControlValue ], 0.0f, 1.0f );
	Float& controlValue = instance[ i_controlValue ];
	controlValue = dontBlend? desiredControlValue : BlendToWithBlendTime( controlValue, desiredControlValue, m_controlValueBlendTime, timeDelta );
	controlValue = controlValue < 0.001f? 0.0f : controlValue;
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::UpdateEventBlockingControlValue( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	Float& eventBlockingControlValue = instance[ i_eventBlockingControlValue ];
	eventBlockingControlValue = 0.0f;
	CAnimationEventFired* firedEvent = output.m_eventsFired;
	for ( Int32 idx = output.m_numEventsFired; idx > 0; -- idx, ++ firedEvent )
	{
		if ( firedEvent->GetEventName() == CNAME( DisallowLookAt ) )
		{
			eventBlockingControlValue += firedEvent->m_alpha;
		}
	}
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::UpdateLookAt( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput * output ) const
{
	// do it here, we won't care that much about one frame delay in normal update, but when just activated it might be very, very important
	if ( output )
	{
		UpdateEventBlockingControlValue( instance, *output );
	}
	Bool& justActivated = instance[ i_justActivated ];
	if ( justActivated )
	{
		UpdateControlValue( instance, 0.0f, true );
	}
	const Float timeDelta = instance [ i_storedTimeDelta ];
	const Float& controlValue = instance[ i_controlValue ];
	Vector& lookAtTargetDirMS = instance[ i_lookAtTargetDirMS ];
	Vector2& lookAt = instance[ i_lookAt ];
	const Int32& boneIdx = instance[ i_boneIdx ];
	Bool& isLooking = instance[ i_isLooking ];
	if ( controlValue > 0.0f)
	{
		Vector2 desiredLookAt = Vector2( 0.0f, 0.0f );

		if ( m_cachedLookAtVariableNode )
		{
			Vector lookAtWS = m_cachedLookAtVariableNode->GetVectorValue( instance );
			Vector lookFromWS = instance.GetAnimatedComponent()->GetWorldPosition();
			EulerAngles currentLookRotWS = instance.GetAnimatedComponent()->GetWorldRotation();
			// use provided bone
			if ( boneIdx != -1 && output )
			{
				const AnimQsTransform lookTMS = output->GetBoneModelTransform( instance.GetAnimatedComponent(), boneIdx );
				const Matrix lookMatMS = AnimQsTransformToMatrix( lookTMS );
				const Matrix lookMatWS = lookMatMS * instance.GetAnimatedComponent()->GetLocalToWorld();
				lookFromWS = lookMatWS.GetTranslation();
				if ( ! m_useCharacterRot )
				{
					Vector lookAxisWS;
					switch ( instance[ i_fwdAxis ] )
					{
					case A_X:	lookAxisWS = lookMatWS.GetAxisX(); break;
					case A_Y:	lookAxisWS = lookMatWS.GetAxisY(); break;
					case A_Z:	lookAxisWS = lookMatWS.GetAxisZ(); break;
					case A_NX:	lookAxisWS = -lookMatWS.GetAxisX(); break;
					case A_NY:	lookAxisWS = -lookMatWS.GetAxisY(); break;
					case A_NZ:	lookAxisWS = -lookMatWS.GetAxisZ(); break;
					default:	lookAxisWS = lookMatWS.GetAxisX(); break;
					}
					currentLookRotWS = lookAxisWS.ToEulerAngles();
				}
			}

			const Vector lookInDirWS = lookAtWS - lookFromWS;
			const EulerAngles lookInRotWS = lookInDirWS.ToEulerAngles();

			// here we assume that we do "look at" for characters that are upright in the world
			EulerAngles desiredDiff = lookInRotWS - currentLookRotWS;
			desiredDiff.Yaw = EulerAngles::NormalizeAngle180( desiredDiff.Yaw );
			desiredDiff.Pitch = EulerAngles::NormalizeAngle180( desiredDiff.Pitch );

			// check (with buffer) if we're out of range
			isLooking = Abs( desiredDiff.Yaw ) < ( m_angleToStopLooking + ( isLooking? 2.5f : -2.5f ) );
			if ( isLooking )
			{
				desiredLookAt = Vector2( Clamp( -desiredDiff.Yaw, -m_angleLimit.X, m_angleLimit.X ),
										 Clamp( -desiredDiff.Pitch, -m_angleLimit.Y, m_angleLimit.Y ) );
			}
		}

		lookAt = justActivated? desiredLookAt : BlendToWithBlendTimeAndSpeed( lookAt, desiredLookAt, GetLookAtBlendTime( instance ), m_lookAtSpeed, timeDelta ).AsVector2();
	}
	else
	{
		// force it to be 0, there's no blend anyway
		lookAt = Vector2( 0.0f, 0.0f );
	}

	Vector2 lookAtAnglesRad( DEG2RAD( lookAt.X ), DEG2RAD( lookAt.Y ) );
	Float cosY = Red::Math::MCos( lookAtAnglesRad.Y );
	lookAtTargetDirMS.X = cosY * Red::Math::MSin( lookAtAnglesRad.X  );
	lookAtTargetDirMS.Y = cosY * Red::Math::MCos( lookAtAnglesRad.X  );
	lookAtTargetDirMS.Z = Red::Math::MSin( lookAtAnglesRad.Y );

	justActivated = false;
}

Float CBehaviorGraphLookAtUsingAnimationsProcessingNode::GetLookAtBlendTime( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedLookAtBlendTimeNode )
	{
		return m_cachedLookAtBlendTimeNode->GetValue( instance );
	}
	else
	{
		return m_lookAtBlendTime;
	}
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	ANIM_NODE_PRE_SAMPLE

#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() >= BL_Lod3 )
	{
		return;
	}
#endif

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	// use pose from input as base pose
	UpdateLookAt( instance, &output );

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphLookAtUsingAnimationsProcessingNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = TBaseClass::ProcessEvent( instance, event );

	if ( m_cachedInputNode && m_cachedInputNode->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	return retVal;
}

Bool CBehaviorGraphLookAtUsingAnimationsProcessingNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedInputNode && m_cachedInputNode->ProcessForceEvent( instance, event ) ) 
	{
		retVal = true;
	}

	return retVal;
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_justActivated ] = true;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}

	if ( m_cachedLookAtVariableNode )
	{
		m_cachedLookAtVariableNode->Activate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	// we can safely deactivate both inputs
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Deactivate( instance );
	}

	if ( m_cachedLookAtVariableNode )
	{
		m_cachedLookAtVariableNode->Deactivate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphLookAtUsingAnimationsProcessingNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedLookAtVariableNode )
	{
		m_cachedLookAtVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphLookAtUsingAnimationsCommonBaseNode::CBehaviorGraphLookAtUsingAnimationsCommonBaseNode()
	: m_type( AT_Local )
	, m_horizontalFirst( true )
	, m_alternativeMapping( true )
{

}

Float CBehaviorGraphLookAtUsingAnimationsCommonBaseNode::MapAdditiveValue( Float val ) const
{
	return m_alternativeMapping? Clamp( val + 2.0f, 1.0f, 3.0f )
		: ( val <= 0.f ? Min( -val, 1.f ) : Min( val  + 2.f, 3.f ) );
}

void CBehaviorGraphLookAtUsingAnimationsCommonBaseNode::BlendVerticalAndHorizontalPoses( SBehaviorGraphOutput &output, Float controlValue, SBehaviorGraphOutput* vertical, SBehaviorGraphOutput* horizontal ) const
{
	// blend anims depending on strategy and what anims are available
	if ( m_type == AT_Ref )
	{
		if ( vertical && horizontal )
		{
			if ( m_horizontalFirst )
			{
				BehaviorUtils::BlendingUtils::BlendAdditive_Ref( output, output, *horizontal, controlValue );
				BehaviorUtils::BlendingUtils::BlendAdditive_Ref( output, output, *vertical, controlValue );
			}
			else
			{
				BehaviorUtils::BlendingUtils::BlendAdditive_Ref( output, output, *vertical, controlValue );
				BehaviorUtils::BlendingUtils::BlendAdditive_Ref( output, output, *horizontal, controlValue );
			}
		}
		else if ( vertical )
		{
			BehaviorUtils::BlendingUtils::BlendAdditive_Ref( output, output, *vertical, controlValue );
		}
		else if ( horizontal )
		{
			BehaviorUtils::BlendingUtils::BlendAdditive_Ref( output, output, *horizontal, controlValue );
		}
	}
	else if ( m_type == AT_Local )
	{
		if ( vertical && horizontal )
		{
			if ( m_horizontalFirst )
			{
				BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, output, *horizontal, controlValue );
				BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, output, *vertical, controlValue );
			}
			else
			{
				BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, output, *vertical, controlValue );
				BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, output, *horizontal, controlValue );
			}
		}
		else if ( vertical )
		{
			BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, output, *vertical, controlValue );
		}
		else if ( horizontal )
		{
			BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, output, *horizontal, controlValue );
		}
	}

	// merge events
	if ( vertical )
	{
		output.MergeEventsAndUsedAnimsAsAdditives( *vertical, controlValue );
	}
	if ( horizontal )
	{
		output.MergeEventsAndUsedAnimsAsAdditives( *horizontal, controlValue );
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphLookAtUsingAnimationsNode::CBehaviorGraphLookAtUsingAnimationsNode()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphLookAtUsingAnimationsNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( LookAtTargetDirMS ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( AddedVertical ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( AddedHorizontal ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( LookAtTarget ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( OnOff ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( LookAtBlendTime ), false ) );
}

#endif

void CBehaviorGraphLookAtUsingAnimationsNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedVerticalAdditiveInputNode = CacheBlock( TXT("AddedVertical") );
	m_cachedHorizontalAdditiveInputNode = CacheBlock( TXT("AddedHorizontal") );
}

void CBehaviorGraphLookAtUsingAnimationsNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// update inputs
	if ( AreAdditiveInputsActive( instance ) )
	{
		if ( m_cachedVerticalAdditiveInputNode )
		{
			m_cachedVerticalAdditiveInputNode->Update( context, instance, timeDelta );
		}
		if ( m_cachedHorizontalAdditiveInputNode )
		{
			m_cachedHorizontalAdditiveInputNode->Update( context, instance, timeDelta );
		}
	}

	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphLookAtUsingAnimationsNode::UpdateControlValue( CBehaviorGraphInstance& instance, Float timeDelta, Bool dontBlend ) const
{
	Float& controlValue = instance[ i_controlValue ];
	Float prevValue = controlValue;
	TBaseClass::UpdateControlValue( instance, timeDelta, dontBlend );
	// now activate or deactivate depending on control value
	if (controlValue == 0.0f && prevValue > 0.0f)
	{
		if ( m_cachedVerticalAdditiveInputNode )
		{
			m_cachedVerticalAdditiveInputNode->Deactivate( instance );
		}

		if ( m_cachedHorizontalAdditiveInputNode )
		{
			m_cachedHorizontalAdditiveInputNode->Deactivate( instance );
		}
	}
	if (controlValue > 0.0f && prevValue == 0.0f)
	{
		if ( m_cachedVerticalAdditiveInputNode )
		{
			m_cachedVerticalAdditiveInputNode->Activate( instance );
		}

		if ( m_cachedHorizontalAdditiveInputNode )
		{
			m_cachedHorizontalAdditiveInputNode->Activate( instance );
		}
	}
}

void CBehaviorGraphLookAtUsingAnimationsNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	ANIM_NODE_PRE_SAMPLE

#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() >= BL_Lod3 )
	{
		return;
	}
#endif

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	// use pose from input as base pose
	UpdateLookAt( instance, &output );

	const Float& controlValue = instance[ i_controlValue ];
	if ( controlValue != 0.0f )
	{
		CCacheBehaviorGraphOutput cachePoseVer( context );
		CCacheBehaviorGraphOutput cachePoseHor( context );

		SBehaviorGraphOutput* tempVer = cachePoseVer.GetPose();
		SBehaviorGraphOutput* tempHor = cachePoseHor.GetPose();

		if ( tempVer && tempHor )
		{
			tempVer->SetIdentity();
			tempHor->SetIdentity();

			const Vector2& lookAt = instance[ i_lookAt ];

			// update and sample anims
			if ( m_cachedVerticalAdditiveInputNode )
			{
				CSyncInfo syncInfo;
				Float val = Clamp( lookAt.Y / m_animationAngleRange.Y, -1.0f, 1.0f);
				syncInfo.m_currTime = MapAdditiveValue( val );
				syncInfo.m_prevTime = syncInfo.m_currTime;
				m_cachedVerticalAdditiveInputNode->SynchronizeTo( instance, syncInfo );
				m_cachedVerticalAdditiveInputNode->Sample( context, instance, *tempVer );
			}
			if ( m_cachedHorizontalAdditiveInputNode )
			{
				CSyncInfo syncInfo;
				Float val = Clamp( lookAt.X / m_animationAngleRange.X, -1.0f, 1.0f);
				syncInfo.m_currTime = MapAdditiveValue( val );
				syncInfo.m_prevTime = syncInfo.m_currTime;
				m_cachedHorizontalAdditiveInputNode->SynchronizeTo( instance, syncInfo );
				m_cachedHorizontalAdditiveInputNode->Sample( context, instance, *tempHor );
			}

			BlendVerticalAndHorizontalPoses( output, controlValue, m_cachedVerticalAdditiveInputNode? tempVer : NULL, m_cachedHorizontalAdditiveInputNode? tempHor : NULL );
		}
	}

	ANIM_NODE_POST_SAMPLE
}

Bool CBehaviorGraphLookAtUsingAnimationsNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = TBaseClass::ProcessEvent( instance, event );

	if ( AreAdditiveInputsActive( instance ) )
	{
		if ( m_cachedVerticalAdditiveInputNode && m_cachedVerticalAdditiveInputNode->ProcessEvent( instance, event ) )
		{
			retVal = true;
		}

		if ( m_cachedHorizontalAdditiveInputNode && m_cachedHorizontalAdditiveInputNode->ProcessEvent( instance, event ) )
		{
			retVal = true;
		}
	}

	return retVal;
}

Bool CBehaviorGraphLookAtUsingAnimationsNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = TBaseClass::ProcessForceEvent( instance, event );

	if ( AreAdditiveInputsActive( instance ) )
	{
		if ( m_cachedVerticalAdditiveInputNode && m_cachedVerticalAdditiveInputNode->ProcessForceEvent( instance, event ) )
		{
			retVal = true;
		}

		if ( m_cachedHorizontalAdditiveInputNode && m_cachedHorizontalAdditiveInputNode->ProcessForceEvent( instance, event ) )
		{
			retVal = true;
		}
	}

	return retVal;
}

void CBehaviorGraphLookAtUsingAnimationsNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( AreAdditiveInputsActive( instance ) )
	{
		const Vector2& lookAt = instance[ i_lookAt ];

		if ( m_cachedVerticalAdditiveInputNode )
		{
			m_cachedVerticalAdditiveInputNode->Activate( instance );
		}

		if ( m_cachedHorizontalAdditiveInputNode )
		{
			m_cachedHorizontalAdditiveInputNode->Activate( instance );
		}

		// use main pose - from last frame - should be good enough
		UpdateLookAt( instance, &instance.GetAnimatedComponent()->GetBehaviorGraphSampleContext()->GetMainPose() );

		// update and sample anims
		if ( m_cachedVerticalAdditiveInputNode )
		{
			CSyncInfo syncInfo;
			Float val = Clamp( lookAt.Y / m_animationAngleRange.Y, -1.0f, 1.0f);
			syncInfo.m_currTime = MapAdditiveValue( val );
			syncInfo.m_prevTime = syncInfo.m_currTime;
			m_cachedVerticalAdditiveInputNode->SynchronizeTo( instance, syncInfo );
		}
		if ( m_cachedHorizontalAdditiveInputNode )
		{
			CSyncInfo syncInfo;
			Float val = Clamp( lookAt.X / m_animationAngleRange.X, -1.0f, 1.0f);
			syncInfo.m_currTime = MapAdditiveValue( val );
			syncInfo.m_prevTime = syncInfo.m_currTime;
			m_cachedHorizontalAdditiveInputNode->SynchronizeTo( instance, syncInfo );
		}

	}
}

void CBehaviorGraphLookAtUsingAnimationsNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( AreAdditiveInputsActive( instance ) )
	{
		if ( m_cachedVerticalAdditiveInputNode )
		{
			m_cachedVerticalAdditiveInputNode->Deactivate( instance );
		}

		if ( m_cachedHorizontalAdditiveInputNode )
		{
			m_cachedHorizontalAdditiveInputNode->Deactivate( instance );
		}
	}
}

Bool CBehaviorGraphLookAtUsingAnimationsNode::AreAdditiveInputsActive( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_controlValue ] > 0.0f;
}

void CBehaviorGraphLookAtUsingAnimationsNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( AreAdditiveInputsActive( instance ) )
	{
		const Float& controlValue = instance[ i_controlValue ];
		if ( m_cachedVerticalAdditiveInputNode )
		{
			m_cachedVerticalAdditiveInputNode->ProcessActivationAlpha( instance, controlValue * alpha );
		}

		if ( m_cachedHorizontalAdditiveInputNode )
		{
			m_cachedHorizontalAdditiveInputNode->ProcessActivationAlpha( instance, controlValue * alpha );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void SLookAtAnimationPairDefinition::CollectUsedAnimations( TDynArray< CName >& anims ) const
{
	if ( ! m_additiveHorizontalAnimationName.Empty() )
	{
		anims.PushBackUnique( m_additiveHorizontalAnimationName );
	}
	if ( ! m_additiveVerticalAnimationName.Empty() )
	{
		anims.PushBackUnique( m_additiveVerticalAnimationName );
	}
}

//////////////////////////////////////////////////////////////////////////

void SLookAtAnimationPairInstance::Setup( CBehaviorGraphInstance& instance, const SLookAtAnimationPairDefinition& definition )
{
	CSkeletalAnimationContainer* animationContainer = instance.GetAnimatedComponent()->GetAnimationContainer();
	m_animation = definition.m_animationName.Empty()? NULL : animationContainer->FindAnimation( definition.m_animationName );
	m_additiveHorizontalAnimation = definition.m_additiveHorizontalAnimationName.Empty()? NULL : animationContainer->FindAnimation( definition.m_additiveHorizontalAnimationName );
	m_additiveVerticalAnimation = definition.m_additiveVerticalAnimationName.Empty()? NULL : animationContainer->FindAnimation( definition.m_additiveVerticalAnimationName );
}

//////////////////////////////////////////////////////////////////////////

void SLookAtAnimationPairInputBasedDefinition::CollectUsedAnimations( TDynArray< CName >& anims ) const
{
	if ( ! m_additiveHorizontalAnimationName.Empty() )
	{
		anims.PushBackUnique( m_additiveHorizontalAnimationName );
	}
	if ( ! m_additiveVerticalAnimationName.Empty() )
	{
		anims.PushBackUnique( m_additiveVerticalAnimationName );
	}
}

void SLookAtAnimationPairInputBasedDefinition::CacheConnections( CBehaviorGraphNode* owner )
{
	m_cachedInputNode = owner->CacheValueBlock( m_inputName );
}

//////////////////////////////////////////////////////////////////////////

void SLookAtAnimationPairInputBasedInstance::Setup( CBehaviorGraphInstance& instance, const SLookAtAnimationPairInputBasedDefinition& definition )
{
	CSkeletalAnimationContainer* animationContainer = instance.GetAnimatedComponent()->GetAnimationContainer();
	m_additiveHorizontalAnimation = definition.m_additiveHorizontalAnimationName.Empty()? NULL : animationContainer->FindAnimation( definition.m_additiveHorizontalAnimationName );
	m_additiveVerticalAnimation = definition.m_additiveVerticalAnimationName.Empty()? NULL : animationContainer->FindAnimation( definition.m_additiveVerticalAnimationName );
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::CBehaviorGraphLookAtUsingEmbeddedAnimationsNode()
	:	m_useHorizontalAnimations( true )
	,	m_useVerticalAnimations( true )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::iterator iPair = m_inputBasedPairs.Begin(); iPair != m_inputBasedPairs.End(); ++ iPair )
	{
		Bool ok = false;
		while ( ! ok )
		{
			ok = true;
			for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::iterator iPrevPair = m_inputBasedPairs.Begin(); iPrevPair != iPair; ++ iPrevPair )
			{
				if ( iPair->m_inputName == iPrevPair->m_inputName )
				{
					iPair->m_inputName += TXT("!");
					ok = false;
				}
			}
		}
		CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CName( iPair->m_inputName ) ) );
	}
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("Input based pairs") )
	{
		OnRebuildSockets();
	}
}

#endif

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_defaultPair;
	compiler << i_inputBasedPairs;
	compiler << i_pairs;
	compiler << i_horizontalPlaybacks;
	compiler << i_verticalPlaybacks;
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeDelta ] = 0.0f;

	// setup default pair
	instance[ i_defaultPair ].Setup( instance, m_defaultPair );

	// input based pairs
	TDynArray< SLookAtAnimationPairInputBasedInstance >& inputBasedPairs = instance[ i_inputBasedPairs ];
	inputBasedPairs.Resize( m_inputBasedPairs.Size() );
	TDynArray< SLookAtAnimationPairInputBasedInstance >::iterator instIBPair = inputBasedPairs.Begin();
	for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::const_iterator pairDef = m_inputBasedPairs.Begin(); pairDef != m_inputBasedPairs.End(); ++ pairDef, ++ instIBPair )
	{
		instIBPair->Setup( instance, *pairDef );
	}

	// animation pairs
	TDynArray< SLookAtAnimationPairInstance >& pairs = instance[ i_pairs ];
	pairs.Resize( m_pairs.Size() );
	TDynArray< SLookAtAnimationPairInstance >::iterator instPair = pairs.Begin();
	for ( TDynArray< SLookAtAnimationPairDefinition >::const_iterator pairDef = m_pairs.Begin(); pairDef != m_pairs.End(); ++ pairDef, ++ instPair )
	{
		instPair->Setup( instance, *pairDef );
	}
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_defaultPair );
	INST_PROP( i_inputBasedPairs );
	INST_PROP( i_pairs );
	INST_PROP( i_horizontalPlaybacks );
	INST_PROP( i_verticalPlaybacks );
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;

	for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::const_iterator iPair = m_inputBasedPairs.Begin(); iPair != m_inputBasedPairs.End(); ++ iPair )
	{
		if ( iPair->m_cachedInputNode )
		{
			iPair->m_cachedInputNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	ANIM_NODE_PRE_SAMPLE

#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() >= BL_Lod3 )
	{
		return;
	}
#endif

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	// use pose from input as base pose
	UpdateLookAt( instance, &output );

	const Float& controlValue = instance[ i_controlValue ];
	const Float& timeDelta = instance[ i_timeDelta ];
	if ( controlValue != 0.0f )
	{
		SSimpleAnimationPlaybackSet& horizontalPlaybacks = instance[ i_horizontalPlaybacks ];
		SSimpleAnimationPlaybackSet& verticalPlaybacks = instance[ i_verticalPlaybacks ];

		// ready for anim switch - this is to clear any anims that are not used
		if ( m_useHorizontalAnimations )
		{
			horizontalPlaybacks.ReadyForAnimSwitch();
		}
		if ( m_useVerticalAnimations )
		{
			verticalPlaybacks.ReadyForAnimSwitch();
		}

		// check what (normal) animations are used (in output's m_usedAnims)
		// find corresponding additive animation pairs for each used animation (or default or nothing)
		const SLookAtAnimationPairInstance& defaultPair = instance[ i_defaultPair ];
		const SBehaviorUsedAnimationData * usedAnim = output.m_usedAnims.m_anims.GetUsedData();
		Float defaultHorizontalWeight = 0.0f;
		Float defaultVerticalWeight = 0.0f;
		for ( Uint32 idx = 0; idx < output.m_usedAnims.m_anims.GetNum(); ++ idx, ++ usedAnim )
		{
			const SLookAtAnimationPairInstance* pair = FindPairFor( instance, usedAnim->m_animation );
			// use animation from default if not provided - TODO on option?
			if ( m_useHorizontalAnimations )
			{
				if ( pair && pair->m_additiveHorizontalAnimation )
				{
					horizontalPlaybacks.AddAnim( pair->m_additiveHorizontalAnimation, usedAnim->m_weight );
				}
				else
				{
					defaultHorizontalWeight += usedAnim->m_weight;
				}
			}
			if ( m_useVerticalAnimations )
			{
				if ( pair && pair->m_additiveVerticalAnimation )
				{
					verticalPlaybacks.AddAnim( pair->m_additiveVerticalAnimation, usedAnim->m_weight );
				}
				else
				{
					defaultVerticalWeight += usedAnim->m_weight;
				}
			}
		}
		// add default anims if needed
		if ( defaultHorizontalWeight > 0.0f || defaultVerticalWeight > 0.0f )
		{
			// use inputs if provided
			TDynArray< SLookAtAnimationPairInputBasedInstance >& pairs= instance[ i_inputBasedPairs ];
			// sum up weights
			Float inputWeightsSum = 0.0f;
			{
				TDynArray< SLookAtAnimationPairInputBasedInstance >::iterator iPair = pairs.Begin();
				TDynArray< SLookAtAnimationPairInputBasedDefinition >::const_iterator iPairDef = m_inputBasedPairs.Begin();
				for ( ; iPair != pairs.End(); ++ iPair, ++ iPairDef )
				{
					iPair->m_usageWeight = iPairDef->m_cachedInputNode? Clamp( iPairDef->m_cachedInputNode->GetValue( instance ), 0.0f, 1.0f ) : 0.0f;
				}
			}
			// use anims from pairs and normalize weights if needed
			{
				Float horizontalUsedWeight = 0.0f;
				Float verticalUsedWeight = 0.0f;
				Float normalizeWeight = inputWeightsSum > 1.0f ? 1.0f / inputWeightsSum : 1.0f;
				inputWeightsSum = Min( 1.0f, inputWeightsSum );
				TDynArray< SLookAtAnimationPairInputBasedInstance >::iterator iPair = pairs.Begin();
				for ( ; iPair != pairs.End(); ++ iPair )
				{
					iPair->m_usageWeight *= normalizeWeight;
					if ( m_useHorizontalAnimations && defaultHorizontalWeight > 0.0f && iPair->m_additiveHorizontalAnimation )
					{
						horizontalPlaybacks.AddAnim( iPair->m_additiveHorizontalAnimation, defaultHorizontalWeight * iPair->m_usageWeight );
						horizontalUsedWeight += iPair->m_usageWeight;
					}
					if ( m_useVerticalAnimations && defaultVerticalWeight > 0.0f && iPair->m_additiveVerticalAnimation )
					{
						horizontalPlaybacks.AddAnim( iPair->m_additiveVerticalAnimation, defaultVerticalWeight * iPair->m_usageWeight );
						verticalUsedWeight += iPair->m_usageWeight;
					}
				}
				if ( m_useHorizontalAnimations && defaultHorizontalWeight > 0.0f && horizontalUsedWeight < 1.0f && defaultPair.m_additiveHorizontalAnimation )
				{
					horizontalPlaybacks.AddAnim( defaultPair.m_additiveHorizontalAnimation, defaultHorizontalWeight * ( 1.0f - horizontalUsedWeight ) );
				}
				if ( m_useVerticalAnimations && defaultVerticalWeight > 0.0f && verticalUsedWeight < 1.0f && defaultPair.m_additiveVerticalAnimation )
				{
					verticalPlaybacks.AddAnim( defaultPair.m_additiveVerticalAnimation, defaultVerticalWeight * ( 1.0f - verticalUsedWeight ) );
				}
			}
		}

		// clear all unused
		if ( m_useHorizontalAnimations )
		{
			horizontalPlaybacks.SwitchOffAnimIfNotUsedAndReadyForSampling();
		}
		if ( m_useVerticalAnimations )
		{
			verticalPlaybacks.SwitchOffAnimIfNotUsedAndReadyForSampling();
		}

		// get temporary poses
		CCacheBehaviorGraphOutput cachePoseVer( context );
		CCacheBehaviorGraphOutput cachePoseHor( context );

		SBehaviorGraphOutput* tempVer = cachePoseVer.GetPose();
		SBehaviorGraphOutput* tempHor = cachePoseHor.GetPose();

		if ( tempVer && tempHor )
		{
			tempVer->SetIdentity();
			tempHor->SetIdentity();

			const Vector2& lookAt = instance[ i_lookAt ];

			// update and sample anims
			if ( m_useVerticalAnimations )
			{
				Float val = Clamp( lookAt.Y / m_animationAngleRange.Y, -1.0f, 1.0f);
				val = MapAdditiveValue( val );
				verticalPlaybacks.SampleAdditively( context, instance, *tempVer, timeDelta, val );
			}
			if ( m_useHorizontalAnimations )
			{
				Float val = Clamp( lookAt.X / m_animationAngleRange.X, -1.0f, 1.0f);
				val = MapAdditiveValue( val );
				horizontalPlaybacks.SampleAdditively( context, instance, *tempHor, timeDelta, val );
			}

			// and blend them together
			BlendVerticalAndHorizontalPoses( output, controlValue, m_useVerticalAnimations? tempVer : NULL, m_useHorizontalAnimations? tempHor : NULL );
		}
	}

	ANIM_NODE_POST_SAMPLE
}

const SLookAtAnimationPairInstance* CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::FindPairFor( CBehaviorGraphInstance& instance, const CSkeletalAnimationSetEntry* animation ) const
{
	const TDynArray< SLookAtAnimationPairInstance >& pairs = instance[ i_pairs ];
	for ( TDynArray< SLookAtAnimationPairInstance >::const_iterator pair = pairs.Begin(); pair != pairs.End(); ++ pair )
	{
		if ( pair->m_animation == animation )
		{
			return &(*pair);
		}
	}
	return NULL;
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::CollectUsedAnimations( TDynArray< CName >& anims ) const
{
	m_defaultPair.CollectUsedAnimations( anims );
	for ( TDynArray< SLookAtAnimationPairDefinition >::const_iterator pairDef = m_pairs.Begin(); pairDef != m_pairs.End(); ++ pairDef )
	{
		pairDef->CollectUsedAnimations( anims );
	}
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::const_iterator iPair = m_inputBasedPairs.Begin(); iPair != m_inputBasedPairs.End(); ++ iPair )
	{
		if ( iPair->m_cachedInputNode )
		{
			iPair->m_cachedInputNode->Activate( instance );
		}
	}
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );
	instance[ i_horizontalPlaybacks ].ClearAnims();
	instance[ i_verticalPlaybacks ].ClearAnims();

	for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::const_iterator iPair = m_inputBasedPairs.Begin(); iPair != m_inputBasedPairs.End(); ++ iPair )
	{
		if ( iPair->m_cachedInputNode )
		{
			iPair->m_cachedInputNode->Deactivate( instance );
		}
	}
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::const_iterator iPair = m_inputBasedPairs.Begin(); iPair != m_inputBasedPairs.End(); ++ iPair )
	{
		if ( iPair->m_cachedInputNode )
		{
			iPair->m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
		}
	}
}

void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	for ( TDynArray< SLookAtAnimationPairInputBasedDefinition >::iterator iPair = m_inputBasedPairs.Begin(); iPair != m_inputBasedPairs.End(); ++ iPair )
	{
		iPair->CacheConnections( this );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphLookAtUsingEmbeddedAnimationsNode::CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const
{
	TBaseClass::CollectAnimationUsageData( instance, collectorArray );

	const Float& controlValue = instance[ i_controlValue ];
	instance[ i_horizontalPlaybacks ].CollectAnimationUsageData( instance, collectorArray, controlValue );
	instance[ i_verticalPlaybacks ].CollectAnimationUsageData( instance, collectorArray, controlValue );
}
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
