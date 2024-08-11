/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////

#include "renderScaleformIncludes.h"

//#include "Render/D3D1x/D3D1x_Config.h"
//#include "Render/D3D1x/D3D1x_Sync.h"
#include "renderScaleformTemp.h"
#include "Render/Render_MeshCache.h"
#include "Kernel/SF_Array.h"
//#include "Kernel/SF_Debug.h"
#include "Kernel/SF_HeapNew.h"

class CRenderScaleformMeshBuffer;
class CRenderScaleformMeshBufferSet;
class CRenderScaleformVertexBuffer;
class CRenderScaleformIndexBuffer;
class CRenderScaleformMeshCache;
class CRenderScaleformHAL;
class CRenderScaleformShaderManager;

// D3D1x version of MeshCacheItem. In D3D1x index and vertex buffers
// are allocated separately.

class CRenderScaleformMeshCacheItem : public SF::Render::MeshCacheItem
{
	friend class CRenderScaleformHAL;
	friend class CRenderScaleformMeshCache;
	friend class CRenderScaleformMeshBuffer;

	CRenderScaleformVertexBuffer* pVertexBuffer;
	CRenderScaleformIndexBuffer* pIndexBuffer;
	// Ranges of allocation in vertex and index buffers.
	SF::UPInt        VBAllocOffset, VBAllocSize;    
	SF::UPInt        IBAllocOffset, IBAllocSize;

public:

	static CRenderScaleformMeshCacheItem* Create(MeshType type,
		SF::Render::MeshCacheListSet* pcacheList, MeshBaseContent& mc,
		CRenderScaleformVertexBuffer* pvb, CRenderScaleformIndexBuffer* pib,
		SF::UPInt vertexOffset, SF::UPInt vertexAllocSize, SFUInt vertexCount,
		SF::UPInt indexOffset, SF::UPInt indexAllocSize, SFUInt indexCount)
	{
		CRenderScaleformMeshCacheItem* p = (CRenderScaleformMeshCacheItem*)
			SF::Render::MeshCacheItem::Create(type, pcacheList, sizeof(CRenderScaleformMeshCacheItem), mc,
			vertexAllocSize + indexAllocSize, vertexCount, indexCount);
		if (p)
		{
			p->pVertexBuffer = pvb;
			p->pIndexBuffer  = pib;
			p->VBAllocOffset = vertexOffset;
			p->VBAllocSize   = vertexAllocSize;
			p->IBAllocOffset = indexOffset;
			p->IBAllocSize   = indexAllocSize;
		}
		return p;
	}
};

namespace GpuApiMeshCache
{
	enum 
	{
		MeshCache_MaxBufferCount      = 256,
		MeshCache_AddressToIndexShift = 24,
		// A multi-byte allocation unit is used in MeshBufferSet::Allocator.
		MeshCache_AllocatorUnitShift  = 4,
		MeshCache_AllocatorUnit       = 1 << MeshCache_AllocatorUnitShift
	};
}

class CRenderScaleformMeshBuffer : public SF::Render::MeshBuffer
{
protected:    
	SF::UPInt       Index; // Index in MeshBufferSet::Buffers array.
	CRenderScaleformMeshBuffer* pNextLock;

public:

	enum BufferType
	{
		Buffer_Vertex,
		Buffer_Index,
	};

	CRenderScaleformMeshBuffer(SF::UPInt size, AllocType type, SFUInt arena)
		: SF::Render::MeshBuffer(size, type, arena)
	{ }
	virtual ~CRenderScaleformMeshBuffer() { }

	inline  SF::UPInt   GetIndex() const { return Index; }

	virtual SFBool    allocBuffer() = 0;
	virtual SFBool    DoLock() = 0;
	virtual void    Unlock() = 0;    
	virtual BufferType GetBufferType() const = 0;

	// Simple LockList class is used to track all MeshBuffers that were locked.
	struct LockList
	{
		CRenderScaleformMeshBuffer *pFirst;
		LockList() : pFirst(0) { }

		void Add(CRenderScaleformMeshBuffer *pbuffer)
		{
			pbuffer->pNextLock = pFirst;
			pFirst = pbuffer;
		}
		void UnlockAll()
		{
			CRenderScaleformMeshBuffer* p = pFirst;
			while(p)
			{
				p->Unlock();
				p = p->pNextLock;
			}
			pFirst = 0;
		}
	};

	inline  SF::UByte*  Lock(LockList& lockedBuffers)
	{
		if (!pData)
		{  
			if (!DoLock())
			{
				HALT( "Render::MeshCache - Vertex/IndexBuffer lock failed" );
				return 0;
			}
			lockedBuffers.Add(this);
		}
		return (SF::UByte*)pData;
	}
};


class CRenderScaleformMeshBufferSet
{
public:
	typedef CRenderScaleformMeshBuffer::AllocType AllocType;

	// Buffers are addressable; index is stored in the upper bits of allocation address,
	// as tracked by the allocator. Buffers array may contain null pointers for freed buffers.    
	SF::ArrayLH<CRenderScaleformMeshBuffer*>  Buffers;
	SF::AllocAddr             Allocator;
	SF::UPInt                 Granularity;
	SF::UPInt                 TotalSize;

	CRenderScaleformMeshBufferSet(SF::MemoryHeap* pheap, SF::UPInt granularity)
		: Allocator(pheap), Granularity(granularity),  TotalSize(0)
	{ }

	virtual ~CRenderScaleformMeshBufferSet()
	{ }

	inline void         SetGranularity(SF::UPInt granularity)
	{ Granularity = granularity; }

	inline SF::AllocAddr&   GetAllocator()          { return Allocator; }
	inline SF::UPInt        GetGranularity() const  { return Granularity; }    
	inline SF::UPInt        GetTotalSize() const    { return TotalSize; }

	virtual CRenderScaleformMeshBuffer* CreateBuffer(SF::UPInt size, AllocType type, SFUInt arena,
		SF::MemoryHeap* pheap) = 0;

	// Destroys all buffers of specified type; all types are destroyed if AT_None is passed.
	void        DestroyBuffers(AllocType type = CRenderScaleformMeshBuffer::AT_None)
	{        
		for (SF::UPInt i = 0; i< Buffers.GetSize(); i++)
		{
			if (Buffers[i] && ((type == CRenderScaleformMeshBuffer::AT_None) || (Buffers[i]->GetType() == type)))
				DestroyBuffer(Buffers[i]);
		}
	}

	static inline SF::UPInt SizeToAllocatorUnit(SF::UPInt size)
	{
		return (size + GpuApiMeshCache::MeshCache_AllocatorUnit - 1) >> GpuApiMeshCache::MeshCache_AllocatorUnitShift;
	}

	void        DestroyBuffer(CRenderScaleformMeshBuffer* pbuffer, SFBool deleteBuffer = true)
	{
		Allocator.RemoveSegment(pbuffer->GetIndex() << GpuApiMeshCache::MeshCache_AddressToIndexShift,
			SizeToAllocatorUnit(pbuffer->GetSize()));
		TotalSize -= pbuffer->GetSize();
		Buffers[pbuffer->GetIndex()] = 0;
		if ( deleteBuffer )
		{
			delete pbuffer;
		}
	}


	// Alloc 
	inline SFBool    Alloc(SF::UPInt size, CRenderScaleformMeshBuffer** pbuffer, SF::UPInt* poffset)
	{
		SF::UPInt offset = Allocator.Alloc(SizeToAllocatorUnit(size));
		if (offset == ~SF::UPInt(0))
			return false;
		*pbuffer = Buffers[offset >> GpuApiMeshCache::MeshCache_AddressToIndexShift];
		*poffset = (offset & ((1 << GpuApiMeshCache::MeshCache_AddressToIndexShift) - 1)) << GpuApiMeshCache::MeshCache_AllocatorUnitShift;
		return true;
	}

	inline SF::UPInt    Free(SF::UPInt size, CRenderScaleformMeshBuffer* pbuffer, SF::UPInt offset)
	{
		return Allocator.Free(
			(pbuffer->GetIndex() << GpuApiMeshCache::MeshCache_AddressToIndexShift) | (offset >> GpuApiMeshCache::MeshCache_AllocatorUnitShift),
			SizeToAllocatorUnit(size)) << GpuApiMeshCache::MeshCache_AllocatorUnitShift;
	}
};


template<class Buffer>
class CRenderScaleformMeshBufferSetImpl : public CRenderScaleformMeshBufferSet
{
public:
	CRenderScaleformMeshBufferSetImpl(SF::MemoryHeap* pheap, SF::UPInt granularity)
		: CRenderScaleformMeshBufferSet(pheap, granularity)
	{ }      
	virtual CRenderScaleformMeshBuffer* CreateBuffer(SF::UPInt size, AllocType type, SFUInt arena,
		SF::MemoryHeap* pheap)
	{
		// Single buffer can't be bigger then (1<<MeshCache_AddressToIndexShift)
		// size due to the way addresses are masked.
		ASSERT(size <= (1<<(GpuApiMeshCache::MeshCache_AddressToIndexShift+GpuApiMeshCache::MeshCache_AllocatorUnitShift)));
		return Buffer::Create(size, type, arena, pheap, *this);
	}
};


template<class Derived>
class CRenderScaleformMeshBufferImpl : public CRenderScaleformMeshBuffer
{
protected:
	typedef CRenderScaleformMeshBufferImpl      Base;
	GpuApi::BufferRef					  pBuffer;

public:

	CRenderScaleformMeshBufferImpl(SF::UPInt size, AllocType type, SFUInt arena)
		: CRenderScaleformMeshBuffer(size, type, arena)
	{ }

	virtual ~CRenderScaleformMeshBufferImpl()
	{
		GpuApi::SafeRelease( pBuffer );
	}

	inline const GpuApi::BufferRef& GetHWBuffer() const { return pBuffer; }

	SFBool   DoLock()
	{
		ASSERT(!pData);
		const GpuApi::BufferDesc& bufferDesc = GpuApi::GetBufferDesc( pBuffer );
		const GpuApi::Uint32 bufferSize = bufferDesc.size;
		void *data = GpuApi::LockBuffer( pBuffer, GpuApi::BLF_NoOverwrite, 0, bufferSize );

		if ( data )
		{
			pData = data;
			return true;
		}

		return false;
	}
	void    Unlock()
	{
		GpuApi::UnlockBuffer( pBuffer );
		pData = 0;
	}

	static Derived* Create(SF::UPInt size, AllocType type, SFUInt arena,
		SF::MemoryHeap* pheap, CRenderScaleformMeshBufferSet& mbs)
	{
		// Determine which address index we'll use in the allocator.
		SF::UPInt index = 0;
		while ((index < mbs.Buffers.GetSize()) && mbs.Buffers[index])
			index++;
		if (index == GpuApiMeshCache::MeshCache_MaxBufferCount)
			return 0;

		// Round-up to Allocator unit.
		size = ((size + GpuApiMeshCache::MeshCache_AllocatorUnit-1) >> GpuApiMeshCache::MeshCache_AllocatorUnitShift) << GpuApiMeshCache::MeshCache_AllocatorUnitShift;

		Derived* p = SF_HEAP_NEW(pheap) Derived(size, type, arena);
		if (!p) return 0;

		if (!p->allocBuffer())
		{
			delete p;
			return 0;
		}
		p->Index = index;

		mbs.Allocator.AddSegment(index << GpuApiMeshCache::MeshCache_AddressToIndexShift, size >> GpuApiMeshCache::MeshCache_AllocatorUnitShift);
		mbs.TotalSize += size;

		if (index == mbs.Buffers.GetSize())
			mbs.Buffers.PushBack(p);
		else
			mbs.Buffers[index] = p;

		return p;
	}
};

class CRenderScaleformVertexBuffer : public CRenderScaleformMeshBufferImpl<CRenderScaleformVertexBuffer>
{
public:
	CRenderScaleformVertexBuffer(SF::UPInt size, AllocType atype, SFUInt arena)
		: Base(size, atype, arena)
	{ }

	virtual SFBool allocBuffer()
	{
		if ( pBuffer )
		{
			GpuApi::Release( pBuffer );
		}
		
		pBuffer = GpuApi::CreateBuffer( static_cast< Uint32 >( Size ), GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( pBuffer )
		{
			GpuApi::SetBufferDebugPath( pBuffer, "SFVB" );
			return true;
		}
		return false;
	}
	virtual BufferType GetBufferType() const { return Buffer_Vertex; }
};

class CRenderScaleformIndexBuffer : public CRenderScaleformMeshBufferImpl<CRenderScaleformIndexBuffer>
{
public:
	CRenderScaleformIndexBuffer(SF::UPInt size, AllocType atype, SFUInt arena)
		: Base(size, atype, arena)
	{ }

	virtual SFBool allocBuffer()
	{
		if ( pBuffer )
		{
			GpuApi::Release( pBuffer );
		}

		const GpuApi::eBufferChunkCategory category = sizeof(SF::Render::IndexType) == 2 ? GpuApi::BCC_Index16Bit : GpuApi::BCC_Index32Bit;
		pBuffer = GpuApi::CreateBuffer( static_cast< GpuApi::Uint32 >( Size ), category, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( pBuffer )
		{
			GpuApi::SetBufferDebugPath( pBuffer, "SFIB" );
			return true;
		}
		return false;
	}
	virtual BufferType GetBufferType() const { return Buffer_Index; }
};

typedef CRenderScaleformMeshBufferSetImpl<CRenderScaleformVertexBuffer> CRenderScaleformVertexBufferSet;
typedef CRenderScaleformMeshBufferSetImpl<CRenderScaleformIndexBuffer>  CRenderScaleformIndexBufferSet;



// D3D1x mesh cache implementation with following characteristics:
//  - Multiple cache lists and allocators; one per Buffer.
//  - Relies on 'Extended Locks'.

class CRenderScaleformMeshCache : public SF::Render::MeshCache
{      
	friend class CRenderScaleformHAL;    

	enum {
		MinSupportedGranularity = 16*1024,
	};

	CRenderScaleformShaderManager*		pShaderManager;
	SF::Render::MeshCacheListSet	CacheList;

	// Allocators managing the buffers. 
	CRenderScaleformVertexBufferSet             VertexBuffers;
	CRenderScaleformIndexBufferSet              IndexBuffers;

	// Locked buffer into.
	Bool                        Locked;
	SF::UPInt                   VBSizeEvictedInLock;
	CRenderScaleformMeshBuffer::LockList        LockedBuffers;
	// A list of all buffers in the order they were allocated. Used to allow
	// freeing them in the opposite order (to support proper IB/VB balancing).
	// NOTE: Must use base MeshBuffer type for List<> to work with internal casts.
	SF::List<SF::Render::MeshBuffer>    ChunkBuffers;
	// If a MeshBuffer could not be immediately destroyed (the GPU is using one of its meshes)
	// It is temporarily stored in this list until it is no longer in use.
	SF::List<SF::Render::MeshBuffer>    PendingDestructionBuffers;

	GpuApi::BufferRef			m_maskEraseBatchVertexBuffer;

	inline CRenderScaleformMeshCache* getThis() { return this; }

	inline SF::UPInt    getTotalSize() const
	{
		return VertexBuffers.GetTotalSize() + IndexBuffers.GetTotalSize();
	}

	SFBool            createStaticVertexBuffers();
	SFBool            createMaskEraseBatchVertexBuffer();

	// Allocates Vertex/Index buffer of specified size and adds it to free list.
	SFBool            allocCacheBuffers(SF::UPInt size, CRenderScaleformMeshBuffer::AllocType type, SFUInt arena = 0);

	SFBool            evictMeshesInBuffer(SF::Render::MeshCacheListSet::ListSlot* plist, SF::UPInt count,
		CRenderScaleformMeshBuffer* pbuffer);

	// Allocates a number of bytes in the specified buffer, while evicting LRU data.
	// Buffer can contain either vertex and/or index data.
	SFBool            allocBuffer(SF::UPInt* poffset, CRenderScaleformMeshBuffer** pbuffer,
		CRenderScaleformMeshBufferSet& mbs, SF::UPInt size, SFBool waitForCache);

	void            destroyBuffers(CRenderScaleformMeshBuffer::AllocType at = CRenderScaleformMeshBuffer::AT_None);

	void            adjustMeshCacheParams(SF::Render::MeshCacheParams* p);    

	// If buffers could not be deleted immediately (their meshes were still in use), they are stored
	// in PendingDestructionBuffers and destroyed in this function (if possible).
	void            destroyPendingBuffers();

public:

	CRenderScaleformMeshCache(SF::MemoryHeap* pheap,
		const SF::Render::MeshCacheParams& params);
	~CRenderScaleformMeshCache();    

	// Initializes MeshCache for operation, including allocation of the reserve
	// buffer. Typically called from SetVideoMode.
	SFBool          Initialize(CRenderScaleformShaderManager* psm);
	// Resets MeshCache, releasing all buffers.
	void            Reset();    

	virtual QueueMode GetQueueMode() const { return QM_ExtendLocks; }


	// *** MeshCacheConfig Interface
	virtual void    ClearCache();
	virtual SFBool  SetParams(const SF::Render::MeshCacheParams& params);

	virtual void    BeginFrame();
	virtual void    EndFrame();
	// Adds a fixed-size buffer to cache reserve; expected to be released at Release.
	//virtual bool    AddReserveBuffer(unsigned size, unsigned arena = 0);
	//virtual bool    ReleaseReserveBuffer(unsigned size, unsigned arena = 0);

	virtual SFBool  LockBuffers();
	virtual void    UnlockBuffers();
	virtual SFBool  AreBuffersLocked() const { return Locked; }

	virtual SF::UPInt Evict(SF::Render::MeshCacheItem* p, SF::AllocAddr* pallocator = 0,
		SF::Render::MeshBase* pmesh = 0); 
	virtual SFBool    PreparePrimitive(SF::Render::PrimitiveBatch* pbatch,
		CRenderScaleformMeshCacheItem::MeshContent &mc, SFBool waitForCache);    

	virtual AllocResult AllocCacheItem(SF::Render::MeshCacheItem** pdata,
		SF::UByte** pvertexDataStart, SF::Render::IndexType** pindexDataStart,
		CRenderScaleformMeshCacheItem::MeshType meshType,
		CRenderScaleformMeshCacheItem::MeshBaseContent &mc,
		SF::UPInt vertexBufferSize,
		SFUInt vertexCount, SFUInt indexCount,
		SFBool waitForCache, const SF::Render::VertexFormat* pDestFormat);

	virtual void GetStats(Stats* stats);
};

// Vertex structures used by both MeshCache and HAL.

class Render_InstanceData
{
public:
	// Color format for instance, Alpha expected to have index.
	SF::UInt32 Instance;
};

/////////////////////////////////////////////////////////////////////////
#endif // USE_SCALEFORM
//////////////////////////////////////////////////////////////////////////