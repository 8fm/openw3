/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "..\redSystem\types.h"
#include "..\redSystem\utility.h"

namespace GpuApi
{
	typedef Red::System::Bool		Bool;
	typedef Red::System::Int8		Int8;
	typedef Red::System::Uint8		Uint8;
	typedef Red::System::Int16		Int16;
	typedef Red::System::Uint16		Uint16;
	typedef Red::System::Int32		Int32;
	typedef Red::System::Uint32		Uint32;
	typedef Red::System::Int64		Int64;
	typedef Red::System::Uint64		Uint64;
	typedef Red::System::Float		Float;
	typedef Red::System::Double		Double;
	typedef Red::System::UniChar	UniChar;
	typedef Red::System::AnsiChar	AnsiChar;
	typedef Red::System::Char		Char;
	typedef Red::System::MemSize	MemSize;

	// Failure category
	enum eFailureCategory
	{
		FAILCAT_Error,
		FAILCAT_Assert,
		FAILCAT_Warning,
	};

	// Basic structures

	struct Color
	{
		Color ()
		{
			Set( 255, 255, 255, 255 );
		}

		Color ( Uint8 newR, Uint8 newG, Uint8 newB, Uint8 newA )
		{
			Set( newR, newG, newB, newA );
		}

		Color& Set( Uint8 newR, Uint8 newG, Uint8 newB, Uint8 newA )
		{
			r = newR;
			g = newG;
			b = newB;
			a = newA;
			return *this;
		}

		Uint8 r, g, b, a;
	};

	struct Rect
	{
		Rect ()
		{
			Set( 0, 0, 0, 0 );
		}

		Rect ( Int32 newLeft, Int32 newTop, Int32 newRight, Int32 newBottom )
		{
			Set( newLeft, newTop, newRight, newBottom );
		}

		Rect& Set( Int32 newLeft, Int32 newTop, Int32 newRight, Int32 newBottom )
		{
			left	= newLeft;
			top		= newTop;
			right	= newRight;
			bottom	= newBottom;
			return *this;
		}

		Int32	left;
		Int32	top;
		Int32	right;
		Int32	bottom;
	};

	enum eTextureAddress
	{
		TEXADDR_Wrap,
		TEXADDR_Mirror,
		TEXADDR_Clamp,
		TEXADDR_MirrorOnce,
		TEXADDR_Border
	};

	enum eComparison
	{
		COMP_None,
		COMP_Less, //todo: add more if you need
	};

	enum eTextureFilterMin
	{
		TEXFILTERMIN_Point,
		TEXFILTERMIN_Linear,
		TEXFILTERMIN_Aniso = 4,	// Use degree from render setting 
		TEXFILTERMIN_AnisoLow,	// Lower degree
	};

	enum eTextureFilterMag
	{
		TEXFILTERMAG_Point,
		TEXFILTERMAG_Linear
	};

	enum eTextureFilterMip
	{
		TEXFILTERMIP_Point,
		TEXFILTERMIP_Linear,
		TEXFILTERMIP_None
	};

	enum
	{
		TEXUSAGE_RenderTarget					= 0x0001,	// May be set as a rendertarget
		TEXUSAGE_DepthStencil					= 0x0002,	// May be set as depth stencil surface
		TEXUSAGE_Samplable						= 0x0004,	// May be set as a traditional texture (sampled in shaders)
		TEXUSAGE_BackBuffer						= 0x0008,	// Device backbuffer (can't be set manually)
		TEXUSAGE_Dynamic   						= 0x0010,	// Dynamic texture
		TEXUSAGE_Staging						= 0x0020,	// Texture used to transfer data between GPU and CPU

		// FIXME: A bit hackish, but this consolidation of CPU access and usage makes it a bit awkward
		TEXUSAGE_StagingWrite					= 0x0020 | 0x0040,	// Texture used to tranfer data from CPU to GPU (added for Scaleform's glyph cache implementation)

		TEXUSAGE_Immutable						= 0x0080,	// Texture cannot be written to after creation

		TEXUSAGE_GenMip							= 0x0100,	//mipmap generation (for nice ESM shadows)

		TEXUSAGE_ESRAMResident					= 0x0200,

		TEXUSAGE_CubeSamplablePerMipLevel		= 0x0400,
		TEXUSAGE_CubeSamplablePerMipFace		= 0x0800,

		TEXUSAGE_Tex2DSamplablePerMipLevel		= 0x1000,
		TEXUSAGE_NoDepthCompression				= 0x2000,

		TEXUSAGE_DynamicScalingAlign1			= 0x4000,
		TEXUSAGE_DynamicScalingAlign2			= 0x8000,
		TEXUSAGE_DynamicScalingAlign4			= 0x10000,

		TEXUSAGE_DynamicScalingAlignSSAO1		= 0x20000,
		TEXUSAGE_DynamicScalingAlignSSAO2		= 0x40000,
		TEXUSAGE_DynamicScalingAlignSSAO4		= 0x80000,
		TEXUSAGE_DynamicScalingAlignSSAO8		= 0x100000,
		TEXUSAGE_DynamicScalingAlignSSAO16		= 0x200000,
		TEXUSAGE_DynamicScalingAlignSSAO32		= 0x400000,
		TEXUSAGE_DynamicScalingAlignSSAO64		= 0x800000,

		TEXUSAGE_FastColorClear					= 0x1000000,

		TEXUSAGE_BackBufferDepth				= TEXUSAGE_BackBuffer | TEXUSAGE_DepthStencil, // HACK GL this is needed to be able to clear the depth buffer
		TEXUSAGE_MASK_DynamicScaling			= TEXUSAGE_DynamicScalingAlign1 | TEXUSAGE_DynamicScalingAlign2 | TEXUSAGE_DynamicScalingAlign4 | TEXUSAGE_DynamicScalingAlignSSAO1 | TEXUSAGE_DynamicScalingAlignSSAO2 | TEXUSAGE_DynamicScalingAlignSSAO4 | TEXUSAGE_DynamicScalingAlignSSAO8 | TEXUSAGE_DynamicScalingAlignSSAO16 | TEXUSAGE_DynamicScalingAlignSSAO32 | TEXUSAGE_DynamicScalingAlignSSAO64,
	};

	enum ePrimitiveType
	{
		PRIMTYPE_LineList,
		PRIMTYPE_LineStrip,
		PRIMTYPE_TriangleList,
		PRIMTYPE_TriangleStrip,
		PRIMTYPE_PointList,
		PRIMTYPE_QuadList,
		PRIMTYPE_1CP_PATCHLIST,
		PRIMTYPE_3CP_PATCHLIST,
		PRIMTYPE_4CP_PATCHLIST,
		PRIMTYPE_Invalid,

		// Remember to keep this one uptodate
		PRIMTYPE_FIRST_CP_PATCH = PRIMTYPE_1CP_PATCHLIST,
	};

	enum eIndexFormat
	{
		INDEXFMT_UShort,
		INDEXFMT_UInt
	};

	enum eInternalTexture
	{			
		INTERTEX_Default2D,
		INTERTEX_DefaultCUBE,
		INTERTEX_DissolvePattern,
		INTERTEX_PoissonRotation,
		INTERTEX_SSAORotation,
		INTERTEX_Blank2D,
		INTERTEX_MipNoise,
		INTERTEX_FlatNormal2D,

		INTERTEX_Max
	};

	enum eSamplerStatePreset
	{
		SAMPSTATEPRESET_WrapPointNoMip,
		SAMPSTATEPRESET_WrapLinearNoMip,
		SAMPSTATEPRESET_WrapAnisoNoMip,
		SAMPSTATEPRESET_WrapPointMip,
		SAMPSTATEPRESET_WrapLinearMip,
		SAMPSTATEPRESET_WrapAnisoMip,
		SAMPSTATEPRESET_WrapLowAnisoMip,
		SAMPSTATEPRESET_WrapPointMipLinear,
		SAMPSTATEPRESET_ClampPointNoMip,
		SAMPSTATEPRESET_ClampLinearNoMip,
		SAMPSTATEPRESET_ClampAnisoNoMip,
		SAMPSTATEPRESET_ClampPointMip,
		SAMPSTATEPRESET_ClampLinearMip,
		SAMPSTATEPRESET_ClampAnisoMip,
		SAMPSTATEPRESET_MirrorPointNoMip,
		SAMPSTATEPRESET_MirrorLinearNoMip,
		SAMPSTATEPRESET_MirrorAnisoNoMip,
		SAMPSTATEPRESET_MirrorPointMip,
		SAMPSTATEPRESET_MirrorLinearMip,
		SAMPSTATEPRESET_MirrorAnisoMip,
		SAMPSTATEPRESET_MirrorOncePointNoMip,
		SAMPSTATEPRESET_MirrorOnceLinearNoMip,
		SAMPSTATEPRESET_MirrorOnceAnisoNoMip,
		SAMPSTATEPRESET_MirrorOncePointMip,
		SAMPSTATEPRESET_MirrorOnceLinearMip,
		SAMPSTATEPRESET_ClampPointNoMipCompareLess,
		SAMPSTATEPRESET_ClampLinearNoMipCompareLess,
		SAMPSTATEPRESET_AtlasLinearMip,
		SAMPSTATEPRESET_Scaleform_WrapPointMip,
		SAMPSTATEPRESET_Scaleform_WrapLinearMip,
		SAMPSTATEPRESET_Scaleform_ClampPointMip,
		SAMPSTATEPRESET_Scaleform_ClampLinearMip,

		SAMPSTATEPRESET_SpeedTreeStandardSampler,
		SAMPSTATEPRESET_SpeedTreeShadowMapCompareSampler,
		SAMPSTATEPRESET_SpeedTreePointSampler,
		SAMPSTATEPRESET_SpeedTreeLinearClampSampler,

		SAMPSTATEPRESET_ClampLinearMipNoBias, // Special case for enviro probes using UberSampling (hack for mipmap bias shifting)

		SAMPSTATEPRESET_BorderLinearNoMip,
		SAMPSTATEPRESET_BorderPointNoMip,

		SAMPSTATEPRESET_Max,
	};

	enum eDrawContext
	{
		// ace_optimize: polaczyc enumy zeby bylo mniej przelaczen

		DRAWCONTEXT_Default,					// ace_fix!!! wyrzucic?
		// Main scene draw
		DRAWCONTEXT_HiResShadows,
		DRAWCONTEXT_OcclusionQueries,
		DRAWCONTEXT_Unlit,
		DRAWCONTEXT_Emissive,
		DRAWCONTEXT_DecalsModulative,
		DRAWCONTEXT_EyeOverlay,
		DRAWCONTEXT_RefractionBuffer,
		DRAWCONTEXT_RefractionAccumBuffer,
		DRAWCONTEXT_ReflectionMask,
		DRAWCONTEXT_OpaqueNoDepthWrite,
		DRAWCONTEXT_Transparency,
		DRAWCONTEXT_HairOpaque,
		DRAWCONTEXT_HairTransparency,
		DRAWCONTEXT_TransparentBackground,
		DRAWCONTEXT_LowResParticles,
		DRAWCONTEXT_TransparencyCombine,
		DRAWCONTEXT_Sprites,
		DRAWCONTEXT_OverlayFillDepthCompare,
		DRAWCONTEXT_Overlay,
		DRAWCONTEXT_GlobalShadow,
		DRAWCONTEXT_GBufferSolid,
		DRAWCONTEXT_Range_NonStencilLightsFillFirst	= DRAWCONTEXT_GBufferSolid,
		DRAWCONTEXT_Range_NonStencilLightsFillLast	= DRAWCONTEXT_GBufferSolid,
		DRAWCONTEXT_GBufferDecalsBlended,
		DRAWCONTEXT_GBufferDecalsBlendedNormals,
		DRAWCONTEXT_DecalsFocusMode,
		DRAWCONTEXT_GlobalWater,
		DRAWCONTEXT_GBufferProjectedStripes,
		DRAWCONTEXT_GBufferStripes,
		DRAWCONTEXT_Terrain,
		// Main scene draw - stencil light channels fill
		DRAWCONTEXT_GBufferSolid_StencilLightsFill,
		DRAWCONTEXT_Range_StencilLightFillsFirst	= DRAWCONTEXT_GBufferSolid_StencilLightsFill,
		DRAWCONTEXT_Range_StencilLightFillsLast		= DRAWCONTEXT_GBufferSolid_StencilLightsFill,
		// Debug
		DRAWCONTEXT_DebugUnlit,
		DRAWCONTEXT_DebugOccluders,
		DRAWCONTEXT_DebugMesh,
		DRAWCONTEXT_DebugTransparent,
		DRAWCONTEXT_DebugOverlay,
		// ShadowMapGen
		DRAWCONTEXT_ShadowMapGenCube,
		DRAWCONTEXT_ShadowMapGenCSM_DepthTex,
		//dex++: Terrain shadows
		DRAWCONTEXT_ShadowMapGenCSM_Terrain,
		//dex--
		// Lights
		DRAWCONTEXT_LightFullscreenModulative,
		DRAWCONTEXT_LightFullscreenAdditive,
		DRAWCONTEXT_LightModulativeInside,
		DRAWCONTEXT_LightModulativeOutside,
		DRAWCONTEXT_LightAdditiveInside,
		DRAWCONTEXT_LightAdditiveOutside,
		DRAWCONTEXT_LightModulativeInsideHiStencilOpt,
		DRAWCONTEXT_LightModulativeOutsideHiStencilOpt,
		DRAWCONTEXT_LightAdditiveInsideHiStencilOpt,
		DRAWCONTEXT_LightAdditiveOutsideHiStencilOpt,
		DRAWCONTEXT_LightAdditiveOutsideStencilMaskedPre,
		DRAWCONTEXT_LightAdditiveOutsideStencilMasked,
		DRAWCONTEXT_LightAdditiveOutsideStencilMaskedPreInside,
		DRAWCONTEXT_LightAdditiveOutsideStencilMaskedInside,
		DRAWCONTEXT_LightAdditiveOutsideStencilMaskedPreModulative,
		DRAWCONTEXT_LightAdditiveOutsideStencilMaskedModulative,
		DRAWCONTEXT_LightAdditiveOutsideStencilMaskedPreInsideModulative,
		DRAWCONTEXT_LightAdditiveOutsideStencilMaskedInsideModulative,
		DRAWCONTEXT_DeferredLightsSkipStencilForward,
		DRAWCONTEXT_DeferredLights,
		// Postprocess and other 2D
		DRAWCONTEXT_NoColor_DepthStencilSet,
		DRAWCONTEXT_PostProcSet,
		DRAWCONTEXT_PostProcSet_BlueAlpha,
		DRAWCONTEXT_PostProcAdd_BlueAlpha,
		DRAWCONTEXT_PostProcSet_DepthWrite,
		DRAWCONTEXT_PostProcSet_StencilMatchAny,
		DRAWCONTEXT_PostProcSet_StencilMatchNone,
		DRAWCONTEXT_PostProcSet_StencilMatchExact,			// Use PackDrawContextRefValue() to specify refValue and readMask.
		DRAWCONTEXT_PostProcSet_HiStencilMatchAny,
		DRAWCONTEXT_PostProcPremulBlend_StencilMatchExact,	// Use PackDrawContextRefValue() to specify refValue and readMask.
		DRAWCONTEXT_PostProcAdd_StencilMatchExact,			// Use PackDrawContextRefValue() to specify refValue and readMask.
		DRAWCONTEXT_PostProcNoColor_SetStencilFull,
		DRAWCONTEXT_PostProcNoColor_SetStencilBits,
		DRAWCONTEXT_PostProcSet_SetStencilFull,
		DRAWCONTEXT_PostProcSet_SetStencilMasked,
		DRAWCONTEXT_PostProcSet_DepthEqual,
		DRAWCONTEXT_PostProcAdd,
		DRAWCONTEXT_DynamicDecalPremulBlend,
		DRAWCONTEXT_PostProcBlend,
		DRAWCONTEXT_PostProcMax,
		DRAWCONTEXT_SkinPost,
		DRAWCONTEXT_Scaleform2D,
		DRAWCONTEXT_TexRect2D,				//< hack
		DRAWCONTEXT_Text2D,					//< hack
		DRAWCONTEXT_2D,
		DRAWCONTEXT_RestoreZ,
		DRAWCONTEXT_RestoreDepth,
		DRAWCONTEXT_RestoreStencilCullEqual,
		DRAWCONTEXT_RestoreStencilCullNotEqual,
		// Other
		DRAWCONTEXT_HitProxiesSolid,
		DRAWCONTEXT_HitProxiesSprite,
		DRAWCONTEXT_FlaresOcclusionFull,
		DRAWCONTEXT_FlaresOcclusionPart,
		DRAWCONTEXT_FlaresDraw,
		DRAWCONTEXT_NoColor_DepthLE_StencilClear,			// Use PackDrawContextRefValue() to specify writeMask.
		// Fade
		DRAWCONTEXT_ScreenFade,
		// Volume rendering
		DRAWCONTEXT_VolumeExterior,
		DRAWCONTEXT_VolumeInterior,
		// Testing
		DRAWCONTEXT_SimpleNoCull,
		DRAWCONTEXT_RenderStatesTest_DrawShitToStencil,
		DRAWCONTEXT_RenderStatesTest_ReadFromStencil,
		DRAWCONTEXT_Particles,
	};

	enum
	{
		DEVSTATECAT_RenderTargetSetup	= 0x0001,
	};

	enum
	{
		MAX_RENDERTARGETS	= 8,
		MAX_UAV				= 8,
		MAX_PS_SAMPLERS			= 32,
		MAX_PS_SAMPLER_STATES	= 16,
		MAX_VS_SAMPLERS			= 16,
		MAX_VS_SAMPLER_STATES	= 16,
		MAX_CONSTANT_BUFFERS	= 32
	};


	// Buffers related

	enum eBufferUsageType
	{
		BUT_Default,
		BUT_Immutable,
		BUT_ImmutableInPlace,
		BUT_Staging,
		BUT_Dynamic,
		BUT_StreamOut,
	};

	enum eBufferAccessFlag
	{
		BAF_CPUWrite,
		BAF_CPURead,
	};

	enum eBufferLockFlag
	{
		BLF_Read			= (1<<0),
		BLF_Write			= (1<<1),
		BLF_Discard			= (1<<2),
		BLF_NoOverwrite		= (1<<3),
		BLF_DoNotWait		= (1<<4),
	};

	enum eTextureType
	{
		TEXTYPE_2D,
		TEXTYPE_CUBE,		// Faces form texture array, in order: +X,-X,+Y,-Y,+Z,-Z
		TEXTYPE_ARRAY
	};

	// NOTE : Any changes to texture formats may break texture cooking. The automated cooks should be fine, but if you need to
	// do a cook yourself, you may need to rebuild the TexCookTool projects in /dev/internal/TexCookTool.
	enum eTextureFormat
	{
		// 8 Bit
		TEXFMT_A8,
		TEXFMT_A8_Scaleform,		// Maps to R8 on DX and A8 on PS4 to match Scaleform's broken implementation
		TEXFMT_L8,
		TEXFMT_R8G8B8X8,
		TEXFMT_R8G8B8A8,
		TEXFMT_A8L8,

		// Uints
		TEXFMT_Uint_16_norm,
		TEXFMT_Uint_16,
		TEXFMT_Uint_32,
		TEXFMT_R32G32_Uint,
		TEXFMT_R16G16_Uint,

		// Float
		TEXFMT_Float_R10G10B10A2,
		TEXFMT_Float_R16G16B16A16,		
		TEXFMT_Float_R11G11B10,	
		TEXFMT_Float_R16G16,
		TEXFMT_Float_R32G32, // dex
		TEXFMT_Float_R32G32B32A32,
		TEXFMT_Float_R32,
		TEXFMT_Float_R16,
	#ifdef RED_PLATFORM_DURANGO
		TEXFMT_R10G10B10_6E4_A2_FLOAT,
	#endif

		// Depth and stencil
		TEXFMT_D24S8,
		TEXFMT_D24FS8,	//CM:xenon only
		TEXFMT_D32F,	// ORBIS only: 32bit float depth
		TEXFMT_D16U,	// ORBIS only: 16 bit unsigned int depth

		// Compressed
		TEXFMT_BC1,
		TEXFMT_BC2,
		TEXFMT_BC3,
		TEXFMT_BC4,
		TEXFMT_BC5,

		// DirectX 11 only
		TEXFMT_BC6H,
		TEXFMT_BC7,

		TEXFMT_R8_Uint,

		// Special
		TEXFMT_NULL,

		TEXFMT_Max
	};

	// Name builders
	inline const AnsiChar* GetTextureFormatName( eTextureFormat format )
	{
#define GPUAPI_TEXFMT_NAME_CASE(fmt)	case fmt: return #fmt;
		switch ( format )
		{
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_L8 );	
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_A8_Scaleform );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_R8G8B8A8 );					
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_R8G8B8X8 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_A8L8 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Uint_16_norm );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Uint_16 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Uint_32 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_R16G16_Uint );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_R32G32_Uint );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R10G10B10A2 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R16G16B16A16 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R11G11B10 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R16G16 );
		#ifdef RED_PLATFORM_DURANGO
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_R10G10B10_6E4_A2_FLOAT );
		#endif
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R32G32 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R32G32B32A32 );					
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R32 );					
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_Float_R16 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_D24S8 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_D24FS8 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_NULL );			
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_BC1 );					
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_BC2 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_BC3 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_BC4 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_BC5 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_BC6H );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_BC7 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_A8 );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_D32F );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_D16U );
			GPUAPI_TEXFMT_NAME_CASE( TEXFMT_R8_Uint );
		default:
			return "UNKNOWN_TEXFMT";
		}
#undef GPUAPI_TEXFMT_NAME_CASE
	}

	// GUI Blending types
	enum eGUIBlendStateType
	{
		GUI_BlendDesc_None,
		GUI_BlendDesc_Normal,
		GUI_BlendDesc_Layer,
		GUI_BlendDesc_Multiply,
		GUI_BlendDesc_Screen,
		GUI_BlendDesc_Lighten,
		GUI_BlendDesc_Darken,
		GUI_BlendDesc_Difference,
		GUI_BlendDesc_Add,
		GUI_BlendDesc_Subtract,
		GUI_BlendDesc_Invert,
		GUI_BlendDesc_Alpha,
		GUI_BlendDesc_Erase,
		GUI_BlendDesc_Overlay,
		GUI_BlendDesc_HardLight,
		GUI_BlendDesc_Overwrite,
		GUI_BlendDesc_OverwriteAll,
		GUI_BlendDesc_FullAdditive,
		GUI_BlendDesc_FilterBlend,
		GUI_BlendDesc_Ignore,

		GUI_BlendDesc_None_SourceAc,
		GUI_BlendDesc_Normal_SourceAc,
		GUI_BlendDesc_Layer_SourceAc,
		GUI_BlendDesc_Multiply_SourceAc,
		GUI_BlendDesc_Screen_SourceAc,
		GUI_BlendDesc_Lighten_SourceAc,
		GUI_BlendDesc_Darken_SourceAc,
		GUI_BlendDesc_Difference_SourceAc,
		GUI_BlendDesc_Add_SourceAc,
		GUI_BlendDesc_Subtract_SourceAc,
		GUI_BlendDesc_Invert_SourceAc,
		GUI_BlendDesc_Alpha_SourceAc,
		GUI_BlendDesc_Erase_SourceAc,
		GUI_BlendDesc_Overlay_SourceAc,
		GUI_BlendDesc_HardLight_SourceAc,
		GUI_BlendDesc_Overwrite_SourceAc,
		GUI_BlendDesc_OverwriteAll_SourceAc,
		GUI_BlendDesc_FullAdditive_SourceAc,
		GUI_BlendDesc_FilterBlend_SourceAc,
		GUI_BlendDesc_Ignore_SourceAc,

		GUI_BlendDesc_NoColorWrite,

		GUI_BlendDesc_Count,
	};

	enum eGUIStencilModeType
	{
		GUI_Stencil_Invalid,
		GUI_Stencil_Disabled,
		GUI_Stencil_StencilClear,
		GUI_Stencil_StencilClearHigher,
		GUI_Stencil_StencilIncrementEqual,
		GUI_Stencil_StencilTestLessEqual,
		GUI_Stencil_DepthWrite,
		GUI_Stencil_DepthTestEqual,
	};

	enum eTextureSaveFormat
	{
		SAVE_FORMAT_DDS,
		SAVE_FORMAT_BMP,
		SAVE_FORMAT_PNG,
		SAVE_FORMAT_JPG,
		SAVE_FORMAT_TGA,
	};

	enum eTextureImportFormat
	{
		TIF_DDS,
		TIF_TGA,
		TIF_WIC,
	};

	enum eShaderType : Uint32
	{
		VertexShader	,
		PixelShader		,
		GeometryShader	,
		HullShader		,
		DomainShader	,
		ComputeShader	,
		ShaderTypeMax	,
	};
	//////////////////////////////////////////////////////////////////////////////////

	// Check if primitive type is valid for tessellation
	RED_INLINE Bool IsControlPointPatch( ePrimitiveType primitive )
	{
		return primitive >= PRIMTYPE_FIRST_CP_PATCH;
	}

	// Map primitive type to it's most convenient control point patch representation
	RED_INLINE ePrimitiveType MapPrimitiveToControlPointPatch( ePrimitiveType primitive )
	{
		// Injecting tesselation
		if ( primitive == PRIMTYPE_TriangleList )
		{
			primitive = PRIMTYPE_3CP_PATCHLIST;
		}

		return primitive;
	}

	//////////////////////////////////////////////////////////////////////////////////
}