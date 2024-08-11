///////////////////////////////////////////////////////////////////////
//  Terrain.cpp
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

#include "Forest/Terrain.h"
#include "Core/Memory.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  CTerrain::CTerrain

CTerrain::CTerrain( ) :
	m_nNumLods(5),
	m_nMaxTileRes(33),
	m_fNearLodDistance(0.0f),
	m_fFarLodDistance(8000.0f),
	m_fGlobalLowPoint(-100.0f),
	m_fGlobalHighPoint(100.0f)
{
	m_aMasterIndices.SetHeapDescription("CTerrain::m_aMasterIndices");
	m_aPerCellIndexData.SetHeapDescription("CTerrain::m_aPerCellIndexData");
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::~CTerrain

CTerrain::~CTerrain( )
{
	#ifndef NDEBUG
		m_nNumLods = -1;
		m_nMaxTileRes = -1;
		m_fNearLodDistance = -1.0f;
		m_fFarLodDistance = -1.0f;
		m_fGlobalLowPoint = -1.0f;
		m_fGlobalHighPoint = -1.0f;
	#endif
}


///////////////////////////////////////////////////////////////////////
//  Function: IsPowerOf2

inline bool IsPowerOf2(st_int32 nValue)
{
	return nValue > 0 && (nValue & (nValue - 1)) == 0;
}


///////////////////////////////////////////////////////////////////////
//  STerrainCullResults::SetHeapReserves

st_bool STerrainCullResults::SetHeapReserves(const SHeapReserves& sHeapReserves)
{
	m_aCellsToUpdate.SetHeapDescription("STerrainCullResults::CArray");
	st_bool bSuccess = m_aCellsToUpdate.reserve(sHeapReserves.m_nMaxVisibleTerrainCells);

	m_aFreedVbos.SetHeapDescription("STerrainCullResults::CArray");
	bSuccess &= m_aFreedVbos.reserve(sHeapReserves.m_nMaxVisibleTerrainCells);

	m_aVisibleCells.SetHeapDescription("STerrainCullResults::CArray");
	bSuccess &= m_aVisibleCells.reserve(sHeapReserves.m_nMaxVisibleTerrainCells);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::InitGeometry

st_bool CTerrain::InitGeometry(st_int32 nNumLods, st_int32 nMaxTileRes, st_float32 fCellSize)
{
	st_bool bSuccess = false;

	if (CCore::IsAuthorized( ))
	{
		// expect reasonable values
		if (nNumLods > 0 && nMaxTileRes >= 5 && IsPowerOf2(nMaxTileRes - 1))
		{
			m_nNumLods = nNumLods;
			m_nMaxTileRes = nMaxTileRes;

			m_cTerrainCellMap.SetCellSize(fCellSize);

			// clear everything before beginning in case the system was initialized prior
			m_cTerrainCellMap.clear( );
			m_aMasterIndices.clear( );
			m_aPerCellIndexData.clear( );

			InitLodIndexStrips( );

			bSuccess = true;
		}
		else
			CCore::SetError("CTerrain::Init failed, bad parameter values; max tile res should be (power of 2) + 1");
	}
	else
		CCore::SetError("Terrain system failed to initialize, SDK has not been authorized; expired eval key?");

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::CullAndComputeLOD

void CTerrain::CullAndComputeLOD(const CView& cView, st_int32 nFrameIndex, STerrainCullResults& sResults)
{
	assert(IsEnabled( ));

	// help avoid any render-time allocations
	SPEEDTREE_HEAP_ALLOC_CHECK(TTerrainCellMap, m_cTerrainCellMap, SDK_LIMIT_MAX_VISIBLE_TERRAIN_CELLS);

	// query frustum data
	const Vec4* pFrustumPlanes = cView.GetFrustumPlanes( );
	const CExtents& cFrustumExtents = cView.GetFrustumExtents( );

	// find the cells that make up the corners of the frustum's orthographic projection
	st_int32 nStartRow = 0, nStartCol = 0, nEndRow = 0, nEndCol = 0;
	ComputeCellCoords(cFrustumExtents.Min( ), m_cTerrainCellMap.GetCellSize( ), nStartRow, nStartCol);
	ComputeCellCoords(cFrustumExtents.Max( ), m_cTerrainCellMap.GetCellSize( ), nEndRow, nEndCol);

	// it's possible that (start < end) given how the coordinate conversion system is setup
	if (nEndRow < nStartRow)
		Swap<st_int32>(nStartRow, nEndRow);
	if (nEndCol < nStartCol)
		Swap<st_int32>(nStartCol, nEndCol);

	const st_int32 c_nNumCols = (nEndCol - nStartCol) + 1;
	const st_float32 c_fCellSize = m_cTerrainCellMap.GetCellSize( );
	const st_float32 c_fAABBSize = c_fCellSize * c_nNumCols;

	const Vec3 vCellMin = CCoordSys::RightAxis( ) * st_float32(nStartCol) * c_fCellSize +
						  CCoordSys::OutAxis( ) * st_float32(nStartRow) * c_fCellSize +
						  CCoordSys::UpAxis( ) * m_fGlobalLowPoint;
	const Vec3 vCellMax = CCoordSys::RightAxis( ) * st_float32(nStartCol + 1) * c_fCellSize +
						  CCoordSys::OutAxis( ) * st_float32(nStartRow + 1) * c_fCellSize +
						  CCoordSys::UpAxis( ) * m_fGlobalHighPoint;
	CExtents cCellExtents(vCellMin, vCellMax);

	Vec3 vCellCenter = cCellExtents.GetCenter( );
	const st_float32 c_fCullRadius = cCellExtents.ComputeRadiusFromCenter3D( );

	st_int32 nTargetCellCount = 0;
	for (st_int32 nRow = nStartRow; nRow <= nEndRow; ++nRow)
	{
		for (st_int32 nCol = nStartCol; nCol <= nEndCol; ++nCol)
		{
			// determine if this cell is visible before constructing it
			if (!FrustumCullsSphere(pFrustumPlanes, vCellCenter, c_fCullRadius))
			{
				++nTargetCellCount;

				CTerrainCell* pCell = m_cTerrainCellMap.GetCellPtrByRowCol_Add(nRow, nCol);

				// cell may be null if cell limit was reached
				if (pCell)
				{
					// update the cell as active
					pCell->SetFrameIndex(nFrameIndex);
				}
			}

			// move x value of extents along
			vCellCenter += CCoordSys::RightAxis( ) * c_fCellSize;
		}

		// reset x value of extents
		vCellCenter = vCellCenter - CCoordSys::RightAxis( ) * c_fAABBSize;

		// move y value of extents along
		vCellCenter += CCoordSys::OutAxis( ) * c_fCellSize;
	}

	// process the remaining cells
	sResults.m_aVisibleCells.resize(0);
	sResults.m_aCellsToUpdate.resize(0);
	for (TTerrainCellMap::iterator iMap = m_cTerrainCellMap.begin( ); iMap != m_cTerrainCellMap.end( ); ++iMap)
	{
		CTerrainCell* pCell = &iMap->second;
		const st_int32 nRow = pCell->Row( );
		const st_int32 nCol = pCell->Col( );

		if (pCell->IsNew( ))
		{
			const Vec3 vNewCellMin = CCoordSys::RightAxis( ) * st_float32(nCol) * c_fCellSize +
								     CCoordSys::OutAxis( ) * st_float32(nRow) * c_fCellSize +
								     CCoordSys::UpAxis( ) * m_fGlobalLowPoint;
			const Vec3 vNewCellMax = CCoordSys::RightAxis( ) * st_float32(nCol + 1) * c_fCellSize +
								     CCoordSys::OutAxis( ) * st_float32(nRow + 1) * c_fCellSize +
								     CCoordSys::UpAxis( ) * m_fGlobalHighPoint;

			pCell->SetExtents(CExtents(vNewCellMin, vNewCellMax));
			assert(pCell->GetExtents( ).Valid( ));

			sResults.m_aCellsToUpdate.push_back(pCell);
		}

		// record all cells in the map as visible
		sResults.m_aVisibleCells.push_back(pCell);
	}

	// cell LODs can't be computed in the loop above since the cell meshes drawn
	// depend on their neighbors' LOD values, too
	ComputeCellLods(cView, sResults);

	// sanity test; one is tracking internal cell data, the other cell data for the user
	assert(m_cTerrainCellMap.size( ) == sResults.m_aVisibleCells.size( ));
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::FrameEnd

void CTerrain::FrameEnd(st_int32 nFrameIndex, STerrainCullResults& sCullResults)
{
	RemoveInactiveCells(sCullResults.m_aFreedVbos, nFrameIndex);
}


///////////////////////////////////////////////////////////////////////
//  Function: CTerrainCell comparison functor

class CTerrainCellSorter
{
public:
	bool operator( )(const CTerrainCell* pLeft, const CTerrainCell* pRight) const
	{
		return (pLeft->GetDistanceFromCamera( ) < pRight->GetDistanceFromCamera( ));
	}
};


///////////////////////////////////////////////////////////////////////
//  CTerrain::ComputeCellLods

void CTerrain::ComputeCellLods(const CView& cView, STerrainCullResults& sResults) const
{
	assert(IsEnabled( ));

	// compute raw LOD values for each cell
	const st_int32 c_nNumCells = st_int32(sResults.m_aVisibleCells.size( ));
	st_int32 nCell = 0;
	for (nCell = 0; nCell < c_nNumCells; ++nCell)
	{
		CTerrainCell* pCell = sResults.m_aVisibleCells[nCell];

		pCell->m_fDistanceFromCamera = pCell->GetCenter( ).Distance(cView.GetCameraPos( ));
		st_float32 fLod = Clamp((pCell->m_fDistanceFromCamera - m_fNearLodDistance) / (m_fFarLodDistance - m_fNearLodDistance), 0.0f, 1.0f);
		pCell->m_nLod = int((m_nNumLods - 1) * fLod);
	}

	// examine each cell's adjacent cells to determine which terrain tile to render
	for (nCell = 0; nCell < c_nNumCells; ++nCell)
	{
		CTerrainCell* pCell = sResults.m_aVisibleCells[nCell];
		ETerrainCellTransition eCellTransitionType = TERRAIN_CELL_TRANSITION_NONE;
		if (pCell->m_nLod > 0)
		{
			const CTerrainCell* pTop = m_cTerrainCellMap.GetCellPtrByRowCol(pCell->Row( ) - 1, pCell->Col( ));
			const st_bool c_bTopTrans = (pTop && pTop->m_nLod < pCell->m_nLod);

			const CTerrainCell* pBottom = m_cTerrainCellMap.GetCellPtrByRowCol(pCell->Row( ) + 1, pCell->Col( ));
			const st_bool c_bBottomTrans = (pBottom && pBottom->m_nLod < pCell->m_nLod);

			const CTerrainCell* pLeft = m_cTerrainCellMap.GetCellPtrByRowCol(pCell->Row( ), pCell->Col( ) - 1);
			const st_bool c_bLeftTrans = (pLeft && pLeft->m_nLod < pCell->m_nLod);

			const CTerrainCell* pRight = m_cTerrainCellMap.GetCellPtrByRowCol(pCell->Row( ), pCell->Col( ) + 1);
			const st_bool c_bRightTrans = (pRight && pRight->m_nLod < pCell->m_nLod);

			// top & bottom and left & right should never be in transition at the same time
			assert(!(c_bTopTrans && c_bBottomTrans));
			assert(!(c_bLeftTrans && c_bRightTrans));

			if (c_bTopTrans)
			{
				if (c_bRightTrans)
					eCellTransitionType = TERRAIN_CELL_TRANSITION_TOP_RIGHT;
				else if (c_bLeftTrans)
					eCellTransitionType = TERRAIN_CELL_TRANSITION_TOP_LEFT;
				else
					eCellTransitionType = TERRAIN_CELL_TRANSITION_TOP;
			}
			else if (c_bBottomTrans)
			{
				if (c_bRightTrans)
					eCellTransitionType = TERRAIN_CELL_TRANSITION_BOTTOM_RIGHT;
				else if (c_bLeftTrans)
					eCellTransitionType = TERRAIN_CELL_TRANSITION_BOTTOM_LEFT;
				else
					eCellTransitionType = TERRAIN_CELL_TRANSITION_BOTTOM;
			}
			else if (c_bLeftTrans)
				eCellTransitionType = TERRAIN_CELL_TRANSITION_LEFT;
			else if (c_bRightTrans)
				eCellTransitionType = TERRAIN_CELL_TRANSITION_RIGHT;
		}

		const SIndexData& sIndexData = pCell->m_nLod == 0 ? m_aPerCellIndexData[0] : m_aPerCellIndexData[1 + (pCell->m_nLod - 1) * TERRAIN_CELL_TRANSITION_COUNT + eCellTransitionType];

		pCell->m_uiIndicesOffset = sIndexData.m_uiOffset;
		pCell->m_uiNumIndices = sIndexData.m_uiNumIndices;
		pCell->m_uiMinIndex = sIndexData.m_uiMinIndex;
		pCell->m_uiNumVertices = sIndexData.m_uiNumVertices;
	}

	// sort front to back
	sResults.m_aVisibleCells.sort(CTerrainCellSorter( ));
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::RemoveInactiveCells

void CTerrain::RemoveInactiveCells(TTerrainVboArray& aFreedVbos, st_int32 nFrameIndex)
{
	assert(IsEnabled( ));

	aFreedVbos.resize(0);
	for (TTerrainCellMap::iterator i = m_cTerrainCellMap.begin( ); i != m_cTerrainCellMap.end( ); )
	{
		CTerrainCell& cCell = i->second;

		if (cCell.GetFrameIndex( ) != nFrameIndex)
		{
			aFreedVbos.push_back(i->second.GetVbo( ));
			i = m_cTerrainCellMap.erase(i);
		}
		else
			++i;
	}
}


///////////////////////////////////////////////////////////////////////
//  CTerrain::InitLodIndexStrips

#define INDEX(col, row) m_aMasterIndices.push_back(st_uint16((col) * m_nMaxTileRes + (row)))
#define TOP_LEFT		INDEX(nCol,					nRow)
#define BOTTOM_LEFT		INDEX(nCol,					nRow + nIncrement)
#define BOTTOM_RIGHT	INDEX(nCol + nIncrement,	nRow + nIncrement)
#define TOP_RIGHT		INDEX(nCol + nIncrement,	nRow)
#define TOP_MID			INDEX(nCol + c_nHalfInc,	nRow)
#define BOTTOM_MID		INDEX(nCol + c_nHalfInc,	nRow + nIncrement)
#define LEFT_MID		INDEX(nCol,					nRow + c_nHalfInc)
#define RIGHT_MID		INDEX(nCol + nIncrement,	nRow + c_nHalfInc)

void CTerrain::InitLodIndexStrips(void)
{
	// the entire terrain system will use one index buffer and the cells will access
	// different parts based on LOD and transitions to neighboring cells

	m_aMasterIndices.reserve(100000); // amount needed for a terrain res of 65 in the reference app example; modify as need
	// only one cell type for highest LOD, so adjust accordingly
	m_aPerCellIndexData.reserve(TERRAIN_CELL_TRANSITION_COUNT * (m_nNumLods - 1) + 1);

	// create entries for all LOD levels
	st_int32 nIncrement = 1;
	for (st_int32 nLod = 0; nLod < m_nNumLods; ++nLod)
	{
		for (st_int32 nCellType = 0; nCellType < TERRAIN_CELL_TRANSITION_COUNT; ++nCellType)
		{
			SIndexData sIndexData;

			sIndexData.m_uiNumIndices = 0;
			sIndexData.m_uiOffset = st_uint32(m_aMasterIndices.size( ));

			for (st_int32 nRow = 0; nRow < m_nMaxTileRes - 1; nRow += nIncrement)
			{
				for (st_int32 nCol = 0; nCol < m_nMaxTileRes - 1; nCol += nIncrement)
				{
					const st_bool c_bLeftEdge   = (nLod > 0) && nCol == 0;
					const st_bool c_bRightEdge  = (nLod > 0) && nCol == m_nMaxTileRes - nIncrement - 1;
					const st_bool c_bTopEdge    = (nLod > 0) && nRow == 0;
					const st_bool c_bBottomEdge = (nLod > 0) && nRow == m_nMaxTileRes - nIncrement - 1;

					size_t siBaseIndexSize = m_aMasterIndices.size( );
					const st_int32 c_nHalfInc = nIncrement / 2;

					if (c_bLeftEdge && c_bTopEdge && nCellType == TERRAIN_CELL_TRANSITION_TOP_LEFT)
					{
						// +---+---+ (really hard to draw in ascii, top_mid and left_mid points are
						// |  / \  |  are both connected to bottom right)
						// | /     |
						// |/    \ |
						// +       |
						// |\    \ |
						// |  \    |
						// |    \ \|
						// +-------+

						TOP_LEFT;	TOP_MID;		LEFT_MID;		// top left
						TOP_MID;	BOTTOM_RIGHT;	LEFT_MID;		// middle
						LEFT_MID;	BOTTOM_RIGHT;	BOTTOM_LEFT;	// bottom
						TOP_RIGHT;	BOTTOM_RIGHT;	TOP_MID;		// right
					}
					else if (c_bLeftEdge && c_bBottomEdge && nCellType == TERRAIN_CELL_TRANSITION_BOTTOM_LEFT)
					{
						// +-------+ (really hard to draw in ascii, bottom_mid and left_mid points are
						// |    / /|  are both connected to top right)
						// |  /    |
						// |/    / |
						// +       |
						// |\    / |
						// | \     |
						// |  \ /   |
						// +---+---+

						TOP_LEFT;	TOP_RIGHT;		LEFT_MID;		// top left
						TOP_RIGHT;	BOTTOM_MID;		LEFT_MID;		// middle
						LEFT_MID;	BOTTOM_MID;		BOTTOM_LEFT;	// bottom left
						TOP_RIGHT;	BOTTOM_RIGHT;	BOTTOM_MID;		// right
					}
					else if (c_bRightEdge && c_bTopEdge && nCellType == TERRAIN_CELL_TRANSITION_TOP_RIGHT)
					{
						// +---+---+ (really hard to draw in ascii, top_mid and right_mid points are
						// |  / \  |  are both connected to bottom left)
						// |     \ |
						// | /    \|
						// |       +
						// | /    /|
						// |/ / /  |
						// |/      |
						// +-------+

						TOP_MID;	BOTTOM_LEFT;	TOP_LEFT;		// top left
						TOP_MID;	RIGHT_MID;		BOTTOM_LEFT;	// middle
						RIGHT_MID;	BOTTOM_RIGHT;	BOTTOM_LEFT;	// bottom
						TOP_RIGHT;	RIGHT_MID;		TOP_MID;		// right
					}
					else if (c_bRightEdge && c_bBottomEdge && nCellType == TERRAIN_CELL_TRANSITION_BOTTOM_RIGHT)
					{
						// +-------+ (really hard to draw in ascii, bottom_mid and right_mid points are
						// |\      |  are both connected to top left)
						// |\ \ \  |
						// | \    \|
						// |       +
						// | \    /|
						// |     / |
						// |  \ /  |
						// +---+---+

						TOP_RIGHT;	RIGHT_MID;		TOP_LEFT;		// top right
						TOP_LEFT;	RIGHT_MID;		BOTTOM_MID;		// middle
						TOP_LEFT;	BOTTOM_MID;		BOTTOM_LEFT;	// bottom
						RIGHT_MID;	BOTTOM_RIGHT;	BOTTOM_MID;		// bottom right
					}
					else if (c_bLeftEdge && (nCellType == TERRAIN_CELL_TRANSITION_LEFT ||
											 nCellType == TERRAIN_CELL_TRANSITION_TOP_LEFT ||
											 nCellType == TERRAIN_CELL_TRANSITION_BOTTOM_LEFT))
					{
						// +---+ (this should be square, but it's the same size as the rest, just split vertically)
						// |  /|
						// | / |
						// |/  |
						// +   |
						// |\  |
						// | \ |
						// |  \|
						// +---+
						
						LEFT_MID;	BOTTOM_RIGHT;	BOTTOM_LEFT;	// bottom
						TOP_RIGHT;	BOTTOM_RIGHT;	LEFT_MID;		// middle/right
						TOP_RIGHT;	LEFT_MID;		TOP_LEFT;		// top
					}
					else if (c_bRightEdge && (nCellType == TERRAIN_CELL_TRANSITION_RIGHT ||
											  nCellType == TERRAIN_CELL_TRANSITION_TOP_RIGHT ||
											  nCellType == TERRAIN_CELL_TRANSITION_BOTTOM_RIGHT))
					{
						// +---+ (this should be square, but it's the same size as the rest, just split vertically)
						// |\  |
						// | \ |
						// |  \|
						// |   +
						// |  /|
						// | / |
						// |/  |
						// +---+

						RIGHT_MID;	BOTTOM_RIGHT;	BOTTOM_LEFT;	// bottom
						TOP_LEFT;	RIGHT_MID;		BOTTOM_LEFT;	// middle/left
						TOP_LEFT;	TOP_RIGHT;		RIGHT_MID;		// top
					}
					else if (c_bTopEdge && (nCellType == TERRAIN_CELL_TRANSITION_TOP ||
											nCellType == TERRAIN_CELL_TRANSITION_TOP_RIGHT ||
											nCellType == TERRAIN_CELL_TRANSITION_TOP_LEFT))
					{
						// +---+---+
						// |  / \  |
						// | /   \ |
						// |/     \|
						// +-------+

						TOP_LEFT;	TOP_MID;		BOTTOM_LEFT;	// left
						TOP_MID;	BOTTOM_RIGHT;	BOTTOM_LEFT;	// middle
						TOP_RIGHT;	BOTTOM_RIGHT;	TOP_MID;		// right
					}
					else if (c_bBottomEdge && (nCellType == TERRAIN_CELL_TRANSITION_BOTTOM ||
											   nCellType == TERRAIN_CELL_TRANSITION_BOTTOM_RIGHT ||
											   nCellType == TERRAIN_CELL_TRANSITION_BOTTOM_LEFT))
					{
						// +-------+
						// |\     /|
						// | \   / |
						// |  \ /  |
						// +---+---+

						TOP_LEFT;	BOTTOM_MID;		BOTTOM_LEFT;	// left
						TOP_LEFT;	TOP_RIGHT;		BOTTOM_MID;		// middle
						TOP_RIGHT;	BOTTOM_RIGHT;	BOTTOM_MID;		// right
					}
					else
					{
						// +---+
						// |  /|
						// | / |
						// |/  |
						// +---+

						TOP_LEFT;	TOP_RIGHT;		BOTTOM_LEFT;	// left
						TOP_RIGHT;	BOTTOM_RIGHT;	BOTTOM_LEFT;	// right
					}

					sIndexData.m_uiNumIndices +=  st_uint32(m_aMasterIndices.size( ) - siBaseIndexSize);
				}
			}

			// compute min index & num vertices
			assert(m_aMasterIndices.size( ) > sIndexData.m_uiOffset);
			st_uint16 uiMinIndex = m_aMasterIndices[sIndexData.m_uiOffset];
			st_uint16 uiMaxIndex = 0;
			for (st_uint32 i = 0; i < sIndexData.m_uiNumIndices; ++i)
			{
				uiMinIndex = st_min(uiMinIndex, m_aMasterIndices[sIndexData.m_uiOffset + i]);
				uiMaxIndex = st_max(uiMaxIndex, m_aMasterIndices[sIndexData.m_uiOffset + i]);
			}

			sIndexData.m_uiMinIndex = uiMinIndex;
			sIndexData.m_uiNumVertices = uiMaxIndex - uiMinIndex + 1;

			m_aPerCellIndexData.push_back(sIndexData);

			// the first LOD never uses transition cells since the lower LOD cells always transition
			// up to the higher LOD
			if (nLod == 0)
				break;
		}

		nIncrement *= 2;
	}
}

#undef INDEX
