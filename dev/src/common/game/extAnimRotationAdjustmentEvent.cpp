/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimRotationAdjustmentEvent.h"
#include "movingAgentComponent.h"
#include "movementAdjustor.h"
#include "..\engine\behaviorGraphStack.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimRotationAdjustmentEvent );

RED_DEFINE_STATIC_NAME( MovementAdjustmentFacingDirection );

CExtAnimRotationAdjustmentEvent::CExtAnimRotationAdjustmentEvent()
	 : CExtAnimDurationEvent()
	 , m_rotationAdjustmentVar( CNAME( MovementAdjustmentFacingDirection ) )
{
}

CExtAnimRotationAdjustmentEvent::CExtAnimRotationAdjustmentEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_rotationAdjustmentVar( CNAME( MovementAdjustmentFacingDirection ) )
{
}

CExtAnimRotationAdjustmentEvent::~CExtAnimRotationAdjustmentEvent()
{
}

void CExtAnimRotationAdjustmentEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( component ) )
	{
		CBehaviorGraphStack* stack = component->GetBehaviorStack();
		ASSERT( stack );
		if ( const Float* value = stack->GetBehaviorFloatVariablePtr( m_rotationAdjustmentVar ) )
		{
			const CName eventName = info.GetEventName();
			CMovementAdjustor* ma = mac->GetMovementAdjustor();
			SMovementAdjustmentRequest* request = ma->CreateNewRequest( eventName );
			// as simple as that - bind to event and setup rotation
			request->BindToEvent( eventName, true );
			request->RotateTo( *value );
		}
	}
}
