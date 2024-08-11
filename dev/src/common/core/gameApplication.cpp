/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameApplication.h"

//////////////////////////////////////////////////////////////////
// CTor
//
CGameApplication::CGameApplication()
	: m_requestedState( nullptr )
	, m_currentState( nullptr )
	, m_currentContext( StateContext::eContextEnterState )
	, m_returnVal( 0 )
{
}

//////////////////////////////////////////////////////////////////
// DTor
//
CGameApplication::~CGameApplication()
{
	m_states.Clear();
}

//////////////////////////////////////////////////////////////////
// InitializeStateCount
//	Reserves the appropriate amount of space before we start populating the state container
void CGameApplication::InitializeStateCount( Uint32 numStates )
{
	m_states.Reserve( numStates );
}

//////////////////////////////////////////////////////////////////
// RegisterState
//	Adds a state to the list, returns false if no space
Bool CGameApplication::RegisterState( Int32 stateId, IGameState* theState )
{
	return m_states.Insert( stateId, theState );
}

//////////////////////////////////////////////////////////////////
// SetReturnValue
//	
void CGameApplication::SetReturnValue( Int32 value )
{
	m_returnVal = value;
}

//////////////////////////////////////////////////////////////////
// RequestState
//	Pushes a state to the next state list. Calls onExistState until the current state is ready to finish
void CGameApplication::RequestState( Int32 stateID )
{
	IGameState* requestedState;
	m_states.Find( stateID, requestedState );

	if (requestedState == nullptr)
	{
		RED_LOG( PLM, TXT("Invalid state requested"));
		return;
	}

	if (requestedState == m_requestedState)
	{
		RED_LOG( PLM, TXT("Same state requested already"));
		return;
	}

	RED_ASSERT( m_requestedState == nullptr, TXT( "The %ls state is already requested while requesting %ls. You must wait for the current state to finish!" ), m_requestedState->GetName(), requestedState->GetName() );
	if( m_requestedState == nullptr )
	{
		m_requestedState = requestedState;
		RED_LOG( PLM, TXT("State request registered for %ls state"), m_requestedState->GetName() );
	}

	// We will immediately go into the next context. This is to ensure that we don't get stuck
	// (e.g. initialising but the OS demanded a suspend, etc)
	m_currentContext = StateContext::eContextExitState;
}

//////////////////////////////////////////////////////////////////
// Run
//	Runs the app!
Int32 CGameApplication::Run()
{
	do
	{
		PumpEvents();

		TickStates();
	}
	// No active states, assume the application will shut down now
	while( m_currentState );

	return m_returnVal;
}

void CGameApplication::TickStates()
{
	if( m_currentState )
	{
		switch( m_currentContext )
		{
		case StateContext::eContextEnterState:
			if( m_currentState->OnEnterState() )		// If Enter() has finished, start ticking
			{
				m_currentContext = StateContext::eContextTick;
			}
			break;

		case StateContext::eContextTick:
			if( !m_currentState->OnTick( *this ) )		// If Tick() failed, close down the state 
			{
				m_currentContext = StateContext::eContextExitState;
			}
			break;

		case StateContext::eContextExitState:
			if( m_currentState->OnExitState() )			// If Exit() finished, go to the next state
			{
				m_currentContext = StateContext::eContextNextState;
			}
			break;

		case StateContext::eContextNextState:			// Switch to the next state
			m_currentState = m_requestedState;
			m_currentContext = StateContext::eContextEnterState;
			m_requestedState = nullptr;
			break;
		}
	}
	else if( m_requestedState )
	{
		m_currentState = m_requestedState;
		m_currentContext = StateContext::eContextEnterState;
		m_requestedState = nullptr;
	}
}
