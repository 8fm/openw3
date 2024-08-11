///////////////////////////////////////////////////////////////////////
//  StateBlock.cpp
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
//  CStateBlockDirectX11::CStateBlockDirectX11

CStateBlockDirectX11::CStateBlockDirectX11( ) :
	m_pDepth(NULL),
	m_pRasterizer(NULL),
	m_pBlend(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX11::Init

st_bool CStateBlockDirectX11::Init(const SAppState& sAppState, const SRenderState& sRenderState)
{
	st_bool bSuccess = true;

	// depth
	{
		assert(!m_pDepth); // can't call more than once

		D3D11_DEPTH_STENCIL_DESC sDepthDesc;
        sDepthDesc.DepthEnable = (sAppState.m_eOverrideDepthTest == SAppState::OVERRIDE_DEPTH_TEST_DISABLE) ? FALSE : TRUE;
		if (!sAppState.m_bDepthPrepass)
		{
			sDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			sDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		}
		else
		{
			if (sRenderState.m_eRenderPass != RENDER_PASS_MAIN)
			{
				sDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
				sDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
			}
			else
			{
				sDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				sDepthDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
			}
		}
		sDepthDesc.StencilEnable = false;
		sDepthDesc.StencilReadMask = 0xFF;
		sDepthDesc.StencilWriteMask = 0xFF;
		sDepthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		sDepthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		sDepthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		sDepthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		sDepthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		sDepthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		sDepthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		sDepthDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		bSuccess &= SUCCEEDED(DX11::Device( )->CreateDepthStencilState(&sDepthDesc, &m_pDepth));

		if (bSuccess)
			ST_NAME_DX11_OBJECT(m_pDepth, "speedtree depth state block");
	}

	// rasterizer
	{
		assert(!m_pRasterizer); // can't call more than once

		D3D11_RASTERIZER_DESC sRasterizerDesc;
		sRasterizerDesc.FillMode = D3D11_FILL_SOLID;
		if (sRenderState.m_eFaceCulling == CULLTYPE_BACK)
			sRasterizerDesc.CullMode = D3D11_CULL_BACK;
		else if (sRenderState.m_eFaceCulling == CULLTYPE_FRONT)
			sRasterizerDesc.CullMode = D3D11_CULL_FRONT;
		else
			sRasterizerDesc.CullMode = D3D11_CULL_NONE;
		sRasterizerDesc.FrontCounterClockwise = true;
		if (sRenderState.m_eRenderPass == RENDER_PASS_SHADOW_CAST)
		{
			const st_float32 c_fNoCullFactor = 10.0f * 6.0f;
			const st_float32 c_fBackCullFactor = 1.0f * 6.0f;

			sRasterizerDesc.DepthBias = false;
			sRasterizerDesc.DepthBiasClamp = 0.0f;
			if (sRenderState.m_eFaceCulling == CULLTYPE_NONE)
				sRasterizerDesc.SlopeScaledDepthBias = c_fNoCullFactor;
			else
				sRasterizerDesc.SlopeScaledDepthBias = c_fBackCullFactor;
		}
		else
		{
			sRasterizerDesc.DepthBias = false;
			sRasterizerDesc.DepthBiasClamp = 0;
			sRasterizerDesc.SlopeScaledDepthBias = 0;
		}
		sRasterizerDesc.DepthClipEnable = true;
		sRasterizerDesc.ScissorEnable = false;
		sRasterizerDesc.MultisampleEnable = sAppState.m_bMultisampling;
		sRasterizerDesc.AntialiasedLineEnable = false;

		bSuccess &= SUCCEEDED(DX11::Device( )->CreateRasterizerState(&sRasterizerDesc, &m_pRasterizer));

		if (bSuccess)
			ST_NAME_DX11_OBJECT(m_pRasterizer, "speedtree rasterizer state block");
	}

	// blend
	{
		assert(!m_pBlend); // can't call more than once

		D3D11_BLEND_DESC sBlendDesc;
		memset(&sBlendDesc, 0, sizeof(D3D11_BLEND_DESC));
		if (sRenderState.m_eRenderPass == RENDER_PASS_SHADOW_CAST)
			sBlendDesc.AlphaToCoverageEnable = false;
		else
			sBlendDesc.AlphaToCoverageEnable = sAppState.m_bAlphaToCoverage;

		for (st_int32 i = 0; i < c_nMaxNumRenderTargets; ++i)
		{
			if (sRenderState.m_bBlending)
			{
				sBlendDesc.RenderTarget[i].BlendEnable = true;
				sBlendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				sBlendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			}
			else
			{
				sBlendDesc.RenderTarget[i].BlendEnable = false;
				sBlendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
				sBlendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
			}
			sBlendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
			sBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
			sBlendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
			sBlendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		
			// color mask
			if (sRenderState.m_eRenderPass != RENDER_PASS_MAIN)
				sBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA; // todo: was 0
			else
				sBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		bSuccess &= SUCCEEDED(DX11::Device( )->CreateBlendState(&sBlendDesc, &m_pBlend));

		if (bSuccess)
			ST_NAME_DX11_OBJECT(m_pBlend, "speedtree blend state block");
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX11::Bind

st_bool CStateBlockDirectX11::Bind(void) const
{
	st_bool bSuccess = false;

	if (m_pDepth && m_pRasterizer && m_pBlend)
	{
		DX11::DeviceContext( )->OMSetDepthStencilState(m_pDepth, 0);

		DX11::DeviceContext( )->RSSetState(m_pRasterizer);

		const FLOAT c_afPixels[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		DX11::DeviceContext( )->OMSetBlendState(m_pBlend, c_afPixels, 0xffffffff);

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockDirectX11::ReleaseGfxResources

void CStateBlockDirectX11::ReleaseGfxResources(void)
{
	// depth
	if (m_pDepth)
	{
		// if currently bound, unbind before releasing
		ID3D11DepthStencilState* pCurrentDepth = NULL;
		UINT uiUnused = 0;
		DX11::DeviceContext( )->OMGetDepthStencilState(&pCurrentDepth, &uiUnused);
		if (pCurrentDepth == m_pDepth)
			DX11::DeviceContext( )->OMSetDepthStencilState(NULL, 0);
		ST_SAFE_RELEASE(pCurrentDepth);

		ST_SAFE_RELEASE(m_pDepth);
	}

	// rasterizer
	if (m_pRasterizer)
	{
		// if currently bound, unbind before releasing
		ID3D11RasterizerState* pCurrentRasterizer = NULL;
		DX11::DeviceContext( )->RSGetState(&pCurrentRasterizer);
		if (pCurrentRasterizer == m_pRasterizer)
			DX11::DeviceContext( )->RSSetState(NULL);
		ST_SAFE_RELEASE(pCurrentRasterizer);

		ST_SAFE_RELEASE(m_pRasterizer);
	}

	// blend
	if (m_pBlend)
	{
		const FLOAT c_afPixels[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		DX11::DeviceContext( )->OMSetBlendState(NULL, c_afPixels, 0xffffffff);

		// if currently bound, unbind before releasing
		ID3D11BlendState* pCurrentState = NULL;
		FLOAT afUnused1[4] = { 0.0f };
		UINT uiUnused2 = 0;
		DX11::DeviceContext( )->OMGetBlendState(&pCurrentState, afUnused1, &uiUnused2);
		if (pCurrentState == m_pBlend)
			DX11::DeviceContext( )->OMSetBlendState(NULL, c_afPixels, 0xffffffff);
		ST_SAFE_RELEASE(pCurrentState);

		ST_SAFE_RELEASE(m_pBlend);
	}
}





