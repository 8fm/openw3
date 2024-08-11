/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/crt.h"

// GpuAPI Utilities
#include "../gpuApiUtils/gpuApiUtils.h"

// GpuApi base
#include "gpuApiBase.h"
#include "gpuApiRenderState.h"
#include "gpuApiMapping.h"
#include "gpuApiGamma.h"

namespace GpuApi
{

#define MAX_SHADER_COUNT 4096
#define MAX_FETCH_SHADER_COUNT 2048

//#define CONSTANT_BUFFER_DEBUG_HISTOGRAM

	// ----------------------------------------------------------------------

	namespace Utils
	{

		inline Uint32 GetTextureFormatPixelSize( eTextureFormat format )
		{
			switch ( format )
			{
			case TEXFMT_A8:						return 8;
			case TEXFMT_A8_Scaleform:			return 8;
			case TEXFMT_L8:						return 8;
			case TEXFMT_R8_Uint:				return 8;
			case TEXFMT_R8G8B8A8:				return 32;
			case TEXFMT_R8G8B8X8:				return 32;
			case TEXFMT_A8L8:					return 16;
			case TEXFMT_Uint_16_norm:			return 16;
			case TEXFMT_Uint_16:				return 16;
			case TEXFMT_Uint_32:				return 32;
			case TEXFMT_R16G16_Uint:			return 32;
			case TEXFMT_R32G32_Uint:			return 64;
			case TEXFMT_Float_R10G10B10A2:		return 32;
			case TEXFMT_Float_R16G16B16A16:		return 64;
			case TEXFMT_Float_R11G11B10:		return 32;	
			case TEXFMT_Float_R16G16:			return 32;
			case TEXFMT_Float_R32G32:			return 64;
			case TEXFMT_Float_R32G32B32A32:		return 128;
			case TEXFMT_Float_R32:				return 32;
			case TEXFMT_Float_R16:				return 16;
			case TEXFMT_D24S8:					return 32;
			case TEXFMT_BC1:					return 4;
			case TEXFMT_BC2:					return 8;
			case TEXFMT_BC3:					return 8;
			case TEXFMT_BC4:					return 4;
			case TEXFMT_BC5:					return 8;
			case TEXFMT_BC6H:					return 8;
			case TEXFMT_BC7:					return 8;
			case TEXFMT_D24FS8:					return 32;
			case TEXFMT_D32F:					return 32;
			case TEXFMT_D16U:					return 16;
			case TEXFMT_NULL:					return 32;
			default:
				GPUAPI_HALT( "Unknown device format" );
				return 0;
			}
		}
	}

	// ----------------------------------------------------------------------

	struct STextureData
	{
		STextureData ()
		 : m_aliasedAsRenderTargets		(nullptr)
		 , m_aliasedAsDepthStencils		(nullptr)
		 , m_aliasedAsRenderTargetsSize (0)
		 , m_aliasedAsDepthStencilsSize (0)
		 , m_Group						(TEXG_Unknown)
		 , m_lockedOnFrame				(0)
		 , m_lockedOnBatch				(0)
		 , m_currentBuffer				(0)
		 , m_memorySize					(0)
		 , m_clearColor					(0)
		 , m_wasBoundAsTarget			(0)
		 , m_doCmask					(false)
		 , m_needsDecompress			(false)
		 , m_needsInitialClear			(false)
		 , m_isTextureAlias				(false)
		{
			m_dynamicBuffers[0] = nullptr;
			m_dynamicBuffers[1] = nullptr;
#ifdef GPU_API_DEBUG_PATH
			m_debugPath[0] = 0;
#endif
			m_lastClearingFrame = 0;
		}

		sce::Gnm::Texture							m_texture;
		sce::Gnm::RenderTarget*						m_aliasedAsRenderTargets;
		sce::Gnm::DepthRenderTarget*				m_aliasedAsDepthStencils;
		Uint16										m_aliasedAsRenderTargetsSize;	/// Uint16 one by one for better alignment
		Uint16										m_aliasedAsDepthStencilsSize;	//  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		Red::MemoryFramework::MemoryRegionHandle	m_memoryRegion;
		Red::MemoryFramework::MemoryRegionHandle	m_memoryRegionSecondary;
		eTextureGroup								m_Group;
		TextureDesc									m_Desc;
		TextureRef									parentTexId;
		Int32										m_lockedOnFrame;
		Uint32										m_lockedOnBatch;
		Uint32										m_currentBuffer;		// index of current dynamic buffer ptr
		void*										m_dynamicBuffers[2];
		Uint32										m_memorySize;
		union {
			Uint64									m_clearColor;			// color the RT was last cleared to (for consistent CMASK behavior)
			Float									m_clearDepth;			// depth value the DT was last cleared to (for consistent HTILE behavior)
		};
		Uint32										m_wasBoundAsTarget;		// stores which frame the RT was bound so we know to waitOnGfxWrites before binding as texture
		Bool										m_doCmask			: 1;	// Remember that we want a cmask for this render target
		Bool										m_needsDecompress	: 1;	// stores when the RT was bound so we know if we need to eliminateFastClear/decompressDepth
		Bool										m_needsInitialClear	: 1;	// If htile accelerated - needs initial clear before first use
		Bool										m_isTextureAlias	: 1;	// Was this created as an alias to another texture
		Uint32										m_lastClearingFrame;

#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif

		// utility functions
		void	EncodeBoundData(Int32 frame, Int16 mip);
		Bool	WasBoundThisFrame(Int32 frame);
		Uint16	DecodeBoundMipSet();
	};

	struct SSamplerStateData
	{
		SamplerStateDesc	m_Desc;
		sce::Gnm::Sampler	m_sampler;
	};


	struct SBufferData
	{
		struct VSharp
		{
			Uint32	m_discardOnBatch	: 31;	// DYNAMIC ONLY: track at what batch number the discard was last used
			Uint32	m_latestHalf		: 1;	// DYNAMIC ONLY: Which half of the buffer is the newer one (0 or 1)
			Uint32	m_elementCount;				// UAV and SRV only
		};
		struct CBuffer
		{
			Int8*	m_mappedBuffer;				// onion memory to write to
			Int8*	m_submitBuffer;				// onion memory for the gpu to read from
			Bool	m_discarded;				// Is currently discarded?
		};

		SBufferData ()
		{	  
#ifdef GPU_API_DEBUG_PATH
			m_debugPath[0] = 0;
#endif
		}
		// wraps up memory ptr access
		Int8* GetMemoryPtr() const;

		BufferDesc	m_Desc;
		Uint32		m_frameDiscarded;			// DYNAMIC ONLY: The frame on which it was last discarded
		union 
		{
			VSharp vsharp;
			CBuffer cbuf;
		};
      
		// this may be  CPU memory (dynamic constant buffer)
		//              GPU memory (static VB/IB)
		//              Double Buffer GPU memory (dynamic VB/IB)
		// do not use this directly!
		Red::MemoryFramework::MemoryRegionHandle	m_memoryRegion;

#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif
	};

	struct SQueryData
	{
		SQueryData ()
			: m_queryType( QT_Unknown )
			, m_pQuery( nullptr )
			, m_timeStamp( nullptr )
		{}

		eQueryType							m_queryType;
		sce::Gnm::OcclusionQueryResults*	m_pQuery;
		Uint64*								m_timeStamp;
	};

	enum eShaderTypeMask : Uint32
	{
		VertexShaderFlag	= FLAG(VertexShader),
		PixelShaderFlag		= FLAG(PixelShader),
		GeometryShaderFlag	= FLAG(GeometryShader),
		HullShaderFlag		= FLAG(HullShader),
		DomainShaderFlag	= FLAG(DomainShader),
		ComputeShaderFlag	= FLAG(ComputeShader),
	};

	struct SShaderData
	{
		SShaderData ()
			: m_type( VertexShader )
			, m_stageType( sce::Gnmx::kInvalidShader )
			, m_shaderStruct( nullptr )
			, m_vgtPrimCount(0)
			, m_patchCount(0)
			, m_tessConstantsMem( nullptr )
			, m_streamOutDesc (nullptr)
			, m_sourceBinaryData( nullptr )
			, m_sourceBinaryDataSize( 0 )
		{
			Red::System::MemoryZero( m_vsFetchShaderId, sizeof( m_vsFetchShaderId ) );
			Red::System::MemoryZero( m_lsFetchShaderId, sizeof( m_lsFetchShaderId ) );
			Red::System::MemoryZero( m_esFetchShaderId, sizeof( m_esFetchShaderId ) );
		}

		eShaderType						m_type;			// GpuApi level shader type, AKA a logical type (geometry shader, hull shader, etc., as opposed to hardware aware distinction like Export shader, Local Shader, specified through the field below) 
		sce::Gnmx::ShaderType			m_stageType;	// A hardware stage it is compiled for. It directly determines which pointer is the right one in the union below. 
		sce::Shader::Binary::Program	m_program;		// A binary that we have loaded. We keep it around for the information that it provides (semantics remapping, CBs, Samplers)
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		sce::Gnmx::InputResourceOffsets m_resourceOffsets;
#endif

		union
		{
			const void*				m_shaderStruct;	///< A pointer to the shader struct -- typeless.
			sce::Gnmx::VsShader*	m_vsShader;		///< A pointer to the shader struct -- used for VS shaders.
			sce::Gnmx::PsShader*	m_psShader;		///< A pointer to the shader struct -- used for PS shaders.
			sce::Gnmx::CsShader*	m_csShader;		///< A pointer to the shader struct -- used for CS shaders.
			sce::Gnmx::LsShader*	m_lsShader;		///< A pointer to the shader struct -- used for LS shaders.
			sce::Gnmx::HsShader*	m_hsShader;		///< A pointer to the shader struct -- used for HS shaders.
			sce::Gnmx::EsShader*	m_esShader;		///< A pointer to the shader struct -- used for ES shaders.
			sce::Gnmx::GsShader*	m_gsShader;		///< A pointer to the shader struct -- used for GS shaders.
			sce::Gnmx::CsVsShader*	m_csvsShader;	///< A pointer to the shader struct -- used for CS-VS shaders.
		};

		Uint32							m_samplerMask;			// bitmask of sampler state slots the shader uses
		Uint32							m_constantBufferMask;	// bitmask of constant buffer slots the shader uses
		Uint32							m_resourceMask;			// bitmask of buffers the shader uses
		Uint32							m_textureMask;			// bitmask of textures the shader uses

		// Fetch shader indices
		Uint16							m_vsFetchShaderId[1024];
		Uint16							m_lsFetchShaderId[1024];
		Uint16							m_esFetchShaderId[1024];

		// Custom stuff
		// HULL shader only
		Uint32							m_vgtPrimCount;	
		Uint32							m_patchCount;
		void*							m_tessConstantsMem;

		VertexLayoutDesc*				m_streamOutDesc;

		void*							m_sourceBinaryData;			//!< binary data this shader was created from
		Uint32							m_sourceBinaryDataSize;		//!< size of the data

		// - Scratch buffer -
		static sce::Gnm::Buffer			s_graphicsScratchBuffer;
		static Uint32					s_graphicsMaxNumWaves;
		static Uint32					s_graphicsNum1KbyteChunksPerWave ;
		static Uint32					s_graphicsScratchBufferSize;
		// -

#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif
	};

	struct SVertexLayoutEntryDesc
	{
		Uint32					m_offset;
		sce::Gnm::DataFormat	m_format;
		Uint32					m_stride;
		Uint32					m_slot;
		Bool					m_slotInstanced;

		SVertexLayoutEntryDesc()
			: m_offset( 0 )
			, m_format ( sce::Gnm::kDataFormatInvalid )
			, m_stride( 0 )
			, m_slot( 0 )
			, m_slotInstanced( false )
		{}
	};

	struct SFetchShader
	{
		Uint32	m_shaderModifier;		// Vertex shader only: Fetch shader parameter
		void*	m_fetchShaderMemory;	// Vertex shader only: Fetch shader memory
#if defined(SCE_GNMX_ENABLE_GFX_LCUE)
		sce::Gnmx::InputResourceOffsets* m_resourceTable;
#endif	
	};

	struct SVertexLayoutData
	{
		SVertexLayoutData()
		{
		}

		VertexLayoutDesc		m_Desc;												// GpuApiUtils descriptor
		SVertexLayoutEntryDesc	m_elements[ GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS ];	// GpuApiGnm descriptors
		Uint32					m_numElements;
	};

	struct SSwapChainData
	{
		enum DisplayBufferState
		{
			EDisplayBufferIdle = 0,
			EDisplayBufferInUse
		};

		struct DisplayBuffer
		{
			sce::Gnmx::GfxContext context;
			sce::Gnmx::ComputeContext computeContext;
			void *cpRamShadow;				// [CPU-Onion] some heap for mirroring CPRAM contents (GPU memory area containing constants etc.)
			void *cueHeap;					// [GPU-Garlic] heap for Constant Update Engine's ring buffers.
			void *dcbBuffer;				// [CPU-Onion] heap for Draw Command Buffer
			void *ccbBuffer;				// [CPU-Onion] heap for Constant Command Buffer
			void *computeDcbBuffer;			// [CPU-Onion] heap for Compute Command Buffer
			void *computeResBuffer;			// [GPU-Garlic] buffer to hold resource info for compute context
			Uint32 *computeResTable;		// [GPU-Garlic] resource table for compute context
			TextureRef	backBufferRef;		// Back buffer texture ref
			TextureRef	depthStencilRef;	// Depth stencil texture ref
			volatile Uint32 *state;			// Current usage state of the display buffer (idle/inuse)
		};

		static const Uint32 displayBufferCount = 2;	// Want double buffering (or more, but unlikely)

		DisplayBuffer displayBuffers[ displayBufferCount ];	// Have double+ buffering
		void *surfaceAddresses[ displayBufferCount ];		// Back buffer surface data pointers
		DisplayBuffer *backBuffer;							// Currently used back buffer
		Uint32 backBufferIndex;

		SSwapChainData()
		{
			backBuffer = displayBuffers;
			backBufferIndex = 0;
		}
	};

	struct SDynamicTexture
	{
		TextureRef		m_Texture;
		const char*		m_Name;
	};


	struct SConstantBufferMem
	{
		void	Init(void* mem, Uint32 size);
		void*	Allocate(Uint32 size, Uint32 align);
		void	FrameStart();
#ifndef RED_FINAL_BUILD
		Uint32	GetDebugThroughput() const;
		Uint32	GetDebugMemSize() const { return m_memSize; }
#endif

		Uint8* m_memStart;	// fixed start of memory pool
		Uint32 m_memSize;	// fixed size of memory pool
		Uint8* m_memPtr;	// current pointer to free memory

		// for assertion only!
		// constants must stick around for the duration the GPU is using them 
		// so ensure we do not start overwriting them silently
		Uint8* m_frameMemStart[2];	
		Uint32 m_frameIndex;
#ifndef RED_FINAL_BUILD
		Uint32 m_debugThroughput[2];
#endif
	};


	// Bit counts are provided as template parameters primarily for testing purposes. Defaults should be fine otherwise.
	template< Uint32 CONTEXT_NUM_BITS, Uint32 COUNTER_NUM_BITS = 32, Uint32 BUCKET_NUM_BITS = 64 - COUNTER_NUM_BITS - CONTEXT_NUM_BITS >
	class CFenceManager : public Red::System::NonCopyable
	{
	private:

		static_assert( COUNTER_NUM_BITS + BUCKET_NUM_BITS + CONTEXT_NUM_BITS <= 64, "Counter, bucket, and context must fit within 64 bits" );
		static_assert( COUNTER_NUM_BITS <= 32, "Counter must fit within 32 bits, because it is written into the 32bit marker by Gnm" );


		// Number of buckets to store. Increasing will provide larger history of old fences, by allocating more marker values.
		static const Uint32 NUM_BUCKETS = 2;
		static const Uint32 BUCKETS_BITMASK = ( NUM_BUCKETS - 1 );
		static_assert( ( NUM_BUCKETS & ( NUM_BUCKETS - 1 ) ) == 0, "NUM_BUCKETS must be power of 2" );
		static_assert( NUM_BUCKETS >= 2, "NUM_BUCKETS must be at least 2" );

		static const Uint32 NUM_CONTEXTS = ( 1 << CONTEXT_NUM_BITS );


		union SFenceValue
		{
			// 64-bit value to use as the actual fence value.
			Uint64				m_index;

			// Breakdown of the index, so we can track where the fence comes from.
			struct Data
			{
				Uint64		m_counter	: COUNTER_NUM_BITS;	// Fence is pending as long as the bucket value is < this value.
				Uint64		m_bucket	: BUCKET_NUM_BITS;	// Bucket index. We allow this to go beyond NUM_BUCKETS, and do the modulo when looking this up, rather than wrapping around so we can detect old fences.
				Uint64		m_context	: CONTEXT_NUM_BITS;	// Which context the fence is from.
			} m_data;
		};

		SFenceValue	m_lastValue[ NUM_CONTEXTS ];			// Track last value for each context.
		Uint32*		m_markers;								// Storage for NUM_BUCKETS values for each of NUM_CONTEXTS contexts.

	public:
		CFenceManager();
		~CFenceManager();

		// Get new fence. Write the returned value to the returned address when the fence is reached. It's assumed that the values
		// will be written in the order that the fences are gotten (i.e. someone won't get fence A, get fence B, write value for B,
		// then write value for A).
		// Given address will be 4-byte aligned.
		Uint64 GetFence( Uint32 context, Uint32*& outAddressToWrite, Uint32& outValueToWrite );

		// Only really provided for testing. Returns false when the maximum number of buckets has been exceeded. For normal
		// use and setup, the counter and bucket bits should use 64 bits effectively, and it should be impossible to actually
		// run out. For testing, it can be useful to use few bits to make sure things behave correctly.
		Bool CanGetFence( Uint32 context ) const;

		Bool IsFencePending( Uint64 fence ) const;
		Bool IsFenceExpired( Uint64 fence ) const;

		// Address is 4-byte aligned. Fence is pending as long as *addressToRead < valueToCompare.
		void GetFenceInfo( Uint64 fence, Uint32*& outAddressToRead, Uint32& outValueToCompare ) const;

	private:
		Bool IsOldValue( const SFenceValue& value ) const;
		Uint32* GetMarkerForFence( const SFenceValue& value ) const;
	};

	enum EFenceContext
	{
		FENCE_Graphics,
		FENCE_AsyncCompute,
		FENCE_MAX
	};

	namespace EmbeddedBinaries
	{
		static constexpr Uint32 cs_copyrawtexture1d_c[] = {
#include "util_shaders/cs_copyrawtexture1d_c.h"
		};
		static constexpr Uint32 cs_copyrawtexture2d_c[] = {
#include "util_shaders/cs_copyrawtexture2d_c.h"
		};
		static constexpr Uint32 cs_copyrawtexture3d_c[] = {
#include "util_shaders/cs_copyrawtexture3d_c.h"
		};
		static constexpr Uint32 cs_copytexture1d_c[] = {
#include "util_shaders/cs_copytexture1d_c.h"
		};
		static constexpr Uint32 cs_copytexture2d_c[] = {
#include "util_shaders/cs_copytexture2d_c.h"
		};
		static constexpr Uint32 cs_copytexture3d_c[] = {
#include "util_shaders/cs_copytexture3d_c.h"
		};
		static constexpr Uint32 cs_set_uint_c[] = {
#include "util_shaders/cs_set_uint_c.h"
		};
		static constexpr Uint32 cs_set_uint_fast_c[] = {
#include "util_shaders/cs_set_uint_fast_c.h"
		};
		static constexpr Uint32 vex_clear_vv[] = {
#include "util_shaders/vex_clear_vv.h"
		};
	}

	class CEmbeddedShaders
	{
#define	ADD_SHADER( _filename, _type )   \
			private: \
			ShaderRef m_ ## _filename; \
			public: \
			ShaderRef _filename() { if ( !m_ ## _filename ) m_ ## _filename = CreateShaderFromBinary( _type, EmbeddedBinaries::_filename, sizeof( EmbeddedBinaries::_filename ), #_filename ); \
			GPUAPI_ASSERT( m_ ## _filename, TXT("Failed to create an embedded shader!") ); \
			return m_ ## _filename; }
			
		ADD_SHADER( cs_copyrawtexture1d_c, ComputeShader );
		ADD_SHADER( cs_copyrawtexture2d_c, ComputeShader );
		ADD_SHADER( cs_copyrawtexture3d_c, ComputeShader );
		ADD_SHADER( cs_copytexture1d_c, ComputeShader );
		ADD_SHADER( cs_copytexture2d_c, ComputeShader );
		ADD_SHADER( cs_copytexture3d_c, ComputeShader );
		ADD_SHADER( cs_set_uint_c, ComputeShader );
		ADD_SHADER( cs_set_uint_fast_c, ComputeShader );
		ADD_SHADER( vex_clear_vv, VertexShader );

#undef ADD_SHADER
	};

	struct SDeviceData
	{
		SDeviceData ( Red::Threads::CMutex &mutex )
			: m_videoOutHandle( 0 )
			, m_DeviceInitDone( false )
			, m_DeviceShutDone( false )
			, m_Textures( mutex )
			, m_SamplerStates( mutex )
			, m_Buffers( mutex )
			, m_Queries( mutex )
			, m_Shaders( mutex )
			, m_VertexLayouts( mutex )
			, m_SwapChains( mutex )
#ifndef RED_FINAL_BUILD
			, m_queryPending( false )
#endif
			, m_StateDrawContext( DRAWCONTEXT_Default )
			, m_StateDrawContextRef( -1 )
			, m_vbChanged( false )
			, m_IndexBufferOffset( 0 )
			, m_shadersChangedMask( 0 )
			, m_currentVertexWritePosition( 0 )
			, m_currentIndexWritePosition( 0 )
			, m_currentRingCB( 0 )
			, m_timeSinceSubmitDone( 0.0f )
			, m_lastIndexBCC( BCC_Invalid )
			, m_lastPrimitiveType( PRIMTYPE_Invalid )
			, m_lastActiveShaderStages( sce::Gnm::kActiveShaderStagesVsPs )
			, m_lastNumInstances( 0 )
			, m_depthWriteNotified( false )
			, m_lastFrameWaitedForBackBuffer( -1 )
		{
			//Red::System::MemorySet( &m_PresentParams, 0, sizeof(DXGI_SWAP_CHAIN_DESC) );
			for ( Uint32 pssi = 0; pssi < MAX_PS_SAMPLER_STATES; ++pssi )
			{
				m_PSSamplerStatePresetsCache[pssi] = SAMPSTATEPRESET_Max;
			}
			for ( Uint32 vssi = 0; vssi < MAX_VS_SAMPLER_STATES; ++vssi )
			{
				m_VSSamplerStatePresetCache[vssi] = SAMPSTATEPRESET_Max;
			}

			Red::System::MemoryZero( m_fetchShaders, sizeof( m_fetchShaders ) );
			for (Uint32 i = 0; i < MAX_FETCH_SHADER_COUNT; ++i)
			{
				m_fetchShaderFreeIndices[i] = MAX_FETCH_SHADER_COUNT - 1 - i;
			}
			m_fetchShaderFreeIndex.SetValue(MAX_FETCH_SHADER_COUNT-1);

			Red::System::MemorySet( &m_DynamicTextures, 0, sizeof(m_DynamicTextures) );
			m_NumDynamicTextures = 0;
		}

		// Device internal data
		Int32 m_videoOutHandle;
		SceKernelEqueue m_eopEventQueue;
		bool m_DeviceInitDone;
		bool m_DeviceShutDone;
		Capabilities m_Caps;
		TextureRef m_BackBuffer;
		SwapChainRef m_SwapChainRef;

		//DXGI_SWAP_CHAIN_DESC m_PresentParams;
		SamplerStateRef m_SamplerStatePresets[SAMPSTATEPRESET_Max];
		TextureRef m_InternalTextures[INTERTEX_Max];
		RenderSettings m_RenderSettings;

		// Resources
		ResourceContainer< STextureData,		16384  >			m_Textures;
		ResourceContainer< SSamplerStateData,	256    >			m_SamplerStates;
		ResourceContainer< SBufferData,			32768  >			m_Buffers; // bumped again for the extensive usage of constant buffers
		ResourceContainer< SQueryData,			2048   >			m_Queries;
		ResourceContainer< SShaderData,			MAX_SHADER_COUNT >	m_Shaders;
		ResourceContainer< SVertexLayoutData,	1024   >			m_VertexLayouts;
		ResourceContainer< SSwapChainData,		32     >			m_SwapChains;

		SFetchShader*												m_fetchShaders[ MAX_FETCH_SHADER_COUNT ];
		Uint16														m_fetchShaderFreeIndices[ MAX_FETCH_SHADER_COUNT ];
		Red::Threads::CAtomic< Int32 >								m_fetchShaderFreeIndex;

#ifndef RED_FINAL_BUILD
		Bool														m_queryPending;
		QueryRef													m_frameStartQuery;
		QueryRef													m_frameEndQuery;
#endif

		// System chunk types
		VertexLayoutRef		m_ChunkTypeVertexLayouts[ BCT_Max ];

		// Device state shadow
		RenderTargetSetup	m_StateRenderTargetSetup;
		CRenderStateCache	m_StateRenderStateCache;
		eSamplerStatePreset m_PSSamplerStatePresetsCache[MAX_PS_SAMPLER_STATES];
		eSamplerStatePreset m_VSSamplerStatePresetCache[MAX_VS_SAMPLER_STATES];
		SamplerStateRef		m_SamplerStateCache[ShaderTypeMax][MAX_PS_SAMPLER_STATES];
		eDrawContext		m_StateDrawContext;
		Int32				m_StateDrawContextRef;
		MeshStats			m_MeshStats;
		TextureStats		m_TextureStats;
		VertexLayoutRef		m_VertexLayout;
		BufferRef			m_VertexBuffers[ GPUAPI_VERTEX_LAYOUT_MAX_SLOTS ];
		Uint32				m_VertexBufferOffsets[ GPUAPI_VERTEX_LAYOUT_MAX_SLOTS ];
		Uint32				m_VertexBufferStrides[ GPUAPI_VERTEX_LAYOUT_MAX_SLOTS ];
		Bool				m_vbChanged;
		BufferRef			m_IndexBuffer;
		Uint32				m_IndexBufferOffset;
		BufferRef			m_indirectArgs;
		BufferRef			m_ConstantBuffersSet[ShaderTypeMax][MAX_CONSTANT_BUFFERS];		// must cache these currently to use LCUE
		BufferRef			m_StreamOutBuffers[GPUAPI_STREAMOUT_MAX_SLOTS];
		Uint32				m_StreamOutOffsets[GPUAPI_STREAMOUT_MAX_SLOTS];

		ShaderRef			m_shadersSet[ ShaderTypeMax ];
		Uint8				m_shadersChangedMask;

		// -- Draw primitive UP mimic functionality --
		BufferRef			m_drawPrimitiveUPVertexBuffer;
		BufferRef			m_drawPrimitiveUPIndexBuffer;
		Int32				m_currentVertexWritePosition;
		Int32				m_currentIndexWritePosition;
		// ----

		Uint32				m_NumDynamicTextures;
		SDynamicTexture		m_DynamicTextures[ 128 ];

		// Vertex Buffer objects reused at each drawcall (initAsVertexBuffer(bufferptr,...))
		sce::Gnm::Buffer	m_vertexBufferObjects[ GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS ];

		static const Uint32	sc_frequentVSConstantBufferReg = 2;
		static const Uint32	sc_customVSConstantBufferReg = 3;
		static const Uint32	sc_frequentPSConstantBufferReg = 2;
		static const Uint32	sc_customPSConstantBufferReg = 3;

		sce::Gnm::Buffer	m_frequentVSConstantBuffer;
		sce::Gnm::Buffer	m_customVSConstantBuffer;
		sce::Gnm::Buffer	m_frequentPSConstantBuffer;
		sce::Gnm::Buffer	m_customPSConstantBuffer;

		Uint8				m_constantBufferDirtyMask;

		Uint32				m_currentRingCB;

		Float*				m_VSConstants;
		Float*				m_PSConstants;
		sce::Gnm::Buffer	m_CustomCSConstantBuffer;	// buffer ref stores compute shader consts buffer
		SConstantBufferMem	m_constantBufferMem;


		void*				m_globalResourceTable;

		void*				m_EsGsRingBufferMem;
		Uint32				m_EsGsRingSizeInBytes;

		void*				m_GsVsRingBufferMem;
		Uint32				m_GsVsRingSizeInBytes;

		sce::Gnm::Texture   m_texturesSet[ ShaderTypeMax ] [ MAX_PS_SAMPLERS ];
		sce::Gnm::Texture	m_textureUAVsSet[MAX_PS_SAMPLERS];		// by convention only support CS
		sce::Gnm::Buffer	m_bufferUAVsSet[MAX_PS_SAMPLERS];		// by convention only support CS
		BufferRef			m_bufferSRVsSet[ShaderTypeMax][MAX_PS_SAMPLERS];

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
		Uint32				m_VSConstantsDebugHistogram[256];
		Uint32				m_PSConstantsDebugHistogram[256];
#endif

#ifndef RED_FINAL_BUILD
		SDeviceUsageStats	m_DeviceUsageStats;
		Uint32				m_NumConstantBufferUpdates;
#endif

		void*				m_tessFactorsMem;
		sce::Gnm::Buffer	m_tessFactorsBuffer;

		CFenceManager< 1 >	m_fenceManager;
		static_assert( FENCE_MAX <= ( 1 << 1 ), "Too many EFenceContext values. Need to increase the context bits." );

		Float				m_timeSinceSubmitDone;
		eBufferChunkCategory			m_lastIndexBCC;
		ePrimitiveType					m_lastPrimitiveType;
		sce::Gnm::ActiveShaderStages	m_lastActiveShaderStages;
		Uint32							m_lastNumInstances;
		sce::Gnm::CbMode				m_lastCBMode;
		
		Bool							m_depthWriteNotified;				// A depth-write enabled drawcall occurred to the currently bound depth target

		CEmbeddedShaders				m_embeddedShaders;

		void*							m_computeQueueBuffer;
		void*							m_computeQueueBufferReadPtr;
		sce::Gnmx::ComputeQueue			m_computeQueue;

		Int32							m_lastFrameWaitedForBackBuffer;
	};

	// ----------------------------------------------------------------------

	SDeviceData&				GetDeviceData();
	const SamplerStateDesc&		GetSamplerStateDesc( const SamplerStateRef& ref );

	SFetchShader*				CreateFetchShader( const SVertexLayoutData& vertexLayout, const SShaderData& shaderData, sce::Gnm::ShaderStage shaderStage);
	void						ReleaseFetchShaders( const ShaderRef& shader );

	Int32						FrameIndex();
	Uint32						BatchIndex();	// get number of batches issued by CPU this frame

	void						SetCBControlInternal( SDeviceData& dd, sce::Gnmx::GfxContext& gfxc, sce::Gnm::CbMode cbMode );

	// ----------------------------------------------------------------------

	void InitBackbufferRef( bool assumeRefsPresent );
	void InitSamplerStatePresets( bool assumeRefsPresent );	
	void InitInternalTextures( bool assumeRefsPresent );
	void InitVertexDeclarations( bool assumeRefsPresent );
	void InitQueries( bool assumeRefsPresent );
	void InitSystemPrimitiveBuffers( Bool assumeRefsPresent );

	void ShutBackbufferRef( bool dropRefs );
	void ShutSamplerStatePresets( bool dropRefs );
	void ShutInternalTextures( bool dropRefs );
	void ShutVertexDeclarations( bool dropRefs );
	void ShutQueries( bool dropRefs );
	void ShutSystemPrimitiveBuffers( Bool dropRefs );

	inline void InitInternalResources( bool assumeRefsPresent )
	{
		InitSamplerStatePresets( assumeRefsPresent );
		InitInternalTextures( assumeRefsPresent );
		InitVertexDeclarations( assumeRefsPresent );
		InitQueries( assumeRefsPresent );
		InitSystemPrimitiveBuffers( assumeRefsPresent );
	}

	inline void ShutInternalResources( bool dropRefs )
	{
		ShutInternalTextures( dropRefs );
		ShutSamplerStatePresets( dropRefs );
		ShutBackbufferRef( dropRefs );
		ShutVertexDeclarations( dropRefs );
		ShutQueries( dropRefs );
		ShutSystemPrimitiveBuffers( dropRefs );
	}

	inline void ForceSetImplicitD3DStates()
	{
		// TODO we have to figure out what are these implicit states
	}

	inline void ForceSetExplicitD3DStates()
	{
		// ace_todo: uzupelnic to o ustawienie calego shitu!!!!! (tekstury, rendertargety etc)
		SDeviceData &dd = GetDeviceData();
		dd.m_StateRenderStateCache.RefreshDeviceState();
	}

	SSwapChainData& GetSwapChainData();


	void ResetGfxcContext(sce::Gnmx::GfxContext& gfxc, sce::Gnmx::ComputeContext& cmpc);
	Int32 SubmitAll(sce::Gnmx::GfxContext& gfxc);
	Int32 SubmitAllAndFlip(sce::Gnmx::GfxContext& gfxc, uint32_t videoOutHandler, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg);
	void DecompressDepth( sce::Gnmx::GfxContext& gfxc, sce::Gnm::DepthRenderTarget* depthRT );
	void EliminateFastClear( sce::Gnmx::GfxContext &gfxc, const sce::Gnm::RenderTarget *target );
	void WaitUntilBackBufferReady();

	Bool DecompressIfRequired(STextureData& texData);
	void SynchronizeComputeToGraphics( sce::Gnmx::GfxContext& gfxc );
	void SynchronizeGraphicsToCompute( sce::Gnmx::GfxContext& gfxc );

	void internalSetCsShader(sce::Gnmx::GfxContext& gfxc, const SShaderData* shaderData);

	// ----------------------------------------------------------------------

	extern SDeviceData* g_DeviceData;
}

inline GpuApi::SDeviceData& GpuApi::GetDeviceData()
{
	return *GpuApi::g_DeviceData;
}


#include "gpuApi.inl"
