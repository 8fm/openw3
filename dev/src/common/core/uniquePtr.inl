/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace Red
{
	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >::TUniquePtr()
		:	m_pointee( nullptr ),
			m_destroyer()
	{ 
		static_assert(!std::is_pointer<DeleterType>::value, "constructed with null function pointer deleter"); 
	}

	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >::TUniquePtr( PtrType * pointer )
		:	m_pointee( pointer ),
			m_destroyer()
	{ 
		static_assert( !std::is_pointer< DeleterType >::value, "constructed with null function pointer deleter"); 
	}

	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >::TUniquePtr( PtrType * pointer, const DeleterType & destroyer )
		:	m_pointee( pointer ),
			m_destroyer( destroyer )	
	{}

	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >::TUniquePtr( PtrType * pointer, DeleterType && destroyer )
		:	m_pointee( std::move( pointer ) ), 
			m_destroyer( std::move( destroyer ) )
	{ 
		static_assert( !std::is_reference< DeleterType >::value, "rvalue deleter bound to reference"); 
	}

	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >::TUniquePtr( TUniquePtr&& moveFrom ) 
		:	m_pointee( moveFrom.Release() ), 
			m_destroyer( std::forward< DeleterType >( moveFrom.GetDeleter() ) ) 
	{}

	template< typename PtrType, typename DeleterType >
	template< typename U, typename V > 
	TUniquePtr< PtrType, DeleterType >::TUniquePtr( TUniquePtr< U, V > && moveFrom ) 
		:	m_pointee( moveFrom.Release() ), 
			m_destroyer( std::forward< DeleterType >( moveFrom.GetDeleter() ) )
	{}

	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >::~TUniquePtr() 
	{ 
		m_destroyer( m_pointee );
	}

	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >& TUniquePtr< PtrType, DeleterType >::operator=(TUniquePtr&& moveFrom)
	{ 
		Reset( moveFrom.Release() ); 
		m_destroyer = std::move( moveFrom.m_destroyer ); 
		return *this;
	}

	template< typename PtrType, typename DeleterType >
	template<typename U, typename V > 
	TUniquePtr< PtrType, DeleterType >& TUniquePtr< PtrType, DeleterType >::operator=( TUniquePtr< U, V > && moveFrom)
	{
		Reset( moveFrom.Release() ); 
		m_destroyer = std::move( moveFrom.GetDeleter() ); 
		return *this;
	}

	template< typename PtrType, typename DeleterType >
	PtrType & TUniquePtr< PtrType, DeleterType >::operator*() const
	{
		RED_FATAL_ASSERT( m_pointee != nullptr, "null pointer access is illegal." );
		return *m_pointee;
	}

	template< typename PtrType, typename DeleterType >
	PtrType * TUniquePtr< PtrType, DeleterType >::operator->() const
	{
		RED_FATAL_ASSERT( m_pointee != nullptr, "null pointer access is illegal." );
		return m_pointee;
	}

	template< typename PtrType, typename DeleterType >
	PtrType * TUniquePtr< PtrType, DeleterType >::Get() const
	{
		return m_pointee; 
	}

	template< typename PtrType, typename DeleterType >
	TUniquePtr< PtrType, DeleterType >::operator typename TUniquePtr< PtrType, DeleterType >::bool_operator () const
	{ 
		return m_pointee != nullptr ? &BoolConversion::valid : 0;
	}

	template< typename PtrType, typename DeleterType >
	bool TUniquePtr< PtrType, DeleterType >::operator !() const
	{
		return m_pointee == nullptr;
	}

	template< typename PtrType, typename DeleterType >
	PtrType * TUniquePtr< PtrType, DeleterType >::Release() 
	{
		PtrType * pointer = m_pointee;
		m_pointee = 0;
		return pointer;
	}

	template< typename PtrType, typename DeleterType >
	void TUniquePtr< PtrType, DeleterType >::Reset( PtrType * pointer )
	{
		TUniquePtr< PtrType, DeleterType >( pointer ).Swap( *this );
	}

	template< typename PtrType, typename DeleterType >
	void TUniquePtr< PtrType, DeleterType >::Swap( TUniquePtr & swapWith )
	{
		::Swap( m_pointee, swapWith.m_pointee );
		::Swap( m_destroyer, swapWith.m_destroyer );
	}

	template< typename PtrType, typename DeleterType >
	DeleterType & TUniquePtr< PtrType, DeleterType >::GetDeleter()
	{
		return m_destroyer;
	}

	template< typename PtrType, typename DeleterType >
	const DeleterType & TUniquePtr< PtrType, DeleterType >::GetDeleter() const
	{
		return m_destroyer;
	}

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator==( const TUniquePtr< LeftType, DeleterType> & leftPtr, const TUniquePtr< RightType, DeleterType> & rightPtr )
	{
		return leftPtr.Get() == rightPtr.Get();
	}

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator!=( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr )
	{
		return leftPtr.Get() != rightPtr.Get();
	}

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator<( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr )
	{
		return leftPtr.Get() < rightPtr.Get();
	}
}
