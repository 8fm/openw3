
#include "renderTextureArray.h"
#include "renderEnvProbeManager.h"

class CPostFxSurfaceFlow
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

public:
	CPostFxSurfaceFlow ()
	{
	}

	~CPostFxSurfaceFlow ()
	{
	}

	void ApplySurfaceFlowFx( 
		CPostProcessDrawer &drawer,			class CRenderSurfaces* surfaces, 
		ERenderTargetName renderTarget,		const TexelArea &renderArea, 
		ERenderTargetName helperTarget,		TexelArea helperArea,
		CRenderTextureArray	*flowTextureArray, Float wetSurfaceIntensity, GpuApi::TextureRef waterIntersectionTexture
		)
	{
		RED_ASSERT( flowTextureArray != nullptr, TXT("flowTextureArray is null! Caller should check this") );
		if ( flowTextureArray == nullptr )
		{
			return;
		}

		// Grab target states
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();
		CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

		// Correct helper area to have the same size as sample area (we don't want any stretching)
		helperArea.m_width  = renderArea.m_width;
		helperArea.m_height = renderArea.m_height;

		// Set sampler state
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip );

		{
			GpuApi::TextureRef sceneDepth =  GetRenderer()->GetSurfaces()->GetDepthBufferTex();

			GpuApi::TextureRef helperRef = surfaces->GetRenderTargetTex( helperTarget );
			GpuApi::TextureRef renderRef = surfaces->GetRenderTargetTex( renderTarget );

			GetRenderer()->StretchRect( renderRef, helperRef );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, renderRef );
			rtSetup.SetViewport( renderArea.m_width, renderArea.m_height, renderArea.m_offsetX, renderArea.m_offsetY );
			// need stencil for dynamic object cut-out
			rtSetup.SetDepthStencilTarget( sceneDepth, -1, true );

			GpuApi::SetupRenderTargets( rtSetup );

			flowTextureArray->Bind( 4, RST_PixelShader );

			GpuApi::BindTextures( PSSMP_SceneDepth, 1, &sceneDepth, GpuApi::PixelShader );
			GpuApi::BindTextures( 0, 1, &helperRef, GpuApi::PixelShader );			
			GpuApi::BindTextures( 6, 1, &waterIntersectionTexture, GpuApi::PixelShader );			
			
			const Int32 sampleFullWidth	= surfaces->GetRenderTarget( renderTarget )->GetWidth();
			const Int32 sampleFullHeight	= surfaces->GetRenderTarget( renderTarget )->GetHeight();
			stateManager.SetVertexConst( VSC_Custom_0 + 0,	CalculateTexCoordTransform( helperArea.GetWithNoOffset(), renderArea, sampleFullWidth, sampleFullHeight ) );		
			stateManager.SetPixelConst(  PSC_Custom_0 + 0,	CalculateTexCoordClamp( renderArea, sampleFullWidth, sampleFullHeight ) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 1,	Vector(1.0f/sampleFullWidth,0,0,0) );
			stateManager.SetPixelConst(  PSC_Custom_0 + 2,	Vector(wetSurfaceIntensity,0,0,0) );
						
			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet_StencilMatchNone, LC_DynamicObject );

			drawer.DrawQuad( GetRenderer()->m_postfxSurfaceFlow );

			// Unbind textures
			GpuApi::BindTextures( 0, 6, nullptr, GpuApi::PixelShader );
			GpuApi::BindTextures( PSSMP_SceneDepth, 1, nullptr, GpuApi::PixelShader );			
		}

		// Restore original rendertargets
		targetStateGrabber.Restore();
	}

};
