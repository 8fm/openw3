///////////////////////////////////////////////////////////////////////  
//  TextureRI.inl
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
//  STextureInfo::STextureInfo

inline STextureInfo::STextureInfo( ) :
	m_nWidth(-1),
	m_nHeight(-1)
{
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::CTextureRI

CTextureRI_TemplateList
inline CTextureRI_t::CTextureRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::~CTextureRI

CTextureRI_TemplateList
inline CTextureRI_t::~CTextureRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::Load

CTextureRI_TemplateList
inline st_bool CTextureRI_t::Load(const char* pFilename, st_int32 nMaxAnisotropy)
{
	// create the cache if first use
	if (!m_pCache)
		m_pCache = st_new(CTextureCache, "CTextureCache")(GFX_RESOURCE_TEXTURE, c_nTextureCacheInitialReserve);
	assert(m_pCache);

	// is this texture already cached?
	TTexturePolicy* pTexture = m_pCache->Retrieve(pFilename);
	if (!pTexture)
	{
		// wasn't in cache, so create it
		if (m_tTexturePolicy.Load(pFilename, nMaxAnisotropy))
		{
            // get file system pointer from Core lib
            CFileSystem* pFileSystem = CFileSystemInterface::Get( );
            assert(pFileSystem);

			// and add to cache
			m_pCache->Add(pFilename, m_tTexturePolicy, pFileSystem->FileSize(pFilename));
		}
	}
	else
		m_tTexturePolicy = *pTexture;

	m_strFilename = pFilename;

	return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::LoadColor

CTextureRI_TemplateList
inline st_bool CTextureRI_t::LoadColor(st_uint32 uiColor)
{
	// need a string descriptor
	m_strFilename = CFixedString::Format("Color_%0x", uiColor).c_str( );

	// create the cache if first use
	if (!m_pCache)
		m_pCache = st_new(CTextureCache, "CTextureCache")(GFX_RESOURCE_TEXTURE, c_nTextureCacheInitialReserve);
	assert(m_pCache);

	// is this texture already cached?
	TTexturePolicy* pTexture = m_pCache->Retrieve(m_strFilename);
	if (!pTexture)
	{
		// wasn't in cache, so create it
		if (m_tTexturePolicy.LoadColor(uiColor))
		{
			// and add to cache
            const size_t c_siColorTextureSize = 4 * 4 * 4; // may not be the same for all platforms
			m_pCache->Add(m_strFilename, m_tTexturePolicy, c_siColorTextureSize);
		}
	}
	else
		m_tTexturePolicy = *pTexture;

	return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::LoadNoise

CTextureRI_TemplateList
inline st_bool CTextureRI_t::LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise)
{
	// need a string descriptor
	m_strFilename = CFixedString::Format("Noise_%dx%d", nWidth, nHeight).c_str( );

	// create the cache if first use
	if (!m_pCache)
		m_pCache = st_new(CTextureCache, "CTextureCache")(GFX_RESOURCE_TEXTURE, c_nTextureCacheInitialReserve);
	assert(m_pCache);

	// is this texture already cached?
	TTexturePolicy* pTexture = m_pCache->Retrieve(m_strFilename);
	if (!pTexture)
	{
		// wasn't in cache, so create it
		if (m_tTexturePolicy.LoadNoise(nWidth, nHeight, fLowNoise, fHighNoise))
		{
			// and add to cache
            const size_t c_siNoiseTextureSize = c_nNoiseTexWidth * c_nNoiseTexWidth; // may not be the same for all platforms
			m_pCache->Add(m_strFilename, m_tTexturePolicy, c_siNoiseTextureSize);
		}
	}
	else
		m_tTexturePolicy = *pTexture;

	return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::LoadPerlinNoiseKernel

CTextureRI_TemplateList
inline st_bool CTextureRI_t::LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth)
{
	// need a string descriptor
	m_strFilename = CFixedString::Format("PerlinNoiseKernel_%dx%dx%d", nWidth, nHeight, nDepth).c_str( );

	// create the cache if first use
	if (!m_pCache)
		m_pCache = st_new(CTextureCache, "CTextureCache")(GFX_RESOURCE_TEXTURE, c_nTextureCacheInitialReserve);
	assert(m_pCache);

	// is this texture already cached?
	TTexturePolicy* pTexture = m_pCache->Retrieve(m_strFilename);
	if (!pTexture)
	{
		// wasn't in cache, so create it
		if (m_tTexturePolicy.LoadPerlinNoiseKernel(nWidth, nHeight, nDepth))
		{
			// and add to cache
			const size_t c_siNoiseTextureSize = nWidth * nHeight * nDepth;
			m_pCache->Add(m_strFilename, m_tTexturePolicy, c_siNoiseTextureSize);
		}
	}
	else
		m_tTexturePolicy = *pTexture;

	return IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::ReleaseGfxResources

CTextureRI_TemplateList
inline st_bool CTextureRI_t::ReleaseGfxResources(void)
{
	if (m_pCache)
	{
		st_int32 nRefCount = m_pCache->Release(m_strFilename);
		if (nRefCount == 0)
			m_tTexturePolicy.ReleaseGfxResources( );
		// todo: a bit of a hack -- https://trello.com/c/Zu12xIBK/232-probably-want-to-get-a-handle-on-graphics-resource-management
		else
			m_tTexturePolicy = TTexturePolicy( );

		// delete cache if empty
		if (m_pCache->Size( ) == 0)
			st_delete<CTextureCache>(m_pCache);
	}

	m_strFilename.clear( );

	return !IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::SetSamplerStates

CTextureRI_TemplateList
inline void CTextureRI_t::SetSamplerStates(void)
{
	TTexturePolicy::SetSamplerStates( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::GetFilename

CTextureRI_TemplateList
inline const char* CTextureRI_t::GetFilename(void) const
{
	return m_strFilename.c_str( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::GetInfo

CTextureRI_TemplateList
inline const STextureInfo& CTextureRI_t::GetInfo(void) const
{
	return m_tTexturePolicy.GetInfo( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::IsValid

CTextureRI_TemplateList
inline st_bool CTextureRI_t::IsValid(void) const
{
	return m_tTexturePolicy.IsValid( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::operator=

CTextureRI_TemplateList
inline CTextureRI_t& CTextureRI_t::operator=(const CTextureRI& cRight)
{
	m_tTexturePolicy = cRight.m_tTexturePolicy;
	m_strFilename = cRight.m_strFilename;

	return *this;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureRI::operator!=

CTextureRI_TemplateList
inline st_bool CTextureRI_t::operator!=(const CTextureRI& cRight) const
{
	return m_tTexturePolicy.operator!=(cRight.m_tTexturePolicy);
}
