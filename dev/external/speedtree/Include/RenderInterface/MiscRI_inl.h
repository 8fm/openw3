///////////////////////////////////////////////////////////////////////  
//  MiscRI.inl
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
//  SSimpleMaterial::SSimpleMaterial

ST_INLINE SSimpleMaterial::SSimpleMaterial( ) :
	m_vAmbient(0.2f, 0.2f, 0.2f),
	m_vDiffuse(1.0f, 1.0f, 1.0f),
	m_vSpecular(1.0f, 1.0f, 1.0f),
	m_vTransmission(0.0f, 0.0f, 0.0f)
{
}


////////////////////////////////////////////////////////////
// CForestRI_Interpolate

template <class T> inline T CForestRI_Interpolate(const T& tStart, const T& tEnd, float fPercent)
{
	return static_cast<T>((tStart + (tEnd - tStart) * fPercent));
}


///////////////////////////////////////////////////////////////////////
//  SForestRenderInfo::SForestRenderInfo

inline SForestRenderInfo::SForestRenderInfo( ) :
	// general rendering
	m_nMaxAnisotropy(1),
	m_bHorizontalBillboards(true),
	m_fNearClip(0.5f),
	m_fFarClip(1000.0f),
	m_bTexturingEnabled(true),
	m_fTextureAlphaScalar3d(1.0f),
	m_fTextureAlphaScalarGrass(1.0f),
	m_fTextureAlphaScalarBillboards(1.0f),
	// fog
	m_fFogStartDistance(2500.0f),
	m_fFogEndDistance(5000.0f),
	m_vFogColor(1.0f, 1.0f, 1.0),
	// sky
	m_vSkyColor(0.2f, 0.3f, 0.5f),
	m_fSkyFogMin(-0.5f),
	m_fSkyFogMax(1.0f),
	// sun
	m_vSunColor(1.0f, 1.0f, 0.85f),
	m_fSunSize(0.001f),
	m_fSunSpreadExponent(200.0f),
	// shadows
	m_bShadowsEnabled(false),
	m_nShadowsNumMaps(4),
	m_nShadowsResolution(1024),
	m_fShadowFadePercent(0.25f)
{
}
