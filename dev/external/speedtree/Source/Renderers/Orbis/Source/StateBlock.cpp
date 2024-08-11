///////////////////////////////////////////////////////////////////////
//  StateBlock.cpp
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

#include "Renderers/Orbis/OrbisRenderer.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOrbis::CStateBlockOrbis

CStateBlockOrbis::CStateBlockOrbis( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOrbis::Init

st_bool CStateBlockOrbis::Init(const SAppState& sAppState, const SRenderState& sRenderState)
{
	m_sAppState = sAppState;
	m_sRenderState = sRenderState;

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOrbis::Bind

st_bool CStateBlockOrbis::Bind(void) const
{
	// blending
	sce::Gnm::BlendControl sBlendControl;
	sBlendControl.init( );

	if (m_sRenderState.m_bBlending)
	{
		sBlendControl.setBlendEnable(true);
		sBlendControl.setAlphaEquation(sce::Gnm::kBlendMultiplierSrcAlpha, sce::Gnm::kBlendFuncAdd, sce::Gnm::kBlendMultiplierOneMinusSrcAlpha);
	}
	else
	{
		sBlendControl.setBlendEnable(false);
	}

	Orbis::Context( )->setBlendControl(0, sBlendControl);
	Orbis::Context( )->setBlendControl(1, sBlendControl);
	Orbis::Context( )->setBlendControl(2, sBlendControl);
	Orbis::Context( )->setBlendControl(3, sBlendControl);

	// color mask
	if (m_sAppState.m_bDeferred)
	{
		Orbis::Context( )->setRenderTargetMask(0xFF);
	}
	else
	{
		Orbis::Context( )->setRenderTargetMask(0xF);
	}

	// depth mask & testing function
	sce::Gnm::DepthStencilControl cDepthStencilSetup;
	cDepthStencilSetup.init( );

	if (m_sAppState.m_eOverrideDepthTest == SAppState::OVERRIDE_DEPTH_TEST_DISABLE)
	{
		cDepthStencilSetup.setDepthEnable(false);
	}
	else
	{
		cDepthStencilSetup.setDepthEnable(true);
	}

	if (!m_sAppState.m_bDepthPrepass)
	{
		cDepthStencilSetup.setDepthControl(sce::Gnm::kDepthControlZWriteEnable, sce::Gnm::kCompareFuncLess);
	}
	else
	{
		if (m_sRenderState.m_eRenderPass != RENDER_PASS_MAIN)
		{
			cDepthStencilSetup.setDepthControl(sce::Gnm::kDepthControlZWriteEnable, sce::Gnm::kCompareFuncLess);
		}
		else
		{
			cDepthStencilSetup.setDepthControl(sce::Gnm::kDepthControlZWriteDisable, sce::Gnm::kCompareFuncLessEqual);
		}
	}

	Orbis::Context( )->setDepthStencilControl(cDepthStencilSetup);


	// multisampling
	sce::Gnm::AlphaToMaskControl cAlphaToMaskSetup;
	cAlphaToMaskSetup.init( );

	if (m_sAppState.m_bMultisampling)
	{
		Orbis::Context( )->setScanModeControl(sce::Gnm::kScanModeControlAaEnable, sce::Gnm::kScanModeControlViewportScissorEnable);
		
		if (m_sAppState.m_bAlphaToCoverage)
		{
			cAlphaToMaskSetup.setEnabled(sce::Gnm::kAlphaToMaskEnable);
			cAlphaToMaskSetup.setPixelDitherThresholds(sce::Gnm::kAlphaToMaskDitherThreshold0, sce::Gnm::kAlphaToMaskDitherThreshold1, sce::Gnm::kAlphaToMaskDitherDisabled, sce::Gnm::kAlphaToMaskDitherThreshold3);
			cAlphaToMaskSetup.setRoundMode(sce::Gnm::kAlphaToMaskRoundDithered);
		}
	}
	else
	{
		Orbis::Context( )->setScanModeControl(sce::Gnm::kScanModeControlAaDisable, sce::Gnm::kScanModeControlViewportScissorDisable);
	}

	Orbis::Context( )->setAlphaToMaskControl(cAlphaToMaskSetup);

	sce::Gnm::PrimitiveSetup cPrimitiveSetup;
	cPrimitiveSetup.init( );

	// face culling
	cPrimitiveSetup.setFrontFace(sce::Gnm::kPrimitiveSetupFrontFaceCcw);
	if (m_sRenderState.m_eFaceCulling == CULLTYPE_NONE)
	{
		cPrimitiveSetup.setCullFace(sce::Gnm::kPrimitiveSetupCullFaceNone);
	}
	else 
	{
		if (m_sRenderState.m_eFaceCulling == CULLTYPE_BACK)
			cPrimitiveSetup.setCullFace(sce::Gnm::kPrimitiveSetupCullFaceBack);
		else
			cPrimitiveSetup.setCullFace(sce::Gnm::kPrimitiveSetupCullFaceFront);
	}

	// polygon offset
	if (m_sRenderState.m_eRenderPass == RENDER_PASS_SHADOW_CAST)
	{
		// values carefully tuned for SpeedTree reference app, but will likely need to change for
		// other applications
		const st_float32 c_fScale = ((m_sRenderState.m_eFaceCulling == CULLTYPE_NONE) ? 10.0f : 4.0f) * 16.0f;
        const st_float32 c_fOffset = (m_sRenderState.m_eFaceCulling == CULLTYPE_NONE) ? 1.0f: 8.0f;
		Orbis::Context( )->setPolygonOffsetFront(c_fScale, c_fOffset);
		Orbis::Context( )->setPolygonOffsetBack(c_fScale, c_fOffset);
		Orbis::Context( )->setPolygonOffsetZFormat(sce::Gnm::kZFormat32Float);
		Orbis::Context( )->setPolygonOffsetClamp(1.0f);

		cPrimitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetEnable, sce::Gnm::kPrimitiveSetupPolygonOffsetEnable);
	}
	else
	{
		cPrimitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetDisable, sce::Gnm::kPrimitiveSetupPolygonOffsetDisable);
	}

	Orbis::Context( )->setPrimitiveSetup(cPrimitiveSetup);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOrbis::ReleaseGfxResources

void CStateBlockOrbis::ReleaseGfxResources(void)
{
}

