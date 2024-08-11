/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace Red
{
	template< typename T >
	RED_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr()
	{}
	
	template< typename T >
	RED_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr( const TAtomicWeakPtr & pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	template< typename U >
	RED_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr( const TAtomicWeakPtr< U > & pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	template< typename U  >
	RED_INLINE TAtomicWeakPtr< T >::TAtomicWeakPtr( const TAtomicSharedPtr< U> & pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	RED_INLINE TAtomicWeakPtr< T > & TAtomicWeakPtr< T >::operator=( const TAtomicWeakPtr & pointer )
	{
		ParentType::operator=( pointer );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_INLINE TAtomicWeakPtr< T > & TAtomicWeakPtr< T >::operator=( const TAtomicWeakPtr< U > & pointer )
	{
		ParentType::operator=( pointer );
		return *this;
	}

	template< typename T >
	template< typename U >
	TAtomicWeakPtr< T > & TAtomicWeakPtr< T >::operator=( const TAtomicSharedPtr< U > & pointer )
	{
		ParentType::operator=( pointer );
		return *this;
	}

	template< typename T >
	TAtomicSharedPtr< T > TAtomicWeakPtr< T >::Lock() const
	{
		return TAtomicSharedPtr< T >( *this );
	}
}