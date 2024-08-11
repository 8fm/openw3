/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "speedTreeRenderInterface.h"


inline CTextureGPUAPI::CTextureGPUAPI( )
	: m_texture( nullptr )
	, m_bIsGeneratedUniformColor(false)
{
}

inline CTextureGPUAPI::~CTextureGPUAPI( )
{
}

inline bool CTextureGPUAPI::IsValid() const
{
	return m_texture != nullptr;
}

inline CRenderTexture* CTextureGPUAPI::GetRenderTexture() const
{
	return m_texture;
}

inline void CTextureGPUAPI::SetSamplers( st_int32 /*nMaxAnisotropy*/ )
{
	// intentionally blank
}

//inline GpuApi::SamplerStateRef CTextureGPUAPI::GetFilteredMipMapSampler()
//{
//	return m_pFilteredMipMapSampler;
//}
//
//inline GpuApi::SamplerStateRef CTextureGPUAPI::GetPointNoMipMapSampler()
//{
//	return m_pPointNoMipMapSampler;
//}

inline const STextureInfo& CTextureGPUAPI::GetInfo() const
{
	return m_sInfo;
}


inline st_bool CTextureGPUAPI::IsGeneratedUniformColor() const
{
	return m_bIsGeneratedUniformColor;
}

inline CTextureGPUAPI& CTextureGPUAPI::operator=(const CTextureGPUAPI& cRight)
{
	// No addref, SpeedTree does it's own stuff internally...
	m_texture = cRight.m_texture;
	m_sInfo = cRight.m_sInfo;
	m_bIsGeneratedUniformColor = cRight.m_bIsGeneratedUniformColor;

	return *this;
}

inline st_bool CTextureGPUAPI::operator!=(const CTextureGPUAPI& cRight) const
{
	return (m_texture != cRight.m_texture);
}