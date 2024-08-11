/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//////////////////////////////////////////////////////////////////////////
// CUserProfileManagerMock
//////////////////////////////////////////////////////////////////////////
class CUserProfileManagerMock : public CUserProfileManager
{
public:
	CUserProfileManagerMock() { }
	~CUserProfileManagerMock() { }
	//TODO check
	//virtual const Char*						GetPlatformUserIdAsString();

public:
	virtual Bool							Initialize() override final { return true; }
	virtual Bool							Shutdown() override final { return true; }

private:
	//TODO check
	virtual Bool							GetSafeArea( Float& x, Float& y ) override final { x = y = 1.f; return true; }

	//////////////////////////////////////////////////////////////////////////
	// Active user management
	//////////////////////////////////////////////////////////////////////////
public:
	virtual String							GetActiveUserDisplayNameRaw() const override final { return TXT( "" ); }
	virtual void							DisplayUserProfileSystemDialog() override final { }

	virtual void							SetActiveUserDefault() override final { }
	virtual void							SetActiveUserPromiscuous() override final { }
	virtual Bool							HasActiveUser() const override final { return true; }

	virtual void							DoUserPresence( const CName& ) override final {}

public:
	// loading
	virtual ELoadGameResult					InitGameLoading( const SSavegameInfo& info ) override { return ELoadGameResult::LOAD_Error; }
	virtual ELoadGameResult					GetLoadGameProgress() const override { return ELoadGameResult::LOAD_Error; }
	virtual void							CancelGameLoading() override { }
	virtual IFile*							CreateSaveFileReader(const Bool rawFile = false) override {  return nullptr; }
	virtual void							RequestScreenshotDataForReading( const SSavegameInfo& info ) override { }
	virtual Bool							IsScreenshotDataReadyForReading() override { return false; }
	virtual IFile*							CreateScreenshotDataReader() override { return nullptr; }
	virtual void							DoneReadingScreenshotData() override { }
	virtual void							FinalizeGameLoading() override { }

	// saving
	virtual ESaveGameResult					InitGameSaving( SSavegameInfo& info ) override { return SAVE_Error; }
	virtual ESaveGameResult					GetSaveGameProgress() const override { return SAVE_NotInitialized; }
	virtual void							CancelGameSaving() override { }
	virtual IFileEx*						CreateSaveFileWriter() override { return nullptr; }
	virtual void*							GetScreenshotBuffer() override { return nullptr; }
	virtual void							OnScreenshotDone( Uint32 realSize ) override { }
	virtual void							FinalizeGameSaving( IGameSaver* ) override { }

	// managing
	virtual void							GetSaveFiles( TDynArray< SSavegameInfo >& files ) const override { }
	virtual Bool							GetLatestSaveFile( SSavegameInfo& info ) const override { return false; }
	virtual void							DeleteSaveGame( const SSavegameInfo& info ) override { }
	virtual Bool							AreSavesInitialized() const { return false; }
	virtual Bool							GetSaveInSlot( ESaveGameType type, Int16 slot, SSavegameInfo& info ) const { return false; }
	virtual String							BuildFullDisplayNameForSave( const SSavegameInfo& info ) const { return TXT( "" ); }

	//////////////////////////////////////////////////////////////////////////
	// User settings
	//////////////////////////////////////////////////////////////////////////
public:
	virtual Bool							LoadUserSettings( StringAnsi& settingsString ) override { return false; }
	virtual Bool							SaveUserSettings( const StringAnsi& settingsString ) override { return false; }
};
