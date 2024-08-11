/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "materialOverrideProvider.h"

///////////////////////////////////////////////////////////////////////////////
// SMaterialOverrideContext

SMaterialOverrideContext::SMaterialOverrideContext ()
	: m_originalMaterial( NULL )
	, m_chunkIndex( 0 )
{}

///////////////////////////////////////////////////////////////////////////////
// CMaterialOverrideProviderMeshComponent

Bool CMaterialOverrideProviderMeshComponent::Qualifies( const CMeshComponent &meshComponent )
{
/*	if ( !meshComponent.GetMaterialOverrideAttachments().Empty() )
		return true;
	CEntity *entity = meshComponent.GetEntity();		
	return entity && NULL!=entity->GetGlobalMaterialOverrideComponent();*/
	return false;
}

CMaterialOverrideProviderMeshComponent::CMaterialOverrideProviderMeshComponent ( const CMeshComponent *meshComponent )
	: m_meshComponent( meshComponent )
{}

/// Overrides given material (returns provided material in case of no override).
IMaterial* CMaterialOverrideProviderMeshComponent::OverrideMaterial( const SMaterialOverrideContext &context )
{
/*	ASSERT( m_meshComponent );
	
	// Attachment based override attempt
	const TDynArray<CMaterialOverrideAttachment*> &attachments = m_meshComponent->GetMaterialOverrideAttachments();
	for ( Uint32 att_i=0; att_i<attachments.Size(); ++att_i )
	{
		ASSERT( NULL != attachments[att_i] );
		if ( attachments[att_i]->IsConformed( context ) )
		{
			const CMaterialOverrideComponent *component = attachments[att_i]->GetMaterialOverrideComponent();
			ASSERT( NULL != component );
			return component->OverrideMaterial( context.m_originalMaterial );
		}
	}

	// Global override attempt
	CEntity *entity = m_meshComponent->GetEntity();
	if ( NULL != entity )
	{
		const CMaterialOverrideComponent *component = entity->GetGlobalMaterialOverrideComponent();
		ASSERT( !component || component->IsGlobalOverride() );
		if ( NULL != component )
		{
			return component->OverrideMaterial( context.m_originalMaterial );
		}
	}*/

	// No override
	return context.m_originalMaterial;
}