#include "build.h"
#include "customCamera.h"
#include "r4DLCFinishersMounter.h"
#include "../../common/core/depot.h"
#include "../../common/core/scriptingSystem.h"

IMPLEMENT_ENGINE_CLASS( CR4FinishersDLCMounter );
IMPLEMENT_ENGINE_CLASS( CR4FinisherDLC );
IMPLEMENT_RTTI_ENUM( EFinisherSide );

RED_DEFINE_STATIC_NAME( LoadFinisher );
RED_DEFINE_STATIC_NAME( UnloadFinisher );

CR4FinisherDLC::CR4FinisherDLC()
{

}

void CR4FinisherDLC::funcIsFinisherForAnim( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationEventAnimInfo, eventAnimInfo, SAnimationEventAnimInfo() );
	FINISH_PARAMETERS;

	if( !eventAnimInfo.m_animation )
	{
		RETURN_BOOL( false );
		return;
	}

	if( eventAnimInfo.m_animation->GetName() == m_finisherAnimName )
	{
		RETURN_BOOL( true );
		return;
	}

	RETURN_BOOL( false );
}

CR4FinishersDLCMounter::CR4FinishersDLCMounter() :
	m_customCameraAnimSetLoaded( false )
{
}

bool CR4FinishersDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4FinishersDLCMounter::OnGameStarting()
{	
	ActivatePhase1();
}

void CR4FinishersDLCMounter::OnGameStarted()
{
	ActivatePhase2();
}

void CR4FinishersDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4FinishersDLCMounter::OnEditorStarted()
{
	ActivatePhase1();
	ActivatePhase2();
}

void CR4FinishersDLCMounter::OnEditorStopped()
{
	Deactivate();
}

Bool CR4FinishersDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	return true;
}

#endif // !NO_EDITOR

void CR4FinishersDLCMounter::ActivatePhase1()
{
	for( CR4FinisherDLC* finisher : m_finishers )
	{
		THandle< CR4FinisherDLC > hFinisher( finisher );
		CallFunction( this, CNAME( LoadFinisher ), hFinisher );
	}
}

void CR4FinishersDLCMounter::ActivatePhase2()
{
	if ( m_customCameraAnimSet )
	{
		CCustomCamera* customCamera = GR4Game->GetCamera();
		if( customCamera )
		{
			m_customCameraAnimSetLoaded = customCamera->AddDLCAnimset( m_customCameraAnimSet );
		}		
	}
}

void CR4FinishersDLCMounter::Deactivate()
{
	IScriptable* gpGame = GScriptingSystem->GetGlobal( CScriptingSystem::GP_GAME );
	if( gpGame != nullptr ) //! script game object can be unloaded on this stage 
	{
		for( CR4FinisherDLC* finisher : m_finishers )
		{
			THandle< CR4FinisherDLC > hFinisher( finisher );
			CallFunction( this, CNAME( UnloadFinisher ), hFinisher );
		}
	}

	if ( m_customCameraAnimSet )
	{
		if( GR4Game->GetActiveWorld() != nullptr ) //! if active world is null camera is invalid
		{
			CCustomCamera* customCamera = GR4Game->GetCamera();
			if( customCamera )
			{
				customCamera->RemoveDLCAnimset( m_customCameraAnimSet );
				m_customCameraAnimSetLoaded = false;
			}
		}		
	}
}
