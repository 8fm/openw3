///////////////////////////////////////////////////////////////////////  
//  MyGrass.h
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
#include "Forest/Forest.h"
#include "MySpeedTreeRenderer.h"
#include "MyConfigFile.h"
#include "My2dInterpolator.h"


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Class CMyGrassLayer

	class CMyGrassLayer
	{
	public:
	friend class CMyApplication;
															CMyGrassLayer( );
															~CMyGrassLayer( );
	
			void											ReleaseGfxResources(void);

			void											SetPopulationParams(const CMyConfigFile::SGrassPopulationParams& sPopulation);
			const CMyConfigFile::SGrassPopulationParams&	GetPopulationParams(void) const { return m_sPopulationParams; }
			void											NotifyOfPopulationChanged(void);

			CTreeRender*									GetBaseGrass(void) { return m_pBaseGrass; }
			st_float32										GetCullRadius(void) { return m_fCullRadius; }

			CVisibleInstancesRender&						GetVisibleFromMainCamera(void) { return m_cVisibleGrassMainCamera; }
			CVisibleInstancesRender&						GetVisibleFromShadowMap(st_int32 nMapIndex) { return m_acVisibleGrassShadowMaps[nMapIndex]; }

			st_bool											IsPositionMasked(st_float32 fX, st_float32 fY, CRandom& cDice, const Vec3& vecMaskSize) const;
			st_bool											IsCellMasked(st_float32 fCellMinX, st_float32 fCellMaxX, st_float32 fCellMinY, st_float32 fCellMaxY) const;

	private:

			// population
			CMyConfigFile::SGrassPopulationParams			m_sPopulationParams;
			CMy2dInterpolator<st_uchar>						m_cMask;

			// culling
			CVisibleInstancesRender							m_cVisibleGrassMainCamera;
			CVisibleInstancesRender							m_acVisibleGrassShadowMaps[c_nMaxNumShadowMaps];

			// geometry & render
			CTreeRender*									m_pBaseGrass;
			st_float32										m_fCullRadius;
	};

} // end namespace SpeedTree


