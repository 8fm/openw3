
class CPostFxToneMapping
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:
	CPostFxToneMapping ()
	{
	}

	~CPostFxToneMapping ()
	{
	}

public:

	void SetTonemapParamsVector( const CEnvToneMappingParametersAtPoint &toneMappingParams, int offset )
	{
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

		stateManager.SetPixelConst(  PSC_Custom_0 + offset, Vector ( 0.0f, 
			toneMappingParams.m_luminanceLimitMin.GetScalar(), 
			toneMappingParams.m_luminanceLimitMax.GetScalar(), 
			0 ) );
	}

	void SetTonemapCurveParams( EEnvManagerModifier modifier, const CEnvToneMappingCurveParametersAtPoint &curveParams, Uint32 offset0, Uint32 offset1 )
	{
		if ( EMM_LinearAll == modifier )
		{
			SetTonemapCurveParams( EMM_None, CEnvToneMappingCurveParametersAtPoint ().SetLinearApproximate(), offset0, offset1 );
			return;
		}

		Vector params0 ( curveParams.m_shoulderStrength.GetScalar(),	curveParams.m_linearStrength.GetScalar(),	curveParams.m_linearAngle.GetScalar(), 0.f );
		Vector params1 ( curveParams.m_toeStrength.GetScalar(),			curveParams.m_toeNumerator.GetScalar(),		curveParams.m_toeDenominator.GetScalar(), 0.f );

		// hack to fix broken values (unpredicted results inside shader)
		if ( Vector ( 1, 1, 1, 0 ) == params0 && Vector ( 1, 1, 1, 0 ) == params1 )
		{
			params0.Set4( 0.22f, 0.3f,  0.1f, 0.f );
			params1.Set4( 0.20f, 0.01f, 0.3f, 0.f );
		}

		ASSERT( offset0 != offset1 );
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		stateManager.SetPixelConst(  PSC_Custom_0 + offset0,	params0 );
		stateManager.SetPixelConst(  PSC_Custom_0 + offset1,	params1 );
	}

	void ApplySimple( CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, Bool displayDebugCurve, ERenderTargetName sampleTarget, ERenderTargetName averageTarget, const CRenderFrameInfo& info )
	{
		// Choose shader
		CRenderShaderPair *shader = NULL;
		const Bool blending = info.m_envBlendingFactor > 0.f;
		const Bool fixedLuminance = info.m_tonemapFixedLumiance > 0.0f;
		const Float blendFactor = blending ? info.m_envBlendingFactor : 0.0f;

		if( displayDebugCurve )
		{
			shader = blending ? GetRenderer()->m_postfxToneMappingApplyBlendDebug : 
								GetRenderer()->m_postfxToneMappingApplyDebug ;
		}
		else if( fixedLuminance )
		{
			shader = blending ? GetRenderer()->m_postfxToneMappingApplyFixedLuminanceBlend :
								GetRenderer()->m_postfxToneMappingApplyFixedLuminance ;
		}
		else
		{
			shader = blending ? GetRenderer()->m_postfxToneMappingApplyBlend :
								GetRenderer()->m_postfxToneMappingApply ;
		}

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		ASSERT( RTN_None != sampleTarget );

		GpuApi::TextureRef sampleTextures[] = { surfaces->GetRenderTargetTex( sampleTarget ), surfaces->GetRenderTargetTex( averageTarget ) };
		GpuApi::BindTextures( 0, ARRAY_COUNT(sampleTextures), sampleTextures, GpuApi::PixelShader );
		
		// Render
		const EEnvManagerModifier displayModifier = static_cast<EEnvManagerModifier>( info.m_envParametersGame.m_displaySettings.m_displayMode );

		if ( displayDebugCurve )
		{
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 15,	Vector( (Float)info.m_width, (Float)info.m_height, (Float)surfaces->GetWidth(), (Float)surfaces->GetHeight() ) );
		}
		
		if ( blending ) // When blending, pass the two curves defined in CRenderFrameInfo
		{
			const CAreaEnvironmentParamsAtPoint &areaParams = info.m_envParametersAreaBlend1;
			const CAreaEnvironmentParamsAtPoint &areaParamsB = info.m_envParametersAreaBlend2;
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 6,	Vector( 0, areaParams.m_toneMapping.m_newToneMapPostScale.GetScalar(), areaParams.m_toneMapping.m_newToneMapWhitepoint.GetScalar(), 0 ) );
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 10,	Vector( 0, areaParamsB.m_toneMapping.m_newToneMapPostScale.GetScalar(), areaParamsB.m_toneMapping.m_newToneMapWhitepoint.GetScalar(), 0 ) );
			
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 16,	Vector( areaParams.m_toneMapping.m_exposureScale.GetScalar(), areaParams.m_toneMapping.m_postScale.GetScalar(), areaParams.m_toneMapping.m_luminanceLimitShape.GetScalar(), 0 ) );
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 17,	Vector( areaParamsB.m_toneMapping.m_exposureScale.GetScalar(), areaParamsB.m_toneMapping.m_postScale.GetScalar(), areaParamsB.m_toneMapping.m_luminanceLimitShape.GetScalar(), 0 ) );

			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 13,	Vector( blendFactor, 0.0f, 0.0f, 0.0f ) );

			SetTonemapCurveParams( displayModifier, areaParams.m_toneMapping.m_newToneMapCurveParameters, 7, 8 );
			SetTonemapCurveParams( displayModifier, areaParamsB.m_toneMapping.m_newToneMapCurveParameters, 11, 12 );
			SetTonemapParamsVector( areaParams.m_toneMapping, 4 );
			SetTonemapParamsVector( areaParamsB.m_toneMapping, 9 );
		}
		else // otherwise pass the regular curve
		{
			const CAreaEnvironmentParamsAtPoint &areaParams = info.m_envParametersArea;
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 6,	Vector( 0, areaParams.m_toneMapping.m_newToneMapPostScale.GetScalar(), areaParams.m_toneMapping.m_newToneMapWhitepoint.GetScalar(), 0.f ) );
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 16,	Vector( areaParams.m_toneMapping.m_exposureScale.GetScalar(), areaParams.m_toneMapping.m_postScale.GetScalar(), areaParams.m_toneMapping.m_luminanceLimitShape.GetScalar(), 0 ) );

			SetTonemapCurveParams( displayModifier, areaParams.m_toneMapping.m_newToneMapCurveParameters, 7, 8 );
			SetTonemapParamsVector( areaParams.m_toneMapping, 4 );
		}

		if( fixedLuminance )
		{
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_18,	Vector( info.m_tonemapFixedLumiance, 0.0f, 0.0f, 0.0f ) );
		}

		// Render
		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::VertexShader );
	}

	void AdaptLuminancesSimple( CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, ERenderTargetName lumFinal, ERenderTargetName lumCurrent, Float adaptationAlphaUp, Float adaptationAlphaDown )
	{
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();

		// Build render area
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set shader textures
		GpuApi::TextureRef targets[2] = { surfaces->GetRenderTargetTex( lumCurrent ), surfaces->GetRenderTargetTex( lumFinal ) };
		GpuApi::BindTextures( 0, 2, &(targets[0]), GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 1, 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

		// Setup params
		stateManager.SetPixelConst(  PSC_Custom_0, Vector( adaptationAlphaUp, adaptationAlphaDown, 0, 0 ) );

		// Render
		drawer.DrawQuad( GetRenderer()->m_postfxToneMappingAdaptSimple );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::VertexShader );
	}
};
