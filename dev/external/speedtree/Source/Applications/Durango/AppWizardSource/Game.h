//
// Game.h
//

#pragma once

#include "pch.h"
#include <agile.h>
#include "BasicTimer.h"
#include "MyApplication.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop
ref class Game sealed
{
public:

    Game();

    // Initialization and management
    void Initialize(Windows::UI::Core::CoreWindow^ window);

    // Basic game loop
    void Tick(void);
    void Update(float totalTime, float elapsedTime);
    void Render(void);

    // Rendering helpers
    void Clear(void);
    void Present(void);

	// SpeedTree
	void Destroy(void);

private:

    void CreateDevice(void);
    void CreateResources(void);
	
	// speedtree member variables
	SpeedTree::CMyApplication*						m_pSpeedTreeApp;
	Windows::Xbox::Input::IGamepadReading^			m_iPreviousGamepadReading;

    // Core Application state
    Platform::Agile<Windows::UI::Core::CoreWindow>  m_window;
    Windows::Foundation::Rect                       m_windowBounds;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_depthStencil;

    // Game state
    BasicTimer^                                     m_timer;
};
