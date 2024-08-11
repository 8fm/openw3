///////////////////////////////////////////////////////////////////////
//  Texture.inl
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
//  CTextureDirectX11::CTextureDirectX11

inline CTextureDirectX11::CTextureDirectX11( ) :
	m_pTexture(NULL),
	m_bIsGeneratedUniformColor(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::~CTextureDirectX11

inline CTextureDirectX11::~CTextureDirectX11( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::IsValid

inline bool CTextureDirectX11::IsValid(void) const
{
	return (m_pTexture != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::GetTextureObject

inline ID3D11ShaderResourceView* CTextureDirectX11::GetTextureObject(void) const
{
	return m_pTexture;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::GetInfo

inline const STextureInfo& CTextureDirectX11::GetInfo(void) const
{
	return m_sInfo;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::IsGeneratedUniformColor

inline st_bool CTextureDirectX11::IsGeneratedUniformColor(void) const
{
	return m_bIsGeneratedUniformColor;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::operator=

inline CTextureDirectX11& CTextureDirectX11::operator=(const CTextureDirectX11& cRight)
{
	m_pTexture = cRight.m_pTexture;
	m_sInfo = cRight.m_sInfo;
	m_bIsGeneratedUniformColor = cRight.m_bIsGeneratedUniformColor;

	return *this;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureDirectX11::operator!=

inline st_bool CTextureDirectX11::operator!=(const CTextureDirectX11& cRight) const
{
	return (m_pTexture != cRight.m_pTexture);
}


///////////////////////////////////////////////////////////////////////  
//  ReleaseSampler

inline void ReleaseSampler(ID3D11SamplerState*& pSampler)
{
	const st_int32 c_nMaxNumSlots = TEXTURE_REGISTER_COUNT;

	// before releasing, unbind sampler if currently bound
	ID3D11SamplerState* apSamplers[c_nMaxNumSlots] = { NULL };
	DX11::DeviceContext( )->PSGetSamplers(0, c_nMaxNumSlots, apSamplers);
	for (st_int32 i = 0; i < c_nMaxNumSlots; ++i)
	{
		if (apSamplers[i] == pSampler)
		{
			ID3D11SamplerState* pNullSampler = NULL;
			DX11::DeviceContext( )->PSSetSamplers(i, 1, &pNullSampler);
		}

		// PSGetSamplers increments ref counts, so decrement
		ST_SAFE_RELEASE(apSamplers[i]);
	}

	// now that it's no longer bound anywhere, release sampler
	ST_SAFE_RELEASE(pSampler);
}


///////////////////////////////////////////////////////////////////////  
//  ReleaseTexture

inline void ReleaseTexture(ID3D11ShaderResourceView*& pTexture)
{
	const st_int32 c_nMaxNumSlots = TEXTURE_REGISTER_COUNT;

	// before releasing, unbind texture if currently bound
	ID3D11ShaderResourceView* apTextures[c_nMaxNumSlots] = { NULL };
	DX11::DeviceContext( )->PSGetShaderResources(0, c_nMaxNumSlots, apTextures);
	for (st_int32 i = 0; i < c_nMaxNumSlots; ++i)
	{
		if (apTextures[i] == pTexture)
		{
			ID3D11ShaderResourceView* pNullView = NULL;
			DX11::DeviceContext( )->PSSetShaderResources(i, 1, &pNullView);
		}

		// PSGetShaderResources docs say we need to release this
		ST_SAFE_RELEASE(apTextures[i]);
	}

	// release texture
	ST_SAFE_RELEASE(pTexture);
}

