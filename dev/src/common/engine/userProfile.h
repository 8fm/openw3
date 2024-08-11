/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef __USER_PROFILE_MANAGER_H__
#define __USER_PROFILE_MANAGER_H__

//////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////
#include "notifier.h"
#include "saveServer.h"
#include "../core/enum.h"
#include "../core/enumBuilder.h"
#include "../core/configVarSystem.h"
#include "gameSaveManager.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
struct SLeaderboardStatsRow;
class CXMLFileReader;

enum ELoadGameResult : CEnum::TValueType 
{
	LOAD_NotInitialized,			// returned when no loading operation was initialized
	LOAD_Initializing,				// returned when initializing, until everything gets initialized (mounted on orbis/synced on durango/etc...)
	LOAD_ReadyToLoad,				// returned when you can actually create loader and start reading save data
	LOAD_Loading,					// returned when the game is actually being loaded (loader was created, but not finalized)
	LOAD_Error,						// returned when initialization failed for some reason
	LOAD_MissingContent				// returned when initialization succeeded, but content requirements are not met
};

BEGIN_ENUM_RTTI( ELoadGameResult )
	ENUM_OPTION( LOAD_NotInitialized )
	ENUM_OPTION( LOAD_Initializing )
	ENUM_OPTION( LOAD_ReadyToLoad )
	ENUM_OPTION( LOAD_Loading )
	ENUM_OPTION( LOAD_Error )
	ENUM_OPTION( LOAD_MissingContent )
END_ENUM_RTTI()

enum ESaveListUpdateState : CEnum::TValueType 
{
	LIST_NeedsUpdate,		
	LIST_Updating,		
	LIST_Updated_EventPending,		
	LIST_Updated,				
};

BEGIN_ENUM_RTTI( ESaveListUpdateState )
	ENUM_OPTION( LIST_NeedsUpdate )		
	ENUM_OPTION( LIST_Updating )		
	ENUM_OPTION( LIST_Updated_EventPending )		
	ENUM_OPTION( LIST_Updated )				
END_ENUM_RTTI()

enum ESaveGameResult : CEnum::TValueType 
{
	SAVE_NotInitialized,			// returned when no saving operation was initialized
	SAVE_Initializing,				// returned when initializing, until everything gets initialized (mounted on orbis/synced on durango/etc...)
	SAVE_ReadyToSave,				// returned when you can actually create saver and start storing save data
	SAVE_Saving,					// returned when the game is actually being saved (saver was created, but not finalized)
	SAVE_Error,						// returned when initialization failed for some reason
};

BEGIN_ENUM_RTTI( ESaveGameResult )
	ENUM_OPTION( SAVE_NotInitialized )
	ENUM_OPTION( SAVE_Initializing )
	ENUM_OPTION( SAVE_ReadyToSave )
	ENUM_OPTION( SAVE_Saving )
	ENUM_OPTION( SAVE_Error )
END_ENUM_RTTI()

///////////////////////////////////////////////////////////////////////////////////
class IUserProfileListener;

///////////////////////////////////////////////////////////////////////////////////

// Signed-in user listener
enum class EUserEvent
{
	UE_SignInStarted,
	UE_SignInCancelled,

	UE_SignedIn,
	UE_SignedOut,

	UE_LoadSaveReady,
	UE_GameSaved,
	UE_GameSaveFailed,

	// Durango only
	UE_AccountPickerOpened,
	UE_AccountPickerClosed,
};

///////////////////////////////////////////////////////////////////////////////////
// Save game
///////////////////////////////////////////////////////////////////////////////////

struct SLeaderboardStatsRow;
struct GameSaveInfo;
class IGameSaver;

class CXMLFileReader;
struct SSavegameInfo;
class IObsoleteSaveImporter;

class IUserProfileSavegameInterface
{ 
	#ifndef NO_SAVE_IMPORT
		protected:
			typedef IObsoleteSaveImporter* ( CreateSaveImporterFunc )( void );
			CreateSaveImporterFunc*	m_saveImporterFunc;
		public:
			virtual void SetImporter( CreateSaveImporterFunc* func ) {	m_saveImporterFunc = func; }

			IUserProfileSavegameInterface()
				: m_saveImporterFunc( nullptr )
			{
			}
	#endif

public:
	// loading
	virtual ELoadGameResult InitGameLoading( const SSavegameInfo& info ) = 0;
	virtual ELoadGameResult GetLoadGameProgress() const = 0;
	virtual void CancelGameLoading() = 0;
	virtual IFile* CreateSaveFileReader( const Bool rawFile = false ) = 0;
	virtual void RequestScreenshotDataForReading( const SSavegameInfo& info ) = 0;
	virtual Bool IsScreenshotDataReadyForReading() = 0;
	virtual IFile* CreateScreenshotDataReader() = 0;
	virtual void DoneReadingScreenshotData() = 0;						
	virtual void FinalizeGameLoading() = 0;
	virtual TDynArray< CName > GetContentRequiredByLastSave() const { return TDynArray< CName > (); }
																							 
	// saving
	virtual ESaveGameResult InitGameSaving( SSavegameInfo& info ) = 0;
	virtual ESaveGameResult GetSaveGameProgress() const = 0;
	virtual void CancelGameSaving() = 0;
	virtual IFileEx* CreateSaveFileWriter() = 0; 
	virtual void* GetScreenshotBuffer() = 0;
	virtual void OnScreenshotDone( Uint32 realSize ) = 0;
	virtual void FinalizeGameSaving( IGameSaver* saverToFinalize ) = 0;

	// managing
	virtual void GetSaveFiles( TDynArray< SSavegameInfo >& files ) const = 0;
	virtual Bool GetLatestSaveFile( SSavegameInfo& info ) const = 0;
	virtual void DeleteSaveGame( const SSavegameInfo& info ) = 0;
	virtual Bool AreSavesInitialized() const = 0;
	virtual Int16 GetNumSaveSlots( ESaveGameType type ) const { return -1; } // no limit by default
	virtual Bool GetSaveInSlot( ESaveGameType type, Int16 slot, SSavegameInfo& info ) const; // no slots by default
	virtual String BuildFullDisplayNameForSave( const SSavegameInfo& info ) const;
	virtual Bool IsDisplayingSystemMessage() const { return false; } // impl only on orbis
	virtual Bool HaveLockedUserActions() const { return false; } // impl only on orbis

	#ifndef NO_SAVE_IMPORT
		//! Import facts from obsolete save
		virtual Bool ImportSave( const SSavegameInfo& gameSaveFileName ) { return false; }

		//! Gets list of obsolete saves
		virtual void Import_GetSaveFiles( TDynArray< SSavegameInfo >& files ) {}

		//! Gets list of obsolete saves
		virtual void Import_GetSearchPaths( TDynArray< String >& outArray ) { }
#endif // NO_SAVE_IMPORT

};

//////////////////////////////////////////////////////////////////////////
// User Profile Manager
//////////////////////////////////////////////////////////////////////////
class CUserProfileManager : public Events::CNotifier< EUserEvent >, public IUserProfileSavegameInterface
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CUserProfileManager();
	virtual ~CUserProfileManager();

	virtual Bool Initialize() = 0;

	//! Updates status of signed-in user.
	virtual void Update();

	virtual Bool Shutdown() = 0;

	Bool LoadMap( CXMLReader& mapFile, const String& filepath );

	//////////////////////////////////////////////////////////////////////////
	// Input Processing
	// Prevent the user profile manager from consuming input (and popping up the account picker at inappropriate times)
	//////////////////////////////////////////////////////////////////////////
public:
	enum EInputProcessingDisabledReason
	{
		// During the initial startup sequence
		eAPDR_Startup = 0,

		// 
		eAPDR_Ingame,

		// Popups and other game related reasons
		eAPDR_User,
	};

	virtual void ToggleInputProcessing( Bool enabled, EInputProcessingDisabledReason reason );

protected:
	Uint32 m_disableInput;

	//////////////////////////////////////////////////////////////////////////
	// Profile event queue
	//////////////////////////////////////////////////////////////////////////
protected:
	void QueueEvent( EUserEvent event );

private:
	void UpdateEventQueue();

	TDynArray< EUserEvent > m_eventQueue;
	Red::Threads::CMutex m_eventQueueMutex;

	//////////////////////////////////////////////////////////////////////////
	// General System
	//////////////////////////////////////////////////////////////////////////
public:
	virtual void DisplayHelp() {}
	
	virtual void DisplayStore() {}

	virtual void LoadContentRestrictionXML( CXMLReader* xmlReader ) {}

	//////////////////////////////////////////////////////////////////////////
	// PLM (As it relates to user profiles)
	//////////////////////////////////////////////////////////////////////////
public:
	virtual void OnEnterSuspendState() {}
	virtual void OnExitSuspendState() {}

	virtual void OnEnterConstrainedState() {}
	virtual void OnExitConstrainedState() {}

public:
	virtual Bool HACK_ShowSteamControllerBindingPanel() const { return false; }
	virtual Bool HACK_IsUsingSteamController() const { return false; }

public:
	//////////////////////////////////////////////////////////////////////////
	// Achievement management
	//////////////////////////////////////////////////////////////////////////
	
	//! Unlocks an achievement for a user.
	virtual void UnlockAchievement( const CName& name );

	//! Locks the achievement, debug only.
	virtual void LockAchievement( const CName& name );

	//! Check if achievement is locked.
	virtual Bool IsAchievementLocked( const CName& name ) const;

	virtual void GetAllAchievements( TDynArray< CName >& achievements ) const;

protected:

	// Debug validation required
	virtual Bool IsAchievementMapped( const CName& name ) const;

	// Prepare for MapAchievement() to be called numAchievements times (Treat this operation as an append, not a reset)
	virtual void MapAchievementInit( Uint32 numAchievements );
	virtual void MapAchievement( const CName& name, const String& platform, const String& id );

	//////////////////////////////////////////////////////////////////////////
	// Presence management
	////////////////////////////////////////////////////////////////////////// 
public:
	//! Sets the presence information for this title for the user
	void SetUserPresence( const CName& presenceName );

private:
	virtual void DoUserPresence( const CName& presenceName ) = 0;
	virtual void MapPresenceInit( Uint32 numEntries );
	virtual void MapPresence( const CName& name, const String& platform, const String& id );

	CName m_queuedUserPresence;
	Red::System::StopClock m_timeSinceLastPresenceUpdate;

protected:
	Float m_minTimeBetweenRichPresenceUpdates;

	//////////////////////////////////////////////////////////////////////////
	// XML Processing
	////////////////////////////////////////////////////////////////////////// 
private:
	typedef void ( CUserProfileManager::*MapItemCallback )( const CName&, const String&, const String& );

	Bool ReadXMLNodeRoot( CXMLReader& xmlReader, const String& filepath );

	Bool ReadXMLNodeGroup( CXMLReader& xmlReader, const String& filepath );
	Bool ReadXMLNodeItems( CXMLReader& xmlReader, const String& itemNode, MapItemCallback callback, const String& filepath );
	Bool ReadXMLNodeItem( CXMLReader& xmlReader, const String& nodeName, MapItemCallback callback, const String& filepath );
	void ReportParsingError( const String& filepath, const String& error, Bool warning = false );

	static const String ROOT_NODE;

	static const String ACHIEVEMENTS_NODE;
	static const String ACHIEVEMENT_NODE;

	static const String PRESENCE_GROUP_NODE;
	static const String PRESENCE_ITEM_NODE;

	static const String PLATFORM_NODE;
	static const String NAME_ATTRIBUTE;

	//////////////////////////////////////////////////////////////////////////
	// Active user management
	//////////////////////////////////////////////////////////////////////////
protected:
	virtual String GetActiveUserDisplayNameRaw() const = 0;

public:
	// Get the nickname/alias/gamertag of the player for the purpose of displaying on the screen somewhere
	String GetActiveUserDisplayName( Uint32 maxVisibleCharacters ) const;

	// If you have the user's name on display, you'll need to call this if they click on it
	virtual void DisplayUserProfileSystemDialog() = 0;

	// Receive input only from the system specified initial user
	virtual void SetActiveUserDefault() = 0;

	// Receive input from all controllers
	virtual void SetActiveUserPromiscuous() = 0;

	virtual Bool HasActiveUser() const = 0;

	// Immediately display the account picker (if applicable)
	virtual void ChangeActiveUser() {}

	virtual const Char* GetPlatformUserIdAsString() { return nullptr; }

	//////////////////////////////////////////////////////////////////////////
	// Screen Safe Area
	//////////////////////////////////////////////////////////////////////////

	// If this function returns true, The minimum distance you should place important UI elements
	// from the sides of the screen are placed into:
	// x: left and right
	// y: top and bottom
	virtual Bool GetSafeArea( Float& x, Float& y ) = 0;

	//////////////////////////////////////////////////////////////////////////
	// Statistics 
	//////////////////////////////////////////////////////////////////////////

	virtual void SetStat( const String& name, Int32 value );
	virtual void SetStat( const String& name, Float value );
	virtual void SetAverageStat( const String& name, Float countThisSession, Double sessionLength );

	virtual void IncrementStat( const String& name, Int32 value );
	virtual void IncrementStat( const String& name, Float value );

	virtual void GetStat( const String& name, Int32& value );
	virtual void GetStat( const String& name, Float& value );

	//////////////////////////////////////////////////////////////////////////
	// Witcher 2 cloud save import
	//////////////////////////////////////////////////////////////////////////
	virtual Uint32 GetNumberOfWitcher2CloudSavePaths() const;
	virtual String GetWitcher2CloudSavePath( Uint32 index ) const;

	//////////////////////////////////////////////////////////////////////////
	// Debug info 
	//////////////////////////////////////////////////////////////////////////
	
public:
	//! debug print statistic info
	virtual void PrintUserStats( const String& ) {}

	//! debug print achievement info
	virtual void PrintUserAchievement( const String& ){}

	//////////////////////////////////////////////////////////////////////////
	// UI
	//////////////////////////////////////////////////////////////////////////

public:
	virtual void OnUserDialogCallback( Int32, Int32 ) {}

	//////////////////////////////////////////////////////////////////////////
	// User settings
	//////////////////////////////////////////////////////////////////////////

public:
	virtual Bool LoadUserSettings( StringAnsi& settingsString ) = 0;			// moved to IUserProfileSaveEngineSettingsInterface
	virtual Bool SaveUserSettings( const StringAnsi& settingsString ) = 0;	// moved to IUserProfileSaveEngineSettingsInterface

	//! We need to ignore saving settings in certain situations (like after sign out event)
	// In future we should introduce PostSignOut | SignOutComplete event or something similar
	virtual void SetIgnoreSavingUserSettings( Bool value ) { /* Intentionally empty - implemented where necessary */ };

	//////////////////////////////////////////////////////////////////////////
	// System Language
	//////////////////////////////////////////////////////////////////////////

public:
	virtual Bool GetSystemLanguageStrings( String& language ) const;
	virtual Bool GetSystemLanguageSpeech( String& language ) const;
};

extern CUserProfileManager* GUserProfileManager;

#endif // __USER_PROFILE_MANAGER_H__
