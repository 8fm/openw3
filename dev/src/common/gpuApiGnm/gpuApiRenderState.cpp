/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
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

	inline EDepthStencilStateMode BuildDepthStencilStateModeIndex( EDepthStencilStateMode newMode, Uint8 stencilReadMask, Uint8 stencilWriteMask )
	{
		Uint8 offsetValue = 0;

		switch ( newMode )
		{
		case DSSM_DepthLE_StencilSet:
		case DSSM_NoDepth_StencilSet:
		case DSSM_FullDepthLE_StencilSet:
			{
				GPUAPI_ASSERT( 0 == stencilReadMask );
				offsetValue = stencilWriteMask;
				GPUAPI_ASSERT( offsetValue < DSSM_STENCIL_VALUES_RANGE_REGULAR );
			}
			break;

		case DSSM_LightsFilterAnyMatch_DepthTestGE:
		case DSSM_LightsFilterAnyMatch_DepthTestLE:
		case DSSM_LightsFilterAnyMatch_NoDepthAlways:
		case DSSM_LightsFilterNoneMatch_NoDepthAlways:
		case DSSM_LightsFilterNoneMatch_DepthTestGE:
		case DSSM_LightsFilterExactMatch_NoDepthAlways:
			{
				GPUAPI_ASSERT( 0 == stencilWriteMask );
				offsetValue = stencilReadMask;
				GPUAPI_ASSERT( offsetValue < DSSM_STENCIL_VALUES_RANGE_REGULAR );
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
				RED_UNUSED( local_lc_foliageoutline );
				GPUAPI_ASSERT( 0xff == stencilReadMask );
				GPUAPI_ASSERT( (0xff & ~local_lc_foliageoutline) == stencilWriteMask );

				offsetValue = 0; // we support only 1 writeMask (no need for extra storage in this simple case - sanity check above)

				GPUAPI_ASSERT( offsetValue < DSSM_STENCIL_VALUES_RANGE_SPEEDTREE );
			}
			break;

		case DSSM_Water_FullDepthLE:
			{
				const Uint8 local_lc_dynamicObject = FLAG( 0 ); // same value as LC_DynamicObject
				RED_UNUSED( local_lc_dynamicObject );
				GPUAPI_ASSERT( 0 == stencilReadMask );
				GPUAPI_ASSERT( local_lc_dynamicObject == stencilWriteMask );

				offsetValue = 0; // we support only 1 writeMask
			}
			break;

		case DSSM_DeferredLightsSkipStencilForward:
			{
				const Uint8 local_lc_forwardShaded = FLAG( 7 ); // same value as LC_ForwardShaded
				RED_UNUSED( local_lc_forwardShaded );
				GPUAPI_ASSERT( local_lc_forwardShaded == stencilReadMask );
				GPUAPI_ASSERT( 0 == stencilWriteMask );

				offsetValue = 0; // we support only 1 readMask
			}
			break;

		default:
			GPUAPI_ASSERT( 0 == stencilReadMask );
			GPUAPI_ASSERT( 0 == stencilWriteMask );
		}

		const Uint32 resultValue = newMode + offsetValue;
		GPUAPI_ASSERT( resultValue < DSSM_Max );
		const EDepthStencilStateMode offsetMode = (EDepthStencilStateMode) resultValue;

#ifndef NO_GPU_ASSERTS
		{
			Uint8 testReadValue = 0;
			Uint8 testWriteValue = 0;
			DecomposeDepthStencilStateModeIndex( newMode, offsetMode, testReadValue, testWriteValue );
			GPUAPI_ASSERT( testWriteValue == stencilWriteMask );
			GPUAPI_ASSERT( testReadValue == stencilReadMask );
		}
#endif

		return offsetMode;
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
		SSwapChainData& swapChainData = GetSwapChainData();
		sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

		if ( DSSM_Max == newMode )
		{
			m_depthStencilMode = DSSM_Max;
			m_depthStencilModeNoOffset = DSSM_Max;
			m_stencilRef = stencilRef;
			return;
		}
		
		const EDepthStencilStateMode offsetMode = BuildDepthStencilStateModeIndex( newMode, stencilReadMask, stencilWriteMask );

		if ( m_depthStencilMode != offsetMode || m_stencilRef != stencilRef )
		{
			//HACK GNM set some default states
			sce::Gnm::DepthStencilControl dscontrol;
			dscontrol.init();

			Uint32 stencilOpRef = stencilRef;

			// Depth enable
			switch ( newMode )
			{
			case DSSM_NoStencilDepthTestLE:
			case DSSM_NoStencilDepthTestGE:
			case DSSM_NoStencilDepthTestEQ:
			case DSSM_NoStencilDepthTestLess:
			case DSSM_NoStencilDepthTestAlways:
			case DSSM_NoStencilFullDepthLE:
			case DSSM_NoStencilFullDepthLess:
			case DSSM_LightsFill_FullDepthLE:
			case DSSM_NoStencilFullDepthAlways:
			case DSSM_SetStencil_FullDepthAlways:
			case DSSM_DeferredLights:
			case DSSM_DeferredLightsSkipStencilForward:
            case DSSM_LightsFilterNoneMatch_DepthTestGE:
			case DSSM_NoStencilDepthWriteLE:
			case DSSM_NoStencilDepthTestEqual:
			case DSSM_ST_WriteStencil_FullDepthL:
			case DSSM_ST_WriteStencil_DepthTestE:
			case DSSM_Water_FullDepthLE:
				dscontrol.setDepthEnable( true );
				break;
			case DSSM_NoStencilNoDepth:
			case DSSM_SetStencil_NoDepthAlways:
			case DSSM_LightsFilterAnyMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_NoDepthAlways:
			case DSSM_Scaleform_Disabled:
			case DSSM_Scaleform_StencilClear:
			case DSSM_Scaleform_StencilClearHigher:
			case DSSM_Scaleform_StencilIncrementEqual:
			case DSSM_Scaleform_StencilTestLessEqual:
			case DSSM_ST_WriteStencil_DepthWriteL:
			case DSSM_ST_WriteStencil_NoDepth:
			case DSSM_LightsFilterExactMatch_NoDepthAlways:
				dscontrol.setDepthEnable( false );
				break;
			default:
				GPUAPI_HALT("DEPTH STENCIL: DepthEnable NOT IMPLEMENTED FOR: %d", newMode);
				break;
			}

			// Stencil enable
			switch (newMode)
			{
			case DSSM_LightsFilterAnyMatch_DepthTestGE:
			case DSSM_LightsFilterAnyMatch_DepthTestLE:
			case DSSM_LightsFilterAnyMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_DepthTestGE:
			case DSSM_LightsFilterExactMatch_NoDepthAlways:
			case DSSM_LightsFill_FullDepthLE:
			case DSSM_Water_FullDepthLE:
			case DSSM_DeferredLightsSkipStencilForward:
			case DSSM_DepthLE_StencilSet:
			case DSSM_NoDepth_StencilSet:
			case DSSM_FullDepthLE_StencilSet:
			case DSSM_WriteStencilTest:
			case DSSM_ReadStencilTest:
			case DSSM_SetStencil_NoDepthAlways:
			case DSSM_StencilIncrement_NoDepthAlways:
			case DSSM_StencilDecrement_NoDepthAlways:
			case DSSM_SetStencil_FullDepthAlways:
			case DSSM_ST_WriteStencil_FullDepthL:
			case DSSM_ST_WriteStencil_DepthWriteL:
			case DSSM_ST_WriteStencil_DepthTestE:
			case DSSM_ST_WriteStencil_NoDepth:
			case DSSM_Scaleform_StencilClear:
			case DSSM_Scaleform_StencilClearHigher:
			case DSSM_Scaleform_StencilIncrementEqual:
			case DSSM_Scaleform_StencilTestLessEqual:
				dscontrol.setStencilEnable(true);
				break;
			default:
				dscontrol.setStencilEnable(false);
				break;
			}

			// Depth write
			sce::Gnm::DepthControlZWrite depthWrite = sce::Gnm::kDepthControlZWriteDisable;
			switch ( newMode )
			{
			case DSSM_NoStencilFullDepthLE:
			case DSSM_NoStencilFullDepthLess:
			case DSSM_NoStencilFullDepthAlways:
			case DSSM_LightsFill_FullDepthLE:
			case DSSM_SetStencil_FullDepthAlways:
			case DSSM_NoStencilDepthWriteLE:
			case DSSM_ST_WriteStencil_FullDepthL:
			case DSSM_ST_WriteStencil_DepthWriteL:
			case DSSM_Water_FullDepthLE:
				depthWrite = sce::Gnm::kDepthControlZWriteEnable;
				m_depthWriteEnabled = true;
				break;
			case DSSM_NoStencilDepthTestLE:
			case DSSM_NoStencilDepthTestGE:
			case DSSM_NoStencilDepthTestEQ:
			case DSSM_NoStencilDepthTestLess:
			case DSSM_NoStencilDepthTestAlways:
			case DSSM_NoStencilNoDepth:
			case DSSM_SetStencil_NoDepthAlways:
            case DSSM_LightsFilterNoneMatch_DepthTestGE:
			case DSSM_LightsFilterAnyMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_NoDepthAlways:
			case DSSM_Scaleform_Disabled:
			case DSSM_Scaleform_StencilClear:
			case DSSM_Scaleform_StencilClearHigher:
			case DSSM_Scaleform_StencilIncrementEqual:
			case DSSM_Scaleform_StencilTestLessEqual:
			case DSSM_ST_WriteStencil_DepthTestE:
			case DSSM_ST_WriteStencil_NoDepth:
			case DSSM_LightsFilterExactMatch_NoDepthAlways:
			case DSSM_DeferredLights:
			case DSSM_DeferredLightsSkipStencilForward:
			case DSSM_NoStencilDepthTestEqual:
				depthWrite = sce::Gnm::kDepthControlZWriteDisable;
				m_depthWriteEnabled = false;
				break;
			default:
				GPUAPI_HALT("DEPTH STENCILS DepthWrite NOT IMPLEMENTED: %d", newMode);
				break;
			}

			// Depth compare func
			sce::Gnm::CompareFunc depthCompare = sce::Gnm::kCompareFuncAlways;
			switch ( newMode )
			{
			case DSSM_NoStencilDepthTestLE:
			case DSSM_NoStencilFullDepthLE:
			case DSSM_LightsFill_FullDepthLE:
			case DSSM_NoStencilDepthWriteLE:
			case DSSM_Water_FullDepthLE:
				depthCompare = m_isReversedProjection ? sce::Gnm::kCompareFuncGreaterEqual : sce::Gnm::kCompareFuncLessEqual;
				break;

			case DSSM_NoStencilDepthTestEQ:
				depthCompare = sce::Gnm::kCompareFuncEqual;
				break;

			case DSSM_NoStencilDepthTestGE:
            case DSSM_LightsFilterNoneMatch_DepthTestGE:
                depthCompare = m_isReversedProjection ? sce::Gnm::kCompareFuncLessEqual : sce::Gnm::kCompareFuncGreaterEqual;
                break;

			case DSSM_NoStencilDepthTestLess:
			case DSSM_NoStencilFullDepthLess:
			case DSSM_ST_WriteStencil_FullDepthL:
				depthCompare = m_isReversedProjection ? sce::Gnm::kCompareFuncGreater : sce::Gnm::kCompareFuncLess;
				break;

			case DSSM_DeferredLights:
			case DSSM_DeferredLightsSkipStencilForward:
				depthCompare = m_isReversedProjection ? sce::Gnm::kCompareFuncLess : sce::Gnm::kCompareFuncGreater;
				break;

			case DSSM_NoStencilNoDepth:
			case DSSM_NoStencilFullDepthAlways:
			case DSSM_NoStencilDepthTestAlways:
			case DSSM_SetStencil_NoDepthAlways:
			case DSSM_SetStencil_FullDepthAlways:
			case DSSM_LightsFilterAnyMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_NoDepthAlways:
			case DSSM_Scaleform_Disabled:
			case DSSM_Scaleform_StencilClear:
			case DSSM_Scaleform_StencilClearHigher:
			case DSSM_Scaleform_StencilIncrementEqual:
			case DSSM_Scaleform_StencilTestLessEqual:
			case DSSM_Scaleform_DepthWrite:
			case DSSM_LightsFilterExactMatch_NoDepthAlways:
				depthCompare = sce::Gnm::kCompareFuncAlways;
				break;
			case DSSM_Scaleform_DepthTestEqual:
			case DSSM_ST_WriteStencil_DepthTestE:
			case DSSM_NoStencilDepthTestEqual:
				depthCompare = sce::Gnm::kCompareFuncEqual;
				break;

			default:
				GPUAPI_HALT("DEPTH STENCILS DepthCompare NOT IMPLEMENTED: %d", newMode);
				break;
			}

			// Stencil function
			sce::Gnm::CompareFunc stencilCompare = sce::Gnm::kCompareFuncAlways;
			switch (newMode)
			{
			case DSSM_LightsFilterAnyMatch_DepthTestGE:
			case DSSM_LightsFilterAnyMatch_DepthTestLE:
			case DSSM_LightsFilterAnyMatch_NoDepthAlways:
				{
					stencilCompare = sce::Gnm::kCompareFuncLess;
				}
				break;
			case DSSM_LightsFilterNoneMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_DepthTestGE:
				{
					stencilCompare = sce::Gnm::kCompareFuncGreaterEqual;
				}
				break;
			case DSSM_Scaleform_StencilClearHigher:
			case DSSM_Scaleform_StencilTestLessEqual:
				{
					stencilCompare = sce::Gnm::kCompareFuncLessEqual;
				}
				break;
			case DSSM_Scaleform_StencilIncrementEqual:
			case DSSM_LightsFilterExactMatch_NoDepthAlways:
			case DSSM_StencilIncrement_NoDepthAlways:
			case DSSM_StencilDecrement_NoDepthAlways:
				{
					stencilCompare = sce::Gnm::kCompareFuncEqual;
				}
				break;
			case DSSM_ReadStencilTest:
				{
					stencilCompare = sce::Gnm::kCompareFuncNotEqual;
				}
				break;
			case DSSM_DeferredLightsSkipStencilForward:
				{
					stencilCompare = sce::Gnm::kCompareFuncGreaterEqual;
				}
				break;
			default:
				{
					stencilCompare = sce::Gnm::kCompareFuncAlways;
				}
				break;
			}
			dscontrol.setStencilFunction( stencilCompare );



			// Stencil op
			sce::Gnm::StencilOpControl stencilOpControl;
			stencilOpControl.init();
			switch (newMode)
			{
			case DSSM_Scaleform_StencilClear:
			case DSSM_Scaleform_StencilClearHigher:
			case DSSM_LightsFill_FullDepthLE:
			case DSSM_Water_FullDepthLE:
			case DSSM_DepthLE_StencilSet:
			case DSSM_NoDepth_StencilSet:
			case DSSM_FullDepthLE_StencilSet:
			case DSSM_SetStencil_NoDepthAlways:
			case DSSM_SetStencil_FullDepthAlways:
			case DSSM_ST_WriteStencil_FullDepthL:
			case DSSM_ST_WriteStencil_DepthWriteL:
			case DSSM_ST_WriteStencil_DepthTestE:
			case DSSM_ST_WriteStencil_NoDepth:
				{
					stencilOpControl.setStencilOps( sce::Gnm::kStencilOpKeep, sce::Gnm::kStencilOpReplaceOp, sce::Gnm::kStencilOpKeep );
				}
				break;
			case DSSM_Scaleform_StencilIncrementEqual:
			case DSSM_WriteStencilTest:
			case DSSM_StencilIncrement_NoDepthAlways:
				{
					stencilOpControl.setStencilOps( sce::Gnm::kStencilOpKeep, sce::Gnm::kStencilOpAddWrap, sce::Gnm::kStencilOpKeep );
					stencilOpRef = 1;	// Emulate DX11 functionality of D3D11_STENCIL_OP_INCR which increments by 1
				}
				break;
			case DSSM_StencilDecrement_NoDepthAlways:
				{
					stencilOpControl.setStencilOps( sce::Gnm::kStencilOpKeep, sce::Gnm::kStencilOpSubWrap, sce::Gnm::kStencilOpKeep );
					stencilOpRef = 1;	// Emulate DX11 functionality of D3D11_STENCIL_OP_DECR which decrements by 1
				}
				break;
			default:
				{
					stencilOpControl.setStencilOps( sce::Gnm::kStencilOpKeep, sce::Gnm::kStencilOpKeep, sce::Gnm::kStencilOpKeep );
				}
				break;
			}
			gfxc.setStencilOpControl( stencilOpControl );


			// Stencil values
			sce::Gnm::StencilControl stencilControl;
			stencilControl.init();
			stencilControl.m_mask = 0xFF;
			stencilControl.m_writeMask = 0xFF;
			stencilControl.m_opVal = (Uint8)stencilOpRef;
			stencilControl.m_testVal = (Uint8)stencilRef;


			// Stencil Masks
			switch (newMode)
			{
			case DSSM_LightsFilterAnyMatch_DepthTestGE:
			case DSSM_LightsFilterAnyMatch_DepthTestLE:
			case DSSM_LightsFilterAnyMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_NoDepthAlways:
			case DSSM_LightsFilterNoneMatch_DepthTestGE:
			case DSSM_LightsFilterExactMatch_NoDepthAlways:
				stencilControl.m_writeMask = 0;
				stencilControl.m_mask = stencilReadMask;
				break;
			case DSSM_ReadStencilTest:
				stencilControl.m_writeMask = 0;
				break;
			case DSSM_Water_FullDepthLE:
			case DSSM_DepthLE_StencilSet:
			case DSSM_NoDepth_StencilSet:
			case DSSM_FullDepthLE_StencilSet:
			case DSSM_ST_WriteStencil_FullDepthL:
			case DSSM_ST_WriteStencil_DepthWriteL:
			case DSSM_ST_WriteStencil_DepthTestE:
			case DSSM_ST_WriteStencil_NoDepth:
				stencilControl.m_writeMask = stencilWriteMask;
				break;
			case DSSM_DeferredLightsSkipStencilForward:
				RED_ASSERT( 0 == stencilWriteMask );
				stencilControl.m_writeMask = stencilWriteMask;
				stencilControl.m_mask = stencilReadMask;
				break;
			default:
				//no need to change anything
				break;
			}


			gfxc.setStencil( stencilControl );

			dscontrol.setDepthControl( depthWrite, depthCompare );
			gfxc.setDepthStencilControl( dscontrol );


			// Remember current mode
			m_depthStencilMode = offsetMode;
			m_depthStencilModeNoOffset = newMode;
			m_stencilRef = stencilRef;
		}
	}

	void CRenderStateCache::SetupShadowDepthBias( Float depthBiasClamp, Float slopeScaledDepthBias )
	{
		if ( depthBiasClamp == m_shadowDepthBiasClamp && slopeScaledDepthBias == m_shadowSlopeScaledDepthBias )
		{
			return;
		}

		m_shadowDepthBiasClamp = depthBiasClamp;
		m_shadowSlopeScaledDepthBias = slopeScaledDepthBias;

		// Ensure current settings are bound
		if ( RASTERIZERMODE_Max != m_rasterizerModeForced )
		{
			const ERasterizerMode prevRasterModeOrig = m_rasterizerModeOriginal;
			const ERasterizerMode prevRasterModeForced = m_rasterizerModeForced;
			m_rasterizerModeOriginal = RASTERIZERMODE_Max;
			m_rasterizerModeForced = RASTERIZERMODE_Max;

			SetRasterizerMode( prevRasterModeOrig );

			GPUAPI_ASSERT( prevRasterModeOrig == m_rasterizerModeOriginal );
			GPUAPI_ASSERT( prevRasterModeForced == m_rasterizerModeForced );
			RED_UNUSED(prevRasterModeForced);
		}
	}


    void SetupFrontFace(ERasterizerMode mode, sce::Gnmx::GfxContext &gfxc, sce::Gnm::PrimitiveSetup& primitiveSetup)
    {
        switch (mode)
        {
        case GpuApi::RASTERIZERMODE_ST_NoCull:
        case GpuApi::RASTERIZERMODE_ST_BackCull:
        case GpuApi::RASTERIZERMODE_ST_FrontCull:
        case GpuApi::RASTERIZERMODE_ST_NoCull_ShadowCast:
        case GpuApi::RASTERIZERMODE_ST_BackCull_ShadowCast:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_ShadowCast:
        case GpuApi::RASTERIZERMODE_ST_NoCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_BackCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
            primitiveSetup.setFrontFace(sce::Gnm::kPrimitiveSetupFrontFaceCcw);
            break;

        default:
            primitiveSetup.setFrontFace(sce::Gnm::kPrimitiveSetupFrontFaceCw);
            break;
        }
    }

    void SetupCullMode(ERasterizerMode mode, sce::Gnmx::GfxContext &gfxc, sce::Gnm::PrimitiveSetup& primitiveSetup)
    {
        switch (mode)
        {
        case GpuApi::RASTERIZERMODE_DefaultCullCCW:
        case GpuApi::RASTERIZERMODE_BiasedCCW:
        case GpuApi::RASTERIZERMODE_CSMCullCCW:
        case GpuApi::RASTERIZERMODE_DefaultCullCCW_Wireframe:
        case GpuApi::RASTERIZERMODE_BiasedCCW_Wireframe:
        case GpuApi::RASTERIZERMODE_CSMCullCCW_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_BackCull:
        case GpuApi::RASTERIZERMODE_ST_BackCull_ShadowCast:
        case GpuApi::RASTERIZERMODE_ST_BackCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
            primitiveSetup.setCullFace(sce::Gnm::kPrimitiveSetupCullFaceBack);
            break;

		case GpuApi::RASTERIZERMODE_DefaultCullCW:
		case GpuApi::RASTERIZERMODE_DefaultCullCW_Wireframe:
		case GpuApi::RASTERIZERMODE_ST_FrontCull:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_ShadowCast:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
            primitiveSetup.setCullFace(sce::Gnm::kPrimitiveSetupCullFaceFront);
            break;

        default:
            primitiveSetup.setCullFace(sce::Gnm::kPrimitiveSetupCullFaceNone);
            break;
        }
    }

    void SetupBias(ERasterizerMode mode, sce::Gnmx::GfxContext &gfxc, sce::Gnm::PrimitiveSetup& primitiveSetup, Float slopeScaleBias, Float slopeScaleClamp)
    {
		Float biasValue = -10.0f;
		if ( GetDeviceData().m_StateRenderStateCache.IsReversedProjection() )
		{
			biasValue = -biasValue;
		}

        // PS4 differs from DX11 by a factor of 16
        slopeScaleBias *= 16.0f;

        switch (mode)
        {
            // Cull NONE, Bias only
        case GpuApi::RASTERIZERMODE_BiasedNoCull:
        case GpuApi::RASTERIZERMODE_BiasedNoCull_Wireframe:
            primitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetEnable, sce::Gnm::kPrimitiveSetupPolygonOffsetEnable);
            gfxc.setPolygonOffsetFront(0.0f, biasValue);
            gfxc.setPolygonOffsetBack(0.0f, biasValue);
            gfxc.setPolygonOffsetClamp(0.0f);
            gfxc.setPolygonOffsetClamp(0.0f);
            gfxc.setPolygonOffsetZFormat(sce::Gnm::kZFormat32Float);
            break;

            // Cull CCW, Bias only
        case GpuApi::RASTERIZERMODE_BiasedCCW:
        case GpuApi::RASTERIZERMODE_BiasedCCW_Wireframe:
            primitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetEnable, sce::Gnm::kPrimitiveSetupPolygonOffsetDisable);
            gfxc.setPolygonOffsetFront(0.0f, biasValue);
            gfxc.setPolygonOffsetZFormat(sce::Gnm::kZFormat32Float);
            break;

            // Cull NONE, SlopeScaleBias + Clamped
        case RASTERIZERMODE_CSMNoCull:
        case RASTERIZERMODE_CSMNoCull_Wireframe:
        case RASTERIZERMODE_ST_NoCull_ShadowCast:
        case RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe:
            primitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetEnable, sce::Gnm::kPrimitiveSetupPolygonOffsetEnable);
            gfxc.setPolygonOffsetFront(slopeScaleBias, 0.0f);
            gfxc.setPolygonOffsetBack(slopeScaleBias, 0.0f);
            gfxc.setPolygonOffsetClamp(slopeScaleClamp);
            gfxc.setPolygonOffsetZFormat(sce::Gnm::kZFormat32Float);
            break;

            // Cull CCW, SlopeScaleBias + Clamped
        case RASTERIZERMODE_CSMCullCCW:
        case RASTERIZERMODE_CSMCullCCW_Wireframe:
        case RASTERIZERMODE_ST_BackCull_ShadowCast:
        case RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
            primitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetEnable, sce::Gnm::kPrimitiveSetupPolygonOffsetDisable);
            gfxc.setPolygonOffsetFront(slopeScaleBias, 0.0f);
            gfxc.setPolygonOffsetClamp(slopeScaleClamp);
            gfxc.setPolygonOffsetZFormat(sce::Gnm::kZFormat32Float);
            break;

        case RASTERIZERMODE_ST_FrontCull_ShadowCast:
        case RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
            primitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetDisable, sce::Gnm::kPrimitiveSetupPolygonOffsetEnable);
            gfxc.setPolygonOffsetBack(slopeScaleBias, 0.0f);
            gfxc.setPolygonOffsetClamp(slopeScaleClamp);
            gfxc.setPolygonOffsetZFormat(sce::Gnm::kZFormat32Float);
            break;

        default:
            primitiveSetup.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetDisable, sce::Gnm::kPrimitiveSetupPolygonOffsetDisable);
            break;
        }
    }


    void SetupPolygonMode(ERasterizerMode mode, sce::Gnmx::GfxContext &gfxc, sce::Gnm::PrimitiveSetup& primitiveSetup)
    {
        switch (mode)
        {
        case GpuApi::RASTERIZERMODE_DefaultCullCCW_Wireframe:
        case GpuApi::RASTERIZERMODE_DefaultCullCW_Wireframe:
        case GpuApi::RASTERIZERMODE_DefaultNoCull_Wireframe:
        case GpuApi::RASTERIZERMODE_BiasedCCW_Wireframe:
        case GpuApi::RASTERIZERMODE_BiasedNoCull_Wireframe:
        case GpuApi::RASTERIZERMODE_CSMCullCCW_Wireframe:
        case GpuApi::RASTERIZERMODE_CSMNoCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_NoCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_BackCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
        case GpuApi::RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
            primitiveSetup.setPolygonMode( sce::Gnm::kPrimitiveSetupPolygonModeLine, sce::Gnm::kPrimitiveSetupPolygonModeLine );
            break;

        default:
            primitiveSetup.setPolygonMode( sce::Gnm::kPrimitiveSetupPolygonModeFill, sce::Gnm::kPrimitiveSetupPolygonModeFill );
            break;
        }
    }


	void CRenderStateCache::SetRasterizerMode( ERasterizerMode newMode )
	{
		SSwapChainData& swapChainData = GetSwapChainData();
		sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

		// setup 'original' rasterizer mode always.
		// 'forced' mode is the one that is synced with the device.
		m_rasterizerModeOriginal = newMode;
		
		// Hack: adjust mode based on two sided enforcement
		if ( m_forcedTwoSided )
		{
			newMode = GetTwoSidedRasterizerMode( newMode );
		}

		if ( m_wireframe && newMode < RASTERIZERMODE_WireframeOffset )
		{
			// Offset the mode to the wireframe enabled equivalent
			newMode = (ERasterizerMode)( newMode + RASTERIZERMODE_WireframeOffset );
		}

		

		if ( m_rasterizerModeForced != newMode )
		{
			if (newMode != RASTERIZERMODE_Max)
			{
				//if ( m_rasterizerStates[ newMode ] == NULL )
				//{
				//	CreateRMode(newMode);
				//}

				//// Set the state
				//GetDeviceContext()->RSSetState( m_rasterizerStates[ newMode ] );

				// CullMode						->	setCullFace()
				// FrontCounterClockwise		->	setFrontFace()
				// DepthBias					->	setPolygonOffsetXXX() + setPolygonOffsetEnable()
				// DepthBiasClamp				->	
				// SlopeScaledDepthBias			->	
				// DepthClipEnable				->	
				// ScissorEnable				->	setScreenScissor() ?
				// MultisampleEnable			->	
				// AntialiasedLineEnable		->	
				// FillMode						->	setPolygonMode()

				sce::Gnm::PrimitiveSetup primitiveSetup;
				primitiveSetup.init();

                SetupFrontFace(newMode, gfxc, primitiveSetup);
                SetupCullMode(newMode, gfxc, primitiveSetup);
                SetupBias(newMode, gfxc, primitiveSetup, m_shadowSlopeScaledDepthBias, m_shadowDepthBiasClamp);
                SetupPolygonMode(newMode, gfxc, primitiveSetup);

				gfxc.setPrimitiveSetup( primitiveSetup );

				sce::Gnm::ClipControl clipControl;
				clipControl.init();

				clipControl.setClipEnable( true );
				clipControl.setClipSpace( sce::Gnm::kClipControlClipSpaceDX );
				//clipControl.setCullOnClippingErrorEnable( false ); //???
				//the rest should be the defaults

				gfxc.setClipControl( clipControl );
			}

			// Remember current rasterizer mode
			m_rasterizerModeForced = newMode;
		}
	}


    Uint32 GetRenderTargetWriteMask(EBlendMode mode)
    {
        switch (mode)
        {
        case BLENDMODE_Set_0RTOnlyWrites:                   return 0x0000000F;
        case BLENDMODE_BlendDecals1:                        return 0xFFFF0FF7;
        case BLENDMODE_BlendStripes:                        return 0x00000077;
        case BLENDMODE_BlendDecals2:                        return 0x00000777;
        case BLENDMODE_BlendDecalsNormals:                  return 0xFFFF00F0;
        case BLENDMODE_Set_VolumeExterior:                  return 0x11111111;

        case BLENDMODE_Set_VolumeInterior:                  // falldown
		case BLENDMODE_BlendMinimum_Green:					// falldown
		case BLENDMODE_Set_Green:							return 0x22222222;

		case BLENDMODE_Set_Red:								return 0x11111111;

		case BLENDMODE_Set_Blue:							return 0x44444444;

		case BLENDMODE_Set_Alpha:							return 0x88888888;

		case BLENDMODE_Set_BlueAlpha:						return 0x88888888 | 0x44444444;

        case BLENDMODE_NoColorWrite:
        case BLENDMODE_SF_NoColorWrite:
        case BLENDMODE_ST_Set_NoAtoC_NoColorWrite:
        case BLENDMODE_ST_Set_WithAtoC_NoColorWrite:
        case BLENDMODE_ST_Blending_NoAtoC_NoColorWrite:
        case BLENDMODE_ST_Blending_WithAtoC_NoColorWrite:   return 0x00000000;

        default:                                            return 0xFFFFFFFF;
        }
    }

	void CRenderStateCache::SetBlendMode( EBlendMode newMode )
	{
		if ( m_blendMode != newMode )
		{
			SSwapChainData& swapChainData = GetSwapChainData();
			sce::Gnmx::GfxContext &gfxc = swapChainData.backBuffer->context;

			sce::Gnm::BlendControl blendControl;
			blendControl.init();

			if (newMode != BLENDMODE_Max)
			{
				bool blendEnable = true;

				// BlendEnable
				switch( newMode )
				{
				case BLENDMODE_Set:
				case BLENDMODE_Set_0RTOnlyWrites:
				case BLENDMODE_Set_VolumeExterior:
				case BLENDMODE_Set_VolumeInterior:
				case BLENDMODE_ST_Set_NoAtoC_ColorWrite:
				case BLENDMODE_ST_Set_NoAtoC_NoColorWrite:
				case BLENDMODE_ST_Set_WithAtoC_ColorWrite:
				case BLENDMODE_ST_Set_WithAtoC_NoColorWrite:
					blendEnable = false;
					break;
				default:
					//we need blending
					break;
				}

				// Restore BlendMode for render targets that must be reset
				switch( m_blendMode )
				{
				case BLENDMODE_BlendStripes:
					{
						gfxc.setBlendControl( 1, blendControl );
					}
					break;
				case BLENDMODE_BlendDecals2:
					{
						gfxc.setBlendControl( 1, blendControl );
						gfxc.setBlendControl( 2, blendControl );
					}
					break;
				default:
					break;
				}

				// Set BlendControl for new mode
				blendControl.setSeparateAlphaEnable( false );
				blendControl.setBlendEnable( blendEnable );

				// Alpha channel blend - we don't have independent blend so this shouldn't matter for now
				sce::Gnm::BlendMultiplier blendSrcMul = sce::Gnm::kBlendMultiplierOne;
				sce::Gnm::BlendFunc blendFunc = sce::Gnm::kBlendFuncAdd;
				sce::Gnm::BlendMultiplier blendDstMul = sce::Gnm::kBlendMultiplierZero;

				// Color channels blend
				blendSrcMul = sce::Gnm::kBlendMultiplierOne;
				blendFunc = sce::Gnm::kBlendFuncAdd;
				blendDstMul = sce::Gnm::kBlendMultiplierZero;

				switch ( newMode )
				{
				case BLENDMODE_Set:
				case BLENDMODE_Set_Red:
				case BLENDMODE_Set_Green:
				case BLENDMODE_Set_Blue:
				case BLENDMODE_Set_Alpha:
				case BLENDMODE_Set_BlueAlpha:
				case BLENDMODE_Set_0RTOnlyWrites:
				case BLENDMODE_NoColorWrite:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierOne;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierZero;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_Add:
				case BLENDMODE_Add_BlueAlpha:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierOne;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierOne;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_Mul:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierZero;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierSrcColor;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_BlendMinimum_Green:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierOne;
						blendFunc = sce::Gnm::kBlendFuncMin;
						blendDstMul = sce::Gnm::kBlendMultiplierOne;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_BlendMaximum:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierOne;
						blendFunc = sce::Gnm::kBlendFuncMax;
						blendDstMul = sce::Gnm::kBlendMultiplierOne;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_Blend:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierSrcAlpha;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierOneMinusSrcAlpha;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_PremulBlend:
				case BLENDMODE_SF_FilterBlend_SourceAc:
				case BLENDMODE_SF_Normal_SourceAc:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierOne;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierOneMinusSrcAlpha;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_BlendDecals2:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierSrcAlpha;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierOneMinusSrcAlpha;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
						gfxc.setBlendControl( 1, blendControl );
						gfxc.setBlendControl( 2, blendControl );
					}
					break;
                case BLENDMODE_BlendStripes:
                    {
                        blendSrcMul = sce::Gnm::kBlendMultiplierSrcAlpha;
                        blendFunc = sce::Gnm::kBlendFuncAdd;
                        blendDstMul = sce::Gnm::kBlendMultiplierOneMinusSrcAlpha;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
						gfxc.setBlendControl( 1, blendControl );
                    }
                    break;
				case BLENDMODE_SF_None:
				case BLENDMODE_SF_Normal:
				case BLENDMODE_SF_NoColorWrite:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierSrcAlpha;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierOneMinusSrcAlpha;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				case BLENDMODE_SF_Add:
					{
						blendSrcMul = sce::Gnm::kBlendMultiplierSrcAlpha;
						blendFunc = sce::Gnm::kBlendFuncAdd;
						blendDstMul = sce::Gnm::kBlendMultiplierOne;
						blendControl.setColorEquation( blendSrcMul, blendFunc, blendDstMul );
						gfxc.setBlendControl( 0, blendControl );
					}
					break;
				default:
					GPUAPI_ASSERT(!blendEnable, TXT("BLENDMODE: [%d] NOT IMPLEMENTED"), newMode);
					break;
				}

                Uint32 renderTargetMask = GetRenderTargetWriteMask(newMode);
                gfxc.setRenderTargetMask(renderTargetMask);
			}
			// Remember current blend mode
			m_blendMode = newMode;
		}
	}
}
