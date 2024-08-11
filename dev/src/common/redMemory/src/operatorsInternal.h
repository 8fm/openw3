/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_OPERATORS_INTERNAL_H_
#define _RED_MEMORY_OPERATORS_INTERNAL_H_

//////////////////////////////////////////////////////////////////////////

#include "../include/types.h"

namespace red
{
namespace memory
{
namespace internal
{
	// TODO it should be possible to reduce the count of needed operator helper function...

	template< typename ObjectType >
	void * NewHelper();

	// This version is called from RED_NEW with a Pool Type directly.
	template< typename ObjectType, typename Proxy >
	void * NewHelper( Proxy proxy );

	template< typename ObjectType, typename Proxy >
	void * NewHelper( Proxy * proxy );

	template< typename ObjectType >
	ObjectType * NewArrayHelper( u32 count );

	// This version is called from RED_NEW_ARRAY with a Pool Type directly.
	template< typename ObjectType, typename Proxy >
	ObjectType * NewArrayHelper( Proxy proxy, u32 count );

	template< typename ObjectType, typename Proxy >
	ObjectType * NewArrayHelper( Proxy * proxy, u32 count );

	template< typename ObjectType >
	void DeleteHelper( ObjectType * ptr );

	template< typename Proxy, typename ObjectType >
	void DeleteHelper( Proxy, ObjectType * ptr );

	template< typename Proxy, typename ObjectType >
	void DeleteHelper( Proxy * proxy, ObjectType * ptr );

	template< typename ObjectType >
	void DeleteArrayHelper( ObjectType * ptr, u32 count );

	template< typename Proxy, typename ObjectType >
	void DeleteArrayHelper( Proxy, ObjectType * ptr, u32 count );

	template< typename Proxy, typename ObjectType >
	void DeleteArrayHelper( Proxy * proxy, ObjectType * ptr, u32 count );

	template< typename Proxy >
	void * AllocateHelper( Proxy, u32 size );

	template< typename Proxy >
	void * AllocateHelper( Proxy * proxy, u32 size );

	template< typename Proxy >
	void * AllocateAlignedHelper( Proxy, u32 size, u32 alignment );

	template< typename Proxy >
	void * AllocateAlignedHelper( Proxy * proxy, u32 size, u32 alignment );

	template< typename Proxy >
	void FreeHelper( Proxy, const void * block );

	template< typename Proxy >
	void FreeHelper( Proxy * proxy, const void * block );

	template< typename Proxy >
	void * ReallocateHelper( Proxy, void * block, u32 size );

	template< typename Proxy >
	void * ReallocateHelper( Proxy * proxy, void * block, u32 size );
	
	template< typename Proxy >
	void * ReallocateAlignedHelper( Proxy , void * block, u32 size, u32 alignment );

	template< typename Proxy >
	void * ReallocateAlignedHelper( Proxy * proxy, void * block, u32 size, u32 alignment );
}
}
}

//////////////////////////////////////////////////////////////////////////

#define _INTERNAL_RED_NEW( type )									new ( red::memory::internal::NewHelper< type >() ) type
#define _INTERNAL_RED_WITH_PROXY( type, proxy )						new ( red::memory::internal::NewHelper< type >( proxy() ) ) type
#define _INTERNAL_RED_NEW_ARRAY( type, count )						red::memory::internal::NewArrayHelper< type >( count )
#define _INTERNAL_RED_NEW_ARRAY_WITH_PROXY( type, count, proxy )	red::memory::internal::NewArrayHelper< type >( proxy(), count )

#define _INTERNAL_RED_DELETE( ptr )									do{ if( ptr ) red::memory::internal::DeleteHelper( ptr ); } while( 0, 0 )
#define _INTERNAL_RED_DELETE_WITH_PROXY( ptr, proxy )				do{ if( ptr ) red::memory::internal::DeleteHelper( proxy(), ptr ); } while( 0, 0 )
#define _INTERNAL_RED_DELETE_ARRAY( ptr, count )					do{ if( ptr ) red::memory::internal::DeleteArrayHelper( ptr, count ); } while( 0, 0 )
#define _INTERNAL_RED_DELETE_ARRAY_WITH_PROXY( ptr, count, proxy )	do{ if( ptr ) red::memory::internal::DeleteArrayHelper( proxy(), ptr, count ); } while( 0, 0 )

#define _INTERNAL_RED_NEW_1( type )			_INTERNAL_RED_NEW( type )
#define _INTERNAL_RED_NEW_2( type, proxy )	_INTERNAL_RED_WITH_PROXY( type, proxy )

#define _INTERNAL_RED_NEW_ARRAY_2( type, count )		_INTERNAL_RED_NEW_ARRAY( type, count )
#define _INTERNAL_RED_NEW_ARRAY_3( type, count, proxy )	_INTERNAL_RED_NEW_ARRAY_WITH_PROXY( type, count, proxy )

#define _INTERNAL_RED_DELETE_1( ptr )			_INTERNAL_RED_DELETE( ptr )
#define _INTERNAL_RED_DELETE_2( ptr, proxy )	_INTERNAL_RED_DELETE_WITH_PROXY( ptr, proxy )

#define _INTERNAL_RED_DELETE_ARRAY_2( ptr, count )			_INTERNAL_RED_DELETE_ARRAY( ptr, count )
#define _INTERNAL_RED_DELETE_ARRAY_3( ptr, count, proxy )	_INTERNAL_RED_DELETE_ARRAY_WITH_PROXY( ptr, count, proxy )

//////////////////////////////////////////////////////////////////////////

#include "operatorsInternal.hpp"

#endif
