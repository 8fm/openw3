/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderStateManager.h"
#include "gameplayFx.h"

CRenderStateManager::CRenderStateManager( )
	: m_prevCamera( nullptr )
	, m_prevViewportDimensions( 0.f, 0.f, 0.f, 0.f )
{
	Reset();
}
CRenderStateManager::~CRenderStateManager()
{
	GpuApi::SafeRelease( m_cameraConstantBuffer );
	GpuApi::SafeRelease( m_globalConstantBuffer );
}

void CRenderStateManager::Reset()
{
	// Because something else could have used the GpuApi to set shaders
	// but not gone through this render state manager
	for ( Int32 index = 0; index < RST_Max; ++index )
	{
		m_cachedShader[index] = GpuApi::ShaderRef::Null();
		GpuApi::SetShader( GpuApi::ShaderRef::Null(), Map( (ERenderShaderType) index ) );
	}

	// Reset sampler states
	GpuApi::InvalidateSamplerStates();
	GpuApi::SetCustomDrawContext(GpuApi::DSSM_Max, GpuApi::RASTERIZERMODE_Max, GpuApi::BLENDMODE_Max);
	GpuApi::BindMainConstantBuffers();
	BindGlobalConstants();
}

void CRenderStateManager::ForceNullPS( Bool enable )
{
	m_forceNullPS = enable;
	if ( !m_cachedShader[RST_PixelShader].isNull() )
	{
		m_cachedShader[RST_PixelShader] = GpuApi::ShaderRef::Null();
		GpuApi::SetShader( GpuApi::ShaderRef::Null(), Map( RST_PixelShader ) );
	}
}


void CRenderStateManager::SetGlobalShaderConstants( const CRenderFrameInfo &info, Uint32 fullRenderTargetWidth, Uint32 fullRenderTargetHeight, CGameplayEffects* gameplayFx )
{
	// Lock CB
	SGlobalConstantBuffer *cbuff = nullptr;
	{
		if ( !m_globalConstantBuffer )
		{
			m_globalConstantBuffer = GpuApi::CreateBuffer( sizeof(SGlobalConstantBuffer), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
			ASSERT ( m_globalConstantBuffer );
			GpuApi::SetBufferDebugPath( m_globalConstantBuffer, "global cbuffer" );
		}

		// Lock the buffer
		void* constantData = GpuApi::LockBuffer( m_globalConstantBuffer, GpuApi::BLF_Discard, 0, sizeof(SGlobalConstantBuffer) );
		if ( !constantData )
		{
			RED_ASSERT( !"Failed to lock global constant buffer" );
			return;
		}

		cbuff = static_cast<SGlobalConstantBuffer*>( constantData );
	}

	// Set Time
	cbuff->TimeVector.Set4( GGame ? info.m_cleanEngineTime : info.m_frameTime, (Float)info.m_gameDays, info.m_worldTime, MFract( info.m_worldTime / (60.0f * 60.0f * 24.0f ) ) );

	// Set viewport size shaders info
	cbuff->ViewportSize.Set4( (Float)fullRenderTargetWidth, (Float)fullRenderTargetHeight, 1.f/fullRenderTargetWidth, 1.f/fullRenderTargetHeight );
	cbuff->ViewportSubSize.Set4( (Float)info.m_width, (Float)info.m_height, info.m_width/(Float)fullRenderTargetWidth, info.m_height/(Float)fullRenderTargetHeight );

	// Set color filter params
	const CEnvColorModTransparencyParametersAtPoint &colorModTransp = info.m_envParametersArea.m_colorModTransparency;
	const Vector colorValueNear = colorModTransp.m_filterNearColor.GetColorScaled( true );
	const Vector colorValueFar  = colorModTransp.m_filterFarColor.GetColorScaled( true );
	cbuff->EnvTranspFilterDistMinColor.Set4( colorValueNear.X, colorValueNear.Y, colorValueNear.Z, colorModTransp.m_contrastNearStrength.GetScalar() );
	cbuff->EnvTranspFilterDistMaxColor.Set4( colorValueFar.X,  colorValueFar.Y,  colorValueFar.Z,  colorModTransp.m_contrastFarStrength.GetScalar() );
	cbuff->EnvTranspFilterParams.Set4( 1.f / Max(0.001f, colorModTransp.m_commonFarDist.GetScalar()), 0.f, 0.f, 0.f );

	// Set water params (transparency)	
	cbuff->WaterShadingParams = info.m_envParametersDayPoint.m_waterShadingParams;
	cbuff->WaterShadingParamsExtra = info.m_envParametersDayPoint.m_waterShadingParamsExtra;	
	cbuff->WaterShadingParamsExtra2.Set4( info.m_envParametersDayPoint.m_waterFoamIntensity, 
		info.m_envParametersDayPoint.m_waterShadingParamsExtra2.X, info.m_envParametersDayPoint.m_waterShadingParamsExtra2.Y, info.m_envParametersDayPoint.m_waterShadingParamsExtra2.Z );
	cbuff->UnderWaterShadingParams.Set4( info.m_envParametersDayPoint.m_underWaterBrightness, info.m_envParametersDayPoint.m_underWaterFogIntensity, 0.0f, 0.0f );

	// Set weather params
	{
		cbuff->WeatherAndPrescaleParams.Set4(
			info.m_envParametersDayPoint.m_fakeCloudsShadowSize,
			info.m_envParametersDayPoint.m_weatherEffectStrength,
			info.m_envParametersDayPoint.m_windParameters.GetCloudsShadowsOffset().Z * info.m_envParametersDayPoint.m_fakeCloudsShadowSpeed, 
			info.m_envParametersDayPoint.m_skyBoxWeatherBlend.Z );

		cbuff->SkyboxShadingParams.Set4(
			info.m_envParametersDayPoint.m_fakeCloudsShadowCurrentTextureIndex, 
			info.m_envParametersDayPoint.m_fakeCloudsShadowTargetTextureIndex,
			info.m_envParametersDayPoint.m_skyBoxWeatherBlend.X,
			info.m_envParametersDayPoint.m_skyBoxWeatherBlend.Y );

		Float windDirZ = info.m_envParametersDayPoint.m_windParameters.GetWindRotationZ();
		Float windScale = info.m_envParametersDayPoint.m_windParameters.GetWindScale();

		cbuff->WeatherShadingParams.Set4( info.m_envParametersDayPoint.m_weatherEffectStrength, windScale, windDirZ, info.m_envParametersDayPoint.m_delayedWetSurfaceEffectStrength );
	}

	// Set gameplay effects parameters
	cbuff->GameplayEffectsRendererParameter = gameplayFx->GetParametersVector();

	// Set world/area environment related params
	Vector	globalLightColor = info.m_envParametersArea.m_globalLight.m_sunColor.GetColorScaled(true);
	cbuff->GlobalLightDir = info.m_envParametersDayPoint.m_globalLightDirection * Vector (1,1,1,0) + Vector (0,0,0,1) * Clamp( info.m_envParametersDayPoint.m_skyDayAmount, 0.f, 1.f );
	cbuff->GlobalLightColorAndSpecScale = Vector (1,1,1,0) * globalLightColor + Vector (0,0,0,1);

	GpuApi::UnlockBuffer( m_globalConstantBuffer );
}

void CRenderStateManager::BindGlobalConstants()
{
	GpuApi::BindConstantBuffer( 0, m_globalConstantBuffer, GpuApi::VertexShader );
	GpuApi::BindConstantBuffer( 0, m_globalConstantBuffer, GpuApi::HullShader );
	GpuApi::BindConstantBuffer( 0, m_globalConstantBuffer, GpuApi::DomainShader );
	GpuApi::BindConstantBuffer( 0, m_globalConstantBuffer, GpuApi::GeometryShader );
	GpuApi::BindConstantBuffer( 0, m_globalConstantBuffer, GpuApi::PixelShader );
}

void CRenderStateManager::UnbindGlobalConstants()
{
	GpuApi::BindConstantBuffer( 0, GpuApi::BufferRef::Null(), GpuApi::VertexShader );
	GpuApi::BindConstantBuffer( 0, GpuApi::BufferRef::Null(), GpuApi::HullShader );
	GpuApi::BindConstantBuffer( 0, GpuApi::BufferRef::Null(), GpuApi::DomainShader );
	GpuApi::BindConstantBuffer( 0, GpuApi::BufferRef::Null(), GpuApi::GeometryShader );
	GpuApi::BindConstantBuffer( 0, GpuApi::BufferRef::Null(), GpuApi::PixelShader );
}

void CRenderStateManager::SetCamera2D()
{
	// Get viewport size
	GpuApi::ViewportDesc viewport;
	GpuApi::GetViewport( viewport );
	
	if ( (Float)viewport.width != m_prevViewportDimensions.X || (Float)viewport.height != m_prevViewportDimensions.Y )
	{
		// Calculate canvas matrix, used for 2D drawing
		Matrix canvasToScreen( 
			Vector( 2.0f / (Float)viewport.width, 0.0f, 0.0f, 0.0f ),
			Vector( 0.0f, -2.0f / (Float)viewport.height, 0.0f, 0.0f ),
			Vector( 0.0f, 0.0f, 1.0f, 0.0f ),
			Vector( -1.0f /*+ 0.5f / (Float)GetWidth()*/, 1.0f /*- 0.5f / (Float)GetHeight()*/, 0.0f, 1.0f )
			);

		Matrix screenToCanvas = canvasToScreen.Inverted();

		if ( m_cameraConstantBuffer.isNull() )
		{
			m_cameraConstantBuffer = GpuApi::CreateBuffer( sizeof(SCameraConstants), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
			GpuApi::SetBufferDebugPath( m_cameraConstantBuffer, "cameraConstBuffer" );
		}
		SCameraConstants cameraConstants;
		cameraConstants.worldToScreen = canvasToScreen.Transposed();
		cameraConstants.worldToView = Matrix::IDENTITY;
		cameraConstants.cameraPosition = Vector::ZEROS;
		cameraConstants.cameraVectorRight = Vector::ZEROS;
		cameraConstants.cameraVectorForward = Vector::ZEROS;
		cameraConstants.cameraVectorUp = Vector::ZEROS;
		cameraConstants.viewportParams = Vector( (Float)viewport.width, (Float)viewport.height, (Float)viewport.minZ, (Float)viewport.maxZ );
		cameraConstants.revProjCameraInfo = Vector ( 1, 0, 0, 0 );
		
		cameraConstants.wetSurfaceEffect = m_weatherParams;

		void* mappedBuffer = GpuApi::LockBuffer( m_cameraConstantBuffer, GpuApi::BLF_Discard, 0, sizeof(SCameraConstants) );
		Red::System::MemoryCopy(mappedBuffer, &cameraConstants, sizeof(SCameraConstants));
		GpuApi::UnlockBuffer( m_cameraConstantBuffer );

		m_prevViewportDimensions.Set4((Float)viewport.width, (Float)viewport.height, 0,0);
		m_prevCamera = nullptr;
	}

	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::VertexShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::GeometryShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::HullShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::DomainShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::PixelShader );
}

void CRenderStateManager::SetCamera( const CRenderCamera& camera )
{
	//// in case someone wants to track the redundancy
	//if ( m_prevCamera == &camera )
	//{
	//	RED_LOG_WARNING( RED_LOG_CHANNEL( Renderer ), TXT("Camera set redundantly, potential performance loss") );
	//}

	if ( m_cameraConstantBuffer.isNull() )
	{
		m_cameraConstantBuffer = GpuApi::CreateBuffer( sizeof(SCameraConstants), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		if ( m_cameraConstantBuffer.isNull() )
		{
			ERR_RENDERER( TXT("Could not create CameraConstantBuffer - expect errors.") );
			return;
		}
		GpuApi::SetBufferDebugPath( m_cameraConstantBuffer, "cameraConstBuffer" );
	}
	SCameraConstants cameraConstants;
	cameraConstants.worldToScreen = camera.GetWorldToScreenRevProjAware().Transposed();
	cameraConstants.worldToView = camera.GetWorldToView().Transposed();
	cameraConstants.cameraPosition = camera.GetPosition();
	cameraConstants.cameraVectorRight = camera.GetCameraRight();
	cameraConstants.cameraVectorForward = camera.GetCameraForward();
	cameraConstants.cameraVectorUp = camera.GetCameraUp();
	cameraConstants.revProjCameraInfo.Set4( camera.IsReversedProjection() ? 0.f : 1.f, 0.f, 0.f, 0.f );
	
	Float oneOverNear = 1.f/camera.GetNearPlane();
	Float oneOverFar = 1.f/camera.GetFarPlane();
	Float oneOverW = 1.f/camera.GetViewToScreen().V[0].A[0];
	Float oneOverH = 1.f/camera.GetViewToScreen().V[1].A[1];
	cameraConstants.cameraNearFar.Set4( oneOverFar, oneOverNear, oneOverW, oneOverH );

	GpuApi::ViewportDesc viewport;
	GpuApi::GetViewport( viewport );
	cameraConstants.viewportParams = Vector( (float)viewport.width, (float)viewport.height, (float)viewport.minZ, (float)viewport.maxZ );

	cameraConstants.wetSurfaceEffect = m_weatherParams;

	void* mappedBuffer = GpuApi::LockBuffer( m_cameraConstantBuffer, GpuApi::BLF_Discard, 0, sizeof(SCameraConstants) );
	Red::System::MemoryCopy(mappedBuffer, &cameraConstants, sizeof(SCameraConstants));
	GpuApi::UnlockBuffer( m_cameraConstantBuffer );

	m_prevCamera = &camera;
	m_prevViewportDimensions.Set4( 0,0,0,0 );

	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::VertexShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::GeometryShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::HullShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::DomainShader );
	GpuApi::BindConstantBuffer( 1, m_cameraConstantBuffer, GpuApi::PixelShader );
}

void CRenderStateManager::SetLocalToWorld( const Matrix* matrix )
{
	if ( matrix )
	{
		SetVertexConst( VSC_LocalToWorld, *matrix );
	}
	else
	{
		static Matrix identity( Vector(1,0,0,0), Vector(0,1,0,0), Vector(0,0,1,0), Vector(0,0,0,1) );
		SetVertexConst( VSC_LocalToWorld, identity );
	}
}

void CRenderStateManager::SetLocalToWorldF( const Float* floatArray )
{
	if( floatArray )
	{
		SetVertexShaderConstRaw( VSC_LocalToWorld, floatArray, 4 );
	}
	else
	{
		static Matrix identity( Vector(1,0,0,0), Vector(0,1,0,0), Vector(0,0,1,0), Vector(0,0,0,1) );
		SetVertexConst( VSC_LocalToWorld, identity );
	}
}

void CRenderStateManager::PushShaderSetup()
{
	for (Int32 i = 0; i < RST_Max; ++i)
	{
		m_shaderStack.PushBack(m_cachedShader[i]);
	}
}

void CRenderStateManager::PopShaderSetup()
{
	for (Int32 i = RST_Max-1; i>=0; i--)
	{
		const GpuApi::ShaderRef& shader = m_shaderStack.PopBack();

		m_cachedShader[i] = shader;
		GpuApi::SetShader( shader, Map((ERenderShaderType)i) );
	}
}


