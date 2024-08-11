///////////////////////////////////////////////////////////////////////  
//  MyShadowSystem.h
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
#include "MySpeedTreeRenderer.h"
#include "MyConfigFile.h"
#include "MyPopulate.h"
#include "MyTerrain.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Class CMyShadowSystem

	class CMyShadowSystem
	{
	public:
											CMyShadowSystem( );
											~CMyShadowSystem( );

			void							SetHeapReserves(const SHeapReserves& sHeapReserves);
			void							SetCullCellSize(st_float32 f3dTreeCellSize);
			st_bool							Init(const CMyConfigFile& cConfigFile);
			st_bool							InitGfx(const CMyConfigFile& cConfigFile, const CArray<CTreeRender*>& aBaseTrees);
			void							ReleaseGfxResources(void);

			st_bool							UpdateView(const CMyConfigFile& cConfigFile, 
													   const CView& cMainCameraView, 
													   const Vec3& vLightDir,
													   CMyTerrain& cTerrain,
													   CArray<CMyGrassLayer>& aGrassLayers,
													   TGrassInstArray& aTmpBuffer,
													   st_int32 nFrameIndex,
                                                       st_bool bForceUpdate,
													   CMyInstancesContainer& cAllTreeInstances);
			void							NotifyOfPopulationChange(void);

			st_bool							RenderIntoMaps(const CMyConfigFile& cConfigFile, 
														   CForestRender& cForest,
														   CArray<CMyGrassLayer>& aGrassLayers,
														   CMyTerrain& cTerrain,
														   st_int32 nFrameIndex,
														   st_bool bCastersActive,
														   st_bool bForceRender);
			st_bool							BindAsSourceTextures(const CMyConfigFile& cConfigFile, st_int32 nRegisterOffset);
			void							BindBlankShadowsAsSourceTextures(void);
			void							UnBindAsSourceTextures(const CMyConfigFile& cConfigFile, st_int32 nRegisterOffset);

			const CVisibleInstancesRender&	GetVisibleInstances(st_int32 nShadowMapIndex) const;

			void							OnResetDevice(const CMyConfigFile& cConfigFile);
			void							OnLostDevice(const CMyConfigFile& cConfigFile);

	private:
			CRenderTarget					m_acShadowMaps[c_nMaxNumShadowMaps];
			CView							m_acLightViews[c_nMaxNumShadowMaps];
			CVisibleInstancesRender			m_acVisibleTreesFromLight[c_nMaxNumShadowMaps];
			STerrainCullResults				m_asTerrainCullResults[c_nMaxNumShadowMaps];
			st_bool							m_bShadowMapsReady;

			// if off, use white
			CTexture						m_cWhiteTexture;
	};

} // end namespace SpeedTree
