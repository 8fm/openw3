/**
* Copyright ? 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Maximum number of blocks
#define MAX_ROOT_BLOCKS			32
#define MAX_SAVE_BLOCKS			32

#include "../core/gameSave.h"
#include "gameSaveFile.h"
#include "../engine/localizedContent.h"
#include "../core/namesReporter.h"
#include "../core/memoryFileWriter.h"

enum ESaveGameType : CEnum::TValueType 
{
	SGT_None				= 0,	// Nothing
	SGT_AutoSave			= 1,	// Save initiated by the game, according to autosave rules
	SGT_QuickSave			= 2,	// Save initiated by the user, by pressing a hotkey
	SGT_Manual				= 3,	// Save initiated by the user, using ingame menu
	SGT_ForcedCheckPoint	= 4,	// Save initiated by the quest, forced to ignore the save locks
	SGT_CheckPoint			= 5,	// Save initiated by the quest, but with respect to the save locks
};

BEGIN_ENUM_RTTI( ESaveGameType )
	ENUM_OPTION( SGT_AutoSave )
	ENUM_OPTION( SGT_QuickSave )
	ENUM_OPTION( SGT_Manual )
	ENUM_OPTION( SGT_ForcedCheckPoint )
	ENUM_OPTION( SGT_CheckPoint )
END_ENUM_RTTI()

// NOTE: this magic values are for DEBUG only.
// They are here so we can validate the save game structure.

#define SAVE_FILE_MAGIC					'YVAS'		// SAVY
#define SAVE_FILE_MAGIC_PATCHED			'3VAS'		// SAV3
#define SAVE_BLOCK_MAGIC				'KCLB'		// BLCK
#define SAVE_VALUE_MAGIC				'LAVA'		// AVAL
#define SAVE_PROPERTY_MAGIC				'PROP'		// PORP
#define SAVE_STORAGE_START_MAGIC		'STOR'		// ROTS
#define SAVE_STORAGE_END_MAGIC			'ROTS'		// STOR
#define SAVE_GD_STORAGE_MAGIC			'PAXS'		// SXAP
#define SAVE_NEW_FORMAT_HEADER			'HFNS'		// SNFH

// Magic number added at the end of save file
#define SAVE_END_FILE_MAGIC			'PUCK'

enum ELoaderCreationResult
{
	LOADER_Success,
	LOADER_WrongVersion,
};

// Game version
enum
{
	GAME_VERSION_UNKNOWN				= 0,
	GAME_VERSION_WITCHER_2				= 1,
	GAME_VERSION_WITCHER_3_CERT			= 2,
	GAME_VERSION_WITCHER_3_PATCH_1_01	= 3,
	GAME_VERSION_WITCHER_3_PATCH_1_02	= 4,
	GAME_VERSION_WITCHER_3_PATCH_1_03	= 5,
	GAME_VERSION_WITCHER_3_PATCH_1_04	= 6,
	GAME_VERSION_WITCHER_3_PATCH_1_05	= 7,
	GAME_VERSION_WITCHER_3_PATCH_1_06	= 8,
	GAME_VERSION_WITCHER_3_PATCH_1_07	= 9,
	GAME_VERSION_WITCHER_3_PATCH_1_08	= 10,
	GAME_VERSION_WITCHER_3_PATCH_1_10	= 11,
	GAME_VERSION_WITCHER_3_PATCH_1_11	= 12,
	GAME_VERSION_WITCHER_3_PATCH_1_12	= 13,
	GAME_VERSION_WITCHER_3_PATCH_1_20	= 14,
	GAME_VERSION_WITCHER_3_PATCH_1_21	= 15,
	GAME_VERSION_WITCHER_3_PATCH_1_22	= 16,
	GAME_VERSION_WITCHER_3_PATCH_1_23	= 17,
	GAME_VERSION_WITCHER_3_PATCH_1_31	= 18,
	GAME_VERSION_WITCHER_3_PATCH_1_32	= 19,
	GAME_VERSION_WITCHER_3_PATCH_1_50	= 20,
	GAME_VERSION_WITCHER_3_PATCH_1_60	= 21,
};

// current game version
#define SAVE_GAME_VERSION		GAME_VERSION_WITCHER_3_PATCH_1_60

// Save internal version list
enum
{
    // Initial version of saves
    SAVE_VERSION_INITIAL = 1,

    // Magic value at end
    SAVE_VERSION_MAGIC_AT_END,

    // Introduced CNames instead of Strings in many places  
    SAVE_VERSION_CNAMES,

    // Saves are memory aligned
    SAVE_VERSION_ALIGNED,

    // Saved games contains CName hashes instead of full CNames
    SAVE_CNAME_HASHES,

    // Saved games contains no save magic at every single value and every single block 
    SAVE_NO_VALUE_AND_BLOCK_MAGIC,

    // Community doesn't contain current AP in sequence
    SAVE_NO_AP_SEQUENCE,

	// Auto play effect is saved when it is different than the template value
	SAVE_AUTO_EFFECT_NAME,

	// Gameplay systems unified inder IGameSystem interface; Changed the order of initialization and tick
	SAVE_I_GAMESYSTEM,

	// Introduced value map within savegame blocks to allow for random order value reads
	SAVE_VERSION_VALUE_OFFSETS_MAP,

	// Saving all worlds, not just the current one
	SAVE_VERSION_UNIVERSE,

	// [DEX] Serialized object data was saved in file version >= VER_CLASS_PROPERTIES_DATA_CLEANUP
	SAVE_SERIALIZED_OBJECTS_NEW_VERSION,

	// Support for transfering objects between worlds
	SAVE_VERSION_TRANSFERABLE_ENTITIES,

	// Savegame includes player state
	SAVE_VERSION_INCLUDE_PLAYER_STATE,

	// Fixed journal serialization
	SAVE_VERSION_JOURNAL,

	// Removed value offsets from blocks (caused performance problems)
	SAVE_VERSION_NO_VALUE_OFFSETS_MAP_AGAIN,

	// Serializing cnames as hashes where it is safe to do so, without having to cook all names
	SAVE_VERSION_USE_HASH_NAMES_WHERE_SAFE,

	// Optimized properties serialization in saved games
	SAVE_VERSION_WRITEPROPERTY,

	// Changed how the layer groups are being saved
	SAVE_VERSION_LAYERGROUPS,

	// Added separate storage for layers visibility info
	SAVE_VERSION_LAYERS_VISIBILITY_STORAGE,

	// Added saving of quest condition properties
	SAVE_VERSION_QUEST_CONDITION_PROPERTIES,

	// Game timers now have persistent IDs
	SAVE_VERSION_TIMERS_WITH_IDS,

	// Community agent stubs savegames only use CName to save action point id
	SAVE_COMMUNITY_AP_ID_BY_NAME,

	// Quest video to show when restoring from a savegame
	SAVE_QUEST_LOADING_VIDEO,

	// Yet another change to CName serialization...
	SAVE_VERSION_KEEP_CNAMES_AS_LIST,

	// Optimized inventories saving
	SAVE_VERSION_DIRECT_STREAM_SAVES,

	// ...and another change to CName serialization...
	SAVE_VERSION_CNAMES_REMAPPER,

	// saving autoplay effect name after it changed for the first time...
	SAVE_VERSION_STORE_AUTOPLAY_EFFECT_NAME_ALWAYS,

	// saving 'Quest' tag on itams as a single bit flag
	SAVE_VERSION_STORE_QUEST_TAG_ON_ITEMS_AS_A_FLAG,

	// Skip-block offsets are all written in one block at the end of the file.
	SAVE_VERSION_STORE_SKIPBLOCKS_AT_END,

	// saving information about category in inventory item (only if mounted flag is set)
	SAVE_VERSION_STORE_CATEGORY_ITEM_INFO,

	// saving loading screen name
	SAVE_VERSION_LOADING_SCREEN_NAME,

	// whether content manager cheats were enabled and required content is suspect
	SAVE_VERSION_CONTENT_TAINTED_FLAGS,

	// small optimization in questThread.h
	SAVE_VERSION_STORE_INPUT_NAMES_DIRECTLY,

	// put activated content in .sav instead of just loose .req file
	SAVE_VERSION_ACTIVATED_CONTENT_IN_SAVE,

	// optimized attitude manager saving
	SAVE_VERSION_OPTIMIZED_ATTITUDE_MANAGER,

    // boat's docking and avoidance subsystems removed, hedgehog system added instead
    SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL,

	// optimized character stats saving
	SAVE_VERSION_OPTIMIZED_CHARACTERSTATS_SAVING,

	// got rid of cname hashes
	SAVE_VERSION_GOT_RID_OF_HASHES,

	// optimized blocks saving
	SAVE_VERSION_OPTIMIZED_BLOCKS_SAVING,

	// optimized journal saving
	SAVE_VERSION_OPTIMIZED_JOURNAL_SAVING,

	// community agent stubs keep active world hash within the save
	SAVE_VERSION_STUB_KEEPS_WORLD_HASH,

	// boat saving fallback
	SAVE_VERSION_BOAT_SAVING_FALLBACK,

	// saving CActor's m_alive 
	SAVE_VERSION_ACTOR_ALIVE_SAVING,

	// saving i_spawnLimit (CSpawnTreeInitializerSpawnLimitMonitor)
	SAVE_VERSION_SPAWN_LIMIT_SAVING,

	// saving cached boats in common map manager
	SAVE_VERSION_MAP_MANAGER_BOATS,

	// put activated content in .sav instead of just loose .req file - DLC also...
	SAVE_VERSION_ACTIVATED_DLC_CONTENT_IN_SAVE,

	// do not load telemetry session id form older saves (session id can be corrupted)
	SAVE_VERSION_TELEMETRY_NEW_SAVE,

	// new game plus feature added
	SAVE_VERSION_NEW_GAME_PLUS,

	// some items may have 32-bit quantity field instead of 16-bit
	SAVE_VERSION_32_BIT_QUANTITY_ITEMS,

	// saving i_numCreaturesToSpawn and i_recalculateTimeout in CBaseCreatureEntry
	SAVE_VERSION_NUM_CREATURES_TO_SPAWN_SAVING,

	// map manager optimization
	SAVE_VERSION_MAP_MANAGER_OPTIMIZATION,

	// On PC, check if game is kosher and whether achievements should be enabled
	SAVE_VERSION_KOSHER_CHECK,

	// Because the first part was a great success
	SAVE_VERSION_KOSHER_CHECK_PART_DEUX,

	// inventory item stores number of slots
	SAVE_VERSION_STORE_NUM_INVENTORY_ITEM_SLOTS,

	// inventory item stores enchantments
	SAVE_VERSION_STORE_ENCHANTMENTS,

	// items have 64-bit flags field
	SAVE_VERSION_ITEMS_FLAGS_64,

	// items have dye color
	SAVE_VERSION_ITEMS_DYE_COLOR,

	// multiple user pins
	SAVE_VERSION_MULTIPLE_USER_PINS,

	// multiple user pins
	SAVE_VERSION_EVEN_MORE_MULTIPLE_USER_PINS,

	// multiple user pins
	SAVE_VERSION_NEW_MAP_FILTERS,

	// map border data for safe teleports after a load
	SAVE_VERSION_MAP_BORDER_DATA,

	// save factd db as a stream
	SAVE_VERSION_STREAM_FACTS_DB,

	// added a hackfix for dead quest signals
	SAVE_VERSION_DEAD_PHASE_HACKFIX,

    // Save converted from ( SAVE_VERSION_INITIAL || SAVE_VERSION_MAGIC_AT_END || SAVE_VERSION_CNAMES ) to SAVE_VERSION_ALIGNED
    SAVE_VERSION_PREALIGNED_CONVERTED = 1000,

    // Current save version
    SAVE_VERSION = SAVE_VERSION_DEAD_PHASE_HACKFIX
};

///////////////////////////////////////////////////////////////////////////////////////////

class IGameSaveDumper
{
public:
	virtual ~IGameSaveDumper() {};
	virtual void OnBlockStart( const Uint32 offset, const Uint32 blockSize, const CName blockName ) = 0;
	virtual void OnBlockEnd( const Uint32 offset ) = 0;
	virtual void OnStorageStart( const Uint32 offset, const Uint32 size ) = 0;
	virtual void OnStorageEnd( const Uint32 offset ) = 0;
	virtual void OnValue( const Uint32 offset, const Bool isProp, const CName name, const IRTTIType* type, const Uint32 dataSize, const void* dataPtr ) = 0;
	virtual void OnError( const Uint32 offset, const AnsiChar* txt, ... ) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_SAVE_IMPORT
	class IObsoleteSaveImporter
	{
	public:
		virtual const TDynArray< String >& GetSearchPaths() const = 0;
		virtual void GetSaveFiles( TDynArray< SSavegameInfo >& files ) const = 0;
		virtual Bool ImportSave( const SSavegameInfo& file ) const = 0;
	};
#endif // NO_SAVE_IMPORT

///////////////////////////////////////////////////////////////////////////////////////////

/// Game save manager
class CGameSaveManager
{
public:
	CGameSaveManager();

	//! Create game stream loader based on data in file
	IGameLoader* CreateLoader( const SSavegameInfo& info, ELoaderCreationResult& res );
																	   
	//! Create game stream loader based on debug data buffer
	IGameLoader* CreateDebugLoader( IFile* debugData );

	//! Create game stream loader based on data in storage
	static IGameLoader* CreateLoader( IGameDataStorage* gameDataStorage, ISaveFile** directStreamAccess, Uint32* version );

	//! Create file based game stream writer 
	IGameSaver* CreateSaver( const SSavegameInfo& info );

	//! Create data storage based game stream writer
	static IGameSaver* CreateSaver( IGameDataStorage* gameDataStorage, ISaveFile** directStreamAccess );

	//! Dump content of save game block
	static void DumpContent( IGameDataStorage* data, IGameSaveDumper* dumper );

private:
	IGameLoader* CreateLegacyLoader( const SSavegameInfo& info, IFile* sourceFile, Uint32 saveVersion, ELoaderCreationResult& res );

	IGameLoader* CreateNewLoader( const SSavegameInfo& info, IFile* sourceFile, Uint32 saveVersion, ELoaderCreationResult& res );

	Bool VerifySaveVersion( Uint32 gameVersion, Uint32 saveVersion, Uint32 serializationVersion ) const;

	IGameSaver* CreateNewSaver( const SSavegameInfo& info );
};

/// Game save manager singleton
typedef TSingleton< CGameSaveManager > SGameSaveManager;

////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef W2_PLATFORM_WIN32
	class CSaveGameDumpHelper;
#endif

struct CGameFileValueOffset
{
	CName	m_name;
	Uint32	m_offset;

	friend IFile& operator<<( IFile& file, CGameFileValueOffset &offset )
	{
		return file << offset.m_name << offset.m_offset;
	}

	struct CmpPred
	{
		RED_INLINE Bool operator () (const CGameFileValueOffset& a, const CGameFileValueOffset& b) const
		{
			return a.m_name < b.m_name;
		}
	};
};

/// Saves game state to file
class CGameStorageSaver : public IGameSaver
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

protected:
	struct SaveBlock
	{
		CName					m_name;			//!< Name of the block
		Uint32					m_offset;		//!< Where is the skip offset saved	(or just offset to block if this is a root block)
	};

protected:
	CGameStorageWriter*		m_file;
	Uint32						m_numBlocks;
	SaveBlock					m_blocks[ MAX_SAVE_BLOCKS ];

public:
	CGameStorageSaver( CGameStorageWriter* theFile );
	~CGameStorageSaver();

	virtual void BeginBlock( CName name );

	virtual void EndBlock( CName name );

	virtual void WriteRawAnsiValue( CName name, const AnsiChar* data, Uint32 size );

	virtual void WriteValue( CName name, IRTTIType* type, const void* data );

	virtual void WriteProperty( void* object, CProperty* prop );

	virtual void AddStorageStream( IGameDataStorage* storageStream );

	virtual void Close();

	virtual void Finalize();

	virtual const void* GetData() const;

	virtual Uint32 GetDataSize() const;
	
	virtual Uint32 GetDataCapacity() const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CGameStorageLoader : public IGameLoader
{
	friend class CSaveGameDumpHelper;

protected:
	struct SaveBlock
	{
		CName					m_name;				//!< Name of the block
		Uint32					m_offset;			//!< Offset of the end of the block
		Bool					m_isValid;			//!< Has this block been successfully loaded?
	};

	typedef TDynArray< SaveBlock, MC_Gameplay > TSaveBlockArray;

protected:
	CGameStorageReader*		m_file;
	TSaveBlockArray				m_blocks;
	SaveBlock					m_rootBlocks[ MAX_ROOT_BLOCKS ];
	Uint32						m_saveVersion;
	Uint32						m_gameVersion;
	Uint32						m_fileVersion;
	Bool						m_hasRootBlocks;

public:
	CGameStorageLoader( CGameStorageReader* file, Uint32 saveVersion, Uint32 gameVersion, Uint32 fileVersion, Bool hasRootBlocks );
	~CGameStorageLoader();

	Uint32 GetGameVersion() const;

	Uint32 GetSaveVersion() const;

	void BeginBlock( CName blockName );

	void EndBlock( CName blockName );

	void ReadValue( CName name, IRTTIType* type, const void* data, CObject* defaultParent );

	void ReadProperty( void* object, CClass* theClass, CObject* defaultParent );

	virtual IGameDataStorage* ExtractDataStorage();

	virtual void SkipDataStorage();
};

///////////////////////////////////////////////////////////////////////////////

class CGameDataExternalStorage : public IGameDataStorage
{
protected:
	TDynArray< Uint8 >*	m_data;

public:
	//! Get data
	RED_INLINE TDynArray< Uint8 >& GetData() { return *m_data; }

	RED_INLINE void SetDataPtr( TDynArray< Uint8 >* ptr ) { m_data = ptr; }

public:
	//! Get data size
	virtual Uint32 GetSize() const
	{
		return m_data->Size();
	}

	//! Get data size
	virtual const void* GetData() const
	{
		return m_data->Data();
	}

	//! Create memory writer
	virtual CGameStorageWriter* CreateWriter();

	//! Create memory reader
	virtual ISaveFile* CreateReader() const;

	//! Reserve memory for storage
	virtual void Reserve( Uint32 bytes ) override
	{
		m_data->Reserve( bytes );
	}
};

////////////////////////////////////////////////////////////////////////////////////////////

struct SSavegameInfo
{
	DECLARE_RTTI_STRUCT( SSavegameInfo );

	Bool					m_w2import : 1;
	Bool					m_customFilename : 1;

	// Sorry to add this hacky bit. Suppressing loading video very contextual instance data, not for serialization of any kind!
	Bool					m_suppressVideo : 1;

	ESaveGameType			m_slotType;
	Int32					m_slotIndex;

	Uint32					m_displayNameIndex;
	Red::System::DateTime	m_timeStamp;
	String					m_filename;

	RED_INLINE SSavegameInfo()
		: m_slotIndex( -1 )
		, m_slotType( SGT_None )
		, m_w2import( false )
		, m_customFilename( false )
		, m_suppressVideo( false )
	{
	}

	RED_INLINE void Clear()
	{
		m_filename.Clear();
		m_displayNameIndex = 0;
		m_timeStamp.Clear();
		m_slotIndex = -1;
		m_slotType = SGT_None;
		m_w2import = false;
		m_suppressVideo = false;
	}

	static const Char* GetSaveExtension() { return TXT(".sav"); }
	static const Char* GetScreenshotExtension() { return TXT(".png"); }
	static const Char* GetRequiredContentExtension() { return TXT(".req"); }

	RED_INLINE Bool IsValid() const { return false == m_filename.Empty() && m_timeStamp.GetDateRaw() != 0 && m_slotType != SGT_None; }
	RED_INLINE Bool IsW2Import() const { return m_w2import; }
	RED_INLINE const String& GetFileName() const { return m_filename; }
	RED_INLINE Int32 GetSlotIndex() const { return m_slotIndex; }
	RED_INLINE Bool IsAutoSave() const { return m_slotType == SGT_AutoSave; }
	RED_INLINE Bool IsQuickSave() const { return m_slotType == SGT_QuickSave; }
	RED_INLINE Bool IsManualSave() const { return m_slotType == SGT_Manual; }
	RED_INLINE Bool IsQMSave() const { return IsQuickSave() || IsManualSave(); }
	RED_INLINE Bool IsCheckPoint() const { return m_slotType == SGT_CheckPoint || m_slotType == SGT_ForcedCheckPoint; }
	RED_INLINE Bool IsACPSave() const { return IsAutoSave() || IsCheckPoint(); }
	RED_INLINE Bool IsCustomFilename() const { return m_customFilename; }
	RED_INLINE Bool IsSuppressVideo() const { return m_suppressVideo; }
	
	String GetDisplayName() const;
	Bool IsDisplayNameAvailable() const;

	struct ComparePredicate
	{
		inline bool operator() ( const SSavegameInfo& a, const SSavegameInfo& b ) const
		{
			return a.m_timeStamp > b.m_timeStamp;
		}
	};
};

BEGIN_CLASS_RTTI( SSavegameInfo )
	PROPERTY( m_filename )
	PROPERTY( m_slotType )
	PROPERTY( m_slotIndex )
END_CLASS_RTTI()
