//
// Game.cpp -
//

#include "pch.h"
#include "Game.h"

using namespace Windows::Xbox::Input;
using namespace Windows::Foundation::Collections;

#define SPEEDTREE_USE_ALLOCATOR_INTERFACE // must be active for Debug MT and Release MT builds
#define SPEEDTREE_OVERRIDE_FILESYSTEM
#include "MyCustomAllocator.h"
#include "MyFileSystem.h"

using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  SpeedTree file-scope variables

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

// Constructor.
Game::Game( )
{
	// speedtre-specifics
	m_pSpeedTreeApp = st_new(CMyApplication, "CMyApplication");
}


void Game::Destroy(void)
{
	m_pSpeedTreeApp->ReleaseGfxResources( );
	st_delete<CMyApplication>(m_pSpeedTreeApp);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(Windows::UI::Core::CoreWindow^ window)
{
    m_window = window;

	if (m_pSpeedTreeApp->ParseCmdLine(1, NULL))
	{
		if (m_pSpeedTreeApp->Init( ))
		{
			CreateDevice( );
			CreateResources( );

			m_timer = ref new BasicTimer( );

			m_pSpeedTreeApp->InitGfx( );
		}
	}
}


// Executes basic game loop.
void Game::Tick( )
{
    m_timer->Update( );

    Update(m_timer->Total, m_timer->Delta);

    Render( );
}

inline static float NormalizeThumbstickValue(float fThumbstickValue, float fDeadZone)
{
	float fNormalizedValue = 0.0f;
	
	if (fThumbstickValue > +fDeadZone)
	{
		fNormalizedValue = (fThumbstickValue - fDeadZone) / (1.0f - fDeadZone);
	}

	if (fThumbstickValue < -fDeadZone)
	{
		fNormalizedValue = (fThumbstickValue + fDeadZone) / (1.0f - fDeadZone);
	}

	return fNormalizedValue;
}

// Updates the world
void Game::Update(float fTotalTime, float fElapsedTime)
{
	ST_UNREF_PARAM(fTotalTime);

	const float c_fTriggerThreshhold = 0.1f;
	const float c_fMoveStandardSpeed = 40.0f;
	const float c_fMoveQuickSpeed = 160.0f;
	const float c_fDeadZone = 0.25f;
	
	// process gamepad
    IVectorView<IGamepad^>^ gamepads = Gamepad::Gamepads;

    if (gamepads->Size > 0)
    {
        IGamepadReading^ reading = gamepads->GetAt(0)->GetCurrentReading( );

		// check for exit
        if (reading->IsMenuPressed)
        {
			Destroy( );
            Windows::ApplicationModel::Core::CoreApplication::Exit( );
        }
		else
		{
			// get stick values
			const st_float32 c_fThumbLeftX = NormalizeThumbstickValue(reading->LeftThumbstickX, c_fDeadZone);
			const st_float32 c_fThumbLeftY = NormalizeThumbstickValue(reading->LeftThumbstickY, c_fDeadZone);
			const st_float32 c_fThumbRightX = NormalizeThumbstickValue(reading->RightThumbstickX, c_fDeadZone);
			const st_float32 c_fThumbRightY = NormalizeThumbstickValue(reading->RightThumbstickY, c_fDeadZone);

			// check for navigation or light-adjust
			if (reading->LeftTrigger < c_fTriggerThreshhold)
			{
				// navigation
				CMyNavigationBase* pNav = m_pSpeedTreeApp->GetNavigation( );
				assert(pNav);

				float fMoveAmt = m_pSpeedTreeApp->GetConfig( ).m_sNavigation.m_fSpeedScalar * (reading->RightTrigger > c_fTriggerThreshhold ? c_fMoveQuickSpeed : c_fMoveStandardSpeed);

				pNav->AdjustAzimuth(-c_fThumbRightX * fElapsedTime * 2.0f);
				pNav->AdjustPitch(c_fThumbRightY * fElapsedTime * 2.0f);
				pNav->MoveForward(c_fThumbLeftY * fElapsedTime * fMoveAmt);
				pNav->Strafe(-c_fThumbLeftX * fElapsedTime * fMoveAmt);
			}
			else
			{
				// adjust light direction
				if (fabs(c_fThumbRightX) > fabs(c_fThumbRightY))
					m_pSpeedTreeApp->GetLightAdjuster( ).m_fHorzAngle -= c_fThumbRightX * fElapsedTime * 2.0f;
				else
					m_pSpeedTreeApp->GetLightAdjuster( ).m_fVertAngle += c_fThumbRightY * fElapsedTime * 2.0f;
				m_pSpeedTreeApp->GetLightAdjuster( ).ComputeDirection( );
				m_pSpeedTreeApp->UpdateLightDir( );
			}
		}

		// get button pressed values
		const st_bool c_bButtonPressedA = reading->IsAPressed && !m_iPreviousGamepadReading->IsAPressed;
		const st_bool c_bButtonPressedB = reading->IsBPressed && !m_iPreviousGamepadReading->IsBPressed;
		const st_bool c_bButtonPressedX = reading->IsXPressed && !m_iPreviousGamepadReading->IsXPressed;
		const st_bool c_bButtonPressedY = reading->IsYPressed && !m_iPreviousGamepadReading->IsYPressed;
		const st_bool c_bButtonPressedLS = reading->IsLeftShoulderPressed && !m_iPreviousGamepadReading->IsLeftShoulderPressed;

		// toggle geometry types
		if (c_bButtonPressedA)
			m_pSpeedTreeApp->ToggleTrees( );
		if (c_bButtonPressedY)
			m_pSpeedTreeApp->ToggleBillboards( );
		if (c_bButtonPressedX)
			m_pSpeedTreeApp->ToggleGrass( );
		#ifdef MY_TERRAIN_ACTIVE
			if (c_bButtonPressedB)
				m_pSpeedTreeApp->ToggleTerrain( );
			if (c_bButtonPressedLS)
				m_pSpeedTreeApp->ToggleTerrainFollowing( );
		#endif
				
		m_iPreviousGamepadReading = reading;
    }

    // speedtree refapp update code
	if (m_pSpeedTreeApp->ReadyToRender( ))
	{
		// update the scene before the render; this can be outside of the render thread/function, but must be
		// completed before the render can proceed
		m_pSpeedTreeApp->Advance( );
		m_pSpeedTreeApp->Cull( );
		m_pSpeedTreeApp->ReportStats( );
	}
}


// Draws the scene
void Game::Render( )
{
    Clear( );

    // speedtree refapp render code
	if (m_pSpeedTreeApp->ReadyToRender( ))	
	{
		m_pSpeedTreeApp->Render( );
	}

    Present( );
}


// Helper method to clear the backbuffers
void Game::Clear( )
{
    // Clear the views
    const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.000f };
    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf( ), m_depthStencilView.Get( ));
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get( ), clearColor);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get( ), D3D11_CLEAR_DEPTH, 1.0f, 0);
}


// Presents the backbuffer contents to the screen
void Game::Present( )
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(0, 0); // todo: (1, 0) for vsync

    // If the device was removed either by a disconnect or a driver upgrade, we
    // must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        Initialize(m_window.Get( ));
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}


// These are the resources that depend on the device.
void Game::CreateDevice( )
{
    // This flag adds support for surfaces with a different color channel ordering than the API default.
    // It is recommended usage, and is required for compatibility with Direct2D.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	#ifdef _DEBUG
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

    // This array defines the set of DirectX hardware feature levels this app will support.
    // Note the ordering should be preserved.
    // Don't forget to declare your application's minimum required feature level in its
    // description.  All applications are assumed to support 9.1 unless otherwise stated.
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0
    };

    // Create the DX11 API device object, and get a corresponding context.
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

    DX::ThrowIfFailed(
        D3D11CreateDevice(
            nullptr,                    // specify null to use the default adapter
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,                    // leave as nullptr unless software device
            creationFlags,              // optionally set debug and Direct2D compatibility flags
            featureLevels,              // list of feature levels this app can support
            ARRAYSIZE(featureLevels),   // number of entries in above list
            D3D11_SDK_VERSION,          // always set this to D3D11_SDK_VERSION
            &device,                    // returns the Direct3D device created
            &m_featureLevel,            // returns feature level of device created
            &context                    // returns the device immediate context
            )
        );

    // Get the DirectX11.1 device by QI off the DirectX11 one.
    DX::ThrowIfFailed(device.As(&m_d3dDevice));

    // And get the corresponding device context in the same way.
    DX::ThrowIfFailed(context.As(&m_d3dContext));

	// pass devices into SpeedTree renderer
	SpeedTree::DX11::SetDevice(device.Get( ));
	SpeedTree::DX11::SetDeviceContext(context.Get( ));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources( )
{
    // Store the window bounds so the next time we get a SizeChanged event we can
    // avoid rebuilding everything if the size is identical.
    m_windowBounds = m_window.Get( )->Bounds;

    // If the swap chain already exists, resize it,
    // otherwise create one.
    if(m_swapChain != nullptr)
    {
        DX::ThrowIfFailed(m_swapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0));
    }
    else
    {
       // First, retrieve the underlying DXGI Device from the D3D Device
        Microsoft::WRL::ComPtr<IDXGIDevice1>  dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

        // And obtain the factory object that created it.
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = 1920;
        swapChainDesc.Height = 1080;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Flags = 0;

        // Create a SwapChain from a CoreWindow.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(m_d3dDevice.Get( ), reinterpret_cast<IUnknown*>(m_window.Get( )), &swapChainDesc, nullptr, &m_swapChain));

        // Ensure that DXGI does not queue more than one frame at a time. This both reduces
        // latency and ensures that the application will only render after each VSync, minimizing
        // power consumption.
        //DX::ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

    // Create a view interface on the rendertarget to use on bind.
	D3D11_RENDER_TARGET_VIEW_DESC sRenderTargetViewDesc;
	ZeroMemory(&sRenderTargetViewDesc, sizeof(sRenderTargetViewDesc));
	sRenderTargetViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sRenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	sRenderTargetViewDesc.Texture2D.MipSlice = 0;
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get( ), &sRenderTargetViewDesc, &m_renderTargetView));

    // Cache the rendertarget dimensions in our helper class for convenient use.
    D3D11_TEXTURE2D_DESC backBufferDesc = {0};
    backBuffer->GetDesc(&backBufferDesc);

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, 
										   backBufferDesc.Width, 
										   backBufferDesc.Height, 
										   1, 
										   1, 
										   D3D11_BIND_DEPTH_STENCIL,
										   D3D11_USAGE_DEFAULT,
										   0,
										   backBufferDesc.SampleDesc.Count,
										   backBufferDesc.SampleDesc.Quality);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get( ), &depthStencilViewDesc, &m_depthStencilView));

    // Create a viewport descriptor of the full window size.
    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(backBufferDesc.Width), static_cast<float>(backBufferDesc.Height));

    // Set the current viewport using the descriptor.
    m_d3dContext->RSSetViewports(1, &viewPort);
}
