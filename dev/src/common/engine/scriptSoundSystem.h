/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

/* Wrapper class, only to use from scripts */
class CScriptSoundSystem : public CObject
{
	DECLARE_ENGINE_CLASS( CScriptSoundSystem, CObject, 0 )

public:
	CScriptSoundSystem();
	virtual ~CScriptSoundSystem();

private:
	void funcSoundSwitch( CScriptStackFrame& stack, void* result );
	void funcSoundState( CScriptStackFrame& stack, void* result );
	void funcSoundEvent( CScriptStackFrame& stack, void* result );
	void funcTimedSoundEvent( CScriptStackFrame& stack, void* result );
	void funcSoundParameter( CScriptStackFrame& stack, void* result );
	void funcSoundGlobalParameter( CScriptStackFrame& stack, void* result );
	void funcSoundSequence( CScriptStackFrame& stack, void* result );
	void funcSoundEventAddToSave( CScriptStackFrame& stack, void* result );
	void funcSoundEventClearSaved( CScriptStackFrame& stack, void* result );
	void funcEnableMusicEvents( CScriptStackFrame& stack, void* result );
	void funcSoundLoadBank( CScriptStackFrame& stack, void* result );
	void funcSoundUnloadBank( CScriptStackFrame& stack, void* result );
	void funcSoundIsBankLoaded( CScriptStackFrame& stack, void* result );
	void funcEnableMusicDebug( CScriptStackFrame& stack, void* result );
	void funcMuteSpeachUnderwater( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CScriptSoundSystem )
	PARENT_CLASS( CObject )
	NATIVE_FUNCTION( "SoundSwitch", funcSoundSwitch )
	NATIVE_FUNCTION( "SoundState", funcSoundState )
	NATIVE_FUNCTION( "SoundEvent", funcSoundEvent )
	NATIVE_FUNCTION( "TimedSoundEvent", funcTimedSoundEvent )
	NATIVE_FUNCTION( "SoundParameter", funcSoundParameter )
	NATIVE_FUNCTION( "SoundGlobalParameter", funcSoundGlobalParameter )
	NATIVE_FUNCTION( "SoundSequence", funcSoundSequence )
	NATIVE_FUNCTION( "SoundEventAddToSave", funcSoundEventAddToSave )
	NATIVE_FUNCTION( "SoundEventClearSaved", funcSoundEventClearSaved )
	NATIVE_FUNCTION( "SoundEnableMusicEvents", funcEnableMusicEvents )
	NATIVE_FUNCTION( "SoundLoadBank", funcSoundLoadBank )
	NATIVE_FUNCTION( "SoundUnloadBank", funcSoundUnloadBank )
	NATIVE_FUNCTION( "SoundIsBankLoaded", funcSoundIsBankLoaded )
	NATIVE_FUNCTION( "EnableMusicDebug", funcEnableMusicDebug )
	NATIVE_FUNCTION( "MuteSpeechUnderwater", funcEnableMusicDebug )
END_CLASS_RTTI()
