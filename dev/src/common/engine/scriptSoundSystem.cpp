/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "scriptSoundSystem.h"
#include "soundStartData.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptingSystem.h"
#include "soundSystem.h"

IMPLEMENT_ENGINE_CLASS( CScriptSoundSystem )

CScriptSoundSystem::CScriptSoundSystem()
{
	// Register in scripts
	ASSERT( GScriptingSystem != NULL );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_SOUND, this );
}

CScriptSoundSystem::~CScriptSoundSystem()
{
	// Unregister from scripts
	ASSERT( GScriptingSystem != NULL );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_SOUND, NULL );
}

void CScriptSoundSystem::funcSoundSwitch( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, switchGroupName, String() );
	GET_PARAMETER( String, switchName, String() );
	FINISH_PARAMETERS;

	if( switchName.Empty() ) switchName = TXT( "None" );
	GSoundSystem->SoundSwitch( UNICODE_TO_ANSI( switchName.AsChar() ), UNICODE_TO_ANSI( switchName.AsChar() ) );

}

void CScriptSoundSystem::funcSoundState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, stateGroupName, String() );
	GET_PARAMETER( String, stateName, String() );
	FINISH_PARAMETERS;

	if( stateName.Empty() ) stateName = TXT( "None" );

	GSoundSystem->SoundState( UNICODE_TO_ANSI( stateGroupName.AsChar() ), UNICODE_TO_ANSI( stateName.AsChar() ) );
}

void CScriptSoundSystem::funcSoundEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, eventName, String() );
	FINISH_PARAMETERS;

	GSoundSystem->SoundEvent( UNICODE_TO_ANSI( eventName.AsChar() ) );
}

void CScriptSoundSystem::funcTimedSoundEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, duration, 0.f);
	GET_PARAMETER_OPT( String, startEvent, String() );
	GET_PARAMETER_OPT( String, stopEvent, String() );
	GET_PARAMETER_OPT( Bool, updateTimeParameter, false );
	FINISH_PARAMETERS;

	GSoundSystem->TimedSoundEvent( startEvent, stopEvent, duration, updateTimeParameter );

}

void CScriptSoundSystem::funcSoundParameter( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, parameterName, String() );
	GET_PARAMETER( Float, value, 0.0f );
	GET_PARAMETER( Float, duration, 0.0f );
	FINISH_PARAMETERS;

	GSoundSystem->SoundParameter( UNICODE_TO_ANSI( parameterName.AsChar() ), value, duration );
}

void CScriptSoundSystem::funcSoundGlobalParameter( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, parameterName, String() );
	GET_PARAMETER( Float, value, 0.0f );
	GET_PARAMETER( Float, duration, 0.0f );
	FINISH_PARAMETERS;

	CSoundEmitterComponent::SoundGlobalParameter( UNICODE_TO_ANSI( parameterName.AsChar() ), value, duration );
}

void CScriptSoundSystem::funcSoundSequence( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, sequenceName, String() );
	GET_PARAMETER( TDynArray< String >, sequenceElements, TDynArray< String >() );
	FINISH_PARAMETERS;

	TDynArray< const wchar_t* > sequence;
	for( Uint32 i = 0; i != sequenceElements.Size(); ++i )
	{
		sequence.PushBack( sequenceElements[ i ].AsChar() );
	}
	GSoundSystem->PlaySoundSequence( sequenceName.AsChar(), &sequence[ 0 ], sequence.Size() );
}

void CScriptSoundSystem::funcSoundEventAddToSave( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, eventName, String() );
	FINISH_PARAMETERS;

	GSoundSystem->AddSoundEventToSave( UNICODE_TO_ANSI( eventName.AsChar() ) );
}

void CScriptSoundSystem::funcSoundEventClearSaved( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	GSoundSystem->ClearSavedSoundEvents();
}

void CScriptSoundSystem::funcEnableMusicEvents( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	GSoundSystem->GetMusicSystem( ).Enable( enable );

	RETURN_VOID( );
}


void CScriptSoundSystem::funcSoundLoadBank( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, bankName, String( ) );
	GET_PARAMETER( Bool, async, false );
	FINISH_PARAMETERS;

	CSoundBank* soundBank = CSoundBank::FindSoundBank( CName( bankName ) );
	if( soundBank )
	{
		if( soundBank->QueueLoading( ) )
		{
			if( !async )
			{
				while( !soundBank->IsLoadingFinished( ) ) { }
			}
		}
	}
}

void CScriptSoundSystem::funcSoundUnloadBank( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, bankName, String( ) );
	FINISH_PARAMETERS;

	CSoundBank* soundBank = CSoundBank::FindSoundBank( CName( bankName ) );
	if( soundBank )
	{
		soundBank->Unload( );
	}
}

void CScriptSoundSystem::funcSoundIsBankLoaded( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, bankName, String( ) );
	FINISH_PARAMETERS;

	CSoundBank* soundBank = CSoundBank::FindSoundBank( CName( bankName ) );
	if( soundBank )
	{
		RETURN_BOOL( soundBank->IsLoaded( ) );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CScriptSoundSystem::funcEnableMusicDebug(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Bool, enable, true );
	FINISH_PARAMETERS;

	GSoundSystem->GetMusicSystem().EnableDebug(enable);
}

void CScriptSoundSystem::funcMuteSpeachUnderwater(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Bool, enable, true );
	FINISH_PARAMETERS;

	GSoundSystem->MuteSpeechUnderWater(enable);
}
