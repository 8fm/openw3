/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimLocationAdjustmentEvent.h"
#include "movingAgentComponent.h"
#include "movementAdjustor.h"
#include "..\engine\behaviorGraphStack.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimLocationAdjustmentEvent );

RED_DEFINE_STATIC_NAME( FinalStep );
RED_DEFINE_STATIC_NAME( MovementAdjustmentLocation );
RED_DEFINE_STATIC_NAME( MovementAdjustmentActive )

CExtAnimLocationAdjustmentEvent::CExtAnimLocationAdjustmentEvent()
	 : CExtAnimDurationEvent()
	 , m_locationAdjustmentVar( CNAME( MovementAdjustmentLocation ) )
	 , m_adjustmentActiveVar( CNAME( MovementAdjustmentActive ) )
{
	SetEventName( CNAME( FinalStep ) );
}

CExtAnimLocationAdjustmentEvent::CExtAnimLocationAdjustmentEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_locationAdjustmentVar( CNAME( MovementAdjustmentLocation ) )
	, m_adjustmentActiveVar( CNAME( MovementAdjustmentActive ) )
{
	if ( GetEventName().Empty() )
	{
		SetEventName( CNAME( FinalStep ) );
	}
}

CExtAnimLocationAdjustmentEvent::~CExtAnimLocationAdjustmentEvent()
{
}

void CExtAnimLocationAdjustmentEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( component ) )
	{
		CBehaviorGraphStack* stack = component->GetBehaviorStack();
		ASSERT( stack );
		if ( const Float* actVarPtr = stack->GetBehaviorFloatVariablePtr( m_adjustmentActiveVar ) )
		{
			if ( *actVarPtr > 0.0f )
			{
				if ( const Vector* varPtr = stack->GetBehaviorVectorVariablePtr( m_locationAdjustmentVar ) )
				{
					const CName eventName = info.GetEventName();
					CMovementAdjustor* ma = mac->GetMovementAdjustor();
					SMovementAdjustmentRequest* request = ma->CreateNewRequest( eventName );
					// as simple as that - bind to event and setup slide
					request->BindToEvent( eventName, true );
					request->SlideTo( *varPtr );
				}
			}
		}
	}
}
