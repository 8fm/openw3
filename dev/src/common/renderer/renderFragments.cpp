/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderEnvProbe.h"
#include "renderDebugMesh.h"
#include "renderShaderPair.h"
#include "renderTexture.h"
#include "renderEnvProbeManager.h"
#include "renderScene.h"
#include "apexRenderInterface.h"
#include "../engine/renderFragment.h"

const Vector GDefaultDetailLevelParams ( 0.f, 0.f, 0.f, 0.f );

void CRenderFragmentText::Draw( const RenderingContext& context )
{
	for( Uint32 i=0; i<m_numTextures; ++i )
	{
		CRenderTexture* texture = static_cast< CRenderTexture* >( m_textures[i] );
		RED_ASSERT( texture != nullptr, TXT("Missing text texture") );
		if ( texture != nullptr )
		{
			GetRenderer()->GetDebugDrawer().DrawText( &GetLocalToWorld(), m_vertices[i], m_numVertices[i] / 3, texture, true );
		}
	}
}

void CRenderFragmentDebugLineList::Draw( const RenderingContext& context )
{
	// Draw lines using debug drawer
	GetRenderer()->GetDebugDrawer().DrawLineList( &GetLocalToWorld(), m_points, GetNumPrimitives() );
}

void CRenderFragmentDebugIndexedLineList::Draw( const RenderingContext& context )
{
	// Draw lines using debug drawer
	GetRenderer()->GetDebugDrawer().DrawIndexedLineList( &GetLocalToWorld(), m_points, m_numPoints, m_indices, GetNumPrimitives() );
}

void CRenderFragmentDebugPolyList::Draw( const RenderingContext& context )
{
	// Draw polygons using debug drawer
	GetRenderer()->GetDebugDrawer().DrawPolyList( &GetLocalToWorld(), m_points, m_numPoints, m_indices, m_numIndices );
}

void CRenderFragmentDebugRectangle::Draw( const RenderingContext& context )
{
	GetRenderer()->GetDebugDrawer().DrawQuad2D( &m_translationMatrix, m_width, m_height, m_shiftY, m_color );
}

void CRenderFragmentDebugRectangle2D::Draw( const RenderingContext& context )
{
	GetRenderer()->GetDebugDrawer().DrawQuad2DExf( m_x, m_y, m_width, m_height, m_color );
}

void CRenderFragmentTexturedDebugRectangle::Draw( const RenderingContext& context )
{
	CRenderTexture* tex = m_texture.Get< CRenderTexture >();
	if ( tex )
	{
		// Draw the 2D rectangle
		const Matrix m = Matrix::IDENTITY;
		GetRenderer()->GetStateManager().SetLocalToWorld( &m );
		GetRenderer()->GetDebugDrawer().DrawTile( m_vertices, tex, context.m_pass, false, m_withAlpha );
	}
}

void CRenderFragmentDynamicTexture::Draw( const RenderingContext& context )
{
	GetRenderer()->GetDebugDrawer().DrawTexturePreviewTile( m_x, m_y, m_width, m_height, m_textureRef, m_mipIndex, m_sliceIndex, m_colorMin, m_colorMax, m_channelSelector );
}

void CRenderFragmentDebugSprite::Draw( const RenderingContext& context )
{
	CRenderTexture* texture = m_texture.Get< CRenderTexture >();
	RED_ASSERT( texture != nullptr, TXT("Missing sprite texture") );
	if ( texture != nullptr )
	{
		// Draw the sprite
		GetRenderer()->GetStateManager().SetLocalToWorld( &GetLocalToWorld() );
		GetRenderer()->GetDebugDrawer().DrawTile( m_vertices, texture, context.m_pass, true );
	}
}

void CRenderFragmentDebugEnvProbe::Draw( const RenderingContext& context )
{
	// Draw ambient
	{
		const Float radius0 = 0.5f * 0.75f;
		const Float radius1 = 0.5f * radius0;

		const Uint32 numMips = CRenderEnvProbeManager::GetEnvProbeTypeNumMips( ENVPROBETYPE_Ambient );
		for ( Uint32 mip_i=0; mip_i<numMips; ++mip_i )
		{
			DrawSingleCube( context, Vector ( -1.f, 0, mip_i * 2.05f * radius0 ), radius0, DRAWMODE_CubeAmbient, mip_i );
		}
	}

	// Draw reflection
	{
		const Float radius = 0.75f;
						
		for ( Uint32 mip_i=0; mip_i<CRenderEnvProbeManager::GetEnvProbeTypeNumMips( ENVPROBETYPE_Reflection ); ++mip_i )
		{
			DrawSingleCube( context, Vector ( 1.f, 0, mip_i * 2.05f * radius ), radius, DRAWMODE_CubeReflection, mip_i );
		}
	}
}

void CRenderFragmentDebugEnvProbe::DrawSingleCube( const RenderingContext& context, const Vector &posOffset, Float radius, EDrawMode drawMode, Int32 forcedMipIndex )
{
	Matrix finalLocalToWorld = GetLocalToWorld();
	finalLocalToWorld = Matrix().SetIdentity().SetTranslation( posOffset ) * finalLocalToWorld;
	finalLocalToWorld = Matrix().SetIdentity().SetScale33( radius ) * finalLocalToWorld;

	// Handle hit proxy
	if ( RP_HitProxies == context.m_pass )
	{
		GetRenderer()->GetStateManager().SetLocalToWorld( &finalLocalToWorld );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, m_hitProxy.GetColor().ToVector() );
		GetRenderer()->GetDebugDrawer().GetShaderSingleColor()->Bind();
		GetRenderer()->GetDebugDrawer().DrawUnitSphere();

		return;
	}

	// Get envProbe (null is valid)
	CRenderEnvProbe* envProbe = m_renderResource.Get< CRenderEnvProbe >();
	
	// Render cube texture
	if ( envProbe && envProbe->GetDynamicData().IsReadyToDisplay( *GetRenderer()->GetEnvProbeManager() ) )
	{
		const Int32 arraySlotIndex = envProbe->GetDynamicData().m_arraySlotIndex;
		const Float effectIntensity = GetRenderer()->GetEnvProbeManager()->GetCubeSlotData( arraySlotIndex ).m_probeParams.GetEffectIntensity( m_gameTime );

		GetRenderer()->GetStateManager().SetLocalToWorld( &finalLocalToWorld );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)arraySlotIndex, DRAWMODE_CubeAmbient == drawMode ? effectIntensity : 0.f, DRAWMODE_CubeReflection == drawMode ? effectIntensity : 0.f, 0.f ) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)Max( -1, forcedMipIndex ), (Float)Max( -1, forcedMipIndex ), 0, 0 ) );
		GetRenderer()->m_shaderEnvProbe->Bind();

		GpuApi::TextureRef tex[] = { GetRenderer()->GetEnvProbeManager()->GetAmbientEnvProbeAtlas(), GetRenderer()->GetEnvProbeManager()->GetReflectionEnvProbeAtlas() };
		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearMipNoBias, GpuApi::PixelShader );
		
		GetRenderer()->GetDebugDrawer().DrawUnitSphere();

		GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
	}
	else // Render fallback cube
	{
		GetRenderer()->GetStateManager().SetLocalToWorld( &finalLocalToWorld );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, envProbe && -1 != envProbe->GetDynamicData().m_arraySlotIndex ? Vector ( 0.25f, 1, 0, 1 ) : Vector ( 1, 0, 1, 1 ) );
		GetRenderer()->GetDebugDrawer().GetShaderSingleColor()->Bind();
		GetRenderer()->GetDebugDrawer().DrawUnitSphere();
	}
}

void CRenderFragmentDebugMesh::Draw( const RenderingContext& context )
{
	CRenderDebugMesh* mesh = m_mesh.Get< CRenderDebugMesh >();
	if ( mesh )
	{
		// Set draw context
		CGpuApiScopedDrawContext drawContext( m_transparent ? GpuApi::DRAWCONTEXT_DebugTransparent : GpuApi::DRAWCONTEXT_DebugMesh );

		// Set transformation
		GetRenderer()->GetStateManager().SetLocalToWorld( &GetLocalToWorld() );

		// Draw mesh
		if ( context.m_pass == RP_HitProxies )
		{
			const Vector hitProxyColor = m_hitProxy.GetColor().ToVector();
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, hitProxyColor );
			mesh->Draw( false, true );
		}
		else if( m_useColor )
		{
			Bool oldWireframe = GpuApi::GetRenderSettings().wireframe;
			GpuApi::SetRenderSettingsWireframe( m_wireframe );
			const Vector hitProxyColor = m_hitProxy.GetColor().ToVector();
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, hitProxyColor );
			mesh->Draw( false, true );
			GpuApi::SetRenderSettingsWireframe( oldWireframe );
		}
		else
		{
			Bool oldWireframe = GpuApi::GetRenderSettings().wireframe;
			GpuApi::SetRenderSettingsWireframe( m_wireframe );
			mesh->Draw( false, false );
			GpuApi::SetRenderSettingsWireframe( oldWireframe );
		}
	}
}

void CRenderFragmentDebugWireMesh::Draw( const RenderingContext& context )
{
	CRenderDebugMesh* mesh = m_mesh.Get< CRenderDebugMesh >();
	if ( mesh )
	{
		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_DebugMesh );

		// Set transformation
		GetRenderer()->GetStateManager().SetLocalToWorld( &GetLocalToWorld() );

		// Draw mesh
		mesh->Draw( true, false );
	}
}

void CRenderFragmentDebugWireSingleColorMesh::Draw( const RenderingContext& context )
{
	CRenderDebugMesh* mesh = m_mesh.Get< CRenderDebugMesh >();
	if ( mesh )
	{
		// Set draw context
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_DebugMesh );

		// Set transformation
		GetRenderer()->GetStateManager().SetLocalToWorld( &GetLocalToWorld() );

		// Draw mesh
		mesh->Draw( true, true );
	}
}
