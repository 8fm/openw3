/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/nameHash.h"
#include "memory.h"
#include "dynarray.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CName;

#define RED_CNAME_NONETXT "None"

//////////////////////////////////////////////////////////////////////////
// CNamesPool
//////////////////////////////////////////////////////////////////////////
class CNamesPool : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	typedef Red::CNameHash CNameHash;
	typedef	Uint32 TIndex;

public:
	static const TIndex INDEX_NONE = 0;

public:
	struct SNameHolder : public Red::System::NonCopyable
	{
		const Char*								m_name;
		const AnsiChar*							m_nameAnsi;
		SNameHolder*							m_next;
		CNameHash								m_nameHash;
		TIndex									m_poolIndex;

		SNameHolder( const Char* name, const AnsiChar* nameAnsi, CNameHash nameHash );
	};

	struct SStaticNameHolder : public SNameHolder
	{
		SStaticNameHolder( const Char* name, const AnsiChar* nameAnsi, CNameHash nameHash );
	};

private:
	static const Uint32	NAME_RESERVE_SIZE	= 66000; //TBD: 66000 in old pool, maybe 40-50000 in some levels
	static const Uint32 TABLE_SIZE_PRIME	= 8191;
	static const Uint32 MAX_INTERNAL_PAGES  = 256;
	static const Uint32 INTERNAL_PAGE_SIZE	= 1024 * 1024;

private:
#ifdef RED_PLATFORM_ORBIS
	typedef Red::MemoryFramework::CAdaptiveMutexBase< Red::MemoryFramework::OSAPI::CAdaptiveMutexImpl > TLock;
#else
	typedef Red::Threads::CMutex TLock;
#endif
	mutable TLock m_mutex;
	
	SNameHolder*					m_head[ TABLE_SIZE_PRIME ];

private:
	//	A block of memory that can be used to allocate CNames
	struct SNamePoolPage
	{
		void* m_rawBuffer;
		void* m_headPtr;
		SNamePoolPage* m_nextPoolPage;
	};

	SNamePoolPage					m_pageHeaderPool[ MAX_INTERNAL_PAGES ];
	SNamePoolPage*					m_freePages;			// linked-list of free page headers
	SNamePoolPage*					m_usedPages;			// linked-list of used pages

	void* AllocateNameHolder( size_t stringLength );

private:
	TDynArray< SNameHolder*, MC_Names >	m_indexedNameList;

public:
									CNamesPool();
									~CNamesPool();

public:
	TIndex							AddDynamicPoolName( const Char* name );
	void							AddStaticPoolName( SNameHolder* nameHolder );
	const Char*						FindText( TIndex index ) const;
	const AnsiChar*					FindTextAnsi( TIndex index ) const;
	TIndex							GetIndexFromHash( CNameHash nameHash ) const;
	void							ReservePages( Uint32 count );

public:
	CNameHash						GetSerializationHash( TIndex index ) const;

private:
	TIndex							AddEntry( const Char* name );

private:
	void							LinkNameHolder_NoSync( SNameHolder* nameHolder );
	SNameHolder*					FindNameHolder_NoSync( CNameHash nameHash ) const;
};

namespace Red { namespace StaticNames {

} } // namespace Red { namespace StaticNames {

#define INTERNAL_DECLARE_NAME(x)						\
namespace Red { namespace StaticNames {					\
	extern ::CNamesPool::SStaticNameHolder holder_##x;	\
	extern const ::CName name_##x;						\
} }

#define INTERNAL_DEFINE_NAME(x)																\
namespace Red { namespace StaticNames {														\
	::CNamesPool::SStaticNameHolder holder_##x( TXT(#x), #x, ::Red::CNameHash(#x) );		\
	const ::CName name_##x( CName::CreateFromHash( ::Red::CNameHash(#x) ) );				\
} }

// Needs to be there regardless of names pool because of our RTTI system, which parses CName text values at runtime
#define RED_DECLARE_RTTI_NAME(x) INTERNAL_DECLARE_NAME(x)
#define RED_DEFINE_RTTI_NAME(x) INTERNAL_DEFINE_NAME(x)

// TBD: If no names pool, would need to change all the functions returning references or else returning reference to a temporary...
#ifndef NO_NAMES_POOL
# define RED_NAME(x) ::Red::StaticNames::name_##x
# define RED_DECLARE_NAME(x) INTERNAL_DECLARE_NAME(x)
# define RED_DEFINE_NAME(x) INTERNAL_DEFINE_NAME(x)
#else
# define RED_NAME(x) CName( ::Red::CNameHash( NAME_TXT(#x) ) )
# define RED_DECLARE_NAME(x)
# define RED_DEFINE_NAME(x)
#endif // !NO_NAMES_POOL

// For the cases where the String would make an invalid identifier
#ifndef NO_NAMES_POOL
#define RED_DECLARE_NAMED_NAME( _var_name, _string ) namespace Red { namespace StaticNames { extern ::CNamesPool::SStaticNameHolder holder_##_var_name; extern const ::CName name_##_var_name; } }
#define RED_DEFINE_NAMED_NAME( _var_name, _string ) namespace Red { namespace StaticNames { ::CNamesPool::SStaticNameHolder holder_##_var_name( TXT(_string), _string, ::Red::CNameHash(_string) ); const ::CName name_##_var_name( ::CName::CreateFromHash( ::Red::CNameHash(_string)) ); } }
#else
#define RED_DECLARE_NAMED_NAME( _var_name, _string )
#define RED_DEFINE_NAMED_NAME( _var_name, _string )
#endif

// First create the static name holder then can initialize a cname with just the hash, since the static init order is well defined here.
#ifndef NO_NAMES_POOL
# define RED_DEFINE_STATIC_NAME(x) namespace Red { namespace StaticNames { static ::CNamesPool::SStaticNameHolder holder_##x(TXT(#x), #x, ::Red::CNameHash(#x)); static const CName name_##x( ::CName::CreateFromHash( ::Red::CNameHash(#x)) ); } }
# define RED_DEFINE_STATIC_NAMED_NAME( _var_name, _string ) namespace Red { namespace StaticNames { static ::CNamesPool::SStaticNameHolder holder_##_var_name(TXT(_string), _string, ::Red::CNameHash(_string)); static const CName name_##_var_name( ::CName::CreateFromHash( ::Red::CNameHash(_string)) ); } }
#else
# define RED_DEFINE_STATIC_NAME(x)
# define RED_DEFINE_STATIC_NAMED_NAME( _var_name, _string )
#endif

// Temporary compat so as not to make a million trivial changes
#define CNAME RED_NAME

// REMOVE ME: Just don't want to spend forever fixing this garbage right now
#define RED_NAME_MAX_LENGTH 256

typedef TSingleton< CNamesPool > SNamesPool;