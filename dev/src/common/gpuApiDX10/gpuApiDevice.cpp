/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redMath/numericalutils.h"
#include "../redSystem/crt.h"
#include "../redThreads/redThreadsThread.h"
#include "../redMath/mathfunctions_fpu.h"

#if defined(USE_NVAPI)

#include "../../../external/nvapi/nvapi.h"

#if defined( RED_PLATFORM_WIN32 )
#pragma comment (lib, "../../../external/nvapi/x86/nvapi.lib")
#elif defined( RED_PLATFORM_WIN64 )
#pragma comment (lib, "../../../external/nvapi/amd64/nvapi64.lib")
#endif

#endif // USE_NVAPI

#if defined(USE_AMDADL)

#include "../../../external/amd_adl_sdk8/include/adl_sdk.h"

#endif

#if defined(USE_AMDAGS)

#include "../../../external/AMDGPUServices_v2_2/AGS Lib/inc/amd_ags.h"

#ifndef _DEBUG
#if defined( RED_PLATFORM_WIN32 )
#pragma comment (lib, "../../../external/AMDGPUServices_v2_2/AGS Lib/lib/Win32/static/amd_ags.lib")
#elif defined( RED_PLATFORM_WIN64 )
#pragma comment (lib, "../../../external/AMDGPUServices_v2_2/AGS Lib/lib/x64/static/amd_ags64.lib")
#endif
#else
#if defined( RED_PLATFORM_WIN32 )
#pragma comment (lib, "../../../external/AMDGPUServices_v2_2/AGS Lib/lib/Win32/static/amd_ags_d.lib")
#elif defined( RED_PLATFORM_WIN64 )
#pragma comment (lib, "../../../external/AMDGPUServices_v2_2/AGS Lib/lib/x64/static/amd_ags64_d.lib")
#endif
#endif

#endif

typedef Red::System::Uint64 (*TGetGPUMemoryFunc)();
extern TGetGPUMemoryFunc			GSystemGPUMemoryStatFunc;

Red::System::Uint64 GetGPUMemoryStat()
{
	return GpuApi::GpuApiMemory::GetInstance()->GetMetricsCollector().GetTotalBytesAllocated();
}

namespace GpuApi
{
	namespace Helper
	{
		template<typename TType>
		class ReleaseGuard
		{
		public:
			ReleaseGuard( TType* ptr ) : m_ptr( ptr ) {}
			~ReleaseGuard() { SAFE_RELEASE( m_ptr ); }

		private:
			TType* m_ptr;
		};
	}

	void AddRef( const SwapChainRef &swapChain )
	{
		GPUAPI_ASSERT( GetDeviceData().m_SwapChains.IsInUse(swapChain) );
		GetDeviceData().m_SwapChains.IncRefCount( swapChain );
	}

	Int32 Release( const SwapChainRef &swapChain )
	{
		GPUAPI_ASSERT( swapChain );

		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_SwapChains.IsInUse(swapChain) );
		GPUAPI_ASSERT( dd.m_SwapChains.GetRefCount(swapChain) >= 1 );
		Int32 refCount = dd.m_SwapChains.DecRefCount( swapChain );
		if ( 0 == refCount )
		{
			SSwapChainData &data = dd.m_SwapChains.Data( swapChain );

			// Release resources
			GPUAPI_ASSERT( nullptr != data.m_swapChain );

			if ( data.m_backBuffer )
			{
				SafeRelease( data.m_backBuffer );
			}

			if ( data.m_depthStencil )
			{
				SafeRelease( data.m_depthStencil );
			}

			if ( data.m_swapChain )
			{
				ULONG refcount = data.m_swapChain->Release();
				RED_UNUSED( refcount );
				//GPUAPI_ASSERT( refcount == 0, TXT( "swap chain leak" ) );
				// the backbuffer will be released when all the rendertargets are unset
				data.m_swapChain = nullptr;
			}

			// Destroy shit
			dd.m_SwapChains.Destroy( swapChain );
		}
		return refCount;
	}

	// ----------------------------------------------------------------------

	const eTextureFormat BACKBUFFER_FORMAT = TEXFMT_R8G8B8X8;

	// ----------------------------------------------------------------------

	Red::Threads::CMutex	g_ResourceMutex;
	SDeviceData*			g_DeviceData;

	// ----------------------------------------------------------------------

	Bool InitEnv()
	{
		GSystemGPUMemoryStatFunc = &GetGPUMemoryStat;

		const MemSize c_deviceDataSize = sizeof( SDeviceData );
		void* deviceDataMemory = GPU_API_ALLOCATE( GpuMemoryPool_Device, MC_Device, c_deviceDataSize, 16 );
		if( !deviceDataMemory )
		{
			GPUAPI_FATAL( "Device data pool is not big enough to hold SDeviceData. Fatal error!" );
			return false;
		}

		g_DeviceData = new (deviceDataMemory) SDeviceData( g_ResourceMutex );

		return true;
	}

	void CreateConstantBuffers();
	Bool InitDevice( Uint32 width, Uint32 height, Bool fullscreen, Bool vsync )
	{
		RED_UNUSED(vsync);
		RED_UNUSED(fullscreen);
		RED_UNUSED(height);
		RED_UNUSED(width);

		if ( g_DeviceData->m_DeviceInitDone )
		{
			// It's possible to init device only once..
			return !g_DeviceData->m_DeviceShutDone;
		}

		GPUAPI_ASSERT( nullptr == g_DeviceData->m_pDevice );

		SDeviceData &dd = GetDeviceData();

		// Setup flags
		Uint32 deviceFlags = 0;

#ifdef _DEBUG
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;				// Save FPU flags
#endif
		// Try to initialize hardware device
#if defined( RED_PLATFORM_WINPC )
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		D3D_FEATURE_LEVEL featureLevel;
		HRESULT hRet = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context );
		if ( FAILED( hRet ) || !device )
		{
			GPUAPI_LOG_WARNING( TXT( "Unable to create Direct3D 11 device" ) );
			return false;
		}

		HRESULT res = context->QueryInterface( __uuidof(ID3DUserDefinedAnnotation) , (void**) &dd.m_pPerformanceInterface );
		if ( FAILED(res) )
		{
			GPUAPI_LOG_WARNING( TXT("Can't query ID3DUserDefinedAnnotation interface, profile blocks won't work") );
			dd.m_pPerformanceInterface = nullptr;
		}

#if defined( _DEBUG )
		ID3D11Debug* debugInterface = nullptr;
		if( SUCCEEDED( device->QueryInterface( __uuidof(ID3D11Debug), (void**)&debugInterface ) ) )
		{
			ID3D11InfoQueue* infoQueue = nullptr;
			if( SUCCEEDED( debugInterface->QueryInterface( __uuidof(ID3D11InfoQueue), (void**)&infoQueue ) ) )
			{
				// Removing safe stuff
				D3D11_MESSAGE_ID denied [] =
				{
					D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL,
					D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
				};
				D3D11_INFO_QUEUE_FILTER filter;
				Red::System::MemorySet( &filter, 0, sizeof(filter) );
				filter.DenyList.NumIDs = _countof(denied);
				filter.DenyList.pIDList = denied;
				infoQueue->AddStorageFilterEntries( &filter );

				infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
				infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
				infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			}

			SAFE_RELEASE( infoQueue );
		}

		SAFE_RELEASE( debugInterface );
#endif

		IDXGIDevice1* dxgiDevice = nullptr;
		HRESULT result = device->QueryInterface( __uuidof(IDXGIDevice1), (void**)&dxgiDevice );
		Helper::ReleaseGuard<IDXGIDevice1> guardDxgiDevice( dxgiDevice );

		IDXGIAdapter* adapter = nullptr;
		result = dxgiDevice->GetAdapter( &adapter );
		Helper::ReleaseGuard<IDXGIAdapter> guardAdapter( adapter );
		
		GPUVendorApiInterface* vendorInterface = nullptr;

		if (adapter != nullptr) //this is for Adam :)
		{
			//get vendor ID
			DXGI_ADAPTER_DESC adapterDesc;
			adapter->GetDesc( &adapterDesc );

			Int32 vendorId = adapterDesc.VendorId;
#if defined( USE_NVAPI )
			if (vendorInterface == nullptr && vendorId == VENDOR_ID_NVIDIA )
			{
				vendorInterface = new NvApi();
				if (!vendorInterface->Initialize())
				{
					delete vendorInterface;
					vendorInterface = nullptr;
				}
			}
#endif

#if defined( USE_AMDADL ) || defined( USE_AMDAGS )
			if (vendorInterface == nullptr && vendorId == VENDOR_ID_AMD)
			{
				vendorInterface = new AMDApi();
				if (!vendorInterface->Initialize())
				{
					delete vendorInterface;
					vendorInterface = nullptr;
				}
			}
#endif
		}

		if (vendorInterface != nullptr) 
		{
			dd.m_GPUVendorInterface = vendorInterface;
		}

#elif defined( RED_PLATFORM_DURANGO )

#if defined( RED_PROFILE_BUILD )
		deviceFlags |= D3D11_CREATE_DEVICE_INSTRUMENTED;
#elif !defined( RED_FINAL_BUILD )
		deviceFlags |= D3D11_CREATE_DEVICE_INSTRUMENTED | D3D11_CREATE_DEVICE_VALIDATED;
#endif

		//deviceFlags |= D3D11_CREATE_DEVICE_VALIDATED;

		D3D11X_CREATE_DEVICE_PARAMETERS deviceParams;
		Red::System::MemorySet( &deviceParams, 0, sizeof(deviceParams) );
		deviceParams.Flags = deviceFlags;
		deviceParams.Version = D3D11_SDK_VERSION;
		deviceParams.DeferredDeletionThreadAffinityMask = 0x3C;

		ID3D11DeviceX* device;
		ID3D11DeviceContextX* context;

		HRESULT hRet = D3D11XCreateDeviceX( &deviceParams, &device, &context );
		if ( FAILED( hRet ) || !device )
		{
			GPUAPI_LOG_WARNING( TXT("Unable to create Direct3D 11 device: %p"), hRet );
			return false;
		}

		Uint32 constantBufferMemSize = 8 * 1024 * 1024;		// shared between 2 frames
		//void* constantBufferMem = VirtualAlloc(nullptr, constantBufferMemSize, MEM_GRAPHICS | MEM_LARGE_PAGES | MEM_TITLE | MEM_COMMIT, PAGE_GPU_COHERENT | PAGE_READWRITE | PAGE_GPU_READONLY);
		void* constantBufferMem = GPU_API_ALLOCATE( GpuMemoryPool_ConstantBuffers, MC_ConstantBuffer, constantBufferMemSize, 64 * 1024 );
		if( !constantBufferMem )
		{
			GPUAPI_ERROR( TXT("Cannot allocate ConstantBuffer ring buffer memory") );
			return false;
		}
		dd.m_constantBufferMem.Init( constantBufferMem, constantBufferMemSize);

		D3D11_BUFFER_DESC placementCBDesc = {};
		placementCBDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		placementCBDesc.ByteWidth = 64 * 1024;
		placementCBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		placementCBDesc.Usage = D3D11_USAGE_DEFAULT;

		device->CreatePlacementBuffer(&placementCBDesc, nullptr, &dd.m_PlacementConstantBuffer);

		// Create DMA engine context
		D3D11_DMA_ENGINE_CONTEXT_DESC dmaDesc;
		ZeroMemory( &dmaDesc, sizeof(dmaDesc) );
		dmaDesc.CreateFlags = D3D11_DMA_ENGINE_CONTEXT_CREATE_SDMA_1;
		dmaDesc.RingBufferSizeBytes = 64 * 1024;   // NOTE: Currently if you overflow the ring buffer you'll hang the GPU
		device->CreateDmaEngineContext( &dmaDesc, &(dd.m_dmaContext1) );

		dmaDesc.CreateFlags = D3D11_DMA_ENGINE_CONTEXT_CREATE_SDMA_2;
		device->CreateDmaEngineContext( &dmaDesc, &(dd.m_dmaContext2) );

		D3D11_COMPUTE_CONTEXT_DESC computeDesc;
		computeDesc.CreateFlags = 0;
		computeDesc.RingBufferSizeBytes = 0;
		computeDesc.PipeIndex = 0;
		computeDesc.QueueIndex = 0;
		computeDesc.InitialPriority = D3D11X_COMPUTE_CONTEXT_PRIORITY_LOW;
		computeDesc.SegmentSizeBytes = 0;
		computeDesc.ScratchMemorySizeBytes = 0;
		device->CreateComputeContextX( &computeDesc, &dd.m_computeContext );
#endif

		// Finalize
		dd.m_pDevice = device;
		dd.m_pImmediateContext = context;
		GPUAPI_ASSERT( dd.m_Textures.IsEmpty() );
		GPUAPI_ASSERT( dd.m_SamplerStates.IsEmpty() );
		GPUAPI_ASSERT( dd.m_Buffers.IsEmpty() );
		GPUAPI_ASSERT( dd.m_Queries.IsEmpty() );

		// Create presets and internal resources
		InitInternalResources( false );

		// Create constant buffers
		CreateConstantBuffers();

		// Set implicit and explicit states
		ForceSetImplicitD3DStates();
		ForceSetExplicitD3DStates();

		// Finish :)
		g_DeviceData->m_DeviceInitDone = true;

		return true;
	}

	Bool IsInit()
	{
		return nullptr != g_DeviceData->m_pDevice;
	}

	eDeviceCooperativeLevel TestDeviceCooperativeLevel()
	{
		// HACK DX10 testing device cooperative level is different now
		//SDeviceData &dd = GetDeviceData();
		//HRESULT hRet = dd.m_swapChain->Present(0, DXGI_PRESENT_TEST);
		return DEVICECOOPLVL_Operational;// MapCooperativeLevel( hRet );
	}

	Bool ResetDevice()
	{
		// Restore state to default
		{
			//GPUAPI_ASSERT( dd.m_BackBuffer );
			//RenderTargetSetup rtSetup;
			//rtSetup.SetColorTarget( 0, dd.m_BackBuffer );
			//rtSetup.SetViewportFromTarget( dd.m_BackBuffer );
			//SetupRenderTargets( rtSetup );

			BindTextures( 0, MAX_PS_SAMPLERS, nullptr, PixelShader );
			BindTextures( 0, MAX_VS_SAMPLERS, nullptr, VertexShader );
			SetSamplerStateCommon( 0, MAX_PS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, PixelShader );
			SetSamplerStateCommon( 0, MAX_VS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, VertexShader );
			ID3D11RenderTargetView* nullRTV = nullptr;
			GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);
			GetDeviceContext()->VSSetShader( nullptr, nullptr, 0 );
			GetDeviceContext()->PSSetShader( nullptr, nullptr, 0 );

			BindNullVertexBuffers();
			BindIndexBuffer( BufferRef::Null() );
		}

		// Release some internal references

		ShutInternalResources( false );

		// Reset device

		//D3DPRESENT_PARAMETERS params;
		//Red::System::MemoryCopy( &params, &g_DeviceData->m_PresentParams, sizeof(D3DPRESENT_PARAMETERS) );
		////GPUAPI_ASSERT( DEVICECOOPLVL_NotReset == TestDeviceCooperativeLevel() );
		//if ( !SUCCEEDED( GetDevice()->Reset( &params ) ) )
		//{
		//	return false;
		//}

		// Reset succeeded, so rebuild internal references

		InitInternalResources( true );

		// Set implicit states

		ForceSetImplicitD3DStates();

		// Set explicit states

		ForceSetExplicitD3DStates();

		return true;
	}

	void ShutDevice()
	{
		SDeviceData &dd = GetDeviceData();

		if ( !dd.m_pDevice )
		{
			return;
		}

		GPUAPI_ASSERT(  dd.m_DeviceInitDone );
		GPUAPI_ASSERT( !dd.m_DeviceShutDone );

		// Restore state to default
		// TODO clean this up
		{
			GPUAPI_ASSERT( dd.m_BackBuffer );
			//RenderTargetSetup rtSetup;
			//rtSetup.SetColorTarget( 0, dd.m_BackBuffer );
			//rtSetup.SetViewportFromTarget( dd.m_BackBuffer );
			//SetupRenderTargets( rtSetup );

			ID3D11RenderTargetView* rts = nullptr;
			ID3D11DepthStencilView* dss = nullptr;

			GetDeviceContext()->OMSetRenderTargets(1, &rts, dss);

			BindTextures( 0, MAX_PS_SAMPLERS, nullptr, PixelShader );
			BindTextures( 0, MAX_VS_SAMPLERS, nullptr, VertexShader );
			//SetSamplerStateCommon( 0, MAX_PS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, PixelShader );
			//SetSamplerStateCommon( 0, MAX_VS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, VertexShader );
			ID3D11SamplerState* stateObject = nullptr;
			for (int i =0; i<8; ++i)
			{
				GetDeviceContext()->PSSetSamplers( i, 1, &stateObject );
				GetDeviceContext()->VSSetSamplers( i, 1, &stateObject );
				GetDeviceContext()->DSSetSamplers( i, 1, &stateObject );
				GetDeviceContext()->HSSetSamplers( i, 1, &stateObject );
				GetDeviceContext()->GSSetSamplers( i, 1, &stateObject );
			}

			BindNullVertexBuffers();
			BindIndexBuffer( BufferRef::Null() );

			GpuApi::InvalidateSamplerStates();
			GpuApi::SetCustomDrawContext(GpuApi::DSSM_Max, GpuApi::RASTERIZERMODE_Max, GpuApi::BLENDMODE_Max);
			GetDeviceContext()->OMSetDepthStencilState( nullptr, 0 );
			GetDeviceContext()->OMSetBlendState( nullptr, nullptr, 0xFFFFFFFF );
			GetDeviceContext()->RSSetState( nullptr );

#ifndef RED_PLATFORM_DURANGO
			ULONG refCount = 0;
			refCount = dd.m_FrequentVSConstantBuffer->Release();
			refCount = dd.m_CustomVSConstantBuffer->Release();
			refCount = dd.m_FrequentPSConstantBuffer->Release();
			refCount = dd.m_CustomPSConstantBuffer->Release();
			refCount = dd.m_CustomCSConstantBuffer->Release();
#endif // !1			
		}

		// Remove shadow state references
		dd.m_StateRenderTargetSetup.ChangeAllRefCounts( false );
		dd.m_pImmediateContext->ClearState();
		dd.m_pImmediateContext->Flush();
		// Shut internal resources
		ShutInternalResources( true );

#if WAITING_FOR_DEX_TO_FIX_GLOBAL_TEXTURES
		// Remove shit so that release ASSERTION would not fire up.
		dd.m_Textures.DestroyAll();
		dd.m_SamplerStates.DestroyAll();
		dd.m_Buffers.DestroyAll();
		dd.m_Queries.DestroyAll();
#endif

		dd.m_StateRenderStateCache.Clear();
		dd.m_pImmediateContext->ClearState();
		dd.m_pImmediateContext->Flush();
		ULONG contextRefCount = dd.m_pImmediateContext->Release();
		RED_UNUSED( contextRefCount );
		dd.m_pImmediateContext = nullptr;

#if defined( _DEBUG ) && defined( RED_PLATFORM_WINPC )
		ID3D11Debug* debugInterface = nullptr;
		if( SUCCEEDED( dd.m_pDevice->QueryInterface( __uuidof(ID3D11Debug), (void**)&debugInterface ) ) )
		{
			debugInterface->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
		}
		SAFE_RELEASE( debugInterface );

		typedef HRESULT (WINAPI *GetDebugInterfaceProc)(REFIID riid, void **ppDebug);

		HMODULE dxgidebugmodule = GetModuleHandle( TXT("Dxgidebug.dll") ); // doesn't increment refcount
		if ( dxgidebugmodule != nullptr )
		{
			GetDebugInterfaceProc getDebugInterfaceProcedure = (GetDebugInterfaceProc) GetProcAddress( dxgidebugmodule, "DXGIGetDebugInterface" );
			if ( getDebugInterfaceProcedure != nullptr )
			{
				IDXGIDebug* debugDxgiInterface;
				getDebugInterfaceProcedure( __uuidof(IDXGIDebug), (void**)&debugDxgiInterface );
				if (debugDxgiInterface)
				{
					debugDxgiInterface->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL );
				}
			}
		}

#endif

		// Remove device
		ULONG deviceRefCount = dd.m_pDevice->Release();
		RED_UNUSED( deviceRefCount );
		dd.m_pDevice = nullptr;

		// Safe info that we already had device init/shut
		dd.m_DeviceShutDone = true;

		//
		GPUAPI_ASSERT( !IsInit() );
	}

	void SuspendDevice()
	{
#ifdef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();
		dd.m_pImmediateContext->Suspend( 0 );
#endif
	}

	void ResumeDevice()
	{
#ifdef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();
		dd.m_pImmediateContext->Resume();
#endif
	}

	const Capabilities& GetCapabilities()
	{
		return GetDeviceData().m_Caps;
	}

	void SetRenderSettings( const RenderSettings &newSettings )
	{
		SDeviceData &dd = GetDeviceData();

		const Bool invalidateSamplerStatePresets = dd.m_RenderSettings.SamplersChanged( newSettings );

		// Copy
		dd.m_RenderSettings = newSettings;

		// Validate
		dd.m_RenderSettings.Validate();

		// Apply some immediate states
		dd.m_StateRenderStateCache.SetWireframe( newSettings.wireframe );

		if ( invalidateSamplerStatePresets )
		{
			ResetSamplerStates();
		}

	}

	void SetRenderSettingsWireframe( Bool enable )
	{
		if ( enable != GetRenderSettings().wireframe )
		{
			RenderSettings sett = GetRenderSettings();
			sett.wireframe = enable;
			SetRenderSettings( sett );
		}
	}

	const RenderSettings& GetRenderSettings()
	{
		return GetDeviceData().m_RenderSettings;
	}

	void InitBackbufferRef( Bool )
	{
	}

	void ShutBackbufferRef( Bool )
	{
	}

	SwapChainRef CreateSwapChainWithBackBuffer( const SwapChainDesc& swapChainDesc )
	{
		SDeviceData &dd = GetDeviceData();
#if defined( RED_PLATFORM_WINPC )
		IDXGIDevice * giDevice;
		HRESULT dxgiHRes = GetDevice()->QueryInterface(__uuidof(IDXGIDevice), (void **)&giDevice);

		IDXGIAdapter * giAdapter;
		dxgiHRes = giDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&giAdapter);

		IDXGIFactory * giFactory;
		dxgiHRes = giAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&giFactory);

		SAFE_RELEASE( giDevice );
		SAFE_RELEASE( giAdapter );

		//IDXGIFactory * giFactory;
		//HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&giFactory) );

		DXGI_SWAP_CHAIN_DESC scDesc;
		Red::System::MemorySet( &scDesc, 0, sizeof(scDesc) );
		scDesc.BufferCount = 1;
		scDesc.BufferDesc.Width = swapChainDesc.width;
		scDesc.BufferDesc.Height = swapChainDesc.height;
		scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.BufferDesc.RefreshRate.Numerator = 0;
		scDesc.BufferDesc.RefreshRate.Denominator = 1;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.OutputWindow = *(static_cast< HWND*>(swapChainDesc.windowHandle));
		scDesc.SampleDesc.Count = 1;
		scDesc.SampleDesc.Quality = 0;
		scDesc.Windowed = !swapChainDesc.fullscreen;
		scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		IDXGISwapChain* swapChain;

		HRESULT swapchainHR = giFactory->CreateSwapChain( GetDevice(), &scDesc, &swapChain);
		if( FAILED( swapchainHR ) )
		{
			GPUAPI_HALT( "Failed to create swapchain. Error code: %d", swapchainHR );
			return SwapChainRef::Null();
		}

		giFactory->MakeWindowAssociation( scDesc.OutputWindow, DXGI_MWA_NO_ALT_ENTER );

		SAFE_RELEASE( giFactory );

		// Create
		SwapChainRef swapChainRef = SwapChainRef( dd.m_SwapChains.Create( 1 ) );
		if (swapChainRef.isNull())
		{
			GPUAPI_HALT("Too many swapchains");
			swapChain->Release();
			return SwapChainRef::Null();
		}
#ifdef GPU_API_DEBUG_PATH
		swapChain->SetPrivateData( WKPDID_D3DDebugObjectName, 7, "swapChain" );
#endif
		SSwapChainData &scd		= dd.m_SwapChains.Data( swapChainRef );
		scd.m_swapChain			= swapChain;
		scd.m_fullscreen		= swapChainDesc.fullscreen;
#elif defined( RED_PLATFORM_DURANGO )

		// First, retrieve the underlying DXGI Device from the D3D Device
		IDXGIDevice1* pdxgiDevice;
		GetDevice()->QueryInterface(__uuidof( pdxgiDevice ), reinterpret_cast< void** >( &pdxgiDevice ) );

		IDXGIAdapter* pdxgiAdapter;
		pdxgiDevice->GetAdapter( &pdxgiAdapter );

		IDXGIFactory2* pdxgiFactory;
		pdxgiAdapter->GetParent( __uuidof( pdxgiFactory ), reinterpret_cast< void** >( &pdxgiFactory ) );

		SAFE_RELEASE( pdxgiDevice );
		SAFE_RELEASE( pdxgiAdapter );

		// Create
		SwapChainRef swapChainRef = SwapChainRef( dd.m_SwapChains.Create( 1 ) );
		if (swapChainRef.isNull())
		{
			GPUAPI_HALT("Too many swapchains");
			return SwapChainRef::Null();
		}

		SSwapChainData &scd		= dd.m_SwapChains.Data( swapChainRef );

		if ( swapChainDesc.overlay )
		{
			IDXGISwapChain1* swapChain = nullptr;

			DXGI_SWAP_CHAIN_DESC scDescFront = { 0 };
			scDescFront.BufferDesc.Width = swapChainDesc.width;
			scDescFront.BufferDesc.Height = swapChainDesc.height;
			scDescFront.BufferDesc.RefreshRate.Numerator = 60;
			scDescFront.BufferDesc.RefreshRate.Denominator = 1;
			scDescFront.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			scDescFront.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			scDescFront.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
			scDescFront.SampleDesc.Count = 1;
			scDescFront.SampleDesc.Quality = 0;
			scDescFront.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scDescFront.BufferCount = 2;
			scDescFront.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			scDescFront.Flags = DXGIX_SWAP_CHAIN_MATCH_OTHER_CONSOLES;
			pdxgiFactory->CreateSwapChain( GetDevice(), &scDescFront, (IDXGISwapChain**)&swapChain );

			scd.m_swapChain = swapChain;

			// overlay backbuffers need depthstencils
			TextureDesc textureDesc;
			textureDesc.type		= GpuApi::TEXTYPE_2D;
			textureDesc.width		= swapChainDesc.width;
			textureDesc.height		= swapChainDesc.height;
			textureDesc.initLevels = 1;
			textureDesc.msaaLevel = 0;
			textureDesc.format		= GpuApi::TEXFMT_D24S8;

			// the esram related things will not change anything on other platforms than Xbox One
			textureDesc.usage		= GpuApi::TEXUSAGE_DepthStencil | GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_ESRAMResident;
			textureDesc.esramOffset = 0;
			textureDesc.esramSize = 0;
			scd.m_depthStencil	= CreateTexture( textureDesc, TEXG_System );
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC1 scDesc;
			Red::System::MemorySet( &scDesc, 0, sizeof(scDesc) );
			scDesc.Width = swapChainDesc.width;
			scDesc.Height = swapChainDesc.height;
			scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			scDesc.Stereo = false;
			scDesc.SampleDesc.Count = 1;
			scDesc.SampleDesc.Quality = 0;
			scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scDesc.BufferCount = 2;
			scDesc.Scaling = DXGI_SCALING_STRETCH;
			scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

			IDXGISwapChain1* swapChain;

			pdxgiFactory->CreateSwapChainForCoreWindow( GetDevice(), nullptr, &scDesc, NULL, (IDXGISwapChain1**)&swapChain );

			scd.m_swapChain = swapChain;
		}

		SAFE_RELEASE( pdxgiFactory );

		scd.m_fullscreen		= swapChainDesc.fullscreen;
#endif
		scd.m_backBuffer	= TextureRef( dd.m_Textures.Create( 1 ) );

		// Init desc
		STextureData &td		= dd.m_Textures.Data( scd.m_backBuffer );
		td.m_Desc.type			= TEXTYPE_2D;
		td.m_Desc.format		= GpuApi::BACKBUFFER_FORMAT;
		td.m_Desc.usage			= TEXUSAGE_BackBuffer;
		td.m_Desc.width			= swapChainDesc.width;
		td.m_Desc.height		= swapChainDesc.height;
		td.m_Desc.initLevels	= 1;

		// Create a render target view
		ID3D11Texture2D *pBackBuffer;
		ID3D11RenderTargetView* renderTargetView;

		// this function will add a reference to the swapChain
		HRESULT hr = scd.m_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
		if( FAILED( hr ) )
		{
			GPUAPI_HALT( "Failed to get backbuffer. Error code: %d", hr );
		}

		hr = GetDevice()->CreateRenderTargetView( pBackBuffer, nullptr, &renderTargetView );
		if( FAILED( hr ) )
		{
			GPUAPI_HALT( "Failed to create rendertarget for backbuffer. Error code: %d", hr );
		}
#ifdef GPU_API_DEBUG_PATH
		else
		{
			renderTargetView->SetPrivateData( WKPDID_D3DDebugObjectName, 7, "backBuf" );
		}
#endif

		td.m_pTexture = pBackBuffer;
		td.m_pRenderTargetView = renderTargetView;

		return swapChainRef;
	}

	void SetBackBufferFromSwapChain( const SwapChainRef& swapChain )
	{
		SDeviceData &dd = GetDeviceData();
		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
		dd.m_BackBuffer = scd.m_backBuffer;
		dd.m_DepthStencil = scd.m_depthStencil;
	}

	Bool GetFullscreenState( const SwapChainRef& swapChain )
	{
	#ifndef RED_PLATFORM_DURANGO
		SDeviceData &dd = GetDeviceData();
		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
		BOOL result;
		scd.m_swapChain->GetFullscreenState( &result, nullptr );
		return result != 0;
	#else
		return true;
	#endif
	}

	void ResizeBackbuffer( Uint32 width, Uint32 height, const SwapChainRef& swapChain )
	{
		SDeviceData &dd = GetDeviceData();
		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
		STextureData &td = dd.m_Textures.Data( scd.m_backBuffer );
		ID3D11RenderTargetView* nullRT = nullptr;
		GetDeviceContext()->OMSetRenderTargets( 1, &nullRT, nullptr );

		td.m_pTexture->AddRef();
		ULONG texRefCount = td.m_pTexture->Release();
		RED_UNUSED(texRefCount);
		td.m_pRenderTargetView->AddRef();
		ULONG rtvRefCount = td.m_pRenderTargetView->Release();
		RED_UNUSED(rtvRefCount);
		SAFE_RELEASE( td.m_pRenderTargetView );
		SAFE_RELEASE( td.m_pTexture );

		if ( width != 0 && height != 0 )
		{
			HRESULT res = scd.m_swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
			if (res != S_OK)
			{
				GPUAPI_HALT( "Failed to resize backbuffer. Error code: %d", res );
			}

			if( scd.m_fullscreen )
			{
				DXGI_SWAP_CHAIN_DESC dsc;
				scd.m_swapChain->GetDesc(&dsc);
				dsc.BufferDesc.RefreshRate.Numerator = 0;
				dsc.BufferDesc.RefreshRate.Denominator = 0;
				res = scd.m_swapChain->ResizeTarget(&dsc.BufferDesc);
				if (res != S_OK)
				{
					GPUAPI_HALT( "Failed to resize target. Error code: %d", res );
				}
			}
		}

		td.m_Desc.width = width;
		td.m_Desc.height = height;

		// Create a render target view
		ID3D11Texture2D *pBackBuffer;
		ID3D11RenderTargetView* renderTargetView;

		HRESULT hr = scd.m_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
		if( FAILED( hr ) )
		{
			GPUAPI_HALT( "Failed to get backbuffer. Error code: %d", hr );
		}

		hr = GetDevice()->CreateRenderTargetView( pBackBuffer, nullptr, &renderTargetView );
		if( FAILED( hr ) )
		{
			GPUAPI_HALT( "Failed to create rendertarget for backbuffer. Error code: %d", hr );
		}
#ifdef GPU_API_DEBUG_PATH
		else
		{
			renderTargetView->SetPrivateData( WKPDID_D3DDebugObjectName, 7, "backBuf" );
		}
#endif
#if MICROSOFT_ATG_DYNAMIC_SCALING
		GPUAPI_ASSERT( !td.m_pDynamicScalingData, TXT("Error, wrong pointers being used"));
#endif
		td.m_pTexture = pBackBuffer;
		td.m_pRenderTargetView = renderTargetView;
	}

	IDXGIAdapter* GetAdapter()		// Internal function
	{
		IDXGIAdapter* result = nullptr;

		if( g_DeviceData != nullptr )
		{
			ID3D11Device* device = GetDevice();

			if( device != nullptr )
			{
				IDXGIDevice * dxgiDevice = nullptr;
				device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);

				dxgiDevice->GetAdapter( &result );

				SAFE_RELEASE( dxgiDevice );
			}
		}

		return result;
	}

	IDXGIOutput* GetOutputMonitor( Int32 outputMonitorIndex )			// Internal function
	{
		IDXGIOutput* result = nullptr;
		IDXGIAdapter* adapter = GetAdapter();

		if( adapter != nullptr && outputMonitorIndex >= 0 )		// outputMonitorIndex less than zero means leave monitor choice up to system
		{
			HRESULT hr = adapter->EnumOutputs( outputMonitorIndex, &result );
			if( hr == DXGI_ERROR_NOT_FOUND )
			{
				result = nullptr;
			}
		}

		SAFE_RELEASE(adapter);

		return result;
	}

	Bool ToggleFullscreenWithOutput( const SwapChainRef& swapChain, Bool fullscreen, IDXGIOutput* outputMonitor )		// Internal function
	{
		SDeviceData &dd = GetDeviceData();
		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );

		HRESULT res = scd.m_swapChain->SetFullscreenState( fullscreen, outputMonitor );
		if (res == S_OK)
		{
			scd.m_fullscreen = fullscreen;
			return true;
		}
		else
		{
			return false;
		}
	}

	Bool ToggleFullscreen( const SwapChainRef& swapChain, Bool fullscreen )
	{
		return ToggleFullscreenWithOutput( swapChain, fullscreen, nullptr );
	}

	Bool ToggleFullscreen( const SwapChainRef& swapChain, Bool fullscreen, Int32 outputMonitorIndex )
	{
	#ifndef RED_PLATFORM_DURANGO
		IDXGIOutput* outputMonitor = GetOutputMonitor( outputMonitorIndex );
		Bool res = ToggleFullscreenWithOutput( swapChain, fullscreen, outputMonitor );
		SAFE_RELEASE(outputMonitor);
		return res;
	#else
		GPUAPI_ASSERT( false, TXT("This function should not be called on XBox One."));
		return false;
	#endif
	}

	Int32 GetMonitorCount()
	{
	#ifndef RED_PLATFORM_DURANGO
		Int32 monitorCount = 0;
		IDXGIAdapter* adapter = GetAdapter();

		if( adapter != nullptr )
		{
			IDXGIOutput* temp;
			for( Int32 i=0; adapter->EnumOutputs( i, &temp ) != DXGI_ERROR_NOT_FOUND; ++i )
			{
				monitorCount++;
				temp->Release();
			}

			SAFE_RELEASE( adapter );
		}
		else
		{
			// lets try to get the outputs without a device

			IDXGIFactory* factory;
			HRESULT result = CreateDXGIFactory( __uuidof( IDXGIFactory ), (void**)&factory );
			Helper::ReleaseGuard<IDXGIFactory> guardFactory( factory );
			if( !FAILED( result ) )
			{
				IDXGIAdapter* adapter;
				IDXGIOutput* output = nullptr;
				if( factory->EnumAdapters( 0, &adapter ) != DXGI_ERROR_NOT_FOUND )
				{
					IDXGIOutput* temp;
					for( Int32 i=0; adapter->EnumOutputs( i, &temp ) != DXGI_ERROR_NOT_FOUND; ++i )
					{
						monitorCount++;
						temp->Release();
					}
					SAFE_RELEASE( adapter );
				}
			}
		}

		return monitorCount;
	#else
		GPUAPI_ASSERT( false, TXT("This function should not be called on XBox One."));
		return 1;
	#endif
	}

	Bool GetMonitorCoordinates( Int32 monitorIndex, Int32& top, Int32& left, Int32& bottom, Int32& right )
	{
#ifndef RED_PLATFORM_DURANGO
		IDXGIOutput* outputMonitor = GetOutputMonitor( monitorIndex );
		if ( outputMonitor )
		{
			DXGI_OUTPUT_DESC desc;
			HRESULT result = outputMonitor->GetDesc( &desc );
			if ( result == S_OK )
			{
				top		= desc.DesktopCoordinates.top;
				bottom	= desc.DesktopCoordinates.bottom;
				left	= desc.DesktopCoordinates.left;
				right	= desc.DesktopCoordinates.right;
				return true;
			}			
		}
		return false;
#else
		GPUAPI_ASSERT( false, TXT("This function should not be called on XBox One."));
		return false;
#endif
	}

	void SetGammaForSwapChain( const SwapChainRef& swapChain, Float gammaValue )
	{
#ifndef RED_PLATFORM_DURANGO
		if( GetFullscreenState( swapChain ) == false )
		{
			return;
		}
#endif

		SDeviceData &dd = GetDeviceData();

		if ( !swapChain.isNull() )
		{
			SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );

			IDXGIOutput* output;

			//We get the IDXGIOutput by retrieving the IDXGIDevice1 interface from the ID3D11Device, from that we get the IDXGIAdapter from the IDXGIDevice1 by calling GetAdapter and finally call EnumOutputs(0, ...) on the IDXGIAdapter.
#ifdef RED_PLATFORM_DURANGO
			IDXGIDevice1* dxgiDevice;
			HRESULT result = GetDevice()->QueryInterface( __uuidof(IDXGIDevice1), (void**)&dxgiDevice );

			IDXGIAdapter* adapter;
			result = dxgiDevice->GetAdapter( &adapter );

			result = adapter->EnumOutputs( 0, &output );
#else
			HRESULT result = scd.m_swapChain->GetContainingOutput( &output );
#endif
			if ( result == S_OK && output != nullptr )
			{
				DXGI_GAMMA_CONTROL_CAPABILITIES gammaCaps;
				output->GetGammaControlCapabilities( &gammaCaps );

				DXGI_GAMMA_CONTROL gammaControl;
				//output->GetGammaControl( &gammaControl );

				gammaControl.Offset.Red = 0.f; gammaControl.Offset.Green = 0.f; gammaControl.Offset.Blue = 0.f;
				gammaControl.Scale.Red = 1.f; gammaControl.Scale.Green = 1.f; gammaControl.Scale.Blue = 1.f;

				Float maxvalue = (Float)(gammaCaps.NumGammaControlPoints-1);
				Uint32 controlPointCount = Red::Math::NumericalUtils::Max( 1026u, gammaCaps.NumGammaControlPoints );
				for (Uint32 i = 0; i < controlPointCount; ++i)
				{
					gammaControl.GammaCurve[i].Red = Red::Math::MPow( (Float)i/maxvalue, gammaValue );
					gammaControl.GammaCurve[i].Green = Red::Math::MPow( (Float)i/maxvalue, gammaValue );
					gammaControl.GammaCurve[i].Blue = Red::Math::MPow( (Float)i/maxvalue, gammaValue );
				}

				output->SetGammaControl( &gammaControl );
			}

#ifdef RED_PLATFORM_DURANGO
			SAFE_RELEASE( dxgiDevice );
			SAFE_RELEASE( adapter );
			SAFE_RELEASE( output );
#else
			SAFE_RELEASE( output );
#endif

		}
	}

	void GetNativeResolution( Uint32 outputIndex, Int32& width, Int32& height )
	{
		width = 1920;
		height = 1080;

#ifndef RED_PLATFORM_DURANGO
		IDXGIFactory* factory;
		HRESULT result = CreateDXGIFactory( __uuidof( IDXGIFactory ), (void**)&factory );
		Helper::ReleaseGuard<IDXGIFactory> guardFactory( factory );
		if( FAILED( result ) )
		{
			return;
		}

		//find the adapter that is feeding the current output
		IDXGIAdapter* adapter;
		IDXGIOutput* output = nullptr;
		if( factory->EnumAdapters( 0, &adapter ) != DXGI_ERROR_NOT_FOUND )
		{
			Helper::ReleaseGuard<IDXGIAdapter> guardAdapter( adapter );
			result = adapter->EnumOutputs( outputIndex, &output );
			if (result != S_OK)
			{
				return;
			}
		}

		// make sure we release it after
		Helper::ReleaseGuard<IDXGIOutput> guardOutput( output );

		// get the size
		if ( output != nullptr )
		{
			DXGI_OUTPUT_DESC outputDesc;
			output->GetDesc( &outputDesc );
			width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
			height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
		}
#endif
	}

	Bool EnumerateDisplayModes( Int32 monitorIndex, Uint32* outNum, DisplayModeDesc** outDescs /*= nullptr*/ )
	{
	#ifndef RED_PLATFORM_DURANGO
		Uint32 properMonitorIndex = (monitorIndex < 0) ? 0 : monitorIndex;

		if( GpuApi::g_DeviceData == nullptr )
			return false;

		IDXGIOutput* output = nullptr;
		// We get the IDXGIOutput by retrieving the IDXGIDevice1 interface from the ID3D11Device,
		// from that we get the IDXGIAdapter from the IDXGIDevice1 by calling GetAdapter and finally call EnumOutputs(0, ...) on the IDXGIAdapter.
		IDXGIDevice1* dxgiDevice = nullptr;
		ID3D11Device* device = GetDevice();

		if( device == nullptr )
			return false;

		HRESULT result = device->QueryInterface( __uuidof(IDXGIDevice1), (void**)&dxgiDevice );
		Helper::ReleaseGuard<IDXGIDevice1> guardDxgiDevice( dxgiDevice );

		IDXGIAdapter* adapter;
		result = dxgiDevice->GetAdapter( &adapter );
		Helper::ReleaseGuard<IDXGIAdapter> guardAdapter( adapter );

		if( result != S_OK )
			return false;

		result = adapter->EnumOutputs( properMonitorIndex, &output );
		Helper::ReleaseGuard<IDXGIOutput> guardOutput( output );

		if( result != S_OK )
			return false;

		if ( output != nullptr )
		{
			UINT modesNum = 0;

			// First - get how many modes are available
			result = output->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, 0, &modesNum, nullptr );
			if( result != S_OK )
				return false;

			(*outNum) = (Uint32)modesNum;

			if( outDescs != nullptr )
			{
				DXGI_MODE_DESC* modesDesc = nullptr;

				// Allocate mode descs
				modesDesc = new DXGI_MODE_DESC[modesNum];

				// Second - get all available modes
				result = output->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, 0, &modesNum, modesDesc );
				if( result != S_OK )
					return false;

				for( Uint32 i=0; i<modesNum; ++i )
				{
					(*outDescs)[i].width = (Int32)modesDesc[i].Width;
					(*outDescs)[i].height = (Int32)modesDesc[i].Height;
					(*outDescs)[i].refreshRate.numerator = (Int32)modesDesc[i].RefreshRate.Numerator;
					(*outDescs)[i].refreshRate.denominator = (Int32)modesDesc[i].RefreshRate.Denominator;
				}

				// Delete allocated mode desctiptions
				delete [] modesDesc;
			}
		}

		return true;
	#else
		GPUAPI_ASSERT( false, TXT("This function should not be called on XBox One."));
		return false;
	#endif
	}

	void* CreateContext()
	{
		ID3D11DeviceContext* context = nullptr;
		GetDevice()->CreateDeferredContext( 0, &context );
		return context;
	}

	void SubmitContext( void* deferredContext )
	{
		ID3D11CommandList* commandList = nullptr;
		((ID3D11DeviceContext*) deferredContext)->FinishCommandList( true, &commandList );
		GetDeviceContext()->ExecuteCommandList( commandList, true );
		ULONG refcount = ((ID3D11DeviceContext*) deferredContext)->Release();
#ifdef NO_GPU_ASSERTS
		RED_UNUSED( refcount );
#endif
		GPUAPI_ASSERT( refcount == 0, TXT( "Context not destroyed" ) );
		commandList->Release();
	}

	void CancelContext( void* deferredContext )
	{
		ULONG refcount = ((ID3D11DeviceContext*) deferredContext)->Release();
#ifdef NO_GPU_ASSERTS
		RED_UNUSED( refcount );
#endif
		GPUAPI_ASSERT( refcount == 0, TXT( "Context not destroyed" ) );
	}

	void* GetCommandListFromContext( void* deferredContext )
	{
		ID3D11CommandList* commandList = nullptr;
		((ID3D11DeviceContext*) deferredContext)->FinishCommandList( true, &commandList );
		return commandList;
	}

	void SubmitCommandList( void* commandList )
	{
		ID3D11CommandList* d3dCommandList = (ID3D11CommandList*)commandList;
		GetDeviceContext()->ExecuteCommandList( d3dCommandList, true );
		d3dCommandList->Release();
	}

	void CancelCommandList( void* commandList )
	{
		ID3D11CommandList* d3dCommandList = (ID3D11CommandList*)commandList;
		d3dCommandList->Release();
	}

	void BeginProfilerBlock( const Char* name )
	{
#if defined( RED_PLATFORM_WINPC )
		SDeviceData &dd = GetDeviceData();
		if ( dd.m_pPerformanceInterface )
		{
			INT res = dd.m_pPerformanceInterface->BeginEvent( name );
			RED_UNUSED(res);
			//GPUAPI_LOG("profileblock begin %d", res);
		}
#elif defined( RED_PLATFORM_DURANGO )
		SDeviceData &dd = GetDeviceData();
		dd.m_pImmediateContext->PIXBeginEvent( name );
#endif
	}

	void EndProfilerBlock()
	{
#if defined( RED_PLATFORM_WINPC )
		SDeviceData &dd = GetDeviceData();
		if ( dd.m_pPerformanceInterface )
		{
			INT res = dd.m_pPerformanceInterface->EndEvent();
			RED_UNUSED(res);
			//GPUAPI_LOG("profileblock end %d", res);
		}
#elif defined( RED_PLATFORM_DURANGO )
		SDeviceData &dd = GetDeviceData();
		dd.m_pImmediateContext->PIXEndEvent();
#endif
	}

	void SetMarker( const Char* name )
	{
#if defined( RED_PLATFORM_WINPC )
		SDeviceData &dd = GetDeviceData();
		if ( dd.m_pPerformanceInterface )
		{
			dd.m_pPerformanceInterface->SetMarker( name );
		}
#elif defined( RED_PLATFORM_DURANGO )
		SDeviceData &dd = GetDeviceData();
		dd.m_pImmediateContext->PIXSetMarker( name );
#endif
	}

#ifndef RED_FINAL_BUILD
	void GetResourceUseStats( SResourceUseStats& stats )
	{
		SDeviceData &dd = GetDeviceData();

		stats.m_usedTextures		= dd.m_Textures.GetUsedCount();
		stats.m_usedBuffers			= dd.m_Buffers.GetUsedCount();
		stats.m_usedSamplerStates	= dd.m_SamplerStates.GetUsedCount();
		stats.m_usedQueries			= dd.m_Queries.GetUsedCount();
		stats.m_usedShaders			= dd.m_Shaders.GetUsedCount();
		stats.m_usedVertexLayouts	= dd.m_VertexLayouts.GetUsedCount();
		stats.m_usedSwapChains		= dd.m_SwapChains.GetUsedCount();

		stats.m_maxTextures			= dd.m_Textures._MaxResCount;
		stats.m_maxBuffers			= dd.m_Buffers._MaxResCount;
		stats.m_maxSamplerStates	= dd.m_SamplerStates._MaxResCount;
		stats.m_maxQueries			= dd.m_Queries._MaxResCount;
		stats.m_maxShaders			= dd.m_Shaders._MaxResCount;
		stats.m_maxVertexLayouts	= dd.m_VertexLayouts._MaxResCount;
		stats.m_maxSwapChains		= dd.m_SwapChains._MaxResCount;
	}
#endif

#if defined( USE_NVAPI )

	struct NvApiImpl
	{
		NV_GET_CURRENT_SLI_STATE m_sliState;
	};

	NvApi::NvApi() 
	: m_isEnabled( false )
	, m_impl(nullptr)
	{
	}

	Bool NvApi::Initialize()
	{
		m_impl = new NvApiImpl;
		NvAPI_Status status = NvAPI_Initialize();
		if (status != NVAPI_OK)
		{
			return false;
		}
		return true;
	}

	NvApi::~NvApi()
	{
		delete m_impl;
		NvAPI_Unload();
	}

	void NvApi::Update()
	{
		m_impl->m_sliState.version = NV_GET_CURRENT_SLI_STATE_VER;

		NvAPI_Status status = NvAPI_D3D_GetCurrentSLIState( GpuApi::Hacks::GetDevice(), &m_impl->m_sliState );
		if ( status != NVAPI_OK )
		{
			//Error code here
			//Requesting a NVIDIA driver update
			//is the prefered action in this case
		}
		else
		{
			m_isEnabled = ( m_impl->m_sliState.maxNumAFRGroups > 1 );
		}
	}

	Uint32 NvApi::GetNumGPUs() const
	{
		return m_impl->m_sliState.numAFRGroups;
	}

	Uint32 NvApi::GetCurrentGPUIndex() const
	{
		return m_impl->m_sliState.currentAFRIndex;
	}

	Bool NvApi::IsFirstRun() const
	{
		return (m_impl->m_sliState.bIsCurAFRGroupNew == 1) ? true : false;
	}

	void NvApi::BeginEarlyPushUAV( const BufferRef& ref )
	{
		if ( ref.isNull() )
		{
			return;
		}
		ID3D11Buffer* buff = GetDeviceData().m_Buffers.Data( ref ).m_pBufferResource;
		NVDX_ObjectHandle handle;
		NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource( GpuApi::Hacks::GetDevice(), buff, &handle );
		if ( status == NVAPI_OK )
		{
			status = NvAPI_D3D_BeginResourceRendering( GpuApi::Hacks::GetDevice(), handle, NVAPI_D3D_RR_FLAG_FORCE_KEEP_CONTENT );
		}
	}

	void NvApi::EndEarlyPushUAV( const BufferRef& ref )
	{
		if (ref.isNull())
		{
			return;
		}
		ID3D11Buffer* buff = GetDeviceData().m_Buffers.Data( ref ).m_pBufferResource;
		NVDX_ObjectHandle handle;
		NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource( GpuApi::Hacks::GetDevice(), buff, &handle );
		if ( status == NVAPI_OK )
		{
			status = NvAPI_D3D_EndResourceRendering( GpuApi::Hacks::GetDevice(), handle, NVAPI_D3D_RR_FLAG_FORCE_KEEP_CONTENT );
		}
	}

	void NvApi::BeginEarlyPushTexture( const TextureRef& ref )
	{
		ID3D11Texture2D* tex = GpuApi::Hacks::GetTexture( ref );
		NVDX_ObjectHandle handle;
		NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource( GpuApi::Hacks::GetDevice(), tex, &handle );
		if ( status == NVAPI_OK )
		{
			status = NvAPI_D3D_BeginResourceRendering( GpuApi::Hacks::GetDevice(), handle, NVAPI_D3D_RR_FLAG_FORCE_KEEP_CONTENT );
		}
	}

	void NvApi::EndEarlyPushTexture( const TextureRef& ref )
	{
		ID3D11Texture2D* tex = GpuApi::Hacks::GetTexture( ref );
		NVDX_ObjectHandle nvapiHandle;
		NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource( GpuApi::Hacks::GetDevice(), tex, &nvapiHandle );
		if ( status == NVAPI_OK )
		{
			status = NvAPI_D3D_EndResourceRendering( GpuApi::Hacks::GetDevice(), nvapiHandle, 0 );
		}
	}

	void NvApi::BeginEarlyPushTextureUAV( const TextureRef& ref )
	{
		if ( ref.isNull() )
		{
			return;
		}
		ID3D11UnorderedAccessView* uav = GetD3DUnorderedAccessView( ref );
		NVDX_ObjectHandle handle;
		NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource( GpuApi::Hacks::GetDevice(), uav, &handle );
		if ( status == NVAPI_OK )
		{
			status = NvAPI_D3D_BeginResourceRendering( GpuApi::Hacks::GetDevice(), handle, NVAPI_D3D_RR_FLAG_FORCE_KEEP_CONTENT );
		}
	}

	void NvApi::EndEarlyPushTextureUAV( const TextureRef& ref )
	{
		if ( ref.isNull() )
		{
			return;
		}
		ID3D11UnorderedAccessView* uav = GetD3DUnorderedAccessView( ref );
		NVDX_ObjectHandle handle;
		NvAPI_Status status = NvAPI_D3D_GetObjectHandleForResource( GpuApi::Hacks::GetDevice(), uav, &handle );
		if ( status == NVAPI_OK )
		{
			status = NvAPI_D3D_EndResourceRendering( GpuApi::Hacks::GetDevice(), handle, NVAPI_D3D_RR_FLAG_FORCE_KEEP_CONTENT );
		}
	}

#endif // USE_NVAPI

#if defined( USE_AMDADL )

	typedef int ( *ADL_MAIN_CONTROL_CREATE )(ADL_MAIN_MALLOC_CALLBACK, int );
	typedef int ( *ADL_MAIN_CONTROL_DESTROY )();
	typedef int ( *ADL_ADAPTER_NUMBEROFADAPTERS_GET ) ( int* );
	typedef int ( *ADL_ADAPTER_ADAPTERINFO_GET ) ( LPAdapterInfo, int );

	// Memory allocation function
	void* __stdcall ADL_Main_Memory_Alloc ( int iSize )
	{
		void* lpBuffer = malloc ( iSize );
		return lpBuffer;
	}

	// Optional Memory de-allocation function
	void __stdcall ADL_Main_Memory_Free ( void** lpBuffer )
	{
		if ( NULL != *lpBuffer )
		{
			free ( *lpBuffer );
			*lpBuffer = NULL;
		}
	}

	class AMDApiImpl
	{
	public:
		AMDApiImpl();
		~AMDApiImpl();

		// implementing GPUVendorApiInterface
		virtual Bool Initialize();

		virtual Uint32 GetNumGPUs() const;
		virtual Uint32 GetCurrentGPUIndex() const;

	private:
		HINSTANCE			m_dll;
		LPAdapterInfo		m_adapterInfo;
		Int32				m_adapterCount;

		ADL_MAIN_CONTROL_CREATE				ADL_Main_Control_Create;
		ADL_MAIN_CONTROL_DESTROY			ADL_Main_Control_Destroy;
		ADL_ADAPTER_NUMBEROFADAPTERS_GET	ADL_Adapter_NumberOfAdapters_Get;
		ADL_ADAPTER_ADAPTERINFO_GET			ADL_Adapter_AdapterInfo_Get;
	};

	AMDApiImpl::AMDApiImpl()
		: m_dll( nullptr )
	{
	}

	Bool AMDApiImpl::Initialize()
	{
		m_dll = LoadLibrary( TXT("atiadlxx.dll") );
		if (m_dll == nullptr)
		{
			return false;
		}

		ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress( m_dll,"ADL_Main_Control_Create" );
		ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY) GetProcAddress(m_dll,"ADL_Main_Control_Destroy");
		ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET) GetProcAddress(m_dll,"ADL_Adapter_NumberOfAdapters_Get");
		ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET) GetProcAddress(m_dll,"ADL_Adapter_AdapterInfo_Get");

		if ( nullptr == ADL_Main_Control_Create || nullptr == ADL_Main_Control_Destroy ||
			nullptr == ADL_Adapter_NumberOfAdapters_Get || nullptr == ADL_Adapter_AdapterInfo_Get )
		{
			FreeLibrary(m_dll);
			m_dll = nullptr;
			return false;
		}

		// Initialize ADL. The second parameter is 1, which means:
		// retrieve adapter information only for adapters that are physically present and enabled in the system
		if ( ADL_OK != ADL_Main_Control_Create (ADL_Main_Memory_Alloc, 1) )
		{
			FreeLibrary(m_dll);
			m_dll = nullptr;
			return false;
		}

		// Obtain the number of adapters for the system
		Int32 maxAdapterCount = 0;
		if ( ADL_OK != ADL_Adapter_NumberOfAdapters_Get ( &maxAdapterCount ) )
		{
			FreeLibrary(m_dll);
			m_dll = nullptr;
			return false;
		}

		if ( 0 < maxAdapterCount )
		{
			m_adapterInfo = (LPAdapterInfo)malloc( sizeof (AdapterInfo) * maxAdapterCount );
			memset( m_adapterInfo,'\0', sizeof (AdapterInfo) * maxAdapterCount );

			// Get the AdapterInfo structure for all adapters in the system
			ADL_Adapter_AdapterInfo_Get(m_adapterInfo, sizeof (AdapterInfo) * maxAdapterCount);
		}

		m_adapterCount = maxAdapterCount;

		return true;
	}

	AMDApiImpl::~AMDApiImpl()
	{
		if ( m_dll != nullptr )
		{
			FreeLibrary(m_dll);
		}
	}

	Uint32 AMDApiImpl::GetNumGPUs() const
	{
		return m_adapterCount;
	}

	Uint32 AMDApiImpl::GetCurrentGPUIndex() const
	{
		return 0;
	}

	AMDApi::AMDApi()
		: m_impl( nullptr )
	{
	}

	AMDApi::~AMDApi()
	{
		if (m_impl)
		{
			delete m_impl;
		}
	}

	Bool AMDApi::Initialize()
	{
		m_impl = new AMDApiImpl();
		if ( !m_impl->Initialize() )
		{
			delete m_impl;
			m_impl = nullptr;
			return false;
		}
		return true;
	}

	void AMDApi::Update()
	{

	}

	Uint32 AMDApi::GetCurrentGPUIndex() const
	{
		return m_impl->GetCurrentGPUIndex();
	}

	Uint32 AMDApi::GetNumGPUs() const
	{
		return m_impl->GetNumGPUs();
	}

	void AMDApi::BeginEarlyPushTexture( const TextureRef& ref )
	{
		// empty
	}

	void AMDApi::EndEarlyPushTexture( const TextureRef& ref )
	{
		// empty
	}
#endif

#if defined( USE_AMDAGS )

	class AMDApiImpl
	{
	};

	AMDApi::AMDApi() 
	: m_impl(nullptr)
	{
	}

	Bool AMDApi::Initialize()
	{
		AGSReturnCode status = AGSInit();
		if (status != AGS_SUCCESS)
		{
			return false;
		}
		return true;
	}

	AMDApi::~AMDApi()
	{
		AGSDeInit();
	}

	void AMDApi::Update()
	{
	}

	Uint32 AMDApi::GetNumGPUs() const
	{
		Int32 displayindex;
		if ( AGS_SUCCESS != AGSGetDefaultDisplayIndex( &displayindex ) )
		{
			return 0;
		}

		Int32 crossfireGPUCount = 1;
		if ( AGS_SUCCESS == AGSCrossfireGetGPUCount( displayindex, &crossfireGPUCount ) )
		{
			return Uint32(crossfireGPUCount);
		}
		else
		{
			return 0;
		}
	}

	Uint32 AMDApi::GetCurrentGPUIndex() const
	{
		return 0;
	}

	void AMDApi::BeginEarlyPushTexture( const TextureRef& ref )
	{
		// empty
	}

	void AMDApi::EndEarlyPushTexture( const TextureRef& ref )
	{
		// empty
	}

#endif // USE_AMDAGS

	namespace Hacks
	{
		ID3D11RenderTargetView* GetTextureRTV( const TextureRef &ref )
		{
			return GetD3DRenderTargetView( ref );
		}

		ID3D11ShaderResourceView* GetTextureSRV( const TextureRef &ref )
		{
			return GetD3DShaderResourceView( ref );
		}

		ID3D11UnorderedAccessView* GetTextureUAV( const TextureRef& ref )
		{
			return GetD3DUnorderedAccessView( ref );
		}

		ID3D11Texture2D* GetTexture( const TextureRef &ref )
		{
			return GetD3DTexture2D( ref );
		}

		ID3D11Buffer* GetBuffer( const BufferRef &ref )
		{
			return GetD3DBuffer( ref );
		}

		ID3D11PixelShader* GetPixelShader( const ShaderRef& ref )
		{
			return GetD3DPixelShader( ref );
		}

		ID3D11ComputeShader* GetComputeShader( const ShaderRef& ref )
		{
			return GetD3DComputeShader( ref );
		}

#ifdef RED_PLATFORM_DURANGO
		ID3D11ComputeContextX* GetComputeContext()
		{
			return GetDeviceData().m_computeContext;
		}

		ID3D11DeviceX* GetDevice()
		{
			return GetDeviceData().m_pDevice;
		}

		ID3D11DeviceContextX* GetDeviceContext()
		{
			return GetDeviceData().m_pImmediateContext;
		}
#else
		ID3D11Device* GetDevice()
		{
			return GetDeviceData().m_pDevice;
		}

		ID3D11DeviceContext* GetDeviceContext()
		{
			return GetDeviceData().m_pImmediateContext;
		}
#endif
	}

	Bool HasMinimumRequiredGPU()
	{
#ifdef RED_PLATFORM_WINPC
		IDXGIFactory* pFactory;
		HRESULT hr = CreateDXGIFactory( __uuidof( IDXGIFactory ), (void**)&pFactory );

		if( FAILED( hr ) )
		{
			return false;
		}

		IDXGIAdapter* pAdapter;

		Bool properAdapterFound = false;		// This is the result of that function
		Uint32 adapterIndex = 0;
		while( pFactory->EnumAdapters( adapterIndex, &pAdapter ) != DXGI_ERROR_NOT_FOUND )
		{
			// Check feature level
			D3D_FEATURE_LEVEL featureLevel;
			HRESULT hRet = D3D11CreateDevice( pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, nullptr, &featureLevel, nullptr );
			pAdapter->Release();

			if( featureLevel >= D3D_FEATURE_LEVEL_11_0 )
			{
				properAdapterFound = true;
				break;
			}

			adapterIndex++;
		}

		pFactory->Release();

		return properAdapterFound;
#else
		return true;
#endif
	}

	Bool GetDeviceId( Uint32& vendorId, Uint32& deviceId )
	{
#ifdef RED_PLATFORM_WINPC
		IDXGIFactory* pFactory;
		HRESULT hr = CreateDXGIFactory( __uuidof( IDXGIFactory ), (void**)&pFactory );

		if( FAILED( hr ) )
		{
			return false;
		}

		IDXGIAdapter* pAdapter;

		Bool properAdapterFound = false;
		Uint32 adapterIndex = 0;
		while( pFactory->EnumAdapters( adapterIndex, &pAdapter ) != DXGI_ERROR_NOT_FOUND )
		{
			DXGI_ADAPTER_DESC adapterDesc;
			pAdapter->GetDesc( &adapterDesc );

			// Check feature level
			ID3D11Device* device = nullptr;
			ID3D11DeviceContext* context = nullptr;
			D3D_FEATURE_LEVEL featureLevel;
			HRESULT hRet = D3D11CreateDevice( pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, nullptr, &featureLevel, nullptr );
			if( featureLevel >= D3D_FEATURE_LEVEL_11_0 )
			{
				IDXGIOutput* output = nullptr;
				pAdapter->EnumOutputs( 0, &output );
				if( output != nullptr )
				{
					properAdapterFound = true;
					output->Release();
				}
			}

			pAdapter->Release();

			if( properAdapterFound == true )
			{
				vendorId = adapterDesc.VendorId;
				deviceId = adapterDesc.DeviceId;
				break;		// We've found the GPU which supports DirectX 11, that's good enough
			}

			adapterIndex++;
		}

		pFactory->Release();

		return true;
#else
		return false;
#endif
	}

	Bool MultiGPU_IsActive()
	{
		return MultiGPU_GetNumGPUs() > 1;
	}

	Uint32 MultiGPU_GetNumGPUs()
	{
	#ifdef RED_PLATFORM_WINPC
		const SDeviceData &dd = GetDeviceData();
		RED_FATAL_ASSERT( !(dd.m_GPUVendorInterface && dd.m_GPUVendorInterface->GetNumGPUs() < 1), "Expected at least one GPU" );
		const Uint32 numGPUs = dd.m_GPUVendorInterface ? dd.m_GPUVendorInterface->GetNumGPUs() : 1;
		return numGPUs > 0 ? numGPUs : 1;
	#else
		return 1;
	#endif
	}

	void MultiGPU_BeginEarlyPushTexture( const TextureRef &tex )
	{
	#ifdef RED_PLATFORM_WINPC
		if ( MultiGPU_IsActive() )
		{
			const SDeviceData &dd = GetDeviceData();
			dd.m_GPUVendorInterface->BeginEarlyPushTexture( tex );
		}
	#endif
	}

	void MultiGPU_EndEarlyPushTexture( const TextureRef &tex )
	{
	#ifdef RED_PLATFORM_WINPC
		if ( MultiGPU_IsActive() )
		{
			const SDeviceData &dd = GetDeviceData();
			dd.m_GPUVendorInterface->EndEarlyPushTexture( tex );
		}
	#endif
	}

}
