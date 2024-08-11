#include "build.h"
#include "profilerBlockFileWriter.h"
#include "fileSys.h"
#include "fileSystemProfiler.h"
#include "fileSystemProfilerWrapper.h"
#include "configVar.h"

CFileManagerProfiler GFileManagerProfiler;

namespace Config
{
	TConfigVar<Bool>		cvEnableFileSystemProfiling( "Profiler/FileSystem", "EnableFileSystemProfiling", false );
	TConfigVar<Bool>		cvAutoStartProfiling( "Profiler/FileSystem", "AutoStart", false );
	TConfigVar<String>		cvOutputFilePrefix( "Profiler/FileSystem", "OutputFilePrefix", TXT("filesystem") );
	TConfigVar<String>		cvOutputFileDirectory( "Profiler/FileSystem", "OutputDirectory", TXT("") );
	TConfigVar<Bool>		cvFlushCurrentData( "Profiler/FileSystem", "FlushCurrentData", false );
} // Config

CFileManagerProfiler::CFileManagerProfiler()
{
	// define block types for system operations
	m_writer.RegisterBlockType( eBlockType_FileSyncOpen, "Sync Open", 1, 1 ); // (hash) (handle)
	m_writer.RegisterBlockType( eBlockType_FileSyncClose, "Sync Close", 1, 0 ); // (handle) ()
	m_writer.RegisterBlockType( eBlockType_FileAsyncOpen, "Async Open", 1, 1 ); // (hash) (handle)
	m_writer.RegisterBlockType( eBlockType_FileAsyncClose, "Async Close", 1, 0 ); // (handle) ()
	m_writer.RegisterBlockType( eBlockType_FileAsyncRead, "Async Read", 1, 1 ); // (reqid) (reqid)
	m_writer.RegisterBlockType( eBlockType_FileSyncRead, "Sync Read", 2, 0 ); // (file, size) ()
	m_writer.RegisterBlockType( eBlockType_FileSyncSeek, "Sync Seek", 3, 0 ); // (file, offsetLo, offsetHi) ()

	// define block types for engine operations
	m_writer.RegisterBlockType( eBlockType_LoadResource, "Load Resource", 1, 1 ); // (path) (path)
	m_writer.RegisterBlockType( eBlockType_SyncLoadData, "Sync Load File", 1, 0 ); // (size) ()
	m_writer.RegisterBlockType( eBlockType_SyncDecompression, "Sync Decompression", 2, 0 ); // (size, type) ()
	m_writer.RegisterBlockType( eBlockType_AsyncDecompression, "Async Decompression", 2, 0 ); // (size, type) ()
	m_writer.RegisterBlockType( eBlockType_Deserialize, "Deserialize", 1, 1 ); // (path) (path)
	m_writer.RegisterBlockType( eBlockType_LoadTables, "Load Tables", 1, 1 ); // (path) (path)
	m_writer.RegisterBlockType( eBlockType_MapTables, "Map Tables", 1, 1 ); // (path) (path)
	m_writer.RegisterBlockType( eBlockType_CreateObjects, "Create Objects", 2, 1 ); // (path, count) (path)
	m_writer.RegisterBlockType( eBlockType_LoadImports, "Load Imports", 2, 1 ); // (path, count) (path)
	m_writer.RegisterBlockType( eBlockType_LoadObjects, "Load Objects", 2, 1 ); // (path, count) (path)
	m_writer.RegisterBlockType( eBlockType_PostLoad, "Post Load", 1, 1 ); // (path) (path)
	m_writer.RegisterBlockType( eBlockType_DeferredSyncRead, "Sync Deferred Load", 1, 0 ); // (size) ()
	m_writer.RegisterBlockType( eBlockType_LoadInplace, "Load Inplace", 2, 1 ); // (path, count) (path)
	m_writer.RegisterBlockType( eBlockType_LoadingJob, "Job", 2, 0 ); // (name, priority) ()
	m_writer.RegisterBlockType( eBlockType_UserProfileBlock, "Block", 1, 0 ); // (name) ()	
	m_writer.RegisterBlockType( eBlockType_Scene, "Scene", 1, 0 ); // (name) ()
	m_writer.RegisterBlockType( eBlockType_SceneSection, "Section", 1, 0 ); // (name) ()
	m_writer.RegisterBlockType( eBlockType_StreamingLock, "Streaming lock", 1, 1 ); // (name) (name)

	// counters
	m_writer.RegisterCounterType( eCounterType_MemoryBlocks, "IO Memory" ); // (size)
	m_writer.RegisterCounterType( eCounterType_PendingFiles, "Pending files" ); // (count)
	m_writer.RegisterCounterType( eCounterType_DecompressionTasks, "Decompression tasks" ); // (count)

	// signals
	m_writer.RegisterSignalType( eSignalType_BlackScreenOn, "Blackscreen: ON", 0 );
	m_writer.RegisterSignalType( eSignalType_BlackScreenOff, "Blackscreen: OFF", 0 );
	m_writer.RegisterSignalType( eSignalType_LoadingScreenOn, "Loading screen: ON", 0 );
	m_writer.RegisterSignalType( eSignalType_LoadingScreenOff, "Loading screen: OFF", 0 );
	m_writer.RegisterSignalType( eSignalType_VideoPlay, "Video: PLAY", 0 );
	m_writer.RegisterSignalType( eSignalType_VideoStop, "Video: STOP", 0 );
	m_writer.RegisterSignalType( eSignalType_VideoReadRequested, "Video read requested", 0 );
	m_writer.RegisterSignalType( eSignalType_VideoReadCompleted, "Video read completed", 0 );
	m_writer.RegisterSignalType( eSignalType_FileAsyncReadScheduled, "Async Schedule", 6 ); // (handle, reqid, size, offsetLow, offsetHi, ioTag)
	m_writer.RegisterSignalType( eSignalType_LoadingProfilerBlock, "Loading Block", 1 ); // (name)
	m_writer.RegisterSignalType( eSignalType_LoadingProfilerEnd, "Loading End", 0 );
	m_writer.RegisterSignalType( eSignalType_StreamingCameraPos, "Camera pos", 3 ); // (x,y,z)
	m_writer.RegisterSignalType( eSignalType_StreamingCameraDist, "Camera move", 1 ); // (distance)
	m_writer.RegisterSignalType( eSignalType_StreamingEntityStats, "Entity streaming", 4 ); // (in range, streaming, loaded, locked)
	m_writer.RegisterSignalType( eSignalType_StreamingSectorStats, "Sector streaming", 4 ); // (in range, streaming, loaded, locked)
}

CFileManagerProfiler::~CFileManagerProfiler()
{
}

void CFileManagerProfiler::AssembleFilePath( String& outAbsoluteFilePath ) const
{
	// use depot path if possible
	String outputPath;
	if ( !Config::cvOutputFileDirectory.Get().Empty() )
	{
		outputPath = Config::cvOutputFileDirectory.Get();
		if ( !outputPath.EndsWith( TXT("\\") ) )
			outputPath += TXT("\\");
	}
	else
	{
#ifdef RED_PLATFORM_ORBIS
		outputPath = TXT("/data/");
#else
		outputPath = m_basePath + TXT("redio\\");
#endif
	}

	// time stamp
	Red::DateTime time;
	Red::Clock::GetInstance().GetLocalTime(time);

	// assemble file path
	outAbsoluteFilePath = outputPath;
	outAbsoluteFilePath += String::Printf( TXT("%ls_[%02d_%02d_%04d]_[%02d_%02d_%02d].redio"), 
		Config::cvOutputFilePrefix.Get().AsChar(),
		time.GetDay(), time.GetMonth(), time.GetYear(),
		time.GetHour(), time.GetMinute(), time.GetSecond() );
}

void CFileManagerProfiler::Initilize( const String& basePath )
{
	m_basePath = basePath;

	// Override setting from command line :)
	const Bool autoProfileIO = (nullptr != Red::StringSearch( SGetCommandLine(), TXT("-profileio") ));
	if ( autoProfileIO )
	{
		LOG_CORE( TXT("IO profiling enabled by commandline") );
		Config::cvAutoStartProfiling.Set(true);
		Config::cvEnableFileSystemProfiling.Set(true);
	}

	// Auto start profiling
	if ( Config::cvAutoStartProfiling.Get() )
	{
		if ( !IsProfiling() )
			Start();
	}
}

void CFileManagerProfiler::Shutdown()
{
	// close on exit
	Stop();
}

void CFileManagerProfiler::Start()
{
	// do not allow new profiling to be started
	if ( !Config::cvEnableFileSystemProfiling.Get() )
		return;

	// stop current profiling
	if ( IsProfiling() )
		Stop();

	// assemble output file path
	String outputFilePath;
	AssembleFilePath( outputFilePath );

	// start file
	m_writer.Start( outputFilePath );
}

void CFileManagerProfiler::Stop()
{
	m_writer.Stop();
}

const Bool CFileManagerProfiler::IsProfiling() const
{
	return m_writer.IsProfiling();
}

void CFileManagerProfiler::Flush()
{
	m_writer.Flush();
}

const Uint32 CFileManagerProfiler::MapFilePath( const String& path )
{
	return m_writer.MapString( path );
}

const Uint32 CFileManagerProfiler::MapFilePath( const StringAnsi& path )
{
	return m_writer.MapString( path );
}

const Uint32 CFileManagerProfiler::MapFilePath( const Char* path )
{
	return m_writer.MapString( path );
}

const Uint32 CFileManagerProfiler::MapFilePath( const AnsiChar* path )
{
	return m_writer.MapString( path );
}

void CFileManagerProfiler::SetThreadName( const AnsiChar* threadName )
{
	return m_writer.SetThreadName( threadName );
}

void CFileManagerProfiler::ConditionalFlushOnce()
{
	if ( Config::cvFlushCurrentData.Get() )
	{
		Config::cvFlushCurrentData.Set(false);
		Flush();
	}
}
	

