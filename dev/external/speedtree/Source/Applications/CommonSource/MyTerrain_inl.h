///////////////////////////////////////////////////////////////////////  
//  MyTerrain.inl
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
//  CMyTerrain::SetActive

inline void CMyTerrain::SetActive(st_bool bActive)
{
	m_bActive = bActive;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::IsActive

inline st_bool CMyTerrain::IsActive(void) const
{
	return m_bActive;
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::GetHeightRange

inline void CMyTerrain::GetHeightRange(st_float32 afHeightRange[2]) const
{
	m_cData.GetHeightRange(afHeightRange[0], afHeightRange[1]);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::GetHeightFromXY

inline st_float32 CMyTerrain::GetHeightFromXY(st_float32 x, st_float32 y) const
{
	return m_cData.GetHeight(x, y);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::GetSmoothHeightFromXY

inline st_float32 CMyTerrain::GetSmoothHeightFromXY(st_float32 x, st_float32 y, st_float32 fSmoothingDistance) const
{
	return m_cData.GetSmoothHeight(x, y, fSmoothingDistance);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::GetSlopeFromXY

inline st_float32 CMyTerrain::GetSlopeFromXY(st_float32 x, st_float32 y) const
{
	return m_cData.GetSlope(x, y);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::GetNormalFromXY

inline Vec3 CMyTerrain::GetNormalFromXY(st_float32 x, st_float32 y) const
{
	return m_cData.GetNormal(x, y);
}


///////////////////////////////////////////////////////////////////////  
//  CMyTerrain::GetHeightFromPlacementParameters

inline st_bool CMyTerrain::GetHeightFromPlacementParameters(st_float32& fHeight, 
															st_float32 x, 
															st_float32 y, 
															st_float32 fMinHeight, 
															st_float32 fMaxHeight,
															st_float32 fMinSlope, 
															st_float32 fMaxSlope) const
{
	return m_cData.GetHeightFromPlacementParameters(fHeight, x, y, fMinHeight, fMaxHeight, fMinSlope, fMaxSlope);
}


///////////////////////////////////////////////////////////////////////
//  CMyTerrain::AdjustZPosBasedOnSlope

inline st_float32 CMyTerrain::AdjustZPosBasedOnSlope(const Vec3& vOrigPos, CTree& sBaseTree) const
{
	st_float32 fNewZ = vOrigPos.z;

	// query tree height (related to how far we push into the ground)
	const st_float32 c_fTreeHeight = CCoordSys::UpComponent(sBaseTree.GetExtents( ).Max( ));

	const st_float32 c_fTerrainSinkScalar = 0.1f;
	const st_float32 c_fSlope = GetSlopeFromXY(vOrigPos.x, vOrigPos.y);
	fNewZ -= c_fTerrainSinkScalar * c_fSlope * c_fTreeHeight;

	return fNewZ;
}


