#include "../engine/renderSettings.h"

class CPostFxPresent
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:
	// Is antialiasing enabled?
	Bool IsPostFxAntialiasingEnabled( EPostProcessCategoryFlags flags, const CRenderFrameInfo& info )
	{
		return ( (flags & PPCF_AntiAlias) != 0 ) && info.m_envParametersGame.m_displaySettings.m_allowAntialiasing && !GIsRendererTakingUberScreenshot && !(Config::cvUberSampling.Get() > 1 ) && !GRender->IsMSAAEnabled( info );
	}

	struct SVignetteParams
	{
		Bool							enableVignette;

		CRenderTexture*					vignetteTexture;
		Vector							vignetteWeights;
		Vector							vignetteColor;
		Float							vignetteOpacity;

		RED_FORCE_INLINE SVignetteParams()
			: enableVignette( false )
			, vignetteTexture( NULL )
			, vignetteWeights( Vector::ZEROS )
			, vignetteColor( Vector::ZEROS )
			, vignetteOpacity( 1.f )
		{}

	};
	 
	enum EVignetteUsage : Uint8
	{
		EVU_None = 0,
		EVU_Texture , 
		EVU_Procedural ,
		EVU_Max
	};

private:
	EVignetteUsage SetupVignette( const TexelArea &renderArea, const SVignetteParams& vignetteParams, EPostProcessCategoryFlags flags )
	{
		const Vector weights = Vector::Max4( Vector::ZEROS, vignetteParams.vignetteWeights );
		if ( 
			( !vignetteParams.enableVignette ) || 
			( weights.SquareMag4() <= FLT_EPSILON ) || 
			( !( PPCF_Vignette & flags ) ) ||
			vignetteParams.vignetteOpacity <= 0 )
		{
			return EVU_None;
		}

		Float width = 1.0f, height = 1.0f;

		// Setup texture
		if ( vignetteParams.vignetteTexture )
		{
			vignetteParams.vignetteTexture->Bind( 2 );

			vignetteParams.vignetteTexture->GetSize( width, height );

			GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearMip );
		}

		// Setup parameters
		TexelArea sampleArea ( (Int32)width, (Int32)height ); //Ugly but no time to rewrite with floats
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_2,	CalculateTexCoordTransform( renderArea, sampleArea ) );		
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 6,	Vector ( weights.X, weights.Y, weights.Z, Clamp( vignetteParams.vignetteOpacity, 0.f, 1.f ) ) );
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 7,	vignetteParams.vignetteColor );

		// Finalize
		return vignetteParams.vignetteTexture ? EVU_Texture : EVU_Procedural;
	}

public:
	CPostFxPresent ()
	{
	}

	~CPostFxPresent ()
	{
	}

	/// Renders image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	void ApplyFinalize(
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName sampleTarget, const TexelArea &sampleArea, const CRenderFrameInfo& info, const SVignetteParams& vignetteParams, EPostProcessCategoryFlags flags )
	{
		// Build render area
		const TexelArea renderArea ( GpuApi::GetViewport().width, GpuApi::GetViewport().height, 0, 0 );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		ASSERT( RTN_None != sampleTarget );
		GpuApi::TextureRef target = surfaces->GetRenderTargetTex( sampleTarget );
		GpuApi::BindTextures( 0, 1, &target, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Render
		const Int32 sampleFullWidth  = surfaces->GetRenderTarget( sampleTarget )->GetWidth();
		const Int32 sampleFullHeight = surfaces->GetRenderTarget( sampleTarget )->GetHeight();

		const Float gamma = info.m_envParametersDayPoint.m_gamma;
		const Float invGamma = 1.f / gamma;

		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0,	CalculateTexCoordTransform( renderArea, sampleArea, sampleFullWidth, sampleFullHeight ) );
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_8,	Vector ( 1.f / Max( 0.001f, gamma ), gamma, 0.f, 0.f ) );

		// setup color tint parameter
		{
			Float scale = info.m_envParametersGame.m_brightnessTint.m_brightness;
			Vector tint = info.m_envParametersGame.m_brightnessTint.m_tint;
			Vector colorTint ( powf( scale * powf( tint.X, gamma ), invGamma ), powf( scale * powf( tint.Y, gamma ), invGamma ), powf( scale * powf( tint.Z, gamma ), invGamma ), 1.f );
			GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_9,	colorTint );
		}

		Vector standardTransform ( 0.f, 1.f, 0.f, 0.f );
		if ( info.IsShowFlagOn( SHOW_VideoOutputLimited ) )
		{
			standardTransform.X = 0.05;//16.f / 255.f;
			standardTransform.Y = 1.f; //235.f / 255.f;
		}
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_15, standardTransform );

		// setup parametric balance parameters
		Bool isBalanceEnabled = false;
		if ( info.m_envParametersGame.m_displaySettings.m_allowColorMod )
		{
			const CEnvFinalColorBalanceParametersAtPoint &params = info.m_envParametersArea.m_finalColorBalance;
			
			const CEnvParametricBalanceParametersAtPoint &paramsLow  = params.m_parametricBalanceLow;
			const CEnvParametricBalanceParametersAtPoint &paramsMid  = params.m_parametricBalanceMid;
			const CEnvParametricBalanceParametersAtPoint &paramsHigh = params.m_parametricBalanceHigh;

			Vector valueLow  = paramsLow.m_color.GetColorScaledGammaToLinear( true );
			Vector valueMid  = paramsMid.m_color.GetColorScaledGammaToLinear( true );
			Vector valueHigh = paramsHigh.m_color.GetColorScaledGammaToLinear( true );
			valueLow.W  = paramsLow.m_saturation.GetScalarClampMin(0);
			valueMid.W  = paramsMid.m_saturation.GetScalarClampMin(0);
			valueHigh.W = paramsHigh.m_saturation.GetScalarClampMin(0);

			const Float levelsShadows = params.m_levelsShadows.GetScalarClampMin( 0.f );
			const Float levelsMidtones = params.m_levelsMidtones.GetScalarClampMin( 0.f );
			const Float levelsHighlights = params.m_levelsHighlights.GetScalarClampMin( 0.f );

			if ( Vector::ONES != valueLow || Vector::ONES != valueMid || Vector::ONES != valueHigh || 0 != levelsShadows || 1 != levelsMidtones || 1 != levelsHighlights )
			{
				isBalanceEnabled = true;
				
				const Float rangeMin = params.m_midtoneRangeMin.GetScalar();
				const Float rangeMax = params.m_midtoneRangeMax.GetScalar();
				const Float rangeClamp = 0.5f * (rangeMin + rangeMax);

				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_10, Vector ( Min( rangeClamp, rangeMin ), -1.f / Max( 0.0001f, params.m_midtoneMarginMin.GetScalar() ), Max( rangeClamp, rangeMax ), 1.f / Max( 0.0001f, params.m_midtoneMarginMax.GetScalar() ) ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_11, valueLow );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_12, valueMid );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_13, valueHigh );

				const Float levelsScale = 1.f / Max( 0.0001f, levelsHighlights - levelsShadows );
				const Vector valueLevels ( levelsScale, -levelsShadows * levelsScale, 1.f / Max( 0.0001f, levelsMidtones ), 0.f );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_14, valueLevels );
			}
		}

		// Setup chromatic aberration
		Bool isAberrationEnabled = false;
		if ( (PPCF_ChromaticAberration & flags) && info.m_envParametersArea.m_finalColorBalance.m_chromaticAberrationSize.GetScalar() > 0 )
		{
			isAberrationEnabled = true;

			Float aberrSize = info.m_envParametersArea.m_finalColorBalance.m_chromaticAberrationSize.GetScalar();
			Float aberrStart = info.m_worldRenderSettings.m_chromaticAberrationStart;
			Float aberrInvRange = 1.f / Max( 0.001f, info.m_worldRenderSettings.m_chromaticAberrationRange );

			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_16, Vector( aberrSize, aberrStart, aberrInvRange, 0.f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_17, Vector( 0.5f * info.m_width / (Float)surfaces->GetWidth(), 0.5f * info.m_height / (Float)surfaces->GetHeight(), 1.f / (Float)surfaces->GetWidth(), 1.f / (Float)surfaces->GetHeight() ) );
		}

		static_assert( EVU_Max == 3 , "Vignette types changed. Take care of following code." );

		CRenderShaderPair* shaders[] =
		{
			GetRenderer()->m_postfxFinalize,							
			GetRenderer()->m_postfxFinalizeVignetteTex,					
			GetRenderer()->m_postfxFinalizeVignetteProc,			

			GetRenderer()->m_postfxFinalizeBalance,						
			GetRenderer()->m_postfxFinalizeBalanceVignetteTex,			
			GetRenderer()->m_postfxFinalizeBalanceVignetteProc,			

			GetRenderer()->m_postfxFinalizeAberration,					
			GetRenderer()->m_postfxFinalizeAberrationVignetteTex,		
			GetRenderer()->m_postfxFinalizeAberrationVignetteProc,		

			GetRenderer()->m_postfxFinalizeAberrationBalance,			
			GetRenderer()->m_postfxFinalizeAberrationBalanceVignetteTex,
			GetRenderer()->m_postfxFinalizeAberrationBalanceVignetteProc
		};

		const EVignetteUsage vignetteUage = SetupVignette( renderArea, vignetteParams, flags );

		const Uint32 shaderID = ( ( isAberrationEnabled ? 2 : 0 ) + ( isBalanceEnabled ? 1 : 0 ) ) * EVU_Max + static_cast<Uint32>( vignetteUage );

		RED_FATAL_ASSERT( shaderID < ARRAY_COUNT(shaders) , "Finalization post process shader ID is out of  array bounds." );

		drawer.DrawQuad( shaders[ shaderID ] );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	/// Renders image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	void ApplyAA( CPostProcessDrawer &drawer, const GpuApi::TextureRef &texture, const TexelArea &sampleArea )
	{
		ASSERT( texture );

		// Build render area
		const TexelArea renderArea ( GpuApi::GetViewport().width, GpuApi::GetViewport().height, 0, 0 );

		CRenderShaderPair *shader = GetRenderer()->m_postFXAA;

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		GpuApi::BindTextures( 0, 1, &texture, GpuApi::PixelShader );
		GpuApi::SetSamplerStateCommon( 0, 3, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Render
		const Int32 sampleFullWidth  = GpuApi::GetTextureLevelDesc( texture, 0 ).width;
		const Int32 sampleFullHeight = GpuApi::GetTextureLevelDesc( texture, 0 ).height;

		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, sampleArea, sampleFullWidth, sampleFullHeight ) );
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0 + 1,	Vector( 1.0f / ((Float)sampleFullWidth), 1.0f / ((Float)sampleFullHeight), 0.0f, 0.0f ) );
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 12,	Vector( 1.0f / ((Float)sampleFullWidth), 1.0f / ((Float)sampleFullHeight), 0.0f, 0.0f ) );
		const float N = 0.5f;
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 13,	Vector( -N / ((Float)sampleFullWidth), -N / ((Float)sampleFullHeight), N / ((Float)sampleFullWidth), N / ((Float)sampleFullHeight) ) );
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 14,	Vector( -2.0f / ((Float)sampleFullWidth), -2.0f / ((Float)sampleFullHeight), 2.0f / ((Float)sampleFullWidth), 2.0f / ((Float)sampleFullHeight) ) );
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 15,	Vector( 8.0f / ((Float)sampleFullWidth), 8.0f / ((Float)sampleFullHeight), -4.0f / ((Float)sampleFullWidth), -4.0f / ((Float)sampleFullHeight) ) );

		drawer.DrawQuad( shader );

		// Unbind texture
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	}

};
