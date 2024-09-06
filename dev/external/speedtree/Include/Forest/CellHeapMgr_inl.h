///////////////////////////////////////////////////////////////////////  
//  CellHeapMgr.inl
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
//  CCellHeapMgr::CCellHeapMgr

template <typename T>
inline CCellHeapMgr<T>::CCellHeapMgr(EPopulationType ePopulationType) :
	m_ePopulationType(ePopulationType),
	m_nMaxNumCells(-1),
	m_nMaxNumInstancesPerCell(-1)
{
	m_aHeapBlocks.SetHeapDescription("CCellHeapMgr::m_aHeapBlocks");
	m_aAvailableCells.SetHeapDescription("CCellHeapMgr::m_aAvailableCells");
}


///////////////////////////////////////////////////////////////////////
//  CCellHeapMgr::~CCellHeapMgr

template <typename T>
inline CCellHeapMgr<T>::~CCellHeapMgr( )
{
	for (size_t i = 0; i < m_aHeapBlocks.size( ); ++i)
		st_delete<SHeapBlock>(m_aHeapBlocks[i]);
}


///////////////////////////////////////////////////////////////////////
//  CCellHeapMgr::Init

template <typename T>
inline void CCellHeapMgr<T>::Init(st_int32 nMaxNumCells, st_int32 nMaxNumInstancesPerCell)
{
	m_nMaxNumCells = nMaxNumCells;
	m_nMaxNumInstancesPerCell = nMaxNumInstancesPerCell;

	Grow( );
}


///////////////////////////////////////////////////////////////////////
//  CCellHeapMgr::CheckOut

template <typename T>
inline T* CCellHeapMgr<T>::CheckOut(void)
{
	T* pOutgoingCell = nullptr;

	// LAVA++
	// We need the ability to disable instance buffer reservations, since they use too much memory when lots of cells are visible
#ifndef DISABLE_VISIBLE_INSTANCE_HEAP_RESERVATION_DEFAULTS
	if (!IsInitialized( ))
	{
		#ifdef SPEEDTREE_RUNTIME_HEAP_CHECK
			CCore::SetError("The culling system made some avoidable heap allocations, set CMyApplication::SetHeapReserves for mangement example");
		#endif

		// it's best to call CVisibleInstances::SetHeapReserves() before the first frame is rendered; using some reasonable
		// default values until the app can provide its own
		m_nMaxNumCells = 50;
		m_nMaxNumInstancesPerCell = 500;
	}
#endif
	// LAVA--
    
    // help avoid any render-time allocations
    SPEEDTREE_HEAP_ALLOC_CHECK(CArray<SHeapBlock*>, m_aHeapBlocks, (m_ePopulationType == POPULATION_TREES) ? SDK_LIMIT_MAX_VISIBLE_TREE_CELLS : SDK_LIMIT_MAX_VISIBLE_GRASS_CELLS);

	if (m_aAvailableCells.empty( ))
	{
		Grow( );
		assert(!m_aAvailableCells.empty( ));
	}

	// use the next available cell
	pOutgoingCell = m_aAvailableCells.back( );
	m_aAvailableCells.pop_back( );

	// since cells are recycled, make sure the instances are clear
	if (m_ePopulationType == POPULATION_TREES)
	{
		pOutgoingCell->m_aTreeInstances.resize(0);
		#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
			pOutgoingCell->m_mBaseTreesToBillboardVboStreamsMap.clear( );
		#endif
	}
	else if (m_ePopulationType == POPULATION_GRASS)
		pOutgoingCell->m_aGrassInstances.resize(0);

	return pOutgoingCell;
}


///////////////////////////////////////////////////////////////////////
//  CCellHeapMgr::CheckIn

template <typename T>
inline void CCellHeapMgr<T>::CheckIn(T* pIncomingCell)
{
	assert(m_nMaxNumInstancesPerCell > -1); // likely means Init() wasn't called

	pIncomingCell->m_aTreeInstances.clear();
	pIncomingCell->m_aGrassInstances.clear();
	pIncomingCell->m_mBaseTreesToBillboardVboStreamsMap.clear();

	// since the cell is no longer used, add to list of available
	m_aAvailableCells.push_back(pIncomingCell);
}


///////////////////////////////////////////////////////////////////////
//  CCellHeapMgr::Grow

template <typename T>
inline void CCellHeapMgr<T>::Grow(void)
{
	assert(m_nMaxNumCells > -1);

	// create cell depot
	size_t siDepotSize = m_aHeapBlocks.size( );
	m_aHeapBlocks.resize(siDepotSize + 1);
	m_aHeapBlocks[siDepotSize] = st_new(SHeapBlock, "CCellHeapMgr::SHeapBlock");

	// alias for new heap block entry
	SHeapBlock* pNewHeapBlock = m_aHeapBlocks.back( );
	assert(pNewHeapBlock);

	// resize new cell buffer
	TCellArray& aNewCellBuffer = pNewHeapBlock->m_aCellBuffer;
	aNewCellBuffer.resize(m_nMaxNumCells);

	// init cells & append to list of available cells
	size_t siAvailableSize = m_aAvailableCells.size( );
	m_aAvailableCells.resize(siAvailableSize + m_nMaxNumCells);
	for (st_int32 i = 0; i < m_nMaxNumCells; ++i)
	{
		InitCell(aNewCellBuffer[i]);
		m_aAvailableCells[siAvailableSize + i] = &aNewCellBuffer[i];
	}

	// grass cell buffer
	if (m_ePopulationType == POPULATION_GRASS && m_nMaxNumCells > 0 && m_nMaxNumInstancesPerCell > 0)
	{
		TGrassInstArray& aNewGrassInstBuffer = pNewHeapBlock->m_aGrassInstBuffer;
		aNewGrassInstBuffer.resize(m_nMaxNumCells * m_nMaxNumInstancesPerCell);

		SGrassInstance* pGrassBufferPtr = &aNewGrassInstBuffer[0];
		for (st_int32 i = 0; i < m_nMaxNumCells; ++i)
		{
			aNewCellBuffer[i].m_aGrassInstances.SetExternalMemory((st_byte*) pGrassBufferPtr, m_nMaxNumInstancesPerCell * sizeof(SGrassInstance));
			pGrassBufferPtr += m_nMaxNumInstancesPerCell;
		}
	}
}


///////////////////////////////////////////////////////////////////////
//  CCellHeapMgr::InitCell

template <typename T>
inline void CCellHeapMgr<T>::InitCell(T& tCell) const
{
	if (m_ePopulationType == POPULATION_TREES)
		tCell.m_aTreeInstances.reserve(m_nMaxNumInstancesPerCell);
	else if (m_ePopulationType == POPULATION_GRASS)
	{
		// intentionally do nothing
	}
	else
		assert(false);
}


///////////////////////////////////////////////////////////////////////
//  CCellHeapMgr::IsInitialized

template <typename T>
inline st_bool CCellHeapMgr<T>::IsInitialized(void) const
{
	return (m_nMaxNumCells > 0 && m_nMaxNumInstancesPerCell > 0);
}


