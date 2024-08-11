
class CPostFxBlurGauss
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
protected:

	enum { TAPS_COUNT = 10 };

	static Float CalcGauss( Float normFactor )
	{
		Float a = 1;
		Float b = 0;
		Float c = 3.1f;
		Float d = 6.4f;
		Float e = 1.5f;

		return powf( a * expf( - powf((normFactor - b) * d, 2) / (2 * c * c) ), e );
	}

	static void CalcTaps( Vector taps[TAPS_COUNT], const Vector &texelSize, Float intensity, bool horizontal )
	{
		Float offX  = -(TAPS_COUNT + 0.5f) * texelSize.X * (horizontal?1:0);
		Float offY  = -(TAPS_COUNT + 0.5f) * texelSize.Y * (horizontal?0:1);
		Float stepX = 2.f * texelSize.X * (horizontal?1:0);
		Float stepY = 2.f * texelSize.Y * (horizontal?0:1);

		Float weightsSum = 0;
		Float weights[TAPS_COUNT*2];
		for ( Int32 i=0; i<TAPS_COUNT; ++i )
		{
			Float w = CalcGauss( 1.f - i/(Float)(TAPS_COUNT-1) );
			weights[2*TAPS_COUNT-1-i] = w;
			weights[i] = w;
			weightsSum += 2 * w;
		}
		for ( Int32 i=0; i<TAPS_COUNT*2; ++i )
		{
			weights[i] /= weightsSum;
		}

		for ( Int32 tap_i=0; tap_i<TAPS_COUNT; ++tap_i )
		{
			Float w0 = weights[2*tap_i];
			Float w1 = weights[2*tap_i+1];
			Float t  = w1/(w0+w1);
			taps[tap_i].Set3( intensity * (offX + (tap_i + t - 0.5f) * stepX), intensity * (offY + (tap_i + t - 0.5f) * stepY), w0+w1 );
		}
	}

public:
	CPostFxBlurGauss ()
	{
	}

	~CPostFxBlurGauss ()
	{
	}

	void ApplyNiceBlur( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName renderTarget, const TexelArea &renderArea, 
		ERenderTargetName sampleTarget, const TexelArea &sampleArea,
		ERenderTargetName helperTarget, TexelArea helperArea,
		GpuApi::eDrawContext drawContext )
	{
		ASSERT( renderArea.IsBothDimsEqual( sampleArea ) );
		ASSERT( helperArea.IsBothDimsGreaterOrEqual( sampleArea ) );

		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		// Correct helper area to have the same size as sample area (we don't want any stretching)
		helperArea.m_width  = sampleArea.m_width;
		helperArea.m_height = sampleArea.m_height;

		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// Horizontal pass
		{
			ASSERT( RTN_None != sampleTarget );
			ASSERT( RTN_None != helperTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget ) );
			rtSetup.SetViewport( helperArea.m_width, helperArea.m_height, helperArea.m_offsetX, helperArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef sampleRef = surfaces->GetRenderTargetTex( sampleTarget );
			GpuApi::BindTextures( 0, 1, &sampleRef, GpuApi::PixelShader );
			const Int32 sampleFullWidth	= surfaces->GetRenderTarget( sampleTarget )->GetWidth();
			const Int32 sampleFullHeight	= surfaces->GetRenderTarget( sampleTarget )->GetHeight();
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( helperArea.GetWithNoOffset(), sampleArea, sampleFullWidth, sampleFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );

			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector(1.0f/sampleFullWidth,0,0,0) );

			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet ); // first pass always without blend

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGaussNice );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Vertical pass
		{
			ASSERT( RTN_None != helperTarget );
			ASSERT( RTN_None != renderTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef helperRef = surfaces->GetRenderTargetTex( helperTarget );
			GpuApi::BindTextures( 0, 1, &helperRef, GpuApi::PixelShader );
			const Int32 helperFullWidth	= surfaces->GetRenderTarget( helperTarget )->GetWidth();
			const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget )->GetHeight();			
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, helperArea, helperFullWidth, helperFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( helperArea, helperFullWidth, helperFullHeight ) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector(0,1.0f/helperFullHeight,0,0) );

			CGpuApiScopedDrawContext scopedDrawContext( drawContext );

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGaussNice );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Restore original rendertargets

		targetStateGrabber.Restore();
	}

	void ApplyNoStretch( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName renderTarget, const TexelArea &renderArea, 
		ERenderTargetName sampleTarget, const TexelArea &sampleArea,
		ERenderTargetName helperTarget, TexelArea helperArea,
		GpuApi::eDrawContext drawContext,
		Float intensity )
	{
		ASSERT( intensity > 0.f );
		ASSERT( renderArea.IsBothDimsEqual( sampleArea ) );
		ASSERT( helperArea.IsBothDimsGreaterOrEqual( sampleArea ) );

		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
		
		// Correct helper area to have the same size as sample area (we don't want any stretching)
		helperArea.m_width  = sampleArea.m_width;
		helperArea.m_height = sampleArea.m_height;

		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		
		// Horizontal pass
		{
			ASSERT( RTN_None != sampleTarget );
			ASSERT( RTN_None != helperTarget );
			
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget ) );
			rtSetup.SetViewport( helperArea.m_width, helperArea.m_height, helperArea.m_offsetX, helperArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef sampleRef = surfaces->GetRenderTargetTex( sampleTarget );
			GpuApi::BindTextures( 0, 1, &sampleRef, GpuApi::PixelShader );
			const Int32 sampleFullWidth	= surfaces->GetRenderTarget( sampleTarget )->GetWidth();
			const Int32 sampleFullHeight	= surfaces->GetRenderTarget( sampleTarget )->GetHeight();
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( helperArea.GetWithNoOffset(), sampleArea, sampleFullWidth, sampleFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );
			
			Vector taps[TAPS_COUNT];
			CalcTaps( taps, CalculateTexCoordTexelOffset( sampleFullWidth, sampleFullHeight ), intensity, true );
			for ( Int32 tap_i=0; tap_i<TAPS_COUNT; ++tap_i )
				stateManager.SetPixelConst(  PSC_Custom_0 + 1 + tap_i, taps[tap_i] );		
			
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet ); // first pass always without blend

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGauss );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Vertical pass
		{
			ASSERT( RTN_None != helperTarget );
			ASSERT( RTN_None != renderTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef helperRef = surfaces->GetRenderTargetTex( helperTarget );
			GpuApi::BindTextures( 0, 1, &helperRef, GpuApi::PixelShader );
			const Int32 helperFullWidth	= surfaces->GetRenderTarget( helperTarget )->GetWidth();
			const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget )->GetHeight();			
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, helperArea, helperFullWidth, helperFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( helperArea, helperFullWidth, helperFullHeight ) );
			
			Vector taps[TAPS_COUNT];
			CalcTaps( taps, CalculateTexCoordTexelOffset( helperFullWidth, helperFullHeight ), intensity, false );
			for ( Int32 tap_i=0; tap_i<TAPS_COUNT; ++tap_i )
				stateManager.SetPixelConst(  PSC_Custom_0 + 1 + tap_i, taps[tap_i] );					

			CGpuApiScopedDrawContext scopedDrawContext( drawContext );

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGauss );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Restore original rendertargets

		targetStateGrabber.Restore();
	}

	void ApplyStrong( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName renderTarget, const TexelArea &renderArea, 
		ERenderTargetName sampleTarget, const TexelArea &sampleArea,
		ERenderTargetName helperTarget, TexelArea helperArea,
		GpuApi::eDrawContext drawContext,
		Float intensity )
	{
		ASSERT( intensity > 0.f );
		ASSERT( renderArea.IsBothDimsEqual( sampleArea ) );
		ASSERT( helperArea.IsBothDimsGreaterOrEqual( sampleArea ) );

		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		// Correct helper area to have the same size as sample area (we don't want any stretching)
		helperArea.m_width  = sampleArea.m_width;
		helperArea.m_height = sampleArea.m_height;

		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// Horizontal pass
		{
			ASSERT( RTN_None != sampleTarget );
			ASSERT( RTN_None != helperTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget ) );
			rtSetup.SetViewport( helperArea.m_width, helperArea.m_height, helperArea.m_offsetX, helperArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef sampleRef = surfaces->GetRenderTargetTex( sampleTarget );
			GpuApi::BindTextures( 0, 1, &sampleRef, GpuApi::PixelShader );
			const Int32 sampleFullWidth	= surfaces->GetRenderTarget( sampleTarget )->GetWidth();
			const Int32 sampleFullHeight	= surfaces->GetRenderTarget( sampleTarget )->GetHeight();
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( helperArea.GetWithNoOffset(), sampleArea, sampleFullWidth, sampleFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector(1.0f/sampleFullWidth,0,0,0) );

			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet ); // first pass always without blend

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGaussStrong );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Vertical pass
		{
			ASSERT( RTN_None != helperTarget );
			ASSERT( RTN_None != renderTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef helperRef = surfaces->GetRenderTargetTex( helperTarget );
			GpuApi::BindTextures( 0, 1, &helperRef, GpuApi::PixelShader );
			const Int32 helperFullWidth	= surfaces->GetRenderTarget( helperTarget )->GetWidth();
			const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget )->GetHeight();			
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, helperArea, helperFullWidth, helperFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( helperArea, helperFullWidth, helperFullHeight ) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector(0,1.0f/helperFullHeight,0,0) );			

			CGpuApiScopedDrawContext scopedDrawContext( drawContext );

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGaussStrongLess );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Restore original rendertargets

		targetStateGrabber.Restore();
	}

	void ApplyHorizontal( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName renderTarget, const TexelArea &renderArea, 
		ERenderTargetName sampleTarget, const TexelArea &sampleArea,
		ERenderTargetName helperTarget, TexelArea helperArea,
		GpuApi::eDrawContext drawContext,
		Float intensity )
	{
		if( intensity == 0.f )
		{
			return;
		}
		ASSERT( renderArea.IsBothDimsEqual( sampleArea ) );
		ASSERT( helperArea.IsBothDimsGreaterOrEqual( sampleArea ) );

		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		// Correct helper area to have the same size as sample area (we don't want any stretching)
		helperArea.m_width  = sampleArea.m_width;
		helperArea.m_height = sampleArea.m_height;

		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// Horizontal pass
		{
			ASSERT( RTN_None != sampleTarget );
			ASSERT( RTN_None != helperTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget ) );
			rtSetup.SetViewport( helperArea.m_width, helperArea.m_height, helperArea.m_offsetX, helperArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef sampleRef = surfaces->GetRenderTargetTex( sampleTarget );
			GpuApi::BindTextures( 0, 1, &sampleRef, GpuApi::PixelShader );
			const Int32 sampleFullWidth	= surfaces->GetRenderTarget( sampleTarget )->GetWidth();
			const Int32 sampleFullHeight	= surfaces->GetRenderTarget( sampleTarget )->GetHeight();
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( helperArea.GetWithNoOffset(), sampleArea, sampleFullWidth, sampleFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector( Clamp<Float>(intensity * 2.f, 0.f, 1.f) /sampleFullWidth,0,0,0) );

			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet ); // first pass always without blend

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGaussStrong );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Vertical pass
		{
			ASSERT( RTN_None != helperTarget );
			ASSERT( RTN_None != renderTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef helperRef = surfaces->GetRenderTargetTex( helperTarget );
			GpuApi::BindTextures( 0, 1, &helperRef, GpuApi::PixelShader );
			const Int32 helperFullWidth	= surfaces->GetRenderTarget( helperTarget )->GetWidth();
			const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget )->GetHeight();			
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, helperArea, helperFullWidth, helperFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( helperArea, helperFullWidth, helperFullHeight ) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector( Clamp<Float>(intensity * 2.f - 1.f, 0.f, 1.f) /helperFullWidth,0.0f,0,0) );			

			CGpuApiScopedDrawContext scopedDrawContext( drawContext );

			drawer.DrawQuad( GetRenderer()->m_postfxBlurGaussStrong );

			// Unbind texture
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}

		// Restore original rendertargets

		targetStateGrabber.Restore();
	}
};
