/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "testEngine.h"

// Tests
#include "runtimeTest.h"
#include "drawIndexedPrimitiveTest.h"
#include "constantBuffersTest.h"
#include "viewportTest.h"
#include "drawPrimitiveTest.h"
#include "drawPrimitiveRawTest.h"
#include "drawsInstancedTest.h"
#include "drawIndexedPrimitiveRawTest.h"
#include "drawSystemPrimitiveTest.h"
#include "drawsInstancedNoBuffersTest.h"
#include "samplerTest.h"
#include "rasterizerStateTest.h"
#include "blendStateTest.h"
#include "simpleCullTest.h"
#include "fenceManagerTest.h"
#include "bufferUpdatesTest.h"

#define MAX_TESTS 32

static CTestUtils GTestUtils;

//----------------------------- Test Engine ----------------------------------//

CTestEngine::CTestEngine( const CommandParameters& params, const GpuApi::SwapChainRef& swapChainRef )
	: m_params( params )
	, m_swapChainRef( swapChainRef )
	, m_clearColor( 0.0f, 0.02f, 0.02f, 1.0f )
	, m_exitValue( EXIT_SUCCESS )
	, m_currentTest( 0 )
{

}

CTestEngine::~CTestEngine()
{
	for( CBaseTest* t : m_tests )
	{
		delete t;
	}
}

Bool CTestEngine::Initialize()
{
	GpuApi::TextureDesc rtDesc;
	rtDesc.format = GpuApi::TEXFMT_R8G8B8A8;
	rtDesc.width = DEVICE_WIDTH;
	rtDesc.height = DEVICE_HEIGHT;
	rtDesc.initLevels = 1;
	rtDesc.msaaLevel = 0;
	rtDesc.sliceNum = 1;
	rtDesc.type = GpuApi::TEXTYPE_2D;
	rtDesc.usage = GpuApi::TEXUSAGE_RenderTarget;
	m_renderTargetTexture = GpuApi::CreateTexture( rtDesc, GpuApi::TEXG_System );
	if( !m_renderTargetTexture )
	{
		ERR_GAT( TXT( "Failed to initialize render target texture!" ) );
		return false;
	}

	GpuApi::TextureDesc dsDesc;
	dsDesc.format = GpuApi::TEXFMT_D32F;
	dsDesc.width = DEVICE_WIDTH;
	dsDesc.height = DEVICE_HEIGHT;
	dsDesc.initLevels = 1;
	dsDesc.msaaLevel = 0;
	dsDesc.sliceNum = 1;
	dsDesc.type = GpuApi::TEXTYPE_2D;
	dsDesc.usage = GpuApi::TEXUSAGE_DepthStencil;
	m_depthStencilTexture = GpuApi::CreateTexture( dsDesc, GpuApi::TEXG_System );
	if( !m_depthStencilTexture )
	{
		ERR_GAT( TXT( "Failed to initialize depth stencil texture!" ) );
		return false;
	}

	// Initialize drawable objects and effects inside
	if( !GTestUtils.Initialize() )
	{
		ERR_GAT( TXT( "Failed to initialize test utils!" ) );
		return false;
	}

	return RegisterTests();
}

Bool CTestEngine::RegisterTests()
{
	// Clang doesn't want to swallow string initializers passed as constructor params
	#define TEST_INIT( name, _class )			  \
		{										   \
			String test = String( TXT(name) );      \
			m_tests.PushBack( new _class( test ));   \
		}

	m_tests.Reserve( MAX_TESTS );

	TEST_INIT( "FLICKERING_ILLUSION",        CRuntimeTest				  );
	TEST_INIT( "CONSTANT_BUFFERS",           CConstantBuffersTest		  );
	TEST_INIT( "VIEWPORTS",                  CViewportTest				  );
	TEST_INIT( "DRAW_PRIMITIVE",             CDrawPrimitiveTest			  );
	TEST_INIT( "DRAW_PRIMITIVE_RAW",         CDrawPrimitiveRawTest		  );
	TEST_INIT( "DRAW_INDEXED_PRIMITIVE",     CDrawsIndexedTest			  );
	TEST_INIT( "DRAW_INDEXED_PRIMITIVE_RAW", CDrawIndexedPrimitiveRawTest );
	TEST_INIT( "DRAW_SYSTEM_PRIMITIVE",		 CDrawSystemPrimitiveTest	  );
	TEST_INIT( "DRAW_INSTANCED",			 CDrawInstancedTest			  );
	TEST_INIT( "DRAW_INSTANCED_NO_BUFFER",   CDrawInstancedNoBuffersTest  );
	TEST_INIT( "SAMPLER_TEXTURE",			 CSamplerTest				  );
	TEST_INIT( "RASTERIZER_STATE",			 CRasterizerStateTest		  );
	TEST_INIT( "BLEND_STATE",				 CBlendStateTest			  );
	//TEST_INIT( "SIMPLE_CULL",				 CSimpleCullTest			  );
	TEST_INIT( "PS4_FENCE_MANAGER",			 CFenceManagerTest			  );
	TEST_INIT( "BUFFER_UPDATES",			 CBufferUpdatesTest			  );

	for( CBaseTest* t : m_tests )
	{
		if( !t->Initialize() )
		{
			ERR_GAT( TXT( "[%ls] INITIALIZATION FAILURE" ), t->GetName().AsChar() );
			m_exitValue = EXIT_FAILURE;
			return false;
		}
		else
		{
			LOG_GAT( TXT( "[%ls] INITIALIZATION SUCCESS" ), t->GetName().AsChar() );
		}
	}

	return true;
}

int CTestEngine::Start()
{
	if( !m_params.m_test.Empty() )
	{
		while( m_currentTest < m_tests.SizeInt() && m_tests[m_currentTest]->GetName() != m_params.m_test )
		{
			m_currentTest++;
		}
	}

	if( m_currentTest == m_tests.Size() )
	{
		ERR_GAT( TXT( "[%ls] TEST WITH THAT NAME DOESN'T EXIST." ), m_params.m_test.AsChar() );
		return EXIT_FAILURE;
	}

	m_params.m_windowed ? RuntimeLoop() : SingleLoop();

	return m_exitValue;
}

void CTestEngine::SetRenderTargetBackBuffer()
{
	//Build rendertarget setups ( runtime test renders to backbuffer )
	// PC DX11 doesn't seem to create a backbuffer depth texture, so use the one we've created for ourselves.
	GpuApi::RenderTargetSetup rtSetupMain;
	rtSetupMain.SetColorTarget( 0, GpuApi::GetBackBufferTexture() );
	rtSetupMain.SetDepthStencilTarget( m_depthStencilTexture );
	rtSetupMain.SetViewportFromTarget( GpuApi::GetBackBufferTexture() );
	GpuApi::SetupRenderTargets( rtSetupMain );
}

void CTestEngine::SetRenderTargetTexture()
{
	GpuApi::RenderTargetSetup rtSetupOffscreen;
	rtSetupOffscreen.SetColorTarget( 0, m_renderTargetTexture );
	rtSetupOffscreen.SetDepthStencilTarget( m_depthStencilTexture );
	rtSetupOffscreen.SetViewportFromTarget( m_renderTargetTexture );
	GpuApi::SetupRenderTargets( rtSetupOffscreen );
}

Bool CTestEngine::LoadTextureFromFile( const Char* path, GpuApi::TextureRef& outRef )
{
	Red::IO::CNativeFileHandle file;
	if ( !file.Open( path, Red::IO::eOpenFlag_Read ) )
	{
		ERR_GAT( TXT("Missing reference file: '%ls'"), path );
		return false;
	}

	GpuApi::Uint64 siTextureFileSize = file.GetFileSize();
	if (siTextureFileSize > 0)
	{
		Uint8* pTextureBuffer = new Uint8[siTextureFileSize];
		Uint32 readBytes;
		file.Read( pTextureBuffer, static_cast< GpuApi::Uint32 > ( siTextureFileSize ), readBytes );
		if ( !pTextureBuffer )
		{
			ERR_GAT( TXT( "[%ls] Couldn't read data of the reference file!" ), path );
			return false;
		}

		outRef = GpuApi::CreateTextureFromMemoryFile( pTextureBuffer, static_cast< GpuApi::Uint32 >( siTextureFileSize ), GpuApi::TEXG_Generic );
		if( !outRef )
		{
			ERR_GAT( TXT( "[%ls] Couldn't create texture out of the reference file!" ), path );
			return false;
		}
		delete[] pTextureBuffer;
	}

	return true;
}

void CTestEngine::SaveTextureToFile( GpuApi::TextureRef& ref, const String& testName )
{
#if defined(RED_PLATFORM_WINPC)
	GpuApi::SaveTextureToFile( ref, String::Printf( TXT( "%ls/%ls.dds" ), SHADERS_PATH, testName.AsChar() ).AsChar(), GpuApi::SAVE_FORMAT_DDS );
#else
	ERR_GAT( TXT( "Saving texture to file only possible on WINPC platform." ) );
#endif
}

Bool CTestEngine::CompareWithReferenceFile( const String& testName, Char* errorMsg )
{
	Char path[256];
	Red::System::SNPrintF( path, ARRAY_COUNT( path ), TXT("%ls/%ls.dds"), SHADERS_PATH, testName.AsChar() );

	GpuApi::TextureRef refTex;
	if( !LoadTextureFromFile( path, refTex ) )
	{
		Red::System::SNPrintF( errorMsg, 256, TXT("Reference file can't be loaded from %ls"), path );
		return false;
	}

	const GpuApi::TextureDesc targetDesc = GpuApi::GetTextureDesc( m_renderTargetTexture );
	const GpuApi::TextureDesc refDesc = GpuApi::GetTextureDesc( refTex ); 

	if( targetDesc.width != refDesc.width || targetDesc.height != refDesc.height )
	{
		Red::System::SNPrintF( errorMsg, 256, TXT("Target texture is not the same size as reference texture!") );
		return false;
	}

	const Uint32 dataSize = 4 * refDesc.width * refDesc.height;
	Uint8* refData = new Uint8[ dataSize ];
	Uint8* targetData = new Uint8[ dataSize ];

	GpuApi::GrabTexturePixels( refTex, 0, 0, refDesc.width, refDesc.height, refData, 4, false );
	GpuApi::GrabTexturePixels( m_renderTargetTexture, 0, 0, targetDesc.width, targetDesc.height, targetData, 4, false );

	Uint8 error = (Uint8)(m_params.m_marginOfError * NumericLimits< Uint8 >::Max());
	for( Uint32 i = 0; i < dataSize; ++i )
	{
		if ( Abs(refData[i] - targetData[i]) > error )
		{
			Uint32 pixelIndex1D = i / 4;
			Uint32 y = pixelIndex1D / refDesc.width;
			Uint32 x = pixelIndex1D - refDesc.width * y;
			Red::System::SNPrintF( errorMsg, 256, TXT("Render target differs from reference texture! Spotted at pixel (%d, %d)"), x, y );
			return false;
		}
	}
	GpuApi::SafeRelease(refTex);
	delete[] refData;
	delete[] targetData;

	return true;
}

#if defined(RED_PLATFORM_WINPC)
void CTestEngine::RuntimeLoop()
{
	MSG msg = { 0 };
	RunTests();

	while ( msg.message != WM_QUIT )
	{
		if ( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
		{
			if ( msg.message == WM_KEYUP )
			{
				switch( msg.wParam )
				{
				case VK_LEFT:
					m_currentTest = m_currentTest - 1;
					if(m_currentTest < 0)
						m_currentTest += m_tests.Size();
					break;
				case VK_RIGHT:
					m_currentTest = (m_currentTest + 1) % m_tests.Size();
					break;
				}
			}
			else
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		RunTests();
	}
}
#elif defined(RED_PLATFORM_ORBIS)
#include "inputInitializationOrbis.h"
void CTestEngine::RuntimeLoop()
{
	CInputInitializerOrbis input;
	input.Init();
	RunTests();

	while ( true )
	{
		input.ReadInput();
		if( input.ButtonPressed( SCE_PAD_BUTTON_CROSS ) || input.ButtonPressed( SCE_PAD_BUTTON_RIGHT ) )
		{
			m_currentTest = (m_currentTest + 1) % m_tests.Size();
		}
		else if( input.ButtonPressed( SCE_PAD_BUTTON_CIRCLE ) || input.ButtonPressed( SCE_PAD_BUTTON_LEFT ) )
		{
			m_currentTest = m_currentTest - 1;
			if(m_currentTest < 0)
				m_currentTest += m_tests.Size();
		}
		RunTests();
	}

	input.Shutdown();
}
#elif defined(RED_PLATFORM_DURANGO)
#include "inputInitializationDurango.h"
void CTestEngine::RuntimeLoop()
{
	CInputInitializerDurango input;
	input.Init();
	RunTests();

	while( true )
	{
		input.ReadInput();
		if( input.ButtonPressed( static_cast<Uint32>(GamepadButtons::A) ) || input.ButtonPressed( static_cast<Uint32>(GamepadButtons::DPadRight) ) )
		{
			m_currentTest = (m_currentTest + 1) % m_tests.Size();
		}
		else if( input.ButtonPressed( static_cast<Uint32>(GamepadButtons::B) ) || input.ButtonPressed( static_cast<Uint32>(GamepadButtons::DPadLeft) ) )
		{
			m_currentTest = m_currentTest - 1;
			if(m_currentTest < 0)
				m_currentTest += m_tests.Size();
		}
		RunTests();
	}

	input.Shutdown();
}
#endif

void CTestEngine::SingleLoop()
{
	if ( m_params.m_test.Empty() )
	{
		while( m_currentTest < m_tests.SizeInt() )
		{
			RunTests();
			m_currentTest++;
		}
	}
	else
	{
		RunTests();
	}
}

void CTestEngine::RunTests()
{
	CBaseTest* t = m_tests[m_currentTest];

	ResetDevice();

	if( !m_params.m_windowed )
	{
		GpuApi::BeginRender();
		SetRenderTargetTexture();
		ResetViewport();
		GpuApi::ClearColorTarget( m_renderTargetTexture, &(m_clearColor.X) );
		GpuApi::ClearDepthTarget( m_depthStencilTexture, 1.0f );
		if ( !t->Execute( GTestUtils ) )
		{
			ERR_GAT( TXT( "[%ls] EXECUTION FAILURE" ), t->GetName().AsChar() );
			m_exitValue = EXIT_FAILURE;
		}
		GpuApi::Present( nullptr, nullptr, m_swapChainRef, false, 0 );
		GpuApi::EndRender();
	}

	if( m_params.m_windowed )
	{
		GpuApi::BeginRender();
		SetRenderTargetBackBuffer();
		//ResetViewport();
		GpuApi::ClearColorTarget( GpuApi::GetBackBufferTexture(), &(m_clearColor.X) );
		// As with in SetRenderTargetBackBuffer, PC DX11 doesn't have a backbuffer depth texture
		GpuApi::ClearDepthTarget( m_depthStencilTexture, 1.0f );
		if ( !t->Execute( GTestUtils ) )
		{
			ERR_GAT( TXT( "[%ls] EXECUTION FAILURE" ), t->GetName().AsChar() );
			m_exitValue = EXIT_FAILURE;
		}
		GpuApi::Present( nullptr, nullptr, m_swapChainRef, false, 0 );
		GpuApi::EndRender();
	}

	if ( m_params.m_saveReferences )
	{
		SaveTextureToFile( m_renderTargetTexture, t->GetName() );
	}
	else if( !m_params.m_windowed )
	{
		Char errorMsg[256] = {0};
		if( !CompareWithReferenceFile( t->GetName(), errorMsg ) )
		{
			ERR_GAT( TXT( "[%ls] FAILURE : %ls" ), t->GetName().AsChar(), errorMsg );
			m_exitValue = EXIT_FAILURE;
		}
		else
		{
			LOG_GAT( TXT( "[%ls] PASS" ), t->GetName().AsChar() );
		}
	}

	return;
}

void CTestEngine::ResetViewport()
{
	GpuApi::ViewportDesc desc;
	desc.x = 0; desc.y = 0; desc.width = DEVICE_WIDTH; desc.height = DEVICE_HEIGHT;
	GpuApi::SetViewport(desc);
}

void CTestEngine::ResetDevice()
{
	GpuApi::BindTextures( 0, GpuApi::MAX_PS_SAMPLERS, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 0, GpuApi::MAX_VS_SAMPLERS, nullptr, GpuApi::VertexShader );
	GpuApi::SetSamplerStateCommon( 0, GpuApi::MAX_PS_SAMPLER_STATES, GpuApi::SAMPSTATEPRESET_WrapPointNoMip, GpuApi::PixelShader );
	GpuApi::SetSamplerStateCommon( 0, GpuApi::MAX_VS_SAMPLER_STATES, GpuApi::SAMPSTATEPRESET_WrapPointNoMip, GpuApi::VertexShader );
	GpuApi::SetShader( GpuApi::ShaderRef::Null(), GpuApi::VertexShader );
	GpuApi::SetShader( GpuApi::ShaderRef::Null(), GpuApi::PixelShader );
	GpuApi::BindIndexBuffer( GpuApi::BufferRef::Null() );
}

//----------------------------- Base Test ----------------------------------//

CBaseTest::CBaseTest( String& name )
	: m_stride( sizeof(SCENE_VERTEX) )
	, m_offset( 0 )
	, m_time( 0.0f )
	, m_testName( name )
{
}

CBaseTest::~CBaseTest()
{
}

RedMatrix4x4 CBaseTest::LookAtLH( RedVector3 eye, RedVector3 target, RedVector3 up )
{
	RedVector3 zaxis = Sub(target, eye).Normalize();      // The "forward" vector.
	RedVector3 xaxis = Cross(up, zaxis);    // The "right" vector.
	RedVector3 yaxis = Cross(zaxis, xaxis);	// The "up" vector.

	// Create a 4x4 view matrix from the right, up, forward and eye position vectors
	RedMatrix4x4 viewMatrix(
		RedVector4(      xaxis.X,            yaxis.X,            zaxis.X,       0 ),
		RedVector4(      xaxis.Y,            yaxis.Y,            zaxis.Y,       0 ),
		RedVector4(      xaxis.Z,            yaxis.Z,            zaxis.Z,       0 ),
		RedVector4(- Dot( xaxis, eye ), -Dot( yaxis, eye ), - Dot( zaxis, eye ),  1 ) 
		);

	return viewMatrix;
}