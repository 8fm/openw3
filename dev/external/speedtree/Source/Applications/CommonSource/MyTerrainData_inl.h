///////////////////////////////////////////////////////////////////////  
//  MyTerrainData.inl
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
//  CMyTerrainData::CMyTerrainData

inline CMyTerrainData::CMyTerrainData(void) :
	m_vSize(1000.0f, 1000.0f, 100.0f),
	m_fHeightScalar(0.1f),
	m_fMinHeight(FLT_MAX),
	m_fMaxHeight(-FLT_MAX),
	m_nTiles(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::~CMyTerrainData

inline CMyTerrainData::~CMyTerrainData(void)
{
	Clear( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::Clear

inline void CMyTerrainData::Clear(void)
{
	m_fMinHeight = FLT_MAX;
	m_fMaxHeight = -FLT_MAX;
	m_nTiles = 0;
	
	m_cHeightData.Clear( );
	m_cNormalData.Clear( );
	m_cSlopeData.Clear( );
	m_cAOData.Clear( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::IsLoaded

inline st_bool CMyTerrainData::IsLoaded(void) const
{
	return m_cHeightData.IsPresent( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GetHeightRange

inline void CMyTerrainData::GetHeightRange(st_float32& fMin, st_float32& fMax) const
{
	fMin = m_fMinHeight;
	fMax = m_fMaxHeight;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GetHeight

inline st_float32 CMyTerrainData::GetHeight(st_float32 x, st_float32 y) const
{
	if (m_nTiles == 1)
		return m_cHeightData.InterpolateValueClamped(x / m_vSize.x, y / m_vSize.y);
	else
		return m_cHeightData.InterpolateValue(x / m_vSize.x, y / m_vSize.y);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GetSmoothHeight

inline st_float32 CMyTerrainData::GetSmoothHeight(st_float32 x, st_float32 y, st_float32 fDistance) const
{
	x /= m_vSize.x;
	y /= m_vSize.y;
	fDistance /= st_max(m_vSize.x, m_vSize.y);

	return m_cHeightData.Smooth(x, y, fDistance, m_cSlopeData.InterpolateValue(x, y), m_nTiles == 1);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GetNormal

inline Vec3 CMyTerrainData::GetNormal(st_float32 x, st_float32 y) const
{
	st_assert(m_cNormalData.m_nWidth > 0 && m_cNormalData.m_nHeight > 0 && m_cNormalData.m_pData, "CMyTerrainData::GetNormal called before CMyTerrainData::m_cNormalData was initialized");

	st_float32 u = x / m_vSize.x;
	st_float32 v = y / m_vSize.y;
	st_float32 fWidthScale = st_float32(m_cNormalData.m_nWidth);
	st_float32 fHeightScale = st_float32(m_cNormalData.m_nHeight);

	if (m_nTiles == 1)
	{
		fWidthScale -= 1.0f;
		fHeightScale -= 1.0f;
		if (u < 0.0f)
			u = 0.0f;
		if (u > 1.0f)
			u = 1.0f;
		if (v < 0.0f)
			v = 0.0f;
		if (v > 1.0f)
			v = 1.0f;
	}
	else
	{
		u -= st_int32(u);
		if (u < 0.0f)
			u += 1.0f;
		if (u >= 1.0f)
			u = 0.0f;
		v -= st_int32(v);
		if (v < 0.0f)
			v += 1.0f;
		if (v >= 1.0f)
			v = 0.0f;
	}

	const st_int32 nLowerX = st_uint32(u * fWidthScale);
	const st_int32 nLowerY = st_uint32(v * fHeightScale);

	// determine how far into quad we are
	u = Frac(u * fWidthScale);
	v = Frac(v * fHeightScale);

	// because quad is triangulated, we have to know if it's the top or bottom triangle
	const st_bool c_bLowerTriangle = (u < 1.0f - v);

	return c_bLowerTriangle ? m_cNormalData.m_pData[nLowerX + nLowerY * m_cNormalData.m_nWidth].m_vLower : 
							  m_cNormalData.m_pData[nLowerX + nLowerY * m_cNormalData.m_nWidth].m_vUpper;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GetSlope

inline st_float32 CMyTerrainData::GetSlope(st_float32 x, st_float32 y) const
{
	if (m_nTiles == 1)
		return m_cSlopeData.InterpolateValueClamped(x / m_vSize.x, y / m_vSize.y);
	else
		return m_cSlopeData.InterpolateValue(x / m_vSize.x, y / m_vSize.y);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GetHeightFromPlacementParameters

inline st_bool CMyTerrainData::GetHeightFromPlacementParameters(st_float32& fHeight, 
															    st_float32 x, 
															    st_float32 y, 
															    st_float32 fMinHeight, 
															    st_float32 fMaxHeight,
															    st_float32 fMinSlope, 
															    st_float32 fMaxSlope) const
{
	st_bool bReturn = false;

	st_float32 fSlope = GetSlope(x, y);
	if (fSlope >= fMinSlope && fSlope <= fMaxSlope)
	{
		fHeight = GetHeight(x, y);
		if (fHeight >= fMinHeight && fHeight <= fMaxHeight)
			bReturn = true;
	}

	return bReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrainData::GetAmbientOcclusion

inline st_float32 CMyTerrainData::GetAmbientOcclusion(st_float32 x, st_float32 y) const
{
	if (m_cAOData.m_pData)
	{
		const st_float32 fU = (x - m_afAOBounds[0]) * m_afAOBounds[2];
		const st_float32 fV = (y - m_afAOBounds[1]) * m_afAOBounds[3];
		if (fU > 0.0f && fU < 1.0f && fV > 0.0f && fV < 1.0f)
		{
			st_uchar ucReturn = m_cAOData.NearestNeighbor(fU, fV);
			return st_float32(ucReturn) * 0.0039215686f; // convert to 0-1
		}
	}

	return 1.0f;
}



