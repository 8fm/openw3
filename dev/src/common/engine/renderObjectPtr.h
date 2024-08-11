/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_RENDER_OBJECT_PTR_H_
#define _ENGINE_RENDER_OBJECT_PTR_H_

#include "../core/sharedPtr.h"

class IRenderObject;

template< typename T >
class RenderObjectOwnership;

template< typename T >
class TRenderObjectPtr : public Red::TSharedPtr< T, RenderObjectOwnership >
{
public:

	typedef Red::TSharedPtr< T, ::RenderObjectOwnership > ParentType;
	typedef typename ParentType::PtrType PtrType;

	TRenderObjectPtr();
	TRenderObjectPtr( const TRenderObjectPtr & copyFrom );
	TRenderObjectPtr( TRenderObjectPtr && rvalue );

	template< typename U >
	explicit TRenderObjectPtr( U * pointer );	

	template< typename U >
	TRenderObjectPtr( const TRenderObjectPtr< U > & copyFrom );

	template< typename U >
	TRenderObjectPtr( TRenderObjectPtr< U > && rvalue );

	TRenderObjectPtr & operator=( const TRenderObjectPtr & copyFrom );
	TRenderObjectPtr & operator=( TRenderObjectPtr && rvalue );

	template< typename U >
	TRenderObjectPtr & operator=( const TRenderObjectPtr< U >  & copyFrom );

	template< typename U >
	TRenderObjectPtr & operator=( TRenderObjectPtr< U > && rvalue );

	void ResetFromExternal( PtrType pointer );
};

template< typename T >
class RenderObjectOwnership
{
protected:

	typedef T * PtrType;

	RenderObjectOwnership();
	RenderObjectOwnership( PtrType pointer );

	RenderObjectOwnership( const RenderObjectOwnership & copyFrom );
	RenderObjectOwnership( RenderObjectOwnership && rvalue );

	template< typename U >
	RenderObjectOwnership( const RenderObjectOwnership< U > & copyFrom );

	template< typename U >
	RenderObjectOwnership( RenderObjectOwnership< U > && rvalue );

	PtrType Get() const;

	void Release();
	void AddRef();

	void Swap( RenderObjectOwnership & swapWith );

private:

	void AssignRValue( RenderObjectOwnership && rvalue );

	PtrType m_pointee;
};

#include "renderObjectPtr.inl"

#endif
