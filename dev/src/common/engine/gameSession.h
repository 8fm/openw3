/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/rc4generator.h"
#include "userProfile.h"
#include "../core/namesReporter.h"

enum ESaveAttemptResult
{
	SAR_Success,
	SAR_SaveLock,
	SAR_WriteFailure,
};

BEGIN_ENUM_RTTI( ESaveAttemptResult );
	ENUM_OPTION( SAR_Success );
	ENUM_OPTION( SAR_SaveLock );
	ENUM_OPTION( SAR_WriteFailure );
END_ENUM_RTTI();

enum ESessionRestoreResult
{
	RESTORE_Success,			// ok
	RESTORE_DataCorrupted,		// corrupted save data
	RESTORE_DLCRequired,		// save made on DLC, and now that DLC is not enabled
	RESTORE_MissingContent,		// missing game content chunk (incomplete streaming install)
	RESTORE_InternalError,		// internal error, should not happen
	RESTORE_NoGameDefinition,	// save was made on a game definition that is missing now (like: some debug definition)
	RESTORE_WrongGameVersion,	// save was made on a newer game version (like: ptched game, while we're running unpatched)
};

BEGIN_ENUM_RTTI( ESessionRestoreResult )
	ENUM_OPTION( RESTORE_Success )
	ENUM_OPTION( RESTORE_DataCorrupted )
	ENUM_OPTION( RESTORE_DLCRequired )
	ENUM_OPTION( RESTORE_MissingContent )
	ENUM_OPTION( RESTORE_InternalError )		
	ENUM_OPTION( RESTORE_NoGameDefinition )	
	ENUM_OPTION( RESTORE_WrongGameVersion )
END_ENUM_RTTI()

///////////////////////////////////////////////////////////////////////////////////////////

class CRenderFrame;
class IGameSystem;
class IGameLoader;

///////////////////////////////////////////////////////////////////////////////////////////

struct STeleportInfo
{
	friend class CTeleportHelper;

	enum TargetType
	{
		TargetType_Unused,
		TargetType_Node,
		TargetType_Custom,
		TargetType_PositionAndRotation,
	};

private:
	TargetType		m_targetType;			// Teleport target type
	TagList			m_targetTag;			// Teleport target location tag
	Vector			m_targetPosition;		// Teleport target location position
	EulerAngles		m_targetRotation;		// Teleport target location rotation
	Vector			m_offset;				// Extra offset from targetTag position
	Float			m_distanceFromTarget;	// Teleport distance relative to target spawn point
	TagList			m_actorsTags;			// Tags of the actors to be teleported

public:
	STeleportInfo();

	void SetTarget( TargetType targetType, const TagList& targetTag, Float distanceFromTarget, const Vector& offset, const TagList& actorsTags );
	void SetTarget( TargetType targetType, const CName& targetTag, const CName& actorTag );
	void SetTarget( TargetType targetType, const Vector& position, const EulerAngles& rotation, const CName& actorTag );
	void SetTarget( TargetType targetType, const TagList& targetTag );
};

/// Game info
class CGameInfo
{
public:
	Vector			m_cameraPosition;					// Initial position of the camera
	EulerAngles		m_cameraRotation;					// Initial camera rotation

	String			m_worldFileToLoad;					// World file that should be loaded
	IGameLoader*	m_gameLoadStream;					// Initialization stream for loading game form game save
	IGameLoader*	m_playerLoadStream;					// Initialization stream for loading only a player form game save
	STeleportInfo	m_teleport;

	Bool			m_inEditorGame				: 1;	// Is this in editor game ( PIE )
	Bool			m_keepExistingLayers		: 1;	// Do not auto load layers from world parition ( PIE )
	Bool			m_hideCursor				: 1;	// Should we make an exclusive lock on the cursor?
	Bool			m_isChangingWorldsInGame	: 1;	// Are we just changing in-game world ( It is otherwise indistinguishable from ending/starting the game :/ )
	Bool			m_allowQuestsToRun			: 1;	// Should the quest be started?
	Bool			m_setBlackscreen			: 1;	// Should the blackscreen be set after game starts
	Bool			m_standaloneDLCJustStarting : 1;	// DLC is starting in "standalone" mode

	Uint32										: 0;	// Pad to 32 bits

public:
	//! Constructor
	CGameInfo( )
		: m_cameraPosition( 0.f, 0.f, 0.f )
		, m_cameraRotation( 0.f, 0.f, 0.f )
		, m_gameLoadStream( nullptr )
		, m_playerLoadStream( nullptr )
		, m_inEditorGame( false )
		, m_keepExistingLayers( false )
		, m_hideCursor( true )
		, m_isChangingWorldsInGame( false )
		, m_allowQuestsToRun( true )
		, m_setBlackscreen( false )
		, m_standaloneDLCJustStarting( false )
	{}

	Bool IsNewGame() const { return !m_gameLoadStream && !m_isChangingWorldsInGame; }
	Bool IsStandaloneDLCStarting() const { return m_standaloneDLCJustStarting; }
	Bool IsSavedGame() const { return !!m_gameLoadStream; }
	Bool IsChangingWorld() const { return m_isChangingWorldsInGame; }
};

///////////////////////////////////////////////////////////////////////////////////////////

/// This is a basic interface for all systems that dump some save-critical data
class IGameSaveSection
{
public:
	IGameSaveSection();
	virtual ~IGameSaveSection();

	//! Called in order to initialize the system
	virtual void OnGameStart( const CGameInfo& gameInfo ) = 0;

	//! Called when we are creating saving the save game
	virtual bool OnSaveGame( IGameSaver* saver )=0;
};

///////////////////////////////////////////////////////////////////////////////////////////

/// Save lock
typedef Int32 CGameSaveLock;

///////////////////////////////////////////////////////////////////////////////////////////

/// This is manager of the game session. 
class CGameSessionManager
{
private:
	typedef THashMap< IGameSystem*, TDynArray< IGameSystem* > > TDependenciesMap;

protected:
	struct SSessionHistoryEvent
	{
		enum EType
		{
			EVENT_Null = 0,
			EVENT_GameStarted,
			EVENT_GameSaved,
			EVENT_GameLoaded,
		};
		
		EType			m_type;	
		Uint16			m_saveVersion;
		Red::DateTime	m_time;

		SSessionHistoryEvent() {}
		SSessionHistoryEvent( EType t, Uint32 v );

		void Save( IGameSaver* saver );
		void Load( IGameLoader* loader );
		void Log();
	};

	struct SLockData
	{
		Uint8	m_lockedTypes;
		Bool	m_unique : 1;
		String	m_reason;

		static const Uint8 DEFAULT_LOCKED_TYPES = ( FLAG( SGT_AutoSave ) | FLAG( SGT_QuickSave ) | FLAG( SGT_Manual ) | FLAG( SGT_CheckPoint ) );

		SLockData() : m_lockedTypes( DEFAULT_LOCKED_TYPES ), m_unique( false ) {}
		explicit SLockData( const String& reason ) : m_lockedTypes( DEFAULT_LOCKED_TYPES ), m_unique( true ), m_reason( reason ) {}
		SLockData( Uint8 lockedTypes, Bool unique, const String& reason ) : m_lockedTypes( lockedTypes ), m_unique( unique ), m_reason( reason ) {}
		Bool operator==( const SLockData& other ) const { return m_unique && other.m_unique && m_reason == other.m_reason; }

		String GetDebugString() const;
	};

	TDynArray< IGameSaveSection* >			m_sections;				//!< Registered game save sections
	TDependenciesMap						m_dependenciesMap;		//!< Systems dependencies map
	THashMap< CGameSaveLock, SLockData >	m_saveLocks;			//!< Save locks
	TDynArray< SSessionHistoryEvent >		m_sessionHistory;		//!< Historical data

	CGameInfo								m_gameInfo;
	CGameSaveLock							m_nextLockIdx;
	Bool									m_showBlackscreenAfterRestore;		//!< Should a blackscreen be displayed when a game is being loaded
	CGameSaveLock							m_saveLockFor5sec;
	Float									m_saveLockCountdown;	//!< Counter for above 3-sec lock
	CStandardRand							m_randomNumberGenerator;
	CNamesRemapper							m_namesReporter;
	TDynArray< CName >						m_missingContent;

	Uint64									m_sessionHistoryTimePlayed;
	Red::System::DateTime					m_lastSessionHistoryTime;

#ifndef FINAL
	struct SDebugMessage
	{
		Float					m_timeout;
		Color					m_color;
		String					m_message;
		TDynArray< SLockData >	m_saveLocksDebug;
	};
	TDynArray< SDebugMessage >				m_debugMessages;
#endif // ndef FINAL

public:
	static const CGameSaveLock	GAMESAVELOCK_INVALID = -1;
	static const Float MSG_TIME;

public:
	//! Are game saves locked ?
	Bool AreGameSavesLocked( ESaveGameType type ) const;
	void PrintActiveSaveLocks() const;

public:
	CGameSessionManager();
	~CGameSessionManager();

	RED_INLINE CNamesRemapper& GetCNamesRemapper() { return m_namesReporter; }

	RED_INLINE const TDynArray< CName >& GetMissingContent() const { return m_missingContent; }

	//! Restore game session from save game ( load game )
	ESessionRestoreResult RestoreSession( IGameLoader* gameLoader, Bool suppressVideoToPlay = false, Bool isStandaloneDLCStart = false );
	ESessionRestoreResult CreateExpansionSession( IGameLoader* gameLoader, Bool suppressVideoToPlay = false );

	//! Create session on given world file, will not reset current session
	Bool CreateSession( 
		const String& gameWorldFile, 
		Bool isChangingWorld = false, 
		const STeleportInfo* teleportInfo = nullptr, 
		const String& loadingVideo = String::EMPTY,
		IGameLoader* playerLoadStream = nullptr );

	//! Create editor session on current world
	Bool CreateEditorSession( Bool fastPlay, const Vector& cameraPosition, const EulerAngles& cameraRotation, Bool hideCursor, const String& loadingVideo = String::EMPTY );

	//! Save game session to file ( save game )
	Bool SaveSession( IGameSaver* gameSaver, const String& saveDescription = String::EMPTY );

	void EndSession();

	//! Generate debug fragments
	void GenerateDebugFragments( CRenderFrame* frame );

	void AddFailedSaveDebugMessage( ESaveGameType, const Char* reason, const Char* initiatedBy );

public:
	//! Create savegame lock
	void CreateNoSaveLock( const String& reason, CGameSaveLock& saveLock, Bool unique = false, Bool allowCheckpoints = true );

	//! Enable save games
	void ReleaseNoSaveLock( CGameSaveLock lock );
	void ReleaseNoSaveLockByName( const String& lockName );

public:
	//! Register game save section saver
	void RegisterGameSaveSection( IGameSaveSection* section );

	//! Unregister game save section saver
	void UnregisterGameSaveSection( IGameSaveSection* section );

	//! Defines an inter-system dependency
	void DefineGameSystemDependency( IGameSystem* child, IGameSystem* parent = NULL );

	void RemoveGameSystemDependency( IGameSystem* parent );

	//! Returns game info of the active game
	RED_INLINE const CGameInfo& GetGameInfo() const { return m_gameInfo; }

	//! Shows debug message on screen
	void ShowDebugMessage( const String& msg, const Color& color, Float timeout );

	// Introduce save lock for X seconds - this is needed to meet TCR 047
	void CreateTimedLock( Float lockDuration = 5.0f );

	void UpdateLocks( Float timeDelta );

	void ShowBlackscreenAfterRestore( Bool flag ) { m_showBlackscreenAfterRestore = flag; }
	Bool ShouldBlackscreenBeShownAfterRestore() const { return m_showBlackscreenAfterRestore; }

	GameTime CalculateTimePlayed() const;

	Bool ContainsHistoryEventFromVersion( Uint16 version ) const;

	Bool IsNewGame() const;
	Bool IsStandaloneDLCStarting() const;
	Bool SHIsLoadedGame() const;

	void OnEditorLoadGame();

private:
	Uint64 CalculateCurrentSessionTime() const;

protected:
	void OnSHGameStarted();
	void OnSHGameSaved();
	void OnSHGameLoaded( Uint32 version );
	Bool IsOldSave() const;

public:

	//////////////////////////////////////////////////////////////////////////
	// Utils for mcinek's protection system :)
	//////////////////////////////////////////////////////////////////////////

	template< typename T>
	void WrapIntegerWithSomeNoise( const T& data, TDynArray< Uint8 >& output, Uint32 randomBytesCount )
	{
		CRC4Generator generator( "Mh*bS^Z#x1", 10 );

		// Add random lead in
		for( Uint32 i = 0; i < randomBytesCount; ++i )
		{
			output.PushBack( m_randomNumberGenerator.Get< Uint8 >() ^ generator.NextByte() );
		}

		// Encrypt data
		for( Uint32 i = 0; i < sizeof( T ); ++i )
		{
			Uint8 b = ( data >> ( i * 8 ) ) & 0xFF;
			output.PushBack( b ^ generator.NextByte() );
		}

		// Add random lead out
		for( Uint32 i = 0; i < randomBytesCount; ++i )
		{
			output.PushBack( m_randomNumberGenerator.Get< Uint8 >() ^ generator.NextByte() );
		}
	}

	template< typename T>
	static void UnwrapIntegerFromSomeNoise( T& output, const TDynArray< Uint8 >& input, Uint32 randomBytesCount )
	{
		CRC4Generator generator( "Mh*bS^Z#x1", 10 );

		// Skip random lead in
		for( Uint32 i = 0; i < randomBytesCount; ++i )
		{
			generator.NextByte();
		}

		// Decrypt data
		output = 0;
		for( Uint32 i = 0; i < sizeof( T ); ++i )
		{
			Uint8 b = input[ i + randomBytesCount ] ^ generator.NextByte();
			output |= b << ( i * 8 );
		}
	}

};

/// Game session manager
typedef TSingleton< CGameSessionManager > SGameSessionManager;
