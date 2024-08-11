/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameContrainedState.h"
#include "states.h"

#include "../../common/engine/baseEngine.h"

Red::System::Bool CGameConstrainedState::OnEnterState()
{
	GEngine->OnEnterConstrainedRunningMode();
	return true;
}

Red::System::Bool CGameConstrainedState::OnTick( IGameApplication & application )
{
	if( !GEngine->MainLoopSingleTick() )
	{
		application.RequestState( GameShutdown );
	}

	return true;
}

Red::System::Bool CGameConstrainedState::OnExitState()
{
	GEngine->OnExitConstrainedRunningMode();
	return true;
}
