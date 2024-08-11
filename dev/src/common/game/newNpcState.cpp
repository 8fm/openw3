/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newNpcState.h"
#include "../game/aiHistory.h"

IMPLEMENT_ENGINE_CLASS( CNewNPCStateBase );

void CNewNPCStateBase::OnEnterState( const CName& previousState )
{
	AI_EVENT_START( GetNPC(), EAIE_State, GetStateName().AsString(), String::Printf( TXT("Processing state: %s"), GetStateName().AsString().AsChar() ) );

	// Pass to base class
	CScriptableState::OnEnterState( previousState );
}

//! Leave state
void CNewNPCStateBase::OnLeaveState( const CName& newState )
{
	// Pass to base class
	CScriptableState::OnLeaveState( newState );

	AI_EVENT_END( GetNPC(), EAIE_State, EAIR_Success );

}

void CNewNPCStateBase::OnUpdate( Float timeDelta, Bool prepareForDeactivation )
{
	// TODO: pass to script
}

void CNewNPCStateBase::funcMarkGoalFinished( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
}

RED_INLINE CNewNPC* CNewNPCStateBase::GetNPC() const
{
	return SafeCast< CNewNPC >( GetStateMachine() );
}

IMPLEMENT_ENGINE_CLASS( CNewNPCStateReactingBase );
