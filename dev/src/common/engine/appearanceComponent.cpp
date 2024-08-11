
#include "build.h"
#include "appearanceComponent.h"
#include "mimicComponent.h"

#include "../core/fileSkipableBlock.h"
#include "../core/scriptStackFrame.h"
#include "../core/dataError.h"
#include "meshTypeResource.h"
#include "../core/resource.h"
#include "externalProxy.h"
#include "layer.h"
#include "entity.h"
#include "baseEngine.h"
#include "../core/gameSave.h"
#include "entityExternalAppearance.h"
#include "appearanceComponentModifier.h"
#include "meshComponent.h"

#ifndef NO_EDITOR
Bool GDisableAppearances = false;
#endif

RED_DEFINE_STATIC_NAME( forcedAppearance );

IMPLEMENT_ENGINE_CLASS( SAppearanceAttachment );
IMPLEMENT_ENGINE_CLASS( SAppearanceAttachments );
IMPLEMENT_ENGINE_CLASS( CAppearanceComponent );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( child_average_shadowmesh );
RED_DEFINE_STATIC_NAME( dwarf_average_shadowmesh );
RED_DEFINE_STATIC_NAME( halfling_average_shadowmesh );
RED_DEFINE_STATIC_NAME( man_average_shadowmesh );
RED_DEFINE_STATIC_NAME( man_big_shadowmesh );
RED_DEFINE_STATIC_NAME( man_strong_shadowmesh );
RED_DEFINE_STATIC_NAME( wildhunt_shadowmesh );
RED_DEFINE_STATIC_NAME( woman_average_shadowmesh );
//RED_DEFINE_STATIC_NAME( uldaryk_him_shadowmesh ); //< not needed since managed by appearance in asset

static Bool GetShadowMeshName( CName &outName, const String &meshPath )
{
	// TODO: don't really see a better way of extracting name from mesh path at this moment without any allocation, 
	//		 so I'll stay with doing it by hand.

	Int32 lastSubstring = -1;
	{
		size_t charIndex;
		while (	meshPath.FindCharacter( '\\', charIndex, (size_t)(-1 != lastSubstring ? lastSubstring : 0), false ) ||
				meshPath.FindCharacter( '/', charIndex, (size_t)(-1 != lastSubstring ? lastSubstring : 0), false ) )
		{
			lastSubstring = (Int32)charIndex + 1;
		}
	}

	Int32 dotPos = -1;
	if ( -1 != lastSubstring )
	{
		size_t localDotPos;
		if ( meshPath.FindCharacter( '.', localDotPos, (size_t)lastSubstring, false ) )
		{
			dotPos = (Int32)localDotPos;
		}
	}

	if ( !(-1 != dotPos && dotPos > lastSubstring) )
	{
		return false;
	}

	const Uint32 nameLen = dotPos - lastSubstring;
	const Uint32 localNameMaxLen = 256;
	if ( nameLen > localNameMaxLen )
	{
		RED_FATAL( "Too long name" );
		return false;
	}

	Char localName[localNameMaxLen + 1];
	/*
	for ( Uint32 i=0; i<nameLen; ++i )
		localName[i] = meshPath[lastSubstring + i];
	*/
	Red::MemoryCopy( localName, meshPath.AsChar() + lastSubstring, nameLen * sizeof(Char) );
	localName[nameLen] = 0;

	outName = CName( localName );
	return true;
}

RED_INLINE Bool IsComponentRemovalByAppearanceAllowed( const CDrawableComponent &component )
{
	Bool forceKeepInEntity = false;

	// We want to force keep components which are character fallback shadows.
	// Note that this condition is here also to early reject the components so that we wouldn't do 
	// any non-trivial tests inside when we know upfront that it's not needed.
	if ( 0 != (component.GetDrawableFlags() & DF_IsCharacterShadowFallback) )
	{
		CName testName;
		if ( component.IsA<CMeshComponent>() && GetShadowMeshName( testName, static_cast<const CMeshComponent&>( component ).GetMeshResourcePath() ) )
		{
			// We're keeping only super crucial fallback shadows, because some fallback shadows (for instance dresses and who knows what else)
			// might need to be filtered out by appearances, so they are being skipped here.
			// Test is being performed by mesh name because component names are less confident (there is no same naming convention in assets).
			if ( testName == CNAME( child_average_shadowmesh ) ||
				 testName == CNAME( dwarf_average_shadowmesh ) ||
				 testName == CNAME( halfling_average_shadowmesh ) ||
				 testName == CNAME( man_average_shadowmesh ) ||
				 testName == CNAME( man_big_shadowmesh ) ||
				 testName == CNAME( man_strong_shadowmesh ) ||
				 testName == CNAME( wildhunt_shadowmesh ) ||
				 testName == CNAME( woman_average_shadowmesh ) 
				 // testName == CNAME( uldaryk_him_shadowmesh ) || //< not needed since managed by appearance in asset
				 )
			{
				forceKeepInEntity = true;
			}
		}
		else
		{
			RED_ASSERT( !"Unable to get the path" );
		}
	}

	return !forceKeepInEntity;
}

//////////////////////////////////////////////////////////////////////////

SAppearanceAttachment::SAppearanceAttachment()
	: m_attachment( nullptr )
{
}

SAppearanceAttachment::~SAppearanceAttachment()
{
}

Bool SAppearanceAttachment::CustomSerializer( IFile& file, void* data )
{
	SAppearanceAttachment& self = *static_cast< SAppearanceAttachment* >( data );

	// Default serialization
	SAppearanceAttachment::GetStaticClass()->DefaultSerialize( file, data );

	// Custom serialization for the attachment
	if ( file.IsGarbageCollector() )
	{
		// nop, GC is handled by the appearance component
	} 
	else if ( file.IsWriter() )
	{
		CFileSkipableBlock block( file );

		Bool hasAttachment = self.m_attachment != nullptr;
		file << hasAttachment;

		if ( hasAttachment )
		{
			CName name = self.m_attachment->GetClass()->GetName();
			file << name;
			self.m_attachment->Nullify();
			self.m_attachment->OnSerialize( file );
		}
	}
	else if ( file.IsReader() )
	{
		CFileSkipableBlock block( file );

		Bool hasAttachment = false;
		file << hasAttachment;

		if ( hasAttachment )
		{
			CName name;
			file << name;

			CClass* cls = SRTTI::GetInstance().FindClass( name );
			if ( cls == nullptr )
			{
				ERR_ENGINE( TXT("Failed to find attachment class '%ls'"), name.AsChar() );
				self.m_attachment = nullptr;
				block.Skip();
				return false;
			}

			self.m_attachment = cls->CreateObject< IAttachment >();
			if ( self.m_attachment == nullptr )
			{
				ERR_ENGINE( TXT("Failed create attachment object from class '%ls'"), name.AsChar() );
				self.m_attachment = nullptr;
				block.Skip();
				return true; // not a deal big enough to abort all the things
			}

			self.m_attachment->OnSerialize( file );
			self.m_attachment->Nullify();
		}
		else
		{
			self.m_attachment = nullptr;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

void SAppearanceAttachments::RemoveAppearanceAttachment( IAttachment* attachmentToRemove )
{
	CName parentClass = attachmentToRemove->GetParent() ? attachmentToRemove->GetParent()->GetClass()->GetName() : CName::NONE;
	CName parentName = attachmentToRemove->GetParent() ? CName( attachmentToRemove->GetParent()->GetName() ) : CName::NONE;
	CName childClass = attachmentToRemove->GetChild() ? attachmentToRemove->GetChild()->GetClass()->GetName() : CName::NONE;
	CName childName = attachmentToRemove->GetChild() ? CName( attachmentToRemove->GetChild()->GetName() ) : CName::NONE;

	for ( Uint32 i=0; i < m_attachments.Size(); ++i )
	{
		const SAppearanceAttachment& attachment = m_attachments[ i ];
		if ( attachment.m_attachment == attachmentToRemove ||
		   ( attachment.m_parentClass == parentClass && attachment.m_parentName == parentName &&
			 attachment.m_childClass == childClass && attachment.m_childName == childName ) )
		{
			m_attachments.RemoveAt( i );
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CAppearanceComponent::CAppearanceComponent()
	: m_initialAppearanceApplied( false )
	, m_initialAppearanceLoaded( false )
	, m_currentAppearance( CName::NONE )
	, m_forcedAppearance( CName::NONE )
	, m_initialAppearance( CName::NONE )
	, m_usesRobe( false )
{
}

#ifndef NO_EDITOR
void CAppearanceComponent::SetForcedAppearance( CName forcedAppearance )
{
	m_forcedAppearance = forcedAppearance;
}
#endif

void CAppearanceComponent::SetAttachmentReplacements( const SAttachmentReplacements& replacements )
{
	//LOG_ENGINE( TXT("[ !! ET !! ]: [%p] Got attachment replacements"), this );
	m_attachmentReplacements = replacements;
}

void CAppearanceComponent::ApplyInitialAppearance( const EntitySpawnInfo& spawnInfo )
{
	// If we've loaded the initial appearance from savegame, skip this
	if ( m_initialAppearanceLoaded )
	{
		return;
	}

	CEntityTemplate* entityTemplate = GetEntity()->GetEntityTemplate();
	const CEntityAppearance* appearance = nullptr;
	// Appearances are supported only for entities from entity templates
	if ( entityTemplate )
	{
		// Select random appearance from given ones
		CName appearanceName = CName::NONE;
		if ( !spawnInfo.m_appearances.Empty() )
		{
			Uint32 randIndex = GEngine->GetRandomNumberGenerator().Get< Uint32 >( spawnInfo.m_appearances.Size() );
			appearanceName = spawnInfo.m_appearances[ randIndex ];
		}
		else
		{
			appearanceName = m_forcedAppearance;
		}

		if( appearanceName != CName::NONE )
		{
			appearance = entityTemplate->GetAppearance( appearanceName, true );
		}

		// If no valid appearance was given, select from all possible ones
		if ( !appearance )	
		{
			// Get all used appearances
			const TDynArray< CName > & usedAppearances = entityTemplate->GetEnabledAppearancesNames();

			// Get random one from used appearances
			if ( !usedAppearances.Empty() )
			{
				const Uint32 index = GEngine->GetRandomNumberGenerator().Get< Uint32 >( usedAppearances.Size() );
				appearanceName = usedAppearances[ index ];
				appearance = entityTemplate->GetAppearance( appearanceName, true );
			}			
		}
			
		if ( appearance )
		{
			m_initialAppearance = appearanceName;

			// Set selected appearance, it will be applied when entity is spawned
			ApplyAppearance( *appearance );

			m_initialAppearanceApplied = true;

			if ( spawnInfo.m_canThrowOutUnusedComponents )
			{
				TDynArray< CComponent* > componentsToRemove;

				const TDynArray<CComponent*>& components = GetEntity()->GetComponents();
				for ( const auto& component : components )
				{
					if ( !component->IsUsedInAppearance() && component->IsA<CDrawableComponent>() )
					{
						CDrawableComponent *drawableComponent = static_cast<CDrawableComponent*>( component );
						if ( IsComponentRemovalByAppearanceAllowed( *drawableComponent ) )
						{
							componentsToRemove.PushBack( component );
						}
					}
				}

				for ( const auto& componentToRemove : componentsToRemove )
				{
					GetEntity()->RemoveComponent( componentToRemove );
				}
			}
		}		
	}
}

const SAppearanceAttachments* CAppearanceComponent::GetAppearanceAttachmentData( const CName& appearanceName ) const
{
	for ( const auto& data : m_appearanceAttachments )
	{
		if ( data.m_appearance == appearanceName )
		{
			return &data;
		}
	}

	// not found
	return nullptr;
}

void CAppearanceComponent::ApplyAppearance( const CEntityAppearance &appearance )
{
	PC_SCOPE( ApplyAppearance );

	// Check if appearances are disabled
#ifndef NO_EDITOR
	if ( GDisableAppearances )
	{
		return;
	}
#endif

	//LOG_ENGINE( TXT("Applying appearance %ls to %ls (0x%llX)"), appearance.GetName().AsChar(), GetEntity() ? GetEntity()->GetFriendlyName().AsChar() : TXT("NULL"), (Uint64)GetEntity() );

	CEntity* entity = GetEntity();
	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	
	// No template
	if ( !entityTemplate )
	{
		//LOG_ENGINE( TXT("[ !! ET !! ]: Unable to apply appearance to entity '%ls' without template"), GetFriendlyName().AsChar() );
		return;
	}

	// Remove the previous appearance
	RemoveCurrentAppearance();
	
	// Remember name of the appearance and whether it uses a robe (for jobtree hacking)
	m_currentAppearance = appearance.GetName();
	m_usesRobe = appearance.GetUsesRobe();

	// Include appearance templates
	TDynArray< CComponent* > addedComponents;
	{
		PC_SCOPE( ApplyAppearance Loop );

		const TDynArray< THandle< CEntityTemplate > >& entityTemplates = appearance.GetIncludedTemplates();

		for ( const auto& includedTemplate : entityTemplates )
		{
			if ( includedTemplate.IsValid() )
			{
				IncludeAppearanceTemplate( includedTemplate.Get(), addedComponents );
			}
			else
			{
				DATA_HALT( DES_Major, entityTemplate, TXT("Entity Template"), TXT("Entity Appearance for asset {%s} contains a NULL included template *THIS HAS BEEN REMOVED*, please re-save and resubmit the asset to perforce"), entityTemplate->GetDepotPath().AsChar() );
			}
		}
	}

	// Create streamed components for the already created streaming LODs
	if ( entity->ShouldBeStreamed() && entity->IsStreamedIn() )
	{
		CreateStreamingComponentsForActiveAppearance( addedComponents );
	}

	// Create attachments stored in the appearance component
	//LOG_ENGINE( TXT("Num total appearance attachments: %d"), m_appearanceAttachments.Size() );
	const SAppearanceAttachments* appearanceAttachmentsData = GetAppearanceAttachmentData( appearance.GetName() );
	if ( appearanceAttachmentsData != nullptr )
	{
		const SAppearanceAttachments& appearanceAttachments = *appearanceAttachmentsData;

		/*if ( !appearanceAttachments.m_attachments.Empty() )
		{
			LOG_ENGINE( TXT("Num appearance attachments: %d"), appearanceAttachments.m_attachments.Size() );
		}*/

		for ( const auto& appearanceAttachment : appearanceAttachments.m_attachments )
		{
			//LOG_ENGINE( TXT("  Class: %ls"), appearanceAttachment.m_attachment ? appearanceAttachment.m_attachment->GetClass()->GetName().AsChar() : TXT("NULL") );
			//LOG_ENGINE( TXT("  Parent: %ls, %ls"), appearanceAttachment.m_parentName.AsChar(), appearanceAttachment.m_parentClass.AsChar() );
			//LOG_ENGINE( TXT("  Child: %ls, %ls"), appearanceAttachment.m_childName.AsChar(), appearanceAttachment.m_childClass.AsChar() );

			// Hm..
			if ( appearanceAttachment.m_attachment == nullptr )
			{
				ERR_ENGINE( TXT("NULL appearance attachment saved") );
				continue;
			}

			// Find parent and child components
			CComponent* parentComponent = entity->FindComponent( appearanceAttachment.m_parentName, appearanceAttachment.m_parentClass );
			CComponent* childComponent = entity->FindComponent( appearanceAttachment.m_childName, appearanceAttachment.m_childClass );

			// Something was lost in the process
			if ( parentComponent == nullptr || childComponent == nullptr )
			{
				ERR_ENGINE( TXT("Cannot create saved appearance attachment: a component is missing") );
				continue;
			}

			// Clone the stored attachment
			IAttachment* clone = Cast< IAttachment >( appearanceAttachment.m_attachment->Clone( parentComponent, false, false ) );
			if ( clone == nullptr )
			{
				ERR_ENGINE( TXT("Failed to clone the saved appearance attachment") );
				continue;
			}

			// Initialize the attachment
			if ( clone->Init( parentComponent, childComponent, NULL ) )
			{
				if ( clone->GetParent() == parentComponent && clone->GetChild() == childComponent )
				{
					clone->SetParent( parentComponent );
				}
				else
				{
					clone->Discard();
					continue;
				}
			}
			else
			{
				clone->Discard();
				continue;
			}

			// Save the created attachment so we can destroy it later
			m_createdTempAttachments.PushBack( clone );
		}
	}

	// Create attachments between streamed components and the components that were created from this appearance
	if ( entity->GetEntityTemplate() != nullptr )
	{
		TDynArray< IAttachment* > createdAttachments;
		entity->CreateStreamedAttachmentsForNewComponents( addedComponents, createdAttachments );
	}

	// Post process added components
	PostProcessAddedComponents( addedComponents );

	{
		PC_SCOPE( ApplyAppearance OnAppearanceChanged );
		entity->OnAppearanceChanged( appearance );
	}
}

#ifdef W3_ARABIC_CENSORED_VERSION
	static const CName GDupaAppearance( TXT("yennefer_naked_no_hair") );
	static const CName GDupaAppearanceCensored( TXT("yennefer_naked_no_hair_arabic") );
	static const CName GDupaAppearance2( TXT("yennefer_head_towel") );
	static const CName GDupaAppearanceCensored2( TXT("yennefer_naked_panties") );
#endif

void CAppearanceComponent::ApplyAppearance( const CName &appearanceNameOrg )
{
	CName appearanceName = appearanceNameOrg;

#ifdef W3_ARABIC_CENSORED_VERSION
	// W3 glorious hack for Arabic build (31.03.2015, the Xbox Cert2 day) Allah is watching...
	// There's hot where I'm going for this...
	{
		if ( appearanceName == GDupaAppearance )
		{
			static const Bool isArabic = (Red::StringSearch( SGetCommandLine(), TXT("-arabic") ) != nullptr);
			if ( isArabic )
			{
				// if we have the censored appearance, use it
				CEntityTemplate* entityTemplate = GetEntity()->GetEntityTemplate();
				if ( entityTemplate )
				{
					const CEntityAppearance* appearance = entityTemplate->GetAppearance( GDupaAppearanceCensored, true );
					if ( appearance )
					{
						// use the censored (Arabic) version
						appearanceName = GDupaAppearanceCensored;
					}
				}
			}
		}
		else if ( appearanceName == GDupaAppearance2 )
		{
			static const Bool isArabic = (Red::StringSearch( SGetCommandLine(), TXT("-arabic") ) != nullptr);
			if ( isArabic )
			{
				// if we have the censored appearance, use it
				CEntityTemplate* entityTemplate = GetEntity()->GetEntityTemplate();
				if ( entityTemplate )
				{
					const CEntityAppearance* appearance = entityTemplate->GetAppearance( GDupaAppearanceCensored2, true );
					if ( appearance )
					{
						// use the censored (Arabic) version
						appearanceName = GDupaAppearanceCensored2;
					}
				}
			}
		}
	}
	// W3 glorious hack end
#endif // W3_ARABIC_CENSORED_VERSION

	if( appearanceName != m_currentAppearance )
	{
		CEntityTemplate* entityTemplate = GetEntity()->GetEntityTemplate();

		if ( entityTemplate )
		{
			// Find appearance in current template
			const CEntityAppearance* appearance = entityTemplate->GetAppearance( appearanceName, true );
			if ( appearance )
			{
				ApplyAppearance( *appearance );
			}
			else
			{
				WARN_ENGINE( TXT("Appearance %s not found in template %s."), appearanceName.AsString().AsChar(), entityTemplate->GetDepotPath().AsChar() );
			}
		}
		else
		{
			WARN_ENGINE( TXT("No template to apply appearance %s."), appearanceName.AsString().AsChar() );
		}
	}
	SAppearanceComponentModifierManager::GetInstance().OnAppearanceChange( this );
}

void CAppearanceComponent::ApplyAppearance( const EntitySpawnInfo& spawnInfo )
{
	if( !spawnInfo.m_appearances.Empty() )
	{
		ApplyAppearance( spawnInfo.m_appearances.Back() );
	}
}

void CAppearanceComponent::RemoveCurrentAppearance( Bool resetAppearanceName /* = true */ )
{
	// Get the entity template where appearances are stored
	CEntityTemplate* entityTemplate = GetEntity()->GetEntityTemplate();
	if ( !entityTemplate )
	{
		return;
	}

	// Destroy attachments created when the previous appearance was applied
	const auto createdTempAttachmentsEndIt = m_createdTempAttachments.End();
	for ( const auto& handle : m_createdTempAttachments )
	{
		if ( handle.Get() )
		{
			handle.Get()->Break();
			handle.Get()->Discard();
		}
	}

	// Cleanup the list
	m_createdTempAttachments.Clear();
	
	// Remove the dynamic templates of the current appearance
	for ( auto& dynamicTemplate : m_dynamicTemplates )
	{
		for ( const auto& component : dynamicTemplate.m_components )
		{
			if ( component.Get() )
			{
				component.Get()->ClearFlag( NF_IncludedFromTemplate );
				component.Get()->OnAppearanceChanged( false );
				GetEntity()->RemoveComponent( component.Get() );
			}
		}
	}

	TDynArray< CComponent* > componentsToRemove;

	const TDynArray<CComponent*>& components = GetEntity()->GetComponents();
	for ( const auto& component : components )
	{
		if ( component->IsUsedInAppearance() )
		{
			componentsToRemove.PushBack( component );
		}
	}

	for ( const auto& componentToRemove : componentsToRemove )
	{
		componentToRemove->ClearFlag( NF_IncludedFromTemplate );
		componentToRemove->OnAppearanceChanged( false );
		GetEntity()->RemoveComponent( componentToRemove );
	}


	// Reset the appearance state to "nothing"
	m_dynamicTemplates.Clear();

	Int32 currentAppearanceAttachmentsIndex  = GetAppearanceAttachmentsIndex( m_currentAppearance );
	if( currentAppearanceAttachmentsIndex != -1)
	{
		m_appearanceAttachments.RemoveAt( currentAppearanceAttachmentsIndex );
	}
	
	if ( resetAppearanceName )
	{
		m_currentAppearance = CName::NONE;
	}
}

void CAppearanceComponent::RemoveAppearanceAttachments( const CName& appearanceName )
{
	// Find and destroy attachments
	for ( auto& it : m_appearanceAttachments )
	{
		if( it.m_appearance == appearanceName )
		{
			m_appearanceAttachments.Erase( &it );
			return;
		}		
	}
}

void CAppearanceComponent::OnInitialized()
{
	TBaseClass::OnInitialized();

	// Apply selected appearance
	if ( !m_initialAppearanceApplied && m_forcedAppearance )
	{
		m_initialAppearance = m_forcedAppearance;
		ApplyAppearance( m_forcedAppearance );
		m_initialAppearanceApplied = true;
	}

#ifndef NO_DATA_VALIDATION
	if ( !m_initialAppearanceApplied &&
		  m_forcedAppearance.Empty() &&
		  GetEntity() && GetEntity()->GetEntityTemplate() &&
		  GetEntity()->GetEntityTemplate()->GetEnabledAppearancesNames().Empty() )
	{
		DATA_HALT( DES_Major, GetEntity()->GetEntityTemplate(), TXT("Entity"), TXT("Entity template has no forced appearance and no appearances ticked, the entity will be spawned without appearance!") );
	}
#endif
}

Bool CAppearanceComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	// Resolve previous appearance as forced appearance name
	if ( propertyName == CNAME( Appearance ) || propertyName == CNAME( appearance ) )
	{
		return readValue.AsType( m_forcedAppearance );
	}

	return false;
}

void CAppearanceComponent::AddAppearanceAttachment( IAttachment* source )
{
	// Ignore broken and external attachments
	if ( source->IsBroken() || source->IsA< CExternalProxyAttachment >() || source->HasFlag( OF_Referenced ) )
	{
		return;
	}

	// Attachments between streamed components are stored in the entity template not in appearance component
	if ( Cast< CComponent >( source->GetParent() )->IsStreamed() || Cast< CComponent >( source->GetChild() )->IsStreamed() )
	{
		return;
	}

	// We only care about attachments where one component is inside this template
	// and the other component is outside - fully inside and fully outside are ignored
	if ( IsComponentPartOfActiveAppearance( Cast< CComponent >( source->GetParent() ) ) == IsComponentPartOfActiveAppearance( Cast< CComponent >( source->GetChild() ) ) )
	{
		return;
	}

	// Get names
	CName parentClass = source->GetParent()->GetClass()->GetName();
	CName parentName = CName( source->GetParent()->GetName() );
	CName childClass = source->GetChild()->GetClass()->GetName();
	CName childName = CName( source->GetChild()->GetName() );

	// Find the proper appearance attachments list
	Int32 localIndex = GetAppearanceAttachmentsIndex( m_currentAppearance );
	if ( localIndex == -1 ) // create new attachments list for the appearance
	{
		localIndex = m_appearanceAttachments.SizeInt();

		m_appearanceAttachments.Grow();
		m_appearanceAttachments.Back().m_appearance = m_currentAppearance;
	}

	// Check if we already have an attachment
	for ( const auto& appearanceAttachment : m_appearanceAttachments[localIndex].m_attachments )
	{
		if ( appearanceAttachment.m_parentClass == parentClass &&
			 appearanceAttachment.m_parentName == parentName &&
			 appearanceAttachment.m_childClass == childClass &&
			 appearanceAttachment.m_childName == childName )
		{
			return;
		}
	}
	
	// Create a clone of the attachment so we can serialize it
	IAttachment* clone = Cast< IAttachment >( source->Clone( nullptr, false, false ) );

	// Break the clone without notifying the parent/child about it
	clone->Nullify();

	// Fill attachment info
	m_appearanceAttachments[localIndex].m_attachments.Grow();
	SAppearanceAttachment& appearanceAttachment = m_appearanceAttachments[localIndex].m_attachments.Back();
	appearanceAttachment.m_attachment = clone;
	appearanceAttachment.m_parentClass = parentClass;
	appearanceAttachment.m_parentName = parentName;
	appearanceAttachment.m_childClass = childClass;
	appearanceAttachment.m_childName = childName;
}

Int32 CAppearanceComponent::GetAppearanceAttachmentsIndex( const CName& appearanceName ) const
{
	// Linear search: i dont expect too many appearances and often switches to justify the
	// overhead of a hashmap, but if this shows up in profiling you know what to do
	const Int32 attachmentsCount = m_appearanceAttachments.SizeInt();
	for ( Int32 i=0; i < attachmentsCount; ++i )
	{
		if ( m_appearanceAttachments[ i ].m_appearance == appearanceName )
		{
			return i;
		}
	}
	return -1;
}

Bool CAppearanceComponent::IsComponentPartOfActiveAppearance( CComponent* component ) const
{
	Bool found = false;
	
	if ( component == nullptr )
	{
		return false;
	}

	for ( const auto& dynamicTemplate : m_dynamicTemplates )
	{
		const auto dynamicTemplateComponentsEndIt = dynamicTemplate.m_components.End();
		for ( auto it = dynamicTemplate.m_components.Begin(); !found && it != dynamicTemplateComponentsEndIt; ++it )
		{
			CComponent* dtComponent = (*it).Get();
			if ( dtComponent == component )
			{
				found = true;
			}
		}
	}
	return found;
}

void CAppearanceComponent::IncludeAppearanceTemplate( CEntityTemplate* entityTemplate, TDynArray< CComponent* >& addedComponents )
{
	PC_SCOPE( CAppearanceComponent IncludeAppearanceTemplate )
	TDynArray< THandle<CResource> > collected;
	TDynArray< CComponent* > locallyAddedComponents;
	TDynArray< IAttachment* > createdAttachments;
	SSavedAttachments savedAttachments;

	//LOG_ENGINE( TXT("[ !! ET !! ]: [%p] Including appearance template '%ls' to entity '%ls'"), this, entityTemplate->GetDepotPath().AsChar(), GetEntity()->GetFriendlyName().AsChar() );
	CEntityTemplate::IncludeEntityTemplate( GetLayer(), GetEntity(), entityTemplate, nullptr, collected, locallyAddedComponents, &savedAttachments, nullptr, true, true );
	// Apply attachments between components in the appearance and components in the entity
	//LOG_ENGINE( TXT("[ !! ET !! ]: [%p] Applying appearance saved attachment to '%ls'"), this, entityTemplate->GetDepotPath().AsChar() );
	m_attachmentReplacements.Apply( savedAttachments );
	//LOG_ENGINE( TXT("[ !! ET !! ]: [%p] Applying local saved attachment to '%ls'"), this, entityTemplate->GetDepotPath().AsChar() );
	savedAttachments.Apply( GetEntity() );
	//LOG_ENGINE( TXT("[ !! ET !! ]: [%p] Done including, processing follows"), this, entityTemplate->GetDepotPath().AsChar() );

	// Push the newly created components to the addedComponents array (which may contain more stuff)
	addedComponents.PushBack( locallyAddedComponents );

	// Destroy broken attachments
	{
		PC_SCOPE( CAppearanceComponent Discard )

		const TDynArray< CComponent* >& components = GetEntity()->GetComponents();
		for ( Int32 i = components.SizeInt() - 1; i >= 0; --i )
		{
			TDynArray< CObject* > children;
			CComponent* component = components[i];
			component->GetChildren( children );

			const auto childrenEndIt = children.End();
			for ( auto it = children.Begin(); it != children.End(); ++it )
			{
				IAttachment* attachment = Cast< IAttachment >( *it );
				if ( attachment != nullptr && ( attachment->IsBroken() || attachment->GetParent() == nullptr || attachment->GetChild() == nullptr ) )
				{
					attachment->Discard();
				}
			}
		}
	}

	// Create dynamic template entry
	SDynamicTemplate dt;
	dt.m_template = entityTemplate;

	// Add the components to the dynamic template entry
	for ( const auto& component : locallyAddedComponents )
	{
		if ( component )
		{
			// Do not create proxy components for appearance components
			component->ClearFlag( OF_Referenced );

			// Mark the component to know that it was included from a template
			component->SetFlag( NF_IncludedFromTemplate );
			component->SetComponentFlag( CF_UsedInAppearance );

			// Put the component in the entry
			dt.m_components.PushBack( component );
		}
	}

	m_dynamicTemplates.PushBack( dt );
}

void CAppearanceComponent::ExcludeAppearanceTemplate( CEntityTemplate* entityTemplate )
{
	for ( Uint32 i = 0; i < m_dynamicTemplates.Size(); i++ )
	{
		if ( m_dynamicTemplates[ i ].m_template == entityTemplate )
		{
			SDynamicTemplate& dt = m_dynamicTemplates[ i ];

			for ( const auto& comp : dt.m_components )
			{
				if ( comp.Get() )
				{
					comp.Get()->ClearFlag( NF_IncludedFromTemplate );
					comp.Get()->OnAppearanceChanged( false );
					GetEntity()->RemoveComponent( comp.Get() );
				}
			}

			m_dynamicTemplates.Erase( m_dynamicTemplates.Begin() + i );
			break;
		}
	}
}

void CAppearanceComponent::PostProcessAddedComponents( TDynArray< CComponent* >& addedComponents )
{
	// Inform all components that they have been included from an appearance
	for ( const auto& component : addedComponents )
	{
		PC_SCOPE( CAppearanceComponent ToggleAppearance )
		component->ForceUpdateTransformNodeAndCommitChanges(); // no transform saved in node any more
		component->OnAppearanceChanged( true );
	}
}

void CAppearanceComponent::GetCurrentAppearanceComponents( TDynArray< CComponent* >& appearanceComponents )
{
	for ( const auto& dt : m_dynamicTemplates )
	{
		for ( const auto& ch : dt.m_components )
		{
			if ( ch.IsValid() )
			{
				appearanceComponents.PushBack( ch.Get() );
			}
		}
	}
}

void CAppearanceComponent::GetCurrentAppearanceTemplates( TDynArray< CEntityTemplate* >& appearanceTemplates )
{
	for ( const auto& dt : m_dynamicTemplates )
	{
		appearanceTemplates.PushBack( dt.m_template );
	}
}

void CAppearanceComponent::CreateStreamingComponentsForActiveAppearance( TDynArray< CComponent* >& createdComponents )
{
	for ( auto& dt : m_dynamicTemplates )
	{	
		// TODO: cleanup invalid handles here
		if ( dt.m_template->GetEntityObject() != nullptr && dt.m_template->GetEntityObject()->GetLocalStreamedComponentDataBuffer().GetSize() > 0 )
		{
			const Uint32 base = createdComponents.Size();
			TDynArray< CComponent* > componentsCreatedByThisTemplate;
			const auto& dataBuffer = dt.m_template->GetEntityObject()->GetLocalStreamedComponentDataBuffer();

			// Create the components from this dynamic template
			CEntityTemplate::IncludeComponents( GetLayer(), GetEntity(), (Uint8*)dataBuffer.GetData(), dataBuffer.GetSize(), componentsCreatedByThisTemplate, nullptr, true );

			// Create components from included templates
			struct
			{
				void IncludeComponents( CLayer* layer, CEntity* entity, CEntityTemplate* entityTemplate, TDynArray< CComponent* >& createdComponents )
				{
					if ( entityTemplate->GetEntityObject() != nullptr && entityTemplate->GetEntityObject()->GetLocalStreamedComponentDataBuffer().GetSize() > 0 )
					{
						const auto& dataBuffer = entityTemplate->GetEntityObject()->GetLocalStreamedComponentDataBuffer();

						TDynArray< CComponent* > componentsCreatedByThisTemplate;
						// Create the components from this dynamic template
						CEntityTemplate::IncludeComponents( layer, entity, (Uint8*)dataBuffer.GetData(), dataBuffer.GetSize(), componentsCreatedByThisTemplate, nullptr, true );
						createdComponents.PushBack( componentsCreatedByThisTemplate );

						// Create attachments for this template
						TDynArray< IAttachment* > createdAttachments;
						entity->CreateStreamedAttachmentsForNewComponentsUsingTemplate( entityTemplate, componentsCreatedByThisTemplate, createdAttachments );
					}

					// Process includes
					const auto& includes = entityTemplate->GetIncludes();
					for ( const auto& it : includes )
					{
						CEntityTemplate* inc = it.Get();
						if ( inc != nullptr )
						{
							IncludeComponents( layer, entity, inc, createdComponents );
						}
					}
				}
			} local;

			// Process includes
			const auto& includes = dt.m_template->GetIncludes();
			for ( const auto& it : includes )
			{
				CEntityTemplate* inc = it.Get();
				if ( inc != nullptr )
				{
					local.IncludeComponents( GetLayer(), GetEntity(), inc, componentsCreatedByThisTemplate );
				}
			}

			createdComponents.PushBack( componentsCreatedByThisTemplate );

			// Put the components back in the dynamic template and set component flags
			const Uint32 createdComponentsCount = createdComponents.Size();
			for ( Uint32 i = base; i < createdComponentsCount; ++i )
			{
				CComponent* component = createdComponents[ i ];
				component->SetComponentFlag( CF_UsedInAppearance );
				component->SetComponentFlag( CF_StreamedComponent );
				dt.m_components.PushBack( component );
			}

			// Create streaming attachments for this template
			TDynArray< IAttachment* > createdAttachments;
			GetEntity()->CreateStreamedAttachmentsForNewComponentsUsingTemplate( dt.m_template, componentsCreatedByThisTemplate, createdAttachments );
		}
	}
}

Bool CAppearanceComponent::HasTagInAppearanceEntities( const CName& tag ) const
{
	for ( const auto& dt : m_dynamicTemplates )
	{
		if ( dt.m_template->GetEntityObject()->HasTag( tag ) )
		{
			return true;
		}
	}
	return false;
}

void CAppearanceComponent::GetInstancePropertyNames( TDynArray< CName >& instancePropertyNames ) const
{
	TBaseClass::GetInstancePropertyNames( instancePropertyNames );
	// Note: we also add the old Appearance here for any resource that still refers to that property
	instancePropertyNames.PushBackUnique( CNAME( Appearance ) );
	instancePropertyNames.PushBackUnique( CNAME( forcedAppearance ) );
}

CAppearanceComponent* CAppearanceComponent::GetAppearanceComponent( const CEntity* entity )
{
	if ( !entity )
	{
		return NULL;
	}

	return entity->FindComponent<CAppearanceComponent>();
}

// Hack, see EntityTemplate::CaptureData in entityTemplateCook.cpp why
Bool GLockAppearanceComponentAttachments;

void CAppearanceComponent::UpdateCurrentAppearanceAttachments()
{
	if ( GLockAppearanceComponentAttachments )
	{
		return;
	}

	// We need a real appearance
	if ( m_currentAppearance == CName::NONE )
	{
		return;
	}

	// Find and remove appearance list
	const Int32 localIndex = GetAppearanceAttachmentsIndex( m_currentAppearance );
	if ( localIndex != -1 )
	{
		m_appearanceAttachments.RemoveAt( localIndex );
	}

	// Re-add all attachments
	const TDynArray< CComponent* >& components = GetEntity()->GetComponents();
	for ( const auto& component : components )
	{
		// We only care about components that come appearances
		// WARNING: this will also return true for included components!
		if ( !component->IsUsedInAppearance() )
		{
			continue;
		}

		// Make sure the component is really from this appearance
		if ( !IsComponentPartOfActiveAppearance( component ) )
		{
			continue;
		}

		// Get all attachments
		const TList< IAttachment* >& parentAttachments = component->GetParentAttachments();
		for ( const auto& attachment : parentAttachments )
		{
			AddAppearanceAttachment( attachment );
		}

		const TList< IAttachment* >& childAttachments = component->GetChildAttachments();
		for ( const auto& attachment : childAttachments )
		{
			AddAppearanceAttachment( attachment );
		}
	}
}

void CAppearanceComponent::RemoveAppearanceAttachment( IAttachment* attachment )
{
	// Find and destroy attachments
	for ( auto& it : m_appearanceAttachments )
	{
		it.RemoveAppearanceAttachment( attachment );
	}
}

void CAppearanceComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// For GC also serialize the IAttachment objects in appearance attachment lists
	if ( file.IsGarbageCollector() )
	{
		for ( auto& dt : m_dynamicTemplates )
		{
			file << dt.m_template;
		}

		for ( auto& appearanceAttachments : m_appearanceAttachments )
		{
			for ( auto& appearanceAttachment : appearanceAttachments.m_attachments )
			{
				if ( appearanceAttachment.m_attachment != nullptr )
				{
					file << appearanceAttachment.m_attachment;
				}
			}
		}
	}
}

void CAppearanceComponent::OnSaveGameplayState( IGameSaver* saver )
{
	saver->WriteValue( CNAME(appearance), m_currentAppearance );
}

void CAppearanceComponent::OnLoadGameplayState( IGameLoader* loader )
{
	CName appearance;
	loader->ReadValue< CName >( CNAME(appearance), appearance );
	m_initialAppearanceApplied = true;
	m_initialAppearanceLoaded = true;
	ApplyAppearance( appearance );
}

void CAppearanceComponent::OnFinalize()
{
	SAppearanceComponentModifierManager::GetInstance().AppearanceComponentOnFinalize( this );
}

void CAppearanceComponent::OnDetachFromEntityTemplate()
{
	SAppearanceComponentModifierManager::GetInstance().AppearanceComponentOnFinalize( this );
}

void CAppearanceComponent::funcApplyAppearance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, appearanceName, String::EMPTY );
	FINISH_PARAMETERS;

	CName name( appearanceName );
	if ( name != CName::NONE )
	{
		ApplyAppearance( name );
	}
}

void CAppearanceComponent::funcGetAppearance( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetAppearance() );
}

void CAppearanceComponent::funcIncludeAppearanceTemplate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, THandle< CEntityTemplate >() );
	FINISH_PARAMETERS;
	TDynArray< CComponent* > addedComponents;
	IncludeAppearanceTemplate( entityTemplate.Get(), addedComponents );
	PostProcessAddedComponents( addedComponents );
}

void CAppearanceComponent::funcExcludeAppearanceTemplate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, THandle< CEntityTemplate >() );
	FINISH_PARAMETERS;
	ExcludeAppearanceTemplate( entityTemplate.Get() );
}
