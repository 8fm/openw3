/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_INTRUSIVE_PTR_HPP_
#define _RED_MEMORY_INTRUSIVE_PTR_HPP_

namespace red
{
	template< typename T >
	RED_MEMORY_INLINE TIntrusivePtr< T >::TIntrusivePtr()
	{}

	template< typename T >
	RED_MEMORY_INLINE TIntrusivePtr< T >::TIntrusivePtr( const TIntrusivePtr & copyFrom )
		: ParentType( copyFrom )
	{}

	template< typename T >
	RED_MEMORY_INLINE TIntrusivePtr< T >::TIntrusivePtr( TIntrusivePtr && rvalue )
		: ParentType( std::forward< TIntrusivePtr >( rvalue ) )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TIntrusivePtr< T >::TIntrusivePtr( U * pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TIntrusivePtr< T >::TIntrusivePtr( const TIntrusivePtr< U > & copyFrom )
		: ParentType( copyFrom )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TIntrusivePtr< T >::TIntrusivePtr( TIntrusivePtr< U > && rvalue )
		: ParentType( std::forward< TIntrusivePtr< U > >( rvalue ) )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TIntrusivePtr< T >::TIntrusivePtr( TUniquePtr< U > && rvalue )
		: ParentType( std::forward< TUniquePtr< U > >( rvalue ) )
	{}

	template< typename T >
	RED_MEMORY_INLINE TIntrusivePtr< T > & TIntrusivePtr< T >::operator=( const TIntrusivePtr & copyFrom )
	{
		ParentType::operator =( copyFrom );
		return *this;
	}

	template< typename T >
	RED_MEMORY_INLINE TIntrusivePtr< T > & TIntrusivePtr< T >::operator=( TIntrusivePtr && rvalue )
	{
		ParentType::operator =( std::forward< TIntrusivePtr >( rvalue ) );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TIntrusivePtr< T > & TIntrusivePtr< T >::operator=( const TIntrusivePtr< U >  & copyFrom )
	{
		ParentType::operator =( copyFrom );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TIntrusivePtr< T > & TIntrusivePtr< T >::operator=( TIntrusivePtr< U > && rvalue )
	{
		ParentType::operator =( std::forward< TIntrusivePtr< U > >( rvalue ) );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TIntrusivePtr< T > & TIntrusivePtr< T >::operator=( TUniquePtr< U > && rvalue )
	{
		ParentType::operator =( std::forward< TUniquePtr< U > >( rvalue ) );
		return *this;
	}

	template< typename T >
	RED_MEMORY_INLINE IntrusiveOwnership< T >::IntrusiveOwnership()
		: m_pointee( nullptr )
	{}

	template< typename T >
	RED_MEMORY_INLINE IntrusiveOwnership< T >::IntrusiveOwnership( PtrType pointer )
		: m_pointee( pointer )
	{}

	template< typename T >
	RED_MEMORY_INLINE IntrusiveOwnership< T >::IntrusiveOwnership( const IntrusiveOwnership & copyFrom )
		: m_pointee( copyFrom.m_pointee )
	{}

	template< typename T >
	RED_MEMORY_INLINE IntrusiveOwnership< T >::IntrusiveOwnership( IntrusiveOwnership && rvalue )
		: m_pointee( nullptr )
	{
		AssignRValue( std::forward< IntrusiveOwnership >( rvalue ) );
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE IntrusiveOwnership< T >::IntrusiveOwnership( const IntrusiveOwnership< U > & copyFrom )
		: m_pointee( reinterpret_cast< const IntrusiveOwnership & >(copyFrom).m_pointee  )
	{
		static_assert( std::is_base_of< T, U >::value, "TSharedPtr can't be constructed from non related type." );
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE IntrusiveOwnership< T >::IntrusiveOwnership( IntrusiveOwnership< U > && rvalue )
		: m_pointee( reinterpret_cast< IntrusiveOwnership & >(rvalue).m_pointee  )
	{
		static_assert( std::is_base_of< T, U >::value, "TSharedPtr can't be constructed from non related type." );
		reinterpret_cast< IntrusiveOwnership & >(rvalue).m_pointee = nullptr;
	}

	template< typename T >
	RED_MEMORY_INLINE void IntrusiveOwnership< T >::Release()
	{
		// Here we expect T::Release to return the current count of reference. 
		// If returned count is 0, release reference. 
		if( m_pointee && m_pointee->Release() == 0 )
		{
			RED_DELETE( m_pointee );
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void IntrusiveOwnership< T >::AddRef()
	{
		if( m_pointee )
		{
			m_pointee->AddRef();
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void IntrusiveOwnership< T >::Swap( IntrusiveOwnership & swapWith )
	{
		std::swap( m_pointee, swapWith.m_pointee );
	}

	template< typename T >
	RED_MEMORY_INLINE T* IntrusiveOwnership< T >::Get() const
	{
		return m_pointee;
	}

	template< typename T >
	RED_MEMORY_INLINE void IntrusiveOwnership< T >::AssignRValue( IntrusiveOwnership && rvalue )
	{
		if( this != &rvalue )
		{
			Swap( rvalue );
		}
	}
}

#endif
