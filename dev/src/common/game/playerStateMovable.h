/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "playerStateBase.h"

/// Movable state
class CPlayerStateMovable : public CPlayerStateBase
{	
	DECLARE_RTTI_SIMPLE_CLASS( CPlayerStateMovable );

protected:
	THandle< CMovingAgentComponent >	m_agent;

public:
	virtual void OnEnterState( const CName& previousState );
	virtual void OnLeaveState( const CName& newState );

	virtual Bool IsPlayerMovingEnabled() const;
};

BEGIN_CLASS_RTTI( CPlayerStateMovable );
	PARENT_CLASS( CPlayerStateBase );
	PROPERTY( m_agent );
END_CLASS_RTTI();
