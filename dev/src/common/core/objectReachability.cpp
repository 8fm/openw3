/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "objectRootSet.h"
#include "objectMap.h"
#include "objectReachability.h"

//---

CFastPointerSet::CFastPointerSet()
	: m_freePages(NULL)
{
	memset( &m_bucketHi, 0, sizeof(m_bucketHi) );

	// preallocate memory pages
	const Uint32 NUM_PREALLOCATED = 10; // 640KB
	for ( Uint32 i=0; i<NUM_PREALLOCATED; ++i )
	{
		FreeMem* page = (FreeMem*) AllocRawMemPage();

		page->m_next = m_freePages;
		m_freePages = page;
	}
}

CFastPointerSet::~CFastPointerSet()
{
	Red::Threads::CScopedLock< Mutex > lock(m_lock);

	// free used page
	while ( !m_usedPages.Empty() )
	{
		FreeRawMemPage( m_usedPages[0] );
	}

	// free empty pages
	FreeMem* cur = m_freePages;
	while ( cur )
	{
		FreeMem* next = cur->m_next;
		FreeRawMemPage( cur );
		cur = next;
	}
}

void CFastPointerSet::Reset()
{
	Red::Threads::CScopedLock< Mutex > lock(m_lock);

	// reset bits on pages
	for ( Uint32 i=0; i<m_bitPages.Size(); ++i )
	{
		void* mem = m_bitPages[i];
		ZeroPage( mem );
	}

#ifdef VERIFY_POINTER_SET
	{
		Red::Threads::CScopedLock< Mutex > lock( m_testLock );
		m_testData.Clear();
	}
#endif
}

void* CFastPointerSet::AllocBitPage()
{
	Red::Threads::CScopedLock< Mutex > lock(m_lock);

	void* mem = AllocUntypedMemPage();

	if ( mem )
	{
		m_bitPages.PushBack( mem );
	}

	return mem;
}

void* CFastPointerSet::AllocUntypedMemPage()
{
	Red::Threads::CScopedLock< Mutex > lock(m_lock);

	void* ptr;

	if ( m_freePages )
	{
		ptr = m_freePages;
		m_freePages = m_freePages->m_next;
	}
	else
	{
		ptr = AllocRawMemPage();
	}

	m_usedPages.PushBack( ptr );
	ZeroPage( ptr );

	return ptr;
}

void CFastPointerSet::FreeUntypedMemPage( void* ptr )
{
	Red::Threads::CScopedLock< Mutex > lock(m_lock);

	// return to free list
	FreeMem* page = (FreeMem*) ptr;
	page->m_next = m_freePages;
	m_freePages = page;

	// remove from used list
	for ( Uint32 i=0; i<m_usedPages.Size(); ++i )
	{
		if ( m_usedPages[i] == ptr )
		{
			m_usedPages.RemoveAtFast( i );
			break;
		}
	}

	// remove from bit list
	for ( Uint32 i=0; i<m_bitPages.Size(); ++i )
	{
		if ( m_bitPages[i] == ptr )
		{
			m_bitPages.RemoveAtFast( i );
			break;
		}
	}
}

void CFastPointerSet::ZeroPage( void* ptr )
{
	Red::MemoryZero( ptr, sizeof(Page) );
}

void* CFastPointerSet::AllocRawMemPage()
{
	return RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ObjectMap, sizeof(Page) );
}

void CFastPointerSet::FreeRawMemPage( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_ObjectMap, ptr );
}

const Bool CFastPointerSet::TestAndMark( const void* ptr )
{
	const Uint64 addr = (const Uint64) ptr;

	// thread safe - get the low bucket
	BucketLo* bucketLo = NULL;
	{
		// get low address bucket
		const Uint32 loIndex = (addr >> BucketHi::ADDR_SHIFT) % BucketLo::NUM_ENTRIES;
		BucketLo** bucketLoPtr = &m_bucketHi.m_entries[ loIndex ];
		BucketLo* bucketLoAllocated = NULL;
		bucketLo = *bucketLoPtr;

		// allocate new page if needed
		if ( !bucketLo )
		{
			do
			{
				if ( !bucketLoAllocated )
					bucketLoAllocated = (BucketLo*)AllocUntypedMemPage();

				// use the allocated one
				bucketLo = bucketLoAllocated;
			}
			while ( Red::Threads::AtomicOps::CompareExchangePtr( (AtomicPtr*) bucketLoPtr, (AtomicPtr)bucketLo, NULL ) ); // swap NULL to valid ptr
		}

		// cleanup not allocated pointer (may happen if two threads requested the same page at the same time)
		if ( bucketLoAllocated && bucketLo != bucketLoAllocated )
		{
			FreeUntypedMemPage( bucketLoAllocated );
		}
	}

	// thread safe - get the page bit buffer
	Page* bucketPage = NULL;
	{
		// get page bucket
		const Uint32 pageIndex = (addr >> BucketLo::ADDR_SHIFT) % BucketLo::NUM_ENTRIES;
		Page** bucketPagePtr = &bucketLo->m_entries[ pageIndex ];
		Page* bucketPageAllocated = NULL;
		bucketPage = *bucketPagePtr;

		// allocate new page if needed
		if ( !bucketPage )
		{
			do
			{
				if ( !bucketPageAllocated )
					bucketPageAllocated = (Page*) AllocBitPage();

				// use the allocated one
				bucketPage = bucketPageAllocated;
			}
			while ( Red::Threads::AtomicOps::CompareExchangePtr( (AtomicPtr*) bucketPagePtr, (AtomicPtr)bucketPage, NULL ) ); // swap NULL to valid ptr
		}

		// cleanup not allocated pointer (may happen if two threads requested the same page at the same time)
		if ( bucketPageAllocated && bucketPage != bucketPageAllocated )
		{
			FreeUntypedMemPage( bucketPageAllocated );
		}
	}

	// test + set (also thread safe)
	const Bool ret = bucketPage->TestAndMark( addr );

#ifdef VERIFY_POINTER_SET
	{
		Red::Threads::CScopedLock< Mutex > lock( m_testLock );
		Bool isInSet = m_testData.KeyExist( (void*)ptr );
		if ( !isInSet )
		{
			m_testData.Set( (void*)ptr, 1 );
		}

		RED_FATAL_ASSERT( !isInSet == ret, "Internal validation failed" );
	}
#endif

	return ret;
}

//---

CFastObjectList::CFastObjectList()
	: m_numEntries( 0 )
	, m_entryBuffer( NULL )
{
	// allocate initial memory
	ResetToZeros( 256 * 1024 );
}

CFastObjectList::~CFastObjectList()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_ObjectMap, m_entryBuffer );
}

void CFastObjectList::ResetToZeros( const Uint32 maxEntryIndex )
{
	if ( maxEntryIndex > m_numEntries )
	{
		const Uint32 memSize = (maxEntryIndex+31) / 32;

		m_numEntries = maxEntryIndex;
		m_entryBuffer = (Uint32*) RED_MEMORY_REALLOCATE( MemoryPool_Default, m_entryBuffer, MC_ObjectMap, memSize*4 );
	}

	const Uint32 memSizeToClear = (maxEntryIndex+7) / 8;
	Red::MemoryZero( m_entryBuffer, memSizeToClear );
}

void CFastObjectList::ResetToOnes( const Uint32 maxEntryIndex )
{
	if ( maxEntryIndex > m_numEntries )
	{
		const Uint32 memSize = (maxEntryIndex+31) / 32;

		m_numEntries = maxEntryIndex;
		m_entryBuffer = (Uint32*) RED_MEMORY_REALLOCATE( MemoryPool_Default, m_entryBuffer, MC_ObjectMap, memSize*4 );
	}

	const Uint32 memSizeToClear = (maxEntryIndex+7) / 8;
	Red::MemorySet( m_entryBuffer, 0xFF, memSizeToClear );
}
//---

CObjectReachability::CObjectReachability()
{

}

/*class CStackMessage_PurgeProperty : public Red::Error::StackMessage
{
private:
	const CProperty*		m_property;

public:
	CStackMessage_PurgeProperty( const CProperty* property )
		: m_property( property )
	{
	}

	virtual void Report( Char* outText, Uint32 outTextLength )
	{
		Red::System::SNPrintF( outText, outTextLength, TXT("Purge property '%ls' in '%ls'"), 
			m_property->GetName().AsChar(),
			m_property->GetParent()->GetName().AsChar() );
	}
};*/

void CObjectReachability::ProcessClassDepdendnecies( const CClass* ptr, void* object, CFastPointerSet& visitedSet, TFastMuiltiStreamList< void*, void*, nullptr >& outList )
{
	/// TODO: legacy stuff, slow 
	class LegacyPtrVisitor : public IFile
	{
	public:
		LegacyPtrVisitor( CFastPointerSet& visitedSet, TFastMuiltiStreamList< void*, void*, nullptr >& outList )
			: IFile( FF_Writer | FF_GarbageCollector | FF_FileBased )
			, m_visitedSet( &visitedSet )
			, m_outList( &outList )
		{}

		// Pointer serialization helper
		virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
		{
			RED_UNUSED( pointerClass );

			// if pointer was not yet visited add it to list of pointers to visit and mark as visited
			// thread safe :)
			if ( pointer && m_visitedSet->TestAndMark( pointer ) )
			{
				if ( pointerClass->IsSerializable() )
				{
					m_outList->Put( pointer );

	#ifndef RED_FINAL_BUILD
					if ( pointerClass->IsObject() )
					{
						CObject* object = static_cast< CObject* >( pointer );
						if ( object->HasFlag( OF_Discarded ) )
						{
							RED_FATAL_ASSERT( false, "Live pointer to discarded object at 0x%LLX, class '%ls'", 
								(Uint64)pointer, pointerClass->GetName().AsChar() );
						}
						else
						{
							const Uint32 index = object->GetObjectIndex();
							RED_UNUSED( index );
							RED_FATAL_ASSERT( index > 0 && index < GObjectsMap->GetMaxObjectIndex(), "Invalid object reference at 0x%LLX, class '%ls'", 
								(Uint64)pointer, pointerClass->GetName().AsChar() );
						}
					}
	#endif
				}
			}
		}

	private:
		CFastPointerSet*		m_visitedSet;
		TFastMuiltiStreamList< void*, void*, nullptr >*	m_outList;

		virtual void Serialize( void* , size_t ) override RED_FINAL {}
		virtual Uint64 GetOffset() const override RED_FINAL { return 0; }
		virtual Uint64 GetSize() const override RED_FINAL { return 0; }
		virtual void Seek( Int64 ) override RED_FINAL {}
	};

	LegacyPtrVisitor visitor( visitedSet, outList );
	if ( ptr->IsSerializable() )
	{
		ISerializable* ser = static_cast< ISerializable* >( object );
		ser->OnSerialize( visitor );
	}
	else
	{
		ptr->SerializeGC( visitor, object );
	}
}

void CObjectReachability::ProcessObjects( TFastMuiltiStreamList< void*, void*, nullptr >& objects, CFastObjectList* unreachables, CFastPointerSet& visitedSet )
{
	void* ptr = objects.Get();
	while ( ptr )
	{
		// object is not unreachable
		ISerializable* serPtr = static_cast< ISerializable* >( ptr );
		const CClass* classPtr = serPtr->GetClass();
		const Uint32 index = serPtr->GetObjectIndex();

		///LOG_CORE( TXT("Visited 0x%LLX (%ls) %d"), (Uint64)serPtr, classPtr->GetName().AsChar(), index );

		// mark objects as reachable (remove from unreachable list)
		if (index)
		{
			unreachables->Clear( index );
		}

		// add class dependencies
		ProcessClassDepdendnecies( classPtr, serPtr, visitedSet, objects );

		// next
		ptr = objects.Get();
	}
}

namespace Helper
{
	struct RootSetExtractor
	{
		TFastMuiltiStreamList< void*, void*, nullptr >* m_objectList;

		RED_INLINE const Bool operator()(const CObject* object)
		{
			m_objectList->Put( (void*) object ); // objects in the list are type agnostic
			return true;
		}

		RootSetExtractor( TFastMuiltiStreamList< void*, void*, nullptr >* objectList )
			: m_objectList( objectList )
		{}
	};
}

void CObjectReachability::InitializeFromRootSet( const Uint32 maxPointerCount )
{
	PC_SCOPE(InitializeFromRootSet);

	// reset pointer list
	m_pointerList.Reset( maxPointerCount );

	// prepare initial pointer list
	Helper::RootSetExtractor extractor( &m_pointerList );
	GObjectsRootSet->VisitRootSet( extractor );
}

void CObjectReachability::InitializeFromObjects( CObject** objects, const Uint32 numObjects )
{
	PC_SCOPE(InitializeFromObjects);

	for ( Uint32 i=0; i<numObjects; ++i )
	{
		CObject* object = objects[i];
		if ( object )
			m_pointerList.Put( object );
	}
}

void CObjectReachability::CollectUnreachables( CFastObjectList* unreachables, const Bool multiThreaded )
{
	RED_UNUSED(multiThreaded);
	PC_SCOPE(Reachability);

	// reset internal pointer map
	m_visitedSet.Reset();
	m_unreachables = unreachables;

	// write list -> read list
	m_pointerList.Swap();

	// process objects
	while ( !m_pointerList.Empty() )
	{
		// TODO: this can be called from multiple threads (it's 100% thread safe)
		ProcessObjects( m_pointerList, m_unreachables, m_visitedSet );

		// write list -> read list
		m_pointerList.Swap();
	}
}

//---
