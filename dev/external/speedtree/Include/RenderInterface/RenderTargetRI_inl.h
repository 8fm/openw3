///////////////////////////////////////////////////////////////////////  
//  RenderTargetRI.inl
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
//  CRenderTargetRI::CRenderTargetRI

CRenderTargetRI_TemplateList
inline CRenderTargetRI_t::CRenderTargetRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::~CRenderTargetRI

CRenderTargetRI_TemplateList
inline CRenderTargetRI_t::~CRenderTargetRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::InitGfx

CRenderTargetRI_TemplateList
inline st_bool CRenderTargetRI_t::InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples)
{
	return m_tRenderTargetPolicy.InitGfx(eType, nWidth, nHeight, nNumSamples);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::ReleaseGfxResources

CRenderTargetRI_TemplateList
inline void CRenderTargetRI_t::ReleaseGfxResources(void)
{
	m_tRenderTargetPolicy.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::Clear

CRenderTargetRI_TemplateList
inline void CRenderTargetRI_t::Clear(const Vec4& vColor)
{
	m_tRenderTargetPolicy.Clear(vColor);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::SetAsTarget

CRenderTargetRI_TemplateList
inline st_bool CRenderTargetRI_t::SetAsTarget(void)
{
	return m_tRenderTargetPolicy.SetAsTarget( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::ReleaseAsTarget

CRenderTargetRI_TemplateList
inline void CRenderTargetRI_t::ReleaseAsTarget(void)
{
	m_tRenderTargetPolicy.ReleaseAsTarget( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::BindAsTexture

CRenderTargetRI_TemplateList
inline st_bool CRenderTargetRI_t::BindAsTexture(st_int32 nRegisterIndex, st_bool bPointFilter) const
{
	return m_tRenderTargetPolicy.BindAsTexture(nRegisterIndex, bPointFilter);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::UnBindAsTexture

CRenderTargetRI_TemplateList
inline void CRenderTargetRI_t::UnBindAsTexture(st_int32 nRegisterIndex) const
{
	m_tRenderTargetPolicy.UnBindAsTexture(nRegisterIndex);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::OnResetDevice

CRenderTargetRI_TemplateList
inline void CRenderTargetRI_t::OnResetDevice(void)
{
	m_tRenderTargetPolicy.OnResetDevice( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::OnLostDevice

CRenderTargetRI_TemplateList
inline void CRenderTargetRI_t::OnLostDevice(void)
{
	m_tRenderTargetPolicy.OnLostDevice( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::SetGroupAsTarget

CRenderTargetRI_TemplateList
inline st_bool CRenderTargetRI_t::SetGroupAsTarget(const CRenderTargetRI<TRenderTargetPolicy>* pTargets, st_int32 nNumTargets, st_bool bClear)
{
	const TRenderTargetPolicy* apTargetPolicies[c_nMaxNumRenderTargets];
	for (st_int32 i = 0; i < nNumTargets; ++i)
		apTargetPolicies[i] = &(pTargets[i].m_tRenderTargetPolicy);

	return TRenderTargetPolicy::SetGroupAsTarget(apTargetPolicies, nNumTargets, bClear);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetRI::ReleaseGroupAsTarget

CRenderTargetRI_TemplateList
inline void CRenderTargetRI_t::ReleaseGroupAsTarget(const CRenderTargetRI<TRenderTargetPolicy>* pTargets, st_int32 nNumTargets)
{
	const TRenderTargetPolicy* apTargetPolicies[c_nMaxNumRenderTargets];
	for (st_int32 i = 0; i < nNumTargets; ++i)
		apTargetPolicies[i] = &(pTargets[i].m_tRenderTargetPolicy);

	TRenderTargetPolicy::ReleaseGroupAsTarget(apTargetPolicies, nNumTargets);
}
