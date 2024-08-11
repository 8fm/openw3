/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderFlaresBatcher.h"
#include "renderProxyFlare.h"
#include "renderCollector.h"
#include "renderMaterial.h"
#include "renderShaderPair.h"
#include "renderSkybox.h"
#include "renderScene.h"
#include "renderInterface.h"
#include "renderRenderSurfaces.h"
#include "../engine/renderFragment.h"

// ---

Float GetFlareOpacityScale( const CRenderFrameInfo &info, const CRenderProxy_Flare &flare )
{
	const SFlareParameters &flareParams = flare.GetParameters();
	if ( FLARECAT_Default == flareParams.m_category )
	{
		return 1;
	}

	RED_ASSERT( FLARECAT_Sun == flareParams.m_category || FLARECAT_Moon == flareParams.m_category );
	return Clamp( 1.f - (info.m_globalWaterLevelAtCameraPos - info.m_camera.GetPosition().Z) /  0.25f, 0.f, 1.f );
}

// ---

CRenderFlaresBatcher::CRenderFlaresBatcher()
	: m_isInitialized( false )
{
	// empty
}

CRenderFlaresBatcher::~CRenderFlaresBatcher()
{
	// empty
}

void CRenderFlaresBatcher::Initialize()
{
	DebugVertex verts[8];
	Uint16 indices[36];

	//Build cube vertices
	verts[0].Set( Vector (-1,-1,-1), 0xffffffff );
	verts[1].Set( Vector ( 1,-1,-1), 0xffffffff );
	verts[2].Set( Vector ( 1, 1,-1), 0xffffffff );
	verts[3].Set( Vector (-1, 1,-1), 0xffffffff );
	verts[4].Set( Vector (-1,-1, 1), 0xffffffff );
	verts[5].Set( Vector ( 1,-1, 1), 0xffffffff );
	verts[6].Set( Vector ( 1, 1, 1), 0xffffffff );
	verts[7].Set( Vector (-1, 1, 1), 0xffffffff );

	// Build indices
	Uint16 off = 0;
	indices[off++] = 0;	indices[off++] = 1;	indices[off++] = 2;	indices[off++] = 0;	indices[off++] = 2;	indices[off++] = 3;
	indices[off++] = 4;	indices[off++] = 6;	indices[off++] = 5;	indices[off++] = 4;	indices[off++] = 7;	indices[off++] = 6;
	indices[off++] = 4;	indices[off++] = 5;	indices[off++] = 1;	indices[off++] = 4;	indices[off++] = 1;	indices[off++] = 0;
	indices[off++] = 6;	indices[off++] = 7;	indices[off++] = 3;	indices[off++] = 6;	indices[off++] = 3;	indices[off++] = 2;
	indices[off++] = 7;	indices[off++] = 4;	indices[off++] = 0;	indices[off++] = 7;	indices[off++] = 0;	indices[off++] = 3;
	indices[off++] = 5;	indices[off++] = 6;	indices[off++] = 2;	indices[off++] = 5;	indices[off++] = 2;	indices[off++] = 1;

	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = verts;
	m_vertexBuffer = GpuApi::CreateBuffer( 8 * sizeof( DebugVertex ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );

	bufInitData.m_buffer = indices;
	m_indexBuffer = GpuApi::CreateBuffer( 36 * sizeof( Uint16 ), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
}

void CRenderFlaresBatcher::GrabTransparencyHelpers( CRenderCollector& collector, const TDynArray< CRenderProxy_Flare* >& flares, const CRenderFrameInfo& info, const GpuApi::TextureRef &texGrabSource )
{
	RED_ASSERT( texGrabSource );
	if ( flares.Empty() )
	{
		return;
	}

	if( !m_isInitialized )
	{
		Initialize();
		m_isInitialized = true;
	}

	CRenderShaderPair *shader = GetRenderer()->m_occlusionHelperGrab;
	if ( !shader )
	{
		return;
	}	

	// Bind vertices and indices for flare occlusion cube
	Uint32 stride = sizeof(DebugVertex);
	Uint32 offset = 0;
	// Don't actually set the input layout yet, because we don't have the correct shaders bound yet.
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor, false );
	GpuApi::BindVertexBuffers( 0, 1, &m_vertexBuffer, &stride, &offset );
	GpuApi::BindIndexBuffer( m_indexBuffer );
	
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );

	shader->Bind();
	GpuApi::BindTextures( 0, 1, &texGrabSource, GpuApi::PixelShader );

	// Draw flares
	for ( Uint32 flare_i=0; flare_i<flares.Size(); ++flare_i )
	{
		CRenderProxy_Flare *flare = flares[flare_i];
		ASSERT( NULL != flare );
		flare->DrawOcclusionShape( info );
	}

	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
}

void CRenderFlaresBatcher::UpdateOcclusion( CRenderCollector& collector, const TDynArray< CRenderProxy_Flare* >& flares, const CRenderFrameInfo& info, const GpuApi::TextureRef &texTransparencyHelperOpaque, const GpuApi::TextureRef &texTransparencyHelperTransparent )
{
	if ( flares.Empty() || !collector.m_scene )
	{
		return;
	}

	if( !m_isInitialized )
	{
		Initialize();
		m_isInitialized = true;
	}
	
	const Bool useTranspHelpers = texTransparencyHelperOpaque && texTransparencyHelperTransparent;

	// Grab target setup
	Bool shouldRestoreTargetsSetup = false;
	GpuApi::RenderTargetSetup originalTargetsSetup;
	if ( useTranspHelpers )
	{
		// Grab current rtSetup
		originalTargetsSetup = GpuApi::GetRenderTargetSetup();
		shouldRestoreTargetsSetup = true;

		// Unbind colro rendertargets
		GpuApi::RenderTargetSetup rtSetup = originalTargetsSetup;
		rtSetup.SetNullColorTarget();
		GpuApi::SetupRenderTargets( rtSetup );
	}

	// Bind vertices and indices for flare occlusion cube
	Uint32 stride = sizeof(DebugVertex);
	Uint32 offset = 0;
	// Don't actually set the input layout yet, because we don't have the correct shaders bound yet.
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor, false );
	GpuApi::BindVertexBuffers( 0, 1, &m_vertexBuffer, &stride, &offset );
	GpuApi::BindIndexBuffer( m_indexBuffer );

	// Draw shit
	for ( Uint32 pass_i=0; pass_i<2; ++pass_i )
	{
		const Bool isFullPass = (0 == pass_i);

		// Setup draw context for current pass
		CGpuApiScopedDrawContext drawContext( isFullPass ? GpuApi::DRAWCONTEXT_FlaresOcclusionFull : GpuApi::DRAWCONTEXT_FlaresOcclusionPart );

		// Setup shader
		if ( useTranspHelpers && !isFullPass )
		{
			GetRenderer()->GetStateManager().ForceNullPS( false );
			GetRenderer()->m_occlusionTest->Bind();
			
			GpuApi::TextureRef texDissolve = GpuApi::GetInternalTexture( GpuApi::INTERTEX_DissolvePattern );
			GpuApi::TextureRef textures[] = { texTransparencyHelperOpaque, texTransparencyHelperTransparent, texDissolve };
			GpuApi::BindTextures( 0, ARRAY_COUNT(textures), textures, GpuApi::PixelShader );

			GpuApi::TextureLevelDesc dissolveDesc = GpuApi::GetTextureLevelDesc( texDissolve, 0 );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)dissolveDesc.width, (Float)dissolveDesc.height, 0, 0 ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( info.m_worldRenderSettings.m_globalFlaresTransparencyThreshold, Max( 0.001f, info.m_worldRenderSettings.m_globalFlaresTransparencyRange ), 0, 0 ) );
		}
		else
		{
			GetRenderer()->m_occlusionTest->Bind();
			GetRenderer()->GetStateManager().ForceNullPS( true );
		}

		// Draw flares
		for ( Uint32 flare_i=0; flare_i<flares.Size(); ++flare_i )
		{
			CRenderProxy_Flare *flare = flares[flare_i];
			ASSERT( NULL != flare );
			flare->DrawOcclusion( collector.m_scene, isFullPass, info );
		}
	}

	// Restore some states
	GetRenderer()->GetStateManager().ForceNullPS( false );
	GpuApi::BindTextures( 0, 3, nullptr, GpuApi::PixelShader );
	if ( shouldRestoreTargetsSetup )
	{
		GpuApi::SetupRenderTargets( originalTargetsSetup );
	}
}

void CRenderFlaresBatcher::DrawDebugOcclusion( CRenderCollector& collector, const TDynArray< CRenderProxy_Flare* >& flares, const CRenderFrameInfo& info )
{
	if ( flares.Empty() || !collector.m_scene )
	{
		return;
	}

	if( !m_isInitialized )
	{
		Initialize();
		m_isInitialized = true;
	}

	// Bind vertices and indices for flare occlusion cube
	Uint32 stride = sizeof(DebugVertex);
	Uint32 offset = 0;
	// Don't actually set the input layout yet, because we don't have the correct shaders bound yet.
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColor, false );
	GpuApi::BindVertexBuffers( 0, 1, &m_vertexBuffer, &stride, &offset );
	GpuApi::BindIndexBuffer( m_indexBuffer );

	// Bind shader
	GetRenderer()->m_shaderSingleColor->Bind();
	GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, Vector ( 1.f, 1.f, 0.f, 1.f ) );
	
	// Setup draw context
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_Unlit );

	// Draw flares
	for ( Uint32 flare_i=0; flare_i<flares.Size(); ++flare_i )
	{
		CRenderProxy_Flare *flare = flares[flare_i];
		ASSERT( NULL != flare );
		flare->DrawOcclusionShape( info );
	}
}

void CRenderFlaresBatcher::DrawFlares( CRenderCollector& collector, const RenderingContext& context, const TDynArray< CRenderProxy_Flare* >& flares )
{	
	if ( flares.Empty() || !collector.m_scene )
	{
		return;
	}

	// Setup draw context
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_FlaresDraw );

	// Reset localToWorld matrix
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	// Build verts
	GpuApi::SystemVertex_ParticleStandard verts[4];
	verts[0].m_uv[0] = 0.f;
	verts[0].m_uv[1] = 0.f;
	verts[1].m_uv[0] = 1.f;
	verts[1].m_uv[1] = 0.f;
	verts[2].m_uv[0] = 1.f;
	verts[2].m_uv[1] = 1.f;
	verts[3].m_uv[0] = 0.f;
	verts[3].m_uv[1] = 1.f;
	for ( Uint32 i=0; i<4; ++i )
	{
		verts[i].m_rotation[0] = 0.f;
		verts[i].m_rotation[1] = 1.f;
		verts[i].m_frame = 0;
		verts[i].m_color = 0xffffffff;

		// Updated later per flare
		// verts[i].m_position[0] = position.X;
		// verts[i].m_position[1] = position.Y;
		// verts[i].m_position[2] = position.Z;
		// verts[i].m_size[0] = (2.f * verts[i].m_uv[0] - 1.f) * radius;
		// verts[i].m_size[1] = (2.f * verts[i].m_uv[1] - 1.f) * radius;
	}

	// Build indices
	const Uint16 indices[6] = { 0, 1, 2, 0, 3, 2 };

	// Sort flares
	m_sortTable.Resize( flares.Size() );
	for ( Uint32 i=0; i<flares.Size(); ++i )
	{
		m_sortTable[i].sortKey = Vector::Dot3( collector.GetRenderCamera().GetCameraForward(), flares[i]->GetLocalToWorld().GetTranslation() - collector.GetRenderCamera().GetPosition() );
		m_sortTable[i].flare = flares[i];
	}
	Sort( m_sortTable.Begin(), m_sortTable.End(), FlareSortPred() );
	
	// Draw flares
	for ( Uint32 sorted_flare_i=0; sorted_flare_i<m_sortTable.Size(); ++sorted_flare_i )
	{
		CRenderProxy_Flare *flare = m_sortTable[sorted_flare_i].flare;
		ASSERT( NULL != flare );

		// If alpha zero then skip flare
		if ( flare->GetCurrentAlpha() <= 0.0f )
		{
			continue;
		}

		// If flare size < 0 - skip shit
		if ( flare->GetParameters().m_flareRadius * flare->GetLocalToWorld().GetScale33().Mag3() < 0.0f )
		{
			continue;
		}

		if ( !context.CheckLightChannels( flare->GetLightChannels() ) )
		{
			// Light channel doesn't match filter
			continue;
		}

		// Calculate final alpha
		Float finalAlpha = flare->GetCurrentAlpha() * collector.GetRenderFrameInfo().m_envParametersArea.GetFlareOpacity( flare->GetParameters().m_colorGroup, 0 ) * GetFlareOpacityScale( collector.GetRenderFrameInfo(), *flare );
		if ( finalAlpha <= 0 )
		{
			continue;
		}

		// Some settings
		CRenderMaterial *material = flare->GetMaterial();
		CRenderMaterialParameters *materialParams = flare->GetMaterialParameters();
		if ( !material || !materialParams )
		{
			continue;
		}

		// Settings
		const EMaterialVertexFactory vertexFactory = MVF_ParticleBilboard;

		// Prepare material context
		MaterialRenderingContext materialContext( context );
		materialContext.m_vertexFactory = vertexFactory;
		materialContext.m_selected = flare->IsSelected();
		materialContext.m_discardingPass = materialParams->IsMasked();

		GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, Vector( materialParams->IsMasked(), 0, 0, 0 ) );

		// Select material and bind parameters, the distance is 0 because the flares are screen space
		if ( !material->Bind( materialContext, materialParams, 0.f ) )
		{
			continue;
		}

		// Calculate final color
		Vector finalColor = collector.GetRenderFrameInfo().m_envParametersArea.GetFlareColor( flare->GetParameters().m_colorGroup, 0 );

		// Setup material parameters
		if(flare->GetEffectParams() != NULL)
		{
			// Parameter 0
			if ( flare->GetMaterial()->IsUsingEffectParam0() )
			{	
				if( flare->GetEffectParams()->m_customParam0.X <= 0.0f )
				{
					// intensity of the flare is zero or below - skip it
					continue;
				}

				finalColor *= flare->GetEffectParams()->m_customParam0.X;

				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, finalColor );
			}

			// Parameter 1
			if ( flare->GetMaterial()->IsUsingEffectParam1() )
			{
				if( flare->GetEffectParams()->m_customParam1.X <= 0.0f )
				{
					// intensity of the flare is zero or below - skip it
					continue;
				}

				finalColor *= flare->GetEffectParams()->m_customParam1.X;

				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, finalColor );
			}
		}
		else
		{
			if ( flare->GetMaterial()->IsUsingEffectParam0() )
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, finalColor );
			}

			if ( flare->GetMaterial()->IsUsingEffectParam1() )
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, finalColor );
			}
		}
		
		// Setup transparency
		{
			GetRenderer()->GetStateManager().SetPixelConst( PSC_TransparencyParams, Vector ( 1.f, 1.f, 1.f, finalAlpha ) );
		}

		// Setup vertices			
		{
			const Vector position = CRenderProxy_Flare::CalculateFlareSourcePosition( *flare, collector.GetRenderFrameInfo() );

			const Float radius = flare->GetParameters().m_flareRadius * CRenderProxy_Flare::GetFlareCategorySize( flare->GetParameters().m_category, collector.GetRenderFrameInfo() );
			
			for ( Uint32 i=0; i<4; ++i )
			{
				verts[i].m_position[0] = position.X;
				verts[i].m_position[1] = position.Y;
				verts[i].m_position[2] = position.Z;
				verts[i].m_size[0] = (2.f * verts[i].m_uv[0] - 1.f) * radius;
				verts[i].m_size[1] = (2.f * verts[i].m_uv[1] - 1.f) * radius;
			}
		}

		// Draw flare
		GpuApi::DrawSystemIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 4, 2, indices, GpuApi::BCT_VertexSystemParticleStandard, verts );
	}

	// Restore default transparency params
	GetRenderer()->GetStateManager().SetPixelConst( PSC_TransparencyParams, Vector::ONES );
}

void CRenderFlaresBatcher::DrawLensFlares( CRenderCollector& collector, const RenderingContext& context, const TDynArray< CRenderProxy_Flare* >& flares )
{
	if ( flares.Empty() || !collector.m_scene )
	{
		return;
	}

	// Setup draw context
	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_FlaresDraw );

	// Reset localToWorld matrix
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	// Build verts
	GpuApi::SystemVertex_ParticleStandard verts[4];
	verts[0].m_uv[0] = 0.f;
	verts[0].m_uv[1] = 0.f;
	verts[1].m_uv[0] = 1.f;
	verts[1].m_uv[1] = 0.f;
	verts[2].m_uv[0] = 1.f;
	verts[2].m_uv[1] = 1.f;
	verts[3].m_uv[0] = 0.f;
	verts[3].m_uv[1] = 1.f;
	for ( Uint32 i=0; i<4; ++i )
	{
		verts[i].m_rotation[0] = 0.f;
		verts[i].m_rotation[1] = 1.f;
		verts[i].m_frame = 0;
		verts[i].m_color = 0xffffffff;

		// Updated later per flare
		// verts[i].m_position[0] = position.X;
		// verts[i].m_position[1] = position.Y;
		// verts[i].m_position[2] = position.Z;
		// verts[i].m_size[0] = (2.f * verts[i].m_uv[0] - 1.f) * radius;
		// verts[i].m_size[1] = (2.f * verts[i].m_uv[1] - 1.f) * radius;
	}

	// Build indices
	const Uint16 indices[6] = { 0, 1, 2, 0, 3, 2 };

	// Sort flares
	m_sortTable.ClearFast();
	m_sortTable.Reserve( flares.Size() );
	for ( Uint32 i=0; i<flares.Size(); ++i )
	{
		const SLensFlareSetupParameters &params = collector.m_scene->GetLensFlareGroupsParameters().m_groups[ flares[i]->GetParameters().m_lensFlareGroup ];
		if ( !params.m_elements.Empty() )
		{	
			m_sortTable.PushBack( SFlareSortData () );
			m_sortTable.Back().sortKey = Vector::Dot3( collector.GetRenderCamera().GetCameraForward(), flares[i]->GetLocalToWorld().GetTranslation() - collector.GetRenderCamera().GetPosition() );
			m_sortTable.Back().flare = flares[i];
		}
	}
	Sort( m_sortTable.Begin(), m_sortTable.End(), FlareSortPred() );
	
	// 
	const Float tanHalfCameraFOV = tanf( DEG2RAD( 0.5f * collector.GetRenderFrameInfo().m_camera.GetFOV() ) );

	// Draw flares
	for ( Uint32 sorted_flare_i=0; sorted_flare_i<m_sortTable.Size(); ++sorted_flare_i )
	{
		CRenderProxy_Flare *flare = m_sortTable[sorted_flare_i].flare;
		const SLensFlareSetupParameters &lensSetup = collector.m_scene->GetLensFlareGroupsParameters().m_groups[ flare->GetParameters().m_lensFlareGroup ];

		// If alpha zero then skip flare
		if ( flare->GetCurrentAlpha() <= 0.f )
		{
			continue;
		}

		// Light channel doesn't match filter
		if ( !context.CheckLightChannels( flare->GetLightChannels() ) )
		{
			continue;
		}

		// Calculate flare source position
		const Vector flareSourcePosition = CRenderProxy_Flare::CalculateFlareSourcePosition( *flare, collector.GetRenderFrameInfo() );		
		const Float flareSourceCameraForwardDist = collector.GetRenderFrameInfo().m_camera.GetCameraForward().Dot3( flareSourcePosition - collector.GetRenderFrameInfo().m_camera.GetPosition() );
		if ( flareSourceCameraForwardDist <= collector.GetRenderFrameInfo().m_camera.GetNearPlane() )
		{
			continue;
		}
		Vector flareSourcePositionProj = collector.GetRenderFrameInfo().m_camera.GetWorldToScreen().TransformVectorWithW( flareSourcePosition );
		flareSourcePositionProj /= flareSourcePositionProj.W;
		const Float flareSourceCameraDist = collector.GetRenderFrameInfo().m_camera.GetPosition().DistanceTo( flareSourcePosition );

		// Calculate alpha
		Float flareSourceAlpha = flare->GetCurrentAlpha();
		{
			flareSourceAlpha *= Clamp( (Min( flareSourceCameraDist, flareSourceCameraForwardDist ) - (collector.GetRenderFrameInfo().m_camera.GetNearPlane() + 0.05f)) / 0.25f, 0.f, 1.f );
			flareSourceAlpha *= Clamp( (1.f - Max( Abs( flareSourcePositionProj.X ), Abs( flareSourcePositionProj.Y ) )) / 0.1f, 0.f, 1.f );
			flareSourceAlpha *= GetFlareOpacityScale( collector.GetRenderFrameInfo(), *flare );

			if ( !(FLARECAT_Sun == flare->GetParameters().m_category || FLARECAT_Moon == flare->GetParameters().m_category) )
			{
				flareSourceAlpha *= Clamp( (flareSourceCameraDist - lensSetup.m_nearDistance) * lensSetup.m_nearInvRange, 0.f, 1.f );
				flareSourceAlpha *= Clamp( (lensSetup.m_farDistance - flareSourceCameraDist) * lensSetup.m_farInvRange, 0.f, 1.f );
			}

			if ( flareSourceAlpha <= 0 )
			{
				continue;
			}
		}

		// Draw flare elements
		for ( Uint32 element_i=0; element_i<lensSetup.m_elements.Size(); ++element_i )
		{
			const SLensFlareElementSetupParameters &lensElement = lensSetup.m_elements[element_i];
			
			// Skip if radius invisible
			if ( lensElement.m_size <= 0 )
			{
				continue;
			}

			// Get material
			CRenderMaterial *material = static_cast<CRenderMaterial*>( lensElement.m_materialResource );
			CRenderMaterialParameters *materialParams = static_cast<CRenderMaterialParameters*>( lensElement.m_materialParamsResource );
			if ( !material || !materialParams )
			{
				continue;
			}

			// Prepare material context
			MaterialRenderingContext materialContext( context );
			materialContext.m_vertexFactory = MVF_ParticleBilboard;
			materialContext.m_selected = flare->IsSelected();
			materialContext.m_discardingPass = materialParams->IsMasked();

			GetRenderer()->GetStateManager().SetPixelConst( PSC_DiscardFlags, Vector( materialParams->IsMasked(), 0, 0, 0 ) );

			// Select material and bind parameters, the distance is 0 because the flares are screen space
			if ( !material->Bind( materialContext, materialParams, 0.f ) )
			{
				continue;
			}

			// Calc final alpha
			Float finalAlpha = Clamp( flareSourceAlpha * lensElement.m_alpha, 0.f, 1.f ) * collector.GetRenderFrameInfo().m_envParametersArea.GetFlareOpacity( flare->GetParameters().m_colorGroup, lensElement.m_colorGroupParamsIndex );
			{
				if ( lensElement.m_centerFadeRange > 0 )
				{
					finalAlpha *= Clamp( (sqrtf( flareSourcePositionProj.X * flareSourcePositionProj.X + flareSourcePositionProj.Y * flareSourcePositionProj.Y ) - lensElement.m_centerFadeStart) / Max( 0.001f, lensElement.m_centerFadeRange ), 0.f, 1.f );
				}

				if ( finalAlpha <= 0 )
				{
					continue;
				}
			}

			// Calc final color
			Vector finalColor = lensElement.m_colorLinear * collector.GetRenderFrameInfo().m_envParametersArea.GetFlareColor( flare->GetParameters().m_colorGroup, lensElement.m_colorGroupParamsIndex );

			// Setup material parameters
			if(flare->GetEffectParams() != NULL)
			{
				// Parameter 0
				if ( material->IsUsingEffectParam0() )
				{
					if( flare->GetEffectParams()->m_customParam0.X <= 0.0f )
					{
						// intensity of the flare is zero or below - skip it
						continue;
					}

					finalColor *= flare->GetEffectParams()->m_customParam0.X;

					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, finalColor );
				}

				// Parameter 1
				if ( material->IsUsingEffectParam1() )
				{
					if( flare->GetEffectParams()->m_customParam1.X <= 0.0f )
					{
						// intensity of the flare is zero or below - skip it
						continue;
					}

					finalColor *= flare->GetEffectParams()->m_customParam1.X;

					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, Vector::ONES );
				}
			}
			else
			{
				if ( material->IsUsingEffectParam0() )
				{
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom0, finalColor );
				}

				if ( material->IsUsingEffectParam1() )
				{
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom1, Vector::ONES );			
				}
			}

			// Setup transparency
			{
				GetRenderer()->GetStateManager().SetPixelConst( PSC_TransparencyParams, Vector ( 1.f, 1.f, 1.f, finalAlpha ) );
			}
			
			// Setup vertices
			{
				const Float elementSize = lensElement.m_size * (lensElement.m_isConstRadius ? tanHalfCameraFOV * flareSourceCameraForwardDist : flare->GetParameters().m_flareRadius * CRenderProxy_Flare::GetFlareCategorySize( flare->GetParameters().m_category, collector.GetRenderFrameInfo() ));
				
				Float rotation0 = 0.f;
				Float rotation1 = 1.f;
				if ( lensElement.m_isAligned )
				{
					rotation0 = -flareSourcePositionProj.X * collector.GetRenderFrameInfo().m_width;
					rotation1 = flareSourcePositionProj.Y * collector.GetRenderFrameInfo().m_height;
					Float invLen = 1.f / Max( 0.00001f, sqrtf( rotation0 * rotation0 + rotation1 * rotation1 ) );
					rotation0 *= invLen;
					rotation1 *= invLen;
				}
				
				Vector finalPosition;
				{
					finalPosition = flareSourcePositionProj;

					finalPosition.X *= -lensElement.m_shift;
					finalPosition.Y *= -lensElement.m_shift;

					if ( 0 != lensElement.m_pivot )
					{
						const Float displace = lensElement.m_pivot * elementSize / (flareSourceCameraForwardDist * tanHalfCameraFOV);
						if ( lensElement.m_isAligned )
						{
							finalPosition.X -= -rotation0 * displace / collector.GetRenderFrameInfo().m_camera.GetAspect();
							finalPosition.Y -= rotation1 * displace;
						}
						else
						{
							finalPosition.Y -= displace;
						}
					}

					finalPosition = collector.GetRenderFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( finalPosition );
					finalPosition /= finalPosition.W;
				}

				for ( Uint32 i=0; i<4; ++i )
				{
					verts[i].m_position[0] = finalPosition.X;
					verts[i].m_position[1] = finalPosition.Y;
					verts[i].m_position[2] = finalPosition.Z;
					verts[i].m_size[0] = (2.f * verts[i].m_uv[0] - 1.f) * elementSize * lensElement.m_aspect;
					verts[i].m_size[1] = (2.f * verts[i].m_uv[1] - 1.f) * elementSize;
					verts[i].m_rotation[0] = rotation0;
					verts[i].m_rotation[1] = rotation1;
				}
			}

			// Draw flare
			GpuApi::DrawSystemIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 4, 2, indices, GpuApi::BCT_VertexSystemParticleStandard, verts );
		}
	}

	// Restore default transparency params
	GetRenderer()->GetStateManager().SetPixelConst( PSC_TransparencyParams, Vector::ONES );
}

void CRenderFlaresBatcher::OnDeviceLost()
{
	// Do nothing
}

void CRenderFlaresBatcher::OnDeviceReset()
{
}

CName CRenderFlaresBatcher::GetCategory() const
{
	return CNAME( RenderFlaresBatcher );
}

Uint32 CRenderFlaresBatcher::GetUsedVideoMemory() const
{
	Uint32 size = 0;
	return size;
}
