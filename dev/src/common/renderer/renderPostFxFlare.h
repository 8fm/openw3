
struct SFXFlareParameters
{
	Vector		m_screenPositionXY;
	Vector		m_bloomTint;
	Vector		m_worldPosition;
	Vector		m_spotDirection;
	Float		m_bloomTreshold;
	Float		m_bloomScale;
	Float		m_occlusionDistance;
	Float		m_blurScale;
	Float		m_screenFactor;
	Float		m_radius;
	Float		m_distance;
	Float		m_distanceFade;
	Float		m_outerRadius;
	Float		m_innerRadius;
	Bool		m_pointLight;
	Bool		m_spotLight;
	Bool		m_additive;

	SFXFlareParameters()
	{
		m_screenPositionXY = Vector( 0.5f, 0.5f, 0.0f, 0.0f );
		m_bloomTreshold = 0.0f;
		m_bloomScale = 2.0f;
		m_occlusionDistance = 100.0f;
		m_blurScale = 100.0f;
		m_bloomTint = Vector( 1.0f, 1.0f, 1.0f, 1.0f );
		m_screenFactor = 0.7f;
		m_pointLight = false;
		m_spotLight = false;
		m_additive = false;
		m_radius = 10.0f;
		m_distance = 10.0f;
		m_distanceFade = 0.0f;
		m_outerRadius = 90.0f;
		m_innerRadius = 80.0f;
	}
};

class CPostFxFlare
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:
	CPostFxFlare()
	{
	}

	~CPostFxFlare()
	{
	}

	void ApplyLowPass( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const SFXFlareParameters& params,
		ERenderTargetName sceneTarget, const TexelArea &sceneArea, 
		const TexelArea &depthArea, const CRenderFrameInfo& info )
	{
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

		CRenderShaderPair* shaderPair;

		if ( params.m_spotLight )
		{
			shaderPair = GetRenderer()->m_postfxFlareGrabSpot;
		}
		else if ( params.m_pointLight )
		{
			shaderPair = GetRenderer()->m_postfxFlareGrabPoint;
		}
		else
		{
			shaderPair = GetRenderer()->m_postfxFlareGrabFullscreen;
		}


		// Setup renderarea
		TexelArea renderArea ( GpuApi::GetViewport().width, GpuApi::GetViewport().height, 0, 0 );

		// Set textures
		ASSERT( RTN_None != sceneTarget );

		GpuApi::TextureRef targets[2] = { surfaces->GetRenderTargetTex( sceneTarget ), surfaces->GetDepthBufferTex() };
		GpuApi::BindTextures( 0, 2, &(targets[0]), GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Render
		const Int32 sceneFullWidth	= surfaces->GetRenderTarget( sceneTarget )->GetWidth();
		const Int32 sceneFullHeight	= surfaces->GetRenderTarget( sceneTarget )->GetHeight();			
		const Int32 depthFullWidth	= surfaces->GetWidth();
		const Int32 depthFullHeight	= surfaces->GetHeight();			
		const Float aspectRatio		= (Float) sceneArea.m_width / ( Float ) sceneArea.m_height;
		const Float halfTexelX = 1.0f / (Float)sceneFullWidth;
		const Float halfTexelY = 1.0f / (Float)sceneFullHeight;

		Vector camPos			= info.m_camera.GetPosition();
		Vector camForward		= info.m_camera.GetCameraForward();
		Vector camRight			= info.m_camera.GetCameraRight();
		Vector camUp			= info.m_camera.GetCameraUp();
		Swap( camPos.Y,		camPos.Z );
		Swap( camForward.Y,	camForward.Z );
		Swap( camRight.Y,	camRight.Z );
		Swap( camUp.Y,		camUp.Z );

		stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, sceneArea, sceneFullWidth, sceneFullHeight ) );
		stateManager.SetVertexConst( VSC_Custom_0 + 1,	CalculateTexCoordTransform( renderArea, depthArea, depthFullWidth, depthFullHeight ) );
		stateManager.SetVertexConst( VSC_Custom_0 + 2, CalculateViewSpaceVecTransform( renderArea, info.m_camera.GetFOV(), info.m_camera.GetAspect(), info.m_camera.GetZoom() ) );
		stateManager.SetVertexConst( VSC_Custom_0 + 3, camForward );
		stateManager.SetVertexConst( VSC_Custom_0 + 4, camRight );
		stateManager.SetVertexConst( VSC_Custom_0 + 5, camUp );
		stateManager.SetPixelConst( PSC_Custom_0 + 0,	Vector( params.m_bloomScale, params.m_bloomTreshold, 1.0f / params.m_occlusionDistance, params.m_radius ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 1,	Vector( params.m_screenPositionXY.X, params.m_screenPositionXY.Y, 0.0f, 1.0f ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 2,	Vector( params.m_bloomTint.X, params.m_bloomTint.Y, params.m_bloomTint.Z, 1.0f ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 3,	Vector( 1.0f, 1.0f / aspectRatio, 1.0f, aspectRatio ) );
		// Do not bind to PSC_Custom_0 + 4
		stateManager.SetPixelConst( PSC_Custom_0 + 9,	Vector( halfTexelX, halfTexelY, 0.0f, 0.0f ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 5,	Vector( 1.0f / (cosf( DEG2RAD(0.5f * params.m_innerRadius) ) - cosf(DEG2RAD(0.5f * params.m_outerRadius) ) ), params.m_distanceFade, params.m_radius, cosf( DEG2RAD(0.5f * params.m_outerRadius) ) ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 6,	info.m_camera.GetPosition() * Vector(1,1,1,0) + Vector(0,0,0,params.m_distance ));
		stateManager.SetPixelConst( PSC_Custom_0 + 7,	params.m_worldPosition );
		stateManager.SetPixelConst( PSC_Custom_0 + 8,	params.m_spotDirection );

		drawer.DrawQuad( shaderPair );

		// Unbind texture
		GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
	}

	void ApplyBlur( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const SFXFlareParameters& params,
		ERenderTargetName filteredTarget, const TexelArea &filteredArea )
	{
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

		// Setup renderarea
		TexelArea renderArea ( GpuApi::GetViewport().width, GpuApi::GetViewport().height, 0, 0 );

		// Set textures
		GpuApi::TextureRef target = surfaces->GetRenderTargetTex( filteredTarget );
		GpuApi::BindTextures( 0, 1, &target, GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 0, 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Blur scale as a percent of viewport size
		const Float blurScalePrc = Clamp< Float >( params.m_blurScale, 0.0f, 100.0f ) / 100.0f;

		// Render
		const Int32 sceneFullWidth	= surfaces->GetRenderTarget( filteredTarget )->GetWidth();
		const Int32 sceneFullHeight	= surfaces->GetRenderTarget( filteredTarget )->GetHeight();			
		const Float halfTexelX = 1.0f / (Float)sceneFullWidth;
		const Float halfTexelY = 1.0f / (Float)sceneFullHeight;

		const Vector texCoordTransform = CalculateTexCoordTransform( renderArea, filteredArea, sceneFullWidth, sceneFullHeight );
		stateManager.SetVertexConst( VSC_Custom_0 + 0, texCoordTransform );
		stateManager.SetPixelConst( PSC_Custom_0 + 0,	Vector( blurScalePrc, 0.0f, 0.0f, 0.0f ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 1,	Vector( params.m_screenPositionXY.X, params.m_screenPositionXY.Y, 0.0f, 0.0f ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 2,	Vector( halfTexelX, halfTexelY, filteredArea.m_width / (Float) sceneFullWidth - halfTexelX, filteredArea.m_height / (Float) sceneFullHeight - halfTexelY ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 3,	texCoordTransform );
		drawer.DrawQuad( GetRenderer()->m_postfxFlareFilter );

		// Unbind texture
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	}

	void ApplyToScene( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const Vector &tintColor, Float screenFactor, Float thresholdAmount,
		ERenderTargetName filteredTarget, const TexelArea &filteredArea, ERenderTargetName colorTarget, const TexelArea &colorArea )
	{
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

		CRenderShaderPair* shaderPair = GetRenderer()->m_postfxFlareApplyScreen;

		// Setup renderarea
		TexelArea renderArea( GpuApi::GetViewport().width, GpuApi::GetViewport().height, 0, 0 );

		// Set textures
		GpuApi::TextureRef targets[2] = { surfaces->GetRenderTargetTex( filteredTarget ), surfaces->GetRenderTargetTex( colorTarget ) };
		GpuApi::BindTextures( 0, 2, &(targets[0]), GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 0, 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Render
		const Int32 sceneFullWidth	= surfaces->GetRenderTarget( filteredTarget )->GetWidth();
		const Int32 sceneFullHeight	= surfaces->GetRenderTarget( filteredTarget )->GetHeight();			
		const Int32 targetFullWidth	= surfaces->GetRenderTarget( colorTarget )->GetWidth();
		const Int32 targetFullHeight	= surfaces->GetRenderTarget( colorTarget )->GetHeight();

		Float aspectRatio = (Float)filteredArea.m_width / (Float)filteredArea.m_height;

		stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, filteredArea, sceneFullWidth, sceneFullHeight ) );
		stateManager.SetVertexConst( VSC_Custom_0 + 1,	CalculateTexCoordTransform( renderArea, colorArea, targetFullWidth, targetFullHeight ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 2,	Vector( tintColor.X, tintColor.Y, tintColor.Z, screenFactor ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 3,	Vector::ONES * thresholdAmount );

		drawer.DrawQuad( shaderPair );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyToScene(
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const SFXFlareParameters& params,
		ERenderTargetName filteredTarget, const TexelArea &filteredArea, ERenderTargetName colorTarget, const TexelArea &colorArea )
	{
		ApplyToScene( drawer, surfaces, params.m_bloomTint, params.m_screenFactor, 1, filteredTarget, filteredArea, colorTarget, colorArea );
	}

	void ApplyToSceneAdditive( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const Vector &tintColor, Float screenFactor, 
		ERenderTargetName filteredTarget, const TexelArea &filteredArea )
	{
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

		CRenderShaderPair* shaderPair = GetRenderer()->m_postfxFlareApplyAdd;

		// Setup renderarea
		TexelArea renderArea( GpuApi::GetViewport().width, GpuApi::GetViewport().height, 0, 0 );

		// Set textures
		GpuApi::TextureRef target = surfaces->GetRenderTargetTex( filteredTarget );
		GpuApi::BindTextures( 0, 1, &target, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcAdd );

		// Render
		const Int32 sceneFullWidth	= surfaces->GetRenderTarget( filteredTarget )->GetWidth();
		const Int32 sceneFullHeight	= surfaces->GetRenderTarget( filteredTarget )->GetHeight();			

		Float aspectRatio = (Float)filteredArea.m_width / (Float)filteredArea.m_height;

		stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, filteredArea, sceneFullWidth, sceneFullHeight ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 2,	Vector( tintColor.X, tintColor.Y, tintColor.Z, screenFactor ) );
		stateManager.SetPixelConst( PSC_Custom_0 + 3,	Vector::ONES );

		drawer.DrawQuad( shaderPair );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	void ApplyToSceneAdditive( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const SFXFlareParameters& params,
		ERenderTargetName filteredTarget, const TexelArea &filteredArea )
	{
		ApplyToSceneAdditive( drawer, surfaces, params.m_bloomTint, params.m_screenFactor, filteredTarget, filteredArea );
	}

};