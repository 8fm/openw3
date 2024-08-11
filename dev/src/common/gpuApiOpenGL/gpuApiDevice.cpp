/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../redMath/numericalutils.h"
#include "../redSystem/crt.h"
#include "../redThreads/redThreadsThread.h"

namespace GpuApi
{

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
		if ( 0 == dd.m_SwapChains.DecRefCount( swapChain ) )
		{
			SSwapChainData &data = dd.m_SwapChains.Data( swapChain );

			//// Release resources
			//GPUAPI_ASSERT( nullptr != data.m_swapChain );

			//if ( data.m_swapChain )
			//{
			//	ULONG refcount = data.m_swapChain->Release();
			//	RED_UNUSED( refcount );
			//	//GPUAPI_ASSERT( refcount == 0, TXT( "swap chain leak" ) );
			//	// the backbuffer will be released when all the rendertargets are unset
			//	data.m_swapChain = nullptr;
			//}

			// Destroy shit
			dd.m_SwapChains.Destroy( swapChain );
		}

		return 0;
	}

	// ----------------------------------------------------------------------

	const eTextureFormat BACKBUFFER_FORMAT = TEXFMT_R8G8B8X8;

	// ----------------------------------------------------------------------

	Red::Threads::CMutex	g_ResourceMutex;
	SDeviceData*			g_DeviceData;

	// ----------------------------------------------------------------------

	Bool InitEnv()
	{
		g_DeviceData = new SDeviceData( g_ResourceMutex );
		return true;
	}

	void CreateConstantBuffers();
	Bool InitDevice( Uint32 width, Uint32 height, Bool fullscreen, Bool vsync )
	{
		RED_UNUSED(vsync);
		RED_UNUSED(fullscreen);
		RED_UNUSED(height);
		RED_UNUSED(width);

//		if ( g_DeviceData->m_DeviceInitDone )
//		{
//			// It's possible to init device only once..
//			return !g_DeviceData->m_DeviceShutDone;
//		}
//
//		GPUAPI_ASSERT( nullptr == g_DeviceData->m_pDevice );
//
//		SDeviceData &dd = GetDeviceData();
//
//		// Setup flags
//		Uint32 deviceFlags = 0;
//
//#ifdef _DEBUG
//		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;				// Save FPU flags
//#endif
//		// Try to initialize hardware device
//#if defined( RED_PLATFORM_WINPC )
//		ID3D11Device* device = nullptr;
//		ID3D11DeviceContext* context = nullptr;
//		D3D_FEATURE_LEVEL featureLevel;
//		HRESULT hRet = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context );
//		if ( FAILED( hRet ) || !device )
//		{
//			GPUAPI_WARNING( TXT( "Unable to create Direct3D 11 device" ) );
//			return false;
//		}
//
//		HRESULT res = context->QueryInterface( __uuidof(ID3DUserDefinedAnnotation) , (void**) &dd.m_pPerformanceInterface );
//		if ( FAILED(res) )
//		{
//			GPUAPI_WARNING( TXT("Can't query ID3DUserDefinedAnnotation interface, profile blocks won't work") );
//			dd.m_pPerformanceInterface = nullptr;
//		}
//
//#elif defined( RED_PLATFORM_DURANGO )
//
//#ifndef RELEASE
//		deviceFlags |= D3D11_CREATE_DEVICE_INSTRUMENTED;
//#endif // !RELEASE
//
//		D3D11X_CREATE_DEVICE_PARAMETERS deviceParams;
//		Red::System::MemorySet( &deviceParams, 0, sizeof(deviceParams) );
//		deviceParams.Flags = deviceFlags;
//		deviceParams.Version = D3D11_SDK_VERSION;
//
//		DXGI_SWAP_CHAIN_DESC1 scDesc;
//		Red::System::MemorySet( &scDesc, 0, sizeof(scDesc) );
//		scDesc.Width = width;
//		scDesc.Height = height;
//		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//		scDesc.Stereo = false;
//		scDesc.SampleDesc.Count = 1;
//		scDesc.SampleDesc.Quality = 0;
//		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//		scDesc.BufferCount = 2;
//		scDesc.Scaling = DXGI_SCALING_STRETCH;
//		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
//
//		IDXGISwapChain1* swapChain;
//		ID3D11DeviceX* device;
//		ID3D11DeviceContextX* context;
//
//		HRESULT hRet = D3D11XCreateDeviceXAndSwapChain1( &deviceParams, &scDesc, &swapChain, &device, &context );
//		if ( FAILED( hRet ) || !device )
//		{
//			GPUAPI_WARNING( TXT("Unable to create Direct3D 11 device") );
//			return false;
//		}
//
//		// Create
//		SwapChainRef swapChainRef = SwapChainRef( dd.m_SwapChains.Create( 1 ) );
//		if (swapChainRef.isNull())
//		{
//			GPUAPI_HALT(TXT("Too many swapchains"));
//			swapChain->Release();
//			return SwapChainRef::nullptr();
//		}
//
//		SSwapChainData &scd		= dd.m_SwapChains.Data( swapChainRef );
//		scd.m_swapChain			= swapChain;
//		scd.m_fullscreen		= fullscreen;
//#endif
//
//#if defined( _DEBUG ) && defined( RED_PLATFORM_WINPC )
//		ID3D11Debug* debugInterface = nullptr;
//		if( SUCCEEDED( device->QueryInterface( __uuidof(ID3D11Debug), (void**)&debugInterface ) ) )
//		{
//			ID3D11InfoQueue* infoQueue = nullptr;
//			if( SUCCEEDED( debugInterface->QueryInterface( __uuidof(ID3D11InfoQueue), (void**)&infoQueue ) ) )
//			{
//				// Disable DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL warnings, that are a confirmed dx11 debug layer bug (fixed in 11.1)
//				D3D11_MESSAGE_ID denied [] =
//				{
//					D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL,
//					//dex++: this is also not a error
//					D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
//					//dex--
//				};
//				D3D11_INFO_QUEUE_FILTER filter;
//				Red::System::MemorySet( &filter, 0, sizeof(filter) );
//				filter.DenyList.NumIDs = _countof(denied);
//				filter.DenyList.pIDList = denied;
//				infoQueue->AddStorageFilterEntries( &filter );
//
//				//infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
//				//infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
//				infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
//			}
//
//			SAFE_RELEASE( infoQueue );
//		}
//
//		SAFE_RELEASE( debugInterface );
//#endif
//
//		// Finalize
//		dd.m_pDevice = device;
//		dd.m_pImmediateContext = context;
//		GPUAPI_ASSERT( dd.m_Textures.IsEmpty() );
//		GPUAPI_ASSERT( dd.m_SamplerStates.IsEmpty() );
//		GPUAPI_ASSERT( dd.m_Buffers.IsEmpty() );
//		GPUAPI_ASSERT( dd.m_Queries.IsEmpty() );

		//// Create presets and internal resources
		//InitInternalResources( false );

		//// Create constant buffers
		//CreateConstantBuffers();

		//// Set implicit and explicit states
		//ForceSetImplicitD3DStates();
		//ForceSetExplicitD3DStates();

		// Finish :)
		g_DeviceData->m_DeviceInitDone = true;

		return true;
	}

	Bool IsInit()
	{
		//return nullptr != g_DeviceData->m_pDevice;
		return true;
	}

	eDeviceCooperativeLevel TestDeviceCooperativeLevel()
	{
		// HACK DX10 testing device cooperative level is different now
		//SDeviceData &dd = GetDeviceData();
		//HRESULT hRet = dd.m_swapChain->Present(0, DXGI_PRESENT_TEST);
		return DEVICECOOPLVL_Operational;// MapCooperativeLevel( hRet );
	}

	void ShutDevice()
	{
		SDeviceData &dd = GetDeviceData();

		GPUAPI_ASSERT(  dd.m_DeviceInitDone );
		GPUAPI_ASSERT( !dd.m_DeviceShutDone );
//
//		// Restore state to default
//		// TODO clean this up
//		{
//			GPUAPI_ASSERT( dd.m_BackBuffer );
//			//RenderTargetSetup rtSetup;
//			//rtSetup.SetColorTarget( 0, dd.m_BackBuffer );
//			//rtSetup.SetViewportFromTarget( dd.m_BackBuffer );
//			//SetupRenderTargets( rtSetup );
//
//			ID3D11RenderTargetView* rts = nullptr;
//			ID3D11DepthStencilView* dss = nullptr;
//
//			GetDeviceContext()->OMSetRenderTargets(1, &rts, dss);
//
//			BindTextures( 0, MAX_PS_SAMPLERS, nullptr, PixelShader );
//			BindTextures( 0, MAX_VS_SAMPLERS, nullptr, VertexShader );
//			//SetSamplerStateCommon( 0, MAX_PS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, PixelShader );
//			//SetSamplerStateCommon( 0, MAX_VS_SAMPLER_STATES, SAMPSTATEPRESET_WrapPointNoMip, VertexShader );
//			ID3D11SamplerState* stateObject = nullptr;
//			for (int i =0; i<8; ++i)
//			{
//				GetDeviceContext()->PSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->VSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->DSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->HSSetSamplers( i, 1, &stateObject );
//				GetDeviceContext()->GSSetSamplers( i, 1, &stateObject );
//			}
//
//			BindVertexBuffers( 0, 0, nullptr, nullptr, nullptr );
//			BindIndexBuffer( BufferRef::Null() );
//
//			GpuApi::InvalidateSamplerStates();
//			GpuApi::SetCustomDrawContext(GpuApi::DSSM_Max, GpuApi::RASTERIZERMODE_Max, GpuApi::BLENDMODE_Max);
//			GetDeviceContext()->OMSetDepthStencilState( nullptr, 0 );
//			GetDeviceContext()->OMSetBlendState( nullptr, nullptr, 0xFFFFFFFF );
//			GetDeviceContext()->RSSetState( nullptr );
//
//			ULONG refCount = 0;
//			refCount = dd.m_GlobalVSConstantBuffer->Release();
//			refCount = dd.m_CustomVSConstantBuffer->Release();
//			refCount = dd.m_SkinningVSConstantBuffer->Release();
//			refCount = dd.m_SpecificVSConstantBuffer->Release();
//			refCount = dd.m_GlobalPSConstantBuffer->Release();
//			refCount = dd.m_CustomPSConstantBuffer->Release();
//			refCount = dd.m_SpecificPSConstantBuffer->Release();
//			refCount = dd.m_CustomCSConstantBuffer->Release();
//		}
//
//		// Remove shadow state references
//		dd.m_StateRenderTargetSetup.ChangeAllRefCounts( false );
//		dd.m_pImmediateContext->ClearState();
//		dd.m_pImmediateContext->Flush();
//		// Shut internal resources
//		ShutInternalResources( true );
//
//#if WAITING_FOR_DEX_TO_FIX_GLOBAL_TEXTURES
//		// Remove shit so that release ASSERTION would not fire up.
//		dd.m_Textures.DestroyAll();
//		dd.m_SamplerStates.DestroyAll();
//		dd.m_Buffers.DestroyAll();
//		dd.m_Queries.DestroyAll();
//#endif
//
//		dd.m_StateRenderStateCache.Clear();
//
		GpuApi::SafeRelease(dd.m_drawPrimitiveUPIndexBuffer);
		GpuApi::SafeRelease(dd.m_drawPrimitiveUPVertexBuffer);
//
//		for ( Uint32 ili = 0; ili < BCT_Max * MAX_SHADER_COUNT; ++ili )
//		{
//			if (dd.m_InputLayouts[ili])
//			{
//				ULONG refcount = dd.m_InputLayouts[ili]->Release();
//				GPUAPI_ASSERT(refcount==0, TXT( "input layout leak" ) );
//				dd.m_InputLayouts[ili] = nullptr;
//			}
//		}
//
//		dd.m_pImmediateContext->ClearState();
//		dd.m_pImmediateContext->Flush();
//		ULONG contextRefCount = dd.m_pImmediateContext->Release();
//		RED_UNUSED( contextRefCount );
//		dd.m_pImmediateContext = nullptr;
//
//#if defined( _DEBUG ) && defined( RED_PLATFORM_WINPC )
//		ID3D11Debug* debugInterface = nullptr;
//		if( SUCCEEDED( dd.m_pDevice->QueryInterface( __uuidof(ID3D11Debug), (void**)&debugInterface ) ) )
//		{
//			debugInterface->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
//		}
//		SAFE_RELEASE( debugInterface );
//
//		typedef HRESULT (WINAPI *GetDebugInterfaceProc)(REFIID riid, void **ppDebug);
//
//		HMODULE dxgidebugmodule = GetModuleHandle( TXT("Dxgidebug.dll") ); // doesn't increment refcount
//		if ( dxgidebugmodule != nullptr )
//		{
//			GetDebugInterfaceProc getDebugInterfaceProcedure = (GetDebugInterfaceProc) GetProcAddress( dxgidebugmodule, "DXGIGetDebugInterface" );
//			if ( getDebugInterfaceProcedure != nullptr )
//			{
//				IDXGIDebug* debugDxgiInterface;
//				getDebugInterfaceProcedure( __uuidof(IDXGIDebug), (void**)&debugDxgiInterface );
//				if (debugDxgiInterface)
//				{
//					debugDxgiInterface->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL );
//				}
//			}
//		}
//
//#endif
//
//		// Remove device
//		ULONG deviceRefCount = dd.m_pDevice->Release();
//		RED_UNUSED( deviceRefCount );
//		dd.m_pDevice = nullptr;
//
//		// Safe info that we already had device init/shut
//		dd.m_DeviceShutDone = true;
//
//		//
//		GPUAPI_ASSERT( !IsInit() );
	}

	const Capabilities& GetCapabilities()
	{
		return GetDeviceData().m_Caps;
	}

	void SetRenderSettings( const RenderSettings &newSettings )
	{
		SDeviceData &dd = GetDeviceData();

		// Copy
		dd.m_RenderSettings = newSettings;

		// Validate
		dd.m_RenderSettings.anisotropy = Red::Math::NumericalUtils::Max< Uint32 >( 1, Red::Math::NumericalUtils::Min< Uint32 >( dd.m_Caps.maxAnisotropy, newSettings.anisotropy ) );

		// Apply some immediate states
		//dd.m_StateRenderStateCache.SetWireframe( newSettings.wireframe );
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

	// This function must have this precise prototype ( parameters and return value )
	// See http://www.opengl.org/registry/specs/ARB/debug_output.txt , "New Types" :
	//      The callback function that applications can define, and
	//      is accepted by DebugMessageCallbackARB, is defined as:
	//      
	//           typedef void (APIENTRY *DEBUGPROC)(GLenum source,
	//           	GLenum type,
	//           	GLuint id,
	//           	GLenum severity,
	//           	GLsizei length,
	//           	const GLchar *message,
	//           	void *userParam);

	void APIENTRY DebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam)
	{
		if(severity == GL_DEBUG_SEVERITY_HIGH_ARB)                              printf("Severity : HIGH; ");
		else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)               printf("Severity : MEDIUM; ");
		else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)                  printf("Severity : LOW; ");
		//else {
		//	printf("Severity : ???; \n");
		//	return;
		//}

		if(source == GL_DEBUG_SOURCE_API_ARB)                                   printf("Source : API; ");
		else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)    printf("Source : WINDOW_SYSTEM; ");
		else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)  printf("Source : SHADER_COMPILER; ");
		else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)              printf("Source : THIRD_PARTY; ");
		else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)              printf("Source : APPLICATION; ");
		else if(source == GL_DEBUG_SOURCE_OTHER_ARB)                    printf("Source : OTHER; ");

		if(type == GL_DEBUG_TYPE_ERROR_ARB)
		{
			OutputDebugStringA("ERROR!\n");
			printf("Type : ERROR; ");
		}
		else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)  printf("Type : DEPRECATED_BEHAVIOR; ");
		else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)   printf("Type : UNDEFINED_BEHAVIOR; ");
		else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)                  printf("Type : PORTABILITY; ");
		else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)                  printf("Type : PERFORMANCE; ");
		else if(type == GL_DEBUG_TYPE_OTHER_ARB)                                printf("Type : OTHER; ");

		// You can set a breakpoint here ! Your debugger will stop the program,
		// and the callstack will immediately show you the offending call.
		OutputDebugStringA(message);
		OutputDebugStringA( "\n" );
		printf(message);
		printf("\n");
	}

#ifdef USE_OPENGL_PROFILE
	void ErrorReport( Int32 errNo , const char* expr, const char* file, Uint32 lineNumber )
	{
		char bufferMsg[ 1024 ];

		do
		{
			const char* errorLog = nullptr;

			switch( errNo )
			{
			case GL_INVALID_ENUM :
				errorLog = "GL_INVALID_ENUM";
				break;
			case GL_INVALID_VALUE :
				errorLog = "GL_INVALID_VALUE";
				break;
			case GL_INVALID_OPERATION :
				errorLog = "GL_INVALID_OPERATION";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION :
				errorLog = "GL_INVALID_FRAMEBUFFER_OPERATION";
				break;
			case GL_OUT_OF_MEMORY :
				errorLog = "GL_OUT_OF_MEMORY";
				break;
			case GL_STACK_OVERFLOW :
				errorLog = "GL_STACK_OVERFLOW";
				break;
			case GL_STACK_UNDERFLOW :
				errorLog = "GL_STACK_UNDERFLOW";
				break;
			default :
				errorLog = "UNKNOWN";
				break;
			}

			sprintf_s( bufferMsg , "OpenGl Error: %u ( %s ) in %s [%u] -> %s " , errNo , errorLog , file , lineNumber , expr );
			OutputDebugStringA( bufferMsg );

			errNo = glGetError();

		} while( errNo != GL_NO_ERROR );

	}
#endif

	void GetGLVersion(int* major, int* minor)
	{
		// for all versions
		char* ver = (char*)glGetString(GL_VERSION); // ver = "3.2.0"

		*major = ver[0] - '0';
		if( *major >= 3)
		{
			// for GL 3.x
			glGetIntegerv(GL_MAJOR_VERSION, major); // major = 3
			glGetIntegerv(GL_MINOR_VERSION, minor); // minor = 2
		}
		else
		{
			*minor = ver[2] - '0';
		}

		// GLSL
		ver = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION); // ver = "1.50 NVIDIA via Cg compiler"
	}

	SwapChainRef CreateSwapChainWithBackBuffer( const SwapChainDesc& swapChainDesc )
	{
		SDeviceData &dd = GetDeviceData();

		PIXELFORMATDESCRIPTOR pfd;
		int format;

		// get the device context (DC)
		HDC dcHandle = GetDC( *(static_cast< HWND*>(swapChainDesc.windowHandle)) );

		// set the pixel format for the DC
		ZeroMemory(&pfd, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.iLayerType = PFD_MAIN_PLANE;
		format = ChoosePixelFormat( dcHandle, &pfd );
		SetPixelFormat( dcHandle, format, &pfd );

		// create and enable the render context (RC)
		HGLRC rcHandle = wglCreateContext( dcHandle );
		if ( !rcHandle )
		{
			GPUAPI_HALT("Can't create OpenGL render context");
			return SwapChainRef::Null();
		}
		wglMakeCurrent( dcHandle, rcHandle);

		// Glew needs a context to be created and set as current
		glewExperimental = GL_TRUE; // Needed for core profile
		GLenum glewStatus = glewInit();
		if (glewStatus == GLEW_OK)
		{
			if (glewIsSupported("GL_VERSION_4_5"))
			{
				GPUAPI_LOG(TXT("Ready for OpenGL 4.5"));
			}
			else if (glewIsSupported("GL_VERSION_4_4"))
			{
				GPUAPI_LOG(TXT("Ready for OpenGL 4.4"));
			}
			else if (glewIsSupported("GL_VERSION_4_3"))
			{
				GPUAPI_LOG(TXT("Ready for OpenGL 4.3"));
			}
			else 
			{
				GPUAPI_HALT("OpenGL 4.2 not supported");
			}
		}
		else
		{
			const GLubyte* errorMessage = glewGetErrorString(glewStatus);
			GPUAPI_HALT("Can't init glew");
		}

#ifdef _DEBUG
		Int32 major, minor;
		GetGLVersion(&major, &minor);

		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, major,
			WGL_CONTEXT_MINOR_VERSION_ARB, minor,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		HGLRC debugRcHandle = wglCreateContextAttribsARB( dcHandle,0, attribs );
		if (debugRcHandle)
		{
			wglDeleteContext( rcHandle );
			rcHandle = debugRcHandle;
		}
#endif

		wglMakeCurrent( dcHandle, rcHandle);
		glEnable(GL_DEBUG_OUTPUT);

		if ( GLEW_ARB_debug_output )
		{
			GPUAPI_LOG(TXT("Debug output available"));
			glDebugMessageCallback( (GLDEBUGPROC)&DebugOutputCallback, nullptr );
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		}
		else
		{
			GPUAPI_LOG(TXT("Debug output unavailable, use gDebugger"));
		}

		glEnable(GL_DEPTH_TEST);
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glEnable(GL_CULL_FACE);
		glCullFace( GL_BACK );
		glFrontFace( GL_CW );

		InitInternalResources( false );
		CreateConstantBuffers();

		// Create
		SwapChainRef swapChainRef = SwapChainRef( dd.m_SwapChains.Create( 1 ) );
		if (swapChainRef.isNull())
		{
			GPUAPI_HALT("Too many swapchains");
			return SwapChainRef::Null();
		}

		SSwapChainData &scd = dd.m_SwapChains.Data(swapChainRef);
		scd.m_deviceContext = dcHandle;
		scd.m_renderContext = rcHandle;

		scd.m_backBuffer = TextureRef(dd.m_Textures.Create(1));

		// Init desc
		STextureData &td		= dd.m_Textures.Data( scd.m_backBuffer );
		td.m_Desc.type			= TEXTYPE_2D;
		td.m_Desc.format		= GpuApi::BACKBUFFER_FORMAT;
		td.m_Desc.usage			= TEXUSAGE_BackBuffer;
		td.m_Desc.width			= swapChainDesc.width;
		td.m_Desc.height		= swapChainDesc.height;
		td.m_Desc.initLevels	= 1;

		scd.m_depthStencil = TextureRef(dd.m_Textures.Create(1));

		// Init desc
		STextureData &depthd		= dd.m_Textures.Data( scd.m_depthStencil );
		depthd.m_Desc.type			= TEXTYPE_2D;
		depthd.m_Desc.format		= TEXFMT_D24S8;
		depthd.m_Desc.usage			= TEXUSAGE_BackBufferDepth;
		depthd.m_Desc.width			= swapChainDesc.width;
		depthd.m_Desc.height		= swapChainDesc.height;
		depthd.m_Desc.initLevels	= 1;

		return swapChainRef;
	}

	void SetBackBufferFromSwapChain( const SwapChainRef& swapChain )
	{
		SDeviceData &dd = GetDeviceData();
		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
		dd.m_BackBuffer = scd.m_backBuffer;
		dd.m_DepthStencil = scd.m_depthStencil;
	}

	void ResizeBackbuffer( Uint32 width, Uint32 height, const SwapChainRef& swapChain )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

//		SDeviceData &dd = GetDeviceData();
//		STextureData &td = dd.m_Textures.Data( backBuffer );
//		ID3D11RenderTargetView* nullRT = nullptr;
//		GetDeviceContext()->OMSetRenderTargets( 1, &nullRT, nullptr );
//
//		td.m_pTexture->AddRef();
//		ULONG texRefCount = td.m_pTexture->Release();
//		RED_UNUSED(texRefCount);
//		td.m_pRenderTargetView->AddRef();
//		ULONG rtvRefCount = td.m_pRenderTargetView->Release();
//		RED_UNUSED(rtvRefCount);
//		SAFE_RELEASE( td.m_pRenderTargetView );
//		SAFE_RELEASE( td.m_pTexture );
//
//		SSwapChainData &scd = dd.m_SwapChains.Data( swapChain );
//
//		if ( width != 0 && height != 0 )
//		{
//			HRESULT res = scd.m_swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
//			if (res != S_OK)
//			{
//				GPUAPI_HALT( TXT( "Failed to resize backbuffer. Error code: %d" ), res );
//			}
//		}
//
//		td.m_Desc.width = width;
//		td.m_Desc.height = height;
//
//		// Create a render target view
//		ID3D11Texture2D *pBackBuffer;
//		ID3D11RenderTargetView* renderTargetView;
//
//		HRESULT hr = scd.m_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
//		if( FAILED( hr ) )
//		{
//			GPUAPI_HALT( TXT( "Failed to get backbuffer. Error code: %d" ), hr );
//		}
//
//		hr = GetDevice()->CreateRenderTargetView( pBackBuffer, nullptr, &renderTargetView );
//		if( FAILED( hr ) )
//		{
//			GPUAPI_HALT( TXT( "Failed to create rendertarget for backbuffer. Error code: %d" ), hr );
//		}
//#ifdef GPU_API_DEBUG_PATH
//		else
//		{
//			renderTargetView->SetPrivateData( WKPDID_D3DDebugObjectName, 7, "backBuf" );
//		}
//#endif
//
//		td.m_pTexture = pBackBuffer;
//		td.m_pRenderTargetView = renderTargetView;
	}

	Bool ToggleFullscreen( SwapChainRef swapChain, Bool fullscreen )
	{
		return false;
	}

	void* CreateContext()
	{
		return nullptr;
	}

	void SubmitContext( void* deferredContext )
	{
	}

	void CancelContext( void* deferredContext )
	{
	}

	void* GetCommandListFromContext( void* deferredContext )
	{
		return nullptr;
	}

	void SubmitCommandList( void* commandList )
	{
	}

	void CancelCommandList( void* commandList )
	{

	}

	void BeginProfilerBlock( const Char* name )
	{

	}

	void EndProfilerBlock()
	{

	}

	void SetMarker( const Char* name )
	{

	}
}
