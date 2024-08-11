/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "namesPool.h"

#ifndef RED_FINAL_BUILD
CNamesPool* GDebuggerNamesPool = nullptr;
#endif

//////////////////////////////////////////////////////////////////////////
// CNamesPool::SNameHolder
//////////////////////////////////////////////////////////////////////////
CNamesPool::SNameHolder::SNameHolder( const Char* name, const AnsiChar* nameAnsi, CNameHash nameHash )
	: m_name( name )
	, m_nameAnsi( nameAnsi )
	, m_next( nullptr )
	, m_nameHash( nameHash )
	, m_poolIndex( INDEX_NONE )
{
	RED_ASSERT( name && *name && nameAnsi && *nameAnsi );
}

//////////////////////////////////////////////////////////////////////////
// CNamesPool::SStaticNameHolder
//////////////////////////////////////////////////////////////////////////
CNamesPool::SStaticNameHolder::SStaticNameHolder( const Char* name, const AnsiChar* nameAnsi, CNameHash nameHash )
	: SNameHolder( name, nameAnsi, nameHash )
{
	SNamesPool::GetInstance().AddStaticPoolName( this );
}

//////////////////////////////////////////////////////////////////////////
// CNamesPool
//////////////////////////////////////////////////////////////////////////
CNamesPool::CNamesPool()
	: m_freePages( nullptr )
	, m_usedPages( nullptr )
{
#ifndef RED_FINAL_BUILD
	GDebuggerNamesPool = this;
#endif

	// Initialise free page list
	Red::System::MemorySet( m_pageHeaderPool, 0, sizeof( m_pageHeaderPool ) );
	for( Uint32 i=0; i<( MAX_INTERNAL_PAGES-1 ); ++i )
	{
		m_pageHeaderPool[i].m_nextPoolPage = &m_pageHeaderPool[i + 1];
	}
	m_freePages = m_pageHeaderPool;

	Red::System::MemorySet( m_head, 0, sizeof m_head );
	m_indexedNameList.Reserve( NAME_RESERVE_SIZE );

	SNameHolder* noneNameHolder = (SNameHolder*)AllocateNameHolder( 0 );
	RED_ASSERT( noneNameHolder );
	::new ( noneNameHolder ) SNameHolder( MACRO_TXT(RED_CNAME_NONETXT), RED_CNAME_NONETXT, CNameHash( CNameHash::INVALID_HASH_VALUE ) );
	
	AddStaticPoolName( noneNameHolder );

	RED_ASSERT( noneNameHolder->m_poolIndex == INDEX_NONE );
}

CNamesPool::~CNamesPool()
{
}

CNamesPool::TIndex CNamesPool::AddDynamicPoolName( const Char* name )
{
	RED_ASSERT( name && *name );

	if ( ! name || !*name )
	{
		return INDEX_NONE;
	}

	return AddEntry( name );
}

void CNamesPool::AddStaticPoolName( SNameHolder* nameHolder )
{
	SNameHolder* foundNameHolder = nullptr;

	{
		Red::Threads::CScopedLock< TLock > lock( m_mutex );

		foundNameHolder = FindNameHolder_NoSync( nameHolder->m_nameHash );
		if ( ! foundNameHolder )
		{
			LinkNameHolder_NoSync( nameHolder );

			m_indexedNameList.PushBack( nameHolder );
			nameHolder->m_poolIndex = m_indexedNameList.Size() - 1;
		}
	}

#ifndef RED_FINAL_BUILD
	// Halt outside of the mutex lock or else can deadlock with the log system getting an unrelated CName log channel.
	if ( foundNameHolder && Red::System::StringCompare( foundNameHolder->m_name, nameHolder->m_name ) != 0 )
	{
		RED_HALT( "!!! CName collision detected !!! '%ls' vs '%ls' with hash '%u'. This requires immediate programmer attention", foundNameHolder->m_name, nameHolder->m_name, nameHolder->m_nameHash.GetValue() );
	}
#endif
}

CNamesPool::TIndex CNamesPool::GetIndexFromHash( CNameHash nameHash ) const
{
	{
		Red::Threads::CScopedLock< TLock > lock( m_mutex );

		SNameHolder* nameHolder = FindNameHolder_NoSync( nameHash );
		if ( nameHolder )
		{
			return nameHolder->m_poolIndex;
		}
	}
	
	RED_HALT( "Hash '%u' not found in pool!", nameHash.GetValue() );

	return INDEX_NONE;
}

void* CNamesPool::AllocateNameHolder( size_t stringLength )
{
	SNamePoolPage* pageToUse = nullptr;

	size_t memoryRequired = sizeof( SNameHolder ) +  sizeof( Char ) * ( stringLength + 1 ) + sizeof( AnsiChar ) * ( stringLength + 1 );
	memoryRequired = ( memoryRequired + sizeof( size_t ) ) & ~( sizeof( size_t ) - 1 );		// Align up to next word boundary

	// Search used pages for free memory first
	SNamePoolPage* usedPage = m_usedPages;
	while( usedPage )
	{
		size_t memoryAvailable = ( reinterpret_cast< size_t >( usedPage->m_rawBuffer ) + INTERNAL_PAGE_SIZE ) - reinterpret_cast< size_t >( usedPage->m_headPtr );
		if( memoryAvailable >= memoryRequired )
		{
			pageToUse = usedPage;
			break;
		}
		usedPage = usedPage->m_nextPoolPage;
	}

	if( pageToUse == nullptr )		// No used pages big enough, allocate a new one
	{
		RED_FATAL_ASSERT( m_freePages, "Ran out of CName pool entries. Please increase MAX_INTERNAL_PAGES or INTERNAL_PAGE_SIZE" );

		pageToUse = m_freePages;
		m_freePages = m_freePages->m_nextPoolPage;		
		pageToUse->m_nextPoolPage = m_usedPages;
		m_usedPages = pageToUse;
		pageToUse->m_headPtr = pageToUse->m_rawBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Names, INTERNAL_PAGE_SIZE );
	}

	void* newPtr = pageToUse->m_headPtr;
	pageToUse->m_headPtr = reinterpret_cast< void* >( reinterpret_cast< size_t >( pageToUse->m_headPtr ) + memoryRequired );
	return newPtr;
}

void CNamesPool::ReservePages( Uint32 count )
{
	Uint32 pageCount = 0;
	SNamePoolPage* usedPage = m_usedPages;
	while( usedPage )
	{
		usedPage = usedPage->m_nextPoolPage;
		++pageCount;
	}

	if( pageCount < count )
	{
		Uint32 pageToAdd = count - pageCount;
		while( pageToAdd-- )
		{
			RED_FATAL_ASSERT( m_freePages, "Ran out of CName pool entries. Please increase MAX_INTERNAL_PAGES or INTERNAL_PAGE_SIZE" );
			SNamePoolPage* pageToUse = m_freePages;
			m_freePages = m_freePages->m_nextPoolPage;		
			pageToUse->m_nextPoolPage = m_usedPages;
			m_usedPages = pageToUse;
			pageToUse->m_headPtr = pageToUse->m_rawBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Names, INTERNAL_PAGE_SIZE );
		}
	}
}

CNamesPool::TIndex CNamesPool::AddEntry( const Char* name )
{
	TIndex poolIndexRetVal = INDEX_NONE;
	SNameHolder* foundNameHolder = nullptr;

	{
		Red::Threads::CScopedLock< TLock > lock( m_mutex );

		const CNameHash nameHash = CNameHash::Hash( name );
		foundNameHolder = FindNameHolder_NoSync( nameHash );

		if ( ! foundNameHolder )
		{
			size_t len = Red::System::StringLength( name );
			SNameHolder* nameHolder = (SNameHolder*)AllocateNameHolder( len );
			Char* str = (Char*)( (Int8*)nameHolder  + sizeof( SNameHolder ) );
			Red::System::StringCopy( str, name, len + 1 );
			AnsiChar* strAnsi = (AnsiChar*)( (Int8*)nameHolder  + sizeof( SNameHolder ) + sizeof( Char ) * ( len + 1 ) );
			Red::System::StringCopy( strAnsi, UNICODE_TO_ANSI( name ), len + 1 );

			::new ( nameHolder ) SNameHolder( str, strAnsi, nameHash );

			LinkNameHolder_NoSync( nameHolder );

			m_indexedNameList.PushBack( nameHolder );
			nameHolder->m_poolIndex = m_indexedNameList.Size() - 1;
			poolIndexRetVal = nameHolder->m_poolIndex;
		}
		else
		{
			poolIndexRetVal = foundNameHolder->m_poolIndex;
		}
	} // Scoped mutex lock

#ifndef RED_FINAL_BUILD
	// Halt outside of the mutex lock or else can deadlock with the log system getting an unrelated CName log channel.
	if ( foundNameHolder && Red::System::StringCompare( foundNameHolder->m_name, name ) != 0 )
	{
		RED_HALT( "!!! CName collision detected !!! '%ls' vs '%ls' with hash '%u'. This requires immediate programmer attention", foundNameHolder->m_name, name, foundNameHolder->m_nameHash.GetValue() );
	}
#endif

	return poolIndexRetVal;
}

const Char* CNamesPool::FindText( TIndex index ) const
{
	Red::Threads::CScopedLock< TLock> lock( m_mutex );

	RED_ASSERT( index < m_indexedNameList.Size() );

	const SNameHolder* nameHolder = m_indexedNameList[ index ];
	RED_ASSERT( nameHolder );

	return nameHolder ? nameHolder->m_name : nullptr;
}

const AnsiChar* CNamesPool::FindTextAnsi( TIndex index ) const
{
	Red::Threads::CScopedLock< TLock > lock( m_mutex );
	
	RED_ASSERT( index < m_indexedNameList.Size() );

	const SNameHolder* nameHolder = m_indexedNameList[ index ];
	RED_ASSERT( nameHolder );

	return nameHolder ? nameHolder->m_nameAnsi : nullptr;
}

CNamesPool::CNameHash CNamesPool::GetSerializationHash( TIndex index ) const
{
	Red::Threads::CScopedLock< TLock > lock( m_mutex );

	RED_ASSERT( index < m_indexedNameList.Size() );

	const SNameHolder* nameHolder = m_indexedNameList[ index ];
	RED_ASSERT( nameHolder );

	return nameHolder ? nameHolder->m_nameHash : nullptr;
}

void CNamesPool::LinkNameHolder_NoSync( SNameHolder* nameHolder )
{
	const Uint32 index = nameHolder->m_nameHash.GetValue() % TABLE_SIZE_PRIME;

	SNameHolder* cur = m_head[ index ];
	if ( ! cur )
	{
		m_head[ index ] = nameHolder;
		nameHolder->m_next = nullptr;
		return;
	}

	// Go to list tail
	while ( cur->m_next )
	{
		cur = cur->m_next;
	}

	cur->m_next = nameHolder;
	nameHolder->m_next = nullptr;
}

CNamesPool::SNameHolder* CNamesPool::FindNameHolder_NoSync( CNameHash nameHash ) const
{
	const Uint32 index = nameHash.GetValue() % TABLE_SIZE_PRIME;

	for( SNameHolder* cur = m_head[ index ]; cur; cur = cur->m_next )
	{
		if ( cur->m_nameHash == nameHash )
		{
			return cur;
		}
	}

	return nullptr;
}
