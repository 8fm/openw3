/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderDefragHelper.h"
#include "renderTextureStreaming.h"
#include "renderScaleformTexture.h"
#include "renderInterface.h"
#include "renderEnvProbeManager.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redMemoryFramework/redMemoryGpuAllocator.h"
#include "../engine/renderScaleform.h"
#include "../core/fileDecompression.h"


// If defrag takes longer than this, it'll get logged. Somewhat arbitrary, but may be useful to see if an empty defrag is
// taking a while to get through.
#define DEFRAG_LOG_SPAM_LIMIT 0.2f

#define COALESCED_DEFRAG

CRenderDefragHelper::CRenderDefragHelper( Red::MemoryFramework::MemoryRegionHandle dmaScratchRegion )
	: m_movesCompleted( 0 )
	, m_bytesMoved( 0 )
	, m_actualMovesCompleted( 0 )
	, m_actualBytesMoved( 0 )
	, m_currentTargetPool( nullptr )
	, m_defragScratch( dmaScratchRegion )
	, m_lastTaskIndex( -1 )
	, m_framesToNextDefrag( 1 )
	, m_needsToFinishLastTask( false )
{
	m_moveRequests.Reserve( 4096 );
	m_defragLock = GpuApi::AllocateValue();
}

CRenderDefragHelper::~CRenderDefragHelper()
{
	if ( m_defragScratch.IsValid() )
	{
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_DefragTemp, m_defragScratch );
	}

	for ( Uint32 i=0; i<m_tasks.Size(); ++i )
	{
		delete m_tasks[i];
	}

	volatile Int32 waitCounter = 0;
	while( *m_defragLock != 0 )
	{
		waitCounter++;
	}
	GpuApi::DeallocateValue( m_defragLock );
}

Bool CRenderDefragHelper::RunNow( CDefragTask* task )
{
	m_currentTargetPool = task->GetPool();
	const Double totalTimeStart = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

	const auto mode = task->GetDefragMode();

	PC_SCOPE_PIX_NAMED( task->GetTaskName().AsChar() );

	// First pass - collect movement requests from defragmentation
	{
		PC_CHECKER_SCOPE( 0.001f, TXT("Defragmentation"), TXT("Collect move requests") );
		const Uint32 UNRESTRICTED_LIMIT = 1024 * 1024 * 1024;

		Red::MemoryFramework::GpuAllocatorThreadSafe::DefragSettings settings;
		settings.moveRequestCb		= OnMoveRequest;
		settings.userData			= this;
		settings.startingPoint		= task->GetStartingPoint();
		settings.maxMemoryToMove	= m_isUnrestrictedTick ? UNRESTRICTED_LIMIT : task->GetApproxMaxToMove();
		settings.blockSizeLimit		= m_isUnrestrictedTick ? UNRESTRICTED_LIMIT : task->GetBlockSizeLimit();

		auto newStartingPoint = m_currentTargetPool->RequestDefragmentation( mode, settings );
		task->SetNextStartingPoint( newStartingPoint );
	}

	// Second pass - coalesce move requests and perform transfers
	if ( !m_moveRequests.Empty() )
	{
		PC_CHECKER_SCOPE( 0.001f, TXT("Defragmentation"), TXT("Merge requests and perform transfer") );
		if( mode == Red::MemoryFramework::GpuAllocatorThreadSafe::DefragShortLivedRegions )
		{
			PerformCoalescedTransfersForBottom();
		}
		else if ( mode == Red::MemoryFramework::GpuAllocatorThreadSafe::DefragLongLivedRegions )
		{
			PerformCoalescedTransfersForTop();
		}
		else
		{
			RED_HALT( "Defragging of both long lived and short lived allocations at once is not supported!" );
		}

		// Need to call AfterDefragmentationMemoryFinished when the moves are done
		m_needsToFinishLastTask = true;
	}

	// Final pass - finalise
	{
		PC_CHECKER_SCOPE( 0.001f, TXT("Defragmentation"), TXT("Finalise") );
		Finalise();
	}

	const Float totalTimeElapsed = (Float)( Red::System::Clock::GetInstance().GetTimer().GetSeconds() - totalTimeStart ) * 1000.0f;
	// If it moved anything, or took longer than the spam limit, log
	if ( m_bytesMoved > 0 || totalTimeElapsed >= DEFRAG_LOG_SPAM_LIMIT )
	{
		LOG_RENDERER( TXT( "-- %ls took %.2fms. Moved %d blocks [%d bytes] with %d separate copy requests [%d bytes total]" ), task->GetTaskName().AsChar(), totalTimeElapsed,
			m_movesCompleted, (Uint32)m_bytesMoved, m_actualMovesCompleted, (Uint32)m_actualBytesMoved );
	}

	const Bool result = m_movesCompleted > 0;
	m_movesCompleted = 0;
	m_bytesMoved = 0;
	m_actualMovesCompleted = 0;
	m_actualBytesMoved = 0;

	return result;
}

void CRenderDefragHelper::OnMoveRequest( void* src, void* dest, MemSize size, void* userData )
{
	CRenderDefragHelper* helper = reinterpret_cast< CRenderDefragHelper* >( userData );
	RED_FATAL_ASSERT( helper, "no helper" );

	helper->m_moveRequests.PushBack( SMoveRequest( src, dest, size ) );
}

// Coalesce for when data is moving upwards towards top of heap
void CRenderDefragHelper::PerformCoalescedTransfersForTop()
{
	GpuApi::SDMABatchElement batchElements[ 4096 ];
	Uint32 batchElementCount = 0;

	// Sort by source address descending to ease merging
	auto sortTransfersBySourceAddress = []( const SMoveRequest& a, const SMoveRequest& b )
	{
		return a.m_sourceAddress > b.m_sourceAddress;
	};
	Sort( m_moveRequests.Begin(), m_moveRequests.End(), sortTransfersBySourceAddress );

#ifndef COALESCED_DEFRAG
	const Uint32 numRequests = m_moveRequests.Size();
	for( Uint32 transferIndex = 0; transferIndex < numRequests; ++transferIndex )
	{
		const auto& thisRequest = m_moveRequests[transferIndex];
		AddMemoryMoveToQueue( reinterpret_cast< void* >( thisRequest.m_sourceAddress ), reinterpret_cast< void* >( thisRequest.m_destAddress ), thisRequest.m_sourceSize, batchElements, batchElementCount );

		if (batchElementCount > 4090)
		{
			GpuApi::BatchedDMAMemory( batchElements, batchElementCount, true );
			batchElementCount = 0;
		}
	}
#else
	// Merge and move
	const Uint32 numRequests = m_moveRequests.Size();
	MemUint moveBaseAddr = 0;
	MemUint moveDestAddr = 0;
	MemSize moveSizeAccumulator = 0;

	for( Uint32 transferIndex = 0; transferIndex < numRequests; ++transferIndex )
	{
		const auto& thisRequest = m_moveRequests[transferIndex];
		if( moveBaseAddr == 0 )	// No moves queued
		{
			moveBaseAddr = thisRequest.m_sourceAddress;
			moveDestAddr = thisRequest.m_destAddress;
			moveSizeAccumulator = thisRequest.m_sourceSize;
		}
		else if( (moveBaseAddr - moveSizeAccumulator == thisRequest.m_sourceAddress) && ( moveDestAddr - moveSizeAccumulator == thisRequest.m_destAddress ) )	// contiguous blocks, ensure destinations are also contiguous
		{
			moveSizeAccumulator += thisRequest.m_sourceSize;
		}
		else if( moveBaseAddr != 0 ) // c..c...c....combo breaker!
		{
			AddMemoryMoveToQueue( reinterpret_cast< void* >( moveBaseAddr ), reinterpret_cast< void* >( moveDestAddr ), moveSizeAccumulator, batchElements, batchElementCount );
			moveBaseAddr = thisRequest.m_sourceAddress;
			moveDestAddr = thisRequest.m_destAddress;
			moveSizeAccumulator = thisRequest.m_sourceSize;

			if (batchElementCount > 4090)
			{
				GpuApi::BatchedDMAMemory( batchElements, batchElementCount, true );
				batchElementCount = 0;
			}
		}
	}

	if( moveBaseAddr != 0 )		// Handle any un-handled moves
	{
		AddMemoryMoveToQueue( reinterpret_cast< void* >( moveBaseAddr ), reinterpret_cast< void* >( moveDestAddr ), moveSizeAccumulator, batchElements, batchElementCount );
	}
#endif

	if (batchElementCount>0)
	{
		GpuApi::BatchedDMAMemory( batchElements, batchElementCount, true );
	}
}

// Coalesce for when data is moving downwards towards bottom of heap
void CRenderDefragHelper::PerformCoalescedTransfersForBottom()
{
#ifdef RED_ASSERTS_ENABLED
	// Used to have a sort here, to put the requests in order by source address, I guess to make it more likely that we
	// can combine the copies. Rearranging the requests is dangerous, because later requests may overlap with previous
	// ones. But we shouldn't ever have out of order requests anyways because the defrag is scanning through memory in
	// order. Just confirming it here.
	for( Uint32 i = 1; i < m_moveRequests.Size(); ++i )
	{
		RED_ASSERT( m_moveRequests[i-1].m_sourceAddress < m_moveRequests[i].m_sourceAddress, TXT("Move requests are not in order! Probably not terrible, but unexpected!") );
	}
#endif


	GpuApi::SDMABatchElement batchElements[ 4096 ];
	Uint32 batchElementCount = 0;

	// Merge and move
	const Uint32 numRequests = m_moveRequests.Size();
	MemUint moveBaseAddr = 0;
	MemUint moveDestAddr = 0;
	MemSize moveSizeAccumulator = 0;
	for( Uint32 transferIndex = 0; transferIndex < numRequests; ++transferIndex )
	{
		const auto& thisRequest = m_moveRequests[transferIndex];
		if( moveBaseAddr == 0 )	// No moves queued
		{
			moveBaseAddr = thisRequest.m_sourceAddress;
			moveDestAddr = thisRequest.m_destAddress;
			moveSizeAccumulator = thisRequest.m_sourceSize;
		}
		else if( (moveBaseAddr + moveSizeAccumulator == thisRequest.m_sourceAddress) && ( moveDestAddr + moveSizeAccumulator == thisRequest.m_destAddress ) )	// contiguous blocks, ensure destinations are also contiguous
		{
			moveSizeAccumulator += thisRequest.m_sourceSize;
		}
		else if( moveBaseAddr != 0 ) // c..c...c....combo breaker!
		{
			RED_ASSERT( moveBaseAddr > moveDestAddr, TXT("[Defrag] DefragBottom moving stuff forward, this should never happen") );
			if ( moveBaseAddr < moveDestAddr && moveSizeAccumulator > m_defragScratch.GetSize() )
			{
				//OutputDebugString( TXT("[Defrag] Moving more than scratch pool size, and moving it forward this will result in corruption!\n") );
				RED_LOG( Defrag, TXT("Moving more than scratch pool size, and moving it forward this will result in corruption!") );
			}

			AddMemoryMoveToQueue( reinterpret_cast< void* >( moveBaseAddr ), reinterpret_cast< void* >( moveDestAddr ), moveSizeAccumulator, batchElements, batchElementCount );
			moveBaseAddr = thisRequest.m_sourceAddress;
			moveDestAddr = thisRequest.m_destAddress;
			moveSizeAccumulator = thisRequest.m_sourceSize;

			if (batchElementCount > 4090)
			{
				GpuApi::BatchedDMAMemory( batchElements, batchElementCount, true );
				batchElementCount = 0;
			}
		}
	}

	if( moveBaseAddr != 0 )		// Handle any un-handled moves
	{
		AddMemoryMoveToQueue( reinterpret_cast< void* >( moveBaseAddr ), reinterpret_cast< void* >( moveDestAddr ), moveSizeAccumulator, batchElements, batchElementCount );
	}

	if (batchElementCount>0)
	{
		GpuApi::BatchedDMAMemory( batchElements, batchElementCount, true );
	}
}

void CRenderDefragHelper::AddMemoryMoveToQueue( void* src, void* dest, MemSize size, GpuApi::SDMABatchElement* batch, Uint32& batchsize )
{
	// DMA cannot handle overlapping requests
	MemUint destAddr = reinterpret_cast< MemUint >( dest );
	MemUint srcAddr = reinterpret_cast< MemUint >( src );
	Bool addressesOverlap = false;

	RED_FATAL_ASSERT( src != dest, "No need for move, why is this here?" );

	Bool addressesOverlapBackwards = false;
	Bool addressesOverlapForwards  = false;

	MemSize actuallyMoved = 0;
	Uint32 numBatches = 0;

	if( destAddr > srcAddr )
	{
		addressesOverlapForwards = ( destAddr - srcAddr ) < size;
	}
	else
	{
		addressesOverlapBackwards = ( srcAddr - destAddr ) < size;
	}

	// Check for overlaps
	if( addressesOverlapBackwards )
	{
		// Only use scratch memory if the gap is smaller than this limit. Otherwise, just copy chunks directly.
		const MemUint scratchLimit = m_defragScratch.GetSize() / 2;
		const MemUint gapSize = srcAddr - destAddr;

		const MemUint maxMoveSize = ( gapSize < scratchLimit ? m_defragScratch.GetSize() : gapSize );

		MemUint bytesMoved = 0;
		while( bytesMoved < size )
		{
			// How much do we move ?  Either the remaining amount, or the scratch size, whichever is smallest
			MemUint bytesToMove = Red::Math::NumericalUtils::Min( size - bytesMoved, maxMoveSize );

			// Start from the beginning of the resource, as we have an overlap move backwards
			void* srcPtr = reinterpret_cast< void* >( srcAddr  + bytesMoved );
			void* dstPtr = reinterpret_cast< void* >( destAddr + bytesMoved );

			// If the chunk we're moving is smaller than the gap, we can move it directly.
			if ( bytesToMove <= gapSize )
			{
				// Add to the batch
				batch[batchsize].Set( dstPtr, srcPtr, (Uint32)bytesToMove );
				batchsize++;

				// Increment the total
				bytesMoved += bytesToMove;
				actuallyMoved += bytesToMove;
			}
			// Otherwise need to use the scratch buffer.
			else
			{
				// Add to the batch
				batch[batchsize].Set( m_defragScratch.GetRawPtr(), srcPtr, (Uint32)bytesToMove );
				batchsize++;

				batch[batchsize].Set( dstPtr, m_defragScratch.GetRawPtr(), (Uint32)bytesToMove );
				batchsize++;

				// Increment the total
				bytesMoved += bytesToMove;

				actuallyMoved += 2*bytesToMove;
				numBatches += 2;
			}
		}
	}
	else if ( addressesOverlapForwards )
	{
		// We need to use the scratch memory to move. We also have to handle moves > scratch size
		MemUint bytesMoved = 0;
		while( bytesMoved < size )
		{
			// How much do we move ?  Either the remaining amount, or the scratch size, whichever is smallest
			MemUint bytesToMove = Red::Math::NumericalUtils::Min( size - bytesMoved, m_defragScratch.GetSize() );

			// Start from the end of the resource, as we have an overlap move forwards, to prevent overlap
			void* srcPtr = reinterpret_cast< void* >( srcAddr  + size - bytesToMove - bytesMoved );
			void* dstPtr = reinterpret_cast< void* >( destAddr + size - bytesToMove - bytesMoved );

			// Add to the batch
			batch[batchsize].Set( m_defragScratch.GetRawPtr(), srcPtr, (Uint32)bytesToMove );
			batchsize++;

			batch[batchsize].Set( reinterpret_cast< void* >( reinterpret_cast< MemUint >( dest ) + bytesMoved ), m_defragScratch.GetRawPtr(), (Uint32)bytesToMove );
			batchsize++;

			// Increment the total
			bytesMoved += bytesToMove;

			actuallyMoved += 2*bytesToMove;
			numBatches += 2;
		}
	}
	else
	{
		// No overlap, so just copy
		batch[batchsize].Set( dest, src, (Uint32)size );
		batchsize++;

		actuallyMoved += size;
		numBatches++;
	}

	m_movesCompleted++;
	m_bytesMoved += size;

	m_actualMovesCompleted += numBatches;
	m_actualBytesMoved += actuallyMoved;
}

void CRenderDefragHelper::Finalise()
{
	m_currentTargetPool->FinaliseDefragmentation();
	m_moveRequests.ClearFast();
}

void CRenderDefragHelper::AppendTask(CDefragTask* defragTask)
{
	RED_FATAL_ASSERT( !m_tasks.Exist( defragTask ), "Adding the same defrag task multiple times!" );
	m_tasks.PushBack( defragTask );
}

Bool CRenderDefragHelper::TryOneTask()
{
	if ( m_tasks.Empty() )
	{
		return false;
	}

	Bool didActualWork = false;

	// Pick next task to run
	const Uint32 currentTaskIndex = (m_lastTaskIndex + 1) % m_tasks.Size();
	CDefragTask* defragTask = m_tasks[ currentTaskIndex ];
	RED_FATAL_ASSERT( defragTask, "Null defrag task in the task list!" );
	
	// See if it is safe to run it
	if ( defragTask->OnCheckAndPrepare() )
	{
		RED_FATAL_ASSERT( *m_defragLock == 0, "Defrag lock is still switched on when making another run. This is bad! Debug!" );

		// Lock the defrag lock
		*m_defragLock = (Uint64)defragTask->GetLockType(); 

		if ( RunNow( defragTask ) )
		{
			// We did schedule some moves
			defragTask->OnPostDefrag();

			// Also make the GPU unlock the defrag lock after it is done processing the DMA moves
			GpuApi::InsertWriteToMemory( m_defragLock, 0 );

			didActualWork = true;
		}
		else
		{
			// We didn't do anything, give the task an opportunity to revert some state changed in OnCheckAndPrepare
			defragTask->OnPostNothingToMove();

			// Unlock, don't need it
			*m_defragLock = 0;
		}
	}

	m_lastTaskIndex = currentTaskIndex;
	return didActualWork;
}


Bool CRenderDefragHelper::TryFinishDefrag()
{
	if ( m_needsToFinishLastTask && *m_defragLock == 0 )
	{
		PC_SCOPE_PIX( PostDefragUnlock );

		RED_FATAL_ASSERT( m_lastTaskIndex >= 0 && m_lastTaskIndex < m_tasks.SizeInt(), "Invalid task index: %d", m_lastTaskIndex );
		CDefragTask* lastTask = m_tasks[ m_lastTaskIndex ];
		lastTask->GetPool()->AfterDefragmentationMemoryFinished( lastTask->GetDefragMode() );
		m_needsToFinishLastTask = false;
	}

	return !m_needsToFinishLastTask;
}


Bool CRenderDefragHelper::Tick( Float timeDelta, Bool unrestricted )
{
	Bool result = false;

	// Set the unrestricted flag for this tick
	m_isUnrestrictedTick = unrestricted;

	if ( --m_framesToNextDefrag == 0 )
	{
		if ( m_needsToFinishLastTask )
		{
			RED_LOG_WARNING( Defrag, TXT("Can't run defrag yet, still waiting for previous one!") );
			m_framesToNextDefrag = 1;
			return false;
		}

		PC_SCOPE_RENDER_LVL1( Defrag );

		// Defrag now!
		if ( TryOneTask() )
		{
			// Since we did actual moves, that costs, and we don't want it to happen again soon
			m_framesToNextDefrag = 15;
			result = true;
		}
		else
		{
			// If we didn't move anything, it's a good idea to let another task try soon
			m_framesToNextDefrag = 5;
			result = false;
		}
	}

	// Reset the unrestricted flag
	m_isUnrestrictedTick = false;

	return result;
}
