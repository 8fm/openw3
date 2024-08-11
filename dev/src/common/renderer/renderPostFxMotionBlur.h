


class CPostFxMotionBlur
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:
	CPostFxMotionBlur()
	{
	}

	~CPostFxMotionBlur()
	{
	}

private:
	// assuming one motion blur post process per frame
	CRenderCamera				m_previousFrameCamera;

	template < Bool ret >
	RED_INLINE Bool Finalize( CRenderCamera currentFrameCamera )
	{
		m_previousFrameCamera = currentFrameCamera;
		return ret;
	}

public:
	//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	Bool Apply( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName &refSceneTarget, ERenderTargetName &refHelperTarget, ERenderTargetName helperHalf1, ERenderTargetName helperHalf2, ERenderTargetName helperHalf3,
		const CRenderFrameInfo& info )
	{
		const Bool enableTwoPasses = true;

		const SWorldMotionBlurSettings &mbSett = info.m_worldRenderSettings.m_motionBlurSettings;
		
		// Build prev frame viewProj matrix
		Matrix reprojectionMatrix;
		Float strengthMultplier = 0.0f;

		{
			// This is needed to rescale discard treshold scales
			const Float targetFPS = Max( GetRenderer()->GetLastTickDelta() / ( 1.0f/30.0f ) , 0.5f );

			// Too big change rejection
			{
				SCameraChangeTreshold treshold;
				treshold.m_greater			= true;
				treshold.m_anyMatch			= true;					
				treshold.m_positionTreshold = 30.0f * targetFPS;
				treshold.m_rotationTreshold = 30.0f * targetFPS;
				treshold.m_fovTreshold		= 10.0f * targetFPS;

				if( treshold.DoesCameraChanged( info.m_camera , m_previousFrameCamera ) )
				{
					// RED_LOG( RED_LOG_CHANNEL(MotionBlur) , TXT("Camera changed too much") );
					return Finalize<false>( info.m_camera );
				}
			}
			
			// Any change of FOV triggers motion blur
			if( ::Abs( m_previousFrameCamera.GetFOV() - info.m_camera.GetFOV() ) > 0.01f )
			{
				strengthMultplier = 1.0;
			}
			// Too small change rejection
			else
			{				

				SCameraChangeTreshold treshold;
				treshold.m_greater			= false;					// Check for smaller values instead grater ones
				treshold.m_anyMatch			= false;					// All 
				treshold.m_positionTreshold = Config::cvMotionBlurPositionTreshold.Get();
				treshold.m_rotationTreshold = Config::cvMotionBlurRotationTreshold.Get();
				treshold.m_checkMask = SCameraChangeTreshold::ECM_Position | SCameraChangeTreshold::ECM_Rotation;

				if( treshold.DoesCameraChanged( info.m_camera , m_previousFrameCamera ) )				
				{
					return Finalize<false>( info.m_camera );
				}			

				strengthMultplier = ::Min( treshold.m_outputSimilarity , 1.0f );

			}

			Matrix prevFrameViewProj = m_previousFrameCamera.GetWorldToView() * m_previousFrameCamera.GetViewToScreen();

			MatrixDouble m0, m1;
			m0.Import( info.m_camera.GetWorldToScreen() );
			m1.Import( prevFrameViewProj );
			(m0.FullInverted() * m1).Export( reprojectionMatrix );

			// Still camera rejection
			// This is experimental code for screen space verification
			/*
			const Float projectedDepthNear = ProjectDepth( info.m_camera, 1.0f );
			const Float projectedDepthFar = ProjectDepth( info.m_camera, 16.0f );
			const Float thresholdSquared = Config::cvMotionBlurRejectionTreshold.Get() * Config::cvMotionBlurRejectionTreshold.Get();
			if( thresholdSquared > 0.0f )
			{
				// Create screen space position in the center of the screen, moved a bit from near plane
				const Vector testPoints[] = 
				{
					Vector(  0.0f, 0.5f, projectedDepthFar, 1.0f ), 
					Vector( -0.5f, 0.0f, projectedDepthNear, 1.0f ),
					Vector(  0.5f, 0.0f, projectedDepthFar, 1.0f )
				};

				for( Uint32 idx = 0; idx < ARRAY_COUNT(testPoints); ++idx )
				{
					const Vector vsp = reprojectionMatrix.TransformVectorWithW( testPoints[idx] );		// Reproject world position to new view position
					const Vector ssp = vsp / vsp.W;														// Get repojected screen space position

					// Squared distance from the screen center in pixels (gets max from viewport size).
					// Note - temporal aliasing jitter, produce 0.000323 delta each frame, so this is unnoticeable
					const Float dst = ssp.DistanceSquaredTo2D( testPoints[idx] ) * Max( info.m_width , info.m_height ) * 0.5f;
					const Float difference = dst - thresholdSquared;

					if( difference > 0.0f )
					{
						strengthMultplier = ::Min( difference / thresholdSquared , 1.0f );
						break;
					}

				}
			}
			// No treshold defined. Run motion blur all the time
			else
			{
				strengthMultplier = 1.0f;
			}
			*/

		}


		//
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

		// Set common motionblur params
		{
			const Float strengthScale = strengthMultplier * info.m_envParametersArea.m_motionBlurParameters.m_strength.GetScalarClampMin( 0 );
			const Float strengthNear = Max( 0.f, mbSett.m_strengthNear ) * strengthScale;
			const Float strengthFar  = Max( 0.f, mbSett.m_strengthFar ) * strengthScale;
			if ( 0 == strengthNear && 0 == strengthFar )
			{
				return Finalize<false>( info.m_camera );
			}

			const Float range = Max( 0.001f, mbSett.m_distanceRange );
			const Float standoutRange = Max( 0.001f, mbSett.m_standoutDistanceRange );
			const Float standoutScaleNear = Clamp( 1.f - mbSett.m_standoutAmountNear, 0.f, 1.f );
			const Float standoutScaleFar = Clamp( 1.f - mbSett.m_standoutAmountFar, 0.f, 1.f );

			const Vector motionBlurParams0 ( strengthFar - strengthNear, strengthNear, 1.f / range, -mbSett.m_distanceNear / range );
			const Vector motionBlurParams1 ( standoutScaleFar - standoutScaleNear, standoutScaleNear, 1.f / standoutRange, -mbSett.m_standoutDistanceNear / standoutRange );
			const Vector motionBlurParams2 ( 1.f / Max( 0.001f, mbSett.m_fullBlendOverPixels ), 1.f + Max( 0.f, mbSett.m_sharpenAmount ), 0, 0 );

			stateManager.SetPixelConst( PSC_Custom_0, motionBlurParams0 );
			stateManager.SetPixelConst( PSC_Custom_1, motionBlurParams1 );
			stateManager.SetPixelConst( PSC_Custom_2, motionBlurParams2 );
			stateManager.SetPixelConst( PSC_Custom_Matrix, reprojectionMatrix );
		}
		
		// Downsample
		{
			PC_SCOPE_RENDER_LVL1( MotionBlurDownsample );

			// Bind target
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperHalf1 ) );
			rtSetup.SetColorTarget( 1, surfaces->GetRenderTargetTex( helperHalf2 ) );
			rtSetup.SetViewport( info.m_width / 2, info.m_height / 2, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );
			
			GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( refSceneTarget ), surfaces->GetDepthBufferTex() };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
			GpuApi::BindTextureStencil( ARRAY_COUNT(tex), surfaces->GetDepthBufferTex(), GpuApi::PixelShader );

			GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader ); 
			GpuApi::SetSamplerStateCommon( 1, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader ); 

			//
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			//
			drawer.DrawQuad( GetRenderer()->m_postfxMotionBlurDownsample );

			//
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
			GpuApi::BindTextureStencil( ARRAY_COUNT(tex) + 1, GpuApi::TextureRef::Null(), GpuApi::PixelShader );
		}

		// Sharpen
		if ( mbSett.m_sharpenAmount > 0 )
		{
			PC_SCOPE_RENDER_LVL1( MotionBlurSharpen );

			// Bind target
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperHalf3 ) );
			rtSetup.SetViewport( info.m_width / 2, info.m_height / 2, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			////////

			GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( helperHalf1 ), surfaces->GetRenderTargetTex( helperHalf2 ) };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

			GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader ); 
			GpuApi::SetSamplerStateCommon( 1, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader ); 

			//
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			//
			drawer.DrawQuad( GetRenderer()->m_postfxMotionBlurSharpen );

			//
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );		

			Swap( helperHalf1, helperHalf3 );
		}

		// Calculate motion blur
		{
			PC_SCOPE_RENDER_LVL1( MotionBlurCalculate );

			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			//
			for ( Uint32 pass_i=0; pass_i<2; ++pass_i )
			{
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( helperHalf3 ) );
				rtSetup.SetViewport( info.m_width / 2, info.m_height / 2, 0, 0 );
				GpuApi::SetupRenderTargets( rtSetup );

				GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( helperHalf1 ), surfaces->GetRenderTargetTex( helperHalf2 ), surfaces->GetDepthBufferTex() };
				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
				GpuApi::BindTextureStencil( ARRAY_COUNT(tex), surfaces->GetDepthBufferTex(), GpuApi::PixelShader );

				GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader ); 
				GpuApi::SetSamplerStateCommon( 1, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader ); 

				stateManager.SetPixelConst( PSC_Custom_4, Vector ( (Float)pass_i, 0, 0, 0 ) );
				drawer.DrawQuad( GetRenderer()->m_postfxMotionBlurCalc );

				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
				GpuApi::BindTextureStencil( ARRAY_COUNT(tex) + 1, GpuApi::TextureRef::Null(), GpuApi::PixelShader );			

				Swap( helperHalf1, helperHalf3 );
			}
		}

		// Apply motion blur
		{
			PC_SCOPE_RENDER_LVL1( MotionBlurApply );

			// Bind target
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( refHelperTarget ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			////////

			GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( refSceneTarget ), surfaces->GetRenderTargetTex( helperHalf1 ), surfaces->GetDepthBufferTex(), surfaces->GetRenderTargetTex( helperHalf2 ) };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
			GpuApi::BindTextureStencil( ARRAY_COUNT(tex), surfaces->GetDepthBufferTex(), GpuApi::PixelShader );

			GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader ); 
			GpuApi::SetSamplerStateCommon( 1, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader ); 

			//
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet ); //Blend );

			//
			drawer.DrawQuad( GetRenderer()->m_postfxMotionBlurApply );

			//
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
			GpuApi::BindTextureStencil( ARRAY_COUNT(tex) + 1, GpuApi::TextureRef::Null(), GpuApi::PixelShader );
		}

		// Swap textures
		Swap( refHelperTarget, refSceneTarget );
				
		return Finalize<true>( info.m_camera );
	}
	
};
