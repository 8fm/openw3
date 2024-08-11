/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_ATOMIC_WEAK_PTR_HPP_
#define _RED_MEMORY_ATOMIC_WEAK_PTR_HPP_

namespace red
{
	template< typename T >
	RED_MEMORY_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr()
	{}
	
	template< typename T >
	RED_MEMORY_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr( const TAtomicWeakPtr & pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr( const TAtomicWeakPtr< U > & pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	template< typename U  >
	RED_MEMORY_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr( const TAtomicSharedPtr< U> & pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	RED_MEMORY_INLINE TAtomicWeakPtr< T > & TAtomicWeakPtr< T >::operator=( const TAtomicWeakPtr & pointer )
	{
		ParentType::operator=( pointer );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicWeakPtr< T > & TAtomicWeakPtr< T >::operator=( const TAtomicWeakPtr< U > & pointer )
	{
		ParentType::operator=( pointer );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicWeakPtr< T > & TAtomicWeakPtr< T >::operator=( const TAtomicSharedPtr< U > & pointer )
	{
		ParentType::operator=( pointer );
		return *this;
	}

	template< typename T >
	RED_MEMORY_INLINE TAtomicSharedPtr< T > TAtomicWeakPtr< T >::Lock() const
	{
		return TAtomicSharedPtr< T >( *this );
	}
}

#endif
