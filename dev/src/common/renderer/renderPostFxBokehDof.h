//#include "renderEnvProbeManager.h"

class CPostFxBokehDof
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

	public:

	CPostFxBokehDof ()
	{
	}

	~CPostFxBokehDof ()
	{
	}

	void ApplyBokehDoffx( 
		CPostProcessDrawer &drawer,								

		// Surfaces
		class CRenderSurfaces* surfaces,

		// Current camera
		const CRenderCamera &camera,						

		// Bokeh params
		const SBokehDofParams& bokehDofParams, 
		const CEnvDepthOfFieldParametersAtPoint& dofParams,
		Float blendScale ,

		// Color source, target and area
		ERenderTargetName renderSource,							
		const TexelArea &renderArea,
		ERenderTargetName renderTarget
		)
	{
		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		ASSERT( RTN_None != renderSource );
		ASSERT( RTN_None != renderTarget );

		ERenderTargetName fullTemp		= RTN_PostProcessTempFull;
		ERenderTargetName halfTemp		= RTN_PostProcessTempHalf3; // We need not overlapping buffer
		ERenderTargetName quaterTemp1	= RTN_PostProcessTempQuater1;
		ERenderTargetName quaterTemp2	= RTN_PostProcessTempQuater2;
		ERenderTargetName quaterTemp3	= RTN_PostProcessTempQuater3;
		ERenderTargetName quaterTemp4	= RTN_PostProcessTempQuater4;
		ERenderTargetName quaterTemp5	= RTN_PostProcessTempQuater5;


		const Int32 renderFullWidth	= surfaces->GetRenderTarget( renderSource )->GetWidth();
		const Int32 renderFullHeight = surfaces->GetRenderTarget( renderSource )->GetHeight();

		const Int32 renderHalfWidth	= surfaces->GetRenderTarget( halfTemp )->GetWidth();
		const Int32 renderHalfHeight = surfaces->GetRenderTarget( halfTemp )->GetHeight();

		const Int32 renderQuaterWidth	= surfaces->GetRenderTarget( quaterTemp1 )->GetWidth();
		const Int32 renderQuaterHeight = surfaces->GetRenderTarget( quaterTemp1 )->GetHeight();

		TexelArea halfArea( renderArea.m_width/2 , renderArea.m_height/2 );
		TexelArea quaterArea( renderArea.m_width/4 , renderArea.m_height/4 );
		
	
		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);
		GpuApi::SetSamplerStatePreset( 3, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);
		GpuApi::SetSamplerStatePreset( 4, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip);

		const Float cameraFOV = DEG2RAD(camera.GetFOV());
		const Float focalLength = 1.0f / (2.0f * tan( cameraFOV / 2.0f ));
		
		///////////////////////////////////////////////////////////////////
		// Distance that cause far bokeh not to be visible at all. Bcoz sometimes
		// its needed to skip all background including sky & clouds. In that case
		// Using paramFarFocus >= 100 uses seperate shader to not calculate far bokeh.
		const Float farDiscardDistance = 100.0f;

		// Some scaling, so bokeh have visually similar bluring range as the regular blur
		// That was forced by cutscene-guys ;) May be moved as property some sunny day.
		const Float paramBokehSizeScale = 0.9f;
		///////////////////////////////////////////////////////////////////

		Float paramNearBlur		= dofParams.m_nearBlurDist.GetScalar();
		Float paramNearFocus	= dofParams.m_nearFocusDist.GetScalar();
		Float paramFarFocus		= dofParams.m_farFocusDist.GetScalar();
		Float paramFarBlur		= dofParams.m_farBlurDist.GetScalar();
		Float paramIntensity	= Clamp<Float>( dofParams.m_intensity.GetScalar(), 0.0f, 1.0f ) * blendScale;

		{
			paramNearFocus = Min( paramNearFocus, paramFarFocus );
			paramNearBlur  = Min( paramNearBlur, paramNearFocus );
			paramFarBlur   = Max( paramFarBlur, paramFarFocus );
		}

		Float maxCoCSize = 0.02f;
		Float planeInFocus = bokehDofParams.m_planeInFocus;
		Float appertureSize =  bokehDofParams.PupilSize();
		Float circleHexBlend = bokehDofParams.m_hexToCircleScale;

		const Vector dofParamsVectorRange		= Vector ( paramNearFocus, 1.f / Max( 0.001f, (paramNearFocus - paramNearBlur)), paramFarFocus, 1.f / Max( 0.001f, (paramFarBlur - paramFarFocus)) );
		const Vector dofParamsVectorIntensity	= Vector ( paramIntensity, paramBokehSizeScale, 0, 0 );

		// Calculate on CPU some helper constants
		{
			const Vector calculateTexCoordTransformHalf = CalculateTexCoordTransform( halfArea, renderArea, renderFullWidth , renderFullHeight );
			stateManager.SetVertexConst( VSC_Custom_0,	calculateTexCoordTransformHalf );

			const Vector calculateTexCoordTransformQuater = CalculateTexCoordTransform( quaterArea, halfArea, renderHalfWidth , renderHalfHeight );
			stateManager.SetVertexConst( VSC_Custom_1,	calculateTexCoordTransformQuater );

			const Vector calculateTexCoordClamp = CalculateTexCoordClamp( renderArea, renderFullWidth, renderFullHeight ) ;
			stateManager.SetPixelConst(	PSC_Custom_0,	calculateTexCoordClamp );

			const Vector calculateAreaTexelSize( 1.0f / ( Float )renderArea.m_width, 1.0f /( Float ) renderArea.m_height, ( Float ) renderArea.m_height / ( Float ) renderArea.m_width, circleHexBlend );
			stateManager.SetPixelConst(	PSC_Custom_1,	calculateAreaTexelSize );

			const Vector cameraApertureParams( planeInFocus, focalLength, appertureSize, maxCoCSize );
			stateManager.SetPixelConst(	PSC_Custom_2,	cameraApertureParams );	

			const Float bokehScreenScale = 0.0005f;
			const Float sampleScreenScale = 0.1f * renderArea.m_height;

			const Vector calculateHelperScales( 
				bokehScreenScale * renderArea.m_height   , bokehScreenScale * renderArea.m_width , 
				sampleScreenScale / renderArea.m_width , sampleScreenScale / renderArea.m_height );
			stateManager.SetPixelConst(	PSC_Custom_3,	calculateHelperScales );	

			stateManager.SetPixelConst( PSC_Custom_4,	dofParamsVectorRange );
			stateManager.SetPixelConst( PSC_Custom_5,	dofParamsVectorIntensity );
		}

		/////////////////////////////////////////////////////
		// Circle of confusion (Full-Res)
		{
			// In	:	FullRes Color	[0]
			//		:	FullRes Depth	[1]
			// Out	:	TempFull
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( fullTemp ) );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );	

			GpuApi::TextureRef colorRef = surfaces->GetRenderTargetTex( renderSource );
			GpuApi::TextureRef sceneDepth =  GetRenderer()->GetSurfaces()->GetDepthBufferTex();

			GpuApi::TextureRef refs[2] = { colorRef, sceneDepth };
			GpuApi::BindTextures( 0, 2, refs, GpuApi::PixelShader );

			if( bokehDofParams.m_usePhysicalSetup )
			{
				drawer.DrawQuad( GetRenderer()->m_postfxCircleOfConfusionPhysical );			
			}
			else
			{
				if( paramFarFocus-paramNearFocus < farDiscardDistance )
				{
					drawer.DrawQuad( GetRenderer()->m_postfxCircleOfConfusion );			
				}
				else
				{
					drawer.DrawQuad( GetRenderer()->m_postfxCircleOfConfusionNoFar );			
				}
			}
			
		}
		/////////////////////////////////////////////////////
		// downscale fullres color to half res
		{
			// In	:	FullRes Color	[0]
			//		:	FullRes Coc		[1]
			// Out	:	Temp Quater1 Color
			//		:	Temp Quater3 CoC
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( quaterTemp1 ) ); // LOW RES COLOR NEAR
			rtSetup.SetColorTarget( 1, surfaces->GetRenderTargetTex( quaterTemp2 ) ); // LOW RES COLOR FAR
			rtSetup.SetColorTarget( 2, surfaces->GetRenderTargetTex( quaterTemp3 ) ); // LOW RES COC
			rtSetup.SetViewport( quaterArea.m_width, quaterArea.m_height, quaterArea.m_offsetX, quaterArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );	

			GpuApi::TextureRef cocRef = surfaces->GetRenderTargetTex( fullTemp );
			GpuApi::BindTextures( 1, 1, &cocRef, GpuApi::PixelShader );

			drawer.DrawQuad( GetRenderer()->m_postfxBokehDownsample );
		}
		/////////////////////////////////////////////////////
		// Bokeh near/far plane at once
		{
			// In	:	Quater res Color	[1]
			//		:	Quater res Coc		[3]
			// Out	:	Temp Quater4 Color Near [2]
			//		:	Temp Quater4 Color Far	[4]
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( quaterTemp5 ) );
			rtSetup.SetColorTarget( 1, surfaces->GetRenderTargetTex( quaterTemp4 ) );
			rtSetup.SetViewport( quaterArea.m_width, quaterArea.m_height, quaterArea.m_offsetX, quaterArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );	

			GpuApi::TextureRef colorNear = surfaces->GetRenderTargetTex( quaterTemp1 );
			GpuApi::TextureRef colorFar  = surfaces->GetRenderTargetTex( quaterTemp2 );
			GpuApi::TextureRef cocRef	 = surfaces->GetRenderTargetTex( quaterTemp3 );

			GpuApi::TextureRef refs[3] = { colorNear, colorFar, cocRef };
			GpuApi::BindTextures( 0, 3, refs, GpuApi::PixelShader );

			drawer.DrawQuad( GetRenderer()->m_postfxBokehDof );		
		}
		/////////////////////////////////////////////////////
		// Blur CoC near plane 1st pass
		{
			// In	:	Quater Coc			[3]
			// Out	:	Quater CoC Blured	[1]
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( quaterTemp1 ) );
			rtSetup.SetViewport( quaterArea.m_width, quaterArea.m_height, quaterArea.m_offsetX, quaterArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );	

			GpuApi::TextureRef cocRef	= surfaces->GetRenderTargetTex( quaterTemp3 );
			GpuApi::BindTextures( 0, 1, &cocRef, GpuApi::PixelShader );

			stateManager.SetPixelConst( PSC_Custom_6, Vector( 1.0f , 0, 0, 0 ) );

			drawer.DrawQuad( GetRenderer()->m_postfxBokehDofCoCBlur );
		}
		/////////////////////////////////////////////////////
		// Blur CoC near plane 2nd pass
		{
			// In	:	Quater Coc			[1]
			// Out	:	Quater CoC Blured	[3]
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( quaterTemp3 ) );
			rtSetup.SetViewport( quaterArea.m_width, quaterArea.m_height, quaterArea.m_offsetX, quaterArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );	

			GpuApi::TextureRef cocRef	= surfaces->GetRenderTargetTex( quaterTemp1 );
			GpuApi::BindTextures( 0, 1, &cocRef, GpuApi::PixelShader );

			stateManager.SetPixelConst( PSC_Custom_6, Vector( 0, 1.0f, 0, 0 ) );

			drawer.DrawQuad( GetRenderer()->m_postfxBokehDofCoCBlur );
		}
		/////////////////////////////////////////////////////
		// Resolve DOF - mix bokeh with original color (Full-Res)
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );	

			GpuApi::TextureRef colorRef			= surfaces->GetRenderTargetTex( renderSource );
			GpuApi::TextureRef cocRef			= surfaces->GetRenderTargetTex( fullTemp );
			GpuApi::TextureRef farRef			= surfaces->GetRenderTargetTex( quaterTemp4 );
			GpuApi::TextureRef nearRef			= surfaces->GetRenderTargetTex( quaterTemp5 );
			GpuApi::TextureRef cocBlurRef		= surfaces->GetRenderTargetTex( quaterTemp3 );

			GpuApi::TextureRef refs[] = { colorRef, cocRef, nearRef , farRef , cocBlurRef };
			GpuApi::BindTextures( 0, 5, refs, GpuApi::PixelShader );

			drawer.DrawQuad( GetRenderer()->m_postfxBokehDof2Pass );			
		}

		// Restore old state
		{
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 1, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 2, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 3, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 4, 1, nullptr, GpuApi::PixelShader );
			targetStateGrabber.Restore();
		}
		
	}
};
