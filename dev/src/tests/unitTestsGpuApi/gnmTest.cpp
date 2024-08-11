/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifdef RED_PLATFORM_ORBIS

// Hack
Float GRenderSettingsMipBias = 0.f;

// Initiate those for each render api
GpuApi::SwapChainRef GSwapChain;
GpuApi::TextureRef GBackBuffer;

// Set the maximum crt heap size to 256 mb
RED_MEMORY_SET_MAXIMUM_CRT_SIZE( 256 * 1024 * 1024 );

void SInitializePlatform( const char* commandLine )
{
	RED_UNUSED( commandLine );

	// Setup error system
#ifndef NO_ASSERTS
	extern Red::System::Error::EAssertAction AssertMessageImp( const Char* cppFile, Uint32 line, const Char* expression, const Char* message, const Char* details );
	Red::System::Error::Handler::GetInstance()->RegisterAssertHook( &AssertMessageImp );
	Red::System::Error::Handler::GetInstance()->SetVersion( TXT("version") );
#endif

	// Init GpuApi
	GpuApi::InitEnv();

	GpuApi::InitDevice( 1920, 1080, false, false );

	GpuApi::SwapChainDesc swapChainDesc;
	swapChainDesc.width = 1920;
	swapChainDesc.height = 1080;
	swapChainDesc.fullscreen = true;
	swapChainDesc.overlay = false;

	GSwapChain = GpuApi::CreateSwapChainWithBackBuffer( swapChainDesc );
}

void SShutdownPlatform()
{
	// Deinit GpuApi
	GpuApi::ShutDevice();
}

void* LoadBinaryFromFile( const Char* path, Uint32& codeSize )
{
	Red::IO::CNativeFileHandle file;

	if ( !file.Open( path, Red::IO::eOpenFlag_Read ) )
	{
		RED_HALT( "Missing shader file: '%ls'", path );
		return nullptr;
	}

	const Uint64 fileSize = file.GetFileSize();
	void* code = GPU_API_ALLOCATE( GpuApi::GpuMemoryPool_Shaders, GpuApi::MC_Temporary, fileSize, 16 );
	Uint32 readBytes = 0;
	file.Read( code, fileSize, readBytes );
	codeSize = readBytes;

	return code;
}

void MainLoop()
{
	// Load shaders
	GpuApi::ShaderRef vertexShader;
	GpuApi::ShaderRef pixelShader;
	{
		Uint32 vsCodeSize = 0;
		Uint32 psCodeSize = 0;
		void* vsCode = LoadBinaryFromFile( TXT("/app0/quad_vs.sb"), vsCodeSize );
		void* psCode = LoadBinaryFromFile( TXT("/app0/quad_ps.sb"), psCodeSize );

		vertexShader = GpuApi::CreateShaderFromBinary( GpuApi::VertexShader, vsCode, vsCodeSize );
		pixelShader = GpuApi::CreateShaderFromBinary( GpuApi::PixelShader, psCode, psCodeSize );

		GPU_API_FREE( GpuApi::GpuMemoryPool_Shaders, GpuApi::MC_Temporary, vsCode );
		GPU_API_FREE( GpuApi::GpuMemoryPool_Shaders, GpuApi::MC_Temporary, psCode );
	}

	// Create buffers
	GpuApi::BufferRef vertexBuffer;
	GpuApi::BufferRef vertexBuffer2;
	GpuApi::BufferRef indexBuffer;
	{
		// Create vertex buffer
		typedef struct Vertex
		{
			Float x, y, z;		// Position
			Uint8  r, g, b, a;	// Color
			Float u, v;			// UVs
		} Vertex;

		static const Vertex vertexData[] =
		{
			// 2    3
			// +----+
			// |\   |
			// | \  |
			// |  \ |
			// |   \|
			// +----+
			// 0    1

			//   POSITION                COLOR               UV
			{-0.5f, -0.5f, 0.5f,    255, 0,   0,   1,    0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.5f,    0,	 255, 0,   1,    1.0f, 1.0f},
			{-0.5f,  0.5f, 0.5f,    0,   0,   255, 1,    0.0f, 0.0f},
			{ 0.5f,  0.5f, 0.5f,    100, 0,   100, 1,    1.0f, 0.0f},
		};

		vertexBuffer = GpuApi::CreateBuffer( 4 * sizeof( Vertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, vertexData, 0 );

		static const Vertex vertexData2[] =
		{
			// 2    3
			// +----+
			// |\   |
			// | \  |
			// |  \ |
			// |   \|
			// +----+
			// 0    1

			//   POSITION                COLOR               UV
			{-0.4f, -0.4f, 0.6f,    255, 0,   0,   1,    0.0f, 1.0f},
			{ 0.6f, -0.4f, 0.6f,    0,	 255, 0,   1,    1.0f, 1.0f},
			{-0.4f,  0.6f, 0.6f,    0,   0,   255, 1,    0.0f, 0.0f},
			{ 0.6f,  0.6f, 0.6f,    100, 0,   100, 1,    1.0f, 0.0f},
		};

		vertexBuffer2 = GpuApi::CreateBuffer( 4 * sizeof( Vertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, vertexData2, 0 );

		// Create index buffer
		static const Uint16 indexData[] =
		{
			0, 1, 2,
			1, 3, 2
		};

		indexBuffer = GpuApi::CreateBuffer( 6 * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, indexData, 0 );
	}

	GpuApi::TextureDesc rtDesc;
	rtDesc.format = GpuApi::TEXFMT_R8G8B8A8;
	rtDesc.width = 1280;
	rtDesc.height = 720;
	rtDesc.initLevels = 1;
	rtDesc.msaaLevel = 0;
	rtDesc.sliceNum = 1;
	rtDesc.type = GpuApi::TEXTYPE_2D;
	rtDesc.usage = GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable;
	GpuApi::TextureRef renderTarget = GpuApi::CreateTexture( rtDesc, GpuApi::TEXG_System );

	GpuApi::TextureDesc dsDesc;
	dsDesc.format = GpuApi::TEXFMT_D32F;
	dsDesc.width = 1280;
	dsDesc.height = 720;
	dsDesc.initLevels = 1;
	dsDesc.msaaLevel = 0;
	dsDesc.sliceNum = 1;
	dsDesc.type = GpuApi::TEXTYPE_2D;
	dsDesc.usage = GpuApi::TEXUSAGE_DepthStencil | GpuApi::TEXUSAGE_Samplable;
	GpuApi::TextureRef depthStencil = GpuApi::CreateTexture( dsDesc, GpuApi::TEXG_System );

	while ( true )
	{
		GpuApi::BeginRender();

		GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilFullDepthLE, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );

		// Build rendertarget setups
		GpuApi::RenderTargetSetup rtSetupOffscreen;
		rtSetupOffscreen.SetColorTarget( 0, renderTarget );
		rtSetupOffscreen.SetDepthStencilTarget( depthStencil );
		rtSetupOffscreen.SetViewport( 1280, 720, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetupOffscreen );

		const Float clearColorOffscreen[] = { 0.4f, 1.0f, 0.4f, 1.0f };
		GpuApi::ClearColorTarget( renderTarget, clearColorOffscreen );
		GpuApi::ClearDepthTarget( depthStencil, 1.0f );

		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColorUV );
		Uint32 stride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPosColorUV );
		Uint32 offset = 0;
		GpuApi::BindVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );
		GpuApi::BindIndexBuffer( indexBuffer );

		GpuApi::TextureRef default2DTexture = GpuApi::GetInternalTexture( GpuApi::INTERTEX_DissolvePattern );

		GpuApi::BindTextures( 0, 1, &default2DTexture, GpuApi::PixelShader );

		GpuApi::SetShader( vertexShader, GpuApi::VertexShader );
		GpuApi::SetShader( pixelShader, GpuApi::PixelShader );

		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 4, 0, 2 );

		GpuApi::BindVertexBuffers( 0, 1, &vertexBuffer2, &stride, &offset );
		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 4, 0, 2 );

		// Build rendertarget setups
		GpuApi::RenderTargetSetup rtSetupMain;
		rtSetupMain.SetColorTarget( 0, GpuApi::GetBackBufferTexture() );
		rtSetupMain.SetViewport( 1920, 1080, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetupMain );

		const Float clearColorMain[] = { 0.4f, 1.0f, 0.0f, 1.0f };
		GpuApi::ClearColorTarget( GpuApi::GetBackBufferTexture(), clearColorMain );

		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColorUV );
		Uint32 strideMain = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPosColorUV );
		Uint32 offsetMain = 0;
		GpuApi::BindVertexBuffers( 0, 1, &vertexBuffer, &strideMain, &offsetMain );
		GpuApi::BindIndexBuffer( indexBuffer );

		GpuApi::TextureRef default2DTextureMain = renderTarget;

		GpuApi::BindTextures( 0, 1, &default2DTextureMain, GpuApi::PixelShader );

		GpuApi::SetShader( vertexShader, GpuApi::VertexShader );
		GpuApi::SetShader( pixelShader, GpuApi::PixelShader );

		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 4, 0, 2 );

		GpuApi::EndRender();

		GpuApi::Present( nullptr, nullptr, GSwapChain, false );
	}
}

#endif
