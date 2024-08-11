/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "..\redSystem\crt.h"

// GpuAPI Utilities
#include "..\\gpuApiUtils\\gpuApiUtils.h"

// GpuApi base
#include "gpuApiBase.h"
#include "gpuApiRenderState.h"
#include "gpuApiMapping.h"
//#include "gpuApiGamma.h"

namespace GpuApi
{

#define MAX_SHADER_COUNT 4096

#define USE_OPENGL_PROFILE

#ifdef USE_OPENGL_PROFILE

	void ErrorReport( Int32 errNo , const char* expr, const char* file, Uint32 lineNumber );

	#define OGL_CHK(a)		a;	if( Int32 errNo = glGetError() ) ::GpuApi::ErrorReport( errNo , #a , __FILE__ , __LINE__ )

#else
	#define OGL_CHK(a)		a
#endif

	// ----------------------------------------------------------------------

	namespace Utils
	{

		inline Uint32 GetTextureFormatPixelSize( eTextureFormat format )
		{
			switch ( format )
			{
			case TEXFMT_A8:						return 8;
			case TEXFMT_L8:						return 8;
			case TEXFMT_R8_Uint:				return 8;
			case TEXFMT_R8G8B8A8:				return 32;
			case TEXFMT_R8G8B8X8:				return 32;
			case TEXFMT_A8L8:					return 16;
			case TEXFMT_Uint_16_norm:			return 16;
			case TEXFMT_Uint_16:				return 16;
			case TEXFMT_R16G16_Uint:			return 32;
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
			case TEXFMT_BC4:					return 8;
			case TEXFMT_BC6H:					return 8;
			case TEXFMT_BC7:					return 8;
			case TEXFMT_NULL:					return 32;
			}

			GPUAPI_HALT( "Unknown device format" );
			return 0;
		}
	}

	// ----------------------------------------------------------------------

	struct STextureData
	{
		STextureData ()
		 : m_texture( 0 )
		 //, m_pRenderTargetView( nullptr )
		 //, m_pDepthStencilView( nullptr )
		 //, m_pDepthStencilViewReadOnly( nullptr )
		 //, m_pShaderResourceView( nullptr )
		 //, m_pUnorderedAccessView( nullptr )
		 //, m_pRenderTargetViewsArray( nullptr )
		 //, m_pDepthStencilViewsArray( nullptr )
		 , m_RenderTargetViewsArraySize ( 0 )
		 , m_DepthTargetViewsArraySize ( 0 )
		 , m_Group( TEXG_Unknown )
#ifdef TEXSWIZZLE_WAITING_FOR_RESAVE
		 , m_onLoadSwapRB( false )
#endif
		{
#ifdef GPU_API_DEBUG_PATH
			m_debugPath[0] = 0;
#endif
		}

		GLuint m_texture;
		//ID3DRenderTargetView* m_pRenderTargetView;
		//ID3DDepthStencilView* m_pDepthStencilView;
		//ID3DDepthStencilView* m_pDepthStencilViewReadOnly;
		//ID3DShaderResourceView* m_pShaderResourceView;
		//ID3D11UnorderedAccessView* m_pUnorderedAccessView;
		//ID3DRenderTargetView** m_pRenderTargetViewsArray;
		//ID3DDepthStencilView** m_pDepthStencilViewsArray;

		Uint16 m_RenderTargetViewsArraySize;	/// Uint16 one by one for better alignment
		Uint16 m_DepthTargetViewsArraySize;		//  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		eTextureGroup m_Group;
		TextureDesc m_Desc;
		TextureRef parentTexId;
#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif

#ifdef TEXSWIZZLE_WAITING_FOR_RESAVE
		// TO BE REMOVED AFTER TEXTURE RESAVE
		// Need to get rid of the R/B swap that's done when importing and loading. To make a smooth transition,
		// we'll use a flag until a full texture resave can be done.
		Bool m_onLoadSwapRB;
#endif
	};

	struct SSamplerStateData
	{
		SamplerStateDesc	m_Desc;
		GLuint				m_samplerID;
	};

	struct SBufferData
	{
		SBufferData ()
			: m_lockedBuffer( nullptr )
			, m_lockedSize( 0 )
			, m_lockedOffset( 0 )
		{	  
#ifdef GPU_API_DEBUG_PATH
			m_debugPath[0] = 0;
#endif
		}

		BufferDesc	m_Desc;
		GLuint		m_buffer;

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
			//: m_pQuery( nullptr )
		{}
		//ID3D11Query *m_pQuery;
	};

	struct SShaderData
	{
		SShaderData ()
			: m_type( VertexShader )
			, m_shader( 0 )
			//, m_byteCode( nullptr )
		{}

		eShaderType m_type;
		GLuint m_shader;
#ifdef GPU_API_DEBUG_PATH
		char m_debugPath[ 256 ];
#endif
	};


	struct SVertexLayoutData
	{
		SVertexLayoutData()
		{
		}

		VertexLayoutDesc m_Desc;
	};

	struct SSwapChainData
	{
		SSwapChainData ()
			//: m_swapChain( nullptr )
		{}
		HDC					m_deviceContext;
		HGLRC				m_renderContext;

		TextureRef			m_backBuffer;
		TextureRef			m_depthStencil; 

		Bool				m_fullscreen;
	};

	struct SDynamicTexture
	{
		TextureRef		m_Texture;
		const char*		m_Name;
	};

	struct SDeviceData
	{
		SDeviceData ( Red::Threads::CMutex &mutex )
			 //: m_pDevice( nullptr )
			 : m_Textures( mutex )
			 , m_SamplerStates( mutex )
			 , m_Queries( mutex )
			 , m_Buffers( mutex )
			 , m_Shaders( mutex )
			 , m_VertexLayouts( mutex )
			 , m_SwapChains( mutex )
			 , m_DeviceInitDone( false )
			 , m_DeviceShutDone( false )
			 , m_StateDrawContext( DRAWCONTEXT_Default )
			 , m_StateDrawContextRef( -1 )
			 , m_currentVertexWritePosition( 0 )
			 , m_currentIndexWritePosition( 0 )
			 , m_GlobalVSConstantBuffer( 0 )
			 , m_CustomVSConstantBuffer( 0 )
			 , m_GlobalPSConstantBuffer( 0 )
			 , m_CustomPSConstantBuffer( 0 )
			 //, m_CustomCSConstantBuffer( 0 )
			 , m_currentFrameBuffer( 0 )
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

			//for ( Uint32 ili = 0; ili < BCT_Max * MAX_SHADER_COUNT; ++ili )
			//{
			//	m_InputLayouts[ili] = nullptr;
			//}

			Red::System::MemorySet( &m_DynamicTextures, 0, sizeof(m_DynamicTextures) );
			m_NumDynamicTextures = 0;
		}

		// Device internal data
		//ID3D11Device* m_pDevice;
		//ID3D11DeviceContext* m_pImmediateContext;
		bool m_DeviceInitDone;
		bool m_DeviceShutDone;
		Capabilities m_Caps;
		TextureRef m_BackBuffer;
		TextureRef m_DepthStencil;
		//DXGI_SWAP_CHAIN_DESC m_PresentParams;
		SamplerStateRef m_SamplerStatePresets[SAMPSTATEPRESET_Max];
		TextureRef m_InternalTextures[INTERTEX_Max];
		RenderSettings m_RenderSettings;

		// Resources
		ResourceContainer< STextureData,		16384  >			m_Textures;
		ResourceContainer< SSamplerStateData,	256    >			m_SamplerStates;
		ResourceContainer< SBufferData,			8192   >			m_Buffers;
		ResourceContainer< SQueryData,			2048   >			m_Queries;
		ResourceContainer< SShaderData,			MAX_SHADER_COUNT >	m_Shaders;
		ResourceContainer< SVertexLayoutData,	1024   >			m_VertexLayouts;
		ResourceContainer< SSwapChainData,		32     >			m_SwapChains;


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
		Int32				m_StateDrawContextRef;
		TextureStats		m_TextureStats;
		VertexLayoutRef		m_VertexLayout;
		BufferRef			m_VertexBuffers[8];
		BufferRef			m_IndexBuffer;
		
		ShaderRef			m_VertexShader;
		ShaderRef			m_PixelShader;
		ShaderRef			m_GeometryShader;
		ShaderRef			m_HullShader;
		ShaderRef			m_DomainShader;
		ShaderRef			m_ComputeShader;

		GLuint				m_ProgramPipeline;

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

		// Input layouts
		//ID3D11InputLayout*	m_InputLayouts[ BCT_Max * MAX_SHADER_COUNT ];

		GLuint				m_GlobalVSConstantBuffer;
		GLuint				m_CustomVSConstantBuffer;
		GLuint				m_GlobalPSConstantBuffer;
		GLuint				m_CustomPSConstantBuffer;
		Bool				m_GlobalVSConstantBufferChanged;
		Bool				m_CustomVSConstantBufferChanged;
		Bool				m_GlobalPSConstantBufferChanged;
		Bool				m_CustomPSConstantBufferChanged;

		//ID3D11Buffer*		m_CustomCSConstantBuffer; //< handled is a different way than other constant buffer

		Float				m_VSConstants[256 * 4];
		Float				m_PSConstants[256 * 4];

		GLuint				m_uniformBufferBindPoints[ 16 ];

#ifdef _DEBUG
		Uint32				m_VSConstantsDebugHistogram[256];
		Uint32				m_PSConstantsDebugHistogram[256];
#endif

		SDeviceUsageStats	m_DeviceUsageStats;
		Uint32				m_NumConstantBufferUpdates;
		

		GLuint				m_currentFrameBuffer;
	};

	// ----------------------------------------------------------------------

	SDeviceData&				GetDeviceData();
	//ID3D11Device*				GetDevice();
	//ID3D11DeviceContext*		GetDeviceContext();
	//ID3D11Resource*				GetD3DTextureBase( const TextureRef& ref );
	//ID3D11Texture2D*			GetD3DTexture2D( const TextureRef& ref );
	//ID3D11Texture2D*			GetD3DTextureCube( const TextureRef& ref );
	//ID3D11UnorderedAccessView*	GetD3DUnorderedAccessView( const TextureRef& ref );
	//
	//ID3D11RenderTargetView*		GetD3DRenderTargetView( const TextureRef& ref, int sliceID=-1 ); // dex: added slice support
	//ID3D11DepthStencilView*		GetD3DDepthStencilView( const TextureRef& ref, int sliceID=-1, Bool isReadOnly=false ); // dex: added slice support
	//
	//ID3D11ShaderResourceView*	GetD3DShaderResourceView( const TextureRef& ref );
	const SamplerStateDesc&		GetSamplerStateDesc( const SamplerStateRef& ref );

	//ID3D11InputLayout*			GetInputLayout( const VertexLayoutRef& vertexLayout, const ShaderRef& vertexShader );
	void						ReleaseInputLayouts( const ShaderRef& shader );

	//ID3D11PixelShader*			GetD3DPixelShader( const ShaderRef& ref );

	
	// ----------------------------------------------------------------------

	void InitBackbufferRef( bool assumeRefsPresent );
	void InitSamplerStatePresets( bool assumeRefsPresent );
	void InitInternalTextures( bool assumeRefsPresent );
	void InitVertexDeclarations( bool assumeRefsPresent );
	void InitQueries( bool assumeRefsPresent );

	void ShutBackbufferRef( bool dropRefs );
	void ShutSamplerStatePresets( bool dropRefs );
	void ShutInternalTextures( bool dropRefs );
	void ShutVertexDeclarations( bool dropRefs );
	void ShutQueries( bool dropRefs );

	inline void InitInternalResources( bool assumeRefsPresent )
	{
		InitBackbufferRef( assumeRefsPresent );
		InitSamplerStatePresets( assumeRefsPresent );
		InitInternalTextures( assumeRefsPresent );
		InitVertexDeclarations( assumeRefsPresent );
		//InitQueries( assumeRefsPresent );
	}

	inline void ShutInternalResources( bool dropRefs )
	{
		// HACK DX10 no stereo
		//ReleaseStereoTexture();
		ShutInternalTextures( dropRefs );
		ShutSamplerStatePresets( dropRefs );
		ShutBackbufferRef( dropRefs );
		ShutVertexDeclarations( dropRefs );
		ShutQueries( dropRefs );
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

//inline ID3D11Device* GpuApi::GetDevice()
//{
//	return GetDeviceData().m_pDevice;
//}
//
//inline ID3D11DeviceContext* GpuApi::GetDeviceContext()
//{
//	return GetDeviceData().m_pImmediateContext;
//}
