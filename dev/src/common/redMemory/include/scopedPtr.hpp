/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SCOPED_PTR_HPP_
#define _RED_MEMORY_SCOPED_PTR_HPP_

namespace red
{
	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TScopedPtr< PtrType, DeleterType >::TScopedPtr( PtrType * object )
		:   m_storage( object )
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TScopedPtr< PtrType, DeleterType >::TScopedPtr( PtrType * object, DeleterType destroyer )
		:   m_storage( object, destroyer )
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TScopedPtr< PtrType, DeleterType >::~TScopedPtr()
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE  PtrType * TScopedPtr< PtrType, DeleterType >::Get() const
	{
		return m_storage.Get();
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE void TScopedPtr< PtrType, DeleterType >::Reset( PtrType * pointer )
	{
		TScopedPtr< PtrType, DeleterType >( pointer ).Swap( *this );
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE void TScopedPtr< PtrType, DeleterType >::Swap( TScopedPtr< PtrType, DeleterType > & object )
	{
		m_storage.Swap( object.m_storage );
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE PtrType * TScopedPtr< PtrType, DeleterType >::operator->() const
	{
		PtrType * ptr = m_storage.Get();
		RED_MEMORY_ASSERT( ptr != nullptr, "null pointer access is illegal." );
		return ptr;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE PtrType & TScopedPtr< PtrType, DeleterType >::operator*() const
	{
		PtrType * ptr = m_storage.Get();
		RED_MEMORY_ASSERT( ptr != nullptr, "null pointer access is illegal." );
		return *ptr;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TScopedPtr< PtrType, DeleterType >::operator typename TScopedPtr< PtrType, DeleterType >::bool_operator() const
	{
		return m_storage.Get() != nullptr ? &BoolConversion::valid : 0;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE bool TScopedPtr< PtrType, DeleterType >::operator !() const
	{
		return m_storage.Get() == nullptr;
	}
}

#endif
