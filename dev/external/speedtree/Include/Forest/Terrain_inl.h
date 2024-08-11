///////////////////////////////////////////////////////////////////////
//  Terrain.inl
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
//  CTerrainCell::CTerrainCell

ST_INLINE CTerrainCell::CTerrainCell( ) :
	// grid data
	m_nRow(c_nInvalidRowColIndex),
	m_nCol(c_nInvalidRowColIndex),
	// extents & culling
	m_fCullRadius(-1.0f),
	m_nFrameIndex(-1),
	// LOD
	m_uiIndicesOffset(0),
	m_uiNumIndices(0),
	m_uiMinIndex(0),
	m_uiNumVertices(0),
	m_fDistanceFromCamera(0.0f),
	m_nLod(-1),
	m_pVbo(NULL),
	// visibility
	m_bPopulated(true)
{
	assert(IsNew( ));
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::~CTerrainCell

ST_INLINE CTerrainCell::~CTerrainCell( )
{
	#ifndef NDEBUG
		m_uiIndicesOffset = 0;
		m_uiNumIndices = 0;
		m_fDistanceFromCamera = 1.0f;
		m_nLod = -1;
		m_pVbo = NULL;
	#endif
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::IsNew

ST_INLINE st_bool CTerrainCell::IsNew(void) const
{
	return !m_cExtents.Valid( );
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::Col

ST_INLINE st_int32 CTerrainCell::Col(void) const
{
	return m_nCol;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::Row

ST_INLINE st_int32 CTerrainCell::Row(void) const
{
	return m_nRow;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::SetRowCol

ST_INLINE void CTerrainCell::SetRowCol(st_int32 nRow, st_int32 nCol)
{
	m_nRow = nRow;
	m_nCol = nCol;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetExtents

ST_INLINE const CExtents& CTerrainCell::GetExtents(void) const
{
	return m_cExtents;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::InvalidateExtents

ST_INLINE void CTerrainCell::InvalidateExtents(void)
{
	m_cExtents.Reset( );
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::SetExtents

ST_INLINE void CTerrainCell::SetExtents(const CExtents& cExtents)
{
	m_cExtents = cExtents;
	m_vCenter = m_cExtents.GetCenter( );
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetDistanceFromCamera

ST_INLINE st_float32 CTerrainCell::GetDistanceFromCamera(void) const
{
	return m_fDistanceFromCamera;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetCenter

ST_INLINE const Vec3& CTerrainCell::GetCenter(void) const
{
	return m_vCenter;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetCullingRadius

ST_INLINE st_float32 CTerrainCell::GetCullingRadius(void) const
{
	return m_fCullRadius;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetFrameIndex

ST_INLINE st_int32 CTerrainCell::GetFrameIndex(void) const
{
	return m_nFrameIndex;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::SetFrameIndex

ST_INLINE void CTerrainCell::SetFrameIndex(st_int32 nFrameIndex)
{
	m_nFrameIndex = nFrameIndex;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetIndices

ST_INLINE void CTerrainCell::GetIndices(st_uint32& uiOffset, st_uint32& uiNumIndices, st_uint32& uiMinIndex, st_uint32& uiNumVertices) const
{
	uiOffset = m_uiIndicesOffset;
	uiNumIndices = m_uiNumIndices;
	uiMinIndex = m_uiMinIndex;
	uiNumVertices = m_uiNumVertices;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetLod

ST_INLINE st_int32 CTerrainCell::GetLod(void) const
{
	return m_nLod;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::GetVbo

ST_INLINE void* CTerrainCell::GetVbo(void) const
{
	return m_pVbo;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::SetVbo

ST_INLINE void CTerrainCell::SetVbo(void* pVbo)
{
	m_pVbo = pVbo;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::SetIsPopulated

ST_INLINE void CTerrainCell::SetIsPopulated(st_bool bVisibile) 
{ 
	m_bPopulated = bVisibile;
}


///////////////////////////////////////////////////////////////////////
//  CTerrainCell::IsPopulated

ST_INLINE st_bool CTerrainCell::IsPopulated(void) const
{ 
	return m_bPopulated; 
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::IsEnabled

ST_INLINE st_bool CTerrain::IsEnabled(void) const
{
	return !m_aMasterIndices.empty( );
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::GetNumLods

ST_INLINE st_int32 CTerrain::GetNumLods(void) const
{
	return m_nNumLods;
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::GetMaxTileRes

ST_INLINE st_int32 CTerrain::GetMaxTileRes(void) const
{
	return m_nMaxTileRes;
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::GetCellSize

ST_INLINE st_float32 CTerrain::GetCellSize(void) const
{
	return m_cTerrainCellMap.GetCellSize( );
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::SetHeightHints

ST_INLINE void CTerrain::SetHeightHints(st_float32 fGlobalLowPoint, st_float32 fGlobalHighPoint)
{
	m_fGlobalLowPoint = fGlobalLowPoint;
	m_fGlobalHighPoint = fGlobalHighPoint;
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::SetHeapReserves

ST_INLINE void CTerrain::SetHeapReserves(const SHeapReserves& sHeapReserves)
{
	m_sHeapReserves = sHeapReserves;

	m_cTerrainCellMap.ResizePool(m_sHeapReserves.m_nMaxVisibleTerrainCells);
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::SetLodRange

ST_INLINE void CTerrain::SetLodRange(st_float32 fNear, st_float32 fFar)
{
	m_fNearLodDistance = fNear;
	m_fFarLodDistance = fFar;
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::GetLodRange

ST_INLINE void CTerrain::GetLodRange(st_float32& fNear, st_float32& fFar) const
{
	fNear = m_fNearLodDistance;
	fFar = m_fFarLodDistance;
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::GetCompositeIndices

ST_INLINE const CArray<st_uint16>& CTerrain::GetCompositeIndices(void) const
{
	return m_aMasterIndices;
}

