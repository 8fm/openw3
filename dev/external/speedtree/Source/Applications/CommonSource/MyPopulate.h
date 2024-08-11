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

#pragma once
#include "MyConfigFile.h"
#include "MyGrass.h"
#include "MySpeedTreeRenderer.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Forward references

	class CMyTerrain;


	///////////////////////////////////////////////////////////////////////  
	//  Structure SPerCellData
	//
	//	The forest is organized as a series of cells.  Each cell contains a list
	//	of base trees that are rendered in that cell and a list of instances of
	//	each base tree.

	struct SPerCellData
	{
									SPerCellData( );

			CExtents				m_cExtents;
			TTreeInstConstPtrArray	m_aInstances;
			TTreePtrArray			m_aBaseTrees;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Forward references

	struct SMyCmdLineOptions;
	class CMyTerrain;


	///////////////////////////////////////////////////////////////////////  
	//  Class CMyInstancesContainer

	class CMyInstancesContainer
	{
	public:
									CMyInstancesContainer( );
									~CMyInstancesContainer( );
	
			// init
			void					SetMaxNumBaseTrees(st_int32 nMaxNumBaseTrees);
			st_bool					SetInstances(CTree* pBaseTree, const TTreeInstArray& aInstances);
			void					SplitIntoCells(st_float32 fCellSize);
			void					Clear(void);

			// simple queries
			st_bool					IsEmpty(void) const;
			SPerCellData*			Cell(const SCellKey& sKey);
			const TTreePtrArray&	GetBaseTrees(void) const;
			const TTreeInstArray2D&	GetInstancesPerBaseTree(void) const;
			st_float32				GetLongestCellOverhang(void) const;

			// computationally non-trivial queries
			void					ComputeExtents(CExtents& cExtents) const;
			st_int32				ComputeTotalInstances(void) const;
			st_int32				ComputeMaxNumInstancesInAnyCell(void) const;
			void					ComputeHeapReserveGuess(SHeapReserves& sReserves, const CMyConfigFile& cConfigFile, st_float32 fScreenAspectRatio) const;

			// dynamic population testing
			void					DoublePopulation(const CMyConfigFile& cConfigFile, const CMyTerrain& cTerrain);
			void					HalvePopulation(void);
			void					AnimateFigure(st_float32 fGlobalTime, const CExtents& cArea, st_float32 fTreeSpacing, const CMyTerrain& cTerrain);

	private:

			// instances and base trees stored here
			TTreePtrArray			m_aBaseTrees;
			TTreeInstArray2D		m_aaInstances;

			// instance pointers stored per cell to speed CForest 
			// cell on-the-fly population
			CArray<SPerCellData>	m_aCells;

			st_int32				m_nFirstCol;
			st_int32				m_nLastCol;
			st_int32				m_nNumCols;
			st_int32				m_nFirstRow;
			st_int32				m_nLastRow;
			st_int32				m_nNumRows;

			st_float32				m_fCellSize;

			// culling parameters
			st_float32				m_fLongestCellOverhang;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CMyPopulate

	class CMyPopulate
	{
	public:
                // SMyPerBasePopulation is a collection of per-base tree data needed 
				struct SMyPerBasePopulation
				{
															SMyPerBasePopulation( ) :
																m_pBaseTree(NULL),
																m_bGrassModel(false),
																m_fLodScalar(1.0f),
																m_fAmbientImageScalar(1.0f)
															{
																m_aInstances.SetHeapDescription("SMyPerBasePopulation::m_aInstances");
															}

					st_bool									operator<(const SMyPerBasePopulation& sRight) const { return m_strSrtFilename < sRight.m_strSrtFilename; }

					CFixedString							m_strSrtFilename;
					CTreeRender*							m_pBaseTree;
					st_bool									m_bGrassModel;
					TTreeInstArray							m_aInstances;
					st_float32								m_fLodScalar;
					st_float32								m_fAmbientImageScalar;
					CCore::SHueVariationParams				m_sHueVariationParams;
				};

								CMyPopulate( );
								~CMyPopulate( );

	static	void				StreamTrees(CMyInstancesContainer& cAllTreeInstances,
											const CView& cView,
											CVisibleInstancesRender& cCullResults,
											st_int32 nFrameIndex,
											st_bool b3dTreesVisible,
											st_bool bBillboardsVisible,
											st_bool bShadowPass);
	static	void				StreamGrass(CArray<CMyGrassLayer>& aGrassLayers,
											const CView& cView,
											const CMyTerrain& cTerrain,
											st_int32 nFrameIndex,
											TGrassInstArray& aTmpBuffer,
											st_int32 nShadowMapIndex = -1);


			// these functions will generate arrays of instances, but will not actually
			// populate a forest or load CTree objects
	static	st_bool				GenerateRandom3dTreeInstances(CArray<SMyPerBasePopulation>& aReturnInstances,
															  const CMyConfigFile::SRandomTreePopulationParams& sRandomParams,
															  st_float32 fSurfaceAdhesion,
															  const CMyTerrain& cTerrain);
	static	st_int32			GenerateRandomGrassInstances(TGrassInstArray& aReturnInstances,
															 const CTree* pBaseTree,
															 const CMyGrassLayer* pLayer,
															 st_float32 fGrassCellSize,
															 CCell* pCell,
															 const CMyTerrain& cTerrain,
															 CRandom& cDice);
	static	st_bool				PreScanStfFile(const CMyConfigFile::SStfTreePopulationParams& sParam, CArray<SMyPerBasePopulation>& aBaseTrees);
	static	st_bool				PreScanSwaFile(const CMyConfigFile::SSwaTreePopulationParams& sParam, CArray<SMyPerBasePopulation>& aBaseTrees);
	static	st_bool				PreScanSpeedForestFile(const CMyConfigFile::SSpeedForestPopulationParams& sParam, CArray<SMyPerBasePopulation>& aBaseTrees);
	static	st_bool				GetStfFileInstances(const CMyConfigFile::SStfTreePopulationParams& sParams,
													CArray<CMyPopulate::SMyPerBasePopulation>& aBaseTrees,
													const CMyTerrain& cTerrain);
	static	st_bool				GetSwaFileInstances(const CMyConfigFile::SSwaTreePopulationParams& sParams,
													CArray<CMyPopulate::SMyPerBasePopulation>& aBaseTrees,
													const CMyTerrain& cTerrain);
	static	st_bool				GetSpeedForestFileInstances(const CMyConfigFile::SSpeedForestPopulationParams& sParams,
															CArray<CMyPopulate::SMyPerBasePopulation>& aBaseTrees,
															const CMyTerrain& cTerrain);
	static	st_bool				GetManualInstances(const CMyConfigFile::SManualTreePopulationParams& sParams,
													CArray<CMyPopulate::SMyPerBasePopulation>& aBaseTrees,
													const CMyTerrain& cTerrain);

			// given an array of base tree filenames, this function will load the base trees into the cForest object and return an array of their pointers
	static	st_bool				LoadBaseTree(SMyPerBasePopulation& sBaseTree, st_bool bDeferredRenderMode);
	static	st_bool				InitBaseTreeGraphics(CTreeRender* pBaseTree, const CForestRender& cForest);

	static	st_bool				RunSanityTests(const SMyPerBasePopulation& sBaseTree, st_bool bDeferredRenderMode);
	};

	#include "MyPopulate_inl.h"

} // end namespace SpeedTree



