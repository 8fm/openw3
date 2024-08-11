/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphAdjustDirectionNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphUtils.inl"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "skeletalAnimationEntry.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAdjustDirectionNode );

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( MovementDirectionWS );
RED_DEFINE_STATIC_NAME( AdjustDirection );
RED_DEFINE_STATIC_NAME( requestedMovementDirection );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphAdjustDirectionNode::CBehaviorGraphAdjustDirectionNode()
	: m_animDirectionChange( 0.0f )
	, m_updateAnimDirectionChangeFromAnimation( true )
	, m_maxDirectionDiff( 45.0f )
	, m_maxOppositeDirectionDiff( 70.0f )
	, m_basedOnEvent( CNAME( AdjustDirection ) )
	, m_adjustmentBlendSpeed( 30.0f )
	, m_requestedMovementDirectionVariableName( CNAME( requestedMovementDirection ) )
	, m_basedOnEventOverrideAnimation( true )
{
}

void CBehaviorGraphAdjustDirectionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_firstUpdate;
	compiler << i_startingDirectionWS;
	compiler << i_animDirectionChange;
	compiler << i_currentAdjustment;
	compiler << i_eventDuration;
	compiler << i_accYaw;
	compiler << i_accTime;
	// Variables
	compiler << i_hasRequestedMovementDirectionVariable;
}

void CBehaviorGraphAdjustDirectionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeDelta ] = 0.0f;
	instance[ i_firstUpdate ] = false;
	instance[ i_startingDirectionWS ] = 0.0f;
	instance[ i_animDirectionChange ] = 0.0f;
	instance[ i_currentAdjustment ] = 0.0f;
	instance[ i_eventDuration ] = 0.0f;
	instance[ i_accYaw ] = 0.f;
	instance[ i_accTime ] = 0.f;
	// Variables
	instance[ i_hasRequestedMovementDirectionVariable ] = instance.HasFloatValue( m_requestedMovementDirectionVariableName );
}

void CBehaviorGraphAdjustDirectionNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_firstUpdate );
	INST_PROP( i_startingDirectionWS );
	INST_PROP( i_animDirectionChange );
	INST_PROP( i_currentAdjustment );
	INST_PROP( i_eventDuration );
	INST_PROP( i_accYaw );
	INST_PROP( i_accTime );
	// Variables
	INST_PROP( i_hasRequestedMovementDirectionVariable );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphAdjustDirectionNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MovementDirectionWS ), false ) );
}

String CBehaviorGraphAdjustDirectionNode::GetCaption() const
{
	return String::Printf( TXT("Adjust direction %.2f'"), m_animDirectionChange );
}
#endif

void CBehaviorGraphAdjustDirectionNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( AdjustDirection );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphAdjustDirectionNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float& eventDuration = instance[ i_eventDuration ];
	Float& timeDelta = instance[ i_timeDelta ];
	Bool& firstUpdate = instance[ i_firstUpdate ];
	Float& startingDirectionWS = instance[ i_startingDirectionWS ];
	Float& animDirectionChange = instance[ i_animDirectionChange ];
	Float& currentAdjustment = instance[ i_currentAdjustment ];
	Float& accYaw = instance[ i_accYaw ];
	Float& accTime = instance[ i_accTime ];

	if ( timeDelta == 0.f )
	{
		return;
	}

	Bool wasActive = eventDuration != 0.0f;
	Bool isActive = false;
	CAnimationEventFired* firedEvent = output.m_eventsFired;
	for ( Int32 idx = output.m_numEventsFired; idx > 0; -- idx, ++ firedEvent )
	{
		if ( firedEvent->GetEventName() == m_basedOnEvent )
		{
			if ( eventDuration == 0.0f )
			{
				eventDuration = Abs( firedEvent->m_animInfo.m_eventEndsAtTime - firedEvent->m_animInfo.m_localTime );

				// Recache value because we need to wait for event and cached value can be obsolete
				startingDirectionWS = instance.GetAnimatedComponent()->GetWorldYaw();
			}
			isActive = eventDuration > 0.0f;
			break;
		}
	}

	const SBehaviorUsedAnimationData* usedAnim = nullptr;
	if ( m_basedOnEvent.Empty() )
	{
		if ( eventDuration == 0.0f )
		{
			usedAnim = output.m_usedAnims.m_anims.FindWithHighestWeight();
			if ( usedAnim && usedAnim->m_animation )
			{
				eventDuration = Max( 0.1f, usedAnim->m_animation->GetDuration() - ( usedAnim->m_currTime - usedAnim->m_playbackSpeed * timeDelta ) ); // use time delta here as animation was just ticked
			}
		}
		isActive = true;
	}

	if ( firstUpdate && isActive )
	{
		if ( m_updateAnimDirectionChangeFromAnimation )
		{
			// through all animations and calculate direction change as weighted sum of each animation changes
			animDirectionChange = 0.0f;
			const SBehaviorUsedAnimationData * anim = output.m_usedAnims.m_anims.GetUsedData();
			for ( Uint32 i = 0; i < output.m_usedAnims.m_anims.GetNum(); ++ i, ++ anim )
			{
				if ( anim && anim->m_animation )
				{
					if ( CSkeletalAnimation* animation = anim->m_animation->GetAnimation() )
					{
						if ( animation->HasExtractedMotion() )
						{
							// get rotation from current time (well, minus recent tick) up to end of event
							// break into two passes to have proper 180 or -180 read
							Float currTime = anim->m_currTime - anim->m_playbackSpeed * timeDelta;
							Float endTime = Min( animation->GetDuration(), currTime + eventDuration );
							Float midTime = ( currTime + endTime ) * 0.5f;
							AnimQsTransform movement0 = animation->GetMovementBetweenTime( currTime, midTime, 0 );
							AnimQsTransform movement1 = animation->GetMovementBetweenTime( midTime, endTime, 0 );
							Matrix movementMatrix0 = AnimQsTransformToMatrix( movement0 );
							Matrix movementMatrix1 = AnimQsTransformToMatrix( movement1 );
							animDirectionChange += ( movementMatrix0.ToEulerAngles().Yaw + movementMatrix1.ToEulerAngles().Yaw ) * anim->m_weight;
						}
					}
				}
			}
		}
		firstUpdate = false;
	}

	if ( isActive )
	{
		Float requestedMovementDirectionWS = GetMovementDirectionWS( instance );
		Float requestedAdjustment = EulerAngles::NormalizeAngle180( EulerAngles::AngleDistance( startingDirectionWS + animDirectionChange , requestedMovementDirectionWS ) );
		Float requestedAdjustmentOld = EulerAngles::NormalizeAngle180( requestedMovementDirectionWS - ( startingDirectionWS + animDirectionChange ) );

		Float rawRequestedAdjustment = requestedAdjustment;
		requestedAdjustment = Clamp( requestedAdjustment, -m_maxDirectionDiff, m_maxDirectionDiff );

		if ( Abs( rawRequestedAdjustment ) > 180.0f - m_maxOppositeDirectionDiff )
		{
			// adjust to match opposite direction
			requestedAdjustment = Clamp( EulerAngles::NormalizeAngle180( rawRequestedAdjustment + 180.0f ), -m_maxDirectionDiff, m_maxDirectionDiff );
		}

		if ( ! wasActive )
		{
			currentAdjustment = requestedAdjustment;
		}
		else
		{
			currentAdjustment = BlendToWithSpeed( currentAdjustment, requestedAdjustment, m_adjustmentBlendSpeed, timeDelta );
		}

		usedAnim = output.m_usedAnims.m_anims.FindWithHighestWeight();

		if ( animDirectionChange == 0.0f )
		{
			// adjust rotation
			const Float p = Clamp( timeDelta / eventDuration, 0.0f, 1.0f );
			const Float yawToAdd = currentAdjustment * p;

			if ( m_basedOnEventOverrideAnimation && !m_basedOnEvent.Empty() )
			{
				SetYawRotationToAnimQsTransform( output.m_deltaReferenceFrameLocal, yawToAdd );
			}
			else
			{
				AddYawRotationToAnimQsTransform( output.m_deltaReferenceFrameLocal, yawToAdd );
			}

			accYaw += yawToAdd;
		}
		else
		{
			// adjust rotation
			const Matrix movementMatrix = AnimQsTransformToMatrix( output.m_deltaReferenceFrameLocal );
			const Float p = movementMatrix.ToEulerAngles().Yaw / animDirectionChange;
			const Float yawToAdd = currentAdjustment * p;
			AddYawRotationToAnimQsTransform( output.m_deltaReferenceFrameLocal, yawToAdd );
			accYaw += yawToAdd;
		}

		accTime += timeDelta;
	}
	else
	{
		eventDuration = 0.0f;
		firstUpdate = true;
	}

	timeDelta = 0.f;
}

Float CBehaviorGraphAdjustDirectionNode::GetMovementDirectionWS( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		return m_cachedRequestedMovementDirectionWSValueNode->GetValue( instance );
	}
	else
	{
		if ( instance[ i_hasRequestedMovementDirectionVariable ] )
		{
			return instance.GetFloatValue( m_requestedMovementDirectionVariableName );
		}
		else
		{
			return 0.0f;
		}
	}
}

void CBehaviorGraphAdjustDirectionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->Activate( instance );
	}

	// store starting direction and anim direction change (as defined in node). just in case we wouldn't start our animation at the beginning
	instance[ i_startingDirectionWS ] = instance.GetAnimatedComponent()->GetWorldYaw();
	instance[ i_animDirectionChange ] = m_animDirectionChange;
	instance[ i_eventDuration ] = 0.0f;
	instance[ i_accYaw ] = 0.0f;
	instance[ i_accTime ] = 0.0f;
	instance[ i_firstUpdate ] = true;
}

void CBehaviorGraphAdjustDirectionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	Float& accYawFinal = instance[ i_accYaw ];
	accYawFinal = 0.f;

	instance[ i_accTime ] = 0.0f;

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphAdjustDirectionNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedRequestedMovementDirectionWSValueNode = CacheValueBlock( TXT("MovementDirectionWS") );
}

void CBehaviorGraphAdjustDirectionNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphAdjustDirectionNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	const Float accYaw = instance[ i_accYaw ];
	if ( MAbs( accYaw ) > 0.f )
	{
		const Float accTime = instance[ i_accTime ];
		const Float duration = instance[ i_eventDuration ];

		const Float startingDirectionWS = instance[ i_startingDirectionWS ];
		const Float requestedMovementDirectionWS = GetMovementDirectionWS( instance );
		const Float animDirectionChange = instance[ i_animDirectionChange ];

		const Float angleToAdd = EulerAngles::AngleDistance( requestedMovementDirectionWS, animDirectionChange + startingDirectionWS );

		const Float cosYawA = MCos( DEG2RAD( startingDirectionWS ) );
		const Float sinYawA = MSin( DEG2RAD( startingDirectionWS ) );
		const Vector directionA( -sinYawA, cosYawA, 0.0f );

		const Float cosYawB = MCos( DEG2RAD( accYaw + startingDirectionWS ) );
		const Float sinYawB = MSin( DEG2RAD( accYaw + startingDirectionWS ) );
		const Vector directionB( -sinYawB, cosYawB, 0.0f );

		const Float cosYawC = MCos( DEG2RAD( requestedMovementDirectionWS ) );
		const Float sinYawC = MSin( DEG2RAD( requestedMovementDirectionWS ) );
		const Vector directionC( -sinYawC, cosYawC, 0.0f );

		const CAnimatedComponent* ac = instance.GetAnimatedComponent();
		Matrix l2w( Matrix::IDENTITY );
		l2w.SetTranslation( ac->GetWorldPosition() );

		frame->AddDebugArrow( l2w, directionA, 1.2f, Color( 255, 150, 64 ), true, true );
		frame->AddDebugArrow( l2w, directionB, 1.2f, Color( 255, 200, 100 ), true, true );
		frame->AddDebugArrow( l2w, directionC, 1.2f, Color( 255, 255, 200 ), true, true );

		const String txt = String::Printf( TXT("a: %1.2f/%1.2f, t: %1.2f/%1.2f"), accYaw, angleToAdd, accTime, duration );
		frame->AddDebugText( l2w.GetTranslation(), txt, true, Color( 255, 255, 200 ) );
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
