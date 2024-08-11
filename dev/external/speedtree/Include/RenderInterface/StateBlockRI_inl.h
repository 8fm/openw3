///////////////////////////////////////////////////////////////////////  
//  StateBlockRI.inl
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
//  CStateBlockRI::Init

CStateBlockRI_TemplateList
inline CStateBlockRI_t::CStateBlockRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockRI::Init

CStateBlockRI_TemplateList
inline st_bool CStateBlockRI_t::Init(const SAppState& sAppState, const SRenderState& sRenderState, st_bool isInteractive)	// LAVA edit: add st_bool isInteractive
{
	st_bool bSuccess = true;

	// copy parameters as needed
	m_sAppState = sAppState;

	// create the cache if first use
	if (!m_pCache)
		m_pCache = st_new(CStateBlockCache, "CStateBlockCache")(GFX_RESOURCE_OTHER, c_nStateBlockCacheInitialReserve);
	assert(m_pCache);

	// is this state block already cached?
	GetStateBlockKey(m_strHashKey, sAppState, sRenderState);
	// LAVA++ interactive foliage has different state block, so we need to check also isInteractive flag
	m_strHashKey = CFixedString::Format("%s%i", m_strHashKey.c_str(), isInteractive);
	// LAVA--
	TStateBlockPolicy* pStateBlock = m_pCache->Retrieve(m_strHashKey);
	if (!pStateBlock)
	{
		// wasn't in cache, so create it
		if (m_tStateBlockPolicy.Init(sAppState, sRenderState, isInteractive))		// LAVA edit: add isInteractive
		{
			// and add to cache
            const size_t c_siStateBlockSize = sizeof(m_tStateBlockPolicy);
			m_pCache->Add(m_strHashKey, m_tStateBlockPolicy, c_siStateBlockSize);
		}
		else
			bSuccess = false;
	}
	else
		m_tStateBlockPolicy = *pStateBlock;

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockRI::ReleaseGfxResources

CStateBlockRI_TemplateList
inline void CStateBlockRI_t::ReleaseGfxResources(void)
{
	if (m_pCache)
	{
		st_int32 nRefCount = m_pCache->Release(m_strHashKey);
		if (nRefCount == 0)
			m_tStateBlockPolicy.ReleaseGfxResources( );

		// delete cache if empty
		if (m_pCache->Size( ) == 0)
			st_delete<CStateBlockCache>(m_pCache);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockRI::Bind

CStateBlockRI_TemplateList
inline st_bool CStateBlockRI_t::Bind(void) const
{
	return m_tStateBlockPolicy.Bind( );
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockRI::GetAppState

CStateBlockRI_TemplateList
inline const SAppState& CStateBlockRI_t::GetAppState(void) const
{
	return m_sAppState;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockRI::GetStateBlockKey

CStateBlockRI_TemplateList
inline void CStateBlockRI_t::GetStateBlockKey(CFixedString& strKey, const SAppState& sAppState, const SRenderState& sRenderState) const
{
	strKey = CFixedString::Format("%d%d%d%d%d%d%d%d",
			    sAppState.m_eOverrideDepthTest,
				sAppState.m_bMultisampling,
				sAppState.m_bAlphaToCoverage,
				sAppState.m_bDepthPrepass,
				st_uint32(sRenderState.m_eFaceCulling),
				sRenderState.m_eRenderPass,
				sRenderState.m_bUsedAsGrass,
				sRenderState.m_bBlending);
}
