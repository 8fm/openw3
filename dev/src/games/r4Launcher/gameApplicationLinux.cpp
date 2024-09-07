/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/gameApplication.h"
#include "../../common/core/messagePump.h"
#include "../../common/core/contentManager.h"

#include "../../common/engine/baseEngine.h"
#include "../../common/engine/game.h"
#include "../../common/engine/userProfile.h"
#include "../../common/engine/rawInputManager.h"
#include "../../common/engine/inputDeviceManager.h"

#include "states.h"
#include "initializationState.h"
#include "gameRunningState.h"
#include "shutdownState.h"
#include "activateState.h"

class CMessagePumpLinux : public IMessagePump
{
public:
	CMessagePumpLinux() {}
	virtual ~CMessagePumpLinux() {}

	virtual void PumpMessages() override final;

private:
	Red::System::StopClock m_timer;
};

void CMessagePumpLinux::PumpMessages()
{
	if( GEngine && GGame && GGame->CERT_HACK_IsInStartGame() )
	{
		// Update the user profile manager to ensure we're ticking the digital distribution dlls during loading
		if( GUserProfileManager )
			GUserProfileManager->Update();

		// Update the input devices so that things that hook into xinput/dinput can still update during loading (i.e. steam/galaxy overlay)
		IInputDeviceManager* inputMan = GEngine->GetInputDeviceManager();
		Bool isStartingGame = !( GContentManager && GGame->IsLoadingScreenVideoPlaying() && GContentManager->GetStallForMoreContent() == eContentStall_None );
		if( inputMan && isStartingGame )
		{
			TDynArray< SBufferedInputEvent > ignoredInput;
			inputMan->Update( ignoredInput );
		}
	}
}

class CGameApplicationLinux : public CGameApplication
{
public:
	CGameApplicationLinux()
	{
		RED_FATAL_ASSERT( GMessagePump == nullptr, "Message pump already initialised" );
		GMessagePump = new CMessagePumpLinux();
	}

	virtual ~CGameApplicationLinux() override final {}

	virtual void PumpEvents() override final
	{
		if ( GMessagePump != nullptr )
		{
			GMessagePump->PumpMessages();
		}
	}
};

namespace ScriptCompilationHelpers
{
	typedef ECompileScriptsReturnValue (*SCRIPT_FAILURE_DLG_FUNC)( const class CScriptCompilationMessages& );
	extern SCRIPT_FAILURE_DLG_FUNC ScriptFailureDlgFunc;
	extern ECompileScriptsReturnValue LauncherScriptCompilationErrorMessage( const class CScriptCompilationMessages& );

	typedef void (*SCRIPT_SHOWHIDE_SPLASH_FUNC)(Bool show);
	extern SCRIPT_SHOWHIDE_SPLASH_FUNC ScriptSplashFunc;
	extern void ShowHideSplash(Bool);
}

int mainLinux( const Char* commandLine )
{
	ScriptCompilationHelpers::ScriptFailureDlgFunc = &ScriptCompilationHelpers::LauncherScriptCompilationErrorMessage;
	ScriptCompilationHelpers::ScriptSplashFunc = &ScriptCompilationHelpers::ShowHideSplash;

	CGameApplicationLinux theGame;

	CActivateState activateState( commandLine );
	theGame.RegisterState( GameActivate, &activateState );

	CInitializationSate initState;
	theGame.RegisterState( GameInitialize, &initState );

	CGameRunningState gameRunningState;
	theGame.RegisterState( GameRunning, &gameRunningState );

	CShutdownState shutodwnState;
	theGame.RegisterState( GameShutdown, &shutodwnState );

	theGame.RequestState( GameActivate );

	return theGame.Run();
}
