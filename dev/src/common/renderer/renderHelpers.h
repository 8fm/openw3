/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/materialCompiler.h"

/// Render target name
enum ERenderTargetName
{
	RTN_Color,
	RTN_Color2,
	RTN_Color3,
	RTN_MSAAColor,
	RTN_UbersampleAccum,
	RTN_TemporalAAColor,
	RTN_TemporalAALum0,
	RTN_TemporalAALum1,
	RTN_GBuffer0,
	RTN_GBuffer0MSAA,
	RTN_GBuffer1,
	RTN_GBuffer1MSAA,
	RTN_GBuffer2,
	RTN_GBuffer2MSAA,
	RTN_GBuffer3Depth,
	RTN_RLRSky,
	RTN_RLRColor,
	RTN_RLRDepth,
	RTN_RLRResultHistory,
	RTN_FinalColor,
	RTN_GlobalShadowAndSSAO,
	RTN_MSAACoverageMask,
	RTN_LuminanceSimpleAPing,
	RTN_LuminanceSimpleAPong,
	RTN_LuminanceSimpleBPing,
	RTN_LuminanceSimpleBPong,
	RTN_LuminanceSimpleFinal,
	RTN_InteriorVolume,

	// The following are for use in post-process. They are intended to be always in ESRAM on Xbox, for the duration of the post-processing.
	// These are all using an 11-11-10 RGB format (no alpha). Where we have ESRAM, PPTempFull and the PPTempHalf targets exist in the same
	// area, so only one or the other can be used at a time.
	RTN_PostProcessTarget1,
	RTN_PostProcessTarget2,
	RTN_PostProcessTempFull,
	RTN_PostProcessTempHalf1,
	RTN_PostProcessTempHalf2,
	RTN_PostProcessTempHalf3,
	RTN_PostProcessTempQuater1,
	RTN_PostProcessTempQuater2,
	RTN_PostProcessTempQuater3,
	RTN_PostProcessTempQuater4,
	RTN_PostProcessTempQuater5,

	RTN_CameraInteriorFactor,

#ifdef RED_PLATFORM_DURANGO
	RTN_DURANGO_PostProcessTarget1_R10G10B10_6E4_A2_FLOAT,
	RTN_DURANGO_PostProcessTarget2_R10G10B10_6E4_A2_FLOAT,
	RTN_DURANGO_InteriorVolume_TempSurface,
	RTN_DURANGO_FFT0,
	RTN_DURANGO_FFT1,
	RTN_DURANGO_RLR_Result0,
#endif

	RTN_Max,
	RTN_None
};

/// Format of the render target
enum ERenderTargetFormat
{
	RTFMT_A2B10G10R10F,
	RTFMT_A16B16G16R16F,
	RTFMT_A32B32G32R32F,
	RTFMT_R11G11B10F,
	RTFMT_G16R16F,
	RTFMT_R32F,
	RTFMT_R16F,
	RTFMT_A8R8G8B8,
	RTFMT_DEPTH,

#ifdef RED_PLATFORM_DURANGO
	RTFMT_R10G10B10_6E4_A2_FLOAT,
#endif

	RTFMT_Max
};

// Get texture format for given rendertarget format
RED_INLINE GpuApi::eTextureFormat GetRenderTargetFormat( ERenderTargetFormat format )
{
	switch( format )
	{
	case RTFMT_A2B10G10R10F:	return GpuApi::TEXFMT_Float_R10G10B10A2;
	case RTFMT_A16B16G16R16F:	return GpuApi::TEXFMT_Float_R16G16B16A16;
	case RTFMT_A32B32G32R32F:	return GpuApi::TEXFMT_Float_R32G32B32A32;
	case RTFMT_R11G11B10F:		return GpuApi::TEXFMT_Float_R11G11B10;
	case RTFMT_G16R16F:			return GpuApi::TEXFMT_Float_R16G16;	
	case RTFMT_R32F:			return GpuApi::TEXFMT_Float_R32;
	case RTFMT_R16F:			return GpuApi::TEXFMT_Float_R16;
	case RTFMT_A8R8G8B8:		return GpuApi::TEXFMT_R8G8B8A8;
	case RTFMT_DEPTH:			return GpuApi::TEXFMT_D24S8;
#ifdef RED_PLATFORM_DURANGO
	case RTFMT_R10G10B10_6E4_A2_FLOAT:	return GpuApi::TEXFMT_R10G10B10_6E4_A2_FLOAT;
#endif
	default:					ASSERT( !"invalid" ); return GpuApi::TEXFMT_Float_R16G16B16A16;
	}
}

/// Shader type
enum ERenderShaderType : Uint32
{
	RST_VertexShader = GpuApi::VertexShader,
	RST_PixelShader = GpuApi::PixelShader,
	RST_GeometryShader = GpuApi::GeometryShader,
	RST_HullShader = GpuApi::HullShader,
	RST_DomainShader = GpuApi::DomainShader,
	RST_ComputeShader = GpuApi::ComputeShader,

	RST_Max
};

RED_INLINE GpuApi::eShaderType Map( ERenderShaderType shaderStage )
{
	return (GpuApi::eShaderType)shaderStage;
}

RED_INLINE ERenderShaderType Map( GpuApi::eShaderType shaderStage )
{
	return (ERenderShaderType)shaderStage;
}

RED_INLINE GpuApi::eTextureAddress Map( ETextureAddressing address )
{
	switch( address )
	{
	case TA_Clamp:
		return GpuApi::TEXADDR_Clamp;
	case TA_Wrap:
		return GpuApi::TEXADDR_Wrap;
	case TA_Mirror:
		return GpuApi::TEXADDR_Mirror;
	case TA_MirrorOnce:
		return GpuApi::TEXADDR_MirrorOnce;
	default:
		ASSERT( false );
		return GpuApi::TEXADDR_Wrap;
	}
}

RED_INLINE GpuApi::eComparison Map( ETextureComparisonFunction comparison )
{
	switch( comparison )
	{
	case TCF_None:
		return GpuApi::COMP_None;
	case TCF_Less:
		return GpuApi::COMP_Less;

	case TCF_Equal:
	case TCF_LessEqual:
	case TCF_Greater:
	case TCF_NotEqual:
	case TCF_GreaterEqual:
	case TCF_Always:
	default:
		ASSERT( false );
		return GpuApi::COMP_None;
	}
}

RED_INLINE GpuApi::eTextureFilterMin Map( ETextureFilteringMin filtering )
{
	switch( filtering )
	{
	case TFMin_Point:
		return GpuApi::TEXFILTERMIN_Point;
	case TFMin_Linear:
		return GpuApi::TEXFILTERMIN_Linear;
	case TFMin_Anisotropic:
		return GpuApi::TEXFILTERMIN_Aniso;
	case TFMin_AnisotropicLow:
		return GpuApi::TEXFILTERMIN_AnisoLow;
	default:
		ASSERT( false );
		return GpuApi::TEXFILTERMIN_Aniso;
	}
}

RED_INLINE GpuApi::eTextureFilterMag Map( ETextureFilteringMag filtering )
{
	switch( filtering )
	{
	case TFMag_Point:
		return GpuApi::TEXFILTERMAG_Point;
	case TFMag_Linear:
		return GpuApi::TEXFILTERMAG_Linear;
	default:
		ASSERT( false );
		return GpuApi::TEXFILTERMAG_Linear;
	}
}

RED_INLINE GpuApi::eTextureFilterMip Map( ETextureFilteringMip filtering )
{
	switch( filtering )
	{
	case TFMip_None:
		return GpuApi::TEXFILTERMIP_None;
	case TFMip_Point:
		return GpuApi::TEXFILTERMIP_Point;
	case TFMip_Linear:
		return GpuApi::TEXFILTERMIP_Linear;
	default:
		ASSERT( false );
		return GpuApi::TEXFILTERMIP_None;
	}
}

/// Extract rendering resource, adds a ref count
template< class R, class T >
RED_INLINE void ExtractRenderResource( R* resource, T*& renderingResource )
{
	T* newRenderingResource = nullptr;
	if ( resource )
	{
		newRenderingResource = static_cast< T* >( resource->GetRenderResource() );
	}

	SAFE_COPY( renderingResource, newRenderingResource );
}

// dissolve math ( old Adam C code ), kind of cryptic :)
RED_INLINE static void CalculateMergedDissolveResult( Float origFactor, Float& dissolveScale, Float& dissolveBias )
{
	Bool  positive	 = origFactor>=0;
	Float scale		 = positive ? 1.f			: -1.f;
	Float threshold	 = positive ? origFactor	: 1.f + origFactor;
	dissolveScale	= scale;
	dissolveBias	= -threshold * scale;
}

RED_INLINE static void CalculateMergedDissolveResultVector( Float origFactor, Float nextFactor, Vector& outVector )
{
	CalculateMergedDissolveResult( origFactor, outVector.X, outVector.Y );
	CalculateMergedDissolveResult( nextFactor, outVector.Z, outVector.W );
}

RED_INLINE static Float CalculateMagicDissolve( Float factor, Float alpha )
{
	if ( alpha < 1.0f )
	{
		return (factor >= 0 ? 1 : -1) * (1 - alpha * (1 - Abs( factor )));
	}
	else
	{
		return factor;
	}
}

RED_INLINE static Uint64 CalculateMaterialHash( const String& fileName, const MaterialRenderingContext& context )
{
	Uint64 filenameHash = Red::System::CalculateAnsiHash32LowerCase( fileName.AsChar() );
	filenameHash = filenameHash << 32;
	Uint32 contextHash = context.CalcID();
	Uint64 hash = filenameHash | contextHash;
	return hash;
}

RED_INLINE static Uint64 CalculateMaterialHash( const String& fileName, const Uint32 contextId )
{
	Uint64 filenameHash = Red::System::CalculateAnsiHash32LowerCase( fileName.AsChar() );
	filenameHash = filenameHash << 32;
	Uint64 hash = filenameHash | contextId;
	return hash;
}

RED_INLINE static Uint64 CalculateMaterialHash( const Uint32 fileNameHash, const MaterialRenderingContext& context )
{
	Uint64 hash = fileNameHash;
	hash = hash << 32;
	Uint32 contextHash = context.CalcID();
	hash = hash | contextHash;
	return hash;
}

RED_FORCE_INLINE static Uint64 CalculateMaterialHash( const Uint32 fileNameHash, const Uint32 contextId )
{
	Uint64 hash = fileNameHash;
	hash = hash << 32;
	return hash | contextId;
}

/// Post-process categories
enum EPostProcessCategoryFlags
{
	PPCF_Tonemapping		= FLAG( 0 ),
	PPCF_Bloom				= FLAG( 1 ),
	PPCF_Shafts				= FLAG( 2 ),
	PPCF_AntiAlias			= FLAG( 3 ),
	PPCF_Blur				= FLAG( 4 ),
	PPCF_DepthOfField		= FLAG( 5 ),
	PPCF_CutsceneDOF		= FLAG( 6 ),
	PPCF_Vignette			= FLAG( 7 ),
	PPCF_Sharpen			= FLAG( 8 ),
	PPCF_Rain				= FLAG( 9 ),
	PPCF_SSAO				= FLAG( 10 ),
	PPCF_MotionBlur			= FLAG( 11 ),
	PPCF_Fog				= FLAG( 12 ),
	PPCF_Underwater			= FLAG( 13 ),
	PPCF_ChromaticAberration= FLAG( 14 ),

	PPCF_DisableAllMask		= 0,
	PPCF_EnableAllMask		= 0xFFFF
};
