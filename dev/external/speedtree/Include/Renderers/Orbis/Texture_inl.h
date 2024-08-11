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
//  CTextureOrbis::CTextureOrbis

inline CTextureOrbis::CTextureOrbis( )
{
	memset(&m_cTexture, 0, sizeof(sce::Gnm::Texture));
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::~CTextureOrbis

inline CTextureOrbis::~CTextureOrbis( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::IsValid

inline st_bool CTextureOrbis::IsValid(void) const
{
	return m_cTexture.isTexture( );
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::GetTextureObject

inline const sce::Gnm::Texture* CTextureOrbis::GetTextureObject(void) const
{
	return &m_cTexture;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::operator!=

inline st_bool CTextureOrbis::operator!=(const CTextureOrbis& cRight) const
{
	return (memcmp(&m_cTexture, &cRight.m_cTexture, sizeof(sce::Gnm::Texture)) != 0);
}

///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::GetInfo

inline const STextureInfo& CTextureOrbis::GetInfo(void) const
{
	return m_sInfo;
}


///////////////////////////////////////////////////////////////////////  
//  CTextureOrbis::SetSamplerStates

inline void CTextureOrbis::SetSamplerStates(void)
{
	sce::Gnm::Sampler acSamplers[3];

	acSamplers[0].init( );
	acSamplers[0].setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	acSamplers[0].setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);

	acSamplers[1].init( );
	acSamplers[1].setMipFilterMode(sce::Gnm::kMipFilterModeLinear);
	acSamplers[1].setXyFilterMode(sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeBilinear);
	acSamplers[1].setDepthCompareFunction(sce::Gnm::kDepthCompareLessEqual);
	acSamplers[1].setWrapMode(sce::Gnm::kWrapModeClampLastTexel, sce::Gnm::kWrapModeClampLastTexel, sce::Gnm::kWrapModeClampLastTexel);

	acSamplers[2].init( );
	acSamplers[2].setMipFilterMode(sce::Gnm::kMipFilterModePoint);
	acSamplers[2].setXyFilterMode(sce::Gnm::kFilterModePoint, sce::Gnm::kFilterModePoint);

	Orbis::Context( )->setSamplers(sce::Gnm::kShaderStagePs, 0, 3, acSamplers);
	Orbis::Context( )->setSamplers(sce::Gnm::kShaderStageVs, 0, 3, acSamplers);
}