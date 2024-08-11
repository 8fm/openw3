///////////////////////////////////////////////////////////////////////
//  RenderTarget.cpp
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

#include "Renderers/Durango/DurangoRenderer.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::CRenderTargetDirectX11

CRenderTargetDirectX11::CRenderTargetDirectX11( ) :
	m_nWidth(-1),
	m_nHeight(-1),
	m_nNumSamples(1),
	m_eType(RENDER_TARGET_TYPE_COLOR),
	m_pTexture(NULL),
	m_pShaderResourceView(NULL),
	m_pRenderTargetView(NULL),
	m_pDepthStencilView(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::~CRenderTargetDirectX11

CRenderTargetDirectX11::~CRenderTargetDirectX11( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::InitGfx

st_bool CRenderTargetDirectX11::InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples)
{
	st_bool bSuccess = false;

	if (nWidth > 0 && nHeight > 0)
	{
		m_eType = eType;
		m_nWidth = nWidth;
		m_nHeight = nHeight;
		m_nNumSamples = nNumSamples;

		if (eType == RENDER_TARGET_TYPE_COLOR || eType == RENDER_TARGET_TYPE_NULL)
			bSuccess = CreateColorTarget( );
		else
			bSuccess = CreateDepthTarget( );
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::ReleaseGfxResources

void CRenderTargetDirectX11::ReleaseGfxResources(void)
{
	ST_SAFE_RELEASE(m_pTexture);
	ST_SAFE_RELEASE(m_pShaderResourceView);
	ST_SAFE_RELEASE(m_pRenderTargetView);
	ST_SAFE_RELEASE(m_pDepthStencilView);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::Clear

void CRenderTargetDirectX11::Clear(const Vec4& vColor)
{
	if (m_eType == RENDER_TARGET_TYPE_COLOR || m_eType == RENDER_TARGET_TYPE_NULL)
	{
		assert(m_pRenderTargetView);
        DX11::DeviceContext( )->ClearRenderTargetView(m_pRenderTargetView, &vColor.x);
	}
	else
	{
		assert(m_pDepthStencilView);
		DX11::DeviceContext( )->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::SetAsTarget

st_bool CRenderTargetDirectX11::SetAsTarget(void)
{
	const CRenderTargetDirectX11* pTarget = this;

	return SetGroupAsTarget(&pTarget, 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::ReleaseAsTarget

void CRenderTargetDirectX11::ReleaseAsTarget(void)
{
	const CRenderTargetDirectX11* pTarget = this;

	ReleaseGroupAsTarget(&pTarget, 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::BindAsTexture

st_bool CRenderTargetDirectX11::BindAsTexture(st_int32 nRegisterIndex, st_bool bPointFilter) const
{
	ST_UNREF_PARAM(bPointFilter);

	// shader resource
	DX11::DeviceContext( )->PSSetShaderResources(nRegisterIndex, 1, &m_pShaderResourceView);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::UnBindAsTexture

void CRenderTargetDirectX11::UnBindAsTexture(st_int32 nRegisterIndex) const
{
	ID3D11ShaderResourceView* pNullView = NULL;
	DX11::DeviceContext( )->PSSetShaderResources(nRegisterIndex, 1, &pNullView);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::GetDx11Texture

ID3D11Texture2D* CRenderTargetDirectX11::GetDx11Texture(void)
{
	return m_pTexture;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::OnResetDevice

void CRenderTargetDirectX11::OnResetDevice(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::OnLostDevice

void CRenderTargetDirectX11::OnLostDevice(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::SetGroupAsTarget

st_bool CRenderTargetDirectX11::SetGroupAsTarget(const CRenderTargetDirectX11** pTargets, st_int32 nNumTargets, st_bool bClear)
{
	st_bool bSuccess = false;

	// todo: remove bClear
	ST_UNREF_PARAM(bClear);

	assert(pTargets);
	assert(nNumTargets <= c_nMaxNumRenderTargets);
	assert(DX11::DeviceContext( ));

	if (nNumTargets > 0)
	{
		// avoid DX10 DEVICE_PSSETSHADERRESOURCES_HAZARD & DEVICE_OMSETRENDERTARGETS_HAZARD warnings by making
		// sure these targets aren't bound as a read resource
		// todo: restore? (may have been causing the ambient image to bind as black)
		//ID3D11ShaderResourceView* pNullView = NULL;
		//for (st_int32 i = 0; i < TEXTURE_REGISTER_COUNT; ++i)
		//	DX11::DeviceContext( )->PSSetShaderResources(i, 1, &pNullView);
		ID3D11ShaderResourceView* pNullView = NULL;
		for (st_int32 i = TEXTURE_REGISTER_SHADOW_MAP_0; i <= TEXTURE_REGISTER_SHADOW_MAP_3; ++i)
			DX11::DeviceContext( )->PSSetShaderResources(i, 1, &pNullView);

		// extract current color & and depth-stencil views
		ID3D11DepthStencilView* pDepthStencilView = NULL;
		ID3D11RenderTargetView* apColorViews[c_nMaxNumRenderTargets] = { NULL };
		ID3D11RenderTargetView** pCurrentColorView = apColorViews;
		for (st_int32 i = 0; i < nNumTargets; ++i)
		{
			assert(pTargets[i]);
			if (pTargets[i]->m_eType == RENDER_TARGET_TYPE_COLOR)
			{
				assert(pTargets[i]->m_pRenderTargetView);
				*pCurrentColorView++ = pTargets[i]->m_pRenderTargetView;
			}
			else if (pTargets[i]->m_eType == RENDER_TARGET_TYPE_NULL)
				*pCurrentColorView++ = NULL;
			else
				pDepthStencilView = pTargets[i]->m_pDepthStencilView;
		}

		// save the current view state
		{
			// get the currently active render targets
			DX11::DeviceContext( )->OMGetRenderTargets(c_nMaxNumRenderTargets, m_apPreviouslyActiveRenderTargetViews, &m_pPreviouslyActiveDepthStencilView);

			// get the currently active viewport
			UINT uiViewports = 1;
			DX11::DeviceContext( )->RSGetViewports(&uiViewports, &m_sPreviouslyActiveViewport);
		}

		// set render targets
		st_int32 nNumColorTargets = st_int32(pCurrentColorView - apColorViews);
		DX11::DeviceContext( )->OMSetRenderTargets(nNumColorTargets, apColorViews, pDepthStencilView);

		// set viewport
		const D3D11_VIEWPORT c_sViewData = { 0.0f, 0.0f, st_float32(pTargets[0]->m_nWidth), st_float32(pTargets[0]->m_nHeight), 0.0f, 1.0f };
		DX11::DeviceContext( )->RSSetViewports(1, &c_sViewData);

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::ReleaseGroupAsTarget

void CRenderTargetDirectX11::ReleaseGroupAsTarget(const CRenderTargetDirectX11** pTargets, st_int32 nNumTargets)
{
	ST_UNREF_PARAM(pTargets);
	ST_UNREF_PARAM(nNumTargets);

	DX11::DeviceContext( )->OMSetRenderTargets(c_nMaxNumRenderTargets, m_apPreviouslyActiveRenderTargetViews, m_pPreviouslyActiveDepthStencilView);

	for (st_int32 i = 0; i < c_nMaxNumRenderTargets; ++i)
		ST_SAFE_RELEASE(m_apPreviouslyActiveRenderTargetViews[i]);
	ST_SAFE_RELEASE(m_pPreviouslyActiveDepthStencilView);

	DX11::DeviceContext( )->RSSetViewports(1, &m_sPreviouslyActiveViewport);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::ResolveSubresource

st_bool CRenderTargetDirectX11::ResolveSubresource(CRenderTarget* pNonMsaaDest, CRenderTarget* pMsaaSrc)
{
	st_bool bSuccess = false;

	if (pNonMsaaDest && pMsaaSrc)
	{
		ID3D11Texture2D* pDestTexture = pNonMsaaDest->m_tRenderTargetPolicy.GetDx11Texture( );
		ID3D11Texture2D* pSrcTexture = pMsaaSrc->m_tRenderTargetPolicy.GetDx11Texture( );

		if (pDestTexture && pSrcTexture)
		{
			assert(DX11::DeviceContext( ));
			DX11::DeviceContext( )->ResolveSubresource(pDestTexture, 0, pSrcTexture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

			bSuccess = true;
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  GetMSAAQuality

static UINT GetMSAAQuality(UINT uiNumSamples, DXGI_FORMAT eFormat)
{
    UINT uiQuality = static_cast<UINT>(-1);
    DX11::Device( )->CheckMultisampleQualityLevels(eFormat, uiNumSamples, &uiQuality);

    return (uiQuality - 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::CreateColorTarget

st_bool CRenderTargetDirectX11::CreateColorTarget(void)
{
	st_bool bSuccess = false;

	// initialize the render target texture description
	D3D11_TEXTURE2D_DESC sRenderTargetDesc;
	ZeroMemory(&sRenderTargetDesc, sizeof(sRenderTargetDesc));

	// set up the render target texture description
	sRenderTargetDesc.Width = m_nWidth;
	sRenderTargetDesc.Height = m_nHeight;
	sRenderTargetDesc.MipLevels = 1;
	sRenderTargetDesc.ArraySize = 1;
	sRenderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sRenderTargetDesc.SampleDesc.Count = m_nNumSamples;
	sRenderTargetDesc.SampleDesc.Quality = GetMSAAQuality(sRenderTargetDesc.SampleDesc.Count, DXGI_FORMAT_R8G8B8A8_UNORM);
	sRenderTargetDesc.Usage = D3D11_USAGE_DEFAULT;
	sRenderTargetDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (sRenderTargetDesc.SampleDesc.Count != 1 && sRenderTargetDesc.SampleDesc.Quality == 0)
		sRenderTargetDesc.SampleDesc.Count = 1;

	// create the render target texture
	if (SUCCEEDED(DX11::Device( )->CreateTexture2D(&sRenderTargetDesc, NULL, &m_pTexture)))
	{
		ST_NAME_DX11_OBJECT(m_pTexture, "speedtree render target texture");

		// set up the description of the render target view
		D3D11_RENDER_TARGET_VIEW_DESC sRenderTargetViewDesc;
		ZeroMemory(&sRenderTargetViewDesc, sizeof(sRenderTargetViewDesc));

		sRenderTargetViewDesc.Format = sRenderTargetDesc.Format;
		sRenderTargetViewDesc.ViewDimension = (sRenderTargetDesc.SampleDesc.Count > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
		sRenderTargetViewDesc.Texture2D.MipSlice = 0;

		// create the render target view
		if (SUCCEEDED(DX11::Device( )->CreateRenderTargetView(m_pTexture, &sRenderTargetViewDesc, &m_pRenderTargetView)))
		{
			ST_NAME_DX11_OBJECT(m_pRenderTargetView, "speedtree render target view");

			// set up the description of the shader resource view
			D3D11_SHADER_RESOURCE_VIEW_DESC sShaderResourceViewDesc;
			ZeroMemory(&sShaderResourceViewDesc, sizeof(sShaderResourceViewDesc));

			sShaderResourceViewDesc.Format = sRenderTargetDesc.Format;
			sShaderResourceViewDesc.ViewDimension = (sRenderTargetDesc.SampleDesc.Count > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
			sShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			sShaderResourceViewDesc.Texture2D.MipLevels = 1;

			// create the shader resource view
			if (SUCCEEDED(DX11::Device( )->CreateShaderResourceView(m_pTexture, &sShaderResourceViewDesc, &m_pShaderResourceView)))
			{
				ST_NAME_DX11_OBJECT(m_pShaderResourceView, "speedtree shader resource view");

				bSuccess = true;
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetDirectX11::CreateDepthTarget

st_bool CRenderTargetDirectX11::CreateDepthTarget(void)
{
	st_bool bSuccess = false;

	// initialize the render target texture description
	D3D11_TEXTURE2D_DESC sDesc;
	ZeroMemory(&sDesc, sizeof(sDesc));

	// set up the render target texture description
	sDesc.Width = m_nWidth;
	sDesc.Height = m_nHeight;
	sDesc.MipLevels = 1;
	sDesc.ArraySize = 1;
	sDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	if (m_eType != RENDER_TARGET_TYPE_SHADOW_MAP)
	{
		sDesc.SampleDesc.Count = m_nNumSamples;
		sDesc.SampleDesc.Quality = GetMSAAQuality(sDesc.SampleDesc.Count, DXGI_FORMAT_R24G8_TYPELESS);
	}
	sDesc.Usage = D3D11_USAGE_DEFAULT;
	sDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	sDesc.CPUAccessFlags = 0;
	sDesc.MiscFlags = 0;
	
	// necessary for shadow maps
	if (sDesc.SampleDesc.Count != 1 && sDesc.SampleDesc.Quality == 0)
		sDesc.SampleDesc.Count = 1;

	// create the render target texture
	if (SUCCEEDED(DX11::Device( )->CreateTexture2D(&sDesc, NULL, &m_pTexture)))
	{
		ST_NAME_DX11_OBJECT(m_pTexture, "speedtree depth texture");

		// set up the description of the depth/stencil view
		D3D11_DEPTH_STENCIL_VIEW_DESC sDepthStencilViewDesc;
		ZeroMemory(&sDepthStencilViewDesc, sizeof(sDepthStencilViewDesc));

		sDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		sDepthStencilViewDesc.ViewDimension = (sDesc.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		sDepthStencilViewDesc.Texture2D.MipSlice = 0;
		
		// create depth/stencil view
		if (SUCCEEDED(DX11::Device( )->CreateDepthStencilView(m_pTexture, &sDepthStencilViewDesc, &m_pDepthStencilView)))
		{
			ST_NAME_DX11_OBJECT(m_pDepthStencilView, "speedtree depth stencil view");

			// set up the description of the shader resource view
			D3D11_SHADER_RESOURCE_VIEW_DESC sShaderResourceViewDesc;
			ZeroMemory(&sShaderResourceViewDesc, sizeof(sShaderResourceViewDesc));

			sShaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			sShaderResourceViewDesc.ViewDimension = (sDesc.SampleDesc.Count > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
			sShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			sShaderResourceViewDesc.Texture2D.MipLevels = 1;

			// create the shader resource view
			bSuccess = SUCCEEDED(DX11::Device( )->CreateShaderResourceView(m_pTexture, &sShaderResourceViewDesc, &m_pShaderResourceView));

			if (bSuccess)
				ST_NAME_DX11_OBJECT(m_pShaderResourceView, "speedtree shader resource view (depth)");
		}
	}

	return bSuccess;
}

