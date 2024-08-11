/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "playerStateBase.h"

IMPLEMENT_ENGINE_CLASS( CPlayerStateBase );

void CPlayerStateBase::OnEnterState( const CName& previousState )
{
	m_noSaveLock = CGameSessionManager::GAMESAVELOCK_INVALID;

	// Pass to base class
	CScriptableState::OnEnterState( previousState );
}

void CPlayerStateBase::OnLeaveState( const CName& newState )
{
	if( m_noSaveLock != CGameSessionManager::GAMESAVELOCK_INVALID )
	{
		SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_noSaveLock );
		m_noSaveLock = CGameSessionManager::GAMESAVELOCK_INVALID;
	}

	// Pass to base class
	CScriptableState::OnLeaveState( newState );
}

CPlayer* CPlayerStateBase::GetPlayer() const
{
	return SafeCast< CPlayer >( GetStateMachine() );
}

void CPlayerStateBase::funcCreateNoSaveLock( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	SGameSessionManager::GetInstance().CreateNoSaveLock( GetStateName().AsString(), m_noSaveLock, false, false );
}
