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
		case DSSM_SetStencil_NoDepthAlways:
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

	void CRenderStateCache::CreateDSMode( EDepthStencilStateMode newMode, Uint8 stencilReadMask /* = 0 */, Uint8 stencilWriteMask /* = 0 */, Bool isReversedProjection )
	{
		const EDepthStencilStateMode offsetMode = BuildDepthStencilStateModeIndex( newMode, stencilReadMask, stencilWriteMask );

		// Setup state for the first time
		D3D11_DEPTH_STENCIL_DESC desc;
		desc.FrontFace.StencilFailOp =		D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilFailOp =		D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilDepthFailOp =	D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilPassOp =		D3D11_STENCIL_OP_KEEP;
		desc.BackFace.StencilPassOp =		D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc =		D3D11_COMPARISON_ALWAYS;
		desc.BackFace.StencilFunc =			D3D11_COMPARISON_ALWAYS;
		desc.DepthWriteMask =				D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc =					isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
		desc.StencilWriteMask = 0xFF;
		desc.StencilReadMask = 0xFF;

		switch ( newMode )
		{
		case DSSM_NoStencilFullDepthLE:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_NoStencilFullDepthLess:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER : D3D11_COMPARISON_LESS;
			break;
		case DSSM_NoStencilFullDepthAlways:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_NoStencilDepthTestLE:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_NoStencilDepthTestGE:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_GREATER_EQUAL;
			break;
		case DSSM_NoStencilDepthTestEQ:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_EQUAL;
			break;
		case DSSM_NoStencilDepthTestLess:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER : D3D11_COMPARISON_LESS;
			break;
		case DSSM_NoStencilNoDepth:
			desc.StencilEnable = false;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_NoStencilDepthTestAlways:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_NoStencilDepthWriteLE:
			desc.StencilEnable = false;
			desc.DepthEnable = false;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_NoStencilDepthTestEqual:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_EQUAL;
			break;
		case DSSM_LightsFilterAnyMatch_DepthTestGE:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS;
			desc.StencilWriteMask = 0;
			desc.StencilReadMask = stencilReadMask;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_GREATER_EQUAL;
			break;
		case DSSM_LightsFilterAnyMatch_DepthTestLE:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS;
			desc.StencilWriteMask = 0;
			desc.StencilReadMask = stencilReadMask;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_LightsFilterAnyMatch_NoDepthAlways:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS;
			desc.StencilWriteMask = 0;
			desc.StencilReadMask = stencilReadMask;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_LightsFilterNoneMatch_NoDepthAlways:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
			desc.StencilWriteMask = 0;
			desc.StencilReadMask = stencilReadMask;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_LightsFilterNoneMatch_DepthTestGE:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
			desc.StencilWriteMask = 0;
			desc.StencilReadMask = stencilReadMask;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_GREATER_EQUAL;
			break;
		case DSSM_LightsFilterExactMatch_NoDepthAlways:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.StencilWriteMask = 0;
			desc.StencilReadMask = stencilReadMask;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_LightsFill_FullDepthLE:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.DepthEnable = true;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_Water_FullDepthLE:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.StencilWriteMask = stencilWriteMask;			
			desc.DepthEnable = true;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_DepthLE_StencilSet:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.StencilWriteMask = stencilWriteMask;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_NoDepth_StencilSet:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.StencilWriteMask = stencilWriteMask;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_FullDepthLE_StencilSet:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.StencilWriteMask = stencilWriteMask;
			desc.DepthEnable = true;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
			break;
			// TESTING ONLY
		case DSSM_WriteStencilTest:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_ReadStencilTest:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
			desc.StencilWriteMask = 0;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_SetStencil_NoDepthAlways:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_StencilIncrement_NoDepthAlways:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_StencilDecrement_NoDepthAlways:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_SetStencil_FullDepthAlways:
			desc.StencilEnable = true;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_DeferredLights:
			desc.StencilEnable = false;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_GREATER;
			break;
		case DSSM_DeferredLightsSkipStencilForward:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
			RED_ASSERT( 0 == stencilWriteMask );
			desc.StencilWriteMask = stencilWriteMask;
			desc.StencilReadMask = stencilReadMask;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_GREATER;
			break;
		case DSSM_Scaleform_Invalid:
			desc.StencilEnable	= true;
			desc.DepthEnable	= true;
			desc.DepthFunc	= D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_Scaleform_Disabled:
			desc.StencilEnable	= false;
			desc.DepthEnable	= false;
			desc.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc	= D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_Scaleform_StencilClear:
			desc.StencilEnable	= true;
			desc.DepthEnable	= false;
			desc.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc	= D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			break;
		case DSSM_Scaleform_StencilClearHigher:
			desc.StencilEnable	= true;
			desc.DepthEnable	= false;
			desc.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc	= D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			break;
		case DSSM_Scaleform_StencilIncrementEqual:
			desc.StencilEnable	= true;
			desc.DepthEnable	= false;
			desc.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc	= D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
			break;
		case DSSM_Scaleform_StencilTestLessEqual:
			desc.StencilEnable	= true;
			desc.DepthEnable	= false;
			desc.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc	= D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
			break;
		case DSSM_Scaleform_DepthWrite:
			desc.StencilEnable	= false;
			desc.DepthEnable	= true;
			desc.DepthFunc	= D3D11_COMPARISON_ALWAYS;
			break;
		case DSSM_Scaleform_DepthTestEqual:
			desc.StencilEnable	= false;
			desc.DepthEnable	= true;
			desc.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc	= D3D11_COMPARISON_EQUAL;
			break;
		case DSSM_ST_WriteStencil_FullDepthL:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.StencilWriteMask = stencilWriteMask;
			desc.StencilReadMask = 0xFF;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER : D3D11_COMPARISON_LESS;
			break;
		case DSSM_ST_WriteStencil_DepthWriteL:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.StencilWriteMask = stencilWriteMask;
			desc.StencilReadMask = 0xFF;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = isReversedProjection ? D3D11_COMPARISON_GREATER : D3D11_COMPARISON_LESS;
			break;
		case DSSM_ST_WriteStencil_DepthTestE:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.StencilWriteMask = stencilWriteMask;
			desc.StencilReadMask = 0xFF;
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_EQUAL;
			break;
		case DSSM_ST_WriteStencil_NoDepth:
			desc.StencilEnable = true;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.StencilWriteMask = stencilWriteMask;
			desc.StencilReadMask = 0xFF;
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_EQUAL;
			break;

		default:
			GPUAPI_HALT( "invalid depth stencil state" );
		}

		// Create state object
		ID3D11DepthStencilState* depthStencilState = NULL;
		// CreateDepthStencilState checks for identical states already loaded. If it finds one, it returns pointer to it, instead of having a duplicate.
		// I'm not sure if the AddRef is called in such case, but I will assume it does for the moment.
		GPUAPI_MUST_SUCCEED( GetDevice()->CreateDepthStencilState( &desc, &depthStencilState ) );
#ifdef GPU_API_DEBUG_PATH
		const char* debugName = "DSS";
		depthStencilState->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( debugName ) - 1, debugName );
#endif
		m_depthStencilStates[ isReversedProjection ? 1 : 0 ][ offsetMode ] = depthStencilState;
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
			const Uint32 reversedIndex = m_isReversedProjection ? 1 : 0;

			// Make sure mode created
			if ( m_depthStencilStates[ reversedIndex ][ offsetMode ] == NULL )
			{
				CreateDSMode( newMode, stencilReadMask, stencilWriteMask, m_isReversedProjection );
			}

			// Set the state
			GetDeviceContext()->OMSetDepthStencilState( m_depthStencilStates[ reversedIndex ][ offsetMode ], stencilRef );

			// Remember current mode
			m_depthStencilMode = offsetMode;
			m_depthStencilModeNoOffset = newMode;
			m_stencilRef = stencilRef;
		}
	}

	void CRenderStateCache::SetupShadowDepthBias( Float depthBiasClamp, Float slopeScaledDepthBias )
	{
		const ERasterizerMode modesToRefresh[] = 
		{ 
			RASTERIZERMODE_CSMCullCCW, 
			RASTERIZERMODE_CSMCullCCW_Wireframe, 
			RASTERIZERMODE_CSMNoCull, 
			RASTERIZERMODE_CSMNoCull_Wireframe,
			RASTERIZERMODE_ST_NoCull_ShadowCast,
			RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe,
			RASTERIZERMODE_ST_BackCull_ShadowCast,
			RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe,
			RASTERIZERMODE_ST_FrontCull_ShadowCast,
			RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe
		};
		
		// Ensure at least one raster states created
		if ( m_rasterStates.Empty() )
		{
			SRasterizerStates* states = new SRasterizerStates();
			states->m_shadowDepthBiasClamp = depthBiasClamp;
			states->m_shadowSlopeScaledDepthBias = slopeScaledDepthBias;
			m_rasterStates.PushBack( states );
		}

		// Find raster state index matching our settings
		Int32 matchingRasterStateIndex = -1;
		for ( Uint32 i=0; i<m_rasterStates.Size(); ++i )
		{
			const SRasterizerStates &states = *m_rasterStates[i];
			if ( states.m_shadowDepthBiasClamp == depthBiasClamp && states.m_shadowSlopeScaledDepthBias == slopeScaledDepthBias )
			{
				matchingRasterStateIndex = (Int32)i;
				break;
			}
		}

		// If current raster state matches, then nothing to do here
		if ( 0 == matchingRasterStateIndex )
		{
			return;
		}

		// If we didn't find rasterStates, then we need to create new one, or recycle existing one
		if ( -1 == matchingRasterStateIndex )
		{
			if ( !m_rasterStates.Full() )
			{
				GPUAPI_LOG( TXT("RasterizerStates created") );
				m_rasterStates.PushBack( new SRasterizerStates () );
			}
			else
			{
				GPUAPI_LOG( TXT("RasterizerStates recycled") );

				SRasterizerStates *backupStates = m_rasterStates.Back(); //< recycle the one that wasn't used longest (e.g. the last one)
				for ( Uint32 mode_idx_i=0; mode_idx_i<ARRAY_COUNT(modesToRefresh); ++mode_idx_i )
				{
					ERasterizerMode rasterMode = modesToRefresh[mode_idx_i];
					SAFE_RELEASE( backupStates->m_states[rasterMode] );				
				}
			}

			matchingRasterStateIndex = (Int32)m_rasterStates.Size() - 1;
			m_rasterStates[matchingRasterStateIndex]->m_shadowDepthBiasClamp = depthBiasClamp;
			m_rasterStates[matchingRasterStateIndex]->m_shadowSlopeScaledDepthBias = slopeScaledDepthBias;

			for ( Uint32 i=0; i<RASTERIZERMODE_Max; ++i )
			{
				GPUAPI_ASSERT( nullptr == m_rasterStates[matchingRasterStateIndex]->m_states[i] );
			}
		}

		// Move our backup raster states right after the current raster states (e.g. position 1).
		// Raster states are sorted by when were they last time used (most recently used comes first)
		GPUAPI_ASSERT( matchingRasterStateIndex > 0 && matchingRasterStateIndex < (Int32)m_rasterStates.Size() );
		if ( 1 != matchingRasterStateIndex )
		{
			SRasterizerStates *backupStates = m_rasterStates[matchingRasterStateIndex];
			m_rasterStates.Remove( matchingRasterStateIndex );
			m_rasterStates.Insert( 1, backupStates );
			matchingRasterStateIndex = 1;
		}

		// Swap shadows related states, and setting between 
		// current raster states object, and the matching one.
		{
			SRasterizerStates *states0 = m_rasterStates[0];
			SRasterizerStates *states1 = m_rasterStates[matchingRasterStateIndex];
			Red::Math::NumericalUtils::Swap( states0->m_shadowDepthBiasClamp, states1->m_shadowDepthBiasClamp );
			Red::Math::NumericalUtils::Swap( states0->m_shadowSlopeScaledDepthBias, states1->m_shadowSlopeScaledDepthBias );
			for ( Uint32 mode_idx_i=0; mode_idx_i<ARRAY_COUNT(modesToRefresh); ++mode_idx_i )
			{
				ERasterizerMode rasterMode = modesToRefresh[mode_idx_i];
				Red::Math::NumericalUtils::Swap( states0->m_states[rasterMode], states1->m_states[rasterMode] );
			}
		}

		// Ensure current settings are bound
		if ( RASTERIZERMODE_Max != m_rasterizerModeForced )
		{
			const ERasterizerMode prevRasterModeOrig = m_rasterizerModeOriginal;
#ifndef NO_GPU_ASSERTS
			const ERasterizerMode prevRasterModeForced = m_rasterizerModeForced;
#endif
			m_rasterizerModeOriginal = RASTERIZERMODE_Max;
			m_rasterizerModeForced = RASTERIZERMODE_Max;

			SetRasterizerMode( prevRasterModeOrig );

			GPUAPI_ASSERT( prevRasterModeOrig == m_rasterizerModeOriginal );
			GPUAPI_ASSERT( prevRasterModeForced == m_rasterizerModeForced );
		}
	}

	void CRenderStateCache::CreateRMode( ERasterizerMode newMode )
	{
		if ( m_rasterStates.Empty() )
		{
			m_rasterStates.PushBack( new SRasterizerStates () );
		}

		SRasterizerStates &currRasterStates = *m_rasterStates[0];

		// Setup rasterizer state for the first time.
		D3D11_RASTERIZER_DESC desc;

		// Fill common values
		desc.FrontCounterClockwise = false;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = true;
		desc.ScissorEnable = false;
		desc.MultisampleEnable = false;
		desc.AntialiasedLineEnable = false;
		if ( newMode < RASTERIZERMODE_WireframeOffset )
		{
			desc.FillMode = D3D11_FILL_SOLID;
		}
		else
		{
			desc.FillMode = D3D11_FILL_WIREFRAME;
		}

		// File mode specific values
		switch ( newMode )
		{
		case RASTERIZERMODE_DefaultCullCCW:
		case RASTERIZERMODE_DefaultCullCCW_Wireframe:
			{
				desc.CullMode = D3D11_CULL_BACK;
			}
			break;
		case RASTERIZERMODE_BiasedCCW:
		case RASTERIZERMODE_BiasedCCW_Wireframe:
			{
				desc.CullMode = D3D11_CULL_BACK;
				desc.DepthBias = -10;						// TODO : Need to handle reversed projection somehow
			}
			break;
		case RASTERIZERMODE_BiasedNoCull:
		case RASTERIZERMODE_BiasedNoCull_Wireframe:
			{
				desc.CullMode = D3D11_CULL_NONE;
				desc.DepthBias = -10;						// TODO : Need to handle reversed projection somehow
			}
			break;

		case RASTERIZERMODE_CSMCullCCW:
		case RASTERIZERMODE_CSMCullCCW_Wireframe:
			{
				desc.CullMode = D3D11_CULL_BACK;
				desc.DepthBiasClamp = currRasterStates.m_shadowDepthBiasClamp;
				desc.SlopeScaledDepthBias = currRasterStates.m_shadowSlopeScaledDepthBias;
			}
			break;

		case RASTERIZERMODE_CSMNoCull:
		case RASTERIZERMODE_CSMNoCull_Wireframe:
			{
				desc.CullMode = D3D11_CULL_NONE;
				desc.DepthBiasClamp = currRasterStates.m_shadowDepthBiasClamp;
				desc.SlopeScaledDepthBias = currRasterStates.m_shadowSlopeScaledDepthBias;
			}
			break;

		case RASTERIZERMODE_DefaultCullCW:
		case RASTERIZERMODE_DefaultCullCW_Wireframe:
			{
				desc.CullMode = D3D11_CULL_FRONT;
			}
			break;

		case RASTERIZERMODE_DefaultNoCull:
		case RASTERIZERMODE_DefaultNoCull_Wireframe:
			{
				desc.CullMode = D3D11_CULL_NONE;
			}
			break;

			// Speed Tree
		case RASTERIZERMODE_ST_NoCull:
		case RASTERIZERMODE_ST_NoCull_Wireframe:
			{
				desc.CullMode = D3D11_CULL_NONE;
				desc.FrontCounterClockwise = true;
				desc.MultisampleEnable = false;
			}
			break;
		case RASTERIZERMODE_ST_BackCull:
		case RASTERIZERMODE_ST_BackCull_Wireframe:
			{
				desc.CullMode = D3D11_CULL_BACK;
				desc.FrontCounterClockwise = true;
				desc.MultisampleEnable = false;
			}
			break;
		case RASTERIZERMODE_ST_FrontCull:
		case RASTERIZERMODE_ST_FrontCull_Wireframe:
			{
				desc.CullMode = D3D11_CULL_FRONT;
				desc.FrontCounterClockwise = true;
				desc.MultisampleEnable = false;
			}
			break;
		case RASTERIZERMODE_ST_NoCull_ShadowCast:
		case RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe:
			{
				desc.CullMode = D3D11_CULL_NONE;
				desc.FrontCounterClockwise = true;
				desc.MultisampleEnable = false;
				desc.DepthBiasClamp = currRasterStates.m_shadowDepthBiasClamp;
				desc.SlopeScaledDepthBias = currRasterStates.m_shadowSlopeScaledDepthBias;
			}
			break;
		case RASTERIZERMODE_ST_BackCull_ShadowCast:
		case RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
			{
				desc.CullMode = D3D11_CULL_BACK;
				desc.FrontCounterClockwise = true;
				desc.MultisampleEnable = false;
				desc.DepthBiasClamp = currRasterStates.m_shadowDepthBiasClamp;
				desc.SlopeScaledDepthBias = currRasterStates.m_shadowSlopeScaledDepthBias;
			}
			break;
		case RASTERIZERMODE_ST_FrontCull_ShadowCast:
		case RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
			{
				desc.CullMode = D3D11_CULL_FRONT;
				desc.FrontCounterClockwise = true;
				desc.MultisampleEnable = false;
				desc.DepthBiasClamp = currRasterStates.m_shadowDepthBiasClamp;
				desc.SlopeScaledDepthBias = currRasterStates.m_shadowSlopeScaledDepthBias;
			}
			break;

		default:
			GPUAPI_HALT( "invalid rasterizer mode" );
		}

		// Create state object
		ID3D11RasterizerState* rasterizerState = NULL;
		// CreateRasterizerState checks for identical states already loaded. If it finds one, it returns pointer to it, instead of having a duplicate.
		// I'm not sure if the AddRef is called in such case, but I will assume it does for the moment.
		GPUAPI_MUST_SUCCEED( GetDevice()->CreateRasterizerState( &desc, &rasterizerState ) );
#ifdef GPU_API_DEBUG_PATH
		const char* debugName = "RS";
		rasterizerState->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( debugName ) - 1, debugName );
#endif
		
		GPUAPI_ASSERT( nullptr == currRasterStates.m_states[ newMode ] );
		currRasterStates.m_states[ newMode ] = rasterizerState;
	}

	void CRenderStateCache::SetRasterizerMode( ERasterizerMode newMode )
	{
		if ( m_rasterStates.Empty() )
		{
			m_rasterStates.PushBack( new SRasterizerStates () );
		}

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
				if ( m_rasterStates[0]->m_states[ newMode ] == NULL )
				{
					CreateRMode(newMode);
				}

				// Set the state
				GetDeviceContext()->RSSetState( m_rasterStates[0]->m_states[ newMode ] );
			}
			// Remember current rasterizer mode
			m_rasterizerModeForced = newMode;
		}
	}

	void CRenderStateCache::CreateBMode( EBlendMode newMode )
	{
		// Setup blend state for the first time.
		D3D11_BLEND_DESC desc;

		// Set common values
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[1].BlendEnable = true;
		desc.RenderTarget[2].BlendEnable = true;
		desc.RenderTarget[3].BlendEnable = true;
		desc.RenderTarget[4].BlendEnable = true;
		desc.RenderTarget[5].BlendEnable = true;
		desc.RenderTarget[6].BlendEnable = true;
		desc.RenderTarget[7].BlendEnable = true;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		// Set blend mode specific values
		switch ( newMode )
		{
		case BLENDMODE_Set:
			desc.AlphaToCoverageEnable = false;
			// In this case, disable blend for all RTs
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Set_Red:
			desc.AlphaToCoverageEnable = false;
			// In this case, disable blend for all RTs
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_RED;
			desc.RenderTarget[0].BlendOp =					D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =				D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =				D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =					D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =			D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Set_Green:
			desc.AlphaToCoverageEnable = false;
			// In this case, disable blend for all RTs
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_GREEN;
			desc.RenderTarget[0].BlendOp =					D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =				D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =				D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =					D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =			D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Set_Blue:
			desc.AlphaToCoverageEnable = false;
			// In this case, disable blend for all RTs
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_BLUE;
			desc.RenderTarget[0].BlendOp =					D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =				D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =				D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =					D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =			D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Set_Alpha:
			desc.AlphaToCoverageEnable = false;
			// In this case, disable blend for all RTs
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_ALPHA;
			desc.RenderTarget[0].BlendOp =					D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =				D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =				D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =					D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =			D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Add_BlueAlpha:						
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_ALPHA;
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;			
			break;

		case BLENDMODE_Set_BlueAlpha:
			desc.AlphaToCoverageEnable = false;
			// In this case, disable blend for all RTs
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_ALPHA;
			desc.RenderTarget[0].BlendOp =					D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =				D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =				D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =					D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =			D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Set_0RTOnlyWrites:
			desc.AlphaToCoverageEnable = false;
			// In this case, disable blend for all RTs
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[1].RenderTargetWriteMask = 0;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = 0;
			desc.RenderTarget[5].RenderTargetWriteMask = 0;
			desc.RenderTarget[6].RenderTargetWriteMask = 0;
			desc.RenderTarget[7].RenderTargetWriteMask = 0;
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Add:				
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Substract:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_REV_SUBTRACT;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_REV_SUBTRACT;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_Mul:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_SRC_COLOR;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_DEST_ALPHA;
			break;

		case BLENDMODE_BlendMinimum_Green:
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_GREEN;
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MIN;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MIN;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_BlendMaximum:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MAX;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MAX;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Blend:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_BlendDecals1:
			// D3DRS_SEPARATEALPHABLENDENABLE - this shit was set in dx9. Seems like no equivalent exists in dx10, but note it just in case.
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_BlendStripes:
			desc.IndependentBlendEnable = true;

			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = 0;
			desc.RenderTarget[5].RenderTargetWriteMask = 0;
			desc.RenderTarget[6].RenderTargetWriteMask = 0;
			desc.RenderTarget[7].RenderTargetWriteMask = 0;

			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;

			desc.RenderTarget[1].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[1].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[1].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[1].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[1].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[1].SrcBlendAlpha =	D3D11_BLEND_ZERO;

			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			break;

		case BLENDMODE_BlendDecals2:
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
			desc.RenderTarget[1].RenderTargetWriteMask = 0;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_BlendDecalsNormals:
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_PremulBlend:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_ParticleCombine:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_ParticleLowRes:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_NoColorWrite:
			// IndependentBlendEnable is false, so only need to set RT[0]
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			break;

		case BLENDMODE_Set_VolumeExterior:
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_RED;
			desc.RenderTarget[0].BlendOp =					D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =				D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =				D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =					D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =			D3D11_BLEND_ONE;
			break;

		case BLENDMODE_Set_VolumeInterior:
			desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_GREEN;
			desc.RenderTarget[0].BlendOp =					D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =				D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =				D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =					D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =			D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_None:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;
		
		case BLENDMODE_SF_Normal:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Layer:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Multiply:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_DEST_COLOR;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Screen:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_INV_DEST_COLOR;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Lighten:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MAX;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MAX;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Darken:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MIN;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MIN;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Difference:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Add:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Subtract:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_REV_SUBTRACT;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Invert:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_INV_DEST_COLOR;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Alpha:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_Erase:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_Overlay:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_HardLight:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Overwrite:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_OverwriteAll:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_FullAdditive:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_FilterBlend:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Ignore:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_None_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Normal_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Layer_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Multiply_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_DEST_COLOR; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Screen_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_INV_DEST_COLOR; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Lighten_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MAX;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MAX;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Darken_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_MIN;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_MIN;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Difference_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Add_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Subtract_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_REV_SUBTRACT;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Invert_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_INV_DEST_COLOR; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			break;

		case BLENDMODE_SF_Alpha_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_Erase_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_Overlay_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha	=	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_HardLight_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Overwrite_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_OverwriteAll_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_FullAdditive_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_FilterBlend_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ONE; // *** NOT SAME ***
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

		case BLENDMODE_SF_Ignore_SourceAc:
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_ZERO; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ZERO;
			break;

		case BLENDMODE_SF_NoColorWrite:
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			desc.RenderTarget[0].BlendOp =			D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha =		D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend =		D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlend =			D3D11_BLEND_SRC_ALPHA; // SAME!
			desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
			break;

			// Speed Tree
		case BLENDMODE_ST_Set_NoAtoC_ColorWrite:
			desc.AlphaToCoverageEnable = false;
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;
		case BLENDMODE_ST_Set_NoAtoC_NoColorWrite:
			desc.AlphaToCoverageEnable = false;
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			desc.RenderTarget[1].RenderTargetWriteMask = 0;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = 0;
			desc.RenderTarget[5].RenderTargetWriteMask = 0;
			desc.RenderTarget[6].RenderTargetWriteMask = 0;
			desc.RenderTarget[7].RenderTargetWriteMask = 0;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;
		case BLENDMODE_ST_Set_WithAtoC_ColorWrite:
			desc.AlphaToCoverageEnable = true;
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;
		case BLENDMODE_ST_Set_WithAtoC_NoColorWrite:
			desc.AlphaToCoverageEnable = true;
			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			desc.RenderTarget[1].RenderTargetWriteMask = 0;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = 0;
			desc.RenderTarget[5].RenderTargetWriteMask = 0;
			desc.RenderTarget[6].RenderTargetWriteMask = 0;
			desc.RenderTarget[7].RenderTargetWriteMask = 0;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;
		case BLENDMODE_ST_Blending_NoAtoC_ColorWrite:
			desc.AlphaToCoverageEnable = false;
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;
		case BLENDMODE_ST_Blending_NoAtoC_NoColorWrite:
			desc.AlphaToCoverageEnable = false;
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			desc.RenderTarget[1].RenderTargetWriteMask = 0;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = 0;
			desc.RenderTarget[5].RenderTargetWriteMask = 0;
			desc.RenderTarget[6].RenderTargetWriteMask = 0;
			desc.RenderTarget[7].RenderTargetWriteMask = 0;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;
		case BLENDMODE_ST_Blending_WithAtoC_ColorWrite:
			desc.AlphaToCoverageEnable = true;
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[4].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[5].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[6].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[7].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;
		case BLENDMODE_ST_Blending_WithAtoC_NoColorWrite:
			desc.AlphaToCoverageEnable = true;
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[2].BlendEnable = false;
			desc.RenderTarget[3].BlendEnable = false;
			desc.RenderTarget[4].BlendEnable = false;
			desc.RenderTarget[5].BlendEnable = false;
			desc.RenderTarget[6].BlendEnable = false;
			desc.RenderTarget[7].BlendEnable = false;
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			desc.RenderTarget[1].RenderTargetWriteMask = 0;
			desc.RenderTarget[2].RenderTargetWriteMask = 0;
			desc.RenderTarget[3].RenderTargetWriteMask = 0;
			desc.RenderTarget[4].RenderTargetWriteMask = 0;
			desc.RenderTarget[5].RenderTargetWriteMask = 0;
			desc.RenderTarget[6].RenderTargetWriteMask = 0;
			desc.RenderTarget[7].RenderTargetWriteMask = 0;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			break;

		default:
			GPUAPI_HALT( "Invalid blend mode" );
			return;
		}

		// Create state object
		ID3D11BlendState* blendState = NULL;
		// CreateBlendState checks for identical states already loaded. If it finds one, it returns pointer to it, instead of having a duplicate.
		// I'm not sure if the AddRef is called in such case, but I will assume it does for the moment.
		GPUAPI_MUST_SUCCEED( GetDevice()->CreateBlendState( &desc, &blendState ) );
#ifdef GPU_API_DEBUG_PATH
		const char* debugName = "BS";
		blendState->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( debugName ) - 1, debugName );
#endif
		m_blendStates[ newMode ] = blendState;
	}

	void CRenderStateCache::SetBlendMode( EBlendMode newMode )
	{
		if ( m_blendMode != newMode )
		{
			if (newMode != BLENDMODE_Max)
			{
				if ( m_blendStates[ newMode ] == NULL )
				{
					CreateBMode(newMode);
				}

				// Set the state
				GetDeviceContext()->OMSetBlendState( m_blendStates[ newMode ], NULL, 0xFFFFFFFF );
			}
			// Remember current blend mode
			m_blendMode = newMode;
		}
	}
}
