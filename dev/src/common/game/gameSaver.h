/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../engine/gameTime.h"
#include "../engine/userProfile.h"
#include "../engine/gameSaveManager.h"
#include "../core/uniquePtr.h"

// Tool for making the game saves
class CGameSaver
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

private:
	EngineTime						m_lastAutoSaveAttemptTime;
	EngineTime						m_lastSaveTime;
	ESaveGameType					m_lastSaveType;
	ESaveGameType					m_requestedSaveType;
	Int16							m_requestedSaveSlot;
	void*							m_screenShotBuffer;
	size_t							m_realWriteSize;
	Red::TUniquePtr< IGameSaver >	m_saver;
	Red::Threads::CAtomic< Bool >	m_busy;
	Red::Threads::CAtomic< Bool >	m_screenshotDone;
	SSavegameInfo					m_currentSaveInfo;
	Bool							m_waitingForScreenshot;
	Bool							m_newGamePlusEnabled;
;
	
#ifndef FINAL
	String							m_debugSaveReason;
#endif

public:
	static const Uint32			SCREENSHOT_BUFFER_SIZE;
	static const Uint32			DEFAULT_SCREENSHOT_SIZE;
	static const Uint8*			DEFAULT_SCREENSHOT_DATA;
	static const Uint32			NGPLUS_SCREENSHOT_SIZE;
	static const Uint8*			NGPLUS_SCREENSHOT_DATA;

public:
	CGameSaver();
	~CGameSaver();

	//! Initializes game save and returns true if saving is possible
	void RequestSaveGame( ESaveGameType type, Int16 slot, const String& reason, const String& customSaveName = String::EMPTY );
	void SaveGameSync( ESaveGameType type, Int16 slot, const String& reason, const String& customSaveName = String::EMPTY );

	RED_INLINE ESaveGameType GetLastSaveType() const { return m_lastSaveType; }
	RED_INLINE ESaveGameType GetRequestedSaveType() const { return m_requestedSaveType; }
	RED_INLINE Int16 GetRequestedSaveSlot() const { return m_requestedSaveSlot; }
	RED_INLINE Bool IsScreenshotBufferReady() { return m_screenshotDone.GetValue(); }
	RED_INLINE Bool IsSaveInProgress() const { return m_busy.GetValue(); }
	RED_INLINE Bool IsNewGamePlusEnabled() const { return m_newGamePlusEnabled; }
	RED_INLINE void EnableNewGamePlus( Bool enable ) { m_newGamePlusEnabled = enable; }

	const Uint8* GetDefaultScreenshotData() const; 
	Uint32 GetDefaultScreenshotDataSize() const; 

	void Update();

	//! Load game from save file, no path should be given but full save name is expected: ex: q000_after_tent_000
	ESessionRestoreResult LoadGame( const SSavegameInfo& info );

	// Last save time
	Float ForHowLongWeHaventSaved() { return Float( EngineTime::GetNow() - m_lastSaveTime ); }
	Float ForHowLongWeHaventAttemptedToAutoSave() { return Float( EngineTime::GetNow() - m_lastAutoSaveAttemptTime ); }
	void OnWorldStart();
	void OnSaveCompleted( Bool success );

	static Bool ShouldShowTheCompatibilityWarning();

#ifndef NO_SAVE_VERBOSITY
	void DebugDumpLevelStats() const;
#endif

private:
	RED_INLINE void CancelSaving( CWorldStorage* ws, const Char* file );
	
	Bool DoSaveGame( CWorld* world, CWorldStorage* activeWorldStorage );

	//! Finalize save game as we were waiting for render thread to provide a thumbnail screenshot
	void FinalizeSaveGame( Bool sync );
	void UpdateUserProfileManager();

	//! request render thread to make screenshot
	void GrabScreenshot();
	void GrabScreenshotNow();
};

class CFinalizeSaveGameTask : public CTask
{
	Red::TUniquePtr< IGameSaver > m_saver;

public:
	CFinalizeSaveGameTask( Red::TUniquePtr<IGameSaver>&& saver );
	virtual ~CFinalizeSaveGameTask() override final;

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override final { return TXT("CFinalizeSaveGameTask"); }
	virtual Uint32 GetDebugColor() const override final { return COLOR_UINT32( 13, 133, 250 ); }
#endif

	virtual void Run() override final;
};
