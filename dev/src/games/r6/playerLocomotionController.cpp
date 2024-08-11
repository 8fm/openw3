/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "playerLocomotionController.h"






CPlayerLocomotionController::CPlayerLocomotionController()
	: m_movementSegment( nullptr )
	, m_isAttached( false )
{
}










void CPlayerLocomotionController::OnSegmentFinished( EMoveStatus status )
{
	// clear the ptr, it's invalid at this point
	m_movementSegment = nullptr;
}











void CPlayerLocomotionController::OnControllerAttached()
{
	R6_ASSERT( !m_isAttached, TXT( "Attaching something that was allready attached?" ) );
	m_isAttached = true;

	if( m_movementSegment == NULL )
	{
		m_movementSegment = new CMoveLSDirect();

		if( CanUseLocomotion() )
		{
			locomotion().PushSegment( m_movementSegment );
		}
	}
}











void CPlayerLocomotionController::OnControllerDetached()
{
	R6_ASSERT( m_isAttached, TXT( "Detaching something that wasn't attached?" ) );
	m_isAttached = false;
}




