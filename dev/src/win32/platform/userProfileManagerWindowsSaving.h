/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/os.h"
#include "../../common/engine/userProfile.h"

#include "../../common/engine/gameSaveManager.h"

class Interface;

class CUserProfileManagerWindowsSaving : public CUserProfileManager
{
private:
	String								m_savesDir;
	String								m_searchPattern;
	Bool								m_isInitialized		: 1;
	Bool								m_saveListUpdated_EventPending	: 1;

	SSavegameInfo						m_currentSaveLoadOp;
	SSavegameInfo						m_requestedScreenshot;
	void*								m_screenshotBuffer;
	Uint32								m_screenshotBufferSize;
	Uint32								m_lastTakenSceenshotSize;
	Red::Threads::CAtomic< Bool >		m_writerCreated;
	Red::Threads::CAtomic< Bool >		m_isSaving;

	static const Int32					SLOT_SANITY_CHECK = 10240;
	static const Int32					NUM_AUTOSAVE_SLOTS = 3;
	static const Int32					NUM_CHECKPOINT_SLOTS = 3;
	static const Int32					NUM_QUICKSAVE_SLOTS  = 1;

public:
	CUserProfileManagerWindowsSaving();
	~CUserProfileManagerWindowsSaving();

	virtual Bool Initialize() override;
	virtual void Update() override;
	virtual Bool Shutdown() override { return true; }

	// loading
	virtual ELoadGameResult InitGameLoading( const SSavegameInfo& info ) override;
	virtual ELoadGameResult GetLoadGameProgress() const override;
	virtual void CancelGameLoading() override;
	virtual IFile* CreateSaveFileReader(const Bool rawFile = false) override;
	virtual void RequestScreenshotDataForReading( const SSavegameInfo& info ) override;
	virtual Bool IsScreenshotDataReadyForReading() override;
	virtual IFile* CreateScreenshotDataReader() override;
	virtual void DoneReadingScreenshotData() override;
	virtual void FinalizeGameLoading() override;

	// saving
	virtual ESaveGameResult	InitGameSaving( SSavegameInfo& info ) override;
	virtual ESaveGameResult	GetSaveGameProgress() const override;
	virtual void CancelGameSaving() override;
	virtual IFileEx* CreateSaveFileWriter() override; 
	virtual void* GetScreenshotBuffer() override;
	virtual void OnScreenshotDone( Uint32 realSize ) override;
	virtual void FinalizeGameSaving( IGameSaver* saverToFinalize ) override;

	// managing
	virtual void GetSaveFiles( TDynArray< SSavegameInfo >& files ) const override;

	virtual Bool GetLatestSaveFile( SSavegameInfo& info ) const override;
	virtual void DeleteSaveGame( const SSavegameInfo& info ) override;
	virtual Bool AreSavesInitialized() const override { return true; }
	virtual String BuildFullDisplayNameForSave( const SSavegameInfo& info ) const;

#ifndef NO_SAVE_IMPORT
	//! Import facts from obsolete save
	virtual Bool ImportSave( const SSavegameInfo& info ) override;

	//! Gets list of obsolete saves
	virtual void Import_GetSaveFiles( TDynArray< SSavegameInfo >& files ) override;

	//! Gets list of obsolete saves
	virtual void Import_GetSearchPaths( TDynArray< String >& outArray ) override;
#endif // NO_SAVE_IMPORT

	//! Get the save directory
	RED_INLINE const String& GetSavesDir() const { return m_savesDir; }

	//! Checks if file exists
	Bool FileExist( const String& fileName );

	//! Returns last file modification
	Red::System::DateTime GetFileTime( const String& fileName, const String& extension ) const;

	//! Deletes the specified save file
	void DeleteFile( const String& gameSaveFilePath, const String& screenshotFilePath );

	SSavegameInfo GetSavegameInfo( const String& filename ) const;
	String BuildFullSaveFilePathForExistingFile( const SSavegameInfo& info );

private:
	static void GenerateSaveFileName( SSavegameInfo& info );
	static void ParseFilename( SSavegameInfo& info );

	static void AssignSlotIndices( TDynArray< SSavegameInfo >& files );

	//////////////////////////////////////////////////////////////////////////
	// User settings

public:
	virtual Bool							LoadUserSettings( StringAnsi& settingsString ) override; 
	virtual Bool							SaveUserSettings( const StringAnsi& settingsString ) override; 

private:
	Bool									TryLoadUserSettingsFromFile( const String& fileAbsolutePath, StringAnsi& settingsString );
};
