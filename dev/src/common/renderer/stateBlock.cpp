/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "speedTreeRenderInterface.h"
#include "../engine/drawableComponent.h"

#ifdef USE_SPEED_TREE

using namespace SpeedTree;

///////////////////////////////////////////////////////////////////////  

CStateBlockGPUAPI::CStateBlockGPUAPI()
	: m_depthStencilMode( GpuApi::DSSM_Max )
	, m_rasterizerMode( GpuApi::RASTERIZERMODE_Max )
	, m_blendMode( GpuApi::BLENDMODE_Max )
	, m_drawContextRefValue( 0 )
{
}

st_bool CStateBlockGPUAPI::Init(const SAppState& sAppState, const SRenderState& sRenderState, st_bool isInteractive)
{
	// Depth stencil mode
	GpuApi::EDepthStencilStateMode depthStencilMode;

	if ( sAppState.m_eOverrideDepthTest != SAppState::OVERRIDE_DEPTH_TEST_DISABLE )
	{
		if ( !sAppState.m_bDepthPrepass || sRenderState.m_eRenderPass == RENDER_PASS_DEPTH_PREPASS )
		{
			depthStencilMode = GpuApi::DSSM_ST_WriteStencil_FullDepthL;
		}
		else
		{
			depthStencilMode = GpuApi::DSSM_ST_WriteStencil_DepthTestE;
		}
	}
	else
	{
		if ( !sAppState.m_bDepthPrepass || sRenderState.m_eRenderPass == RENDER_PASS_DEPTH_PREPASS )
		{
			depthStencilMode = GpuApi::DSSM_ST_WriteStencil_DepthWriteL;
		}
		else
		{
			depthStencilMode = GpuApi::DSSM_ST_WriteStencil_NoDepth;
		}
	}

	// Rasterizer mode
	GpuApi::ERasterizerMode rasterizerMode = GpuApi::RASTERIZERMODE_Max;

	if ( sRenderState.m_eRenderPass == RENDER_PASS_SHADOW_CAST )
	{
		switch( sRenderState.m_eFaceCulling )
		{
		case CULLTYPE_BACK:
			rasterizerMode = GpuApi::RASTERIZERMODE_ST_BackCull_ShadowCast;
			break;
		case CULLTYPE_FRONT:
			rasterizerMode = GpuApi::RASTERIZERMODE_ST_FrontCull_ShadowCast;
			break;
		case CULLTYPE_NONE:
			rasterizerMode = GpuApi::RASTERIZERMODE_ST_NoCull_ShadowCast;
			break;
		}
	}
	else
	{
		switch( sRenderState.m_eFaceCulling )
		{
		case CULLTYPE_BACK:
			rasterizerMode = GpuApi::RASTERIZERMODE_ST_BackCull;
			break;
		case CULLTYPE_FRONT:
			rasterizerMode = GpuApi::RASTERIZERMODE_ST_FrontCull;
			break;
		case CULLTYPE_NONE:
			rasterizerMode = GpuApi::RASTERIZERMODE_ST_NoCull;
			break;
		}
	}

	// Blend
	GpuApi::EBlendMode	blendMode;

	if (sRenderState.m_bBlending)
	{
		if ( sAppState.m_bAlphaToCoverage && sRenderState.m_eRenderPass != RENDER_PASS_SHADOW_CAST )
		{
			blendMode = GpuApi::BLENDMODE_ST_Blending_WithAtoC_ColorWrite;
		}
		else
		{
			blendMode = GpuApi::BLENDMODE_ST_Blending_NoAtoC_ColorWrite;
		}
	}
	else
	{
		if ( sAppState.m_bAlphaToCoverage && sRenderState.m_eRenderPass != RENDER_PASS_SHADOW_CAST )
		{
			blendMode = GpuApi::BLENDMODE_ST_Set_WithAtoC_ColorWrite;
		}
		else
		{
			blendMode = GpuApi::BLENDMODE_ST_Set_NoAtoC_ColorWrite;
		}
	}
	if ( sRenderState.m_eRenderPass != RENDER_PASS_MAIN )
	{
		blendMode = (GpuApi::EBlendMode)( blendMode + 1 );
	}

	m_depthStencilMode = depthStencilMode;
	m_rasterizerMode = rasterizerMode;
	m_blendMode = blendMode;

	// Occlusion Outline : Trees should show outlines for any occluded characters. So, we gotta set up a separate
	// reference value for them. The trunks/branches come as 3D_TREES, the leaves come as BILLBOARDS, so we catch
	// both.
	Uint8 stencilRef = LC_Default | LC_DynamicObject; // Use LC_DynamicObjects, so that decals (focus clues) do not stretch on grass blades
	
	if( isInteractive == true )
	{
		stencilRef |= LC_Interactive;
	}

	// Foliage should not affect the "characters" stencil bit, so that we can track when characters are behind
	// trees. So, we don't write to that bit. Not the nicest thing, but it works...
	m_drawContextRefValue = GpuApi::PackDrawContextRefValue( stencilRef, ( Uint8 )0xFF, ( Uint8 )( ~LC_FoliageOutline ) );

	return true;
}

st_bool CStateBlockGPUAPI::Bind() const
{
	GpuApi::SetCustomDrawContext( m_depthStencilMode, m_rasterizerMode, m_blendMode, m_drawContextRefValue );

	return true;
}

void CStateBlockGPUAPI::ReleaseGfxResources()
{
	// Leave it to draw context management
}

st_int32 CStateBlockGPUAPI::GetDrawContextRefValue() const
{
	return (st_int32)m_drawContextRefValue;
}

#endif
