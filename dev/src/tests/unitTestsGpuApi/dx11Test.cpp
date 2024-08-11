/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if defined(RED_PLATFORM_WINPC) || defined(RED_PLATFORM_DURANGO)

// Hack
Float GRenderSettingsMipBias = 0.f;

// Initiate those for each render api
GpuApi::SwapChainRef GSwapChain;

HWND windowHandle;

LONG APIENTRY StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return static_cast< LONG >( DefWindowProc( hWnd, uMsg, (WPARAM)wParam, (LPARAM)lParam ) );
}

void SInitializePlatform( const char* commandLine )
{
	RED_UNUSED( commandLine );

	// Init GpuApi
	GpuApi::InitEnv();

	// Create window and swapchain

	RECT windowRect;
	windowRect.left = 100;
	windowRect.top = 100;
	windowRect.right = 1380;
	windowRect.bottom = 820;

	// Adjust window rectangle with border size

	Uint32 mainStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
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
	::ShowWindow( windowHandle, SW_SHOW );
	::SendMessage( windowHandle, WM_ERASEBKGND, 0, 0 );

	GpuApi::InitDevice( 1280, 720, false, false );

	GpuApi::SwapChainDesc swapChainDesc;
	swapChainDesc.width = 1280;
	swapChainDesc.height = 720;
	swapChainDesc.fullscreen = false;
	swapChainDesc.windowHandle = &windowHandle;
	swapChainDesc.overlay = false;

	GSwapChain = GpuApi::CreateSwapChainWithBackBuffer( swapChainDesc );
}

void SShutdownPlatform()
{
	// Deinit GpuApi
	GpuApi::ShutDevice();
}

void MainLoop()
{
	Float time = 0.f;

	while ( true ) 
	{
		GpuApi::SetBackBufferFromSwapChain( GSwapChain );

		// Build rendertarget setups
		GpuApi::RenderTargetSetup rtSetupMain;
		rtSetupMain.SetColorTarget( 0, GpuApi::GetBackBufferTexture() );
		rtSetupMain.SetViewport( 1280, 720, 0, 0 );

		GpuApi::SetupRenderTargets( rtSetupMain );

		const Float clearColor[] = { 0.1f, 0.15f, 0.1f, 1.0f };
		GpuApi::ClearColorTarget( GpuApi::GetBackBufferTexture(), clearColor );

		{
			static const char* vertexShaderCode = 
				"cbuffer cb0	 																							 \r\n\
				{																											 \r\n\
				row_major float4x4 mWorldViewProj : packoffset(c0);															 \r\n\
				float fTime :                       packoffset(c5.y);														 \r\n\
				};																											 \r\n\
				\r\n\
				cbuffer cb1																									 \r\n\
				{																											 \r\n\
				float4 objectPos;																							 \r\n\
				};																											 \r\n\
				\r\n\
				struct VS_INPUT																								 \r\n\
				{																											 \r\n\
				float4 Position   : POSITION;					 															 \r\n\
				float4 Diffuse    : COLOR0;																					 \r\n\
				float2 Tex		  : TEXCOORD0;																				 \r\n\
				};																											 \r\n\
				\r\n\
				struct VS_OUTPUT																							 \r\n\
				{																											 \r\n\
				float4 Position   : SV_Position;					 															 \r\n\
				float4 Diffuse    : COLOR0;																					 \r\n\
				float2 Tex		  : TEXCOORD0;																				 \r\n\
				};																											 \r\n\
				\r\n\
				VS_OUTPUT vs_main( VS_INPUT input )																			 \r\n\
				{																											 \r\n\
				VS_OUTPUT Output;																						 \r\n\
				\r\n\
				float fSin, fCos;   																					 \r\n\
				float x = length( input.Position ) * sin( fTime ) * 15.0f;												 \r\n\
				\r\n\
				sincos( x, fSin, fCos );																				 \r\n\
				\r\n\
				float4 OutPos = mul( float4( input.Position.x, fSin * 0.1f, input.Position.y, 0.0f ) + float4(objectPos.xyz,1), mWorldViewProj );		 \r\n\
				\r\n\
				Output.Position = OutPos;																				 \r\n\
				Output.Diffuse = 0.5f - 0.5f * fCos;																	 \r\n\
				Output.Tex = input.Tex;																					 \r\n\
				\r\n\
				return Output;																							 \r\n\
				}";

			static const char* pixelShaderCode = 
				"Texture2D txDiffuse;																					 \r\n\
				SamplerState samLinear																					 \r\n\
				{																										 \r\n\
				Filter = MIN_MAG_MIP_LINEAR;																			 \r\n\
				AddressU = Wrap;																						 \r\n\
				AddressV = Wrap;																						 \r\n\
				};																										 \r\n\
				\r\n\
				struct VS_OUTPUT																						 \r\n\
				{																										 \r\n\
				float4 Position   : SV_Position;					 														 \r\n\
				float4 Diffuse    : COLOR0;																				 \r\n\
				float2 Tex		  : TEXCOORD0;																			 \r\n\
				};																										 \r\n\
				\r\n\
				float4 ps_main( VS_OUTPUT input ) : SV_Target															 \r\n\
				{																										 \r\n\
				return /*txDiffuse.Sample( samLinear, input.Tex ) *  */input.Diffuse;									 \r\n\
				}";

			struct VS_CONSTANT_BUFFER
			{
				RedMatrix4x4 mWorldViewProj;      //mWorldViewProj will probably be global to all shaders in a project.
				//It's a good idea not to move it around between shaders.
				RedVector4 vSomeVectorThatMayBeNeededByASpecificShader;
				float fSomeFloatThatMayBeNeededByASpecificShader;
				float fTime;                    //fTime may also be global to all shaders in a project.
				float fSomeFloatThatMayBeNeededByASpecificShader2;
				float fSomeFloatThatMayBeNeededByASpecificShader3;
			};

			static GpuApi::ShaderRef vertexShader;
			static GpuApi::ShaderRef pixelShader;
			static GpuApi::BufferRef indexBuffer;
			static GpuApi::BufferRef vertexBuffer;
			static Uint32 stride = 0;
			static Uint32 offset = 0;
			static GpuApi::TextureRef g_pTextureRV;
			//static CRenderTexture* renderResource;

#define VERTS_PER_EDGE 64
			static Uint32                       g_dwNumVertices = VERTS_PER_EDGE * VERTS_PER_EDGE;
			static Uint32 g_dwNumIndices = 6 * ( VERTS_PER_EDGE - 1 ) * ( VERTS_PER_EDGE - 1 );

			if (vertexShader.isNull())
			{
				vertexShader = GpuApi::CreateShaderFromSource(GpuApi::VertexShader, vertexShaderCode, "vs_main", "vs_4_0", nullptr, 0, "vertexShader.hlsl");
				pixelShader = GpuApi::CreateShaderFromSource(GpuApi::PixelShader, pixelShaderCode, "ps_main", "ps_4_0", nullptr, 0, "pixelShader.hlsl");

				//static CGatheredResource resWayPointIcon( TXT("engine\\textures\\editor\\checker_d.xbm"), RGF_NotCooked );
				//CBitmapTexture* bitmapTexture = resWayPointIcon.LoadAndGet< CBitmapTexture >();

				//renderResource = static_cast< CRenderTexture* >( GRender->UploadTexture( bitmapTexture ) );
				//g_pTextureRV = renderResource->GetTextureRef();

				// Create index buffer
				Uint16* pIndexData = new Uint16[ g_dwNumIndices ];
				Uint16* pIndices = pIndexData;
				for( Uint16 y = 1; y < VERTS_PER_EDGE; y++ )
				{
					for( Uint32 x = 1; x < VERTS_PER_EDGE; x++ )
					{
						*pIndices++ = ( Uint16 )( ( y - 1 ) * VERTS_PER_EDGE + ( x - 1 ) );
						*pIndices++ = ( Uint16 )( ( y - 0 ) * VERTS_PER_EDGE + ( x - 1 ) );
						*pIndices++ = ( Uint16 )( ( y - 1 ) * VERTS_PER_EDGE + ( x - 0 ) );

						*pIndices++ = ( Uint16 )( ( y - 1 ) * VERTS_PER_EDGE + ( x - 0 ) );
						*pIndices++ = ( Uint16 )( ( y - 0 ) * VERTS_PER_EDGE + ( x - 1 ) );
						*pIndices++ = ( Uint16 )( ( y - 0 ) * VERTS_PER_EDGE + ( x - 0 ) );
					}
				}

				indexBuffer = GpuApi::CreateBuffer( g_dwNumIndices * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, pIndexData, GpuApi::BCC_Index16Bit );
				delete [] pIndexData;

				// Create vertex buffer
				struct SCENE_VERTEX
				{
					float pos[3];
					GpuApi::Color color;
					float tex[2];
				};

				stride = sizeof( SCENE_VERTEX );

				SCENE_VERTEX* pVertData = new SCENE_VERTEX[ g_dwNumVertices ];
				SCENE_VERTEX* pVertices = pVertData;
				for( Uint32 y = 0; y < VERTS_PER_EDGE; y++ )
				{
					for( Uint32 x = 0; x < VERTS_PER_EDGE; x++ )
					{
						pVertices->pos[0] = ( ( float )x / ( float )( VERTS_PER_EDGE - 1 ) - 0.5f ) * M_PI;
						pVertices->pos[1] = ( ( float )y / ( float )( VERTS_PER_EDGE - 1 ) - 0.5f ) * M_PI;
						pVertices->pos[2] = 0;

						//pVertices->color = Color( Vector( ( float )x / ( float )( VERTS_PER_EDGE - 1 ), 0.0f, 0.0f, 1.0f ) );
						//pVertices->color = Color::BLACK;

						pVertices->tex[0] = ( float )x / ( float )( VERTS_PER_EDGE - 1 );
						pVertices->tex[1] = ( float )y / ( float )( VERTS_PER_EDGE - 1 );
						*pVertices++;
					}
				}
				vertexBuffer = GpuApi::CreateBuffer( g_dwNumVertices * sizeof( SCENE_VERTEX ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, pVertData, GpuApi::BCC_Vertex );
				delete [] pVertData;
			}

			RedVector4 timeVec;
			timeVec.Y = time;
			time += 1.f/30000.f;

			RedMatrix4x4 axesConversion( RedVector4( 1,0,0,0), RedVector4( 0,0,1,0), RedVector4( 0,1,0,0), RedVector4( 0,0,0,1 ) );
			RedMatrix4x4 rotMatrix = RedMatrix4x4::IDENTITY;
			RedMatrix4x4 transMatrix = RedMatrix4x4::IDENTITY;
			RedMatrix4x4 worldToCamera = transMatrix * rotMatrix;
			RedMatrix4x4 worldToView = worldToCamera * axesConversion;

			RedMatrix4x4 viewToScreen;
			viewToScreen.BuildPerspectiveLH( DEG2RAD(70.f), 1.65426695f, 0.2f, 1900.f );
			//viewToScreen.BuildOrthoLH( 16, 9, 0.01f, 1000.f );

			RedMatrix4x4 worldToScreen = worldToView * viewToScreen;

			GpuApi::SetVertexShaderConstF(0, worldToScreen.AsFloat(), 4);
			GpuApi::SetVertexShaderConstF(5, &(timeVec.V[0]), 1);
			//GpuApi::BindSampler(0, g_pTextureRV);
			//renderResource->Bind( 0, RST_PixelShader );

			GpuApi::SetShader( vertexShader, GpuApi::VertexShader );
			GpuApi::SetShader( pixelShader, GpuApi::PixelShader );

			GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_SimpleNoCull, 1 );

			GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColorUV );
			GpuApi::BindVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			GpuApi::BindIndexBuffer( indexBuffer );

			RedVector4 objectPos(0, 3, 0, 0);
			GpuApi::SetVertexShaderConstF(30, &objectPos.X, 1);
			GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 0, 0, g_dwNumIndices / 3 );

			objectPos.Y += 10;
			GpuApi::SetVertexShaderConstF(30, &objectPos.X, 1);
			GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 0, 0, g_dwNumIndices / 3 );
		}

		GpuApi::Present( nullptr, nullptr, GSwapChain, false );
	}
}

#endif
