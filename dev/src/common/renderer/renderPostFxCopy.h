#include "renderTexture.h"
#include "renderRenderSurfaces.h"
#include "renderPostProcess.h"

using namespace PostProcessUtilities;

class CPostFxCopy
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:
	CPostFxCopy ()
	{
	}

	~CPostFxCopy ()
	{
	}

	/// Renders source image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	void ApplyWithScale( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName sampleTarget, const TexelArea &sampleArea, Float scale, GpuApi::eDrawContext context )
	{
		ApplyWithScale( drawer, surfaces->GetRenderTargetTex( sampleTarget ), sampleArea, scale, context );
	}

	/// Renders source image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	void ApplyWithScale( 
		CPostProcessDrawer &drawer, 
		GpuApi::TextureRef sampleTexture, const TexelArea &sampleArea, Float scale, GpuApi::eDrawContext context )
	{
		// Build renderarea
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );

		CRenderShaderPair* shader = GetRenderer()->m_postFXCopyScale;

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( context );

		// Set textures
		ASSERT( !sampleTexture.isNull() );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );


		// Render
		const Int32 sampleFullWidth  = GpuApi::GetTextureLevelDesc( sampleTexture, 0 ).width;
		const Int32 sampleFullHeight = GpuApi::GetTextureLevelDesc( sampleTexture, 0 ).height;
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, renderArea.m_width, renderArea.m_height, sampleArea, sampleFullWidth, sampleFullHeight ) );		
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0 + 1,	Vector::ONES * scale );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	/// Renders source image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	void Apply( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName sampleTarget, const TexelArea &sampleArea,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		ASSERT( RTN_None != sampleTarget );
		ASSERT( surfaces->GetRenderTarget(sampleTarget)->GetWidth()  == GpuApi::GetTextureDesc( surfaces->GetRenderTargetTex(sampleTarget) ).width );
		ASSERT( surfaces->GetRenderTarget(sampleTarget)->GetHeight() == GpuApi::GetTextureDesc( surfaces->GetRenderTargetTex(sampleTarget) ).height );

		CRenderShaderPair* shader = GetRenderer()->m_postFXCopy;
		ApplyShader( drawer, shader, surfaces->GetRenderTargetTex(sampleTarget), sampleArea, drawContext );
	}

	/// Renders source image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	void ApplyShader( 
		CPostProcessDrawer &drawer, CRenderShaderPair *shader,
		GpuApi::TextureRef sampleTexture, const TexelArea &sampleArea,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{	

		// Build renderarea
		const TexelArea renderArea ( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		// Render
		const GpuApi::TextureLevelDesc sampleTextureDesc = GpuApi::GetTextureLevelDesc( sampleTexture, 0 );
		const Int32 sampleFullWidth  = sampleTextureDesc.width;
		const Int32 sampleFullHeight = sampleTextureDesc.height;
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, sampleArea, sampleFullWidth, sampleFullHeight ) );		
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );

		drawer.DrawQuad( shader );

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	/// Renders source image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	void ApplyDepthTarget( 
		CPostProcessDrawer &drawer, 
		GpuApi::TextureRef sampleTexture, const TexelArea &sampleArea )
	{	
		ASSERT( 0 == GpuApi::GetRenderTargetSetup().numColorTargets && !GpuApi::GetRenderTargetSetup().depthTarget.isNull() );
		CRenderShaderPair* shader = GetRenderer()->m_postFXCopyDepth;
		ApplyShader( drawer, shader, sampleTexture, sampleArea, GpuApi::DRAWCONTEXT_PostProcSet_DepthWrite );
	}

	//
	void ApplyDeprojectDepth( CPostProcessDrawer &drawer, GpuApi::TextureRef sampleTexture )
	{	
		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		ASSERT ( sampleTexture );
		GpuApi::BindTextures( 0, 1, &sampleTexture, GpuApi::PixelShader );		
		
		// Render
		const bool isMultisampled = GpuApi::GetTextureDesc( sampleTexture ).msaaLevel > 1;
		CRenderShaderPair* shader = isMultisampled ? GetRenderer()->m_postFXDeprojectDepthMSAA : GetRenderer()->m_postFXDeprojectDepth;
		drawer.DrawQuad( shader );

		//
		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	}

	/// Renders source image from [sampleTarget,sampleArea] on currently set rendertarget at full currently set viewport.
	Bool ApplyWithBalanceMap(
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, 
		ERenderTargetName renderTarget, const TexelArea &targetArea,
		ERenderTargetName sampleTarget, const TexelArea &sampleArea,
		Float lerpFactor, Float amountFactor, Float postBrightness,
		CRenderTexture *balanceMap0, CRenderTexture *balanceMap1,
		Float blendFactor,
		Float lerpFactorB, Float amountFactorB, Float postBrightnessB,
		CRenderTexture *balanceMap0B, CRenderTexture *balanceMap1B,
		GpuApi::eDrawContext drawContext = GpuApi::DRAWCONTEXT_PostProcSet )
	{
		struct Local
		{
			static void ResetPair( Float &lerpFactor, Float &amountFactor, Float &postBrightness, CRenderTexture *&balanceMap0, CRenderTexture *&balanceMap1 )
			{
				lerpFactor = 0;
				amountFactor = 0;
				postBrightness = 1;
				balanceMap0 = nullptr;
				balanceMap1 = nullptr;
			}

			static void OptimizePair( Float &lerpFactor, Float &amountFactor, Float &postBrightness, CRenderTexture *&balanceMap0, CRenderTexture *&balanceMap1 )
			{
				if ( amountFactor <= 0 )
				{
					balanceMap0 = nullptr;
					balanceMap1 = nullptr;
					lerpFactor = 0;
				}

				if ( balanceMap0 == balanceMap1 )
				{
					balanceMap1 = nullptr;
				}

				if ( balanceMap0 && balanceMap1 )
				{
					if ( lerpFactor <= 0 )
					{
						balanceMap1 = nullptr;
					}
					else if ( lerpFactor >= 1 )
					{
						balanceMap0 = balanceMap1;
						balanceMap1 = nullptr;
						lerpFactor = 0;
					}
				}
				else if ( balanceMap0 )
				{
					lerpFactor = 0;
				}
				else if ( balanceMap1 )
				{
					balanceMap0 = balanceMap1;
					balanceMap1 = nullptr;
					lerpFactor = 0;
				}
				else
				{
					lerpFactor = 0;
					amountFactor = 0;
					postBrightness = 1;
				}

				lerpFactor = Clamp( lerpFactor, 0.f, 1.f );
				amountFactor = Clamp( amountFactor, 0.f, 1.f );
				postBrightness = Max( 0.f, postBrightness );

				RED_ASSERT( !((nullptr == balanceMap0) && (nullptr != balanceMap1)) );
				RED_ASSERT( (amountFactor > 0) == (nullptr != balanceMap0) );
			}
		};

		// Optimize input
		{	
			blendFactor = Clamp( blendFactor, 0.f, 1.f );
			Local::OptimizePair( lerpFactor, amountFactor, postBrightness, balanceMap0, balanceMap1 );
			Local::OptimizePair( lerpFactorB, amountFactorB, postBrightnessB, balanceMap0B, balanceMap1B );

			if ( amountFactor > 0 && amountFactorB > 0 )
			{
				if ( 0 == blendFactor )
				{
					Local::ResetPair( lerpFactorB, amountFactorB, postBrightnessB, balanceMap0B, balanceMap1B );
				}
				else if ( 1 == blendFactor )
				{
					lerpFactor = lerpFactorB;
					amountFactor = amountFactorB;
					postBrightness = postBrightnessB;
					balanceMap0 = balanceMap0B;
					balanceMap1 = balanceMap1B;
					Local::ResetPair( lerpFactorB, amountFactorB, postBrightnessB, balanceMap0B, balanceMap1B );
					//
					blendFactor = 0;
				}
			}
			else if ( amountFactor > 0 )
			{
				amountFactor *= (1 - blendFactor);
				blendFactor = 0;
			}
			else if ( amountFactorB > 0 )
			{
				lerpFactor = lerpFactorB;
				amountFactor = amountFactorB;
				postBrightness = postBrightnessB;
				balanceMap0 = balanceMap0B;
				balanceMap1 = balanceMap1B;
				Local::ResetPair( lerpFactorB, amountFactorB, postBrightnessB, balanceMap0B, balanceMap1B );
				//
				amountFactor *= blendFactor;
				blendFactor = 0;
			}

			// Merge [double->double] into [double] in case balancemap pairs are the same
			if ( amountFactorB > 0 && balanceMap1 && balanceMap1B )
			{
				if ( balanceMap0 == balanceMap1B && balanceMap1 == balanceMap0B )
				{
					Swap( balanceMap0B, balanceMap1B );
					lerpFactorB = 1 - lerpFactorB;
				}

				if ( balanceMap0 == balanceMap0B && balanceMap1 == balanceMap1B )
				{
					amountFactor = Lerp( blendFactor, amountFactor, amountFactorB );
					postBrightness = Lerp( blendFactor, postBrightness, postBrightnessB );
					lerpFactor = Lerp( blendFactor, lerpFactor, lerpFactorB );
					//
					Local::ResetPair( lerpFactorB, amountFactorB, postBrightnessB, balanceMap0B, balanceMap1B );
					blendFactor = 0;
				}
			}

			// Merge [single->single] into [single] in case balancemap pairs are the same
			if ( amountFactorB > 0 && !balanceMap1 && !balanceMap1B )
			{
				ASSERT( balanceMap0 && balanceMap0B );
				if ( balanceMap0 == balanceMap0B )
				{
					amountFactor = Lerp( blendFactor, amountFactor, amountFactorB );
					postBrightness = Lerp( blendFactor, postBrightness, postBrightnessB );
					lerpFactor = Lerp( blendFactor, lerpFactor, lerpFactorB );
					//
					Local::ResetPair( lerpFactorB, amountFactorB, postBrightnessB, balanceMap0B, balanceMap1B );
					blendFactor = 0;
				}
			}
		}

		// Early exit
		if ( amountFactor <= 0 )
		{
			ASSERT( 0 == amountFactorB );
			return false;
		}

		// Setup rendertargets
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
		rtSetup.SetViewport( targetArea.m_width, targetArea.m_height, targetArea.m_offsetX, targetArea.m_offsetY );
		GpuApi::SetupRenderTargets( rtSetup );		

		// Set draw context
		CGpuApiScopedDrawContext scopedDrawContext( drawContext );
		
		// Choose shader
		CRenderShaderPair* shader = NULL;
		ASSERT( !(balanceMap1 && !balanceMap0) );
		ASSERT( !(balanceMap1B && !balanceMap0B) );
		ASSERT( !(balanceMap0B && !balanceMap0) );
		ASSERT( !(amountFactorB>0 && amountFactor<=0) );
		Bool useBothMaps = balanceMap0 && balanceMap1;
		Bool useBothMapsB = balanceMap0B && balanceMap1B;
		if ( blendFactor > 0 ) // shader for blending
		{
			if ( useBothMaps ) // double to [double/single]
			{
				shader = useBothMapsB ? GetRenderer()->m_postFXCopyWithBalanceMapDoubleBlendDouble : GetRenderer()->m_postFXCopyWithBalanceMapDoubleBlendSingle;
			}
			else // single to [double/single]
			{
				shader = useBothMapsB ? GetRenderer()->m_postFXCopyWithBalanceMapSingleBlendDouble : GetRenderer()->m_postFXCopyWithBalanceMapSingleBlendSingle;
			}
		}
		else // no blending
		{
			shader = useBothMaps ? GetRenderer()->m_postFXCopyWithBalanceMapDouble : GetRenderer()->m_postFXCopyWithBalanceMapSingle;
		}
		ASSERT( shader, TXT("I don't know how, but no shader was chosen for the balancemaps!") );

		// Set textures
		ASSERT( RTN_None != sampleTarget );
		GpuApi::TextureRef sampleTargetRef = surfaces->GetRenderTargetTex( sampleTarget );
		GpuApi::BindTextures( 0, 1, &sampleTargetRef, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		if ( NULL != balanceMap0 )
			balanceMap0->Bind( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		if ( NULL != balanceMap1 )
			balanceMap1->Bind( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		if ( NULL != balanceMap0B )
			balanceMap0B->Bind( 3, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );
		if ( NULL != balanceMap1B )
			balanceMap1B->Bind( 4, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		// Render
		const Int32 sampleFullWidth  = surfaces->GetRenderTarget( sampleTarget )->GetWidth();
		const Int32 sampleFullHeight = surfaces->GetRenderTarget( sampleTarget )->GetHeight();
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( targetArea, sampleArea, sampleFullWidth, sampleFullHeight ) );		
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0 + 1,	Vector ( lerpFactor, amountFactor, postBrightness, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst(	 PSC_Custom_0 + 2,	Vector ( lerpFactorB, amountFactorB, postBrightnessB, blendFactor ) );

		drawer.DrawQuad( shader );
		
		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
		return true;
	}
};
