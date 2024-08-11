/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

enum EDefragLockType : Uint64
{
	DLT_None				= 0,
	DLT_LockMeshesLoading	= 1,
	DLT_LockTexturesLoading = 2
};

class CDefragTask
{
public:
	// NOTE : The "maxToMove" stuff is how much should be moved from the memory system's point of view. More may need to
	// be transfered during a single run, depending on "scratch buffer" usage (overlap in move operations).
	static const Uint32 DEFAULT_APPROX_MAX_TO_MOVE	= 16 * 1024 * 1024;
	static const Uint32 DEFAULT_BLOCK_SIZE_LIMIT	= 12 * 1024 * 1024;

public:
	CDefragTask( Red::MemoryFramework::GpuAllocatorThreadSafe* pool, Red::MemoryFramework::GpuAllocatorThreadSafe::DefragMode mode, const String& taskName, Uint32 approxMaxToMove = DEFAULT_APPROX_MAX_TO_MOVE, Uint32 blockSizeLimit = DEFAULT_BLOCK_SIZE_LIMIT )
		: m_targetPool( pool )
		, m_mode( mode )
		, m_approxMaxToMove( approxMaxToMove )
		, m_blockSizeLimit( blockSizeLimit )
		, m_ticksSinceDefrag( -1 )
		, m_taskName( taskName )
		, m_nextStartingPoint( 0 )
	{
	}

	virtual ~CDefragTask() {}

	// Lock/unlock any systems that might use the defragmented memory
	virtual Bool OnCheckAndPrepare() { return false; }
	virtual void OnPostDefrag()	{}
	virtual void OnPostNothingToMove() {}
	virtual EDefragLockType GetLockType() const { return DLT_None; }

	Red::MemoryFramework::GpuAllocatorThreadSafe*				GetPool() const				{ return m_targetPool; }
	Red::MemoryFramework::GpuAllocatorThreadSafe::DefragMode	GetDefragMode() const		{ return m_mode; } 
	Uint32														GetApproxMaxToMove() const	{ return m_approxMaxToMove; }
	Uint32														GetBlockSizeLimit() const	{ return m_blockSizeLimit; }
	const String&												GetTaskName() const			{ return m_taskName; }


	Red::System::MemUint GetStartingPoint() const { return m_nextStartingPoint; }
	void SetNextStartingPoint( Red::System::MemUint startingPoint ) { m_nextStartingPoint = startingPoint; }

private:
	Red::MemoryFramework::GpuAllocatorThreadSafe*				m_targetPool;		// The pool to be defragged
	Red::MemoryFramework::GpuAllocatorThreadSafe::DefragMode	m_mode;				// Is it long lived, short lived, or both parts?

	Uint32														m_approxMaxToMove;	// No more than this amount of memory should be moved in one go
	Uint32														m_blockSizeLimit;	// Blocks bigger than this will not be moved, even if approxMaxToMove is not reached
	Int32														m_ticksSinceDefrag; // How many ticks ago did we run this task?
	String														m_taskName;			// Identifier to know which task is running
	Red::System::MemUint										m_nextStartingPoint;
};

class CRenderDefragHelper
{
public:
	CRenderDefragHelper( Red::MemoryFramework::MemoryRegionHandle dmaScratchRegion );
	~CRenderDefragHelper();

	// If there is a previous defrag running, check if it can be finished. Returns false if moves are still in flight,
	// true if there are no active moves.
	Bool TryFinishDefrag();

	// Pick next defrag task in the array and try to run it
	Bool Tick( Float timeDelta, Bool unrestricted );

	// Add task to be periodically run by the helper
	void AppendTask( CDefragTask* defragTask );

private:
	Bool TryOneTask();
	Bool RunNow( CDefragTask* task );

	void PerformCoalescedTransfersForBottom();
	void PerformCoalescedTransfersForTop();
	void AddMemoryMoveToQueue( void* src, void* dest, MemSize size, GpuApi::SDMABatchElement* batch, Uint32& batchsize );
	void Finalise();
	static void OnMoveRequest( void* src, void* dest, MemSize size, void* userData );

	struct SMoveRequest
	{
		SMoveRequest( void* src, void* dest, MemSize size )
			: m_sourceAddress( reinterpret_cast< MemUint >( src ) )
			, m_sourceSize( size )
			, m_destAddress( reinterpret_cast< MemUint >( dest ) )
		{
		}
		MemUint m_sourceAddress;
		MemSize m_sourceSize;
		MemUint m_destAddress;
	};
	Uint32											m_movesCompleted;
	MemSize											m_bytesMoved;
	Uint32											m_actualMovesCompleted;
	MemSize											m_actualBytesMoved;
	TDynArray< SMoveRequest >						m_moveRequests;
	Red::MemoryFramework::GpuAllocatorThreadSafe*	m_currentTargetPool;

private:
	Red::MemoryFramework::MemoryRegionHandle		m_defragScratch;
	TDynArray< CDefragTask* >						m_tasks;
	Int32											m_lastTaskIndex;
	Uint32											m_framesToNextDefrag;
	Bool											m_isUnrestrictedTick;
	volatile Uint64*								m_defragLock;

	Bool											m_needsToFinishLastTask;
};

class CMeshDefragTask : public CDefragTask
{
public:
	CMeshDefragTask( Red::MemoryFramework::GpuAllocatorThreadSafe* pool, Red::MemoryFramework::GpuAllocatorThreadSafe::DefragMode mode )
		: CDefragTask( pool, mode, TXT("Meshes defrag") )
	{}

	virtual Bool OnCheckAndPrepare() override { return true; }
	virtual EDefragLockType GetLockType() const override { return DLT_LockMeshesLoading; }
};

class CStreamedAndResidentTexturesDefragTask : public CDefragTask
{
private:
	// Texture defrag tends to run a lot more than meshes, so we do less at a time, as well as a lower limit
	// on maximum amount in a single move.
	static const Uint32 TEX_APPROX_MAX_TO_MOVE	= 8 * 1024 * 1024;
	static const Uint32 TEX_BLOCK_SIZE_LIMIT	= 8 * 1024 * 1024;

public:
	CStreamedAndResidentTexturesDefragTask( Red::MemoryFramework::GpuAllocatorThreadSafe* pool, Red::MemoryFramework::GpuAllocatorThreadSafe::DefragMode mode )
		: CDefragTask( pool, mode, TXT("StreamedTextures defrag"), TEX_APPROX_MAX_TO_MOVE, TEX_BLOCK_SIZE_LIMIT )
	{}

	// Lock/unlock any systems that might use the defragmented memory
	virtual Bool OnCheckAndPrepare() override { return true; }
	virtual EDefragLockType GetLockType() const override { return DLT_LockTexturesLoading; }
};

#if 0
class CResidentTexturesDefragTask : public CDefragTask
{
public:
	CResidentTexturesDefragTask( Red::MemoryFramework::GpuAllocatorThreadSafe* pool, Red::MemoryFramework::GpuAllocatorThreadSafe::DefragMode mode )
		: CDefragTask( pool, mode, TXT("ResidentTextures defrag") )
	{}

	// Lock/unlock any systems that might use the defragmented memory
	virtual Bool OnCheckAndPrepare() override { return true; }
	virtual EDefragLockType GetLockType() const { return DLT_LockTexturesLoading; }
};
#endif