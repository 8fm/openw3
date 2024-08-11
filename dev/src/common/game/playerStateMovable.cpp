/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "playerStateMovable.h"

IMPLEMENT_ENGINE_CLASS( CPlayerStateMovable )

void CPlayerStateMovable::OnEnterState( const CName& previousState )
{
	// Find character component
	m_agent = GetPlayer()->GetMovingAgentComponent();
	ASSERT( m_agent.Get() );

	// Pass to base class
	CPlayerStateBase::OnEnterState( previousState );
}

void CPlayerStateMovable::OnLeaveState( const CName& newState )
{
	// Pass to base class
	CPlayerStateBase::OnLeaveState( newState );

	// Clear character
	m_agent = NULL;
}

Bool CPlayerStateMovable::IsPlayerMovingEnabled() const
{
	return true;
}

