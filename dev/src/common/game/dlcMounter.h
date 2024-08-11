#pragma once

#ifndef NO_EDITOR
#include "../core/analyzer.h"
#endif

//--------------------------------------------------

/// DLC Mounter for gameplay side, do whatever you want with the DLC data
class IGameplayDLCMounter : public CObject
{
	DECLARE_ENGINE_CLASS( IGameplayDLCMounter, CObject, 0 );

public:
	IGameplayDLCMounter();

	/// Game is about to be started/loaded with this content
	/// Do whatever you need to prepare the game for this
	/// This is called after the actual game was initialize
	virtual void OnGameStarting() = 0;

	/// Game is about to be after start/load
	/// Most game subsystems are loaded
	/// This is called after the actual game was start
	virtual void OnGameStarted(){};

	/// Game is about to be finished
	virtual void OnGameEnding() = 0;

	/// Check if the current state of the game is actually using content provided by this DLC
	/// If you return true, you will prevent user from loading such tainted savegame if he no longer has the content
	/// If you return false, it will mean that either the user is not using the content yet/already or you can handle it being gone gracefully
	/// by Default we always taint the savegame
	virtual bool OnCheckContentUsage() { return true; }

#ifndef NO_EDITOR

	virtual void OnEditorStarted() = 0;
	virtual void OnEditorStopped() = 0;

	/// Do analyze of content used by mounter
	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) = 0;

protected:
	static  Bool DoAnalyzeCSV( CAnalyzerOutputList& outputList, const String& csvFilePath );

#endif // !NO_EDITOR
};

BEGIN_ABSTRACT_CLASS_RTTI( IGameplayDLCMounter );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//--------------------------------------------------

/// Basic scriptable DLC mounter - good for script's only systems
class CGameplayDLCMounterScripted : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CGameplayDLCMounterScripted, IGameplayDLCMounter, 0 );

public:
	CGameplayDLCMounterScripted();

	// IGameplayDLCMounter interface
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;
	virtual bool OnCheckContentUsage() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR
};

BEGIN_CLASS_RTTI( CGameplayDLCMounterScripted );
	PARENT_CLASS( IGameplayDLCMounter );
END_CLASS_RTTI();

//--------------------------------------------------
