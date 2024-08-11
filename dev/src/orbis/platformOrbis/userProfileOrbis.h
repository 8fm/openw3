/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include <np/np_trophy.h>
#include <save_data.h>
#include <save_data_dialog.h>

class IOrbisSystemDialog;

//////////////////////////////////////////////////////////////////////////
// CUserProfileManagerOrbis
//////////////////////////////////////////////////////////////////////////
class CUserProfileManagerOrbis : public CUserProfileManager
{
private:
	class CProfileTask : public CTask
	{
	protected:
		CUserProfileManagerOrbis*	m_profile;
		CProfileTask( CUserProfileManagerOrbis* profile ) : m_profile( profile ) {}

		#ifndef NO_DEBUG_PAGES
			virtual const Char*		GetDebugName() const { return TXT("ORBIS profile task"); }
			virtual Uint32			GetDebugColor() const { return 0x84376599; } // whatever
		#endif
	};

	class CMountTask : public CProfileTask
	{		
		SceSaveDataDirName			m_dirName;
		SceSaveDataMount			m_mount;

		Bool						m_isUserSettings	: 1;
		Bool						m_canFail			: 1;
		Bool						m_result			: 1;

	public:
		CMountTask(	CUserProfileManagerOrbis* profile ) : CProfileTask( profile ) 
		{
			Red::MemoryZero( &m_dirName, sizeof( m_dirName ) );
			Red::MemoryZero( &m_mount, sizeof( m_mount ) );
			m_mount.userId = profile->m_userId;
			m_isUserSettings = false;
			m_canFail = false;
			m_result = false;
		}

		void Run()
		{
			m_result = m_profile->MountSaveData( &m_mount, m_isUserSettings ? &m_profile->m_userSettingsMountResult : &m_profile->m_saveDataMountResult, m_canFail, m_isUserSettings );
		}

		RED_INLINE void SetDirName( const Char* dirName )
		{
			Red::StringCopy( m_dirName.data, UNICODE_TO_ANSI( dirName ), SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE );
			m_mount.dirName = &m_dirName;
		}

		RED_INLINE void SetBlocks( SceSaveDataBlocks blocks ) { m_mount.blocks = blocks; }
		RED_INLINE void SetMountMode( SceSaveDataMountMode mountMode ) { m_mount.mountMode = mountMode; }	
		RED_INLINE void SetIsUserSettings( Bool isUserSettings ) { m_isUserSettings = isUserSettings; }
		RED_INLINE void SetCanFail( Bool canFail ) { m_canFail = canFail; }
		RED_INLINE Bool CanFail() const { return m_canFail; }
		RED_INLINE Bool GetResult() const { return m_result; }
		RED_INLINE Bool IsUserSettings() const { return m_isUserSettings; }
		RED_INLINE SceSaveDataMountMode GetMountMode() const { return m_mount.mountMode; }
	};

	class CUnMountTask : public CProfileTask
	{
		Bool m_isUserSettings;
	public:
		CUnMountTask(	CUserProfileManagerOrbis* profile ) : CProfileTask( profile ), m_isUserSettings( false ) {}

		void Run()
		{
			m_profile->UnmountSaveData( m_isUserSettings ? &m_profile->m_userSettingsMountResult : &m_profile->m_saveDataMountResult, m_isUserSettings );
		}

		RED_INLINE void SetIsUserSettings( Bool isUserSettings ) { m_isUserSettings = isUserSettings; }
	};

	class CUpdateSaveInfosTask : public CProfileTask
	{
	public:
		CUpdateSaveInfosTask( CUserProfileManagerOrbis* profile ) : CProfileTask( profile ) {}
		void Run() { m_profile->UpdateSaveInfos(); }
	};

	class CSaveSettingsTask : public CProfileTask
	{
	public:
		CSaveSettingsTask( CUserProfileManagerOrbis* profile ) : CProfileTask( profile ) {}
		void Run() { m_profile->DoSaveSettings(); }
	};

	class CScreenshotReadTask : public CProfileTask
	{
		SSavegameInfo m_info;
	public:
		CScreenshotReadTask( CUserProfileManagerOrbis* profile ) : CProfileTask( profile ) {}
		void Run() { m_profile->DoSyncReadScreenshotData( m_info ); }
		RED_INLINE void SetSaveInfo( const SSavegameInfo& info ) { m_info = info; }
	};

	class CDeleteSaveDataTask : public CProfileTask
	{		
		SceSaveDataDirName			m_dirName;

	public:
		CDeleteSaveDataTask( CUserProfileManagerOrbis* profile ) : CProfileTask( profile ) 
		{
			Red::MemoryZero( &m_dirName, sizeof( m_dirName ) );
		}

		void Run()
		{
			m_profile->DoDeleteSaveData( &m_dirName );
		}

		RED_INLINE void SetDirName( const Char* dirName )
		{
			Red::StringCopy( m_dirName.data, UNICODE_TO_ANSI( dirName ), SCE_SAVE_DATA_DIRNAME_DATA_MAXSIZE );
		}
	};

public:
	CUserProfileManagerOrbis();
	virtual ~CUserProfileManagerOrbis() override final;

	virtual Bool Initialize() override final;
	virtual void Update() override final;
	virtual Bool Shutdown() override final;

private:
	Bool InitializeDebugTitleId();

	Bool CheckPermissions();

	Bool InitializeTrophySystem();
	Bool ShutdownTrophySystem();

	Bool InitializeGamePresence();
	Bool ShutdownGamePresence();

	virtual void UnlockAchievement( const CName& name ) override final;
	virtual void LockAchievement( const CName& name ) override final;

	virtual void GetAllAchievements( TDynArray< CName >& achievements ) const override final;

	virtual Bool IsAchievementMapped( const CName& name ) const override final;
	virtual void MapAchievementInit( Uint32 numAchievements ) override final;
	virtual void MapAchievement( const CName& name, const String& platform, const String& id ) override final;

	virtual void DoUserPresence( const CName& presenceName ) override final;

	virtual void MapPresenceInit( Uint32 numEntries ) override final;
	virtual void MapPresence( const CName& name, const String& platform, const String& id ) override final;

	virtual void LoadContentRestrictionXML( CXMLReader* xml ) override final;

	//////////////////////////////////////////////////////////////////////////
	// Active user management
	//////////////////////////////////////////////////////////////////////////

	virtual String GetActiveUserDisplayNameRaw() const override final;

	virtual void DisplayUserProfileSystemDialog() override final;
	Bool DisplayUserProfileSystemDialogInitialize();
	Bool DisplayUserProfileSystemDialogShutdown();

	// Receive input only from the system specified initial user
	virtual void SetActiveUserDefault() override final;

	// Receive input from all controllers
	virtual void SetActiveUserPromiscuous() override final;

	void OnActiveUserAcquired( SceUserServiceUserId userId );
	void OnActiveUserLost();

	virtual Bool HasActiveUser() const override final;

	virtual void DisplayHelp() override final;
	virtual void DisplayStore() override final;

	template< typename TSystemDialog, typename TParam >
	Bool DisplayDialog( const TParam& param );

	virtual void ToggleInputProcessing( Bool enabled, EInputProcessingDisabledReason reason ) override final;
	virtual Bool GetSafeArea( Float& x, Float& y ) override final;

private:
	SceUserServiceUserId m_userId;

	SceNpTrophyContext m_trophyContext;
	TSortedMap< CName, SceNpTrophyId > m_nameToTrophyIdMap;
	TSortedMap< CName, Uint32 > m_nameToPresenceIdMap;

	//Game Presence

	enum EPresenceInitialization
	{
		PI_Uninitialized = 0,
		PI_NetPool,
		PI_SSL,
		PI_HTTP,
		PI_WebApi,
	};

	Int32									m_netPoolId;
	Int32									m_sslContextId;
	Int32									m_httpContextId;
	Int32									m_webApiContextId;
	Bool									m_initialized;

	Red::Threads::CAtomic< Bool >			m_hasOnlinePermission;
	EPresenceInitialization					m_presenceInitializationStage;

	IOrbisSystemDialog* 					m_systemDialog;

	typedef class CCompressedFileWriter		TSaveWriter;

	static const Uint32						MAX_SAVE_SIZE = 16 * 1024 * 1024;
	static const Int16						NUM_AUTOSAVE_SLOTS = 2;
	static const Int16						NUM_CHECKPOINT_SLOTS = 3;
	static const Int16						NUM_QUICKMANUAL_SLOTS = 8;
	static const Int16						NUM_USERCONFIG_SLOTS = 1;
	static const Int32						SLOT_SANITY_CHECK = 256;
	static const Uint32						NUM_BLOCKS_PER_SAVE_SLOT = 512;

	CMountTask*								m_mountTask;
	CUnMountTask*							m_unmountTask;
	CUpdateSaveInfosTask*					m_updateSaveInfosTask;
	CSaveSettingsTask*						m_saveSettingsTask;
	CScreenshotReadTask*					m_screenshotReadTask;
	CDeleteSaveDataTask*					m_deleteSaveDataTask;
	mutable Red::Threads::CMutex			m_saveInfosMutex;

	SceSaveDataMountResult					m_saveDataMountResult;
	SceSaveDataMountResult					m_userSettingsMountResult;
	TDynArray< SSavegameInfo >				m_saveInfos;
	Int32									m_initializedLoadSaveInfoIndex;
	Red::Threads::CAtomic< Int32 >			m_saveInfosUpdateState;
	Int32									m_saveLock;
	StringAnsi								m_requiredContentString;
	StringAnsi								m_settingsToSave;
	TQueue< SSavegameInfo >					m_deleteQueue;
	TDynArray< Uint8, MC_Gameplay, MemoryPool_Default >	m_saveBuffer;

	ELoadGameResult							m_currentLoadOpProgress;
	ESaveGameResult							m_currentSaveOpProgress;

	Bool									m_saveSystemInitialized						: 1;
	Bool									m_displayingOutOfDiskSpaceError				: 1;
	Bool									m_displayingBrokenSaveDataError				: 1;
	Bool									m_displayingSaveDataErrorCode				: 1;
	Bool									m_displayingCorruptedSaveOverwriteMessage	: 1;
	Bool									m_userSettingsExists						: 1;
	Bool									m_shouldMountSettingsForReading				: 1;
	Bool									m_loadSaveReadyEventSent					: 1;
	Bool									m_lockedUserActions							: 1;

	enum EScreenshotReadRequestStatus
	{
		SCR_NotRequested = 0,
		SCR_Requested,
		SCR_Ready,
		SCR_Failed,
	};

	DataBuffer								m_screenshotBuffer;
	Uint32									m_realScreenshotSize;
	Red::Threads::CAtomic< Int32 >			m_screenshotReadingRequestStatus;
	Red::Threads::CAtomic< Int32 >			m_screenshotReadingRequestInfoIndex;
	Red::Threads::CAtomic< Bool >			m_ignoreSaveUserSettingsRequest;

	#ifdef SAVE_SERVER_ENABLED
		CSaveServer							m_debugSaveServer;
	#endif // SAVE_SERVER_ENABLED

	//////////////////////////////////////////////////////////////////////////
	// Savegame system

public:
	Bool									RequiresPriorityIO() const;

public:
	// loading
	virtual ELoadGameResult					InitGameLoading( const SSavegameInfo& info ) override;
	virtual ELoadGameResult					GetLoadGameProgress() const override;
	virtual void							CancelGameLoading() override;
	virtual IFile*							CreateSaveFileReader(const Bool rawFile = false) override;
	virtual void							RequestScreenshotDataForReading( const SSavegameInfo& info ) override;

	virtual Bool							IsScreenshotDataReadyForReading() override;
	virtual IFile*							CreateScreenshotDataReader() override;
	virtual void							DoneReadingScreenshotData() override;
	virtual void							FinalizeGameLoading() override;
	virtual TDynArray< CName >				GetContentRequiredByLastSave() const override;

	// saving
	virtual ESaveGameResult					InitGameSaving( SSavegameInfo& info ) override;
	virtual ESaveGameResult					GetSaveGameProgress() const override;
	virtual void							CancelGameSaving() override;
	virtual IFileEx*						CreateSaveFileWriter() override; 
	virtual void*							GetScreenshotBuffer() override;
	virtual void							OnScreenshotDone( Uint32 realSize ) override;
	virtual void							FinalizeGameSaving( IGameSaver* saverToFinalize ) override;

	// managing
	virtual void							GetSaveFiles( TDynArray< SSavegameInfo >& files ) const override;
	virtual Bool							GetLatestSaveFile( SSavegameInfo& info ) const override;
	virtual void							DeleteSaveGame( const SSavegameInfo& info ) override;
	virtual Bool							AreSavesInitialized() const override;
	virtual Int16							GetNumSaveSlots( ESaveGameType type ) const;
	virtual Bool							GetSaveInSlot( ESaveGameType type, Int16 slot, SSavegameInfo& info ) const;
	virtual String							BuildFullDisplayNameForSave( const SSavegameInfo& info ) const override;
	virtual Bool							IsDisplayingSystemMessage() const override;
	virtual Bool							HaveLockedUserActions() const override;

private:
	Bool									InitializeSaveSystem();
	Bool									ShutdownSaveSystem();

	void									UpdateSaveSystem();
	void									UpdateUserActionsLock( Bool lock );

	void									UpdateGameLoadingInit();
	void									UpdateGameSavingInit();
	void									UpdateSaveInfos();
	Int32									FindSaveInfoIndex( ESaveGameType type, Int16 slotIndex ) const;
	static void								GenerateSaveFileName( SSavegameInfo& info );
	static void								ParseFilename( SSavegameInfo& info );

	virtual void							BuildFullDisplayNameForSave( AnsiChar* out, Uint32 size, const SSavegameInfo& info ) const;

	Bool									MountSaveData( SceSaveDataMount* mount, SceSaveDataMountResult* result, Bool canFail, Bool isUserSettings );
	void									UnmountSaveData( SceSaveDataMountResult* result, Bool isUserSettings );

	void									CreateMountTask();
	void									CreateUnMountTask();
	void									CreateUpdateSaveInfosTask();
	void									CreateSaveSettingsTask();
	void									CreateScreenshotReadTask();
	void									CreateDeleteSaveDataTask();

	void									RunMountTask();
	void									RunUnMountTask();
	void									RunUpdateSaveInfosTask();
	void									RunSaveSettingsTask();
	void									RunScreenshotReadTask();
	void									RunDeleteSaveDataTask();

	void									OnMountTaskFinished();
	void									OnUnMountTaskFinished();
	void									OnUpdateSaveInfosTaskFinished();
	void									OnSaveSettingsTaskFinished();
	void									OnScreenshotReadTaskFinished();
	void									OnDeleteSaveDataTaskFinished();

	RED_INLINE Bool							IsMounting() const { return m_mountTask != nullptr; }
	RED_INLINE Bool							IsUnMounting() const { return m_unmountTask != nullptr; }
	RED_INLINE Bool							IsUpdatingSaveInfos() const { return m_updateSaveInfosTask != nullptr; }
	RED_INLINE Bool							IsSavingSettings() const { return m_saveSettingsTask != nullptr; }
	RED_INLINE Bool							IsReadingScreenshot() const { return m_screenshotReadTask != nullptr; }
	RED_INLINE Bool							IsDeletingSaveData() const { return m_deleteSaveDataTask != nullptr; }

	RED_INLINE Bool							IsDoingAsyncOp() const 
											{ 
												return IsMounting() 
													|| IsUnMounting() 
													|| IsUpdatingSaveInfos() 
													|| IsSavingSettings() 
													|| IsReadingScreenshot() 
													|| IsDeletingSaveData(); 
											}

	RED_INLINE Bool							IsSaveDataMounted() const { return ( m_saveDataMountResult.mountPoint.data[ 0 ] == '/' ); }
	RED_INLINE Bool							AreSettingsMounted() const { return ( m_userSettingsMountResult.mountPoint.data[ 0 ] == '/' ); }

	Bool									HandleOutOfDiskSpaceError( Int32 errorCode, const SceSaveDataDirName* dirName, Uint64 requiredBlocks );
	Bool									HandleBrokenSaveDataError( Int32 errorCode, const SceSaveDataDirName* dirName );
	Bool									HandleSaveDataErrorCode( Int32 errorCode, const SceSaveDataDirName* dirName, SceSaveDataDialogType type );

	void									DoSyncReadScreenshotData( const SSavegameInfo &info );
	void									DoDeleteSaveData( SceSaveDataDirName* dirName );

	//////////////////////////////////////////////////////////////////////////
	// UI
	void									InitCommonDialogParams( SceSaveDataDialogParam &param, const SceSaveDataDirName* dirName );

public:
	virtual void							OnUserDialogCallback( Int32 messageId, Int32 actionId );

	//////////////////////////////////////////////////////////////////////////
	// User settings
	void									UpdateSettingsSaving();
	void									DoSaveSettings();
	void									DoDeleteSettings();
	void									UpdateSettingsLoading();

public:
	virtual Bool							LoadUserSettings( StringAnsi& settingsString ) override;
	virtual Bool							SaveUserSettings( const StringAnsi& settingsString ) override;
	void									SetIgnoreSavingUserSettings(Bool value);

};
