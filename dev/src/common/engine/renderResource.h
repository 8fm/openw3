/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderObject.h"

class LatentDataBuffer;

/// Base render resource
class IRenderResource : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderResource )
protected:
	virtual ~IRenderResource();

public:
	IRenderResource();

	// Get resource category
	virtual CName GetCategory() const=0;

	virtual Bool IsRenderTexture() const {return false;} // we need this to optimize texture streaming

	// Is the resource fully loadeed
	virtual Bool IsFullyLoaded() const { return true; } // by default we assume they are fully initialized

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;
};

/// Smart pointer to rendering resource
class CRenderResourceSmartPtr
{
public:
	IRenderResource*	m_resource;

public:
	//! Default constructor
	RED_INLINE CRenderResourceSmartPtr()
		: m_resource( NULL )
	{};

	//! Copy constructor
	RED_INLINE CRenderResourceSmartPtr( const CRenderResourceSmartPtr& ptr )
		: m_resource( ptr.m_resource )
	{
		if ( m_resource )
		{
			m_resource->AddRef();
		}
	}

	//! Get from resource
	template< class T >
	RED_INLINE CRenderResourceSmartPtr( const T* resource )
		: m_resource( resource ? resource->GetRenderResource() : NULL )
	{
		if ( m_resource )
		{
			m_resource->AddRef();
		}
	}

	//! Get from resource handle
	template< class T >
	RED_INLINE CRenderResourceSmartPtr( const THandle<T>& resource )
		: m_resource( resource ? resource->GetRenderResource() : NULL )
	{
		if ( m_resource )
		{
			m_resource->AddRef();
		}
	}

	//! Initialize
	RED_INLINE CRenderResourceSmartPtr( IRenderResource* ptr )
		: m_resource( ptr )
	{
		if ( m_resource )
		{
			m_resource->AddRef();
		}
	}

	//! Destructor
	RED_INLINE ~CRenderResourceSmartPtr()
	{
		Reset();
	}

	//! Assignment
	RED_INLINE CRenderResourceSmartPtr& operator=( const CRenderResourceSmartPtr& ptr )
	{
		if ( m_resource != ptr.m_resource )
		{
			if ( m_resource )
			{
				m_resource->Release();
			}

			m_resource = ptr.m_resource;

			if ( m_resource )
			{
				m_resource->AddRef();
			}
		}

		return *this;
	}

public:
	//! Get as type
	template< class T > 
	RED_INLINE T* Get() const
	{
		return static_cast< T* >( m_resource );
	}

	//! Is NULL resource
	RED_INLINE bool IsNull() const
	{
		return NULL == m_resource;
	}

	//! Release reference
	RED_INLINE void Reset()
	{
		if ( m_resource )
		{
			m_resource->Release();
			m_resource = NULL;
		}
	}
};