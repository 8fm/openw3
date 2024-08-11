/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entity.h"
#include "entityStreamingProxy.h"
#include "entityTemplate.h"
#include "streamingSectorData.h"
#include "appearanceComponent.h"
#include "layer.h"
#include "mesh.h"
#include "collisionMesh.h"
#include "collisionCache.h"
#include "../core/hashset.h"
#include "../core/depot.h"
#include "../core/dependencySaver.h"
#include "../core/dependencyLoader.h"

void CEntity::NotifyComponentsStreamedIn()
{
	PC_SCOPE_PIX( NotifyComponentsStreamedIn );
	RED_ASSERT( SIsMainThread(), TXT("The flag SWN_DoNotNotifyWorld can only be used from the main thread!") );

	LoadStreamedComponentsState();

	// Inform directly the components they were streamed in
	for ( auto it : m_streamingComponents )
	{
		if( it ) it->OnStreamIn();
	}

	// Inform subclasses about the streamin operation
	OnStreamIn();
}

void CEntity::RegisterInStreamingGrid( CWorld* world )
{
	RED_ASSERT( SIsMainThread(), TXT("RegisterInStreamingGrid should be called only from the main thread") );
	RED_ASSERT( m_streamingProxy == nullptr, TXT("Entity already registered in the streaming grid, not adding") );

	// already registered ?
	if ( m_streamingProxy == nullptr )
	{
		if ( world->GetStreamingSectorData() )
		{
			const Vector& entityReferencePosition = GetWorldPositionRef();
			m_streamingProxy = world->GetStreamingSectorData()->RegisterEntity(this, entityReferencePosition);
		}
	}
}

void CEntity::UnregisterFromStreamingGrid( CWorld* world )
{
	RED_ASSERT( SIsMainThread(), TXT("RegisterInStreamingGrid should be called only from the main thread") );

	// unregister
	if ( m_streamingProxy )
	{
		if ( world->GetStreamingSectorData() )
		{
			world->GetStreamingSectorData()->UnregisterEntity( m_streamingProxy );
		}

		m_streamingProxy = nullptr;
	}
}

void CEntity::UpdateInStreamingGrid( CWorld* world )
{
	if ( m_streamingProxy )
	{
		if ( world->GetStreamingSectorData() )
		{
			const Vector& entityReferencePosition = GetWorldPositionRef();
			world->GetStreamingSectorData()->UpdateEntity( m_streamingProxy, entityReferencePosition );
		}
	}
}

void CEntity::ResaveStreamingBuffer( const DependencySavingContext& baseSavingContext )
{
	// no data 
	const Uint32 originalSize = m_streamingDataBuffer.GetSize();
	if ( m_streamingDataBuffer.GetSize() == 0 )
		return;

	// create fake entity as parent
	CEntity* fakeParent = new CEntity();

	// create streaming components
	CMemoryFileReader reader( (const Uint8*) m_streamingDataBuffer.GetData(), m_streamingDataBuffer.GetSize(), 0 );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = fakeParent;
	CDependencyLoader loader( reader, nullptr );
	if ( !loader.LoadObjects( loadingContext ) )
	{
		fakeParent->Discard();
		ERR_ENGINE( TXT("Failed to load streaming data for resaving in entity '%ls'"), GetFriendlyName().AsChar() );
		return;
	}

	// no objects to save - buffer had no valid data
	if ( loadingContext.m_loadedRootObjects.Empty() )
	{
		fakeParent->Discard();
		ERR_ENGINE( TXT("No streamuing data found in entity '%ls'"), GetFriendlyName().AsChar() );
		m_streamingDataBuffer.Clear();
		return;
	}

	// Clear the names of the components
	Uint32 compIndex = 0;
	for ( CObject* obj : loadingContext.m_loadedRootObjects )
	{
		if ( obj->IsA< CComponent >() )
		{
			CComponent* comp = static_cast< CComponent* >( obj );

			// Component name hash
			const Uint64 nameHash = Red::CalculateHash64( UNICODE_TO_ANSI( comp->GetName().AsChar() ) );

			// Rebuild special GUID for the component that's only unique inside the component
			CGUID componentGUID;
			componentGUID.guid[0] = (Uint32)( (nameHash >> 32) & 0xFFFFFFFF );
			componentGUID.guid[1] = (Uint32)( (nameHash >> 0) & 0xFFFFFFFF );
			componentGUID.guid[2] = 0xEEEE3333;
			componentGUID.guid[3] = 2; // streamed component

			// reset name only if entity is raw entity
			if ( IsExactlyA< CEntity >() )
			{
				// W3:  not safe :(
				//comp->SetName( String::EMPTY );
			}
		}
	}

	// setup component save
	DependencySavingContext savingContext( loadingContext.m_loadedRootObjects );
	savingContext.m_hashPaths = baseSavingContext.m_hashPaths; // size optimization for the streaming data buffers
	savingContext.m_saveReferenced = baseSavingContext.m_saveTransient;
	savingContext.m_saveReferenced = baseSavingContext.m_saveReferenced;
	savingContext.m_isResourceResave = baseSavingContext.m_isResourceResave;
	savingContext.m_zeroNonDeterministicData = true; // always zero the timestamp of the streaming buffer - it makes it more save
#ifndef NO_RESOURCE_COOKING
	savingContext.m_cooker = baseSavingContext.m_cooker; // this will cause data to be cooked and additional dependencies to be reported
	savingContext.m_speechCollector = baseSavingContext.m_speechCollector;
#endif

	// write to new memory
	TDynArray< Uint8 > finalData;
	CMemoryFileWriter writer( finalData );
	CDependencySaver saver( writer, nullptr );
	if ( !saver.SaveObjects( savingContext ) )
	{
		fakeParent->Discard();
		ERR_ENGINE( TXT("Failed to resave streaming data for entity '%ls'"), GetFriendlyName().AsChar() );
		return;
	}

	// discard the helper entity and the loaded objects themselves
	fakeParent->Discard();

	// store data
	m_streamingDataBuffer.SetData( finalData.Data(), (Uint32) finalData.DataSize() );
}

Bool CEntity::IsFullyLoaded() const
{
	if ( ShouldBeStreamed() )
	{
		if ( IsStreamedIn() )
		{
			return true;
		}
		return false;
	}
	return true;

}

class CResourceExtractor
{
private:
	THashSet< String >						m_resources;
	Bool									m_includeAllAppearances;
	THashSet< const CEntityTemplate* >		m_visitedTemplates;

public:
	CResourceExtractor( const Bool allAppearances = false )
		: m_includeAllAppearances( allAppearances )
	{
	}

	void Add( const String& resourcePath )
	{
		m_resources.Insert( resourcePath );
	}

	const THashSet< String >& GetResources() const
	{
		return m_resources;
	}

	void GetResources( TDynArray< String >& outPaths )
	{
		for ( auto it = m_resources.Begin(); it != m_resources.End(); ++it )
		{
			outPaths.PushBack( *it );
		}
	}

	void Extract( const void* data, const Uint32 size )
	{
		if ( size && data )
		{
			CMemoryFileReader reader( (const Uint8*) data, size, 0 );
			CDependencyLoader loader( reader, nullptr );

			TDynArray< FileDependency > deps;
			loader.LoadDependencies( deps, true );

			for ( const auto& it : deps )
			{
				Add( it.m_depotPath );
			}
		}
	}

	void Extract( const SharedDataBuffer& data )
	{
		Extract( data.GetData(), data.GetSize() );
	}

	void Extract( const CEntity* entity )
	{
		// Process base data
		if ( entity->GetEntityTemplate() )
		{
			Extract( entity->GetEntityTemplate() );
		}
		else
		{
			Extract( entity->GetLocalStreamedComponentDataBuffer());
		}

		// Process appearances
		if ( m_includeAllAppearances && entity->GetEntityTemplate() )
		{
			for ( const auto& appearance : entity->GetEntityTemplate()->GetAppearances() )
			{
				for ( const auto appearanceTemplate : appearance.GetIncludedTemplates() )
				{
					if ( appearanceTemplate.IsValid() )
					{
						Extract( appearanceTemplate.Get() );
					}
				}
			}
		}
		else
		{
			CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
			if ( appearanceComponent != nullptr )
			{
				TDynArray< CEntityTemplate* > dynamicTemplates;
				appearanceComponent->GetCurrentAppearanceTemplates( dynamicTemplates );

				for ( Uint32 i=0; i<dynamicTemplates.Size(); ++i )
				{
					Extract( dynamicTemplates[i] );
				}
			}
		}
	}

	void Extract( const CEntityTemplate* entityTemplate )
	{
		if ( !entityTemplate )
			return;

		if ( m_visitedTemplates.Exist( entityTemplate ) )
			return;

		m_visitedTemplates.Insert( entityTemplate );

		CEntity* entity = entityTemplate->GetEntityObject();
		if ( entity )
		{
			Extract( entity->GetLocalStreamedComponentDataBuffer() );
		}

		// go to the includes
		const auto& includes = entityTemplate->GetIncludes();
		for ( auto it : includes )
		{
			if ( it.IsValid() )
			{
				Extract( it.Get() );
			}
		}
	}
};

void CEntity::CollectResourcesInStreaming( TDynArray< String >& outResourcePaths ) const
{
	// Extract resources used in the template
	CResourceExtractor extractor( /* all appearances */ true );
	extractor.Extract( this );
	extractor.GetResources( outResourcePaths );
}

void CEntity::PrecacheStreamedComponents( CEntityStreamingData& outData, Bool createComponentsFromIncludes ) const
{
	// Extract resources used in the template
	CResourceExtractor extractor;
	extractor.Extract( this );

	/*// Yup, resources ;]
	if ( !extractor.m_resources.Empty() )
	{
		LOG_ENGINE( TXT("Found %d resource to precache for streaming of '%ls'"), 
			extractor.m_resources.Size(), GetFriendlyName().AsChar() );

		Uint32 index = 0;
		for ( auto it = extractor.m_resources.Begin(); it != extractor.m_resources.End(); ++it )
		{
			LOG_ENGINE( TXT("   [%d]: %ls"), index, (*it).AsChar() );
			index += 1;
		}
	}*/

	// precache resources
	if ( !extractor.GetResources().Empty() )
	{
		for ( auto it = extractor.GetResources().Begin(); it != extractor.GetResources().End(); ++it )
		{
			const String& path = *it;
			THandle< CResource > res = GDepot->LoadResource( path );
			if ( res )
			{
				outData.m_precachedResources.PushBack( res );

				// precache mesh collisions
				if ( res && res->IsA< CMesh >() )
				{
					Red::System::DateTime meshTime;

#ifndef NO_EDITOR
					{
						CMesh* mesh = static_cast< CMesh* >( res.Get() );
						meshTime = mesh->GetFileTime();
					}
#endif

					CompiledCollisionPtr ptr;
					const auto result = GCollisionCache->FindCompiled( ptr, path, meshTime );

					if( result == ICollisionCache::eResult_Valid )
					{
						outData.m_precachedCollision.PushBack( ptr );
						break;
					}
				}
			}
		}
	}
}

void CEntity::CreateStreamedComponents( EStreamingWorldNotification notification, Bool createComponentsFromIncludes/* =true */, const CEntityStreamingData* precachedData /*= nullptr*/, Bool notifyComponents /*= true*/ )
{
	// Check if streaming is locked
	if ( IsStreamingLocked() )
	{
		return;
	}

	// Check if the request is valid
	if ( !ShouldBeStreamed() || IsStreamedIn() )
	{
		return;
	}

	// We should not have streamed components
	RED_ASSERT( m_streamingComponents.Empty(), TXT("The streamed component array was not clear, this will leak in final builds") );
#ifndef RED_FINAL_BUILD
	m_streamingComponents.Clear();
#endif

	// Set the flag that we have the streaming components created
	SetDynamicFlag( EDF_StreamedIn );

#ifndef RED_FINAL_BUILD
	// Break
	if ( CheckDynamicFlag( EDF_DebugBreakOnStreamIn ) )
	{
		LOG_ENGINE( TXT("!!! DEBUG BREAK ON STREAMING OF ENTITY 0x%016llX, '%ls'"), (Uint64)this, GetFriendlyName().AsChar() );
		RED_BREAKPOINT();
	}
#endif

	// Get data buffer with streaming data
	const auto* dataBuffer = &GetLocalStreamedComponentDataBuffer();
	if ( m_template != nullptr )
	{
		// Use template data
		RED_ASSERT( m_template->GetEntityObject(), TXT("Entity template without entity object, something is BROKEN! Template: '%ls'"), m_template->GetDepotPath().AsChar() );
		if ( !m_template->GetEntityObject() )
		{
			return;
		}

		// Use the templated data if we have it
		dataBuffer = &m_template->GetEntityObject()->GetLocalStreamedComponentDataBuffer();
	}

	// Get data
	// TODO: compression !
	const Uint8* compressedData = (const Uint8*) dataBuffer->GetData();
	Uint32 compressedDataSize = dataBuffer->GetSize();

	RED_ASSERT( m_streamingComponents.Empty(), TXT("The streamed component array was not clear, this will leak in final builds") );
#ifndef RED_FINAL_BUILD
	m_streamingComponents.Clear();
#endif

	// Create the streamed components
	TDynArray< CComponent* > streamingComponents;
	if ( compressedDataSize > 0 )
	{
		CEntityTemplate::IncludeComponents( 
			this,
			this,
			compressedData, 
			compressedDataSize,
			streamingComponents,
			NULL,
			false
			);
	}
#ifndef RED_FINAL_BUILD	
	else
	{

		// Create old streamed components
		if ( GetEntityTemplate() != nullptr )
		{
			for ( Uint32 i=0; i < 3; ++i )
			{
				if ( GetEntityTemplate()->GetEntityObject()->m_oldBuffer[i].Size() > 0 )
				{
					CEntityTemplate::IncludeComponents( 
						this,
						this,
						static_cast< const Uint8* >( GetEntityTemplate()->GetEntityObject()->m_oldBuffer[i].Data() ), 
						GetEntityTemplate()->GetEntityObject()->m_oldBuffer[i].Size(), 
						streamingComponents,
						NULL,
						false
						);
				}
			}
		}
		else
		{
			for ( Uint32 i=0; i < 3; ++i )
			{
				if ( m_oldBuffer[i].Size() > 0 )
				{
					CEntityTemplate::IncludeComponents( 
						this,
						this,
						static_cast< const Uint8* >( m_oldBuffer[i].Data() ), 
						m_oldBuffer[i].Size(), 
						streamingComponents,
						NULL,
						false
						);
				}
			}
		}
	}
#endif

	// If we have a template, create streaming components defined in its includes
	if ( createComponentsFromIncludes && GetEntityTemplate() != nullptr )
	{
		struct LocalFunc {
			static void IncludeStreamingComponentsFromTemplate( Bool includeLocal, CEntity* entity, TDynArray< CComponent* >& addedComponents, CEntityTemplate* tpl )
			{
				// Include local components
				if ( includeLocal )
				{
					CEntity* tplent = tpl->GetEntityObject();
					if ( tplent != nullptr )
					{
						const auto& dataBuffer = tplent->GetLocalStreamedComponentDataBuffer();
						if ( dataBuffer.GetSize() > 0 )
						{
							CEntityTemplate::IncludeComponents( entity, entity, (Uint8*)dataBuffer.GetData(), dataBuffer.GetSize(), addedComponents, nullptr, false );
						}
#ifndef RED_FINAL_BUILD	
						// Create old streamed components
						for ( Uint32 i=0; i < 3; ++i )
						{
							if ( tplent->m_oldBuffer[i].Size() > 0 )
							{
								CEntityTemplate::IncludeComponents( 
									entity,
									entity,
									static_cast< const Uint8* >( tplent->m_oldBuffer[i].Data() ), 
									tplent->m_oldBuffer[i].Size(), 
									addedComponents,
									NULL,
									false
									);
							}
						}
#endif
					}
				}

				// Include components from this template's own included templates
				const TDynArray< THandle< CEntityTemplate > >& includes = tpl->GetIncludes();
				for ( auto it=includes.Begin(); it != includes.End(); ++it )
				{
					CEntityTemplate* subtpl = (*it).Get();
					if ( subtpl != nullptr && subtpl->GetEntityObject() != nullptr )
					{
						IncludeStreamingComponentsFromTemplate( true, entity, addedComponents, subtpl );

#ifndef RED_FINAL_BUILD	
						// Create old streamed components
						for ( Uint32 i=0; i < 3; ++i )
						{
							if ( subtpl->GetEntityObject()->m_oldBuffer[i].Size() > 0 )
							{
								CEntityTemplate::IncludeComponents( 
									entity,
									entity,
									static_cast< const Uint8* >(  subtpl->GetEntityObject()->m_oldBuffer[i].Data() ), 
									subtpl->GetEntityObject()->m_oldBuffer[i].Size(), 
									addedComponents,
									NULL,
									false
									);
							}
						}
#endif
					}
				}
			}
		};
		Uint32 base = streamingComponents.Size();
		LocalFunc::IncludeStreamingComponentsFromTemplate( false, this, streamingComponents, GetEntityTemplate() );

		// Mark included streamed components
		for ( Uint32 i=base; i < streamingComponents.Size(); ++i )
		{
			streamingComponents[i]->SetFlag( NF_IncludedFromTemplate );
		}
	}

	// If we have an appearance component, create components stored in it
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( this );
	if ( appearanceComponent != nullptr )
	{
		appearanceComponent->CreateStreamingComponentsForActiveAppearance( streamingComponents );
	}

	// Put the new components in the proper array and set the streamed flag
	for ( auto it=streamingComponents.Begin(); it != streamingComponents.End(); ++it )
	{
		(*it)->SetComponentFlag( CF_StreamedComponent );
		(*it)->ForceUpdateTransformNodeAndCommitChanges(); // needed because there's no valid position in the streaming buffer
		m_streamingComponents.PushBack( *it );
	}

	// If we have a template, create any streamed attachments between components we may have
	if ( GetEntityTemplate() != nullptr )
	{
		// Create the attachments
		TDynArray< IAttachment* > createdAttachments;
		CreateStreamedAttachmentsForNewComponents( streamingComponents, createdAttachments );
	}

	// Notify the world if needed
	if ( notification == SWN_NotifyWorld && GetLayer() != nullptr )
	{
		// moved
	}
	else // Do not notify the world or the layer is null
	{
		// TODO: move
		if ( notifyComponents )
		{
			NotifyComponentsStreamedIn();
		}
	}
}

void CEntity::DestroyStreamedComponents( EStreamingWorldNotification notification )
{
	RED_ASSERT( SIsMainThread(), TXT("CEntity::DestroyStreamedComponents can only be called from the main thread!") );

	// Check if streaming is locked
	if ( IsStreamingLocked() )
	{
		return;
	}

	// Check if we have components streamed in
	if ( !IsStreamedIn() )
	{
		return;
	}

#ifndef RED_FINAL_BUILD
	// Break
	if ( CheckDynamicFlag( EDF_DebugBreakOnStreamOut ) )
	{
		LOG_ENGINE( TXT("!!! DEBUG BREAK ON UNSTREAMING OF ENTITY 0x%016llX, '%ls'"), (Uint64)this, GetFriendlyName().AsChar() );
		RED_BREAKPOINT();
	}
#endif

	// Inform subclasses about the stream out operation
	OnStreamOut();

	// Inform the components they are streamed out
	for ( auto it=m_streamingComponents.Begin(); it != m_streamingComponents.End(); ++it )
	{
		THandle< CComponent >& component = *it;
		if ( component.IsValid() )
		{
			(*it)->OnStreamOut();
		}
	}

	SaveStreamedComponentsState();

	// Remove streamed attachments
	for ( auto it=m_streamingAttachments.Begin(); it != m_streamingAttachments.End(); ++it )
	{
		IAttachment* attachment = (*it).Get();

		// Make sure we still have the attachment
		if ( attachment != nullptr )
		{
			// Break the attachment
			attachment->Break();

			// Discard it
			attachment->Discard();
		}
	}

	// Remove the streamed components
	for ( auto it=m_streamingComponents.Begin(); it != m_streamingComponents.End(); ++it )
	{
		CComponent* component = (*it).Get();

		// Make sure we still have the component
		if ( component != nullptr )
		{
			// Inform active effects that we are going to destroy the component
			for ( auto it=m_activeEffects.Begin(); it != m_activeEffects.End(); ++it )
			{
				(*it)->OnComponentStreamOut( component );
			}

			// Destroy the component
			component->Destroy();
		}
	}

	// Clean the streaming arrays for this LOD level
	m_streamingComponents.Clear();
	m_streamingAttachments.Clear();

	// Clear the flag for this streaming LOD
	ClearDynamicFlag( EDF_StreamedIn );
}

void CEntity::SetStreamed( const Bool streamed )
{
	if ( streamed != ShouldBeStreamed() )
	{
		// Update entity static flags
		if ( streamed )
		{
			m_entityStaticFlags |= ESF_Streamed;
		}
		else
		{
			m_entityStaticFlags &= ~ESF_Streamed;
		}

		// Handle streamed flag change
		StreamedFlagChanged();
	}
}

void CEntity::CreateStreamedAttachmentsForNewComponentsUsingTemplate( CEntityTemplate* entityTemplate, const TDynArray< CComponent* >& newComponents, TDynArray< IAttachment* >& createdAttachments )
{
	Uint32 base = createdAttachments.Size();

	// Streamed attachments are stored in the entity template, so we need one...
	if ( entityTemplate == nullptr ) return;

	// Ask the entity template to create the streamed attachments
	entityTemplate->CreateStreamedAttachmentsInEntity( this, newComponents, createdAttachments );

	// Put the new attachments in the proper array
	for ( Uint32 i=base; i < createdAttachments.Size(); ++i )
	{
		if( createdAttachments[i]->GetParent() )
		{
			ASSERT( createdAttachments[i]->GetParent()->IsA< CComponent >(), TXT("Not a component parent in streamed attachment! Crash imminent...") );
		}
		if( createdAttachments[i]->GetChild() )
		{
			ASSERT( createdAttachments[i]->GetChild()->IsA< CComponent >(), TXT("Not a component child in streamed attachment! Crash imminent...") );
		}
		m_streamingAttachments.PushBack( createdAttachments[i] );
	}
}

void CEntity::CreateStreamedAttachmentsForNewComponents( const TDynArray< CComponent* >& newComponents, TDynArray< IAttachment* >& createdAttachments )
{
	CreateStreamedAttachmentsForNewComponentsUsingTemplate( GetEntityTemplate(), newComponents, createdAttachments );
}

#ifndef NO_EDITOR

void CEntity::RemoveAllStreamingData()
{
	if ( !MarkModified() )
	{
		return;
	}

	// Clear all local data buffers
	m_streamingDataBuffer.Clear();

	// Clear template data buffers (if any)
	if ( GetEntityTemplate() != nullptr )
	{
		GetEntityTemplate()->GetEntityObject()->m_streamingDataBuffer.Clear();
	}
}

void CEntity::UpdateStreamingDistance()
{
	PC_SCOPE( CEntity_UpdateStreamingDistance );

	SEntityStreamingState state;
	PrepareStreamingComponentsEnumeration( state, false, SWN_DoNotNotifyWorld );

	// Collect minimum distance from components
	Uint32 minimumDistance = 0;
	for ( auto it=m_components.Begin(); it != m_components.End(); ++it )
	{
		if ( (*it)->IsStreamed() )
		{
			minimumDistance = Max( minimumDistance, (*it)->GetMinimumStreamingDistance() );
		}
	}

	// Collect minimum distance from appearances
	if ( GetEntityTemplate() != nullptr )
	{
		TDynArray< const CEntityAppearance* > appearances;
		GetEntityTemplate()->GetAllAppearances( appearances );
		for ( const CEntityAppearance* appearance : appearances )
		{
			const auto& templates = appearance->GetIncludedTemplates();
			for ( CEntityTemplate* tpl : templates )
			{
				// We need to create a temporary entity here since an
				// appearance template can have its own includes and we
				// cannot rely on just the embedded entity - also we need
				// to force an update of the template's distance to make
				// sure that we have an up-to-date value (otherwise we
				// could have an issue when this entity is resaved before
				// an entity template it depends on is resaved and has its
				// distance updated)
				RED_ASSERT( tpl != nullptr, TXT("Missing included template") );
				if ( tpl )
				{
					CEntity* temp = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
					temp->UpdateStreamingDistance();
					minimumDistance = Max( minimumDistance, temp->GetStreamingDistance() );
					temp->Discard();
				}
			}
		}
	}

	minimumDistance = Clamp<Uint32>( minimumDistance, 0, 2040 );
	if ( (minimumDistance % 8) != 0 ) minimumDistance = ((minimumDistance/8) + 1)*8;
	SetStreamingDistance( minimumDistance );
	FinishStreamingComponentsEnumeration( state, SWN_DoNotNotifyWorld );
}

// Collect all components that fulfill conditions required to be streamed
template < typename T >
void CollectComponentsAllowedForStreaming( const TDynArray< CComponent* > & components, TDynArray< T > & streamedComponents )
{
	for ( auto it = components.Begin(); it != components.End(); ++it )
	{
		CComponent* component = *it;
		RED_ASSERT( component, TXT("NULL in components") );

		// Ignore non-streamed components
		if ( !component->IsStreamed() )
		{
			continue;
		}

		// Ignore included streaming components
		if ( component->HasComponentFlag( CF_StreamedComponent ) && component->HasFlag( NF_IncludedFromTemplate ) )
		{
			continue;
		}

		// Ignore components from appearances
		if ( component->HasComponentFlag( CF_UsedInAppearance ) )
		{
			continue;
		}

		// Ignore components that they refuse to be written to disk
		if ( !component->ShouldWriteToDisk() )
		{
			continue;
		}

		// Check if we should stream this component
		if ( !CEntity::AllowStreamingForComponent( component ) )
		{
			continue;
		}

		streamedComponents.PushBack( component );
	}
}

Bool CEntity::UpdateStreamedComponentDataBuffers( Bool includeExistingComponents /* = true */ )
{
	PC_SCOPE( CEntity_UpdateStreamedComponentDataBuffers );

	// The entity isn't streamed
	if ( !ShouldBeStreamed() )
	{
		return true;
	}

	// Keep previous state and unlock streaming
	Bool previousLock = SetStreamingLock( false );
	Bool wasStreamedIn = IsStreamedIn();

	// Create streamed attachments
	if ( GetEntityTemplate() != nullptr )
	{
		// Make sure all components are created
		CreateStreamedComponents( SWN_DoNotNotifyWorld );

		// Build array with streamed components
		TDynArray< CComponent* > streamedComponents;
		CollectComponentsAllowedForStreaming( m_components, streamedComponents );

		// Update the entity template's streamed attachments using the to-be streamed components
		GetEntityTemplate()->UpdateStreamedAttachmentsFromEntity( this, streamedComponents );
	}

	// Make sure the components are created in memory before we clean the buffers
	if ( includeExistingComponents )
	{
		CreateStreamedComponents( SWN_DoNotNotifyWorld, false );
	}

	// Recreate buffers
	TDynArray< Uint8 > newBuffer;
	m_streamingComponents.Clear();
	CollectComponentsAllowedForStreaming( m_components, m_streamingComponents );

	// Check if we have components
	if ( m_streamingComponents.Empty() )
	{
		// No components, just clean the buffer to save some bytes
		newBuffer.Clear();
	}
	else
	{
		// Build the component array
		TDynArray< CComponent* > streamingComponents;
		for ( auto it=m_streamingComponents.Begin(); it != m_streamingComponents.End(); ++it )
		{
			if ( (*it).IsValid() )
			{
				streamingComponents.PushBack( (*it).Get() );
			}
		}

		// Serialize the streamed component array to streaming buffer
		CMemoryFileWriter writer( newBuffer );
		CDependencySaver saver( writer, nullptr );
		DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&)( streamingComponents ) );
		context.m_saveTransient = true;
		context.m_saveReferenced = true;
		saver.SaveObjects( context );
	}

	// We'll need the meshes, etc to finish loading so we have proper distance data
	ForceFinishAsyncResourceLoads();

	// Destroy the streamed components
	DestroyStreamedComponents( SWN_DoNotNotifyWorld );

	// Create streaming buffer
	m_streamingDataBuffer.SetData( newBuffer.Data(), (Uint32) newBuffer.Size() );

	// Clean up the remaining component properties
	for ( auto it=m_components.Begin(); it != m_components.End(); ++it )
	{
		CComponent* component = *it;

		// Skip components that come from other templates
		if ( component->IsUsedInAppearance() || component->IsIncludedFromTemplate() || !component->ShouldWriteToDisk() )
		{
			continue;
		}

		// Remove the referenced and transient flags so we wont lose components which used to be streamed
		component->ClearFlag( OF_Referenced );
		component->ClearFlag( OF_Transient );

		// Make the component unstreamed to avoid destroying it
		component->SetStreamed( false );
	}

	// For templated entities, the data must be copied to the entity template
	if ( GetEntityTemplate() != nullptr && GetEntityTemplate()->GetEntityObject() != nullptr )
	{
		GetEntityTemplate()->GetEntityObject()->m_streamingDataBuffer = GetLocalStreamedComponentDataBuffer();
	}

	// Recreate previously created components
	if ( wasStreamedIn )
	{
		CreateStreamedComponents( SWN_DoNotNotifyWorld );
	}

	// Restore streaming lock state
	SetStreamingLock( previousLock );

	return true;
}

void CEntity::PrepareStreamingComponentsEnumeration( SEntityStreamingState& state, Bool lockStreaming, EStreamingWorldNotification notification /*=SWN_NotifyWorld*/ )
{
	state.m_streamedIn = IsStreamedIn();
	state.m_wasLocked = IsStreamingLocked();
	CreateStreamedComponents( notification );
	SetStreamingLock( lockStreaming );
}

void CEntity::FinishStreamingComponentsEnumeration( const SEntityStreamingState& state, EStreamingWorldNotification notification /*=SWN_NotifyWorld*/ )
{
	SetStreamingLock( state.m_wasLocked );
	if ( IsStreamedIn() != state.m_streamedIn )
	{
		if ( state.m_streamedIn )
		{
			CreateStreamedComponents( notification );
		}
		else
		{
			DestroyStreamedComponents( notification );
		}
	}
}

Bool CEntity::SetStreamingLock( Bool lock )
{
	const Bool previous = IsStreamingLocked();
	SetDynamicFlag( EDF_StreamingLocked, lock );
	return previous;
}

void CEntity::RemoveComponentFromStreaming( CComponent* component )
{
	// Remove from streaming components
	for ( Int32 i=m_streamingComponents.SizeInt() - 1; i >= 0; --i )
	{
		if ( m_streamingComponents[i].Get() == component )
		{
			m_streamingComponents.RemoveAt( i );
		}
	}

	// Remove from attachments
	for ( Int32 i=m_streamingAttachments.SizeInt() - 1; i >= 0; --i )
	{
		IAttachment* attachment = m_streamingAttachments[i].Get();
		if ( attachment != nullptr )
		{
			if ( attachment->GetParent() == component ||
				attachment->GetChild() == component )
			{
				m_streamingAttachments.RemoveAt( i );
			}
		}
		else
		{
			m_streamingAttachments.RemoveAt( i );
		}
	}
}

Bool CEntity::CanUnstream() const
{
	// Do not unstream the entity if it is modified to avoid losing any unsaved properties
	if ( GetLayer() != nullptr && GetLayer()->IsModified() )
	{
		return false;
	}

	return true;
}

#endif // NO_EDITOR
