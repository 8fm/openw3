///////////////////////////////////////////////////////////////////////
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
//  Preprocessor

#include "MyPopulate.h"
#include "Core/Random.h"
#include "MyTerrain.h"
#include "Utilities/Utility.h"
#include <ctime>
#include <errno.h>
#include <ctype.h>
#ifdef SPEEDTREE_OPENMP
	#include <omp.h>
#endif
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  SPerCellData::SPerCellData

SPerCellData::SPerCellData( )
{
	m_aInstances.SetHeapDescription("CArray (client-side population)");
	m_aBaseTrees.SetHeapDescription("CArray (client-side population)");
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::CMyInstancesContainer

CMyInstancesContainer::CMyInstancesContainer( ) :
	m_nFirstCol(0),
	m_nLastCol(0),
	m_nNumCols(0),
	m_nFirstRow(0),
	m_nLastRow(0),
	m_nNumRows(0),
	m_fCellSize(-1.0f),
	m_fLongestCellOverhang(0.0f)
{
	m_aBaseTrees.SetHeapDescription("CArray (client-side population)");
	m_aaInstances.SetHeapDescription("CArray (client-side population)");
	m_aCells.SetHeapDescription("CArray (client-side population)");
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::~CMyInstancesContainer

CMyInstancesContainer::~CMyInstancesContainer( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::SetMaxNumBaseTrees

void CMyInstancesContainer::SetMaxNumBaseTrees(st_int32 nMaxNumBaseTrees)
{
	m_aBaseTrees.resize(nMaxNumBaseTrees, NULL);
	m_aaInstances.resize(nMaxNumBaseTrees);

	for (st_int32 i = 0; i < nMaxNumBaseTrees; ++i)
		m_aaInstances[i].SetHeapDescription("CArray (client-side population)");
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::SetInstances

st_bool CMyInstancesContainer::SetInstances(CTree* pBaseTree, const TTreeInstArray& aInstances)
{
	st_assert(pBaseTree, "CMyInstancesContainer::SetInstances() expects non-NULL CTree* parameter");

	st_bool bSuccess = false;

	// look to see if base tree occupies a slot
	st_int32 nSlot = -1;
	for (st_int32 i = 0; i < st_int32(m_aBaseTrees.size( )); ++i)
	{
		if (m_aBaseTrees[i] == pBaseTree)
		{
			nSlot = i;
			break;
		}
	}

	// if not found, look for first open slot
	if (nSlot == -1)
	{
		// look for first open base tree slot
		for (st_int32 i = 0; i < st_int32(m_aBaseTrees.size( )); ++i)
		{
			if (m_aBaseTrees[i] == NULL)
			{
				nSlot = i;
				break;
			}
		}
	}

	// if base tree slot found, copy the instances
	if (nSlot > -1 && nSlot < st_int32(m_aBaseTrees.size( )))
	{
		m_aBaseTrees[nSlot] = pBaseTree;
		m_aaInstances[nSlot] = aInstances;

		// run through the instances, looking for a new largest cell overhang value
		for (st_int32 i = 0; i < st_int32(aInstances.size( )); ++i)
			m_fLongestCellOverhang = st_max(m_fLongestCellOverhang, aInstances[i].GetCullingRadius( ));

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  Helper function: GetCellKeyByPos

SCellKey GetCellKeyByPos(const Vec3& vPos, st_float32 fCellSize)
{
	Vec3 vPos_cs = CCoordSys::ConvertToStd(vPos);

	return SCellKey((vPos_cs.y < 0.0f) ? st_int32((vPos_cs.y - fCellSize) / fCellSize) : st_int32(vPos_cs.y / fCellSize),
					(vPos_cs.x < 0.0f) ? st_int32((vPos_cs.x - fCellSize) / fCellSize) : st_int32(vPos_cs.x / fCellSize));
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::SplitIntoCells

void CMyInstancesContainer::SplitIntoCells(st_float32 fCellSize)
{
	// init
	m_aCells.clear( );
	m_fCellSize = fCellSize;

	// determine extents
	CExtents cForestExtents;
	for (st_int32 nBase = 0; nBase < st_int32(m_aBaseTrees.size( )); ++nBase)
	{
		CTree* pBase = m_aBaseTrees[nBase];
		st_assert(pBase, "All m_aBaseTrees members should be non-NULL (successfully loaded SRT files)");

		const CExtents& cBaseTreeExtents = pBase->GetExtents( );

		for (st_int32 nInstance = 0; nInstance < st_int32(m_aaInstances[nBase].size( )); ++nInstance)
		{
			const CTreeInstance* pInstance = &m_aaInstances[nBase][nInstance];

			CExtents cInstExtents = cBaseTreeExtents;
			cInstExtents.Scale(pInstance->GetScalar( ));
			cInstExtents.Orient(pInstance->GetUpVector( ), pInstance->GetRightVector( ));
			cInstExtents.Translate(pInstance->GetPos( ));
			cForestExtents.ExpandAround(cInstExtents);
		}
	}

	if (cForestExtents.Valid( ))
	{
		// determine col/row extents
		ComputeCellCoords(cForestExtents.Min( ), m_fCellSize, m_nFirstRow, m_nFirstCol);
		ComputeCellCoords(cForestExtents.Max( ), m_fCellSize, m_nLastRow, m_nLastCol);
		OrderPair<st_int32>(m_nFirstRow, m_nLastRow);
		OrderPair<st_int32>(m_nFirstCol, m_nLastCol);
		m_nNumRows = m_nLastRow - m_nFirstRow + 1;
		m_nNumCols = m_nLastCol - m_nFirstCol + 1;

		// clear out old cells
		m_aCells.resize(0);

		// allocate cell pool
		m_aCells.resize(m_nNumRows * m_nNumCols, SPerCellData( ));

		for (st_int32 nBase = 0; nBase < st_int32(m_aBaseTrees.size( )); ++nBase)
		{
			CTree* pBase = m_aBaseTrees[nBase];
			st_assert(pBase, "All m_aBaseTrees members should be non-NULL (successfully loaded SRT files)");

			const CExtents& cBaseTreeExtents = pBase->GetExtents( );

			for (st_int32 nInstance = 0; nInstance < st_int32(m_aaInstances[nBase].size( )); ++nInstance)
			{
				const CTreeInstance* pInstance = &m_aaInstances[nBase][nInstance];

				// build key to find instance's cell
				SCellKey sCellKey = GetCellKeyByPos(pInstance->GetPos( ), fCellSize);

				SPerCellData* pPerCellData = Cell(sCellKey);
				st_assert(pPerCellData, "Cell() returned a NULL cell; probably wasn't initialized");

				// add base tree, but just once
				pPerCellData->m_aBaseTrees.insert_sorted_unique(pBase);

				// add instance
				st_assert(pInstance->InstanceOf( ), "Every instance should know which base tree it's an instance of");
				pPerCellData->m_aInstances.push_back(pInstance);

				// adjust cell's extents based on instance's position
				CExtents cInstExtents = cBaseTreeExtents;
				cInstExtents.Scale(pInstance->GetScalar( ));
				cInstExtents.Orient(pInstance->GetUpVector( ), pInstance->GetRightVector( ));
				cInstExtents.Translate(pInstance->GetPos( ));
				pPerCellData->m_cExtents.ExpandAround(cInstExtents);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::Clear

void CMyInstancesContainer::Clear(void)
{
	m_aBaseTrees.clear( );
	m_aaInstances.clear( );

	// organize for fast population reporting to SDK
	SplitIntoCells(m_fCellSize);
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::ComputeExtents

void CMyInstancesContainer::ComputeExtents(CExtents& cExtents) const
{
	cExtents.Reset( );
	if (m_fCellSize > -1.0f && !m_aCells.empty( ))
	{
		for (CArray<SPerCellData>::const_iterator iCell = m_aCells.begin( ); iCell != m_aCells.end( ); ++iCell)
		{
			cExtents.ExpandAround(iCell->m_cExtents);
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::ComputeTotalInstances

st_int32 CMyInstancesContainer::ComputeTotalInstances(void) const
{
	st_int32 nCount = 0;

	for (st_int32 nBase = 0; nBase < st_int32(m_aaInstances.size( )); ++nBase)
		nCount += st_int32(m_aaInstances[nBase].size( ));

	return nCount;
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::ComputeMaxNumInstancesInAnyCell

st_int32 CMyInstancesContainer::ComputeMaxNumInstancesInAnyCell(void) const
{
	st_int32 nMax = 0;

	for (CArray<SPerCellData>::const_iterator iCell = m_aCells.begin( ); iCell != m_aCells.end( ); ++iCell)
		nMax = st_max(nMax, st_int32(iCell->m_aInstances.size( )));

	return nMax;
}


///////////////////////////////////////////////////////////////////////  
//  Helper function: ComputeMaxNumCellsForFrustum
//
//	fFieldOfView is in degrees

st_int32 ComputeMaxNumCellsForFrustum(st_float32 fCellSize, st_float32 fFieldOfView, st_float32 fAspectRatio, st_float32 fFarClip)
{
	const st_float32 c_fWideAngle = DegToRad(fFieldOfView * fAspectRatio);
	const st_float32 c_fFullWidth = 2.0f * fFarClip * tan(0.5f * c_fWideAngle);

	const st_int32 c_nNumCols = st_int32(c_fFullWidth / fCellSize) + 2;
	const st_int32 c_nNumRows = st_int32(fFarClip / fCellSize) + 2;

	// multiply by two in cases where the camera changes to a completely different section; the way the culling
	// system works, we'll set up the newly visible cells before we release the old cells
	return 2 * c_nNumCols * c_nNumRows;
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::ComputeHeapReserveGuess

void CMyInstancesContainer::ComputeHeapReserveGuess(SHeapReserves& sReserves, const CMyConfigFile& cConfigFile, st_float32 fScreenAspectRatio) const
{
	// max base trees
	sReserves.m_nMaxBaseTrees = st_int32(m_aBaseTrees.size( ) + cConfigFile.m_aGrassPopulations.size( ));

	// max terrain cell counts
	sReserves.m_nMaxVisibleTerrainCells = ComputeMaxNumCellsForFrustum(cConfigFile.m_sTerrain.m_fCellSize, cConfigFile.m_sWorld.m_fFieldOfView, fScreenAspectRatio, cConfigFile.m_sWorld.m_fFarClip);

	// max tree cell counts
	sReserves.m_nMaxVisibleTreeCells = ComputeMaxNumCellsForFrustum(cConfigFile.m_sWorld.m_f3dTreeCellSize, cConfigFile.m_sWorld.m_fFieldOfView, fScreenAspectRatio, cConfigFile.m_sWorld.m_fFarClip);

	// max grass cell counts
	if (!cConfigFile.m_aGrassPopulations.empty( ))
	{
		// pick the furthest far LOD of all the grass entries
		st_float32 fFurthestLod = 0.0f;
		for (size_t i = 0; i < cConfigFile.m_aGrassPopulations.size( ); ++i)
			fFurthestLod = st_max(fFurthestLod, cConfigFile.m_aGrassPopulations[i].m_afLodRange[1]);

		// find smallest grass cell size
		st_float32 fSmallestGrassCell = FLT_MAX;
		for (size_t i = 0; i < cConfigFile.m_aGrassPopulations.size( ); ++i)
			fSmallestGrassCell = st_min(fSmallestGrassCell, cConfigFile.m_aGrassPopulations[i].m_fCellSize);

		if (fSmallestGrassCell != FLT_MAX)
			sReserves.m_nMaxVisibleGrassCells = ComputeMaxNumCellsForFrustum(fSmallestGrassCell, cConfigFile.m_sWorld.m_fFieldOfView, fScreenAspectRatio, fFurthestLod);

		// todo: hack
		sReserves.m_nMaxVisibleGrassCells /= 10;
	}

	// max tree instances in any cell
	sReserves.m_nMaxTreeInstancesInAnyCell = 1;
	for (size_t i = 0; i < m_aCells.size( ); ++i)
		sReserves.m_nMaxTreeInstancesInAnyCell = st_max(sReserves.m_nMaxTreeInstancesInAnyCell, st_int32(m_aCells[i].m_aInstances.size( )));

	// max grass blades in any one cell
	if (!cConfigFile.m_aGrassPopulations.empty( ))
	{
		st_float32 fMax = 0.0f;
		for (size_t i = 0; i < cConfigFile.m_aGrassPopulations.size( ); ++i)
		{
			const st_float32 c_fCellArea = cConfigFile.m_aGrassPopulations[i].m_fCellSize * cConfigFile.m_aGrassPopulations[i].m_fCellSize;
			fMax = st_max(fMax, cConfigFile.m_aGrassPopulations[i].m_fDensity * c_fCellArea);
		}

		// todo: this was getting out of hand for the varied types of grass we have in the Meadow
		sReserves.m_nMaxPerBaseGrassInstancesInAnyCell = st_int32(fMax);
	}

	// number of shadow maps
	sReserves.m_nNumShadowMaps = cConfigFile.m_sShadows.m_bEnabled ? cConfigFile.m_sShadows.m_nNumMaps : 0;
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::DoublePopulation

void CMyInstancesContainer::DoublePopulation(const CMyConfigFile& cConfigFile, const CMyTerrain& cTerrain)
{
	Report("Doubling forest population, from %d to ", ComputeTotalInstances( ));

	for (st_int32 nBase = 0; nBase < st_int32(m_aBaseTrees.size( )); ++nBase)
	{
		const CTree* pBaseTree = m_aBaseTrees[nBase];

		// find parameter set in cConfigFile.m_aRandomTreePopulations that matches pBaseTree by filename
		const CMyConfigFile::SRandomTreePopulationParams* pPlacementParams = NULL;
		for (st_int32 i = 0; i < st_int32(cConfigFile.m_aRandomTreePopulations.size( )); ++i)
		{
			if (cConfigFile.m_aRandomTreePopulations[i].m_strSrtFilename == CFixedString(pBaseTree->GetFilename( )))
			{
				pPlacementParams = &cConfigFile.m_aRandomTreePopulations[i];
				break;
			}
		}

		if (pPlacementParams)
		{
			// create a CMyConfigFile::SRandomTreePopulationParams copy so the seed can be adjusted
			CMyConfigFile::SRandomTreePopulationParams sNewPlacementParams = *pPlacementParams;
			sNewPlacementParams.m_nSeed += nBase + 123; // just knock the seed of from its original value
			sNewPlacementParams.m_nQuantity = st_int32(m_aaInstances[nBase].size( ));

			// setup SMyPerBasePopulation expected by random population function
			CMyPopulate::SMyPerBasePopulation sNewPopulation;
			sNewPopulation.m_strSrtFilename = pBaseTree->GetFilename( );
			sNewPopulation.m_pBaseTree = (CTreeRender*) pBaseTree;
			CArray<CMyPopulate::SMyPerBasePopulation> aBaseTrees(1);
			aBaseTrees[0] = sNewPopulation;

			// generate random instance
			(void) CMyPopulate::GenerateRandom3dTreeInstances(aBaseTrees, 
															  sNewPlacementParams,
															  cConfigFile.m_sWorld.m_fTreeSurfaceAdhesion,
															  cTerrain);
			const TTreeInstArray& aNewInstances = aBaseTrees[0].m_aInstances;

			// add instances into active population
			m_aaInstances[nBase].reserve(m_aaInstances[nBase].size( ) + aNewInstances.size( ));
			for (st_int32 i = 0; i < st_int32(aNewInstances.size( )); ++i)
				m_aaInstances[nBase].push_back(aNewInstances[i]);
		}
		else
			Warning("CMyInstancesContainer::DoublePopulation, was not able to locate random placement params for [%s]\n", pBaseTree->GetFilename( )); 
	}

	Report("%d\n", ComputeTotalInstances( ));

	// organize for fast population reporting to SDK
	SplitIntoCells(m_fCellSize);
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::HalvePopulation

void CMyInstancesContainer::HalvePopulation(void)
{
	Report("Halving forest population, from %d to ", ComputeTotalInstances( ));

	for (st_int32 nBase = 0; nBase < st_int32(m_aBaseTrees.size( )); ++nBase)
	{
		st_int32 nNumInstances = st_int32(m_aaInstances[nBase].size( ));
		m_aaInstances[nBase].resize(nNumInstances / 2);
	}

	Report("%d\n", ComputeTotalInstances( ));

	// organize for fast population reporting to SDK
	SplitIntoCells(m_fCellSize);
}


///////////////////////////////////////////////////////////////////////  
//  Helper function: FillTreeLine

void FillTreeLine(TTreeInstArray& aTreeLine, const CTreeInstance& cTemplate, const Vec3& vStart, const Vec3& vEnd, st_float32 fTreeSpacing, const CMyTerrain& cTerrain)
{
	st_int32 nNumInstances = st_max(2, st_int32(vEnd.Distance(vStart) / fTreeSpacing));
	aTreeLine.reserve(aTreeLine.size( ) + nNumInstances);

	const Vec3 vSpacingVector = (vEnd - vStart).Normalize( ) * fTreeSpacing;

	Vec3 vPos = vStart;
	for (st_int32 i = 0; i < nNumInstances; ++i)
	{
		CTreeInstance cInstance = cTemplate;

		vPos[2] = cTerrain.GetHeightFromXY(vPos[0], vPos[1]);
		cInstance.SetPos(vPos);
		cInstance.ComputeCullParameters( );

		aTreeLine.push_back(cInstance);

		vPos += vSpacingVector;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CMyInstancesContainer::AnimateFigure

void CMyInstancesContainer::AnimateFigure(st_float32 fGlobalTime, const CExtents& cArea, st_float32 fTreeSpacing, const CMyTerrain& cTerrain)
{
	st_assert(m_aaInstances.size( ) == m_aBaseTrees.size( ), "m_aaInstances and m_aBaseTrees should always be the same size");

	// pre-compute some needed values
	const Vec3 c_vCenter = cArea.GetCenter( );
	const st_float32 c_fRadius = st_min(c_vCenter.Distance(Vec3(cArea.Max( ).x, c_vCenter.y, c_vCenter.z)), c_vCenter.Distance(Vec3(c_vCenter.x, cArea.Max( ).y, c_vCenter.z)));

	// clear current instances
	for (st_int32 nBase = 0; nBase < st_int32(m_aaInstances.size( )); ++nBase)
		m_aaInstances[nBase].resize(0);

	const st_int32 c_nNumBaseTreePairs = st_int32(m_aBaseTrees.size( )) / 2;
	for (st_int32 nBasePair = 0; nBasePair < c_nNumBaseTreePairs; ++nBasePair)
	{
		const Vec3 c_vLocalCenter = c_vCenter + Vec3(c_fRadius * 2.2f * nBasePair, 0.0f, 0.0f);

		// create rectangular border
		TTreeInstArray aBorder;
		CTreeInstance cBorderTemplate;
		cBorderTemplate.SetInstanceOf(m_aBaseTrees[nBasePair * 2]);
		FillTreeLine(aBorder, cBorderTemplate, c_vLocalCenter + Vec3(cArea.Min( ).x, cArea.Min( ).y, 0.0f), c_vLocalCenter + Vec3(cArea.Min( ).x, cArea.Max( ).y, 0.0f), fTreeSpacing, cTerrain);
		FillTreeLine(aBorder, cBorderTemplate, c_vLocalCenter + Vec3(cArea.Min( ).x, cArea.Max( ).y, 0.0f), c_vLocalCenter + Vec3(cArea.Max( ).x, cArea.Max( ).y, 0.0f), fTreeSpacing, cTerrain);
		FillTreeLine(aBorder, cBorderTemplate, c_vLocalCenter + Vec3(cArea.Max( ).x, cArea.Min( ).y, 0.0f), c_vLocalCenter + Vec3(cArea.Max( ).x, cArea.Max( ).y, 0.0f), fTreeSpacing, cTerrain);
		FillTreeLine(aBorder, cBorderTemplate, c_vLocalCenter + Vec3(cArea.Min( ).x, cArea.Min( ).y, 0.0f), c_vLocalCenter + Vec3(cArea.Max( ).x, cArea.Min( ).y, 0.0f), fTreeSpacing, cTerrain);
		SetInstances(m_aBaseTrees[nBasePair * 2], aBorder);

		// create rotating cross
		TTreeInstArray aCross;
		CTreeInstance cCrossTemplate;
		cCrossTemplate.SetInstanceOf(m_aBaseTrees[nBasePair * 2 + 1]);
		const st_float32 c_fAngle = fGlobalTime;
		const Vec3 c_vCrossPoint1 = Vec3(c_fRadius * cosf(c_fAngle), c_fRadius * sinf(c_fAngle));
		const Vec3 c_vCrossPoint2 = Vec3(c_fRadius * cosf(c_fAngle + c_fHalfPi), c_fRadius * sinf(c_fAngle + c_fHalfPi));
		FillTreeLine(aCross, cCrossTemplate, c_vLocalCenter +  + c_vCrossPoint1, c_vLocalCenter +  - c_vCrossPoint1, fTreeSpacing, cTerrain);
		FillTreeLine(aCross, cCrossTemplate, c_vLocalCenter +  + c_vCrossPoint2, c_vLocalCenter +  - c_vCrossPoint2, fTreeSpacing, cTerrain);
		SetInstances(m_aBaseTrees[nBasePair * 2 + 1], aCross);
	}

	// organize for fast population reporting to SDK
	SplitIntoCells(m_fCellSize);
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::CMyPopulate

CMyPopulate::CMyPopulate( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::~CMyPopulate

CMyPopulate::~CMyPopulate( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::StreamTrees

void CMyPopulate::StreamTrees(CMyInstancesContainer& cAllTreeInstances,
							  const CView& cView,
							  CVisibleInstancesRender& cCullResults,
							  st_int32 nFrameIndex,
							  st_bool b3dTreesVisible,
							  st_bool bBillboardsVisible,
							  st_bool bShadowPass)
{
	ScopeTrace(bShadowPass ? "CMyPopulate::StreamTrees(shadow)" : "CMyPopulate::StreamTrees");

	// the SDK organizes the world as a series of cells. As the camera moves, cells go in and out 
	// of visibility. As cells become visible, the SDK will provide a list of these cells that need 
	// to have their populations streamed in. Hence, the SDK will focus most efficiently is the
	// client has the tree instances already organized by cells so that the data might be passed 
	// into the SDK without further on-the-fly processing.

	if (!cAllTreeInstances.IsEmpty( ))
	{
		const bool c_bRenderTreesOrBillboards = b3dTreesVisible || bBillboardsVisible;

		// determine a rough/long list of cells in the frustum
		//
		// because the SDK does not store the entire tree instance population for a given forest (they're stored
		// app-side), it cannot know a priori the complete extents of the cells. specifically, it knows the width
		// and height of the cells because they're in a grid layout, but it cannot know the height. RoughCullCells() 
		// will return a long list of cells that would be visible if the grid were composed of infinitely tall cells.
		{
			ScopeTrace("RoughCullCells");
			(void) cCullResults.RoughCullCells(cView, nFrameIndex, cAllTreeInstances.GetLongestCellOverhang( ));
		}

		// set the extents for these cells (but do no more at this stage)
		{
			ScopeTrace("SetCellExtents");
			for (st_int32 nCell = 0; nCell < st_int32(cCullResults.RoughCells( ).size( )); ++nCell)
			{
				// get SDK-side rough cell
				CCell& cRoughCell = cCullResults.RoughCells( )[nCell];

				// lookup app-side instance cell to get extents
				const SPerCellData* pCellData = cAllTreeInstances.Cell(SCellKey(cRoughCell.Row( ), cRoughCell.Col( )));
				if (pCellData)
					cRoughCell.SetExtents(pCellData->m_cExtents);
			}
		}

		// using new extents, do a fine cull and update on cells

		// let the SDK use the updated cell extents to give an exact list of those cells that
		// are within the view frustum
		{
			ScopeTrace("FineCullTreeCells");
			cCullResults.FineCullTreeCells(cView, nFrameIndex);
		}

		// fill the newly visible cells (those cells that are entering the frustum for the first time)
		// with the corresponding instances
		{
			ScopeTrace("FillNewCells");
			for (size_t i = 0; i < cCullResults.NewlyVisibleCells( ).size( ); ++i)
			{
				CCell* pNewCell = cCullResults.NewlyVisibleCells( )[i];
				st_assert(pNewCell, "m_cVisibleTreesFromCamera.NewlyVisibleCells( ) should never contain a NULL cell pointer");

				// the app needs to take this cell id and determine which of the app-side trees reside
				// in it; in this example, there is a cell already stored that matches the SDK cell; it's
				// recalled here using the app-side CMyInstancesContainer::Cell() call which uses a CMap
				// of row/col coords to cell pointers
				const SPerCellData* pCellData = cAllTreeInstances.Cell(SCellKey(pNewCell->Row( ), pNewCell->Col( )));
				if (pCellData)
				{
					// copy the instance pointers of all base trees into the SDK using AppendTreeInstances(); note
					// that the instances are packed so that all of the instances of one base tree are adjacent, then
					// the next base tree and so on. for example, if there are base trees A, B, C, then:
					//
					//	- the third parameter will point to an array of A, B, C base tree pointers
					//
					//  - the first parameter will contain the packed instances, as in [aaaaaaabbbbbbbbbbccccccc], 
					//    where the lowercase letters represent base tree instances
					//
					//  - the second parameter is the total number of instances, across all base trees

					const TTreeInstConstPtrArray& aAppSideInstances = pCellData->m_aInstances;
					const TTreePtrArray& aAppSideBaseTrees = pCellData->m_aBaseTrees;
					if (!aAppSideInstances.empty( ) && !aAppSideBaseTrees.empty( ))
					{
						const CTreeInstance** pPackedListOfInstances = (const CTreeInstance**) &aAppSideInstances[0];
						const st_int32 c_nTotalInstancesForAllBaseTrees = st_int32(aAppSideInstances.size( ));
						const CTree** pListOfBaseTrees = (const CTree**) &aAppSideBaseTrees[0];
						const st_int32 c_nNumBaseTrees = st_int32(aAppSideBaseTrees.size( ));

						pNewCell->AppendTreeInstances(pListOfBaseTrees, c_nNumBaseTrees, pPackedListOfInstances, c_nTotalInstancesForAllBaseTrees);
					}
				}
			}
		}

		// update the LOD state of each visible 3d tree and copy them into the instance vertex buffers
		//
		// this is likely the most expensive part of the culling & population code as in runs in O(N)
		// time where N is the number of visible tree instances; on some of the lesser platforms the
		// instance vertex buffer may take some time or even block even though it's double-buffered
		if (c_bRenderTreesOrBillboards)
		{
			ScopeTrace("Update3dInstBuffers");
			if (!cCullResults.Update3dTreeInstanceBuffers(cView))
				CCore::SetError("m_cVisibleTreesFromCamera.Update3dTreeInstanceBuffers() failed in CMyApplication::StreamTreePopulation\n");
		}

		if (!bShadowPass)
		{
			// update the billboard instance vertex buffers
			//
			// unlike the 3d tree instances, billboards have no LOD state as computed by the CPU, so the time
			// is mostly spent turning the instance attributes into billboard vertices inside UpdateBillboardInstanceBuffers()
			if (c_bRenderTreesOrBillboards && !cCullResults.NewlyVisibleCells( ).empty( )) // only update when new cells are visible
			{
				#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
					// setup
					const TTreePtrArray& aBaseTrees = cAllTreeInstances.GetBaseTrees( );
					if (!aBaseTrees.empty( ))
					{
						// run this in parallel, but restrict to c_nMaxNumThreads simultaneous threads to reduce memory usage
						const st_int32 c_nMaxNumThreads = 8;
						for (st_int32 nBatch = 0; nBatch < st_int32(aBaseTrees.size( )); nBatch += c_nMaxNumThreads)
						{
							const st_int32 c_nThreadsInBatch = st_min(c_nMaxNumThreads, st_int32(aBaseTrees.size( )) - nBatch);

							// multiple buffers to support openmp loop
							SBillboardVboBuffer aVboBuffers[c_nMaxNumThreads];

							// update loop
							#ifdef SPEEDTREE_OPENMP
								#pragma omp parallel for num_threads(c_nMaxNumThreads)
							#endif
							for (st_int32 nBaseTree = nBatch; nBaseTree < nBatch + c_nThreadsInBatch; ++nBaseTree)
							{
								const CTreeRender* pBaseTree = (const CTreeRender*) aBaseTrees[nBaseTree];
								if (!pBaseTree->IsGrassModel( ))
									cCullResults.GetBaseTreeBillboardVboData(aVboBuffers[nBaseTree - nBatch], pBaseTree);
							}

							// copy loop
							for (st_int32 nBaseTree = nBatch; nBaseTree < nBatch + c_nThreadsInBatch; ++nBaseTree)
							{
								const CTreeRender* pBaseTree = (const CTreeRender*) aBaseTrees[nBaseTree];
								if (!pBaseTree->IsGrassModel( ))
									cCullResults.CopyVboDataToGpu(aVboBuffers[nBaseTree - nBatch], pBaseTree);
							}
						}
					}
				#else
					// billboards
					const TTreePtrArray& aBaseTrees = cAllTreeInstances.GetBaseTrees( );
					for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aBaseTrees.size( )); ++nBaseTree)
					{
						const CTreeRender* pBaseTree = (const CTreeRender*) aBaseTrees[nBaseTree];
						if (!pBaseTree->IsGrassModel( ))
						{
							// one possible option
							{
								// older versions of SpeedTree used this approach, but it's slower than below
								//cCullResults.UpdateBillboardInstanceBuffers(pBaseTree);
							}

							// another option (preferred)
							{
								// fill sVboBuffer with a buffer of continuous billboard instances 
								// belonging to pBaseTree
								SBillboardVboBuffer sVboBuffer;
								cCullResults.GetBaseTreeBillboardVboData(sVboBuffer, pBaseTree);

								// upload the billboard instances to the GPU
								cCullResults.CopyVboDataToGpu(sVboBuffer, pBaseTree);
							}
						}
					}
				#endif // SPEEDTREE_FAST_BILLBOARD_STREAMING
			}
		}
	}
	else
		cCullResults.Clear( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::StreamGrass
//
//	StreamGrass() shows an example implementation of how grass might be populated
//	on the app side. You're free to do it pretty much however you want, provided
//	the population is provided efficiently cell-by-cell.
//
//	Parameter details:
//
//	aGrassLayers - Each grass layer features one SpeedTree grass model and its
//				   own set of population rules. This is a "grass_population" in
//				   the SFC file.
//	cView - Contains the view frustum, be it the main camera or a light view.
//	cTerrain - Our example uses the terrain to generate grass at the correct
//			   heights (all grass adheres to the terrain).
//	nFrameIndex - A value kept by the application, it helps the SDK determine
//				  when cells are newly visible or newly culled.
//	aTmpBuffer - StreamGrass() needs a variable amount of heap space; passing
//				 a CArray for it to use prevents repetitive heap allocations,
//				 reducing potential heap fragmentation.
//	nShadowMapIndex - Defaults to -1 meaning it's a camera point of view,
//					  otherwise it's an index into which shadow map

void CMyPopulate::StreamGrass(CArray<CMyGrassLayer>& aGrassLayers,
							  const CView& cView,
							  const CMyTerrain& cTerrain,
							  st_int32 nFrameIndex,
							  TGrassInstArray& aTmpBuffer,
							  st_int32 nShadowMapIndex)
{
    ScopeTrace("CMyPopulate::StreamGrass");

	const st_bool c_bShadowPass = (nShadowMapIndex > -1);

	// random number generator declared and seeded here to be used by all cells; will result
	// in different populations for the same cell if visited twice. to avoid this, seed per
	// cell as commented nearer the end of this function
	CRandom cDice;

	// the SDK organizes the world as a series of cells. As the camera moves, cells go in and out 
	// of visibility. As cells become visible, the SDK will provide a list of these cells that need 
	// to have their populations streamed in. Hence, the SDK will focus most efficiently is the
	// client has the tree instances already organized by cells so that the data might be passed 
	// into the SDK without further on-the-fly processing.

	// experiment to boost speed
    //#ifdef SPEEDTREE_OPENMP
    //    // restrict the # of threads to no more than SPEEDTREE_OPENMP_MAX_THREADS
	//	st_int32 nAvailableThreads = omp_get_max_threads( );
	//	omp_set_num_threads(st_min(SPEEDTREE_OPENMP_MAX_THREADS, nAvailableThreads));
	//	#pragma omp parallel for
    //#endif
	for (st_int32 nLayer = 0; nLayer < st_int32(aGrassLayers.size( )); ++nLayer)
	{
        CMyGrassLayer* pLayer = &aGrassLayers[nLayer];
        const CTreeRender* pBaseGrass = pLayer->GetBaseGrass( );
        if (!pBaseGrass)
            continue;
		if (c_bShadowPass && !pLayer->GetPopulationParams( ).m_bCastShadows)
			continue;

		CVisibleInstancesRender& cVisibleGrass = c_bShadowPass ? pLayer->GetVisibleFromShadowMap(nShadowMapIndex) : pLayer->GetVisibleFromMainCamera( );

        // since there is only one frustum for the main camera, this section of code takes the main frustum
        // with its default far clip and makes a copy with with a much shorter far clip (represented by user-
        // configurable c_fGrassFarClip below)
        CView cGrassView;
		if (!c_bShadowPass)
        {
            ScopeTrace("PrepareView");
            Mat4x4 mGrassProjection = cView.GetProjection( );

            // adjust the grass projection matrix if left-handed to work with right-handed adjustments coming up
            if (CCoordSys::IsLeftHanded( ))
                mGrassProjection.Scale(1.0f, 1.0f, -1.0f);

		    // adjust frustum to closer far clip
		    const st_float32 c_fGrassFarClip = pBaseGrass->GetLodProfile( ).m_fLowDetail3dDistance;
		    mGrassProjection.AdjustPerspectiveNearAndFar(cView.GetNearClip( ), c_fGrassFarClip); 

		    // adjust back to left-handed if necessary and update cGrassView
		    if (CCoordSys::IsLeftHanded( ))
			    mGrassProjection.Scale(1.0f, 1.0f, -1.0f);
		    cGrassView.Set(cView.GetCameraPos( ), mGrassProjection, cView.GetModelview( ), cView.GetNearClip( ), c_fGrassFarClip);
        }
		else
			cGrassView = cView;

		// determine a rough/long list of cells in the frustum
		//
		// because the grass population is a streaming random set of instances, it cannot know a priori the complete
		// extents of the cells. specifically, it knows the width and height of the cells because they're in a grid 
		// layout, but it cannot know the height. RoughCullCells() will return a long list of cells that would be 
		// visible if the grid were composed of infinitely tall cells.
		st_bool bRoughCellListChanged = true;

		// determine a rough/long list of cells in the frustum
        {
            ScopeTrace("RoughCullCells");
		    bRoughCellListChanged = cVisibleGrass.RoughCullCells(cGrassView, nFrameIndex, pLayer->GetCullRadius( ));
        }
		if (!bRoughCellListChanged)
			continue;

		TCellArray& aRoughCells = cVisibleGrass.RoughCells( );
		if (!aRoughCells.empty( ))
		{
            ScopeTrace("SetCellExtents");

			// get frustum extents
			st_int32 nStartRow, nStartCol, nEndRow, nEndCol;
			cVisibleGrass.GetExtentsAsRowCols(nStartRow, nStartCol, nEndRow, nEndCol);
			const st_int32 c_nWidthInCells = (nEndCol - nStartCol) + 2; // +2 to include samples on both ends
			const st_int32 c_nHeightInCells = (nEndRow - nStartRow) + 2;

			// compute terrain height samples in a grid matching the extents
			CStaticArray<st_float32> aHeights(c_nWidthInCells * c_nHeightInCells, "StreamGrassPopulation::Heights");
			const st_float32 c_fCellSize = pLayer->GetPopulationParams( ).m_fCellSize;
			st_float32* pHeightPtr = &aHeights[0];

			st_float32 y = nStartRow * c_fCellSize;
			for (st_int32 nRow = 0; nRow < c_nHeightInCells; ++nRow)
			{
				st_float32 x = nStartCol * c_fCellSize;
				for (st_int32 nCol = 0; nCol < c_nWidthInCells; ++nCol)
				{
					*pHeightPtr++ = cTerrain.GetHeightFromXY(x, y);
					x += c_fCellSize;
				}
				y += c_fCellSize;
			}

			// compute the extents for each cell
			const st_float32 c_fGrassModelHeight = pBaseGrass->GetExtents( ).GetHeight( );
			pHeightPtr = &aHeights[0];
			CCell* pRoughCell = &aRoughCells[0];
			y = nStartRow * c_fCellSize;
			for (st_int32 nRow = 0; nRow < c_nHeightInCells - 1; ++nRow)
			{
				st_float32 x = nStartCol * c_fCellSize;
				for (st_int32 nCol = 0; nCol < c_nWidthInCells - 1; ++nCol)
				{
					const st_float32 c_fSample1 = *pHeightPtr;
					const st_float32 c_fSample2 = *(pHeightPtr + c_nWidthInCells + 1);

					Vec3 vMin(x, y, st_min(c_fSample1, c_fSample2));
					Vec3 vMax(x + c_fCellSize, y + c_fCellSize, st_max(c_fSample1, c_fSample2));

					// adjust for grass height
					vMax.z += c_fGrassModelHeight * pLayer->GetPopulationParams( ).m_afScalarRange[1];

					pRoughCell->SetExtents(CExtents(CCoordSys::ConvertFromStd(vMin), CCoordSys::ConvertFromStd(vMax)));

					++pRoughCell;
					++pHeightPtr;
					x += c_fCellSize;
				}

				++pHeightPtr;
				y += c_fCellSize;
			}
		}

		// using new extents, do a fine cull and update on cells
        {
            ScopeTrace("FineCullCells");
            cVisibleGrass.FineCullGrassCells(cGrassView, nFrameIndex, pLayer->GetCullRadius( ));
        }

		// fill the newly visible cells (those cells that are entering the frustum for the first time)
		// with the corresponding instances
		const st_int32 c_nNumNewCells = st_int32(cVisibleGrass.NewlyVisibleCells( ).size( ));
		if (c_nNumNewCells > 0)
		{
            {
                ScopeTrace("FillNewCells");
			    for (st_int32 i = 0; i < c_nNumNewCells; ++i)
			    {
				    CCell* pNewCell = cVisibleGrass.NewlyVisibleCells( )[i];
				    st_assert(pNewCell, "cVisibleGrass.NewlyVisibleCells( ) should never contain a NULL cell pointer");

				    // uncomment to seed per cell for consistent populations if a cell is populated (visited)
				    // more than once; avoided by default as seeding with our example random class is expensive
					cDice.Seed(pNewCell->UniqueRandomSeed( ) + nLayer);

				    // generate grasses for each grass type
					aTmpBuffer.resize(0);
				    st_int32 nNumInstances = CMyPopulate::GenerateRandomGrassInstances(aTmpBuffer,
																				       pBaseGrass, 
																				       pLayer, 
																				       pLayer->GetPopulationParams( ).m_fCellSize,
																				       pNewCell,
																				       cTerrain,
																				       cDice);
				    
					if (nNumInstances > 0)
					    pNewCell->SetGrassInstances(&aTmpBuffer[0], nNumInstances);
			    }
            }

			// update the grass instance vertex buffers
			//
			// unlike the 3d tree instances, grass instances have no LOD state as computed by the CPU, 
			// so the time is mostly spent turning the instance attributes into grass inst vertices inside
			// UpdateGrassInstanceBuffers()
			if (!cVisibleGrass.NewlyVisibleCells( ).empty( ))
			{
				ScopeTrace("UpdateGrassInstBuffers");
				cVisibleGrass.UpdateGrassInstanceBuffers(pBaseGrass);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//  FindBaseTreeBySrtFilename

CMyPopulate::SMyPerBasePopulation* FindBaseTreeBySrtFilename(const CFixedString& strSrtFilename, CArray<CMyPopulate::SMyPerBasePopulation>& aBaseTrees)
{
	CMyPopulate::SMyPerBasePopulation* pBaseTree = NULL;

	// simple linear search will suffice
	for (size_t i = 0; i < aBaseTrees.size( ); ++i)
	{
		if (aBaseTrees[i].m_strSrtFilename == strSrtFilename)
		{
			pBaseTree = &aBaseTrees[i];
			break;
		}
	}

	return pBaseTree;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::GenerateRandom3dTreeInstances

st_bool CMyPopulate::GenerateRandom3dTreeInstances(CArray<SMyPerBasePopulation>& aReturnInstances,
												   const CMyConfigFile::SRandomTreePopulationParams& sRandomParams,
												   st_float32 fSurfaceAdhesion,
												   const CMyTerrain& cTerrain)
{
	st_bool bSuccess = false;

	// extract SRT filename
	const char* pSrtFilename = sRandomParams.m_strSrtFilename.c_str( );

	SMyPerBasePopulation* pBaseTreeData = FindBaseTreeBySrtFilename(pSrtFilename, aReturnInstances);
	if (pBaseTreeData && pBaseTreeData->m_pBaseTree)
	{
		CTreeRender* pBaseTree = pBaseTreeData->m_pBaseTree;

		if (sRandomParams.m_nQuantity > 0)
		{
			CRandom cDice(sRandomParams.m_nSeed);

			// size the instance array for maximum possible instance count
			const st_int32 c_nStartingInstanceIndex = st_int32(pBaseTreeData->m_aInstances.size( ));
			pBaseTreeData->m_aInstances.resize(c_nStartingInstanceIndex + sRandomParams.m_nQuantity);

			// start placing the instances, but note there's no guarantee of placement as the terrain 
			// elevation/slope restrictions may make it impossible to place the requested quantity
			st_int32 nPlaced = 0;
			for (st_int32 i = 0; i < sRandomParams.m_nQuantity; ++i)
			{
				const st_int32 c_nMaxAttempts = 10000;
				st_int32 nAttempts = 0;

				st_bool bPlacementFound = false;
				while (!bPlacementFound && nAttempts < c_nMaxAttempts) 
				{
					++nAttempts;

					CTreeInstance& cInstance = pBaseTreeData->m_aInstances[c_nStartingInstanceIndex + i];
					cInstance.SetInstanceOf(pBaseTree);

					st_float32 x = cDice.GetFloat(sRandomParams.m_vArea[0], sRandomParams.m_vArea[1]);
					st_float32 y = cDice.GetFloat(sRandomParams.m_vArea[2], sRandomParams.m_vArea[3]);
					st_float32 z = 0.0f;

					// adjust z and possibly abort placement if terrain following is active
					if (sRandomParams.m_bFollowTerrain)
					{
						if (cTerrain.GetHeightFromPlacementParameters(z, x, y, sRandomParams.m_afElevationRange[0], sRandomParams.m_afElevationRange[1], sRandomParams.m_afSlopeRange[0], sRandomParams.m_afSlopeRange[1]))
							z = cTerrain.AdjustZPosBasedOnSlope(Vec3(x, y, z), *pBaseTree);
						else
							// failed to place instance at this (x,y) location based on specified elevation/slope range
							continue;
					}
					else
						z = cDice.GetFloat(sRandomParams.m_afElevationRange[0], sRandomParams.m_afElevationRange[1]);

					// set instance values
					Vec3 vPos(CCoordSys::ConvertFromStd(x, y, z));
					cInstance.SetPos(vPos);
					cInstance.SetScalar(cDice.GetFloat(sRandomParams.m_afScalarRange[0], sRandomParams.m_afScalarRange[1]));

					// get up vector as a function of both terrain normal and geometric up
					const Vec3 c_vGeometricUp = CCoordSys::UpAxis( );
					const Vec3 c_vTerrainUp = CCoordSys::ConvertFromStd(cTerrain.GetNormalFromXY(x, y));
					const Vec3 c_vUp = Interpolate(c_vGeometricUp, c_vTerrainUp, fSurfaceAdhesion);

					// generate random right vector by creating random normal and crossing it against up
					Vec3 vRight(cDice.GetFloat(-1.0f, 1.0f), cDice.GetFloat(-1.0f, 1.0f), cDice.GetFloat(-1.0f, 1.0f));
					vRight = c_vUp.Cross(vRight);
					cInstance.SetOrientation(c_vUp, vRight);

					// compute cull parameters now that other attributes are set
					cInstance.ComputeCullParameters( ); 

					bPlacementFound = true;
					++nPlaced;

					if (nAttempts == c_nMaxAttempts)
					{
						Warning("CMyPopulate::GenerateRandomInstances, placement parameters for [%s] to restrictive for efficient placement; [%d] instances placed", pBaseTree->GetFilename( ), nPlaced);
						pBaseTreeData->m_aInstances.resize(c_nStartingInstanceIndex + nPlaced);
						break;
					}
				}
			}

			bSuccess = (pBaseTreeData->m_aInstances.size( ) - c_nStartingInstanceIndex == size_t(sRandomParams.m_nQuantity));
		}

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::GenerateRandomGrassInstances
//
//
//  This is merely an example population function that happens to suit 
//	the needed of the speedtree reference application. No doubt a more 
//  optimized population function could be written for your application's
//  specific needs.
//
//	Returns the number of generated grass instances.

st_int32 CMyPopulate::GenerateRandomGrassInstances(TGrassInstArray& aInstances,
												   const CTree* pBaseTree,
												   const CMyGrassLayer* pLayer,
												   st_float32 fGrassCellSize,
												   CCell* pCell,
												   const CMyTerrain& cTerrain,
												   CRandom& cDice)
{
	const CMyConfigFile::SGrassPopulationParams& sParams = pLayer->GetPopulationParams( );

	// get cell extents
	const st_float32 c_fCellMinX = pCell->Col( ) * fGrassCellSize;
	const st_float32 c_fCellMaxX = c_fCellMinX + fGrassCellSize;
	const st_float32 c_fCellMinY = pCell->Row( ) * fGrassCellSize;
	const st_float32 c_fCellMaxY = c_fCellMinY + fGrassCellSize;

	// if masked, don't populate
	if (pLayer->IsCellMasked(c_fCellMinX, c_fCellMaxX, c_fCellMinY, c_fCellMaxY))
		return 0;

	// determine surface adhesion -- if grass layer's is negative, use the global value
	const st_float32 c_fSurfaceAdhesion = pLayer->GetPopulationParams( ).m_fSurfaceAdhesion;

	// setup instance array
	const st_float32 c_fCellArea = fGrassCellSize * fGrassCellSize;
	st_int32 c_nNumAttempts = st_int32(sParams.m_fDensity * c_fCellArea);
	CGrassInstance* pInstance = NULL;

	// attempt to place a number of instances based on user-specified density
	if (c_nNumAttempts > 0)
	{
		const Vec3 vTerrainSize = cTerrain.GetSize( );

		aInstances.resize(c_nNumAttempts);
		pInstance = &aInstances[0];

		Vec3 vPos;
		for (st_int32 i = 0; i < c_nNumAttempts; ++i)
		{
			// determine position on terrain, height corrected
			vPos.x = cDice.GetFloat(c_fCellMinX, c_fCellMaxX);
			vPos.y = cDice.GetFloat(c_fCellMinY, c_fCellMaxY);

			if (!pLayer->IsPositionMasked(vPos.x, vPos.y, cDice, vTerrainSize) &&
				cTerrain.GetHeightFromPlacementParameters(vPos.z, vPos.x, vPos.y, sParams.m_afElevationRange[0], sParams.m_afElevationRange[1], sParams.m_afSlopeRange[0], sParams.m_afSlopeRange[1]))
			{
				pInstance->SetInstanceOf(pBaseTree);
				pInstance->SetPos(CCoordSys::ConvertFromStd(vPos));
				pInstance->SetScalar(cDice.GetFloat(sParams.m_afScalarRange[0], sParams.m_afScalarRange[1]));

				// set random rotation
				const Vec3 c_vGeometricUp = CCoordSys::UpAxis( );
				const Vec3 c_vTerrainUp = CCoordSys::ConvertFromStd(cTerrain.GetNormalFromXY(vPos.x, vPos.y));
				const Vec3 c_vUp = Interpolate(c_vGeometricUp, c_vTerrainUp, c_fSurfaceAdhesion);

				// generate random right vector by creating random normal and crossing it against up
				Vec3 vRight(cDice.GetFloat(-1.0f, 1.0f), cDice.GetFloat(-1.0f, 1.0f), cDice.GetFloat(-1.0f, 1.0f));
				vRight = c_vUp.Cross(vRight);
				pInstance->SetOrientation(c_vUp, vRight);

				++pInstance;
			}
		}
	}

	return pInstance ? st_int32(pInstance - &aInstances[0]) : 0;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::PreScanStfFile

st_bool CMyPopulate::PreScanStfFile(const CMyConfigFile::SStfTreePopulationParams& sParams, CArray<SMyPerBasePopulation>& aBaseTrees)
{
	st_bool bSuccess = false;

	// constants
	const char* pFilename = sParams.m_strFilename.c_str( );

	// extract STF path so we can build a complete path to the STF file
	CFixedString strPath = CFixedString(pFilename).Path( );

	// open the file
	st_int32 nAttemptedReads = 0, nSuccessfulReads = 0;            
	FILE* pFile = fopen(pFilename, "r");
	if (pFile)
	{
		// find the end of the file
		fseek(pFile, 0L, SEEK_END);
		st_int32 nEnd = st_int32(ftell(pFile));
		fseek(pFile, 0L, SEEK_SET);

		st_int32 nTree = 0;
		while (ftell(pFile) != nEnd)
		{
			// keep track of the tree for error reporting
			++nTree;

			// read the mandatory data
			char szName[1024];
			st_int32 nNumInstances = 0;
			st_float32 fMaxScalar = 0.0f;
			nAttemptedReads++;
			if (fscanf(pFile, "%s %d\n", szName, &nNumInstances) == 2)
			{
				nSuccessfulReads++;
				nAttemptedReads += nNumInstances;

				for (st_int32 i = 0; i < nNumInstances; ++i)
				{
					// read the instance location, but discard
					Vec3 vPos;
					st_float32 fRotation = 0.0f;
					st_float32 fScalar = 1.0f;
					if (fscanf(pFile, "%g %g %g %g %g\n", &vPos.x, &vPos.y, &vPos.z, &fRotation, &fScalar) == 5)
					{
						fMaxScalar = st_max(fMaxScalar, fScalar);
						++nSuccessfulReads;
					}
					else
						CCore::SetError("Error reading STF instance data [tree %d in %s]", nTree, pFilename);
				}

				SMyPerBasePopulation sBaseTreeData;

				sBaseTreeData.m_strSrtFilename = strPath + szName;
				sBaseTreeData.m_fLodScalar = (fMaxScalar == 0.0f) ? 1.0f : fMaxScalar;
				sBaseTreeData.m_fAmbientImageScalar = sParams.m_fAmbientImageScalar;
				sBaseTreeData.m_sHueVariationParams = sParams.m_sHueVariationParams;

				aBaseTrees.insert_sorted_unique(sBaseTreeData);
			}
			else
				CCore::SetError("Error reading STF base data [tree %d in %s]", nTree, pFilename);
		}
		fclose(pFile);

		bSuccess = nAttemptedReads == nSuccessfulReads && nAttemptedReads > 0;
	}
	else
	{
		char szError[512];
		st_strerror(szError, 512, errno);
		CCore::SetError("Failed to open STF file [%s]: %s", pFilename, szError);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  ReadStringFromFile

st_bool ReadStringFromFile(CFixedString& strString, FILE* pFile)
{
	strString.resize(0);

	// skip whitespace
	st_char chTemp;
	do
	{
		chTemp = st_char(fgetc(pFile));
	}
	while (chTemp != EOF && isspace(chTemp));

	if (chTemp == '"')
	{
		chTemp = st_char(fgetc(pFile));
		while (chTemp != '"' && chTemp != EOF && chTemp != '\n')
		{
			strString += chTemp;
			chTemp = st_char(fgetc(pFile));
		}
	}
	else
	{
		strString += chTemp;
		chTemp = st_char(fgetc(pFile));
		while (!isspace(chTemp) && chTemp != EOF)
		{
			strString += chTemp;
			chTemp = st_char(fgetc(pFile));
		}
	}

	return !strString.empty( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::PreScanSwaFile

st_bool CMyPopulate::PreScanSwaFile(const CMyConfigFile::SSwaTreePopulationParams& sParams, CArray<SMyPerBasePopulation>& aBaseTrees)
{
	st_bool bSuccess = false;

	// constants
	const char* pFilename = sParams.m_strFilename.c_str( );

	// extract SWA path so we can build a complete path to the SWA file
	CFixedString strPath = CFixedString(pFilename).Path( );

	// open the file
	st_int32 nAttemptedReads = 0, nSuccessfulReads = 0;            
	FILE* pFile = fopen(pFilename, "r");
	if (pFile)
	{
		// find the end of the file
		fseek(pFile, 0L, SEEK_END);
		st_int32 nEnd = st_int32(ftell(pFile));

		// return to the beginning
		fseek(pFile, 0L, SEEK_SET);

		st_int32 nTree = 0;
		while (ftell(pFile) != nEnd)
		{
			// keep track of the tree for error reporting
			++nTree;

			// read the mandatory data
			CFixedString strName;
			st_int32 nNumInstances = 0;
			st_float32 fMaxScalar = 0.0f;
			nAttemptedReads++;
			if (ReadStringFromFile(strName, pFile) && fscanf(pFile, "%d\n", &nNumInstances) == 1)
			{
				nSuccessfulReads++;
				nAttemptedReads += nNumInstances;

				for (st_int32 i = 0; i < nNumInstances; ++i)
				{
					// read the instance location, but discard
					Vec3 vPos, vRight, vUp;
					st_float32 fScalar = 1.0f;
					if (fscanf(pFile, "%g %g %g %g %g %g %g %g %g %g\n", &vPos.x, &vPos.y, &vPos.z, &vUp.x, &vUp.y, &vUp.z, &vRight.x, &vRight.y, &vRight.z, &fScalar) == 10)
					{
						fMaxScalar = st_max(fMaxScalar, fScalar);
						++nSuccessfulReads;
					}
					else
						CCore::SetError("Error reading SWA instance data [tree %d in %s]", nTree, pFilename);
				}

				SMyPerBasePopulation sBaseTreeData;

				sBaseTreeData.m_strSrtFilename = strPath + c_szFolderSeparator + strName;
				sBaseTreeData.m_fLodScalar = (fMaxScalar == 0.0f) ? 1.0f : fMaxScalar;
				sBaseTreeData.m_fAmbientImageScalar = sParams.m_fAmbientImageScalar;
				sBaseTreeData.m_sHueVariationParams = sParams.m_sHueVariationParams;

				aBaseTrees.insert_sorted_unique(sBaseTreeData);
			}
			else
				CCore::SetError("Error reading SWA base data [tree %d in %s]", nTree, pFilename);
		}
		fclose(pFile);

		bSuccess = nAttemptedReads == nSuccessfulReads && nAttemptedReads > 0;
	}
	else
	{
		char szError[512];
		st_strerror(szError, 512, errno);
		CCore::SetError("Failed to open SWA file [%s]: %s", pFilename, szError);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::PreScanSpeedForestFile

st_bool CMyPopulate::PreScanSpeedForestFile(const CMyConfigFile::SSpeedForestPopulationParams& sParams, CArray<SMyPerBasePopulation>& aBaseTrees)
{
	st_bool bSuccess = false;

	// scan the file to determine the max instance scale used
	st_float32 fMaxScalar = 0.0f;

	const char* pFilename = sParams.m_strSpeedForestFilename.c_str( );
	FILE* pFile = fopen(pFilename, "r");
	if (pFile)
	{
		Vec3 vPos, vRight, vUp;
		st_float32 fScalar = 1.0f;
		while (fscanf(pFile, "%g %g %g %g %g %g %g %g %g %g\n", &vPos.x, &vPos.y, &vPos.z, &vUp.x, &vUp.y, &vUp.z, &vRight.x, &vRight.y, &vRight.z, &fScalar) == 10)
		{
			fMaxScalar = st_max(fMaxScalar, fScalar);
		}

		fclose(pFile);
		bSuccess = true;
	}
	else
	{
		char szError[512];
		st_strerror(szError, 512, errno);
		CCore::SetError("Failed to open SpeedForest file [%s]: %s", pFilename, szError);
	}

	// for SpeedForest files, the base trees are specified in the SFC file, not the SpeedForest file
	SMyPerBasePopulation sBaseTreeData;
	sBaseTreeData.m_strSrtFilename = sParams.m_strSrtFilename;
	sBaseTreeData.m_fLodScalar = st_max(sParams.m_fLodScalar, fMaxScalar);
	sBaseTreeData.m_fAmbientImageScalar = sParams.m_fAmbientImageScalar;
	sBaseTreeData.m_sHueVariationParams = sParams.m_sHueVariationParams;

	aBaseTrees.insert_sorted_unique(sBaseTreeData);

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::,Instances
//
//	This function will read an ASCII STF file and generate arrays of CTreeInstance
//	objects that match the parameters in the file.  There are a couple of
//	important usage notes:
//
//	GetStfFileInstances() expects an array of CTreeRender* base trees prior to
//	generating the instance list.  This requires a pre-scan of the STF file, which
//	can be performed by CMyPopulate::PreScanStfFile().
//
//	The instances are generated and stored in what is essentially a 2D array in
//	aaInstances.  The first dimension matches the order of base trees as passed
//	in aPopulationByBaseTree which should match the base tree declaration order in the STF
//	file.  The second dimension contains the instances in the same order they
//	appears in the STF file.

st_bool CMyPopulate::GetStfFileInstances(const CMyConfigFile::SStfTreePopulationParams& sParams,
									     CArray<CMyPopulate::SMyPerBasePopulation>& aPopulationByBaseTree,
										 const CMyTerrain& cTerrain)
{
	st_bool bSuccess = false;

	// extract filename & path
	const char* pFilename = sParams.m_strFilename.c_str( );
	CFixedString strPath = CFixedString(pFilename).Path( );

	// open the file
	FILE* pFile = fopen(pFilename, "r");
	if (pFile)
	{
		// find the end of the file
		fseek(pFile, 0L, SEEK_END);
		st_int32 nEnd = st_int32(ftell(pFile));
		fseek(pFile, 0L, SEEK_SET);

		st_int32 nAttemptedReads = 0;
		st_int32 nSuccessfulReads = 0;            
		st_int32 nTree = 0;
		while (ftell(pFile) != nEnd)
		{
			// read the mandatory data
			st_char szName[1024];
			st_int32 nNumInstances = 0;
			st_float32 afPos[3] = { 0.0f };
			nAttemptedReads++;
			if (fscanf(pFile, "%s %d\n", szName, &nNumInstances) == 2)
			{
				nSuccessfulReads++;
				nAttemptedReads += nNumInstances;

				// lookup base tree data object based on SRT filename
				SMyPerBasePopulation* pBaseTreeData = FindBaseTreeBySrtFilename(strPath + szName, aPopulationByBaseTree);
				if (pBaseTreeData)
				{
					// get base tree
					CTreeRender* pBaseTree = pBaseTreeData->m_pBaseTree;
					st_assert(pBaseTree, "GetStfFileInstances() expects that all base trees have already been loaded");

					// instances
					const st_int32 c_nStartingInstanceIndex = st_int32(pBaseTreeData->m_aInstances.size( ));
					pBaseTreeData->m_aInstances.resize(c_nStartingInstanceIndex + nNumInstances);
					for (st_int32 i = 0; i < nNumInstances; ++i)
					{
						// read the instance location
						st_float32 fRotation = 0.0f;
						st_float32 fScalar = 1.0f;

						if (fscanf(pFile, "%g %g %g %g %g\n", afPos, afPos + 1, afPos + 2, &fRotation, &fScalar) == 5) // rotation in radians
						{
							if (sParams.m_bFollowTerrain)
							{
								afPos[2] = cTerrain.GetHeightFromXY(afPos[0], afPos[1]);
								afPos[2] = cTerrain.AdjustZPosBasedOnSlope(Vec3(afPos), *pBaseTree);
							}

							CTreeInstance& cInstance = pBaseTreeData->m_aInstances[c_nStartingInstanceIndex + i];

							// set instance parameters
							cInstance.SetPos(CCoordSys::ConvertFromStd(Vec3(afPos)));
							cInstance.SetScalar(fScalar);
							cInstance.SetInstanceOf(pBaseTree);

							// adjust orientation based on rotation value (user value is in radians)
							Vec3 cRightVector = CCoordSys::RightAxis( );
							Mat4x4 mRotationMatrix;
							mRotationMatrix.RotateZ(fRotation);
							cInstance.SetOrientation(CCoordSys::UpAxis( ), mRotationMatrix * cRightVector);

							// update cull now that everything's set
							cInstance.ComputeCullParameters( );

							++nSuccessfulReads;
						}
						else
							CCore::SetError("Error reading STF instance data [tree %d in %s]", nTree, pFilename);
					}
				}
				else
				{
					CCore::SetError("CMyPopulate::GetStfFileInstances, base trees passed in don't match those in the STF file");
					break;
				}
			}
			else
				CCore::SetError("Error reading STF base data [tree %d in %s]", nTree, pFilename);
		}

		fclose(pFile);
		bSuccess = nAttemptedReads == nSuccessfulReads && nAttemptedReads > 0;
	}
	else
	{
		char szError[512];
		st_strerror(szError, 512, errno);
		CCore::SetError("Failed to open STF file [%s]: %s", pFilename, szError);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::GetSwaFileInstances

st_bool CMyPopulate::GetSwaFileInstances(const CMyConfigFile::SSwaTreePopulationParams& sParams,
										 CArray<CMyPopulate::SMyPerBasePopulation>& aPopulationByBaseTree,
										 const CMyTerrain& cTerrain)
{
	st_bool bSuccess = false;

	// extract filename & path
	const char* pFilename = sParams.m_strFilename.c_str( );
	CFixedString strPath = CFixedString(pFilename).Path( );

	// open the file
	FILE* pFile = fopen(pFilename, "r");
	if (pFile)
	{
		// find the end of the file
		fseek(pFile, 0L, SEEK_END);
		st_int32 nEnd = st_int32(ftell(pFile));
		fseek(pFile, 0L, SEEK_SET);

		st_int32 nAttemptedReads = 0;
		st_int32 nSuccessfulReads = 0;            
		st_int32 nTree = 0;
		while (ftell(pFile) != nEnd)
		{
			// read the mandatory data
			CFixedString strName;
			st_int32 nNumInstances = 0;
			nAttemptedReads++;
			
			if (ReadStringFromFile(strName, pFile) && fscanf(pFile, "%d\n", &nNumInstances) == 1)
			{
				nSuccessfulReads++;
				nAttemptedReads += nNumInstances;

				// lookup base tree data object based on SRT filename
				CFixedString strFullName = strPath + c_szFolderSeparator + strName;
				SMyPerBasePopulation* pBaseTreeData = FindBaseTreeBySrtFilename(strFullName, aPopulationByBaseTree);
				if (pBaseTreeData)
				{
					CTreeRender* pBaseTree = pBaseTreeData->m_pBaseTree;
					st_assert(pBaseTree, "GetSwaFileInstances() expects that all base trees have already been loaded");

					// instances
					const st_int32 c_nStartingInstanceIndex = st_int32(pBaseTreeData->m_aInstances.size( ));
					pBaseTreeData->m_aInstances.resize(c_nStartingInstanceIndex + nNumInstances);
					for (st_int32 i = 0; i < nNumInstances; ++i)
					{
						// read the instance location
						Vec3 vPos, vRight, vUp;
						st_float32 fScalar = 1.0f;

						if (fscanf(pFile, "%g %g %g %g %g %g %g %g %g %g\n", &vPos.x, &vPos.y, &vPos.z, &vUp.x, &vUp.y, &vUp.z, &vRight.x, &vRight.y, &vRight.z, &fScalar) == 10)
						{
							if (sParams.m_bFollowTerrain)
							{
								vPos.z = cTerrain.GetHeightFromXY(vPos.x, vPos.y);
								vPos.z = cTerrain.AdjustZPosBasedOnSlope(vPos, *pBaseTree);
							}

							CTreeInstance& cInstance = pBaseTreeData->m_aInstances[c_nStartingInstanceIndex + i];

							// set instance parameters
                            cInstance.SetPos(CCoordSys::ConvertFromStd(vPos));
							cInstance.SetScalar(fScalar);
							cInstance.SetInstanceOf(pBaseTree);
							cInstance.SetOrientation(CCoordSys::ConvertFromStd(vUp), CCoordSys::ConvertFromStd(vRight));

							// update cull now that everything's set
							cInstance.ComputeCullParameters( );
							++nSuccessfulReads;
						}
						else
							CCore::SetError("Error reading SWA instance data [tree %d in %s]", nTree, pFilename);
					}
				}
				else
				{
					CCore::SetError("CMyPopulate::GetSwaFileInstances, base trees passed in don't match those in the SWA file");
					break;
				}
			}
			else
				CCore::SetError("Error reading SWA base data [tree %d in %s]", nTree, pFilename);
		}

		fclose(pFile);
		bSuccess = nAttemptedReads == nSuccessfulReads && nAttemptedReads > 0;
	}
	else
	{
		char szError[512];
		st_strerror(szError, 512, errno);
		CCore::SetError("Failed to open SWA file [%s]: %s", pFilename, szError);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::GetSpeedForestFileInstances

st_bool CMyPopulate::GetSpeedForestFileInstances(const CMyConfigFile::SSpeedForestPopulationParams& sParams,
												 CArray<CMyPopulate::SMyPerBasePopulation>& aPopulationByBaseTree,
												 const CMyTerrain& cTerrain)
{
	st_bool bSuccess = false;

	// extract filename & path
	const char* pFilename = sParams.m_strSpeedForestFilename.c_str( );
	CFixedString strPath = CFixedString(pFilename).Path( );

	// lookup base tree data object based on SRT filename
	SMyPerBasePopulation* pBaseTreeData = FindBaseTreeBySrtFilename(sParams.m_strSrtFilename, aPopulationByBaseTree);
	if (pBaseTreeData)
	{
		// open the file
		FILE* pFile = fopen(pFilename, "r");
		if (pFile)
		{
			CTreeRender* pBaseTree = pBaseTreeData->m_pBaseTree;
			st_assert(pBaseTree, "GetSwaFileInstances() expects that all base trees have already been loaded");

			// read the instance location
			Vec3 vPos, vRight, vUp;
			st_float32 fScalar = 1.0f;
			while (fscanf(pFile, "%g %g %g %g %g %g %g %g %g %g\n", &vPos.x, &vPos.y, &vPos.z, &vUp.x, &vUp.y, &vUp.z, &vRight.x, &vRight.y, &vRight.z, &fScalar) == 10)
			{
				if (sParams.m_bFollowTerrain)
				{
					vPos.z = cTerrain.GetHeightFromXY(vPos.x, vPos.y);
					vPos.z = cTerrain.AdjustZPosBasedOnSlope(vPos, *pBaseTree);
				}

				CTreeInstance cInstance;

				// set instance parameters
				cInstance.SetPos(CCoordSys::ConvertFromStd(vPos));
				cInstance.SetScalar(fScalar);
				cInstance.SetInstanceOf(pBaseTree);
				cInstance.SetOrientation(CCoordSys::ConvertFromStd(vUp), CCoordSys::ConvertFromStd(vRight));

				// update cull now that everything's set
				cInstance.ComputeCullParameters( );

				// add to base tree's instances
				pBaseTreeData->m_aInstances.push_back(cInstance);
			}

			fclose(pFile);
			bSuccess = true;
		}
		else
		{
			char szError[512];
			st_strerror(szError, 512, errno);
			CCore::SetError("Failed to open SpeedForest file [%s]: %s", pFilename, szError);
		}
	}
	else
		Internal("CMyPopulate::GetSpeedForestFileInstances, base tree passed in does not match the one specified in the SFC file");

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::GetManualInstances

st_bool CMyPopulate::GetManualInstances(const CMyConfigFile::SManualTreePopulationParams& sParams,
										CArray<CMyPopulate::SMyPerBasePopulation>& aPopulationByBaseTree,
										const CMyTerrain& cTerrain)
{
	st_bool bSuccess = false;

	// lookup base tree data object based on SRT filename
	SMyPerBasePopulation* pBaseTreeData = FindBaseTreeBySrtFilename(sParams.m_strSrtFilename, aPopulationByBaseTree);
	if (pBaseTreeData)
	{
		const st_int32 c_nStartingInstanceIndex = st_int32(pBaseTreeData->m_aInstances.size( ));
		pBaseTreeData->m_aInstances.resize(c_nStartingInstanceIndex + sParams.m_aInstances.size( ));

		for (st_int32 i = 0; i < st_int32(sParams.m_aInstances.size( )); ++i)
		{
			const CMyConfigFile::SManualTreePopulationParams::SInstance& sUserInstance = sParams.m_aInstances[i];
			CTreeInstance& cAppInstance = pBaseTreeData->m_aInstances[c_nStartingInstanceIndex + i];

			// set instance parameters
			st_assert(pBaseTreeData->m_pBaseTree, "GetManualInstances() expects that all base trees have already been loaded");
			cAppInstance.SetInstanceOf(pBaseTreeData->m_pBaseTree);
			cAppInstance.SetScalar(sUserInstance.m_fScalar);
			cAppInstance.SetOrientation(CCoordSys::ConvertFromStd(sUserInstance.m_vUp), CCoordSys::ConvertFromStd(sUserInstance.m_vRight));

			// assign position, adjusted by terrain if requested
			Vec3 vPos = sUserInstance.m_vPos;
			if (sParams.m_bFollowTerrain)
			{
				// move z position to lie on top of terrain
				vPos.z = cTerrain.GetHeightFromXY(vPos.x, vPos.y);

				// push tree into ground a bit
				vPos.z = cTerrain.AdjustZPosBasedOnSlope(vPos, *pBaseTreeData->m_pBaseTree);
			}
            cAppInstance.SetPos(CCoordSys::ConvertFromStd(vPos));

			// update cull now that everything's set
			cAppInstance.ComputeCullParameters( );
		}

		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CMyPopulate::InitBaseTreeGraphics

st_bool CMyPopulate::InitBaseTreeGraphics(CTreeRender* pBaseTree, const CForestRender& cForest)
{
	st_bool bSuccess = false;

	if (pBaseTree)
	{
		// setup search paths for textures & shaders
		const st_int32 c_nMaxExpectedPaths = 5;
		CStaticArray<CFixedString> aSearchPaths(c_nMaxExpectedPaths, "InitBaseTreeGraphics", false);
		{
			// add no path (most helpful when dealing with memory blocks, not actual files)
			aSearchPaths.push_back("");

			const CFixedString c_strSrtPath = CFixedString(pBaseTree->GetFilename( )).Path( );

			// add shared shader path, if used; it's relative to the SRT's path
			const char* pShaderPath = pBaseTree->GetGeometry( )->m_strShaderPath;
			if (pShaderPath && strlen(pShaderPath) > 0)
				aSearchPaths.push_back(c_strSrtPath + c_szFolderSeparator + CFixedString(pShaderPath) + c_szFolderSeparator + CShaderTechnique::GetCompiledShaderFolder( ));

			// same path as the SRT model
			aSearchPaths.push_back(c_strSrtPath);

			// shader path right off of the SRT model (Compiler writes here by default, changes per platform)
			aSearchPaths.push_back(c_strSrtPath + CShaderTechnique::GetCompiledShaderFolder( ));
		}

		bSuccess = pBaseTree->InitGfx(cForest.GetRenderInfo( ).m_sAppState, 
									  aSearchPaths, 
									  cForest.GetRenderInfo( ).m_nMaxAnisotropy,
									  cForest.GetRenderInfo( ).m_fTextureAlphaScalar3d);

		if (bSuccess)
		{
			// delete the CCore copy of the geometry now that it's been used to create the
			// vertex and index buffers
			//
			// * note that geometry will not be accessible after this call; it's not strictly
			//   necessary but does free some likely-unused memory
			pBaseTree->DeleteGeometry( );
		}
		else
			CCore::SetError("Failed to initialize model [%s]", pBaseTree->GetFilename( ));
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::LoadBaseTree

st_bool CMyPopulate::LoadBaseTree(CMyPopulate::SMyPerBasePopulation& sBaseTree, st_bool bDeferredRenderMode)
{
	st_bool bSuccess = true;

	// allocate
	sBaseTree.m_pBaseTree = st_new(CTreeRender, "CTreeRender");

	// load
	CFixedString strSrtFilename = CFileSystem::CleanPlatformFilename(sBaseTree.m_strSrtFilename);
	// last parameter, size scalar, is 1.0 (scaling is done in this example on a per-instance basis)
	if (sBaseTree.m_pBaseTree->LoadTree(strSrtFilename.c_str( ), sBaseTree.m_bGrassModel, 1.0f)) 
	{
		if (RunSanityTests(sBaseTree, bDeferredRenderMode))
		{
			Report("   SRT file [%s] OK", strSrtFilename.c_str( ));

			// set parameters specified in the SFC file
			sBaseTree.m_pBaseTree->SetAmbientImageScalar(sBaseTree.m_fAmbientImageScalar);
			sBaseTree.m_pBaseTree->SetHueVariationParams(sBaseTree.m_sHueVariationParams);

			bSuccess = true;
		}
		else
			bSuccess = false;
	}
	else
	{
		CCore::SetError("Failed to load SRT file [%s]", strSrtFilename.c_str( ));
		bSuccess = false;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyPopulate::RunSanityTests
//
//	True means the sanity test passed.

st_bool CMyPopulate::RunSanityTests(const SMyPerBasePopulation& sBaseTree, st_bool bDeferredRenderMode)
{
	st_bool bPassed = true;

	if (sBaseTree.m_pBaseTree)
	{
		// test to see if the model was compiled for grass but used as a general tree, or vice-versa
		{
			st_bool bCompiledForGrass = sBaseTree.m_pBaseTree->IsCompiledAsGrass( );

			// issue warning if compiled as grass but used as a general tree
			if (bCompiledForGrass && !sBaseTree.m_bGrassModel)
			{
				CCore::SetError("SRT file [%s] is compiled as grass model, but used as general 3D tree", sBaseTree.m_strSrtFilename.c_str( ));
				bPassed = false;
			}
			// issue warning if compiled as general tree model but used as grass
			else if (!bCompiledForGrass && sBaseTree.m_bGrassModel)
			{
				CCore::SetError("SRT file [%s] is compiled as general 3D tree model, but used as grass", sBaseTree.m_strSrtFilename.c_str( ));
				bPassed = false;
			}
		}

		// test to see if models were compiled for deferred but SDK is set for forward rendering, or vice-versa
		{
			if (sBaseTree.m_pBaseTree->IsCompiledForDeferred( ) != bDeferredRenderMode)
			{
				CCore::SetError("SRT file [%s] is compiled for %s rendering, but the reference app is configured for %s rendering",
					sBaseTree.m_strSrtFilename.c_str( ),
					sBaseTree.m_pBaseTree->IsCompiledForDeferred( ) ? "deferred" : "forward",
					bDeferredRenderMode ? "deferred" : "forward");
				bPassed = false;
			}
		}
	}

	return bPassed;
}






