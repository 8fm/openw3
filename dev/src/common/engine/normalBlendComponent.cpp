/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "normalBlendComponent.h"
#include "normalBlendAttachment.h"
#include "normalBlendDataSource.h"
#include "materialDefinition.h"
#include "materialInstance.h"
#include "world.h"
#include "tickManager.h"
#include "entity.h"
#include "texture.h"
#include "../core/dataError.h"

// Size of buffer to use to copy material parameters, when mapping replaced materials to the normal-blend
// material. A 4-element vector, currently the biggest type, would require a 16-byte buffer. We use a
// bigger one to allow for possible expansion. 64 bytes would allow for a matrix parameter. Since it's
// allocated on the stack, and not initialized, the size shouldn't really matter much.
#define COPY_PARAMETER_BUFFER_SIZE 64

IMPLEMENT_ENGINE_CLASS( CNormalBlendComponent );

CNormalBlendComponent::CNormalBlendComponent()
	: m_isConnectedToMesh( false )
	, m_normalBlendMaterial( NULL )
	, m_sourceMaterial( NULL )
	, m_sourceNormalTexture( NULL )
	, m_dataSource( NULL )
	, m_useMainTick( false )
#ifndef NO_EDITOR
	, m_shouldRemapParameters( false )
#endif
{
	m_normalBlendAreas.Resize( NUM_NORMALBLEND_AREAS );
	for ( Int32 i = 0; i < NUM_NORMALBLEND_AREAS; ++i )
	{
		m_normalBlendAreas[i] = Vector( 0, 0, 1, 1 );
	}
}

CNormalBlendComponent::~CNormalBlendComponent()
{
}


Bool CNormalBlendComponent::HasCachedNBAttachment() const
{
	return m_isConnectedToMesh;
}

CNormalBlendAttachment* CNormalBlendComponent::GetCachedNBAttachment()
{
	return HasCachedNBAttachment() ? Cast< CNormalBlendAttachment >( GetChildAttachments().Front() ) : NULL;
}

Bool CNormalBlendComponent::CheckCanCacheNBAttachment() const
{
	const TList< IAttachment* >& childAttachments = GetChildAttachments();
	return !childAttachments.Empty() && childAttachments.Front()->IsA< CNormalBlendAttachment >();
}


void CNormalBlendComponent::CacheNBAttachment()
{
	ASSERT( !m_isConnectedToMesh );
	if ( m_isConnectedToMesh )
	{
		ResetCachedNBAttachment();
	}

	m_isConnectedToMesh = true;

	CNormalBlendAttachment* attachment = GetCachedNBAttachment();
	ASSERT( attachment );

	if ( !attachment )
	{
		m_isConnectedToMesh = false;
		return;
	}

#ifndef NO_EDITOR
	// Find a mapping between the replaced materials and our normal-blend material, fill in as many parameters as we can.
	MapMaterialParameters();
#endif
	attachment->SetNormalBlendMaterial( m_normalBlendMaterial.Get(), m_sourceMaterial.Get(), m_sourceNormalTexture.Get() );
	attachment->SetNormalBlendAreas( 0, NUM_NORMALBLEND_AREAS, m_normalBlendAreas.TypedData() );
	ClearWeights();
}

void CNormalBlendComponent::ResetCachedNBAttachment()
{
	ASSERT( m_isConnectedToMesh );
	if ( !m_isConnectedToMesh ) return;

	CNormalBlendAttachment* attachment = GetCachedNBAttachment();
	if ( attachment )
	{
		// Clear out any existing normal-blend material.
		attachment->SetNormalBlendMaterial( NULL, m_sourceMaterial.Get(), m_sourceNormalTexture.Get() );
	}

	m_isConnectedToMesh = false;
}

void CNormalBlendComponent::OnPropertyPreChange( IProperty* property )
{
	TBaseClass::OnPropertyPreChange( property );
	
	CNormalBlendAttachment* attachment = GetCachedNBAttachment();

	// When changing the source texture/material, clear out the existing normalblend material first. This should unset it on any
	// parts where it should no longer be applied.
	if ( property->GetName() == TXT("sourceNormalTexture") )
	{
		if ( attachment )
		{
			attachment->SetNormalBlendMaterial( NULL, m_sourceMaterial.Get(), m_sourceNormalTexture.Get() );
		}
	}
	else if ( property->GetName() == TXT("sourceMaterial") )
	{
		if ( attachment )
		{
			attachment->SetNormalBlendMaterial( NULL, m_sourceMaterial.Get(), m_sourceNormalTexture.Get() );
		}
	}
}

void CNormalBlendComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	
	CNormalBlendAttachment* attachment = GetCachedNBAttachment();

	if ( property->GetName() == TXT("normalBlendMaterial") )
	{
		if ( attachment )
		{
#ifndef NO_EDITOR
			// HACK: When a material or material property is changed in the editor, it triggers all render proxies to be
			// re-created (CDrawableComponent::RecreateProxiesOfRenderableComponents() is called). This happens after we
			// exit this function, so anything we do here will basically be replaced and need to be redone shortly after.
			//
			// We also can't map material parameters here, because the CMaterialInstance is informed of the property
			// change after us, and setting the baseMaterial will cause it to reset all parameters... which we would have
			// already mapped...
			//
			// Soo... we just set a flag to say we want to remap material parameters when the proxies are re-created, and
			// let the rest happen when it happens...
			m_shouldRemapParameters = true;
#endif
		}
	}
	// When the source normal texture or source material have been changed, we need to reapply our normal blend material,
	// using the new filter. Since we already cleared out the old setting in OnPropertyPreChange(), we don't have to
	// worry about extra materials being replaced.
	else if ( property->GetName() == TXT("sourceNormalTexture") || property->GetName() == TXT("sourceMaterial") )
	{
		if ( attachment )
		{
			attachment->SetNormalBlendMaterial( m_normalBlendMaterial.Get(), m_sourceMaterial.Get(), m_sourceNormalTexture.Get() );
		}
	}
	else if ( property->GetName() == TXT("normalBlendAreas") )
	{
		if ( attachment )
		{
			attachment->SetNormalBlendAreas( 0, NUM_NORMALBLEND_AREAS, m_normalBlendAreas.TypedData() );
		}
	}
	else if ( property->GetName() == TXT("dataSource") )
	{
		if ( !m_dataSource && attachment )
		{
			// We no longer have a data source, so clear all weights.
			ClearWeights();
		}
	}
}

#ifndef NO_EDITOR

void CNormalBlendComponent::MapMaterialParameters()
{
	CNormalBlendAttachment* attachment = GetCachedNBAttachment();

	// Can't map anything if we aren't attached to some proxies, or don't have a NB material!
	if ( !attachment || !m_normalBlendMaterial || !m_normalBlendMaterial->GetMaterialDefinition() ) return;

	// Out of the materials used by the attached proxies, we want to find only the ones that are being replaced by our normal-blend material.
	// For each of them, we will look for parameters that match (name and type) the parameters in the normal-blend material, and pull values
	// so artists don't have to duplicate them manually. Ideally, this will make the normal-blend material match the material(s) it is
	// replacing. We don't touch any parameters that have been already set (don't want to overwrite what's already been done).

	//
	// Build a map of the un-written parameters in the normal-blend material, to an array of matching parameters in the replaced materials.
	//
	typedef TPair< IMaterial*, IMaterialDefinition::Parameter > MaterialParamPair;
	typedef TPair< Uint8, TDynArray< MaterialParamPair > > ParamMapPair;
	THashMap< CName, ParamMapPair > paramMap;

	// Step 1: Find un-written NB parameters.
	const IMaterialDefinition::TParameterArray& nbParams = m_normalBlendMaterial->GetMaterialDefinition()->GetPixelParameters();
	for ( Uint32 i = 0; i < nbParams.Size(); ++i )
	{
		const IMaterialDefinition::Parameter& param = nbParams[i];
		// If this parameter is not instanced yet (hasn't been explicitly set), we'll add it to our map to try to match it.
		if ( !m_normalBlendMaterial->IsParameterInstanced( param.m_name ) )
		{
			// Just start out with an empty array, it'll get filled in the next step.
			paramMap.Insert( param.m_name, ParamMapPair( param.m_type, TDynArray< MaterialParamPair >() ) );
		}
	}


	// Step 2: Find matching parameters in the replaced materials, add them to our map.
	CMeshTypeResource::TMaterials destMaterials;
	attachment->GetMaterials( destMaterials );
	for ( Uint32 i = 0; i < destMaterials.Size(); ++i )
	{
		if ( destMaterials[i]->IsA< CMaterialInstance >() )
		{
			CMaterialInstance* mtl = Cast< CMaterialInstance >( destMaterials[i].Get() );

			//
			// Check if this material should be replaced.
			//

			// If we've been given a source base material, and it doesn't match this material, it isn't replaced.
			if ( m_sourceMaterial && mtl->GetBaseMaterial() != m_sourceMaterial.Get() )
			{
				continue;
			}

			if ( mtl->GetMaterialDefinition() == nullptr )
			{
				DATA_HALT( DES_Major, GetEntity()->GetEntityTemplate(), TXT("Rendering"), TXT("Material '%ls' in normal blend target '%ls' has no MaterialDefinition"), mtl->GetFriendlyName().AsChar(), attachment->GetChild()->GetName().AsChar() );
				continue;
			}

			const IMaterialDefinition::TParameterArray& destParams = mtl->GetMaterialDefinition()->GetPixelParameters();

			// If we've been given a source normal texture, and it isn't used by this material, it isn't replaced.
			if ( m_sourceNormalTexture )
			{
				Bool isNormTexUsed = false;

				// This is a bit unfortunate. We need to scan through the textures used by this material, to see if any match.
				for ( Uint32 p = 0; p < destParams.Size(); ++p )
				{
					const IMaterialDefinition::Parameter& destParam = destParams[p];
					if ( destParam.m_type == IMaterialDefinition::PT_Texture )
					{
						THandle< ITexture > tex;
						if ( mtl->ReadParameter( destParam.m_name, tex ) )
						{
							if ( tex.Get() == m_sourceNormalTexture.Get() )
							{
								isNormTexUsed = true;
								break;
							}
						}
					}
				}
				// If this material is not using our source normal texture, then it should not be replaced.
				if ( !isNormTexUsed )
				{
					continue;
				}
			}

			// Find this material's parameters in our parameter map, see if any match.
			for ( Uint32 p = 0; p < destParams.Size(); ++p )
			{
				const IMaterialDefinition::Parameter& destParam = destParams[p];

				// If the name and type match something from the NB material, add this param to the map.
				ParamMapPair* mapPair = paramMap.FindPtr( destParam.m_name );
				if ( mapPair && mapPair->m_first == destParam.m_type )
				{
					mapPair->m_second.PushBack( MaterialParamPair( mtl, destParam ) );
				}
			}
		}
	}


	// Step 3: Now that we've found all parameter matches, we can set some values on the normal-blend material!
	Uint8 copyBuffer[COPY_PARAMETER_BUFFER_SIZE];
	typedef THashMap< CName, ParamMapPair >::iterator Iterator;
	for ( Iterator paramIter = paramMap.Begin(); paramIter != paramMap.End(); ++paramIter )
	{
		TDynArray< MaterialParamPair >& matchedParams = paramIter->m_second.m_second;
		// If we had any matches...
		if ( matchedParams.Size() > 0 )
		{
			const CName& paramName = paramIter->m_first;
			IMaterial* copyFromMaterial = matchedParams[0].m_first;

			// Copy the parameter value.
			// TODO: Maybe only copy if all matches have the same value?
			Red::MemoryZero( copyBuffer, sizeof(copyBuffer) );
			VERIFY( copyFromMaterial->ReadParameterRaw( paramName, copyBuffer ) );
			VERIFY( m_normalBlendMaterial->WriteParameterRaw( paramName, copyBuffer ) );
		}
	}
}

#endif


void CNormalBlendComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CNormalBlendComponent_OnAttached );
	
	// It would appear that when instancing an entity template, it's possible that OnChildAttachmentAdded() might not be called.
	// So we'll do a quick check for attachments here too.
	if ( !HasCachedNBAttachment() && CheckCanCacheNBAttachment() )
	{
		CacheNBAttachment();
	}

	if ( m_useMainTick )
	{
		world->GetTickManager()->AddToGroup( this, TICK_PostUpdateTransform );
	}
}

void CNormalBlendComponent::OnDetached( CWorld* world )
{
	if ( m_useMainTick )
	{
		world->GetTickManager()->RemoveFromGroup( this, TICK_PostUpdateTransform );
	}

	if ( HasCachedNBAttachment() )
	{
		ResetCachedNBAttachment();
	}

	TBaseClass::OnDetached( world );
}


void CNormalBlendComponent::OnChildAttachmentAdded( IAttachment* attachment )
{
	TBaseClass::OnChildAttachmentAdded( attachment );

	if ( attachment->IsA< CNormalBlendAttachment >() )
	{
		if ( !HasCachedNBAttachment() )
		{
			CacheNBAttachment();
		}
	}
}

void CNormalBlendComponent::OnChildAttachmentBroken( IAttachment* attachment )
{
	if ( attachment->IsA< CNormalBlendAttachment >() )
	{
		if ( attachment == GetCachedNBAttachment() )
		{
			ResetCachedNBAttachment();
		}
	}

	TBaseClass::OnChildAttachmentBroken( attachment );

	// If we have another NB Attachment, cache it!
	if ( !HasCachedNBAttachment() && CheckCanCacheNBAttachment() )
	{
		CacheNBAttachment();
	}
}

void CNormalBlendComponent::OnAppearanceChanged( Bool added )
{
	if ( added )
	{
		// Remove any previous attachment
		if ( HasCachedNBAttachment() )
		{
			ResetCachedNBAttachment();
		}

		// Cache new attachment
		if ( CheckCanCacheNBAttachment() )
		{
			CacheNBAttachment();
		}
	}
}

void CNormalBlendComponent::OnStreamIn()
{
	if ( IsUsedInAppearance() )
	{
		// Remove any previous attachment
		if ( HasCachedNBAttachment() )
		{
			ResetCachedNBAttachment();
		}

		// Cache new attachment
		if ( CheckCanCacheNBAttachment() )
		{
			CacheNBAttachment();
		}
	}
}

void CNormalBlendComponent::UpdateWeights()

{
	CNormalBlendAttachment* attachment = GetCachedNBAttachment();
	if ( attachment )
	{
		if(m_dataSource)
		{
			// If DataSource supports an internal weight buffer, we can use that directly.
			const Float* weights = m_dataSource->GetWeights();
			attachment->SetNormalBlendWeights( 0, NUM_NORMALBLEND_AREAS, weights );
		}
		else
		{
			ClearWeights();
		}
	}
}

void CNormalBlendComponent::ClearWeights()

{
	CNormalBlendAttachment* attachment = GetCachedNBAttachment();
	if ( attachment )
	{
		// Setting all blend weights to zero
		Float zeroWeights[NUM_NORMALBLEND_AREAS];
		Red::System::MemoryZero( zeroWeights, sizeof( Float ) * NUM_NORMALBLEND_AREAS );
		attachment->SetNormalBlendWeights( 0, NUM_NORMALBLEND_AREAS, zeroWeights );
	}
}

void CNormalBlendComponent::SetAreas( const TDynArray< Vector >& areas )
{
	m_normalBlendAreas = areas;

	// If we're already attached to something with NB Attachment, send along the new areas.
	if ( HasCachedNBAttachment() )
	{
		CNormalBlendAttachment* attachment = GetCachedNBAttachment();
		ASSERT( attachment, TXT("Couldn't get a CNormalBlendAttachment, but we think there was one...") );
		if ( attachment )
		{
			attachment->SetNormalBlendAreas( 0, NUM_NORMALBLEND_AREAS, m_normalBlendAreas.TypedData() );
		}
	}
}

void CNormalBlendComponent::OnRenderProxiesChanged()
{
	CNormalBlendAttachment* attachment = GetCachedNBAttachment();
	if ( attachment )
	{
#ifndef NO_EDITOR
		if ( m_shouldRemapParameters )
		{
			MapMaterialParameters();
			m_shouldRemapParameters = false;
		}
#endif

		// When the attached render proxies have been changed, we need to re-send the normal blend material, areas, and weights.
		attachment->SetNormalBlendMaterial( m_normalBlendMaterial.Get(), m_sourceMaterial.Get(), m_sourceNormalTexture.Get() );
		attachment->SetNormalBlendAreas( 0, NUM_NORMALBLEND_AREAS, m_normalBlendAreas.TypedData() );
		UpdateWeights();
	}
}

void CNormalBlendComponent::UpdateDataManually( const Float* weights )
{
	CNormalBlendAttachment* attachment = GetCachedNBAttachment();
	if ( attachment )
	{
		attachment->SetNormalBlendWeights( 0, NUM_NORMALBLEND_AREAS, weights );
	}
}

void CNormalBlendComponent::OnTickPostUpdateTransform( Float dt )
{
	if ( m_dataSource )
	{
		m_dataSource->Tick( dt );

		UpdateWeights();
	}
}
