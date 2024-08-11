/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "diskFile.h"
#include "depot.h"
#include "depotBundles.h"
#include "dependencyLinkerFactory.h"
#include "resource.h"
#include "resourceLoading.h"
#include "namesPool.h"
#include "namesRegistry.h"
#include "events.h"
#include "profiler.h"
#include "dependencyLoader.h"
#include "versionControl.h"
#include "feedback.h"
#include "filePath.h"
#include "engineTime.h"
#include "dependencySaver.h"
#include "directory.h"
#include "thumbnail.h"
#include "objectRootSet.h"
#include "bufferedTempFile.h"
#include "object.h"
#include "objectGC.h"
#include "asyncFileAccess.h"

#include "fileSystemProfilerWrapper.h"

#if !defined( RED_FINAL_BUILD ) && defined( RED_PLATFORM_WINPC )
# include "../../common/redIO/redIO.h"
#endif

// Uncomment this to record file loads - note that you may also want to define FULL_DETERMINISM
// in the game/editor settings header file to record callstacks from jobs, unless you also
// uncomment RECORD_MAINTHREADS_LOADS_ONLY
//
//#define RECORD_LOADS
//#define RECORD_MAINTHREADS_LOADS_ONLY
#define RECORD_LOADS_FILENAME "f:\\loads.txt"
//

#ifdef RECORD_LOADS

static Red::Threads::CMutex GrabTheStackMutex;
static RED_TLS Int32 LoadRecordDepth = 0;

static void GrabTheStack( Red::System::Error::Callstack& stack )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > mutex( GrabTheStackMutex );
	Red::System::Error::Handler::GetInstance()->GetCallstack( stack );
}

static Uint64 AccurateTime()
{
	LARGE_INTEGER counter, frequency;
	::QueryPerformanceCounter( &counter );
	::QueryPerformanceFrequency( &frequency );
	return (Uint64)counter.QuadPart * (Uint64)1000000 / (Uint64)frequency.QuadPart;
}

static void RecordLoad( const String& path )
{
#ifdef RECORD_MAINTHREADS_LOADS_ONLY
	if ( !::SIsMainThread() ) return;
#endif

	Red::System::Error::Callstack stack;
	GrabTheStack( stack );

	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > mutex( GrabTheStackMutex );

		FILE* f = fopen( RECORD_LOADS_FILENAME, "at" );
		if (!f) f = fopen( RECORD_LOADS_FILENAME, "wt" );
		if (f) {

			fprintf(f, "F%s\n", UNICODE_TO_ANSI( path.AsChar() ) );
			fprintf(f, "D%i\n", LoadRecordDepth );
			fprintf(f, "T%llu\n", (unsigned long long)AccurateTime() );
#ifndef RECORD_MAINTHREADS_LOADS_ONLY
			fprintf(f, "t%i\n", ::SIsMainThread() ? 0 : (int)Red::System::Internal::ThreadId::CurrentThread().id);
#endif

			for ( Uint32 i=3; i<stack.numFrames; ++i )
			{
				fprintf(f, "C%s\n", UNICODE_TO_ANSI( stack.frame[i].file ) );
				fprintf(f, "S%s\n", UNICODE_TO_ANSI( stack.frame[i].symbol ) );
				fprintf(f, "L%d\n", stack.frame[i].line );
			}

			fflush(f);
			fclose(f);
		}
	}
}

static void RecordExtraLoad( char ch, const String& data )
{
#ifdef RECORD_MAINTHREADS_LOADS_ONLY
	if ( !::SIsMainThread() ) return;
#endif

	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > mutex( GrabTheStackMutex );

		FILE* f = fopen( RECORD_LOADS_FILENAME, "at" );
		if (!f) f = fopen( RECORD_LOADS_FILENAME, "wt" );
		if (f) {

			fprintf(f, "%c%s\n", ch, UNICODE_TO_ANSI( data.AsChar() ) );
			fflush(f);
			fclose(f);
		}
	}
}

#endif

#ifdef ENABLE_RESOURCE_MONITORING
//----

Red::Threads::CMutex	ResourceMonitorStats::st_lock;
ResourceMonitorStats*	ResourceMonitorStats::st_events = nullptr;
Uint32					ResourceMonitorStats::st_eventMarker = 0;
Bool					ResourceMonitorStats::st_enableMonitoring = false;

ResourceMonitorStats::ResourceMonitorStats( const CDiskFile* file )
	: m_file( file )
	, m_userPointer( nullptr )
	, m_chainNext( nullptr )
	, m_isLoaded( 0 )
	, m_isQuarantined( 0 )
	, m_isMissing( 0 )
	, m_isSyncLoaded( 0 )
	, m_eventNext( nullptr )
	, m_eventPrev( nullptr )
	, m_lastEvent( eEventType_None )
{
	Reset();
}

ResourceMonitorStats::~ResourceMonitorStats()
{
	if ( m_lastEvent != eEventType_None )
	{
		UnlinkFromEventList();
	}
}

void ResourceMonitorStats::EnableMonitoring( Bool enable )
{
	st_enableMonitoring = enable;
}

Bool ResourceMonitorStats::IsEnabled()
{
	return st_enableMonitoring;
}

void ResourceMonitorStats::Report( const EEventType eventType )
{
	if ( m_lastEvent != eventType )
	{
		if ( m_lastEvent != eEventType_None )
			UnlinkFromEventList();

		m_lastEvent = eventType;

		LinkToEventList();
	}
}

void ResourceMonitorStats::LinkToEventList()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );

	RED_FATAL_ASSERT( m_eventNext == nullptr, "File event list corruption" );
	RED_FATAL_ASSERT( m_eventPrev == nullptr, "File event list corruption" );

	if ( st_events ) st_events->m_eventPrev = &m_eventNext;
	m_eventPrev = &st_events;
	m_eventNext = st_events;
	st_events = this;

	st_eventMarker += 1;
}

void ResourceMonitorStats::UnlinkFromEventList()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );
	
	RED_FATAL_ASSERT( m_eventPrev != nullptr, "File event list corruption" );

	if ( m_eventNext ) m_eventNext->m_eventPrev = m_eventPrev;
	*m_eventPrev = m_eventNext;
	m_eventPrev = nullptr;
	m_eventNext = nullptr;

	m_lastEvent = eEventType_None;
}

void ResourceMonitorStats::CollectLastEvents( TDynArray< ResourceMonitorStats* >& outList, const Uint32 maxEvents, Uint32& outLastEventMarker )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );
	
	outList.Reserve( maxEvents );
	outLastEventMarker = st_eventMarker;

	for ( ResourceMonitorStats* info = st_events; info; info = info->m_eventNext )
	{
		if ( outList.Size() == maxEvents )
			break;

		outList.PushBack( info );
	}
}

void ResourceMonitorStats::Reset()
{
	m_frameLoaded = 0;
	m_timeLoaded = 0.0;
	m_loadCount = 0;

	m_frameUnloaded = 0;
	m_timeUnloaded = 0.0;
	m_unloadCount = 0;

	m_frameExpelled = 0;
	m_timeExpelled = 0.0;
	m_expellCount = 0;

	m_frameRevived = 0;
	m_timeRevived = 0.0;
	m_reviveCount = 0;

	m_loadTime = 0.0f;
	m_worstLoadTime = 0.0f;
	m_hadImports = false;

	m_postLoadTime = 0.0f;
	m_worstPostLoadTime = 0.0f;
	m_hadPostLoadImports = false;

	m_isSyncLoaded = 0;
}

#endif

static Bool IsNameUpperCase( const String& fileName )
{
	for ( Uint32 i = 0; i < fileName.GetLength(); ++i )
	{
		Char ch = fileName[i];
		if ( ch >= 'A' && ch <= 'Z' )
		{
			return true;
		}
	}

	return false;
}

//-----------------

SDiskFileLocation::SDiskFileLocation( const TPhysicalFileID fileID, const Uint32 offset )
	: m_fileID( fileID )
	, m_offset( offset )
{
}

SDiskFileLocation::SDiskFileLocation( const SDiskFileLocation& other )
	: m_fileID( other.m_fileID )
	, m_offset( other.m_offset )
{
}

//-----------------

SDiskFilePostLoadList::SDiskFilePostLoadList()
{
}

void SDiskFilePostLoadList::AddObject( ISerializable* object )
{
	RED_FATAL_ASSERT( object != nullptr, "Invalid object" );
	m_objects.PushBack( object );
}

void SDiskFilePostLoadList::CallEvents() const
{
	for ( THandle< ISerializable > object : m_objects )
	{
		if ( object )
			object->OnPostLoad();
	}
}

//-----------------

CDiskFile::Quarantine CDiskFile::st_quarantine;
Uint32 CDiskFile::st_frameIndex = 1000;

CDiskFile::CDiskFile()
	:	m_resource( NULL )
	,	m_isLoading( false )
	,	m_wasLoadedAtLeastOnce( false )
	,	m_depCacheIndex( 0 )
	,	m_loadingThreadID( 0 )
	,	m_isBreakOnLoad( 0 )
	,	m_isOverriden( 0 )
	,	m_isRuntimeAttached( 0 )
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	,	m_isModified( false )
	,	m_isSaved( true )
	,	m_noThumbnail( false )
	,	m_state( DFS_CheckedIn )
	,	m_dataCachedInFile ( false )
	,	m_changelist( SChangelist::DEFAULT )
	, 	m_forceOverwrite( eAsk )
#endif
#ifdef ENABLE_RESOURCE_MONITORING
	,	m_monitorData( nullptr )
#endif
{}

CDiskFile::CDiskFile( CDirectory* directory, const String& fileName, CResource* existingResoruce /*= nullptr*/ )
:	m_directory( directory )
,	m_fileName( fileName )
,	m_resource( NULL )
,	m_isLoading( false )
,	m_wasLoadedAtLeastOnce( false )
,	m_depCacheIndex( 0 )
,	m_loadingThreadID( 0 )
,	m_isBreakOnLoad( 0 )
,	m_isOverriden( 0 )
,	m_isRuntimeAttached( 0 )
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
,	m_isModified( false )
,	m_isSaved( true )
,	m_noThumbnail( false )
,	m_state( DFS_CheckedIn )
,	m_dataCachedInFile ( false )
,	m_changelist( SChangelist::DEFAULT )
,	m_diskFilename( fileName )
,	m_forceOverwrite( eAsk )
#endif
#ifdef ENABLE_RESOURCE_MONITORING
,	m_monitorData( nullptr )
#endif
{
	// Keep lower case version of file name
	if ( IsNameUpperCase( fileName ) )
	{
		m_fileName = fileName.ToLower();
	}

#ifdef ENABLE_RESOURCE_MONITORING
	// Create stats object
	if( ResourceMonitorStats::IsEnabled() )
	{
		m_monitorData = new ResourceMonitorStats( this );
	}
#endif

	// Bind file with resource
	if ( existingResoruce )
	{
		RED_FATAL_ASSERT( existingResoruce->GetFile() == nullptr, "Specified resource is already bound" );
		Bind( existingResoruce );
	}
}

CDiskFile::~CDiskFile()
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	// Remove thumbnail
	RemoveThumbnail();

	RemoveCacheSaveData();	
#endif

	// unmap from dependency cache (if mapped)
	if ( m_depCacheIndex )
	{
		GDepot->UnmapFileFromDependencyCache( this );
		m_depCacheIndex = 0;
	}

#ifdef ENABLE_RESOURCE_MONITORING
	// delete internal monitor data (stats)
	if ( m_monitorData )
	{
		delete m_monitorData;
		m_monitorData = nullptr;
	}
#endif
}


void CDiskFile::NextFrame()
{
	++st_frameIndex;
}

void CDiskFile::LoadBatch( CDiskFile** files, const Uint32 numFiles, EResourceLoadingPriority priority, TDynArray< THandle< CResource > >& outLoadedResources )
{
	PC_SCOPE_PIX( LoadBatch );

	// load stuff
	SResourceLoader::GetInstance().Load( nullptr, files, numFiles, priority, nullptr );

	// extract resources
	outLoadedResources.Reserve( outLoadedResources.Size() + numFiles );
	for ( Uint32 i=0; i<numFiles; ++i )
	{
		if ( files[i]->m_resource != nullptr )
		{
			outLoadedResources.PushBack( files[i]->m_resource );
		}
	}
}

void CDiskFile::Rebind( CResource* newResource )
{
	if ( m_resource != newResource )
	{
		if( m_resource )
		{
			Unbind( m_resource );
		}
		
		if ( newResource )
		{
			Bind( newResource );
		}
	}
}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::Unmodify()
{
	m_isModified = false;
}

Bool CDiskFile::MarkModified()
{
	// Ask for permission
	if ( !m_isModified )
	{
		if ( !Edit() )
		{
			return false;
		}
	}

	// Mark as modified
	m_isModified = true;

	// Dispatch system wide event
	EDITOR_QUEUE_EVENT( CNAME( FileModified ), CreateEventData< String >( GetDepotPath() ) );

	return true;
}

#endif

void CDiskFile::Bind( CResource* resource )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	RED_FATAL_ASSERT( !m_isLoading, "Bind() called during resource loading" );

	// rebind file<->resource references
	if ( resource != m_resource )
	{
		// Unbind current resource
		if ( m_resource )
			Unbind( m_resource );

		// Set new resource
		m_resource = resource;

		// Bind with file
		if ( m_resource )
		{
			RED_FATAL_ASSERT( m_resource->m_file == nullptr, "Resource is already bound to a disk file" );
			m_resource->m_file = this;

			// Binding between CResource and CDiskFile happens only during loading, report this event as such
			ReportLoaded();

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
			// Reset modified flag if we bound a new resource
			if ( !m_dataCachedInFile )
			{
				m_isModified = false;
			}
#endif
		}
	}
}

void CDiskFile::Unbind( CResource* owningResource )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > lock( m_lock );

	// May happen that the file was not bounded (W3 ModTools hack)
	RED_ASSERT( owningResource == m_resource, TXT("Trying to unbind disk file with mismatched resource. Don't try to cheat here.") );
	if ( !m_resource || owningResource != m_resource )
		return;

	// Fatal checks
	RED_FATAL_ASSERT( !m_isLoading, "Resource is being unbounded while it's being loaded at the same time" );
	RED_UNUSED( owningResource );

	// Remove attached file from quarantine (we are deleting it anyway)
	RemoveFromQuarantine();

	// Unbind file side
	RED_ASSERT( m_resource->m_file == this );	
	m_resource->m_file = nullptr;
	m_resource = nullptr;

	// Report to system
	ReportUnloaded();
}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::CacheSaveData( String& outCachedFilePath )
{
	const String depotPath = GetDepotPath();
	const Uint64 fileHash = Red::CalculateHash64( UNICODE_TO_ANSI( depotPath.AsChar() ) );

	// already cached
	if ( m_dataCachedInFile )
	{
		outCachedFilePath = m_cacheFilename;
		return true;
	}

	// Determine the target file path, use the depot file hash name for the temp file
	String cachedFilePath = GFileManager->GetTempDirectory();
	cachedFilePath += TXT("pie\\temp"); // HACK - right now we use this system only for PIE
	cachedFilePath += ToString( fileHash );
	cachedFilePath += TXT("_");
	cachedFilePath += ToString( GetFileTime().GetTimeRaw() );
	cachedFilePath += TXT(".tmp");

	// If we are not modified and we already have a cached file that matches our name and timestamp than don't bother resaving the file
	if ( !IsModified() && GFileManager->FileExist( cachedFilePath ) )
	{
		// cached file is up to date
		return false;
	}

	// Create the writer
	IFile* writer = GFileManager->CreateFileWriter( cachedFilePath, FOF_AbsolutePath | FOF_Buffered );
	if ( writer )
	{
		// Create saver
		IDependencySaver* saver = SDependencyLinkerFactory::GetInstance().GetSaver( this, *writer );
		if ( saver )
		{
			// Save object
			DependencySavingContext context( m_resource );
			if ( saver->SaveObjects( context ) )
			{
				// Data saved
				m_dataCachedInFile = true;
				m_cacheFilename = cachedFilePath;

				// Show info
				LOG_CORE( TXT("File '%ls' cached in '%ls'"), GetDepotPath().AsChar(), cachedFilePath.AsChar() );
			}

			// Close saver
			delete saver;
		}

		// Close file
		delete writer;
	}

	// output path
	if ( m_dataCachedInFile )
		m_cacheFilename = cachedFilePath;

	return m_dataCachedInFile;
}

void CDiskFile::RemoveCacheSaveData()
{
	// Remove cached file
	if ( m_dataCachedInFile )
	{
		//GSystemIO.DeleteFile( m_cacheFilename.AsChar() ); keep the file
		m_cacheFilename = String::EMPTY;
		m_dataCachedInFile = false;
	}
}

#endif

CDiskFile::Quarantine::Quarantine()
	: m_isLocked( false )
{
	m_tokens.Reserve( 20 * 1024 );
}

void CDiskFile::Quarantine::LockQuarantine( const Uint32 frameCutoff, TDynArray< CDiskFile* >& outQuarantinedObjects )
{
	if ( !m_isLocked )
	{
		m_isLocked = true;
		m_lock.Acquire();
	}

	outQuarantinedObjects.ClearFast();

	// collect entries matching frame cutoff
	for ( auto it = m_tokens.Begin(); it != m_tokens.End(); ++it )
	{
		if ( (*it).m_second <= frameCutoff )
		{
			outQuarantinedObjects.PushBack( (*it).m_first );
		}
	}

	// remove the collected files from quarantine list
	for ( CDiskFile* file : outQuarantinedObjects )
	{
		file->m_isQuarantined = false; // reset the flag
		m_tokens.Erase( file );
	}
}

void CDiskFile::Quarantine::UnlockQuarantine()
{
	RED_FATAL_ASSERT( m_isLocked, "Disk file purgatory lock was released to many times" );

	if ( m_isLocked )
	{
		m_lock.Release();
		m_isLocked = false;
	}
}

void CDiskFile::Quarantine::Add(CDiskFile* file, const Uint32 currentFrameIndex)
{
	RED_ASSERT(file);

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

#ifndef RED_FINAL_BUILD
	if (m_tokens.FindPtr(file) != nullptr)
	{
		RED_FATAL( "Disk file '%ls' is already in the quarantine",
			file->GetDepotPath().AsChar() );

		return;
	}
#endif

	// we store the quarantined file and the time at which it was quarantined
	m_tokens.Set(file, currentFrameIndex);
}

void CDiskFile::Quarantine::Remove(CDiskFile* file)
{
	RED_ASSERT(file);

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

#ifndef RED_FINAL_BUILD
	if (m_tokens.FindPtr(file) == nullptr)
	{
		RED_FATAL( "Disk file '%ls' is not in the quarantine",
			file->GetDepotPath().AsChar() );

		return;
	}
#endif

	m_tokens.Erase(file);
}

void CDiskFile::MoveToQuarantine()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

	// File is being loaded (on the same thread, total bullshit situation :()
	if ( m_isLoading )
	{
		RED_FATAL( "Disk file '%ls' is being loaded and we try to add it to a quarantine",
			GetDepotPath().AsChar() );
		return;
	}

	// Should not be called twice
	if ( m_isQuarantined )
	{
		RED_FATAL( "Disk file '%ls' is already in quarantine", 
			GetDepotPath().AsChar() );
		return;
	}

	// TODO: it would be nice to have some memory estimate for the resources
	const Uint32 memoryUsage = 0;
	st_quarantine.Add( this, memoryUsage );
	m_isQuarantined = true;

	// Report event to the monitor system
	ReportExpelled();
}

void CDiskFile::RemoveFromQuarantine()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

	// Not in quarantine
	// It's safe to call it multiple times
	if ( !m_isQuarantined )
		return;

	// Remove from internal quarantine list
	st_quarantine.Remove(this);
	m_isQuarantined = false;

	// Revive the handle to this object
	m_resource->DisableHandleProtection();
	m_resource->EnableReferenceCounting( true );

	// Report event to the monitor system
	ReportRevived();
}

class CStackMessage_PurgeResource : public Red::Error::StackMessage
{
private:
	const CDiskFile*		m_file;

public:
	CStackMessage_PurgeResource( const CDiskFile* file )
		: m_file( file )
	{
	}

	virtual void Report( Char* outText, Uint32 outTextLength )
	{
		Red::System::SNPrintF( outText, outTextLength, TXT("Purge '%ls'"), m_file->GetDepotPath().AsChar() );
	}
};

void CDiskFile::Purge()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

	// This should never be called while resource is being loaded
	if ( m_isLoading )
	{
		RED_FATAL( "Disk file '%ls' is being loaded and we try to purge it at the same time. Badness.",
			GetDepotPath().AsChar() );
		return;
	}

	// Sanity checks
	RED_FATAL_ASSERT( m_resource != nullptr, "Trying to purge object with no resource. Why was it in the purge list ?" );
	RED_FATAL_ASSERT( m_resource->m_file == this, "Resource is not bounded properly." );
	RED_FATAL_ASSERT( !m_isQuarantined, "Trying to purge object that is still in the purge list." );

	// DISCARD THE RESOURCE
	{
		CStackMessage_PurgeResource marker( this );
		m_resource->Discard();
	}

	// We should be unbounded by now
	RED_FATAL_ASSERT( m_resource == nullptr, "Resource was not unbounded properly after it was purged." );
}

void CDiskFile::Unload()
{
	// Remove from quarantine list
	RemoveFromQuarantine();

	{
		Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

		// This should never be called while resource is being loaded
		if ( m_isLoading )
		{
			RED_FATAL( "Disk file '%ls' is being loaded and we try to unload it at the same time. Badness.",
				GetDepotPath().AsChar() );
			return;
		}

		// Save copy of resource when unloading modified resource
		if ( m_resource )
		{
			CResource* res = m_resource;

			res->UnbindFile();
			RED_FATAL_ASSERT( m_resource == nullptr, "Resource was not unbounded properly after it was unloaded." );

			res->Discard();
		}
	}

	// Reset resource pointer
	m_resource = nullptr;
}

class CUnknownResource : public CResource
{
	DECLARE_ENGINE_CLASS( CUnknownResource, CResource, 0 );

public:
	Uint8		m_dummy[ 64*1024 ];

public:
	CUnknownResource() {};

	virtual const Char* GetExtension() const { return TXT("dupa"); }
#ifndef NO_EDITOR
	virtual const Char* GetFriendlyDescription() const { return TXT("dupa"); }
#endif
};

BEGIN_CLASS_RTTI( CUnknownResource );
	PARENT_CLASS( CResource );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CUnknownResource );

namespace DiskFileHelpers
{
	class ResourceClassExtractor
	{
	public:
		ResourceClassExtractor()
		{
			BuildClassMap();
		}

		const CClass* GetResourceClassForFile( const CDiskFile* diskFile );

		static ResourceClassExtractor& GetInstance()
		{
			static ResourceClassExtractor theInstance;
			return theInstance;
		}

	private:
		void BuildClassMap();

		const CClass* ExtractClassOfRootExport( const CDiskFile* file );

		typedef THashMap< String, const CClass*, DefaultHashFunc< String >, DefaultEqualFunc< String >, MC_RTTI > TResourceClasses;
		TResourceClasses	m_classes; // only classes that have unique extension->CClass mapping

		typedef THashMap< const CDiskFile*, const CClass*, DefaultHashFunc< const CDiskFile* >, DefaultEqualFunc< const CDiskFile* >, MC_RTTI > TFileClasses; // only dynamic files
		TFileClasses		m_resolvedFiles;

		Red::Threads::CMutex	m_lock;
	};

	const CClass* ResourceClassExtractor::GetResourceClassForFile( const CDiskFile* diskFile )
	{
		const String fileExtension = StringHelpers::GetFileExtension( diskFile->GetFileName() );

		// find in map
		const CClass* existingResourceClass = nullptr;
		m_classes.Find( fileExtension, existingResourceClass );
		if ( existingResourceClass )
			return existingResourceClass; // known class

		// HACK: dynamic resolve for redapex in normal game is to costly, use a hack
		if ( GFileManager->IsReadOnly() && fileExtension == TXT("redapex") )
			return ClassID< CUnknownResource >();

		// generic resolve
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		// find by disk file
		const CClass* existingDiskFileClass = nullptr;
		m_resolvedFiles.Find( diskFile, existingDiskFileClass );
		if ( existingDiskFileClass )
			return existingDiskFileClass; // disk file was already seen

		// create file reader
		Red::TScopedPtr<IFile> reader( diskFile->CreateReader() );
		if ( reader )
		{
			CDependencyLoader loader( *reader.Get(), diskFile );
			existingDiskFileClass = loader.GetRootClass();

			// store in map for the future
			if ( !existingDiskFileClass )
			{
				ERR_CORE( TXT("Dynamic class resolve for resource '%ls' failed!'"), 
					diskFile->GetDepotPath().AsChar() );
			}
			else if ( existingDiskFileClass->IsAbstract() )
			{
				ERR_CORE( TXT("Dynamic class resolve for resource '%ls' resulted in abstract class: '%ls'. Resource will not be loaded."), 
					diskFile->GetDepotPath().AsChar(), 
					existingDiskFileClass->GetName().AsChar() );

				existingDiskFileClass = nullptr;
			}
			else
			{
				WARN_CORE( TXT("Dynamic class resolve for resource '%ls': '%ls'"), 
					diskFile->GetDepotPath().AsChar(), 
					existingDiskFileClass->GetName().AsChar() );
			}

			m_resolvedFiles.Insert( diskFile, existingDiskFileClass );
		}

		// return extracted class
		return existingDiskFileClass;
	}

	void ResourceClassExtractor::BuildClassMap()
	{
		TDynArray< CClass* > resourceClasses;
		SRTTI::GetInstance().EnumClasses( ClassID< CResource >(), resourceClasses );

		for ( CClass* resourceClass : resourceClasses )
		{
			// get the default object
			CResource* resource = resourceClass->GetDefaultObject< CResource >();
			RED_FATAL_ASSERT( resource != nullptr, "Invalid resource class '%ls'", resourceClass->GetName().AsChar() );

			// get the extension
			const Char* extension = resource->GetExtension();
			RED_FATAL_ASSERT( extension != nullptr && extension[0] != 0, "Invalid resource '%ls' extension", resourceClass->GetName().AsChar() );

#ifdef NO_EDITOR
			// In cooked builds we ASSUME that all the worlds are CGameWorlds - this saves us problems later on dynamic checking if the class is what we think it should be
			// This reduces memory spikes by up to 30 MB on loading the worlds
			if ( (0 == Red::StringCompare( extension, TXT("w2w") )) && resourceClass->GetName() != TXT("CGameWorld") )
				continue;
#endif

			// make sure extension is not duplicated
			const CClass* existingResourceClass = nullptr;
			if ( m_classes.Find( extension, existingResourceClass ) )
			{
				if ( existingResourceClass )
				{
					WARN_CORE( TXT( "Resource extension '%ls' is already used by class '%ls'. Trying to reuse it for class '%ls'" ),
						extension, existingResourceClass->GetName().AsChar(), resourceClass->GetName().AsChar() );
				}

				// inconclusive class
				LOG_CORE( TXT("Class '%ls', inconclusive"), resourceClass->GetName().AsChar() );
				m_classes.Set( extension, nullptr );
				continue;
			}

			// register in the map
			//LOG_CORE( TXT("Class '%ls', ext = '%ls'"), resourceClass->GetName().AsChar(), extension );
			m_classes.Insert( extension, resourceClass );
		}

		// TODO: remove the terrain hack!
		m_classes.Insert( TXT("w2ter"), SRTTI::GetInstance().FindClass( CName( TXT("CTerrainTile") ) ) );
	}
}

const CClass* CDiskFile::GetResourceClass() const
{
	return DiskFileHelpers::ResourceClassExtractor::GetInstance().GetResourceClassForFile( this );
}

void CDiskFile::SetBreakOnLoad( const Bool shouldBreakOnLoad )
{
	m_isBreakOnLoad = shouldBreakOnLoad;
}

void CDiskFile::MarkAsOverridden()
{
	m_isOverriden = 1;
}

void CDiskFile::MarkAsRuntimeAttached()
{
	m_isRuntimeAttached = 1;
}

Bool CDiskFile::BeingLoading()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

	// Resource failed loading, will never be valid again
	if ( m_isFailed )
		return false;

	// We are loading the file and at the same time we got a request to load it again
	// While technically it's not illegal it's extremely touchy.
	// Usually it means two things - either the same file is requested from two different threads (unsafe)
	// or we got a recursive file dependency (ok)
	// In any case we are not creating a new loading task.
	if ( m_isLoading )
	{
		RED_FATAL_ASSERT( m_loadingThreadID.GetValue() != 0, "Loading flag is set but the loading therad is not specified" );

		// the same thread ? 
		const Uint32 threadId = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
		if ( m_loadingThreadID.GetValue() == threadId )
		{
			WARN_CORE( TXT( "Recursive loading request on file '%ls'. Returning incomplete resource - can crash!" ), GetDepotPath().AsChar() );
		}
		else
		{
			// two different threads are requesting the same resource to be loaded at the same time!!
			ERR_CORE( TXT( "!!! RESOURCE LOADING THREAD CONTENTION !!!") );
			ERR_CORE( TXT( "Resource '%ls' has a pending load task for thread %d yet another task was requested form thread %d"), 
				GetDepotPath().AsChar(), m_loadingThreadID.GetValue(), threadId );

			// release lock on the file state so the other thread can finish
			m_lock.Release();

			// active wait until the loading of this resource finishes
			// TODO: I have no idea how to solve this case in a non-blocking way
			CTimeCounter contentionTimer;
			while ( m_loadingThreadID.GetValue() != 0 )
			{
				Red::Threads::YieldCurrentThread();
			}

			// final stats
			ERR_CORE( TXT( "Contention lock on resource '%ls' release after %1.2fms"), 
				GetDepotPath().AsChar(), contentionTimer.GetTimePeriodMS() );

			// reacquire lock on the file state
			m_lock.Acquire();
		}

		return false; // do not load the file again
	}

	// Break on a load attempt
	if ( m_isBreakOnLoad )
	{
		m_isBreakOnLoad = false;
		LOG_CORE( TXT("!!! BREAK ON RESOURCE LOAD !!!: %ls"), GetDepotPath().AsChar() );

#ifdef RED_PLATFORM_WINPC
		DebugBreak();
#endif
	}

	// File is in quarantine, rescue it from it
	// Note: This is the only place in then engine where quarantined files can be safely revived
	if ( m_isQuarantined )
	{
		RED_FATAL_ASSERT( m_resource, "Resource is unloaded but still in quarantinte" );
		if ( m_resource )
		{
			// unlink this depot file from quarantine list
			RemoveFromQuarantine();
			RED_ASSERT( !m_isQuarantined );

			// revive the handle
			//m_resource->DisableHandleProtection();
			return false;
		}
		else
		{
			// reset flag - it was invalid
			m_isQuarantined = false;
		}
	}

	// Resource is already loaded
	if ( m_resource )
		return false;

	// Extract the resource class
	// For most of the resource the extension to class mapping is unique and we can do it in zero time
	// For some classes when two or more resources use the same extension the resolving is happening at the spot (SLOW!)
	const CClass* resourceClass = GetResourceClass();
	if ( !resourceClass )
	{
		ERR_CORE( TXT("Unable to resolve extension of file '%ls' to resource class. No resource will be loaded."), m_fileName.AsChar() );

#ifdef ENABLE_RESOURCE_MONITORING
		if ( m_monitorData )
			m_monitorData->m_isMissing = true;
#endif

		m_isFailed = true;
		return false; // we will not be able to load this file any way
	}

	// Make sure it's not an abstract class
	RED_FATAL_ASSERT( !resourceClass->IsAbstract(), "Cannot load resources that use abstract base class '%ls'", resourceClass->GetName().AsChar() );
	if ( resourceClass->IsAbstract() )
	{
		return false; // unable to load abstract resources
	}

	// Mark this resource as loading
	// It will be in this state until we have some result form the resource loading pipeline
	m_isLoading = true;
	m_loadingThreadID.SetValue( Red::System::Internal::ThreadId::CurrentThread().AsNumber() );

	// Preallocate resource object knowing it's class
	m_resource = resourceClass->CreateObject< CResource >();

	// Bind it as a resource pending load
	m_resource->m_isLoading = true;
	m_resource->m_file = this;

	// It's ok to start loading
	return true;
}

THandle< CResource > CDiskFile::Load( const ResourceLoadingContext& context )
{
	// make sure internal state is coherent
	RED_THREADS_MEMORY_BARRIER();

	// request a loading task description to be created for this disk file
	// it will not be created when there's not need or no possibility to load the file
	if ( BeingLoading() )
	{
		// load the resource with all of the dependencies
		// this function will wait until the resource is loaded
		CDiskFile* fileList = this;
		SResourceLoader::GetInstance().Load( this, &fileList, 1, context.m_priority, context.m_importLoader );

		// make sure the m_resource is written
		RED_THREADS_MEMORY_BARRIER();

		// in here we either succeeded with loading and the m_resource is valid
		// or we did not and the resource is NULL
		if ( m_resource && m_resource->IsLoading() )
		{
			ERR_CORE( TXT("Resource '%ls' is still loading!"), GetDepotPath().AsChar() );
		}
		return m_resource;
	}
	else
	{
		// make sure the m_resource is written
		RED_THREADS_MEMORY_BARRIER();

		// in here we either return valid resource or NULL
		RED_ASSERT( !m_resource || !m_resource->IsLoading(), TXT("Resource state is not stable: %ls"), m_resource->GetFriendlyName().AsChar() );
		return m_resource;
	}
}

void CDiskFile::InternalFailedLoading()
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

	// log
	ERR_CORE( TXT("!!! LOADING FAILED !!! ") );
	ERR_CORE( TXT("Unable to load '%ls' "), GetDepotPath().AsChar() );

	// sanity checks
	RED_FATAL_ASSERT( m_isLoading, "This function must be called inside the loading phase" );
	RED_FATAL_ASSERT( !m_isFailed , "File failed loading twice. Double load ?" );
	RED_FATAL_ASSERT( m_resource, "Trying to load a file without proper resource buffer" );

	// mark file as failed
	m_isLoading = false;
	m_isFailed = true;

	// unbind prebound file
	m_resource->m_file = nullptr;
	m_resource->m_isMissing = true;
	m_resource->m_isLoading = false;
	m_resource = nullptr;
	
	// delete the created resource stub
	delete m_resource;
	m_resource = nullptr;

	// unlock any pending thread contention
	m_loadingThreadID.SetValue( 0 );

	// reporting
	ReportFailedLoad();
}

void CDiskFile::InternalDeserialize( IFile* file, class IDependencyImportLoader* dependencyLoader, SDiskFilePostLoadList& postLoadList )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileDeserializeStart( GetDepotPath().AsChar() );
#endif

	CTimeCounter loadTimer; // time the full loading cycle

	// sanity checks
	RED_FATAL_ASSERT( m_isLoading, "This function must be called inside the loading phase" );
	RED_FATAL_ASSERT( !m_isFailed , "Trying to load failed file" );
	RED_FATAL_ASSERT( m_resource, "Trying to load a file without proper resource buffer" );

	{
		// Create loader
		Red::TScopedPtr< IDependencyLoader > loader( SDependencyLinkerFactory::GetInstance().GetLoader( this, *file ) );
		RED_FATAL_ASSERT( loader, "Failed to create dependency loader for file '%ls'", GetDepotPath().AsChar() );

		// Internal load counter - helps us with determining if we loaded something else
		static Uint32 st_loadCounter = 0;
		const Uint32 loadCounterAtStart = ++st_loadCounter;

		// Setup loader context
		DependencyLoadingContext loadingContext;
		loadingContext.m_firstExportMemory = m_resource;
		loadingContext.m_firstExportMemorySize = m_resource->GetClass()->GetSize(); // just for double checking
		loadingContext.m_parent = NULL; // resources from files are never parented
		loadingContext.m_validateHeader = true; // do header validation - at this stage we can safely handle missing assets
		loadingContext.m_getAllLoadedObjects = true;

		// Set custom dependency loader
		if ( dependencyLoader )
			loadingContext.m_importLoader = dependencyLoader;

		// Load data from file
		if ( !loader->LoadObjects( loadingContext ) )
		{
			// general failure
			ERR_CORE( TXT("Deserialization failure in file '%ls'"), 
				GetDepotPath().AsChar() );

			// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
			RedIOProfiler::ProfileDiskFileDeserializeEnd( GetDepotPath().AsChar() );
#endif
			InternalFailedLoading();
			return;
		}

		// Only one resource is supported per disk file
		if ( loadingContext.m_loadedResources.Size() != 1 )
		{
			ERR_CORE( TXT("Expected exactly one CResource object inside file '%ls'. Found %d."), 
				GetDepotPath().AsChar(), loadingContext.m_loadedResources.Size() );

			// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
			RedIOProfiler::ProfileDiskFileDeserializeEnd( GetDepotPath().AsChar() );
#endif
			InternalFailedLoading();
			return;
		}

		// Record loading time
		const Bool loadedImports = (loadCounterAtStart != st_loadCounter);
		ReportLoadingEvent( loadTimer.GetTimePeriod(), loadedImports );

		// Get object to post load
		for ( const auto& loadedObject : loadingContext.m_loadedObjects  )
		{
			ISerializable* serializable = loadedObject.GetSerializablePtr();
			if ( serializable )
				postLoadList.AddObject( serializable );
		}
	}

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileDeserializeEnd( GetDepotPath().AsChar() );
#endif
}

void CDiskFile::InternalPostLoad( const SDiskFilePostLoadList& postLoadList )
{
	Red::Threads::CScopedLock< Red::Threads::CLightMutex > loadingLock( m_lock );

	// sanity checks
	RED_FATAL_ASSERT( m_isLoading, "This function must be called inside the loading phase" );
	RED_FATAL_ASSERT( !m_isFailed , "Trying to load failed file" );
	RED_FATAL_ASSERT( m_resource, "Trying to load a file without proper resource buffer" );

	// Post load timed
	postLoadList.CallEvents();

	// Resource was loaded, reset the loading flag
	m_resource->m_isLoading = false;
	m_isLoading = false;

	// unlock any pending thread contention
	// resource is SAFE to access after this point
	m_loadingThreadID.SetValue( 0 );

	// Enable reference counting on this resource
	// NOTE: it's important for this to be outside the live scope of IDependencyLoader
	m_resource->EnableReferenceCounting();

	// Loaded!
	ReportLoaded();
}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Save()
{
	Bool ret = GVersionControl->Save( *this );
	if ( ret && m_resource )
	{
		m_resource->OnSave();
	}

	return ret;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Save( const String &path )
{
	// Resource must be valid
	if ( m_resource )
	{
		// See if destination is read only and ask user if he wants to remove this attribute
		if ( GFileManager->IsFileReadOnly( path ) )
		{
			if ( m_forceOverwrite == eOverwrite || ( m_forceOverwrite == eAsk && GFeedback->AskYesNo( TXT("File '%ls' has read-only attribute. Overwrite?"), path.AsChar() ) ) )
			{
				GFileManager->SetFileReadOnly( path, false );
			}
			else
			{
				// We cannot save to read only file
				return false;
			}
		}

		// hack to speed tree thumbnail generation
		if ( GetFileName().EndsWith( TXT("srt") ) )
		{
			const CFilePath filePath( GetAbsolutePath() );

			// hack for speed tree resources
			const String pathToThumbnail = filePath.GetPathString() + TXT("\\") + filePath.GetFileName() + TXT(".thmb");
			IFile* newFile = CreateWriter( pathToThumbnail );
			if( newFile )
			{
				SaveThumbnail(newFile);
				delete newFile;
			}

			return (newFile != nullptr);
		}

		// Open target file
		IFile* file = CreateWriter( path );
		if ( file )
		{
			// Allow resource to modify itself before saving, but only in editor
#ifndef NO_EDITOR_RESOURCE_SAVE
			if ( GIsEditor )
			{
				m_resource->OnResourceSavedInEditor();
			}
#endif

			// Saving can fail from many reasons - the main one is mismatched dependencies ( e.g. link to internal object in another resource )
			IDependencySaver* saver = SDependencyLinkerFactory::GetInstance().GetSaver( this, *file );

			// Save objects
			DependencySavingContext context( m_resource );
			const bool isSavingToTheSameFile = (Red::StringCompareNoCase( path.AsChar(), GetAbsolutePath().AsChar() ) == 0);
			context.m_isResourceResave = isSavingToTheSameFile;
			context.m_filterScriptedProps = true;
			if ( !saver->SaveObjects( context ) )
			{
				LOG_CORE( TXT("Dependency save failed for '%ls'"), GetDepotPath().AsChar() );
				delete file;
				return false;
			}

			// Save thumbnail
			SaveThumbnail(file);
			
			// Close saver
			delete saver;

			// Close target file
			delete file;

			// broadcast event
			EDITOR_DISPATCH_EVENT( CNAME( FileSaved ), CreateEventData< String >( GetDepotPath() ) );

			// Allow resource to update some shit
			m_resource->OnSave();

			// Mark as not modified
			Unmodify();
			return true;
		}
	}

	// Not saved
	return false;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Overwrite()
{
#ifndef RED_PLATFORM_CONSOLE
	return SetFileAttributes(GetAbsolutePath().AsChar(), FILE_ATTRIBUTE_NORMAL ) != 0;
#else
	return false;
#endif
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Delete( Bool versionControlConfirm, Bool autoSubmit )
{
	if ( IsAdded() == true )
	{
		if ( GVersionControl->Revert( *this , true ) == false )
		{
			return false;
		}
	}

	if ( IsLocal() == true )
	{
		// Remove the file read only flag
		String absolutePath = GetAbsolutePath();
		const Char *filePath = absolutePath.AsChar();
		Bool wasFileReadOnly = GSystemIO.IsFileReadOnly( filePath );
		GSystemIO.SetFileReadOnly( filePath, false );

		// Delete file localy
		if ( GSystemIO.DeleteFile( filePath ) == false )
		{
			GSystemIO.SetFileReadOnly( filePath, wasFileReadOnly );
			return false;
		}

		// remove this file from the directory files
		m_directory->DeleteFile( *this );

		// file was successfully deleted and can be removed from the memory
		SetDeleted();
		return true;
	}

	if ( IsCheckedOut() == true )
	{
		if ( GVersionControl->Revert( *this , true ) == false )
		{
			return false;
		}
	}

	// file must be deleted using source control
	if ( GVersionControl->Delete( *this, versionControlConfirm ) == true )
	{
		// remove the file entry from the directory
		m_directory->DeleteFile( *this );

		if ( autoSubmit )
		{
			return GVersionControl->Submit( *this );
		}
		else
		{
			return true;
		}
	}

	// not deleted for some reason
	return false;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::LoadThumbnail()
{
	// Already loaded
	if ( m_thumbnails.Size() > 0 )
	{
		ASSERT( !m_noThumbnail );
		return true;
	}

	// No thumbnail data, do not try to reload
	if ( m_noThumbnail )
	{
		ASSERT( m_thumbnails.Size() == 0 );
		return false;
	}

	// Open source file
	IFile* file = NULL;

	// hack for speed tree resources
	if ( GetFileName().EndsWith( TXT("srt") ) )
	{
		const CFilePath filePath( GetAbsolutePath() );
		const String pathToThumbnail = filePath.GetPathString() + TXT("\\") + filePath.GetFileName() + TXT(".thmb");
		file = GFileManager->CreateFileReader( pathToThumbnail, FOF_Buffered | FOF_AbsolutePath | FOF_DoNotIOManage );
	}
	else
	{
		file = CreateReader();
	}
	
	if ( !file )
	{
		// No source file
		m_noThumbnail = true;
		return false;
	}

	// Read marker
	Uint32 marker[ 2 ] = {0,0};
	file->Seek( file->GetSize() - sizeof(marker) );
	file->Serialize( marker, sizeof(marker) );

	// File does not contain thumbnail data
	if ( marker[1] != 'THMB' )
	{
		// No thumbnail data detected
		delete file;
		m_noThumbnail = true;
		return false;
	}

	// Go to thumbnail data
	file->Seek( marker[0] );

	// Load serialized objects
	CDependencyLoader loader( *file, NULL );
	DependencyLoadingContext loadingContext;
	if ( !loader.LoadObjects( loadingContext ) )
	{
		delete file;
		m_noThumbnail = true;
		return false;
	}

	// Initialize objects
	loader.PostLoad();

	// Close file
	delete file;

	// Get thumbnail
	for ( Uint32 i = 0; i < loadingContext.m_loadedRootObjects.Size(); ++i )
	{
		m_thumbnails.PushBack( Cast< CThumbnail >( loadingContext.m_loadedRootObjects[i] ) );
		m_thumbnails[i]->AddToRootSet();
	}
	if ( m_thumbnails.Size() == 0 )
	{
		m_noThumbnail = true;
		return false;
	}

	// Do not collect thumbnails
	m_noThumbnail = false;

	// Loaded
	return true;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::SaveThumbnail(IFile* file)
{
	// Save thumbnail
	if ( m_thumbnails.Size() > 0 )
	{
		// Always save the thumbnails at the end of the file
		file->Seek( file->GetSize() );

		// Thumbnail offset
		Uint32 fileThumbOffset = static_cast< Uint32 >( file->GetOffset() );

		// Collect thumbnails to save
		DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&) m_thumbnails );

		// Save thumbnail
		CDependencySaver thumbSaver( *file, NULL );
		if ( thumbSaver.SaveObjects( context ) )
		{
			// Add thumbnail marker
			Uint32 marker[2] = { fileThumbOffset, 'THMB' };

			// store the marker - also, at the end
			file->Seek( file->GetSize() );
			file->Serialize( marker, sizeof( marker ) );
		}
	}

	// Saved
	return true;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::CreateThumbnail( TDynArray<CThumbnail*>& thumbnails, const SThumbnailSettings* settings )
{
	// Load resource if not loaded
	Bool wasLoaded = IsLoaded();
	if ( !wasLoaded )
	{
		if ( !Load() )
			return false;
	}

	// Find thumbnail generator
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( ClassID< IThumbnailGenerator >(), classes );
	if ( !classes.Size() )
	{
		WARN_CORE( TXT("No thumbnail generator found") );
		if ( !wasLoaded ) Unload();
		return false;
	}

	SThumbnailSettings settingsToUse;
	if ( settings )
	{
		settingsToUse = *settings;
	}
	else if ( !m_thumbnails.Empty() )
	{
		// If no settings given, use the previously stored ones
		settingsToUse.m_flags          = m_thumbnails[0]->GetFlags();
		settingsToUse.m_cameraPosition = m_thumbnails[0]->GetCameraPosition();
		settingsToUse.m_cameraRotation = m_thumbnails[0]->GetCameraRotation();
		settingsToUse.m_lightPosition  = m_thumbnails[0]->GetSunRotation().Yaw;
		settingsToUse.m_customSettings = TCS_All;
	}

	for ( Uint32 i=0; i<classes.Size(); i++ )
	{
		// Generate thumbnail using generator
		IThumbnailGenerator* generator = classes[i]->GetDefaultObject< IThumbnailGenerator >();
		thumbnails = generator->Create( m_resource, settingsToUse );

		// Valid thumbnail generated
		if ( thumbnails.Size() > 0 )
		{
			if ( !wasLoaded ) Unload();
			return true;
		}
	}

	if ( !wasLoaded ) Unload();
	return false;
}

Bool CDiskFile::UpdateThumbnail( const SThumbnailSettings* settings )
{
	// Remove old thumbnails
	RemoveThumbnail();

	TDynArray<CThumbnail*> newThumbnails;
	if ( CreateThumbnail( newThumbnails, settings ) )
	{
		// Set new thumbnail
		m_noThumbnail = false;
		m_thumbnails = newThumbnails;
		for ( Uint32 i = 0; i < m_thumbnails.Size(); ++i )
		{
			CThumbnail *thumbnail = m_thumbnails[i];
			thumbnail->AddToRootSet();
		}
		return true;
	}

	// No thumbnail generated
	m_noThumbnail = true;
	WARN_CORE( TXT("Unable to compile thumbnail for %s"), m_resource ? m_resource->GetFriendlyName().AsChar() : TXT("nullresource") );
	return false;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::RemoveThumbnail()
{
	// Remove existing thumbnails
	for ( Uint32 i = 0; i < m_thumbnails.Size(); ++i )
	{
		CThumbnail *thumbnail = m_thumbnails[i];
		thumbnail->RemoveFromRootSet();
	}

	// Done
	m_thumbnails.Clear();
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Copy( CDirectory* directory, const String& fileName, Bool forceOverwrite /*=false*/ )
{
	// Check if target file already exists
	CDiskFile* targetFile = directory->FindLocalFile( fileName );
	if ( targetFile )
	{
		if ( targetFile->IsLoaded() )
		{
			WARN_CORE( TXT("Unable to copy %s', destination file is in use"), GetDepotPath().AsChar() );
			return false;
		}

		if ( !forceOverwrite )
		{
			if ( !GFeedback->AskYesNo( TXT("File '%ls' already exists in directory '%ls'. Overwrite?"), fileName.AsChar(), directory->GetDepotPath().AsChar() ) )
			{
				return false;
			}
		}
		WARN_CORE( TXT("File '%ls' is already in the folder '%ls', but it's not in use. Overwriting.."), GetDepotPath().AsChar(), directory->GetDepotPath().AsChar() );		
	}
	else
	{
		// Create target file if does not exist
		targetFile = new CDiskFile( directory, fileName );
		directory->AddFile( targetFile );

		// Create final path
		GFileManager->CreatePath( targetFile->GetAbsolutePath() );
	}

	// Load resource first
	ResourceLoadingContext context;
	if ( Load( context ) )
	{
		// Open target file
		IFile* file = targetFile->CreateWriter( targetFile->GetAbsolutePath() );
		if ( file )
		{
			// Saving can fail from many reasons - the main one is mismatched dependencies ( e.g. link to internal object in another resource )
			DependencySavingContext context( m_resource );
			IDependencySaver* saver = SDependencyLinkerFactory::GetInstance().GetSaver( targetFile, *file );
			saver->SaveObjects( context );

			// Close saver
			delete saver;

			// Close file
			delete file;

			// Add file to directory map
			// directory->AddFile( targetFile );
		}
		else
		{
			WARN_CORE( TXT("Unable to save copy of '%ls'"), GetDepotPath().AsChar() );
			return false;
		}
	}

	// Copied
	return true;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::SetDeleted()
{
	if ( m_state == DFS_CheckedIn )
	{
		m_directory->CheckedOutInc();
	}
	m_state = DFS_Deleted; 
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::SetAdded()
{
	if ( m_state == DFS_CheckedIn )
	{
		m_directory->CheckedOutInc();
	}
	m_state = DFS_Added; 
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::SetLocal()
{ 
	if ( m_state == DFS_Added )
	{
		m_directory->CheckedOutDec();
	}
	m_state = DFS_Local; 
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::SetCheckedIn()
{ 
	if ( IsEdited() )
	{
		m_directory->CheckedOutDec();
	}
	m_state = DFS_CheckedIn; 
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::SetNotSynced()
{
	if ( IsEdited() )
	{
		m_directory->CheckedOutDec();
	}
	m_state = DFS_NotSynced;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::SetCheckedOut()
{ 
	if ( m_state == DFS_CheckedIn )
	{	
		m_directory->CheckedOutInc();
	}
	m_state = DFS_CheckedOut; 
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::GetStatus() 
{	
	GVersionControl->GetStatus( *this );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

EOpenStatus CDiskFile::GetOpenStatus()
{
	return GVersionControl->GetOpenStatus( GetAbsolutePath() );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::VerifyExistence()
{
	if ( !GFileManager->GetFileSize( GetAbsolutePath() ) && m_directory )
	{
		m_directory->DeleteFile( *this );
		return true;
	}
	return false;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Rename( const String& filename, const String& extension )
{
	if ( !MarkModified() )
	{
		return false;
	}

	CFilePath absolutePath( GetAbsolutePath() );
	absolutePath.SetFileName( filename );
	absolutePath.SetExtension( extension );

	if ( GVersionControl->IsSourceControlDisabled() )
	{
		if ( GSystemIO.MoveFile( GetAbsolutePath().AsChar(), absolutePath.ToString().AsChar() ) )
		{
			m_fileName = absolutePath.GetFileNameWithExt();
			return true;
		}
	}
	else
	{
		if ( GVersionControl->Rename( *this, absolutePath.ToString() ) )
		{
			m_fileName = absolutePath.GetFileNameWithExt();
			return true;
		}
	}

	return false;
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Edit()
{
	TDynArray< String > users;

	// Make sure that the resource's OnBefore/AfterCheckout is called
	// in case of a checkout
	if ( m_resource )
	{
		GetStatus();
		if ( !( IsLocal() || IsCheckedOut() ) )
		{
			if ( !m_resource->OnBeforeCheckOut() )
			{
				return false;
			}

			Bool success = GVersionControl->Edit( *this );

			m_resource->OnAfterCheckOut( success );

			return success;
		}
	}

	return GVersionControl->Edit( *this );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::CheckOut( Bool exclusive ) 
{
	if ( m_resource )
	{
		if ( !m_resource->OnBeforeCheckOut() )
		{
			return false;
		}

		Bool success = GVersionControl->CheckOut( *this, exclusive );

		m_resource->OnAfterCheckOut( success );

		return success;
	}
	else
	{
		return GVersionControl->CheckOut( *this, exclusive );
	}
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::SilentCheckOut( Bool exclusive )
{
	if ( m_resource )
	{
		if ( !m_resource->OnBeforeCheckOut() )
		{
			return false;
		}

		Bool success = GVersionControl->SilentCheckOut( *this, exclusive );

		m_resource->OnAfterCheckOut( success );

		return success;
	}
	else
	{
		return GVersionControl->SilentCheckOut( *this, exclusive );
	}
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Submit()
{
	if ( m_resource )
	{
		EResourceSubmitOperation rso = m_resource->OnBeforeSubmit( true, String::EMPTY );
		switch ( rso )
		{
		case RSO_ReturnSuccess:
			return true;
		case RSO_ReturnFailure:
			return false;
		default: ;
			// Any other operation will cause the file to be submitted
		}
	}
	return GVersionControl->Submit( *this );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Submit( const String& description )
{
	if ( m_resource )
	{
		EResourceSubmitOperation rso = m_resource->OnBeforeSubmit( false, description );
		switch ( rso )
		{
		case RSO_ReturnSuccess:
			return true;
		case RSO_ReturnFailure:
			return false;
		default: ;
			// Any other operation will cause the file to be submitted
		}
	}
	return GVersionControl->Submit( *this, description );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Revert( Bool silent /*= false*/ )
{
	return GVersionControl->Revert( *this, silent );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Add()
{
	if ( m_resource )
	{
		m_resource->OnBeforeAddToVersionControl();
	}
	return GVersionControl->Add( *this, m_changelist );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Lock()
{
	return GVersionControl->Lock( *this );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CDiskFile::Sync()
{
	return GVersionControl->GetLatest( *this );
}

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

void CDiskFile::Reload()
{
	ASSERT( m_resource );
	
	// Info
	LOG_CORE( TXT("Reload file: %s"), GetDepotPath().AsChar() );

	// Load the new content
	CDiskFile* file = new CDiskFile(GetDirectory(), GetFileName());
	file->Load();

	// Loading failed
	if ( !file->GetResource() )
	{
		WARN_CORE( TXT("Reloading of '%ls' failed"), GetDepotPath().AsChar() );
		return;
	}

	CResource* oldResource = m_resource;
	CResource* newResource = file->GetResource();

	// Reloading is causing fatal errors in quarantine mechanism due to not removing 'file'
	// from hash map and then receiving the same address for 'file' in one of the next reloads
	// which leads to receiving the same hash with associated leftovers
	if ( file->IsQuarantined() )
	{
		file->RemoveFromQuarantine();
	}
	if ( this->IsQuarantined() )
	{
		RemoveFromQuarantine();
	}

	// Unlink resource <-> file binding for old resource
	oldResource->m_file = NULL;
	file->m_resource = NULL;

	// Create new binding for new resource
	m_resource = newResource;
	newResource->m_file = this;

	// Get number of references to the root set
	const Uint32 rootRefCount = Max< Uint32 >( 0, GObjectsRootSet->GetCount( oldResource ) );
	for ( Uint32 i=0; i<rootRefCount; i++ )
	{
		oldResource->RemoveFromRootSet();
	}

	// Replace references
	CObject::ReplaceReferences(oldResource, newResource);

	// Inform listeners that file has been reloaded
	EDITOR_DISPATCH_EVENT( CNAME( FileReload ), CreateEventData( CReloadFileInfo( oldResource, newResource, String::EMPTY ) ) );

	// Add new resource to the root set
	if ( newResource )
	{
		const Uint32 newRootRefCount = Max< Uint32 >( 0, GObjectsRootSet->GetCount( oldResource ) );
		if ( newRootRefCount != rootRefCount )
		{
			ASSERT( newRootRefCount <= rootRefCount );
			LOG_CORE( TXT("Reloaded resource root set ref count %i -> %i. Restoring."), rootRefCount, newRootRefCount );

			// Add missing references :)
			for ( Uint32 i=newRootRefCount; i<rootRefCount; i++ )
			{
				m_resource->AddToRootSet();
			}
		}
	}

	// Discard old resource
	oldResource->Discard();
	oldResource = NULL;

	// Discard loading file
	delete file;

	// Cleanup
	GObjectGC->CollectNow();
}

#endif

Red::System::DateTime CDiskFile::GetFileTime() const
{
	return GFileManager->GetFileTime( GetAbsolutePath() );
}

// fake wrapper for non-bundled files
// there's no async access for them ATM because in general, they can be to big to fit into the memory
class CAsyncFile_Fallback : public IAsyncFile
{
public:
	CAsyncFile_Fallback( CDiskFile* diskFile )
		: m_diskFile( diskFile )
	{}

	virtual const Char* GetFileNameForDebug() const override
	{
		if ( m_filePath.Empty() )
			m_filePath = m_diskFile->GetDepotPath();

		return m_filePath.AsChar();
	}

	virtual const EResult GetReader( IFile*& outReader) const
	{
		// create normal, synchronous reader
		IFile* reader = m_diskFile->CreateReader();
		if ( !reader )
			return eResult_Failed;

		// valid reader
		outReader = reader;
		return eResult_OK;
	}

private:
	mutable String		m_filePath;
	CDiskFile*			m_diskFile;
};

EAsyncReaderResult CDiskFile::CreateAsyncReader( const Uint8 ioTag, IAsyncFile*& outAsyncReader ) const
{
	outAsyncReader = new CAsyncFile_Fallback( const_cast< CDiskFile* >( this ) );
	return eAsyncReaderResult_OK;
}

IFile* CDiskFile::CreateReader() const
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( m_dataCachedInFile )
	{
		ASSERT( GIsEditor );

		// Initialize file reader for temporary file
		IFile* reader = GFileManager->CreateFileReader( m_cacheFilename, FOF_Buffered | FOF_AbsolutePath | FOF_DoNotIOManage );
		if ( reader )
			return reader;

		// Show error message
		WARN_CORE( TXT("Unable to open temp file: %s"), m_cacheFilename.AsChar() );
		return nullptr;
	}
#endif

	// default for loose files (direct disk access)
	return GFileManager->CreateFileReader( GetAbsolutePath(), FOF_Buffered | FOF_AbsolutePath | FOF_DoNotIOManage );
}

#ifdef RED_PLATFORM_WINPC
	const Uint32 RED_MAX_PATH = 220;		// ??
#endif

IFile* CDiskFile::CreateWriter( const String& path ) const
{
#ifdef RED_PLATFORM_WINPC
	if ( path.Size() > RED_MAX_PATH )
	{
		GFeedback->ShowError( String::Printf( TXT("The file path exceeds %i characters"), RED_MAX_PATH ).AsChar() );
		return nullptr;
	}
#endif
	return new CBufferedTempFile( path );
}

String CDiskFile::GetAbsolutePath() const
{
	ASSERT( m_directory );

	String path;
	m_directory->GetAbsolutePath( path );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	path += m_diskFilename; // perfroce is case sensitive.
#else
	path += m_fileName;
#endif
	
	return path;
}

String CDiskFile::GetDepotPath() const
{
	const Int32 c_maximumDirectoryDepth = 32;

	struct SDirectoryStackEntry
	{
		SDirectoryStackEntry()	{ }
		SDirectoryStackEntry( const Char* buffer, Uint32 length ) : m_depotNamePtr( buffer ), m_depotNameBufferLength( length )	{ }
		const Char* m_depotNamePtr;
		Uint32 m_depotNameBufferLength;
	};

	// Build a stack of directories. We need to walk it backwards to append without recursion
	SDirectoryStackEntry directoryStack[ c_maximumDirectoryDepth ];
	Int32 directoryStackHead = 0;
	CDirectory* pathDirectory = m_directory;
	Uint32 totalPathLength = 0;
	while( pathDirectory && pathDirectory->GetParent() != nullptr )		// Directory with null parent = depot
	{
		RED_FATAL_ASSERT( directoryStackHead < c_maximumDirectoryDepth, "Stack got too deep! c_maximumDirectoryDepth must be increased" );
		directoryStack[ directoryStackHead++ ] = SDirectoryStackEntry( pathDirectory->GetName().AsChar(), pathDirectory->GetName().GetLength() );
		totalPathLength += pathDirectory->GetName().GetLength() + 1;		// +1 for the path seperator
		pathDirectory = pathDirectory->GetParent();
	}

	// At this point we know exactly how big the string must be
	totalPathLength += m_fileName.GetLength() + 1;	// +1 for null terminator

	String outputStr;
	outputStr.Reserve( totalPathLength + m_fileName.GetLength() + 1 );	
	for( Int32 directoryIndex = directoryStackHead - 1; directoryIndex >= 0; --directoryIndex )
	{
		const SDirectoryStackEntry& theDirectory = directoryStack[ directoryIndex ];
		outputStr += theDirectory.m_depotNamePtr;
		outputStr += TXT( '\\' );
	}

	// Append the local filename + null terminator
	outputStr += m_fileName.AsChar();
	return outputStr;
}

const Bool CDiskFile::IsLooseFile() const
{
	return true;
}

Red::Core::Bundle::FileID CDiskFile::GetBufferFileID( const Uint32 bufferIndex ) const
{
	return 0;
}

void CDiskFile::GetPhysicalFileIDs( TPhysicalFileMap& outLocations ) const
{
	RED_UNUSED( outLocations );
}

void CDiskFile::GetPhysicalFileLocation( SDiskFileLocation& outLocation, const TPhysicalFileMap& preferredLocations ) const
{
	RED_UNUSED( outLocation );
	RED_UNUSED( preferredLocations );
}

void CDiskFile::GetPhysicalFileLocations( TDynArray< SDiskFileLocation >& outLocations ) const
{
	RED_UNUSED( outLocations );
}

#ifndef NO_EDITOR
Bool CDiskFile::IsResourceMergeable() const
{
	Uint32 size = ARRAY_COUNT_U32( SMergeableResourceExtensions );
	for( Uint32 i=0; i<size; ++i )
	{
		if ( CFilePath( GetFileName() ).GetExtension() == SMergeableResourceExtensions[i] )
		{
			return true;
		}
	}
	return false;
}

#endif

//----

void CDiskFile::ReportLoaded() const
{
#ifdef ENABLE_RESOURCE_MONITORING
	if ( m_monitorData )
	{
		RED_ASSERT( m_monitorData->m_isLoaded == 0, TXT("Corrupted internal flag state for resource '%ls'"), GetDepotPath().AsChar() );

		if ( !m_monitorData->m_isLoaded )
		{
			m_monitorData->m_frameLoaded = st_frameIndex;
			m_monitorData->m_timeLoaded = EngineTime::GetNow();
			m_monitorData->m_loadCount += 1;
			m_monitorData->m_isLoaded = 1;
			m_monitorData->m_isSyncLoaded = ::SIsMainThread(); // hacky

			m_monitorData->Report( ResourceMonitorStats::eEventType_Loaded );
		}
	}
#endif
}

void CDiskFile::ReportUnloaded() const
{
#ifdef ENABLE_RESOURCE_MONITORING
	if ( m_monitorData )
	{
		RED_ASSERT( m_monitorData->m_isLoaded == 1, TXT("Corrupted internal flag state for resource '%ls'"), GetDepotPath().AsChar() );

		if ( m_monitorData->m_isLoaded )
		{
			m_monitorData->m_frameUnloaded = st_frameIndex;
			m_monitorData->m_timeUnloaded = EngineTime::GetNow();
			m_monitorData->m_unloadCount += 1;
			m_monitorData->m_isLoaded = 0;

			m_monitorData->Report( ResourceMonitorStats::eEventType_Unloaded );
		}
	}
#endif
}

void CDiskFile::ReportExpelled() const
{
#ifdef ENABLE_RESOURCE_MONITORING
	if ( m_monitorData )
	{
		RED_ASSERT( m_monitorData->m_isQuarantined == 0, TXT("Corrupted internal flag state for resource '%ls'"), GetDepotPath().AsChar() );

		if ( !m_monitorData->m_isQuarantined )
		{
			m_monitorData->m_frameExpelled = st_frameIndex;
			m_monitorData->m_timeExpelled = EngineTime::GetNow();
			m_monitorData->m_expellCount += 1;
			m_monitorData->m_isQuarantined = 1;

			m_monitorData->Report( ResourceMonitorStats::eEventType_Expelled );
		}
	}
#endif
}

void CDiskFile::ReportRevived() const
{
#ifdef ENABLE_RESOURCE_MONITORING
	if ( m_monitorData )
	{
		RED_ASSERT( m_monitorData->m_isQuarantined == 1, TXT("Corrupted internal flag state for resource '%ls'"), GetDepotPath().AsChar() );

		if ( m_monitorData->m_isQuarantined )
		{
			m_monitorData->m_frameRevived = st_frameIndex;
			m_monitorData->m_timeRevived = EngineTime::GetNow();
			m_monitorData->m_reviveCount += 1;
			m_monitorData->m_isQuarantined = 0;

			m_monitorData->Report( ResourceMonitorStats::eEventType_Revived );
		}
	}
#endif
}

void CDiskFile::ReportLoadingEvent( const Float timeTaken, const Bool hadImports ) const
{
#ifdef ENABLE_RESOURCE_MONITORING
	if ( m_monitorData )
	{
		m_monitorData->m_loadTime = timeTaken;
		m_monitorData->m_hadImports |= hadImports;
		m_monitorData->m_worstLoadTime = Red::Math::NumericalUtils::Max<Float>( timeTaken, m_monitorData->m_worstLoadTime );
	}
#endif
}

void CDiskFile::ReportPostLoadEvent( const Float timeTaken, const Bool hadOtherLoads ) const
{
#ifdef ENABLE_RESOURCE_MONITORING
	if ( m_monitorData )
	{
		m_monitorData->m_postLoadTime = timeTaken;
		m_monitorData->m_hadPostLoadImports |= hadOtherLoads;
		m_monitorData->m_worstPostLoadTime = Red::Math::NumericalUtils::Max<Float>( timeTaken, m_monitorData->m_worstPostLoadTime );
	}
#endif
}

void CDiskFile::ReportFailedLoad() const
{
#ifdef ENABLE_RESOURCE_MONITORING
	if ( m_monitorData )
	{
		m_monitorData->m_isMissing = 1;
	}
#endif
}

//----

CBundleDiskFile::CBundleDiskFile( CDirectory* directory, const String& fileName, const Red::Core::Bundle::FileID fileID )
	: CDiskFile( directory, fileName )
	, m_fileID( fileID )
{
}

CBundleDiskFile::~CBundleDiskFile()
{
}

EAsyncReaderResult CBundleDiskFile::CreateAsyncReader( const Uint8 ioTag, IAsyncFile*& outAsyncReader ) const
{
	return GDepot->GetBundles()->CreateAsyncFileReader( m_fileID, ioTag, outAsyncReader );
}

IFile* CBundleDiskFile::CreateReader() const
{
	IFile* ret = GDepot->GetBundles()->CreateFileReader( m_fileID );
	if ( !ret )
	{
		ERR_CORE( TXT("!!! MISSING BUNDLE FILE !!!") );
		ERR_CORE( TXT("File '%ls' is not avaiable in currently attached bundles"), GetDepotPath().AsChar() );
	}
	return ret;
}

IFile* CBundleDiskFile::CreateWriter( const String& path ) const
{
	ERR_CORE( TXT("Tring to open writer for bundled file '%ls' to path '%ls'. This is not supported because bundled files are read only."), 
		GetDepotPath().AsChar(), path.AsChar() );
	RED_UNUSED( path );
	return nullptr;
}

Red::System::DateTime CBundleDiskFile::GetFileTime() const
{
	// TODO: (copied from old bundle proxy)
	return Red::System::DateTime();
}

Red::Core::Bundle::FileID CBundleDiskFile::GetBufferFileID( const Uint32 bufferIndex ) const
{
	return GDepot->GetBundles()->GetBufferFileID( m_fileID, bufferIndex );
}

void CBundleDiskFile::GetPhysicalFileLocations( TDynArray< SDiskFileLocation >& outLocations ) const
{
	TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > placements;
	placements.Reserve( 8 );

	// Get all entries for given file - will also return unmounted ones
	GDepot->GetBundles()->GetFileEntries( m_fileID, placements );

	// Create location information
	outLocations.Reserve( outLocations.Size() + placements.Size() );
	for ( const auto& placement : placements )
	{
		new ( outLocations ) SDiskFileLocation( placement.m_bundleID, placement.m_offsetInBundle );
	}
}

void CBundleDiskFile::GetPhysicalFileIDs( TPhysicalFileMap& outLocations ) const
{
	TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > placements;
	placements.Reserve( 8 );

	// Get all entries for given file - will also return unmounted ones
	GDepot->GetBundles()->GetFileEntries( m_fileID, placements );

	// Create location information
	for ( const auto& placement : placements )
	{
		outLocations[ placement.m_bundleID ] += 1;
	}
}

void CBundleDiskFile::GetPhysicalFileLocation( SDiskFileLocation& outLocation, const TPhysicalFileMap& preferredLocations ) const
{
	TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > placements;
	placements.Reserve( 8 );

	// Get all entries for given file - will also return unmounted ones
	GDepot->GetBundles()->GetFileEntries( m_fileID, placements );
	RED_FATAL_ASSERT( !placements.Empty(), "No file placement where there should be at least one" );

	// Try to use location that is most commonly used
	Uint32 bestLocation = 0;
	Uint32 bestPhysicalFile = 0;
	Uint32 bestFileCount = 0;
	for ( Uint32 i=0; i<placements.Size(); ++i )
	{
		const Uint32 physicalFile = placements[i].m_bundleID;

		Uint32 physicalFileUseCount = 0;
		preferredLocations.Find( physicalFile, physicalFileUseCount );

		// make sure this is deterministic - hence the case for the equal count
		if ( physicalFileUseCount > bestFileCount || (physicalFileUseCount == bestFileCount && physicalFile < bestPhysicalFile) )
		{
			bestLocation = i;
			bestPhysicalFile = physicalFile;
			bestFileCount = physicalFileUseCount;
		}
	}

	// Return best location
	const auto& best = placements[ bestLocation ];
	outLocation = SDiskFileLocation( best.m_bundleID, best.m_offsetInBundle );
}

//----
