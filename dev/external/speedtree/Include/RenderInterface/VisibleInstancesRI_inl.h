///////////////////////////////////////////////////////////////////////  
//  VisibleInstancesRI.inl
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
//  SForestCullResultsRI_t::SForestInstancingData::SForestInstancingData

CVisibleInstancesRI_TemplateList
inline CVisibleInstancesRI_t::SForestInstancingData::SForestInstancingData( ) :
	m_pBaseTree(NULL)
{
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::CVisibleInstancesRI

CVisibleInstancesRI_TemplateList
inline CVisibleInstancesRI_t::CVisibleInstancesRI(EPopulationType ePopulationType, st_bool bTrackNearestInsts/*LAVA++*/, Uint32 instanceRBSize) :
	CVisibleInstances(ePopulationType, bTrackNearestInsts),
	m_mPerBaseInstancingDataMap(30)
{
	// LAVA++
	m_instancingRingBuffer.SetRingBufferSize( instanceRBSize );
	// LAVA--
	m_mPerBaseInstancingDataMap.SetHeapDescription("CVisibleInstancesRI::m_mPerBaseInstancingDataMap");
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::~CVisibleInstancesRI

CVisibleInstancesRI_TemplateList
inline CVisibleInstancesRI_t::~CVisibleInstancesRI( )
{
	for (typename TInstanceDataPtrMap::iterator i = m_mPerBaseInstancingDataMap.begin( ); i != m_mPerBaseInstancingDataMap.end( ); ++i)
		st_delete<SForestInstancingData>(i->second);
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::SetHeapReserves

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::SetHeapReserves(const SHeapReserves& sHeapReserves)
{
	// base-class specific
	{
		ScopeTrace("CVisibleInstances::SetHeapReserves"); // todo: remove
		CVisibleInstances::SetHeapReserves(sHeapReserves);
	}
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::InitGfx

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::InitGfx(const CArray<CTreeRI_t*>& aBaseTrees)
{
	st_bool bSuccess = true;

	for (size_t i = 0; i < aBaseTrees.size( ); ++i)
	{
		CTreeRI_t* pBaseTree = aBaseTrees[i];

		// find the VB associate with this base tree
		typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
		assert(pInstancingData);

		// initialize instance manager if not already
		if (!pInstancingData->m_t3dTreeInstancingMgr.IsInitialized( ))
		{
			// get base tree geometry
			assert(pBaseTree->GetGeometry( ));
			const st_int32 c_nNumLods = pBaseTree->GetGeometry( )->m_nNumLods;

			bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.Init3dTrees(c_nNumLods, pBaseTree->GetGeometryBuffers( ));
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::InitGfx

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::Clear(void)
{
	m_mPerBaseInstancingDataMap.clear( );
	CVisibleInstances::Clear( );
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::ReleaseGfxResources

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::ReleaseGfxResources(void)
{
	for (typename TInstanceDataPtrMap::iterator i = m_mPerBaseInstancingDataMap.begin( ); i != m_mPerBaseInstancingDataMap.end( ); ++i)
	{
		assert(i->second);
		i->second->m_t3dTreeInstancingMgr.ReleaseGfxResources( );
		i->second->m_tBillboardInstancingMgr.ReleaseGfxResources( );
	}

	// LAVA++
	m_instancingRingBuffer.ReleaseBuffer();
	// LAVA--
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::UpdateGrassInstanceBuffers

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::UpdateGrassInstanceBuffers(const CTreeRI_t* pBaseGrass/*LAVA++*/, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer)
{
	assert(pBaseGrass);

	st_bool bSuccess = true;

	// find the instance data associated with this base grass object
	typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseGrass);
	assert(pInstancingData);

	// initialize the instance manager if not already
	if (!pInstancingData->m_t3dTreeInstancingMgr.IsInitialized( ))
	{
		assert(!pBaseGrass->GetGeometryBuffers( ).empty( ));
		bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.InitGrass(pBaseGrass->GetGeometryBuffers( ));
	}

	bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.UpdateGrassInstanceBuffers(VisibleCells( )/*LAVA++*/, frustumPlanes, cullingRadius, instanceRingBuffer);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::UpdateBillboardInstanceBuffers

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::UpdateBillboardInstanceBuffers(SBillboardVboBuffer& sBuffer, const CTreeRI_t* pBaseTree)
{
	assert(pBaseTree);

	st_bool bSuccess = true;

	// find the instance data associated with this base tree object
	typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
	assert(pInstancingData);

	// initialize the instance manager if not already
	if (!pInstancingData->m_tBillboardInstancingMgr.IsInitialized( ))
		bSuccess &= pInstancingData->m_tBillboardInstancingMgr.InitBillboards(&pBaseTree->GetBillboardGeometryBuffer( ));

	bSuccess &= pInstancingData->m_tBillboardInstancingMgr.UpdateBillboardInstanceBuffers(sBuffer, pBaseTree, VisibleCells( ));

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::GetBaseTreeBillboardVboData

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::GetBaseTreeBillboardVboData(SBillboardVboBuffer& sBuffer, const CTreeRI_t* pBaseTree)
{
	assert(pBaseTree);

	st_bool bSuccess = true;

	typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = NULL;
#ifdef _OPENMP
#pragma omp critical
#endif

	{
		// find the instance data associated with this base tree object
		pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
		assert(pInstancingData);

		// initialize the instance manager if not already (each base tree has its own billboard instance manager)
		if (!pInstancingData->m_tBillboardInstancingMgr.IsInitialized( ))
			bSuccess &= pInstancingData->m_tBillboardInstancingMgr.InitBillboards(&pBaseTree->GetBillboardGeometryBuffer( ));
	}

	// this base tree's billboard instance manager will copy the per-instance billboard data
	bSuccess &= pInstancingData->m_tBillboardInstancingMgr.CopyInstancesToBillboardInstanceBuffer(sBuffer, pBaseTree, VisibleCells( ));

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::CopyVboDataToGpu

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::CopyVboDataToGpu(const SBillboardVboBuffer& sBuffer, const CTreeRI_t* pBaseTree)
{
	// find the instance data associated with this base tree object
	typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
	assert(pInstancingData);

	return pInstancingData->m_tBillboardInstancingMgr.CopyVboDataToGpu(sBuffer, m_instancingRingBuffer);
}

///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::NotifyOfPopulationChange

CVisibleInstancesRI_TemplateList
	inline void CVisibleInstancesRI_t::NotifyOfFrustumReset(void)
{
	CVisibleInstances::NotifyOfFrustumReset( );
}

///////////////////////////////////////////////////////////////////////  
//  CVisibleInstancesRI::GetNumBillboardsCeiling

CVisibleInstancesRI_TemplateList
	inline st_int32 CVisibleInstancesRI_t::GetNumBillboardsCeiling()
{

	const TRowColCellPtrMap &visibleCells = VisibleCells( );
	st_int32 siNumBillboardsCeiling = 0;
	for (TRowColCellPtrMap::const_iterator i = visibleCells.begin( ); i != visibleCells.end( ); ++i)
	{
		assert(i->second);
		siNumBillboardsCeiling += (st_int32)i->second->GetTreeInstances( ).size( );
	}
	return siNumBillboardsCeiling;
}

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::LAVACopyBillboardInstanceDataToInstanceBufferDoneRight( const CTreeRI_t* pBaseTree, const Vec4 * const frustumPlanes, st_float32 cullingRadius )
{
	assert(pBaseTree);

	st_bool bSuccess = true;

	typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = NULL;

	{
		// find the instance data associated with this base tree object
		pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
		assert(pInstancingData);

		// initialize the instance manager if not already (each base tree has its own billboard instance manager)
		if (!pInstancingData->m_tBillboardInstancingMgr.IsInitialized( ))
			bSuccess &= pInstancingData->m_tBillboardInstancingMgr.InitBillboards(&pBaseTree->GetBillboardGeometryBuffer( ));
	}

	// this base tree's billboard instance manager will copy the per-instance billboard data
	bSuccess &= pInstancingData->m_tBillboardInstancingMgr.LAVACopyInstancesToBillboardInstanceBufferDoneRight(pBaseTree, VisibleCells( ), frustumPlanes, cullingRadius, m_instancingRingBuffer);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::NotifyOfPopulationChange

CVisibleInstancesRI_TemplateList
inline void CVisibleInstancesRI_t::NotifyOfPopulationChange(void)
{
	// delete per-base-tree instance data
	for (typename TInstanceDataPtrMap::iterator i = m_mPerBaseInstancingDataMap.begin( ); i != m_mPerBaseInstancingDataMap.end( ); ++i)
	{
		i->second->m_t3dTreeInstancingMgr.ReleaseGfxResources( );
		i->second->m_tBillboardInstancingMgr.ReleaseGfxResources( );
		st_delete<SForestInstancingData>(i->second);
	}
	m_mPerBaseInstancingDataMap.clear( );

	CVisibleInstances::NotifyOfPopulationChange( );
}

///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::NotifyOfPopulationRemoval

CVisibleInstancesRI_TemplateList
	inline void CVisibleInstancesRI_t::NotifyOfPopulationRemoval( st_int32 rowCol[4], const CTree* tree )
{
	// delete per-base-tree instance data
	typename TInstanceDataPtrMap::iterator it = m_mPerBaseInstancingDataMap.find( tree );
	if( it != m_mPerBaseInstancingDataMap.end() )
	{
		it->second->m_t3dTreeInstancingMgr.ReleaseGfxResources();
		it->second->m_tBillboardInstancingMgr.ReleaseGfxResources();
		st_delete<SForestInstancingData>( it->second );
		
		m_mPerBaseInstancingDataMap.erase( it );
	}

	CVisibleInstances::NotifyOfPopulationRemoval( rowCol );
}

///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::NotifyOfPopulationAddition

CVisibleInstancesRI_TemplateList
	inline void CVisibleInstancesRI_t::NotifyOfPopulationAddition( st_int32 row, st_int32 col, const CTree* tree )
{
	// delete per-base-tree instance data
	typename TInstanceDataPtrMap::iterator it = m_mPerBaseInstancingDataMap.find( tree );
	if( it != m_mPerBaseInstancingDataMap.end() )
	{
		it->second->m_t3dTreeInstancingMgr.ReleaseGfxResources();
		it->second->m_tBillboardInstancingMgr.ReleaseGfxResources();
		st_delete<SForestInstancingData>( it->second );

		m_mPerBaseInstancingDataMap.erase( it );
	}

	CVisibleInstances::NotifyOfPopulationAddition( row, col );
}

///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::FindInstancingDataByBaseTree

CVisibleInstancesRI_TemplateList
inline const typename CVisibleInstancesRI_t::SForestInstancingData* CVisibleInstancesRI_t::FindInstancingDataByBaseTree(const CTree* pTree) const
{
	const SForestInstancingData* pData = NULL;

	if (pTree)
	{
		typename TInstanceDataPtrMap::const_iterator iFind = m_mPerBaseInstancingDataMap.find(pTree);
		if (iFind != m_mPerBaseInstancingDataMap.end( ))
			pData = iFind->second;

		// todo: when billboards are toggled, this fires pretty often
		//if (!pData)
		//	CCore::SetError("CVisibleInstancesRI_t::FindInstancingDataByBaseTree() is returning NULL; long story short: you probably started rendering a forest before it was ever culled");
	}

	return pData;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::FindOrAddInstancingDataByBaseTree

CVisibleInstancesRI_TemplateList
inline typename CVisibleInstancesRI_t::SForestInstancingData* CVisibleInstancesRI_t::FindOrAddInstancingDataByBaseTree(const CTree* pTree)
{
	SForestInstancingData* pData = NULL;

	if (pTree)
	{
		typename TInstanceDataPtrMap::const_iterator iFind = m_mPerBaseInstancingDataMap.find(pTree);
		if (iFind != m_mPerBaseInstancingDataMap.end( ))
			pData = iFind->second;
		else
		{
			pData = st_new(SForestInstancingData, "SForestInstancingData");
			m_mPerBaseInstancingDataMap[pTree] = pData;

			pData->m_pBaseTree = pTree;
		}
	}

	return pData;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::GetPerBaseInstancingDataMap

CVisibleInstancesRI_TemplateList
	inline const typename CVisibleInstancesRI_t::TInstanceDataPtrMap& CVisibleInstancesRI_t::GetPerBaseInstancingDataMap(void) const
{
	return m_mPerBaseInstancingDataMap;
}


///////////////////////////////////////////////////////////////////////
//  CVisibleInstancesRI::Update3dTreeInstanceBuffers

CVisibleInstancesRI_TemplateList
inline st_bool CVisibleInstancesRI_t::Update3dTreeInstanceBuffers(const CView& cView)
{
	st_bool bSuccess = true;

    ScopeTrace("InstMgr::Update3dTreeInstanceBuffers");
	const TDetailedCullDataArray& aPerBase3dInstances = Get3dInstanceLods( );
	for (st_int32 nBase = 0; nBase < st_int32(aPerBase3dInstances.size( )); ++nBase)
	{
		// get the instances for this base tree index
		const T3dTreeInstanceLodArray& aInstanceLodsForBase = aPerBase3dInstances[nBase].m_a3dInstanceLods;
		if (!aInstanceLodsForBase.empty( ))
		{
			// access base tree as render interface type
			CTreeRI_t* pBaseTree = (CTreeRI_t*) aPerBase3dInstances[nBase].m_pBaseTree;
			assert(pBaseTree);

			// get base tree geometry
			assert(pBaseTree->GetGeometry( ));
			const st_int32 c_nNumLods = pBaseTree->GetGeometry( )->m_nNumLods;

			// find the VB associate with this base tree
			typename CVisibleInstancesRI_t::SForestInstancingData* pInstancingData = FindOrAddInstancingDataByBaseTree(pBaseTree);
			assert(pInstancingData);

			// initialize instance manager if not already
			if (!pInstancingData->m_t3dTreeInstancingMgr.IsInitialized( ))
				bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.Init3dTrees(c_nNumLods, pBaseTree->GetGeometryBuffers( ));

			bSuccess &= pInstancingData->m_t3dTreeInstancingMgr.Update3dTreeInstanceBuffers(c_nNumLods, aInstanceLodsForBase/*LAVA++*/, m_instancingRingBuffer);
		}
	}

	return bSuccess;
}

// LAVA++
CVisibleInstancesRI_TemplateList
	inline st_bool CVisibleInstancesRI_t::PreUpdate3dTreeInstanceBuffers()
{
	return m_instancingRingBuffer.Lock();
}

CVisibleInstancesRI_TemplateList
	inline void CVisibleInstancesRI_t::PostUpdate3dTreeInstanceBuffers()
{
	m_instancingRingBuffer.Unlock();
}
// LAVA--



