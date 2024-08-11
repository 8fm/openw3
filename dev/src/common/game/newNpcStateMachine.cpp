/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

void CNewNPC::UpdateStateMachine( Float timeDelta )
{
	// Do not update states when not in game
	if ( !GGame->IsActive() )
	{
		return;
	}

	// Update current state
	CNewNPCStateBase* state = GetCurrentState();
	if ( state )
	{
		const Bool isExisting = false;
		state->OnUpdate( timeDelta, isExisting );
	}
}

void CNewNPC::EnterForcedDespawn()
{
	Destroy();	
}

void CNewNPC::QuestLock( const CQuestPhase* phase, const CQuestGraphBlock* block )
{
	ASSERT( phase );
	ASSERT( block );

	// Add info and unlock
	m_questLocksHistory.PushBack( new NPCQuestLock( true, phase, block ) );
	m_questLockCount++;
}

void CNewNPC::QuestUnlock( const CQuestPhase* phase, const CQuestGraphBlock* block )
{
	ASSERT( phase );
	ASSERT( block );

	// Invalid unlock
	if ( m_questLockCount == 0 )
	{
		ERR_GAME( TXT("Trying to quest unlock NPC '%ls' that was not locked by quests"), GetFriendlyName().AsChar() );
		return;
	}
	
	// Add info and unlock
	m_questLocksHistory.PushBack( new NPCQuestLock( false, phase, block ) );
	m_questLockCount--;
}
