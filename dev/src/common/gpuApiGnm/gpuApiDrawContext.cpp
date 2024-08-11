/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"


	// ace_optimize: w zwiazku z drawcontextem bedzie lekka korbka bo ten shit jest naduzywany. zrobic lazy shit czy cos takiego.

namespace GpuApi
{

	// ----------------------------------------------------------------------

	void SetDrawContext( eDrawContext newContext, Uint32 newRefValue )
	{
		SDeviceData &dd = GetDeviceData();
		if ( dd.m_StateDrawContext == newContext && dd.m_StateDrawContextRef == newRefValue )
		{
			return;
		}

		const eDrawContext prevContext = dd.m_StateDrawContext;
		const Uint32 prevRefValue = dd.m_StateDrawContextRef;
		dd.m_StateDrawContext = newContext;
		dd.m_StateDrawContextRef = newRefValue;
		CRenderStateCache &rs = dd.m_StateRenderStateCache;	

		// SetForcedTwoSided should not be touched in functions below !			
		switch ( newContext )
		{
		case DRAWCONTEXT_Default:						// falldown		
		case DRAWCONTEXT_GBufferSolid:					// falldown
		case DRAWCONTEXT_DebugUnlit:					// falldown
		case DRAWCONTEXT_DebugOccluders:				// falldown
		case DRAWCONTEXT_DebugMesh:						// falldown
		case DRAWCONTEXT_ShadowMapGenCube:				// falldown
		case DRAWCONTEXT_HitProxiesSolid:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;
		case DRAWCONTEXT_GlobalShadow:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Set_Green );
			}
			break;
		case DRAWCONTEXT_Unlit:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set_0RTOnlyWrites );
			}
			break;

		case DRAWCONTEXT_OcclusionQueries:
			{
				GPUAPI_HALT(  "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_Emissive:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_DecalsModulative:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_BiasedCCW );
				rs.SetBlendMode( BLENDMODE_Mul );
			}
			break;

		case DRAWCONTEXT_HiResShadows:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_BlendMinimum_Green );
			}
			break;

		case DRAWCONTEXT_EyeOverlay:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Mul );
			}
			break;

		case DRAWCONTEXT_RefractionBuffer:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_RefractionAccumBuffer:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_ReflectionMask:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_OpaqueNoDepthWrite:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_Transparency:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_PremulBlend );
			}
			break;

		case DRAWCONTEXT_HairOpaque:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestEqual, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_HairTransparency:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLess, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_PremulBlend );
			}
			break;

		case DRAWCONTEXT_TransparentBackground:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_PremulBlend );
			}
			break;

		case DRAWCONTEXT_LowResParticles:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_TransparencyCombine:
			{	
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_Sprites:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Blend );

				//rs.SetAlphaTestMode( ALPHATEST_Greater_80 );
			}
			break;
		case DRAWCONTEXT_OverlayFillDepthCompare:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;
		case DRAWCONTEXT_Overlay:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;
		case DRAWCONTEXT_GBufferSolid_StencilLightsFill:
			{
				rs.SetDepthStencilMode( DSSM_LightsFill_FullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_GBufferDecalsBlended:
			{
				// Draw backfaces, and use GE depth test. This prevents clipping when decals are close to the camera.
				// Only draw where the provided stencil bits are not set (e.g. only on static geometry).
				rs.SetDepthStencilMode( DSSM_LightsFilterNoneMatch_DepthTestGE, 0, static_cast<Uint8>( newRefValue ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCW );
				rs.SetBlendMode( BLENDMODE_BlendDecals2 );
			}
			break;
		case DRAWCONTEXT_DecalsFocusMode:
			{
				// Draw backfaces, and use GE depth test. This prevents clipping when decals are close to the camera.
				// Only draw where the provided stencil bits are not set (e.g. only on static geometry).
				rs.SetDepthStencilMode( DSSM_LightsFilterNoneMatch_DepthTestGE, 0, static_cast<Uint8>( newRefValue ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCW );
				rs.SetBlendMode( BLENDMODE_Blend );
			}
			break;

		case DRAWCONTEXT_DebugTransparent:			// falldown
		case DRAWCONTEXT_DebugOverlay:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Blend );
			}
			break;

		case DRAWCONTEXT_ShadowMapGenCSM_DepthTex:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_CSMCullCCW );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;

		//dex++
		case DRAWCONTEXT_ShadowMapGenCSM_Terrain:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;
		//dex--

		case DRAWCONTEXT_LightFullscreenModulative:
		case DRAWCONTEXT_LightFullscreenAdditive:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_LightModulativeInside:		// falldown
		case DRAWCONTEXT_LightAdditiveInside:
			{
				const bool isModulative = (DRAWCONTEXT_LightModulativeInside == newContext);

				GPUAPI_ASSERT( newRefValue >= 0 && newRefValue < 256 );

				rs.SetDepthStencilMode( DSSM_LightsFilterAnyMatch_DepthTestGE, 0, static_cast<Uint8>( newRefValue ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCW );
				rs.SetBlendMode( isModulative ? BLENDMODE_Mul : BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_LightModulativeOutside:	// falldown
		case DRAWCONTEXT_LightAdditiveOutside:
			{
				const bool isModulative = (DRAWCONTEXT_LightModulativeOutside == newContext);

				GPUAPI_ASSERT( newRefValue >= 0 && newRefValue < 256 );

				rs.SetDepthStencilMode( DSSM_LightsFilterAnyMatch_DepthTestLE, 0, static_cast<Uint8>( newRefValue ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( isModulative ? BLENDMODE_Mul : BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_LightModulativeInsideHiStencilOpt:		// falldown
		case DRAWCONTEXT_LightAdditiveInsideHiStencilOpt:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_LightModulativeOutsideHiStencilOpt:	// falldown
		case DRAWCONTEXT_LightAdditiveOutsideHiStencilOpt:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;
		case DRAWCONTEXT_LightAdditiveOutsideStencilMaskedPre:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_LightAdditiveOutsideStencilMasked:
		case DRAWCONTEXT_LightAdditiveOutsideStencilMaskedModulative:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_LightAdditiveOutsideStencilMaskedPreInside:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_LightAdditiveOutsideStencilMaskedInside:
		case DRAWCONTEXT_LightAdditiveOutsideStencilMaskedInsideModulative:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_DeferredLights:
			{
				rs.SetDepthStencilMode( DSSM_DeferredLights );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCW );
				rs.SetBlendMode( BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_DeferredLightsSkipStencilForward:
			{
				rs.SetDepthStencilMode( DSSM_DeferredLightsSkipStencilForward, 0, newRefValue, 0 );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCW );
				rs.SetBlendMode( BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_NoColor_DepthStencilSet:
			{
				rs.SetDepthStencilMode( DSSM_SetStencil_FullDepthAlways, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;

		case DRAWCONTEXT_PostProcSet:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcSet_BlueAlpha:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Set_BlueAlpha );
			}
			break;

		case DRAWCONTEXT_PostProcAdd_BlueAlpha:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Add_BlueAlpha );
			}
			break;

		case DRAWCONTEXT_PostProcNoColor_SetStencilFull:
			{	
				rs.SetDepthStencilMode( DSSM_SetStencil_NoDepthAlways, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;

		case DRAWCONTEXT_PostProcSet_SetStencilMasked:
			{
				rs.SetDepthStencilMode( DSSM_NoDepth_StencilSet, static_cast<Uint8>( newRefValue&0xFF ) , static_cast<Uint8>( (newRefValue>>8)&0xFF ) , static_cast<Uint8>( (newRefValue>>16)&0xFF ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcNoColor_SetStencilBits:
			{	
				GPUAPI_ASSERT( newRefValue >= 0 && newRefValue < 256 );

				rs.SetDepthStencilMode( DSSM_NoDepth_StencilSet, 0xff, 0, static_cast<Uint8>( newRefValue ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;

		case DRAWCONTEXT_PostProcSet_DepthEqual:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestEQ , 0 , 0 , 0 );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcSet_DepthWrite:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthAlways, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcSet_StencilMatchAny:
			{	
				GPUAPI_ASSERT( newRefValue >= 0 && newRefValue < 256 );

				rs.SetDepthStencilMode( DSSM_LightsFilterAnyMatch_NoDepthAlways, 0, static_cast<Uint8>( newRefValue ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcSet_StencilMatchExact:
			{
				Uint8 refValue, readMask;
				UnpackDrawContextRefValue( newRefValue, &refValue, &readMask, NULL );
				rs.SetDepthStencilMode( DSSM_LightsFilterExactMatch_NoDepthAlways, refValue, readMask );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcPremulBlend_StencilMatchExact:
			{
				Uint8 refValue, readMask;
				UnpackDrawContextRefValue( newRefValue, &refValue, &readMask, NULL );
				rs.SetDepthStencilMode( DSSM_LightsFilterExactMatch_NoDepthAlways, refValue, readMask );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_PremulBlend );
			}
			break;

		case DRAWCONTEXT_PostProcAdd_StencilMatchExact:
			{
				Uint8 refValue, readMask;
				UnpackDrawContextRefValue( newRefValue, &refValue, &readMask, NULL );
				rs.SetDepthStencilMode( DSSM_LightsFilterExactMatch_NoDepthAlways, refValue, readMask );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_PostProcSet_StencilMatchNone:
			{	
				GPUAPI_ASSERT( newRefValue >= 0 && newRefValue < 256 );
				rs.SetDepthStencilMode( DSSM_LightsFilterNoneMatch_NoDepthAlways, 0, static_cast<Uint8>( newRefValue ) );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcSet_HiStencilMatchAny:
			{	
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_SkinPost:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_PostProcAdd:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Add );
			}
			break;

		case DRAWCONTEXT_PostProcMax:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_BlendMaximum );
			}
			break;

		case DRAWCONTEXT_DynamicDecalPremulBlend:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_PremulBlend );
			}
			break;

		case DRAWCONTEXT_PostProcBlend:
			{	
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Blend );
			}
			break;

		case DRAWCONTEXT_Scaleform2D:
			{
				if ( newRefValue != -1 )
				{
					Bool setBlendType = true; 
					Bool setStencilType = true; 

					const eGUIBlendStateType blendTypeNew = static_cast< eGUIBlendStateType >( newRefValue & 0xFF );
					const eGUIStencilModeType stencilModeNew = static_cast< eGUIStencilModeType >( (newRefValue & 0xFF00) >> 8 );
					const Uint32	stencilRefNew = ( (newRefValue & 0xFF0000) >> 16 );

					if ( prevContext == DRAWCONTEXT_Scaleform2D && (prevRefValue != -1) )
					{
						const eGUIBlendStateType blendTypeOld = static_cast< eGUIBlendStateType >( prevRefValue & 0xFF );
						const eGUIStencilModeType stencilModeOld = static_cast< eGUIStencilModeType >( (prevRefValue & 0xFF00) >> 8 );
						const Uint32	stencilRefOld = ( (prevRefValue & 0xFF0000) >> 16 );

						if ( blendTypeNew == blendTypeOld )
						{
							setBlendType = false;
						}
						if (( stencilModeNew == stencilModeOld ) && ( stencilRefOld == stencilRefNew))
						{
							setStencilType = false;
						}
					}
					else
					{
						// Some common setup to all scaleform stuff, we will do this only once
						rs.SetWireframe( false );
						rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
					}

					if ( setBlendType )
					{
						rs.SetBlendModeScaleform( blendTypeNew );
					}

					if ( setStencilType )
					{
						rs.SetStencilModeScaleform( stencilModeNew, stencilRefNew );
					}
					break;
				}
			}
		case DRAWCONTEXT_2D:	// falldown only if not using ref values and new scaleform renderer
			{
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );

				if ( newRefValue == -1 )
				{
					rs.SetBlendMode( BLENDMODE_Blend );
				}
				else
				{
					rs.SetBlendMode( BLENDMODE_Set );
				}
			}
			break;

		case DRAWCONTEXT_TexRect2D:
			{
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_Text2D:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Blend );
			}
			break;

		case DRAWCONTEXT_RestoreZ:
			{	
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_RestoreDepth:
			{	
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_RestoreStencilCullEqual:
		case DRAWCONTEXT_RestoreStencilCullNotEqual:
			{	
				GPUAPI_HALT( "Unsupported draw context" );
			}
			break;

		case DRAWCONTEXT_HitProxiesSprite:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );

				//rs.SetAlphaTestMode( ALPHATEST_Greater_80 );
			}
			break;

		case DRAWCONTEXT_FlaresOcclusionFull:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestAlways, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;

		case DRAWCONTEXT_FlaresOcclusionPart:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;

		case DRAWCONTEXT_FlaresDraw:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_PremulBlend );

			}
			break;

		case DRAWCONTEXT_NoColor_DepthLE_StencilClear:
			{
				Uint8 writeMask;
				UnpackDrawContextRefValue( newRefValue, NULL, NULL, &writeMask );
				rs.SetDepthStencilMode( DSSM_DepthLE_StencilSet, 0, 0, writeMask );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_NoColorWrite );
			}
			break;

		case DRAWCONTEXT_ScreenFade:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilNoDepth, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Blend );
			}
			break;

		// TESTING PURPOSE ONLY
		case DRAWCONTEXT_SimpleNoCull:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, 0 );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;
		case DRAWCONTEXT_RenderStatesTest_DrawShitToStencil:
			{
				rs.SetDepthStencilMode( DSSM_WriteStencilTest, 0 );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Blend );
			}
			break;
		case DRAWCONTEXT_RenderStatesTest_ReadFromStencil:
			{
				rs.SetDepthStencilMode( DSSM_ReadStencilTest, 0 );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Blend );
			}
			break;
		case DRAWCONTEXT_Particles:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultNoCull );
				rs.SetBlendMode( BLENDMODE_Add );
			}
			break;
        case DRAWCONTEXT_Terrain:
            {
                rs.SetDepthStencilMode( DSSM_LightsFill_FullDepthLE, newRefValue );
                rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
                rs.SetBlendMode( BLENDMODE_Set );
            }
            break;
        case DRAWCONTEXT_GBufferProjectedStripes:
            {
                rs.SetDepthStencilMode( DSSM_LightsFilterAnyMatch_NoDepthAlways, 0, static_cast<Uint8>( newRefValue ) );
                rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
                rs.SetBlendMode( BLENDMODE_BlendStripes );
            }
            break;
        case DRAWCONTEXT_GBufferStripes:
            {
                rs.SetDepthStencilMode( DSSM_NoStencilDepthTestLE, newRefValue );
                rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
                rs.SetBlendMode( BLENDMODE_BlendStripes );
            }
            break;

		case DRAWCONTEXT_GlobalWater:
			{
				rs.SetDepthStencilMode( DSSM_Water_FullDepthLE, newRefValue, 0, (Uint8)newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set );
			}
			break;

		case DRAWCONTEXT_VolumeExterior:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCCW );
				rs.SetBlendMode( BLENDMODE_Set_VolumeExterior );				
			}
			break;

		case DRAWCONTEXT_VolumeInterior:
			{
				rs.SetDepthStencilMode( DSSM_NoStencilFullDepthLE, newRefValue );				
				rs.SetRasterizerMode( RASTERIZERMODE_DefaultCullCW );
				rs.SetBlendMode( BLENDMODE_Set_VolumeInterior );			
			}
			break;


		default:
			GPUAPI_HALT( "invalid context: %d", newContext );
		}
	}

	void SetCustomDrawContext( EDepthStencilStateMode depthStencilMode, ERasterizerMode rasterizerMode, EBlendMode blendMode, Uint32 packedRefValue /*= 1*/ )
	{
		SDeviceData &dd = GetDeviceData();
		dd.m_StateDrawContext = DRAWCONTEXT_Default;
		dd.m_StateDrawContextRef = packedRefValue;


		Uint8 stencilRef, stencilRead, stencilWrite;
		UnpackDrawContextRefValue( packedRefValue, &stencilRef, &stencilRead, &stencilWrite );

		CRenderStateCache &rs = dd.m_StateRenderStateCache;
		rs.SetDepthStencilMode( depthStencilMode, stencilRef, stencilRead, stencilWrite );
		rs.SetRasterizerMode( rasterizerMode );
		rs.SetBlendMode( blendMode );
	}

	eDrawContext GetDrawContext()
	{
		return GetDeviceData().m_StateDrawContext;
	}	

	Uint32 GetDrawContextRefValue()
	{
		return GetDeviceData().m_StateDrawContextRef;
	}

	void SetDrawContextRefValue( Uint8 refValue )
	{
		SDeviceData &dd = GetDeviceData();
		dd.m_StateDrawContextRef = refValue;

		Uint8 stencilRef, stencilRead, stencilWrite;
		UnpackDrawContextRefValue( refValue, &stencilRef, &stencilRead, &stencilWrite );

		CRenderStateCache &rs = dd.m_StateRenderStateCache;
		rs.SetDepthStencilMode( rs.GetDepthStencilModeNoOffset(), stencilRef, stencilRead, stencilWrite );
	}

	Bool IsForcedTwoSidedRender()
	{
		return GetDeviceData().m_StateRenderStateCache.IsForcedTwoSided();
	}

	void SetForcedTwoSidedRender( bool force )
	{
		GetDeviceData().m_StateRenderStateCache.SetForcedTwoSided( force );
	}

	Bool IsReversedProjectionState()
	{
		return GetDeviceData().m_StateRenderStateCache.IsReversedProjection();
	}

	void SetReversedProjectionState( bool isReversed )
	{
		GetDeviceData().m_StateRenderStateCache.SetReversedProjection( isReversed );
	}

	Float GetClearDepthValueRevProjAware() 
	{ 
		return IsReversedProjectionState() ? 0.f : 1.f; 
	}
	
	void SetupShadowDepthBias( Float depthBiasClamp, Float slopeScaledDepthBias )
	{
		GetDeviceData().m_StateRenderStateCache.SetupShadowDepthBias( depthBiasClamp, slopeScaledDepthBias );
	}

}
