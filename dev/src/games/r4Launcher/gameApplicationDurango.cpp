/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifdef RED_PLATFORM_DURANGO
#include "gameApplicationDurango.h"
#include "../../common/core/gameApplication.h"
#include "states.h"
#include "activateState.h"
#include "initializationState.h"
#include "gameRunningState.h"
#include "gameContrainedState.h"
#include "shutdownState.h"
#include "../../common/core/messagePump.h"

class CGameRunningDurango : public CGameRunningState
{
public:
	virtual Red::System::Bool OnTick( IGameApplication & application ) override;
};

class CGameConstrainedDurango : public CGameConstrainedState
{
#ifndef RED_FINAL_BUILD
public:
	virtual String GetName() { return TXT("Constrained"); };
#endif

public:
	virtual Red::System::Bool OnTick( IGameApplication & application ) override;
};

Red::System::Bool CGameRunningDurango::OnTick( IGameApplication & application )
{
	return CGameRunningState::OnTick( application );
}

Red::System::Bool CGameConstrainedDurango::OnTick( IGameApplication & application )
{
	return CGameConstrainedState::OnTick( application );
}

CGameApplication * CreateGameApplication( const wchar_t * commandLine )
{
	CGameApplication * gameApplication = new CGameApplicationDurango();

	gameApplication->InitializeStateCount( GameStateCount );

	gameApplication->RegisterState( GameActivate, new CActivateState( commandLine ) );
	gameApplication->RegisterState( GameInitialize, new CInitializationSate() );
	gameApplication->RegisterState( GameRunning, new CGameRunningDurango() );
	gameApplication->RegisterState( GameConstrained, new CGameConstrainedDurango() );
	gameApplication->RegisterState( GameShutdown, new CShutdownState() );
	
	return gameApplication;
}

void DestroyGameApplication( CGameApplication * gameApplication )
{
	delete gameApplication;
}

void CGameApplicationDurango::PumpEvents()
{
	if ( GMessagePump != nullptr )
	{
		GMessagePump->PumpMessages();
	}
}

#endif
