
#include "build.h"
#include "renderSimWaterFFT.h"
#include "renderShaderPair.h"
#include "renderShader.h"
#include "renderRenderSurfaces.h"
#include "../redMath/float16compressor.h"
#include "../engine/globalWaterUpdateParams.h"

#define PAT_PI 3.1415926535897932384626433f
#define PAT_SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr
#define HALF 0.5f
#define PAT_2PI 6.2831853071795864769252866f
#define PAT_G 9.81f

#define TILE_SIZE 16

namespace Config
{
	TConfigVar< Bool >	cvUseComputeFFT(		"WaterSimulation", "UseComputeFFT",			true );
	TConfigVar< Bool >	cvUseAsyncComputeFFT(	"WaterSimulation", "UseAsyncComputeFFT",	true );
}

CRenderSimWaterFFT::CRenderSimWaterFFT() 
	: m_order_texture( NULL )
	, m_indices_texture( NULL )
	, m_output( NULL )
	, m_readBackTexRes( WATER_RESOLUTION )
	, m_stagingFlip( 0 )
	, m_resolution( 0 )
	, m_steps( 0 )
	, m_time( EngineTime::GetNow() )
	, m_stageHelper(NULL)
	, m_framesLeftBeforeFirstReadback(FFTStageFrameLatency)
	, m_computeFence(0)
{
	for( Uint32 i = 0; i < FFTStageFrameLatency; ++i )
		m_stage[i] = GpuApi::TextureRef( NULL );

	for( Uint32 i = 0; i < FFTPingPongFrames; ++i )
		m_tex[i] = GpuApi::TextureRef( NULL );
}

CRenderSimWaterFFT::~CRenderSimWaterFFT()
{
	GpuApi::SafeRelease(m_order_texture);
	GpuApi::SafeRelease(m_indices_texture);
	GpuApi::SafeRelease(m_output);
	GpuApi::SafeRelease(m_input);

	for( Uint32 i = 0; i < FFTStageFrameLatency; ++i )
		GpuApi::SafeRelease(m_stage[i]);

	for( Uint32 i = 0; i < FFTPingPongFrames; ++i )
		GpuApi::SafeRelease(m_tex[i]);

	GpuApi::SafeRelease(m_stageHelper);

	GpuApi::SafeRelease( m_phillipsAnimationConstants );
	for( Int32 i=0; i<m_steps; i++ )
	{
		GpuApi::SafeRelease( m_butterflyConstants[i] );
	}
	GpuApi::SafeRelease( m_finalizationConstants );
}
void CRenderSimWaterFFT::Initialize( Int32 res )
{
	m_points[2].Set( Vector( -1, -1,  0 ),	Color::WHITE, 0, 1 );
	m_points[1].Set( Vector(  1, -1,  0 ),	Color::WHITE, 1, 1 );
	m_points[0].Set( Vector(  1,  1,  0 ),	Color::WHITE, 1, 0 );
	m_points[5].Set( Vector( -1, -1,  0 ),	Color::WHITE, 0, 1 );
	m_points[4].Set( Vector(  1,  1,  0 ),	Color::WHITE, 1, 0 );
	m_points[3].Set( Vector( -1,  1,  0 ),	Color::WHITE, 0, 0 );

	m_resolution = res;
	m_steps = (Int32)((log10f((Float)res)/log10f(2.0f))+HALF);

	Float* swapbuf		= new Float[ m_resolution ];			for(Int32 i=0;i<m_resolution;i++){	swapbuf[i] = Float(i);	}
	Float* indicesbuf	= new Float[ m_resolution*2*m_steps ];	memset(indicesbuf,0,sizeof(Float)*m_resolution*2*m_steps);
	Float* factorsbuf	= new Float[ m_resolution*2*m_steps ];	memset(factorsbuf,0,sizeof(Float)*m_resolution*2*m_steps);
	Float* signsbuf		= new Float[ m_resolution*m_steps ];	memset(signsbuf,0,sizeof(Float)*m_resolution*m_steps);

	//filling buffers

	FFTswap(swapbuf, m_resolution);
	FFTindices(indicesbuf, m_resolution);
	FFTfactors(factorsbuf, m_resolution);
	FFTsigns(signsbuf, m_resolution);

	for( Int32 i=0;i<m_resolution*m_steps;i++ )
	{
		factorsbuf[(i*2)+0] *= signsbuf[i];
		factorsbuf[(i*2)+1] *= signsbuf[i];
	}

	//creation of the textures

	Uint8* data = (Uint8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, m_resolution * m_resolution * 2 * 2 );
	if( data )
	{
		Uint16* pixels = (Uint16*)(data);
		for( Int32 i=0;i<m_resolution;i++ )
		{
			pixels[i] = Float16Compressor::Compress( swapbuf[i] );
		}
	}

	GpuApi::TextureDesc texDescOrder;
	texDescOrder.type = GpuApi::TEXTYPE_2D;
	texDescOrder.format = GpuApi::TEXFMT_Float_R16;
	texDescOrder.initLevels = 1;
	texDescOrder.usage = GpuApi::TEXUSAGE_Samplable;
	texDescOrder.width = m_resolution;
	texDescOrder.height = 1;

	GpuApi::TextureLevelInitData mip0initData;
	mip0initData.m_isCooked = false;
	mip0initData.m_data = data;

	GpuApi::TextureInitData initData;
	initData.m_isCooked = false;
	initData.m_mipsInitData = &mip0initData;

	m_order_texture = GpuApi::CreateTexture( texDescOrder, GpuApi::TEXG_System, &initData );
	GpuApi::SetTextureDebugPath( m_order_texture, "WaterFFTOrder" );


	Uint32 pitch = m_resolution * 2 * 4;
	if( data )
	{
		for(Int32 i=0;i<m_steps;i++)
		{
			Uint16* pixels = (Uint16*)(data+(pitch*i));
			for( Int32 j=0;j<m_resolution;j++ )
			{
				pixels[(j*4)+0] = Float16Compressor::Compress(  *(indicesbuf+(m_resolution*2*i)+(j*2)+0)  );
				pixels[(j*4)+1] = Float16Compressor::Compress(  *(indicesbuf+(m_resolution*2*i)+(j*2)+1)  );

				pixels[(j*4)+2] = Float16Compressor::Compress(  *(factorsbuf+(m_resolution*2*i)+(j*2)+0)  );
				pixels[(j*4)+3] = Float16Compressor::Compress(  *(factorsbuf+(m_resolution*2*i)+(j*2)+1)  );
			}
		}
	}

	GpuApi::TextureDesc texDescIndices;
	texDescIndices.type = GpuApi::TEXTYPE_2D;
	texDescIndices.format = GpuApi::TEXFMT_Float_R16G16B16A16;
	texDescIndices.initLevels = 1;
	texDescIndices.usage = GpuApi::TEXUSAGE_Samplable;
	texDescIndices.width = m_resolution;
	texDescIndices.height = m_steps;
	m_indices_texture = GpuApi::CreateTexture( texDescIndices, GpuApi::TEXG_System, &initData );
	GpuApi::SetTextureDebugPath( m_indices_texture, "WaterFFTIndices" );

	//clearing
	delete [] swapbuf;
	delete [] indicesbuf;
	delete [] factorsbuf;
	delete [] signsbuf;

	//creating ping pong textures and surfaces + input texture ( complex numbers set )
	for( Uint32 i = 0; i < FFTPingPongFrames; ++i )
	{
		GpuApi::TextureDesc texDescTex;
		texDescTex.type = GpuApi::TEXTYPE_2D;
		texDescTex.format = GpuApi::TEXFMT_Float_R16G16;
		texDescTex.initLevels = 1;
		texDescTex.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		texDescTex.width = m_resolution;
		texDescTex.height = m_resolution;
		m_tex[i] = GpuApi::CreateTexture( texDescTex, GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_tex[i], "WaterFFTPingPong" );
	}

	CStandardRand frandom = GetRenderer()->GetRandomNumberGenerator();
	pitch = m_resolution * 2 * 2;
	if( data )
	{
		for(Int32 j=0;j<m_resolution;j++)
		{
			Uint16* pixels = (Uint16*)(data+j*pitch);
			for(Int32 i=0;i<m_resolution*2;i++)
			{
				pixels[i] = Float16Compressor::Compress(   frandom.Get< Float >( -1.0f , 1.0f )   );
			}
		}
	}

	GpuApi::TextureDesc texDescInput;
	texDescInput.type = GpuApi::TEXTYPE_2D;
	texDescInput.format = GpuApi::TEXFMT_Float_R16G16;
	texDescInput.initLevels = 1;
	texDescInput.usage = GpuApi::TEXUSAGE_Samplable;
	texDescInput.width = m_resolution;
	texDescInput.height = m_resolution;
	m_input = GpuApi::CreateTexture( texDescInput, GpuApi::TEXG_System, &initData );
	GpuApi::SetTextureDebugPath( m_input, "WaterFFTInput" );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, data );

	//this is output texture float4 x,y,z - normal, w - height
	GpuApi::TextureDesc texDescOutput;
	texDescOutput.type = GpuApi::TEXTYPE_2D;
	texDescOutput.format = GpuApi::TEXFMT_Float_R16G16B16A16;
	texDescOutput.initLevels = 1;//(Uint16)( Red::Math::MLog2( m_resolution ) + 1 );
	texDescOutput.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;// | GpuApi::TEXUSAGE_GenMip;
	texDescOutput.width = m_resolution;
	texDescOutput.height = m_resolution;
	m_output = GpuApi::CreateTexture( texDescOutput, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( m_output, "WaterFFTOutput" );

	///////////////////////////////////////staging textures for cpu read

	m_stagingFlip = 0;

	for( Uint32 i =0; i < FFTStageFrameLatency; ++i )
	{
		GpuApi::TextureDesc texDescTexStage;
		texDescTexStage.type = GpuApi::TEXTYPE_2D;
		texDescTexStage.format = GpuApi::TEXFMT_Float_R16;
		texDescTexStage.initLevels = 1;
		texDescTexStage.usage = GpuApi::TEXUSAGE_Staging;
		texDescTexStage.width = GetReadBackResolution();
		texDescTexStage.height = GetReadBackResolution();
		m_stage[i] = GpuApi::CreateTexture( texDescTexStage , GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_stage[i], "WaterFFTStaging" );
	}

	{
		GpuApi::TextureDesc texDescTexStage;
		texDescTexStage.type = GpuApi::TEXTYPE_2D;
		texDescTexStage.format = GpuApi::TEXFMT_Float_R16;
		texDescTexStage.initLevels = 1;
		texDescTexStage.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
		texDescTexStage.width = GetReadBackResolution();
		texDescTexStage.height = GetReadBackResolution();
		m_stageHelper = GpuApi::CreateTexture( texDescTexStage , GpuApi::TEXG_System );
		GpuApi::SetTextureDebugPath( m_stageHelper, "WaterFFTStagingHelper" );
	}


	{
		m_phillipsAnimationConstants = GpuApi::CreateBuffer( sizeof(Vector) * 2, GpuApi::BCC_Constant, GpuApi::BUT_Default, 0, nullptr );
		RED_FATAL_ASSERT( m_steps < 16, "not enough space for constant buffers" );
		for( Int32 i=0; i<m_steps; i++ )
		{
			Vector data = Vector( (Float)i, 0.f, 0.f, 0.f );
			GpuApi::BufferInitData initData;
			initData.m_buffer = data.AsFloat();
			initData.m_elementCount = 1;
			m_butterflyConstants[i] = GpuApi::CreateBuffer( sizeof(Vector), GpuApi::BCC_Constant, GpuApi::BUT_Default, 0, &initData );
		}
		m_finalizationConstants = GpuApi::CreateBuffer( sizeof(Vector), GpuApi::BCC_Constant, GpuApi::BUT_Default, 0, nullptr );
	}
}

void CRenderSimWaterFFT::FlipTarget( GpuApi::RenderTargetSetup * targets, GpuApi::TextureRef *textures, Uint32 & flipIdx )
{
	if (Config::cvUseComputeFFT.Get())
	{
		FlipTargetCompute( textures, flipIdx );
	}
	else
	{
		FlipTargetPixel( targets, textures, flipIdx );
	}
}

void CRenderSimWaterFFT::FlipTargetPixel( GpuApi::RenderTargetSetup * targets, GpuApi::TextureRef *textures, Uint32 & flipIdx )
{
	RED_ASSERT( textures );
	Uint32 flipNextIdx = ( flipIdx + 1 ) % FFTPingPongFrames ;

	if( targets != nullptr )
	{
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		GpuApi::SetupRenderTargets( targets[flipNextIdx] );
	}

	GpuApi::BindTextures( 0, 1, &textures[flipIdx], GpuApi::PixelShader );
	flipIdx = flipNextIdx;
}

void CRenderSimWaterFFT::FlipTargetCompute( GpuApi::TextureRef *textures, Uint32 & flipIdx )
{
	RED_ASSERT( textures );
	GpuApi::SetupBlankRenderTargets();
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 0, 1, &textures[flipIdx], GpuApi::ComputeShader );
	flipIdx = ( flipIdx + 1 ) % FFTPingPongFrames;
	GpuApi::BindTextureUAVs( 4, 1, &textures[flipIdx] );
}

void CRenderSimWaterFFT::Cleanup()
{
	if (Config::cvUseComputeFFT.Get())
	{
		CleanupCompute();
	}
}

void CRenderSimWaterFFT::CleanupCompute()
{
	GpuApi::BindTextureUAVs( 4, 1, nullptr );
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );
}


#ifdef RED_PLATFORM_CONSOLE
static Bool trulyAsync = true;
#else
static Bool trulyAsync = false;
#endif


Bool CRenderSimWaterFFT::IsAsyncCompute() const
{
	return Config::cvUseAsyncComputeFFT.Get() && trulyAsync;
}


GpuApi::TextureRef CRenderSimWaterFFT::Calculate( Float wdir, Float wspeed, Float amplitude, Float lambda, Float deltaTime, Float windScale, Bool& wasSimulated )
{
	PC_SCOPE_RENDER_LVL1( FFTWater_Calculate);

	EngineTime currentTime = EngineTime::GetNow();
	if ( currentTime - m_time < EngineTime(0.03f) )
	{
		wasSimulated = false;
		return m_output;
	}
	m_time = currentTime;

	//setting matrices
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );

	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );
	//creating plane

	GpuApi::TextureRef *tempTextures = m_tex;

	COMPILE_ASSERT( 2 == FFTPingPongFrames );

	GpuApi::RenderTargetSetup rtSetup[ FFTPingPongFrames ];
	for( Uint32 i = 0; i < FFTPingPongFrames; ++i )
	{
		rtSetup[i].SetColorTarget(0, tempTextures[i]);
		rtSetup[i].SetViewport(m_resolution, m_resolution);
	}

	GpuApi::TextureRef refs[3] = { m_input, m_order_texture, m_indices_texture };

	if (Config::cvUseComputeFFT.Get())
	{
		GpuApi::BindTextures( 1, 3, &(refs[0]), GpuApi::ComputeShader );
	}
	else
	{
		const GpuApi::SamplerStateRef &stateRef = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapPointNoMip );
		GpuApi::SetSamplerState( 0, stateRef, GpuApi::PixelShader );
		GpuApi::SetSamplerState( 1, stateRef, GpuApi::PixelShader );
		GpuApi::SetSamplerState( 2, stateRef, GpuApi::PixelShader );
		GpuApi::SetSamplerState( 3, stateRef, GpuApi::PixelShader );

		GpuApi::BindTextures( 1, 3, &(refs[0]), GpuApi::PixelShader );
	}

	//ping pong
	Uint32 flipIdx = 0; //first is set	

	// Combined Phillips and Animation passes
	{
		PC_SCOPE_RENDER_LVL1( FFTPhillipsAnimate );

		Float accelWaterSpeed = wspeed * wspeed / PAT_G;

		struct SConstantBuffer
		{
			Vector dataPhilips;
			Vector dataAnimation;
		};

		// hack, simulation becomes unstable after ~300000 seconds (few days of playthrough...)
		Float modDeltaTime = fmod( deltaTime, 100000.0f );

		SConstantBuffer constBuffer;
		constBuffer.dataPhilips = Vector( Red::Math::MSin( wdir ) , Red::Math::MCos( wdir ) , accelWaterSpeed * accelWaterSpeed , amplitude );
		constBuffer.dataAnimation = Vector( windScale + 0.4f, 0.4f*(1.0f - windScale), modDeltaTime, lambda );

		if (Config::cvUseAsyncComputeFFT.Get())
		{
			GetRenderer()->LoadBufferData( m_phillipsAnimationConstants, 0, sizeof( SConstantBuffer ), &constBuffer );

			GpuApi::ComputeTaskDesc computeTask;
			// Sync to ensure CB is ready.
			computeTask.m_insertSync = true;
			computeTask.m_inputTextures[0] = tempTextures[flipIdx];
			computeTask.m_inputTextures[1] = m_input;
			computeTask.m_inputTextures[2] = m_order_texture;
			computeTask.m_inputTextures[3] = m_indices_texture;
			computeTask.m_inputTextureCount = 4;
			flipIdx = ( flipIdx + 1 ) % FFTPingPongFrames;
			computeTask.m_uav = tempTextures[flipIdx];
			computeTask.m_uavIndex = 4;
			computeTask.m_shader = GetRenderer()->m_shaderFFTGPU_PhillipsAnimationCompute->GetShader()->GetShader();
			computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupZ = 1;
			computeTask.m_constantBuffers[0] = m_phillipsAnimationConstants;
			computeTask.m_constantBufferCount = 1;

			if (trulyAsync)
			{
				AddAsyncComputeTaskToQueue(computeTask);
			}
			else
			{
				AddSyncComputeTaskToQueue(computeTask);
			}
		}
		else
		{
			FlipTarget( rtSetup, tempTextures, flipIdx );
			if (Config::cvUseComputeFFT.Get())
			{
				GpuApi::SetComputeShaderConsts( constBuffer );
				GetRenderer()->m_shaderFFTGPU_PhillipsAnimationCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 );
			}
			else
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, constBuffer.dataPhilips );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, constBuffer.dataAnimation );
				GetRenderer()->m_shaderFFTGPU_PhillipsAnimation->Bind();
				GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
			}
			Cleanup();
		}
	}
	//ordering
	{
		PC_SCOPE_RENDER_LVL1( Order );

		if (Config::cvUseAsyncComputeFFT.Get())
		{
			GpuApi::ComputeTaskDesc computeTask;
			computeTask.m_inputTextures[0] = tempTextures[flipIdx];
			computeTask.m_inputTextures[1] = m_input;
			computeTask.m_inputTextures[2] = m_order_texture;
			computeTask.m_inputTextures[3] = m_indices_texture;
			computeTask.m_inputTextureCount = 4;
			flipIdx = ( flipIdx + 1 ) % FFTPingPongFrames;
			computeTask.m_uav = tempTextures[flipIdx];
			computeTask.m_uavIndex = 4;
			computeTask.m_shader = GetRenderer()->m_shaderFFTGPU_OrderingCompute->GetShader()->GetShader();
			computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupZ = 1;
			computeTask.m_constantBufferCount = 0;

			if (trulyAsync)
			{
				AddAsyncComputeTaskToQueue(computeTask);
			}
			else
			{
				AddSyncComputeTaskToQueue(computeTask);
			}
		}
		else
		{
			FlipTarget( rtSetup, tempTextures, flipIdx );
			if (Config::cvUseComputeFFT.Get())
			{
				GetRenderer()->m_shaderFFTGPU_OrderingCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 );
			}
			else
			{
				GetRenderer()->m_shaderFFTGPU_Ordering->Bind();
				GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
			}
			Cleanup();
		}
	}
	//butterfly
	for(int i=0;i<m_steps;i++)
	{
		PC_SCOPE_RENDER_LVL1( Butterfly );

		if (Config::cvUseAsyncComputeFFT.Get())
		{

			GpuApi::ComputeTaskDesc computeTask;
			computeTask.m_inputTextures[0] = tempTextures[flipIdx];
			computeTask.m_inputTextures[1] = m_input;
			computeTask.m_inputTextures[2] = m_order_texture;
			computeTask.m_inputTextures[3] = m_indices_texture;
			computeTask.m_inputTextureCount = 4;
			flipIdx = ( flipIdx + 1 ) % FFTPingPongFrames;
			computeTask.m_uav = tempTextures[flipIdx];
			computeTask.m_uavIndex = 4;
			computeTask.m_shader = GetRenderer()->m_shaderFFTGPU_ButterflyCompute->GetShader()->GetShader();
			computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupZ = 1;
			computeTask.m_constantBufferCount = 1;
			computeTask.m_constantBuffers[0] = m_butterflyConstants[i];

			if (trulyAsync)
			{
				AddAsyncComputeTaskToQueue(computeTask);
			}
			else
			{
				AddSyncComputeTaskToQueue(computeTask);
			}
		}
		else
		{
			struct SConstantBuffer
			{
				Vector data;
			};

			SConstantBuffer constBuffer;

			FlipTarget( rtSetup, tempTextures, flipIdx );
			if (Config::cvUseComputeFFT.Get())
			{
				constBuffer.data = Vector(Float(i),0.0f,0.0f,0.0f);
				GpuApi::SetComputeShaderConsts( constBuffer );
				GetRenderer()->m_shaderFFTGPU_ButterflyCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 );
			}
			else
			{
				constBuffer.data = Vector(((Float(i)+HALF)/Float(m_steps)),0.0f,0.0f,0.0f);
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, constBuffer.data );
				GetRenderer()->m_shaderFFTGPU_Butterfly->Bind();
				GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
			}
			Cleanup();
		}
	}
	//transpose ordering
	{
		PC_SCOPE_RENDER_LVL1( Transpose );

		if (Config::cvUseAsyncComputeFFT.Get())
		{
			GpuApi::ComputeTaskDesc computeTask;
			computeTask.m_inputTextures[0] = tempTextures[flipIdx];
			computeTask.m_inputTextures[1] = m_input;
			computeTask.m_inputTextures[2] = m_order_texture;
			computeTask.m_inputTextures[3] = m_indices_texture;
			computeTask.m_inputTextureCount = 4;
			flipIdx = ( flipIdx + 1 ) % FFTPingPongFrames;
			computeTask.m_uav = tempTextures[flipIdx];
			computeTask.m_uavIndex = 4;
			computeTask.m_shader = GetRenderer()->m_shaderFFTGPU_TransposeOrderingCompute->GetShader()->GetShader();
			computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupZ = 1;
			computeTask.m_constantBufferCount = 0;

			if (trulyAsync)
			{
				AddAsyncComputeTaskToQueue(computeTask);
			}
			else
			{
				AddSyncComputeTaskToQueue(computeTask);
			}
		}
		else
		{
			FlipTarget( rtSetup, tempTextures, flipIdx );
			if (Config::cvUseComputeFFT.Get())
			{
				GetRenderer()->m_shaderFFTGPU_TransposeOrderingCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 );
			}
			else
			{
				GetRenderer()->m_shaderFFTGPU_TransposeOrdering->Bind();
				GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
			}
			Cleanup();
		}
	}
	//butterfly
	for(int i=0;i<m_steps;i++)
	{
		PC_SCOPE_RENDER_LVL1( Butterfly_Again );

		if (Config::cvUseAsyncComputeFFT.Get())
		{
			GpuApi::ComputeTaskDesc computeTask;
			computeTask.m_inputTextures[0] = tempTextures[flipIdx];
			computeTask.m_inputTextures[1] = m_input;
			computeTask.m_inputTextures[2] = m_order_texture;
			computeTask.m_inputTextures[3] = m_indices_texture;
			computeTask.m_inputTextureCount = 4;
			flipIdx = ( flipIdx + 1 ) % FFTPingPongFrames;
			computeTask.m_uav = tempTextures[flipIdx];
			computeTask.m_uavIndex = 4;
			computeTask.m_shader = GetRenderer()->m_shaderFFTGPU_ButterflyCompute->GetShader()->GetShader();
			computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupZ = 1;
			computeTask.m_constantBufferCount = 1;
			computeTask.m_constantBuffers[0] = m_butterflyConstants[i];

			if (trulyAsync)
			{
				AddAsyncComputeTaskToQueue(computeTask);
			}
			else
			{
				AddSyncComputeTaskToQueue(computeTask);
			}
		}
		else
		{
			struct SConstantBuffer
			{
				Vector data;
			};

			SConstantBuffer constBuffer;

			FlipTarget( rtSetup, tempTextures, flipIdx );
			if (Config::cvUseComputeFFT.Get())
			{
				constBuffer.data = Vector(Float(i),0.0f,0.0f,0.0f);
				GpuApi::SetComputeShaderConsts( constBuffer );
				GetRenderer()->m_shaderFFTGPU_ButterflyCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 ); 
			}
			else
			{
				constBuffer.data = Vector(((Float(i)+HALF)/Float(m_steps)),0.0f,0.0f,0.0f);
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, constBuffer.data );
				GetRenderer()->m_shaderFFTGPU_Butterfly->Bind();	
				GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
			}
			Cleanup();
		}
	}
	//finalize
	{
		PC_SCOPE_RENDER_LVL1( Finalize );

		if (Config::cvUseAsyncComputeFFT.Get())
		{
			Float inv_sq	= 1.0f / Float(m_resolution);
			Int32 offset	= Int32(1.0f + 8.0f * windScale); // Lerp( 2 , 18 )
			Vector constData = Vector( inv_sq, Float(offset), 0.0f, 0.0f);
			GetRenderer()->LoadBufferData( m_finalizationConstants, 0, sizeof( Vector ), &constData );

			GpuApi::ComputeTaskDesc computeTask;
			// Sync to make sure CB Is ready.
			computeTask.m_insertSync = true;
			computeTask.m_inputTextures[0] = tempTextures[flipIdx];
			computeTask.m_inputTextures[1] = m_input;
			computeTask.m_inputTextures[2] = m_order_texture;
			computeTask.m_inputTextures[3] = m_indices_texture;
			computeTask.m_inputTextureCount = 4;
			flipIdx = ( flipIdx + 1 ) % FFTPingPongFrames;
			computeTask.m_uav = m_output;
			computeTask.m_uavIndex = 4;
			computeTask.m_shader = GetRenderer()->m_shaderFFTGPU_FinalizeCompute->GetShader()->GetShader();
			computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
			computeTask.m_threadGroupZ = 1;
			computeTask.m_constantBufferCount = 1;
			computeTask.m_constantBuffers[0] = m_finalizationConstants;

			if (trulyAsync)
			{
				AddAsyncComputeTaskToQueue(computeTask);
			}
			else
			{
				AddSyncComputeTaskToQueue(computeTask);
			}
		}
		else
		{
			struct SConstantBuffer
			{
				Vector data;
			};

			FlipTarget( rtSetup, tempTextures, flipIdx );
			if (Config::cvUseComputeFFT.Get())
			{
				Float inv_sq	= 1.0f / Float(m_resolution);
				Int32 offset	= Int32(1.0f + 8.0f * windScale); // Lerp( 2 , 18 )

				SConstantBuffer constBuffer;
				constBuffer.data = Vector( inv_sq, Float(offset), 0.0f, 0.0f);
				GpuApi::SetComputeShaderConsts( constBuffer );
				GpuApi::BindTextureUAVs( 4, 1, &m_output ); // instead of continuing the ping-pong write the results into the output texture
				GetRenderer()->m_shaderFFTGPU_FinalizeCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 );
			}
			else
			{
				Float inv_sq	= 1.0f / Float(m_resolution);
				Float offsx		= 2.0f + 16.0f * windScale; // Lerp( 2 , 18 )

				SConstantBuffer constBuffer;
				constBuffer.data = Vector( inv_sq , inv_sq * offsx , 0.0f, 0.0f );

				GpuApi::RenderTargetSetup rtSetupOutput;
				rtSetupOutput.SetColorTarget(0, m_output);
				rtSetupOutput.SetViewport(m_resolution, m_resolution);

				GpuApi::SetupRenderTargets(rtSetupOutput);
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, constBuffer.data );
				GetRenderer()->m_shaderFFTGPU_Finalize->Bind();
				GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
			}
			Cleanup();
		}
	}

	if ( Config::cvUseAsyncComputeFFT.Get() && trulyAsync )
	{
		m_computeFence = GpuApi::KickoffAsyncComputeTasks();
	}
	else
	{
		m_computeFence = 0;
	}

	// use old rendertarget
	if (Config::cvUseComputeFFT.Get())
	{
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::ComputeShader );
	}
	else
	{
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::PixelShader );
	}

	wasSimulated = true;

	return  ( m_output );
}

void CRenderSimWaterFFT::FillCPUData( void* heightReadback )
{
	if ( Config::cvUseAsyncComputeFFT.Get() && m_computeFence > 0 )
	{
		GpuApi::InsertWaitOnFence( m_computeFence );
	}

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget(0, m_stageHelper );
	rtSetup.SetViewport( GetReadBackResolution() , GetReadBackResolution() );

	const GpuApi::SamplerStateRef &stateRef = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapPointNoMip );
	GpuApi::SetSamplerState( 0, stateRef, GpuApi::PixelShader );

	// copy stage height to helper/stage
	{
		PC_SCOPE_RENDER_LVL1( CopyHeightStageMap );
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

		GpuApi::SetupRenderTargets(rtSetup);
		GpuApi::BindTextures( 0, 1, &m_output, GpuApi::PixelShader );

		GetRenderer()->m_shaderFFTGPU_CopyHeight->Bind();
		GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
	}
	// Copy from helper/stage	
	{		
		// Most last stage texture to read
		Uint32 readFlipIdx = m_stagingFlip;
		Uint32 writeFlipIdx = ( m_stagingFlip + FFTStageFrameLatency - 1 ) % FFTStageFrameLatency;

		GpuApi::TextureRef readDest = m_stage[ readFlipIdx ];
		ReadHeightMap( readDest , heightReadback );

		// tell GPU to copy the top mipmap of outputHMap -> dest
		GpuApi::TextureRef writeDest = m_stage[ writeFlipIdx ];
		GetRenderer()->StretchRect(m_stageHelper, writeDest);

		// Flip stage texure to the next one
		m_stagingFlip = ( m_stagingFlip + 1 ) % FFTStageFrameLatency;
	}
}

void CRenderSimWaterFFT::ReadHeightMap( GpuApi::TextureRef readDest , void* heightReadback )
{	
	PC_SCOPE_RENDER_LVL1( ReadHeightMap );

	if ( m_framesLeftBeforeFirstReadback > 0 )
	{
		--m_framesLeftBeforeFirstReadback;
		return;
	}

	// read staging texture data from most previous frame
	Uint32 destPitch = 0;
	Uint32 flags = GpuApi::BLF_Read;	// | GpuApi::BLF_DoNotWait;
										// This is causing issues on > 45 fps										
										// Wait, what? > 45 fps? Where?

	const void* mem = GpuApi::LockLevel (readDest, 0, 0, flags, destPitch);
	if (mem && heightReadback)
	{
		const Uint32 readBackResolution = GetReadBackResolution();
		const Uint32 readBackStructSize = sizeof( Uint16 );

		Red::System::MemoryCopy( heightReadback, mem, readBackResolution * readBackResolution * readBackStructSize );

		GpuApi::UnlockLevel (readDest, 0, 0);
	}
	else
	{
		RED_LOG(RED_LOG_CHANNEL(TXT("GpuApi") ), TXT("Cannot lock the water height readback texture!"));
	}

}

void CRenderSimWaterFFT::FFTswap (Float* data, Int32 n)
{
	Float* temp = new Float[n*2];
	for(Int32 k=0;k<n;k++)
	{
		temp[k*2+0] = data[k];
		temp[k*2+1] = 0;
	}
	Int32 nn = n*2;
	Int32 m,j,i;
	Float tempr;
	j=0;
	for (i=0;i<nn/2;i+=2) 
	{
		if (j > i) 
		{
			PAT_SWAP(temp[j],temp[i]);
			PAT_SWAP(temp[j+1],temp[i+1]);
			if((j/2)<(nn/4))
			{
				PAT_SWAP(temp[(nn-(i+2))],temp[(nn-(j+2))]);
				PAT_SWAP(temp[(nn-(i+2))+1],temp[(nn-(j+2))+1]);
			}
		}
		m=nn/2;
		while (m >= 2 && j >= m) 
		{
			j -= m;
			m = m/2;
		}
		j += m;
	}
	for(Int32 k=0;k<n;k++)
	{
		data[k] = (temp[k*2]+HALF)/Float(n);
	}
	delete[] temp;
}
void CRenderSimWaterFFT::FFTindices (Float* data, Int32 n)
{
	Int32 siz = 1;
	for(Int32 s=0;s<m_steps;s++)
	{
		siz = siz<<1;
		Float* ptr = &data[n*2*s];
		Float* temp = new Float[n];
		for(Int32 i=0;i<n;i++){ temp[i]=Float(i); }
		Int32 numm = n/siz;
		for(Int32 k=0;k<numm;k++)
		{
			Float* p = &temp[siz*k];
			Float st = p[0];
			p[1] = st+(siz/2);
			for(Int32 m=2;m<siz;m+=2)
			{
				p[m] = ++st;
				p[m+1] = p[m]+(siz/2);
			}
		}
		Int32 ind = 0;
		for(Int32 k=0;k<numm;k++)
		{
			Float* p = &temp[siz*k];
			Float* p1 = ptr+((siz*k*2)+0);
			Float* p2 = ptr+((siz*k*2)+siz);
			memcpy(p1, p, sizeof(Float)*siz);
			memcpy(p2, p, sizeof(Float)*siz);
		}

		delete [] temp;

		for(Int32 j=0;j<n*2;j++)
		{
			ptr[j] = (ptr[j]+HALF)/Float(n);
		}
	}
}
void CRenderSimWaterFFT::FFTfactors (Float* data, Int32 n)
{
	Int32 siz = 1;
	for(Int32 s=0;s<m_steps;s++)
	{
		Int32 num = siz<<1;
		Float* ptr = &data[n*2*s];
		for(Int32 i=0;i<n;i++)
		{
			Int32 k = i % siz;
			float a = (Float(k)/Float(num))*2.0f*PAT_PI;
			ptr[(i*2)+0] = cosf(a*-1.0f);
			ptr[(i*2)+1] = sinf(a*-1.0f);
		}
		siz = siz<<1;
	}
}
void CRenderSimWaterFFT::FFTsigns (Float* data, Int32 n)
{
	Int32 siz = 1;
	for(Int32 s=0;s<m_steps;s++)
	{
		Int32 num = siz<<1;
		Float* ptr = &data[n*s];
		for(Int32 i=0;i<n;i++)
		{
			ptr[i] = i % num<siz ? 1.0f : -1.0f;
		}
		siz = siz<<1;
	}
}

#undef HALF
#undef PAT_PI
#undef PAT_SWAP
#undef PAT_2PI
#undef PAT_G
