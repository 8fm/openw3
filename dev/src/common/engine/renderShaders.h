

// Present shaders
RENDER_SHADER_GEN( m_postFXAA,									TXT("postfx_present.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_AA"), ("1") ) );

// Finalize
RENDER_SHADER_GEN( m_postfxFinalize,							TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxFinalizeVignetteTex,					TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_VIGNETTE"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFinalizeVignetteProc,				TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_VIGNETTE"), ("2") ) );
RENDER_SHADER_GEN( m_postfxFinalizeBalance,						TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_BALANCE"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFinalizeBalanceVignetteTex,			TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_BALANCE"), ("1") ).Add( ("ENABLE_VIGNETTE"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFinalizeBalanceVignetteProc,			TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_BALANCE"), ("1") ).Add( ("ENABLE_VIGNETTE"), ("2") ) );
RENDER_SHADER_GEN( m_postfxFinalizeAberration,					TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_ABERRATION"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFinalizeAberrationVignetteTex,		TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_ABERRATION"), ("1") ).Add( ("ENABLE_VIGNETTE"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFinalizeAberrationVignetteProc,		TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_ABERRATION"), ("1") ).Add( ("ENABLE_VIGNETTE"), ("2") ) );
RENDER_SHADER_GEN( m_postfxFinalizeAberrationBalance,			TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_ABERRATION"), ("1") ).Add( ("ENABLE_BALANCE"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFinalizeAberrationBalanceVignetteTex,TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_ABERRATION"), ("1") ).Add( ("ENABLE_BALANCE"), ("1") ).Add( ("ENABLE_VIGNETTE"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFinalizeAberrationBalanceVignetteProc,TXT("postfx_finalize.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_ABERRATION"), ("1") ).Add( ("ENABLE_BALANCE"), ("1") ).Add( ("ENABLE_VIGNETTE"), ("2") ) );

// Copy/clear/add shaders
RENDER_SHADER_GEN( m_postFXCopy,								TXT("postfx_copy.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXCopyDecode,							TXT("postfx_copyDecode.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXCopyScale,							TXT("postfx_copyScale.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXDeprojectDepth,						TXT("postfx_deprojectDepth.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXDeprojectDepthMSAA,					TXT("postfx_deprojectDepth.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("MULTISAMPLED_DEPTH"), ("1") ) );
RENDER_SHADER_GEN( m_postFXCopyDepth,							TXT("postfx_copyDepth.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXCopyWithBalanceMapSingle,			TXT("postfx_copyWithBalanceMap.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXCopyWithBalanceMapDouble,			TXT("postfx_copyWithBalanceMap.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_SECOND_BALANCEMAP"), ("1") ) );
RENDER_SHADER_GEN( m_postFXCopyWithBalanceMapSingleBlendSingle,	TXT("postfx_copyWithBalanceMap.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_BALANCEMAP_BLENDING"), ("1") ) );
RENDER_SHADER_GEN( m_postFXCopyWithBalanceMapSingleBlendDouble,	TXT("postfx_copyWithBalanceMap.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_BALANCEMAP_BLENDING"), ("1") ).Add( ("ENABLE_SECOND_BALANCEMAPB"), ("1") ) );
RENDER_SHADER_GEN( m_postFXCopyWithBalanceMapDoubleBlendSingle,	TXT("postfx_copyWithBalanceMap.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_BALANCEMAP_BLENDING"), ("1") ).Add( ("ENABLE_SECOND_BALANCEMAP"), ("1") ) );
RENDER_SHADER_GEN( m_postFXCopyWithBalanceMapDoubleBlendDouble,	TXT("postfx_copyWithBalanceMap.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_BALANCEMAP_BLENDING"), ("1") ).Add( ("ENABLE_SECOND_BALANCEMAP"), ("1") ).Add( ("ENABLE_SECOND_BALANCEMAPB"), ("1") ) );
RENDER_SHADER_GEN( m_postFXClear,								TXT("postfx_clear.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXClearDepth,							TXT("postfx_clearDepth.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXSharpen,								TXT("postfx_sharpen.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXPaintEffect0,						TXT("postfx_paintEffect.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("SAMPLES_NUM_SELECTION"), ("0") ) );
RENDER_SHADER_GEN( m_postFXPaintEffect1,						TXT("postfx_paintEffect.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("SAMPLES_NUM_SELECTION"), ("1") ) );
RENDER_SHADER_GEN( m_postFXPaintEffect2,						TXT("postfx_paintEffect.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("SAMPLES_NUM_SELECTION"), ("2") ) );
RENDER_SHADER_GEN( m_postFXClear_FocusMode,						TXT("postfx_clear.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("FOCUS_MODE_CLEAR"), ("1") ) );

RENDER_SHADER_GEN( m_postFXTemporalAA,							TXT("postfx_temporalAA.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postFXTemporalAALum,						TXT("postfx_temporalAA.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_LUM_CALC", "1" ) );

RENDER_SHADER_GEN( m_simpleClearDepth,							TXT("simpleClear.fx"), RENDER_SHADER_DEFINES_BASE().Add( "CLEARDEPTH", "1" ) );
RENDER_SHADER_GEN( m_simpleClear,								TXT("simpleClear.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_simpleClear32,								TXT("simpleClear.fx"), RENDER_SHADER_DEFINES_BASE().Add( "OUTPUT32", "1" ) );
RENDER_SHADER_COMPUTE( m_simpleClearCS,							TXT("simpleClear.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_COMPUTE( m_simpleClearBufferCS,					TXT("simpleClear.fx"), RENDER_SHADER_DEFINES_BASE().Add("CLEAR_BUFFER", "1") );
RENDER_SHADER_GEN( m_simpleCopy,								TXT("simpleCopy.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_simpleCopy32_R,							TXT("simpleCopy.fx"), RENDER_SHADER_DEFINES_BASE().Add( "OUTPUT_32_R", "1" ) );
RENDER_SHADER_GEN( m_simpleCopy32_RG,							TXT("simpleCopy.fx"), RENDER_SHADER_DEFINES_BASE().Add( "OUTPUT_32_RG", "1" ) );
RENDER_SHADER_GEN( m_simpleCopy32_RGBA,							TXT("simpleCopy.fx"), RENDER_SHADER_DEFINES_BASE().Add( "OUTPUT_32_RGBA", "1" ) );
RENDER_SHADER_COMPUTE( m_simpleCopyCS,							TXT("simpleCopy.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_simpleCopyAlpha,							TXT("simpleCopyAlpha.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_simpleCopyKeyMasking,						TXT("simpleCopyKeyMasking.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_simpleCopyScale,							TXT("simpleCopy.fx"), RENDER_SHADER_DEFINES_BASE().Add("SCALE_COLOR","1") );
RENDER_SHADER_COMPUTE( m_computeCull,							TXT("simpleCull.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_simpleDrawCsVs,							TXT("simpleDrawCsVs.fx"), RENDER_SHADER_DEFINES_BASE() );

// Local reflections
RENDER_SHADER_GEN(  m_shaderRLRCalculateWithOcean,				TXT("rlr_calculate.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ENABLE_OCEAN", "1" ) );
RENDER_SHADER_GEN(  m_shaderRLRCalculateNoOcean,				TXT("rlr_calculate.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ENABLE_OCEAN", "0" ) );
RENDER_SHADER_GEN(  m_shaderRLRMerge,							TXT("rlr_calculate.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_MERGE", "1" ) );
RENDER_SHADER_GEN(  m_shaderRLRExtrude,							TXT("rlr_calculate.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_EXTRUDE", "1" ) );
RENDER_SHADER_GEN(  m_shaderRLRMergeExtruded,					TXT("rlr_calculate.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_MERGE_EXTRUDED", "1" ) );
RENDER_SHADER_GEN(  m_shaderRLRDownsampleSourceColor,			TXT("rlr_downsampleSource.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_COLOR", "1" ) );
RENDER_SHADER_GEN(  m_shaderRLRDownsampleSourceDepth,			TXT("rlr_downsampleSource.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_DEPTH", "1" ) );
RENDER_SHADER_GEN(  m_shaderRLRApplyTraced,						TXT("rlr_apply.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_FULL_APPLY", "1" ) );
RENDER_SHADER_GEN(  m_shaderRLRApplyFallback,					TXT("rlr_apply.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_FULL_APPLY", "0" ) );

// Dual paraboloid
RENDER_SHADER_GEN(  m_shaderDualParaboloidGen,					TXT("dualParaboloidGen.fx"), RENDER_SHADER_DEFINES_BASE() );

// Summed area tables
RENDER_SHADER_GEN(  m_shaderSummedAreaTableGen,					TXT("summedAreaTableGen.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN(  m_shaderSummedAreaTableFetch,				TXT("summedAreaTableFetch.fx"), RENDER_SHADER_DEFINES_BASE() );

// Generic xbox/pc shit
RENDER_SHADER_GEN( m_occlusionTest,								TXT("occlusionTest.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_ALPHA_FETCH"), ("1") ) );
RENDER_SHADER_GEN( m_occlusionHelperGrab,						TXT("occlusionTest.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_HELPER_COLOR_GRAB"), ("1") ) );

// DOF
RENDER_SHADER_GEN( m_postfxBokehDof,							TXT("postfx_bokehdof.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxCircleOfConfusion,					TXT("postfx_circleOfConfusion.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxCircleOfConfusionNoFar,				TXT("postfx_circleOfConfusion.fx"), RENDER_SHADER_DEFINES_BASE().Add("NO_FAR" , "1") );
RENDER_SHADER_GEN( m_postfxCircleOfConfusionPhysical,			TXT("postfx_circleOfConfusion.fx"), RENDER_SHADER_DEFINES_BASE().Add("PHYSICAL","1") );
RENDER_SHADER_GEN( m_postfxBokehDof2Pass,						TXT("postfx_bokehdof_2pass.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxBokehDofCoCBlur,						TXT("postfx_circleOfConfusionBlur.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxBokehDownsample,						TXT("postfx_bokehdof_colorDownsample.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxDofBlurCS,							TXT("postfx_dofBlurCS.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxDofMergeCS,							TXT("postfx_dofMergeCS.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxDofBlurNoFarCS,						TXT("postfx_dofBlurCS.fx"), RENDER_SHADER_DEFINES_BASE().Add("NO_FAR" , "1") );
RENDER_SHADER_GEN( m_postfxDofMergeNoFarCS,						TXT("postfx_dofMergeCS.fx"), RENDER_SHADER_DEFINES_BASE().Add("NO_FAR" , "1") );
RENDER_SHADER_GEN( m_postfxDofGameplayPrepare,					TXT("postfx_dofBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_GAMEPLAY_PREPARE", "1" ) );
RENDER_SHADER_GEN( m_postfxDofGameplayApply,					TXT("postfx_dofBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_GAMEPLAY_APPLY", "1" ) );

// Tonemapping and bloom
RENDER_SHADER_GEN( m_postfxToneMappingApply,					TXT("postfx_toneMappingApply.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxToneMappingApplyFixedLuminance,		TXT("postfx_toneMappingApply.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("FIXED_LUMINANCE") , ("1") ) );
RENDER_SHADER_GEN( m_postfxToneMappingApplyFixedLuminanceBlend,	TXT("postfx_toneMappingApply.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("FIXED_LUMINANCE") , ("1") ).Add( ("ENABLE_CURVE_BLENDING"), ("1") ) );
RENDER_SHADER_GEN( m_postfxToneMappingApplyBlend,				TXT("postfx_toneMappingApply.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_CURVE_BLENDING"), ("1") ) );
RENDER_SHADER_GEN( m_postfxToneMappingApplyDebug,				TXT("postfx_toneMappingApply.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("SCURVE_DISPLAY"), ("1") ) );
RENDER_SHADER_GEN( m_postfxToneMappingApplyBlendDebug,			TXT("postfx_toneMappingApply.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_CURVE_BLENDING"), ("1") ).Add( ("SCURVE_DISPLAY"), ("1") ) );
RENDER_SHADER_GEN( m_postfxToneMappingAdaptSimple,				TXT("postfx_toneMappingAdapt.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("SIMPLE_ADAPT"), ("1") ) );

// Bloom
RENDER_SHADER_GEN( m_postfxBloomSkyFilter,						TXT("postfx_bloomDownscale.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_SKY_FILTER"), ("1") ) );
RENDER_SHADER_GEN( m_postfxBloomBrightpass,						TXT("postfx_bloomDownscale.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_BRIGHTPASS"), ("1") ) );
RENDER_SHADER_GEN( m_postfxBloomDownscale,						TXT("postfx_bloomDownscale.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxBloomGauss,							TXT("postfx_bloomGaussCombine.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_GAUSS"), ("1") ) );
RENDER_SHADER_GEN( m_postfxBloomCombineDirtOff,					TXT("postfx_bloomGaussCombine.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_COMBINE_DIRT_OFF"), ("1") ) );
RENDER_SHADER_GEN( m_postfxBloomCombineDirtOn,					TXT("postfx_bloomGaussCombine.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_COMBINE_DIRT_ON"), ("1") ) );
RENDER_SHADER_GEN( m_postfxBloomGaussCombine,					TXT("postfx_bloomGaussCombine.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("ENABLE_GAUSS"), ("1") ).Add( ("ENABLE_COMBINE_DIRT_OFF"), ("1") ) );
RENDER_SHADER_GEN( m_postfxBloomShaftsQ0,						TXT("postfx_bloomShafts.fx"), RENDER_SHADER_DEFINES_BASE().Add( "QUALITY_LEVEL", "0" ) );
RENDER_SHADER_GEN( m_postfxBloomShaftsQ1,						TXT("postfx_bloomShafts.fx"), RENDER_SHADER_DEFINES_BASE().Add( "QUALITY_LEVEL", "1" ) );
RENDER_SHADER_GEN( m_postfxBloomShaftsQ2,						TXT("postfx_bloomShafts.fx"), RENDER_SHADER_DEFINES_BASE().Add( "QUALITY_LEVEL", "2" ) );

// Blur shaders
RENDER_SHADER_GEN( m_postfxBlur,								TXT("postfx_blur.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxBlurGauss,							TXT("postfx_blurGauss.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxBlurGaussNice,						TXT("postfx_blurGaussNice.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxBlurGaussStrong,						TXT("postfx_blurGaussStrong.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxBlurGaussStrongLess,					TXT("postfx_blurGaussStrong.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("LESS"), ("1") ) );
RENDER_SHADER_GEN( m_postfxMotionBlurDownsample,				TXT("postfx_motionBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_DOWNSAMPLE"), ("1") ) );
RENDER_SHADER_GEN( m_postfxMotionBlurCalc,						TXT("postfx_motionBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_CALC"), ("1") ) );
RENDER_SHADER_GEN( m_postfxMotionBlurApply,						TXT("postfx_motionBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_APPLY"), ("1") ) );
RENDER_SHADER_GEN( m_postfxMotionBlurSharpen,					TXT("postfx_motionBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_SHARPEN"), ("1") ) );
RENDER_SHADER_GEN( m_postfxLoadingBlur,							TXT("postfx_loadingBlur.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxLoadingBlurFinal,					TXT("postfx_loadingBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( "FINAL_BLEND" , "1" ) );

// Underwater
RENDER_SHADER_GEN( m_postfxUnderwater,							TXT("postfx_underwater.fx"), RENDER_SHADER_DEFINES_BASE() );

// Surface flow - rain
RENDER_SHADER_GEN( m_postfxSurfaceFlow,							TXT("postfx_surfaceFlow.fx"), RENDER_SHADER_DEFINES_BASE() );

// Full screen wetness gain
RENDER_SHADER_GEN( m_gbuffPostProc,								TXT("gbuffPostProc.fx"), RENDER_SHADER_DEFINES_BASE() );

// Flares/shafts/sky
RENDER_SHADER_GEN( m_postfxFlareGrabFullscreen,					TXT("postfx_flareGrab.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxFlareGrabPoint,						TXT("postfx_flareGrab.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("POINT_LIGHT_SHAFTS"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFlareGrabSpot,						TXT("postfx_flareGrab.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("SPOT_LIGHT_SHAFTS"), ("1") ).Add( ("POINT_LIGHT_SHAFTS"), ("1") ) );
RENDER_SHADER_GEN( m_postfxFlareFilter,							TXT("postfx_flareFilter.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxFlareApplyAdd,						TXT("postfx_flareApply.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_postfxFlareApplyScreen,					TXT("postfx_flareApply.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_SCREEN"), ("1") ) );

// Gameplay post FX
RENDER_SHADER_GEN( m_gameplayPostFXSurface,						TXT("gameplayPostFXSurface.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_gameplayPostFXSepia,						TXT("gameplayPostFXSepia.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_gameplayPostFXCatView,						TXT("gameplayPostFXCatView.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_gameplayPostFXCatViewObjects,				TXT("gameplayPostFXCatViewObjects.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_gameplayPostFXDrunk,						TXT("gameplayPostFXDrunk.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_gameplayPostFXFocus,						TXT("gameplayPostFXFocus.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_gameplayCharacterOutline,					TXT("gameplayCharacterOutline.fx"), RENDER_SHADER_DEFINES_BASE() );

// Debug shaders
RENDER_SHADER_GEN( m_shaderPlain,								TXT("debugColor.fx"), CMaterialCompilerDefines() );
RENDER_SHADER_GEN( m_shaderSingleColor,							TXT("debugSingleColor.fx"), CMaterialCompilerDefines() );
RENDER_SHADER_GEN( m_shaderCubeTexture,							TXT("debugCubeTexture.fx"), CMaterialCompilerDefines() );
RENDER_SHADER_GEN( m_shaderEnvProbe,							TXT("debugEnvProbe.fx"), CMaterialCompilerDefines() );
RENDER_SHADER_GEN( m_shaderTextured,							TXT("debugTexture.fx"), CMaterialCompilerDefines().Add( ("ENABLE_EXPONENT"), ("1") ) );
RENDER_SHADER_GEN( m_shaderTexturedNoAlpha,						TXT("debugTexture.fx"), CMaterialCompilerDefines().Add( ("ENABLE_EXPONENT"), ("1") ).Add( ("NO_ALPHA"), ("1") ) );
RENDER_SHADER_GEN( m_shaderAlpha,								TXT("debugAlpha.fx"), CMaterialCompilerDefines().Add( ("ENABLE_EXPONENT"), ("1") ) );
RENDER_SHADER_GEN( m_shaderHitProxy,							TXT("debugHitProxy.fx"), CMaterialCompilerDefines() );
RENDER_SHADER_GEN( m_shaderTextNormal,							TXT("textNormal.fx"), CMaterialCompilerDefines().Add( ("ENABLE_EXPONENT"), ("1") ) );
RENDER_SHADER_GEN( m_shaderTextOutline,							TXT("textOutline.fx"), CMaterialCompilerDefines() );
RENDER_SHADER_GEN( m_postfxDebug,								TXT("postfx_debug.fx"), RENDER_SHADER_DEFINES_BASE() );

// Camera interior factor
RENDER_SHADER_GEN( m_shaderCameraInteriorFactor,				TXT("cameraInteriorFactor.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_COMPUTE( m_shaderPatchCameraInteriorFactor,		TXT("cameraInteriorFactor.fx"), RENDER_SHADER_DEFINES_BASE().Add( "IS_CONSTANTBUFFER_PATCH", "1" ) );

//dex++: shaders for previewing textures
RENDER_SHADER_GEN( m_shaderTexturePreview2D,					TXT("debugTexturePreview.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderTexturePreviewArray,					TXT("debugTexturePreview.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("TEX_ARRAY"), ("1") ) );
RENDER_SHADER_GEN( m_shaderTexturePreviewCube,					TXT("debugTexturePreview.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("CUBE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderTexturePreviewCubeArray,				TXT("debugTexturePreview.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("TEX_ARRAY"), ("1") ).Add( ("CUBE"), ("1") ) );
//dex--

//dex++: shaders for terrain shadows
RENDER_SHADER_GEN( m_shaderTerrainPatchDraw,					TXT("terrainShadowsPatch.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderTerrainShadowCalc,					TXT("terrainShadowsCalc.fx"), RENDER_SHADER_DEFINES_BASE() );
//dex--

RENDER_SHADER_GEN( m_terrainPigmentGenerate,					TXT("terrainPigmentGenerate.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_grassMapGenerate,							TXT("grassMapGenerate.fx"), RENDER_SHADER_DEFINES_BASE() );

RENDER_SHADER_GEN( m_shaderTessBlocksErrorsGenerate,			TXT("tesselationBlocksErrorsGenerate.fx"), CMaterialCompilerDefines() );
RENDER_SHADER_GEN( m_shaderTerrainNormalsCalc,					TXT("terrainNormalsCalc.fx"), CMaterialCompilerDefines() );

//dex++: shaders for filtering ESM buffers
RENDER_SHADER_GEN( m_shaderESMGauss3,							TXT("esmFilterGauss.fx"), RENDER_SHADER_DEFINES_BASE() );
//dex--

// Global ocean

//gpu fft

// CS
RENDER_SHADER_COMPUTE( m_shaderFFTGPU_PhillipsAnimationCompute,	TXT("fft.fx"), CMaterialCompilerDefines().Add( ("PHILLIPS_ANIMATION"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderFFTGPU_OrderingCompute,			TXT("fft.fx"), CMaterialCompilerDefines().Add( ("ORDERING"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderFFTGPU_TransposeOrderingCompute,	TXT("fft.fx"), CMaterialCompilerDefines().Add( ("ORDERING"), ("1") ).Add( ("TRANSPOSE"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderFFTGPU_ButterflyCompute,			TXT("fft.fx"), CMaterialCompilerDefines().Add( ("BUTTERFLY"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderFFTGPU_FinalizeCompute,			TXT("fft.fx"), CMaterialCompilerDefines().Add( ("FINALIZE"), ("1") ) );

//PSVS 
RENDER_SHADER_GEN( m_shaderFFTGPU_PhillipsAnimation,			TXT("fft.fx"), CMaterialCompilerDefines().Add( ("PHILLIPS_ANIMATION"), ("1") ) );
RENDER_SHADER_GEN( m_shaderFFTGPU_Ordering,						TXT("fft.fx"), CMaterialCompilerDefines().Add( ("ORDERING"), ("1") ) );
RENDER_SHADER_GEN( m_shaderFFTGPU_TransposeOrdering,			TXT("fft.fx"), CMaterialCompilerDefines().Add( ("ORDERING"), ("1") ).Add( ("TRANSPOSE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderFFTGPU_Butterfly,					TXT("fft.fx"), CMaterialCompilerDefines().Add( ("BUTTERFLY"), ("1") ) );
RENDER_SHADER_GEN( m_shaderFFTGPU_Finalize,						TXT("fft.fx"), CMaterialCompilerDefines().Add( ("FINALIZE"), ("1") ) );

RENDER_SHADER_GEN( m_shaderFFTGPU_CopyHeight,					TXT("fft.fx"), CMaterialCompilerDefines().Add( ("COPYHEIGHT"), ("1") ) );
//gpu fft

// dynamic water
RENDER_SHADER_GEN( m_shaderDynamicWaterImpulseInstanced,		TXT("dynamicWater.fx"), CMaterialCompilerDefines().Add( "DYNAMICWATERIMPULSE", "1" ) );
RENDER_SHADER_COMPUTE( m_shaderDynamicWaterCompute,				TXT("dynamicWater.fx"), CMaterialCompilerDefines().Add( "DYNAMICWATER", "1" ) );
RENDER_SHADER_COMPUTE( m_shaderDynamicWaterFinalizeCompute,		TXT("dynamicWater.fx"), CMaterialCompilerDefines().Add( "DYNAMICWATERFINALIZE", "1" ) );
RENDER_SHADER_GEN( m_shaderDynamicWaterImpulse,					TXT("dynamicWater.fx"), CMaterialCompilerDefines().Add( "USE_OLD_DYNWATER", "1" ).Add( "DYNAMICWATERIMPULSE", "1" ) );
RENDER_SHADER_GEN( m_shaderDynamicWater,						TXT("dynamicWater.fx"), CMaterialCompilerDefines().Add( "USE_OLD_DYNWATER", "1" ).Add( "DYNAMICWATER", "1" ) );
RENDER_SHADER_GEN( m_shaderDynamicWaterFinalize,				TXT("dynamicWater.fx"), CMaterialCompilerDefines().Add( "USE_OLD_DYNWATER", "1" ).Add( "DYNAMICWATERFINALIZE", "1" ) );
// dynamic water

// diffusion2d
RENDER_SHADER_GEN( m_shaderDiffusion2D,							TXT("diffusion2D.fx"), CMaterialCompilerDefines().Add( ("DIFFUSION2D"), ("1") ) );
// diffusion2d

RENDER_SHADER_TESS_GEN( m_shaderGlobalOcean,					TXT("globalOcean.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderGlobalOceanIntersectionRtt,			TXT("globalOceanIntersectionRtt.fx"), RENDER_SHADER_DEFINES_BASE() );

RENDER_SHADER_GEN( m_blitAccumRefraction,						TXT("blitAccumRefraction.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_blitAccumRefraction_MSAA,					TXT("blitAccumRefraction.fx"), RENDER_SHADER_MSAA_DEFINES_BASE() );

RENDER_SHADER_GEN( m_shaderGlobalShadow,						TXT("globalShadow.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderGlobalShadow_MSAA,					TXT("globalShadow.fx"), RENDER_SHADER_MSAA_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderGlobalShadowBlendedCascades,			TXT("globalShadow.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("BLEND_CASCADES"), ("1") ) );
RENDER_SHADER_GEN( m_shaderGlobalShadow_MSAA_BlendedCascades,	TXT("globalShadow.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("BLEND_CASCADES"), ("1") ) );

RENDER_SHADER_GEN( m_shaderSimpleDecal,							TXT("screenSpaceDecal.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderSimpleDecalSpeculared,				TXT("screenSpaceDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("USE_SPECULAR"), ("1") ) );
RENDER_SHADER_GEN( m_shaderSimpleDecalNormalmapped,				TXT("screenSpaceDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("USE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderSimpleDecalSpecularedNormalmapped,	TXT("screenSpaceDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("USE_SPECULAR"), ("1") ).Add( ("USE_NORMALS"), ("1") ) );

RENDER_SHADER_GEN( m_shaderSimpleDecalFocusMode,				TXT("screenSpaceDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_FOCUS_MODE"), ("1") ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalStatic,				TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSkinned,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalStaticN,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_NORMALS", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSkinnedN,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_NORMALS", "1" ).Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalStaticBN,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_NORMALS", "1" ).Add( "CALC_BINORMAL", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSkinnedBN,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_NORMALS", "1" ).Add( "CALC_BINORMAL", "1" ).Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSingleSkinned,		TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "SINGLE_BONE", "1" ).Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSingleSkinnedN,		TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "SINGLE_BONE", "1" ).Add( "USE_NORMALS", "1" ).Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSingleSkinnedBN,	TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "SINGLE_BONE", "1" ).Add( "USE_NORMALS", "1" ).Add( "CALC_BINORMAL", "1" ).Add( "USE_SKINNING", "1" ) );

RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalStaticNa,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ADDITIVE_NORMALS", "1" ).Add( "USE_NORMALS", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSkinnedNa,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ADDITIVE_NORMALS", "1" ).Add( "USE_NORMALS", "1" ).Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalStaticBNa,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ADDITIVE_NORMALS", "1" ).Add( "USE_NORMALS", "1" ).Add( "CALC_BINORMAL", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSkinnedBNa,			TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ADDITIVE_NORMALS", "1" ).Add( "USE_NORMALS", "1" ).Add( "CALC_BINORMAL", "1" ).Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSingleSkinnedNa,	TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ADDITIVE_NORMALS", "1" ).Add( "SINGLE_BONE", "1" ).Add( "USE_NORMALS", "1" ).Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_GEOM_GEN( m_shaderDynamicDecalSingleSkinnedBNa,	TXT("dynamicDecal.fx"), RENDER_SHADER_DEFINES_BASE().Add( "ADDITIVE_NORMALS", "1" ).Add( "SINGLE_BONE", "1" ).Add( "USE_NORMALS", "1" ).Add( "CALC_BINORMAL", "1" ).Add( "USE_SKINNING", "1" ) );

RENDER_SHADER_SO_GEN( m_shaderDynamicDecalGen,					TXT("dynamicDecalGen.fx"), RENDER_SHADER_DEFINES_BASE(), GpuApi::BCT_VertexDynamicDecalGenOut );
RENDER_SHADER_SO_GEN( m_shaderDynamicDecalSkinnedGen,			TXT("dynamicDecalGen.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_SKINNING", "1" ), GpuApi::BCT_VertexDynamicDecalGenOut );
RENDER_SHADER_SO_GEN( m_shaderDynamicDecalSingleSkinnedGen,		TXT("dynamicDecalGen.fx"), RENDER_SHADER_DEFINES_BASE().Add( "SINGLE_BONE", "1" ).Add( "USE_SKINNING", "1" ), GpuApi::BCT_VertexDynamicDecalGenOut );

RENDER_SHADER_GEN( m_shaderStripeRegularSingle,						TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderStripeRegularSingleNormals,				TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDouble,						TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDoubleFirstNormals,			TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDoubleSecondNormals,		TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDoubleBothNormals,			TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularBlendVis,					TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("VIS_STRIPE_BLEND"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedSingle,					TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedSingleNormals,			TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDouble,					TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDoubleFirstNormals,		TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDoubleSecondNormals,		TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDoubleBothNormals,		TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedBlendVis,					TXT("stripe.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("VIS_STRIPE_BLEND"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularSingle_MSAA,				TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderStripeRegularSingleNormals_MSAA,			TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDouble_MSAA,				TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDoubleFirstNormals_MSAA,	TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDoubleSecondNormals_MSAA,	TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeRegularDoubleBothNormals_MSAA,		TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedSingle_MSAA,				TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedSingleNormals_MSAA,		TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDouble_MSAA,				TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDoubleFirstNormals_MSAA,	TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDoubleSecondNormals_MSAA,	TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderStripeProjectedDoubleBothNormals_MSAA,	TXT("stripe.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_STRIPE_DOUBLE"), ("1") ).Add( ("HAS_FIRST_STRIPE_NORMALS"), ("1") ).Add( ("HAS_SECOND_STRIPE_NORMALS"), ("1") ).Add( ("IS_STRIPE_PROJECTED"), ("1") ) );

RENDER_SHADER_COMPUTE( m_shaderHistogramPrepare,					TXT("histogram.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("HAS_DEBUG_HISTORGRAM_OUTPUT"), ("0") ).Add( ("IS_HISTOGRAM_PREPARE"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderHistogramGather,						TXT("histogram.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("HAS_DEBUG_HISTORGRAM_OUTPUT"), ("0") ).Add( ("IS_HISTOGRAM_GATHER"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderHistogramDebug,						TXT("histogram.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("HAS_DEBUG_HISTORGRAM_OUTPUT"), ("1") ).Add( ("IS_HISTOGRAM_GATHER"), ("1") ) );

RENDER_SHADER_COMPUTE( m_shaderLightsCompute,						TXT("lights_compute.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_COMPUTE( m_shaderLightsCompute_Deferred,				TXT("lights_compute.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_COMPUTE"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderLightsCompute_EnvProbeGen,			TXT("lights_compute.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_ENVPROBE_GEN"), ("1") ) );
RENDER_SHADER_COMPUTE( m_shaderLightsCulling_Deferred,				TXT("lights_cull.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_COMPUTE( m_shaderInteriorCompute,						TXT("interior_compute.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_COMPUTE( m_shaderInteriorCompute_NoCulling,			TXT("interior_compute.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("SKIP_CULLING"), ("1") ) );
RENDER_SHADER_GEN( m_shaderVolumesCombine,							TXT("interior_combine.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_VOLUMES_COMBINE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderInteriorCombine,							TXT("interior_combine.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterUnpackRegular,				TXT("envProbeFilterUnpack.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterUnpackInterior,			TXT("envProbeFilterUnpack.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_INTERIOR_FALLBACK"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterAmbientMerge,				TXT("envProbeFilterAmbientMerge.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterBlur,						TXT("envProbeFilterBlur.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterBlurSmall,					TXT("envProbeFilterBlur.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_SMALL_BLUR"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterMerge,						TXT("envProbeFilterMerge.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterPrepareMerge,				TXT("envProbeFilterMerge.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PREPARE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterCopyToTemp,				TXT("envProbeFilterDownscale.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_COPY_TO_TEMP"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterDownscale,					TXT("envProbeFilterDownscale.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterPrepareDownscale,			TXT("envProbeFilterDownscale.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PREPARE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterFillCubeFaceAlbedo,		TXT("envProbeFilterFillCubeFace.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_ALBEDO"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterFillCubeFaceNormals,		TXT("envProbeFilterFillCubeFace.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_NORMALS"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterFillCubeFaceDepth,			TXT("envProbeFilterFillCubeFace.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_DEPTH"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterFillCubeFaceDepthAndSky,	TXT("envProbeFilterFillCubeFace.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_DEPTH_AND_SKY"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterFillCubeFaceSkyFactor,		TXT("envProbeFilterFillCubeFace.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_SKYFACTOR"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterFillCubeFaceInterior,		TXT("envProbeFilterFillCubeFace.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_INTERIOR_FALLBACK"), ("1") ) );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterCopy,						TXT("envProbeFilterCopy.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeFilterReflectionMerge,			TXT("envProbeFilterReflectionMerge.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeShadowmask,						TXT("envProbeShadowmask.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderEnvProbeUnwrap,							TXT("envProbeUnwrap.fx"), RENDER_SHADER_DEFINES_BASE() );

RENDER_SHADER_GEN( m_shaderGlobalFog,								TXT("globalFog.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderDistantLight,							TXT("distantLight.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderDistantLightDebug,						TXT("distantLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_DEBUG_DISTANT_LIGHT"), ("1") ) );
RENDER_SHADER_GEN( m_shaderSkyColor,								TXT("skyColor.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderSkyColorRLREnvProbe,						TXT("skyColor.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_RLR_ENVPROBE"), ("1") ) );

RENDER_SHADER_GEN( m_shaderDeferredLightPoint,						TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_POINT"), ("1") ) );
RENDER_SHADER_GEN( m_shaderDeferredLightSpot,						TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_SPOT"), ("1") ) );
RENDER_SHADER_GEN( m_shaderDeferredLightPointShadow,				TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_POINT"), ("1") ).Add( ("IS_PURE_DEFERRED_LIGHT_SHADOWED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderDeferredLightSpotShadow,					TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_SPOT"), ("1") ).Add( ("IS_PURE_DEFERRED_LIGHT_SHADOWED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderDeferredLightCamLightModPoint,			TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_POINT"), ("1") ).Add( ("IS_CAMERA_LIGHTS_MODIFIER_ALLOWED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderDeferredLightCamLightModSpot,			TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_SPOT"), ("1") ).Add( ("IS_CAMERA_LIGHTS_MODIFIER_ALLOWED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderDeferredLightCamLightModPointShadow,		TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_POINT"), ("1") ).Add( ("IS_PURE_DEFERRED_LIGHT_SHADOWED"), ("1") ).Add( ("IS_CAMERA_LIGHTS_MODIFIER_ALLOWED"), ("1") ) );
RENDER_SHADER_GEN( m_shaderDeferredLightCamLightModSpotShadow,		TXT("deferredLight.fx"), RENDER_SHADER_DEFINES_BASE().Add( ("IS_PURE_DEFERRED_LIGHT_SPOT"), ("1") ).Add( ("IS_PURE_DEFERRED_LIGHT_SHADOWED"), ("1") ).Add( ("IS_CAMERA_LIGHTS_MODIFIER_ALLOWED"), ("1") ) );

RENDER_SHADER_GEN( m_shaderResolveDepth_MSAA,						TXT("msaaResolveDepth.fx"), RENDER_SHADER_MSAA_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderResolveStencil_MSAA,						TXT("msaaResolveStencil.fx"), RENDER_SHADER_MSAA_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderResolveColor_SingleMSAA,					TXT("msaaResolveColor.fx"), RENDER_SHADER_MSAA_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderResolveColor_SingleMSAABuffered,			TXT("msaaResolveColor.fx"), RENDER_SHADER_MSAA_DEFINES_BASE().Add( ("IS_BUFFER_RESOLVE"), ("1") ) );
RENDER_SHADER_GEN( m_shaderMarkStencil_MSAA,						TXT("msaaMarkStencil.fx"), RENDER_SHADER_MSAA_DEFINES_BASE() );

RENDER_SHADER_SO_GEN( m_shaderMorphGen_Static,						TXT("morphedMeshGen.fx"), RENDER_SHADER_DEFINES_BASE(), GpuApi::BCT_VertexMeshStaticMorphGenOut );
RENDER_SHADER_SO_GEN( m_shaderMorphGen_Skinned,						TXT("morphedMeshGen.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_SKINNING", "1" ), GpuApi::BCT_VertexMeshSkinnedMorphGenOut );
RENDER_SHADER_SO_GEN( m_shaderMorphGen_Static_ControlTex,			TXT("morphedMeshGen.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_CONTROL_TEX", "1" ), GpuApi::BCT_VertexMeshStaticMorphGenOut );
RENDER_SHADER_SO_GEN( m_shaderMorphGen_Skinned_ControlTex,			TXT("morphedMeshGen.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_CONTROL_TEX", "1" ).Add( "USE_SKINNING", "1" ), GpuApi::BCT_VertexMeshSkinnedMorphGenOut );

RENDER_SHADER_COMPUTE( m_shaderMorphGen_StaticCS,					TXT("morphedMeshGenCS.fx"), RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_COMPUTE( m_shaderMorphGen_SkinnedCS,					TXT("morphedMeshGenCS.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_SKINNING", "1" ) );
RENDER_SHADER_COMPUTE( m_shaderMorphGen_Static_ControlTexCS,		TXT("morphedMeshGenCS.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_CONTROL_TEX", "1" ) );
RENDER_SHADER_COMPUTE( m_shaderMorphGen_Skinned_ControlTexCS,		TXT("morphedMeshGenCS.fx"), RENDER_SHADER_DEFINES_BASE().Add( "USE_CONTROL_TEX", "1" ).Add( "USE_SKINNING", "1" ) );

#ifdef USE_NVIDIA_FUR
RENDER_SHADER_GEN( m_shaderHair,									TXT("hair.fx"),  RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderHairGbuffer,								TXT("hairGbuffer.fx"),  RENDER_SHADER_DEFINES_BASE() );
RENDER_SHADER_GEN( m_shaderHairShadow,								TXT("hairShadow.fx"),  RENDER_SHADER_DEFINES_BASE() );
#endif

// BINK
#ifdef USE_BINK_VIDEO
	RENDER_SHADER_GEN( m_binkRGBA,									TXT("binkRGBA.fx"),  RENDER_SHADER_DEFINES_BASE() );
	RENDER_SHADER_GEN( m_binkRGB,									TXT("binkRGB.fx"),  RENDER_SHADER_DEFINES_BASE() );
#endif

#ifdef USE_MSSSAO
	RENDER_SHADER_COMPUTE( m_AoPrepareDepthAndNormalBuffers1CS,		TXT("aopreparedepthbuffers1cs.fx"),				RENDER_SHADER_DEFINES_BASE().Add( "USE_NORMALS", "1" ));
	RENDER_SHADER_COMPUTE( m_AoPrepareDepthBuffers1CS,				TXT("aopreparedepthbuffers1cs.fx"),				RENDER_SHADER_DEFINES_BASE());
	RENDER_SHADER_COMPUTE( m_AoPrepareDepthBuffers2CS,				TXT("aopreparedepthbuffers2cs.fx"),				RENDER_SHADER_DEFINES_BASE());
	RENDER_SHADER_COMPUTE( m_AoLinearizeDepthCS,					TXT("aolinearizedepthcs.fx"),					RENDER_SHADER_DEFINES_BASE());
	RENDER_SHADER_COMPUTE( m_AoRender1CS,							TXT("aorendercs.fx"),							RENDER_SHADER_DEFINES_BASE().Add( "INTERLEAVE_RESULT", "1" ));
	RENDER_SHADER_COMPUTE( m_AoRender2CS,							TXT("aorendercs.fx"),							RENDER_SHADER_DEFINES_BASE());
	RENDER_SHADER_COMPUTE( m_AoRenderNormals1CS,					TXT("aorendercs.fx"),							RENDER_SHADER_DEFINES_BASE().Add( "INTERLEAVE_RESULT", "1" ).Add( "USE_NORMAL_BUFFER", "1" ));
	RENDER_SHADER_COMPUTE( m_AoRenderNormals2CS,					TXT("aorendercs.fx"),							RENDER_SHADER_DEFINES_BASE().Add( "USE_NORMAL_BUFFER", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsampleCS,						TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE());
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePreMinCS,				TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "COMBINE_LOWER_RESOLUTIONS", "1" ).Add( "COMBINE_BEFORE_BLUR", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePreMulCS,				TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "COMBINE_LOWER_RESOLUTIONS", "1" ).Add( "COMBINE_BEFORE_BLUR", "1" ).Add( "COMBINE_WITH_MUL", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePostMinCS,				TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "COMBINE_LOWER_RESOLUTIONS", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePostMulCS,				TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "COMBINE_LOWER_RESOLUTIONS", "1" ).Add( "COMBINE_WITH_MUL", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsampleBlendOutCS,				TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "BLEND_WITH_HIGHER_RESOLUTION", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePreMinBlendOutCS,		TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "BLEND_WITH_HIGHER_RESOLUTION", "1" ).Add( "COMBINE_LOWER_RESOLUTIONS", "1" ).Add( "COMBINE_BEFORE_BLUR", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePreMulBlendOutCS,		TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "BLEND_WITH_HIGHER_RESOLUTION", "1" ).Add( "COMBINE_LOWER_RESOLUTIONS", "1" ).Add( "COMBINE_BEFORE_BLUR", "1" ).Add( "COMBINE_WITH_MUL", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePostMinBlendOutCS,		TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "BLEND_WITH_HIGHER_RESOLUTION", "1" ).Add( "COMBINE_LOWER_RESOLUTIONS", "1" ));
	RENDER_SHADER_COMPUTE( m_AoBlurUpsamplePostMulBlendOutCS,		TXT("aoblurandupsamplecs.fx"),					RENDER_SHADER_DEFINES_BASE().Add( "BLEND_WITH_HIGHER_RESOLUTION", "1" ).Add( "COMBINE_LOWER_RESOLUTIONS", "1" ).Add( "COMBINE_WITH_MUL", "1" ));
#endif
