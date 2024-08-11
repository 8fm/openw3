/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "gpuApiRenderState.h"
namespace GpuApi
{	
	inline void DecomposeDepthStencilStateModeIndex( EDepthStencilStateMode baseMode, EDepthStencilStateMode offsetMode, Uint8 &outStencilReadMask, Uint8 &outStencilWriteMask )
	{
		outStencilReadMask = 0;
		outStencilWriteMask = 0;

		switch ( baseMode )
		{
		case DSSM_DepthLE_StencilSet:
		case DSSM_NoDepth_StencilSet:
		case DSSM_FullDepthLE_StencilSet:
			{
				const Uint32 value = (Uint32)offsetMode - (Uint32)baseMode;
				GPUAPI_ASSERT( value < 256 );
				GPUAPI_ASSERT( value < DSSM_STENCIL_VALUES_RANGE_REGULAR );
				outStencilWriteMask = (Uint8)value;
			}
			break;

		case DSSM_LightsFilterAnyMatch_DepthTestGE:
		case DSSM_LightsFilterAnyMatch_DepthTestLE:
		case DSSM_LightsFilterAnyMatch_NoDepthAlways:
		case DSSM_LightsFilterNoneMatch_NoDepthAlways:
		case DSSM_LightsFilterNoneMatch_DepthTestGE:
		case DSSM_LightsFilterExactMatch_NoDepthAlways:
			{
				const Uint32 value = (Uint32)offsetMode - (Uint32)baseMode;
				GPUAPI_ASSERT( value < 256 );
				GPUAPI_ASSERT( value < DSSM_STENCIL_VALUES_RANGE_REGULAR );
				outStencilReadMask = (Uint8)value;
			}
			break;

		case DSSM_ST_WriteStencil_FullDepthL:
		case DSSM_ST_WriteStencil_DepthTestE:
		case DSSM_ST_WriteStencil_DepthWriteL:
		case DSSM_ST_WriteStencil_NoDepth:
			{
				// This is for the "foliage cover outline" effect, basically we need to be sure that
				// foliage is not writing over the stencil bits marking the things that should be outlined.
				const Uint8 local_lc_foliageoutline = FLAG( 4 ); // same value as LC_FoliageOutline

				outStencilReadMask = 0xff;
				outStencilWriteMask = (0xff & ~local_lc_foliageoutline);
			}
			break;

		case DSSM_Water_FullDepthLE:
			{
				const Uint8 local_lc_dynamicObject = FLAG( 0 ); // same value as LC_DynamicObject

				outStencilReadMask = 0;
				outStencilWriteMask = local_lc_dynamicObject;
			}
			break;

		case DSSM_DeferredLightsSkipStencilForward:
			{
				const Uint8 local_lc_forwardShaded = FLAG( 7 ); // same value as LC_ForwardShaded

				outStencilReadMask = local_lc_forwardShaded;
				outStencilWriteMask = 0;
			}
			break;

		default:
			// empty
			break;
		}
	}

	void CRenderStateCache::SetReversedProjection( Bool newReversedProjection )
	{
		if ( m_isReversedProjection == newReversedProjection )
		{
			return;
		}

		// Grab current settings depth/stencil settings
		const EDepthStencilStateMode origMode = m_depthStencilMode;
		const EDepthStencilStateMode origModeNoOffset = m_depthStencilModeNoOffset;
		const Uint32 origStencilRef = m_stencilRef;
		Uint8 origStencilReadMask = 0;
		Uint8 origStencilWriteMask = 0;
		DecomposeDepthStencilStateModeIndex( origModeNoOffset, origMode, origStencilReadMask, origStencilWriteMask );

		// Force current depth/stencil state to be invalidated
		SetDepthStencilMode( DSSM_Max, 0, 0, 0 );
		GPUAPI_ASSERT( DSSM_Max == m_depthStencilMode );

		// Set reversed projection
		m_isReversedProjection = newReversedProjection;

		// Restore depth/stencil state (new reversedProjection state will be used here)
		SetDepthStencilMode( origModeNoOffset, origStencilRef, origStencilReadMask, origStencilWriteMask );
		GPUAPI_ASSERT( origMode == m_depthStencilMode );
		GPUAPI_ASSERT( origModeNoOffset == m_depthStencilModeNoOffset );
		GPUAPI_ASSERT( origStencilRef == m_stencilRef );
	}

	void CRenderStateCache::SetDepthStencilMode( EDepthStencilStateMode newMode, Uint32 stencilRef /* = 0 */, Uint8 stencilReadMask /* = 0 */, Uint8 stencilWriteMask /* = 0 */ )
	{
		switch ( newMode )
		{
		case DSSM_NoStencilNoDepth:
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_STENCIL_TEST);
			glDepthMask(GL_FALSE);
			break;
			
		case DSSM_NoStencilDepthTestLE:
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDisable(GL_STENCIL_TEST);
			break;

		case DSSM_NoStencilFullDepthLE:
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glDisable(GL_STENCIL_TEST);
			break;

		default:
			GPUAPI_HALT(" Not implemented DSS ");
			break;
		}
	}

	void CRenderStateCache::SetupShadowDepthBias( Float depthBiasClamp, Float slopeScaledDepthBias )
	{
//		const ERasterizerMode modesToRefresh[] = 
//		{ 
//			RASTERIZERMODE_CSMCullCCW, 
//			RASTERIZERMODE_CSMCullCCW_Wireframe, 
//			RASTERIZERMODE_CSMNoCull, 
//			RASTERIZERMODE_CSMNoCull_Wireframe,
//			RASTERIZERMODE_ST_NoCull_ShadowCast,
//			RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe,
//			RASTERIZERMODE_ST_BackCull_ShadowCast,
//			RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe,
//			RASTERIZERMODE_ST_FrontCull_ShadowCast,
//			RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe
//		};
//
//		// Ensure at least one raster states created
//		if ( m_rasterStates.Empty() )
//		{
//			SRasterizerStates* states = new SRasterizerStates();
//			states->m_shadowDepthBiasClamp = depthBiasClamp;
//			states->m_shadowSlopeScaledDepthBias = slopeScaledDepthBias;
//			m_rasterStates.PushBack( states );
//		}
//
//		// Find raster state index matching our settings
//		Int32 matchingRasterStateIndex = -1;
//		for ( Uint32 i=0; i<m_rasterStates.Size(); ++i )
//		{
//			const SRasterizerStates &states = *m_rasterStates[i];
//			if ( states.m_shadowDepthBiasClamp == depthBiasClamp && states.m_shadowSlopeScaledDepthBias == slopeScaledDepthBias )
//			{
//				matchingRasterStateIndex = (Int32)i;
//				break;
//			}
//		}
//
//		// If current raster state matches, then nothing to do here
//		if ( 0 == matchingRasterStateIndex )
//		{
//			return;
//		}
//
//		// If we didn't find rasterStates, then we need to create new one, or recycle existing one
//		if ( -1 == matchingRasterStateIndex )
//		{
//			if ( !m_rasterStates.Full() )
//			{
//				GPUAPI_LOG( TXT("RasterizerStates created") );
//				m_rasterStates.PushBack( new SRasterizerStates () );
//			}
//			else
//			{
//				GPUAPI_LOG( TXT("RasterizerStates recycled") );
//
//				SRasterizerStates *backupStates = m_rasterStates.Back(); //< recycle the one that wasn't used longest (e.g. the last one)
//				for ( Uint32 mode_idx_i=0; mode_idx_i<ARRAY_COUNT(modesToRefresh); ++mode_idx_i )
//				{
//					ERasterizerMode rasterMode = modesToRefresh[mode_idx_i];
//					SAFE_RELEASE( backupStates->m_states[rasterMode] );				
//				}
//			}
//
//			matchingRasterStateIndex = (Int32)m_rasterStates.Size() - 1;
//			m_rasterStates[matchingRasterStateIndex]->m_shadowDepthBiasClamp = depthBiasClamp;
//			m_rasterStates[matchingRasterStateIndex]->m_shadowSlopeScaledDepthBias = slopeScaledDepthBias;
//
//			for ( Uint32 i=0; i<RASTERIZERMODE_Max; ++i )
//			{
//				GPUAPI_ASSERT( nullptr == m_rasterStates[matchingRasterStateIndex]->m_states[i] );
//			}
//		}
//
//		// Move our backup raster states right after the current raster states (e.g. position 1).
//		// Raster states are sorted by when were they last time used (most recently used comes first)
//		GPUAPI_ASSERT( matchingRasterStateIndex > 0 && matchingRasterStateIndex < (Int32)m_rasterStates.Size() );
//		if ( 1 != matchingRasterStateIndex )
//		{
//			SRasterizerStates *backupStates = m_rasterStates[matchingRasterStateIndex];
//			m_rasterStates.Remove( matchingRasterStateIndex );
//			m_rasterStates.Insert( 1, backupStates );
//			matchingRasterStateIndex = 1;
//		}
//
//		// Swap shadows related states, and setting between 
//		// current raster states object, and the matching one.
//		{
//			SRasterizerStates *states0 = m_rasterStates[0];
//			SRasterizerStates *states1 = m_rasterStates[matchingRasterStateIndex];
//			Red::Math::NumericalUtils::Swap( states0->m_shadowDepthBiasClamp, states1->m_shadowDepthBiasClamp );
//			Red::Math::NumericalUtils::Swap( states0->m_shadowSlopeScaledDepthBias, states1->m_shadowSlopeScaledDepthBias );
//			for ( Uint32 mode_idx_i=0; mode_idx_i<ARRAY_COUNT(modesToRefresh); ++mode_idx_i )
//			{
//				ERasterizerMode rasterMode = modesToRefresh[mode_idx_i];
//				Red::Math::NumericalUtils::Swap( states0->m_states[rasterMode], states1->m_states[rasterMode] );
//			}
//		}
//
//		// Ensure current settings are bound
//		if ( RASTERIZERMODE_Max != m_rasterizerModeForced )
//		{
//			const ERasterizerMode prevRasterModeOrig = m_rasterizerModeOriginal;
//#ifndef NO_GPU_ASSERTS
//			const ERasterizerMode prevRasterModeForced = m_rasterizerModeForced;
//#endif
//			m_rasterizerModeOriginal = RASTERIZERMODE_Max;
//			m_rasterizerModeForced = RASTERIZERMODE_Max;
//
//			SetRasterizerMode( prevRasterModeOrig );
//
//			GPUAPI_ASSERT( prevRasterModeOrig == m_rasterizerModeOriginal );
//			GPUAPI_ASSERT( prevRasterModeForced == m_rasterizerModeForced );
//		}
	}

	void CRenderStateCache::CreateRMode( ERasterizerMode newMode )
	{
//		// Fill common values
//		desc.FrontCounterClockwise = false;
//		desc.DepthBias = 0;
//		desc.DepthBiasClamp = 0.0f;
//		desc.SlopeScaledDepthBias = 0.0f;
//		desc.DepthClipEnable = true;
//		desc.ScissorEnable = false;
//		desc.MultisampleEnable = false;
//		desc.AntialiasedLineEnable = false;
//		if ( newMode < RASTERIZERMODE_WireframeOffset )
//		{
//			desc.FillMode = D3D11_FILL_SOLID;
//		}
//		else
//		{
//			desc.FillMode = D3D11_FILL_WIREFRAME;
//		}
//
//		// File mode specific values
//		switch ( newMode )
//		{
//		case RASTERIZERMODE_DefaultCullCCW:
//		case RASTERIZERMODE_DefaultCullCCW_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_BACK;
//				desc.DepthBias = 0;
//			}
//			break;
//		case RASTERIZERMODE_BiasedCCW:
//		case RASTERIZERMODE_BiasedCCW_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_BACK;
//				Float biasValue = 100.0f / 16777216.0f;
//				desc.DepthBias = *(Int32*)&biasValue;
//			}
//			break;
//		case RASTERIZERMODE_BiasedNoCull:
//		case RASTERIZERMODE_BiasedNoCull_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_NONE;
//				Float biasValue = 100.0f / 16777216.0f;
//				desc.DepthBias = *(Int32*)&biasValue;
//			}
//			break;
//		case RASTERIZERMODE_DefaultCullCW:
//		case RASTERIZERMODE_DefaultCullCW_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_FRONT;
//				desc.DepthBias = 0;
//			}
//			break;
//		case RASTERIZERMODE_DefaultNoCull:
//		case RASTERIZERMODE_DefaultNoCull_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_NONE;
//				desc.DepthBias = 0;
//			}
//			break;
//			// Speed Tree
//		case RASTERIZERMODE_ST_NoCull:
//		case RASTERIZERMODE_ST_NoCull_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_NONE;
//				desc.FrontCounterClockwise = true;
//				desc.SlopeScaledDepthBias = 0.0f;
//				desc.MultisampleEnable = false;
//			}
//			break;
//		case RASTERIZERMODE_ST_BackCull:
//		case RASTERIZERMODE_ST_BackCull_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_BACK;
//				desc.FrontCounterClockwise = true;
//				desc.SlopeScaledDepthBias = 0.0f;
//				desc.MultisampleEnable = false;
//			}
//			break;
//		case RASTERIZERMODE_ST_FrontCull:
//		case RASTERIZERMODE_ST_FrontCull_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_FRONT;
//				desc.FrontCounterClockwise = true;
//				desc.SlopeScaledDepthBias = 0.0f;
//				desc.MultisampleEnable = false;
//			}
//			break;
//		case RASTERIZERMODE_ST_NoCull_ShadowCast:
//		case RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_NONE;
//				desc.FrontCounterClockwise = true;
//				desc.SlopeScaledDepthBias = 10.0f * 6.0f;
//				desc.MultisampleEnable = false;
//			}
//			break;
//		case RASTERIZERMODE_ST_BackCull_ShadowCast:
//		case RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_BACK;
//				desc.FrontCounterClockwise = true;
//				desc.SlopeScaledDepthBias = 1.0f * 6.0f;
//				desc.MultisampleEnable = false;
//			}
//			break;
//		case RASTERIZERMODE_ST_FrontCull_ShadowCast:
//		case RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
//			{
//				desc.CullMode = D3D11_CULL_FRONT;
//				desc.FrontCounterClockwise = true;
//				desc.SlopeScaledDepthBias = 1.0f * 6.0f;
//				desc.MultisampleEnable = false;
//			}
//			break;
//
//		default:
//			GPUAPI_HALT( TXT( "invalid rasterizer mode" ) );
//		}
//
//		// Create state object
//		ID3D11RasterizerState* rasterizerState = NULL;
//		// CreateRasterizerState checks for identical states already loaded. If it finds one, it returns pointer to it, instead of having a duplicate.
//		// I'm not sure if the AddRef is called in such case, but I will assume it does for the moment.
//		GPUAPI_MUST_SUCCEED( GetDevice()->CreateRasterizerState( &desc, &rasterizerState ) );
//#ifdef GPU_API_DEBUG_PATH
//		const char* debugName = "RS";
//		rasterizerState->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( debugName ) - 1, debugName );
//#endif
//		m_rasterizerStates[ newMode ] = rasterizerState;
	}

	void CRenderStateCache::SetRasterizerMode( ERasterizerMode newMode )
	{
		// setup 'original' rasterizer mode always.
		// 'forced' mode is the one that is synced with the device.
		m_rasterizerModeOriginal = newMode;
		
		// Hack: adjust mode based on two sided enforcement
		if ( m_forcedTwoSided )
		{
			if ( newMode == RASTERIZERMODE_DefaultCullCCW )
			{
				newMode = RASTERIZERMODE_DefaultNoCull;
			}
			else if ( newMode == RASTERIZERMODE_BiasedCCW )
			{
				newMode = RASTERIZERMODE_BiasedNoCull;
			}
		}

		glDisable( GL_POLYGON_OFFSET_FILL );
		glDisable( GL_POLYGON_OFFSET_LINE );
		glDisable( GL_SCISSOR_TEST );
		glEnable( GL_MULTISAMPLE );  //this is just enabling the possibility to multisample, this is the default setting

		Bool wireframe = false;

		if ( newMode < RASTERIZERMODE_WireframeOffset )
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else
		{
			wireframe = true;
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		if ( m_wireframe && newMode < RASTERIZERMODE_WireframeOffset )
		{
			// Offset the mode to the wireframe enabled equivalent
			newMode = (ERasterizerMode)( newMode + RASTERIZERMODE_WireframeOffset );
		}

		if ( m_rasterizerModeForced != newMode )
		{
				// File mode specific values
				switch ( newMode )
				{
				case RASTERIZERMODE_DefaultCullCCW:
				case RASTERIZERMODE_DefaultCullCCW_Wireframe:
					{
						glFrontFace( GL_CW );
						glEnable( GL_CULL_FACE );
						glCullFace( GL_BACK );
					}
					break;
				case RASTERIZERMODE_BiasedCCW:
				case RASTERIZERMODE_BiasedCCW_Wireframe:
					{
						glFrontFace( GL_CW );
						glEnable( GL_CULL_FACE );
						glCullFace( GL_BACK );
						if (wireframe)
						{
							glEnable( GL_POLYGON_OFFSET_LINE );
						}
						else
						{
							glEnable( GL_POLYGON_OFFSET_FILL );
						}
						glPolygonOffset( 0.f, 100.f );
					}
					break;
				case RASTERIZERMODE_BiasedNoCull:
				case RASTERIZERMODE_BiasedNoCull_Wireframe:
					{
						glFrontFace( GL_CW );
						glDisable( GL_CULL_FACE );
						if (wireframe)
						{
							glEnable( GL_POLYGON_OFFSET_LINE );
						}
						else
						{
							glEnable( GL_POLYGON_OFFSET_FILL );
						}
						glPolygonOffset( 0.f, 100.f );
					}
					break;
				case RASTERIZERMODE_DefaultCullCW:
				case RASTERIZERMODE_DefaultCullCW_Wireframe:
					{
						glFrontFace( GL_CW );
						glEnable( GL_CULL_FACE );
						glCullFace( GL_FRONT );
					}
					break;
				case RASTERIZERMODE_DefaultNoCull:
				case RASTERIZERMODE_DefaultNoCull_Wireframe:
					{
						glFrontFace( GL_CW );
						glDisable( GL_CULL_FACE );
					}
					break;
					// Speed Tree
				case RASTERIZERMODE_ST_NoCull:
				case RASTERIZERMODE_ST_NoCull_Wireframe:
					{
						glDisable( GL_CULL_FACE );
						glFrontFace( GL_CCW );
						glDisable( GL_MULTISAMPLE );
					}
					break;
				case RASTERIZERMODE_ST_BackCull:
				case RASTERIZERMODE_ST_BackCull_Wireframe:
					{
						glEnable( GL_CULL_FACE );
						glCullFace( GL_BACK );
						glFrontFace( GL_CCW );
						glDisable( GL_MULTISAMPLE );
					}
					break;
				case RASTERIZERMODE_ST_FrontCull:
				case RASTERIZERMODE_ST_FrontCull_Wireframe:
					{
						glEnable( GL_CULL_FACE );
						glCullFace( GL_FRONT );
						glFrontFace( GL_CCW );
						glDisable( GL_MULTISAMPLE );
					}
					break;
				case RASTERIZERMODE_ST_NoCull_ShadowCast:
				case RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe:
					{
						glDisable( GL_CULL_FACE );
						glFrontFace( GL_CCW );
						if (wireframe)
						{
							glEnable( GL_POLYGON_OFFSET_LINE );
						}
						else
						{
							glEnable( GL_POLYGON_OFFSET_FILL );
						}
						glPolygonOffset( 10.0f * 6.0f, 0.f );
						glDisable( GL_MULTISAMPLE );
					}
					break;
				case RASTERIZERMODE_ST_BackCull_ShadowCast:
				case RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
					{
						glEnable( GL_CULL_FACE );
						glCullFace( GL_BACK );
						glFrontFace( GL_CCW );
						if (wireframe)
						{
							glEnable( GL_POLYGON_OFFSET_LINE );
						}
						else
						{
							glEnable( GL_POLYGON_OFFSET_FILL );
						}
						glPolygonOffset( 1.0f * 6.0f, 0.f );
						glDisable( GL_MULTISAMPLE );
					}
					break;
				case RASTERIZERMODE_ST_FrontCull_ShadowCast:
				case RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
					{
						glEnable( GL_CULL_FACE );
						glCullFace( GL_FRONT );
						glFrontFace( GL_CCW );
						if (wireframe)
						{
							glEnable( GL_POLYGON_OFFSET_LINE );
						}
						else
						{
							glEnable( GL_POLYGON_OFFSET_FILL );
						}
						glPolygonOffset( 1.0f * 6.0f, 0.f );
						glDisable( GL_MULTISAMPLE );
					}
					break;
		
				default:
					GPUAPI_HALT( "invalid rasterizer mode" );
				}
		}
	}

	void CRenderStateCache::CreateBMode( EBlendMode newMode )
	{
//		// Setup blend state for the first time.
//		D3D11_BLEND_DESC desc;
//
//		// Set common values
//		desc.AlphaToCoverageEnable = false;
//		desc.IndependentBlendEnable = false;
//		desc.RenderTarget[0].BlendEnable = true;
//		desc.RenderTarget[1].BlendEnable = true;
//		desc.RenderTarget[2].BlendEnable = true;
//		desc.RenderTarget[3].BlendEnable = true;
//		desc.RenderTarget[4].BlendEnable = true;
//		desc.RenderTarget[5].BlendEnable = true;
//		desc.RenderTarget[6].BlendEnable = true;
//		desc.RenderTarget[7].BlendEnable = true;
//		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//		desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//		desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//		desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//		desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//		desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//		desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//		desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//
//		// Set blend mode specific values
//		switch ( newMode )
//		{
//		case BLENDMODE_Set:
//			desc.AlphaToCoverageEnable = false;
//			// In this case, disable blend for all RTs
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_Set_0RTOnlyWrites:
//			desc.AlphaToCoverageEnable = false;
//			// In this case, disable blend for all RTs
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[1].RenderTargetWriteMask = 0;
//			desc.RenderTarget[2].RenderTargetWriteMask = 0;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = 0;
//			desc.RenderTarget[5].RenderTargetWriteMask = 0;
//			desc.RenderTarget[6].RenderTargetWriteMask = 0;
//			desc.RenderTarget[7].RenderTargetWriteMask = 0;
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_Set_0RTNoWrites:
//			desc.AlphaToCoverageEnable = false;
//			// In this case, disable blend for all RTs
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = 0;
//			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//			break;
//
//		case BLENDMODE_Add:				
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_Substract:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_REV_SUBTRACT;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_REV_SUBTRACT;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_Mul:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_SRC_COLOR;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_DEST_ALPHA;
//			break;
//
//		case BLENDMODE_Blend:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_BlendDecals1:
//			// D3DRS_SEPARATEALPHABLENDENABLE - this shit was set in dx9. Seems like no equivalent exists in dx10, but note it just in case.
//			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
//			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_BlendDecals2:
//			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN; // two channels only -> luminance/interleavedChrominance
//			desc.RenderTarget[1].RenderTargetWriteMask = 0;
//			desc.RenderTarget[2].RenderTargetWriteMask = 0;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_BlendDecalsNormals:
//			desc.RenderTarget[0].RenderTargetWriteMask = 0;
//			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[2].RenderTargetWriteMask = 0;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_PremulBlend:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_ParticleCombine:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_ParticleLowRes:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_NoColorWrite:
//			// IndependentBlendEnable is false, so only need to set RT[0]
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = 0;
//			break;
//
//		case BLENDMODE_SF_Blend:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_SF_Blend_Opaque:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_SF_Lighten:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MAX;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MAX;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
//			break;
//
//		case BLENDMODE_SF_Lighten_Opaque:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MAX;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MAX;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
//			break;
//
//		case BLENDMODE_SF_Darken:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MIN;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MIN;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
//			break;
//
//		case BLENDMODE_SF_Darken_Opaque:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MIN;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MIN;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
//			break;
//
//		case BLENDMODE_SF_Add:				
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_SF_Add_Opaque:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_SF_Subtract_Opaque:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_REV_SUBTRACT;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_REV_SUBTRACT;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//		case BLENDMODE_SF_Alpha:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
//			break;
//
//		case BLENDMODE_SF_SetPreserveAlpha:
//			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
//			break;
//
//			// Speed Tree
//		case BLENDMODE_ST_Set_NoAtoC_ColorWrite:
//			desc.AlphaToCoverageEnable = false;
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//		case BLENDMODE_ST_Set_NoAtoC_NoColorWrite:
//			desc.AlphaToCoverageEnable = false;
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = 0;
//			desc.RenderTarget[1].RenderTargetWriteMask = 0;
//			desc.RenderTarget[2].RenderTargetWriteMask = 0;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = 0;
//			desc.RenderTarget[5].RenderTargetWriteMask = 0;
//			desc.RenderTarget[6].RenderTargetWriteMask = 0;
//			desc.RenderTarget[7].RenderTargetWriteMask = 0;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//		case BLENDMODE_ST_Set_WithAtoC_ColorWrite:
//			desc.AlphaToCoverageEnable = true;
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//		case BLENDMODE_ST_Set_WithAtoC_NoColorWrite:
//			desc.AlphaToCoverageEnable = true;
//			desc.RenderTarget[0].BlendEnable = false;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = 0;
//			desc.RenderTarget[1].RenderTargetWriteMask = 0;
//			desc.RenderTarget[2].RenderTargetWriteMask = 0;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = 0;
//			desc.RenderTarget[5].RenderTargetWriteMask = 0;
//			desc.RenderTarget[6].RenderTargetWriteMask = 0;
//			desc.RenderTarget[7].RenderTargetWriteMask = 0;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//		case BLENDMODE_ST_Blending_NoAtoC_ColorWrite:
//			desc.AlphaToCoverageEnable = false;
//			desc.RenderTarget[0].BlendEnable = true;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//		case BLENDMODE_ST_Blending_NoAtoC_NoColorWrite:
//			desc.AlphaToCoverageEnable = false;
//			desc.RenderTarget[0].BlendEnable = true;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = 0;
//			desc.RenderTarget[1].RenderTargetWriteMask = 0;
//			desc.RenderTarget[2].RenderTargetWriteMask = 0;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = 0;
//			desc.RenderTarget[5].RenderTargetWriteMask = 0;
//			desc.RenderTarget[6].RenderTargetWriteMask = 0;
//			desc.RenderTarget[7].RenderTargetWriteMask = 0;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//		case BLENDMODE_ST_Blending_WithAtoC_ColorWrite:
//			desc.AlphaToCoverageEnable = true;
//			desc.RenderTarget[0].BlendEnable = true;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//		case BLENDMODE_ST_Blending_WithAtoC_NoColorWrite:
//			desc.AlphaToCoverageEnable = true;
//			desc.RenderTarget[0].BlendEnable = true;
//			desc.RenderTarget[1].BlendEnable = false;
//			desc.RenderTarget[2].BlendEnable = false;
//			desc.RenderTarget[3].BlendEnable = false;
//			desc.RenderTarget[4].BlendEnable = false;
//			desc.RenderTarget[5].BlendEnable = false;
//			desc.RenderTarget[6].BlendEnable = false;
//			desc.RenderTarget[7].BlendEnable = false;
//			desc.RenderTarget[0].RenderTargetWriteMask = 0;
//			desc.RenderTarget[1].RenderTargetWriteMask = 0;
//			desc.RenderTarget[2].RenderTargetWriteMask = 0;
//			desc.RenderTarget[3].RenderTargetWriteMask = 0;
//			desc.RenderTarget[4].RenderTargetWriteMask = 0;
//			desc.RenderTarget[5].RenderTargetWriteMask = 0;
//			desc.RenderTarget[6].RenderTargetWriteMask = 0;
//			desc.RenderTarget[7].RenderTargetWriteMask = 0;
//			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
//			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
//			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
//			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
//			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
//			break;
//
//		default:
//			GPUAPI_HALT( TXT( "Invalid blend mode" ) );
//			return;
//		}
//
//		// Create state object
//		ID3D11BlendState* blendState = NULL;
//		// CreateBlendState checks for identical states already loaded. If it finds one, it returns pointer to it, instead of having a duplicate.
//		// I'm not sure if the AddRef is called in such case, but I will assume it does for the moment.
//		GPUAPI_MUST_SUCCEED( GetDevice()->CreateBlendState( &desc, &blendState ) );
//#ifdef GPU_API_DEBUG_PATH
//		const char* debugName = "BS";
//		blendState->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( debugName ) - 1, debugName );
//#endif
//		m_blendStates[ newMode ] = blendState;
	}

	void CRenderStateCache::SetBlendMode( EBlendMode newMode )
	{
		if ( m_blendMode != newMode )
		{
			if ( newMode == BLENDMODE_Max )
			{
				return;
			}

			//desc.AlphaToCoverageEnable = false;  --> glSampleCoverage
			//desc.IndependentBlendEnable = false; --> glBlendEquationSeparate

			//glEnablei(GL_BLEND, 0);
			//glEnablei(GL_BLEND, 1);
			//glEnablei(GL_BLEND, 2);
			//glEnablei(GL_BLEND, 3);
			//glEnablei(GL_BLEND, 4);
			//glEnablei(GL_BLEND, 5);
			//glEnablei(GL_BLEND, 6);
			//glEnablei(GL_BLEND, 7);
			//glColorMaski( 0, true, true, true, true );
			//glColorMaski( 1, true, true, true, true );
			//glColorMaski( 2, true, true, true, true );
			//glColorMaski( 3, true, true, true, true );
			//glColorMaski( 4, true, true, true, true );
			//glColorMaski( 5, true, true, true, true );
			//glColorMaski( 6, true, true, true, true );
			//glColorMaski( 7, true, true, true, true );

			switch ( newMode )
			{
			case BLENDMODE_Blend:
				glEnable(GL_BLEND);
				glEnablei(GL_BLEND, 0);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BLENDMODE_Set:
				glDisable(GL_BLEND);
				break;
			default:
				GPUAPI_HALT("invalid blend mode");
				break;
			}

			// Remember current blend mode
			m_blendMode = newMode;
		}
	}
}
