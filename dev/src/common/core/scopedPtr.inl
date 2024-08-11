/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../redSystem/error.h"

namespace Red
{
	template< typename PtrType, typename DeleterType >
	RED_INLINE TScopedPtr< PtrType, DeleterType >::TScopedPtr( PtrType * object )
		:   m_pointee( object ),
			m_destroyer()
	{
		static_assert( !std::is_pointer< DeleterType >::value, "Constructed with null function deleter!" );
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE TScopedPtr< PtrType, DeleterType >::TScopedPtr( PtrType * object, DeleterType destroyer )
		:   m_pointee( object ),
			m_destroyer( destroyer )
	{}

	template< typename PtrType, typename DeleterType >
	RED_INLINE TScopedPtr< PtrType, DeleterType >::~TScopedPtr()
	{
		m_destroyer(m_pointee);
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE  PtrType * TScopedPtr< PtrType, DeleterType >::Get() const
	{
		return m_pointee;
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE void TScopedPtr< PtrType, DeleterType >::Reset( PtrType * pointer )
	{
		TScopedPtr< PtrType, DeleterType >( pointer ).Swap( *this );
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE void TScopedPtr< PtrType, DeleterType >::Swap( TScopedPtr< PtrType, DeleterType > & object )
	{
		::Swap( m_pointee, object.m_pointee );
		::Swap( m_destroyer, object.m_destroyer );
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE  PtrType * TScopedPtr< PtrType, DeleterType >::operator->() const
	{
		RED_FATAL_ASSERT( m_pointee != nullptr, "null pointer access is illegal." );
		return m_pointee;
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE  PtrType & TScopedPtr< PtrType, DeleterType >::operator*() const
	{
		RED_FATAL_ASSERT( m_pointee != nullptr, "null pointer access is illegal." );
		return *m_pointee;
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE  TScopedPtr< PtrType, DeleterType >::operator typename TScopedPtr< PtrType, DeleterType >::bool_operator() const
	{
		return m_pointee != nullptr ? &BoolConversion::valid : 0;
	}

	template< typename PtrType, typename DeleterType >
	RED_INLINE  bool TScopedPtr< PtrType, DeleterType >::operator !() const
	{
		return m_pointee == nullptr;
	}
}

