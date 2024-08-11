#include "build.h"
#include "renderTextureStreamRequest.h"

#include "meshTypeComponent.h"
#include "meshTypeResource.h"
#include "material.h"
#include "appearanceComponent.h"
#include "furComponent.h"
#include "furMeshResource.h"
#include "bitmapTexture.h"
#include "morphedMeshComponent.h"
#include "mesh.h"


void IRenderTextureStreamRequest::AddMaterial( IMaterial* material )
{
	if ( material == nullptr )
	{
		return;
	}

	TDynArray< IRenderResource* > texs;
	material->GatherTextureRenderResources( texs );
	for ( auto tex : texs )
	{
		AddRenderTexture( tex );
	}
}

void IRenderTextureStreamRequest::AddEntity( const CEntity* entity, Bool includeAppearance /*= true*/ )
{
	if ( entity == nullptr )
	{
		return;
	}

	CAppearanceComponent* appComp = CAppearanceComponent::GetAppearanceComponent( entity );
	THashSet< CComponent* > componentsInAppearance;
	if ( !includeAppearance && appComp != nullptr )
	{
		TDynArray< CComponent* > comps;
		appComp->GetCurrentAppearanceComponents( comps );
		for ( CComponent* comp : comps )
		{
			componentsInAppearance.Insert( comp );
		}
	}


	for ( ComponentIterator< CDrawableComponent > it( entity ); it; ++it )
	{
		CDrawableComponent* component = *it;

		// Check if this is part of the appearance.
		if ( !includeAppearance && componentsInAppearance.Exist( component ) )
		{
			continue;
		}

		// Special case for morphed mesh, need to get materials from source and target mesh.
		if ( CMorphedMeshComponent* morphedMesh = Cast< CMorphedMeshComponent >( component ) )
		{
			if ( CMesh* source = morphedMesh->GetMorphSource() )
			{
				AddRenderMesh( source->GetRenderResource() );
				for ( IMaterial* mtl : source->GetMaterials() )
				{
					AddMaterial( mtl );
				}
			}
			if ( CMesh* target = morphedMesh->GetMorphTarget() )
			{
				AddRenderMesh( target->GetRenderResource() );
				for ( IMaterial* mtl : target->GetMaterials() )
				{
					AddMaterial( mtl );
				}
			}
		}
		else if ( CMeshTypeComponent* mtc = Cast< CMeshTypeComponent >( component ) )
		{
			if ( CMeshTypeResource* mtr = mtc->GetMeshTypeResource() )
			{
				// Special case for fur, since it doesn't use normal materials.
				if ( CFurMeshResource* furResource = Cast< CFurMeshResource >( mtr ) )
				{
					CBitmapTexture* textures[] = {
						furResource->m_physicalMaterials.m_volume.m_densityTex.Get(),
						furResource->m_physicalMaterials.m_volume.m_lengthTex.Get(),
						furResource->m_graphicalMaterials.m_color.m_rootColorTex.Get(),
						furResource->m_graphicalMaterials.m_color.m_tipColorTex.Get(),
						furResource->m_graphicalMaterials.m_color.m_strandTex.Get(),
						furResource->m_graphicalMaterials.m_specular.m_specularTex.Get(),
						furResource->m_physicalMaterials.m_strandWidth.m_rootWidthTex.Get(),
						furResource->m_physicalMaterials.m_strandWidth.m_tipWidthTex.Get(),
						furResource->m_physicalMaterials.m_stiffness.m_stiffnessTex.Get(),
						furResource->m_physicalMaterials.m_stiffness.m_rootStiffnessTex.Get(),
						furResource->m_physicalMaterials.m_clumping.m_clumpScaleTex.Get(),
						furResource->m_physicalMaterials.m_clumping.m_clumpRoundnessTex.Get(),
						furResource->m_physicalMaterials.m_clumping.m_clumpNoiseTex.Get(),
						furResource->m_physicalMaterials.m_waveness.m_waveScaleTex.Get(),
						furResource->m_physicalMaterials.m_waveness.m_waveFreqTex.Get(),
					};
					for ( CBitmapTexture* tex : textures )
					{
						if ( tex != nullptr )
						{
							AddRenderTexture( tex->GetRenderResource() );
						}
					}
				}
				else
				{
					for ( IMaterial* mtl : mtr->GetMaterials() )
					{
						AddMaterial( mtl );
					}
				}

				if ( CMesh* mesh = Cast< CMesh >( mtr ) )
				{
					AddRenderMesh( mesh->GetRenderResource() );
				}
			}
		}
	}

	entity->PropagateCallToItemEntities( [this, includeAppearance]( CEntity* childEnt ) {
		AddEntity( childEnt, includeAppearance );
	} );
}

void IRenderTextureStreamRequest::AddEntityTemplate( CEntityTemplate* templ )
{
	if ( templ == nullptr )
	{
		return;
	}

	// We just care about the template itself, not any default appearance.
	AddEntity( templ->GetEntityObject(), false );
}

void IRenderTextureStreamRequest::AddEntityAppearance( CEntityTemplate* templ, CName appearance )
{
	if ( templ == nullptr )
	{
		return;
	}
	const CEntityAppearance* app = templ->GetAppearance( appearance, true );
	if ( app == nullptr )
	{
		return;
	}

	for ( CEntityTemplate* templ : app->GetIncludedTemplates() )
	{
		AddEntityTemplate( templ );
	}
}
