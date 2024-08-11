/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderMesh.h"
#include "renderMaterial.h"
#include "renderShaderPair.h"
#include "renderSkybox.h"
#include "../engine/renderFragment.h"
#include "../engine/meshEnum.h"


CRenderSkybox::CRenderSkybox ()
{}

CRenderSkybox::~CRenderSkybox ()
{
	m_setup.ReleaseAll();
}

void CRenderSkybox::SetParameters( const SSkyboxSetupParameters &params )
{
	m_setup.ReleaseAll();
	m_setup = params;
	m_setup.AddRefAll();
}

void CRenderSkybox::Tick( Float timeDelta )
{	
	// empty
}

static Bool DrawSkyboxChunk( const RenderingContext &context, IRenderResource *meshResource, IRenderResource *materialResource, IRenderResource *materialParamsResource, MeshDrawingStats &meshStats )
{
	if ( !(meshResource && materialResource && materialParamsResource) )
	{
		return false;
	}
	
	CRenderMesh* mesh = static_cast<CRenderMesh*>( meshResource );
	CRenderMaterial *material = static_cast<CRenderMaterial*>( materialResource );
	CRenderMaterialParameters *materialParams = static_cast<CRenderMaterialParameters*>( materialParamsResource );

	if ( !mesh->IsFullyLoaded() )
	{
		return false;
	}

	// Prepare material context
	MaterialRenderingContext materialContext( context );
	materialContext.m_vertexFactory = MVF_MeshStatic;
	materialContext.m_selected = false;
	materialContext.m_discardingPass = material->IsMasked();

	GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, Vector( material->IsMasked(), 0, 0, 0 ) );

	
	// Bind material, the distance is set to 0, so that the skybox textures are always high res
	if ( !material->Bind( materialContext, materialParams, 0.f ) )
	{
		return false;
	}
	
	// Draw the shit
	for ( Uint32 chunk_i=0; chunk_i<mesh->GetNumChunks(); ++chunk_i )
	{
		// Draw only scene chunks here
		const auto& chunkInfo = mesh->GetChunks()[chunk_i];
		if ( chunkInfo.m_baseRenderMask & MCR_Scene )
		{
			mesh->Bind( chunk_i );
			mesh->DrawChunkNoBind( chunk_i, 1, meshStats );
		}
	}

	return true;
}

static Matrix CalcSunMoonLocalToWorld( const Vector &cameraPos, const Vector &objectDir, Float sizeScale )
{
	const Float objectDist = CRenderSkybox::SUN_AND_MOON_RENDER_DIST;
	const Float objectSize = CRenderSkybox::SUN_AND_MOON_RENDER_SCALE_BASE * sizeScale;

	Matrix matrix;
	//matrix.BuildFromDirectionVector( objectDir );
	matrix.SetIdentity();
	matrix.SetTranslation( cameraPos + objectDir * objectDist );
	matrix.SetScale33( objectSize );
	return matrix;
}

Bool IsSkyboxMeshValid( IRenderResource *meshResource )
{
	if ( !meshResource || CNAME( RenderMesh ) != meshResource->GetCategory() )
	{
		return false;
	}

	return true;

/*
	CRenderMesh &mesh = *static_cast<CRenderMesh*>( meshResource );
	for ( Uint32 chunk_i=0; chunk_i<mesh.GetNumChunks(); ++chunk_i )
	{
		const CRenderMesh::Chunk &chunk = mesh.GetChunks()[chunk_i];
		const Vector chunkSize = chunk.m_boundingBox.CalcSize();
		if ( Abs( chunkSize.X - 2.f ) > 0.001f || Abs( chunkSize.Y - 2.f ) > 0.001f || chunkSize.Z < 0.999f || chunkSize.Z > 2.001f )
		{
			return false;
		}
	}

	return true;
*/
}

void CRenderSkybox::Render( const CRenderFrameInfo &info, const RenderingContext &context, MeshDrawingStats &meshStats, Bool allowNonClouds, Bool allowClouds ) const
{
	ASSERT( info.m_camera.IsReversedProjection() == GpuApi::IsReversedProjectionState() );

	if ( !(allowNonClouds || allowClouds) )
	{
		return;
	}

	Matrix localToWorld;
	
	const GpuApi::ViewportDesc origViewport = GpuApi::GetViewport();

	// Setup viewport so that rendered objects would be forced to be on the farplane
	GpuApi::ViewportDesc currViewport = origViewport;
	if ( info.m_camera.IsReversedProjection() )
	{
		currViewport.maxZ = currViewport.minZ;
	}
	else
	{
		currViewport.minZ = currViewport.maxZ;
	}
	GpuApi::SetViewport( currViewport );

	// Disable two sided render in case it's enabled
	const CGpuApiScopedTwoSidedRender scopedForcedTwoSided ( false );
	
	// Draw skybox
	if ( allowNonClouds )
	{
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_OpaqueNoDepthWrite );

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetIdentity().SetTranslation( context.GetCamera().GetPosition() ).SetScale33( SUN_AND_MOON_RENDER_DIST ) );
	
		ASSERT( !(m_setup.m_skyboxMeshResource && !IsSkyboxMeshValid(m_setup.m_skyboxMeshResource)) && "Expected unit skydome mesh" );
		if ( !DrawSkyboxChunk( context, m_setup.m_skyboxMeshResource, m_setup.m_skyboxMaterialResource, m_setup.m_skyboxMaterialParamsResource, meshStats ) )
		{
			GetRenderer()->m_shaderSkyColor->Bind();
			GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );
		}
	}
	      
	// Draw moon
	const Float moonSize = info.m_envParametersArea.m_sunParams.m_moonSize.GetScalar();
	if ( allowNonClouds && moonSize > 0.f )
	{		
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Transparency );

		localToWorld = CalcSunMoonLocalToWorld( context.GetCamera().GetPosition(), info.m_envParametersDayPoint.m_moonDirection, moonSize );
		
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, info.m_envParametersArea.m_sunParams.m_moonColor.GetColorScaledGammaToLinear(true) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld );
		DrawSkyboxChunk( context, m_setup.m_moonMeshResource, m_setup.m_moonMaterialResource, m_setup.m_moonMaterialParamsResource, meshStats );
	}

	// Draw sun
	const Float sunSize = info.m_envParametersArea.m_sunParams.m_sunSize.GetScalar();
	if ( allowNonClouds && sunSize > 0.f )
	{
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Transparency );
		
		localToWorld = CalcSunMoonLocalToWorld( context.GetCamera().GetPosition(), info.m_envParametersDayPoint.m_sunDirection, sunSize );

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, info.m_envParametersArea.m_sunParams.m_sunColor.GetColorScaledGammaToLinear(true) );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld );
		DrawSkyboxChunk( context, m_setup.m_sunMeshResource, m_setup.m_sunMaterialResource, m_setup.m_sunMaterialParamsResource, meshStats );
	}

	// Draw clouds
	if ( allowClouds )
	{
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_Transparency );

		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, Vector::ZEROS );
		GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetIdentity().SetTranslation( context.GetCamera().GetPosition() ).SetScale33( SUN_AND_MOON_RENDER_DIST ) );
		
		ASSERT( !(m_setup.m_skyboxMeshResource && !IsSkyboxMeshValid(m_setup.m_skyboxMeshResource)) && "Expected unit skydome mesh" );
		DrawSkyboxChunk( context, m_setup.m_cloudsMeshResource, m_setup.m_cloudsMaterialResource, m_setup.m_cloudsMaterialParamsResource, meshStats );
	}

	// Restore some states
	GpuApi::SetViewport( origViewport );
}
