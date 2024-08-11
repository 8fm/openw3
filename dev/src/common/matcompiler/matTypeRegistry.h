/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _H_MATERIAL_TYPE_REGISTRY
#define _H_MATERIAL_TYPE_REGISTRY

// this file contains list of all types in for the 'material compiler' project

// Not defined when included in matcompiler/build.h, but defined when included in matClasses.cpp
#if !defined( REGISTER_RTTI_TYPE )
	#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } static Bool IsArray() { return false; } }
	#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className )
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className )
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className )

//////////////////////////////////////////////////////////////////////////

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

REGISTER_RTTI_CLASS( CMaterialBlockMathDerivativeToNormal );
REGISTER_RTTI_CLASS( CMaterialBlockMathNormalToDerivative );
REGISTER_RTTI_CLASS( CMaterialBlockDerivativeNormal );
REGISTER_RTTI_CLASS( CMaterialBlockPackNormal );
REGISTER_RTTI_CLASS( CMaterialBlockConvertGammaToLinear );
REGISTER_RTTI_CLASS( CMaterialBlockConvertLinearToGamma );
REGISTER_RTTI_CLASS( CMaterialBlockGammaCorrection );
REGISTER_RTTI_CLASS( CMaterialBlockEnvTransparencyColorFilter );
REGISTER_RTTI_CLASS( CMaterialBlockShiftColor );
REGISTER_RTTI_CLASS( CMaterialBlockShiftColorRB );
REGISTER_RTTI_CLASS( CMaterialBlockTangentToWorld );
REGISTER_RTTI_CLASS( CMaterialBlockDithering );
REGISTER_RTTI_CLASS( CMaterialBlockTexCoordMad );
REGISTER_RTTI_CLASS( CMaterialBlockGlobalFog );
REGISTER_RTTI_CLASS( CMaterialBlockLocalReflection );
REGISTER_RTTI_CLASS( CMaterialBlockSoftTransparencyAlpha );
REGISTER_RTTI_CLASS( CMaterialBlockWetness );
REGISTER_RTTI_CLASS( CMaterialBlockRenderFeedbackDataFetch );
REGISTER_RTTI_CLASS( CMaterialBlockVolumeBlend );
REGISTER_RTTI_CLASS( CMaterialBlockCombineNormals );
REGISTER_RTTI_CLASS( CMaterialBlockAmbientLight );
REGISTER_RTTI_CLASS( CMaterialBlockCoarseReflection );
REGISTER_RTTI_CLASS( CMaterialBlockBlendByDistance );
//REGISTER_RTTI_CLASS( CMaterialBlockLighting );
REGISTER_RTTI_CLASS( CMaterialBlockLightingPhong );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerNormal );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerHeightmap2Normal );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerDetail );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerNormalDetail );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerTextureDetail );
REGISTER_RTTI_CLASS( CMaterialBlockViewZ );
REGISTER_RTTI_CLASS( CMaterialBlockViewVector );
REGISTER_RTTI_CLASS( CMaterialBlockWorldBasis );
REGISTER_RTTI_CLASS( CMaterialBlockWorldNormal );
REGISTER_RTTI_CLASS( CMaterialBlockWorldTangent );
REGISTER_RTTI_CLASS( CMaterialBlockWorldBinormal );
REGISTER_RTTI_CLASS( CMaterialBlockWorldToTangent );
REGISTER_RTTI_CLASS( CMaterialBlockWorldTangentRecalculation );
REGISTER_RTTI_CLASS( CMaterialBlockWorldViewRecalculation );
REGISTER_RTTI_CLASS( CMaterialBlockCustomFunction );
REGISTER_RTTI_CLASS( CMaterialBlockForwardLight );
REGISTER_RTTI_CLASS( CMaterialBlockForwardLightCustom );
REGISTER_RTTI_CLASS( CMaterialBlockShadowSample );
REGISTER_RTTI_CLASS( CMaterialBlockShadowSurfaceDepth );
REGISTER_RTTI_CLASS( CMaterialBlockViewPosition );
REGISTER_RTTI_CLASS( CMaterialBlockVFace );
REGISTER_RTTI_CLASS( CMaterialBlockWorldPosition );
REGISTER_RTTI_CLASS( CMaterialBlockStreamingBlendRatio );
REGISTER_RTTI_CLASS( CMaterialBlockVertexColor );
REGISTER_RTTI_CLASS( CMaterialBlockVertexPosition );
REGISTER_RTTI_CLASS( CMaterialBlockVertexNormal );
REGISTER_RTTI_CLASS( CMaterialBlockVertexLight );
REGISTER_RTTI_CLASS( CMaterialBlockVertexFrame );
REGISTER_RTTI_CLASS( CMaterialBlockVertexMotionBlend );
REGISTER_RTTI_CLASS( CMaterialBlockMathDesaturate );
REGISTER_RTTI_CLASS( CMaterialBlockMathInterpolate );
REGISTER_RTTI_CLASS( CMaterialBlockMathMin );
REGISTER_RTTI_CLASS( CMaterialBlockMathMax );
REGISTER_RTTI_CLASS( CMaterialBlockMathSin );
REGISTER_RTTI_CLASS( CMaterialBlockMathCos );
REGISTER_RTTI_CLASS( CMaterialBlockMathPower );
REGISTER_RTTI_CLASS( CMaterialBlockMathClamp );
REGISTER_RTTI_CLASS( CMaterialBlockMathMapValue );
REGISTER_RTTI_CLASS( CMaterialBlockMathFresnel );
REGISTER_RTTI_CLASS( CMaterialBlockMathSaturate );
REGISTER_RTTI_CLASS( CMaterialBlockMathSH );
REGISTER_RTTI_CLASS( CMaterialBlockMathNormalize );
REGISTER_RTTI_CLASS( CMaterialBlockMathInvert );
REGISTER_RTTI_CLASS( CMaterialBlockMathTime );
REGISTER_RTTI_CLASS( CMaterialBlockMathReflection );
REGISTER_RTTI_CLASS( CMaterialBlockMathSubtract );
REGISTER_RTTI_CLASS( CMaterialBlockMathNegate );
REGISTER_RTTI_CLASS( CMaterialBlockMathSplitVector );
REGISTER_RTTI_CLASS( CMaterialBlockMathSplitAppendVector );
REGISTER_RTTI_CLASS( CMaterialBlockMathAppendVector );
REGISTER_RTTI_CLASS( CMaterialBlockNoise );
REGISTER_RTTI_CLASS( CMaterialBlockNoise2D );
REGISTER_RTTI_CLASS( CMaterialBlockGradient );
REGISTER_RTTI_CLASS( CMaterialBlockGlossinessPack );
REGISTER_RTTI_CLASS( CMaterialBlockOutputVertexModifiers );
// REGISTER_RTTI_CLASS( CMaterialBlockOutputColor );
// REGISTER_RTTI_CLASS( CMaterialBlockOutputColorDeferred );
//REGISTER_RTTI_CLASS( CMaterialBlockOutputColorHair );
//REGISTER_RTTI_CLASS( CMaterialBlockOutputColorEnhanced );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorDecalModulative );
REGISTER_RTTI_CLASS( CMaterialBlockOutputColorDecalBlended );
//REGISTER_RTTI_CLASS( CMaterialBlockOutputColorSkin );
REGISTER_RTTI_CLASS( CMaterialBlockTexCoords );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerTexture );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerTextureArray );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerNormalArray );
REGISTER_RTTI_CLASS( CMaterialBlockSamplerCube );
REGISTER_RTTI_CLASS( CMaterialBlockMathAdd );
REGISTER_RTTI_CLASS( CMaterialBlockMathAbs );
REGISTER_RTTI_CLASS( CMaterialBlockMathDot3 );
REGISTER_RTTI_CLASS( CMaterialBlockMathCross );
REGISTER_RTTI_CLASS( CMaterialBlockMathDiv );
REGISTER_RTTI_CLASS( CMaterialBlockMathMultiply );
REGISTER_RTTI_CLASS( CMaterialBlockMathFrac );
REGISTER_RTTI_CLASS( CMaterialBlockMathFloor );
REGISTER_RTTI_CLASS( CMaterialBlockNormalmapBlend );
REGISTER_RTTI_CLASS( CMaterialTerrainMaterialSampler );
REGISTER_RTTI_CLASS( CMaterialTerrainMaterialBlending );

#endif

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
	#undef REGISTER_RTTI_TYPE
	#undef REGISTER_NOT_REGISTERED
#endif

#endif // _H_MATERIAL_TYPE_REGISTRY
