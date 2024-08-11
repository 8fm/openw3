

class CPostFxBlur
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:

	CPostFxBlur ()
	{
	}

	~CPostFxBlur ()
	{
	}

	void Apply( 
		CPostProcessDrawer &drawer, class CRenderSurfaces* surfaces, const class CEnvRadialBlurParameters& blurParams, const CRenderCamera &camera,
		ERenderTargetName sampleTarget, const TexelArea &sampleArea )
	{
		// Setup render
		TexelArea renderArea ( GpuApi::GetViewport().width, GpuApi::GetViewport().height, 0, 0 );
		
		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );

		// Set textures
		ASSERT( RTN_None != sampleTarget );
		GpuApi::TextureRef target = surfaces->GetRenderTargetTex( sampleTarget );
		GpuApi::BindTextures( 0, 1, &target, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

		Float blurAmountParam = blurParams.m_radialBlurAmount;
		Vector vCenterOfBlurVS( camera.GetWorldToScreen().TransformVectorWithW( Vector( blurParams.m_radialBlurSource.X, blurParams.m_radialBlurSource.Y, blurParams.m_radialBlurSource.Z, 1.0f ) ) );
		vCenterOfBlurVS /= vCenterOfBlurVS.W;
		vCenterOfBlurVS = Vector::Clamp4( vCenterOfBlurVS, -1.0f, 1.0f );

		const Float amount = Clamp( (blurParams.m_radialBlurSource - camera.GetPosition()).Normalized3().Dot3( camera.GetCameraForward().Normalized3() ), 0.0f , 1.0f )
			/ Max( MSqrt((blurParams.m_radialBlurSource - camera.GetPosition()).Mag3()) - 2.0f, 1.0f );
		blurAmountParam *= amount * amount;

		// Render
		const Int32 sampleFullWidth	= surfaces->GetRenderTarget( sampleTarget )->GetWidth();
		const Int32 sampleFullHeight	= surfaces->GetRenderTarget( sampleTarget )->GetHeight();			
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea, sampleArea, sampleFullWidth, sampleFullHeight ) );		
		GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0 + 1,	vCenterOfBlurVS );		
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 0,	Vector( blurParams.m_radialBlurSource.X, blurParams.m_radialBlurSource.Y, blurParams.m_centerMultiplier, blurAmountParam ) );		
		GetRenderer()->GetStateManager().SetPixelConst(  PSC_Custom_0 + 1,	Vector( blurParams.m_sineWaveAmount * amount, blurParams.m_sineWaveSpeed, blurParams.m_sineWaveFreq, 0.0f ) );		
		drawer.DrawQuad( GetRenderer()->m_postfxBlur );

		// Unbind texture
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	}
};
