/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderScaleformMeshCache.h"
#include "renderScaleformTemp.h"
#include "renderScaleformShader.h"
#include "Kernel/SF_Alg.h"
#include "Kernel/SF_HeapNew.h"

// For SF_AMP_SCOPE_TIMER
using SF::AmpFunctionTimer;
using SF::AmpServer;

//#define SF_RENDER_LOG_CACHESIZE

// Helpers used to initialize default granularity sizes,
// splitting VB/Index size by 5/9.
inline SF::UPInt calcVBGranularity(SF::UPInt granularity)
{
	return (((granularity >> 4) * 5) / 9) << 4;
}
inline SF::UPInt calcIBGranularity(SF::UPInt granularity, SF::UPInt vbGranularity)
{
	return (((granularity >> 4) - (vbGranularity >> 4)) << 4);
}


// ***** MeshCache

CRenderScaleformMeshCache::CRenderScaleformMeshCache(SF::MemoryHeap* pheap, const SF::Render::MeshCacheParams& params)
	: SF::Render::MeshCache(pheap, params)
	, pShaderManager( 0 )
	, CacheList(getThis())
	, VertexBuffers(pheap, calcVBGranularity(params.MemGranularity))
	, IndexBuffers(pheap, calcIBGranularity(params.MemGranularity, VertexBuffers.GetGranularity()))
	, Locked(false)
	, VBSizeEvictedInLock(0)
{
}

CRenderScaleformMeshCache::~CRenderScaleformMeshCache()
{
	Reset();
}

// Initializes MeshCache for operation, including allocation of the reserve
// buffer. Typically called from SetVideoMode.
SFBool CRenderScaleformMeshCache::Initialize(CRenderScaleformShaderManager* psm)
{
	// NOTE: I believe the assert in the reference implementation mistakenly asserts "!psm", but passes
	// because of being OR'd with other assertions that are alright.
	ASSERT( ! pShaderManager );

	pShaderManager = psm;

	adjustMeshCacheParams(&Params);

	if (!StagingBuffer.Initialize(pHeap, Params.StagingBufferSize))
		return false;

	if (!createStaticVertexBuffers())
	{
		Reset();
		return false;
	}

	if (Params.MemReserve &&
		!allocCacheBuffers(Params.MemReserve, CRenderScaleformMeshBuffer::AT_Reserve))
	{
		Reset();
		return false;
	}

	return true;
}

void CRenderScaleformMeshCache::Reset()
{
	if ( GpuApi::IsInit() )
	{
		destroyBuffers();
	}

	// Unconditional to simplify Initialize fail logic:
	GpuApi::SafeRelease( m_maskEraseBatchVertexBuffer );
	StagingBuffer.Reset();
}


void CRenderScaleformMeshCache::ClearCache()
{
	destroyBuffers(CRenderScaleformMeshBuffer::AT_Chunk);
	StagingBuffer.Reset();
	StagingBuffer.Initialize(pHeap, Params.StagingBufferSize);
	SF_ASSERT(BatchCacheItemHash.GetSize() == 0);
}

void CRenderScaleformMeshCache::destroyBuffers(CRenderScaleformMeshBuffer::AllocType at)
{
	// TBD: Evict everything first!
	CacheList.EvictAll();
	VertexBuffers.DestroyBuffers(at);
	IndexBuffers.DestroyBuffers(at);
	ChunkBuffers.Clear();
}

SFBool CRenderScaleformMeshCache::SetParams(const SF::Render::MeshCacheParams& argParams)
{
	SF::Render::MeshCacheParams params(argParams);
	adjustMeshCacheParams(&params);

	if ( GpuApi::IsInit() )
	{
		CacheList.EvictAll();

		if (Params.StagingBufferSize != params.StagingBufferSize)
		{
			if (!StagingBuffer.Initialize(pHeap, params.StagingBufferSize))
			{
				if (!StagingBuffer.Initialize(pHeap, Params.StagingBufferSize))
				{
					HALT( "MeshCache::SetParams - couldn't restore StagingBuffer after fail" );
				}
				return false;
			}
		}

		if ((Params.MemReserve != params.MemReserve) ||
			(Params.MemGranularity != params.MemGranularity))
		{
			destroyBuffers();

			// Allocate new reserve. If not possible, restore previous one and fail.
			if (params.MemReserve &&
				!allocCacheBuffers(params.MemReserve, CRenderScaleformMeshBuffer::AT_Reserve))
			{
				if (Params.MemReserve &&
					!allocCacheBuffers(Params.MemReserve, CRenderScaleformMeshBuffer::AT_Reserve))
				{
					HALT( "MeshCache::SetParams - couldn't restore Reserve after fail" );
				}
				return false;
			}

			VertexBuffers.SetGranularity(calcVBGranularity(params.MemGranularity));
			IndexBuffers.SetGranularity(calcIBGranularity(params.MemGranularity,
				VertexBuffers.GetGranularity()));
		}
	}
	Params = params;
	return true;
}

void CRenderScaleformMeshCache::adjustMeshCacheParams(SF::Render::MeshCacheParams* p)
{
	p->MaxBatchInstances    = SF::Alg::Clamp<Uint32>(p->MaxBatchInstances, 1, SF_RENDER_MAX_BATCHES);
	p->VBLockEvictSizeLimit = SF::Alg::Clamp<SF::UPInt>(p->VBLockEvictSizeLimit, 0, p->VBLockEvictSizeLimit = 1024 * 256);

	SF::UPInt maxStagingItemSize = p->MaxVerticesSizeInBatch +
		sizeof(SF::UInt16) * p->MaxIndicesInBatch;
	if (maxStagingItemSize * 2 > p->StagingBufferSize)
		p->StagingBufferSize = maxStagingItemSize * 2;

	// If we have a shader manager, we can query whether we have instancing. If not, disable it.
	if (!pShaderManager->HasInstancingSupport())
		p->InstancingThreshold = 0;
}


void CRenderScaleformMeshCache::destroyPendingBuffers()
{
	// Destroy any pending buffers that are waiting to be destroyed (if possible).
	SF::List<SF::Render::MeshBuffer> remainingBuffers;
	CRenderScaleformMeshBuffer* p = (CRenderScaleformMeshBuffer*)PendingDestructionBuffers.GetFirst();
	while (!PendingDestructionBuffers.IsNull(p))
	{
		SF::Render::MeshCacheListSet::ListSlot& pendingFreeList = CacheList.GetSlot(SF::Render::MCL_PendingFree);
		CRenderScaleformMeshCacheItem* pitem = (CRenderScaleformMeshCacheItem*)pendingFreeList.GetFirst();
		Bool itemsRemaining = false;
		CRenderScaleformMeshBuffer* pNext = (CRenderScaleformMeshBuffer*)p->pNext;
		p->RemoveNode();
		while (!pendingFreeList.IsNull(pitem))
		{
			if ((pitem->pVertexBuffer == p) || (pitem->pIndexBuffer == p))
			{
				// If the fence is still pending, cannot destroy the buffer.
				if ( pitem->GPUFence && pitem->GPUFence->IsPending(SF::Render::FenceType_Vertex) )
				{
					itemsRemaining = true;
					remainingBuffers.PushFront(p);
					break;
				}
			}
			pitem = (CRenderScaleformMeshCacheItem*)pitem->pNext;
		}
		if ( !itemsRemaining )
		{
			delete p;
		}
		p = pNext;
	}
	PendingDestructionBuffers.PushListToFront(remainingBuffers);
}

void CRenderScaleformMeshCache::BeginFrame()
{
}

void CRenderScaleformMeshCache::EndFrame()
{
#ifndef RED_FINAL_BUILD
	SF_AMP_SCOPE_RENDER_TIMER(__FUNCTION__, SF::Amp_Profile_Level_Medium);
#endif

	CacheList.EndFrame();

	// Try and reclaim memory from items that have already been destroyed, but not freed.
	CacheList.EvictPendingFree(IndexBuffers.Allocator);
	CacheList.EvictPendingFree(VertexBuffers.Allocator);

	destroyPendingBuffers();


	// Simple Heuristic used to shrink cache. Shrink is possible once the
	// (Total_Frame_Size + LRUTailSize) exceed the allocated space by more then
	// one granularity unit. If this case, we destroy the cache buffer in the
	// order opposite to that of which it was created.

	// TBD: This may have a side effect of throwing away the current frame items
	// as well. Such effect is undesirable and can perhaps be avoided on consoles
	// with buffer data copies (copy PrevFrame content into other buffers before evict).

	SF::UPInt totalFrameSize = CacheList.GetSlotSize(SF::Render::MCL_PrevFrame);
	SF::UPInt lruTailSize    = CacheList.GetSlotSize(SF::Render::MCL_LRUTail);
	SF::UPInt expectedSize   = totalFrameSize + SF::Alg::PMin(lruTailSize, Params.LRUTailSize);
	expectedSize += expectedSize / 4; // + 25%, to account for fragmentation.

	SF::SPInt extraSpace = getTotalSize() - (SF::SPInt)expectedSize;
	if (extraSpace > (SF::SPInt)Params.MemGranularity)
	{        
		while (!ChunkBuffers.IsEmpty() && (extraSpace > (SF::SPInt)Params.MemGranularity))
		{
			CRenderScaleformMeshBuffer* p = (CRenderScaleformMeshBuffer*)ChunkBuffers.GetLast();
			p->RemoveNode();
			extraSpace -= (SF::SPInt)p->GetSize();

			CRenderScaleformMeshBufferSet&  mbs = (p->GetBufferType() == CRenderScaleformMeshBuffer::Buffer_Vertex) ?
				(CRenderScaleformMeshBufferSet&)VertexBuffers : (CRenderScaleformMeshBufferSet&)IndexBuffers;

			// Evict first! This may fail if a query is pending on a mesh inside the buffer. In that case,
			// simply store the buffer to be destroyed later.
			Bool allEvicted = evictMeshesInBuffer(CacheList.GetSlots(), SF::Render::MCL_ItemCount, p);
			mbs.DestroyBuffer(p, allEvicted);
			if ( !allEvicted )
				PendingDestructionBuffers.PushBack(p);


#ifdef SF_RENDER_LOG_CACHESIZE
			LogDebugMessage(Log_Message,
				"Cache shrank to %dK. Start FrameSize = %dK, LRU = %dK\n",
				getTotalSize() / 1024, totalFrameSize/1024, lruTailSize/1024);
#endif
		}
	}    
}



// Adds a fixed-size buffer to cache reserve; expected to be released at Release.
/*
bool    MeshCache::AddReserveBuffer(unsigned size, unsigned arena)
{
}
bool    MeshCache::ReleaseReserveBuffer(unsigned size, unsigned arena)
{
}
*/


// Allocates Vertex/Index buffer of specified size and adds it to free list.
SFBool CRenderScaleformMeshCache::allocCacheBuffers(SF::UPInt size, CRenderScaleformMeshBuffer::AllocType type, SFUInt arena)
{
	// Assign (5/9) of space for VB; need to experiment with fractions in the future.
	// Round to 16-byte units for now...   
	SF::UPInt vbsize = calcVBGranularity(size);
	SF::UPInt ibsize = calcIBGranularity(size, vbsize);

	CRenderScaleformVertexBuffer *pvb = (CRenderScaleformVertexBuffer*)VertexBuffers.CreateBuffer(vbsize, type, arena, pHeap);
	if (!pvb)
		return false;
	CRenderScaleformMeshBuffer   *pib = IndexBuffers.CreateBuffer(ibsize, type, arena, pHeap);
	if (!pib)
	{
		VertexBuffers.DestroyBuffer(pvb);
		return false;
	}
#ifdef SF_RENDER_LOG_CACHESIZE
	LogDebugMessage(Log_Message, "Cache grew to %dK\n", getTotalSize() / 1024);
#endif
	return true;
}


SFBool CRenderScaleformMeshCache::createStaticVertexBuffers()
{
	return createMaskEraseBatchVertexBuffer();
}

SFBool CRenderScaleformMeshCache::createMaskEraseBatchVertexBuffer()
{
	static const Uint32     bufferSize = sizeof(SF::Render::VertexXY16fAlpha) * 6 * SF_RENDER_MAX_BATCHES;
	SF::Render::VertexXY16fAlpha pbuffer[6 * SF_RENDER_MAX_BATCHES];
	fillMaskEraseVertexBuffer<SF::Render::VertexXY16fAlpha>(pbuffer, SF_RENDER_MAX_BATCHES);

	{
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = pbuffer;
		m_maskEraseBatchVertexBuffer = GpuApi::CreateBuffer( bufferSize, GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
	}
	
	GpuApi::SetBufferDebugPath( m_maskEraseBatchVertexBuffer, "maskEraseBatch" );
	return m_maskEraseBatchVertexBuffer != GpuApi::BufferRef::Null();
}

SFBool CRenderScaleformMeshCache::LockBuffers()
{
	ASSERT(!Locked);
	Locked = true;
	VBSizeEvictedInLock = 0;
	if (pRQCaches)
		pRQCaches->SetCacheLocked(SF::Render::Cache_Mesh);
	return true;
}

void CRenderScaleformMeshCache::UnlockBuffers()
{
	ASSERT(Locked != 0); 
	LockedBuffers.UnlockAll();
	Locked = false;
	if (pRQCaches)
		pRQCaches->ClearCacheLocked(SF::Render::Cache_Mesh);
}


SFBool CRenderScaleformMeshCache::evictMeshesInBuffer(SF::Render::MeshCacheListSet::ListSlot* plist, SF::UPInt count,
	CRenderScaleformMeshBuffer* pbuffer)
{
	Bool evictionFailed = false;
	for (Uint32 i=0; i< count; i++)
	{
		CRenderScaleformMeshCacheItem* pitem = (CRenderScaleformMeshCacheItem*)plist[i].GetFirst();
		while (!plist[i].IsNull(pitem))
		{
			if ((pitem->pVertexBuffer == pbuffer) || (pitem->pIndexBuffer == pbuffer))
			{
				// Evict returns the number of bytes released. If this is zero, it means the mesh
				// was still in use. 
				if ( Evict(pitem) == 0 )
				{
					evictionFailed = true;
					ASSERT(pitem->Type == SF::Render::MeshCacheItem::Mesh_Destroyed);

					// We still need to delete all the addresses allocated in the buffer, because it is 
					// going to be deleted, and AllocAddr will break otherwise.
					if ( pitem->pVertexBuffer == pbuffer)
					{
						VertexBuffers.Free(pitem->VBAllocSize, pitem->pVertexBuffer, pitem->VBAllocOffset);
						pitem->pVertexBuffer = 0;
					}
					if ( pitem->pIndexBuffer == pbuffer)
					{
						IndexBuffers.Free(pitem->IBAllocSize, pitem->pIndexBuffer, pitem->IBAllocOffset);
						pitem->pIndexBuffer = 0;
					}
				}

				// Evict may potentially modify the cache items, so start again.
				// This is less than ideal, but better than accessing a dangling pointer.
				pitem = (CRenderScaleformMeshCacheItem*)plist[i].GetFirst();
				continue;
			}
			pitem = (CRenderScaleformMeshCacheItem*)pitem->pNext;
		}
	}
	return !evictionFailed;
}


SF::UPInt CRenderScaleformMeshCache::Evict(SF::Render::MeshCacheItem* pbatch, SF::AllocAddr* pallocator, SF::Render::MeshBase* pskipMesh)
{
	CRenderScaleformMeshCacheItem* p = (CRenderScaleformMeshCacheItem*)pbatch;

	// If a fence is not pending, then the memory for the item can be reclaimed immediately.
	if ( !p->GPUFence || !p->GPUFence->IsPending(SF::Render::FenceType_Vertex))
	{
		// - Free allocator data.
		SF::UPInt vbfree = p->pVertexBuffer ? VertexBuffers.Free(p->VBAllocSize, p->pVertexBuffer, p->VBAllocOffset) : 0;
		SF::UPInt ibfree = p->pIndexBuffer  ? IndexBuffers.Free(p->IBAllocSize, p->pIndexBuffer, p->IBAllocOffset) : 0;
		SF::UPInt freedSize = pallocator ? ((&VertexBuffers.GetAllocator() == pallocator) ? vbfree : ibfree) : vbfree + ibfree;

		VBSizeEvictedInLock += (Uint32)  p->VBAllocSize;
		p->Destroy(pskipMesh, true);
		return freedSize;
	}
	else
	{
		// Still in use, push it on the pending to delete list.
		// It should be valid for this to be called multiple times for a single mesh (for example, in a PendingFree situation).
		p->Destroy(pskipMesh, false);
		CacheList.PushFront(SF::Render::MCL_PendingFree, p);
		return 0;
	}

}


// Allocates the buffer, while evicting LRU data.
SFBool CRenderScaleformMeshCache::allocBuffer(SF::UPInt* poffset, CRenderScaleformMeshBuffer** pbuffer,
	CRenderScaleformMeshBufferSet& mbs, SF::UPInt size, SFBool waitForCache)
{
	SF_UNUSED(waitForCache);

	if (mbs.Alloc(size, pbuffer, poffset))
		return true;

	// If allocation failed... need to apply swapping or grow buffer.
	CRenderScaleformMeshCacheItem* pitems;

	// #1. Try and reclaim memory from items that have already been destroyed, but not freed.
	//     These cannot be reused, so it is best to evict their memory first, if possible.
	if (CacheList.EvictPendingFree(mbs.Allocator))
	{
		//	goto alloc_size_available;
		if (!mbs.Alloc(size, pbuffer, poffset))
		{
			ASSERT(0);
			return false;
		}

		return true;
	}

	// #2. Then, apply LRU (least recently used) swapping from data stale in
	//    earlier frames until the total size 

	if ((getTotalSize() + MinSupportedGranularity) <= Params.MemLimit)
	{
		if (CacheList.EvictLRUTillLimit(SF::Render::MCL_LRUTail, mbs.GetAllocator(),
			size, Params.LRUTailSize))
		{
			// goto alloc_size_available;
			if (!mbs.Alloc(size, pbuffer, poffset))
			{
				ASSERT(0);
				return false;
			}

			return true;
		}

		// TBD: May cause spinning? Should we have two error codes?
		ASSERT(size <= mbs.GetGranularity());
		if (size > mbs.GetGranularity())
			return false;

		SF::UPInt allocSize = SF::Alg::PMin(Params.MemLimit - getTotalSize(), mbs.GetGranularity());
		if (size <= allocSize)
		{
			CRenderScaleformMeshBuffer* pbuff = mbs.CreateBuffer(allocSize, CRenderScaleformMeshBuffer::AT_Chunk, 0, pHeap);
			if (pbuff)
			{
				ChunkBuffers.PushBack(pbuff);
#ifdef SF_RENDER_LOG_CACHESIZE
				LogDebugMessage(Log_Message, "Cache grew to %dK\n", getTotalSize() / 1024);
#endif
				//goto alloc_size_available;
				if (!mbs.Alloc(size, pbuffer, poffset))
				{
					ASSERT(0);
					return false;
				}

				return true;
			}
		}
	}

	if (CacheList.EvictLRU(SF::Render::MCL_LRUTail, mbs.GetAllocator(), size)) 	//		goto alloc_size_available;
	{
		if (!mbs.Alloc(size, pbuffer, poffset))
		{
			ASSERT(0);
			return false;
		}

		return true;
	}

	if (VBSizeEvictedInLock > Params.VBLockEvictSizeLimit)
		return false;

	// #3. Apply MRU (most recently used) swapping to the current frame content.
	// NOTE: MRU (GetFirst(), pNext iteration) gives
	//       2x improvement here with "Stars" test swapping.
	SF::Render::MeshCacheListSet::ListSlot& prevFrameList = CacheList.GetSlot(SF::Render::MCL_PrevFrame);
	pitems = (CRenderScaleformMeshCacheItem*)prevFrameList.GetFirst();
	while(!prevFrameList.IsNull(pitems))
	{
		if (!pitems->GPUFence || !pitems->GPUFence->IsPending(SF::Render::FenceType_Vertex))
		{
			if (Evict(pitems, &mbs.GetAllocator()) >= size)
			{
				if (!mbs.Alloc(size, pbuffer, poffset))
				{
					ASSERT(0);
					return false;
				}

				return true;
			}

			// Get the first item in the list, because and the head of the list will now be different, due to eviction.
			pitems = (CRenderScaleformMeshCacheItem*)prevFrameList.GetFirst();
		}
		else
		{
			pitems = (CRenderScaleformMeshCacheItem*)prevFrameList.GetNext(pitems);
		}
	}

	// #4. If MRU swapping didn't work for ThisFrame items due to them still
	// being processed by the GPU and we are being asked to wait, wait
	// until fences are passed to evict items.
	SF::Render::MeshCacheListSet::ListSlot& thisFrameList = CacheList.GetSlot(SF::Render::MCL_ThisFrame);
	pitems = (CRenderScaleformMeshCacheItem*)thisFrameList.GetFirst();
	while(waitForCache && !thisFrameList.IsNull(pitems))
	{
		if ( pitems->GPUFence )
			pitems->GPUFence->WaitFence(SF::Render::FenceType_Vertex);
		if (Evict(pitems, &mbs.GetAllocator()) >= size)
			goto alloc_size_available;
		pitems = (CRenderScaleformMeshCacheItem*)thisFrameList.GetFirst();
	}
	return false;

	// At this point we know we have a large enough block either due to
	// swapping or buffer growth, so allocation shouldn't fail.
alloc_size_available:
	if (!mbs.Alloc(size, pbuffer, poffset))
	{
		ASSERT(0);
		return false;
	}

	return true;
}


// Generates meshes and uploads them to buffers.
SFBool CRenderScaleformMeshCache::PreparePrimitive(SF::Render::PrimitiveBatch* pbatch,
	CRenderScaleformMeshCacheItem::MeshContent &mc,
	SFBool waitForCache)
{
#ifndef RED_FINAL_BUILD
	SF_AMP_SCOPE_RENDER_TIMER(__FUNCTION__, SF::Amp_Profile_Level_Medium);
#endif

	SF::Render::Primitive* prim = pbatch->GetPrimitive();

	if (mc.IsLargeMesh())
	{
		ASSERT(mc.GetMeshCount() == 1);
		MeshResult mr = GenerateMesh(mc[0], prim->GetVertexFormat(),
			pbatch->pFormat, 0, waitForCache);

		if (mr.Succeded())
			pbatch->SetCacheItem(mc[0]->CacheItems[0]);
		// Return 'false' if we just need more cache, to flush and retry.
		if (mr == MeshResult::Fail_LargeMesh_NeedCache)
			return false;
		return true;
	}

	// Prepare and Pin mesh data with the StagingBuffer. NOTE: this must happen before calculating mesh sizes.
	// This stage updates the mesh vertex/index counts, so calculating them first, it could be incorrect, for
	// example in the case of MeshCache::ClearCache, and changing ToleranceParams.
	StagingBufferPrep   meshPrep(this, mc, prim->GetVertexFormat(), false);

	// NOTE: We always know that meshes in one batch fit into Mesh Staging Cache.
	Uint32 totalVertexCount, totalIndexCount;
	pbatch->CalcMeshSizes(&totalVertexCount, &totalIndexCount);

	ASSERT(pbatch->pFormat);

	SF::Render::MeshCacheItem* batchData = 0;
	Uint32		   destVertexSize = pbatch->pFormat->Size;
	SF::UByte*     pvertexDataStart;
	SF::Render::IndexType*     pindexDataStart;
	AllocResult    allocResult;

	allocResult = AllocCacheItem(&batchData, &pvertexDataStart, &pindexDataStart,
		CRenderScaleformMeshCacheItem::Mesh_Regular, mc,
		totalVertexCount * destVertexSize,
		totalVertexCount, totalIndexCount, waitForCache, 0);
	if (allocResult != Alloc_Success)
	{
		// Return 'true' for state error (we can't recover by swapping cache and re-trying).
		return (allocResult == Alloc_Fail) ? false : true;
	}

	pbatch->SetCacheItem(batchData);

	// This step either generates the mesh into the staging buffer, or locates an existing MeshCacheItem
	// that we can copy the mesh from. It must be done after creating the cache item, because that may
	// evict the item that would be copied from.
	meshPrep.GenerateMeshes(batchData);

	// Copy meshes into the Vertex/Index buffers.

	// All the meshes have been pinned, so we can
	// go through them and copy them into buffers
	SF::UByte*      pstagingBuffer   = StagingBuffer.GetBuffer();
	const SF::Render::VertexFormat* pvf      = prim->GetVertexFormat();
	const SF::Render::VertexFormat* pdvf     = pbatch->pFormat;

	Uint32    i;
	Uint32	indexStart = 0;

	for(i = 0; i< mc.GetMeshCount(); i++)
	{
		SF::Render::Mesh* pmesh = mc[i];
		ASSERT(pmesh->StagingBufferSize != 0);

		// Convert vertices and initialize them to the running index
		// within this primitive.
		void*   convertArgArray[1] = { &i };
		SF::Render::ConvertVertices_Buffered(*pvf, pstagingBuffer + pmesh->StagingBufferOffset,
			*pdvf, pvertexDataStart,
			pmesh->VertexCount, &convertArgArray[0]);
		// Copy and assign indices.
		SF::Render::ConvertIndices(pindexDataStart,
			(SF::Render::IndexType*)(pstagingBuffer + pmesh->StagingBufferIndexOffset),
			pmesh->IndexCount, (SF::Render::IndexType)indexStart);

		pvertexDataStart += pmesh->VertexCount * destVertexSize;
		pindexDataStart  += pmesh->IndexCount;
		indexStart       += pmesh->VertexCount;
	}

	// ~StagingBufferPrep will Unpin meshes in the staging buffer.
	return true;
}

CRenderScaleformMeshCache::AllocResult
	CRenderScaleformMeshCache::AllocCacheItem(SF::Render::MeshCacheItem** pdata,
	SF::UByte** pvertexDataStart, SF::Render::IndexType** pindexDataStart,
	CRenderScaleformMeshCacheItem::MeshType meshType,
	CRenderScaleformMeshCacheItem::MeshBaseContent &mc,
	SF::UPInt vertexBufferSize,
	SFUInt vertexCount, SFUInt indexCount,
	SFBool waitForCache, const SF::Render::VertexFormat*)
{

	if (!AreBuffersLocked() && !LockBuffers())
		return Alloc_StateError;

	// Compute and allocate appropriate VB/IB space.
	SF::UPInt       vbOffset = 0, ibOffset = 0;
	CRenderScaleformMeshBuffer  *pvb = 0, *pib = 0;
	SF::UByte       *pvdata, *pidata;
	CRenderScaleformMeshCache::AllocResult failType = Alloc_Fail;

	if (!allocBuffer(&vbOffset, &pvb, VertexBuffers,
		vertexBufferSize, waitForCache))
	{
handle_alloc_fail:
		if (pvb) VertexBuffers.Free(vertexBufferSize, pvb, vbOffset);
		if (pib) IndexBuffers.Free(indexCount * sizeof(SF::Render::IndexType), pib, ibOffset);        
		return failType;
	}
	if (!allocBuffer(&ibOffset, &pib, IndexBuffers,
		indexCount * sizeof(SF::Render::IndexType), waitForCache))
		goto handle_alloc_fail;

	pvdata = pvb->Lock(LockedBuffers);
	pidata = pib->Lock(LockedBuffers);

	if (!pvdata || !pidata)
		goto handle_alloc_fail;

	// Create new MeshCacheItem; add it to hash.
	*pdata = CRenderScaleformMeshCacheItem::Create(meshType,
		&CacheList, mc, (CRenderScaleformVertexBuffer*)pvb, (CRenderScaleformIndexBuffer*)pib,
		(Uint32)vbOffset, vertexBufferSize, vertexCount,
		(Uint32)ibOffset, indexCount * sizeof(SF::Render::IndexType), indexCount);

	if (!*pdata)
	{
		// Memory error; free buffers, skip mesh.
		ASSERT(0);
		failType = Alloc_StateError;
		goto handle_alloc_fail;
	}

	*pvertexDataStart = pvdata + vbOffset;
	*pindexDataStart  = (SF::Render::IndexType*)(pidata + ibOffset);
	return Alloc_Success;
}

void CRenderScaleformMeshCache::GetStats(Stats* stats)
{
	*stats = Stats();
	Uint32 memType = MeshBuffer_GpuMem;

	stats->TotalSize[memType + MeshBuffer_Vertex] = VertexBuffers.GetTotalSize();
	stats->UsedSize[memType + MeshBuffer_Vertex] = VertexBuffers.Allocator.GetFreeSize() << GpuApiMeshCache::MeshCache_AllocatorUnitShift;

	stats->TotalSize[memType + MeshBuffer_Index] = IndexBuffers.GetTotalSize();
	stats->UsedSize[memType + MeshBuffer_Index] = IndexBuffers.Allocator.GetFreeSize() << GpuApiMeshCache::MeshCache_AllocatorUnitShift;
}

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////