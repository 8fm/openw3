/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityTemplate.h"
#include "fxSystem.h"
#include "entityTemplateParams.h"
#include "animMimicParam.h"
#include "appearanceComponent.h"
#include "../core/dependencySaver.h"
#include "../core/scriptStackFrame.h"
#include "../core/scriptSnapshot.h"
#include "../core/objectIterator.h"
#include "../core/memoryFileReader.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/physics/physicsWorld.h"
#include "../core/dataError.h"
#include "hardAttachment.h"
#include "behaviorGraph.h"
#include "externalProxy.h"
#include "skeletonProvider.h"
#include "componentIterator.h"
#include "boundedComponent.h"
#include "appearanceComponent.h"
#include "../core/events.h"
#include "layer.h"
#include "dynamicLayer.h"
#include "world.h"
#include "entity.h"
#include "entityTemplateModifier.h"

IMPLEMENT_ENGINE_CLASS( VoicetagAppearancePair );
IMPLEMENT_ENGINE_CLASS( SAttachmentReplacement );
IMPLEMENT_ENGINE_CLASS( SAttachmentReplacements );
IMPLEMENT_ENGINE_CLASS( SStreamedAttachment );
IMPLEMENT_ENGINE_CLASS( SEntityTemplateColoringEntry );
IMPLEMENT_ENGINE_CLASS( SEntityTemplateOverride );
IMPLEMENT_ENGINE_CLASS( CEntityTemplate );
IMPLEMENT_ENGINE_CLASS( EntitySlot );
IMPLEMENT_ENGINE_CLASS( SComponentInstancePropertyEntry );

RED_DEFINE_NAME( voicetagAppearances )
RED_DEFINE_NAME( transformParent );
RED_DEFINE_NAME( boundingBox );
RED_DEFINE_NAME( graphPositionX );
RED_DEFINE_NAME( graphPositionY );

#ifndef NO_DATA_VALIDATION
namespace Config
{
	TConfigVar<Bool> cvValidateStreamingFlag( "Streaming", "ValidateStreamingFlag", true );
}
#endif

//////////////////////////////////////////////////////////////////////////

SAttachmentReplacement::SAttachmentReplacement(){}

//////////////////////////////////////////////////////////////////////////

void SAttachmentReplacements::Add( CComponent* oldComponent, CComponent* newComponent )
{
	CName oldComponentName( oldComponent->GetName() );
	CName newComponentName( newComponent->GetName() );

	// Scan to see if there is already a replacement for the old component
	for ( auto it=m_replacements.Begin(); it != m_replacements.End(); ++it )
	{
		SAttachmentReplacement& replacement = *it;
		if ( replacement.m_oldClass == oldComponent->GetClass()->GetName() && replacement.m_oldName == oldComponentName )
		{
			// Just update the replacement
			replacement.m_newName = newComponentName;
			replacement.m_newClass = newComponent->GetClass()->GetName();
			//LOG_ENGINE( TXT("[ !! ET !! ]: Updated component replacement from '%ls' to '%ls'"), oldComponent->GetName().AsChar(), newComponent->GetName().AsChar() );
			return;
		}
	}

	// No existing replacement, add new
	m_replacements.Grow();
	m_replacements.Back().m_oldName = oldComponentName;
	m_replacements.Back().m_oldClass = oldComponent->GetClass()->GetName();
	m_replacements.Back().m_newName = newComponentName;
	m_replacements.Back().m_newClass = newComponent->GetClass()->GetName();
	//LOG_ENGINE( TXT("[ !! ET !! ]: Registered replacement from '%ls' to '%ls'"), oldComponent->GetName().AsChar(), newComponent->GetName().AsChar() );
}

void SAttachmentReplacements::Apply( struct SSavedAttachments& savedAttachments )
{
	PC_SCOPE( SAttachmentReplacements Apply )
	// Apply replacements
	for ( auto it=m_replacements.Begin(); it != m_replacements.End(); ++it )
	{
		const SAttachmentReplacement& replacement = *it;

		Bool applied = false;

		for ( Int32 i=savedAttachments.m_attachments.SizeInt() - 1; i >= 0; --i )
		{
			SSavedAttachments::SSavedAttachment& attachment = savedAttachments.m_attachments[i];
			if ( attachment.m_parentName == replacement.m_oldName && attachment.m_parentClass == replacement.m_oldClass )
			{
				//LOG_ENGINE( TXT("[ !! ET !! ]: Replaced parent component from '%ls' to '%ls'"), replacement.m_oldName.AsString().AsChar(), replacement.m_newName.AsString().AsChar() );
				attachment.m_parentName = replacement.m_newName;
				attachment.m_parentClass = replacement.m_newClass;
				applied = true;
			}
			else if ( attachment.m_childName == replacement.m_oldName && attachment.m_childClass == replacement.m_oldClass )
			{
				//LOG_ENGINE( TXT("[ !! ET !! ]: Replaced child component from '%ls' to '%ls'"), replacement.m_oldName.AsString().AsChar(), replacement.m_newName.AsString().AsChar() );
				attachment.m_childName = replacement.m_newName;
				attachment.m_childClass = replacement.m_newClass;
				applied = true;
			}

			// In some rare cases, the replacements cause an attachment to point to the same object.
			// This can happen if an appearance had an included template that was deleted and there
			// were attachments with references to that template that were supposed to be replaced
			// in a previous "pass" of this function but weren't so we have wrong (reused) names
			if ( attachment.m_childName == attachment.m_parentName && attachment.m_childClass == attachment.m_parentClass )
			{
				attachment.m_attachmentCopy->RemoveFromRootSet();
				savedAttachments.m_attachments.RemoveAt( i );
			}
		}

		if ( ! applied )
		{
			//LOG_ENGINE( TXT("[ !! ET !! ]: [!!] Failed to replace component from '%ls' to '%ls'"), replacement.m_oldName.AsString().AsChar(), replacement.m_newName.AsString().AsChar() );
			//LOG_ENGINE( TXT("[ !! ET !! ]:      [II] Attachment dump:") );
			for ( Int32 i=savedAttachments.m_attachments.SizeInt() - 1; i >= 0; --i )
			{
				SSavedAttachments::SSavedAttachment& attachment = savedAttachments.m_attachments[i];
			    //LOG_ENGINE( TXT("[ !! ET !! ]:      [--] From '%s %s' to '%s %s'"),
				//	attachment.m_parentClass.AsChar(), attachment.m_parentName.AsChar(),
				//	attachment.m_childClass.AsChar(), attachment.m_childName.AsChar() );
			}
			//LOG_ENGINE( TXT("[ !! ET !! ]:      [II] Dump ended") );
		}
		else
		{
			//LOG_ENGINE( TXT("[ !! ET !! ]: ---OK---") );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

SSavedAttachments::~SSavedAttachments()
{
	// Release unused attachment copies from the root set
	for ( auto it=m_attachments.Begin(); it != m_attachments.End(); ++it )
	{
		SSavedAttachment& attachment = *it;
		if ( attachment.m_attachmentCopy )
		{
			attachment.m_attachmentCopy->RemoveFromRootSet();
			attachment.m_attachmentCopy->Discard();
		}
	}
}

void SSavedAttachments::Add( IAttachment* attachment )
{
	// Get information about parent and child
	CNode* parent = attachment->GetParent();
	CNode* child = attachment->GetChild();
	IAttachment* attachmentCopy;
	if ( !parent || !child )
	{
		if ( parent ) 
		{
		    //LOG_ENGINE( TXT("[ !! ET !! ]: [!!] Partial attachment FROM %s '%ls'"), parent->GetClass()->GetName().AsChar(), parent->GetName().AsChar() );
		}
		if ( child ) 
		{
			//LOG_ENGINE( TXT("[ !! ET !! ]: [!!] Partial attachment  TO  %s '%ls'"), child->GetClass()->GetName().AsChar(), child->GetName().AsChar() );
		}
		return;
	}

	ASSERT( parent != child, TXT("This is wrong: the attachment cannot have the child and parent being the same object!") );

	//LOG_ENGINE( TXT("[ !! ET !! ]: Saved attachment from '%ls' to '%ls'"), parent->GetName().AsChar(), child->GetName().AsChar() );

	// Convert names to CName
	CName parentName( parent->GetName().AsChar() );
	CName childName( child->GetName().AsChar() );

	// Check if the attachment already exists
	for ( auto it=m_attachments.Begin(); it != m_attachments.End(); ++it )
	{
		SSavedAttachment& attachment = *it;
		if ( attachment.m_parentName == parentName && attachment.m_parentClass == parent->GetClass()->GetName() &&
			 attachment.m_childName == childName && attachment.m_childClass == child->GetClass()->GetName() )
		{
			return;
		}
	}

	// Create copy and extract parent GUID from external attachment proxies
	if ( attachment->IsA< CExternalProxyAttachment >() )
	{
		IAttachment* original = static_cast< CExternalProxyAttachment* >( attachment )->GetOriginalLink();

		// Make sure the external proxy attachment points to an attachment
		if ( original )
		{
			Bool wasReferenced = original->HasFlag( OF_Referenced );
			original->ClearFlag( OF_Referenced );
			attachmentCopy = static_cast< IAttachment* >( original->Clone( NULL, false, false ) );

			if ( wasReferenced )
				original->SetFlag( OF_Referenced );
		}
		else
		{
			return;
		}
	}
	else
	{
		Bool wasReferenced = attachment->HasFlag( OF_Referenced );
		attachment->ClearFlag( OF_Referenced );
		attachmentCopy = static_cast< IAttachment* >( attachment->Clone( NULL, false, false ) );

		if ( wasReferenced )
			attachment->SetFlag( OF_Referenced );
	}

	// Add attachment copy to root set to avoid GC messing up with it
	ASSERT( attachmentCopy, TXT("Somehow, this managed to fail. Attachment copy is NULL and it shouldn't be. Expect crash.") );
	attachmentCopy->AddToRootSet();

	// Remove attachment references to avoid bugs from invalid references
	// (they will be recreated when saved attachments are applied)
	attachmentCopy->m_parent = NULL;
	attachmentCopy->m_child = NULL;

	// Not found, add new
	m_attachments.Grow();
	m_attachments.Back().m_attachmentCopy = attachmentCopy;
	m_attachments.Back().m_parentName = parentName;
	m_attachments.Back().m_parentClass = parent->GetClass()->GetName();
	m_attachments.Back().m_childName = childName;
	m_attachments.Back().m_childClass = child->GetClass()->GetName();
}

void SSavedAttachments::Add( const TList< IAttachment* >& attachments )
{
	for ( auto it=attachments.Begin(); it != attachments.End(); ++it )
	{
		Add( *it );
	}
}

void SSavedAttachments::Add( CComponent* component )
{
	Add( component->GetChildAttachments() );
	Add( component->GetParentAttachments() );
}

void SSavedAttachments::Add( CEntity* entity )
{
	const TDynArray< CComponent* >& components = entity->GetComponents();
	for ( auto it=components.Begin(); it != components.End(); ++it )
	{
		Add( *it );
	}
}

void SSavedAttachments::AddReplacement( CComponent* oldComponent, CComponent* newComponent )
{
	m_replacements.Add( oldComponent, newComponent );
}

void SSavedAttachments::Apply( CEntity* entity )
{
	// Apply replacements
	m_replacements.Apply( *this );

	// Apply attachments
	for ( auto it=m_attachments.Begin(); it != m_attachments.End(); ++it )
	{
		SSavedAttachment& attachment = *it;
		IAttachment* attachmentCopy = attachment.m_attachmentCopy;
		CComponent* parentComponent = entity->FindComponent( attachment.m_parentName, attachment.m_parentClass );
		CComponent* childComponent = entity->FindComponent( attachment.m_childName, attachment.m_childClass );

		ASSERT( !parentComponent || !childComponent || parentComponent != childComponent, TXT("Fatal error with attachment: parent and child both point to the same node!") );
		// Remove attachment from root set
		attachmentCopy->RemoveFromRootSet();

		// Only create the attachment if both parent and child are here and there isn't already an attachment
		if ( parentComponent && childComponent && !parentComponent->HasChild( childComponent, attachmentCopy->GetClass() ) )
		{
			//LOG_ENGINE( TXT("[ !! ET !! ]: Creating attachment from '%ls' to '%ls'"), parentComponent->GetName().AsChar(), childComponent->GetName().AsChar() );
			if ( attachmentCopy->Init( parentComponent, childComponent, NULL ) )
			{
				if ( attachmentCopy->GetParent() == parentComponent && attachmentCopy->GetChild() == childComponent )
				{
					attachmentCopy->SetParent( parentComponent );
				}
				else
				{
					attachmentCopy->Discard();
				}
			}
			else
			{
				attachmentCopy->Discard();
			}
		}
		else
		{
			attachmentCopy->Discard();
			if ( parentComponent && childComponent )
			{
				//LOG_ENGINE( TXT("[ !! ET !! ]: Attachment from '%ls' to '%ls' already exists"), attachment.m_parentName.AsString().AsChar(), attachment.m_childName.AsString().AsChar() );
			}
			else
			{
				//LOG_ENGINE( TXT("[ !! ET !! ]: [!!] Failed to make attachment from '%ls' to '%ls'"), attachment.m_parentName.AsString().AsChar(), attachment.m_childName.AsString().AsChar() );
			}
		}
		attachment.m_attachmentCopy = NULL;
	}
	m_attachments.Clear();
}

//////////////////////////////////////////////////////////////////////////

Bool SStreamedAttachment::InitFromAttachment( IAttachment* attachment )
{
	// Reset values
	m_parentName = m_parentClass = m_childName = m_childClass = CName::NONE;
	m_data.Clear();

	// Make sure we have a proper attachment
	if ( attachment == nullptr || attachment->IsBroken() || attachment->HasFlag( OF_Referenced ) )
	{
		return false;
	}

	// Get parent and child
	CNode* parent = attachment->GetParent();
	CNode* child = attachment->GetChild();

	// Paranoia test (IsBroken should return false in the case above, but crashes suck, so...)
	if ( parent == nullptr || child == nullptr )
	{
		return false;
	}

	// Fill names
	m_parentName = CName( parent->GetName() );
	m_parentClass = parent->GetClass()->GetName();
	m_childName = CName( child->GetName() );
	m_childClass = child->GetClass()->GetName();

	// Serialize attachment
	CMemoryFileWriter writer( m_data );
	CDependencySaver saver( writer, nullptr );
	DependencySavingContext context( attachment );
	return saver.SaveObjects( context );
}

Bool SStreamedAttachment::CreateInEntity( CEntity* entity, IAttachment*& attachment, const TDynArray< CComponent* >* limitToComponents ) const
{
#ifndef RED_FINAL_BUILD
	// Make sure we have proper data
	if ( m_parentName.Empty() || m_parentClass.Empty() || m_childName.Empty() || m_childClass.Empty() || m_data.Empty() )
	{
		return false;
	}
#endif

	// Find the parent and child components in the entity
	CComponent* parent = entity->FindComponent( m_parentName, m_parentClass );
	CComponent* child = entity->FindComponent( m_childName, m_childClass );
	if ( parent == nullptr || child == nullptr )
	{
		return false;
	}

	// If we have a limited set of components to apply to, check if the parent or child is part of it
	if ( limitToComponents != nullptr && !limitToComponents->Exist( parent ) && !limitToComponents->Exist( child ) )
	{
		return false;
	}

	// Unserialize the attachment object
	CMemoryFileReader reader( m_data.TypedData(), m_data.Size(), 0 );
	CDependencyLoader loader( reader, NULL );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = entity;
	if ( loader.LoadObjects( loadingContext ) )
	{
		// Post load them
		loader.PostLoad();

		// Check if we actually got something
		if ( loadingContext.m_loadedRootObjects.Empty() )
		{
			return false;
		}

		// Get the attachment object
#ifndef RED_FINAL_BUILD
		attachment = Cast<IAttachment>( loadingContext.m_loadedRootObjects[0] );
#else
		attachment = static_cast<IAttachment*>( loadingContext.m_loadedRootObjects[0] );
#endif

		// Initialize it using the parent and child
		if ( !attachment->Init( parent, child, nullptr ) )
		{
			attachment->Discard();
			return false;
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

static bool operator==( const SStreamedAttachment& a, const SStreamedAttachment& b )
{
	return a.m_parentName == b.m_parentName &&
		   a.m_parentClass == b.m_parentClass &&
		   a.m_childName == b.m_childName &&
		   a.m_childClass == b.m_childClass;
}

static void RemoveDuplicateStreamedAttachments( TDynArray< SStreamedAttachment >& attachments )
{
	// Create a hashset with all attachments
	THashSet< SStreamedAttachment > set;
	for ( auto it=attachments.Begin(); it != attachments.End(); ++it )
	{
		set.Insert( *it );
	}

	// Recreate array from the set
	attachments.Clear();
	attachments.Reserve( set.Size() );
	for ( auto it=set.Begin(); it != set.End(); ++it )
	{
		attachments.PushBack( *it );
	}
}

//////////////////////////////////////////////////////////////////////////

Uint32 CEntityTemplate::s_cookedEffectsCurrentVersion = 1;

CEntityTemplate::CEntityTemplate()
	:	m_entityClass( CNAME( CEntity ) )
	,   m_effectsPreloaded( new CEntityTemplatePreloadedEffects() )
	,	m_cookedEffectsVersion( 0 )
{
}

CEntityTemplate::~CEntityTemplate()
{
	m_effectsPreloaded->Release();
}

Bool CEntityTemplate::ShouldEmbedResource() const
{
	// Entity templates are never embedded
	return false;
}

void CEntityTemplate::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		m_effectsPreloaded->CollectForGC( file );
		return;
	}

	if ( file.GetVersion() >= VER_STREAMED_ENTITY_TEMPLATE )
	{
		// TODO: remove this
		Uint32 arraySize = 0;
		file << arraySize;

		for ( Uint32 i = 0; i < arraySize; ++i )
		{
			DataBuffer ignoreMePlease;
			ignoreMePlease.Serialize( file );
		}
	}

	if ( file.GetVersion() < VER_FIX_GET_TIME_FOR_CACHED_ENTITY_BUFFERS )
	{
		// Bug in GetUTCTime() caused m_dataCompilationTime to essentially get garbage, so for anything before it was fixed,
		// we'll need to recompile the cached data.
		m_dataCompilationTime.Clear();
		m_flatCompiledData.Clear();
	}

	// Special cooked data
	if ( IsCooked() )
	{
		if( m_cookedEffectsVersion < s_cookedEffectsCurrentVersion )
		{
			TDynArray< CEntityTemplateCookedEffectLegacy >	cookedEffectsLegacy;	// For backward compatibility

			file << cookedEffectsLegacy;
			DataBuffer cookedEffectBuffer;
			cookedEffectBuffer.Serialize( file );

			for( CEntityTemplateCookedEffectLegacy& effectLegacy : cookedEffectsLegacy )
			{
				void* data = (void*)(((Uint64)cookedEffectBuffer.GetData()) + (Uint64)effectLegacy.m_offset);
				SharedDataBuffer sharedBuffer( data, effectLegacy.m_size );
				CEntityTemplateCookedEffect newCookedEffect( effectLegacy.m_name, effectLegacy.m_animName, sharedBuffer );
				m_cookedEffects.PushBack( newCookedEffect );
			}
		}
	}
}

#ifndef NO_EDITOR

void CEntityTemplate::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Change entity
	if ( property->GetName() == CNAME( entityClass ) )
	{
		CClass* entityClass = ( CClass* ) SRTTI::GetInstance().FindClass( m_entityClass );
		if ( entityClass )
		{
			SetEntityClass( entityClass );
		}
	}
	// Change includes
	if ( property->GetName() == CNAME( includes ) )
	{
		THandle< CEntityTemplate > thisHandle( this );

		// Do not allow including entity template into itself!
		for ( Uint32 i=0; i<m_includes.Size(); i++ )
		{
			if ( m_includes[i] == thisHandle )
			{
				WARN_ENGINE( TXT("'%ls' tried to include itself!"), GetFriendlyName().AsChar() );
				m_includes[i] = NULL;
			}
		}
	}
}

#endif

void CEntityTemplate::OnScriptReloaded()
{
	// Pass to base class
	TBaseClass::OnScriptReloaded();

	// Discard cached entity template data
	if ( !IsCooked() )
	{
		m_flatCompiledData.Clear();
	}
}

#ifndef NO_EDITOR

void CEntityTemplate::SetEntityClass( CClass* entityClass )
{
	ASSERT( entityClass );
	ASSERT( entityClass->IsA( ClassID< CEntity >() ) );
	ASSERT( !entityClass->IsAbstract() );
	ASSERT( !IsCooked() );

	// Cannot change on cooked entity
	if ( IsCooked() )
		return;

	// Class changing
	EDITOR_DISPATCH_EVENT( CNAME( EntityTemplateClassChanging ), CreateEventData( this ) );

	// Get current entity components
	if ( m_entityObject )
	{
		// Snapshot properties
		CScriptSnapshot snapshot;
		CScriptSnapshot::ScriptableSnapshot *snap = snapshot.BuildEditorObjectSnapshot( m_entityObject );

		// Get components
		TDynArray< CComponent* > components = m_entityObject->GetComponents();
		m_entityObject->m_components.Clear();

		// Create new entity object
		CEntity* newEntity = CreateObject< CEntity >( entityClass );
		bool streamed = m_entityObject->ShouldBeStreamed();
		newEntity->m_components = components;

		// Copy streaming data buffers
		if ( streamed )
		{
			newEntity->m_streamingDataBuffer = m_entityObject->m_streamingDataBuffer;
		}

		// Reattach component to new entity
		for ( Uint32 i=0; i<components.Size(); i++ )
		{
			CComponent* component = components[i];
			component->SetParent( newEntity );
		}
		
		// Restore properties
		snapshot.RestoreEditorObjectSnapshot( newEntity, snap );

		// Use the previous value for streaming
		newEntity->SetStreamed( streamed );

		delete snap;

		// Save
		CaptureData( newEntity );
	}
	else
	{
		// Create new entity object
		CEntity* newEntity = CreateObject< CEntity >( entityClass );
		CaptureData( newEntity );
	}
}

#endif

Bool CEntityTemplate::IsDetachable() const
{
	// We have effects :(
	if ( !m_effects.Empty() || !m_cookedEffects.Empty() )
		return false;

	// We have appearances :(
	if ( !m_appearances.Empty() )
		return false;

	// We have template parameters
	if ( !m_templateParams.Empty() )
		return false;

	// We have slots
	if ( !m_slots.Empty() )
		return false;

	// We cannot detach if we have streaming attachments
	// THIS IS WASTING SO MUCH MEMORY :(
	if ( !m_streamedAttachments.Empty() )
		return false;

	// Check includes
	for ( const THandle< CEntityTemplate >& it : m_includes )
	{
		if ( it.IsValid() && !it->IsDetachable() )
			return false;
	}

	// We can detach
	return true;
}

#ifndef NO_RESOURCE_COOKING

namespace Helper
{
	static Bool CompareStreamingAttachment( const SStreamedAttachment& a, const SStreamedAttachment& b )
	{
		{
			const int cmp = Red::StringCompare( a.m_parentName.AsChar(), b.m_parentName.AsChar() );
			if ( cmp < 0 ) return true;
			if ( cmp > 0 ) return false;
		}

		{
			const int cmp = Red::StringCompare( a.m_childName.AsChar(), b.m_childName.AsChar() );
			if ( cmp < 0 ) return true;
			if ( cmp > 0 ) return false;
		}

		{
			const int cmp = Red::StringCompare( a.m_parentClass.AsChar(), b.m_parentClass.AsChar() );
			if ( cmp < 0 ) return true;
			if ( cmp > 0 ) return false;
		}

		{
			const int cmp = Red::StringCompare( a.m_childClass.AsChar(), b.m_childClass.AsChar() );
			if ( cmp < 0 ) return true;
			if ( cmp > 0 ) return false;
		}

		// equal is still not less
		return false;
	}
};

void CEntityTemplate::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	// sort the magical streaming attachments
	::Sort( m_streamedAttachments.Begin(), m_streamedAttachments.End(), Helper::CompareStreamingAttachment );
}
#endif

void CEntityTemplate::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Entity is cooked and has no flattened data - WTF ?
	if ( IsCooked() && m_flatCompiledData.Empty() )
	{
		ERR_ENGINE( TXT("Entity '%ls' is cooked but has no cooked data - it will not work.") );
	}

	// MattH: Since the entity object is NOT an instance (in the sense that it should not be referenced outside its template)
	//, we set the GUI to zero
	if( m_entityObject )
	{
		m_entityObject->SetGUID( CGUID::ZERO );
	}
	
	// Remove duplicate streamed attachments (this can be removed after resaves)
#ifndef NO_EDITOR
	RemoveDuplicateStreamedAttachments( m_streamedAttachments );
#endif

	// Rebuild overrides for existing (broken) data
	if ( !m_properOverrides && m_entityObject )
	{
		RebuildPropertyOverridesFromIncludes( m_entityObject, false );
		m_properOverrides = true;
	}

	//! Not imported already 
	if ( m_voicetagAppearances.Empty() == true )
	{
		TDynArray< const CEntityAppearance* > usedAppearances;
		GetAllEnabledAppearances( usedAppearances );
		for ( TDynArray< const CEntityAppearance* >::const_iterator appearanceIter = usedAppearances.Begin(); appearanceIter != usedAppearances.End(); ++appearanceIter )
		{
			const CEntityAppearance* appearance = *appearanceIter;
			m_voicetagAppearances.PushBack( VoicetagAppearancePair( appearance->GetVoicetag(), appearance->GetName() ) );
		}
	}

#ifndef NO_RESOURCE_COOKING
	// Restore local data buffer
	CompileDataBuffer();
#endif
	SEntityTemplateModifierManager::GetInstance().EntityTemplateOnPostLoad( this );
}

#ifndef NO_EDITOR
void CEntityTemplate::ResetEntityTemplateData()
{
	// Kill entity state
	m_entityObject->DestroyStreamedComponents( SWN_DoNotNotifyWorld );
	m_entityObject->DisableMaterialReplacement();

	// Removing streaming data
	m_entityObject->m_streamingDataBuffer.Clear();
	m_entityObject->m_streamingComponents.Clear();
	m_entityObject->m_streamingAttachments.Clear();
	m_streamedAttachments.Clear();

	// Remove template data
	m_bodyParts.Clear();
	m_appearances.Clear();
	m_usedAppearances.Clear();
	m_voicetagAppearances.Clear();
	m_cookedEffects.Clear();
	m_slots.Clear();
	m_coloringEntries.Clear();
	m_instancePropEntries.Clear();
	m_effects.Clear();
	m_templateParams.Clear();
	m_entityObject->DestroyAllComponents();
	m_entityObject->m_components.Clear();

	// Remove includes
	m_includes.Clear();
	m_backgroundOffset = Vector::ZEROS;
	m_properOverrides = false;
	m_overrides.Clear();
	m_flatCompiledData.Clear();

	// Recompile data buffer
	CompileDataBuffer();

	// Inform editor
	EDITOR_DISPATCH_EVENT( CNAME( EntityTemplateModified ), CreateEventData( this ) );
}

void CEntityTemplate::UpdateStreamedAttachmentsFromEntity( CEntity* entity, const TDynArray< CComponent* >& streamedComponents )
{
	// Remove existing attachments
	m_streamedAttachments.Clear();

	// Create new attachments for the streamed components
	for ( auto it=streamedComponents.Begin(); it != streamedComponents.End(); ++it )
	{
		CComponent* component = *it;

		// Grab child attachments
		const TList< IAttachment* >& cattachments = component->GetChildAttachments();
		for ( auto it=cattachments.Begin(); it != cattachments.End(); ++it )
		{
			m_streamedAttachments.Grow();
			if ( !m_streamedAttachments.Back().InitFromAttachment( *it ) )
			{
				m_streamedAttachments.PopBack();
			}
		}

		// Grab parent attachments
		const TList< IAttachment* >& pattachments = component->GetParentAttachments();
		for ( auto it=pattachments.Begin(); it != pattachments.End(); ++it )
		{
			m_streamedAttachments.Grow();
			if ( !m_streamedAttachments.Back().InitFromAttachment( *it ) )
			{
				m_streamedAttachments.PopBack();
			}
		}
	}

	// Remove any duplicates we collected
	RemoveDuplicateStreamedAttachments( m_streamedAttachments );
}
#endif // NO_EDITOR

void CEntityTemplate::CreateStreamedAttachmentsInEntity( CEntity* entity, const TDynArray< CComponent* >& newComponents, TDynArray< IAttachment* >& createdAttachments ) const
{
	Bool toplevelCall = createdAttachments.Empty();

	// Create locally stored attachments
	for ( auto it=m_streamedAttachments.Begin(); it != m_streamedAttachments.End(); ++it )
	{
		const SStreamedAttachment& attachment = *it;
		IAttachment* attachmentObject = nullptr;
		if ( (*it).CreateInEntity( entity, attachmentObject, &newComponents ) )
		{
			if ( attachmentObject != nullptr )
			{
				createdAttachments.PushBack( attachmentObject );
			}
		}
	}

	// Mark where the attachments we created ourselves begin
	Uint32 base = createdAttachments.Size();

	// Create streamed attachments from includes
	for ( auto it=m_includes.Begin(); it != m_includes.End(); ++it )
	{
		CEntityTemplate* inc = (*it).Get();
		if ( inc != nullptr )
		{
			inc->CreateStreamedAttachmentsInEntity( entity, newComponents, createdAttachments );
		}
	}

	// Mark all included attachments as referenced so they are not preserved
	if ( toplevelCall )
	{
		for ( Uint32 i=base; i < createdAttachments.Size(); ++i )
		{
			createdAttachments[i]->SetFlag( OF_Referenced );
		}
	}
}

#ifndef NO_EDITOR

void CEntityTemplate::OnPaste( Bool wasCopied )
{
	if ( wasCopied == true && m_entityObject != NULL)
	{
		CompileDataBuffer();
	}
}

#endif

#ifndef NO_DATA_VALIDATION
static Bool CheckStreamingFlag( const CEntityTemplate* origTpl, const CEntityTemplate* tpl )
{
	CEntity* entity = tpl->GetEntityObject();

	// Ignore invalid templates
	if ( entity == nullptr ) return true;

	// Reached streamed template, error
	if ( entity->ShouldBeStreamed() )
	{
		String originalName = origTpl ? ( origTpl->GetFile() ? origTpl->GetFile()->GetFileName() : origTpl->GetFriendlyName() ) : TXT("(null original template)");
		String thisName = tpl ? ( tpl->GetFile() ? tpl->GetFile()->GetFileName() : tpl->GetFriendlyName() ) : TXT("(null included template)");
		String depotPath = tpl ? tpl->GetDepotPath() : TXT("(unknown path)");
		DATA_HALT( DES_Major, origTpl, TXT("Streaming"), TXT("The non-streamed entity template '%ls' relies on streamed entity template '%ls' at '%ls', some components will be missing!"),
			originalName.AsChar(),
			thisName.AsChar(),
			depotPath.AsChar() );
		return false;
	}

	// Scan includes
	for ( const auto& include : tpl->GetIncludes() )
	{
		CEntityTemplate* includeTpl = include.Get();
		if ( includeTpl != nullptr )
		{
			if ( !CheckStreamingFlag( origTpl, includeTpl ) )
			{
				return false;
			}
		}
	}

	// Scan appearances
	for ( const auto& appearance : tpl->GetAppearances() )
	{
		for ( const auto& include : appearance.GetIncludedTemplates() )
		{
			CEntityTemplate* includeTpl = include.Get();
			if ( includeTpl != nullptr )
			{
				if ( !CheckStreamingFlag( origTpl, includeTpl ) )
				{
					return false;
				}
			}
		}
	}

	// Everything passed fine
	return true;
}

void CEntityTemplate::OnCheckDataErrors() const
{
	TBaseClass::OnCheckDataErrors();
	
	// check the entity
	if ( !m_entityObject )
	{
		DATA_HALT( DES_Uber, this, TXT("Entity"), TXT("Entity template has no internal entity - resource is corrupted") );
	}
	else
	{
		// Make sure that this non-streamed template doesn't actually rely on streamed templates
		if ( !m_entityObject->ShouldBeStreamed() )
		{
			CheckStreamingFlag( this, this );
		}

		// Also ask entity itself for data errors
		m_entityObject->OnCheckDataErrors( true );
	}
}

// UBER HACK: needed becase
//void InitEntityTemplatePreviewworld

void CEntityTemplate::OnFullValidation( const String& additionalContext ) const
{
	// make sure the entity is created fresh
	const_cast<TDynArray< Uint8 >&>(m_flatCompiledData).Clear();

	// create fake world and fake layer
	CWorld* world = new CWorld();

	// init the world as preview world with no physics
	WorldInitInfo initInfo;
	initInfo.m_initializePhysics = false;
	initInfo.m_previewWorld = true;
	world->Init( initInfo );

	// create fake entity using this template on the dynamic layer in the fake world
	EntitySpawnInfo info;
	info.m_detachTemplate = false;
	info.m_template = this;
	CTimeCounter timer;
	CEntity* entity = world->GetDynamicLayer()->CreateEntitySync( info );
	if ( !entity )
	{
		DATA_HALT( DES_Uber, this, TXT("Entity"), TXT("Failed to spawn an entity from this template - resource is corrupted") );
	}
	else
	{
		LOG_ENGINE( TXT("[Profile]: Instancing %1.3fms of '%ls'"), timer.GetTimePeriodMS(), GetDepotPath().AsChar() );

		// enumerate appearances
		TDynArray< const CEntityAppearance* > allAppearances;
		GetAllAppearances( allAppearances );

		// find appearance components
		TDynArray< CAppearanceComponent* > appearanceComponents;
		for ( Uint32 j=0; j<entity->GetComponents().Size(); ++j )
		{
			CComponent* component = entity->GetComponents()[j];
			if ( component && component->IsA< CAppearanceComponent >() )
			{
				appearanceComponents.PushBack( static_cast< CAppearanceComponent* >( component ) );
			}
		}

		// more than one appearance component :(
		if ( appearanceComponents.Size() > 1 )
		{
			DATA_HALT( DES_Uber, this, TXT("Entity"), TXT("Entity has more than one appearance component (%ls and %ls). There should be only one."),
				appearanceComponents[0]->GetName().AsChar(), appearanceComponents[1]->GetName().AsChar() );
		}

		// apply each appearance and check stuff
		if ( allAppearances.Empty() )
		{
			// why the hell do we have appearance component when we don't have appearances ?
			if ( !appearanceComponents.Empty() )
			{
				DATA_HALT( DES_Minor, this, TXT("Entity"), TXT("Entity has appearance component (%ls) but it has no appearances."),
					appearanceComponents[0]->GetName().AsChar() );
			}
			else
			{
				// validate the default version of the entity
				entity->FullValidation( TXT("(no appearance)") );
			}
		}
		else
		{
			// no appearance component ?
			if ( appearanceComponents.Empty() )
			{
				DATA_HALT( DES_Uber, this, TXT("Entity"), TXT("Entity has appearances but no appearance component.") );
			}
			else
			{
				CAppearanceComponent* apComponent = appearanceComponents[0];
				CName appearanceName = apComponent->GetAppearance();

				// is the default appearance among the ones that we have ?
				Bool defaultFound = false;
				Bool hasRandom = false;
				for ( Uint32 i=0; i<allAppearances.Size(); ++i )
				{
					if ( allAppearances[i]->GetName() == appearanceName )
					{
						defaultFound = true;
					}
				}

				if ( appearanceName && !defaultFound )
				{
					DATA_HALT( DES_Uber, this, TXT("Entity"), TXT("Default entity appearance '%ls' does not exist in the appearance list."), appearanceName.AsChar() );
				}

				// test every appearance
				for ( Uint32 i=0; i<allAppearances.Size(); ++i )
				{
					const CName appearanceName = allAppearances[i]->GetName();
					apComponent->ApplyAppearance( appearanceName );

					const String context = String::Printf( TXT("(appearance '%ls')"), appearanceName.AsChar() );
					entity->FullValidation( context );
				}
			}
		}

		// remove entity form the layer
		entity->Destroy();
	}


	// delete the world
	world->Shutdown();
	world->Discard();

	// make sure the physics engine is destroyed
	GPhysicEngine->Update( 0.01f );
}

#endif

CObject* CEntityTemplate::CreateTemplateInstance( CObject* parent ) const
{
	EntityTemplateInstancingInfo instancingInfo;
	CLayer* parentLayer = Cast< CLayer >( parent );
	return CreateInstance( parentLayer, instancingInfo );
}

// Get template default instance
const CObject* CEntityTemplate::GetTemplateInstance() const
{
	return m_entityObject;
}

Bool CEntityTemplate::FindComponentOverrideIndex( CComponent* component, Uint32& index ) const
{
	CName componentName( component->GetName() );
	for ( Uint32 i=0; i < m_overrides.Size(); ++i )
	{
		const SEntityTemplateOverride& override = m_overrides[i];
		if ( override.m_componentName == componentName && override.m_className == component->GetClass()->GetName() )
		{
			index = i;
			return true;
		}
	}
	index = (Uint32)-1;
	return false;
}

static CComponent* FindIncludedComponent( const CEntityTemplate* templateWithIncludes, CComponent* localComponent )
{
	CName componentName( localComponent->GetName() );

	// First scan the appearances (they're added after all regular template includes)
	{
		TDynArray< const CEntityAppearance* > appearances;
		templateWithIncludes->GetAllAppearances( appearances );
		for ( auto it=appearances.Begin(); it != appearances.End(); ++it )
		{
			const CEntityAppearance& appearance = *(*it);
			const TDynArray< THandle< CEntityTemplate > >& includes = appearance.GetIncludedTemplates();

			// Scan all includes of the appearance (in reverse order to make sure the
			// last template things first)
			for ( Int32 i=includes.SizeInt() - 1; i >= 0; --i )
			{
				CEntityTemplate* includeTemplate = includes[i].Get();

				if ( includeTemplate )
				{
					CEntity* includeEntity = includeTemplate->GetEntityObject();

					// First check the template at this level of include hierarchy
					if ( includeEntity )
					{
						CComponent* includedComponent = includeEntity->FindComponent( componentName, localComponent->GetClass()->GetName() );
						if ( includedComponent )
						{
							return includedComponent;
						}
					}

					// A component wasn't found in this include, check the include's own includes
					CComponent* includedComponent = FindIncludedComponent( includeTemplate, localComponent );
					if ( includedComponent )
					{
						return includedComponent;
					}
				}
			}
		}
	}
	
	// The component wasn't in any appearance, scan all includes of the given template
	// (in reverse order to make sure the last template things first)
	const TDynArray< THandle< CEntityTemplate > >& includes = templateWithIncludes->GetIncludes();
	for ( Int32 i=includes.SizeInt() - 1; i >= 0; --i )
	{
		CEntityTemplate* includeTemplate = includes[i].Get();

		if ( includeTemplate )
		{
			CEntity* includeEntity = includeTemplate->GetEntityObject();

			// First check the template at this level of include hierarchy
			if ( includeEntity )
			{
				CComponent* includedComponent = includeEntity->FindComponent( componentName, localComponent->GetClass()->GetName() );
				if ( includedComponent )
				{
					return includedComponent;
				}
			}

			// A component wasn't found in this include, check the include's own includes
			CComponent* includedComponent = FindIncludedComponent( includeTemplate, localComponent );
			if ( includedComponent )
			{
				return includedComponent;
			}
		}
	}

	// Failed to find an included equivalent of the given component
	return NULL;
}

Bool CEntityTemplate::HasOverridesForComponent( const CComponent* component ) const
{
	CName componentName( component->GetName() );
	for ( Uint32 i=0; i < m_overrides.Size(); ++i )
	{
		const SEntityTemplateOverride& override = m_overrides[i];
		if ( override.m_componentName == componentName && override.m_className == component->GetClass()->GetName() )
		{
			return true;
		}
	}
	return false;
}

void CEntityTemplate::ResetPropertiesForComponent( CComponent* component, const TDynArray< CName >& propertiesToReset ) const
{
	if ( !propertiesToReset.Empty() )
	{
		CComponent* includedComponent = FindIncludedComponent( this, component );
		if ( includedComponent )
		{
			CClass* componentClass = component->GetClass();
			TDynArray< CProperty* > properties;
			componentClass->GetProperties( properties );

			// Reset values to those of the included component
			for ( auto it=propertiesToReset.Begin(); it != propertiesToReset.End(); ++it )
			{
				CProperty* property = componentClass->FindProperty( *it );
				if ( property != nullptr )
				{
					const void* includedValue = property->GetOffsetPtr( includedComponent );
					void* localValue = property->GetOffsetPtr( component );
					property->GetType()->Copy( localValue, includedValue );
				}
			}
		}
	}
}

void CEntityTemplate::RemoveOverrideForComponentProperty( CComponent* component, const CName& propertyName, Bool resetPropertyValue )
{
	// Find override index
	Uint32 overrideIndex;
	if ( !FindComponentOverrideIndex( component, overrideIndex ) )
	{
		return;
	}
	SEntityTemplateOverride& override = m_overrides[overrideIndex];
	
	// Scan the overridden properties for the property name
	for ( Int32 i=override.m_overriddenProperties.SizeInt() - 1; i >= 0; --i )
	{
		// Remove the property and stop
		if ( override.m_overriddenProperties[i] == propertyName )
		{
			override.m_overriddenProperties.RemoveAt( i );

			// If the list is now empty, remove the override itself
			if ( override.m_overriddenProperties.Empty() )
			{
				m_overrides.RemoveAt( overrideIndex );
			}

			break;
		}
	}

	// Reset property value
	if ( resetPropertyValue )
	{
		TDynArray< CName > singleName;
		singleName.PushBack( propertyName );
		ResetPropertiesForComponent( component, singleName );
	}
}

void CEntityTemplate::RemoveOverrideForComponentProperties( CComponent* component, const TDynArray< CName >& propertiesToRemove, Bool resetPropertyValues )
{
	// This is slow, but it will only be used by the editor with a few properties, so it should be fine
	for ( auto it=propertiesToRemove.Begin(); it != propertiesToRemove.End(); ++it )
	{
		RemoveOverrideForComponentProperty( component, *it, false );
	}

	// Reset any properties
	if ( resetPropertyValues )
	{
		ResetPropertiesForComponent( component, propertiesToRemove );
	}
}

void CEntityTemplate::RemoveAllOverridesForComponent( CComponent* component, Bool resetPropertyValues )
{
	// Collect properties to reset
	TDynArray< CName > propertiesToReset;
	Uint32 overrideIndex;
	if ( FindComponentOverrideIndex( component, overrideIndex ) )
	{
		propertiesToReset.PushBack( m_overrides[ overrideIndex ].m_overriddenProperties );
		m_overrides.RemoveAt( overrideIndex );
	}

	// Reset any properties
	if ( resetPropertyValues )
	{
		ResetPropertiesForComponent( component, propertiesToReset );
	}
}

void CEntityTemplate::CreatePropertyOverride( CComponent* component, const CName& propertyName )
{
	// Find or create override for the given component
	Uint32 overrideIndex;
	if ( !FindComponentOverrideIndex( component, overrideIndex ) )
	{
		m_overrides.Grow();
		m_overrides.Back().m_className = component->GetClass()->GetName();
		m_overrides.Back().m_componentName = CName( component->GetName() );
		m_overrides.Back().m_overriddenProperties.PushBack( propertyName );
		return;
	}
	SEntityTemplateOverride& override = m_overrides[overrideIndex];

	// Make sure there isn't already an entry for the given property
	for ( auto it=override.m_overriddenProperties.Begin(); it != override.m_overriddenProperties.End(); ++it )
	{
		if (*it == propertyName )
		{
			return;
		}
	}

	// Add new entry and be done with it
	override.m_overriddenProperties.PushBack( propertyName );
}

static Bool AlwaysIgnoreComponentProperty( CComponent* component, IRTTIType* propertyType, CProperty* property )
{
	// Special case for "transformParent"
	// transformParent will be always different due to new attachments
	if ( propertyType->GetType() == RT_Pointer && static_cast< CRTTIPointerType* >( propertyType )->GetPointedType() == CHardAttachment::GetStaticClass() && property->GetName() == CNAME( transformParent ) )
	{
		return true;
	}

	// Special case for CBoundedComponent
	// boundingBox will change after attachments
	if ( propertyType == Box::GetStaticClass() && property->GetName() == CNAME( boundingBox ) && component->IsA< CBoundedComponent >() )
	{
		return true;
	}

#ifndef NO_COMPONENT_GRAPH
	// Special case for component graph position
	// Just ignore the graph position, it isn't worth it to save a local copy of the component just for it
	if ( propertyType->GetType() == RT_Simple && ( property->GetName() == CNAME( graphPositionX ) || property->GetName() == CNAME( graphPositionY ) ) )
	{
		return true;
	}
#endif

	return false;
}

static Bool ComparePropertyValues( CComponent* component, CProperty* property, const void* ptr1, const void* ptr2 )
{
	class CompareMemoryFileWriter : public CMemoryFileWriter
	{
		THashSet< ISerializable* >	m_reached;
	public:
		CompareMemoryFileWriter( TDynArray< Uint8 >& data )
			: CMemoryFileWriter( data )
		{}

		virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
		{
			if ( pointerClass->IsSerializable() )
			{
				ISerializable* obj = static_cast< ISerializable* >( pointer );
				if ( obj && m_reached.Insert( obj ) )
				{
					obj->GetClass()->Serialize( *this, (void*)obj );
				}
			}
		}
	};

	IRTTIType* propertyType = property->GetType();

	// Check some special cases which should never be overridden
	if ( AlwaysIgnoreComponentProperty( component, propertyType, property ) )
	{
		return true;
	}

	// If we have a potentially complex property (such a CObject), use serialization to make sure
	// that everything is checked
	if ( propertyType->GetType() == RT_Pointer && property->IsInlined() )
	{
		TDynArray< Uint8 > data1, data2;
		CompareMemoryFileWriter writer1( data1 ), writer2( data2 );
		propertyType->Serialize( writer1, const_cast< void* >( ptr1 ) );
		propertyType->Serialize( writer2, const_cast< void* >( ptr2 ) );
		return data1 == data2;
	}
	else // For simple properties, just use the property type
	{
		return propertyType->Compare( ptr1, ptr2, 0 );
	}
}

void CEntityTemplate::CreatePropertyOverridesForComponent( CComponent* includedComponent, CComponent* localComponent )
{
	// Get class
	CClass* componentClass = includedComponent->GetClass();
	ASSERT( componentClass == localComponent->GetClass(), TXT("Requested to create property overrides for components of different classes") );

	// Ignore the call if the included component is streamed since we
	// cannot save duplicates for streamed components
	if ( includedComponent->IsStreamed() )
	{
		return;
	}

	// Get all properties
	TDynArray< CProperty* > properties;
	componentClass->GetProperties( properties );

	// Check the value of each property between the two components
	for ( auto it=properties.Begin(); it != properties.End(); ++it ) {
		// Get values
		CProperty* property = *it;
		IRTTIType* propertyType = property->GetType();
		const void* includedValue = property->GetOffsetPtr( includedComponent );
		const void* localValue = property->GetOffsetPtr( localComponent );

		// If the values are the same or the components do not allow overriding them, remove any relevant property override
		if ( ComparePropertyValues( localComponent, property, includedValue, localValue ) )
		{
			RemoveOverrideForComponentProperty( localComponent, property->GetName(), false );
		}
		else // otherwise add a new property override
		{
			// Check if the included component actually allows this
			if ( includedComponent->CanOverridePropertyViaInclusion( property->GetName() ) )
			{
				CreatePropertyOverride( localComponent, property->GetName() );
			}
			else // even though the properties differ, the component vetoed the override so remove it
			{
				RemoveOverrideForComponentProperty( localComponent, property->GetName(), false );
			}
		}
	}
}

void CEntityTemplate::GetOverridenPropertiesForComponent( CComponent* component, TDynArray< CName >& propertyNames )
{
	Uint32 overrideIndex;
	if ( FindComponentOverrideIndex( component, overrideIndex ) )
	{
		propertyNames.PushBack( m_overrides[overrideIndex].m_overriddenProperties );
	}
}

static Bool GetIncludeHierarchyLevelForOverridenComponentScanner( const CEntityTemplate* currentTemplate, CComponent* component, Int32 level, Int32& foundLevel )
{
	// Check this level
	if ( currentTemplate->HasOverridesForComponent( component ) )
	{
		foundLevel = level;
		return true;
	}

	// Not found, check higher levels
	const TDynArray< THandle< CEntityTemplate > >& includes = currentTemplate->GetIncludes();
	for ( auto it=includes.Begin(); it != includes.End(); ++it )
	{
		CEntityTemplate* includedTemplate = (*it).Get();
		if ( includedTemplate )
		{
			if ( GetIncludeHierarchyLevelForOverridenComponentScanner( includedTemplate, component, level + 1, foundLevel ) )
			{
				return true;
			}
		}
	}

	return false;
}

Int32 CEntityTemplate::GetIncludeHierarchyLevelForOverridenComponent( CComponent* component ) const
{
	Int32 result = -1;
	GetIncludeHierarchyLevelForOverridenComponentScanner( this, component, 0, result );
	return result;
}

void CEntityTemplate::RebuildPropertyOverridesFromIncludes( CEntity* sourceEntity, Bool addExternalFlags )
{
	// Rebuild overrides
	m_overrides.Clear();
	const TDynArray< CComponent* >& components = sourceEntity->GetComponents();
	for ( auto it=components.Begin(); it != components.End(); ++it )
	{
		CComponent* localComponent = *it;
		CComponent* includedComponent = FindIncludedComponent( this, localComponent );
		if ( includedComponent )
		{
			CreatePropertyOverridesForComponent( includedComponent, localComponent );

			// Mark the object as external (used by the cook to throw out the component
			// if it wasn't referenced by any overrides)
			if ( addExternalFlags )
			{
				localComponent->CObject::SetFlag( OF_Transient );
				localComponent->CObject::SetFlag( OF_Referenced );

				localComponent->SetFlag( NF_IncludedFromTemplate );
			}
		}
	}
}

void CEntityTemplate::ProcessIncludedComponent( CComponent* component, const SEntityTemplateOverride* componentOverride, CEntity* entity, TDynArray< CComponent* > &addedComponents, struct SSavedAttachments* savedAttachments, bool properOverrides, bool fullAttach /* = true */, bool includedFromTemplate /* = true */ )
{
	PC_SCOPE( CEntityTemplate ProcessIncludedComponent );
	// Check if component isn't overridden
	CExternalProxyComponent* proxy = NULL;
	CComponent* overriddenComponent = NULL;

	String name   = component->GetName();
	CClass *cclass = component->GetClass();
	const Uint32 componentsSize = entity->m_components.Size();
	for ( Uint32 j = 0; j < componentsSize; ++j )
	{
		CComponent* entityComponent = entity->m_components[ j ];		

		// proxy components cannot override
		if ( !entityComponent->IsA< CExternalProxyComponent >() )
		{
			if ( overriddenComponent == NULL && entityComponent->GetClass() == cclass && entityComponent->GetName() == name )
			{
				overriddenComponent = entityComponent;
			}
		}
		else if ( proxy == NULL )
		{
			CExternalProxyComponent *tmpProxy = SafeCast<CExternalProxyComponent>( entityComponent );
			if ( tmpProxy->IsProxyFor( *component ) )
			{
				proxy = tmpProxy;
			}
		}
	}

	if ( overriddenComponent != NULL )
	{
		// Copy non-overridden properties from component
		if ( componentOverride && !componentOverride->m_overriddenProperties.Empty() )
		{
			Bool madeACopy = false;

			// TODO: get rid of this, save the hashes in template data
			THashSet< CName > localPropertiesSet( componentOverride->m_overriddenProperties.Size() );
			for ( CName name : componentOverride->m_overriddenProperties )
			{
				localPropertiesSet.Insert( name );
			}

			// Scan the properties and copy the non-overridden ones
			TDynArray< CProperty* > properties;
			CClass* componentClass = overriddenComponent->GetClass();
			componentClass->GetProperties( properties );
			for ( auto it=properties.Begin(); it != properties.End(); ++it )
			{
				CProperty* property = *it;
				
				// If the property is marked as overridden, then don't copy it from the included component
				if ( localPropertiesSet.Exist( property->GetName() ) )
				{
					continue;
				}

				// Copy the value from the included component to the local component
				const void* includedComponentValue = property->GetOffsetPtr( component );
				void* localComponentValue = property->GetOffsetPtr( overriddenComponent );

				// We're copying a CObject pointer, create a clone of it if it is inlined
				if ( property->GetType()->GetType() == RT_Pointer &&
					 static_cast< CRTTIPointerType* >( property->GetType() )->GetPointedType()->GetType() == RT_Class )
				{
					CObject* const* source = reinterpret_cast< CObject* const* >( includedComponentValue );
					CObject** target = reinterpret_cast< CObject** >( localComponentValue );
					if ( source && target && *source )
					{
						// Clone inlined objects
						if ( (*source)->IsInlined() )
						{
							*target = *source ? (*source)->Clone( overriddenComponent, true, true ) : NULL;
						}
						else // Do a regular copy for referenced objects
						{
							*target = *source;
						}
					}
					else
					{
						if ( target )
						{
							*target = NULL;
						}
					}

					// We made a copy
					madeACopy = true;
				}
				else // we're copying... something else, just use property's own Copy method
				{
					property->GetType()->Copy( localComponentValue, includedComponentValue );
				}
			}

			// If we didn't made a copy, the overriddenComponent is useless, so destroy it
			if ( !madeACopy )
			{
				entity->RemoveComponent( overriddenComponent );
				overriddenComponent = NULL;
			}
		}
		else
		{
			// Remove overridden component (keeps the included one)
			if ( properOverrides )
			{
				entity->RemoveComponent( overriddenComponent );
				overriddenComponent = NULL;
			}
		}
			
		// Check again for the overridenComponent if it is still there
		if ( overriddenComponent )
		{
			if ( proxy )
			{
				if ( savedAttachments )
				{
					savedAttachments->AddReplacement( proxy, overriddenComponent );
				}
				else
				{
					entity->DestroyComponent( proxy );
				}
			}

			overriddenComponent->CObject::SetFlag( OF_Transient );
			overriddenComponent->CObject::SetFlag( OF_Referenced );
			addedComponents.PushBack( overriddenComponent );

			return;
		}
	}

	// Set flag
	
	
	if ( fullAttach )
	{
		entity->AddComponent( component );
	}
	else
	{
		component->SetParent( entity );

		// Add to entity
		entity->m_components.PushBack( component );

		entity->SendNotifyComponentAdded( component );
	}

	component->CObject::SetFlag( OF_Transient );
	component->CObject::SetFlag( OF_Referenced );
	
	if ( includedFromTemplate )
		component->SetFlag( NF_IncludedFromTemplate );

	component->OnPostInstanced();

	addedComponents.PushBack( component );

	//  We don't need the proxy anymore
	if ( proxy )
	{
		if ( savedAttachments )
		{
			savedAttachments->AddReplacement( proxy, component );
		}
		entity->DestroyComponent(proxy);
	}
}

void CEntityTemplate::IncludeComponents( CObject* layer, CEntity* entity, const Uint8* componentsData, Uint32 dataSize, TDynArray< CComponent* >& addedComponents, struct SSavedAttachments* savedAttachments, Bool includedFromTemplate )
{
	if ( dataSize == 0 ) 
	{
		return;
	}

	// Already collected
	const Uint8* sourceData = componentsData;

	// Deserialize
	CMemoryFileReader reader( sourceData, dataSize, 0 );
	CDependencyLoader loader( reader, NULL );

	// Load
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = layer; 
	if ( loader.LoadObjects( loadingContext ) )
	{
		// Post load them
		loader.PostLoad();

		// Mark all components as transient
		const TDynArray< CObject* >& components = loadingContext.m_loadedRootObjects;

		// Save attachments
		if ( savedAttachments )
		{
			for ( auto it=components.Begin(); it != components.End(); ++it )
			{
				savedAttachments->Add( static_cast< CComponent* >( *it ) );
			}
		}

		const Uint32 componentsSize = components.Size();
		for ( Uint32 i = 0; i < componentsSize; ++i )
		{
			CComponent* comp = Cast<CComponent>( components[i] );
			ProcessIncludedComponent( comp, NULL, entity, addedComponents, savedAttachments, true, true, includedFromTemplate );
		}
	}
}

void CEntityTemplate::IncludeEntityTemplate( CObject* layer, CEntity* entity, CEntityTemplate* includedEntityTemplate, const CEntityTemplate* localEntityTemplate, TDynArray< THandle< CResource > > & collected, TDynArray< CComponent* >& addedComponents, struct SSavedAttachments* savedAttachments, THashSet< CName >* foundOverrides, bool fullInclude /* = true */, bool fullAttach /* = true */ )
{
	PC_SCOPE( IncludeEntityTemplate )
	// Make sure template is loaded
	if ( !includedEntityTemplate )
	{
		return;
	}

	// Already collected
	THandle< CResource > templateHandle( includedEntityTemplate );
	if ( Find( collected.Begin(), collected.End(), templateHandle ) != collected.End() )
	{
		return;
	}

	collected.PushBack( includedEntityTemplate );

	// Create entity from include
	EntityTemplateInstancingInfo iinfo;
	iinfo.m_async = false;
	iinfo.m_detachFromTemplate = false;
	iinfo.m_previewOnly = false;

	// If we are cooked than use the cooked data
	CEntity* createdEntity = nullptr;
	if ( includedEntityTemplate->IsCooked() )
	{
		createdEntity = includedEntityTemplate->CreateInstance( static_cast< CLayer* >( layer ), EntityTemplateInstancingInfo() );
	}
	else
	{
		createdEntity = includedEntityTemplate->CreateEntityUncached( layer, iinfo, savedAttachments, NULL, false );
	}

	// 
	if ( createdEntity )
	{
		if ( savedAttachments )
		{
			savedAttachments->Add( createdEntity );
		}

		TDynArray< CComponent* > components = createdEntity->GetComponents();

		// Detach components from temporary entity
		createdEntity->m_components.Clear();
		createdEntity->m_streamingComponents.Clear();

		// Mark all components as being transient ( they are not saved directly )
		for ( Uint32 i=0; i < components.Size(); ++i )
		{
			CComponent* component = components[i];
			const SEntityTemplateOverride* componentOverride = NULL;
			if ( localEntityTemplate )
			{
				Uint32 componentOverrideIndex;
				if ( localEntityTemplate->FindComponentOverrideIndex( component, componentOverrideIndex ) )
				{
					componentOverride = &localEntityTemplate->m_overrides[componentOverrideIndex];
					if ( foundOverrides )
					{
						foundOverrides->Insert( CName( component->GetName() ) );
					}
				}
			}

			const Bool properOverrides = localEntityTemplate ? localEntityTemplate->m_properOverrides : true;
			ProcessIncludedComponent( component, componentOverride, entity, addedComponents, savedAttachments, fullInclude, fullAttach && properOverrides, true );
		}
	}
}



void CEntityTemplate::IncludeTemplateSaveAttachments( const TDynArray< CComponent* >& components, SSavedAttachments& savedAttachments )
{
}

void CEntityTemplate::IncludeTemplateRestoreBrokenAttachments( const TDynArray< CComponent* >& addedComponents, CEntity* entity, TDynArray< SSavedAttachment >& savedAttachments )
{
}

CEntity* CEntityTemplate::CreateEntityUncached( CObject* owner, const EntityTemplateInstancingInfo& instanceInfo, SSavedAttachments* savedAttachments, SSavedAttachments* extraAttachments, Bool destroyBrokenAttachments ) const
{
	PC_SCOPE( CEntityTemplate CreateEntityUncached );

	// If we have no data we can't create an entity - return.
	if ( !m_data.Size() )
	{
		return NULL;
	}

	// We shouldn't use this function if we have cooked data
	if ( IsCooked() )
	{
		WARN_ENGINE( TXT("Using uncached entity creation with template '%ls'"), GetDepotPath().AsChar() );
	}

	// Interpret data as a byte array data buffer, so we can provide it to the MemoryFileReader.
	const TDynArray< Uint8 >& dataBuffer = reinterpret_cast< const TDynArray< Uint8 >& >(m_data);

	// Construct a loading context
	SDependencyLoaderCreateEntityContext loadingContext;
	loadingContext.m_parent = owner;

	// Override class using the given one in instance info
	if ( m_entityObject && instanceInfo.m_entityClass )
	{
		ASSERT( m_entityObject->GetClass()->IsA( instanceInfo.m_entityClass ), TXT("Attempted to create entity from template that is not subclass of desired entity class") );

		loadingContext.m_srcEntityClass = m_entityObject->GetClass();
		loadingContext.m_dstEntityClass = instanceInfo.m_entityClass;
	}

	CEntity* createdEntity = NULL;
	if( !LoadEntityFromMemoryDataArray( dataBuffer, loadingContext, &createdEntity ) )
	{
		return NULL;
	}

	// Use local copy of saved attachments if we weren't provided with one
	SSavedAttachments localSavedAttachments;
	Bool applyAttachments = false;
	if ( !savedAttachments )
	{
		//LOG_ENGINE( TXT("[ !! ET !! ]: Using own attachments for '%ls'"), GetFile()->GetDepotPath().AsChar() );
		savedAttachments = &localSavedAttachments;
		if ( extraAttachments )
		{
			// Move the extra attachments here
			savedAttachments->m_attachments.PushBack( extraAttachments->m_attachments );
			extraAttachments->m_attachments.Clear();
		}
		applyAttachments = true; // we'll apply our own attachments
	}
	else
	{
		ASSERT( extraAttachments == nullptr, TXT("Passed extra attachments without local attachments!") );
	}

	// Save attachments from components defined in this entity template
	savedAttachments->Add( createdEntity );

	// Collect components from includes
	TDynArray< THandle < CResource > > collected;
	THashSet< CName > foundOverrides;
	const Uint32 includesSize = m_includes.Size();
	for ( Uint32 i = 0; i < includesSize; ++i )
	{
		TDynArray<CComponent*> components;
		IncludeEntityTemplate( owner, createdEntity, m_includes[i].Get(), this, collected, components, savedAttachments, &foundOverrides, false );
	}

	// Remove components which were used to store overridden properties but didn't found the original component
	const Uint32 overridesSize = m_overrides.Size();
	for ( Uint32 i = 0; i < overridesSize; ++i )
	{
		// Check if the entry was found
		if ( foundOverrides.Exist( m_overrides[i].m_componentName ) )
		{
			continue;
		}

		// Not found, find the local component
		CComponent* overridingComponent = createdEntity->FindComponent( m_overrides[i].m_componentName );
		if ( overridingComponent != nullptr )
		{
			createdEntity->RemoveComponent( overridingComponent );
			overridingComponent->Discard();
		}
	}

	// Remove all existing attachments
	const TDynArray< CComponent* >& components = createdEntity->GetComponents();
	for ( auto it=components.Begin(); it != components.End(); ++it )
	{
		CComponent* component = *it;
		component->BreakAllAttachments();
	}

	// Reattach them using the savedAttachments which contains attachments from all includes
	if ( applyAttachments )
	{
		//LOG_ENGINE( TXT("[ !! ET !! ]: Applying saved attachments for '%ls'"), GetFile()->GetDepotPath().AsChar() );
		savedAttachments->Apply( createdEntity );

		CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( createdEntity );
		if ( appearanceComponent )
		{
			//LOG_ENGINE( TXT("[ !! ET !! ]: Saving attachments to appearance component for '%ls'"), GetFile()->GetDepotPath().AsChar() );
			appearanceComponent->SetAttachmentReplacements( savedAttachments->m_replacements );
		}
		else
		{
			//LOG_ENGINE( TXT("[ !! ET !! ]: No appearance component found for '%ls'"), GetFile()->GetDepotPath().AsChar() );
		}

		// Remove any proxies that link to non-existent external components
		//TC - to be removed for attachments fix
		//THashSet< CGUID > componentsToRemove;
		for ( Int32 i=components.SizeInt() - 1; i >= 0; --i )
		{
			CExternalProxyComponent* proxy = Cast< CExternalProxyComponent >( components[i] );
			if ( proxy )
			{
				createdEntity->DestroyComponent( proxy );
			}
		}
	}

	// Always attach created entity to the template
	createdEntity->m_template = const_cast< CEntityTemplate* >( this );

	// Inform entity that instancing has finished
	createdEntity->OnIncludesFinished();

	// Destroy broken attachments if requested
	if ( destroyBrokenAttachments )
	{
		for ( Int32 i=components.SizeInt() - 1; i >= 0; --i )
		{
			TDynArray< CObject* > children;
			CComponent* component = components[i];
			component->GetChildren( children );
			for ( auto it=children.Begin(); it != children.End(); ++it )
			{
				IAttachment* attachment = Cast< IAttachment >( *it );
				if ( attachment != nullptr && ( attachment->IsBroken() || attachment->GetParent() == nullptr || attachment->GetChild() == nullptr ) )
				{
					attachment->Discard();
				}
			}
		}
	}

	// Return created entity
	return createdEntity;
}

Bool CEntityTemplate::CreateFullDataBufferFromEntity( CEntity* entity )
{
	// Clear buffer
	m_flatCompiledData.Clear();

	// Always detach here so the created entity can be saved directly
	entity->m_template = NULL;

	// Zero out the entity GUID
	entity->SetGUID( CGUID::ZERO );

	// Cycle all the components.
	const TDynArray< CComponent* >& components = entity->GetComponents();
	const Uint32 componentsSize = components.Size();
	for ( Uint32 i = 0; i < componentsSize; ++i )
	{
		CComponent* component = components[i];

		// Remove the "referenced" and the "transient" flag
		component->CObject::ClearFlag( OF_Referenced );
		component->CObject::ClearFlag( OF_Transient );

		// Component name hash
		const Uint64 nameHash = Red::CalculateHash64( UNICODE_TO_ANSI( component->GetName().AsChar() ) );

		// Rebuild special GUID for the component that's only unique inside the component
		CGUID componentGUID;
		componentGUID.guid[0] = (Uint32)( (nameHash >> 32) & 0xFFFFFFFF );
		componentGUID.guid[1] = (Uint32)( (nameHash >> 0) & 0xFFFFFFFF );
		componentGUID.guid[2] = 0xEEEE3333;
		componentGUID.guid[3] = 1; // static component
	}

	TDynArray< Uint8, MC_EntityTemplate > tempBuffer;
	DependencySavingContext savingContext( entity );
	//savingContext.m_hashPaths = true;
	savingContext.m_zeroNonDeterministicData = true;
	SaveToMemoryDataArray( savingContext, &tempBuffer );

	m_flatCompiledData.Clear();
	m_flatCompiledData.Reserve( tempBuffer.Size() );
	m_flatCompiledData.PushBack( *reinterpret_cast< TDynArray< Uint8 >* >( &tempBuffer ) );

	// Keep the compilation time. Local time, because all file times are local and we're comparing this to filetime.
	Red::System::Clock::GetInstance().GetLocalTime( m_dataCompilationTime );
	return true;
}

Bool CEntityTemplate::CreateFullDataBuffer( CObject* owner, const EntityTemplateInstancingInfo& instanceInfo, SSavedAttachments* extraAttachments )
{
	PC_SCOPE( CEntityTemplate CreateFullDataBuffer );

	// Clear buffer
	m_flatCompiledData.Clear();

	// Create the raw entity
	CEntity* rawEntity = CreateEntityUncached( owner, instanceInfo, NULL, extraAttachments, true );
	if ( !rawEntity )
	{
		// Not cached :(
		return false;
	}

	// Rebuild the data buffer from the raw entity
	CreateFullDataBufferFromEntity( rawEntity );

	// cleanup
	rawEntity->Discard();

	// Done
	return true;
}


Bool CEntityTemplate::SaveToMemoryDataArray( DependencySavingContext& savingContext, TDynArray<Uint8, MC_EntityTemplate >* dstBuffer )
{
	Bool success = false;
	TDynArray< Uint8 > saveDataBuffer;

	//CH: This is based of the largest reserve I got back from a little test I did loading skellige - this should be calculated programmatically.
	saveDataBuffer.Reserve( 1024 * 34 );
	
	// Hook the memory file writer into our temp save data buffer.
	CMemoryFileWriter memFileWriter( saveDataBuffer );

	// Create a dependency saver for saving the file to memory.
	CDependencySaver dependencySaver( memFileWriter, NULL );
	
	// Save the object to the buffer.
	success = dependencySaver.SaveObjects( savingContext );
	if( success )
	{
		dstBuffer->Resize( saveDataBuffer.Size() );

		Red::System::MemoryCopy( dstBuffer->Data(), saveDataBuffer.Data(), saveDataBuffer.Size() );
	}
	return success;
}

CEntity* CEntityTemplate::CreatePreviewWindowEntityInstance( CLayer* parentLayer, const EntityTemplateInstancingInfo& instanceInfo ) const
{
	CEntity* newEntityInstance = NULL;

	// We don't want to have the inactive components removed, create from the raw template, always :)
	newEntityInstance = CreateEntityUncached( parentLayer, instanceInfo, NULL, NULL, true );

	if ( newEntityInstance != NULL )
	{
		// Mark components as instanced
		for ( CComponent* component : newEntityInstance->GetComponents() )
		{
			component->SetFlag( NF_WasInstancedFromTemplate	);
		}

		// Mark entity as instanced
		newEntityInstance->SetFlag( NF_WasInstancedFromTemplate	);
	}

	return newEntityInstance; 
}

Bool CEntityTemplate::LoadEntityFromMemoryDataArray( const TDynArray< Uint8 >& srcDataBuffer, SDependencyLoaderCreateEntityContext& loadingContext, CEntity** loadedEntity ) const
{
	PC_SCOPE( CEntityTemplate LoadEntityFromMemoryDataArray );

	Bool success = false;
	// Create a file reader for our dataBuffer
	CMemoryFileReader memFileReader( srcDataBuffer, 0 );

	// Create a dependency loader to load dependencies from our reader.
	CDependencyLoaderCreateEntity dependencyLoader( memFileReader, NULL );

	// Return if we fail to load the objects
	if ( dependencyLoader.LoadObjects( loadingContext ) )
	{
		// Post load them - which should initialize the loaded object.
		dependencyLoader.PostLoad();

		// Get the entity back from the loadingContext.
		(*loadedEntity) = loadingContext.m_loadedRootObjects.Empty() ? nullptr : Cast< CEntity >( loadingContext.m_loadedRootObjects[0] );
		ASSERT( (*loadedEntity), TXT( "Entity wasn't created" ) );
		if ( (*loadedEntity) )
		{
			success = true;
		}
	}
	return success;
}

CEntity* CEntityTemplate::CreateEntityInstanceFromCompiledData( CLayer* parentLayer, const EntityTemplateInstancingInfo& instanceInfo ) const
{
	CEntity* createdEntity = NULL;

	// Try to load the entity from the flat compiled data
	SDependencyLoaderCreateEntityContext loadingContext;
	loadingContext.m_parent = parentLayer;

	// Override class using the given one in instance info
	if ( m_entityObject && instanceInfo.m_entityClass )
	{
		ASSERT( m_entityObject->GetClass()->IsA( instanceInfo.m_entityClass ), TXT("Attempted to create entity from template that is not subclass of desired entity class") );

		loadingContext.m_srcEntityClass = m_entityObject->GetClass();
		loadingContext.m_dstEntityClass = instanceInfo.m_entityClass;
	}

	if( !LoadEntityFromMemoryDataArray( m_flatCompiledData, loadingContext, &createdEntity ) )
	{
		WARN_ENGINE( TXT("Deserialization of entity template '%ls' failed"), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get entity
	ASSERT( createdEntity, TXT("Failed to create the entity from flat compiled data.") );

	// Unbind from entity template
	if ( instanceInfo.m_detachFromTemplate )
	{
		for ( CComponent* component : createdEntity->GetComponents() )
		{
			component->CObject::ClearFlag( OF_Transient );
			component->CObject::ClearFlag( OF_Referenced );
		}

		// Detach from template
		createdEntity->m_template = NULL;
	}
	else
	{
		// Attach created entity to the template
		createdEntity->m_template = const_cast< CEntityTemplate* >( this );
	}

	createdEntity->OnPostInstanced();

	return createdEntity;
}

static Bool AreIncludesNewerThan( const CEntityTemplate* templateWithIncludes, const CDateTime& time )
{
	const TDynArray< THandle< CEntityTemplate > >& includes = templateWithIncludes->GetIncludes();

	// Scan all includes to see if any of the resource files are newer than the data
	for ( auto it=includes.Begin(); it != includes.End(); ++it )
	{
		const CEntityTemplate* includedTemplate = (*it).Get();

		if ( includedTemplate )
		{
			// Check the includes at this level

			CDateTime includedTemplateFileTime( includedTemplate->GetFile()->GetFileTime() );
			if ( includedTemplateFileTime > time )
			{
				return true;
			}

			// Check the includes at upper level
			if ( AreIncludesNewerThan( includedTemplate, time ) )
			{
				return true;
			}
		}
	}

	return false;
}

CEntity* CEntityTemplate::CreateInstance( CLayer* parentLayer, const EntityTemplateInstancingInfo& instanceInfo ) const
{
	PC_SCOPE_PIX( CreateEntityInstance );

	RED_ASSERT( ( m_entityObject && m_entityObject->GetGUID() == CGUID::ZERO ) || !m_entityObject, TXT( "Template Entity Object can not have a GUID!" ) );

	CEntity* createdEntity = NULL;

#if !(defined NO_DATA_VALIDATION) && !(defined NO_EDITOR)
	// Make sure that this non-streamed template doesn't actually rely on streamed templates
	if ( Config::cvValidateStreamingFlag.Get() && m_entityObject && !m_entityObject->ShouldBeStreamed() )
	{
		CheckStreamingFlag( this, this );
	}
#endif

	// Non cooked data checks
	if ( !IsCooked() )
	{
		// If we got a preview request, create an uncached entity that also
		// contains the proper component flags
		if ( instanceInfo.m_previewOnly )
		{
			return CreateEntityUncached( parentLayer, instanceInfo, nullptr, nullptr, true );
		}

		Bool shouldUpdateDataBuffer = true;

		// Check if we have flat compiled data
		if ( !m_flatCompiledData.Empty() )
		{
			shouldUpdateDataBuffer = GIsCooker || AreIncludesNewerThan( this, m_dataCompilationTime );
		}

		// We need the data buffer
		if ( shouldUpdateDataBuffer )
		{
			// We can't serialize object of overriden class to compiled buffer!
			if ( instanceInfo.m_entityClass != nullptr && (!m_entityObject || m_entityObject->GetClass() != instanceInfo.m_entityClass) )
			{
				return CreateEntityUncached( parentLayer, instanceInfo, nullptr, nullptr, true );
			}
			CEntityTemplate* nonConstThis = const_cast< CEntityTemplate* >( this );
			nonConstThis->CreateFullDataBuffer( parentLayer, instanceInfo, NULL );
		}
	}

	return CreateEntityInstanceFromCompiledData( parentLayer, instanceInfo );
}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CEntityTemplate::Reload( Bool confirm )
{
	if ( GetFile() != nullptr )
	{
		if ( confirm )
		{
			EDITOR_QUEUE_EVENT( CNAME( FileReloadConfirm ), CreateEventData( this ) );
		}
		else
		{
			CDiskFile* file = GetFile();
			// after invoking TBaseClass::Reload 'this' no longer exists :-) trolololo
			if ( TBaseClass::Reload( confirm ) )
			{
				CEntityTemplate* entityTemplate = static_cast< CEntityTemplate* > ( file->GetResource() );
				EDITOR_DISPATCH_EVENT( CNAME( EntityTemplateChanged ), CreateEventData( entityTemplate ) );
			}
		}
	}

	return true;
}

#endif

/************************************************************************/
/* Appearances                                                          */
/************************************************************************/
void CEntityTemplate::GetAllAppearances( TDynArray< const CEntityAppearance* > & appearances, const TDynArray< CName > * filter /*=NULL*/ ) const
{
	for ( Uint32 i = 0; i < m_appearances.Size(); ++i )
	{
		const CEntityAppearance &appearance = m_appearances[i];

		if ( filter && filter->Empty() == false && ! filter->Exist( appearance.GetName() ) )
		{
			continue;
		}

		appearances.PushBack( &appearance );
	}

	for ( Uint32 i = 0; i < m_includes.Size(); ++i )
	{
		CEntityTemplate *templ = m_includes[i].Get();
		if ( templ )
		{
			templ->GetAllAppearances( appearances, filter );
		}
	}
}

void CEntityTemplate::GetAllAppearances( TDynArray< CEntityAppearance* > & appearances, const TDynArray< CName > * filter /*=NULL*/ )
{
	for ( Uint32 i = 0; i < m_appearances.Size(); ++i )
	{
		CEntityAppearance &appearance = m_appearances[i];

		if ( filter && filter->Empty() == false && ! filter->Exist( appearance.GetName() ) )
		{
			continue;
		}

		appearances.PushBack( &appearance );
	}

	for ( Uint32 i = 0; i < m_includes.Size(); ++i )
	{
		CEntityTemplate *templ = m_includes[i].Get();
		if ( templ )
		{
			templ->GetAllAppearances( appearances, filter );
		}
	}
}

const CEntityAppearance * CEntityTemplate::GetAppearance( const CName &_name, Bool recursive /*= true*/ ) const
{
	CName name = SEntityTemplateModifierManager::GetInstance().EntityTemplateOnGetAppearance( this, _name );
	if( name == CName::NONE )
	{
		name = _name;
	}

	if ( IsCooked( ) )
	{
		for ( Uint32 i = 0; i < m_appearances.Size(); ++i )
		{
			const Bool canTest = recursive || !m_appearances[i].WasIncluded();
			if ( canTest && m_appearances[i].GetName() == name )
			{
				return &m_appearances[i];
			}
		}
	}
	else
	{
		for ( Uint32 i = 0; i < m_appearances.Size(); ++i )
		{
			if ( m_appearances[i].GetName() == name )
			{
				return &m_appearances[i];
			}
		}

		if ( recursive )
		{
			for ( Uint32 i = 0; i < m_includes.Size(); ++i )
			{
				const CEntityTemplate *templ = m_includes[i].Get();
				if ( templ )
				{
					const CEntityAppearance * appearance = templ->GetAppearance( name, true );
					if ( appearance )
					{
						return appearance;
					}
				}
			}
		}
	}

	return NULL;
}

CName CEntityTemplate::GetApperanceVoicetag( const CName& appearanceName ) const
{
	Uint32 matchFound = 0;
	CName voicetagName;
	CStandardRand random;

	for ( TDynArray< VoicetagAppearancePair >::const_iterator voicetagAppearanceIter = m_voicetagAppearances.Begin();
		voicetagAppearanceIter != m_voicetagAppearances.End(); ++voicetagAppearanceIter )
	{
		const VoicetagAppearancePair& voicetagAppearancePair = *voicetagAppearanceIter;

		if ( voicetagAppearancePair.m_appearance == appearanceName && !voicetagAppearancePair.m_voicetag.Empty() )
		{
			++matchFound;

			if ( random.Get< Uint32 >( matchFound ) == 0 )
				voicetagName = voicetagAppearancePair.m_voicetag;
		}
	}
	
	return voicetagName;
}

void CEntityTemplate::RefreshVoicetagAppearances()
{
	TDynArray< CEntityAppearance* > allAppearances;
	GetAllAppearances( allAppearances );

	for ( TDynArray< CEntityAppearance* >::const_iterator appearanceIter = allAppearances.Begin();
		appearanceIter != allAppearances.End(); ++appearanceIter )
	{
		const CEntityAppearance* appearance = *appearanceIter;
		ASSERT( appearance != NULL );

		Bool appearanceHasVoicetag = false;
		for ( TDynArray< VoicetagAppearancePair >::iterator pairIter = m_voicetagAppearances.Begin();
			pairIter != m_voicetagAppearances.End(); ++pairIter )
		{
			VoicetagAppearancePair& pair = *pairIter;
			if ( pair.m_voicetag == appearance->GetVoicetag() )
			{
				if ( m_usedAppearances.Exist( appearance->GetName() ) == false )
				{
					pair.m_appearance = CName::NONE;
				}
				else if ( pair.m_appearance == CName::NONE )
				{
					pair.m_appearance = appearance->GetName();
				}

				appearanceHasVoicetag = true;
				break;
			}
		}

		if ( appearanceHasVoicetag == false )
		{
			CName appearanceName;
			if ( m_usedAppearances.Exist( appearance->GetName() ) == true )
			{
				appearanceName = appearance->GetName();
			}
			m_voicetagAppearances.PushBack( VoicetagAppearancePair( appearance->GetVoicetag(), appearanceName ) );
		}
	}
}

Bool CEntityTemplate::AddAppearance( const CEntityAppearance& appearance )
{
	if ( GetAppearance( appearance.GetName(), true ) != NULL )
		return false;

	m_appearances.PushBack( appearance );
	return true;
}

void CEntityTemplate::RemoveAppearance( const CEntityAppearance& appearance )
{
	TDynArray<CEntityAppearance>::iterator found = Find( m_appearances.Begin(), m_appearances.End(), appearance );
	if ( found == m_appearances.End() )
		return;

	m_appearances.Erase( found );
}

/************************************************************************/
/* Effects                                                              */
/************************************************************************/

IFile& operator<<( IFile& file, CEntityTemplateCookedEffectLegacy& cookedEffect )
{
	file << cookedEffect.m_name;
	file << cookedEffect.m_animName;
	file << cookedEffect.m_offset;
	file << cookedEffect.m_size;
	return file;
}

EntitySlot::EntitySlot()
	: m_freePositionAxisX( false )
	, m_freePositionAxisY( false )
	, m_freePositionAxisZ( false )
	, m_freeRotation( false )
	, m_wasIncluded( false )
{
}

EntitySlot::EntitySlot( const CName& slotName, const EntitySlotInitInfo* initInfo )
	: m_name( slotName )
	, m_freePositionAxisX( false )
	, m_freePositionAxisY( false )
	, m_freePositionAxisZ( false )
	, m_freeRotation( false )
	, m_wasIncluded( false )
{
	if ( initInfo )
	{
		m_componentName = initInfo->m_componentName;
		m_boneName = initInfo->m_boneName;
		m_transform = initInfo->m_transform;
		m_freePositionAxisX = initInfo->m_freePositionAxisX;
		m_freePositionAxisY = initInfo->m_freePositionAxisY;
		m_freePositionAxisZ = initInfo->m_freePositionAxisZ;
		m_freeRotation = initInfo->m_freeRotation;
	}
}

void EntitySlot::SetIncluded()
{
	m_wasIncluded = true;
}

void EntitySlot::SetBoneName( const CName& boneName )
{
	m_boneName = boneName;
}

bool EntitySlot::InitializeFromComponent( CEntity* entity, CComponent* component )
{
	return false;
}

bool EntitySlot::CalcMatrix( CEntity const * entity, Matrix& outLocalToWorld, String* outError ) const
{
	// No entity
	if ( !entity )
	{
		if ( outError ) *outError = TXT("No entity specified");
		return false;
	}

	// Calculate relative matrix
	Matrix localToParent;
	m_transform.CalcLocalToWorld( localToParent );

	// Parent component case
	if ( m_componentName )
	{
		// Find component
		CComponent const * baseComponent = entity->FindComponent( m_componentName );
		if ( !baseComponent )
		{
			if ( outError )
			{
				*outError = String::Printf( TXT( "Component '%ls' not found in entity '%ls'" ),
					m_componentName.AsString().AsChar(),
					entity->GetFriendlyName().AsChar() );
			}

			return false;
		}

		// Bone case
		if ( m_boneName )
		{
			// We need skeleton provider here
			const ISkeletonDataProvider* skeleton = baseComponent->QuerySkeletonDataProvider();
			if ( !skeleton )
			{
				if ( outError )
				{
					*outError = String::Printf( TXT("Component '%ls' does not implement skeleton provider"), m_componentName.AsString().AsChar() );
				}

				return false;
			}

			// Find bone
			Int32 boneIndex = skeleton->FindBoneByName( m_boneName );
			if ( boneIndex == -1 )
			{
				if ( outError )
				{
					*outError = String::Printf( TXT("Component '%ls' does not have bone '%ls'"), m_componentName.AsString().AsChar(), m_boneName.AsString().AsChar() );
				}

				return false;
			}

			// Query bone matrix and calculate final matrix
			const Matrix parentToWorld = skeleton->GetBoneMatrixWorldSpace( boneIndex );
			outLocalToWorld = localToParent * parentToWorld;
		}
		else
		{
			// Calculate final matrix using component base
			const Matrix parentToWorld = baseComponent->GetLocalToWorld();
			outLocalToWorld = localToParent * parentToWorld;
		}
	}
	else
	{
		// No component, just relative transform vs entity
		const Matrix parentToWorld = entity->GetLocalToWorld();
		outLocalToWorld = localToParent * parentToWorld;
	}

	// Calculated
	return true;
}

#ifndef NO_EDITOR

Bool EntitySlot::SetTransformWorldSpace( CEntity* entity, const Vector* posWS, const EulerAngles* rotWS )
{
	// No entity
	if ( !entity )
	{
		return false;
	}

	if ( !posWS && !rotWS )
	{
		// Nothing to do
		return false;
	}

	// Calculate relative matrix
	Matrix localToParent;
	m_transform.CalcLocalToWorld( localToParent );

	Matrix parentToWorld;

	// Parent component case
	if ( m_componentName )
	{
		// Find component
		CComponent* baseComponent = entity->FindComponent( m_componentName );
		if ( !baseComponent )
		{
			return false;
		}

		// Bone case
		if ( m_boneName )
		{
			// We need skeleton provider here
			const ISkeletonDataProvider* skeleton = baseComponent->QuerySkeletonDataProvider();
			if ( !skeleton )
			{
				return false;
			}

			// Find bone
			Int32 boneIndex = skeleton->FindBoneByName( m_boneName );
			if ( boneIndex == -1 )
			{
				return false;
			}

			// Query bone matrix and calculate final matrix
			parentToWorld = skeleton->GetBoneMatrixWorldSpace( boneIndex );
			
		}
		else
		{
			// Calculate final matrix using component base
			parentToWorld = baseComponent->GetLocalToWorld();
		}
	}
	else
	{
		// No component, just relative transform vs entity
		parentToWorld = entity->GetLocalToWorld();
	}

	// Calc new transform in local space
	Matrix newWS( localToParent * parentToWorld );
	if ( rotWS )
	{
		Vector temp = newWS.GetTranslation();
		newWS = rotWS->ToMatrix();
		newWS.SetTranslation( temp );
	}
	if ( posWS )
	{
		newWS.SetTranslation( *posWS );
	}

	localToParent = newWS * parentToWorld.FullInverted();

	m_transform.Init( localToParent );

	// Calculated
	return true;
}

#endif

IAttachment* EntitySlot::CreateAttachment( const HardAttachmentSpawnInfo& baseSpawnInfo, CEntity* entity, CNode* nodeToAttach, String* outError ) const
{
	HardAttachmentSpawnInfo spawnInfo( baseSpawnInfo );

	// No entity
	if ( !entity )
	{
		if ( outError ) *outError = TXT("No entity specified");
		return nullptr;
	}

	// Find node to attach to
	CNode* attachmentNode = NULL;
	if ( m_componentName )
	{
		attachmentNode = entity->FindComponent( m_componentName );
		if ( attachmentNode == nullptr )
		{
			if ( outError )
			{
				*outError = String::Printf( TXT( "Component '%ls' not found in entity '%ls'" ),
					m_componentName.AsString().AsChar(),
					entity->GetFriendlyName().AsChar() );
			}
			return nullptr;
		}
	}
	else
	{
		// Use the entity itself
		attachmentNode = entity;
	}

	// Bone case
	if ( m_boneName )
	{
		// We need skeleton provider here
		const ISkeletonDataProvider* skeleton = attachmentNode->QuerySkeletonDataProvider();
		if ( !skeleton )
		{
			if ( outError )
			{
				*outError = String::Printf( TXT("Component '%ls' does not implement skeleton provider"), m_componentName.AsString().AsChar() );
			}

			return nullptr;
		}

		// Find bone
		Int32 boneIndex = skeleton->FindBoneByName( m_boneName );
		if ( boneIndex == -1 )
		{
			if ( outError )
			{
				*outError = String::Printf( TXT("Component '%ls' does not have bone '%ls'"), m_componentName.AsString().AsChar(), m_boneName.AsString().AsChar() );
			}

			return nullptr;
		}

		// Use bone name
		spawnInfo.m_parentSlotName = m_boneName;
	}
	else
	{
		// No slot
		spawnInfo.m_parentSlotName = CName::NONE;
	}

	// Setup relative position and rotation
	spawnInfo.m_freePositionAxisX |= m_freePositionAxisX;
	spawnInfo.m_freePositionAxisY |= m_freePositionAxisY;
	spawnInfo.m_freePositionAxisZ |= m_freePositionAxisZ;
	spawnInfo.m_freeRotation |= m_freeRotation;

	// Calculate new relative position	
	const EngineTransform baseTransform( baseSpawnInfo.m_relativePosition, baseSpawnInfo.m_relativeRotation );
	Matrix baseMatrix, slotMatrix;
	baseTransform.CalcLocalToWorld( baseMatrix );
	m_transform.CalcLocalToWorld( slotMatrix );
	const Matrix finalMatrix = baseMatrix * slotMatrix;
	EngineTransform finalTransform( finalMatrix );
	spawnInfo.m_relativePosition = finalTransform.GetPosition();
	spawnInfo.m_relativeRotation = finalTransform.GetRotation();

	// Create attachment
	return attachmentNode->Attach( nodeToAttach, spawnInfo );
}

const EntitySlot* CEntityTemplate::FindSlotByName( const CName& slotName, Bool recursive ) const
{
	// Local search
	for ( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		const EntitySlot& slot = m_slots[i];
		if ( slot.GetName() == slotName )
		{
			return &slot;
		}
	}

	// Pass to included templates
	if ( recursive )
	{
		const TDynArray< THandle< CEntityTemplate > >& includes = m_includes;
		for ( Uint32 i=0; i<includes.Size(); ++i )
		{
			CEntityTemplate* entityTemplate = includes[i].Get();
			if ( entityTemplate )
			{
				const EntitySlot* slot = entityTemplate->FindSlotByName( slotName, recursive );
				if ( slot )
				{
					return slot;
				}
			}
		}
	}

	// Not found
	return NULL;
}

bool CEntityTemplate::AddSlot( const CName& name, const EntitySlotInitInfo* initInfo/*=NULL*/, Bool markAsModifie /*= true*/  )
{
	if ( markAsModifie == false || MarkModified() )
	{
		// Find by name
		for ( Uint32 i=0; i<m_slots.Size(); ++i )
		{
			const EntitySlot& slot = m_slots[i];
			if ( slot.GetName() == name )
			{
				// Already added, remove it from list
				m_slots.RemoveAt( i );
				break;
			}
		}

		// Create new slot
		::new ( m_slots ) EntitySlot( name, initInfo );
		return true;
	}

	// Not added
	return false;
}

void CEntityTemplate::AddSlot( const CName& name, const EntitySlot& entitySlot, Bool markAsModifie /*= true*/  )
{
	if ( markAsModifie == false || MarkModified() )
	{
		// Find by name
		for ( Uint32 i=0; i<m_slots.Size(); ++i )
		{
			const EntitySlot& slot = m_slots[i];
			if ( slot.GetName() == name )
			{
				// Already added, remove it from list
				m_slots.RemoveAt( i );
				break;
			}
		}

		// Create new slot
		m_slots.PushBack( entitySlot );
	}
}

bool CEntityTemplate::RemoveSlot( const CName& name, Bool markAsModifie /*= true*/  )
{
	// Find by name
	for ( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		const EntitySlot& slot = m_slots[i];
		if ( slot.GetName() == name )
		{
			if ( markAsModifie == false || MarkModified() )
			{
				m_slots.RemoveAt( i );
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	// Not found
	return false;
}

void CEntityTemplate::CollectSlots( TDynArray< const EntitySlot* >& slots, Bool recursive ) const
{
	// Collect local slots
	for ( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		slots.PushBack( &m_slots[i] );
	}

	// Recurse
	for ( Uint32 i=0; i<m_includes.Size(); ++i )
	{
		CEntityTemplate* entityTemplate = m_includes[i].Get();
		if ( entityTemplate )
		{
			entityTemplate->CollectSlots( slots, recursive );
		}
	}	
}

#ifndef NO_EDITOR

Bool CEntityTemplate::SetSlotTransform( CEntity* entity, const CName& name, const Vector* posWS, const EulerAngles* rotWS )
{
	// Find local slot
	for ( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		EntitySlot& slot = m_slots[i];

		if ( slot.GetName() == name )
		{
			return slot.SetTransformWorldSpace( entity, posWS, rotWS );
		}
	}
	return false;
}

#endif

/************************************************************************/
/* Coloring Entries                                                     */
/************************************************************************/
void CEntityTemplate::AddColoringEntry( const SEntityTemplateColoringEntry& entry )
{
	// Try to find an existing entry with the same appearance and component
	// name in the local coloring entries array
	for ( Uint32 i=0; i<m_coloringEntries.Size(); ++i )
	{
		SEntityTemplateColoringEntry& existing = m_coloringEntries[i];
		if ( entry.m_appearance == existing.m_appearance &&
			 entry.m_componentName == existing.m_componentName )
		{
			existing.m_colorShift1 = entry.m_colorShift1;
			existing.m_colorShift2 = entry.m_colorShift2;
			return;
		}
	}

	// No such entry exists, add it
	m_coloringEntries.PushBack( entry );
}

void CEntityTemplate::RemoveColoringEntry( const CName& appearance, const CName& componentName )
{
	// Find and remove a coloring entry from the local array
	for ( Uint32 i=0; i<m_coloringEntries.Size(); ++i )
	{
		SEntityTemplateColoringEntry& entry = m_coloringEntries[i];
		if ( appearance == entry.m_appearance &&
			 componentName == entry.m_componentName )
		{
			m_coloringEntries.RemoveAt( i );
			break;
		}
	}
}

void CEntityTemplate::RemoveAllColoringEntries()
{
	// Clear all local coloring entries
	m_coloringEntries.Clear();
}

void CEntityTemplate::UpdateColoringEntry( const CName& appearance, const CName& componentName, const CColorShift& colorShift1, const CColorShift& colorShift2 )
{
	// Find and update a local coloring entry
	for ( Uint32 i=0; i<m_coloringEntries.Size(); ++i )
	{
		SEntityTemplateColoringEntry& entry = m_coloringEntries[i];
		if ( appearance == entry.m_appearance &&
			 componentName == entry.m_componentName )
		{
			entry.m_colorShift1 = colorShift1;
			entry.m_colorShift2 = colorShift2;
			break;
		}
	}
}

Bool CEntityTemplate::FindColoringEntry( const CName& appearance, const CName& componentName, CColorShift& colorShift1, CColorShift& colorShift2 ) const
{
	// NOTE: at this time there is no support for merging coloring entries
	//       in cooked templates.  Fix this and remove the comment from
	//       here and GetColoringEntriesForAppearance below.    -badsector


	// Search in our own coloring entries first
	for ( Uint32 i=0; i<m_coloringEntries.Size(); ++i )
	{
		const SEntityTemplateColoringEntry& entry = m_coloringEntries[i];
		if ( appearance == entry.m_appearance &&
			 componentName == entry.m_componentName )
		{
			colorShift1 = entry.m_colorShift1;
			colorShift2 = entry.m_colorShift2;
			return true;
		}

	}

	// No entry was found locally, search in imported coloring entries
	for ( Uint32 i=0; i<m_includes.Size(); ++i )
	{
		CEntityTemplate* tpl = m_includes[i].Get();
		if ( tpl )
		{
			if ( tpl->FindColoringEntry( appearance, componentName, colorShift1, colorShift2 ) )
			{
				return true;
			}
		}
	}

	// Failed to find an entry
	return false;
}

void CEntityTemplate::GetColoringEntriesForAppearance( const CName& appearance, TDynArray<SEntityTemplateColoringEntry>& entries ) const
{
	// TODO: at this time there is no support for merging coloring entries
	//       in cooked templates.  Fix this and remove the comment from
	//       here and FindColoringEntry above.                  -badsector


	// Collect our own coloring entries
	for ( Uint32 i=0; i<m_coloringEntries.Size(); ++i )
	{
		const SEntityTemplateColoringEntry& entry = m_coloringEntries[i];
		if ( appearance == entry.m_appearance )
		{
			entries.PushBack( entry );
		}
	}

	// Collect coloring entries from included templates
	for ( Uint32 i=0; i<m_includes.Size(); ++i )
	{
		CEntityTemplate* tpl = m_includes[i].Get();
		if ( tpl )
		{
			tpl->GetColoringEntriesForAppearance( appearance, entries );
		}
	}
}


/************************************************************************/
/* Instance Property Entries                                            */
/************************************************************************/
void CEntityTemplate::ClearInstancePropertyEntries()
{
	m_instancePropEntries.Clear();
}

void CEntityTemplate::AddInstancePropertyEntry( const CName& componentName, const CName& propertyName )
{
	// Check if we already have an entry
	for ( auto it=m_instancePropEntries.Begin(); it != m_instancePropEntries.End(); ++it )
	{
		const SComponentInstancePropertyEntry& entry = *it;
		if ( entry.m_component == componentName && entry.m_property == propertyName )
		{
			return;
		}
	}

	// Add a new entry
	m_instancePropEntries.Grow();
	m_instancePropEntries.Back().m_component = componentName;
	m_instancePropEntries.Back().m_property = propertyName;
}

void CEntityTemplate::RemoveInstancePropertyEntry( const CName& componentName, const CName& propertyName )
{
	// Scan the instance property entries for the given entry
	for ( auto it=m_instancePropEntries.Begin(); it != m_instancePropEntries.End(); ++it )
	{
		const SComponentInstancePropertyEntry& entry = *it;
		if ( entry.m_component == componentName && entry.m_property == propertyName )
		{
			m_instancePropEntries.RemoveAt( it - m_instancePropEntries.Begin() );
			return;
		}
	}
}

void CEntityTemplate::GetInstancePropertiesForComponent( const CName& componentName, TDynArray< CName >& propertyNames ) const
{
	// Scan the instance property entries for the given component
	for ( auto it=m_instancePropEntries.Begin(); it != m_instancePropEntries.End(); ++it )
	{
		const SComponentInstancePropertyEntry& entry = *it;
		if ( entry.m_component == componentName )
		{
			propertyNames.PushBackUnique( entry.m_property );
		}
	}

	// Scan includes
	for ( auto it=m_includes.Begin(); it != m_includes.End(); ++it )
	{
		CEntityTemplate* inctemp = (*it).Get();
		if ( inctemp != nullptr )
		{
			inctemp->GetInstancePropertiesForComponent( componentName, propertyNames );
		}
	}
}

void CEntityTemplate::GetAllInstanceProperties( TDynArray<SComponentInstancePropertyEntry>& entries ) const
{
	entries.PushBack( GetLocalInstanceProperties() );

	// Scan includes
	for ( auto it=m_includes.Begin(); it != m_includes.End(); ++it )
	{
		CEntityTemplate* inctemp = (*it).Get();
		if ( inctemp != nullptr )
		{
			inctemp->GetAllInstanceProperties( entries );
		}
	}
}

/************************************************************************/
/* Effects                                                              */
/************************************************************************/

Red::Threads::CMutex CEntityTemplatePreloadedEffects::s_effectsPreloadedMutex;

CFXDefinition* CEntityTemplate::AddEffect( const String& effectName )
{
	// Make sure such effect does not exist
	CName realEffectName( effectName.AsChar() );
	if ( FindEffect( realEffectName ) )
	{
		WARN_ENGINE( TXT("Effect '%ls' already exists in '%ls'. Cannot create new one."), effectName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Create the effect definition
	CFXDefinition* fx = new CFXDefinition( this, realEffectName );
	if ( !fx )
	{
		WARN_ENGINE( TXT("Unable to create effect '%ls' in '%ls'."), effectName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Add to list of effects
	m_effects.PushBack( fx );
	return fx;
}

Bool CEntityTemplate::AddEffect( CFXDefinition* effect )
{
	// Make sure such effect does not exist
	if ( FindEffect( effect->GetName() ) )
	{
		WARN_ENGINE( TXT("Effect '%ls' already exists in '%ls'. Cannot paste new one."), effect->GetName().AsString().AsChar(), GetFriendlyName().AsChar() );
		return false;
	}

	effect->SetParent( this );

	// Add to list of effects
	m_effects.PushBack( effect );
	return true;
}

Bool CEntityTemplate::RemoveEffect( CFXDefinition* effect )
{
	for ( ObjectIterator< CEntity > it; it; ++it )
	{
		CEntity* entity = *it;
		if ( entity->IsPlayingEffect( effect ) )
		{
			entity->DestroyAllEffects();
		}
	}

	// Delete effect
	m_effects.Remove( effect );
	return true;
}

CFXDefinition* CEntityTemplate::FindEffect( const CName& effectName, Bool recursive /*=true*/ )
{
	m_effectsPreloaded->FlushPreloadedEffectsTo( m_effects, this );

	// Search by name
	for ( Uint32 i=0; i<m_effects.Size(); ++i )
	{
		CFXDefinition* fx = m_effects[i];
		if ( fx && fx->GetName() == effectName )
		{
			return fx;
		}
	}

	for( Uint32 i=0; i<m_cookedEffects.Size(); ++i )
	{
		if( m_cookedEffects[i].GetName() == effectName )
		{
			m_cookedEffects[i].LoadSync( m_effectsPreloaded );
			m_effectsPreloaded->FlushPreloadedEffectsTo( m_effects, this );

			// Search by name again
			for ( Uint32 i=0; i<m_effects.Size(); ++i )
			{
				CFXDefinition* fx = m_effects[i];
				if ( fx && fx->GetName() == effectName )
				{
					return fx;
				}
			}
		}
	}

	// Try in base templates
	if ( recursive )
	{
		for ( Uint32 i=0; i<m_includes.Size(); ++i )
		{
			CEntityTemplate* baseInclude = m_includes[i].Get();
			if ( baseInclude )
			{
				// Search in included template
				CFXDefinition* fx = baseInclude->FindEffect( effectName, recursive );
				if ( fx )
				{
					return fx;
				}
			}
		}
	}

	// Not found
	return NULL;
}

CFXDefinition* CEntityTemplate::FindEffectForAnimation( const CName& animationName, Bool recursive /*=true*/ )
{
	m_effectsPreloaded->FlushPreloadedEffectsTo( m_effects, this );

	// Search by name
	for ( Uint32 i=0; i<m_effects.Size(); ++i )
	{
		CFXDefinition* fx = m_effects[i];
		if ( fx && fx->GetAnimationName() == animationName )
		{
			return fx;
		}
	}

	for( Uint32 i=0; i<m_cookedEffects.Size(); ++i )
	{
		if( m_cookedEffects[i].GetAnimName() == animationName )
		{
			m_cookedEffects[i].LoadSync( m_effectsPreloaded );
			m_effectsPreloaded->FlushPreloadedEffectsTo( m_effects, this );

			// Search by name again
			for ( Uint32 i=0; i<m_effects.Size(); ++i )
			{
				CFXDefinition* fx = m_effects[i];
				if ( fx && fx->GetAnimationName() == animationName )
				{
					return fx;
				}
			}
		}
	}

	// Try in base templates
	if ( recursive )
	{
		for ( Uint32 i=0; i<m_includes.Size(); ++i )
		{
			CEntityTemplate* baseInclude = m_includes[i].Get();
			if ( baseInclude )
			{
				// Search in included template
				CFXDefinition* fx = baseInclude->FindEffectForAnimation( animationName, recursive );
				if ( fx )
				{
					return fx;
				}
			}
		}
	}

	// Not found
	return NULL;
}


Bool CEntityTemplate::HasEffect( const CName& effectName, Bool recursive /*=true*/ ) const
{

	// Search by name
	for ( Uint32 i=0; i<m_effects.Size(); i++ )
	{
		CFXDefinition* fx = m_effects[i];
		if ( fx && fx->GetName() == effectName )
		{
			return true;
		}
	}

	// Try in base templates
	if ( recursive )
	{
		for ( Uint32 i=0; i<m_includes.Size(); i++ )
		{
			CEntityTemplate* baseInclude = m_includes[i].Get();
			if ( baseInclude )
			{
				// Search in included template
				CFXDefinition* fx = baseInclude->FindEffect( effectName, recursive );
				if ( fx )
				{
					return true;
				}
			}
		}
	}

	for ( Uint32 i = 0; i < m_cookedEffects.Size(); ++i )
	{
		if ( m_cookedEffects[i].GetName() == effectName )
		{
			return true;
		}
	}

	// Not found
	return false;
}

Bool CEntityTemplate::HasEffectForAnimation( const CName& animationName, Bool recursive /*=true*/ ) const
{
	// Search by name
	for ( Uint32 i=0; i<m_effects.Size(); i++ )
	{
		CFXDefinition* fx = m_effects[i];
		if ( fx && fx->GetAnimationName() == animationName )
		{
			return true;
		}
	}

	// Try in base templates
	if ( recursive )
	{
		for ( Uint32 i=0; i<m_includes.Size(); i++ )
		{
			CEntityTemplate* baseInclude = m_includes[i].Get();
			if ( baseInclude )
			{
				// Search in included template
				CFXDefinition* fx = baseInclude->FindEffectForAnimation( animationName, recursive );
				if ( fx )
				{
					return true;
				}
			}
		}
	}

	for ( Uint32 i = 0; i < m_cookedEffects.Size(); ++i )
	{
		if ( m_cookedEffects[i].GetAnimName() == animationName )
		{
			return true;
		}
	}

	// Not found
	return false;
}

void CEntityTemplate::GetAllEffects( TDynArray< CFXDefinition* >& allEffects )
{
	m_effectsPreloaded->FlushPreloadedEffectsTo( m_effects, this );

	// Include template effects
	allEffects.PushBack( m_effects );

	// Try in base templates
	for ( Uint32 i=0; i<m_includes.Size(); i++ )
	{
		CEntityTemplate* baseInclude = m_includes[i].Get();
		if ( baseInclude )
		{
			// Search in included template
			baseInclude->GetAllEffects( allEffects );
		}
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

Bool CEntityTemplate::IsBasedOnTemplate( const CEntityTemplate* entityTemplate ) const
{
	// Same
	if ( this == entityTemplate )
	{
		return true;
	}

	// Check includes
	for ( Uint32 i=0; i<m_includes.Size(); i++ )
	{
		const CEntityTemplate* otherTemplate = m_includes[i].Get();
		if ( otherTemplate && otherTemplate->IsBasedOnTemplate( entityTemplate ) )
		{
			return true;
		}
	}

	// Not based
	return false;
}

Bool CEntityTemplate::AddParameter( CEntityTemplateParam* param )
{
	if ( !param )
	{
		return false;
	}

	param->SetParent( this );
	m_templateParams.PushBack( param );
	return true;
}

Bool CEntityTemplate::AddParameterUnique( CEntityTemplateParam* param )
{
	if ( !param )
	{
		return false;
	}

	// verify that the param we want to add doesn't share a type
	// with an existing one
	const CName& newParamType = param->GetClass()->GetName();
	for ( TDynArray< CEntityTemplateParam* >::iterator it = m_templateParams.Begin();
		it != m_templateParams.End(); ++it )
	{
		if ( *it == param )
		{
			// we don't need duplicates either
			return false;
		}

		if ( !*it || !(*it)->GetClass() )
		{
			continue;
		}
		const CName& type = (*it)->GetClass()->GetName();
		if ( type == newParamType )
		{
			// types match - we can't add this here
			return false;
		}
	}

	param->SetParent( this );
	m_templateParams.PushBack( param );
	return true;
}

Bool CEntityTemplate::RemoveParameter( CEntityTemplateParam* param )
{
	if ( !param )
	{
		return false;
	}

	// Delete effect
	m_templateParams.Remove( param );
	return true;
}

void CEntityTemplate::PreloadAllEffects()
{
	for ( Uint32 i = 0; i < m_cookedEffects.Size(); ++i )
	{
		m_cookedEffects[i].LoadAsync( m_effectsPreloaded );
	}
}

Bool CEntityTemplate::PreloadEffect( const CName& effect )
{
	for ( Uint32 i = 0; i < m_effects.Size(); ++i )
	{
		if ( m_effects[i]->GetName() == effect )
		{
			// Already present
			return true;
		}
	}

	for ( Uint32 i = 0; i < m_cookedEffects.Size(); ++i )
	{
		if ( m_cookedEffects[i].GetName() == effect )
		{
			m_cookedEffects[i].LoadAsync( m_effectsPreloaded );
			return true;
		}
	}

	return false;
}

Bool CEntityTemplate::PreloadEffectForAnimation( const CName& animName )
{
	for ( Uint32 i = 0; i < m_effects.Size(); ++i )
	{
		if ( m_effects[i]->GetAnimationName() == animName )
		{
			// Already present
			return true;
		}
	}

	for ( Uint32 i = 0; i < m_cookedEffects.Size(); ++i )
	{
		if ( m_cookedEffects[i].GetAnimName() == animName )
		{
			m_cookedEffects[i].LoadAsync( m_effectsPreloaded );
			return true;
		}
	}

	return false;
}

void CEntityTemplate::OnFinalize()
{
	SEntityTemplateModifierManager::GetInstance().EntityTemplateOnFinalize( this );
	TBaseClass::OnFinalize();
}

void CEntityTemplate::GetAllEffectsNames( TDynArray< CName >& allEffects ) const
{
	// Include template effects
	for ( Uint32 i = 0; i < m_effects.Size(); ++i )
	{
		allEffects.PushBackUnique( m_effects[i]->GetName() );
	}

	// Try in base templates
	for ( Uint32 i=0; i<m_includes.Size(); i++ )
	{
		CEntityTemplate* baseInclude = m_includes[i].Get();
		if ( baseInclude )
		{
			// Search in included template
			baseInclude->GetAllEffectsNames( allEffects );
		}
	}

	for ( Uint32 i = 0; i < m_cookedEffects.Size(); ++i )
	{
		allEffects.PushBackUnique( m_cookedEffects[i].GetName() );
	}
}

static void funcPreloadEffectForAnimationForEntityTemplate( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, THandle< CEntityTemplate >() );
	GET_PARAMETER( CName, animName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false;
	CEntityTemplate* templ = entityTemplate.Get();
	if ( templ )
	{
		ret = templ->PreloadEffectForAnimation( animName );
	}

	RETURN_BOOL( ret );
}

static void funcPreloadEffectForEntityTemplate( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, THandle< CEntityTemplate >() );
	GET_PARAMETER( CName, effectName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false;
	CEntityTemplate* templ = entityTemplate.Get();
	if ( templ )
	{
		ret = templ->PreloadEffect( effectName );
	}

	RETURN_BOOL( ret );
}

static void funcGetAppearanceNames( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, THandle< CEntityTemplate >() );
	GET_PARAMETER_REF( TDynArray< CName >, output, TDynArray< CName >() );
	FINISH_PARAMETERS;

	CEntityTemplate* templ = entityTemplate.Get();
	if ( templ )
	{
		const TDynArray< CEntityAppearance >& ret = templ->GetAppearances();

		Uint32 num = ret.Size();
		output.Resize( num );
		for ( Uint32 i = 0; i < num; ++i )
		{
			output[i] = ret[i].GetName();
		}
	}
}

void RegisterEntityTemplateFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "PreloadEffectForEntityTemplate",	funcPreloadEffectForEntityTemplate );
	NATIVE_GLOBAL_FUNCTION( "PreloadEffectForAnimationForEntityTemplate",	funcPreloadEffectForAnimationForEntityTemplate );
	NATIVE_GLOBAL_FUNCTION( "GetAppearanceNames",	funcGetAppearanceNames );
}


///////////////////////////////////////////////////////////////////////////////

void CEntityTemplate::CollectGameplayParams( TDynArray< CGameplayEntityParam* >& params, CClass* ofClass, Bool exploreTemplateList /*= true*/, Bool recursive /*= true*/ ) const
{
	//it is used for collecting CGameplayEntityParam's only
	ASSERT( ofClass->IsBasedOn( CGameplayEntityParam::GetStaticClass() ) );
	Bool isCooked								= IsCooked();
	Bool overrideInherited						= false;
#ifndef RED_FINAL_BUILD
	Bool hasTemplateList						= false;
#endif

	for ( auto i = m_templateParams.Begin(), end = m_templateParams.End(); i != end; ++i )
	{
		CEntityTemplateParam* param = *i;
		if ( param )
		{
			const Bool canTest = !isCooked || ( recursive || param->WasIncluded() == false );
			if ( canTest && param->IsA( ofClass ) )
			{
				CGameplayEntityParam* gameplayParamFound = static_cast< CGameplayEntityParam* >( param );
				params.PushBackUnique( gameplayParamFound );	
				
				if ( gameplayParamFound->OverrideInherited() )
				{
					overrideInherited = true;
				}
			}
#ifndef RED_FINAL_BUILD
			else if ( !isCooked && exploreTemplateList )
			{
				if ( param->IsA< CTemplateListParam >() )
				{
					CTemplateListParam*const templateListParam = static_cast< CTemplateListParam *>(param);
					for ( auto templateIt = templateListParam->m_templateList.Begin(), templateEnd = templateListParam->m_templateList.End(); templateIt != templateEnd; ++templateIt )
					{
						// Do not explore template lists recusively ( includes ) if this template has a template list 
						hasTemplateList = true;
						CEntityTemplate* entityTemplate = (*templateIt).Get();
						if ( entityTemplate )
						{
							Uint32 prevSize = params.Size();
							// need exploreTemplateList = true so that we may have aipresets sat-up with AIWizard
							entityTemplate->CollectGameplayParams( params, ofClass, true, recursive );

							if ( !overrideInherited )
							{
								// update override inherited flag
								for ( Uint32 i = prevSize, n = params.Size(); i < n; ++i )
								{
									if ( params[ i ]->OverrideInherited() )
									{
										overrideInherited = true;
									}
								}
							}
						}
					}
				}
			}
#endif			// #ifndef RED_FINAL_BUILD
		}
	}

#ifndef RED_FINAL_BUILD
	if ( hasTemplateList )
	{
		exploreTemplateList = false;
	}

	if ( !isCooked && recursive && !overrideInherited )
	{
		for ( auto i = m_includes.Begin(), end = m_includes.End(); i != end; ++i )
		{
			CEntityTemplate *templ = i->Get();
			if ( templ )
			{
				templ->CollectGameplayParams( params, ofClass, exploreTemplateList, recursive );
			}
		}
	}
#endif			// #ifndef RED_FINAL_BUILD
}

///////////////////////////////////////////////////////////////////////////////

CGameplayEntityParam * CEntityTemplate::FindGameplayParam( CClass* ofClass, Bool checkIncludes /*= false */, Bool exploreTemplateList /*= true */ ) const
{
	CGameplayEntityParam* gameplayParamFound = NULL;
	Bool hasTemplateList	= false;
#ifndef RED_FINAL_BUILD
	Bool isCooked			= IsCooked();
#endif

	for ( auto i = m_templateParams.Begin(), end = m_templateParams.End(); i != end; ++i )
	{
		if ( CEntityTemplateParam* param = *i )
		{
			if ( param->IsA( ofClass ) )
			{
				gameplayParamFound = Cast< CGameplayEntityParam >( param );
			}
#ifndef RED_FINAL_BUILD
			else if ( !isCooked && exploreTemplateList )
			{
				if ( param->IsA< CTemplateListParam >() )
				{
					CTemplateListParam*const templateListParam = static_cast< CTemplateListParam *>(param);
					for ( auto templateIt = templateListParam->m_templateList.Begin(), templateEnd = templateListParam->m_templateList.End(); templateIt != templateEnd; ++templateIt )
					{
						// Do not explore template lists recusively ( includes ) if this template has a template list 
						hasTemplateList = true;
						CEntityTemplate* entityTemplate = (*templateIt).Get();
						if ( entityTemplate )
						{
							// need exploreTemplateList = true so that we may have aipresets sat-up with AIWizard
							gameplayParamFound = entityTemplate->FindGameplayParam( ofClass, checkIncludes, true );

							if ( gameplayParamFound )
							{
								break;
							}
						}
					}
				}
			}
#endif			// #ifndef RED_FINAL_BUILD
		}
	}
	if ( hasTemplateList )
	{
		exploreTemplateList = false;
	}

#ifndef RED_FINAL_BUILD
	if ( checkIncludes && !isCooked && ( gameplayParamFound == NULL || gameplayParamFound->OverrideInherited() == false ) ) 			  	
	{
		for ( auto i = m_includes.Begin(), end = m_includes.End(); i != end; ++i )
		{
			if ( CEntityTemplate *templ = i->Get() )
			{
				CGameplayEntityParam* incParFound = templ->FindGameplayParam( ofClass, checkIncludes, exploreTemplateList );
				if ( incParFound )
				{
					return incParFound;
				}
			}
		}
	}
#endif

	return gameplayParamFound;
}

Bool CEntityTemplate::ExportAppearance( const CName &name, CEntityAppearance& appearance ) const
{
	const CEntityAppearance *foundAppearance = GetAppearance( name, false );
	if( foundAppearance )
	{
		appearance = *foundAppearance;
		return true;
	}
	return false;
}

Bool CEntityTemplate::ImportAppearance( const CName &name, const CEntityAppearance& appearance )
{
	const CEntityAppearance *foundAppearance = GetAppearance( name, false );
	if( foundAppearance == nullptr )
	{
		return AddAppearance( appearance );
	}

	return false;
}

#ifndef NO_EDITOR
void CEntityTemplate::GetAdditionalInfo( TDynArray< String >& info ) const
{
	CEntity* entity = CreateInstance( nullptr, EntityTemplateInstancingInfo() );
	if ( entity != nullptr )
	{
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld, true, nullptr, false );
		Uint32 componentCount = entity->GetComponents().Size();
		String isStreamed = entity->ShouldBeStreamed() ? TXT("Yes") : TXT("No");
		info.PushBack( String::Printf( TXT("Component count: %d"), componentCount) );
		if ( entity->ShouldBeStreamed() )
		{
			info.PushBack( String::Printf( TXT("Streamed at %im"), (int)entity->GetStreamingDistance() ) );
		}
		else
		{
			info.PushBack( TXT("Not streamed") );
		}
		entity->Discard();
	}
	else
	{
		info.PushBack( TXT("Error in entity template!") );
	}
}
#endif
