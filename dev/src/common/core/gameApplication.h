/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "coreInternal.h"
#include "sortedmap.h"

// IGameApplication
//	An interface to the game application state machine
class IGameApplication
{
public:
	virtual void SetReturnValue( Int32 value ) = 0;		// Set the value returned by Run()
	virtual void RequestState( Int32 stateId ) = 0;		// Request the state to use next
};

// IGameState
//	An interface for a single state of the game application at a platform level (init, running, shutdown, etc)
class IGameState
{

public:
	virtual const Char* GetName() const =0;

public:
	virtual ~IGameState()						{}
	virtual Bool OnEnterState()					{ return true; }	// True = start running this state, False = not ready yet
	virtual Bool OnTick( IGameApplication& )	{ return true; }	// Run a tick of this state. True = continue, False = shutdown
	virtual Bool OnExitState()					{ return true; }	// True = this state is finished, false = not ready yet
};

// CGameApplication
//	Represents the global game state and is the main entry point for each platform
class CGameApplication : public IGameApplication
{
public:
	CGameApplication();
	virtual ~CGameApplication();

	void RequestState( Int32 stateId );
	void SetReturnValue( Int32 value );

	void InitializeStateCount( Uint32 numStates );
	Bool RegisterState( Int32 stateId, IGameState* theState );
	Int32 Run();

protected:
	virtual void PumpEvents() {}

	void TickStates();

	TSortedMap< Int32, IGameState* > m_states;

	enum class StateContext
	{
		eContextEnterState,				// Keep calling enter state on the current state
		eContextTick,					// Keep calling tick on the current state
		eContextExitState,				// Keep calling exit state on the current state
		eContextNextState				// Finished with this state, go to the next one
	};

	IGameState* m_requestedState;		// Next state to use
	IGameState* m_currentState;			// The current state
	StateContext m_currentContext;		// What is the current state doing
	Int32 m_returnVal;					// Value returned to the platform on shutdown
};
