/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimRotationAdjustmentLocationBasedEvent.h"
#include "movingAgentComponent.h"
#include "movementAdjustor.h"
#include "..\engine\behaviorGraphStack.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimRotationAdjustmentLocationBasedEvent );

RED_DEFINE_STATIC_NAME( MovementAdjustmentLocation );
RED_DEFINE_STATIC_NAME( MovementAdjustmentFacingTargetLocation );
RED_DEFINE_STATIC_NAME( MovementAdjustmentActive )

CExtAnimRotationAdjustmentLocationBasedEvent::CExtAnimRotationAdjustmentLocationBasedEvent()
	 : CExtAnimDurationEvent()
	 , m_locationAdjustmentVar( CNAME( MovementAdjustmentLocation ) )
	 , m_targetLocationVar( CNAME( MovementAdjustmentFacingTargetLocation ) )
	 , m_adjustmentActiveVar( CNAME( MovementAdjustmentActive ) )
{
}

CExtAnimRotationAdjustmentLocationBasedEvent::CExtAnimRotationAdjustmentLocationBasedEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_locationAdjustmentVar( CNAME( MovementAdjustmentLocation ) )
	, m_targetLocationVar( CNAME( MovementAdjustmentFacingTargetLocation ) )
	, m_adjustmentActiveVar( CNAME( MovementAdjustmentActive ) )
{
}

CExtAnimRotationAdjustmentLocationBasedEvent::~CExtAnimRotationAdjustmentLocationBasedEvent()
{
}

void CExtAnimRotationAdjustmentLocationBasedEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( component ) )
	{
		CBehaviorGraphStack* stack = component->GetBehaviorStack();
		ASSERT( stack );
		if ( const Float* actVarPtr = stack->GetBehaviorFloatVariablePtr( m_adjustmentActiveVar ) )
		{
			if ( *actVarPtr >= 0.0f )
			{
				const Vector* locVarPtr = stack->GetBehaviorVectorVariablePtr( m_locationAdjustmentVar );
				const Vector* tgtVarPtr = stack->GetBehaviorVectorVariablePtr( m_targetLocationVar );
				if ( locVarPtr && tgtVarPtr )
				{
					const CName eventName = info.GetEventName();
					CMovementAdjustor* ma = mac->GetMovementAdjustor();
					SMovementAdjustmentRequest* request = ma->CreateNewRequest( eventName );
					// as simple as that - bind to event and setup slide
					request->BindToEvent( eventName, true );
					Vector locToTgt = *tgtVarPtr - *locVarPtr;
					request->RotateTo( EulerAngles::YawFromXY( locToTgt.X, locToTgt.Y ) );
				}
			}
		}
	}
}
