/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
	Kamil Nowakowski
*/
#include "build.h"

#include "renderGameplayRenderTarget.h"

#include "renderInterface.h"
#include "renderRenderSurfaces.h"
#include "renderTexture.h"
#include "renderShaderPair.h"

RED_INLINE Uint32 NextPowerOfTwo(Uint32 x)
{
	// If not power of two, round it. 
	if( x & ( x-1 ) )
	{
		x |= (x >> 1);
		x |= (x >> 2);
		x |= (x >> 4);
		x |= (x >> 8);
		x |= (x >> 16);
		return x+1;
	}
	return x;
}

CRenderGameplayRenderTarget::CRenderGameplayRenderTarget( const AnsiChar* tag )
	: m_tag( tag )
	, m_renderTexture( NULL )
{

}

CRenderGameplayRenderTarget::~CRenderGameplayRenderTarget()
{
	if( m_renderTexture )
	{
		GpuApi::Release( m_renderTexture );
	}
}

GpuApi::TextureRef CRenderGameplayRenderTarget::GetGpuTexture() const
{
	return m_renderTexture;
}

Bool CRenderGameplayRenderTarget::RequestResizeRenderSurfaces( Uint32 width, Uint32 height )
{

	Bool needResize = !m_renderTexture || ( width > m_textureWidth ) || ( height > m_textureHeight );

	if( !needResize )
	{
		// Doesn't need to resize, everything here is OK
		return true;
	}
	
	if( m_renderTexture )
	{
		GpuApi::Release( m_renderTexture );
	}
	
	m_imageWidth	= width;
	m_imageHeight	= height;
	
	m_textureWidth	= NextPowerOfTwo( width ) ;
	m_textureHeight = NextPowerOfTwo( height );
	
	GpuApi::TextureDesc desc;
	desc.type			= GpuApi::TEXTYPE_2D;
	desc.width			= m_textureWidth;
	desc.height			= m_textureHeight;
	desc.initLevels		= 1;
	desc.msaaLevel		= 0;
	desc.usage			= GpuApi::TEXUSAGE_RenderTarget | GpuApi::TEXUSAGE_Samplable;
	desc.format			= GpuApi::TEXFMT_R8G8B8A8;

	m_renderTexture = GpuApi::CreateTexture( desc, GpuApi::TEXG_UI );
	GpuApi::SetTextureDebugPath( m_renderTexture, "gameplayRenderTarget" );
	if( m_renderTexture )
	{
		//GpuApi::ClearColorTarget( m_renderTexture, Vector::ZEROS.A );
		return true;
	}

	RED_ASSERT( m_renderTexture );
	return false;
}

void CRenderGameplayRenderTarget::CopyFromRenderTarget( ERenderTargetName sourceRenderTarget, Rect sourceRect )
{
	// Get rendering surfaces to use
	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	ASSERT( surfaces );

	CRenderStateManager& m_stateManager = GetRenderer()->GetStateManager();

	const GpuApi::TextureDesc& desc = GpuApi::GetTextureDesc( GetGpuTexture() );
	::Rect  desitnationRect;
	desitnationRect.m_left = 0;
	desitnationRect.m_top = 0;
	desitnationRect.m_right = desc.width;
	desitnationRect.m_bottom = desc.height;
	
	if ( m_copyMode == CopyMode::Normal )
	{
		GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( sourceRenderTarget ), sourceRect, GetGpuTexture(), desitnationRect );
	}
	else if ( m_copyMode == CopyMode::TextureMask )
	{
		CGpuApiScopedDrawContext context( GpuApi::DRAWCONTEXT_PostProcSet );

		CRenderStateManager& stateManager = GetRenderer()->GetStateManager();

		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, GetGpuTexture() );
		rtSetup.SetViewport( GetImageWidth(), GetImageHeight() );
		GpuApi::SetupRenderTargets( rtSetup );

		CRenderTarget* source = surfaces->GetRenderTarget( sourceRenderTarget );
		RED_FATAL_ASSERT( source != nullptr, "" );

		// Make sure we don't have any extra shaders.
		stateManager.SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
		stateManager.SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
		stateManager.SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

		{
			const GpuApi::TextureRef textures[] = { source->GetTexture(), surfaces->GetDepthBufferTex() };
			GpuApi::BindTextures( 0 , 2 , textures , GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		}
		{
			static_cast<CRenderTexture*>( m_textureMask )->Bind( 2 );
			GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		}

		// Render
		stateManager.SetVertexConst( VSC_Custom_0,	Vector( 0.0, 0.0, GetImageWidth() / Float( source->GetWidth() ) , GetImageHeight() / Float(source->GetHeight() ) ) );
		stateManager.SetVertexConst( VSC_Custom_1,	Vector( 0.0, 0.0, 1.0f, 1.0f ) );

		GetRenderer()->m_simpleCopyKeyMasking->Bind();

		GpuApi::DrawPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleStrip, 2 );			
		GpuApi::BindTextures( 0 , 3 , nullptr , GpuApi::PixelShader );
	}
	else if ( m_copyMode == CopyMode::BackgroundColor )
	{
		if ( m_backgroundColor != Vector::ZEROS )
		{
			// just copy; in this path background is applied right after g-buffer opaque pass has completed (in renderRenderFrame.cpp)
			GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( sourceRenderTarget ), sourceRect, GetGpuTexture(), desitnationRect );
		}
		else
		{
			GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( sourceRenderTarget ), sourceRect, GetGpuTexture(), desitnationRect );

			GpuApi::SetCustomDrawContext( GpuApi::DSSM_NoStencilDepthTestEqual, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set );

			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, GetGpuTexture() );
			rtSetup.SetDepthStencilTarget( GetRenderer()->GetSurfaces()->GetDepthBufferTex() );
			rtSetup.SetViewport( GetImageWidth(), GetImageHeight() );
			GpuApi::SetupRenderTargets( rtSetup );

			CRenderStateManager& stateManager = GetRenderer()->GetStateManager();

			stateManager.SetLocalToWorld( NULL );
			stateManager.SetCamera2D();

			GetRenderer()->m_shaderSingleColor->Bind();
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, GetBackgroundColor() );
		
			GetRenderer()->GetDebugDrawer().DrawQuad( Vector( 0.f, 0.f, 0.f, 1.f ), Vector( ( Float )GetImageWidth(), ( Float )GetImageHeight(), 0.f, 1.f ), 0.0f );
		}
	}
	else
	{
		RED_ASSERT( !"INVALID" );
	}
}
