/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "objectMap.h"
#include "objectRootSet.h"
#include "objectDiscardList.h"
#include "objectAllocationTracer.h"
#include "objectGC.h"
#include "objectGC_Legacy.h"
#include "resource.h"
#include "uniqueBuffer.h"

//#define DEBUG_GC

#ifdef DEBUG_GC
#pragma optimize("",off)
#endif

#define MULTITHREADED_GC

namespace // anonymous
{
	inline Bool IsInvalidatedPointer( void* ptr )
	{
#ifdef RED_ARCH_X64
		return reinterpret_cast<uintptr_t>(ptr) == 0xEEEEEEEEEEEEEEEEULL;
#else
		return reinterpret_cast<uintptr_t>(ptr) == 0xEEEEEEEEU;
#endif
	}
}

RED_DEFINE_STATIC_NAMED_NAME( _IAttachment, "IAttachment" );
RED_DEFINE_STATIC_NAMED_NAME( _CMeshSkinningAttachment, "CMeshSkinningAttachment" );

class CGCStackMessage_Marker : public Red::Error::StackMessage
{
private:
	const CObject*		m_object;

public:
	CGCStackMessage_Marker( const CObject* obj )
		: m_object( obj )
	{
	}

	virtual void Report( Char* outText, Uint32 outTextLength )
	{
		Int32 charsWritten = 0;
		charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("MarkerObject: 0x%016llX "), m_object );
		outText += charsWritten;	outTextLength -= charsWritten;

#ifdef RED_PLATFORM_WINPC
		if ( IsBadReadPtr( m_object, sizeof(CObject) ) == 0 )
		{
			charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("Index: %d "), m_object->GetObjectIndex() );
			outText += charsWritten;	outTextLength -= charsWritten;

			CClass* cls = m_object->GetLocalClass(); // no virtual
			if ( IsBadReadPtr( cls, sizeof(CClass) ) == 0 )
			{
				if ( cls->GetType() == RT_Class )
				{
					charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("%ls "), cls->GetName().AsChar() );
					outText += charsWritten;	outTextLength -= charsWritten;
				}
				else
				{
					charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD CLS2!!") );
					outText += charsWritten;	outTextLength -= charsWritten;
				}
			}
			else
			{
				charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD CLS!!") );
				outText += charsWritten;	outTextLength -= charsWritten;
			}
		}
		else
		{
			charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD!!") );
			outText += charsWritten;	outTextLength -= charsWritten;
		}
#endif
	}
};

class CFullReachablityMarker : public IFile
{
protected:
	TDynArray< Bool, MC_Engine >* m_unreachables;
	THashMap< const ISerializable*, Uint32 > m_visitedSerializables;
	Red::UniqueBuffer m_objectBuffer;
	Uint32 m_maxObjectCount;
	CObject ** m_objectIterator;
	CObject ** m_objectEndIterator;


public:
	CFullReachablityMarker()
		: IFile( FF_Writer | FF_GarbageCollector | FF_FileBased )
		, m_unreachables( nullptr )
		, m_maxObjectCount( 500 * 1024 )
		, m_objectIterator( nullptr )
		, m_objectEndIterator( nullptr )
	{
		m_visitedSerializables.Reserve( 100 * 1024 );
		m_objectBuffer = Red::CreateUniqueBuffer( m_maxObjectCount * sizeof( CObject* ), __alignof( CObject* ), MC_Engine );
	}

	void Reset()
	{
		m_objectIterator = nullptr;
		m_objectEndIterator = nullptr;
		m_unreachables = nullptr;
		m_visitedSerializables.ClearFast();
	}

	RED_INLINE void SetUnreachables( TDynArray< Bool, MC_Engine >& unreachables )
	{
		m_unreachables = &unreachables;
	}

	RED_INLINE void SetObjectsToProcess( TDynArray< CObject*, MC_Engine >& objects )
	{
		RED_FATAL_ASSERT( m_maxObjectCount >= objects.Size(), "GC Object limit busted. This most likely means the Root set is massive therefor should not happen." );

		Red::System::MemoryCopy( m_objectBuffer.Get(), objects.Data(), objects.DataSize() );
		
		m_objectIterator = static_cast< CObject** >( m_objectBuffer.Get() );
		m_objectEndIterator = m_objectIterator + objects.Size();

	}

	RED_INLINE void MarkReachable( CObject* object )
	{
		(*m_unreachables)[ object->GetObjectIndex() ] = true;
	}

	RED_INLINE Bool IsReachable( CObject* object )
	{
		return (*m_unreachables)[ object->GetObjectIndex() ];
	}

	RED_INLINE void Process()
	{
		while( m_objectIterator != m_objectEndIterator )
		{
			CObject * object = *m_objectIterator;
			++m_objectIterator;
			if( object )
			{
				if ( !IsReachable( object ) )
				{
					MarkReachable( object );

					object->OnSerialize( *this );
				}
			}
		}
	}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		if ( nullptr != pointer && pointerClass->IsSerializable() )
		{
			ISerializable* obj = (ISerializable*&) pointer;
			if ( obj->GetClass()->IsObject() )
			{
				CObject* realObject = static_cast< CObject* >( obj );

				// Do these tests here rather than MarkObject to reduce L1 thrashing from calling it when we don't need to
				if ( !IsReachable(realObject) && !realObject->HasFlag( OF_Discarded ) )
				{
					MarkObject( realObject );
				}
			}
			else if ( !obj->CanIgnoreInGC() )
			{
				// visit only once
				if( m_visitedSerializables.Insert( obj,  1 ) )
				{
					// recurse in place
					obj->OnSerialize( *this );
				}
			}
		}
	}

	RED_INLINE void MarkObject( CObject* obj )
	{
#ifndef RED_FINAL_BUILD
			CGCStackMessage_Marker crash_message( obj );
#endif
		*m_objectEndIterator = obj;
		++m_objectEndIterator;

		if( m_objectEndIterator >= static_cast< CObject** >( m_objectBuffer.Get() ) + m_maxObjectCount )
		{
			const MemSize distance = m_objectEndIterator - m_objectIterator;
			const MemSize size = distance * sizeof( CObject* );

			RED_WARNING_ONCE( distance < m_maxObjectCount, "Garbage Collector object buffer is out of memory." );

			if( distance < m_maxObjectCount )
			{
				// TODO ctremblay This is not needed. 
				// We can use buffer as a circular one. It will make code a bit more complicated. Gain might also be not so great.
				// However it would allow us to gain maybe one or two MB of memory as we don't need huge buffer to reduce call to MemMove.
				Red::System::MemoryMove( m_objectBuffer.Get(), m_objectIterator, size );
			}
			else
			{
				// Well, I do not want to crash. So let's try to increase memory ...
				m_maxObjectCount = static_cast< Uint32 >( m_maxObjectCount * 1.5f );
				Red::UniqueBuffer newBuffer = Red::CreateUniqueBuffer( m_maxObjectCount * sizeof( CObject* ), __alignof( CObject* ), MC_Engine );
				Red::System::MemoryCopy( newBuffer.Get(), m_objectIterator, size );
				m_objectBuffer = std::move( newBuffer );
			}

			m_objectIterator = static_cast< CObject** >( m_objectBuffer.Get() );
			m_objectEndIterator = m_objectIterator + distance;
		}
	}

protected:
	// Fake IFile interface
	virtual void Serialize( void*, size_t ) {}
	virtual Uint64 GetOffset() const { return 0; };
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64 ) {};
};



CObjectGC_Legacy::CObjectGC_Legacy()
{
	m_marker = new CFullReachablityMarker();
	m_objectsResults.Reserve( 8 * 1024 );
	m_toRemoveList.Reserve( 100 * 1024 );
	m_unreachables.ResizeFast( 700 * 1024 );
}

CObjectGC_Legacy::~CObjectGC_Legacy()
{
	delete m_marker;
}

void CObjectGC_Legacy::CollectGarbage( const Bool reportLeaks )
{
	// Discard objects from the discard list
	// LEGACY: old GC required the previous objects to be deleted before next pass
	GObjectsDiscardList->ProcessList( true );

	// Start timing
	Double startTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

	// Count initial objects
	Uint32 numRoots = 0;
	Uint32 numInitialObjects = 0;

	// Mark all objects as unreachable
	const Uint32 allObjectsSize = GObjectsMap->GetMaxObjectIndex();

	m_unreachables.ClearFast();
	m_unreachables.ResizeFast(allObjectsSize);
	Red::System::MemorySet( m_unreachables.Data(), 0, allObjectsSize );

	m_objectsResults.ClearFast();

	const Double fullReachabilityStart = Red::System::Clock::GetInstance().GetTimer().GetSeconds(); 
	RED_UNUSED(fullReachabilityStart);

	// Perform reachablity test
	{

		// root set is the initial set of objects we visit
		struct RootCollector
		{
			TDynArray<CObject*, MC_Engine >*		m_result;

			RED_INLINE const Bool operator()(CObject* object)
			{
				m_result->PushBack( object );
				return true;
			}
		};

		// collect objects from root set
		RootCollector rootCollector;
		rootCollector.m_result = &m_objectsResults;
		GObjectsRootSet->VisitRootSet( rootCollector );
		numRoots = m_objectsResults.Size();
		numInitialObjects = GObjectsMap->GetNumLiveObjects();

		// in editor there are resource we don't want to collect
		if ( GIsEditor )
		{
			for ( BaseObjectIterator it; it; ++it )
			{
				CResource* res = Cast< CResource >( *it );
				if ( res && res->PreventCollectingResource() )
				{
					m_objectsResults.PushBack( res );
				}
			}
		}

		m_marker->Reset();
		m_marker->SetUnreachables( m_unreachables );
		m_marker->SetObjectsToProcess( m_objectsResults );
		m_marker->Process();
	}

	const Double markingEnd = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	RED_UNUSED(markingEnd);

	m_toRemoveList.ClearFast();
	
	// Collect unreachable objects and sort them in generation order
	Uint32 numRemovedObjects = 0;
	{
		PC_SCOPE(CollectUnreachables);

		struct UnreachableExtractor
		{
			TDynArray<Bool, MC_Engine>*			m_unreachables;
			TDynArray<CObject*, MC_Engine>*		m_toRemoveList;

			RED_INLINE void operator()( CObject* object, const Uint32 index ) const
			{
				// object index can GROW because it's possible to create objects during GC (the default objects for example)
				if ( index < m_unreachables->Size() && !(*m_unreachables)[index] )
				{
					(*m_toRemoveList).PushBack(object);
				}
			}
		};

		UnreachableExtractor extractor;
		extractor.m_toRemoveList = &m_toRemoveList;
		extractor.m_unreachables = &m_unreachables;
		GObjectsMap->VisitAllObjectsNoFilter(extractor);
		numRemovedObjects = m_toRemoveList.Size();
	}

	const Double prethrowing = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	RED_UNUSED(prethrowing);

	LOG_CORE( TXT("Reachability\t\t %1.2f ms"), (markingEnd - fullReachabilityStart) * 1000.0f );
	LOG_CORE( TXT("Collecting unreachables %1.2f ms"), (prethrowing - markingEnd) * 1000.0f );

	// Report leaks
	if ( reportLeaks )
	{
		Helper::SObjectAllocationTracker::GetInstance().DumpCallstackInfo( m_toRemoveList );
	}

	DiscardAllObjectToRemove();

	const Double endTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	const Double gcTime = endTime - startTime;
	LOG_CORE( TXT("GC %i->%i, %i roots, %1.2f ms"), numInitialObjects, numInitialObjects-numRemovedObjects, numRoots, gcTime * 1000.0f );
	RED_UNUSED(gcTime);
}

void CObjectGC_Legacy::DiscardAllObjectToRemove()
{
	// Discard the objects
	{
		PC_SCOPE(DiscardingObjects);
		extern Red::Threads::CMutex GTheFuckingChildListMutex;
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GTheFuckingChildListMutex );
		Red::Threads::CScopedLock< CObjectsDiscardList > discardListLock( *GObjectsDiscardList );
		Red::Threads::CScopedLock< CObjectsMap > objectMapLock( *GObjectsMap );


		for ( Uint32 i = 0; i < m_toRemoveList.Size(); ++i )
		{
			CObject* objectToRemove = m_toRemoveList[i];
#ifdef DEBUG_GC
			LOG_CORE( TXT("GCDiscard[%d]: 0x%llX"), i, (Uint64)objectToRemove);
			LOG_CORE( TXT("   index: %d"), objectToRemove->m_index );
			LOG_CORE( TXT("   class: '%ls'"), objectToRemove->GetClass()->GetName().AsChar() );
#endif
			objectToRemove->DiscardNoLock(); // won't do anything if already discarded
		}
	}
}

