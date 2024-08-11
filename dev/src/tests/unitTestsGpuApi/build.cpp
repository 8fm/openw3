//
//* Copyright © 2013 CD Projekt Red. All Rights Reserved.
//

#include "build.h"
#include "testEngine.h"

// Hack
Float GRenderSettingsMipBias = 0.f;

// Initiate those for each render api
GpuApi::SwapChainRef GSwapChain;
CTestEngine* GEngine = nullptr;

#if defined(RED_PLATFORM_WINPC)

HWND windowHandle;

LONG APIENTRY StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_SIZE && !GSwapChain.isNull() )
	{
		// When switching from fullscreen to other modes, DXGI sends WM_SIZE event and changes out resolution to whatever was set before getting into fullscreen
		// so we want to ignore that event when VWM_Fullscreen
		Int32 width = LOWORD(lParam);
		Int32 height = HIWORD(lParam);
		GpuApi::ResizeBackbuffer( width, height, GSwapChain );
	}
	return static_cast< LONG >( DefWindowProc( hWnd, uMsg, (WPARAM)wParam, (LPARAM)lParam ) );
}

void SetupWindow( Int32 width, Int32 height )
{
	// Create window and swapchain
	RECT windowRect;
	windowRect.left = 100;
	windowRect.top = 100;
	windowRect.right = 100 + (width > 0 ? width : DEVICE_WIDTH);
	windowRect.bottom = 100 + (height > 0 ? height : DEVICE_HEIGHT);

	// Adjust window rectangle with border size
	Uint32 mainStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
	::AdjustWindowRect( &windowRect, mainStyle, false );

	WNDCLASSEX info;
	Red::System::MemorySet( &info, 0, sizeof( WNDCLASSEX ) );
	info.cbSize = sizeof( WNDCLASSEX );

	// Assemble class info
	info.cbWndExtra = 8;
	info.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
	info.hCursor = ::LoadCursor( nullptr, IDC_CROSS );
	info.hInstance = ::GetModuleHandle( nullptr );
	//info.hIcon = ::LoadIconW( info.hInstance, MAKEINTRESOURCEW( WIN32_ICON_RESOURCE_NUMBER ) );
	info.lpfnWndProc = reinterpret_cast< WNDPROC >( &StaticWndProc );
	info.lpszClassName = TXT("GpuApiUnitTestsClass");
	info.lpszMenuName = nullptr;
	info.style = CS_VREDRAW | CS_HREDRAW;

	// Register class
	RED_VERIFY( ::RegisterClassEx( &info ) );

	Uint32 exStyle = WS_EX_APPWINDOW;
	// Create window
	windowHandle = ::CreateWindowEx( 
		exStyle,
		TXT("GpuApiUnitTestsClass"), 
		TXT("GpuApiUnitTests"), 
		mainStyle | WS_CLIPCHILDREN, 
		windowRect.left,
		windowRect.top, 
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, 
		NULL, 
		::GetModuleHandle( nullptr ),
		nullptr
		);

	DWORD error = GetLastError();
	RED_UNUSED( error );

	// Bring to front 
	::SetForegroundWindow( windowHandle );
	::SetFocus( windowHandle );
	::UpdateWindow( windowHandle );
	::SendMessage( windowHandle, WM_ERASEBKGND, 0, 0 );
}

#endif

void SInitializePlatform( const CommandLineArguments & args )
{
	// Setup error system
#ifndef NO_ASSERTS
	extern Red::System::Error::EAssertAction AssertMessageImp( const Char* cppFile, Uint32 line, const Char* expression, const Char* message, const Char* details );
	Red::System::Error::Handler::GetInstance()->RegisterAssertHook( &AssertMessageImp );
	Red::System::Error::Handler::GetInstance()->SetVersion( TXT("version") );
#endif

	// Init Red IO
	Red::IO::Initialize();

	// Init GpuApi DX11
	if( !GpuApi::InitEnv() )
	{
		RED_HALT( "Couldn't initialize GPU API!" );
	}
	
	if( !GpuApi::InitDevice( DEVICE_WIDTH, DEVICE_HEIGHT, false, false ) )
	{
		RED_HALT( "Couldn't initialize device!" );
	}

#if defined(RED_PLATFORM_WINPC)
	SetupWindow( args.m_params.m_width, args.m_params.m_height );
	if( args.m_params.m_windowed )
	{
		::ShowWindow( windowHandle, SW_SHOW );
	}
#endif

	// Setup Swapchain
	GpuApi::SwapChainDesc swapChainDesc;
	swapChainDesc.width = DEVICE_WIDTH;
	swapChainDesc.height = DEVICE_HEIGHT;

#if defined(RED_PLATFORM_WINPC)
	swapChainDesc.fullscreen = false;
	swapChainDesc.windowHandle = &windowHandle;
#elif defined(RED_PLATFORM_CONSOLE)
	swapChainDesc.fullscreen = true;
#endif

	swapChainDesc.overlay = false;
	GSwapChain = GpuApi::CreateSwapChainWithBackBuffer( swapChainDesc );
	if( !GSwapChain )
	{
		RED_HALT( "Couldn't create swap chain with back buffer!" );
	}

	GpuApi::SetBackBufferFromSwapChain( GSwapChain );

	GEngine = new CTestEngine( args.m_params, GSwapChain );
	if ( !GEngine || !GEngine->Initialize() )
	{
		RED_HALT( "Couldn't initialize Test Engine." );
	}
}

int Run()
{
	return GEngine->Start();
}

void SShutdownPlatform()
{
	// Deinit GpuApi
	GpuApi::ShutDevice();
	Red::IO::Shutdown();
	delete GEngine;
}