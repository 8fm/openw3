///////////////////////////////////////////////////////////////////////  
//  MyTerrain.h
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
#define MY_TERRAIN_ACTIVE // if not defined, will disable all initialization and rendering of all terrain geometry
#include "MySpeedTreeRenderer.h"
#include "MyCmdLineOptions.h"
#include "MyTerrainData.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////
	//  Structure STerrainVertex

	struct STerrainVertex
	{
			Vec3		m_vCoords;
			st_float32	m_afTexCoords[3]; // texcoord_s, texcoord_t, ambient occlusion
	};


	// if compiled w/o terrain active, use stubbed-out class definition to keep the
	// MyApplication.* files from getting too cluttered
	#ifdef MY_TERRAIN_ACTIVE


	///////////////////////////////////////////////////////////////////////  
	//  Class CMyTerrain

	class CMyTerrain : public CTerrainRender
	{
	public:
       										CMyTerrain( );
	virtual									~CMyTerrain( );

			// flags
			void							SetActive(st_bool bActive);
			st_bool							IsActive(void) const;

			// initialization
			st_bool							Init(const CMyConfigFile& cConfigFile, const SMyCmdLineOptions& sCmdLine);
			void							SetHeapReserves(const SHeapReserves& sHeapReserves);
			void							ComputeAmbientOcclusion(const CMyConfigFile& cConfigFile, const CMyInstancesContainer& cInstances);

			// rendering
			void							CullAndPopulate(const CView& cView, st_int32 nFrameIndex, STerrainCullResults& sResults);
			st_bool							Render(ERenderPass ePass,
												   const STerrainCullResults& sResults,
												   const SSimpleMaterial& sLightMaterial, 
												   CRenderStats& cStats);

			// other queries
			const Vec3&						GetSize(void) const	 { return m_cData.GetSize( ); }
			void							GetHeightRange(st_float32 afHeightRange[2]) const;
			st_float32						GetHeightFromXY(st_float32 x, st_float32 y) const;
			st_float32						GetSmoothHeightFromXY(st_float32 x, st_float32 y, st_float32 fSmoothingDistance) const;
			st_float32						GetSlopeFromXY(st_float32 x, st_float32 y) const;
			Vec3							GetNormalFromXY(st_float32 x, st_float32 y) const;
			st_bool							GetHeightFromPlacementParameters(st_float32& fHeight, 
																			 st_float32 x, 
																			 st_float32 y, 
																			 st_float32 fMinHeight, 
																			 st_float32 fMaxHeight,
																			 st_float32 fMinSlope = 0.0f, 
																			 st_float32 fMaxSlope = 1.0f) const;

			// utilities
			st_float32						AdjustZPosBasedOnSlope(const Vec3& vOrigPos, CTree& sBaseTree) const;

	private:
			void							Populate(STerrainCullResults& sResults);

			SVertexDecl						m_sVertexDecl;
			st_bool							m_bActive;
			CMyTerrainData					m_cData;
	};

	// include inline functions
	#include "MyTerrain_inl.h"


	#else // MY_TERRAIN_ACTIVE


	///////////////////////////////////////////////////////////////////////  
	//  Class CMyTerrain

	class CMyTerrain : public CTerrainRender
	{
	public:
 			struct SCullAndPopulateTimes
			{
											SCullAndPopulateTimes( ) { }

				st_float32					m_fCullTime;		// in ms
				st_float32					m_fPopulationTime;	// in ms
			};

											CMyTerrain( ) { }
	virtual									~CMyTerrain( ) { }

			// flags
			void							SetActive(st_bool) { }
			st_bool							IsActive(void) { return false; }

			// initialization
			st_bool							Init(const CMyConfigFile&, const SMyCmdLineOptions&) { return true; }
			void							ComputeAmbientOcclusion(const CMyConfigFile&, const CMyInstancesContainer&) { }
			
			// render loop
			void							CullAndPopulate(const CView&, st_int32, STerrainCullResults&) { }
			const SCullAndPopulateTimes&	GetCullAndPopulateTimes(void) const { static SCullAndPopulateTimes sTimes; return sTimes; }
			st_bool							Render(const SRenderPass&, 
												   const STerrainCullResults&, 
												   const Vec3&, 
												   const SSimpleMaterial&, 
												   CRenderStats&) const { return true; }

			// other queries
			const Vec3&						GetSize(void) const	{ static Vec3 vZero; return vZero; }
			void							GetHeightRange(st_float32 afHeightRange[2]) const { afHeightRange[0] = afHeightRange[1] = 0.0f; }
			st_float32						GetHeightFromXY(st_float32, st_float32) const { return 0.0f; }
			st_float32						GetSmoothHeightFromXY(st_float32, st_float32, st_float32) const { return 0.0f; }
			st_float32						GetSlopeFromXY(st_float32, st_float32) const { return 0.0f; }
			Vec3							GetNormalFromXY(st_float32, st_float32) const { return CCoordSys::UpAxis( ); }
			st_bool							GetHeightFromPlacementParameters(st_float32& fHeight, 
																			 st_float32, 
																			 st_float32, 
																			 st_float32, 
																			 st_float32,
																			 st_float32 fMinSlope = 0.0f, 
																			 st_float32 fMaxSlope = 1.0f) const { (void)(fMinSlope); (void)(fMaxSlope); fHeight = 0.0f; return true; };
			const STerrainCullResults&		GetCullResults(void) const { static STerrainCullResults sResults; return sResults; }
																			 
			// utilities
			st_float32						AdjustZPosBasedOnSlope(const Vec3&, CTree&) const { return 0.0f; }
	};

	#endif // MY_TERRAIN_ACTIVE

} // end namespace SpeedTree

