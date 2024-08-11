/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "shutdownState.h"

#include "../../common/redIO/redIO.h"
#include "../../common/platformCommon/platform.h"
#include "../../common/engine/baseEngine.h"
#include "../../common/engine/platformViewport.h"

Red::System::Bool CShutdownState::OnTick( IGameApplication& application )
{
	GEngine->Shutdown();
	application.SetReturnValue( GEngine->GetReturnValue() );

	CPlatform::Shutdown();

	delete GEngine->GetPlatformViewport();
	delete GEngine;
	GEngine = NULL;

	Red::IO::Shutdown();

	return false;			// Exit the state
}
