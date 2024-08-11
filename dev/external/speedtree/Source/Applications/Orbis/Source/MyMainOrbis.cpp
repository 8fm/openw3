///////////////////////////////////////////////////////////////////////  
//  MyMainOrbis.cpp
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

#define __CELL_ASSERT__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <pad.h>
#include <user_service.h>
//#include <razor.h>
#include <video_out.h>
#include "Utilities/toolkit/toolkit.h"

#pragma comment(lib,"libSceUserService_stub_weak.a")


#define SPEEDTREE_USE_ALLOCATOR_INTERFACE // must be active for Debug MT and Release MT builds
#define SPEEDTREE_OVERRIDE_FILESYSTEM
#include "MyCustomAllocator.h"

#include "MyApplication.h"
#include "MyFileSystem.h"
#include "Renderers/Orbis/OrbisRenderer.h"
#include "MyCmdLineOptions.h"


// Set default heap size
size_t sceLibcHeapSize = 512 * 1024 * 1024;
unsigned int sceLibcHeapExtendedAlloc = 1; // Enable

using namespace SpeedTree;
using namespace sce;


///////////////////////////////////////////////////////////////////////  
//  Global variables

#ifdef SPEEDTREE_USE_ALLOCATOR_INTERFACE
		// passing the allocator to a static member helps get it placed as
		// soon as possible to hopefully trap all relevant heap allocations
static	CMyCustomAllocator		g_cCustomAllocator;
static	CAllocatorInterface		g_cAllocatorInterface(&g_cCustomAllocator);
#endif

struct VideoInfo
{
	int32_t handle;
	uint64_t* label;
	uint32_t label_num;
	uint32_t flip_index;
	uint32_t buffer_num;
	SceKernelEqueue eq;
};
VideoInfo g_sVideoInfo;

struct SFrameBuffer
{
	Gnmx::GfxContext			m_cContext;
	sce::Gnm::RenderTarget		m_cRenderTarget;
	sce::Gnm::RenderTarget		m_cMultisampledRenderTarget;
	sce::Gnm::DepthRenderTarget	m_cDepthTarget;
	volatile uint64_t*			m_pLabel;
	uint32_t					m_uiExpectedLabel;
};

#define FRAME_BUFFER_NUM 2
SFrameBuffer	g_aBuffers[FRAME_BUFFER_NUM];
uint32_t		g_uiBufferIndex = 0;
uint32_t		g_uiFrameCount = 0;

volatile uint64_t* g_pLabels = NULL;

static	CMyApplication*			g_pApplication = NULL;

static st_bool					g_bRunning = false;

static int32_t					g_iControllerHandle = -1;


///////////////////////////////////////////////////////////////////////  
//  InitDisplay

static st_bool InitDisplay(void)
{
	int iWidth = g_pApplication->GetCmdLineOptions( ).m_nWindowWidth;
	int iHeight = g_pApplication->GetCmdLineOptions( ).m_nWindowHeight;

	Gnmx::Toolkit::MemoryRequests sToolkitMemoryRequests;
	sToolkitMemoryRequests.initialize( );
	Gnmx::Toolkit::addToMemoryRequests(&sToolkitMemoryRequests);
	sToolkitMemoryRequests.m_garlic.fulfill(Orbis::Allocate(sToolkitMemoryRequests.m_garlic.m_sizeAlign.m_size, sToolkitMemoryRequests.m_garlic.m_sizeAlign.m_align, true));
	sToolkitMemoryRequests.m_onion.fulfill(Orbis::Allocate(sToolkitMemoryRequests.m_onion.m_sizeAlign.m_size, sToolkitMemoryRequests.m_onion.m_sizeAlign.m_align, false));
	Gnmx::Toolkit::initializeWithMemoryRequests(&sToolkitMemoryRequests);

	g_pLabels = const_cast<volatile uint64_t*>(static_cast<uint64_t*>(Orbis::Allocate(sizeof(uint64_t) * FRAME_BUFFER_NUM, sizeof(uint64_t), true)));


	Gnm::NumSamples eSamples = Gnm::kNumSamples1;
	Gnm::NumFragments eFragments = Gnm::kNumFragments1;
	bool bMultisampled = false;
	if (g_pApplication->GetCmdLineOptions( ).m_nNumSamples == 2)
	{
		eSamples = Gnm::kNumSamples2;
		eFragments = Gnm::kNumFragments2;
		bMultisampled = true;
	}
	else if (g_pApplication->GetCmdLineOptions( ).m_nNumSamples == 4)
	{
		eSamples = Gnm::kNumSamples4;
		eFragments = Gnm::kNumFragments4;
		bMultisampled = true;
	}

	void* aBufferAddress[8];
	for (st_uint32 uiBuffer = 0; uiBuffer < FRAME_BUFFER_NUM; ++uiBuffer)
	{
		g_pLabels[uiBuffer] = 0xFFFFFFFF;
		g_aBuffers[uiBuffer].m_pLabel = &g_pLabels[uiBuffer];
		g_aBuffers[uiBuffer].m_uiExpectedLabel = 0xFFFFFFFF;

		// Currently, display render targets must be BGRA 8888 UNORM, LinearAligned
		Gnm::DataFormat eFormat = Gnm::kDataFormatB8G8R8A8Unorm;
		Gnm::TileMode tileMode;
		GpuAddress::computeSurfaceTileMode(&tileMode, GpuAddress::kSurfaceTypeColorTargetDisplayable, eFormat, 1);
		const Gnm::SizeAlign cSizeAlign = g_aBuffers[uiBuffer].m_cRenderTarget.init(iWidth, iHeight, 1, eFormat, tileMode, Gnm::kNumSamples1, Gnm::kNumFragments1, NULL, NULL);
		void* pRenderTarget = Orbis::Allocate(cSizeAlign.m_size, cSizeAlign.m_align, true);
		aBufferAddress[uiBuffer] = pRenderTarget;
		g_aBuffers[uiBuffer].m_cRenderTarget.setAddresses(pRenderTarget, 0, 0);

		if (bMultisampled)
		{
			// multisampled render target
			Gnm::TileMode tileMode;
			GpuAddress::computeSurfaceTileMode(&tileMode, GpuAddress::kSurfaceTypeColorTargetDisplayable, eFormat, 1 << eFragments);
			Gnm::SizeAlign sCMaskSizeAlign;
			Gnm::SizeAlign sFMaskSizeAlign;
			const Gnm::SizeAlign cSizeAlign = g_aBuffers[uiBuffer].m_cMultisampledRenderTarget.init(iWidth, iHeight, 1, eFormat, tileMode, eSamples, eFragments, &sCMaskSizeAlign, &sFMaskSizeAlign);
			void* pRenderTarget = Orbis::Allocate(cSizeAlign.m_size, cSizeAlign.m_align, true);
			g_aBuffers[uiBuffer].m_cMultisampledRenderTarget.setAddresses(pRenderTarget, 0, 0);

			void* pCMaskAddr = Orbis::Allocate(sCMaskSizeAlign.m_size, sCMaskSizeAlign.m_align, true);
			void* pFMaskAddr = Orbis::Allocate(sFMaskSizeAlign.m_size, sFMaskSizeAlign.m_align, true);
			SCE_GNM_ASSERT(sCMaskSizeAlign.m_size && pCMaskAddr || !sCMaskSizeAlign.m_size);
			SCE_GNM_ASSERT(sFMaskSizeAlign.m_size && pFMaskAddr || !sFMaskSizeAlign.m_size);
			g_aBuffers[uiBuffer].m_cMultisampledRenderTarget.setCmaskAddress(pCMaskAddr);
			g_aBuffers[uiBuffer].m_cMultisampledRenderTarget.setFmaskAddress(pFMaskAddr);
			g_aBuffers[uiBuffer].m_cMultisampledRenderTarget.setFmaskCompressionEnable(true);
			g_aBuffers[uiBuffer].m_cMultisampledRenderTarget.setCmaskFastClearEnable(true);
		}

		// depth target, multisampled if needed
		const Gnm::StencilFormat eStencilFormat = Gnm::kStencilInvalid;
		Gnm::TileMode eDepthTileMode;
		Gnm::DataFormat cDepthFormat = Gnm::DataFormat::build(Gnm::kZFormat32Float);
		GpuAddress::computeSurfaceTileMode(&eDepthTileMode, GpuAddress::kSurfaceTypeDepthOnlyTarget, cDepthFormat, 1 << eFragments);
		const Gnm::SizeAlign cDepthTargetSizeAlign = g_aBuffers[uiBuffer].m_cDepthTarget.init(iWidth, iHeight, cDepthFormat.getZFormat( ), eStencilFormat, eDepthTileMode, eFragments, NULL, NULL);

		void* pDepthBuffer = Orbis::Allocate(cDepthTargetSizeAlign.m_size, cDepthTargetSizeAlign.m_align, true);
		g_aBuffers[uiBuffer].m_cDepthTarget.setAddresses(pDepthBuffer, 0);

		// create context
		const uint32_t kNumRingEntries = 16;
		const uint32_t uiCpRamShadowSize = Gnmx::ConstantUpdateEngine::computeCpRamShadowSize( );
		const uint32_t uiHeapSize = Gnmx::ConstantUpdateEngine::computeHeapSize(kNumRingEntries);
		g_aBuffers[uiBuffer].m_cContext.init(malloc(uiCpRamShadowSize),
											Orbis::Allocate(uiHeapSize, sce::Gnm::kAlignmentOfBufferInBytes, true), kNumRingEntries,
											Orbis::Allocate(sce::Gnm::kIndirectBufferMaximumSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes, false), sce::Gnm::kIndirectBufferMaximumSizeInBytes,
											Orbis::Allocate(sce::Gnm::kIndirectBufferMaximumSizeInBytes, sce::Gnm::kAlignmentOfBufferInBytes, false), sce::Gnm::kIndirectBufferMaximumSizeInBytes);
	}

	Orbis::SetContext(&g_aBuffers[0].m_cContext);

	const uint32_t kPlayerId = 0;
	g_sVideoInfo.handle = sceVideoOutOpen(kPlayerId, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);
	SCE_GNM_ASSERT(g_sVideoInfo.handle >= 0);
	// Prepare Equeue for Flip Sync
	sceKernelCreateEqueue(&g_sVideoInfo.eq, __FUNCTION__);
	sceVideoOutAddFlipEvent(g_sVideoInfo.eq, g_sVideoInfo.handle, NULL);
	g_sVideoInfo.flip_index = 0;
	g_sVideoInfo.buffer_num = FRAME_BUFFER_NUM;

	SceVideoOutBufferAttribute sAttribute;
	sceVideoOutSetBufferAttribute(&sAttribute,
								SCE_VIDEO_OUT_PIXEL_FORMAT_B8_G8_R8_A8_SRGB,
								SCE_VIDEO_OUT_TILING_MODE_TILE,
								SCE_VIDEO_OUT_ASPECT_RATIO_16_9,
								g_aBuffers[0].m_cRenderTarget.getWidth( ),
								g_aBuffers[0].m_cRenderTarget.getHeight( ),
								g_aBuffers[0].m_cRenderTarget.getPitch( ));
	sceVideoOutRegisterBuffers(g_sVideoInfo.handle, 0, aBufferAddress, FRAME_BUFFER_NUM, &sAttribute);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  Flip

void Flip(void)
{
	++g_uiFrameCount;
	g_aBuffers[g_uiBufferIndex].m_uiExpectedLabel = g_uiFrameCount;

	if (g_aBuffers[g_uiBufferIndex].m_cMultisampledRenderTarget.getBaseAddress( ) != NULL)
		sce::Gnmx::Toolkit::SurfaceUtil::resolveMsaaBuffer(*Orbis::Context( ), &g_aBuffers[g_uiBufferIndex].m_cRenderTarget, &g_aBuffers[g_uiBufferIndex].m_cMultisampledRenderTarget);

	Orbis::Context( )->writeImmediateAtEndOfPipe(Gnm::kEopFlushCbDbCaches, const_cast<uint64_t*>(g_aBuffers[g_uiBufferIndex].m_pLabel), g_aBuffers[g_uiBufferIndex].m_uiExpectedLabel, Gnm::kCacheActionNone);
	Orbis::Context( )->submitAndFlip(g_sVideoInfo.handle, g_sVideoInfo.flip_index, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);
	Gnm::submitDone( );

	g_uiBufferIndex = (g_uiBufferIndex + 1) % FRAME_BUFFER_NUM;
	Orbis::SetContext(&g_aBuffers[g_uiBufferIndex].m_cContext);

	if (g_aBuffers[g_uiBufferIndex].m_uiExpectedLabel != 0)
	{
		volatile uint32_t uiWait = 0;
		while (*g_aBuffers[g_uiBufferIndex].m_pLabel != g_aBuffers[g_uiBufferIndex].m_uiExpectedLabel)
		{
			++uiWait;
		}

		//sceVideoOutSubmitFlip(g_sVideoInfo.handle, g_sVideoInfo.flip_index, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);
		SceKernelEvent ev;
		int out;
		sceKernelWaitEqueue(g_sVideoInfo.eq, &ev, 1, &out, 0);
		g_sVideoInfo.flip_index = (g_sVideoInfo.flip_index + 1) % g_sVideoInfo.buffer_num;
		Orbis::Context( )->waitUntilSafeForRendering(g_sVideoInfo.handle, g_sVideoInfo.flip_index);
	}
	
	sce::Gnm::RenderTarget* pRenderTarget = &g_aBuffers[g_uiBufferIndex].m_cRenderTarget;
	if (g_aBuffers[g_uiBufferIndex].m_cMultisampledRenderTarget.getBaseAddress( ) != NULL)
		pRenderTarget = &g_aBuffers[g_uiBufferIndex].m_cMultisampledRenderTarget;

	Orbis::Context( )->reset( );
	Orbis::SetMainRenderTargets(pRenderTarget, &g_aBuffers[g_uiBufferIndex].m_cDepthTarget);
	sce::Gnmx::Toolkit::SurfaceUtil::clearRenderTarget(*Orbis::Context( ), pRenderTarget, Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	sce::Gnmx::Toolkit::SurfaceUtil::clearDepthTarget(*Orbis::Context( ), &g_aBuffers[g_uiBufferIndex].m_cDepthTarget, 1.0f);
	Orbis::Context( )->initializeDefaultHardwareState( );
	Orbis::Context( )->setActiveShaderStages(Gnm::kActiveShaderStagesVsPs);
	Orbis::Context( )->setRenderTarget(0, pRenderTarget);
	Orbis::Context( )->setDepthRenderTarget(&g_aBuffers[g_uiBufferIndex].m_cDepthTarget);
	Orbis::Context( )->setupScreenViewport(0, 0, g_aBuffers[g_uiBufferIndex].m_cRenderTarget.getWidth( ), g_aBuffers[g_uiBufferIndex].m_cRenderTarget.getHeight( ), 0.5f, 0.5f);

	sce::Gnm::DepthEqaaControl sDepthEquaa;
	sDepthEquaa.init( );
	if (g_aBuffers[g_uiBufferIndex].m_cMultisampledRenderTarget.getBaseAddress( ) != NULL)
	{
		Orbis::Context( )->setAaSampleCount(sce::Gnm::kNumSamples4);
		Orbis::Context( )->setAaDefaultSampleLocations(sce::Gnm::kNumSamples4);
		sDepthEquaa.setMaxAnchorSamples(sce::Gnm::kNumSamples4);
		sDepthEquaa.setStaticAnchorAssociations(true);
		sDepthEquaa.setAlphaToMaskSamples(sce::Gnm::kNumSamples4);
		sDepthEquaa.setMaskExportNumSamples(sce::Gnm::kNumSamples4);
	}
	else
	{
		Orbis::Context( )->setAaSampleCount(sce::Gnm::kNumSamples1);
		Orbis::Context( )->setAaDefaultSampleLocations(sce::Gnm::kNumSamples1);
	}
	Orbis::Context( )->setDepthEqaaControl(sDepthEquaa);
}


///////////////////////////////////////////////////////////////////////  
//  InitApp

static st_bool InitApp(int argc, char* argv[])
{
	//Razor::initialize( );

	Orbis::Initialize( );

	CTimer cInitTimer;
	cInitTimer.Start( );
	
	// init input
	sceUserServiceInitialize(NULL);
	SceUserServiceUserId sInitialUserId;
	sceUserServiceGetInitialUser(&sInitialUserId);
	scePadInit( );
	g_iControllerHandle = scePadOpen(sInitialUserId, SCE_PAD_PORT_TYPE_STANDARD, 0, NULL);
	scePadSetMotionSensorState(g_iControllerHandle, true);
	printf("pad input initialization complete.\n");

	// init SpeedTree app (it parses multisample settings so has to be before display)
	g_pApplication = st_new(CMyApplication, "CMyApplication");
	if (g_pApplication->ParseCmdLine(argc, argv))
	{
		if (!g_pApplication->Init( ))
		{
			Error("Failed to initialize forest");
			return false;
		}
	}
	
	// init display
	if (!InitDisplay( ))
		return false;
	printf("display initialization complete.\n");
	
	// init graphics
	if (g_pApplication->InitGfx( ))
	{
		cInitTimer.Stop( );
		printf("forest initialization time: %g ms\n", cInitTimer.GetMilliSec( ));
		return true;
	}

	return false;
}


///////////////////////////////////////////////////////////////////////  
//  ExitApp

static void ExitApp(void)
{
	// wait for rsx to finish
	// release all resources
	g_pApplication->ReleaseGfxResources( );

	// shutdown SpeedTree
	PrintSpeedTreeErrors( );
	g_pApplication->ReleaseGfxResources( );
	st_delete<CMyApplication>(g_pApplication);
	g_pApplication = NULL;
	CCore::ShutDown( );

	// cloase controller
	if (g_iControllerHandle != -1)
	{
		scePadClose(g_iControllerHandle);
		sceUserServiceTerminate( );
	}

	Orbis::Finalize( );

	printf("SpeedTreeRefApp_Orbis terminated\n");
	exit(0);
}


///////////////////////////////////////////////////////////////////////  
//  ConvertAxis

static st_float32 ConvertAxis(uint8_t uiValue)
{
	st_float32 fReturn = ((st_float32)uiValue - 127.5f) / -127.5f;

	const float fDeadZone = 0.2f;
	if (fReturn < fDeadZone && fReturn > -fDeadZone)
		fReturn = 0.0f;

	return fReturn;
}


///////////////////////////////////////////////////////////////////////  
//  ProcessInput

static void ProcessInput(void)
{
	ScePadData sPad;
	if (scePadReadState(g_iControllerHandle, &sPad) < 0)
		return;

	st_float32 fElapsedTime = g_pApplication->GetLastFrameTime( );
	
	st_float32 afLeft[] = { ConvertAxis(sPad.leftStick.x), ConvertAxis(sPad.leftStick.y) };
	st_float32 afRight[] = { ConvertAxis(sPad.rightStick.x), ConvertAxis(sPad.rightStick.y) };

	if (sPad.buttons & SCE_PAD_BUTTON_L2)
	{
		// adjust light direction
		g_pApplication->GetLightAdjuster( ).m_fHorzAngle += afRight[0] * fElapsedTime;
		g_pApplication->GetLightAdjuster( ).m_fVertAngle += afRight[1] * fElapsedTime;
		g_pApplication->GetLightAdjuster( ).ComputeDirection( );
		g_pApplication->UpdateLightDir( );
	}
	else
	{
		// navigation
		CMyNavigationBase* pNav = g_pApplication->GetNavigation( );
		assert(pNav);

		float fMoveAmt = g_pApplication->GetConfig( ).m_sNavigation.m_fSpeedScalar;
		if (sPad.buttons & SCE_PAD_BUTTON_R2)
			fMoveAmt *= 160.0f;
		else
			fMoveAmt *= 40.0f;

		pNav->AdjustAzimuth(afRight[0] * fElapsedTime);
		pNav->AdjustPitch(afRight[1] * fElapsedTime);
		pNav->MoveForward(afLeft[1] * fElapsedTime * fMoveAmt);
		pNav->Strafe(afLeft[0] * fElapsedTime * fMoveAmt);
	}

	// toggle geometry types
	static st_uint32 uiLastButtons = 0;
	st_uint32 uiPressedButtons = sPad.buttons & ~uiLastButtons;
	uiLastButtons = sPad.buttons;

	if (uiPressedButtons & SCE_PAD_BUTTON_CROSS)
		g_pApplication->ToggleTrees( );
	if (uiPressedButtons & SCE_PAD_BUTTON_TRIANGLE)
		g_pApplication->ToggleBillboards( );
	if (uiPressedButtons & SCE_PAD_BUTTON_CIRCLE)
		g_pApplication->ToggleTerrain( );
	if (uiPressedButtons & SCE_PAD_BUTTON_SQUARE)
		g_pApplication->ToggleGrass( );
	
	if (uiPressedButtons & SCE_PAD_BUTTON_L1)
		g_pApplication->ToggleTerrainFollowing( );

	if ((sPad.buttons & SCE_PAD_BUTTON_L3) && (sPad.buttons & SCE_PAD_BUTTON_R3))
	{
		g_bRunning = false;
	}
}


///////////////////////////////////////////////////////////////////////  
//  main

int main(int argc, char* argv[])
{
	if (InitApp(argc, argv))
	{
		// display controls
		printf("Controls:\n");
		printf("    Left stick - move/strafe\n");
		printf("    Right stick - turn\n");
		printf("    Cross - toggle trees\n");
		printf("    Circle - toggle terrain\n");
		printf("    Triangle - toggle billboards\n");
		printf("    Square - toggle grass\n");
		printf("    Left shoulder - toggle terrain following\n");
		printf("    Left trigger - hold to move light\n");
		printf("    Right trigger - hold to move faster\n");
		printf("    DPad left - toggle depth rendering\n");
		printf("    DPad right - toggle transparency\n");
		printf("    L3 + R3 - exit app\n\n");

		g_bRunning = true;
		while (g_bRunning)
		{
			// process user input
			ProcessInput( );
			
			// advance scene while drawing
			g_pApplication->Advance( );
			g_pApplication->Cull( );
			g_pApplication->ReportStats( );

			// flip buffers
			Flip( );

			// render scene
			g_pApplication->Render( );
		}
	}

	ExitApp( );

	return 0;
}
