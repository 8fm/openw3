/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/os.h"
#include "../redSystem/crt.h"

// GpuAPI Utilities
#include "../gpuApiUtils//gpuApiUtils.h"

// GpuApi base
#include "gpuApiBase.h"
#include "gpuApiRenderState.h"
#include "gpuApiMapping.h"
#include "gpuApiGamma.h"

#if defined( RED_PLATFORM_WINPC )
#define USE_NVAPI
#define VENDOR_ID_NVIDIA 0x10DE

#define USE_AMDAGS
//ADL usage is obsolete, we switched to AGS
//#define USE_AMDADL 
#define VENDOR_ID_AMD 0x1002
#endif

namespace GpuApi
{

#define MAX_SHADER_COUNT 4096

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
		#ifdef RED_PLATFORM_DURANGO
			case TEXFMT_R10G10B10_6E4_A2_FLOAT:	return 32;
		#endif
			//dex++
			case TEXFMT_Float_R32G32:			return 64;
			//dex--
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
			case TEXFMT_NULL:					return 32;
			}

			GPUAPI_HALT( "Unknown device format" );
			return 0;
		}
	}
#if MICROSOFT_ATG_DYNAMIC_SCALING
	#define DYNAMIC_SCALING_MAX_MIPS		13	// up 8192
	#define DYNAMIC_SCALING_MAXSLICES		32

	// ----------------------------------------------------------------------
	struct SDynamicScalingTextureData
	{
		struct SDynamicScalingTextureData *next;
		struct SDynamicScalingTextureData *prev;
		struct STextureData *m_pTextureData;
		Uint32 m_minSizeW;
		Uint32 m_minSizeH; //< pack this shit with less bits
		Uint32 m_alignmentMinus1; //< pack this shit with less bits
		float  m_deltaSizeW;
		float  m_deltaSizeH;
		
		ID3DResource* m_pTexture[DYANMIC_SCALING_NUM_TARGETS];
		ID3DRenderTargetView* m_pRenderTargetView[DYANMIC_SCALING_NUM_TARGETS];
		ID3DDepthStencilView* m_pDepthStencilView[DYANMIC_SCALING_NUM_TARGETS];
		ID3DDepthStencilView* m_pDepthStencilViewReadOnly[DYANMIC_SCALING_NUM_TARGETS];
		ID3DShaderResourceView* m_pStencilShaderResourceView[DYANMIC_SCALING_NUM_TARGETS];
		ID3DShaderResourceView* m_pShaderResourceView[DYANMIC_SCALING_NUM_TARGETS];
		ID3D11UnorderedAccessView* m_pUnorderedAccessView[DYANMIC_SCALING_NUM_TARGETS];

		ID3DRenderTargetView* m_pp2DRenderTargetViewPerMipLevel[DYANMIC_SCALING_NUM_TARGETS][DYNAMIC_SCALING_MAX_MIPS];
		ID3DShaderResourceView* m_ppTex2DShaderResourceViewPerMipLevel[DYANMIC_SCALING_NUM_TARGETS][DYNAMIC_SCALING_MAX_MIPS];
		ID3DRenderTargetView* m_pRenderTargetViewsArray[DYANMIC_SCALING_NUM_TARGETS][DYNAMIC_SCALING_MAXSLICES];
		ID3DDepthStencilView* m_pDepthStencilViewsArray[DYANMIC_SCALING_NUM_TARGETS][DYNAMIC_SCALING_MAXSLICES];

		void Remove();
		void Add(Uint32 minSizeW, Uint32 minSizeH, float deltaSizeW, float deltaSizeH, Uint32 alignment);
		void UpdateResolution();
	};
#endif

	struct STextureData
	{
		STextureData () 
			: m_pTexture( nullptr ) 
			, m_pRenderTargetView( nullptr )
			, m_pShaderResourceView( nullptr )
			, m_pDepthStencilView( nullptr )
			, m_pDepthStencilViewReadOnly( nullptr )
			, m_pStencilShaderResourceView( nullptr )		
			, m_pUnorderedAccessView( nullptr )
			, m_ppTex2DShaderResourceViewPerMipLevel( nullptr )
			, m_ppCubeShaderResourceViewPerMipLevel( nullptr )
			, m_ppCubeShaderResourceViewPerMipFace( nullptr )
			, m_pp2DRenderTargetViewPerMipLevel( nullptr )
			, m_pRenderTargetViewsArray( nullptr )
			, m_pDepthStencilViewsArray( nullptr )
			, m_RenderTargetViewsArraySize ( 0 )
			, m_DepthTargetViewsArraySize ( 0 )
			, m_Group( TEXG_Unknown )
#ifdef RED_PLATFORM_DURANGO
			, m_textureBaseMemory( nullptr )
			, m_inplaceMemoryRegion( nullptr )
# if MICROSOFT_ATG_DYNAMIC_SCALING
			, m_pDynamicScalingData( nullptr )
# endif
#endif
		{
#ifdef GPU_API_DEBUG_PATH
			m_debugPath[0] = 0;
#endif
		}
		//dex++: merged DX10 and DX11 (to many divergent code paths)
#if MICROSOFT_ATG_DYNAMIC_SCALING
		SDynamicScalingTextureData *m_pDynamicScalingData;
#endif
		ID3DResource* m_pTexture;
		ID3DRenderTargetView* m_pRenderTargetView;
		ID3DDepthStencilView* m_pDepthStencilView;
		ID3DDepthStencilView* m_pDepthStencilViewReadOnly;
		ID3DShaderResourceView* m_pStencilShaderResourceView;
		ID3DShaderResourceView* m_pShaderResourceView;
		ID3DShaderResourceView** m_ppTex2DShaderResourceViewPerMipLevel;
		ID3DShaderResourceView** m_ppCubeShaderResourceViewPerMipLevel;
		ID3DShaderResourceView** m_ppCubeShaderResourceViewPerMipFace;
		//dex--

		Uint16 m_RenderTargetViewsArraySize;	/// Uint16 one by one for better alignment
		Uint16 m_DepthTargetViewsArraySize;		//  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		ID3DRenderTargetView** m_pRenderTargetViewsArray;
		ID3DDepthStencilView** m_pDepthStencilViewsArray;
		ID3DRenderTargetView** m_pp2DRenderTargetViewPerMipLevel;
		ID3D11UnorderedAccessView* m_pUnorderedAccessView;

		eTextureGroup m_Group;
		TextureDesc m_Desc;
		TextureRef parentTexId;

#ifdef RED_PLATFORM_DURANGO
		void* m_textureBaseMemory;
		Red::MemoryFramework::MemoryRegionHandle m_inplaceMemoryRegion;
# if MICROSOFT_ATG_DYNAMIC_SCALING
		bool m_partialResidencyAllocation;
# endif
#endif

#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif
	};

	struct SSamplerStateData
	{
		SamplerStateDesc m_Desc;
		ID3D11SamplerState* m_samplerState;
	};

	struct SBufferData
	{
		SBufferData ()
			: m_pBufferResource( NULL )
			, m_lockedBuffer( NULL )
			, m_lockedSize( 0 )
			, m_lockedOffset( 0 )
			, m_pUnorderedAccessView( NULL )
			, m_pShaderResourceView( NULL ) 
		{	  
#ifdef GPU_API_DEBUG_PATH
			m_debugPath[0] = 0;
#endif
		}

		BufferDesc									m_Desc;
		ID3D11Buffer*								m_pBufferResource;
		ID3D11ShaderResourceView*					m_pShaderResourceView;
		ID3D11UnorderedAccessView*					m_pUnorderedAccessView;
		Red::MemoryFramework::MemoryRegionHandle	m_memoryRegion;

		void* m_lockedBuffer;
		Uint32 m_lockedSize;
		Uint32 m_lockedOffset;

#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif
	};

	struct SQueryData
	{
		SQueryData ()
			: m_pQuery( NULL )
			, m_queryType( QT_Unknown )
		{}
		ID3D11Query*	m_pQuery;
		eQueryType		m_queryType;
	};

	struct SShaderData
	{
		SShaderData ()
			: m_pShader( NULL )
			, m_type( VertexShader )
		{}

		eShaderType m_type;
		ID3D11DeviceChild* m_pShader;
		ID3DBlob* m_byteCode;
#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif
	};


	struct SVertexLayoutData
	{
		SVertexLayoutData()
		{
			Red::System::MemoryZero( m_inputLayouts, sizeof( m_inputLayouts ) );
		}

		VertexLayoutDesc m_Desc;
		ID3D11InputLayout* m_inputLayouts[ MAX_SHADER_COUNT ];
	};

	struct SSwapChainData
	{
		SSwapChainData ()
			: m_swapChain( nullptr )
		{}
#if defined( RED_PLATFORM_WINPC )
		IDXGISwapChain*		m_swapChain;
#elif defined( RED_PLATFORM_DURANGO )
		IDXGISwapChain1*	m_swapChain;
#endif
		TextureRef			m_backBuffer;
		TextureRef			m_depthStencil; 
		Bool				m_fullscreen;
	};

	struct SDynamicTexture
	{
		TextureRef		m_Texture;
		const char*		m_Name;
	};

	//////////////////////////////////////////////////////////////////////////
	// Dynamic Constant Buffer

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

#if defined( RED_PLATFORM_WINPC )
	class GPUVendorApiInterface
	{
	public:
		// if this fails we move on to the next API and try that, if all fail we don't have a GPUVendorApi to use
		virtual Bool Initialize() = 0;
		virtual void Update() = 0;

		virtual Uint32 GetNumGPUs() const = 0;
		virtual Uint32 GetCurrentGPUIndex() const = 0;

		virtual void BeginEarlyPushTexture( const TextureRef& ref ) = 0;
		virtual void EndEarlyPushTexture( const TextureRef& ref ) = 0;
	};

#if defined(USE_NVAPI)
	class NvApi : public GPUVendorApiInterface
	{
	public:
		NvApi();
		~NvApi();

		Bool IsEnabled() const
		{
			return m_isEnabled;
		}

		// implementing GPUVendorApiInterface
		virtual Bool Initialize() override;
		virtual void Update() override;

		virtual Uint32 GetNumGPUs() const override;
		virtual Uint32 GetCurrentGPUIndex() const override;

		virtual void BeginEarlyPushTexture( const TextureRef& ref ) override;
		virtual void EndEarlyPushTexture( const TextureRef& ref ) override;

		// Is this frame the first time running on the current AFR group
		Bool IsFirstRun() const;

		static void BeginEarlyPushTextureUAV( const TextureRef& ref );
		static void EndEarlyPushTextureUAV( const TextureRef& ref );

		static void BeginEarlyPushUAV( const BufferRef& ref );
		static void EndEarlyPushUAV( const BufferRef& ref );

	private:
		struct NvApiImpl* m_impl;
		Bool m_isEnabled;
	};

#endif

#if defined( USE_AMDADL ) || defined( USE_AMDAGS )

	class AMDApi : public GPUVendorApiInterface
	{
	public:
		AMDApi();
		~AMDApi();

		// implementing GPUVendorApiInterface
		virtual Bool Initialize() override;
		virtual void Update() override;

		virtual Uint32 GetNumGPUs() const override;
		virtual Uint32 GetCurrentGPUIndex() const override;

		virtual void BeginEarlyPushTexture( const TextureRef& ref ) override;
		virtual void EndEarlyPushTexture( const TextureRef& ref ) override;

	private:
		class AMDApiImpl*	m_impl;
	};
#endif
#endif

	struct SDeviceData
	{
		SDeviceData ( Red::Threads::CMutex &mutex )
			 : m_pDevice( nullptr )
			 , m_pPerformanceInterface( nullptr )
			 , m_Textures( mutex )
			 , m_SamplerStates( mutex )
			 , m_Queries( mutex )
			 , m_Buffers( mutex )
			 , m_Shaders( mutex )
			 , m_VertexLayouts( mutex )
			 , m_SwapChains( mutex )
			 , m_DeviceInitDone( false )
			 , m_DeviceShutDone( false )
			 , m_StateDrawContext( DRAWCONTEXT_Default )
			 , m_StateDrawContextRef( (Uint32)-1 )
			 , m_currentVertexWritePosition( 0 )
			 , m_currentIndexWritePosition( 0 )
			 , m_constantBufferDirtyMask( 0 )
			 , m_FrequentVSConstantBuffer( nullptr )
			 , m_CustomVSConstantBuffer( nullptr )
			 , m_FrequentPSConstantBuffer( nullptr )
			 , m_CustomPSConstantBuffer( nullptr )
			 , m_CustomCSConstantBuffer( nullptr )
		#if defined( RED_PLATFORM_WINPC )
			 , m_GPUVendorInterface( nullptr )
		#endif
		{
			for ( Uint32 pssi = 0; pssi < MAX_PS_SAMPLER_STATES; ++pssi )
			{
				m_PSSamplerStatePresetsCache[pssi] = SAMPSTATEPRESET_Max;
			}
			for ( Uint32 vssi = 0; vssi < MAX_VS_SAMPLER_STATES; ++vssi )
			{
				m_VSSamplerStatePresetCache[vssi] = SAMPSTATEPRESET_Max;
			}

			Red::System::MemorySet( &m_DynamicTextures, 0, sizeof(m_DynamicTextures) );
#ifdef RED_PLATFORM_DURANGO
			Red::System::MemorySet( &m_needsUnbind, 0, sizeof(m_needsUnbind) );
#endif
			m_NumDynamicTextures = 0;
		}

		// Device internal data
#if defined( RED_PLATFORM_WINPC )
		ID3D11Device*				m_pDevice;
		ID3D11DeviceContext*		m_pImmediateContext;
#elif defined( RED_PLATFORM_DURANGO )
		ID3D11DeviceX*				m_pDevice;
		ID3D11DeviceContextX*		m_pImmediateContext;
		ID3D11DmaEngineContextX*	m_dmaContext1;
		ID3D11DmaEngineContextX*	m_dmaContext2;
		ID3D11ComputeContextX*		m_computeContext;
#endif
		ID3DUserDefinedAnnotation* m_pPerformanceInterface;
		Bool m_DeviceInitDone;
		Bool m_DeviceShutDone;
		Capabilities m_Caps;
		TextureRef m_BackBuffer;
		TextureRef m_DepthStencil;
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

#ifndef RED_FINAL_BUILD
		QueryRef													m_frameStartQuery;
		QueryRef													m_frameEndQuery;
		QueryRef													m_frameQueryDisjoint;
		QueryRef													m_frameQueryDisjointPending;
#endif

		// System chunk types
		VertexLayoutRef		m_ChunkTypeVertexLayouts[ BCT_Max ];

		// Device state shadow
		RenderTargetSetup	m_StateRenderTargetSetup;
		CRenderStateCache	m_StateRenderStateCache;
		eSamplerStatePreset m_PSSamplerStatePresetsCache[MAX_PS_SAMPLER_STATES];
		eSamplerStatePreset m_VSSamplerStatePresetCache[MAX_VS_SAMPLER_STATES];
		SamplerStateRef		m_PSSamplerStateCache[MAX_PS_SAMPLER_STATES];
		SamplerStateRef		m_VSSamplerStateCache[MAX_VS_SAMPLER_STATES];
		eDrawContext		m_StateDrawContext;
		Uint32				m_StateDrawContextRef;
		MeshStats			m_MeshStats;
		TextureStats		m_TextureStats;
		VertexLayoutRef		m_VertexLayout;
		BufferRef			m_VertexBuffers[8];
		BufferRef			m_IndexBuffer;
		BufferRef			m_indirectArgs;
		ShaderRef			m_VertexShader;
		ShaderRef			m_PixelShader;

		ShaderRef			m_GeometryShader;

		// -- Draw primitive UP mimic functionality --
		BufferRef	m_drawPrimitiveUPVertexBuffer;
		BufferRef	m_drawPrimitiveUPIndexBuffer;
		Int32			m_currentVertexWritePosition;
		Int32			m_currentIndexWritePosition;
		// ----

		//dex++ Texture preview system
		Uint32				m_NumDynamicTextures;
		SDynamicTexture		m_DynamicTextures[ 128 ];
		//dex--

		ShaderRef			m_HullShader;
		ShaderRef			m_DomainShader;
		ShaderRef			m_ComputeShader;

#ifdef RED_PLATFORM_DURANGO
		ID3D11Buffer*		m_PlacementConstantBuffer;
		void*				m_FrequentVSConstantBuffer;
		void*				m_CustomVSConstantBuffer;
		void*				m_FrequentPSConstantBuffer;
		void*				m_CustomPSConstantBuffer;

		SConstantBufferMem	m_constantBufferMem;
		Bool				m_needsUnbind[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
#else
		ID3D11Buffer*		m_FrequentVSConstantBuffer;
		ID3D11Buffer*		m_CustomVSConstantBuffer;
		ID3D11Buffer*		m_FrequentPSConstantBuffer;
		ID3D11Buffer*		m_CustomPSConstantBuffer;
#endif

		static const Uint32	sc_frequentVSConstantBufferReg = 2;
		static const Uint32	sc_customVSConstantBufferReg = 3;
		static const Uint32	sc_frequentPSConstantBufferReg = 2;
		static const Uint32	sc_customPSConstantBufferReg = 3;

		Uint8				m_constantBufferDirtyMask;

		ID3D11Buffer*		m_CustomCSConstantBuffer; //< handled is a different way than other constant buffer

		Float				m_VSConstants[NUM_VSC_REGS * 4];
		Float				m_PSConstants[NUM_PSC_REGS * 4];

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
		Uint32				m_VSConstantsDebugHistogram[256];
		Uint32				m_PSConstantsDebugHistogram[256];
#endif

#ifndef RED_FINAL_BUILD
		SDeviceUsageStats	m_DeviceUsageStats;
		Uint32				m_NumConstantBufferUpdates;
#endif

#ifdef RED_PLATFORM_WINPC
		QueryRef			m_renderFence;
#endif

#if defined( RED_PLATFORM_WINPC )
		GPUVendorApiInterface* m_GPUVendorInterface;
#endif
	};

	// ----------------------------------------------------------------------

	SDeviceData&				GetDeviceData();
	ID3D11Device*				GetDevice();
	ID3D11DeviceContext*		GetDeviceContext();
	ID3D11Resource*				GetD3DTextureBase( const TextureRef& ref );
	ID3D11Texture2D*			GetD3DTexture2D( const TextureRef& ref );
	ID3D11Texture2D*			GetD3DTextureCube( const TextureRef& ref );
	ID3D11UnorderedAccessView*	GetD3DUnorderedAccessView( const TextureRef& ref );
	ID3D11RenderTargetView*		GetD3DRenderTargetView( const TextureRef& ref, int sliceID=-1 ); // dex: added slice support
	ID3D11DepthStencilView*		GetD3DDepthStencilView( const TextureRef& ref, int sliceID=-1, Bool isReadOnly=false ); // dex: added slice support
	ID3D11Buffer*				GetD3DBuffer( const BufferRef& ref );
	ID3D11ShaderResourceView*	GetD3DShaderResourceView( const TextureRef& ref );
	const SamplerStateDesc&		GetSamplerStateDesc( const SamplerStateRef& ref );

	ID3D11InputLayout*			GetInputLayout( const VertexLayoutRef& vertexLayout, const ShaderRef& vertexShader );
	void						ReleaseInputLayouts( const ShaderRef& shader );

	ID3D11PixelShader*			GetD3DPixelShader( const ShaderRef& ref );
	ID3D11ComputeShader*		GetD3DComputeShader( const ShaderRef& ref );

	
	// ----------------------------------------------------------------------

	void InitBackbufferRef( Bool assumeRefsPresent );
	void InitSamplerStatePresets( Bool assumeRefsPresent );	
	void InitInternalTextures( Bool assumeRefsPresent );
	void InitVertexDeclarations( Bool assumeRefsPresent );
	void InitQueries( Bool assumeRefsPresent );
	void InitSystemPrimitiveBuffers( Bool assumeRefsPresent );

	void ShutBackbufferRef( Bool dropRefs );
	void ShutSamplerStatePresets( Bool dropRefs );
	void ShutInternalTextures( Bool dropRefs );
	void ShutVertexDeclarations( Bool dropRefs );
	void ShutQueries( Bool dropRefs );
	void ShutSystemPrimitiveBuffers( Bool dropRefs );

	inline void InitInternalResources( Bool assumeRefsPresent )
	{
		InitBackbufferRef( assumeRefsPresent );
		InitSamplerStatePresets( assumeRefsPresent );
		InitInternalTextures( assumeRefsPresent );
		InitVertexDeclarations( assumeRefsPresent );
		InitQueries( assumeRefsPresent );
		InitSystemPrimitiveBuffers( assumeRefsPresent );
	}

	inline void ShutInternalResources( Bool dropRefs )
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
	
	// ----------------------------------------------------------------------

	extern SDeviceData* g_DeviceData;
}

inline GpuApi::SDeviceData& GpuApi::GetDeviceData()
{
	return *GpuApi::g_DeviceData;
}

inline ID3D11Device* GpuApi::GetDevice()
{
	return GetDeviceData().m_pDevice;
}

inline ID3D11DeviceContext* GpuApi::GetDeviceContext()
{
	return GetDeviceData().m_pImmediateContext;
}
