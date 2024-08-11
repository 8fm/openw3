/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __DEPOT_ENTRY_H__
#define __DEPOT_ENTRY_H__

#include "changelist.h"
#include "fileSys.h"
#include "scopedPtr.h"
#include "hashmap.h"
#include "handleMap.h"
#include "bundleMetadataStore.h"

class CDirectory;
class CResource;
class CThumbnail;
struct SThumbnailSettings;
class CDiskPackage;
class CFileLoadingStats;
class ResourceLoadingContext;
class ResourceLoadingTask;
class IAsyncFile;

namespace DebugWindows
{
	class CDebugWindowFileLoadingStats;
}

/// Info about resource being reloaded
class CReloadFileInfo
{
public:
	CResource*	m_oldResource;
	CResource*	m_newResource;
	String		m_editor;

public:
	CReloadFileInfo( CResource* oldResource, CResource* newResource, const String& editor )
		: m_oldResource( oldResource )
		, m_newResource( newResource )
		, m_editor( editor )
	{};
};

/// Resource loading priorities
enum EResourceLoadingPriority
{
	eResourceLoadingPriority_Low,
	eResourceLoadingPriority_Normal,
	eResourceLoadingPriority_High,
};

enum ESaveReadonlyFileBehaviour
{
	eOverwrite,
	eCancel,
	eAsk
};

/// Loading context for resource
class ResourceLoadingContext
{
public:
	Bool							m_isAsyncLoader;			//!< Resource is being loaded inside an async loader
	Bool							m_isImportDependency;		//!< We are loading an import from other resource
	Bool							m_isPrefetchFile;			//!< We are prefetching the file.
	CFileLoadingStats*				m_stats;					//!< Loading stats
	IFile*							m_customFileStream;			//!< IFile pointer to custom IFile data location
	EResourceLoadingPriority		m_priority;					//!< Resource loading priority - do not abuse the high setting
	class IDependencyImportLoader*	m_importLoader;				//!< Custom import loader

public:
	ResourceLoadingContext();
};

/// VC state of file
enum EDiskFileState
{
	DFS_Local,
	DFS_Added,
	DFS_CheckedIn,
	DFS_NotSynced,
	DFS_CheckedOut,
	DFS_Deleted,
};

/// General result stuff for async reader
enum EAsyncReaderResult
{
	eAsyncReaderResult_OK,
	eAsyncReaderResult_NotReady,
	eAsyncReaderResult_Failed,
};

enum EOpenStatus
{
	OS_OpenedOnlyByMe,
	OS_OpenedBySomeoneElse,
	OS_NotOpen,
	OS_Unknown
};

#ifdef ENABLE_RESOURCE_MONITORING

/// Resource loading/unloading statistics
class ResourceMonitorStats
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Depot );

public:
	enum EEventType
	{
		eEventType_None,
		eEventType_Loaded,
		eEventType_Unloaded,
		eEventType_Expelled,
		eEventType_Revived,
	};

	Uint32					m_frameLoaded;			// last frame index (from engine)
	Double					m_timeLoaded;			// last absolute loading time
	Uint32					m_loadCount;			// number of times resource was loaded

	Uint32					m_frameUnloaded;		// last frame index (from engine)
	Double					m_timeUnloaded;			// last absolute unload time
	Uint32					m_unloadCount;			// number of times resource was loaded

	Uint32					m_frameExpelled;		// last frame index (from engine)
	Double					m_timeExpelled;			// last absolute expel time
	Uint32					m_expellCount;			// number of times resource was expelled

	Uint32					m_frameRevived;			// last frame index (from engine)
	Double					m_timeRevived;			// last absolute revive time
	Uint32					m_reviveCount;			// number of times resource was revived

	Float					m_loadTime;				// time it took to load the resource (last loading)
	Float					m_worstLoadTime;		// worst load time so far
	Bool					m_hadImports;			// did we load any imports during resource loading (indicates broken burst loading)

	Float					m_postLoadTime;			// time it took to post load the resource (last loading)
	Float					m_worstPostLoadTime;	// worst post load time so far
	Bool					m_hadPostLoadImports;	// did we load any other resources during the post load ? (indicates total fuckup)

	void*					m_userPointer;			// user pointer data link (debug windows, use with extreeme care)
	ResourceMonitorStats*	m_chainNext;			// internal chain pointer, use with care

	Uint8					m_isLoaded:1;			// reflected disk file flag
	Uint8					m_isQuarantined:1;		// reflected disk file flag
	Uint8					m_isMissing:1;			// resource failed to load at least once
	Uint8					m_isSyncLoaded:1;		// was it loaded from main thread (last load only)

	RED_INLINE const class CDiskFile* GetFile() const { return m_file; }
	RED_INLINE const EEventType GetLastEvent() const { return m_lastEvent; }

	ResourceMonitorStats( const class CDiskFile* file );
	~ResourceMonitorStats();

	// reset state of the monitor object, user pointer, flags and chain link are NOT reset
	// monitor data is removed from event list
	void Reset(); 

	// report even type (will bump object to the front of the event list)
	void Report( const EEventType eventType );

	// extract event list into an array (thread safe)
	static void CollectLastEvents( TDynArray< ResourceMonitorStats* >& outList, const Uint32 maxEvents, Uint32& outLastEventMarker );

	static void EnableMonitoring( Bool enable );
	static Bool IsEnabled();

private:
	static Red::Threads::CMutex		st_lock;
	static Uint32					st_eventMarker;
	static ResourceMonitorStats*	st_events;
	static Bool						st_enableMonitoring;

	void LinkToEventList();
	void UnlinkFromEventList();

	ResourceMonitorStats*	m_eventNext;			// event list, next event (older)
	ResourceMonitorStats**	m_eventPrev;			// previous event pointer
	EEventType				m_lastEvent;			// last event type	

	const CDiskFile*		m_file;
};

#endif		//!ENABLE_RESOURCE_MONITORING

/// Base depot class
class IDepotObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Depot );

public:
	virtual ~IDepotObject() {};
};

/// Physical file location info
typedef Uint32 TPhysicalFileID;
typedef TSortedMap< TPhysicalFileID, Uint32 > TPhysicalFileMap;

/// Generalized disk file location
class SDiskFileLocation
{
public:
	SDiskFileLocation( const TPhysicalFileID fileID, const Uint32 offset );
	SDiskFileLocation( const SDiskFileLocation& other );
	RED_INLINE ~SDiskFileLocation() {};

	RED_INLINE const TPhysicalFileID GetPhysicalFileID() const { return m_fileID; }
	RED_INLINE const Uint32 GetOffsetInFile() const { return m_offset; }

	RED_INLINE const Bool operator<( const SDiskFileLocation& a ) const
	{
		if ( m_fileID < a.m_fileID ) return true;
		if ( m_fileID > a.m_fileID ) return false;
		return m_offset < a.m_offset;
	}

private:
	TPhysicalFileID		m_fileID;		// Generalized physical FileID - unique
	Uint32				m_offset;		// Offset in the file
};

/// List of object to post load
class SDiskFilePostLoadList
{
public:
	SDiskFilePostLoadList();

	// Add object to the list
	void AddObject( ISerializable* object );

	// Call post load event
	void CallEvents() const;

private:
	TDynArray< THandle< ISerializable > >	m_objects;
};

/****************************************/
/* Disk file ( serialized resource )	*/
/****************************************/
class CDiskFile : public IDepotObject, public Red::System::NonCopyable
{
	friend class CResource;
	friend class CDirectory;
	friend class DebugWindows::CDebugWindowFileLoadingStats;

public:
	CDiskFile( CDirectory* directory, const String& fileName, CResource* existingResoruce = nullptr );
	virtual ~CDiskFile();

protected:
	CDiskFile(); // For Unit Test only
	CDiskFile( const CDiskFile& ); // Not copyable

public:
	// Accessors
	RED_INLINE CDirectory* GetDirectory				() const { return m_directory; }
	RED_INLINE CResource* GetResource				() const { return m_resource; }
	RED_INLINE const Uint32 GetDepCacheIndex		() const { return m_depCacheIndex; }
	RED_INLINE const String& GetFileName			() const { return m_fileName; }
	RED_INLINE Bool IsLoaded						() const { return (m_resource != NULL) && !m_isLoading; }
	RED_INLINE Bool IsFailed						() const { return m_isFailed; }
	RED_INLINE Bool IsLoading						() const { return m_isLoading; }
	RED_INLINE Bool IsQuarantined					() const { return m_isQuarantined; }
	RED_INLINE Bool IsBreakOnLoadSet				() const { return m_isBreakOnLoad; }
	RED_INLINE Bool WasLoadedAtLeastOnce			() const { return m_wasLoadedAtLeastOnce; }
	RED_INLINE Bool IsOverriden						() const { return m_isOverriden; }
	RED_INLINE Bool IsRuntimeAttached				() const { return m_isRuntimeAttached; }
	RED_INLINE void EverybodyDeservesASecondChance  () { m_isFailed = false; }

#ifdef ENABLE_RESOURCE_MONITORING
	RED_INLINE ResourceMonitorStats* GetMonitorData() const { return m_monitorData; }
#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

	RED_INLINE TDynArray<CThumbnail*> GetThumbnails	() const { return m_thumbnails; }
	RED_INLINE Bool IsModified						() const { return m_isModified; }
	RED_INLINE Bool IsSaved							() const { return m_isSaved; }
	RED_INLINE Bool IsEdited						() const { return m_state == DFS_CheckedOut || m_state == DFS_Added || m_state == DFS_Deleted; }
	RED_INLINE Bool IsCheckedOut					() const { return m_state == DFS_CheckedOut; }
	RED_INLINE Bool IsCheckedIn						() const { return m_state == DFS_CheckedIn; }
	RED_INLINE Bool IsLocal							() const { return m_state == DFS_Local; }
	RED_INLINE Bool IsDeleted						() const { return m_state == DFS_Deleted; }
	RED_INLINE Bool IsAdded							() const { return m_state == DFS_Added; }
	RED_INLINE Bool IsNotSynced						() const { return m_state == DFS_NotSynced; }

	RED_INLINE void SetForcedOverwriteFlag( ESaveReadonlyFileBehaviour forceOverwrite ) { m_forceOverwrite = forceOverwrite; }

	RED_INLINE void SetChangelist( const SChangelist& changelist ) { m_changelist = changelist; }
	RED_INLINE const SChangelist& GetChangelist() const { return m_changelist; }

#ifndef NO_EDITOR
	Bool IsResourceMergeable						() const;
#endif

	// state setters
	void SetDeleted();
	void SetAdded();
	void SetLocal();
	void SetCheckedIn();
	void SetCheckedOut();
	void SetNotSynced();

#else

	RED_INLINE Bool IsModified() const { return false; }

#endif

	// Get RTTI class for the resource inside this file (NOTE: does not have to load the file)
	const CClass* GetResourceClass() const;

	// Load resource associated with this file from disk, more options
	THandle< CResource > Load( const ResourceLoadingContext& context = ResourceLoadingContext() );

	// Unload file
	void Unload();

	//! Get file time
	virtual Red::System::DateTime GetFileTime() const;

	// Create reader for this disk file
	virtual IFile* CreateReader() const;

	// Create writer for this disk file
	virtual IFile* CreateWriter( const String& path ) const;

	// Create a request to load file's data into memory, can fail. In such cases you need to go the normal way.
	virtual EAsyncReaderResult CreateAsyncReader( const Uint8 ioTag, IAsyncFile*& outAsyncReader ) const;

	//! Get absolute path of file
	RED_MOCKABLE String GetAbsolutePath() const;

	//! Get depot path of file
	RED_MOCKABLE String GetDepotPath() const;

	//! Is this a loose file (no proxy)
	virtual const Bool IsLooseFile() const;

	//! Get the bundle FileID for a given buffer in this file, returns 0 if not found
	virtual Red::Core::Bundle::FileID GetBufferFileID( const Uint32 bufferIndex ) const;

	//! Get physical file IDs
	virtual void GetPhysicalFileIDs( TPhysicalFileMap& outLocations ) const;

	//! Get generalized file location information - helps to sort files for access speed
	virtual void GetPhysicalFileLocations( TDynArray< SDiskFileLocation >& outLocations ) const;
	
	//! Get single preferred physical location for file
	virtual void GetPhysicalFileLocation( SDiskFileLocation& outLocation, const TPhysicalFileMap& preferredLocations ) const;

	// Bump the internal frame counter
	static void NextFrame();

	// Batch loader - load list of files, it will pack the IO queries together
	// This is the fastest way to load unassociated files in the engine, try to use it over a normal loop of CDiskFile::Load
	static void LoadBatch( CDiskFile** files, const Uint32 numFiles, EResourceLoadingPriority priority, TDynArray< THandle< CResource > >& outLoadedResources );

	// Debug - request to break next time this file is loaded
	void SetBreakOnLoad( const Bool shouldBreakOnLoad );

	// Mods - mark this file as overridden
	void MarkAsOverridden();

	// DLC - mark this file as runtime attached
	void MarkAsRuntimeAttached();

	// Rebind disk file to new resource
	void Rebind( CResource* newResource );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

public:
	// Unmodify, remove modified flag from this disk file
	void Unmodify();

	// Mark file as modified, usually called because resource was modified in editor, returns false if file cannot be changed
	Bool MarkModified();

	// Save resource associated with this file to disk
	Bool Save();

	// Save resource associated with this file to given place on disk
	Bool Save( const String& path );

	// Sets write rights for a read only file
	Bool Overwrite();

	// Delete resource
	Bool Delete( Bool versionControlConfirm = true, Bool autoSubmit = true );

	// Create copy (if the file with the same name already exists in the target directory, it will be overwritten)
	Bool Copy( CDirectory* directory, const String& fileName, Bool forceOverwrite = false );

	// Prepare file for edition
	Bool Edit();

	// Verify if file is still checked out or not
	void GetStatus();

	// Inform whether file was checked out by someone else
	EOpenStatus GetOpenStatus();

	// Checks out the file from the depot
	Bool CheckOut( Bool exclusive = true );

	// Checks out the file from the depot without asking the user
	Bool SilentCheckOut( Bool exclusive = true );
	
	// Submits file to the depot
	Bool Submit();

	// Submits file to the depot with description
	Bool Submit( const String& description );

	// Reverts file
	Bool Revert( Bool silent = false );

	// Adds local file to the version control
	RED_MOCKABLE Bool Add();

	// Locks file in version control
	Bool Lock();

	// Sync file to newest revision
	Bool Sync();

	// Check if file still exists in the file system; if not, remove it from the memory
	Bool VerifyExistence();

	// Rename the file
	Bool Rename( const String& filename, const String& extension );

public:
	// Cache saved data for resource, used in editor to support unloading of dirty layers
	// This will check if the file is up to date (same timestamp and NOT modified), if so it returns false
	// If the copy is made this function returns true and the name of the cached file
	Bool CacheSaveData( String& outCachedFilePath );

	// Remove the cached temporary file, does nothing if file was not cached
	void RemoveCacheSaveData();

	// Load thumbnail only
	Bool LoadThumbnail();

	// Save thumbnail only
	Bool SaveThumbnail( IFile* file );

	// Create thumbnail(s) from resource
	Bool CreateThumbnail( TDynArray<CThumbnail*>& thumbnails, const SThumbnailSettings* settings = nullptr );

	// Update ( recreate ) thumbnail
	Bool UpdateThumbnail( const SThumbnailSettings* settings = nullptr );

	//! Remove resource thumbnail
	void RemoveThumbnail();

protected:
	// Reload file
	void Reload();

#endif

private:		
		// quarantine list (not thread safe, it's assumed that the caller is)
		class Quarantine
		{
		private:
			typedef THashMap< CDiskFile*, Uint32 >		TTokens; // TODO: a good set implementation would be more useful here
			TTokens			m_tokens;

			Red::Threads::CMutex	m_lock;
			Uint32					m_isLocked;

		public:
			Quarantine();

			void LockQuarantine( const Uint32 frameCutoff, TDynArray< CDiskFile* >& outQuarantinedObjects );
			void UnlockQuarantine();

			void Add(CDiskFile* file, const Uint32 currentFrameIndex);
			void Remove(CDiskFile* file);
		};

		static Quarantine			st_quarantine;
		static Uint32				st_frameIndex;

		// generic stuff
		CDirectory*				m_directory;			// Directory this file is in
		CResource*				m_resource;				// Loaded resource
		String					m_fileName;				// File name with extension
#ifdef ENABLE_RESOURCE_MONITORING
		ResourceMonitorStats*	m_monitorData;			// Data for resource monitor
#endif
		Uint32					m_depCacheIndex;		// Index of this file in the dependency cache

		// generic file flags, usually not thread safe unless accessed withing file mutex
		Bool					m_wasLoadedAtLeastOnce:1;	// File was loaded at least once, never reset
		Bool					m_isQuarantined:1;			// File is in the quarantine
		Bool					m_isLoading:1;				// File is being loaded right now
		Bool					m_isFailed:1;				// File failed to load (will not be retried)
		Bool					m_isBreakOnLoad:1;			// We will break (only if debugger is attached) when loading this file
		Bool					m_isOverriden:1;			// mods only: file was overriden from external source, DO NOT ALLOW TO LOAD INLINED FILES
		Bool					m_isRuntimeAttached:1;		// dlc only: file is from runtime DLC (or a patched file that overrode a runtime DLC file)


		// thread sync stuff - needed to prevent two threads from loading the same file (0 if not loading)
		Red::Threads::CAtomic< Uint32 >		m_loadingThreadID;

		// thread sync stuff - lock on the resource
		Red::Threads::CLightMutex	m_lock;

		// source control crap
		// TODO: move to CEditorDiskFile or something....
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		Uint8					m_noThumbnail:1;		// No thumbnail data
		Uint8					m_isModified:1;			// True if object was modified since loading ( in editor )
		Uint8					m_isSaved:1;			// True if file was ever saved ( false for newly created resources )
		Uint8					m_dataCachedInFile:1;	// Saved resource state, used in editor to prevent unloading of modified resources
		EDiskFileState			m_state;				// State of the file in respect of version control
		SChangelist				m_changelist;			// File changelist
		TDynArray<CThumbnail*>	m_thumbnails;			// Resource thumbnails
		String 					m_cacheFilename;		// Temporary file name
		String					m_diskFilename;			// Our Perforce server is case sensitive. For fstat to work, it need correct case.
		
		ESaveReadonlyFileBehaviour	m_forceOverwrite;	// Force overwiting files with read-only attribute in this directory
#endif

		//--------------

		// resource monitor reporting tools
		void ReportLoaded() const;
		void ReportUnloaded() const;
		void ReportExpelled() const;
		void ReportRevived() const;
		void ReportLoadingEvent( const Float timeTaken, const Bool hadImports ) const;
		void ReportPostLoadEvent( const Float timeTaken, const Bool hadOtherLoads ) const;
		void ReportFailedLoad() const;

		// Request a file loading task for this file, can return NULL if file cannot be loaded or is already loaded/loading
		Bool BeingLoading();

		// internal loading callbacks
		void InternalFailedLoading();
		void InternalDeserialize( IFile* file, class IDependencyImportLoader* dependencyLoader, SDiskFilePostLoadList& postLoadList );
		void InternalPostLoad( const SDiskFilePostLoadList& postLoadList );

		// Bind disk file to resource ( use with care )
		// Direct access to this method is not for faint hearted
		void Bind( CResource* resource );

		// Unbind resource from disk file, it's assumed that the resource was unloaded/destroyed.
		// Direct access to this method is not for faint hearted
		void Unbind( CResource* owningResource );

		// Move the resource to the quarantine
		// This will put a file in a list of resources that can be potentially freed if we run low on memory
		// This is a thread safe call
		void MoveToQuarantine();

		// Remove file from quarantine
		// This will remove file from a list of resource that can be freed
		// This is a thread safe call
		void RemoveFromQuarantine();

		// Purge the object!
		void Purge();

		friend class CResource;
		friend class ResourceLoadingTask;
		friend class CResourceLoader;
		friend class CResourceUnloader;
		friend class CDiskBundlePreloader;
		friend class CDependencyLoader;
};

/*********************************************/
/* Disk file mapping entry in a bundle file  */
/*********************************************/
class CBundleDiskFile : public CDiskFile
{
public:
	CBundleDiskFile( CDirectory* directory, const String& fileName, const Red::Core::Bundle::FileID fileID );
	virtual ~CBundleDiskFile();

	// file interface
	virtual IFile* CreateReader() const override;
	virtual IFile* CreateWriter( const String& path ) const override;
	virtual EAsyncReaderResult CreateAsyncReader( const Uint8 ioTag, IAsyncFile*& outAsyncReader ) const override;
	virtual Red::System::DateTime GetFileTime() const override;
	virtual const Bool IsLooseFile() const override { return false; }
	virtual Red::Core::Bundle::FileID GetBufferFileID( const Uint32 bufferIndex ) const override;
	virtual void GetPhysicalFileLocations( TDynArray< SDiskFileLocation >& outLocations ) const override;
	virtual void GetPhysicalFileLocation( SDiskFileLocation& outLocation, const TPhysicalFileMap& preferredLocations ) const override;
	virtual void GetPhysicalFileIDs( TPhysicalFileMap& outLocations ) const override;

	// get the bundle file ID - debug mostly
	RED_FORCE_INLINE const Red::Core::Bundle::FileID GetFileID() const { return m_fileID; }

private:
	Red::Core::Bundle::FileID		m_fileID;
};

#endif // __DEPOT_ENTRY_H__
