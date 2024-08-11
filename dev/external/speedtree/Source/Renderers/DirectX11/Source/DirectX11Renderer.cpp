///////////////////////////////////////////////////////////////////////
//  DirectX11Renderer.cpp
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

#include "Renderers/DirectX11/DirectX11Renderer.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//	Static member variables

ID3D11Device* DX11::m_pDx11 = NULL;
ID3D11DeviceContext* DX11::m_pDx11Context = NULL;

CShaderTechnique::CVertexShaderCache* CShaderTechnique::m_pVertexShaderCache = NULL;
CShaderTechnique::CPixelShaderCache* CShaderTechnique::m_pPixelShaderCache = NULL;
CTexture::CTextureCache* CTexture::m_pCache = NULL;
CStateBlock::CStateBlockCache* CStateBlock::m_pCache = NULL;

CTexture CRenderState::m_atLastBoundTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
CTexture CRenderState::m_atFallbackTextures[TL_NUM_TEX_LAYERS] = { CTexture( ) };
st_int32 CRenderState::m_nFallbackTextureRefCount = 0;

CShaderConstantBuffer CForestRender::m_cFrameConstantBuffer = CShaderConstantBuffer( );
SFrameCBLayout CForestRender::m_sFrameConstantBufferLayout = SFrameCBLayout( );

ID3D11RenderTargetView* CRenderTargetDirectX11::m_apPreviouslyActiveRenderTargetViews[c_nMaxNumRenderTargets] = { NULL };
ID3D11DepthStencilView* CRenderTargetDirectX11::m_pPreviouslyActiveDepthStencilView = { NULL };
D3D11_VIEWPORT CRenderTargetDirectX11::m_sPreviouslyActiveViewport = D3D11_VIEWPORT( );




///////////////////////////////////////////////////////////////////////
//	DX11::SetDevice

void DX11::SetDevice(ID3D11Device* pDevice)							
{ 
	m_pDx11 = pDevice; 
}


///////////////////////////////////////////////////////////////////////
//	DX11::SetDeviceContext

void DX11::SetDeviceContext(ID3D11DeviceContext* pDeviceContext)							
{ 
	m_pDx11Context = pDeviceContext; 
}


