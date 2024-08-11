#include "build.h"
#include "fileSystemProfiler.h"
#include "fileSystemProfilerWrapper.h"

#ifdef RED_PROFILE_FILE_SYSTEM

namespace RedIOProfiler
{
	static Red::Threads::AtomicOps::TAtomic32 RequestIDCounter = 0;

	Uint32 ProfileAllocRequestId()
	{
		return Red::Threads::AtomicOps::Increment32( &RequestIDCounter );
	}

	void ProfileSetThreadName( const Red::AnsiChar* name )
	{
		GFileManagerProfiler.SetThreadName( name );
	}

	void ProfileFinishLoadingPhase( const Red::AnsiChar* name )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( name );
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_LoadingProfilerBlock, nameHash );
	}

	void ProfileEndLoading()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_LoadingProfilerEnd );
	}

	void ProfileAsyncIOOpenFileStart( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_FileAsyncOpen, nameHash );
	}

	void ProfileAsyncIOOpenFileEnd( const Uint32 allocatedHandle )
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_FileAsyncOpen, allocatedHandle );
	}

	void ProfileAsyncIOCloseFileStart( const Uint32 fileHandle )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_FileAsyncClose, fileHandle );
	}

	void ProfileAsyncIOCloseFileEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_FileAsyncClose );
	}

	void ProfileAsyncIOReadScheduled( const Uint32 fileHandle, const Uint32 requestID, const Uint64 offset, const Uint32 size, const Red::Uint32 ioTag )
	{
		const Uint32 offsetLow = (Uint32)(offset >> 0);
		const Uint32 offsetHigh = (Uint32)(offset >> 32);
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_FileAsyncReadScheduled, fileHandle, requestID, offsetLow, offsetHigh, size, ioTag );
	}

	void ProfileAsyncIOReadStart( const Uint32 requestID )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_FileAsyncRead, requestID );
	}

	void ProfileAsyncIOReadEnd( const Uint32 requestID )
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_FileAsyncRead, requestID );
	}

	void ProfileSyncIOOpenFileStart( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_FileSyncOpen, nameHash );
	}

	void ProfileSyncIOOpenFileEnd( const Uint32 allocatedHandle )
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_FileSyncOpen, allocatedHandle );
	}

	void ProfileSyncIOCloseFileStart( const Uint32 fileHandle )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_FileSyncClose, fileHandle );
	}

	void ProfileSyncIOCloseFileEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_FileSyncClose );
	}

	void ProfileSyncIOReadStart( const Uint32 fileHandle, const Uint32 size )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_FileSyncRead, fileHandle, size );
	}

	void ProfileSyncIOReadEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_FileSyncRead );
	}

	void ProfileSyncIOSeekStart( const Uint32 fileHandle, const Uint64 offset )
	{
		const Uint32 offsetLow = (Uint32)(offset >> 0);
		const Uint32 offsetHigh = (Uint32)(offset >> 32);
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_FileSyncSeek, fileHandle, offsetLow, offsetHigh );
	}

	void ProfileSyncIOSeekEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_FileSyncSeek );
	}

	void ProfileDiskFileLoadResourceStart( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_LoadResource, nameHash );
	}

	void ProfileDiskFileLoadResourceEnd( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_LoadResource, nameHash );
		GFileManagerProfiler.ConditionalFlushOnce();
	}

	void ProfileLoadingJobStart( const Red::Char* jobName, const Red::Uint32 priority )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( jobName );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_LoadingJob, nameHash, priority );
	}

	void ProfileLoadingJobEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_LoadingJob );
	}

	void ProfileDiskFileSyncLoadDataStart( const Uint32 size )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_SyncLoadData, size );
	}

	void ProfileDiskFileSyncLoadDataEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_SyncLoadData );
	}

	void ProfileDiskFileSyncDecompressStart( const Red::Uint32 size, const Red::Uint8 type )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_SyncDecompression, size, type );
	}

	void ProfileDiskFileSyncDecompressEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_SyncDecompression );
	}

	void ProfileDiskFileAsyncDecompressStart( const Red::Uint32 size, const Red::Uint8 type )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_AsyncDecompression, size, type );
	}

	void ProfileDiskFileAsyncDecompressEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_AsyncDecompression );
	}

	void ProfileDiskFileDeserializeStart( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_Deserialize, nameHash );
	}

	void ProfileDiskFileDeserializeEnd( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_Deserialize, nameHash );
	}

	void ProfileDiskFilePostLoadStart( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_PostLoad, nameHash );
	}

	void ProfileDiskFilePostLoadEnd( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_PostLoad, nameHash );
	}

	void ProfileDiskFileCreateExportsStart( const Char* filePath, const Uint32 numExports )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_CreateObjects, nameHash, numExports );
	}

	void ProfileDiskFileCreateExportsEnd( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_CreateObjects, nameHash );
	}

	void ProfileDiskFileLoadExportsStart( const Char* filePath, const Uint32 numExports )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_LoadObjects, nameHash, numExports );
	}

	void ProfileDiskFileLoadExportsEnd( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_LoadObjects, nameHash );
	}

	void ProfileDiskFileLoadImportsStart( const Char* filePath, const Uint32 numImports )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_LoadImports, nameHash, numImports );
	}

	void ProfileDiskFileLoadImportsEnd( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_LoadImports, nameHash );
	}

	void ProfileDiskFileLoadInplaceStart( const Char* filePath, const Uint32 numImports )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_LoadInplace, nameHash, numImports );
	}

	void ProfileDiskFileLoadInplaceEnd( const Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_LoadInplace, nameHash );
	}

	void ProfileDiskFileLoadTablesStart( const Red::Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_LoadTables, nameHash );
	}

	void ProfileDiskFileLoadTablesEnd( const Red::Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_LoadTables, nameHash );
	}

	void ProfileDiskFileMapTablesStart( const Red::Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_MapTables, nameHash );
	}

	void ProfileDiskFileMapTablesEnd( const Red::Char* filePath )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( filePath );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_MapTables, nameHash );
	}

	void ProfileDiskFileDeferredDataLoadSyncStart( const Red::Uint32 size )
	{
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_DeferredSyncRead, size );
	}

	void ProfileDiskFileDeferredDataLoadSyncEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_DeferredSyncRead );
	}

	void ProfileLoadingUserBlockStart( const Red::Char* name )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( name );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_UserProfileBlock, nameHash );
	}

	void ProfileLoadingUserBlockEnd()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_UserProfileBlock );
	}

	//--------------------

	void ProfileVarAllocMemoryBlock( const Red::Uint32 size )
	{
		GFileManagerProfiler.CounterIncrement( CFileManagerProfiler::eCounterType_MemoryBlocks, size );
	}

	void ProfileVarFreeMemoryBlock( const Red::Uint32 size )
	{
		GFileManagerProfiler.CounterDecrement( CFileManagerProfiler::eCounterType_MemoryBlocks, size );
	}

	void ProfileVarAllocDecompressionTask()
	{		
		GFileManagerProfiler.CounterIncrement( CFileManagerProfiler::eCounterType_DecompressionTasks, 1 );
	}

	void ProfileVarFreeDecompressionTask()
	{
		GFileManagerProfiler.CounterDecrement( CFileManagerProfiler::eCounterType_DecompressionTasks, 1 );
	}

	void ProfileVarAddPendingFiles( const Red::Uint32 count )
	{
		GFileManagerProfiler.CounterIncrement( CFileManagerProfiler::eCounterType_PendingFiles, count );
	}

	void ProfileVarRemovePendingFiles( const Red::Uint32 count )
	{
		GFileManagerProfiler.CounterDecrement( CFileManagerProfiler::eCounterType_PendingFiles, 1 );
	}

	//------------------

	void ProfileSignalShowLoadingScreen()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_LoadingScreenOn );
	}

	void ProfileSignalHideLoadingScreen()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_LoadingScreenOff );
	}

	void ProfileSignalShowBlackScreen()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_BlackScreenOn );
	}

	void ProfileSignalHideBlackScreen()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_BlackScreenOff );
	}

	void ProfileSignalVideoPlay()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_VideoPlay );
	}

	void ProfileSignalVideoStop()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_VideoStop );
	}

	void ProfileSignalVideoReadRequested()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_VideoReadRequested );
	}

	void ProfileSignalVideoReadCompleted()
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_VideoReadCompleted );
	}

	void ProfileSceneStarted( const Red::Char* sceneName )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( sceneName );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_Scene, nameHash );
	}

	void ProfileSceneEnded()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_Scene );
	}

	void ProfileSceneSectionStarted( const Red::Char* sectionName )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( sectionName );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_SceneSection, nameHash );
	}

	void ProfileSceneSectionEnded()
	{
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_SceneSection );
	}

	void ProfileStreamingLockAdded( const Red::Char* sectionName )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( sectionName );
		GFileManagerProfiler.Start( CFileManagerProfiler::eBlockType_StreamingLock, nameHash );
	}

	void ProfileStreamingLockRemoved( const Red::Char* sectionName )
	{
		const Uint32 nameHash = GFileManagerProfiler.MapFilePath( sectionName );
		GFileManagerProfiler.End( CFileManagerProfiler::eBlockType_StreamingLock, nameHash );
	}

	void ProfileStreamingCameraPos( const Red::Float x, const Red::Float y, const Red::Float z )
	{
		const Uint32 posX = (Int32)(x * 1000.0f);
		const Uint32 posY = (Int32)(y * 1000.0f);
		const Uint32 posZ = (Int32)(z * 1000.0f);
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_StreamingCameraPos, posX, posY, posZ );		
	}

	void ProfileStreamingCameraDist( const Red::Float dist )
	{
		const Uint32 distInt = (Int32)(dist * 1000.0f);
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_StreamingCameraDist, distInt );
	}

	void ProfileStreamingEntity( const Red::Uint32 numInRange, const Red::Uint32 numStreaming, const Red::Uint32 numStreamed, const Red::Uint32 numLocked )
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_StreamingEntityStats, numInRange, numStreaming, numStreamed, numLocked );
	}

	void ProfileStreamingSector( const Red::Uint32 numInRange, const Red::Uint32 numStreaming, const Red::Uint32 numStreamed, const Red::Uint32 numLocked )
	{
		GFileManagerProfiler.Signal( CFileManagerProfiler::eSignalType_StreamingSectorStats, numInRange, numStreaming, numStreamed, numLocked );
	}


} // RedIOProfiler

#else

#endif
