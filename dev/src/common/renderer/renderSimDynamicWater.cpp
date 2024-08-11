/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderSimDynamicWater.h"
#include "renderShaderPair.h"
#include "renderShader.h"


#define MAX_IMPULSES 32


// Keep in sync with dynamicWater.fx
Uint32 TILE_SIZE = 16;


namespace Config
{
	TConfigVar< Bool > cvDynamicWaterSimCompute(		"WaterSimulation", "UseComputeDynamicWater",		true );
	TConfigVar< Bool > cvDynamicWaterSimComputeAsync(	"WaterSimulation", "UseAsyncComputeDynamicWater",	true );
	TConfigVar< Bool > cvDynamicWaterInstancedImpulses(	"WaterSimulation", "UseInstancedImpulses",			true );
}


struct ImpulseData
{
	Matrix transform;
	Vector randomNumbers;
};
static_assert( sizeof( ImpulseData ) == 80, "ImpulseData should not be padded" );

struct SimulateData {
	Vector2 cameraDelta;
	Vector2 randoms;
	Float rainIntensity;
	Float scaledTimeDelta;
	Float PADDING[2];
};
static_assert( sizeof( SimulateData ) == 32, "SimulateData should not be padded" );



/////////////////////////////////////////////////////////////////
CRenderSimDynamicWater::CRenderSimDynamicWater()
	: m_resolution( 0 )
	, m_flipped( false )
	, m_rainIntensity( 0.0f )
	, m_framesSinceImpulse( 0 )
	, m_time( 0.0f )
	, m_appliedImpulses( false )
	, m_initialClearDone( false )
	, m_simulationFence( 0 )
{
	m_impulses.Reserve( 32 );
	
	m_snappedCameraPosition = Vector(0.0f,0.0f,0.0f,0.0f);
	m_lastPos = Vector(0.0f,0.0f,0.0f,0.0f);
	
	for( Uint32 i=0; i<6; ++i) m_points[i].Set(Vector::ZEROS, Color::BLACK, 0.0f, 0.0f );	
	m_lastdt = 0.0f;


	// Create vertex layout for drawing impulses.
	{
		using namespace GpuApi::VertexPacking;

		PackingElement impulseLayoutElements[] =
		{
			//	Type			Usage					UsageIndex	Slot	SlotType
			{	PT_Float4,		PS_InstanceTransform,	0,			0,		ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	1,			0,		ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	2,			0,		ST_PerInstance	},
			{	PT_Float4,		PS_InstanceTransform,	3,			0,		ST_PerInstance	},
			{	PT_Float4,		PS_ExtraData,			0,			0,		ST_PerInstance	},
			PackingElement::END_OF_ELEMENTS
		};

		GpuApi::VertexLayoutDesc impulseLayoutDesc;
		impulseLayoutDesc.AddElements( impulseLayoutElements );
		m_impulseVertexLayout = GpuApi::CreateVertexLayout( impulseLayoutDesc );
	}

	// Create instance buffer for the impulses.
	m_impulseVertexBuffer = GpuApi::CreateBuffer( MAX_IMPULSES * sizeof( ImpulseData ), GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );

	m_simulateConstants = GpuApi::CreateBuffer( sizeof( SimulateData ), GpuApi::BCC_Constant, GpuApi::BUT_Default, 0 );
}

CRenderSimDynamicWater::~CRenderSimDynamicWater()
{
	if ( m_simulationFence > 0 )
	{
		GpuApi::ReleaseFence( m_simulationFence );
	}

	GpuApi::SafeRelease( m_simulateConstants );

	GpuApi::SafeRelease( m_impulseVertexBuffer );
	GpuApi::SafeRelease( m_impulseVertexLayout );

	GpuApi::RemoveDynamicTexture( m_output );
	GpuApi::RemoveDynamicTexture( m_input );
	GpuApi::RemoveDynamicTexture( m_tex1 );
	GpuApi::RemoveDynamicTexture( m_tex2 );

	GpuApi::SafeRelease(m_output);
	GpuApi::SafeRelease(m_input);
	GpuApi::SafeRelease(m_tex1);
	GpuApi::SafeRelease(m_tex2);
}

void CRenderSimDynamicWater::Initialize()
{
	m_points[0].Set( Vector(  1,  1,  0 ),	Color::WHITE, 1, 0 );
	m_points[1].Set( Vector(  1, -1,  0 ),	Color::WHITE, 1, 1 );
	m_points[2].Set( Vector( -1, -1,  0 ),	Color::WHITE, 0, 1 );
	m_points[3].Set( Vector( -1,  1,  0 ),	Color::WHITE, 0, 0 );
	m_points[4].Set( Vector(  1,  1,  0 ),	Color::WHITE, 1, 0 );
	m_points[5].Set( Vector( -1, -1,  0 ),	Color::WHITE, 0, 1 );

	m_resolution = static_cast<Uint32>( DYNAMIC_WATER_RESOLUTION );

	GpuApi::TextureDesc texDescTex1;
	texDescTex1.type = GpuApi::TEXTYPE_2D;
	texDescTex1.format = GpuApi::TEXFMT_Float_R16G16B16A16;
	texDescTex1.initLevels = 1;
	texDescTex1.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
	texDescTex1.width = m_resolution;
	texDescTex1.height = m_resolution;
	m_tex1 = GpuApi::CreateTexture( texDescTex1, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( m_tex1, "DynWater1" );
	GpuApi::AddDynamicTexture( m_tex1, "DynWater1" );

	GpuApi::TextureDesc texDescTex2;
	texDescTex2.type = GpuApi::TEXTYPE_2D;
	texDescTex2.format = GpuApi::TEXFMT_Float_R16G16B16A16;
	texDescTex2.initLevels = 1;
	texDescTex2.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
	texDescTex2.width = m_resolution;
	texDescTex2.height = m_resolution;
	m_tex2 = GpuApi::CreateTexture( texDescTex2, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( m_tex2, "DynWater2" );
	GpuApi::AddDynamicTexture( m_tex2, "DynWater2" );

	GpuApi::TextureDesc texDescInput;
	texDescInput.type = GpuApi::TEXTYPE_2D;
	texDescInput.format = GpuApi::TEXFMT_Float_R16G16;
	texDescInput.initLevels = 1;
	texDescInput.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
	texDescInput.width = m_resolution;
	texDescInput.height = m_resolution;
	m_input = GpuApi::CreateTexture( texDescInput, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( m_input, "DynWaterInput" );
	GpuApi::AddDynamicTexture( m_input, "DynWaterInput" );


	GpuApi::TextureDesc texDescOutput;
	texDescOutput.type = GpuApi::TEXTYPE_2D;
	texDescOutput.format = GpuApi::TEXFMT_Float_R16G16B16A16;
	texDescOutput.initLevels = 1;
	texDescOutput.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;
	texDescOutput.width = m_resolution;
	texDescOutput.height = m_resolution;
	m_output = GpuApi::CreateTexture( texDescOutput, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( m_output, "DynWaterOutput" );
	GpuApi::AddDynamicTexture( m_output, "DynWaterOutput" );
}

void CRenderSimDynamicWater::InitialClear()
{
	// Need to have a latent clear like this, because the main Initialize is being called from outside of the BeginRender/EndRender block.
	if ( !m_initialClearDone )
	{
		Vector clearColor = Vector(0.0f, 0.0f, 0.0f, 0.0f);
		GetRenderer()->ClearColorTarget( m_tex1, clearColor );
		GetRenderer()->ClearColorTarget( m_tex2, clearColor );
		GetRenderer()->ClearColorTarget( m_input, clearColor );
		GetRenderer()->ClearColorTarget( m_output, clearColor );
		m_initialClearDone = true;
	}
}

void CRenderSimDynamicWater::AddImpulse( const Matrix& matrix )
{
	m_impulses.PushBack(matrix);
}

void CRenderSimDynamicWater::ApplyImpulses()
{
	PC_SCOPE_GPU( DynamicWater_Impulses );

	InitialClear();

	m_appliedImpulses = false;
	const Uint32 impulseCount = m_impulses.Size();
	if ( impulseCount == 0 )
	{
		m_framesSinceImpulse++;
		return;
	}

	m_appliedImpulses = true;
	m_framesSinceImpulse = 0;

	// Grab render target setup. It'll be automatically restored.
	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcAdd );
	CGpuApiScopedTwoSidedRender scopedForcedTwoSided( true );

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget(0, m_input);
	rtSetup.SetViewport(m_resolution, m_resolution);
	GpuApi::SetupRenderTargets(rtSetup);

	GetRenderer()->ClearColorTarget( Vector::ZEROS );


	CStandardRand frandom = GetRenderer()->GetRandomNumberGenerator();


	// FIXME : When meditating near/in water, it can cause this to get called multiple times in a frame, resulting in GpuApiGnm
	// fatal asserting about doing multiple discard locks. So for now, no instanced impulses for PS4...
#ifndef RED_PLATFORM_ORBIS
	if ( Config::cvDynamicWaterInstancedImpulses.Get() )
	{
		GetRenderer()->m_shaderDynamicWaterImpulseInstanced->Bind();
		GpuApi::SetVertexFormatRaw( m_impulseVertexLayout );

		Uint32 numLeft = impulseCount;
		Uint32 readOffset = 0;
		while ( numLeft > 0 )
		{
			const Uint32 numInBatch = Min< Uint32 >( numLeft, MAX_IMPULSES );

			ImpulseData* impulseData = (ImpulseData*)GpuApi::LockBuffer( m_impulseVertexBuffer, GpuApi::BLF_Discard, 0, numInBatch * sizeof( ImpulseData ) );
			RED_ASSERT( impulseData != nullptr, TXT("Failed to lock impulse buffer") );
			if ( impulseData != nullptr )
			{
				for ( Uint32 i = 0; i < numInBatch; ++i )
				{
					Matrix tm = m_impulses[ readOffset + i ];
					Float val = tm.V[3].A[3];
					tm.V[3].A[3] = 1.0f;


					impulseData[ i ].transform = tm;
					impulseData[ i ].randomNumbers.Set4( frandom.Get<Float>(), frandom.Get<Float>(), frandom.Get<Float>(), val );
				}

				GpuApi::UnlockBuffer( m_impulseVertexBuffer );
				impulseData = nullptr;


				Uint32 stride = sizeof( ImpulseData );
				Uint32 offset = 0;

				GpuApi::BindVertexBuffers( 0, 1, &m_impulseVertexBuffer, &stride, &offset );
				GpuApi::DrawInstancedPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 0, 4, 2, numInBatch );
			}

			readOffset += numInBatch;
			numLeft -= numInBatch;
		}
	}
	else
#endif
	{
		for( Uint32 i = 0; i < impulseCount; ++i )
		{
			Matrix tm = m_impulses[i];
			{
				Float val = tm.V[3].A[3];
				tm.V[3].A[3] = 1.0f;
				Vector data( frandom.Get<Float>(), frandom.Get<Float>(), frandom.Get<Float>(), val );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, data );
				GetRenderer()->GetStateManager().SetLocalToWorld( &tm );
				GetRenderer()->m_shaderDynamicWaterImpulse->Bind();
				GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
			}
		}
	}

	m_impulses.ClearFast();
	GpuApi::SetupBlankRenderTargets();
}

void CRenderSimDynamicWater::UpdateCamera( const Vector& campos )
{
	const Float worldTexelSize = DYNAMIC_WATER_WORLD_SIZE / m_resolution;
	m_snappedCameraPosition.X = (Int32)( campos.X/worldTexelSize ) * worldTexelSize;
	m_snappedCameraPosition.Y = (Int32)( campos.Y/worldTexelSize ) * worldTexelSize;
}


void CRenderSimDynamicWater::FinishAsyncSimulate()
{
	if ( m_simulationFence > 0 )
	{
		GpuApi::InsertWaitOnFence( m_simulationFence );
		GpuApi::ReleaseFence( m_simulationFence );
		m_simulationFence = 0;
	}
}


#ifdef RED_PLATFORM_CONSOLE
static Bool trulyAsync = true;
#else
static Bool trulyAsync = false;
#endif


void CRenderSimDynamicWater::DoSimulateCompute( Float timeDelta, const Vector& cameraDelta )
{
	PC_SCOPE_RENDER_LVL1( DynamicWater_SimulateCompute);

	CStandardRand& frandom = GetRenderer()->GetRandomNumberGenerator();

	SimulateData simData;
	simData.cameraDelta.Set( cameraDelta.X / DYNAMIC_WATER_WORLD_SIZE, cameraDelta.Y / DYNAMIC_WATER_WORLD_SIZE );
	simData.randoms.Set( frandom.Get<Float>(), frandom.Get<Float>() );
	simData.rainIntensity = Clamp( m_rainIntensity, 0.0f, 1.0f );
	simData.scaledTimeDelta = Min( timeDelta * 20.0f, 0.9f );

	if ( Config::cvDynamicWaterSimComputeAsync.Get() )
	{
		GetRenderer()->LoadBufferData( m_simulateConstants, 0, sizeof( SimulateData ), &simData );

		GpuApi::ComputeTaskDesc computeTask;
		// Sync with graphics, to ensure impulses are done and buffer data is ready.
		computeTask.m_insertSync = true;
		computeTask.m_constantBuffers[ 0 ] = m_simulateConstants;
		computeTask.m_constantBufferCount = 1;
		computeTask.m_uav = m_flipped ? m_tex1 : m_tex2;
		computeTask.m_uavIndex = 0;
		computeTask.m_inputTextures[ 0 ] = m_flipped ? m_tex2 : m_tex1;
		computeTask.m_inputTextures[ 1 ] = m_appliedImpulses ? m_input : GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D );
		computeTask.m_inputTextureCount = 2;
		computeTask.m_inputBufferCount = 0;
		computeTask.m_shader = GetRenderer()->m_shaderDynamicWaterCompute->GetShader()->GetShader();
		computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
		computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
		computeTask.m_threadGroupZ = 1;

		if ( trulyAsync )
		{
			GpuApi::AddAsyncComputeTaskToQueue( computeTask );
		}
		else
		{
			GpuApi::AddSyncComputeTaskToQueue( computeTask );
		}
	}
	else
	{
		GpuApi::SetComputeShaderConsts( simData );

		GpuApi::TextureRef outputTex = m_flipped ? m_tex1 : m_tex2;
		GpuApi::BindTextureUAVs( 0, 1, &outputTex );

		GpuApi::TextureRef inputs[] = {
			m_flipped ? m_tex2 : m_tex1,
			m_appliedImpulses ? m_input : GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D )
		};
		GpuApi::BindTextures( 0, 2, inputs, GpuApi::ComputeShader );

		GetRenderer()->m_shaderDynamicWaterCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 );

		// Clear output UAVs
		GpuApi::BindTextureUAVs( 0, 1, nullptr );
		GpuApi::BindTextures( 0, 2, nullptr, GpuApi::ComputeShader );
	}
}

void CRenderSimDynamicWater::DoSimulatePixel( Float timeDelta, const Vector& cameraDelta )
{
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	GpuApi::RenderTargetSetup rtSetup1;
	rtSetup1.SetColorTarget(0, m_tex1);
	rtSetup1.SetViewport(m_resolution, m_resolution);

	GpuApi::RenderTargetSetup rtSetup2;
	rtSetup2.SetColorTarget(0, m_tex2);
	rtSetup2.SetViewport(m_resolution, m_resolution);

	GpuApi::SamplerStateRef stateRef = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_BorderPointNoMip );
	GpuApi::SetSamplerState( 0, stateRef, GpuApi::PixelShader );


	GpuApi::TextureRef refs[1] = { m_appliedImpulses ? m_input : GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D ) };
	GpuApi::SetSamplerState( 1, stateRef, GpuApi::PixelShader );


	CStandardRand& frandom = GetRenderer()->GetRandomNumberGenerator();

	Vector data( cameraDelta.X / DYNAMIC_WATER_WORLD_SIZE, cameraDelta.Y / DYNAMIC_WATER_WORLD_SIZE,  frandom.Get<Float>(), frandom.Get<Float>() );		
	Vector data2( m_rainIntensity, m_snappedCameraPosition.X,m_snappedCameraPosition.Y,m_snappedCameraPosition.Z );
	Vector data3( timeDelta, 0.0f, 0.0f, 0.0f );

	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, data );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, data2 );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, data3 );
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	if( m_flipped )
	{
		GpuApi::SetupRenderTargets(rtSetup1);
		GpuApi::BindTextures( 0, 1, &m_tex2, GpuApi::PixelShader );
		GpuApi::BindTextures( 1, 1, &(refs[0]), GpuApi::PixelShader );
	}
	else
	{
		GpuApi::SetupRenderTargets(rtSetup2);
		GpuApi::BindTextures( 0, 1, &m_tex1, GpuApi::PixelShader );
		GpuApi::BindTextures( 1, 1, &(refs[0]), GpuApi::PixelShader );
	}

	GetRenderer()->m_shaderDynamicWater->Bind();
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );
}



void CRenderSimDynamicWater::DoFinalizeCompute()
{
	PC_SCOPE_RENDER_LVL1( DynamicWater_FinalizeCompute );

	if ( Config::cvDynamicWaterSimComputeAsync.Get() )
	{
		GpuApi::ComputeTaskDesc computeTask;
		computeTask.m_constantBufferCount = 0;
		computeTask.m_uav = m_output;
		computeTask.m_uavIndex = 0;
		computeTask.m_inputTextures[ 0 ] = m_flipped ? m_tex2 : m_tex1;
		computeTask.m_inputTextureCount = 1;
		computeTask.m_inputBufferCount = 0;
		computeTask.m_shader = GetRenderer()->m_shaderDynamicWaterFinalizeCompute->GetShader()->GetShader();
		computeTask.m_threadGroupX = m_resolution / TILE_SIZE;
		computeTask.m_threadGroupY = m_resolution / TILE_SIZE;
		computeTask.m_threadGroupZ = 1;

		if ( trulyAsync )
		{
			GpuApi::AddAsyncComputeTaskToQueue( computeTask );
		}
		else
		{
			GpuApi::AddSyncComputeTaskToQueue( computeTask );
		}
	}
	else
	{
		GpuApi::BindTextureUAVs( 0, 1, &m_output );

		GpuApi::TextureRef inputTex = m_flipped ? m_tex2 : m_tex1;
		GpuApi::BindTextures( 0, 1, &inputTex, GpuApi::ComputeShader );

		GetRenderer()->m_shaderDynamicWaterFinalizeCompute->Dispatch( m_resolution / TILE_SIZE, m_resolution / TILE_SIZE, 1 );

		// Clear output UAVs
		GpuApi::BindTextureUAVs( 0, 1, nullptr );
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::ComputeShader );
	}
}


void CRenderSimDynamicWater::DoFinalizePixel()
{
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	GpuApi::RenderTargetSetup rtSetup3;
	rtSetup3.SetColorTarget(0, m_output);
	rtSetup3.SetViewport(m_resolution, m_resolution);

	GpuApi::SamplerStateRef stateRef = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_BorderPointNoMip );
	GpuApi::SetSamplerState( 0, stateRef, GpuApi::PixelShader );


	GpuApi::TextureRef refs[1] = { m_appliedImpulses ? m_input : GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D ) };
	GpuApi::BindTextures( 1, 1, &(refs[0]), GpuApi::PixelShader );
	GpuApi::SetSamplerState( 1, stateRef, GpuApi::PixelShader );

	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	if( m_flipped )
	{
		GpuApi::SetupRenderTargets(rtSetup3);
		GpuApi::BindTextures( 0, 1, &m_tex2, GpuApi::PixelShader );
	}
	else
	{
		GpuApi::SetupRenderTargets(rtSetup3);
		GpuApi::BindTextures( 0, 1, &m_tex1, GpuApi::PixelShader );
	}
	GetRenderer()->m_shaderDynamicWaterFinalize->Bind();	
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, m_points );

	GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
}


GpuApi::TextureRef CRenderSimDynamicWater::Calculate( GpuApi::TextureRef terrainHeightMapTextureArray, Float tim )
{
	PC_SCOPE_RENDER_LVL1( DynamicWater_Calculate);
	PC_SCOPE_GPU( DynamicWater_Calculate );

	InitialClear();

	Float dt = Clamp<Float>( tim - m_time, 0.0f, 0.1f );
	m_time = tim;
	
	if ( !ShouldSimulate() )
	{
		return GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D );
	}


	Vector delta = m_snappedCameraPosition - m_lastPos;
	m_lastPos = m_snappedCameraPosition;


	if ( m_simulationFence > 0 )
	{
		GpuApi::ReleaseFence( m_simulationFence );
		m_simulationFence = 0;
	}


	// Dynamic water
	if ( Config::cvDynamicWaterSimCompute.Get() )
	{
		DoSimulateCompute( dt, delta );
	}
	else
	{
		DoSimulatePixel( dt, delta );
	}

	m_flipped = !m_flipped;

	if ( Config::cvDynamicWaterSimCompute.Get() )
	{
		DoFinalizeCompute();

		if ( Config::cvDynamicWaterSimComputeAsync.Get() && trulyAsync )
		{
			m_simulationFence = GpuApi::KickoffAsyncComputeTasks();
		}
	}
	else
	{
		DoFinalizePixel();
	}

	return m_output;
}
