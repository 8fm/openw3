///////////////////////////////////////////////////////////////////////  
//  Culling.inl
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
//  Function ComputeCellCoords

ST_INLINE void ComputeCellCoords(const Vec3& vPos, st_float32 fCellSize, st_int32& nRow, st_int32& nCol)
{
	assert(fCellSize > 0.0f);

	Vec3 vPos_cs = CCoordSys::ConvertToStd(vPos);

	nRow = (vPos_cs.y < 0.0f) ? st_int32((vPos_cs.y - fCellSize) / fCellSize) : st_int32(vPos_cs.y / fCellSize);
	nCol = (vPos_cs.x < 0.0f) ? st_int32((vPos_cs.x - fCellSize) / fCellSize) : st_int32(vPos_cs.x / fCellSize);
}


///////////////////////////////////////////////////////////////////////
//  Function: ExtractPlanes

ST_INLINE void ExtractPlanes(const Mat4x4& mCompositeView, Vec4 avFrustumPlanes[NUM_PLANES])
{
	// compute the fix planes defining the view frustum
	for (st_int32 i = 0; i < 4; ++i)
	{
		st_int32 nOffset = i * 4;
		avFrustumPlanes[RIGHT_PLANE][i]  = mCompositeView[3 + nOffset] - mCompositeView[0 + nOffset];
		avFrustumPlanes[LEFT_PLANE][i]   = mCompositeView[3 + nOffset] + mCompositeView[0 + nOffset];
		avFrustumPlanes[BOTTOM_PLANE][i] = mCompositeView[3 + nOffset] + mCompositeView[1 + nOffset];
		avFrustumPlanes[TOP_PLANE][i]    = mCompositeView[3 + nOffset] - mCompositeView[1 + nOffset];
		avFrustumPlanes[FAR_PLANE][i]    = mCompositeView[3 + nOffset] - mCompositeView[2 + nOffset];
		avFrustumPlanes[NEAR_PLANE][i]   = mCompositeView[3 + nOffset] + mCompositeView[2 + nOffset];
	}

	// normalize the plane normals
	for (st_int32 nPlane = NEAR_PLANE; nPlane < NUM_PLANES; ++nPlane)
	{
		st_float32 t = Vec3(avFrustumPlanes[nPlane]).Magnitude( );
		avFrustumPlanes[nPlane][0] /= t;
		avFrustumPlanes[nPlane][1] /= t;
		avFrustumPlanes[nPlane][2] /= t;
		avFrustumPlanes[nPlane][3] /= t;
	}
}


///////////////////////////////////////////////////////////////////////
//  Function: ComputeFrustumAABB

ST_INLINE void ComputeFrustumAABB(const Vec3 avFrustumPoints[8], CExtents& cFrustumExtents)
{
	// compute the frustum's AABB
	cFrustumExtents.Reset( );
	for (st_int32 i = 0; i < 8; ++i)
		cFrustumExtents.ExpandAround(avFrustumPoints[i]);
}


///////////////////////////////////////////////////////////////////////
//  Function: Compute3PlaneIntersection

ST_INLINE st_bool Compute3PlaneIntersection(const Vec4& vPlane1, const Vec4& vPlane2, const Vec4& vPlane3, Vec3& vIntersection)
{
	st_bool bSuccess = false;

	const Vec3 vN1(vPlane1);
	const Vec3 vN2(vPlane2);
	const Vec3 vN3(vPlane3);
	const st_float32 fD1 = vPlane1.w;
	const st_float32 fD2 = vPlane2.w;
	const st_float32 fD3 = vPlane3.w;

	const st_float32 fDenom = vN1.Dot(vN2.Cross(vN3));
	if (fabs(fDenom) > FLT_EPSILON)
	{
		const Vec3 vNumerator = vN2.Cross(vN3) * fD1 + vN3.Cross(vN1) * fD2 + vN1.Cross(vN2) * fD3;
		vIntersection = vNumerator * (-1.0f / fDenom);

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  Function: ComputeFrustumPoints

ST_INLINE void ComputeFrustumPoints(const CView& cView, Vec3 avFrustumPoints[8])
{
	Vec4 avFrustumPlanes[NUM_PLANES];
	ExtractPlanes(cView.GetComposite( ), avFrustumPlanes);

	// near plane points
	(void) Compute3PlaneIntersection(avFrustumPlanes[NEAR_PLANE], avFrustumPlanes[RIGHT_PLANE], avFrustumPlanes[TOP_PLANE], avFrustumPoints[0]);
	(void) Compute3PlaneIntersection(avFrustumPlanes[NEAR_PLANE], avFrustumPlanes[LEFT_PLANE], avFrustumPlanes[TOP_PLANE], avFrustumPoints[1]);
	(void) Compute3PlaneIntersection(avFrustumPlanes[NEAR_PLANE], avFrustumPlanes[LEFT_PLANE], avFrustumPlanes[BOTTOM_PLANE], avFrustumPoints[2]);
	(void) Compute3PlaneIntersection(avFrustumPlanes[NEAR_PLANE], avFrustumPlanes[RIGHT_PLANE], avFrustumPlanes[BOTTOM_PLANE], avFrustumPoints[3]);

	// far plane points
	(void) Compute3PlaneIntersection(avFrustumPlanes[FAR_PLANE], avFrustumPlanes[RIGHT_PLANE], avFrustumPlanes[TOP_PLANE], avFrustumPoints[4]);
	(void) Compute3PlaneIntersection(avFrustumPlanes[FAR_PLANE], avFrustumPlanes[LEFT_PLANE], avFrustumPlanes[TOP_PLANE], avFrustumPoints[5]);
	(void) Compute3PlaneIntersection(avFrustumPlanes[FAR_PLANE], avFrustumPlanes[LEFT_PLANE], avFrustumPlanes[BOTTOM_PLANE], avFrustumPoints[6]);
	(void) Compute3PlaneIntersection(avFrustumPlanes[FAR_PLANE], avFrustumPlanes[RIGHT_PLANE], avFrustumPlanes[BOTTOM_PLANE], avFrustumPoints[7]);
}


///////////////////////////////////////////////////////////////////////  
//  Function: FrustumCullsSphere

ST_INLINE st_bool FrustumCullsSphere(const Vec4 avFrustumPlanes[NUM_PLANES], const Vec3& vCenter, st_float32 fRadius)
{
	for (st_int32 nPlane = NEAR_PLANE; nPlane < NUM_PLANES; ++nPlane)
	{
		st_float32 fDistance = avFrustumPlanes[nPlane].x * vCenter.x + 
							   avFrustumPlanes[nPlane].y * vCenter.y +
							   avFrustumPlanes[nPlane].z * vCenter.z +
							   avFrustumPlanes[nPlane].w;

		if (fDistance < -fRadius)
			return true;
	}

	return false;
}


///////////////////////////////////////////////////////////////////////  
//  Function: FrustumCullsSphere_Detailed

ST_INLINE ECullStatus FrustumCullsSphere_Detailed(const Vec4 avFrustumPlanes[NUM_PLANES], const Vec3& vCenter, st_float32 fRadius)
{
	for (st_int32 nPlane = NEAR_PLANE; nPlane < NUM_PLANES; ++nPlane)
	{
		st_float32 fDistance = avFrustumPlanes[nPlane].x * vCenter.x + 
							   avFrustumPlanes[nPlane].y * vCenter.y +
							   avFrustumPlanes[nPlane].z * vCenter.z +
							   avFrustumPlanes[nPlane].w;

		if (fDistance < -fRadius)
			return CS_FULLY_OUTSIDE_FRUSTUM;
		else if (fabs(fDistance) < fRadius)
			return CS_INTERSECTS_FRUSTUM;
	}

	return CS_FULLY_INSIDE_FRUSTUM;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::CCell

ST_INLINE CCell::CCell( ) :
#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
	m_mBaseTreesToBillboardVboStreamsMap(0),
#endif
	m_nRow(c_nInvalidRowColIndex),
	m_nCol(c_nInvalidRowColIndex),
	m_nFrameIndex(-1),
	m_eCullStatus(CS_FULLY_OUTSIDE_FRUSTUM),
	m_fLodDistanceSquared(0.0f),
	m_fLongestBaseTreeLodDistanceSquared(0.0f)
{
	m_aTreeInstances.SetHeapDescription("CArray (SDK-side tree instance population)");
	m_aGrassInstances.SetHeapDescription("CArray (SDK-side grass instance population)");
}


///////////////////////////////////////////////////////////////////////  
//  CCell::~CCell

ST_INLINE CCell::~CCell( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CCell::SetRowCol

ST_INLINE void CCell::SetRowCol(st_int32 nRow, st_int32 nCol)
{
	m_nRow = nRow;
	m_nCol = nCol;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::Row

ST_INLINE st_int32 CCell::Row(void) const
{
	return m_nRow;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::Col

ST_INLINE st_int32 CCell::Col(void) const
{
	return m_nCol;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::UniqueRandomSeed

ST_INLINE st_int32 CCell::UniqueRandomSeed(void) const
{
	return m_nRow * 1237 + m_nCol;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::SetExtents

ST_INLINE void CCell::SetExtents(const CExtents& cExtents)
{
	m_cExtents = cExtents;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::GetExtents

ST_INLINE const CExtents& CCell::GetExtents(void) const
{
	return m_cExtents;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::GetCullStatus

ST_INLINE ECullStatus CCell::GetCullStatus(void) const
{
	return m_eCullStatus;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::GetLodDistanceSquared

ST_INLINE st_float32 CCell::GetLodDistanceSquared(void) const
{
	return m_fLodDistanceSquared;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::GetLongestBaseTreeLodDistanceSquared

ST_INLINE st_float32 CCell::GetLongestBaseTreeLodDistanceSquared(void) const
{
	return m_fLongestBaseTreeLodDistanceSquared;
}

///////////////////////////////////////////////////////////////////////  
//  Helper function: SearchInstancesFromClumpedBounds

inline st_bool SearchInstancesFromClumpedBounds(const CTreeInstance** pInstances, st_int32 nNumInstances, const CTree* pBaseTree, st_int32& nStart, st_int32& nEnd)
{
	assert(pInstances && pBaseTree);

	// init
	st_bool bFound = false;
	nStart = -1;
	nEnd = -1;

	// special search only makes sense when there are a lot of instances
	if (nNumInstances > 48)
	{
		const st_int32 c_nDivideAttempts = 5;
		st_int32 nNumTaps = 2;
		st_int32 nDivisor = 4;
		st_int32 nInterval = nNumInstances / nDivisor;

		for (st_int32 nPass = 0; nPass < c_nDivideAttempts && !bFound; ++nPass)
		{
			st_int32 nLocation = nNumInstances / nDivisor;
			for (st_int32 nTap = 0; nTap < nNumTaps; ++nTap)
			{
				if (pInstances[nLocation]->InstanceOf( ) == pBaseTree)
				{
					// search left to find start
					for (nStart = nLocation - 1; nStart > -1; --nStart)
					{
						if (pInstances[nStart]->InstanceOf( ) != pBaseTree)
						{
							++nStart;
							break;
						}
					}
					if (nStart == -1)
						nStart = 0;

					// search right to find end
					for (nEnd = nLocation + 1; nEnd < nNumInstances; ++nEnd)
					{
						if (pInstances[nEnd]->InstanceOf( ) != pBaseTree)
							break;
					}

					bFound = true;
					break;
				}

				nLocation += nInterval;
			}

			nDivisor *= 2;
			nInterval = nNumInstances / nDivisor;
			nNumTaps *= 2;
		}
	}

	// if not found, do a regular linear search
	if (!bFound)
	{
		// find start
		for (st_int32 i = 0; i < nNumInstances; ++i)
		{
			if (pInstances[i]->InstanceOf( ) == pBaseTree)
			{
				nStart = i;
				break;
			}
		}

		// if start found, continue on until base tree is no longer a match
		if (nStart > -1)
		{
			for (st_int32 i = nStart + 1; i < nNumInstances; ++i)
			{
				if (pInstances[i]->InstanceOf( ) != pBaseTree)
				{
					nEnd = i;
					break;
				}
			}

			if (nStart > -1 && nEnd == -1)
				nEnd = nNumInstances;
		}
	}

	return bFound;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::AppendTreeInstances

ST_INLINE void CCell::AppendTreeInstances(const CTree** pBaseTrees, st_int32 nNumBaseTrees, const CTreeInstance** pInstances, st_int32 nNumInstances)
{
    assert(pBaseTrees);
    assert(nNumBaseTrees > -1);
	assert(pInstances);
	assert(nNumInstances > -1);

	// help avoid any render-time allocations
	SPEEDTREE_HEAP_ALLOC_CHECK(TTreeInstConstPtrArray, m_aTreeInstances, SDK_LIMIT_MAX_TREE_INSTANCES_IN_ANY_CELL);

	// copy instance pointers as quickly as possible
	st_int32 nInsertionIndex = st_int32(m_aTreeInstances.size( ));
	m_aTreeInstances.resize(m_aTreeInstances.size( ) + nNumInstances);
	memcpy(&m_aTreeInstances[nInsertionIndex], pInstances, nNumInstances * sizeof(CTreeInstance*));

	// scan base trees for the longest base tree distance (will help with 3d LOD computation later)
	m_fLongestBaseTreeLodDistanceSquared = 0.0f;
	for (st_int32 i = 0; i < nNumBaseTrees; ++i)
	{
		const CTree* pBaseTree = pBaseTrees[i];
		const SLodProfile& sLodProfileSquared = pBaseTree->GetLodProfileSquared( );
		if (sLodProfileSquared.m_bLodIsPresent)
		{
			m_fLongestBaseTreeLodDistanceSquared = st_max(m_fLongestBaseTreeLodDistanceSquared, sLodProfileSquared.m_fBillboardFinalDistance);
		}
		else
		{
			m_fLongestBaseTreeLodDistanceSquared = FLT_MAX;
		}
	}

	#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
		m_mBaseTreesToBillboardVboStreamsMap.clear( );
		for (st_int32 i = 0; i < nNumBaseTrees; ++i)
		{
			// base tree
			const CTree* pBaseTree = pBaseTrees[i];

			// determine which instances from pInstances refer to this base tree
			st_int32 nStart = -1, nEnd = -1;
			SearchInstancesFromClumpedBounds(pInstances, nNumInstances, pBaseTree, nStart, nEnd);

			if (nStart > -1 && nEnd > 0)
			{
				// copy into array
				CArray<SBillboardInstanceVertex>& aVboStream = m_mBaseTreesToBillboardVboStreamsMap[pBaseTree];
				aVboStream.resize(nEnd - nStart);

				SBillboardInstanceVertex* pVertex = &aVboStream[0];
				for (st_int32 j = nStart; j < nEnd; ++j)
				{
					const CTreeInstance* pInstance = pInstances[j];

					pVertex->m_vPos = pInstance->GetPos( );
					pVertex->m_fScalar = pInstance->GetScalar( );
					pVertex->m_vUpVector = pInstance->GetUpVector( );
					pVertex->m_vRightVector = pInstance->GetRightVector( );

					++pVertex;
				}
			}
		}
	#endif
}


///////////////////////////////////////////////////////////////////////  
//  CCell::GetTreeInstances

ST_INLINE const TTreeInstConstPtrArray& CCell::GetTreeInstances(void) const
{
    return m_aTreeInstances;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::GetGrassInstances

ST_INLINE const TGrassInstArray& CCell::GetGrassInstances(void) const
{
	return m_aGrassInstances;
}


///////////////////////////////////////////////////////////////////////  
//  CCell::SetGrassInstances

ST_INLINE void CCell::SetGrassInstances(const SGrassInstance* pInstances, st_int32 nNumInstances)
{
	assert(pInstances);
	assert(nNumInstances > -1);

	if (nNumInstances > 0)
	{
		// help avoid any render-time allocations
		SPEEDTREE_HEAP_ALLOC_CHECK(TGrassInstArray, m_aGrassInstances, SDK_LIMIT_MAX_PER_BASE_GRASS_INSTANCES_IN_ANY_CELL);

		// copy instance pointers as quickly as possible
		m_aGrassInstances.resize(nNumInstances);
		size_t siNumInstancesToBeCopied = size_t(nNumInstances);
		if (siNumInstancesToBeCopied > m_aGrassInstances.size( ))
		{
			siNumInstancesToBeCopied = m_aGrassInstances.size( );
			CCore::SetError("CCell::SetGrassInstances() -- number of grass instances in cell exceeded reserve value; adjust call to SetHeapReserves()");
		}
		memcpy(static_cast<void*>(&m_aGrassInstances[0]), static_cast<const void*>(pInstances), siNumInstancesToBeCopied * sizeof(SGrassInstance));
	}
	else
		m_aGrassInstances.clear( );
}


///////////////////////////////////////////////////////////////////////  
//  CCell::operator<

ST_INLINE st_bool CCell::operator<(const CCell& cIn) const
{
	return (m_nRow == cIn.m_nRow) ? (m_nCol < cIn.m_nCol) : (m_nRow < cIn.m_nRow);
}


///////////////////////////////////////////////////////////////////////  
//  CCell::operator!=

ST_INLINE st_bool CCell::operator!=(const CCell& cIn) const
{
	return (m_nRow != cIn.m_nRow || m_nCol != cIn.m_nCol);
}


///////////////////////////////////////////////////////////////////////  
//  SDetailedCullData::SDetailedCullData

ST_INLINE SDetailedCullData::SDetailedCullData( ) :
	m_pBaseTree(nullptr),
	m_fClosest3dTreeDistanceSquared(FLT_MAX),
	m_fClosestBillboardCellDistanceSquared(FLT_MAX)
{
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::CVisibleInstances

ST_INLINE CVisibleInstances::CVisibleInstances(EPopulationType ePopulationType, st_bool bTrackNearestInsts) :
	m_ePopulationType(ePopulationType),
	m_bTrackNearestInsts(bTrackNearestInsts),
	m_cCellHeapMgr(ePopulationType),
	m_fCellSize(1200.0f),
	m_nFrustumExtentsStartRow(c_nInvalidRowColIndex),
	m_nFrustumExtentsStartCol(c_nInvalidRowColIndex),
	m_nFrustumExtentsEndRow(c_nInvalidRowColIndex),
	m_nFrustumExtentsEndCol(c_nInvalidRowColIndex)
{
	for (st_int32 i = 0; i < 8; ++i)
		for (st_int32 j = 0; j < 2; ++j)
			m_anLastFrustumPointCells[i][j] = c_nInvalidRowColIndex;

	m_aPerBase3dInstances.SetHeapDescription("CVisibleInstances::m_aPerBase3dInstances");
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::~CVisibleInstances

ST_INLINE CVisibleInstances::~CVisibleInstances( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::SetHeapReserves

ST_INLINE void CVisibleInstances::SetHeapReserves(const SHeapReserves& sHeapReserves)
{
	ScopeTrace("CVisibleInstances::SetHeapReserves");

	const st_int32 c_nMaxNumVisibleCells = (m_ePopulationType == POPULATION_TREES) ? sHeapReserves.m_nMaxVisibleTreeCells : sHeapReserves.m_nMaxVisibleGrassCells;
	const st_int32 c_nMaxNumInstancesInAnyCell = (m_ePopulationType == POPULATION_TREES) ? sHeapReserves.m_nMaxTreeInstancesInAnyCell : sHeapReserves.m_nMaxPerBaseGrassInstancesInAnyCell;

	m_aRoughCullCells.SetHeapDescription("CVisibleInstances::CArray");
	m_aRoughCullCells.reserve(c_nMaxNumVisibleCells * 2); // rough cull needs larger capacity since only fine cull will result
														  // in the visible cells; rough cull is a superset of cells

	m_mVisibleCells.SetHeapDescription("CVisibleInstances::CMap");
	m_aNewlyVisibleCells.SetHeapDescription("CVisibleInstances::CArray");
	m_aNewlyVisibleCells.reserve(c_nMaxNumVisibleCells);

	if (m_ePopulationType == POPULATION_TREES)
	{
		m_aPerBase3dInstances.SetHeapDescription("CVisibleInstances::CArray");
		m_aPerBase3dInstances.resize(sHeapReserves.m_nMaxBaseTrees);
		for (st_int32 i = 0; i < sHeapReserves.m_nMaxBaseTrees; ++i)
		{
			m_aPerBase3dInstances[i].m_a3dInstanceLods.SetHeapDescription("CVisibleInstances::CArray");
			m_aPerBase3dInstances[i].m_a3dInstanceLods.reserve(c_nMaxNumInstancesInAnyCell);
		}
	}

	{
		ScopeTrace("m_cCellHeapMgr.Init"); // todo (all these)
		m_cCellHeapMgr.Init(c_nMaxNumVisibleCells, c_nMaxNumInstancesInAnyCell);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::SetCellSize

ST_INLINE void CVisibleInstances::SetCellSize(st_float32 fCellSize)
{
	m_fCellSize = fCellSize;
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::Clear

ST_INLINE void CVisibleInstances::Clear(void)
{
	m_aPerBase3dInstances.clear( );
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::InstanceIsVisible

ST_INLINE st_bool CVisibleInstances::InstanceIsVisible(const CView& cView, const CTreeInstance& cInstance)
{
	return !FrustumCullsSphere(cView.GetFrustumPlanes( ), cInstance.GetGeometricCenter( ), cInstance.GetCullingRadius( ));
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::RoughCells

ST_INLINE TCellArray& CVisibleInstances::RoughCells(void)
{
	return m_aRoughCullCells;
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::VisibleCells

ST_INLINE const TRowColCellPtrMap& CVisibleInstances::VisibleCells(void) const
{
	return m_mVisibleCells;
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::NewlyVisibleCells

ST_INLINE TCellPtrArray& CVisibleInstances::NewlyVisibleCells(void)
{
	return m_aNewlyVisibleCells;
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::GetExtentsAsRowCols

ST_INLINE void CVisibleInstances::GetExtentsAsRowCols(st_int32& nStartRow, st_int32& nStartCol, st_int32& nEndRow, st_int32& nEndCol) const
{
	nStartRow = m_nFrustumExtentsStartRow;
	nStartCol = m_nFrustumExtentsStartCol;
	nEndRow = m_nFrustumExtentsEndRow;
	nEndCol = m_nFrustumExtentsEndCol;
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::NotifyOfPopulationChange

ST_INLINE void CVisibleInstances::NotifyOfFrustumReset(void)
{
	m_nFrustumExtentsStartRow = c_nInvalidRowColIndex;
	m_nFrustumExtentsStartCol = c_nInvalidRowColIndex;
	m_nFrustumExtentsEndRow = c_nInvalidRowColIndex;
	m_nFrustumExtentsEndCol = c_nInvalidRowColIndex;

	for (st_int32 i = 0; i < 8; ++i)
		for (st_int32 j = 0; j < 2; ++j)
			m_anLastFrustumPointCells[i][j] = c_nInvalidRowColIndex;
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::NotifyOfPopulationChange

ST_INLINE void CVisibleInstances::NotifyOfPopulationChange(void)
{
	for (TRowColCellPtrMap::const_iterator i = m_mVisibleCells.begin( ); i != m_mVisibleCells.end( ); ++i)
		m_cCellHeapMgr.CheckIn(i->second);
	m_mVisibleCells.clear( ); // todo: really don't want to deallocate here, rather empty & hold on to the heap buffer

	NotifyOfFrustumReset();
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::NotifyOfPopulationRemoval

ST_INLINE void CVisibleInstances::NotifyOfPopulationRemoval( st_int32 rowCol[4] )
{
	for ( st_int32 row = rowCol[0]; row <= rowCol[1]; ++row )
	{
		for ( st_int32 col = rowCol[2]; col <= rowCol[3]; ++col)
		{
			SRowCol key(row, col);
			TRowColCellPtrMap::iterator it = m_mVisibleCells.find(key);
			if( it != m_mVisibleCells.end() )
			{
				m_cCellHeapMgr.CheckIn( it->second );
				m_mVisibleCells.erase( it );
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::NotifyOfPopulationAddition

ST_INLINE void CVisibleInstances::NotifyOfPopulationAddition( st_int32 row, st_int32 col )
{
	SRowCol key(row, col);
	TRowColCellPtrMap::iterator it = m_mVisibleCells.find(key);
	if( it != m_mVisibleCells.end() )
	{
		m_cCellHeapMgr.CheckIn( it->second );
		m_mVisibleCells.erase( it );
	}
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::Get3dInstanceLods

ST_INLINE const CArray<SDetailedCullData>& CVisibleInstances::Get3dInstanceLods(void) const
{
	return m_aPerBase3dInstances;
}


///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::FindInstanceLodArray

// todo: remove entire function
ST_INLINE T3dTreeInstanceLodArray* CVisibleInstances::FindInstanceLodArray(const CTree* pBaseTree)
{
	T3dTreeInstanceLodArray* p3dInstanceList = nullptr;

	for (size_t i = 0; i < m_aPerBase3dInstances.size( ); ++i)
	{
		if (m_aPerBase3dInstances[i].m_pBaseTree == pBaseTree)
		{
			p3dInstanceList = &m_aPerBase3dInstances[i].m_a3dInstanceLods;
			break;
		}
	}

	// if a 3d instance list can't be found, most likely it's because m_aPerBase3dInstances didn't receive a large enough
	// reserve value; CVisibleInstances::SetReserves() accepts a number of reserve values, including 
	// CVisibleInstances::SReserves::m_nMaxBaseTrees, which directly affects m_aPerBase3dInstances's initial size
	if (!p3dInstanceList)
	{
		// help avoid any render-time allocations
		SPEEDTREE_HEAP_ALLOC_CHECK(CArray<SDetailedCullData>, m_aPerBase3dInstances, SDK_LIMIT_MAX_BASE_TREES);

		m_aPerBase3dInstances.resize(m_aPerBase3dInstances.size( ) + 1);
		m_aPerBase3dInstances.back( ).m_pBaseTree = pBaseTree;
		p3dInstanceList = &m_aPerBase3dInstances.back( ).m_a3dInstanceLods;
		assert(p3dInstanceList);
	}

	return p3dInstanceList;
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstances::GetInstaceLodArrayByBase

ST_INLINE SDetailedCullData* CVisibleInstances::GetInstaceLodArrayByBase(const CTree* pBaseTree)
{
	SDetailedCullData* pReturn = nullptr;

	for (size_t i = 0; i < m_aPerBase3dInstances.size( ); ++i)
	{
		if (m_aPerBase3dInstances[i].m_pBaseTree == pBaseTree)
		{
			pReturn = &m_aPerBase3dInstances[i];
			break;
		}
	}

	// if a 3d instance list can't be found, most likely it's because m_aPerBase3dInstances didn't receive a large enough
	// reserve value; CVisibleInstances::SetReserves() accepts a number of reserve values, including 
	// CVisibleInstances::SReserves::m_nMaxBaseTrees, which directly affects m_aPerBase3dInstances's initial size
	if (!pReturn)
	{
		// help avoid any render-time allocations
		SPEEDTREE_HEAP_ALLOC_CHECK(CArray<SDetailedCullData>, m_aPerBase3dInstances, SDK_LIMIT_MAX_BASE_TREES);

		m_aPerBase3dInstances.resize(m_aPerBase3dInstances.size( ) + 1);
		m_aPerBase3dInstances.back( ).m_pBaseTree = pBaseTree;
		pReturn = &m_aPerBase3dInstances.back( );
	}

	return pReturn;
}


