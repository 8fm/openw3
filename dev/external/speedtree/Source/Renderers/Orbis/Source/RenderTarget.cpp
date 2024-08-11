///////////////////////////////////////////////////////////////////////
//  RenderTarget.cpp
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
#include "Utilities/Utility.h"
#include "Utilities/toolkit/toolkit.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::CRenderTargetOrbis

CRenderTargetOrbis::CRenderTargetOrbis( ) :
	m_eType(RENDER_TARGET_TYPE_COLOR),
	m_nWidth(-1),
	m_nHeight(-1),
	m_pWaitLabel(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::~CRenderTargetOrbis

CRenderTargetOrbis::~CRenderTargetOrbis( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::InitGfx

st_bool CRenderTargetOrbis::InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples)
{
	st_bool bSuccess = false;

	if (nWidth > 0 && nHeight > 0 && (int)eType < (int)c_nMaxNumRenderTargets)
	{
		m_eType = eType;
		m_nWidth = nWidth;
		m_nHeight = nHeight;

		sce::Gnm::NumSamples eSamples = sce::Gnm::kNumSamples1;
		sce::Gnm::NumFragments eFragments = sce::Gnm::kNumFragments1;
		bool bMultisampled = false;
		if (nNumSamples == 2)
		{
			eSamples = sce::Gnm::kNumSamples2;
			eFragments = sce::Gnm::kNumFragments2;
			bMultisampled = true;
		}
		else if (nNumSamples == 4)
		{
			eSamples = sce::Gnm::kNumSamples4;
			eFragments = sce::Gnm::kNumFragments4;
			bMultisampled = true;
		}

		if (eType == RENDER_TARGET_TYPE_COLOR)
		{
			sce::Gnm::DataFormat sFormat = sce::Gnm::kDataFormatB8G8R8A8Unorm;
			sce::Gnm::TileMode sTileMode;
			sce::GpuAddress::computeSurfaceTileMode(&sTileMode, sce::GpuAddress::kSurfaceTypeColorTarget, sFormat, 1);

			const sce::Gnm::SizeAlign sSizeAlign = m_cRenderTarget.init(nWidth, nHeight, 1, sFormat, sTileMode, eSamples, eFragments, NULL, 0);
			void* pRenderTargetPtr = Orbis::Allocate(sSizeAlign.m_size, sSizeAlign.m_align, true);
			m_cRenderTarget.setAddresses(pRenderTargetPtr, 0, 0);


			m_cTexture.initFromRenderTarget(&m_cRenderTarget, false);
			m_cTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
		}
		else
		{
			sce::Gnm::DataFormat sShadowDepthFormat = sce::Gnm::DataFormat::build(sce::Gnm::kZFormat32Float);
			sce::Gnm::TileMode sShadowDepthTileMode;
			sce::GpuAddress::computeSurfaceTileMode(&sShadowDepthTileMode, sce::GpuAddress::kSurfaceTypeDepthOnlyTarget, sShadowDepthFormat, 1 << eFragments);
			sce::Gnm::SizeAlign shadowDepthTargetSizeAlign = m_cDepthRenderTarget.init(nWidth, nHeight, sShadowDepthFormat.getZFormat( ), sce::Gnm::kStencilInvalid, sShadowDepthTileMode, eFragments, NULL, NULL);
			
			void* pDepth = Orbis::Allocate(shadowDepthTargetSizeAlign.m_size, shadowDepthTargetSizeAlign.m_align, true);
			m_cDepthRenderTarget.setZReadAddress(pDepth);
			m_cDepthRenderTarget.setZWriteAddress(pDepth);

			m_cTexture.initFromDepthRenderTarget(&m_cDepthRenderTarget, false);
			m_cTexture.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
		}

		bSuccess = m_cTexture.isTexture( );
	}
	
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::ReleaseGfxResources

void CRenderTargetOrbis::ReleaseGfxResources(void)
{
	if (m_cRenderTarget.getWidth( ) > 0)
		Orbis::Release(m_cRenderTarget.getBaseAddress( ));
	if (m_cDepthRenderTarget.getWidth( ) > 0)
		Orbis::Release(m_cDepthRenderTarget.getZWriteAddress( ));
	memset(&m_cRenderTarget, 0, sizeof(sce::Gnm::RenderTarget));
	memset(&m_cDepthRenderTarget, 0, sizeof(sce::Gnm::DepthRenderTarget));
	memset(&m_cTexture, 0, sizeof(sce::Gnm::Texture));
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::Clear

void CRenderTargetOrbis::Clear(const Vec4& vColor)
{
	if (m_cRenderTarget.getWidth( ) != 0)
	{
		sce::Gnmx::Toolkit::SurfaceUtil::clearRenderTarget(*Orbis::Context( ), &m_cRenderTarget, sce::Vectormath::Scalar::Aos::Vector4(vColor.x, vColor.y, vColor.z, vColor.w));
	}

	if (m_cDepthRenderTarget.getWidth( ) != 0)
	{
		sce::Gnmx::Toolkit::SurfaceUtil::clearDepthTarget(*Orbis::Context( ), &m_cDepthRenderTarget, 1.0f);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::SetAsTarget

st_bool CRenderTargetOrbis::SetAsTarget(void)
{
	if (m_cRenderTarget.getWidth( ) != 0)
	{
		Orbis::Context( )->setRenderTarget(0, &m_cRenderTarget);
		Orbis::Context( )->setupScreenViewport(0, 0, m_cRenderTarget.getWidth( ), m_cRenderTarget.getHeight( ), 0.5f, 0.5f);
	}

	if (m_cDepthRenderTarget.getWidth( ) != 0)
	{
		Orbis::Context( )->setRenderTarget(0, NULL);
		Orbis::Context( )->setDepthRenderTarget(&m_cDepthRenderTarget);	
		Orbis::Context( )->setupScreenViewport(0, 0, m_cDepthRenderTarget.getWidth( ), m_cDepthRenderTarget.getHeight( ), 0.5f, 0.5f);
	}
	
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::ReleaseAsTarget

void CRenderTargetOrbis::ReleaseAsTarget(void)
{
	Orbis::BindMainRenderTargets( );

	m_pWaitLabel = (uint64_t*)Orbis::Context( )->allocateFromCommandBuffer(sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8);
	*m_pWaitLabel = 0;
	Orbis::Context( )->writeImmediateAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches, (void*)m_pWaitLabel, 1, sce::Gnm::kCacheActionNone);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::BindAsTexture

st_bool CRenderTargetOrbis::BindAsTexture(st_int32 nRegister, st_bool bPointFilter) const
{
	ST_UNREF_PARAM(bPointFilter);

	if (m_pWaitLabel != NULL)
	{
		Orbis::Context( )->waitOnAddress((void*)m_pWaitLabel, 0xFFFFFFFF, sce::Gnm::kWaitCompareFuncEqual, 1);
		m_pWaitLabel = NULL;
	}

	Orbis::Context( )->setTextures(sce::Gnm::kShaderStagePs, nRegister, 1, &m_cTexture);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::UnBindAsTexture

void CRenderTargetOrbis::UnBindAsTexture(st_int32 nRegister) const
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::OnResetDevice

void CRenderTargetOrbis::OnResetDevice(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::OnLostDevice

void CRenderTargetOrbis::OnLostDevice(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::SetGroupAsTarget

st_bool CRenderTargetOrbis::SetGroupAsTarget(const CRenderTargetOrbis** pTargets, st_int32 nNumTargets, st_bool bClear)
{
	ST_UNREF_PARAM(bClear);

	if (nNumTargets == 0 || nNumTargets > c_nMaxNumRenderTargets)
		return false;

	st_uint32 uiTarget = 0;
	for (st_int32 i = 0; i < nNumTargets; ++i)
	{
		if (pTargets[i]->m_eType == RENDER_TARGET_TYPE_DEPTH)
		{
			Orbis::Context( )->setDepthRenderTarget(&pTargets[i]->m_cDepthRenderTarget);
		}
		else
		{
			Orbis::Context( )->setRenderTarget(uiTarget, &pTargets[i]->m_cRenderTarget);
			if (uiTarget == 0)
			{
				Orbis::Context( )->setupScreenViewport(0, 0, pTargets[i]->m_cRenderTarget.getWidth( ), pTargets[i]->m_cRenderTarget.getHeight( ), 0.5f, 0.5f);
			}

			++uiTarget;
		}
	}
	
	return true;	
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOrbis::ReleaseGroupAsTarget

void CRenderTargetOrbis::ReleaseGroupAsTarget(const CRenderTargetOrbis** pTargets, st_int32 nNumTargets)
{
	volatile uint64_t* pWaitLabel = (uint64_t*)Orbis::Context( )->allocateFromCommandBuffer(sizeof(uint64_t), sce::Gnm::kEmbeddedDataAlignment8);
	*pWaitLabel = 0;
	Orbis::Context( )->writeImmediateAtEndOfPipe(sce::Gnm::kEopFlushCbDbCaches, (void*)pWaitLabel, 1, sce::Gnm::kCacheActionNone);
	
	for (st_int32 i = 0; i < nNumTargets; ++i)
	{
		const_cast<CRenderTargetOrbis*>(pTargets[i])->m_pWaitLabel = pWaitLabel;
	}
	
	Orbis::BindMainRenderTargets( );
}
