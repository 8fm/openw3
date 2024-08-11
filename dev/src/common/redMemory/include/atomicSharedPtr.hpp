/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_ATOMIC_SHARED_PTR_HPP_
#define _RED_MEMORY_ATOMIC_SHARED_PTR_HPP_

namespace red
{
	template< typename T >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr()
	{}

	template< typename T >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr( const TAtomicSharedPtr & copyFrom )
		: ParentType( copyFrom )
	{}

	template< typename T >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr( TAtomicSharedPtr && rvalue )
		: ParentType( std::forward< TAtomicSharedPtr >( rvalue ) )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr( U * pointer )
		: ParentType( pointer )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr( const TAtomicSharedPtr< U > & copyFrom )
		: ParentType( copyFrom )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr( TAtomicSharedPtr< U > && rvalue )
		: ParentType( std::forward< TAtomicSharedPtr< U > >( rvalue ) )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr( const TAtomicWeakPtr< U > & copyFrom )
		: ParentType( copyFrom )
	{}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T >::TAtomicSharedPtr( TUniquePtr< U > && rvalue )
		: ParentType( std::forward< TUniquePtr< U > >( rvalue ) )
	{}

	template< typename T >
	RED_MEMORY_INLINE TAtomicSharedPtr< T > & TAtomicSharedPtr< T >::operator=( const TAtomicSharedPtr & copyFrom )
	{
		ParentType::operator =( copyFrom );
		return *this;
	}

	template< typename T >
	RED_MEMORY_INLINE TAtomicSharedPtr< T > & TAtomicSharedPtr< T >::operator=( TAtomicSharedPtr && rvalue )
	{
		ParentType::operator =( std::forward< TAtomicSharedPtr >( rvalue ) );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T > & TAtomicSharedPtr< T >::operator=( const TAtomicSharedPtr< U >  & copyFrom )
	{
		ParentType::operator =( copyFrom );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T > & TAtomicSharedPtr< T >::operator=( TAtomicSharedPtr< U > && rvalue )
	{
		ParentType::operator =( std::forward< TAtomicSharedPtr< U > >( rvalue ) );
		return *this;
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE TAtomicSharedPtr< T > & TAtomicSharedPtr< T >::operator=( TUniquePtr< U > && rvalue )
	{
		ParentType::operator =( std::forward< TUniquePtr< U > >( rvalue ) );
		return *this;
	}

	template< typename T >
	RED_MEMORY_INLINE AtomicRefCountingOwnership< T >::AtomicRefCountingOwnership()
		:	m_refCount( nullptr ),
			m_pointee( nullptr )
	{}

	template< typename T >
	RED_MEMORY_INLINE AtomicRefCountingOwnership< T >::AtomicRefCountingOwnership( PtrType ptr )
		:	m_refCount( RED_NEW( AtomicRefCount )( 1, 1 ) ),
			m_pointee( ptr )
	{}

	template< typename T >
	RED_MEMORY_INLINE AtomicRefCountingOwnership< T >::AtomicRefCountingOwnership( const AtomicRefCountingOwnership & copyFrom )
		:	m_refCount( copyFrom.m_refCount ),
			m_pointee( copyFrom.m_pointee )
	{}

	template< typename T >
	RED_MEMORY_INLINE AtomicRefCountingOwnership< T >::AtomicRefCountingOwnership( AtomicRefCountingOwnership && rvalue )
		:	m_refCount( nullptr ),
			m_pointee( nullptr )
	{
		AssignRValue( std::forward< AtomicRefCountingOwnership >( rvalue ) );
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE AtomicRefCountingOwnership< T >::AtomicRefCountingOwnership( const AtomicRefCountingOwnership< U > & copyFrom )
		:	m_refCount( reinterpret_cast< const AtomicRefCountingOwnership & >(copyFrom).m_refCount ),
			m_pointee( reinterpret_cast< const AtomicRefCountingOwnership & >(copyFrom).m_pointee )
	{
		static_assert( std::is_base_of< T, U >::value, "TAtomicSharedPtr can't be constructed from non related type." );
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE AtomicRefCountingOwnership< T >::AtomicRefCountingOwnership( AtomicRefCountingOwnership< U > && rvalue )
		:	m_refCount( reinterpret_cast< AtomicRefCountingOwnership & >(rvalue).m_refCount ),
			m_pointee( reinterpret_cast< AtomicRefCountingOwnership & >(rvalue).m_pointee )
	{
		static_assert( std::is_base_of< T, U >::value, "TAtomicSharedPtr can't be constructed from non related type." );
		reinterpret_cast< AtomicRefCountingOwnership & >(rvalue).m_refCount = nullptr;
		reinterpret_cast< AtomicRefCountingOwnership & >(rvalue).m_pointee = nullptr;
	}

	template< typename T >
	RED_MEMORY_INLINE void AtomicRefCountingOwnership< T >::Release()
	{
		if( m_refCount && red::atomic::Decrement32( &m_refCount->strong ) == 0 )
		{
			ReleaseWeak();
			static_assert( !std::is_polymorphic< T >::value || std::has_virtual_destructor< T >::value, "Virtual dtor is missing." );
			static_assert( sizeof( T ) > 0, "Cannot delete pointer to incomplete type. Did you forgot the include?" );
			RED_DELETE( m_pointee );
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void AtomicRefCountingOwnership< T >::ReleaseWeak()
	{
		if( m_refCount && atomic::Decrement32( &m_refCount->weak ) == 0 )
		{
			RED_DELETE( m_refCount );
			m_refCount = nullptr;
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void AtomicRefCountingOwnership< T >::AddRef()
	{
		if( m_refCount )
		{
			atomic::Increment32( &m_refCount->strong );
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void AtomicRefCountingOwnership< T >::AddRefWeak()
	{
		if( m_refCount )
		{
			atomic::Increment32( &m_refCount->weak );
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void AtomicRefCountingOwnership< T >::Swap( AtomicRefCountingOwnership & swapWith )
	{
		std::swap( m_refCount, swapWith.m_refCount );
		std::swap( m_pointee, swapWith.m_pointee );
	}

	template< typename T >
	RED_MEMORY_INLINE typename AtomicRefCountingOwnership< T >::RefCountType AtomicRefCountingOwnership< T >::GetRefCount() const
	{
		return m_refCount ? atomic::Or32( &m_refCount->strong, RefCountType(0) ) : 0;
	}

	template< typename T >
	RED_MEMORY_INLINE typename AtomicRefCountingOwnership< T >::RefCountType AtomicRefCountingOwnership< T >::GetWeakRefCount() const
	{
		return m_refCount ? atomic::Or32( &m_refCount->weak, RefCountType(0) ) : 0;
	}

	template< typename T >
	RED_MEMORY_INLINE void AtomicRefCountingOwnership< T >::UpgradeFromWeakToStrong( AtomicRefCountingOwnership & upgradeFrom )
	{
		if( upgradeFrom.m_refCount ) // if true, always true for the lifetime of this function
		{
			while( 1 )
			{
				RefCountType count = atomic::Or32( &upgradeFrom.m_refCount->strong, RefCountType(0) );
				if( count == 0 )
				{
					return;
				}

				if( atomic::CompareExchange32( &upgradeFrom.m_refCount->strong, count + 1, count ) == count )
				{
					m_refCount = upgradeFrom.m_refCount;
					m_pointee = upgradeFrom.m_pointee;
					return;
				}
			}
		}
	}

	template< typename T >
	RED_MEMORY_INLINE T* AtomicRefCountingOwnership< T >::Get() const
	{
		return m_pointee;
	}

	template< typename T >
	RED_MEMORY_INLINE void AtomicRefCountingOwnership< T >::AssignRValue( AtomicRefCountingOwnership && rvalue )
	{
		if( this != &rvalue )
		{
			Swap( rvalue );
		}
	}

}

#endif
