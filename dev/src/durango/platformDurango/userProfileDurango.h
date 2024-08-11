/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//////////////////////////////////////////////////////////////////////////
// IUserProfileGamepadListenerDurango
//////////////////////////////////////////////////////////////////////////
class IUserProfileGamepadListenerDurango
{
public:
	typedef Windows::Xbox::Input::IGamepad IGamepad;

public:
	virtual void OnUserProfileGamepadChanged( IGamepad^ gamepad ) = 0;

protected:
	IUserProfileGamepadListenerDurango() {}
	virtual ~IUserProfileGamepadListenerDurango() {}
};

//////////////////////////////////////////////////////////////////////////
// CUserProfileManagerDurango
//
// Behavior modeled after "Non-joinable (lobby) apps" in the whitepaper
// "Users Controllers and Pairing Identity on Xbox One"
//////////////////////////////////////////////////////////////////////////
class CUserProfileManagerDurango : public CUserProfileManager
{
private:
	typedef Windows::Xbox::System::IUser									IUser;
	typedef Windows::Xbox::System::User										User;
	typedef Windows::Xbox::System::SignOutDeferral							SignOutDeferral;
	typedef Windows::Xbox::Input::IController								IController;
	typedef Windows::Xbox::Input::IGamepad									IGamepad;
	typedef Windows::Xbox::Input::Gamepad									Gamepad;
	typedef Windows::Xbox::Input::IGamepadReading							IGamepadReading;
	typedef Windows::Xbox::Input::RawGamepadReading							RawGamepadReading;
	typedef Windows::Xbox::Input::GamepadButtons							GamepadButtons;
	typedef Windows::Xbox::UI::AccountPickerResult							AccountPickerResult;
	typedef Windows::Foundation::IAsyncOperation< AccountPickerResult^ >	AccountPickerResultAsyncAction;
	typedef	Windows::Foundation::DateTime									DateTime;

	typedef Windows::Xbox::Services::XboxLiveConfiguration					LiveConfig;
	typedef Microsoft::Xbox::Services::XboxLiveContext						LiveContext;
	typedef Microsoft::Xbox::Services::Achievements::AchievementsResult		AchievementsResult;
	typedef Microsoft::Xbox::Services::Achievements::AchievementType		AchievementType;
	typedef Microsoft::Xbox::Services::Achievements::AchievementOrderBy		AchievementOrderBy;
	typedef Microsoft::Xbox::Services::Achievements::Achievement			Achievement;

	typedef Microsoft::Xbox::Services::Presence::PresenceData				PresenceData;
	typedef Microsoft::Xbox::Services::Presence::PresenceRecord				PresenceRecord;

	typedef Windows::Xbox::Storage::ConnectedStorageSpace					ConnectedStorageSpace;
	typedef Windows::Foundation::IAsyncOperation< ConnectedStorageSpace^ >	ConnectedStorageSpaceAsyncAction;
	typedef Windows::Storage::Streams::Buffer								Buffer;
	typedef Windows::Storage::Streams::IBuffer								IBuffer;
	typedef class CMemoryFileWriterExternalBuffer							TSaveWriter;

private:
	typedef Windows::Foundation::Collections::IVectorView< IGamepad^ > TGamepadVectorView;

private:
	enum EManagerState
	{
		eWaitingForNewActiveUserEngagement,		// E.g., start screen
		eWaitingForNewActiveUserRefresh,
		eWaitingForCurrentActiveUserEngagement,	// E.g., active controller lost
		eWaitingForUserSignIn,
		eWaitingForUserSignOut,
		eHasSignedInUserWithGamepad,
	};

	enum class ERefreshSubState
	{
		eRTS_Inactive,
		eRTS_Start,
		eRTS_Ongoing,
	};

	enum ERefreshState
	{
		// Not currently refreshing
		eRS_Inactive = 0,

		// The following 2 states are processed at the same time as they are not dependent on each other
		eRS_CachePads,			// Fetching the Pads array is a Cross-OS call and potentially blocking
		eRS_User,				// Deals with sign out/in and recovery of user objects after resume from suspend

		// The following 3 states are processed at the same time as they are not dependent on each other
		eRS_ConnectedStorage,	// Save data
		eRS_LiveContext,		// Rich presence, 
		eRS_Controllers,		// Has the users controller been disconnected?

		eRS_Finished
	};

	typedef Uint32 TRefreshType;

	static const TRefreshType RT_None								= 0;
	static const TRefreshType RT_Basic								= FLAG( 0 );
	static const TRefreshType RT_ManualControllerEngagement			= FLAG( 1 );
	static const TRefreshType RT_AutomaticControllerReestablishment	= FLAG( 2 );
	static const TRefreshType RT_ReacquireAfterSuspend				= FLAG( 3 );

	// Bit flags
	Uint32 m_refreshStatesActive;				// Which states have been activated and are currently processing
	Uint32 m_refreshStatesFinished;				// Which states have finished (When m_refreshStatesFinished matches m_refreshStatesActive, move onto the next set of states)

	ERefreshSubState m_refreshSubState;			// Indicates whether we're idle, waiting for states to complete or if they're ready to be started
	ERefreshState m_refreshState;				// Used to figure out which state to activate next
	Bool m_exitRefresh;							// One of the currently active states has requested we stop the refresh

	enum ELoadSaveState 
	{
		STATE_None				= 0,
		STATE_InitializeLoad	= 1,
		STATE_LoadInitialized	= 2,
		STATE_ModalUI			= 3,
		STATE_NoSaveMode		= 4, // note missing 5 
		STATE_Updating			= 6,
		STATE_DeletingSave		= 7,
		STATE_SaveInitialized	= 8,
		STATE_Saving			= 9,
		STATE_Ready				= 10,
	};

	enum EUserSettingsState : Int32 
	{ 
		SETTINGS_NotAvailable	= 0,
		SETTINGS_Loading		= 1,
		SETTINGS_Saving			= 2,
		SETTINGS_Ready			= 3,
	};

#ifdef RED_LOGGING_ENABLED
	static const Char* GetRefreshStateTxtForLog( ERefreshState state );
	static const Char* GetRefreshSubstateTxtForLog( ERefreshSubState substate );
	static const Char* GetManagerStateTxtForLog( EManagerState state );
	static const Char* GetRefreshTypeTxtForLog( TRefreshType type );
	static const Char* GetSaveStateTxtForLog( ELoadSaveState state );
#endif

	struct SUserDialogRequest
	{
		enum EMessageId
		{
			TYPE_Nothing,
			QUESTION_WouldYouLikeToContinueWithoutSaving	= 0x360, // others will also use ids, hopefully with no conflict with this. Otherwise we'll need a better solution.
			WARNING_LoadingFailedDamagedData,
			WARNING_NoSpaceForSave,
			QUESTION_DebugTest,
		};

		enum EButtonSet
		{
			BUTTONS_Ok,
			BUTTONS_OkCancel,
			BUTTONS_YesNo
		};

		enum EResult
		{
			RESULT_Unknown = -1,
			RESULT_Ok,
			RESULT_Cancel,
			RESULT_Abort,
			RESULT_Yes,
			RESULT_No
		};

		enum EStatus
		{
			STATUS_Initialized,
			STATUS_Displayed,
			STATUS_Closed
		};

		EMessageId	m_type;
		EButtonSet	m_buttonSet;
		EStatus		m_status;
		EResult		m_result;

		SUserDialogRequest();
		SUserDialogRequest( EMessageId type );

		RED_INLINE Bool IsValid() const { return m_type != TYPE_Nothing; }
		RED_INLINE void Invalidate() { m_type = TYPE_Nothing; }

		String GetLocalizedTitleString() const;
		String GetLocalizedMessageString() const;
	};

private:
	TDynArray< IUserProfileGamepadListenerDurango* > m_gamepadListeners;
	THashMap< CName, String >				m_presenceIDs;

	IUser^									m_userQueuedForSignin;
	IUser^									m_activeUserStub;		// Active user stub, regardless of subsequent controller handoffs and user pairing change events.
	SignOutDeferral^						m_signoutDeferral;		// Set when the active user begins signing out. This allows us to pause the signout operation and save data beforehand
	Red::Threads::CMutex					m_signoutDeferralMutex;
	IGamepad^								m_activeGamepad;		// Active gamepad to poll for input
	User^									m_activeUserObject;		// Active user object, associated with the stub
	LiveContext^							m_liveContext;			// Xbox Live context, associated with the active user object
	Uint32									m_activeUserId;			// Used to re-establish the active user object in the event of a sign out & in during suspension
	Bool									m_userObjectReestablished;

	TGamepadVectorView^						m_cachedGamepads;		// TBD: Cache async to avoid burdening the WinRT UI thread.

	EManagerState							m_managerState;
	Bool									m_plmSuspended;
	Bool									m_accountPickerActive;
	AccountPickerResultAsyncAction^			m_accountPickerOperation;

	static const Uint32						SAVE_BUFFER_SIZE = 16 * 1024 * 1024;
	static const Uint32						MAX_SAVE_SIZE = 16 * 1024 * 1024;
	static const Int16						NUM_AUTOSAVE_SLOTS = 2;
	static const Int16						NUM_CHECKPOINT_SLOTS = 3;
	static const Int16						NUM_QUICKMANUAL_SLOTS = 8;
	static const Uint32						MAX_USERSETTINGS_SIZE = 10 * 1024;
	static const Uint32						MAX_CONTENTDESCRIPTOR_SIZE = 1024;
	static const Int32						SLOT_SANITY_CHECK = 256;

	Red::Threads::CAtomic< Int32 >			m_currentLoadOpProgress;
	Red::Threads::CAtomic< Int32 >			m_loadSaveState;
	ConnectedStorageSpaceAsyncAction^		m_connectedStorageSpaceAsyncOperation;
	ConnectedStorageSpace^					m_connectedStorageSpace;
	Buffer^									m_saveBuffer;
	Buffer^									m_screenshotBuffer;
	Buffer^									m_userSettingsBuffer;
	Buffer^									m_contentDescriptorBuffer;
	Windows::Foundation::IAsyncAction^		m_screenshotDataReadAction;
	TDynArray< SSavegameInfo >				m_saveInfos;
	Int32									m_initializedLoadSaveInfoIndex;
	Red::Threads::CAtomic< Int32 >			m_saveInfosUpdateState;
	Red::Threads::CAtomic< Bool >			m_screenshotDataRead;
	Red::Threads::CAtomic< Int32 >			m_userSettingsState;
	Red::Threads::CAtomic< Bool >			m_ignoreSaveUserSettingsRequest;
	mutable Red::Threads::CMutex			m_saveInfosMutex;
	Int32									m_saveLock;
	SUserDialogRequest						m_modalBox;
	Bool									m_writerCreated;

private:

	// Futile to respond to the events directly
	Red::Threads::CAtomic< TRefreshType >	m_refreshRequested;
	TRefreshType							m_refreshOngoing;

	TSortedMap<CName, String>				m_nameToXboxAchievementMap;

	#ifdef SAVE_SERVER_ENABLED
		CSaveServer							m_debugSaveServer;
	#endif // SAVE_SERVER_ENABLED

public:
											CUserProfileManagerDurango();
											~CUserProfileManagerDurango();

	virtual const Char*						GetPlatformUserIdAsString();

public:
	virtual Bool							Initialize() override final { return true; }
	virtual void							Update() override final;
	virtual Bool							Shutdown() override final { return true; }

	void									AddGamepadListener( IUserProfileGamepadListenerDurango* listener );
	void									RemoveGamepadListener( IUserProfileGamepadListenerDurango* listener );

private:
	void									InitEventHandlers();

	virtual Bool							GetSafeArea( Float& x, Float& y ) override final;
	Bool									SendTelemetryEvent( const String& service, const String& name, const String& category, const String& value );

	//////////////////////////////////////////////////////////////////////////
	// General System
	//////////////////////////////////////////////////////////////////////////

	virtual void							DisplayHelp() override final;
	virtual void							DisplayStore() override final;

	//////////////////////////////////////////////////////////////////////////
	// Processor Lifetime Management
	//////////////////////////////////////////////////////////////////////////
private:
	virtual void							OnEnterSuspendState() override final;
	virtual void							OnExitSuspendState() override final;
	virtual void							OnEnterConstrainedState() override final;
	virtual void							OnExitConstrainedState() override final;

	//////////////////////////////////////////////////////////////////////////
	// Active user management
	//////////////////////////////////////////////////////////////////////////
public:
	virtual String							GetActiveUserDisplayNameRaw() const override final;
	virtual void							DisplayUserProfileSystemDialog() override final;

	virtual void							SetActiveUserDefault() override final;
	virtual void							SetActiveUserPromiscuous() override final;
	virtual Bool							HasActiveUser() const override final;
	virtual void							ChangeActiveUser() override final;

private:
	void									NeedsRefresh( TRefreshType type );

	Bool									HandleInput();
	Bool									ResetState();

#ifdef RED_LOGGING_ENABLED
	void									LogUserProfileState();
#endif

	void									HandleUserEngagement();

	void									RefreshTick();
	void									RefreshStateActivated( ERefreshState state );
	void									RefreshStateComplete( ERefreshState state, Bool exitRefresh = false );

	void									CheckForNewActiveUserEngagement();
	void									CheckForCurrentActiveUserEngagement();

	IGamepad^								GetFirstEngagedGamepad();

	void									PromptUserSignInAsync( IGamepad^ engagedGamepad );
	void									CachePadsAsync();

	void									UserRefresh();
	void									ControllerRefresh();

	Bool									UpdateActiveUser( IUser^ user );
	Bool									ReacquireActiveUser();
	void									FindUserFromIdAsync( Uint32 id );
	void									FindUserFromInterfaceAsync( IUser^ user );
	static User^							FindUserFromInterface( IUser^ user );
	void									CreateLiveContextAsync( User^ user );
	static LiveContext^						CreateLiveContext( User^ user );

	RED_INLINE static Bool					IsValidActiveUser( IUser^ user );

	RED_INLINE Bool							HasValidActiveUser() const;
	RED_INLINE Bool							HasValidActiveGamepad() const { return m_activeGamepad != nullptr; }
	void									UpdateActiveGamepad( IGamepad^ gamepad );

	static IGamepad^						FindUserGamepad( IUser^ user );

	//////////////////////////////////////////////////////////////////////////
	// Savegame management
	//////////////////////////////////////////////////////////////////////////
private:
	void									OnSaveSystemUpdate();
	void									PrepareConnectedStorage();
	void									UpdateSaveInfos();

	void									RequestModalUIBox( SUserDialogRequest::EMessageId type );
	void									OnModalUIBoxClosed();
	
	void									DoSave( Platform::String^ containerName, Platform::String^ displayName );
	
	void									OnSaveCompleted( Bool successful );
	void									OnSaveInfosUpdated();
	void									OnLoadGameInitialized();
	void									OnLoadGameInitFailed();
	void									OnSaveGameDeleted();
	void									OnScreenshotDataRead( Bool success );

	Bool									IsSaveSystemBusy() const;

	Int32									FindSaveInfoIndex( ESaveGameType type, Int16 slotIndex ) const;
	static void								GenerateSaveFileName( SSavegameInfo& info );
	static void								ParseFilename( SSavegameInfo& info );

	Uint8*									GetBufferByteAccess( IBuffer^ buf );

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
	virtual void							FinalizeGameSaving( IGameSaver* ) override;
	
											// managing
	virtual void							GetSaveFiles( TDynArray< SSavegameInfo >& files ) const override;
	virtual Bool							GetLatestSaveFile( SSavegameInfo& info ) const override;
	virtual void							DeleteSaveGame( const SSavegameInfo& info ) override;
	virtual Bool							AreSavesInitialized() const;
	virtual Int16							GetNumSaveSlots( ESaveGameType type ) const;
	virtual Bool							GetSaveInSlot( ESaveGameType type, Int16 slot, SSavegameInfo& info ) const;
	virtual String							BuildFullDisplayNameForSave( const SSavegameInfo& info ) const;

	//////////////////////////////////////////////////////////////////////////
	// Achievement management
	////////////////////////////////////////////////////////////////////////// 
private:
	virtual Bool							IsAchievementMapped( const CName& name ) const override final;
	virtual void							MapAchievementInit( Uint32 numAchievements ) override final;
	virtual void							MapAchievement( const CName& name, const String& platform, const String& id ) override final;

	//! Unlocks an achievement for a user.
	virtual void							UnlockAchievement( const CName& name ) override final;


	//////////////////////////////////////////////////////////////////////////
	// Presence management
	////////////////////////////////////////////////////////////////////////// 
public:
	virtual void							DoUserPresence( const CName& presenceName ) override;
	virtual void							MapPresenceInit( Uint32 numEntries );
	virtual void							MapPresence( const CName& name, const String& platform, const String& id );

	//////////////////////////////////////////////////////////////////////////
	// UI
	//////////////////////////////////////////////////////////////////////////

public:
	virtual void							OnUserDialogCallback( Int32 messageId, Int32 actionId );

	//////////////////////////////////////////////////////////////////////////
	// User settings
	//////////////////////////////////////////////////////////////////////////

public:
	virtual Bool							LoadUserSettings( StringAnsi& settingsString ) override;
	virtual Bool							SaveUserSettings( const StringAnsi& settingsString ) override;
	virtual void							SetIgnoreSavingUserSettings( Bool value ) override;

private:
	void									UpdateUserSettings();
	void									OnUserSettingsLoaded();
	void									OnUserSettingsLoadingFailed();
	void									OnUserSettingsSaved();
	void									OnUserSettingsSavingFailed();

	//////////////////////////////////////////////////////////////////////////
	// Save-games
	//////////////////////////////////////////////////////////////////////////
	IFileEx* CreateSaveFileRawWriter( const SSavegameInfo& saveInfo );	// Get an IFile with raw access to save buffer

	//////////////////////////////////////////////////////////////////////////
	// DEBUG
	////////////////////////////////////////////////////////////////////////// 
public:
	//! debug print statistic info
	virtual void PrintUserStats( const String& statName );
	//! debug print achievement info
	virtual void PrintUserAchievement( const String& achievementName );

	static void DebugPrintUserAndControllerBindings( IUser^ activeUser );
};

#ifdef RED_LOGGING_ENABLED
#	define DEBUG_USER_PROFILE
#endif

#ifdef DEBUG_USER_PROFILE
#	define STATE_CHECK( x ) if ( m_loadSaveState.GetValue() != x ) RED_LOG( DurangoSaveState, TXT( "State check failure in %ls line %ld, expected state %ld, while encountered %ld\n" ), __FUNCTIONW__, __LINE__, x, m_loadSaveState.GetValue() );
#	define SET_STATE( x ) RED_LOG( DurangoSaveState, TXT( "State change in %ls line %ld, state %ld -> %ld\n" ), __FUNCTIONW__, __LINE__, m_loadSaveState.GetValue(), x ); m_loadSaveState.SetValue( x )
#else
#	define STATE_CHECK( x )
#	define SET_STATE( x ) m_loadSaveState.SetValue( x )
#endif
