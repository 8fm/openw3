/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_WEAK_PTR_INL_
#define _CORE_WEAK_PTR_INL_

namespace Red
{
	template< typename T, template< typename > class Ownership >
	TWeakPtr< T, Ownership >::TWeakPtr()
	{}

	template< typename T, template< typename > class Ownership >
	TWeakPtr< T, Ownership >::TWeakPtr( const TWeakPtr & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRefWeak();
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	TWeakPtr< T, Ownership >::TWeakPtr( const TWeakPtr< U, Ownership  > & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRefWeak();
	}

	template< typename T, template< typename > class Ownership >
	template< typename U  >
	TWeakPtr< T, Ownership >::TWeakPtr( const TSharedPtr< U, Ownership  > & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRefWeak();
	}

	template< typename T, template< typename > class Ownership >
	TWeakPtr< T, Ownership >::~TWeakPtr()
	{
		ParentType::ReleaseWeak();
	}

	template< typename T, template< typename > class Ownership >
	TWeakPtr< T, Ownership > & TWeakPtr< T, Ownership >::operator=( const TWeakPtr & copyFrom )
	{
		TWeakPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	TWeakPtr< T, Ownership > & TWeakPtr< T, Ownership >::operator=( const TWeakPtr< U, Ownership  > & copyFrom )
	{
		TWeakPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	TWeakPtr< T, Ownership > & TWeakPtr< T, Ownership >::operator=( const TSharedPtr< U, Ownership  > & copyFrom )
	{
		TWeakPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	TSharedPtr< T, Ownership > TWeakPtr< T, Ownership >::Lock() const
	{
		return TSharedPtr< T, Ownership >( *this );
	}

	template< typename T, template< typename > class Ownership >
	bool TWeakPtr< T, Ownership >::Expired() const
	{
		return ParentType::GetRefCount() == 0;
	}
	
	template< typename T, template< typename > class Ownership >
	void TWeakPtr< T, Ownership >::Reset()
	{
		TWeakPtr().Swap( *this );
	}
	
	template< typename T, template< typename > class Ownership >
	void TWeakPtr< T, Ownership >::Swap( TWeakPtr & swapWith )
	{
		ParentType::Swap( swapWith );
	}
}

#endif
