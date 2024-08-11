///////////////////////////////////////////////////////////////////////  
//  ResourceCache.inl
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  CResourceCache::CResourceCache

CResourceCache_TemplateList
inline CResourceCache_t::CResourceCache(EGfxResourceType eResourceType, st_int32 nInitialReserve) :
    m_eResourceType(eResourceType),
	m_mCache(nInitialReserve)
{
}


///////////////////////////////////////////////////////////////////////  
//  CResourceCache::Retrieve

CResourceCache_TemplateList
inline TResource* CResourceCache_t::Retrieve(const TKey& tKey) const
{
	TResource* pResource = NULL;

	// search the cache/map for key
	typename CCacheMap::iterator iFind = m_mCache.find(tKey);
	if (iFind != m_mCache.end( ))
	{
		++iFind->second.m_nRefCount;
		pResource = &(iFind->second.m_tResource);
	}

	return pResource;
}


///////////////////////////////////////////////////////////////////////  
//  CResourceCache::Add
//
//  Returns true is resource added to cache.

CResourceCache_TemplateList
inline st_bool CResourceCache_t::Add(const TKey& tKey, const TResource& tResource, size_t siSize)
{
    st_bool bAdded = Add_NoCoreTracking(tKey, tResource);
    if (bAdded)
    {
        // update Core's resource tracking system
        CCore::ResourceAllocated(m_eResourceType, tKey, siSize);
    }

    return bAdded;
}


///////////////////////////////////////////////////////////////////////  
//  CResourceCache::Add_NoCoreTracking
//
//  Returns true is resource added to cache.

CResourceCache_TemplateList
inline st_bool CResourceCache_t::Add_NoCoreTracking(const TKey& tKey, const TResource& tResource)
{
    st_bool bAdded = false;

    // verify that entry isn't already in map
    typename CCacheMap::iterator iFind = m_mCache.find(tKey);
    if (iFind == m_mCache.end( ))
    {
        // create new cache entry
        SCacheEntry sNewEntry;
        sNewEntry.m_tResource = tResource;
        sNewEntry.m_nRefCount = 1;

        // add entry to cache/map
        m_mCache[tKey] = sNewEntry;

        bAdded = true;
    }

    return bAdded;
}


///////////////////////////////////////////////////////////////////////  
//  CResourceCache::Release
//
//	Returns reference count of cached item after release:
//		>0 : still active
//		 0 : ready to free app-side
//		-1 : key not found in cache

CResourceCache_TemplateList
inline st_int32 CResourceCache_t::Release(const TKey& tKey)
{
	st_int32 nRefCount = Release_NoCoreTracking(tKey);
    if (nRefCount == 0)
        CCore::ResourceReleased(tKey);

	return nRefCount;
}


///////////////////////////////////////////////////////////////////////  
//  CResourceCache::Release_NoCoreTracking
//
//	Returns reference count of cached item after release:
//		>0 : still active
//		 0 : ready to free app-side
//		-1 : key not found in cache

CResourceCache_TemplateList
inline st_int32 CResourceCache_t::Release_NoCoreTracking(const TKey& tKey)
{
    st_int32 nRefCount = -1;

    // search for entry 
    typename CCacheMap::iterator iFind = m_mCache.find(tKey);
    if (iFind != m_mCache.end( ))
    {
        // reduce reference count
        nRefCount = --iFind->second.m_nRefCount;
        assert(nRefCount >= 0);

        // last reference, so delete from cache
        if (nRefCount == 0)
            m_mCache.erase(iFind);
    }

    return nRefCount;
}


///////////////////////////////////////////////////////////////////////  
//  CResourceCache::Size

CResourceCache_TemplateList
inline st_int32 CResourceCache_t::Size(void) const
{
	return st_int32(m_mCache.size( ));
}
