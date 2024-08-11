///////////////////////////////////////////////////////////////////////
//  Culling.cpp
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

#include "Forest/Forest.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  Helper function: FindCellByRowCol

ST_INLINE CCell* FindCellByRowCol(TRowColCellPtrMap& mMap, const SRowCol& sRowColKey)
{
	CCell* pCell = NULL;

	TRowColCellPtrMap::const_iterator iFind = mMap.find(sRowColKey);
	if (iFind != mMap.end( ))
		pCell = iFind->second;

	return pCell;
}


///////////////////////////////////////////////////////////////////////  
//  Helper function: ComputeFrustumExtents

void ComputeFrustumExtents(CExtents& cFrustumExtents, const CView& cView, st_float32 fLargestInstanceCullRadius)
{
	// query the frustum's AABB and expand it based on the largest instance in the forest
	cFrustumExtents = cView.GetFrustumExtents( );
	SpeedTree::Vec3 cLargestRadius = SpeedTree::CCoordSys::ConvertFromStd(fLargestInstanceCullRadius, fLargestInstanceCullRadius, 0.0f);
	const SpeedTree::Vec3& cMin = cFrustumExtents.Min( );
	const SpeedTree::Vec3& cMax = cFrustumExtents.Max( );
	cFrustumExtents.ExpandAround(cMin + cLargestRadius);
	cFrustumExtents.ExpandAround(cMin - cLargestRadius);
	cFrustumExtents.ExpandAround(cMax + cLargestRadius);
	cFrustumExtents.ExpandAround(cMax - cLargestRadius);
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::RoughCullCells

st_bool CVisibleInstances::RoughCullCells(const CView& cView, st_int32 nFrameIndex, st_float32 fLargestCellOverhang)
{
	st_bool bChangedSinceLastCall = false;

	// find the cells where the frustum's 8 points land; if no change, no need to continue
	const Vec3* pFrustumPoints = cView.GetFrustumPoints( );
	assert(pFrustumPoints);

	st_int32 anCurrentFrustumPointCells[c_nNumFrustumPoints][2];
	for (st_int32 i = 0; i < c_nNumFrustumPoints; ++i)
		ComputeCellCoords(pFrustumPoints[i], m_fCellSize, anCurrentFrustumPointCells[i][0], anCurrentFrustumPointCells[i][1]);

	if (memcmp(anCurrentFrustumPointCells, m_anLastFrustumPointCells, sizeof(anCurrentFrustumPointCells)) != 0)
	{
		bChangedSinceLastCall = true;
		memcpy(m_anLastFrustumPointCells, anCurrentFrustumPointCells, sizeof(anCurrentFrustumPointCells));
	}

	if (bChangedSinceLastCall)
	{
		// compute the frustum extents as cell coords; it's tempting to just use the min/max values of the
		// frustum point cells, but we need to account for overhang (that is, a model is placed on the edge of a cell
		// and may hang over into an adjacent cell considerably)

		// query the frustum's AABB and expand it based on the largest instance in the forest
		CExtents cFrustumExtents;
		ComputeFrustumExtents(cFrustumExtents, cView, fLargestCellOverhang);

		// find the cells that make up the corners of the frustum's orthographic projection
		ComputeCellCoords(cFrustumExtents.Min( ), m_fCellSize, m_nFrustumExtentsStartRow, m_nFrustumExtentsStartCol);
		ComputeCellCoords(cFrustumExtents.Max( ), m_fCellSize, m_nFrustumExtentsEndRow, m_nFrustumExtentsEndCol);

		// it's possible that (start < end) given how the coordinate conversion system is setup
		if (m_nFrustumExtentsEndRow < m_nFrustumExtentsStartRow)
			Swap<st_int32>(m_nFrustumExtentsStartRow, m_nFrustumExtentsEndRow);
		if (m_nFrustumExtentsEndCol < m_nFrustumExtentsStartCol)
			Swap<st_int32>(m_nFrustumExtentsStartCol, m_nFrustumExtentsEndCol);

		m_aRoughCullCells.resize((m_nFrustumExtentsEndRow - m_nFrustumExtentsStartRow + 1) * (m_nFrustumExtentsEndCol - m_nFrustumExtentsStartCol + 1));
		if (!m_aRoughCullCells.empty( ))
		{
			CCell* pRoughCell = &m_aRoughCullCells[0];
			for (st_int32 nRow = m_nFrustumExtentsStartRow; nRow <= m_nFrustumExtentsEndRow; ++nRow)
			{
				for (st_int32 nCol = m_nFrustumExtentsStartCol; nCol <= m_nFrustumExtentsEndCol; ++nCol)
				{
					pRoughCell->SetRowCol(nRow, nCol);
					pRoughCell->m_nFrameIndex = nFrameIndex;

					++pRoughCell;
				}
			}
		}
	}

	return bChangedSinceLastCall;
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::FineCullTreeCells

void CVisibleInstances::FineCullTreeCells(const CView& cView, st_int32 nFrameIndex)
{
	m_aNewlyVisibleCells.resize(0);

	// query culling data
	const Vec4* c_pFrustumPlanes = cView.GetFrustumPlanes( );
	const st_int32 c_nNumRoughCells = st_int32(m_aRoughCullCells.size( ));

	for (st_int32 nCell = 0; nCell < c_nNumRoughCells; ++nCell)
	{
		CCell& cRoughCell = m_aRoughCullCells[nCell];

		if (cRoughCell.GetExtents( ).Valid( ))
		{
			// cell culling parameters
			const st_int32 c_nRow = cRoughCell.Row( );
			const st_int32 c_nCol = cRoughCell.Col( );
			const SRowCol c_sRowCol(c_nRow, c_nCol);
			const Vec3 c_vCellCenter = cRoughCell.GetExtents( ).GetCenter( );
			const st_float32 c_fCellCullRadius = cRoughCell.GetExtents( ).ComputeRadiusFromCenter3D( );

			// determine if this cell is in the frustum
			ECullStatus eCullStatus = FrustumCullsSphere_Detailed(c_pFrustumPlanes, c_vCellCenter, c_fCellCullRadius);
			if (eCullStatus != CS_FULLY_OUTSIDE_FRUSTUM)
			{
				// check to see if the visible cell was already visible on the last frame so that we don't recreate it
				CCell* pVisibleCell = FindCellByRowCol(m_mVisibleCells, c_sRowCol);
				if (pVisibleCell)
				{
					// cell already exists, so just update its frame stamp
					pVisibleCell->m_nFrameIndex = nFrameIndex;
				}
				else
				{
					// obtain new cell and copy rough cell attributes into it
					pVisibleCell = m_cCellHeapMgr.CheckOut( );
					pVisibleCell->SetRowCol(c_nRow, c_nCol);
					pVisibleCell->m_nFrameIndex = nFrameIndex;

					m_mVisibleCells[c_sRowCol] = pVisibleCell;

					SPEEDTREE_HEAP_ALLOC_CHECK(TCellPtrArray, m_aNewlyVisibleCells, (m_ePopulationType == POPULATION_TREES) ? SDK_LIMIT_MAX_VISIBLE_TREE_CELLS : SDK_LIMIT_MAX_VISIBLE_GRASS_CELLS);
					{
						m_aNewlyVisibleCells.push_back(pVisibleCell);
					}
				}

				// compute cell's distance from LOD ref point
				assert(pVisibleCell);
				pVisibleCell->m_eCullStatus = eCullStatus;
				pVisibleCell->m_cExtents = cRoughCell.GetExtents( );

				pVisibleCell->m_fLodDistanceSquared = c_vCellCenter.Distance(cView.GetLodRefPoint( ));
				if (c_fCellCullRadius > pVisibleCell->m_fLodDistanceSquared)
					pVisibleCell->m_fLodDistanceSquared = 0.0f;
				else
				{
					pVisibleCell->m_fLodDistanceSquared -= c_fCellCullRadius;
					pVisibleCell->m_fLodDistanceSquared *= pVisibleCell->m_fLodDistanceSquared;
				}
			}
		}
	}

	// remove cells from cVisibleInstances.m_aVisibleCells that didn't get a fresh frame stamp (since they're no longer visible)
	for (TRowColCellPtrMap::iterator iCell = m_mVisibleCells.begin( ); iCell != m_mVisibleCells.end( ); )
	{
		if (iCell->second->m_nFrameIndex != nFrameIndex)
		{
			m_cCellHeapMgr.CheckIn(iCell->second);
			iCell = m_mVisibleCells.erase(iCell);
		}
		else
			++iCell;
	}
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::FineCullGrassCells

void CVisibleInstances::FineCullGrassCells(const CView& cView, st_int32 nFrameIndex, st_float32 fLargestCellOverhang)
{
	m_aNewlyVisibleCells.resize(0);

	// query culling data
	const Vec4* c_pFrustumPlanes = cView.GetFrustumPlanes( );
	const st_int32 c_nNumRoughCells = st_int32(m_aRoughCullCells.size( ));

	if (c_nNumRoughCells > 0)
	{
		st_int32 nFirstRowCellIndex = 0;
		st_int32 nRow = m_aRoughCullCells[nFirstRowCellIndex].Row( );

		while (nFirstRowCellIndex < c_nNumRoughCells)
		{
			// look for last cell with this same row value
			st_int32 nLastRowCellIndex = nFirstRowCellIndex + 1;
			while (nLastRowCellIndex < c_nNumRoughCells && m_aRoughCullCells[nLastRowCellIndex].Row( ) == nRow)
				++nLastRowCellIndex;

			// start from first entry, continue until first non-culled cell
			const CCell* pFirstRoughCellInRow = NULL;
			int nStartCellCol = c_nInvalidRowColIndex, nEndCellCol = c_nInvalidRowColIndex;
			for (st_int32 nCellIndex = nFirstRowCellIndex; nCellIndex < nLastRowCellIndex; ++nCellIndex)
			{
				CCell& cRoughCell = m_aRoughCullCells[nCellIndex];

				// determine if this cell is in the frustum
				if (!FrustumCullsSphere(c_pFrustumPlanes, 
									    cRoughCell.GetExtents( ).GetCenter( ),
									    cRoughCell.GetExtents( ).ComputeRadiusFromCenter3D( ) + fLargestCellOverhang))
				{
					nStartCellCol = cRoughCell.Col( );
					pFirstRoughCellInRow = &cRoughCell;
					break;
				}
			}

			if (nStartCellCol != c_nInvalidRowColIndex)
			{
				for (st_int32 nCellIndex = nLastRowCellIndex - 1; nCellIndex >= nFirstRowCellIndex; --nCellIndex)
				{
					CCell& cRoughCell = m_aRoughCullCells[nCellIndex];

					// determine if this cell is in the frustum
					if (!FrustumCullsSphere(c_pFrustumPlanes, 
										    cRoughCell.GetExtents( ).GetCenter( ),
										    cRoughCell.GetExtents( ).ComputeRadiusFromCenter3D( ) + fLargestCellOverhang))
					{
						nEndCellCol = cRoughCell.Col( );
						break;
					}
				}
				assert(nEndCellCol >= nStartCellCol);

				if (nEndCellCol != c_nInvalidRowColIndex)
				{
					for (st_int32 nCol = nStartCellCol; nCol <= nEndCellCol; ++nCol)
					{
						const SRowCol c_sRowCol(nRow, nCol);

						// check to see if the visible cell was already visible on the last frame so that we don't recreate it
						CCell* pVisibleCell = FindCellByRowCol(m_mVisibleCells, c_sRowCol);
						if (pVisibleCell)
						{
							// cell already exists, so just update its frame stamp
							pVisibleCell->m_nFrameIndex = nFrameIndex;
						}
						else
						{
							// obtain new cell and copy rough cell attributes into it
							pVisibleCell = m_cCellHeapMgr.CheckOut( );
							pVisibleCell->SetRowCol(nRow, nCol);
							pVisibleCell->m_nFrameIndex = nFrameIndex;

							m_mVisibleCells[c_sRowCol] = pVisibleCell;

							SPEEDTREE_HEAP_ALLOC_CHECK(TCellPtrArray, m_aNewlyVisibleCells, (m_ePopulationType == POPULATION_TREES) ? SDK_LIMIT_MAX_VISIBLE_TREE_CELLS : SDK_LIMIT_MAX_VISIBLE_GRASS_CELLS);
							{
								m_aNewlyVisibleCells.push_back(pVisibleCell);
							}
						}

						assert(pFirstRoughCellInRow);
						pVisibleCell->m_cExtents = pFirstRoughCellInRow++->GetExtents( );
					}
				}
			}

			++nRow;
			nFirstRowCellIndex = nLastRowCellIndex;
		}
	}

	// remove cells from cVisibleInstances.m_aVisibleCells that didn't get a fresh frame stamp (since they're no longer visible)
	for (TRowColCellPtrMap::iterator iCell = m_mVisibleCells.begin( ); iCell != m_mVisibleCells.end( ); )
	{
		if (iCell->second->m_nFrameIndex != nFrameIndex)
		{
			m_cCellHeapMgr.CheckIn(iCell->second);
			iCell = m_mVisibleCells.erase(iCell);
		}
		else
			++iCell;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::Update3dTreeLists

void CVisibleInstances::Update3dTreeLists(const CView& cView)
{
	// clear any 3d instances from last frame
	for (size_t i = 0; i < m_aPerBase3dInstances.size( ); ++i)
		m_aPerBase3dInstances[i].m_a3dInstanceLods.resize(0);

	if (!m_bTrackNearestInsts)
	{
		// init distances
		for (size_t i = 0; i < m_aPerBase3dInstances.size( ); ++i)
		{
			m_aPerBase3dInstances[i].m_fClosest3dTreeDistanceSquared = FLT_MAX;
			m_aPerBase3dInstances[i].m_fClosestBillboardCellDistanceSquared = FLT_MAX;
		}
	}

	for (TRowColCellPtrMap::iterator i = m_mVisibleCells.begin( ); i != m_mVisibleCells.end( ); ++i)
	{
		CCell* pCell = i->second;
		assert(pCell);

		// only proceed if the cell holds at least one 3D tree that's in a 3D LOD state
		if (pCell->GetLodDistanceSquared( ) <= pCell->GetLongestBaseTreeLodDistanceSquared( ))
		{
			const CTree* pBaseTree = NULL;
			SDetailedCullData* pInstanceLodsByBase = NULL;
			st_bool bCellBaseTreeIn3dRange = false;

            const TTreeInstConstPtrArray& aTreeInstances = pCell->GetTreeInstances( );
			const size_t c_siNumInstances = aTreeInstances.size( );
			for (size_t siInstance = 0; siInstance < c_siNumInstances; ++siInstance)
			{
				const CTreeInstance* pInstance = aTreeInstances[siInstance];
                st_assert(pInstance->InstanceOf( ),  "Every instance should know which base tree it's an instance of");
				if (pInstance->InstanceOf( ) != pBaseTree)
				{
					pBaseTree = pInstance->InstanceOf( );
					const SLodProfile& sLodProfileSquared = pBaseTree->GetLodProfileSquared( );

					bCellBaseTreeIn3dRange = sLodProfileSquared.m_bLodIsPresent ? (pCell->GetLodDistanceSquared( ) <= sLodProfileSquared.m_fBillboardFinalDistance) : true;

					pInstanceLodsByBase = GetInstaceLodArrayByBase(pBaseTree);
					assert(pInstanceLodsByBase);
				}

				if (bCellBaseTreeIn3dRange)
				{
                    ScopeTrace("Computing 3D LOD Params");

					const st_float32 c_fDistanceSquared = pInstance->GetPos( ).DistanceSquared(cView.GetLodRefPoint( ));
					const SLodProfile& sLodProfileSquared = pBaseTree->GetLodProfileSquared( );

					if (!sLodProfileSquared.m_bLodIsPresent || c_fDistanceSquared < sLodProfileSquared.m_fBillboardFinalDistance)
					{
						st_bool bSkipVisibilityTest = (pCell->m_eCullStatus == CS_FULLY_INSIDE_FRUSTUM);

						if ( bSkipVisibilityTest || InstanceIsVisible(cView, *pInstance) )
						{
							const SLodProfile& sLodProfile = pBaseTree->GetLodProfile( );

							SPEEDTREE_HEAP_ALLOC_CHECK(T3dTreeInstanceLodArray, pInstanceLodsByBase->m_a3dInstanceLods, SDK_LIMIT_MAX_TREE_INSTANCES_IN_ANY_CELL);

							pInstanceLodsByBase->m_a3dInstanceLods.resize(pInstanceLodsByBase->m_a3dInstanceLods.size( ) + 1);
							S3dTreeInstanceLod& sInstanceLod = pInstanceLodsByBase->m_a3dInstanceLods.back( );
							
							sInstanceLod.m_pInstance = pInstance;

						    sInstanceLod.m_fDistanceFromCameraSquared = c_fDistanceSquared;
							sInstanceLod.m_fLod = sLodProfile.m_bLodIsPresent ? pBaseTree->ComputeLodByDistanceSquared(sInstanceLod.m_fDistanceFromCameraSquared) : 1.0f;
						    sInstanceLod.m_nLodLevel = pBaseTree->ComputeLodSnapshot(sInstanceLod.m_fLod);

						    assert(pBaseTree->GetGeometry( ));
						    sInstanceLod.m_fLodTransition = CCore::ComputeLodTransition(sInstanceLod.m_fLod, pBaseTree->GetGeometry( )->m_nNumLods);

							// is this instance the closest of its type to the LOD ref point?
							pInstanceLodsByBase->m_fClosest3dTreeDistanceSquared = st_min(pInstanceLodsByBase->m_fClosest3dTreeDistanceSquared, c_fDistanceSquared);
						}
					}
				}
				else
				{
					// this cell holds only billboards of this tree type; is it the closest billboard cell to the LOD ref point?
					pInstanceLodsByBase->m_fClosestBillboardCellDistanceSquared = st_min(pInstanceLodsByBase->m_fClosestBillboardCellDistanceSquared, pCell->GetLodDistanceSquared( ));
				}
			}
		}
		else
		{
			if (m_bTrackNearestInsts)
			{
#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
				for (size_t j = 0; j < m_aPerBase3dInstances.size( ); ++j)
				{
					// does this base tree have instances in this cell?
					CMap<const CTree*, CArray<SBillboardInstanceVertex> >::const_iterator iFind = pCell->m_mBaseTreesToBillboardVboStreamsMap.find(m_aPerBase3dInstances[j].m_pBaseTree);
					if (iFind != pCell->m_mBaseTreesToBillboardVboStreamsMap.end( ))
						m_aPerBase3dInstances[j].m_fClosestBillboardCellDistanceSquared = st_min(m_aPerBase3dInstances[j].m_fClosestBillboardCellDistanceSquared, pCell->GetLodDistanceSquared( ));
				}
#endif
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::Update3dTreeLists

void CVisibleInstances::Update3dTreeLists(const CView& cView, VisibilityHelper* visibilityHelper )
{
	// clear any 3d instances from last frame
	for (size_t i = 0; i < m_aPerBase3dInstances.size( ); ++i)
		m_aPerBase3dInstances[i].m_a3dInstanceLods.resize(0);

	if (!m_bTrackNearestInsts)
	{
		// init distances
		for (size_t i = 0; i < m_aPerBase3dInstances.size( ); ++i)
		{
			m_aPerBase3dInstances[i].m_fClosest3dTreeDistanceSquared = FLT_MAX;
			m_aPerBase3dInstances[i].m_fClosestBillboardCellDistanceSquared = FLT_MAX;
		}
	}

	for (TRowColCellPtrMap::iterator i = m_mVisibleCells.begin( ); i != m_mVisibleCells.end( ); ++i)
	{
		CCell* pCell = i->second;
		assert(pCell);

		// only proceed if the cell holds at least one 3D tree that's in a 3D LOD state
		if (pCell->GetLodDistanceSquared( ) <= pCell->GetLongestBaseTreeLodDistanceSquared( ))
		{
			const CTree* pBaseTree = NULL;
			SDetailedCullData* pInstanceLodsByBase = NULL;
			st_bool bCellBaseTreeIn3dRange = false;
			const st_bool bNeedsPerTreeVisibilityTest = pCell->m_eCullStatus != CS_FULLY_INSIDE_FRUSTUM && visibilityHelper->NeedsPerTreeVisTest();

			const TTreeInstConstPtrArray& aTreeInstances = pCell->GetTreeInstances( );
			const size_t c_siNumInstances = aTreeInstances.size( );
			for (size_t siInstance = 0; siInstance < c_siNumInstances; ++siInstance)
			{
				const CTreeInstance* pInstance = aTreeInstances[siInstance];
				st_assert(pInstance->InstanceOf( ),  "Every instance should know which base tree it's an instance of");
				if (pInstance->InstanceOf( ) != pBaseTree)
				{
					pBaseTree = pInstance->InstanceOf( );
					const SLodProfile& sLodProfileSquared = pBaseTree->GetLodProfileSquared( );

					bCellBaseTreeIn3dRange = sLodProfileSquared.m_bLodIsPresent ? (pCell->GetLodDistanceSquared( ) <= sLodProfileSquared.m_fBillboardFinalDistance) : true;

					pInstanceLodsByBase = GetInstaceLodArrayByBase(pBaseTree);
					assert(pInstanceLodsByBase);
				}

				if (bCellBaseTreeIn3dRange)
				{
					ScopeTrace("Computing 3D LOD Params");

					const st_float32 c_fDistanceSquared = pInstance->GetPos( ).DistanceSquared(cView.GetLodRefPoint( ));
					const SLodProfile& sLodProfileSquared = pBaseTree->GetLodProfileSquared( );

					const st_float32 instanceHeight = pInstance->GetScalar() * pBaseTree->GetExtents().GetHeight();					

					if ( !sLodProfileSquared.m_bLodIsPresent || c_fDistanceSquared < sLodProfileSquared.m_fBillboardFinalDistance )
					{
						st_float32 lodRatio;
						const st_bool isTreeVisible = !bNeedsPerTreeVisibilityTest || visibilityHelper->IsTreeVisible( pInstance->GetGeometricCenter(), pInstance->GetCullingRadius() );
						if ( isTreeVisible && visibilityHelper->CalculateLodRatio( pInstance->GetGeometricCenter(), instanceHeight, *pBaseTree, c_fDistanceSquared, lodRatio ) )
						{
							const SLodProfile& sLodProfile = pBaseTree->GetLodProfile( );

							SPEEDTREE_HEAP_ALLOC_CHECK(T3dTreeInstanceLodArray, pInstanceLodsByBase->m_a3dInstanceLods, SDK_LIMIT_MAX_TREE_INSTANCES_IN_ANY_CELL);

							pInstanceLodsByBase->m_a3dInstanceLods.resize(pInstanceLodsByBase->m_a3dInstanceLods.size( ) + 1);
							S3dTreeInstanceLod& sInstanceLod = pInstanceLodsByBase->m_a3dInstanceLods.back( );

							sInstanceLod.m_pInstance = pInstance;

							sInstanceLod.m_fDistanceFromCameraSquared = c_fDistanceSquared;						
							
							sInstanceLod.m_fLod = lodRatio;

							sInstanceLod.m_nLodLevel = pBaseTree->ComputeLodSnapshot(sInstanceLod.m_fLod);

							assert(pBaseTree->GetGeometry( ));
							sInstanceLod.m_fLodTransition = CCore::ComputeLodTransition(sInstanceLod.m_fLod, pBaseTree->GetGeometry( )->m_nNumLods);

							// is this instance the closest of its type to the LOD ref point?
							pInstanceLodsByBase->m_fClosest3dTreeDistanceSquared = st_min(pInstanceLodsByBase->m_fClosest3dTreeDistanceSquared, c_fDistanceSquared);
						}
					}
				}
				else
				{
					// this cell holds only billboards of this tree type; is it the closest billboard cell to the LOD ref point?
					pInstanceLodsByBase->m_fClosestBillboardCellDistanceSquared = st_min(pInstanceLodsByBase->m_fClosestBillboardCellDistanceSquared, pCell->GetLodDistanceSquared( ));
				}
			}
		}
		else
		{
			if (m_bTrackNearestInsts)
			{
#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
				for (size_t j = 0; j < m_aPerBase3dInstances.size( ); ++j)
				{
					// does this base tree have instances in this cell?
					CMap<const CTree*, CArray<SBillboardInstanceVertex> >::const_iterator iFind = pCell->m_mBaseTreesToBillboardVboStreamsMap.find(m_aPerBase3dInstances[j].m_pBaseTree);
					if (iFind != pCell->m_mBaseTreesToBillboardVboStreamsMap.end( ))
						m_aPerBase3dInstances[j].m_fClosestBillboardCellDistanceSquared = st_min(m_aPerBase3dInstances[j].m_fClosestBillboardCellDistanceSquared, pCell->GetLodDistanceSquared( ));
				}
#endif
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////
// LAVA++
void CVisibleInstances::FineCullTreeCells( const CView& cView, st_int32 nFrameIndex, VisibilityHelper* visibilityHelper )
{
	m_aNewlyVisibleCells.resize(0);

	// query culling data
	const st_int32 c_nNumRoughCells = st_int32(m_aRoughCullCells.size() );
	for (st_int32 nCell = 0; nCell < c_nNumRoughCells; ++nCell)
	{
		CCell& cRoughCell = m_aRoughCullCells[nCell];

		if ( cRoughCell.GetExtents().Valid() )
		{
			// cell culling parameters
			const st_int32 c_nRow = cRoughCell.Row();
			const st_int32 c_nCol = cRoughCell.Col();
			const SRowCol c_sRowCol(c_nRow, c_nCol);
			const Vec3 c_vCellCenter = cRoughCell.GetExtents( ).GetCenter( );
			const st_float32 c_fCellCullRadius = cRoughCell.GetExtents( ).ComputeRadiusFromCenter3D( );

			// determine if this cell is in the frustum
			VisibilityHelper::VisibilityTestResult visibilityRes = visibilityHelper->IsVisible( cRoughCell.GetExtents() );
			if ( visibilityRes != VisibilityHelper::VisibilityTestResult::OCCLUDED )
			{
				// check to see if the visible cell was already visible on the last frame so that we don't recreate it
				CCell* pVisibleCell = FindCellByRowCol(m_mVisibleCells, c_sRowCol);
				if (pVisibleCell)
				{
					// cell already exists, so just update its frame stamp
					pVisibleCell->m_nFrameIndex = nFrameIndex;
				}
				else
				{
					// obtain new cell and copy rough cell attributes into it
					pVisibleCell = m_cCellHeapMgr.CheckOut( );
					pVisibleCell->SetRowCol(c_nRow, c_nCol);
					pVisibleCell->m_nFrameIndex = nFrameIndex;

					m_mVisibleCells[c_sRowCol] = pVisibleCell;

					SPEEDTREE_HEAP_ALLOC_CHECK(TCellPtrArray, m_aNewlyVisibleCells, (m_ePopulationType == POPULATION_TREES) ? SDK_LIMIT_MAX_VISIBLE_TREE_CELLS : SDK_LIMIT_MAX_VISIBLE_GRASS_CELLS);
					{
						m_aNewlyVisibleCells.push_back(pVisibleCell);
					}
				}

				// compute cell's distance from LOD ref point
				assert(pVisibleCell);
				pVisibleCell->m_eCullStatus = visibilityRes == VisibilityHelper::VisibilityTestResult::FULLY_VISIBLE ? CS_FULLY_INSIDE_FRUSTUM : CS_INTERSECTS_FRUSTUM; // todo: map 1-1 so it can be faster
				pVisibleCell->m_cExtents = cRoughCell.GetExtents( );

				pVisibleCell->m_fLodDistanceSquared = c_vCellCenter.Distance(cView.GetLodRefPoint( ));
				if (c_fCellCullRadius > pVisibleCell->m_fLodDistanceSquared)
					pVisibleCell->m_fLodDistanceSquared = 0.0f;
				else
				{
					pVisibleCell->m_fLodDistanceSquared -= c_fCellCullRadius;
					pVisibleCell->m_fLodDistanceSquared *= pVisibleCell->m_fLodDistanceSquared;
				}
			}
		}
	}

	// remove cells from cVisibleInstances.m_aVisibleCells that didn't get a fresh frame stamp (since they're no longer visible)
	for (TRowColCellPtrMap::iterator iCell = m_mVisibleCells.begin( ); iCell != m_mVisibleCells.end( ); )
	{
		if (iCell->second->m_nFrameIndex != nFrameIndex)
		{
			m_cCellHeapMgr.CheckIn(iCell->second);
			iCell = m_mVisibleCells.erase(iCell);
		}
		else
			++iCell;
	}
}

// LAVA--
