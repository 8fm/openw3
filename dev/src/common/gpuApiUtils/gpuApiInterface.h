/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "gpuApiTypes.h"
#include "gpuApiVertexFormats.h"
#include "gpuApiErrorHandling.h"
#include "gpuApiMemory.h"
#include "gpuApiMappingCommon.h"

#include "../redSystem/utility.h" // for __LOC__
#include "../redThreads/redThreadsAtomic.h" // for exchangeadd
#include "../redMemoryFramework/redMemoryRegionAllocator.h"


#ifdef RED_PLATFORM_DURANGO
# define MICROSOFT_ATG_DYNAMIC_SCALING	1
# define DYANMIC_SCALING_NUM_TARGETS	21
#else
# define MICROSOFT_ATG_DYNAMIC_SCALING	0
#endif




#define GPUAPI_DEFINE_RESOURCE(resType, resource)		\
	typedef ResourceRef<resource> resType;				\
	void AddRef( const resType& );					\
	Int32 Release( const resType& );				\
													\
	inline void AddRefIfValid( const resType& res )	\
	{												\
		if ( resType::Null() != res )				\
		{											\
			AddRef( res );							\
		}											\
	}												\
													\
	inline void SafeRelease( resType& res )			\
	{												\
		if ( resType::Null() != res )				\
		{											\
			GpuApi::Release( res );					\
			res = resType::Null();					\
		}											\
	}												\
													\
	inline void SafeChangeRefCount( resType& res, Bool inc )	\
	{												\
		if ( resType::Null() != res )				\
		{											\
			if ( inc )								\
			{										\
				GpuApi::AddRef( res );				\
			}										\
			else									\
			{										\
				GpuApi::Release( res );				\
				res = resType::Null();				\
			}										\
		}											\
	}												\
													\
	inline void SafeRefCountAssign( resType& resDest, const resType& resSrc )	\
	{												\
		if ( resDest != resSrc )					\
		{											\
			if ( resType::Null() != resDest )		\
			{										\
				Release( resDest );					\
			}										\
													\
			resDest = resSrc;						\
													\
			if ( resType::Null() != resDest )		\
			{										\
				AddRef( resDest );					\
			}										\
		}											\
	}

// ---------------------------------------------------------------------------------------------------------------
// Enums and structures used by the gpuapi
// ---------------------------------------------------------------------------------------------------------------

namespace GpuApi
{	
	static const Uint64 GPUAPI_HASH64_BASE = 0xCBF29CE484222325; // FNV64_Prime

// Maximum number of vertex elements allowed in a vertex layout.
#define GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS		32
// Maximum number of slots allowed in a vertex layout.
#define GPUAPI_VERTEX_LAYOUT_MAX_SLOTS			8
// Maximum number of usage indices allowed (usageIndex < INDEX_COUNT)
// Changes here should be reflected in gpuApiVertexPacking.h -- PackingVertexLayout::USAGE_INDEX_COUNT
#define GPUAPI_VERTEX_LAYOUT_USAGE_INDEX_COUNT	16
// Maximum number of streamout slots
#define GPUAPI_STREAMOUT_MAX_SLOTS				4

// TODO: try to find a better place to store this, preferably just include globalConstantsPS.fx and globalConstantsVS.fx in the renderer project and pass those values to the GPU API on init.
#define NUM_VSC_REGS 21
#define NUM_PSC_REGS 45

	// Query related
	enum eQueryResult
	{
		QR_Success,
		QR_Pending,
		QR_Error
	};

	enum eQueryType
	{
		QT_Occlusion,
		QT_PipelineStats,
		QT_CommandsFinished,
		QT_Timestamp,
		QT_TimestampDisjoint,
		QT_Unknown
	};

	enum eShaderLanguage
	{
		SL_HLSL,
		SL_PSSL,
		SL_GLSL,
		SL_Unknown
	};

	struct SPipelineStatistics
	{
		Uint64 VerticesRead;
		Uint64 PrimitivesRead;
		Uint64 VertexShaderInvocations;
		Uint64 GeometryShaderInvocations;
		Uint64 PrimitivesOutputByGeometryShader;
		Uint64 PrimitivesSentToRasterizer;
		Uint64 PrimitivesRendered;
		Uint64 PixelShaderInvocations;
		Uint64 HullShaderInvocations;
		Uint64 DomainShaderInvocations;
		Uint64 ComputeShaderInvocations;
	};

	// Resource reference
	template< Uint32 >
	struct ResourceRef
	{
		//OMG how to do it!!?!?!?!?!??!!? DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_RenderData );
	public:
		explicit ResourceRef ( Uint32 initId = 0 ) 
		 : id( initId ) 
		{}

	public:		
		Bool operator==( const ResourceRef& other ) const { return id == other.id; }
		Bool operator!=( const ResourceRef& other ) const { return id != other.id; }
		operator Uint32() const { return id; }
		// This operator is not defined, because we don't want resourceRef's of different types to be assigned directly between.
		// Uint32& operator=( Uint32 newId ) { id = newId; return id; }

		RED_FORCE_INLINE Uint32 CalcHash() const { return id; }

	public:
		static ResourceRef Null() { return ResourceRef ( 0 ); }
		Bool isNull() const { return 0 == id; }

		Uint32 id;
	};
	

	// Resources
	GPUAPI_DEFINE_RESOURCE( TextureRef,			0 );
	GPUAPI_DEFINE_RESOURCE( BufferRef,			1 );
	GPUAPI_DEFINE_RESOURCE( SamplerStateRef,	2 );
	GPUAPI_DEFINE_RESOURCE( QueryRef,			3 );
	GPUAPI_DEFINE_RESOURCE( ShaderRef,			4 );
	GPUAPI_DEFINE_RESOURCE( VertexLayoutRef,	5 );
	GPUAPI_DEFINE_RESOURCE( SwapChainRef,		6 );



	struct SResourceUseStats
	{
		Uint32 m_usedTextures;
		Uint32 m_usedBuffers;
		Uint32 m_usedSamplerStates;
		Uint32 m_usedQueries;
		Uint32 m_usedShaders;
		Uint32 m_usedVertexLayouts;
		Uint32 m_usedSwapChains;

		Uint32 m_maxTextures;
		Uint32 m_maxBuffers;
		Uint32 m_maxSamplerStates;
		Uint32 m_maxQueries;
		Uint32 m_maxShaders;
		Uint32 m_maxVertexLayouts;
		Uint32 m_maxSwapChains;
	};

	

	enum eColorWrite
	{
		COLWRITE_None,
		COLWRITE_RGBA,
		COLWRITE_RGB,
	};

	enum ERasterizerMode
	{
		// Solid modes
		RASTERIZERMODE_DefaultCullCCW,
		RASTERIZERMODE_DefaultCullCW,
		RASTERIZERMODE_DefaultNoCull,
		RASTERIZERMODE_BiasedCCW,
		RASTERIZERMODE_BiasedNoCull,
		RASTERIZERMODE_CSMCullCCW,
		RASTERIZERMODE_CSMNoCull,
		
		// Speed Tree
		RASTERIZERMODE_ST_NoCull,
		RASTERIZERMODE_ST_BackCull,
		RASTERIZERMODE_ST_FrontCull,
		RASTERIZERMODE_ST_NoCull_ShadowCast,
		RASTERIZERMODE_ST_BackCull_ShadowCast,
		RASTERIZERMODE_ST_FrontCull_ShadowCast,

		// !!!!!!Wireframe version of each mode
		RASTERIZERMODE_DefaultCullCCW_Wireframe,
		RASTERIZERMODE_DefaultCullCW_Wireframe,
		RASTERIZERMODE_DefaultNoCull_Wireframe,
		RASTERIZERMODE_BiasedCCW_Wireframe,
		RASTERIZERMODE_BiasedNoCull_Wireframe,
		RASTERIZERMODE_CSMCullCCW_Wireframe,
		RASTERIZERMODE_CSMNoCull_Wireframe,

		// Speed Tree (wireframe)
		RASTERIZERMODE_ST_NoCull_Wireframe,
		RASTERIZERMODE_ST_BackCull_Wireframe,
		RASTERIZERMODE_ST_FrontCull_Wireframe,
		RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe,
		RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe,
		RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe,

		// Number of modes
		RASTERIZERMODE_Max,

		// Offset to the wireframe version of each mode
		RASTERIZERMODE_WireframeOffset = RASTERIZERMODE_DefaultCullCCW_Wireframe
	};

	inline ERasterizerMode GetTwoSidedRasterizerMode( ERasterizerMode mode )
	{
		switch( mode )
		{
		case RASTERIZERMODE_DefaultCullCCW:
		case RASTERIZERMODE_DefaultCullCW:
		case RASTERIZERMODE_DefaultNoCull:
			mode = RASTERIZERMODE_DefaultNoCull;
			break;
		case RASTERIZERMODE_BiasedCCW:
		case RASTERIZERMODE_BiasedNoCull:
			mode = RASTERIZERMODE_BiasedNoCull;
			break;
		case RASTERIZERMODE_CSMCullCCW:
		case RASTERIZERMODE_CSMNoCull:
			mode = RASTERIZERMODE_CSMNoCull;
			break;

		// Speed Tree
		case RASTERIZERMODE_ST_NoCull:
		case RASTERIZERMODE_ST_BackCull:
		case RASTERIZERMODE_ST_FrontCull:
			mode = RASTERIZERMODE_ST_NoCull;
			break;
		case RASTERIZERMODE_ST_NoCull_ShadowCast:
		case RASTERIZERMODE_ST_BackCull_ShadowCast:
		case RASTERIZERMODE_ST_FrontCull_ShadowCast:
			mode = RASTERIZERMODE_ST_NoCull_ShadowCast;
			break;

		// !!!!!!Wireframe version of each mode
		case RASTERIZERMODE_DefaultCullCCW_Wireframe:
		case RASTERIZERMODE_DefaultCullCW_Wireframe:
		case RASTERIZERMODE_DefaultNoCull_Wireframe:
			mode = RASTERIZERMODE_DefaultNoCull_Wireframe;
			break;
		case RASTERIZERMODE_BiasedCCW_Wireframe:
		case RASTERIZERMODE_BiasedNoCull_Wireframe:
			mode = RASTERIZERMODE_BiasedNoCull_Wireframe;
			break;
		case RASTERIZERMODE_CSMCullCCW_Wireframe:
		case RASTERIZERMODE_CSMNoCull_Wireframe:
			mode = RASTERIZERMODE_CSMNoCull_Wireframe;
			break;

		// Speed Tree (wireframe)
		case RASTERIZERMODE_ST_NoCull_Wireframe:
		case RASTERIZERMODE_ST_BackCull_Wireframe:
		case RASTERIZERMODE_ST_FrontCull_Wireframe:
			mode = RASTERIZERMODE_ST_NoCull_Wireframe;
			break;
		case RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe:
		case RASTERIZERMODE_ST_BackCull_ShadowCast_Wireframe:
		case RASTERIZERMODE_ST_FrontCull_ShadowCast_Wireframe:
			mode = RASTERIZERMODE_ST_NoCull_ShadowCast_Wireframe;
			break;

		default:
			RED_ASSERT( !"Invalid" );
		}

		return mode;
	};

	enum 
	{ 
		DSSM_STENCIL_VALUES_RANGE_REGULAR	= 128,
		DSSM_STENCIL_VALUES_RANGE_SPEEDTREE	= 1,
	};

	enum EDepthStencilStateMode
	{
		// No stencil
		DSSM_NoStencilFullDepthLE,
		DSSM_NoStencilFullDepthLess,
		DSSM_NoStencilFullDepthAlways, // Added for Scaleform
		DSSM_NoStencilDepthTestLE,
		DSSM_NoStencilDepthTestGE,
		DSSM_NoStencilDepthTestEQ,
		DSSM_NoStencilDepthTestLess,
		DSSM_NoStencilNoDepth,
		DSSM_NoStencilDepthTestAlways,
		DSSM_NoStencilDepthWriteLE,
		DSSM_NoStencilDepthTestEqual,

		// With stencil, varying masks
		DSSM_DepthLE_StencilSet,
		DSSM_NoDepth_StencilSet						= DSSM_DepthLE_StencilSet + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_FullDepthLE_StencilSet					= DSSM_NoDepth_StencilSet + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_LightsFilterAnyMatch_DepthTestGE		= DSSM_FullDepthLE_StencilSet + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_LightsFilterAnyMatch_DepthTestLE		= DSSM_LightsFilterAnyMatch_DepthTestGE + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_LightsFilterAnyMatch_NoDepthAlways		= DSSM_LightsFilterAnyMatch_DepthTestLE + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_LightsFilterNoneMatch_NoDepthAlways	= DSSM_LightsFilterAnyMatch_NoDepthAlways + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_LightsFilterNoneMatch_DepthTestGE		= DSSM_LightsFilterNoneMatch_NoDepthAlways + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_LightsFilterExactMatch_NoDepthAlways	= DSSM_LightsFilterNoneMatch_DepthTestGE + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		
		// With stencil, constant masks
		DSSM_LightsFill_FullDepthLE					= DSSM_LightsFilterExactMatch_NoDepthAlways + DSSM_STENCIL_VALUES_RANGE_REGULAR,
		DSSM_SetStencil_NoDepthAlways,
		DSSM_StencilIncrement_NoDepthAlways,
		DSSM_StencilDecrement_NoDepthAlways,

		// Water shaders
		DSSM_Water_FullDepthLE,						// Uses write mask, but not a range because ONLY FOR WATER!

		DSSM_SetStencil_FullDepthAlways,
		DSSM_DeferredLightsSkipStencilForward,
		DSSM_DeferredLights,
	
		// Scaleform
		DSSM_Scaleform_Invalid,
		DSSM_Scaleform_Disabled,
		DSSM_Scaleform_StencilClear,
		DSSM_Scaleform_StencilClearHigher,
		DSSM_Scaleform_StencilIncrementEqual,
		DSSM_Scaleform_StencilTestLessEqual,
		DSSM_Scaleform_DepthWrite,
		DSSM_Scaleform_DepthTestEqual,

		// Speed Tree
		DSSM_ST_WriteStencil_FullDepthL,
		DSSM_ST_WriteStencil_DepthWriteL	= DSSM_ST_WriteStencil_FullDepthL + DSSM_STENCIL_VALUES_RANGE_SPEEDTREE,
		DSSM_ST_WriteStencil_DepthTestE		= DSSM_ST_WriteStencil_DepthWriteL + DSSM_STENCIL_VALUES_RANGE_SPEEDTREE,
		DSSM_ST_WriteStencil_NoDepth		= DSSM_ST_WriteStencil_DepthTestE + DSSM_STENCIL_VALUES_RANGE_SPEEDTREE,

		// TESTING
		DSSM_WriteStencilTest				= DSSM_ST_WriteStencil_NoDepth + DSSM_STENCIL_VALUES_RANGE_SPEEDTREE,
		DSSM_ReadStencilTest,

		// number of modes
		DSSM_Max
	};

	enum EBlendMode
	{
		BLENDMODE_Set,
		BLENDMODE_Set_0RTOnlyWrites,
		BLENDMODE_Set_Red,
		BLENDMODE_Set_Green,
		BLENDMODE_Set_Blue,
		BLENDMODE_Set_Alpha,
		BLENDMODE_Set_BlueAlpha,
		BLENDMODE_Add_BlueAlpha,
		BLENDMODE_Add,
		BLENDMODE_Substract,
		BLENDMODE_Mul,
		BLENDMODE_BlendMinimum_Green,
		BLENDMODE_BlendMaximum,
		BLENDMODE_Blend,
		BLENDMODE_BlendDecals1,
		BLENDMODE_BlendDecals2,
		BLENDMODE_BlendDecalsNormals,
		BLENDMODE_BlendStripes,
		BLENDMODE_PremulBlend,
		BLENDMODE_ParticleLowRes,
		BLENDMODE_ParticleCombine,
		BLENDMODE_NoColorWrite,

		// Volumes
		BLENDMODE_Set_VolumeExterior,
		BLENDMODE_Set_VolumeInterior,

		// Scaleform
		BLENDMODE_SF_None,
		BLENDMODE_SF_Normal,
		BLENDMODE_SF_Layer,
		BLENDMODE_SF_Multiply,
		BLENDMODE_SF_Screen,
		BLENDMODE_SF_Lighten,
		BLENDMODE_SF_Darken,
		BLENDMODE_SF_Difference,
		BLENDMODE_SF_Add,
		BLENDMODE_SF_Subtract,
		BLENDMODE_SF_Invert,
		BLENDMODE_SF_Alpha,
		BLENDMODE_SF_Erase,
		BLENDMODE_SF_Overlay,
		BLENDMODE_SF_HardLight,
		BLENDMODE_SF_Overwrite,
		BLENDMODE_SF_OverwriteAll,
		BLENDMODE_SF_FullAdditive,
		BLENDMODE_SF_FilterBlend,
		BLENDMODE_SF_Ignore,

		BLENDMODE_SF_None_SourceAc,
		BLENDMODE_SF_Normal_SourceAc,
		BLENDMODE_SF_Layer_SourceAc,
		BLENDMODE_SF_Multiply_SourceAc,
		BLENDMODE_SF_Screen_SourceAc,
		BLENDMODE_SF_Lighten_SourceAc,
		BLENDMODE_SF_Darken_SourceAc,
		BLENDMODE_SF_Difference_SourceAc,
		BLENDMODE_SF_Add_SourceAc,
		BLENDMODE_SF_Subtract_SourceAc,
		BLENDMODE_SF_Invert_SourceAc,
		BLENDMODE_SF_Alpha_SourceAc,
		BLENDMODE_SF_Erase_SourceAc,
		BLENDMODE_SF_Overlay_SourceAc,
		BLENDMODE_SF_HardLight_SourceAc,
		BLENDMODE_SF_Overwrite_SourceAc,
		BLENDMODE_SF_OverwriteAll_SourceAc,
		BLENDMODE_SF_FullAdditive_SourceAc,
		BLENDMODE_SF_FilterBlend_SourceAc,
		BLENDMODE_SF_Ignore_SourceAc,

		BLENDMODE_SF_NoColorWrite,

		// Speed Tree
		BLENDMODE_ST_Set_NoAtoC_ColorWrite,
		BLENDMODE_ST_Set_NoAtoC_NoColorWrite,
		BLENDMODE_ST_Set_WithAtoC_ColorWrite,
		BLENDMODE_ST_Set_WithAtoC_NoColorWrite,
		BLENDMODE_ST_Blending_NoAtoC_ColorWrite,
		BLENDMODE_ST_Blending_NoAtoC_NoColorWrite,
		BLENDMODE_ST_Blending_WithAtoC_ColorWrite,
		BLENDMODE_ST_Blending_WithAtoC_NoColorWrite,

		// Number of modes
		BLENDMODE_Max,
	};

	// Descriptors
	struct ViewportDesc
	{
		ViewportDesc ()
		 : x( 0 )
		 , y( 0 )
		 , width( 0 )
		 , height( 0 )
		 , minZ( 0 )
		 , maxZ( 1 )
		{}

		ViewportDesc& Set( Uint32 newWidth, Uint32 newHeight, Uint32 newX, Uint32 newY, Float newMinZ, Float newMaxZ )
		{
			x = newX;
			y = newY;
			width = newWidth;
			height = newHeight;
			minZ = newMinZ;
			maxZ = newMaxZ;
			return *this;
		}

		ViewportDesc& Set( Uint32 newWidth, Uint32 newHeight, Uint32 newX, Uint32 newY )
		{
			return Set( newWidth, newHeight, newX, newY, 0.f, 1.f );
		}

		ViewportDesc& Set( Uint32 newWidth, Uint32 newHeight )
		{
			return Set( newWidth, newHeight, 0, 0, 0.f, 1.f );
		}

		Bool operator==( const ViewportDesc &other ) const
		{
			return 
				x == other.x &&
				y == other.y &&
				width == other.width &&
				height == other.height &&
				minZ == other.minZ &&
				maxZ == other.maxZ;
		}

		Bool operator!=( const ViewportDesc &other ) const
		{
			return !operator==( other );
		}

		Uint32 x;
		Uint32 y;
		Uint32 width;
		Uint32 height;
		Float minZ;
		Float maxZ;
	};

	struct SwapChainDesc 
	{
		Uint32	width;
		Uint32	height;
		Bool	fullscreen;
		Bool	overlay;
		void*	windowHandle;
	};

	enum eTextureGroup
	{
		TEXG_Unknown,
		TEXG_System,
		TEXG_Generic,
		TEXG_Streamable,
		TEXG_UI,
		TEXG_Envprobe,
		TEXG_Shadow,
		TEXG_Terrain,
		TEXG_TerrainShadow,

		TEXG_MAX,
	};

	struct TextureDesc
	{
		TextureDesc ();

		Uint32 CalcTargetSlices() const;
		Uint32 CalcTargetLevels() const;
		Bool HasMips() const;
		Bool IsInPlace() const { return inPlaceType != INPLACE_None; }

		Bool operator==( const TextureDesc &other ) const;
		Bool operator!=( const TextureDesc &other ) const { return !operator==( other ); }

		eTextureType type;					//< type
		Uint32 width;						//< width
		Uint32 height;						//< height
		Uint16 initLevels;					//< levels count to be used during initialization (zero to generate all mipmap levels)
		Uint16 sliceNum;					//< texture array slices
		Uint32 usage;						//< usage
		eTextureFormat format;				//< format
		Uint32 msaaLevel;					//< multisampling level for rendertarget surfaces.
		Int32 esramOffset;					//< XboxOne only, for RenderTarget surfaces, -1 means that the rendertarget is not in the ESRAM
		Uint32 esramSize;					//< XboxOne only, for RenderTarget surfaces
		EInPlaceResourceType inPlaceType;	//< If a texture is created in place then it doesn't have to allocate memory, also it can have a different memory pool
	};

	struct BufferDesc
	{
		BufferDesc ();

		Uint32 CalcUsedVideoMemory() const
		{
			return size;
		}

		Uint32 size;
		eBufferChunkCategory category;
		eBufferUsageType usage;
		Uint32 accessFlags;
	};

	struct TextureLevelDesc
	{
		TextureLevelDesc ()
		 : width( 0 )
		 , height( 0 )
		{}

		Uint32 width;
		Uint32 height;
	};

	struct SamplerStateDesc
	{
		SamplerStateDesc ()
			: filterMin( TEXFILTERMIN_Linear )
			, filterMag( TEXFILTERMAG_Linear )
			, filterMip( TEXFILTERMIP_Point )
			, addressU( TEXADDR_Wrap )
			, addressV( TEXADDR_Wrap )
			, addressW( TEXADDR_Wrap )
			, comparisonFunction( COMP_None )
			, mipLODBias( 0.f )
			, pointZFilter ( false )
			, allowSettingsBias ( false )
		{
			borderColor[0] = 0.f;
			borderColor[1] = 0.f;
			borderColor[2] = 0.f;
			borderColor[3] = 0.f;
		}

		bool operator==( const SamplerStateDesc &other ) const
		{
			return 
				filterMin == other.filterMin &&
				filterMag == other.filterMag &&
				filterMip == other.filterMip &&
				addressU == other.addressU &&
				addressV == other.addressV &&
				addressW == other.addressW && 
				comparisonFunction == other.comparisonFunction &&
				mipLODBias == other.mipLODBias &&
				borderColor[0] == other.borderColor[0] && 
				borderColor[1] == other.borderColor[1] && 
				borderColor[2] == other.borderColor[2] && 
				borderColor[3] == other.borderColor[3] && 
				pointZFilter == other.pointZFilter &&
				allowSettingsBias == other.allowSettingsBias;
		}

		bool operator!=( const SamplerStateDesc &other ) const
		{
			return !operator==( other );
		}

		eTextureFilterMin	filterMin;
		eTextureFilterMag	filterMag;
		eTextureFilterMip	filterMip;
		eTextureAddress		addressU;
		eTextureAddress		addressV;
		eTextureAddress		addressW;
		eComparison			comparisonFunction;
		Float				mipLODBias;
		Float				borderColor[4];
		Bool				pointZFilter;	//Xenon only
		Bool				allowSettingsBias;
	};

	struct TextureDataDesc
	{
		TextureDataDesc();

		size_t				width;		//< width
		size_t				height;		//< height
		eTextureFormat		format;		//< format
		size_t				rowPitch;	//< rowPitch
		size_t				slicePitch;	//< slicePitch
		Uint8**				data;		//< address of the texture data
	};

	struct VertexLayoutDesc
	{
		static const Uint8				invalidOverride = (Uint8) -1;

		VertexPacking::PackingElement	m_elements[GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS];
		Uint8							m_slotStrides[GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS];
		Uint32							m_slotMask;												// Bitmask of which slots have elements (non-zero stride)
		Uint32							m_elementCount;											//GL this is needed for the proper binding
		static_assert( GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS <= 32, "Slot mask may need to be bigger" );

		VertexLayoutDesc()
			: m_slotMask( 0 )
			, m_elementCount( 0 )
		{
			for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
			{
				m_elements[i] = VertexPacking::PackingElement::END_OF_ELEMENTS;
			}

			Red::System::MemoryZero(m_slotStrides, sizeof(m_slotStrides));
		}

		VertexLayoutDesc( const VertexPacking::PackingElement* elements );

		Bool operator==( const VertexLayoutDesc &other ) const
		{
			if (m_elementCount != other.m_elementCount)
			{
				return false;
			}

			for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
			{
				if ( m_elements[i] != other.m_elements[i] )
				{
					return false;
				}
			}
			return true;
		}

		Bool operator!=( const VertexLayoutDesc &other ) const
		{
			return !operator==( other );
		}

		/// Update cached strides. If you're only using AddElements to fill the Desc, this is not required to be manually called. If
		/// you're filling m_elements directly, call this once it is filled, before creating a layout.
		void UpdateStrides();

		/// Add elements from newElements until an empty PackingElement is found, or GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS elements.
		/// If slotOverride != -1, it and slotTypeOverride will be used to replace any slot/slotType info in the input elements.
		Bool AddElements( const VertexPacking::PackingElement* newElements, Uint8 slotOverride = invalidOverride, VertexPacking::eSlotType slotTypeOverride = VertexPacking::ST_Invalid );

		/// Get offset of an element within its slot. -1 if no matching element is found.
		Int32 GetElementOffset( const VertexPacking::PackingElement& element ) const;
		Int32 GetUsageOffset( VertexPacking::ePackingUsage usage, Uint8 usageIndex ) const;
		/// -1 if usage not found.
		Int32 FindHighestIndexForUsage( VertexPacking::ePackingUsage usage ) const;
		/// Get the byte stride of the given slot. 0 if slot not used.

		Uint32 GetSlotStride( Uint32 slot ) const
		{
			return m_slotStrides[slot];
		}
	};

	struct ComputeTaskDesc
	{
		Bool				m_insertSync;			// Whether the async context should synchronize with main. Use if you need to update buffers or something.

		TextureRef			m_uav;
		Uint32				m_uavIndex;
		
		TextureRef			m_inputTextures[16];
		Uint32				m_inputTextureCount;

		BufferRef			m_inputBuffers[16];
		Uint32				m_inputBufferCount;

		BufferRef			m_constantBuffers[16];
		Uint32				m_constantBufferCount;

		ShaderRef			m_shader;

		Uint32				m_threadGroupX;
		Uint32				m_threadGroupY;
		Uint32				m_threadGroupZ;

		ComputeTaskDesc()
			: m_insertSync( false )
			, m_inputTextureCount( 0 )
			, m_inputBufferCount( 0 )
			, m_constantBufferCount( 0 )
		{}
	};

	// Some simple utility functions

	/// Calculate cubemap mipmap index based on sliceIndex
	inline Uint16 CalculateCubemapMipIndexFromSlice( const TextureDesc &desc, Uint16 sliceIndex )
	{
		GPUAPI_ASSERT( TEXTYPE_CUBE == desc.type );
		GPUAPI_ASSERT( sliceIndex < desc.sliceNum * 6 * desc.initLevels );
		GPUAPI_ASSERT( desc.initLevels >= 1 );
		return sliceIndex % desc.initLevels;
	}

	inline Uint16 CalculateCubemapFaceIndexFromSlice( const TextureDesc &desc, Uint16 sliceIndex )
	{
		GPUAPI_ASSERT( (desc.type != TEXTYPE_CUBE) || sliceIndex < desc.sliceNum * 6 * desc.initLevels );
		GPUAPI_ASSERT( (desc.type == TEXTYPE_CUBE) || sliceIndex < desc.sliceNum * desc.initLevels );
		GPUAPI_ASSERT( desc.initLevels >= 1 );
		return ( sliceIndex / desc.initLevels ) % 6;
	}

	/// Calculate cubemap mipmap index based on sliceIndex
	inline Uint16 CalculateCubemapArrayIndexFromSlice( const TextureDesc &desc, Uint16 sliceIndex )
	{
		GPUAPI_ASSERT( TEXTYPE_CUBE == desc.type );
		GPUAPI_ASSERT( sliceIndex < desc.sliceNum * 6 * desc.initLevels );
		GPUAPI_ASSERT( desc.initLevels >= 1 );
		GPUAPI_ASSERT( desc.sliceNum >= 1 );
		return sliceIndex / ( desc.initLevels * 6 );
	}

	/// Calculate index of cubemap texture slice
	inline Uint16 CalculateCubemapSliceIndex( const TextureDesc &desc, Uint16 cubeSliceIndex, Uint16 faceIndex, Uint16 mipIndex )
	{
		GPUAPI_ASSERT( TEXTYPE_CUBE == desc.type );
		GPUAPI_ASSERT( cubeSliceIndex < desc.sliceNum );
		GPUAPI_ASSERT( faceIndex < 6 );
		GPUAPI_ASSERT( mipIndex < desc.initLevels );
		GPUAPI_ASSERT( desc.initLevels >= 1 );
		const Uint16 sliceIndex = (cubeSliceIndex * 6 + faceIndex) * desc.initLevels + mipIndex;
		GPUAPI_ASSERT( mipIndex == CalculateCubemapMipIndexFromSlice( desc, sliceIndex ) );
		GPUAPI_ASSERT( faceIndex == CalculateCubemapFaceIndexFromSlice( desc, sliceIndex ) );
		GPUAPI_ASSERT( cubeSliceIndex == CalculateCubemapArrayIndexFromSlice( desc, sliceIndex ) );
		return sliceIndex;
	}

	inline Uint16 CalculateCubemapPerMipSliceIndex( const TextureDesc &desc, Uint16 cubeSliceIndex, Uint16 mipIndex )
	{
		GPUAPI_ASSERT( TEXTYPE_CUBE == desc.type );
		GPUAPI_ASSERT( cubeSliceIndex < desc.sliceNum );
		GPUAPI_ASSERT( mipIndex < desc.initLevels );
		GPUAPI_ASSERT( desc.initLevels >= 1 );

		const Uint16 sliceIndex = cubeSliceIndex * desc.initLevels + mipIndex;
		return sliceIndex;
	}

	inline Uint16 CalculateTextureSliceMipIndex( const GpuApi::TextureDesc &desc, Uint16 sliceIndex, Uint16 mipIndex )
	{
		RED_ASSERT( sliceIndex < desc.sliceNum );
		RED_ASSERT( mipIndex < desc.initLevels );
		RED_ASSERT( desc.initLevels >= 1 );

		const Uint16 combinedIndex = sliceIndex * desc.initLevels + mipIndex;
		return combinedIndex;
	}

	inline Uint16 CalculateArrayIndexFromSlice( const TextureDesc &desc, Uint16 sliceIndex )
	{
		GPUAPI_ASSERT( (desc.type != TEXTYPE_CUBE) || sliceIndex < desc.sliceNum * 6 * desc.initLevels );
		GPUAPI_ASSERT( (desc.type == TEXTYPE_CUBE) || sliceIndex < desc.sliceNum * desc.initLevels );
		GPUAPI_ASSERT( desc.initLevels >= 1 );
		return sliceIndex / desc.initLevels;
	}

	inline Uint16 CalculateMipFromSliceIndex(const TextureDesc& desc, Uint16 sliceIndex)
	{
		GPUAPI_ASSERT( (desc.type != TEXTYPE_CUBE) || sliceIndex < desc.sliceNum * 6 * desc.initLevels );
		GPUAPI_ASSERT( (desc.type == TEXTYPE_CUBE) || sliceIndex < desc.sliceNum * desc.initLevels );
		GPUAPI_ASSERT( desc.initLevels >= 1 );
		return sliceIndex % desc.initLevels;
	}

	inline Bool IsStencilLightChannelsContext( eDrawContext context )
	{
		return context >= DRAWCONTEXT_Range_StencilLightFillsFirst && context <= DRAWCONTEXT_Range_StencilLightFillsLast;
	}

	inline eDrawContext GetStencilLightChannelsContext( eDrawContext context )
	{
		if ( context >= DRAWCONTEXT_Range_NonStencilLightsFillFirst && context <= DRAWCONTEXT_Range_NonStencilLightsFillLast )
		{
			context = (eDrawContext) ((context - DRAWCONTEXT_Range_NonStencilLightsFillFirst) + DRAWCONTEXT_Range_StencilLightFillsFirst);
		}

		return context;
	}

	/// Pack stencil reference value, read mask, and write mask into a single Int32, which is used by (some) draw contexts/depth-stencil modes.
	inline Uint32 PackDrawContextRefValue( Uint8 refValue, Uint8 readMask, Uint8 writeMask )
	{
		return ( (Int32)refValue | ( (Int32)readMask << 8 ) | ( (Int32)writeMask << 16 ) );
	}

	/// Unpack previously packed stencil reference value, read mask, and write mask. The outputs can be NULL if they are not needed.
	inline void UnpackDrawContextRefValue( Uint32 packed, Uint8* refValue, Uint8* readMask, Uint8* writeMask )
	{
		if ( refValue ) *refValue = ( Uint8 )( packed & 0xFF );
		if ( readMask ) *readMask = ( Uint8 )( ( packed & 0xFF00 ) >> 8 );
		if ( writeMask ) *writeMask = ( Uint8 )( ( packed & 0xFF0000 ) >> 16 );
	}


	/// Calculate texture pitch for a given width and format.
	RED_INLINE Uint32 CalculateTexturePitch( Uint32 width, eTextureFormat format )
	{
		// Keep width > 0, so if caller is using something like mip0Width >> mipLevel with a non-square texture, we aren't going to end up
		// with 0 pitch.
		width = Red::Math::NumericalUtils::Max( width, 1u );

		switch ( format )
		{
		case TEXFMT_L8:
		case TEXFMT_A8:
		case TEXFMT_A8_Scaleform:
		case TEXFMT_R8_Uint:
			return width;

		case TEXFMT_A8L8:
		case TEXFMT_Float_R16:
		case TEXFMT_Uint_16_norm:
		case TEXFMT_Uint_16:
			return width * 2;

		case TEXFMT_R8G8B8X8:
		case TEXFMT_R8G8B8A8:
		case TEXFMT_Float_R10G10B10A2:
		case TEXFMT_Float_R11G11B10:
		case TEXFMT_Float_R16G16:
		case TEXFMT_R16G16_Uint:
		case TEXFMT_Float_R32:
	#ifdef RED_PLATFORM_DURANGO
		case TEXFMT_R10G10B10_6E4_A2_FLOAT:
	#endif
			return width * 4;

		case TEXFMT_Float_R16G16B16A16:
		case TEXFMT_Float_R32G32:
		case TEXFMT_R32G32_Uint:
			return width * 8;

		case TEXFMT_Float_R32G32B32A32:
			return width * 16;

		case TEXFMT_BC1:
		case TEXFMT_BC4:
			return Red::Math::NumericalUtils::Max( width * 2, 8u );

		case TEXFMT_BC2:
		case TEXFMT_BC3:
		case TEXFMT_BC5:
		case TEXFMT_BC6H:
		case TEXFMT_BC7:
			return Red::Math::NumericalUtils::Max( width * 4, 16u );

		case TEXFMT_D24S8:
			return width * 4;

		default:
			GPUAPI_HALT( "Not supported yet. Please add a case for texture format %" RED_PRIWas, GetTextureFormatName( format ) );
			return 0;
		}
	}
	/// Calculate byte-size of data for a texture with given size and format. Just one subresource, not a full mip chain.
	RED_INLINE Uint32 CalculateTextureSize( Uint32 width, Uint32 height, eTextureFormat format )
	{
		Uint32 pitch = CalculateTexturePitch( width, format );

		// Keep height > 0, so if caller is using something like mip0Height >> mipLevel with a non-square texture, we aren't going to end up
		// with 0 size. We don't need to do the same with width, since CalculateTexturePitch takes care of that for us.
		height = Red::Math::NumericalUtils::Max( height, 1u );

		switch ( format )
		{
			// Compressed formats combine multiple rows in the image into one data row.
		case TEXFMT_BC1:
		case TEXFMT_BC2:
		case TEXFMT_BC3:
		case TEXFMT_BC4:
		case TEXFMT_BC5:
		case TEXFMT_BC6H:
		case TEXFMT_BC7:
			return pitch * Red::Math::NumericalUtils::Max( height / 4, 1u );

		default:
			return pitch * height;
		}
	}

	/// Calculate dimension of a texture mip level. Width and height are treated equally, so this can be used for either.
	/// Clamps output to 1 for uncompressed formats, 4 for block-compressed.
	RED_INLINE Uint32 CalculateTextureMipDimension( Uint32 baseSize, Uint8 mip, eTextureFormat format )
	{
		const Bool isCompressed = IsTextureFormatDXT( format );
		return Red::Math::NumericalUtils::Max< Uint32 >( baseSize >> mip, isCompressed ? 4 : 1 );
	}



	// Runtime shit

	/// Render target setup doesn't do any implicit AddRef/Release calls!!
	struct RenderTargetSetup
	{
		RenderTargetSetup ();

		//dex++: added slice support and NULL target support
		RenderTargetSetup& SetNullColorTarget();
		RenderTargetSetup& SetColorTarget( Uint32 index, const TextureRef &target, Int16 arraySlice=-1 );
		RenderTargetSetup& SetDepthStencilTarget( const TextureRef &target, Int16 arraySlice=-1, Bool isReadOnly=false );
		//dex--

		RenderTargetSetup& SetUnorderedAccessView( Uint32 index, const TextureRef &target );
		RenderTargetSetup& SetUnorderedAccessView( Uint32 index, const BufferRef &target );
		RenderTargetSetup& SetViewport( const ViewportDesc &viewport );
		RenderTargetSetup& SetViewport( Uint32 newWidth, Uint32 newHeight, Uint32 newX, Uint32 newY );
		RenderTargetSetup& SetViewport( Uint32 newWidth, Uint32 newHeight );
		RenderTargetSetup& SetViewportFromTarget( const TextureRef &target );
		RenderTargetSetup& ChangeAllRefCounts( bool inc );
		RenderTargetSetup& Reset();
		
		Uint32 numColorTargets;
		Uint32 numUAVs;

		TextureRef		colorTargets[MAX_RENDERTARGETS];
		Int16			colorTargetsSlices[MAX_RENDERTARGETS]; //-1 for 2D textures
		TextureRef		unorderedAccessViews[MAX_UAV];
		BufferRef		unorderedAccessViewsBuf[MAX_UAV];
		TextureRef		depthTarget;
		Bool			depthTargetReadOnly;
		Int32			depthTargetSlice; //-1 for 2D textures
		ViewportDesc	viewport;
	};

	RED_INLINE const Char* GetTextureGroupName( const GpuApi::eTextureGroup& group )
	{
		switch (group)
		{
		case TEXG_Unknown:			return TXT("TEXG_Unknown");
		case TEXG_System:			return TXT("TEXG_System");
		case TEXG_Generic:			return TXT("TEXG_Generic");
		case TEXG_Streamable:		return TXT("TEXG_Streamable");
		case TEXG_UI:				return TXT("TEXG_UI");
		case TEXG_Envprobe:			return TXT("TEXG_Envprobe");
		case TEXG_Shadow:			return TXT("TEXG_Shadow");
		case TEXG_Terrain:			return TXT("TEXG_Shadow");
		case TEXG_TerrainShadow:	return TXT("TEXG_Shadow");
		default:
			GPUAPI_HALT( "Unhandled texture group" );
		}
		return TXT("Unknown");
	}

	struct MeshStats
	{
		Red::Threads::AtomicOps::TAtomic32	m_vertexBufferMemory;
		Red::Threads::AtomicOps::TAtomic32	m_indexBufferMemory;
		Red::Threads::AtomicOps::TAtomic32	m_constantBufferMemory;
		Red::Threads::AtomicOps::TAtomic32	m_otherBufferMemory;

		MeshStats()
			: m_vertexBufferMemory( 0 )
			, m_indexBufferMemory( 0 )
			, m_constantBufferMemory( 0 )
			, m_otherBufferMemory( 0 )
		{}

		void AddBuffer( Int32 bufferMemory, eBufferChunkCategory category )
		{
			if ( category == BCC_Vertex || category == BCC_VertexSRV )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_vertexBufferMemory, bufferMemory );
			}
			else if ( category == BCC_Index16Bit || category == BCC_Index32Bit || category == BCC_Index16BitUAV )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_indexBufferMemory, bufferMemory );
			}
			else if ( category == BCC_Constant )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_constantBufferMemory, bufferMemory );
			}
			else
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_otherBufferMemory, bufferMemory );
			}
		}

		void RemoveBuffer( Int32 bufferMemory, eBufferChunkCategory category )
		{
			if ( category == BCC_Vertex || category == BCC_VertexSRV )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_vertexBufferMemory, -bufferMemory );
			}
			else if ( category == BCC_Index16Bit || category == BCC_Index32Bit || category == BCC_Index16BitUAV )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_indexBufferMemory, -bufferMemory );
			}
			else if ( category == BCC_Constant )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_constantBufferMemory, -bufferMemory );
			}
			else
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_otherBufferMemory, -bufferMemory );
			}
		}
	};

	/// Texture stats
	struct TextureStats
	{
		Red::Threads::AtomicOps::TAtomic32	m_systemTextureMemory;
		Red::Threads::AtomicOps::TAtomic32	m_systemTextureCount;
		Red::Threads::AtomicOps::TAtomic32	m_genericTextureMemory;
		Red::Threads::AtomicOps::TAtomic32	m_genericTextureCount;
		Red::Threads::AtomicOps::TAtomic32	m_streamableTextureMemory;
		Red::Threads::AtomicOps::TAtomic32	m_streamableTextureCount;
		Red::Threads::AtomicOps::TAtomic32  m_streamableTextureMemoryInFlight;
		Red::Threads::AtomicOps::TAtomic32  m_streamableTextureCountInFlight;
		Red::Threads::AtomicOps::TAtomic32	m_guiTextureMemory;
		Red::Threads::AtomicOps::TAtomic32	m_guiTextureCount;
		Red::Threads::AtomicOps::TAtomic32	m_envprobeTextureMemory;
		Red::Threads::AtomicOps::TAtomic32	m_envprobeTextureCount;

		TextureStats()
			: m_systemTextureMemory( 0 )
			, m_systemTextureCount( 0 )
			, m_genericTextureMemory( 0 )
			, m_genericTextureCount( 0 )
			, m_streamableTextureMemory( 0 )
			, m_streamableTextureCount( 0 )
			, m_streamableTextureMemoryInFlight( 0 )
			, m_streamableTextureCountInFlight( 0 )
			, m_guiTextureMemory( 0 )
			, m_guiTextureCount( 0 )
			, m_envprobeTextureMemory( 0 )
			, m_envprobeTextureCount( 0 )
		{};

		Bool GetTextureStats( eTextureGroup group, Int32& textureMemory, Int32& textureCount )
		{
			switch ( group )
			{
			case TEXG_System:
				textureMemory	= m_systemTextureMemory;
				textureCount	= m_systemTextureCount;
				return true;
			case TEXG_UI:
				textureMemory	= m_guiTextureMemory;
				textureCount	= m_guiTextureCount;
				return true;
			case TEXG_Generic:
				textureMemory	= m_genericTextureMemory;
				textureCount	= m_genericTextureCount;
				return true;
			case TEXG_Streamable:
				textureMemory	= m_streamableTextureMemory;
				textureCount	= m_streamableTextureCount;
				return true;
			case TEXG_Envprobe:
				textureMemory	= m_envprobeTextureMemory;
				textureCount	= m_envprobeTextureCount;
				return true;
			default:
				textureMemory = -1;
				textureCount = -1;
				return false;
			}
		}

		void IncrementTextureMemoryInFlight( Int32 inFlightMemory )
		{
			if ( inFlightMemory > 0 )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_streamableTextureMemoryInFlight, inFlightMemory );
				Red::Threads::AtomicOps::Increment32( &m_streamableTextureCountInFlight );
			}
		}

		void DecrementTextureMemoryInFlight( Int32 inFlightMemory )
		{
			if ( inFlightMemory > 0 )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_streamableTextureMemoryInFlight, -inFlightMemory );
				Red::Threads::AtomicOps::Decrement32( &m_streamableTextureCountInFlight );
			}
		}

		void AddTexture( Int32 textureMemory, eTextureGroup group )
		{
			if ( group == TEXG_System )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_systemTextureMemory, textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_systemTextureCount, 1 );	
			}
			else if ( group == TEXG_UI )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_guiTextureMemory, textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_guiTextureCount, 1 );	
			}
			else if ( group == TEXG_Generic )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_genericTextureMemory, textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_genericTextureCount, 1 );	
			}
			else if ( group == TEXG_Streamable )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_streamableTextureMemory, textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_streamableTextureCount, 1 );	
			}
			else if ( group == TEXG_Envprobe )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_envprobeTextureMemory, textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_envprobeTextureCount, 1 );	
			}
		}

		void RemoveTexture( Int32 textureMemory, eTextureGroup group )
		{
			if ( group == TEXG_System )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_systemTextureMemory, -textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_systemTextureCount, -1 );	
			}
			else if ( group == TEXG_UI )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_guiTextureMemory, -textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_guiTextureCount, -1 );	
			}
			else if ( group == TEXG_Generic )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_genericTextureMemory, -textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_genericTextureCount, -1 );	
			}
			else if ( group == TEXG_Streamable )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_streamableTextureMemory, -textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_streamableTextureCount, -1 );	
			}
			else if ( group == TEXG_Envprobe )
			{
				Red::Threads::AtomicOps::ExchangeAdd32( &m_envprobeTextureMemory, -textureMemory );
				Red::Threads::AtomicOps::ExchangeAdd32( &m_envprobeTextureCount, -1 );	
			}
		}
	};

#if MICROSOFT_ATG_DYNAMIC_SCALING
	Bool UpdateDynamicScaling( const bool allowUpScale );
	void RevertToPreviousDynamicScale();
	Uint32 DynamicScalingGetTargetWidth(const GpuApi::TextureRef &ref);
	Uint32 DynamicScalingGetTargetHeight(const GpuApi::TextureRef &ref);
#endif

	/// Device capabilities
	struct Capabilities
	{
		Capabilities ()
		 : depthBoundTest( false )
		 , renderToCubeMap( true )
		 , textureArray( true ) // dex
		 , maxAnisotropy( 8 )
		 , hardwareHistogram( false )
		{}

		bool depthBoundTest;
		bool nullTextures;
		bool renderToCubeMap;
		bool textureArray; //dex
		Uint32 maxAnisotropy;
		Bool hardwareHistogram;
	};

	/// Device settings
	struct RenderSettings
	{
		Float		mipMapBias;
		Uint32		anisotropy;
		Bool		wireframe;		

		RenderSettings ()
		 : mipMapBias( 0 )
		 , anisotropy( 8 )
		 , wireframe( false )
		{}

		Bool Validate();

		RED_INLINE Bool operator != ( const RenderSettings& other ) const
		{
			return ( mipMapBias != other.mipMapBias ) || ( anisotropy != other.anisotropy ) || ( wireframe != other.wireframe );
		}

		RED_INLINE Bool operator == ( const RenderSettings& other ) const
		{
			return ( mipMapBias == other.mipMapBias ) && ( anisotropy == other.anisotropy ) && ( wireframe == other.wireframe );
		}

		RED_INLINE Bool SamplersChanged( const RenderSettings& other ) const 
		{
			return ( mipMapBias == other.mipMapBias ) && ( anisotropy == other.anisotropy );
		}

	};

	/// Device states shadowing stucture
	/** All GpuApi resources are AddRef/Release safe. */
	class DeviceState
	{
		friend DeviceState GrabDeviceState( Uint32 categories );
		
	public:
		DeviceState ();
		DeviceState ( const DeviceState &other );
		~DeviceState ();
		DeviceState& operator= ( const DeviceState &other );
		
	public:
		/// Return state categories which this device state object contains
		inline Uint32 GetCategories() const { return m_categories; }

		/// Return rendertarget setup 
		inline const RenderTargetSetup& GetRenderTargetSetup() const { return m_renderTargetSetup; }

		/// Clear device state
		void Clear();

	private:
		void ChangeAllRefCounts( bool inc );

	private:
		Uint32 m_categories;
		RenderTargetSetup	m_renderTargetSetup;
	};


	// Device

	enum eDeviceCooperativeLevel
	{
		DEVICECOOPLVL_Operational,
		DEVICECOOPLVL_Reset,
		DEVICECOOPLVL_Removed,
		DEVICECOOPLVL_Occluded,
		DEVICECOOPLVL_Unknown
	};

	struct SDeviceUsageStats
	{
		Uint32	m_NumConstantBufferUpdates;
#ifdef RED_PLATFORM_CONSOLE
		Uint32	m_constantMemoryLoad;
		Bool	m_isConstantBufferMemoryWithinBounds;
#endif
		Float	m_GPUFrameTime;
		Float	m_GPUFrequency;
	};

	struct ShaderDefine
	{
		const AnsiChar* Name;
		const AnsiChar* Definition;
	};


	struct TextureLevelInitData
	{
		const void*	m_data;			//< The data used to initialize the mip level
		Bool		m_isCooked;		//< Whether the data is in a cooked format
	};
	struct TextureInitData
	{
		// Note! Use one or the other (uncooked individual mips, or full cooked data)

		// Array of initial data for each slice/mip, ordered as as:
		//     slice0mip0, slice0mip1, ..., slice0mipM, slice1mip0, etc.
		// Can provide a mix of cooked/uncooked data.
		//////////////////////////////////////////////////////////////////////////
		// TODO : Switch ordering of these, so that it's the same as what cooked data looks like?
		const TextureLevelInitData* m_mipsInitData;

		// Single block of cooked data for entire texture.
		Red::MemoryFramework::MemoryRegionHandle m_cookedData;
		Bool m_isCooked;									//< Whether to use cookedData or mipsInitData.
	};

	struct BufferInitData
	{
		BufferInitData() : m_buffer( nullptr ), m_elementCount( 0 ) {}

		// Note! Use one or the other (just buffer pointer or memory region handle)
		Red::MemoryFramework::MemoryRegionHandle	m_memRegionHandle;		// In-place buffers
		const void*									m_buffer;				// All other buffers

		// elementCount is only used when creating raw or structured buffers, these will get UAV and SRV created automatically
		Uint32										m_elementCount;
	};

	struct SDMABatchElement
	{
		void*	dst;
		void*	src;
		Uint32	size;

		void Set( void* inDst, void* inSrc, Uint32 inSize )
		{
			dst = inDst;
			src = inSrc;
			size = inSize;
		}
	};

	// Display Mode Desc
	struct DisplayModeDesc
	{
		struct RefreshRate
		{
			Int32 numerator;
			Int32 denominator;
		};

		Int32 width;
		Int32 height;
		RefreshRate refreshRate;
	};
}


// ---------------------------------------------------------------------------------------------------------------
// The real interface starts here
// ---------------------------------------------------------------------------------------------------------------

namespace GpuApi
{
	//////////////////////////////////////////////////////////////////////////
	// Setup and management of the gpuapi and the device

	Bool					InitEnv();
	Bool					InitDevice( Uint32 width, Uint32 height, Bool fullscreen, Bool vsync );
	Bool					IsInit();
	const Capabilities&		GetCapabilities();
	eDeviceCooperativeLevel TestDeviceCooperativeLevel();
	Bool					ResetDevice();
	void					ShutDevice();

	void					SuspendDevice();
	void					ResumeDevice();

#ifndef RED_FINAL_BUILD
	//////////////////////////////////////////////////////////////////////////
	// Statistics and debugging

	void GetResourceUseStats( SResourceUseStats& stats );
	void					DumpResourceStats();
	const SDeviceUsageStats& GetDeviceUsageStats();
#endif

	//////////////////////////////////////////////////////////////////////////
	// Profile blocks / Annotations

	void					BeginProfilerBlock( const Char* name );
	void					EndProfilerBlock();
	void					SetMarker( const Char* name );

	
	//////////////////////////////////////////////////////////////////////////
	// Device contexts and command lists

	void*					CreateContext();
	void					SubmitContext( void* deferredContext );
	void					CancelContext( void* deferredContext );
	void*					GetCommandListFromContext( void* deferredContext );
	void					SubmitCommandList( void* commandList );
	void					CancelCommandList( void* commandList );

	
	//////////////////////////////////////////////////////////////////////////
	// Render flow functions

	void					BeginRender();
	void					EndRender();

	
	//////////////////////////////////////////////////////////////////////////
	// Swapchains and backbuffers

	SwapChainRef			CreateSwapChainWithBackBuffer( const SwapChainDesc& swapChainDesc );
	void					SetBackBufferFromSwapChain( const SwapChainRef& swapChain );
	void					ResizeBackbuffer( Uint32 width, Uint32 height, const SwapChainRef& swapChain );
	Bool					ToggleFullscreen( const SwapChainRef& swapChain, Bool fullscreen );
	Bool					ToggleFullscreen( const SwapChainRef& swapChain, Bool fullscreen, Int32 outputMonitorIndex );
	void					Present( const Rect* sourceRect, const Rect* destRect, const SwapChainRef& swapChain, Bool useVsync, Uint32 vsyncThreshold );
	void					PresentMultiplane( const Rect* sourceRect, const Rect* destRect, const SwapChainRef& swapChain, const SwapChainRef& swapChainOverlay, Bool useVsync, Uint32 vsyncThreshold );
	void					SetGammaForSwapChain( const SwapChainRef& swapChain, Float gammaValue );
	void					PS4_SubmitDone();
	Float					PS4_GetAndUpdateTimeSinceSubmitDone( Float timeToAdd );
	void					DX_WaitOnVSyncEvent();
	Int32					GetMonitorCount();
	Bool					GetMonitorCoordinates( Int32 monitorIndex, Int32& top, Int32& left, Int32& bottom, Int32& right );
	void					ClearState();
	Bool					GetFullscreenState( const SwapChainRef& swapChain );

	//////////////////////////////////////////////////////////////////////////
	// Adapters and outputs
	void					GetNativeResolution( Uint32 outputIndex, Int32& width, Int32& height );
	Bool					EnumerateDisplayModes( Int32 monitorIndex, Uint32* outNum, DisplayModeDesc** outDescs = nullptr );	// When outDescs == nullptr, returns only outNum
	Bool					HasMinimumRequiredGPU();

	//////////////////////////////////////////////////////////////////////////
	// Clearing functions

	// Avoid using these directly. Instead, opt for the functions in CRenderInterface.

	Bool					ClearColorTarget( const TextureRef &target, const Float* colorValue );
	Bool					ClearDepthTarget( const TextureRef &target, Float depthValue, Int32 slice = -1 );
	Bool					ClearStencilTarget( const TextureRef &target, Uint8 stencilValue, Int32 slice = -1 );
	Bool					ClearDepthStencilTarget( const TextureRef &target, Float depthValue, Uint8 stencilValue, Int32 slice = -1 );

	
	//////////////////////////////////////////////////////////////////////////
	// Drawing functions

	void					DrawPrimitive( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount );
	void					DrawPrimitiveRaw( ePrimitiveType primitive, Uint32 startVertex, Uint32 primitiveCount );
	void					DrawPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 primitiveCount );
	void					DrawIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount );
	void					DrawSystemPrimitive( ePrimitiveType primitive, Uint32 primitiveCount, eBufferChunkType vertexType, const void *vertexBuffer );
	void					DrawSystemIndexedPrimitive( ePrimitiveType primitive, Uint32 minVertexIndex, Uint32 numVertices, Uint32 primitiveCount, const Uint16 *indexBuffer, eBufferChunkType vertexType, const void *vertexBuffer );
	void					DrawSystemIndexedPrimitive32( ePrimitiveType primitive, Uint32 minVertexIndex, Uint32 numVertices, Uint32 primitiveCount, const Uint32 *indexBuffer, eBufferChunkType vertexType, const void *vertexBuffer );
	void					DrawInstancedIndexedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount );
	void					DrawInstancedPrimitiveIndirect( ePrimitiveType primitive );
	void					DrawInstancedIndexedPrimitiveIndirect( ePrimitiveType primitive );
	void					DrawInstancedPrimitive( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 numVertices, Uint32 primitiveCount, Uint32 instancesCount );
	void					DrawInstancedPrimitiveNoBuffers( ePrimitiveType primitive, Uint32 vertexCount, Uint32 instancesCount );
	void					DrawIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount );
	void					DrawInstancedIndexedPrimitiveRaw( ePrimitiveType primitive, Int32 baseVertexIndex, Uint32 minIndex, Uint32 numVertices, Uint32 startIndex, Uint32 primitiveCount, Uint32 instancesCount );

	/// Template for system primitives render, so that you wouldn't have to explicitly provide vertex type.
	template< class _VertexType >
	void					DrawSystemPrimitive( ePrimitiveType primitive, Uint32 primitiveCount, const _VertexType *vertexBuffer )
	{
		DrawSystemPrimitive( primitive, primitiveCount, _VertexType::GetSystemVertexType(), vertexBuffer );
	}

	/// Template for system indexed primitives render, so that you wouldn't have to explicitly provide vertex type.
	template< class _VertexType >
	void					DrawSystemIndexedPrimitive( ePrimitiveType primitive, Uint32 minVertexIndex, Uint32 numVertices, Uint32 primitiveCount, const Uint16 *indexBuffer, const _VertexType *vertexBuffer )
	{	
		DrawSystemIndexedPrimitive( primitive, minVertexIndex, numVertices, primitiveCount, indexBuffer, _VertexType::GetSystemVertexType(), vertexBuffer );
	}

	void					DispatchCompute( ShaderRef& computeShader, Uint32 x, Uint32 y, Uint32 z );

	
	//////////////////////////////////////////////////////////////////////////
	// Draw context management

	void					SetDrawContext( eDrawContext newContext, Uint32 newRefValue );
	eDrawContext			GetDrawContext();
	Uint32					GetDrawContextRefValue();
	void					SetDrawContextRefValue( Uint32 refValue );

	// Build new refValue for DRAWCONTEXT_PostProcSet_SetStencilMasked
	RED_INLINE Uint32		GetMaskedStencilValue( Uint8 writeBits, Uint8 readBits , Uint8 writeMask )
	{
		return static_cast<Uint32>(writeBits) | ( static_cast<Uint32>(readBits) << 8 ) | ( static_cast<Uint32>(writeMask) << 16 );
	}

	//////////////////////////////////////////////////////////////////////////
	// Two sided rendering
	// This setting is independant of draw context and does not change implicitly.

	Bool					IsForcedTwoSidedRender();
	void					SetForcedTwoSidedRender( bool force );	


	//////////////////////////////////////////////////////////////////////////
	// Reversed Projection
	
	Bool					IsReversedProjectionState();
	void					SetReversedProjectionState( bool isReversed );	
	Float					GetClearDepthValueRevProjAware();


	//////////////////////////////////////////////////////////////////////////
	// Shadow bias

	void					SetupShadowDepthBias( Float depthBiasClamp, Float slopeScaledDepthBias );
	

	//////////////////////////////////////////////////////////////////////////
	// Device state

	DeviceState				GrabDeviceState( Uint32 categories );
	void					GrabDeviceState( DeviceState &outState, Uint32 categories );
	void					RestoreDeviceState( const DeviceState &state, Uint32 categoriesMask );
	void					RestoreDeviceState( const DeviceState &state );
	const RenderTargetSetup&	GetRenderTargetSetup();

	
	//////////////////////////////////////////////////////////////////////////
	// Render settings

	void					SetRenderSettings( const RenderSettings &newSettings );
	void					SetRenderSettingsWireframe( Bool enable );
	const RenderSettings&	GetRenderSettings();	

	
	//////////////////////////////////////////////////////////////////////////
	// Render targets / viewports

	void					SetupBlankRenderTargets();
	void					SetupRenderTargets( const RenderTargetSetup &setup );
	void					SetViewportRaw( const ViewportDesc& viewport );	
	void					WaitForDepthTargetWrites( Bool flushHTile );

	Bool					IsSetupSupported( const RenderTargetSetup &setup );
	void					GetRenderTargetsSetup( const RenderTargetSetup &setup );
	void					SetViewport( const ViewportDesc &viewport );
	void					GetViewport( ViewportDesc &viewport );
	const ViewportDesc&		GetViewport();

	// HACK moradin: this is somewhat hacky, the swapchain has the backbuffer and possibly 
	// the corresponding depthstencil but most of the rendering functions don't have any access to this
	TextureRef				GetBackBufferTexture(); 
	TextureRef				GetDepthStencilTexture(); 

	//////////////////////////////////////////////////////////////////////////
	// Screenshots

	void					GrabBackBuffer( Uint32* targetBuffer, const Rect& r, const Uint32 fullHDChunks, const Uint32 chunkNumX, const Uint32 chunkNumY, const Uint32 fullHDResX );
#ifndef RED_PLATFORM_CONSOLE
	void					TakeScreenshot( Uint32 targetScreenshotWidth, Uint32 targetScreenshotHeight, eTextureSaveFormat format, const Char* fileName, void* buffer, size_t bufferSize, size_t* outBufferSizeWritten );
	void					TakeScreenshotPCImplementation( Uint32 targetScreenshotWidth, Uint32 targetScreenshotHeight, eTextureSaveFormat format, const Char* fileName, void* buffer, size_t bufferSize, size_t* outBufferSizeWritten );
	Bool					SaveBufferToFile( Uint32* targetBuffer, const Char* fileName, const Uint32 width, const Uint32 height, eTextureSaveFormat saveFormat, const Bool normalize = false, const Uint32 denominator = 1, eTextureFormat srcDataFormat =TEXFMT_R8G8B8X8 );
	Bool					SaveTextureToFile( const TextureRef& textureRef, const Char* fileName, const eTextureSaveFormat format );
	Bool					SaveTextureToFile( const TextureDataDesc& image, const Char* fileName, const eTextureSaveFormat saveFormat );
#endif


	//////////////////////////////////////////////////////////////////////////
	// Shaders

	ShaderRef				CreateGeometryShaderWithSOFromSource( const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, Uint32 numDefines, const char* fileName, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc = nullptr );
	ShaderRef				CreateGeometryShaderWithSOFromBinary( const void* shaderBuffer, Uint32 shaderBufferSize, const VertexLayoutDesc& outputDesc, GpuApi::VertexLayoutDesc* adjustedDesc = nullptr );
	ShaderRef				CreateShaderFromSource( eShaderType shaderType, const char* code, const char* mainFunction, const char* shaderTarget, ShaderDefine* defines, Uint32 numDefines, const char* fileName );
	ShaderRef				CreateShaderFromBinary( eShaderType shaderType, const void* shaderBuffer, Uint32 shaderBufferSize, const char* debugName = nullptr );
	Uint32					GetShaderCodeSize( const ShaderRef& ref );
	void					CopyShaderCode( const ShaderRef& ref, void* targetCode );
	Bool					GetShaderTargetAndEntryPoint( eShaderType shaderType, const AnsiChar*& outShaderTarget, const AnsiChar*& outMainFunction );

	void					SetShader( const ShaderRef& shader, eShaderType shaderType );

	void					SetPixelShaderConstF( Uint32 first, const Float* data, Uint32 num );
	void					SetVertexShaderConstF( Uint32 first, const Float* data, Uint32 num );
	
	void					SetComputeShaderConstsRaw( Uint32 dataSize, const void *dataBuffer );

	template< class _DataClass >
	void					SetComputeShaderConsts( const _DataClass &data )
	{
		SetComputeShaderConstsRaw( sizeof(data), &data );
	}

	void					SetShaderDebugPath( const ShaderRef& shader, const char* debugPath );
	
	const Char*				GetShaderRootPath();
	const AnsiChar*			GetShaderRootPathAnsi();
	const Char*				GetShaderIncludePath();
	const AnsiChar*			GetShaderIncludePathAnsi();

	eShaderLanguage			GetShaderLanguage();

	//////////////////////////////////////////////////////////////////////////
	// Textures
	
	// Create texture with optional initial data.
	// If using desc.inPlaceType!=0, initData must be provided, and must have m_isCooked set. The memory should be passed via a region handle
	// If CreateTexture succeeds, it takes ownership of the inplace memory, and the caller should no longer use it.
	//
	// inPlaceCreated is limited to TEXUSAGE_Immutable textures.
	// Cooked textures must be TEXUSAGE_Immutable.
	TextureRef				CreateTexture( const TextureDesc &desc, eTextureGroup group, const TextureInitData* initData = nullptr );
	TextureRef				CreateTextureAlias( const TextureDesc &desc, const TextureRef& texture );

	TextureRef				CreateTextureFromMemoryFile( const void *memoryFile, Uint32 fileSize, eTextureGroup group, TextureDesc *outDesc = nullptr );

	// Binding
	void					BindTextures( Uint32 startSlot, Uint32 numTextures, const TextureRef *textures, eShaderType shaderStage );
	void					BindTextureCubeMipLevel( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, eShaderType shaderStage );
	void					BindTextureCubeMipFace( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, Uint32 faceIndex, eShaderType shaderStage );
	void					BindTextureMipLevel( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, eShaderType shaderStage );
	void					BindTextureStencil( Uint32 slot, const TextureRef &texture, eShaderType shaderStage );

	// Fast semantic binding, faster only on XB1
	void					BindTexturesFast( Uint32 startSlot, Uint32 numTextures, const TextureRef *textures, eShaderType shaderStage );

	
	// UAVs are only valid for compute shaders so there is no reason to set a shader stage
	void					BindTextureUAVs( Uint32 startSlot, Uint32 numTextures, const TextureRef* textures );
	void					BindTextureMipLevelUAV( Uint32 slot, const TextureRef& texture, Uint32 sliceIndex, Uint32 mipIndex );

	// Locking
	void*					LockLevel( const TextureRef& ref, Uint32 level, Uint32 slice, Uint32 lockFlags, Uint32& outPitch );
	void					UnlockLevel( const TextureRef& ref, Uint32 level, Uint32 slice );

	Uint32					CalcTextureSize( const TextureRef &tex );
	Uint32					CalcTextureSize( const TextureDesc &desc );

	void					GetTextureDesc( const TextureRef &ref, TextureDesc &outDesc );
	const TextureDesc&		GetTextureDesc( const TextureRef &ref );
	void					GetTextureLevelDesc( const TextureRef &ref, Uint16 level, TextureLevelDesc &outDesc );
	TextureLevelDesc		GetTextureLevelDesc( const TextureRef &ref, Uint16 level );
	void					SetTextureDebugPath( const TextureRef &tex, const char* debugPath );
	const MeshStats			*GetMeshStats();
	const TextureStats		*GetTextureStats();

#ifndef NO_TEXTURE_IMPORT
	// Texture import / export
	Bool					ImportTexture( const wchar_t* importPath, eTextureImportFormat format, /*out*/void*& src, /*out*/Uint32& rowPitch, /*out*/Uint32& depthPitch, /*out*/TextureDesc& desc );
#endif

	enum EImageCompressionHint
	{
		CIH_None,
		CIH_NormalmapRGB,
	};

	Bool					CompressImage( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& compressedImage, EImageCompressionHint compressionHint, Float alphaThreshold = 0.5f );
	Bool					DecompressImage( const TextureDataDesc& srcImage, TextureDataDesc& decompressedImage );

	// Copy data from one texture to another. Copies the entire subresource, so source and destination must be the same size and format.
	Bool					CopyTextureData( const TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice );
	Bool					CopyTextureDataAsync( const TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice, void* deferredContext );

	// Copy region of source to destination. Mip level 0. Size of region is same. Fails if formats differ, multisamples differ, or otherwise unsupported.
	Bool					CopyRect( const TextureRef &sourceRef, const Rect& sourceRect, Uint32 sourceArraySlice, const TextureRef &destRef, Uint32 destX, Uint32 destY, Uint32 destArraySlice );

	// DMA copy data from one texture to another. Copies the entire subresource, so source and destination must be the same size and format.
	// Tested only for situations from ESRAM to DRAM (formerly known as resolve) so far
	void					CopyTextureDataDMA( const TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice );

	// A general purpose memory move that leverages DMA requests inserted into draw command buffer
	void					DMAMemory( void* dst, void* src, Uint32 numBytes, Bool isBlocking );

	void					BatchedDMAMemory( SDMABatchElement* batch, Uint32 batchCount, Bool isBlocking );

	// Texture loading. Copies data from given source buffer into the given mip and array slice. destRect allows the caller to copy into
	// only a region of the texture, but may be null to indicate the entire texture. For non-array textures, slice should be 0.
	void					LoadTextureData2D( const TextureRef &destTex, Uint32 mipLevel, Uint32 arraySlice, const Rect* destRect, const void *srcMemory, Uint32 srcPitch );
	void					LoadTextureData2DAsync( const TextureRef &destTex, Uint32 mipLevel, Uint32 arraySlice, const Rect* destRect, const void *srcMemory, Uint32 srcPitch, void* deferredContext );

	Bool					GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Uint8 *outDataRGBA, Uint32 stride, Bool forceFullAlpha );
	Bool					GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Float *outData );
	Bool					SaveTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, const Char* fileName, eTextureSaveFormat format );
	Bool					SaveTextureToMemory( const TextureRef &texture, eTextureSaveFormat format, const Rect* sourceRect, void** buffer, Uint32& size );
	Bool					SaveTextureToMemory( const Uint8* textureData, const size_t textureDataSize, const size_t width, size_t height, const eTextureFormat format, const size_t pitch, const eTextureSaveFormat saveFormat, void** buffer, size_t& size );

	// Other stuff
	Bool					IsDescSupported( const TextureDesc &desc );
	TextureRef				GetInternalTexture( eInternalTexture internalTexture );

	// automatic mipmap generation ( for ESM shadows )
	Bool					GenerateMipmaps( const TextureRef &texRef );

	// Texture debug preview
	Uint32					GetNumDynamicTextures();
	const char*				GetDynamicTextureName( Uint32 index );
	TextureRef				GetDynamicTextureRef( Uint32 index );
	void					AddDynamicTexture( TextureRef tex, const char* name );
	void					RemoveDynamicTexture( TextureRef tex );

	// Texture streaming stats
	void					IncrementInFlightTextureMemory( Uint32 textureSize );
	void					DecrementInFlightTextureMemory( Uint32 textureSize );


	// Calculate the position and size of a single mip within a cooked texture's data.
	Bool					CalculateCookedTextureMipOffsetAndSize( const TextureDesc& texDesc, Uint32 mip, Uint32 slice, Uint32* outOffset, Uint32* outSize );
	// Calculate the total size of a cooked texture's data.
	Uint32					CalculateCookedTextureSize( const TextureDesc& texDesc );


	// Get the MemoryRegionHandle that was used when creating an in-place texture. If texture was not in-place created, will return a
	// null handle. Should not be used on platforms that don't have full support (i.e. DX11)
	Red::MemoryFramework::MemoryRegionHandle GetTextureInPlaceMemory( const TextureRef& texture );


	//////////////////////////////////////////////////////////////////////////
	// Buffers

	// bufferSize is the byte size of the buffer, or elementCount*stride but we only need the elementCount when creating raw or structured buffers
	// category describes the type of the buffer
	// usage 
	// accessFlags 
	// BufferInitData - buffer pointer or inplace memory region handle
	BufferRef				CreateBuffer( Uint32 bufferSize, eBufferChunkCategory category, eBufferUsageType usage, Uint32 accessFlags, const BufferInitData* initData = nullptr );

	// creates a copy of any buffer that will be accessible from the CPU
	// it can block as it uses CopySubresourceRegion to copy the data to the staging buffer
	// the created buffer is 
	BufferRef				CreateStagingCopyBuffer( BufferRef target );

	void					GetBufferDesc( const BufferRef &ref, BufferDesc &outDesc );
	const BufferDesc&		GetBufferDesc( const BufferRef &ref );

	void					SetBufferDebugPath( const BufferRef &ref, const char* debugPath );

	// 
	void*					LockBuffer( const BufferRef &ref, Uint32 flags, Uint32 offset, Uint32 size );
	void*					LockBufferAsync( const BufferRef &ref, Uint32 flags, Uint32 offset, Uint32 size, void* deferredContext );

	void					UnlockBuffer( const BufferRef &ref );
	void					UnlockBufferAsync( const BufferRef &ref, void* deferredContext );

	void					LoadBufferData( const GpuApi::BufferRef &destBuffer, Uint32 offset, Uint32 size, const void* srcMemory );

	void					CopyBuffer( const BufferRef &dest, Uint32 destOffset, const BufferRef &source, Uint32 sourceOffset, Uint32 length );

	void					BindVertexBuffers( Uint32 startIndex, Uint32 count, const BufferRef* buffers, const Uint32* strides, const Uint32* offsets );
	void					BindNullVertexBuffers();
	void					BindIndexBuffer( const BufferRef &buffer, Uint32 offset = 0 );
	void					BindIndirectArgs( const BufferRef &buffer );
	void					BindBufferSRV( const BufferRef &buffer, Uint32 slot, eShaderType shaderStage = PixelShader );
	void					BindBufferUAV( const BufferRef &buffer, Uint32 slot );
	void					BindConstantBuffer( Uint32 slot, BufferRef buffer, eShaderType shaderStage = PixelShader );
	void					BindStreamOutBuffers( Uint32 count, const BufferRef* buffers, const Uint32* offsets );

	void					ClearBufferUAV_Uint( const BufferRef &buffer, const Uint32 values[4] );

	//////////////////////////////////////////////////////////////////////////
	// Sampler states

	SamplerStateRef			RequestSamplerState( const SamplerStateDesc &desc );
	void					GetSamplerStateDesc( const SamplerStateRef &ref, SamplerStateDesc &outDesc, eShaderType shaderStage = PixelShader );
	void					SetSamplerStateCommon( Uint32 startSlot, Uint32 numSamplers, const eSamplerStatePreset &statePreset, eShaderType shaderStage = PixelShader );
	void					SetSamplerStatePreset( Uint32 slot, const eSamplerStatePreset &statePreset, eShaderType shaderStage = PixelShader );
	void					SetSamplerState( Uint32 slot, const SamplerStateRef& ref, eShaderType shaderStage = PixelShader );
	Bool					IsDescSupported( const SamplerStateDesc &desc );
	void					InvalidateSamplerStates();
	void					ResetSamplerStates();
	void					SetSamplerStateDebugPath( const SamplerStateRef &ref, const char* debugPath );	

	/// Returns samplerStateRef for given samplerStatePreset
	SamplerStateRef			GetSamplerStatePreset( eSamplerStatePreset preset );

	
	//////////////////////////////////////////////////////////////////////////
	// Vertex layouts

	VertexLayoutRef			CreateVertexLayout( const VertexLayoutDesc &desc );
	Bool					IsDescSupported( const VertexLayoutDesc &desc );
	void					SetVertexFormatRaw( VertexLayoutRef vertexLayout, Bool setInputLayout = true );
	Uint32					GetVertexLayoutStride( const VertexLayoutRef& ref, Uint32 slot = 0 );
	const VertexLayoutDesc*	GetVertexLayoutDesc( const VertexLayoutRef& ref );

	// BufferChunkType vertex layouts
	void					SetVertexFormatRaw( eBufferChunkType chunkType, Bool setInputLayout = true );
	VertexLayoutRef			GetVertexLayoutForChunkType( eBufferChunkType chunkType );
	eBufferChunkType		GetChunkTypeForVertexLayout( VertexLayoutRef layout );			//< BCT_MAX if the vertex layout is not from a chunk type.
	Uint32					GetChunkTypeStride( eBufferChunkType type, Uint32 slot = 0  );


	//////////////////////////////////////////////////////////////////////////
	// Queries

	QueryRef				CreateQuery( eQueryType queryType );
	Bool					CreateQueries( Uint32 numQueries, QueryRef *outQueries );		//< Given queries table should have NULL references
	Bool					BeginQuery( const QueryRef &query );
	Bool					EndQuery( const QueryRef &query );
	eQueryResult			GetQueryResult( const QueryRef &query, Uint64& outResult, Bool forceImmediate );
	eQueryResult			GetQueryResult( const QueryRef &query, SPipelineStatistics& outStats, Bool forceImmediate );
	eQueryResult			GetQueryResult( const QueryRef &query, Uint64& frequency, Bool& disjoint, Bool forceImmediate );
	Uint8					GetNumberOfDelayedFrames();

	//////////////////////////////////////////////////////////////////////////
	// Synchronization

	void					Flush();
	Uint64					InsertFence();
	void					InsertWaitOnFence( Uint64 fence );
	Bool					IsFencePending( Uint64 fence );
	void					ReleaseFence( Uint64 fence );
	void					InsertWriteToMemory( volatile Uint64* dstAddress, Uint64 value );
	volatile Uint64*		AllocateValue();
	void					DeallocateValue( volatile Uint64* value );

	//////////////////////////////////////////////////////////////////////////
	// Synchronization
	void					AddSyncComputeTaskToQueue( const ComputeTaskDesc& computeTaskDesc );
	void					AddAsyncComputeTaskToQueue( const ComputeTaskDesc& computeTaskDesc );
	Uint64					KickoffAsyncComputeTasks();

	//////////////////////////////////////////////////////////////////////////
	// Middleware utils

	void					BindMainConstantBuffers();

	// refValue should be packed with PackDrawContextRefValue().
	void					SetCustomDrawContext( EDepthStencilStateMode depthStencilMode, ERasterizerMode rasterizerMode, EBlendMode blendMode, Uint32 packedRefValue = 1 );


	// Validate the gpu command buffer
	void					ValidateContext(const char* msg);

	//////////////////////////////////////////////////////////////////////////
	// MultiGPU

	Bool MultiGPU_IsActive();
	Uint32 MultiGPU_GetNumGPUs();
	void MultiGPU_BeginEarlyPushTexture( const TextureRef &tex );
	void MultiGPU_EndEarlyPushTexture( const TextureRef &tex );

	//////////////////////////////////////////////////////////////////////////
	// For internal use, but shared between platforms

	Int32 FrameIndex();

	// On consoles, where we can manage resource memory directly, we need to be able to destroy some things "sometime in the future," when
	// they are no longer in a command list or anything. These functions are for that.

	// Schedule a resource for future destruction. On PC, because we have no memory control, these will call Destroy() immediately.
	void QueueForDestroy( const TextureRef& ref );
	void QueueForDestroy( const BufferRef& ref );
	void QueueForDestroy( const QueryRef& ref );
	void QueueForDestroy( const ShaderRef& ref );

	// Destroy a resource now. These must be implemented per-platform, to actually free whatever resources.
	void Destroy( const TextureRef& ref );
	void Destroy( const BufferRef& ref );
	void Destroy( const QueryRef& ref );
	void Destroy( const ShaderRef& ref );

	// Destroy queued resources when it is safe. This should be called once per frame. The current implementation just waits a couple frames
	// before destroying anything.
	void UpdateQueuedResourceDeletions();

	void SetVsWaveLimits(Uint32 waveLimitBy16, Uint32 lateAllocWavesMinus1);
	void ResetVsWaveLimits();
}


/// Device state grabber utility
class CGpuApiDeviceStateGrabber
{
public:
	CGpuApiDeviceStateGrabber ( GpuApi::Uint32 categories );
	~CGpuApiDeviceStateGrabber ();

public:
	/// Restores device state (automatic restore will be skipped in destructor).
	void Restore();

private:
	GpuApi::DeviceState m_State;
};


/// Scoped draw context 
class CGpuApiScopedDrawContext
{
	CGpuApiScopedDrawContext ( const CGpuApiScopedDrawContext& );				// no definition
	CGpuApiScopedDrawContext& operator= ( const CGpuApiScopedDrawContext& );	// no definition

public:
	CGpuApiScopedDrawContext ( );
	CGpuApiScopedDrawContext ( GpuApi::eDrawContext context, GpuApi::Uint32 refValue = (GpuApi::Uint32)-1 );
	~CGpuApiScopedDrawContext ( );

	GpuApi::eDrawContext GetOrigDrawContext() const { return m_prevContext; }
	GpuApi::Int32 GetOrigRefValue() const { return m_prevRefValue; }

private:
	GpuApi::eDrawContext m_prevContext;
	GpuApi::Uint32 m_prevRefValue;
};


/// Scoped forced two sided render
class CGpuApiScopedTwoSidedRender
{
	CGpuApiScopedTwoSidedRender ( const CGpuApiScopedTwoSidedRender& );				// no definition
	CGpuApiScopedTwoSidedRender& operator= ( const CGpuApiScopedTwoSidedRender& );	// no definition

public:
	CGpuApiScopedTwoSidedRender ();
	CGpuApiScopedTwoSidedRender ( GpuApi::Bool forceTwoSidedRendering );
	~CGpuApiScopedTwoSidedRender ();

	void SetForcedTwoSided( GpuApi::Bool forceTwoSidedRendering );
	GpuApi::Bool GetOrigTwoSidedRendering() const { return m_prevTwoSided; }

private:
	GpuApi::Bool m_prevTwoSided;
};


#undef GPUAPI_DEFINE_RESOURCE
