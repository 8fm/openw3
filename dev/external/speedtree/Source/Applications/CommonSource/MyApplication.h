///////////////////////////////////////////////////////////////////////
//  MyApplication.h
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
#include "MyRenderTargets.h"
#include "MyShadowSystem.h"
#include "MyGrass.h"
#include "MyTerrain.h"
#include "MySky.h"
#include "MyOverlays.h"
#include "MyCmdLineOptions.h"
#include "MyStatsReporter.h"
#include "MyParamAdjuster.h"
#include "MyFullscreenQuad.h"
#if (defined(_WIN32) || defined(__GNUC__) || defined(__APPLE__)) && !defined(_XBOX) && !defined(__CELLOS_LV2__) && !defined(_DURANGO) && !defined(__ORBIS__)
	#ifdef _WIN32
		#define SPEEDTREE_WINDOWS // but not Xbox
	#endif
	#define SPEEDTREE_MOUSE_AND_KEYBOARD
	#include "MyMouseNavigation.h"
#else
	#include "MyNavigationBase.h"
#endif
// todo: disable
//#define HAND_TUNED_SYSTEM_ACTIVE


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////
	//  Enumeration EBloomDisplayMode

	enum EBloomDisplayMode
	{
		BLOOM_DISPLAY_MODE_FULL,
		BLOOM_DISPLAY_MODE_NONE,
		BLOOM_DISPLAY_MODE_OVERLAY,
		BLOOM_DISPLAY_MODE_COUNT
	};


	///////////////////////////////////////////////////////////////////////
	//  Class CMyApplication
	//
	//	Note that this class is merely an example of how a forest might be initialized;
	//	none of the methods that we have chosen for tree placement & population are
	//	required by the SpeedTree libraries.  You're free to implement it however you like.

	class CMyApplication
	{
	public:
											CMyApplication( );
											~CMyApplication( );

			// main functions; initialization/rendering
			st_bool							ParseCmdLine(st_int32 argc, char* argv[]);
			st_bool							Init(void);	// application configuration (non-graphics resources)
			st_bool							ReadyToRender(void) const;
			st_bool							InitGfx(void);
			st_bool                     	InitForwardSupportGfx(void);
			st_bool                     	InitDeferredSupportGfx(void);
			st_bool							InitPostEffectsGfx(void);
			void							ReleaseGfxResources(void);
			void							Render(void);
			void							DeferredRender(void);
			void							ForwardRender(void);
			void							ForwardRenderWithBloom(void);
			void							ForwardRenderWithDepthPrepass(void);
			void							ForwardRenderWithManualMsaaResolve(void);
			void							ForwardRenderWithManualMsaaResolveWithDepthPrepass(void);
			void							EnableTextureAlphaMode(void);
			void							SetTextureAlphaScalars(st_float32 fScalar3d, st_float32 fScalarGrass, st_float32 fScalarBillboard);
			void							Advance(void);						// called once per frame
			void							Cull(void);							// called once per frame
			void							HandleCollision(void);
			void							ReportStats(void);
			void							ReportResourceUsage(void);
			const SMyCmdLineOptions&		GetCmdLineOptions(void) const		{ return m_sCmdLine; }	// peek into command-line settings
			const CMyConfigFile&			GetConfig(void) const				{ return m_cConfigFile; }
			CForestRender*					GetForest(void)						{ return &m_cForest; }

			void							SetGamma(st_float32 fGamma);

			CMyNavigationBase*				GetNavigation(void)					{ return m_pCurrentNavigation; }
			CRenderStats&					GetRenderStats(void)				{ return m_cForest.GetRenderStats( ); }
			st_float32						GetLastFrameTime(void)				{ return m_fFrameTime; }

		#ifdef SPEEDTREE_MOUSE_AND_KEYBOARD
			void							KeyDown(st_uchar chKey, st_int32 x, st_int32 y);
			void							KeyUp(st_uchar chKey, st_int32 x, st_int32 y);
			void							MouseClick(CMyNavigationBase::EMouseButton eButton, st_bool bPressed, st_int32 x, st_int32 y);
			void							MouseMotion(st_int32 x, st_int32 y);
			void							WindowReshape(st_int32 nWindowWidth, st_int32 nWindowHeight);
	static	void							PrintKeys(void);
		#endif
	static	void							PrintId(void);

			// render adjustment
			CMyParamAdjuster&				GetLightAdjuster(void)				{ return m_sLightAdjuster; }
			void							ToggleTrees(void)					{ m_bRenderTrees = !m_bRenderTrees; }
			void							ToggleBillboards(void)				{ m_bRenderBillboards = !m_bRenderBillboards; }
			void							ToggleGrass(void)					{ m_bRenderGrass = !m_bRenderGrass; }
			void							ToggleTerrain(void)					{ m_cTerrain.SetActive(!m_cTerrain.IsActive( )); m_bCameraChanged = true; }
			void							ToggleTerrainFollowing(void)		{ m_bFollowTerrain = !m_bFollowTerrain; NotifyOfPopulationChange( ); }
			void							ToggleOverlays(void)				{ m_bRenderOverlays = !m_bRenderOverlays; }
			void							UpdateLightDir(void);
			void							UpdateGlobalLod(void);
			void							UpdateHandTune(void);

			// force DirectX9 resources to reset
			void							OnResetDevice(void);
			void							OnLostDevice(void);

	private:
			// forest population utilities; InitGfx() will call PopulateForest()
			// which will call the other population functions based on the population
			// method is set on command-line
			st_bool							PopulateForest(void);
			void							SetCullCellSizes(void);
			void							SetHeapReserves(void);
			st_bool							SetUpGrassLayers(void);
			void							StreamGrassPopulation(void);
			void							NotifyOfPopulationChange(void);

			// render support
			void							StartBloomPostPass(void);
			void							EndBloomPostPass(void);

			// the main forest render function; includes shadow mapping pass
			void							UpdateView(void);
			void							UpdateTimeOfDay(void);

			// diagnostic/stressing functions
			void							AdjustPopulation(void);

			// main objects
			CForestRender					m_cForest;					// main forest object; type is defined in PlatformSpecifics.h
			CMyShadowSystem					m_cShadowSystem;
			CMyTerrain						m_cTerrain;
			STerrainCullResults				m_sTerrainCullResults;
			CMySky							m_cSky;
			CMyOverlays						m_cOverlays;
			SMyCmdLineOptions				m_sCmdLine;					// command-line options store here
			CMyConfigFile					m_cConfigFile;
			CMyStatsReporter				m_cStatsReporter;			// used to report rendering stats to the console

            // deferred rendering
            CMyDeferredRenderTargets		m_cDeferredTarget;
			CRenderState					m_cDeferredState;
			CMyFullscreenQuad				m_cDeferredRenderResolveQuad;

			// optional bloom post fx
			CMyForwardTargets				m_cBloomForwardTarget;
			CRenderTarget					m_cBloomAuxTargetA;
			CRenderTarget					m_cBloomAuxTargetB;
			CRenderState					m_cBloomHiPassState;
			CMyFullscreenQuad           	m_cBloomHiPassQuad;
			CRenderState					m_cBloomBlurState;
			CMyFullscreenQuad           	m_cBloomBlurQuad;
			CRenderState					m_cBloomFinalState;
			CMyFullscreenQuad           	m_cBloomFinalQuad;
			CRenderState					m_cFullscreenAlpaState;
			CMyFullscreenQuad           	m_cFullscreenAlpaQuad;
			CShaderConstantBuffer			m_cBloomConstantBuffer;
			SBloomCBLayout					m_sBloomConstantBufferLayout;

			// optional depth blur post fx
			CRenderTarget					m_cDepthBlurTarget;
			CRenderState					m_cDepthBlurState;
	        CMyFullscreenQuad           	m_cDepthBlurQuad;

			// explicit resolve support (durango)
			CMyForwardTargets				m_cExplicitMsaaResolveTarget;
			CRenderState					m_cExplicitMsaaResolveState;
			CMyFullscreenQuad				m_cExplicitMsaaResolveQuad;
			st_bool							m_bExplicitMsaaResolveEnabled;

			// population
			CMyInstancesContainer			m_cAllTreeInstances;
			SHeapReserves					m_sHeapReserves;

			// view & culling
			CView							m_cView;
			st_bool							m_bCameraChanged;
			CVisibleInstancesRender			m_cVisibleTreesFromCamera;

			// grass
			CArray<CMyGrassLayer>			m_aGrassLayers;
			TGrassInstArray					m_aRandomGrassInstancesBuffer;

			// keep track of all the base trees & grass models so that their wind
			// can be updated together and can all be deleted on destruction
			TTreePtrArray					m_aAllBaseTreesAndGrass;
			CArray<CTreeRender*>			m_aAllBaseTrees;
			CArray<CTreeRender*>			m_aAllBaseGrasses;

			// navigation
		#ifdef SPEEDTREE_MOUSE_AND_KEYBOARD
			CMyMouseNavigation				m_cMyMouseNavigation;		// mouse-based navigation controls housed here
			st_bool							m_bIgnoreNextMouseMove;		// needed to support WASD navigation
			st_int32						m_anMousePos[2];
		#else
			CMyNavigationBase				m_cOtherNavigation;
		#endif
			CMyNavigationBase*				m_pCurrentNavigation;		// defaults to pointing to m_CMyMouseNavigation; can be used for other nav objects
			st_bool							m_bFollowTerrain;

			// time management
			st_float32						m_fCurrentTime;				// time since initialization in seconds
			CTimer							m_cWallClock;				// real-time wall clock
			st_float32						m_fFrameTime;				// time it took to render the last frame in seconds
			st_float32						m_fTimeMarker;				// used to calculate m_fFrameTime in Advance()
			st_int32						m_nFrameIndex;

			// state management
			st_bool							m_bRenderTrees;				// toggle rendering of 3D tree geometry
			st_bool							m_bRenderBillboards;		// toggle rendering of billboards
			st_bool							m_bRenderGrass;
			st_bool							m_bRenderOverlays;
			ETextureAlphaRenderMode			m_eTransparentTextureRenderMode;
			st_bool							m_bRenderTransparentMultiPass;
			st_bool							m_bHandleCollision;
			st_bool							m_bDepthBlur;
			st_int32						m_nBloomDisplayMode;

			// parameter adjustment via 	mouse controls
			CMyParamAdjuster				m_sLightAdjuster;
			CMyParamAdjuster				m_sWindAdjuster;
			CMyParamAdjuster				m_sLodAdjuster;
			CMyParamAdjuster				m_asHandTuneAdjuster[4];
			typedef CMap<CTree*, SLodProfile> TOrigLodMap;
			TOrigLodMap						m_mOrigLodsMap;

			// window management
			st_bool							m_bReadyToRender;
			st_float32						m_fAspectRatio;				// window width / window height
		#ifdef _WIN32
			HWND							m_hWindow;					// necessary for WASD navigation
		#endif
	};

} // end namespace SpeedTree

