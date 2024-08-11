/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

template< typename T >
RED_INLINE TRenderObjectPtr< T >::TRenderObjectPtr()
{
	static_assert( std::is_base_of< IRenderObject, T >::value, "TRenderObjectPtr can only be use with IRenderObject" );
}

template< typename T >
RED_INLINE TRenderObjectPtr< T >::TRenderObjectPtr( const TRenderObjectPtr & copyFrom )
	: ParentType( copyFrom )
{
	static_assert( std::is_base_of< IRenderObject, T >::value, "TRenderObjectPtr can only be use with IRenderObject" );
}

template< typename T >
RED_INLINE TRenderObjectPtr< T >::TRenderObjectPtr( TRenderObjectPtr && rvalue )
	: ParentType( std::forward< TRenderObjectPtr >( rvalue ) )
{
	static_assert( std::is_base_of< IRenderObject, T >::value, "TRenderObjectPtr can only be use with IRenderObject" );
}

template< typename T >
template< typename U >
RED_INLINE TRenderObjectPtr< T >::TRenderObjectPtr( U * pointer )
	: ParentType( pointer )
{
	static_assert( std::is_base_of< IRenderObject, U >::value, "TRenderObjectPtr can only be use with IRenderObject" );
}

template< typename T >
template< typename U >
RED_INLINE TRenderObjectPtr< T >::TRenderObjectPtr( const TRenderObjectPtr< U > & copyFrom )
	: ParentType( copyFrom )
{
	static_assert( std::is_base_of< IRenderObject, U >::value, "TRenderObjectPtr can only be use with IRenderObject" );
}

template< typename T >
template< typename U >
RED_INLINE TRenderObjectPtr< T >::TRenderObjectPtr( TRenderObjectPtr< U > && rvalue )
	: ParentType( std::forward< TRenderObjectPtr< U > >( rvalue ) )
{
	static_assert( std::is_base_of< IRenderObject, U >::value, "TRenderObjectPtr can only be use with IRenderObject" );
}

template< typename T >
RED_INLINE TRenderObjectPtr< T > & TRenderObjectPtr< T >::operator=( const TRenderObjectPtr & copyFrom )
{
	ParentType::operator =( copyFrom );
	return *this;
}

template< typename T >
RED_INLINE TRenderObjectPtr< T > & TRenderObjectPtr< T >::operator=( TRenderObjectPtr && rvalue )
{
	ParentType::operator =( std::forward< TRenderObjectPtr >( rvalue ) );
	return *this;
}

template< typename T >
template< typename U >
RED_INLINE TRenderObjectPtr< T > & TRenderObjectPtr< T >::operator=( const TRenderObjectPtr< U > & copyFrom )
{
	ParentType::operator =( copyFrom );
	return *this;
}

template< typename T >
template< typename U >
RED_INLINE TRenderObjectPtr< T > & TRenderObjectPtr< T >::operator=( TRenderObjectPtr< U > && rvalue )
{
	ParentType::operator =(  std::forward< TRenderObjectPtr< U > >( rvalue ) );
	return *this;
}

template< typename T >
RED_INLINE void TRenderObjectPtr< T >::ResetFromExternal( PtrType pointer )
{
	ParentType::Reset( pointer );
	ParentType::AddRef();
}

template< typename T >
RED_INLINE RenderObjectOwnership< T >::RenderObjectOwnership()
	: m_pointee( nullptr )
{}

template< typename T >
RED_INLINE RenderObjectOwnership< T >::RenderObjectOwnership( PtrType pointer )
	: m_pointee( pointer )
{}

template< typename T >
RED_INLINE RenderObjectOwnership< T >::RenderObjectOwnership( const RenderObjectOwnership & copyFrom )
	: m_pointee( copyFrom.m_pointee )
{}

template< typename T >
RED_INLINE RenderObjectOwnership< T >::RenderObjectOwnership( RenderObjectOwnership && rvalue )
	: m_pointee( nullptr )
{
	AssignRValue( std::forward< RenderObjectOwnership >( rvalue ) );
}

template< typename T >
template< typename U >
RED_INLINE RenderObjectOwnership< T >::RenderObjectOwnership( const RenderObjectOwnership< U > & copyFrom )
	: m_pointee( reinterpret_cast< const RenderObjectOwnership & >(copyFrom).m_pointee  )
{}

template< typename T >
template< typename U >
RED_INLINE RenderObjectOwnership< T >::RenderObjectOwnership( RenderObjectOwnership< U > && rvalue )
	: m_pointee( reinterpret_cast< RenderObjectOwnership & >(rvalue).m_pointee  )
{
	 reinterpret_cast< RenderObjectOwnership & >(rvalue).m_pointee = nullptr;
}

template< typename T >
RED_INLINE void RenderObjectOwnership< T >::Release()
{
	if( m_pointee )
	{
		m_pointee->Release();
	}
}

template< typename T >
RED_INLINE void RenderObjectOwnership< T >::AddRef()
{
	if( m_pointee )
	{
		m_pointee->AddRef();
	}
}

template< typename T >
RED_INLINE void RenderObjectOwnership< T >::Swap( RenderObjectOwnership & swapWith )
{
	::Swap( m_pointee, swapWith.m_pointee );
}

template< typename T >
RED_INLINE T* RenderObjectOwnership< T >::Get() const
{
	return m_pointee;
}

template< typename T >
RED_INLINE void RenderObjectOwnership< T >::AssignRValue( RenderObjectOwnership && rvalue )
{
	if( this != &rvalue )
	{
		Swap( rvalue );
	}
}
