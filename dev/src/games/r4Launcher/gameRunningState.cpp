/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameRunningState.h"
#include "states.h"

#include "../../common/engine/baseEngine.h"

Red::System::Bool CGameRunningState::OnEnterState()
{ 
	GEngine->OnEnterNormalRunningMode();
	return true; 
}

Red::System::Bool CGameRunningState::OnTick( IGameApplication & application )
{
	if( !GEngine->MainLoopSingleTick() )
	{
		application.RequestState( GameShutdown );
	}

	return true;
}
