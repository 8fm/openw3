
class CPostFxDepthOfField
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:
	CPostFxDepthOfField ()
	{
	}

	~CPostFxDepthOfField ()
	{
	}

	bool ApplyGameplayDof( CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const CRenderFrameInfo &info, Float blendScale, ERenderTargetName renderTarget, ERenderTargetName helperTarget )
	{	
		RED_ASSERT( renderTarget != helperTarget );

		const CRenderCamera &renderCamera = info.m_camera;
		const CEnvDepthOfFieldParametersAtPoint &dofParams = info.m_envParametersArea.m_depthOfField;
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		
		// Set common states
		Float paramNear			= dofParams.m_farFocusDist.GetScalar();
		Float paramFar			= dofParams.m_farBlurDist.GetScalar();
		Float paramNear2		= dofParams.m_nearFocusDist.GetScalar();
		Float paramFar2			= dofParams.m_nearBlurDist.GetScalar();
		
		// Reject gameplay dof if camera's far plane is closer than closes dof blur range
		if( paramNear > renderCamera.GetFarPlane() )
		{
			return false;
		}

		Float paramIntensity	= dofParams.m_intensity.GetScalarClamp( 0.f, 1.f ) * blendScale;
		const Vector dofParamsVector  = Vector ( paramNear, paramFar, paramIntensity, 1.f / Max( 0.001f, paramFar - paramNear ) );
		const Vector dofParamsVector2 = Vector ( paramNear2, paramFar2, paramIntensity, 1.f / Max( 0.001f, paramNear2 - paramFar2 ) );
		const Vector dofParamsVector3 = Vector ( dofParams.m_skyThreshold, 1.f / Max(0.0001f, dofParams.m_skyRange), 0, 1 );

		GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		GpuApi::SetSamplerStateCommon( 1, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip );

		stateManager.SetPixelConst( PSC_Custom_0, dofParamsVector  );
		stateManager.SetPixelConst( PSC_Custom_1, dofParamsVector2 );
		stateManager.SetPixelConst( PSC_Custom_2, dofParamsVector3 );

		Float drawZ = 0;
		RED_ASSERT( !GpuApi::IsReversedProjectionState() );
		{
			const Float cameraNear = renderCamera.GetNearPlane();
			const Float cameraFar = renderCamera.GetFarPlane();
			const Float oneOverNear = 1.f/info.m_camera.GetNearPlane();
			const Float oneOverFar = 1.f/info.m_camera.GetFarPlane();
			const Float xx = oneOverFar - oneOverNear;
			const Float yy = oneOverNear;
			drawZ = 1.f / (xx * paramNear) - yy / xx;

			if ( renderCamera.IsReversedProjection() )
			{
				drawZ = 1.f - drawZ;
			}
		}

		//
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
		CGpuApiScopedDrawContext scopedContext;
		
		//
		// Prepare helper texture
		{
			// Setup targets
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget ) );
			rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex(), -1, true );
			rtSetup.SetViewport( info.m_width, info.m_height );
			GpuApi::SetupRenderTargets( rtSetup );

			GetRenderer()->ClearColorTarget( Vector::ZEROS );

			//
			const GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( renderTarget ), surfaces->GetDepthBufferTex() };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

			//
			GpuApi::SetCustomDrawContext( renderCamera.IsReversedProjection() ? GpuApi::DSSM_NoStencilDepthTestGE : GpuApi::DSSM_NoStencilDepthTestLE, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );
			drawer.DrawQuad( GetRenderer()->m_postfxDofGameplayPrepare, drawZ );

			//
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
		}

		// Render DOF
		{
			// Setup targets
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
			rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex(), -1, true );
			rtSetup.SetViewport( info.m_width, info.m_height );
			GpuApi::SetupRenderTargets( rtSetup );

			//
			const GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( helperTarget ), surfaces->GetDepthBufferTex() };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

			//
			GpuApi::SetCustomDrawContext( renderCamera.IsReversedProjection() ? GpuApi::DSSM_NoStencilDepthTestGE : GpuApi::DSSM_NoStencilDepthTestLE, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Blend );
			drawer.DrawQuad( GetRenderer()->m_postfxDofGameplayApply, drawZ );

			//
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
		}

		return true;
	}

	void CalculateCutsceneDofClampMax( class CRenderSurfaces* surfaces, GpuApi::TextureRef texture, const TexelArea &area, Float &outX, Float &outY )
	{
		RED_ASSERT( 0 == area.m_offsetX );
		RED_ASSERT( 0 == area.m_offsetY );
		if ( surfaces && texture )
		{
			const GpuApi::TextureLevelDesc desc = GpuApi::GetTextureLevelDesc( texture, 0 );
			outX = (area.m_width - 0.5f) / Max( 1.f, (Float)desc.width );
			outY = (area.m_height - 0.5f) / Max( 1.f, (Float)desc.height );
		}
		else
		{
			outX = 1;
			outY = 1;
		}
	}

	void ApplyCutsceneDof( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const CEnvDepthOfFieldParametersAtPoint &dofParams, Float blendScale, 
		ERenderTargetName sampleTarget,  TexelArea sampleArea,
		ERenderTargetName helperTarget1, TexelArea helperArea1,
		ERenderTargetName helperTarget2, TexelArea helperArea2,
		ERenderTargetName helperTarget3, TexelArea helperArea3,
		TexelArea depthArea )
	{
		Float clampMaxHelper1[2];
		Float clampMaxHelper2[2];
		Float clampMaxHelper3[2];
		Float clampMaxDepth[2];
		CalculateCutsceneDofClampMax( surfaces, surfaces->GetRenderTargetTex( helperTarget1 ), helperArea1, clampMaxHelper1[0], clampMaxHelper1[1] );
		CalculateCutsceneDofClampMax( surfaces, surfaces->GetRenderTargetTex( helperTarget2 ), helperArea2, clampMaxHelper2[0], clampMaxHelper2[1] );
		CalculateCutsceneDofClampMax( surfaces, surfaces->GetRenderTargetTex( helperTarget3 ), helperArea3, clampMaxHelper3[0], clampMaxHelper3[1] );
		CalculateCutsceneDofClampMax( surfaces, surfaces->GetDepthBufferTex(), depthArea, clampMaxDepth[0], clampMaxDepth[1] );

		///////////////////////////////////////////////////////////////////
		// Distance that cause far bokeh not to be visible at all. Bcoz sometimes
		// its needed to skip all background including sky & clouds. In that case
		// Using paramFarFocus >= 100 uses seperate shader to not calculate far bokeh.

		// Thanks to numerical inaccuracies, something that should be ==100 might be
		// slightly more or slightly less. So bump this down a bit.
		const Float farDiscardDistance = 99.995f;

		// Some dof blur scaling
		// That was forced by cutscene-guys ;) May be moved as property some sunny day.
		const Float paramDofBlurScale = 0.5f;
		///////////////////////////////////////////////////////////////////

		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
		
		const Int32 origViewportWidth  = GpuApi::GetViewport().width;
		const Int32 origViewportHeight = GpuApi::GetViewport().height;
		TexelArea renderArea ( origViewportWidth, origViewportHeight, 0, 0 );		

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set some renderstates
		GpuApi::SetSamplerStateCommon( 0, 3, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		Float paramNearBlur		= dofParams.m_nearBlurDist.GetScalar();
		Float paramNearFocus	= dofParams.m_nearFocusDist.GetScalar();
		Float paramFarFocus		= dofParams.m_farFocusDist.GetScalar();
		Float paramFarBlur		= dofParams.m_farBlurDist.GetScalar();
		Float paramIntensity	= Clamp<Float>( dofParams.m_intensity.GetScalar() , 0.0f, 1.0f ) * blendScale;

		{
			paramNearFocus = Min( paramNearFocus, paramFarFocus );
			paramNearBlur  = Min( paramNearBlur, paramNearFocus );
			paramFarBlur   = Max( paramFarBlur, paramFarFocus );
			
		}
		const Vector dofParamsVectorRange		= Vector ( paramNearFocus, 1.f / Max( 0.001f, (paramNearFocus - paramNearBlur)), paramFarFocus, 1.f / Max( 0.001f, (paramFarBlur - paramFarFocus)) );

		// X is used mostly in blurring (bcoz of the extra hacky 'paramDofBlurScale' artists driven. Y is regular Intensivity for scaling the merge effect
		const Vector dofParamsVectorIntensity	= Vector ( paramIntensity * paramDofBlurScale , paramIntensity , 0, 0 );

		// Downsample
		{
			Rect sampleRect( sampleArea.m_offsetX, sampleArea.m_offsetX + sampleArea.m_width, sampleArea.m_offsetY, sampleArea.m_offsetY + sampleArea.m_height );
			Rect helperRect( helperArea1.m_offsetX, helperArea1.m_offsetX + helperArea1.m_width, helperArea1.m_offsetY, helperArea1.m_offsetY + helperArea1.m_height );
			GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( sampleTarget ), sampleRect, surfaces->GetRenderTargetTex( helperTarget1 ), helperRect );
		}

		CRenderShaderPair* blurShader	= nullptr;
		CRenderShaderPair* mergeShader	= nullptr;

		if( paramFarFocus-paramNearFocus >= farDiscardDistance )
		{
			blurShader = GetRenderer()->m_postfxDofBlurNoFarCS;
			mergeShader = GetRenderer()->m_postfxDofMergeNoFarCS;
		}
		else
		{
			blurShader = GetRenderer()->m_postfxDofBlurCS;
			mergeShader = GetRenderer()->m_postfxDofMergeCS;
		}

		// Primary blur
		{ 
			// Blur horizontal

			{
				// Setup targets
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget2 ) );
					rtSetup.SetViewport( helperArea2.m_width, helperArea2.m_height, helperArea2.m_offsetX, helperArea2.m_offsetY );
					GpuApi::SetupRenderTargets( rtSetup );
				}

				// Set textures
				ASSERT( RTN_None != helperTarget1 );

				// NOTE : We're binding Depth to t1 here, and setting the depth TexCoordTransform to VSC_Custom_1. These will be used for
				// the next four blur passes, so don't unbind or change them!

				GpuApi::TextureRef targets[2] = { surfaces->GetRenderTargetTex( helperTarget1 ), surfaces->GetDepthBufferTex() };
				GpuApi::BindTextures( 0, 2, &(targets[0]), GpuApi::PixelShader );

				// Render
				const Int32 helperFullWidth		= surfaces->GetRenderTarget( helperTarget1 )->GetWidth();
				const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget1 )->GetHeight();
				const Int32 depthFullWidth		= surfaces->GetWidth();
				const Int32 depthFullHeight		= surfaces->GetHeight();
				stateManager.SetVertexConst( VSC_Custom_0,	CalculateTexCoordTransform( helperArea2.GetWithNoOffset(), helperArea1,	helperFullWidth, helperFullHeight ) );		
				stateManager.SetVertexConst( VSC_Custom_1,	CalculateTexCoordTransform( depthArea.GetWithNoOffset(), depthArea,	depthFullWidth, depthFullHeight ) );		
				stateManager.SetPixelConst( PSC_Custom_0,	Vector ( 1, 0, 0, 0 ) * CalculateTexCoordTexelOffset( helperFullWidth, helperFullHeight ) );
				stateManager.SetPixelConst( PSC_Custom_1,	dofParamsVectorRange );
				stateManager.SetPixelConst( PSC_Custom_2,	dofParamsVectorIntensity );
				stateManager.SetPixelConst( PSC_Custom_3,	Vector ( clampMaxHelper1[0], clampMaxHelper1[1], clampMaxDepth[0], clampMaxDepth[1] ) );
				
				drawer.DrawQuad( blurShader );

				// Don't unbind depth, we'll use it again.
				GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
			}

			// Blur vertical

			{
				// Setup targets
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget1 ) );
					rtSetup.SetViewport( helperArea1.m_width, helperArea1.m_height, helperArea1.m_offsetX, helperArea1.m_offsetY );
					GpuApi::SetupRenderTargets( rtSetup );
				}
				
				// Set textures
				ASSERT( RTN_None != helperTarget2 );

				// Depth texture and texcoord transform are still set from the horizontal pass

				GpuApi::TextureRef target = surfaces->GetRenderTargetTex( helperTarget2 );
				GpuApi::BindTextures( 0, 1, &target, GpuApi::PixelShader );

				// Render
				const Int32 helperFullWidth		= surfaces->GetRenderTarget( helperTarget2 )->GetWidth();
				const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget2 )->GetHeight();
				stateManager.SetVertexConst( VSC_Custom_0,	CalculateTexCoordTransform( helperArea1.GetWithNoOffset(), helperArea2, helperFullWidth, helperFullHeight ) );		
				stateManager.SetPixelConst( PSC_Custom_0,	Vector ( 0, 1, 0, 0 ) * CalculateTexCoordTexelOffset( helperFullWidth, helperFullHeight ) );		
				stateManager.SetPixelConst( PSC_Custom_1,	dofParamsVectorRange );
				stateManager.SetPixelConst( PSC_Custom_2,	dofParamsVectorIntensity );
				stateManager.SetPixelConst( PSC_Custom_3,	Vector ( clampMaxHelper2[0], clampMaxHelper2[1], clampMaxDepth[0], clampMaxDepth[1] ) );

				drawer.DrawQuad( blurShader );

				// Still keeping depth!
				GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
			}
		}

		// Secondary blur

		{
			// Blur horizontal

			{
				// Setup targets
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget3 ) );
					rtSetup.SetViewport( helperArea3.m_width, helperArea3.m_height, helperArea3.m_offsetX, helperArea3.m_offsetY );
					GpuApi::SetupRenderTargets( rtSetup );
				}
				
				// Set textures
				ASSERT( RTN_None != helperTarget1 );

				// Depth texture and texcoord transform are still set from the horizontal pass

				GpuApi::TextureRef target = surfaces->GetRenderTargetTex( helperTarget1 );
				GpuApi::BindTextures( 0, 1, &target, GpuApi::PixelShader );

				// Render
				const Int32 helperFullWidth		= surfaces->GetRenderTarget( helperTarget1 )->GetWidth();
				const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget1 )->GetHeight();
				stateManager.SetVertexConst( VSC_Custom_0,	CalculateTexCoordTransform( helperArea3.GetWithNoOffset(), helperArea1,	helperFullWidth, helperFullHeight ) );		
				stateManager.SetPixelConst( PSC_Custom_0,	Vector ( 1, 0, 0, 0 ) * CalculateTexCoordTexelOffset( helperFullWidth, helperFullHeight ) );
				stateManager.SetPixelConst( PSC_Custom_1,	dofParamsVectorRange );
				stateManager.SetPixelConst( PSC_Custom_2,	dofParamsVectorIntensity );
				stateManager.SetPixelConst( PSC_Custom_3,	Vector ( clampMaxHelper1[0], clampMaxHelper1[1], clampMaxDepth[0], clampMaxDepth[1] ) );

				drawer.DrawQuad( blurShader );

				// Still keeping depth!
				GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
			}

			// Blur vertical

			{
				// Setup targets
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperTarget2 ) );
					rtSetup.SetViewport( helperArea2.m_width, helperArea2.m_height, helperArea2.m_offsetX, helperArea2.m_offsetY );
					GpuApi::SetupRenderTargets( rtSetup );
				}

				// Set textures
				ASSERT( RTN_None != helperTarget3 );

				// Depth texture and texcoord transform are still set from the horizontal pass

				GpuApi::TextureRef target = surfaces->GetRenderTargetTex( helperTarget3 );
				GpuApi::BindTextures( 0, 1, &target, GpuApi::PixelShader );

				// Render
				const Int32 helperFullWidth		= surfaces->GetRenderTarget( helperTarget3 )->GetWidth();
				const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget3 )->GetHeight();
				stateManager.SetVertexConst( VSC_Custom_0,	CalculateTexCoordTransform( helperArea2.GetWithNoOffset(), helperArea3, helperFullWidth, helperFullHeight ) );		
				stateManager.SetPixelConst( PSC_Custom_0,	Vector ( 0, 1, 0, 0 ) * CalculateTexCoordTexelOffset( helperFullWidth, helperFullHeight ) );		
				stateManager.SetPixelConst( PSC_Custom_1,	dofParamsVectorRange );
				stateManager.SetPixelConst( PSC_Custom_2,	dofParamsVectorIntensity );
				stateManager.SetPixelConst( PSC_Custom_3,	Vector ( clampMaxHelper3[0], clampMaxHelper3[1], clampMaxDepth[0], clampMaxDepth[1] ) );

				drawer.DrawQuad( blurShader );

				// Now we can clear it out. Not _strictly_ required here...
				GpuApi::BindTextures( 0, 2, nullptr, GpuApi::PixelShader );
			}
		}

		// Done with blurring, so we can unbind/change the depth texture settings from before.

		// Restore rendertarget
		targetStateGrabber.Restore();

		// Merge

		{
			// Set textures
			ASSERT( RTN_None != sampleTarget );
			ASSERT( RTN_None != helperTarget1 );

			const GpuApi::TextureRef targets[4] = 
			{
				surfaces->GetRenderTargetTex( sampleTarget ), 
				surfaces->GetRenderTargetTex( helperTarget1 ),
				surfaces->GetRenderTargetTex( helperTarget2 ), 
				surfaces->GetDepthBufferTex()
			};
			GpuApi::BindTextures( 0, 4, &(targets[0]), GpuApi::PixelShader );

			// Set samplers
			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip,	GpuApi::PixelShader );	// sampleTarget
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip,	GpuApi::PixelShader );	// helperTarget1
			GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip,	GpuApi::PixelShader );	// helperTarget2
			GpuApi::SetSamplerStatePreset( 3, GpuApi::SAMPSTATEPRESET_ClampPointNoMip,	GpuApi::PixelShader );	// depth

			// Render
			ASSERT( helperArea1 == helperArea2 );
			ASSERT( surfaces->GetRenderTarget( helperTarget1 )->GetWidth()  == surfaces->GetRenderTarget( helperTarget2 )->GetWidth() );
			ASSERT( surfaces->GetRenderTarget( helperTarget1 )->GetHeight() == surfaces->GetRenderTarget( helperTarget2 )->GetHeight() );
			const Int32 sampleFullWidth		= surfaces->GetRenderTarget( sampleTarget )->GetWidth();
			const Int32 sampleFullHeight	= surfaces->GetRenderTarget( sampleTarget )->GetHeight();
			const Int32 helperFullWidth		= surfaces->GetRenderTarget( helperTarget1 )->GetWidth();
			const Int32 helperFullHeight	= surfaces->GetRenderTarget( helperTarget1 )->GetHeight();
			stateManager.SetVertexConst( VSC_Custom_0,	CalculateTexCoordTransform( renderArea, origViewportWidth, origViewportHeight, sampleArea, sampleFullWidth, sampleFullHeight ) );
			stateManager.SetVertexConst( VSC_Custom_1,	CalculateTexCoordTransform( renderArea, origViewportWidth, origViewportHeight, helperArea1, helperFullWidth, helperFullHeight ) );
			stateManager.SetPixelConst( PSC_Custom_0,	CalculateTexCoordTexelOffset( sampleFullWidth, sampleFullHeight ) );
			stateManager.SetPixelConst( PSC_Custom_1,	dofParamsVectorRange );
			stateManager.SetPixelConst( PSC_Custom_2,	dofParamsVectorIntensity );
			stateManager.SetPixelConst( PSC_Custom_3,	Vector ( clampMaxHelper1[0], clampMaxHelper1[1], clampMaxHelper2[0], clampMaxHelper2[1] ) );

			drawer.DrawQuad( mergeShader );

			// Unbind textures
			GpuApi::BindTextures( 0, 4, nullptr, GpuApi::PixelShader );
		}
	}

};
