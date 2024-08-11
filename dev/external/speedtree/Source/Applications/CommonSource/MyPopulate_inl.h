///////////////////////////////////////////////////////////////////////  
//  MyPopulate.inl
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
//  CMyInstancesContainer::IsEmpty

inline st_bool CMyInstancesContainer::IsEmpty(void) const
{
	// if no base trees are here, it must be empty
	st_bool bEmpty = m_aBaseTrees.empty( ) || m_aaInstances.empty( );

	// if base trees are present, check instances
	if (!bEmpty)
	{
		st_bool bInstsPresent = false;

		for (size_t i = 0; i < m_aaInstances.size( ); ++i)
		{
			if (!m_aaInstances[i].empty( ))
			{
				bInstsPresent = true;
				break;
			}
		}

		bEmpty = !bInstsPresent;
	}

	return bEmpty;
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::Cell

inline SPerCellData* CMyInstancesContainer::Cell(const SCellKey& sKey)
{
	SPerCellData* pData = NULL;

	if (sKey.m_nRow >= m_nFirstRow && sKey.m_nRow <= m_nLastRow &&
		sKey.m_nCol >= m_nFirstCol && sKey.m_nCol <= m_nLastCol)
	{
		st_int32 nRow = sKey.m_nRow - m_nFirstRow;
		st_int32 nCol = sKey.m_nCol - m_nFirstCol;

		st_assert(st_int32(m_aCells.size( )) == m_nNumCols * m_nNumRows, "CMyInstancesContainer::m_aCells was not propertly initialized");
		pData = &m_aCells[nCol * m_nNumRows + nRow];
	}

	return pData;
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::GetBaseTrees

inline const TTreePtrArray& CMyInstancesContainer::GetBaseTrees(void) const
{
	return m_aBaseTrees;
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::GetBaseTrees

inline const TTreeInstArray2D& CMyInstancesContainer::GetInstancesPerBaseTree(void) const
{
	return m_aaInstances;
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::GetLongestCellOverhang

inline st_float32 CMyInstancesContainer::GetLongestCellOverhang(void) const
{
	return m_fLongestCellOverhang;
}
