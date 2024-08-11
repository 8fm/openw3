///////////////////////////////////////////////////////////////////////  
//  CellContainer.inl
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
//  CCellContainer::CCellContainer

template<class TCellType>
ST_INLINE CCellContainer<TCellType>::CCellContainer( )
	: CMap<SCellKey, TCellType>(10)
	, m_fCellSize(1200.0f)
{
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::~CCellContainer

template<class TCellType>
ST_INLINE CCellContainer<TCellType>::~CCellContainer( )
{
	#ifndef NDEBUG
		m_fCellSize = -1.0f;
	#endif
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByRowCol_Add

template<class TCellType>
ST_INLINE TCellType* CCellContainer<TCellType>::GetCellPtrByRowCol_Add(st_int32 nRow, st_int32 nCol)
{
	typename CCellContainer<TCellType>::iterator iCell = GetCellItrByRowCol_Add(nRow, nCol);
	if (iCell == CMap<SCellKey, TCellType>::end( ))
		return NULL;
	else
		return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByRowCol

template<class TCellType>
ST_INLINE const TCellType* CCellContainer<TCellType>::GetCellPtrByRowCol(st_int32 nRow, st_int32 nCol) const
{
	typename CCellContainer<TCellType>::const_iterator iCell = GetCellItrByRowCol(nRow, nCol);
	if (iCell == CMap<SCellKey, TCellType>::end( ))
		return NULL;
	else
		return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByPos_Add

template<class TCellType>
ST_INLINE TCellType* CCellContainer<TCellType>::GetCellPtrByPos_Add(const Vec3& vPos)
{
	typename CCellContainer<TCellType>::iterator iCell = GetCellItrByPos(vPos);
	if (iCell == CMap<SCellKey, TCellType>::end( ))
		return NULL;
	else
		return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByPos

template<class TCellType>
ST_INLINE const TCellType* CCellContainer<TCellType>::GetCellPtrByPos(const Vec3& vPos) const
{
	typename CCellContainer<TCellType>::const_iterator iCell = GetCellItrByPos(vPos);
	if (iCell == CMap<SCellKey, TCellType>::end( ))
		return NULL;
	else
		return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByRowCol_Add

template<class TCellType>
ST_INLINE typename CCellContainer<TCellType>::iterator CCellContainer<TCellType>::GetCellItrByRowCol_Add(st_int32 nRow, st_int32 nCol)
{
	// use a cell key to see if the map already contains this row/col cell
	SCellKey sKey(nRow, nCol);
	typename CCellContainer<TCellType>::iterator iCell = CMap<SCellKey, TCellType>::find(sKey);

	// cell wasn't found; did the caller specify that a new cell should be added?
	if (iCell == CMap<SCellKey, TCellType>::end( ))
	{
		// add it to the map and return the result
		TCellType& cCell = (*this)[sKey];
		assert(!cCell.GetExtents( ).Valid( ));
		cCell.SetRowCol(nRow, nCol);

		iCell = CMap<SCellKey, TCellType>::find(sKey);
		assert((iCell != CMap<SCellKey, TCellType>::end( )));

		// sanity test
		assert(cCell.IsNew( ));
	}

	return iCell;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByRowCol

template<class TCellType>
ST_INLINE typename CCellContainer<TCellType>::const_iterator CCellContainer<TCellType>::GetCellItrByRowCol(st_int32 nRow, st_int32 nCol) const
{
	// use a cell key to see if the map already contains this row/col cell
	SCellKey sKey(nRow, nCol);
	typename CCellContainer::const_iterator iCell = CMap<SCellKey, TCellType>::find(sKey);

	return iCell;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByPos_Add

template<class TCellType>
ST_INLINE typename CCellContainer<TCellType>::iterator CCellContainer<TCellType>::GetCellItrByPos_Add(const Vec3& vPos)
{
	// convert 3D position to a row/col pair
	st_int32 nRow, nCol;
	ComputeCellCoords(vPos, m_fCellSize, nRow, nCol);

	// use the row/col pair to get the cell
	return GetCellItrByRowCol_Add(nRow, nCol);
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByPos

template<class TCellType>
ST_INLINE typename CCellContainer<TCellType>::const_iterator CCellContainer<TCellType>::GetCellItrByPos(const Vec3& vPos) const
{
	// convert 3D position to a row/col pair
	st_int32 nRow, nCol;
	ComputeCellCoords(vPos, m_fCellSize, nRow, nCol);

	// use the row/col pair to get the cell
	return GetCellItrByRowCol(nRow, nCol);
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::Erase

template<class TCellType>
ST_INLINE typename CCellContainer<TCellType>::iterator CCellContainer<TCellType>::Erase(typename CCellContainer<TCellType>::iterator iCell)
{
	return erase(iCell);
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellSize

template<class TCellType>
ST_INLINE st_float32 CCellContainer<TCellType>::GetCellSize(void) const
{
	return m_fCellSize;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::SetCellSize

template<class TCellType>
ST_INLINE void CCellContainer<TCellType>::SetCellSize(st_float32 fCellSize)
{
	m_fCellSize = fCellSize;
}




