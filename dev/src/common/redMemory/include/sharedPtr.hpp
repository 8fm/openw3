/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SHARED_PTR_HPP_
#define _RED_MEMORY_SHARED_PTR_HPP_

#include "operators.h"

namespace red
{
	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr()
	{}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr( U * pointer )
		:	ParentType( pointer )
	{}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr( const TSharedPtr & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRef();
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr( TSharedPtr && rvalue )
		:	ParentType( std::forward< TSharedPtr >( rvalue ) )
	{}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr( const TSharedPtr< U, Ownership > & copyFrom )
		:	ParentType( copyFrom )
	{
		ParentType::AddRef();
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr( TSharedPtr< U, Ownership > && rvalue )
		:	ParentType( std::forward< TSharedPtr< U, Ownership > >( rvalue ) )
	{}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr( const TWeakPtr< U, Ownership > & copyFrom )
	{
		ParentType::UpgradeFromWeakToStrong( const_cast< TWeakPtr< U, Ownership > & >( copyFrom ) );
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::TSharedPtr( TUniquePtr< U > && rvalue )
	{
		Reset( rvalue.Release() );
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::~TSharedPtr()
	{
		ParentType::Release();
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership > & TSharedPtr< T, Ownership >::operator=( const TSharedPtr & copyFrom )
	{
		TSharedPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership > & TSharedPtr< T, Ownership >::operator=( TSharedPtr && rvalue )
	{
		TSharedPtr( std::move( rvalue ) ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership > & TSharedPtr< T, Ownership >::operator=( const TSharedPtr< U, Ownership >  & copyFrom )
	{
		TSharedPtr( copyFrom ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership > & TSharedPtr< T, Ownership >::operator=( TSharedPtr< U, Ownership > && rvalue )
	{
		TSharedPtr( std::move( rvalue ) ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership > & TSharedPtr< T, Ownership >::operator=( TUniquePtr< U > && rvalue )
	{
		TSharedPtr( std::move( rvalue ) ).Swap( *this );
		return *this;
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE typename TSharedPtr< T, Ownership >::PtrType TSharedPtr< T, Ownership >::Get() const
	{
		return ParentType::Get();
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE typename TSharedPtr< T, Ownership >::PtrType TSharedPtr< T, Ownership >::operator->() const
	{
		RED_MEMORY_ASSERT( ParentType::Get(), "null pointer access is illegal." );
		return ParentType::Get();
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE typename TSharedPtr< T, Ownership >::RefType TSharedPtr< T, Ownership >::operator*() const
	{
		RED_MEMORY_ASSERT( ParentType::Get(), "null pointer access is illegal." );
		return *ParentType::Get();
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE void TSharedPtr< T, Ownership >::Reset()
	{
		TSharedPtr().Swap( *this );
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE void TSharedPtr< T, Ownership >::Reset( PtrType pointer )
	{
		TSharedPtr( pointer ).Swap( *this );
	}

	template< typename T, template< typename > class Ownership >
	template< typename U >
	RED_MEMORY_INLINE void TSharedPtr< T, Ownership >::Reset( U * pointer )
	{
		TSharedPtr( pointer ).Swap( *this );
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE void TSharedPtr< T, Ownership >::Swap( TSharedPtr & swapWith )
	{
		ParentType::Swap( swapWith );
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE TSharedPtr< T, Ownership >::operator typename TSharedPtr< T, Ownership >::bool_operator() const
	{
		return ParentType::Get() != nullptr ? &BoolConversion::valid : 0;
	}

	template< typename T, template< typename > class Ownership >
	RED_MEMORY_INLINE bool TSharedPtr< T, Ownership >::operator!() const
	{
		return !ParentType::Get();
	}

	template< typename LeftType, template< typename > class LeftOwnership, typename RightType, template< typename > class RightOwnership >
	RED_MEMORY_INLINE bool operator==( const TSharedPtr< LeftType, LeftOwnership> & leftPtr, const TSharedPtr< RightType, RightOwnership> & rightPtr )
	{
		return leftPtr.Get() == rightPtr.Get();
	}

	template< typename LeftType, template< typename > class LeftOwnership, typename RightType, template< typename > class RightOwnership >
	RED_MEMORY_INLINE bool operator!=( const TSharedPtr< LeftType, LeftOwnership > & leftPtr, const TSharedPtr< RightType, RightOwnership > & rightPtr )
	{
		return leftPtr.Get() != rightPtr.Get();
	}

	template< typename LeftType, template< typename > class LeftOwnership, typename RightType, template< typename > class RightOwnership >
	RED_MEMORY_INLINE bool operator<( const TSharedPtr< LeftType, LeftOwnership > & leftPtr, const TSharedPtr< RightType, RightOwnership > & rightPtr )
	{
		return leftPtr.Get() < rightPtr.Get();
	}

	template< typename T >
	RED_MEMORY_INLINE DefaultOwnership< T >::DefaultOwnership()
		:	m_refCount( nullptr ),
			m_pointee( nullptr )
	{}

	template< typename T >
	RED_MEMORY_INLINE DefaultOwnership< T >::DefaultOwnership( PtrType pointer )
		:	m_refCount( RED_NEW( DefaultOwnershipRefCount )( 1, 1 ) ),
			m_pointee( pointer )
	{}

	template< typename T >
	RED_MEMORY_INLINE DefaultOwnership< T >::DefaultOwnership( const DefaultOwnership & copyFrom )
		:	m_refCount( copyFrom.m_refCount ),
			m_pointee( copyFrom.m_pointee )
	{}

	template< typename T >
	RED_MEMORY_INLINE DefaultOwnership< T >::DefaultOwnership( DefaultOwnership && rvalue )
		:	m_refCount( nullptr ),
			m_pointee( nullptr )
	{
		AssignRValue( std::forward< DefaultOwnership >( rvalue ) );
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE DefaultOwnership< T >::DefaultOwnership( const DefaultOwnership< U > & copyFrom )
		:	m_refCount( reinterpret_cast< const DefaultOwnership & >(copyFrom).m_refCount ),
			m_pointee( reinterpret_cast< const DefaultOwnership & >(copyFrom).m_pointee )
	{
		static_assert( std::is_base_of< T, U >::value, "TSharedPtr can't be constructed from non related type." );
	}

	template< typename T >
	template< typename U >
	RED_MEMORY_INLINE DefaultOwnership< T >::DefaultOwnership( DefaultOwnership< U > && rvalue )
		:	m_refCount( reinterpret_cast< DefaultOwnership & >(rvalue).m_refCount ),
			m_pointee( reinterpret_cast< DefaultOwnership & >(rvalue).m_pointee )
	{
		static_assert( std::is_base_of< T, U >::value, "TSharedPtr can't be constructed from non related type." );
		reinterpret_cast< DefaultOwnership & >(rvalue).m_refCount = nullptr;
		reinterpret_cast< DefaultOwnership & >(rvalue).m_pointee = nullptr;
	}

	template< typename T >
	RED_MEMORY_INLINE void DefaultOwnership< T >::Release()
	{
		if( m_refCount && --(m_refCount->strong) == 0 )
		{
			ReleaseWeak();
			static_assert( !std::is_polymorphic< T >::value || std::has_virtual_destructor< T >::value, "Virtual dtor is missing." );
			static_assert( sizeof( T ) > 0, "Cannot delete pointer to incomplete type. Did you forgot the include?" );
			RED_DELETE( m_pointee );
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void DefaultOwnership< T >::ReleaseWeak()
	{
		if( m_refCount && --(m_refCount->weak) == 0 )
		{
			RED_DELETE( m_refCount );
			m_refCount = nullptr;
		}
	}
	
	template< typename T >
	RED_MEMORY_INLINE void DefaultOwnership< T >::AddRef()
	{
		if( m_refCount )
		{
			++(m_refCount->strong);
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void DefaultOwnership< T >::AddRefWeak()
	{
		if( m_refCount )
		{
			++(m_refCount->weak);
		}
	}

	template< typename T >
	RED_MEMORY_INLINE void DefaultOwnership< T >::Swap( DefaultOwnership & swapWith )
	{
		std::swap( m_refCount, swapWith.m_refCount );
		std::swap( m_pointee, swapWith.m_pointee );  
	}

	template< typename T >
	RED_MEMORY_INLINE typename DefaultOwnership< T >::RefCountType DefaultOwnership< T >::GetRefCount() const
	{
		return m_refCount ? m_refCount->strong : 0;
	}

	template< typename T >
	RED_MEMORY_INLINE typename DefaultOwnership< T >::RefCountType DefaultOwnership< T >::GetWeakRefCount() const
	{
		return m_refCount ? m_refCount->weak : 0;
	}

	template< typename T >
	RED_MEMORY_INLINE void DefaultOwnership< T >::UpgradeFromWeakToStrong( DefaultOwnership & upgradeFrom )
	{
		// NOT THREAD SAFE !!! Use Atomic Version...

		if( upgradeFrom.m_refCount && upgradeFrom.m_refCount->strong != 0 )
		{
			++upgradeFrom.m_refCount->strong;
			m_refCount = upgradeFrom.m_refCount;
			m_pointee = upgradeFrom.m_pointee;
		}
	}

	template< typename T >
	RED_MEMORY_INLINE T* DefaultOwnership< T >::Get() const
	{
		return m_pointee;
	}

	template< typename T >
	RED_MEMORY_INLINE void DefaultOwnership< T >::AssignRValue( DefaultOwnership && rvalue )
	{
		if( this != &rvalue )
		{
			Swap( rvalue );
		}
	}
}

#endif 
