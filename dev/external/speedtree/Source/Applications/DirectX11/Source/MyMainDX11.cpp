///////////////////////////////////////////////////////////////////////  
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		Web: http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//	Preprocessor

#include "DXUT.h"
#pragma warning (disable : 4995)
#include "MyApplication.h"
//#define SPEEDTREE_OVERRIDE_GLOBAL_NEW_AND_DELETE
#define SPEEDTREE_USE_ALLOCATOR_INTERFACE // must be active for Debug MT and Release MT builds
#define SPEEDTREE_OVERRIDE_FILESYSTEM
#include "MyCustomAllocator.h"
#include "MyFileSystem.h"
using namespace SpeedTree;

#ifndef NDEBUG
	#pragma comment( lib, "dxguid.lib") 
#endif

// enable to allow debugging with NVPerfHUD
#define SPEEDTREE_NVPERFHUD


///////////////////////////////////////////////////////////////////////  
//  File-scope function prototypes

static  PCHAR*					CommandLineToArgvA(PCHAR CmdLine, int* _argc);


///////////////////////////////////////////////////////////////////////  
//  File-scope variables

		// memory management
#ifdef SPEEDTREE_USE_ALLOCATOR_INTERFACE
static	CMyCustomAllocator		g_cCustomAllocator;
static	CAllocatorInterface		g_cAllocatorInterface(&g_cCustomAllocator);
#endif
		
		// file system
#ifdef SPEEDTREE_OVERRIDE_FILESYSTEM
static	CMyFileSystem			g_cMyFileSystem;
static	CFileSystemInterface	g_cFileSystemInterface(&g_cMyFileSystem);
#endif

		// app
static	CMyApplication*			g_pApplication = NULL;

		// initialization timing
static	CTimer					g_cInitTimer;


///////////////////////////////////////////////////////////////////////  
//	ModifyDeviceSettings
//
//	Called right before creating a D3D9 or d3d11 device, allowing the app to modify the device settings as needed

bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	if (pDeviceSettings->ver == DXUT_D3D11_DEVICE)
	{
        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
		#ifdef DEBUG_VS
			if (pDeviceSettings->d3d11.DriverType != D3D_DRIVER_TYPE_REFERENCE)
				pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif
		#ifdef DEBUG_PS
			pDeviceSettings->d3d11.DriverType = D3D_DRIVER_TYPE_REFERENCE;
		#endif

		// Request the multisampling quality specified on the command-line
		assert(g_pApplication);
		#ifdef ST_FW_POSTFX
			pDeviceSettings->d3d11.sd.SampleDesc.Count = 1;
		#else
			pDeviceSettings->d3d11.sd.SampleDesc.Count = g_pApplication->GetCmdLineOptions( ).m_nNumSamples;	
		#endif

		// handle nVidia PerfHUD if it is enabled
		#ifdef SPEEDTREE_NVPERFHUD 
			UINT uiAdapter = 0; 
			IDXGIAdapter* pAdapter = NULL;
			IDXGIFactory* pDXGIFactory = DXUTGetDXGIFactory( );
			while (pDXGIFactory->EnumAdapters(uiAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND) 
			{
				DXGI_ADAPTER_DESC sDescription; 
				if (pAdapter != NULL && SUCCEEDED(pAdapter->GetDesc(&sDescription)) &&
					wcscmp(sDescription.Description, L"NVIDIA PerfHUD") == 0) 
				{						 
					pDeviceSettings->d3d11.AdapterOrdinal = uiAdapter;
					pDeviceSettings->d3d11.DriverType = D3D_DRIVER_TYPE_REFERENCE;
					printf("Detected NVIDIA PerfHUD\n");
				}
					
				ST_SAFE_RELEASE(pAdapter);
				++uiAdapter;
			}
		#endif
    }

	// For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if ((DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
            (DXUT_D3D11_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE))
		{
			printf("\n\n\t*** Currently using a reference renderer ***\n\n");
		}
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//	Isd3d11DeviceAcceptable
//
//	Reject any D3D11 devices that aren't acceptable by returning false

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo* pAdapterInfo, UINT uiOutput, const CD3D11EnumDeviceInfo* pDeviceInfo,
                                       DXGI_FORMAT sBackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//	Ond3d11CreateDevice
//
//	Create any d3d11 resources that aren't dependant on the back buffer

HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	assert(pd3dDevice);
	SpeedTree::DX11::SetDevice(pd3dDevice);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//	Ond3d11ResizedSwapChain
//
//	Create any d3d11 resources that depend on the back buffer

HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, 
										 IDXGISwapChain* pSwapChain, 
										 const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, 
										 void* pUserContext)
{
	if (g_pApplication != NULL)
	{
		g_pApplication->WindowReshape(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	}

    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//	Ond3d11ReleasingSwapChain
//
//	Release d3d11 resources created in Ond3d11ResizedSwapChain 

void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//	Ond3d11DestroyDevice
//
//	Release D3D11 resources created in Ond3d11CreateDevice 

void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//	OnFrameMove
//
//	Handle updates to the scene.  This is called regardless of which D3D API is used

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	ST_UNREF_PARAM(fTime);
	ST_UNREF_PARAM(fElapsedTime);
	ST_UNREF_PARAM(pUserContext);

}


///////////////////////////////////////////////////////////////////////  
//	Ond3d11FrameRender
//
//	Render the scene using the d3d11 device

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, 
								 ID3D11DeviceContext* pd3dImmediateContext,
								 double fTime, 
								 float fElapsedTime, 
								 void* pUserContext)
{
    bool bInitError = false;
	static bool bFirstDisplay = true;

	CScopeTrace::Init( );

	if (bFirstDisplay)
	{
		DX11::SetDeviceContext(pd3dImmediateContext);

		assert(g_pApplication);
		if (!g_pApplication->InitGfx( ))
		{
			bInitError = true;
			PrintSpeedTreeErrors( );
			system("pause");
			exit(-1);
		}

		g_cInitTimer.Stop( );
		printf("Initialization time: %g ms\n", g_cInitTimer.GetMilliSec( ));

		bFirstDisplay = false;
	}

	if (!bInitError)
	{
		assert(g_pApplication);
		if (g_pApplication->ReadyToRender( ))
		{
			// update the scene before the render; this can be outside of the render thread/function, but must be
			// completed before the render can proceed
			g_pApplication->Advance( );
			g_pApplication->Cull( );
			g_pApplication->ReportStats( );

			// draw calls
			DX11::DeviceContext( )->ClearRenderTargetView(DXUTGetD3D11RenderTargetView( ), D3DXVECTOR4( ));
			DX11::DeviceContext( )->ClearDepthStencilView(DXUTGetD3D11DepthStencilView( ), D3D11_CLEAR_DEPTH, 1.0f, 0);

			g_pApplication->Render( );
		}
	}
}


///////////////////////////////////////////////////////////////////////  
//	MsgProc
//
//	Handle messages to the application

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
	// handle keyboard and mouse events
	int x = int(LOWORD(lParam));
	int y = int(HIWORD(lParam));

	assert(g_pApplication);
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		g_pApplication->MouseClick(CMyMouseNavigation::BUTTON_LEFT, true, x, y);
		SetCapture(hWnd);
		break;
	case WM_LBUTTONUP:
		g_pApplication->MouseClick(CMyMouseNavigation::BUTTON_LEFT, false, x, y);
		ReleaseCapture( );
		break;
	case WM_MBUTTONDOWN:
		ShowCursor(FALSE);
		g_pApplication->MouseClick(CMyMouseNavigation::BUTTON_MIDDLE, true, x, y);
		SetCapture(hWnd);
		break;
	case WM_MBUTTONUP:
		ShowCursor(TRUE);
		g_pApplication->MouseClick(CMyMouseNavigation::BUTTON_MIDDLE, false, x, y);
		ReleaseCapture( );
		break;
	case WM_RBUTTONDOWN:
		ShowCursor(FALSE);
		g_pApplication->MouseClick(CMyMouseNavigation::BUTTON_RIGHT, true, x, y);
		SetCapture(hWnd);
		break;
	case WM_RBUTTONUP:
		ShowCursor(TRUE);
		g_pApplication->MouseClick(CMyMouseNavigation::BUTTON_RIGHT, false, x, y);
		ReleaseCapture( );
		break;
	case WM_MOUSEMOVE:
		g_pApplication->MouseMotion(x, y);
		break;
	}

    return 0;
}


///////////////////////////////////////////////////////////////////////  
//	OnKeyboard
//
//	Handle key presses

void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	assert(g_pApplication);

	if (bKeyDown)
		g_pApplication->KeyDown((unsigned char) nChar, 0, 0);
	else
		g_pApplication->KeyUp((unsigned char) nChar, 0, 0);
}


///////////////////////////////////////////////////////////////////////  
//	OnMouse
//
//	Handle mouse button presses

void CALLBACK OnMouse(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                      bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                      int xPos, int yPos, void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//	OnDeviceRemoved
//
//	Call if device was removed. Return true to find a new device, false to quit

bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
    return true;
}


///////////////////////////////////////////////////////////////////////  
//	wWinMain
//
//	Initialize everything and go into a render loop

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif

	// set current path to that of the app
	char szBuf[1024];
	GetModuleFileNameA(hInstance, szBuf, 1024);
	SetCurrentDirectoryA(SpeedTree::CString(szBuf).Path( ).c_str( ));

	// check DirectX version
	if (!D3DX11CheckVersion(D3D_SDK_VERSION, D3DX_SDK_VERSION))
	{
		printf("This SpeedTree Reference Application was built with a different version of the DirectX SDK than is installed.\n");
		exit(1);
	}

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove(OnFrameMove);
    DXUTSetCallbackKeyboard(OnKeyboard);
    DXUTSetCallbackMouse(OnMouse);
    DXUTSetCallbackMsgProc(MsgProc);
    DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
    DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

	// D3D11 DXUT callbacks
	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	// perform any application-level initialization here
	{
		// convert the command-line to standard argc and argv
		int argc = 0;
		char** argv = CommandLineToArgvA(GetCommandLineA( ), &argc);

		g_cInitTimer.Start( );

		g_pApplication = st_new(CMyApplication, "CMyApplication");
		if (g_pApplication->ParseCmdLine(argc, argv))
		{
			// create console so that prints can be seen
			if (g_pApplication->GetCmdLineOptions( ).m_bConsole)
			{
				AllocConsole( );                 // allocate console window
				FILE* pStream = NULL;
				freopen_s(&pStream, "CONOUT$", "a", stderr); // redirect stderr to console
				freopen_s(&pStream, "CONOUT$", "a", stdout); // redirect stdout also
				freopen_s(&pStream, "CONIN$", "r", stdin);
				SetConsoleTitleA("SpeedTree Console Window");
			}

			if (!g_pApplication->Init( ))
			{
				Error("Failed to initialize forest");
				return DXUTGetExitCode( );
			}
		}

		GlobalFree(argv);
	}

	CMyApplication::PrintId( );
	CMyApplication::PrintKeys( );

	// extract command-line settings to set up the window
	assert(g_pApplication);
	const SMyCmdLineOptions& sCmdLine = g_pApplication->GetCmdLineOptions( );

	DXUTSetIsInGammaCorrectMode(false);
    DXUTInit(false, true, NULL); // Parse the command line, show msgboxes on error, no extra command line params
    
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	{
		SpeedTree::CString strTitle = SpeedTree::CString::Format("SpeedTree SDK v%s DirectX 11.0 Application", SPEEDTREE_VERSION_STRING);
		WCHAR wstrAppTitle[64];
		MultiByteToWideChar(CP_ACP, 0, strTitle.c_str( ), -1, wstrAppTitle, 64);
		DXUTCreateWindow(wstrAppTitle);
	}

	if (sCmdLine.m_bFullscreen)
	{
		if (sCmdLine.m_bFullscreenResOverride)
			DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, false, sCmdLine.m_nWindowWidth, sCmdLine.m_nWindowHeight);
		else
			DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, false);
	}
	else
		DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, sCmdLine.m_nWindowWidth, sCmdLine.m_nWindowHeight);

    DXUTMainLoop( ); // Enter into the DXUT render loop

	g_pApplication->ReleaseGfxResources( );
	st_delete<CMyApplication>(g_pApplication);

	CCore::ShutDown( );
	#ifdef SPEEDTREE_MEMORY_STATS
		SpeedTree::CAllocator::Report("st_memory_report_windows_directx11.csv", true);
	#endif

	// cleanup console
	fclose(stderr);
	fclose(stdout);
	fclose(stdin);
	FreeConsole( );

    return DXUTGetExitCode( );
}


///////////////////////////////////////////////////////////////////////  
//  CommandLineToArgvA
//
// Taken from: http://alter.org.ua/docs/win/args/index.php?lang=en - not
// formatted to IDV/SpeedTree coding standard.

PCHAR* CommandLineToArgvA(PCHAR CmdLine, int* _argc)
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG   len;
	ULONG   argc;
	CHAR   a;
	ULONG   i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = (ULONG) strlen(CmdLine);
	i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
		i + (len+2)*sizeof(CHAR));

	_argv = (PCHAR)(((PUCHAR)argv)+i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	a = CmdLine[i];
	while(a) {
		if(in_QM) {
			if(a == '\"') {
				in_QM = FALSE;
			} else {
				_argv[j] = a;
				j++;
			}
		} else {
			switch(a) {
				case '\"':
					in_QM = TRUE;
					in_TEXT = TRUE;
					if(in_SPACE) {
						argv[argc] = _argv+j;
						argc++;
					}
					in_SPACE = FALSE;
					break;
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					if(in_TEXT) {
						_argv[j] = '\0';
						j++;
					}
					in_TEXT = FALSE;
					in_SPACE = TRUE;
					break;
				default:
					in_TEXT = TRUE;
					if(in_SPACE) {
						argv[argc] = _argv+j;
						argc++;
					}
					_argv[j] = a;
					j++;
					in_SPACE = FALSE;
					break;
			}
		}
		i++;
		a = CmdLine[i];
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}



