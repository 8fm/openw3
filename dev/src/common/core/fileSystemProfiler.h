/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _FILESYS_PROFILER_H__
#define _FILESYS_PROFILER_H__

// profile the file system
#if !defined(RED_FINAL_BUILD) || defined(RED_PROFILE_BUILD)
	#define RED_PROFILE_FILE_SYSTEM
#endif

#include "hashset.h"
#include "profilerBlockFileWriter.h"

/// Profiler for file system
class CFileManagerProfiler
{
public:
	CFileManagerProfiler();
	~CFileManagerProfiler();

	// counter types
	enum ECounterType
	{
		eCounterType_MemoryBlocks = 1,
		eCounterType_DecompressionTasks = 3,
		eCounterType_PendingFiles = 4,
		eCounterType_CachedDataBlocks = 5,
		eCounterType_CachedDataSize = 6,
	};

	// general signals
	enum ESignalType
	{
		eSignalType_FileAsyncReadScheduled = 1,
		eSignalType_LoadingScreenOn = 2,
		eSignalType_LoadingScreenOff = 3,
		eSignalType_BlackScreenOn = 4,
		eSignalType_BlackScreenOff = 5,
		eSignalType_VideoPlay = 6,
		eSignalType_VideoStop = 7,
		eSignalType_VideoReadRequested = 8,
		eSignalType_VideoReadCompleted = 9,
		eSignalType_LoadingProfilerBlock = 10,
		eSignalType_LoadingProfilerEnd = 11,
		eSignalType_StreamingCameraPos = 12,
		eSignalType_StreamingCameraDist = 13,
		eSignalType_StreamingEntityStats = 14,
		eSignalType_StreamingSectorStats = 15,
	};

	// general block types (single param)
	enum EBlockType
	{
		eBlockType_FileSyncOpen = 1,
		eBlockType_FileSyncClose = 2,
		eBlockType_FileAsyncOpen = 3,
		eBlockType_FileAsyncClose = 4,
		eBlockType_FileAsyncRead = 5,
		eBlockType_FileSyncRead = 6,
		eBlockType_FileSyncSeek = 7,
	
		eBlockType_LoadResource = 10,
		eBlockType_SyncLoadData = 11,
		eBlockType_SyncDecompression = 12,
		eBlockType_AsyncDecompression = 13,
		eBlockType_Deserialize = 14,
		eBlockType_LoadTables = 15,
		eBlockType_MapTables = 16,
		eBlockType_CreateObjects = 17,
		eBlockType_LoadImports = 18,
		eBlockType_LoadObjects = 19,
		eBlockType_PostLoad = 20,
		eBlockType_DeferredSyncRead = 21,
		eBlockType_CachedRead = 22,
		eBlockType_LoadInplace = 23,
		eBlockType_LoadingJob = 24,
		eBlockType_UserProfileBlock = 25,

		eBlockType_Scene = 30,
		eBlockType_SceneSection = 31,
		eBlockType_StreamingLock = 32,
	};

	// General initialize
	void Initilize( const String& basePath );
	void Shutdown();

	// Start/Stop profiling
	void Start();
	void Stop();

	// Flush current profiling
	void Flush();
	void ConditionalFlushOnce();

	// Are we profiling ?
	const Bool IsProfiling() const;

	// Map string to hash
	const Uint32 MapFilePath( const String& path );
	const Uint32 MapFilePath( const StringAnsi& path );
	const Uint32 MapFilePath( const AnsiChar* path );
	const Uint32 MapFilePath( const Char* path );

	// Set thread name
	void SetThreadName( const AnsiChar* threadName );

	// Block start
	RED_FORCE_INLINE void Start( const EBlockType type );
	RED_FORCE_INLINE void Start( const EBlockType type, const Uint32 paramA );
	RED_FORCE_INLINE void Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB );
	RED_FORCE_INLINE void Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC );
	RED_FORCE_INLINE void Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD );
	RED_FORCE_INLINE void Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE );

	// Block stop
	RED_FORCE_INLINE void End( const EBlockType type );
	RED_FORCE_INLINE void End( const EBlockType type, const Uint32 paramA );
	RED_FORCE_INLINE void End( const EBlockType type, const Uint32 paramA, const Uint32 paramB );
	RED_FORCE_INLINE void End( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC );
	RED_FORCE_INLINE void End( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD );
	RED_FORCE_INLINE void End( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE );

	// Emit profiling signal
	RED_FORCE_INLINE void Signal( const ESignalType type );
	RED_FORCE_INLINE void Signal( const ESignalType type, const Uint32 paramA );
	RED_FORCE_INLINE void Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB );
	RED_FORCE_INLINE void Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC );
	RED_FORCE_INLINE void Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD );
	RED_FORCE_INLINE void Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE );
	RED_FORCE_INLINE void Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE, const Uint32 paramF );

	// Update counter value
	RED_FORCE_INLINE void CounterIncrement( const ECounterType counter, const Uint32 value ); 
	RED_FORCE_INLINE void CounterDecrement( const ECounterType counter, const Uint32 value );

private:
	String						m_basePath;
	CProfilerBlockFileWriter	m_writer;

	void AssembleFilePath( String& outAbsoluteFilePath ) const;
};

extern CFileManagerProfiler GFileManagerProfiler;

#include "fileSystemProfiler.inl"

#endif
