/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Base D3D9 resource
class IDynamicRenderResource : public IRenderResource
{
	friend class CRenderResourceIterator;
	template< typename RES > friend class TRenderResourceList;

private:
	static Red::Threads::CLightMutex		s_resourceMutex;

private:
	static IDynamicRenderResource*		s_allResources;				//!< List of all rendering resources

private:
	IDynamicRenderResource**		m_prevResource;			//!< Previous resource in list
	IDynamicRenderResource*			m_nextResource;			//!< Next resource in resource list

protected:
	virtual void DestroySelfIfRefCountZero();

public:
	IDynamicRenderResource();
	virtual ~IDynamicRenderResource();

	// Device was lost
	virtual void OnDeviceLost()=0;

	// Device was reset
	virtual void OnDeviceReset()=0;

	// Get displayable name
	virtual String GetDisplayableName() const { return TXT("<unknown_name>"); }

public:
	// Get all resources
	static void GetAllResources( TDynArray< IDynamicRenderResource* >& allResources );
};

/// Resource iterator
class CRenderResourceIterator
{
	IDynamicRenderResource*	m_cur;

public:
	RED_INLINE CRenderResourceIterator()
	{
		IDynamicRenderResource::s_resourceMutex.Acquire();
		m_cur = IDynamicRenderResource::s_allResources;
	}

	RED_INLINE ~CRenderResourceIterator()
	{
		IDynamicRenderResource::s_resourceMutex.Release();
	}
	
	// This is needed to be able to prefetch the next element in a loop
	RED_INLINE IDynamicRenderResource* Next()
	{
		return m_cur ? m_cur->m_nextResource : NULL;
	}

	RED_INLINE void operator++()
	{
		m_cur = m_cur ? m_cur->m_nextResource : NULL;
	}

	RED_INLINE operator Bool() const
	{
		return m_cur != NULL;
	}

	RED_INLINE IDynamicRenderResource* operator->()
	{
		return m_cur;
	}

	RED_INLINE IDynamicRenderResource* operator*()
	{
		return m_cur;
	}
};
