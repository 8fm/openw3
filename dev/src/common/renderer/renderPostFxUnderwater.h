
#include "renderEnvProbeManager.h"
#include "renderTextureArray.h"

class CPostFxUnderwater
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

public:
	CPostFxUnderwater ()
	{
	}

	~CPostFxUnderwater ()
	{
	}
	
	void ApplyUnderwaterFx( 
		CPostProcessDrawer &drawer,								class CRenderSurfaces* surfaces,
		ERenderTargetName sampleTarget,							const TexelArea &sampleArea,
		ERenderTargetName renderTarget,							const TexelArea &renderArea,
		GpuApi::TextureRef	underwaterIntersectionTexture,		Float intensity,
		GpuApi::TextureRef	furier )
	{
		ASSERT( intensity > 0.f );
		ASSERT( renderArea.IsBothDimsGreaterOrEqual( sampleArea ) );

		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_MirrorLinearNoMip );

		// Horizontal pass
		{
			ASSERT( RTN_None != sampleTarget );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( renderTarget ) );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			GpuApi::SetupRenderTargets( rtSetup );

			GpuApi::TextureRef colorRef = surfaces->GetRenderTargetTex( sampleTarget );
			GpuApi::BindTextures( 0, 1, &colorRef, GpuApi::PixelShader );
			
			GpuApi::TextureRef sceneDepth =  GetRenderer()->GetSurfaces()->GetDepthBufferTex();
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );
						
			if( furier ) 
			{
				GpuApi::BindTextures( 5, 1, &furier, GpuApi::PixelShader );
				GpuApi::SetSamplerStatePreset( 5, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, GpuApi::PixelShader );
			}

			if( underwaterIntersectionTexture ) 
			{			
				GpuApi::BindTextures( 2, 1, &underwaterIntersectionTexture, GpuApi::PixelShader );
				GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, GpuApi::PixelShader );
			}			
			// Bind scene depth texture
			GpuApi::TextureRef sceneDepthLinear = surfaces->GetRenderTargetTex( RTN_GBuffer3Depth );
			GpuApi::BindTextures( 3, 1, &sceneDepthLinear, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 3, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, GpuApi::PixelShader );

			// normals for uv projection			
			GpuApi::BindTextures( 1, 1, nullptr, GpuApi::PixelShader );

			// bind ambient
			GpuApi::TextureRef envProbeRefs = GetRenderer()->GetEnvProbeManager()->GetAmbientEnvProbeAtlas();
			GpuApi::BindTextures( 22, 1, &envProbeRefs, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 11, GpuApi::SAMPSTATEPRESET_ClampLinearMipNoBias, GpuApi::PixelShader );

			const Int32 sampleFullWidth	= surfaces->GetRenderTarget( sampleTarget )->GetWidth();
			const Int32 sampleFullHeight	= surfaces->GetRenderTarget( sampleTarget )->GetHeight();
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( renderArea.GetWithNoOffset(), sampleArea, sampleFullWidth, sampleFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( sampleArea, sampleFullWidth, sampleFullHeight ) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector(1.0f/sampleFullWidth,0,0,0) );

			
			//CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet_StencilMatchAny );
			//CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcBlend );
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet ); // first pass always without blend

			drawer.DrawQuad( GetRenderer()->m_postfxUnderwater );

			// Unbind textures
			GpuApi::BindTextures( 0, 6, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( 22, 1, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );
			if( underwaterIntersectionTexture ) GpuApi::BindTextures( 2, 1, nullptr, GpuApi::PixelShader );
		}		
				
		// Restore original rendertargets
		targetStateGrabber.Restore();
	}

	
};
