/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_WEAK_PTR_HPP_
#define _RED_MEMORY_WEAK_PTR_HPP_

namespace red
{
	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership >::TWeakPtr()
	{}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership >::TWeakPtr( const TWeakPtr & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRefWeak();
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership >::TWeakPtr( const TWeakPtr< U, Ownership  > & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRefWeak();
	}

	template< typename T, template< typename > class Ownership >
	template< typename U  >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership >::TWeakPtr( const TSharedPtr< U, Ownership  > & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRefWeak();
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership >::~TWeakPtr()
	{
		ParentType::ReleaseWeak();
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership > & TWeakPtr< T, Ownership >::operator=( const TWeakPtr & copyFrom )
	{
		TWeakPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership > & TWeakPtr< T, Ownership >::operator=( const TWeakPtr< U, Ownership  > & copyFrom )
	{
		TWeakPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TWeakPtr< T, Ownership > & TWeakPtr< T, Ownership >::operator=( const TSharedPtr< U, Ownership  > & copyFrom )
	{
		TWeakPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership > TWeakPtr< T, Ownership >::Lock() const
	{
		return TSharedPtr< T, Ownership >( *this );
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE bool TWeakPtr< T, Ownership >::Expired() const
	{
		return ParentType::GetRefCount() == 0;
	}
	
	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE void TWeakPtr< T, Ownership >::Reset()
	{
		TWeakPtr().Swap( *this );
	}
	
	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE void TWeakPtr< T, Ownership >::Swap( TWeakPtr & swapWith )
	{
		ParentType::Swap( swapWith );
	}
}

#endif
