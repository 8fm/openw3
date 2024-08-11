/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeAttachToCurve.h"

#include "../engine/behaviorGraphStack.h"

#include "aiPositionPrediction.h"
#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAttachToCurveDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeFlyOnCurveDefinition )


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAttachToCurveInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeAttachToCurveInstance::PostUpdate()
{

}

void CBehTreeNodeAttachToCurveInstance::Update()
{
	CPropertyAnimationSet* animSet = m_animationSet.Get();
	CNode* node = m_node.Get();
	if ( !animSet || !node )
	{
		Complete( BTTO_FAILED );
		return;
	}
	
	Vector pos = node->GetWorldPositionRef();
	Float yaw = node->GetWorldYaw();

	if ( m_blendInTimeLeft > 0.f )
	{
		m_blendInTimeLeft -= m_owner->GetLocalTimeDelta();

		Float ratio = m_blendInTimeLeft / m_blendInTime;

		pos.AsVector3() += ( m_blendingPos - pos.AsVector3() ) * ratio;
		yaw = EulerAngles::Interpolate( yaw, m_blendingYaw, ratio );
	}

	CActor* actor = m_owner->GetActor();
	actor->SetPosition( pos );
	actor->Teleport( pos, EulerAngles( 0.f, 0.f, yaw ) );

	// test if animation is finished AFTER setting the position
	if ( !animSet->IsPlaying( m_animationName ) )
	{
		Complete( BTTO_SUCCESS );
		return;
	}

	PostUpdate();
}
Bool CBehTreeNodeAttachToCurveInstance::Activate()
{	
	CGameplayEntity* actionTarget = Cast< CGameplayEntity >( m_owner->GetActionTarget().Get() );
	if ( !actionTarget )
	{
		DebugNotifyActivationFail();
		return false;
	}
	CPropertyAnimationSet* animSet = actionTarget->GetPropertyAnimationSet();
	if ( !animSet )
	{
		DebugNotifyActivationFail();
		return false;
	}
	
	CComponent* component = nullptr;
	ComponentIterator< CWayPointComponent > it( actionTarget );
	while( it )
	{
		if ( m_componentName.Empty() || (*it)->GetName() == m_componentName )
		{
			component = *it;
		}
		++it;
	}
	if ( !component )
	{
		DebugNotifyActivationFail();
		return false;
	}
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		DebugNotifyActivationFail();
		return false;
	}
	if ( !animSet->Play( m_animationName, 1 ) )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_animationSet = animSet;
	m_node = component;
	m_blendInTimeLeft = m_blendInTime;
	m_blendingPos = mac->GetWorldPositionRef().AsVector3();
	m_blendingYaw = mac->GetWorldYaw();

	
	mac->ForceEntityRepresentation( true, CMovingAgentComponent::LS_AI );
	return Super::Activate();
}
void CBehTreeNodeAttachToCurveInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor ? actor->GetMovingAgentComponent() : nullptr;
	if ( mac )
	{
		mac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_AI );
	}
	
	Super::Deactivate();
}

Bool CBehTreeNodeAttachToCurveInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == SAIPositionPrediction::EventName() )
	{
		SAIPositionPrediction* eventData = e.m_gameplayEventData.Get< SAIPositionPrediction >();
		if ( !eventData )
		{
			return false;
		}
		CPropertyAnimationSet* animSet = m_animationSet.Get();
		if ( !animSet )
		{
			return false;
		}

		SPropertyAnimationInstance* instance = animSet->FindAnyAnimationInstance( m_animationName );
		if ( !instance )
		{
			return false;
		}

		CGameplayEntity* actionTarget = Cast< CGameplayEntity >( m_owner->GetActionTarget().Get() );
		if ( !actionTarget )
		{
			return false;
		}

		const auto& curve = instance->m_animation->m_curve;
		Float time = Min( instance->m_timer + eventData->m_inTime / instance->m_lengthScale, curve.GetTotalTime() );

		curve.GetRootPosition( time, eventData->m_outPosition );

		eventData->m_outPosition = actionTarget->GetLocalToWorld().TransformPoint( eventData->m_outPosition );

		eventData->m_outIsHandled = true;

		return false;
	}
	return Super::OnEvent( e );
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeFlyOnCurveInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeFlyOnCurveInstance::PostUpdate()
{
	const Float SAMPLE_TIME = 0.1f;
	const Float SAMPLE_MULT = 1.f / SAMPLE_TIME;

	CActor* actor = m_owner->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();

	CPropertyAnimationSet* animSet = m_animationSet.Get();												// tested in mother class. Behavior would fail if it won't be true.
	SPropertyAnimationInstance* instance = animSet->FindAnyAnimationInstance( m_animationName );		// tested in mother class. Behavior would complete itself if animation instance would deactivate.
	const auto& curve = instance->m_animation->m_curve;
	Float time0 = instance->m_timer;
	Float time1 = Min( instance->m_timer + SAMPLE_TIME / instance->m_lengthScale, curve.GetTotalTime() );

	EulerAngles currentRotation;
	EulerAngles futureRotation;

	curve.GetRotation( time0, currentRotation );
	curve.GetRotation( time1, futureRotation );

	Float yaw = EulerAngles::AngleDistance( currentRotation.Yaw, futureRotation.Yaw ) * SAMPLE_MULT;
	Float pitch = EulerAngles::NormalizeAngle180( currentRotation.Pitch );

	Float yawRatio = Clamp( yaw / m_maxYawInput, -1.f, 1.f );
	Float pitchRatio = Clamp( pitch / m_maxPitchInput, -1.f, 1.f );
		
	Float yawOutput = yawRatio * m_maxYawOutput;
	Float pitchOutput = pitchRatio * m_maxPitchOutput;

	CBehaviorGraphStack* stack = mac->GetBehaviorStack();
	stack->SetBehaviorVariable( m_animValYaw, yawOutput );
	stack->SetBehaviorVariable( m_animValPitch, pitchOutput );
}
