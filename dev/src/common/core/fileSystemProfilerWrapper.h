/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _FILESYS_PROFILER_WRAPPER_H__
#define _FILESYS_PROFILER_WRAPPER_H__

// NOTE: This can cause large frame drops from mutex contention when allocating! Don't ship with this enabled!!!
// profile the file system
#if !defined(RED_FINAL_BUILD) || defined(RED_LOGGING_ENABLED) || defined(RED_PROFILE_BUILD)
	#define RED_PROFILE_FILE_SYSTEM
#endif

#ifdef RED_PROFILE_FILE_SYSTEM

namespace RedIOProfiler
{
	extern Red::Uint32 ProfileAllocRequestId();

	extern void ProfileSetThreadName( const Red::AnsiChar* name );

	extern void ProfileFinishLoadingPhase( const Red::AnsiChar* name );
	extern void ProfileEndLoading();

	extern void ProfileAsyncIOOpenFileStart( const Red::Char* filePath );
	extern void ProfileAsyncIOOpenFileEnd( const Red::Uint32 allocatedHandle );
	extern void ProfileAsyncIOCloseFileStart( const Red::Uint32 fileHandle );
	extern void ProfileAsyncIOCloseFileEnd();
	extern void ProfileAsyncIOReadScheduled( const Red::Uint32 fileHandle, const Red::Uint32 requestID, const Red::Uint64 offset, const Red::Uint32 size, const Red::Uint32 ioTag );
	extern void ProfileAsyncIOReadStart( const Red::Uint32 requestID );
	extern void ProfileAsyncIOReadEnd( const Red::Uint32 requestID );

	extern void ProfileSyncIOOpenFileStart( const Red::Char* filePath );
	extern void ProfileSyncIOOpenFileEnd( const Red::Uint32 allocatedHandle );
	extern void ProfileSyncIOCloseFileStart( const Red::Uint32 fileHandle );
	extern void ProfileSyncIOCloseFileEnd();
	extern void ProfileSyncIOReadStart( const Red::Uint32 fileHandle, const Red::Uint32 size );
	extern void ProfileSyncIOReadEnd();
	extern void ProfileSyncIOSeekStart( const Red::Uint32 fileHandle, const Red::Uint64 offset );
	extern void ProfileSyncIOSeekEnd();

	extern void ProfileDiskFileLoadResourceStart( const Red::Char* filePath );
	extern void ProfileDiskFileLoadResourceEnd( const Red::Char* filePath );

	extern void ProfileLoadingJobStart( const Red::Char* jobName, const Red::Uint32 priority );
	extern void ProfileLoadingJobEnd();

	extern void ProfileLoadingUserBlockStart( const Red::Char* name );
	extern void ProfileLoadingUserBlockEnd();

	extern void ProfileDiskFileSyncLoadDataStart( const Red::Uint32 size );
	extern void ProfileDiskFileSyncLoadDataEnd();
	extern void ProfileDiskFileSyncDecompressStart( const Red::Uint32 size, const Red::Uint8 type );
	extern void ProfileDiskFileSyncDecompressEnd();
	extern void ProfileDiskFileAsyncDecompressStart( const Red::Uint32 size, const Red::Uint8 type );
	extern void ProfileDiskFileAsyncDecompressEnd();

	extern void ProfileDiskFileDeserializeStart( const Red::Char* filePath );
	extern void ProfileDiskFileDeserializeEnd( const Red::Char* filePath );
	extern void ProfileDiskFileLoadTablesStart( const Red::Char* filePath );
	extern void ProfileDiskFileLoadTablesEnd( const Red::Char* filePath );
	extern void ProfileDiskFileMapTablesStart( const Red::Char* filePath );
	extern void ProfileDiskFileMapTablesEnd( const Red::Char* filePath );
	extern void ProfileDiskFileCreateExportsStart( const Red::Char* filePath, const Red::Uint32 count );
	extern void ProfileDiskFileCreateExportsEnd( const Red::Char* filePath );
	extern void ProfileDiskFileLoadExportsStart( const Red::Char* filePath, const Red::Uint32 count );
	extern void ProfileDiskFileLoadExportsEnd( const Red::Char* filePath );
	extern void ProfileDiskFileLoadImportsStart( const Red::Char* filePath, const Red::Uint32 count );
	extern void ProfileDiskFileLoadImportsEnd( const Red::Char* filePath );
	extern void ProfileDiskFilePostLoadStart( const Red::Char* filePath );
	extern void ProfileDiskFilePostLoadEnd( const Red::Char* filePath );
	extern void ProfileDiskFileLoadInplaceStart( const Red::Char* filePath, const Red::Uint32 count );
	extern void ProfileDiskFileLoadInplaceEnd( const Red::Char* filePath );

	extern void ProfileDiskFileDeferredDataLoadSyncStart( const Red::Uint32 size );
	extern void ProfileDiskFileDeferredDataLoadSyncEnd();

	extern void ProfileVarAllocMemoryBlock( const Red::Uint32 size );
	extern void ProfileVarFreeMemoryBlock( const Red::Uint32 size );
	extern void ProfileVarAllocDecompressionTask();
	extern void ProfileVarFreeDecompressionTask();
	extern void ProfileVarAddPendingFiles( const Red::Uint32 count );
	extern void ProfileVarRemovePendingFiles( const Red::Uint32 count );

	extern void ProfileSignalShowLoadingScreen();
	extern void ProfileSignalHideLoadingScreen();
	extern void ProfileSignalShowBlackScreen();
	extern void ProfileSignalHideBlackScreen();
	extern void ProfileSignalVideoPlay();
	extern void ProfileSignalVideoStop();
	extern void ProfileSignalVideoReadRequested();
	extern void ProfileSignalVideoReadCompleted();

	extern void ProfileSceneStarted( const Red::Char* sceneName );
	extern void ProfileSceneEnded();
	extern void ProfileSceneSectionStarted( const Red::Char* sectionName );
	extern void ProfileSceneSectionEnded();

	extern void ProfileStreamingLockAdded( const Red::Char* sectionName );
	extern void ProfileStreamingLockRemoved( const Red::Char* sectionName );

	extern void ProfileStreamingCameraPos( const Red::Float x, const Red::Float y, const Red::Float z );
	extern void ProfileStreamingCameraDist( const Red::Float dist );
	extern void ProfileStreamingEntity( const Red::Uint32 numInRange, const Red::Uint32 numStreaming, const Red::Uint32 numStreamed, const Red::Uint32 numLocked );
	extern void ProfileStreamingSector( const Red::Uint32 numInRange, const Red::Uint32 numStreaming, const Red::Uint32 numStreamed, const Red::Uint32 numLocked );

} // RedIOProfiler

#endif

#endif

/// Scoped user block
class RedIOProfilerBlock
{
public:
	RedIOProfilerBlock( const Red::Char* name )
	{
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileLoadingUserBlockStart( name );
#endif
	}

	~RedIOProfilerBlock()
	{
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileLoadingUserBlockEnd();
#endif
	}
};