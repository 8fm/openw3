/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "referencable.h"
#include "handleMap.h"
#include "object.h"

// use handle stats only in debug builds
#ifdef DEBUG
	#define USE_HANDLE_STATS
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_CLASS( IReferencable );

//////////////////////////////////////////////////////////////////////////

#ifdef USE_HANDLE_STATS

static Bool GPrintHandleStats = false;

class CReferencableInternalHandleStats
{
public:
	Red::Threads::AtomicOps::TAtomic32	m_numReferencables;
	Red::Threads::AtomicOps::TAtomic32	m_numHandleData;
	Red::Threads::AtomicOps::TAtomic32	m_numUnbound;

	Red::Threads::CMutex m_mutex;
	THashMap< const CClass*, Int32 > m_perClassCounts;

public:
	CReferencableInternalHandleStats()
	{
		m_numReferencables = 0;
		m_numHandleData = 0;
		m_numUnbound = 0;
	}

	void AddPerClass( const CClass* objectClass )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

		Red::Threads::AtomicOps::Decrement32( &CReferencableInternalHandleStats::GetInstance().m_numUnbound );

		Int32 value = 0;
		m_perClassCounts.Find( objectClass, value );
		value += 1;
		m_perClassCounts.Set( objectClass, value );
	}

	void RemovePerClass( const CClass* objectClass )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

		Red::Threads::AtomicOps::Increment32( &CReferencableInternalHandleStats::GetInstance().m_numUnbound );

		Int32 value = 0;
		if ( m_perClassCounts.Find( objectClass, value ) )
		{
			if ( value > 0 )
			{
				value -= 1;
				m_perClassCounts.Set( objectClass, value );
			}
			else
			{
				ERR_CORE( TXT("Invalid handle ref count for '%ls'"), objectClass->GetName().AsChar() );
			}
		}
		else
		{
			ERR_CORE( TXT("Invalid handle ref count map for '%ls'"), objectClass->GetName().AsChar() );
		}
	}

	struct ClassInfo
	{
		const CClass*	m_class;
		Int32			m_count;

		RED_INLINE ClassInfo()
			: m_class( nullptr )
			, m_count( 0 )
		{}

		RED_INLINE ClassInfo( const CClass* theClass, const Int32 count )
			: m_class( theClass )
			, m_count( count )
		{}

		static int CmpFunc( const void* a, const void* b )
		{
			const ClassInfo& classA = *(const ClassInfo*)a;
			const ClassInfo& classB = *(const ClassInfo*)b;
			return classB.m_count - classA.m_count;
		}
	};

	void PrintStats()
	{
		GPrintHandleStats = false;

		Uint32 classSum = 0;
		TDynArray< ClassInfo > info;
		{
			// exctract data
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
			for ( THashMap< const CClass*, Int32 >::const_iterator it = m_perClassCounts.Begin();
				it != m_perClassCounts.End(); ++it )
			{
				if ( it->m_second > 0 )
				{
					new ( info ) ClassInfo( it->m_first, it->m_second );
					classSum += it->m_second;
				}
			}
		}

		LOG_CORE( TXT("Num total referencables: %d"), m_numReferencables );
		LOG_CORE( TXT("Num total handle data: %d (%d unbound)"), m_numHandleData, m_numUnbound );
		LOG_CORE( TXT("Num handles in class stats: %d"), classSum );

		qsort( info.Data(), info.Size(), sizeof(ClassInfo), &ClassInfo::CmpFunc );

		for ( Uint32 i=0; i<info.Size(); ++i )
		{
			LOG_CORE( TXT("[%d]: %7d: %ls"), i, info[i].m_count, info[i].m_class->GetName().AsChar() );
		}
	}

	static CReferencableInternalHandleStats& GetInstance()
	{
		static CReferencableInternalHandleStats theInstance;
		return theInstance;
	}
};

#endif

//////////////////////////////////////////////////////////////////////////

ReferencableInternalHandle::ReferencableInternalHandle( IReferencable* object )
	: m_object( object )
	, m_class( object->GetClass() )
	, m_flags( 0 )
	, m_weakRefCount( 1 )
	, m_strongRefCount( 0 )
{
#ifdef USE_HANDLE_STATS
	Red::Threads::AtomicOps::Increment32( &CReferencableInternalHandleStats::GetInstance().m_numHandleData );
	Red::Threads::AtomicOps::Increment32( &CReferencableInternalHandleStats::GetInstance().m_numUnbound );

	CReferencableInternalHandleStats::GetInstance().AddPerClass( m_class );
#endif
}

ReferencableInternalHandle::~ReferencableInternalHandle()
{
#ifdef USE_HANDLE_STATS
	if ( m_object != nullptr )
	{
		RED_ASSERT( m_class != nullptr );
		CReferencableInternalHandleStats::GetInstance().RemovePerClass( m_class );
	}

	Red::Threads::AtomicOps::Decrement32( &CReferencableInternalHandleStats::GetInstance().m_numUnbound );
	Red::Threads::AtomicOps::Decrement32( &CReferencableInternalHandleStats::GetInstance().m_numHandleData );
#endif
}

void ReferencableInternalHandle::Unbind()
{
#ifdef USE_HANDLE_STATS
	if ( nullptr != m_object )
	{
		RED_ASSERT( m_class != nullptr );
		CReferencableInternalHandleStats::GetInstance().RemovePerClass( m_class );
	}
#endif

	m_object = nullptr;
	m_flags = 0;
}

void ReferencableInternalHandle::SetFlags( const Uint32 flagsToSet )
{
	RED_ASSERT( 0 == (m_flags & flagsToSet), TXT("Handle flag is already set") );
	m_flags |= flagsToSet;
}

void ReferencableInternalHandle::ClearFlags( const Uint32 flagsToClear )
{
	RED_ASSERT( 0 != (m_flags & flagsToClear), TXT("Handle flag is already cleared") );
	m_flags &= ~flagsToClear;
}

//////////////////////////////////////////////////////////////////////////

IReferencable::IReferencable()
	: m_internalHandle( new ReferencableInternalHandle(this) )
{
#ifdef USE_HANDLE_STATS
	Red::Threads::AtomicOps::Increment32( &CReferencableInternalHandleStats::GetInstance().m_numReferencables );
#endif
}

IReferencable::IReferencable( const IReferencable& )
	: m_internalHandle( new ReferencableInternalHandle(this) )
{
#ifdef USE_HANDLE_STATS
	Red::Threads::AtomicOps::Increment32( &CReferencableInternalHandleStats::GetInstance().m_numReferencables );
#endif
}

IReferencable& IReferencable::operator=(const IReferencable&)
{
	return *this;
}

IReferencable::~IReferencable()
{
#ifdef USE_HANDLE_STATS
	Red::Threads::AtomicOps::Decrement32( &CReferencableInternalHandleStats::GetInstance().m_numReferencables );
#endif

	if (m_internalHandle)
	{
		m_internalHandle->Unbind();
		m_internalHandle->ReleaseWeakRef();
		m_internalHandle = NULL;
	}
}

const Bool IReferencable::IsHandleReferenceCounted() const
{
	RED_FATAL_ASSERT( m_internalHandle != nullptr, "Calling status method on dead reference" );
	return m_internalHandle && m_internalHandle->IsRefCounted();
}

const Bool IReferencable::IsHandleProtected() const
{
	RED_FATAL_ASSERT( m_internalHandle != nullptr, "Calling status method on dead reference" );
	return m_internalHandle && m_internalHandle->IsProtected();
}

void IReferencable::DiscardHandles()
{
	if (m_internalHandle)
	{
		// it's illegal to discard handles on stuff with enabled reference counting
		/*if ( m_internalHandle->GetPtr() && m_internalHandle->IsRefCounted() )
		{
			IReferencable* ref = m_internalHandle->GetPtr();
			if ( ref && ref->GetClass()->IsObject() )
			{
				CObject* object = static_cast< CObject* >( ref );
				ERR_CORE( TXT("DiscardHandles called on an object '%ls' with enabled reference counting."),
					object->GetFriendlyName().AsChar() );
			}
		}*/

		// recreate object reference
		if( m_internalHandle->m_weakRefCount.Decrement() == 0 )
		{
			// No one is using the handle. Might as well reuse the handle, bypassing allocator. 
			m_internalHandle->m_flags = 0;
			m_internalHandle->m_weakRefCount.SetValue( 1 );
		}
		else
		{
			// Handle still in use, disconnect other owner from object and create a new one one.
			m_internalHandle->Unbind();
			m_internalHandle = new ReferencableInternalHandle(this);
		}
	}
}

void IReferencable::EnableReferenceCounting( const Bool enable )
{
	if ( enable  )
	{
		m_internalHandle->SetFlags( HF_ObjectReferenceCounting );
	}
	else
	{
		m_internalHandle->ClearFlags( HF_ObjectReferenceCounting );
	}
}

void IReferencable::EnableAutomaticDiscard( const Bool enable )
{
	if ( enable  )
	{
		m_internalHandle->SetFlags( HF_AutoDiscard );
	}
	else
	{
		m_internalHandle->ClearFlags( HF_AutoDiscard );
	}
}

void IReferencable::EnableHandleProtection()
{
	m_internalHandle->SetFlags( HF_ObjectProtected );
}

void IReferencable::DisableHandleProtection()
{
	m_internalHandle->ClearFlags( HF_ObjectProtected );
}

void IReferencable::OnAllHandlesReleased()
{
	delete this;
}

Bool IReferencable::OnValidateHandleCreation() const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
