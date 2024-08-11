///////////////////////////////////////////////////////////////////////
//  MyRenderTargets.cpp
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
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
//  Preprocessor

#include "MyRenderTargets.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::CMyDeferredRenderTargets

CMyDeferredRenderTargets::CMyDeferredRenderTargets( ) :
	m_nWidth(0),
	m_nHeight(0),
	m_bA2C(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::~CMyDeferredRenderTargets

CMyDeferredRenderTargets::~CMyDeferredRenderTargets( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::InitGfx

st_bool CMyDeferredRenderTargets::InitGfx(st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples, st_bool bA2C)
{
	st_bool bSuccess = true;

	#ifdef _XBOX
		// make sure all the targets fit into EDRAM on the XBox 360
		nWidth = st_min(nWidth, 1200);
		nHeight = st_min(nHeight, 720);
	#endif

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_bA2C = bA2C;

	st_int32 nTarget = 0;
	if (bA2C)
		bSuccess &= m_atTargets[nTarget++].InitGfx(RENDER_TARGET_TYPE_NULL, nWidth, nHeight, nNumSamples);

	bSuccess &= m_atTargets[nTarget++].InitGfx(RENDER_TARGET_TYPE_COLOR, nWidth, nHeight, nNumSamples);
	bSuccess &= m_atTargets[nTarget++].InitGfx(RENDER_TARGET_TYPE_COLOR, nWidth, nHeight, nNumSamples);
	bSuccess &= m_atTargets[nTarget++].InitGfx(RENDER_TARGET_TYPE_DEPTH, nWidth, nHeight, nNumSamples);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::ReleaseGfxResources

void CMyDeferredRenderTargets::ReleaseGfxResources(void)
{
	for (st_int32 i = 0; i < NumRenderTargets( ); ++i)
		m_atTargets[i].ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::Clear

void CMyDeferredRenderTargets::Clear(const Vec4& vColor)
{
	for (st_int32 i = 0; i < NumRenderTargets( ); ++i)
		m_atTargets[i].Clear(vColor);
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::SetAsTarget

st_bool CMyDeferredRenderTargets::SetAsTarget(void)
{
	return CRenderTarget::SetGroupAsTarget(m_atTargets, NumRenderTargets( ));
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::ReleaseAsTarget

void CMyDeferredRenderTargets::ReleaseAsTarget(void)
{
	CRenderTarget::ReleaseGroupAsTarget(m_atTargets, NumRenderTargets( ));
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::BindAsTextures

st_bool CMyDeferredRenderTargets::BindAsTextures(void)
{
	st_bool bSuccess = true;

	for (st_int32 i = 0; i < NumRenderTargets( ); ++i)
		bSuccess &= m_atTargets[i].BindAsTexture(i);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::UnBindAsTextures

void CMyDeferredRenderTargets::UnBindAsTextures(void)
{
	for (st_int32 i = 0; i < NumRenderTargets( ); ++i)
		m_atTargets[i].UnBindAsTexture(i);
}


///////////////////////////////////////////////////////////////////////  
//  CMyDeferredRenderTargets::NumRenderTargets

st_int32 CMyDeferredRenderTargets::NumRenderTargets(void) const
{
	return m_bA2C ? MY_DEFERRED_RENDER_TARGET_COUNT_WITH_A2C : MY_DEFERRED_RENDER_TARGET_COUNT_NO_A2C;
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::CMyForwardTargets

CMyForwardTargets::CMyForwardTargets( ) :
	m_nWidth(0),
	m_nHeight(0),
	m_nNumSamples(1)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::~CMyForwardTargets

CMyForwardTargets::~CMyForwardTargets( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::InitGfx

st_bool CMyForwardTargets::InitGfx(st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples)
{
	st_bool bSuccess = true;

	#ifdef _XBOX
		// make sure all the targets fit into EDRAM on the XBox 360
		nWidth = st_min(nWidth, 1200);
		nHeight = st_min(nHeight, 720);
	#endif

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nNumSamples = nNumSamples;

	bSuccess &= m_atMsaaTargets[MY_FORWARD_TARGET_0].InitGfx(RENDER_TARGET_TYPE_COLOR, nWidth, nHeight, nNumSamples);
	bSuccess &= m_atMsaaTargets[MY_FORWARD_TARGET_DEPTH].InitGfx(RENDER_TARGET_TYPE_DEPTH, nWidth, nHeight, nNumSamples);

	if (nNumSamples > 1)
		bSuccess &= m_tNonMsaaResolveTarget.InitGfx(RENDER_TARGET_TYPE_COLOR, nWidth, nHeight, 1);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::ReleaseGfxResources

void CMyForwardTargets::ReleaseGfxResources(void)
{
	for (st_int32 i = 0; i < MY_FORWARD_TARGET_COUNT; ++i)
		m_atMsaaTargets[i].ReleaseGfxResources( );

	if (m_nNumSamples > 1)
		m_tNonMsaaResolveTarget.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::Clear

void CMyForwardTargets::Clear(const Vec4& vColor)
{
	for (st_int32 i = 0; i < MY_FORWARD_TARGET_COUNT; ++i)
		m_atMsaaTargets[i].Clear(vColor);
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::SetAsTarget

st_bool CMyForwardTargets::SetAsTarget(st_bool bClear)
{
	return CRenderTarget::SetGroupAsTarget(m_atMsaaTargets, MY_FORWARD_TARGET_COUNT, bClear);
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::ReleaseAsTarget

void CMyForwardTargets::ReleaseAsTarget(void)
{
	CRenderTarget::ReleaseGroupAsTarget(m_atMsaaTargets, MY_FORWARD_TARGET_COUNT);
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::Resolve

st_bool CMyForwardTargets::ResolveToTexture(void)
{
	st_bool bSuccess = false;

	if (m_nNumSamples > 1)
	{
		#if defined(_DURANGO) || defined(SPEEDTREE_DIRECTX11)
			bSuccess = CRenderTargetDirectX11::ResolveSubresource(&m_tNonMsaaResolveTarget, m_atMsaaTargets + MY_FORWARD_TARGET_0);
		#elif SPEEDTREE_OPENGL
			// make sure each has a framebuffer associated with it
			SetAsTarget(false);
			ReleaseAsTarget( );

			CRenderTarget::SetGroupAsTarget(&m_tNonMsaaResolveTarget, 1);
			CRenderTarget::ReleaseGroupAsTarget(&m_tNonMsaaResolveTarget, 1);

			// set source framebuffer
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_atMsaaTargets[MY_FORWARD_TARGET_0].m_tRenderTargetPolicy.GetFbo( ));

			// set destination framebuffer
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_tNonMsaaResolveTarget.m_tRenderTargetPolicy.GetFbo( ));

			// copy/resolve from MSAA target to regular to be sampled in pixel shader
			glBlitFramebuffer(0, 0, m_nWidth, m_nHeight, 0, 0, m_nWidth, m_nHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			
			// clear binding
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		#endif
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::BindAsTextures

st_bool CMyForwardTargets::BindAsTextures(void)
{
	st_bool bSuccess = true;

	// no need to bind the depth buffer
	bSuccess &= m_atMsaaTargets[MY_FORWARD_TARGET_0].BindAsTexture(0);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::UnBindAsTextures

void CMyForwardTargets::UnBindAsTextures(void)
{
	for (st_int32 i = 0; i < MY_FORWARD_TARGET_COUNT; ++i)
		m_atMsaaTargets[i].UnBindAsTexture(i);
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::GetNonMsaaTarget

const CRenderTarget& CMyForwardTargets::GetNonMsaaTarget(void) const
{
	return (m_nNumSamples > 1 ? m_tNonMsaaResolveTarget : m_atMsaaTargets[MY_FORWARD_TARGET_0]);
}


///////////////////////////////////////////////////////////////////////  
//  CMyForwardTargets::GetTarget

const CRenderTarget& CMyForwardTargets::GetTarget(EMyForwardTargetType eTarget) const
{
	return m_atMsaaTargets[eTarget];
}

