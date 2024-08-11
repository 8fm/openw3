///////////////////////////////////////////////////////////////////////
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
#include <D3DX11tex.h>
#include "Core/PerlinNoiseKernel.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  CTextureDirectX11 static member variables

ID3D11SamplerState* CTextureDirectX11::m_pStandardSampler = NULL;
ID3D11SamplerState* CTextureDirectX11::m_pShadowMapCompareSampler = NULL;
ID3D11SamplerState* CTextureDirectX11::m_pPointSampler = NULL;
ID3D11SamplerState* CTextureDirectX11::m_pLinearClampSampler = NULL;
st_int32 CTextureDirectX11::m_nSharedSamplerRefCount = 0;
st_int32 CTextureDirectX11::m_nMaxAnisotropy = 1;


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::Load

st_bool CTextureDirectX11::Load(const char* pFilename, st_int32 nMaxAnisotropy)
{
	if (pFilename && strlen(pFilename) > 0)
	{
		assert(DX11::Device( ));

		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		// set up a temporary buffer for reading the texture
		size_t siTextureFileSize = pFileSystem->FileSize(pFilename);
		if (siTextureFileSize > 0)
		{
			st_byte* pTextureBuffer = pFileSystem->LoadFile(pFilename);
			if (pTextureBuffer)
			{
				D3DX11_IMAGE_LOAD_INFO sInfo;
				sInfo.Filter = D3DX11_FILTER_TRIANGLE | D3DX11_FILTER_MIRROR;
				if (SUCCEEDED(D3DX11CreateShaderResourceViewFromMemory(DX11::Device( ), pTextureBuffer, siTextureFileSize, &sInfo, NULL, &m_pTexture, NULL)))
				{
					ST_NAME_DX11_OBJECT(m_pTexture, "speedtree texture shader resource view");

					m_bIsGeneratedUniformColor = false;

					// fill out texture info struct
					D3DX11_IMAGE_INFO sImageInfo;
					if (SUCCEEDED(D3DX11GetImageInfoFromMemory(pTextureBuffer, siTextureFileSize, NULL, &sImageInfo, NULL)))
					{
						m_sInfo.m_nWidth = st_int32(sImageInfo.Width);
						m_sInfo.m_nHeight = st_int32(sImageInfo.Height);

						m_nMaxAnisotropy = nMaxAnisotropy;
						++m_nSharedSamplerRefCount;
					}
				}

				pFileSystem->Release(pTextureBuffer);
			}
		}
	}

	return (m_pTexture != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::LoadColor

st_bool CTextureDirectX11::LoadColor(st_uint32 uiColor)
{
	st_bool bSuccess = false;

	CD3D11_TEXTURE2D_DESC sDesc;
	ZeroMemory(&sDesc, sizeof(sDesc));
	sDesc.Width = 4;
	sDesc.Height = 4;
	sDesc.MipLevels = sDesc.ArraySize = 1;
	sDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sDesc.SampleDesc.Count = 1;
	sDesc.Usage = D3D11_USAGE_DYNAMIC;
	sDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ID3D11Texture2D* pTexture = NULL;

	assert(DX11::Device( ));
	if (SUCCEEDED(DX11::Device( )->CreateTexture2D(&sDesc, NULL, &pTexture)))
	{
		ST_NAME_DX11_OBJECT(pTexture, "speedtree uniform color texture");

		D3D11_MAPPED_SUBRESOURCE sMappedTex;
		DX11::DeviceContext( )->Map(pTexture, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE_DISCARD, 0, &sMappedTex);

		for (UINT uiRow = 0; uiRow < sDesc.Height; ++uiRow)
		{
			UCHAR* pTexels = reinterpret_cast<UCHAR*>(sMappedTex.pData) + uiRow * sMappedTex.RowPitch;
			for (UINT uiCol = 0; uiCol < sDesc.Width; ++uiCol)
			{
				*pTexels++ = UCHAR((uiColor & 0xff000000) >> 24); // red
				*pTexels++ = UCHAR((uiColor & 0x00ff0000) >> 16); // green
				*pTexels++ = UCHAR((uiColor & 0x0000ff00) >> 8);  // blue
				*pTexels++ = UCHAR((uiColor & 0x000000ff) >> 0);  // alpha
			}
		}

		DX11::DeviceContext( )->Unmap(pTexture, D3D11CalcSubresource(0, 0, 1));

		bSuccess = SUCCEEDED(DX11::Device( )->CreateShaderResourceView(pTexture, NULL, &m_pTexture));
		ST_SAFE_RELEASE(pTexture);

		if (bSuccess)
		{
			ST_NAME_DX11_OBJECT(m_pTexture, "speedtree uniform color shader resource view");
			++m_nSharedSamplerRefCount;
		}
	}

	m_bIsGeneratedUniformColor = true;

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::LoadNoise

st_bool CTextureDirectX11::LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise)
{
	st_bool bSuccess = false;

	if (nWidth > 4 && nHeight > 4 && nWidth <= 4096 && nHeight <= 4096)
	{
		assert(DX11::Device( ));
		CRandom cRandom;

		D3D11_TEXTURE2D_DESC sDesc;
		ZeroMemory(&sDesc, sizeof(sDesc));
		sDesc.Width = nWidth;
		sDesc.Height = nHeight;
		sDesc.MipLevels = sDesc.ArraySize = 1;
		sDesc.Format = DXGI_FORMAT_A8_UNORM;
		sDesc.SampleDesc.Count = 1;
		sDesc.Usage = D3D11_USAGE_DYNAMIC;
		sDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ID3D11Texture2D* pTexture = NULL;
		if (DX11::Device( )->CreateTexture2D(&sDesc, NULL, &pTexture) == S_OK)
		{
			ST_NAME_DX11_OBJECT(pTexture, "speedtree noise texture");

			D3D11_MAPPED_SUBRESOURCE sMappedTex;
			DX11::DeviceContext( )->Map(pTexture, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE_DISCARD, 0, &sMappedTex);

			for (UINT uiRow = 0; uiRow < sDesc.Height; ++uiRow)
			{
				UCHAR* pTexels = reinterpret_cast<UCHAR*>(sMappedTex.pData) + uiRow * sMappedTex.RowPitch;
				for (UINT uiCol = 0; uiCol < sDesc.Width; ++uiCol)
					*pTexels++ = UCHAR(cRandom.GetInteger(st_int32(fLowNoise * 255), st_int32(fHighNoise * 255)));
			}

			DX11::DeviceContext( )->Unmap(pTexture, D3D11CalcSubresource(0, 0, 1));

			bSuccess = (DX11::Device( )->CreateShaderResourceView(pTexture, NULL, &m_pTexture) == S_OK);
			pTexture->Release( );

			if (bSuccess)
			{
				ST_NAME_DX11_OBJECT(m_pTexture, "speedtree noise shader resource view");
				++m_nSharedSamplerRefCount;
			}
		}
	}

	m_bIsGeneratedUniformColor = true;

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::LoadPerlinNoiseKernel

// todo: update parameters to just size?
st_bool CTextureDirectX11::LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth)
{
	st_bool bSuccess = false;

	ST_UNREF_PARAM(nDepth); // todo
	if (nWidth > 4 && nHeight > 4 && nWidth <= 4096 && nHeight <= 4096)
	{
		assert(DX11::Device( ));
		CRandom cRandom;

		D3D11_TEXTURE2D_DESC sDesc;
		ZeroMemory(&sDesc, sizeof(sDesc));
		sDesc.Width = nWidth;
		sDesc.Height = nHeight;
		sDesc.MipLevels = sDesc.ArraySize = 1;
		sDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sDesc.SampleDesc.Count = 1;
		sDesc.Usage = D3D11_USAGE_DYNAMIC;
		sDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ID3D11Texture2D* pTexture = NULL;
		if (DX11::Device( )->CreateTexture2D(&sDesc, NULL, &pTexture) == S_OK)
		{
			ST_NAME_DX11_OBJECT(pTexture, "speedtree perlin noise kernel texture");

			D3D11_MAPPED_SUBRESOURCE sMappedTex;
			DX11::DeviceContext( )->Map(pTexture, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE_DISCARD, 0, &sMappedTex);
			{
				// todo: comment this stuff
				CPerlinNoiseKernel cKernel(nWidth);

				#define XXX 1.0 // todo
				const st_float32 c_afSampleOffsets[4][2] = 
				{
					// todo
					{ -0.5f * XXX, -0.5f * XXX },
					{ -0.5f * XXX,  0.5f * XXX },
					{  0.5f * XXX, -0.5f * XXX },
					{  0.5f * XXX,  0.5f * XXX }
				};

				for (UINT uiRow = 0; uiRow < sDesc.Height; ++uiRow)
				{
					UCHAR* pTexels = reinterpret_cast<UCHAR*>(sMappedTex.pData) + uiRow * sMappedTex.RowPitch;
					for (UINT uiCol = 0; uiCol < sDesc.Width; ++uiCol)
					{
						*pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[0][0], uiRow + c_afSampleOffsets[0][1])); // red
						*pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[1][0], uiRow + c_afSampleOffsets[1][1])); // green
						*pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[2][0], uiRow + c_afSampleOffsets[2][1])); // blue
						*pTexels++ = UCHAR(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[3][0], uiRow + c_afSampleOffsets[3][1])); // alpha
					}
				}
			}
			DX11::DeviceContext( )->Unmap(pTexture, D3D11CalcSubresource(0, 0, 1));

			bSuccess = (DX11::Device( )->CreateShaderResourceView(pTexture, NULL, &m_pTexture) == S_OK);
			pTexture->Release( );

			if (bSuccess)
			{
				ST_NAME_DX11_OBJECT(m_pTexture, "speedtree perlin noise kernel shader resource view");
				++m_nSharedSamplerRefCount;
			}
		}
	}

	m_bIsGeneratedUniformColor = true;

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::ReleaseGfxResources

st_bool CTextureDirectX11::ReleaseGfxResources(void)
{
	st_bool bSuccess = true;

	if (m_pTexture)
	{
		if (--m_nSharedSamplerRefCount == 0)
		{
			ReleaseSampler(m_pStandardSampler);
			ReleaseSampler(m_pShadowMapCompareSampler);
			ReleaseSampler(m_pPointSampler);
			ReleaseSampler(m_pLinearClampSampler);
		}

		ReleaseTexture(m_pTexture);

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::SetSamplerStates

void CTextureDirectX11::SetSamplerStates(void)
{
	// create the shared samplers if they haven't already been
	if (!m_pStandardSampler)
		CreateSharedSamplers(m_nMaxAnisotropy);

	if (m_pStandardSampler && m_pShadowMapCompareSampler && m_pPointSampler && m_pLinearClampSampler)
	{
		ID3D11SamplerState* apSamplers[SAMPLER_REGISTER_COUNT] =
		{
			m_pStandardSampler,
			m_pShadowMapCompareSampler,
			m_pPointSampler,
			m_pLinearClampSampler
		};

		DX11::DeviceContext( )->PSSetSamplers(SAMPLER_REGISTER_STANDARD, SAMPLER_REGISTER_COUNT, apSamplers);
		DX11::DeviceContext( )->VSSetSamplers(SAMPLER_REGISTER_STANDARD, SAMPLER_REGISTER_COUNT, apSamplers);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::CreateSharedSamplers

void CTextureDirectX11::CreateSharedSamplers(st_int32 nMaxAnisotropy)
{
	// standard sampler
	D3D11_SAMPLER_DESC sStandardSamplerDesc;
	ZeroMemory(&sStandardSamplerDesc, sizeof(sStandardSamplerDesc));
	sStandardSamplerDesc.Filter = (nMaxAnisotropy > 1) ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sStandardSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sStandardSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sStandardSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sStandardSamplerDesc.MipLODBias = 0.0f;
	sStandardSamplerDesc.MaxAnisotropy = nMaxAnisotropy;
	sStandardSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	st_float32 afWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	memcpy(sStandardSamplerDesc.BorderColor, afWhite, sizeof(afWhite));
	sStandardSamplerDesc.MinLOD = 0.0f;
	sStandardSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sStandardSamplerDesc, &m_pStandardSampler)))
		ST_NAME_DX11_OBJECT(m_pStandardSampler, "speedtree standard sampler");

	// shadow map comparison sampler
	D3D11_SAMPLER_DESC sShadowMapSamplerDesc;
	ZeroMemory(&sShadowMapSamplerDesc, sizeof(sShadowMapSamplerDesc));
	sShadowMapSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sShadowMapSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sShadowMapSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sShadowMapSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sShadowMapSamplerDesc.MipLODBias = 0.0f;
	sShadowMapSamplerDesc.MaxAnisotropy = 0;
	sShadowMapSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	const st_float32 c_afWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	memcpy(sShadowMapSamplerDesc.BorderColor, c_afWhite, sizeof(c_afWhite));
	sShadowMapSamplerDesc.MinLOD = 0.0f;
	sShadowMapSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sShadowMapSamplerDesc, &m_pShadowMapCompareSampler)))
		ST_NAME_DX11_OBJECT(m_pShadowMapCompareSampler, "speedtree shadow map sampler");

	// point sampler
	D3D11_SAMPLER_DESC sPointSamplerDesc;
	ZeroMemory(&sPointSamplerDesc, sizeof(sPointSamplerDesc));
	sPointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sPointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; // need wrap when accessing generated noise texture
	sPointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sPointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sPointSamplerDesc, &m_pPointSampler)))
		ST_NAME_DX11_OBJECT(m_pPointSampler, "speedtree point sampler");

	// no mipmap sampler
	D3D11_SAMPLER_DESC sLinearClampSamplerDesc;
	ZeroMemory(&sLinearClampSamplerDesc, sizeof(sLinearClampSamplerDesc));
	sLinearClampSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sLinearClampSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sLinearClampSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sLinearClampSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	if (SUCCEEDED(DX11::Device( )->CreateSamplerState(&sLinearClampSamplerDesc, &m_pLinearClampSampler)))
		ST_NAME_DX11_OBJECT(m_pLinearClampSampler, "speedtree linear clamp sampler");
}
