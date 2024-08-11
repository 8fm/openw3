/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_UNIQUE_PTR_HPP_
#define _RED_MEMORY_UNIQUE_PTR_HPP_

#include "assert.h"

namespace red
{
	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::TUniquePtr()
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::TUniquePtr( PtrType * pointer )
		:	m_storage( pointer )
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::TUniquePtr( PtrType * pointer, const DeleterType & destroyer )
		:	m_storage( pointer, destroyer )
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::TUniquePtr( PtrType * pointer, DeleterType && destroyer )
		:	m_storage( std::move( pointer ), std::forward< DeleterType >( destroyer ) )
	{ 
		static_assert( !std::is_reference< DeleterType >::value, "rvalue deleter bound to reference."); 
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::TUniquePtr( TUniquePtr&& moveFrom ) 
		:	m_storage( std::forward< StorageType >( moveFrom.m_storage ) )
	{}

	template< typename PtrType, typename DeleterType >
	template< typename U, typename V > 
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::TUniquePtr( TUniquePtr< U, V > && moveFrom ) 
		:	m_storage(  moveFrom.Release(), std::forward< DeleterType >( moveFrom.GetDeleter() ) )
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::~TUniquePtr() 
	{}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >& TUniquePtr< PtrType, DeleterType >::operator=( TUniquePtr && moveFrom )
	{ 
		TUniquePtr( std::move( moveFrom ) ).Swap( *this );
		return *this;
	}

	template< typename PtrType, typename DeleterType >
	template<typename U, typename V > 
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >& TUniquePtr< PtrType, DeleterType >::operator=( TUniquePtr< U, V > && moveFrom )
	{
		TUniquePtr( std::move( moveFrom ) ).Swap( *this );
		return *this;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE PtrType & TUniquePtr< PtrType, DeleterType >::operator*() const
	{
		PtrType * ptr = m_storage.Get();
		RED_MEMORY_ASSERT( ptr != nullptr, "null pointer access is illegal." );
		return *ptr;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE PtrType * TUniquePtr< PtrType, DeleterType >::operator->() const
	{
		PtrType * ptr = m_storage.Get();
		RED_MEMORY_ASSERT( ptr != nullptr, "null pointer access is illegal." );
		return ptr;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE PtrType * TUniquePtr< PtrType, DeleterType >::Get() const
	{
		return m_storage.Get(); 
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE TUniquePtr< PtrType, DeleterType >::operator typename TUniquePtr< PtrType, DeleterType >::bool_operator () const
	{ 
		return m_storage.Get() != nullptr ? &BoolConversion::valid : 0;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE bool TUniquePtr< PtrType, DeleterType >::operator !() const
	{
		return m_storage.Get() == nullptr;
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE PtrType * TUniquePtr< PtrType, DeleterType >::Release() 
	{
		return m_storage.Release();
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE void TUniquePtr< PtrType, DeleterType >::Reset( PtrType * pointer )
	{
		TUniquePtr< PtrType, DeleterType >( pointer ).Swap( *this );
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE void TUniquePtr< PtrType, DeleterType >::Swap( TUniquePtr & swapWith )
	{
		m_storage.Swap( swapWith.m_storage );
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE DeleterType & TUniquePtr< PtrType, DeleterType >::GetDeleter()
	{
		return m_storage.GetDestructor();
	}

	template< typename PtrType, typename DeleterType >
	RED_MEMORY_INLINE const DeleterType & TUniquePtr< PtrType, DeleterType >::GetDeleter() const
	{
		return m_storage.GetDestructor();
	}

	template< typename LeftType, typename RightType, typename DeleterType >
	RED_MEMORY_INLINE bool operator==( const TUniquePtr< LeftType, DeleterType> & leftPtr, const TUniquePtr< RightType, DeleterType> & rightPtr )
	{
		return leftPtr.Get() == rightPtr.Get();
	}

	template< typename LeftType, typename RightType, typename DeleterType >
	RED_MEMORY_INLINE bool operator!=( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr )
	{
		return leftPtr.Get() != rightPtr.Get();
	}

	template< typename LeftType, typename RightType, typename DeleterType >
	RED_MEMORY_INLINE bool operator<( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr )
	{
		return leftPtr.Get() < rightPtr.Get();
	}
}

#endif
