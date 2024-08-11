/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityTemplateParams.h"
#include "appearanceComponent.h"
#include "../core/dependencySaver.h"
#include "externalProxy.h"
#include "componentIterator.h"
#include "entity.h"
#include "entityTemplate.h"
#include "layer.h"
#include "../core/events.h"
#include "fxDefinition.h"
#include "drawableComponent.h"
#include "animatedComponent.h"

// NOTE: at this time there is no support for merging coloring entries
//       in cooked templates (not testable ATM).  Fix this and remove the
//       comment from here in CEntityTemplate.                 -badsector

#ifndef NO_RESOURCE_COOKING

static void FillActorVoiceRandomize( CEntityTemplate* baseTemplate )
{
	// Avoid bad juju
	if ( baseTemplate == nullptr || baseTemplate->GetEntityObject() == nullptr )
	{
		return;
	}

	// Check if this entity template is an actor
	CClass* actorClass = SRTTI::GetInstance().FindClass( CName( TXT("CActor") ) );
	if ( !baseTemplate->GetEntityObject()->GetClass()->IsA( actorClass ) )
	{
		return;
	}

	// Find the first actor ancestor with voiceToRandomize
	TDynArray< StringAnsi >* voices = nullptr;
	struct {
		CClass* actorClass;
		CProperty* vtrProp;
		Bool Scan( CEntityTemplate* tpl, TDynArray< StringAnsi >*& voices )
		{
			CEntity* tplActor = tpl->GetEntityObject();

			// Found actor, check the voices
			if ( tplActor->IsA( actorClass ) )
			{
				TDynArray< StringAnsi >* voiceToRandomize = (TDynArray< StringAnsi >*)vtrProp->GetOffsetPtr( tplActor );
				if ( !voiceToRandomize->Empty() )
				{
					voices = voiceToRandomize;
					return true;
				}
			}
			for ( THandle< CEntityTemplate > subtpl : tpl->GetIncludes() )
			{
				if ( subtpl.IsValid() )
				{
					if ( Scan( subtpl.Get(), voices ) )
					{
						return true;
					}
				}
			}
			return false;
		}
	} local;
	local.actorClass = actorClass;
	local.vtrProp = actorClass->FindProperty( CName( TXT("voiceToRandomize") ) );
	RED_ASSERT( local.vtrProp != nullptr, TXT("Propery voiceToRandomize is removed, this will fail!") );
	if ( local.Scan( baseTemplate, voices ) )
	{
		TDynArray< StringAnsi >* targetVoices = (TDynArray< StringAnsi >*)local.vtrProp->GetOffsetPtr( baseTemplate->GetEntityObject() );
		if ( !voices->Empty() && voices != targetVoices )
		{
			targetVoices->PushBack( *voices );
		}
	}
}

static void MergeIncludedEffects( CEntityTemplate* baseTemplate, CEntityTemplate* currentTemplate, TDynArray< CFXDefinition* >& allEffects )
{
	// Clone the effects
	if ( baseTemplate != currentTemplate )
	{
		const TDynArray< CFXDefinition* >& effects = currentTemplate->GetEffects();
		for ( Uint32 i=0; i<effects.Size(); ++i )
		{
			CFXDefinition* effect = effects[i];
			ASSERT( effect->GetParent() == currentTemplate );

			// Make sure effect with such a name does not already exist in the list
			Bool isAlreadyAdded = false;
			for ( Uint32 j=0; j<allEffects.Size(); ++j )
			{
				CFXDefinition* existingEffect = allEffects[j];
				if ( existingEffect->GetName() == effect->GetName() )
				{
					isAlreadyAdded = true;
					break;
				}
			}

			// Clone and add this effect to base entity template
			if ( !isAlreadyAdded )
			{
				// Clone the crap
				CFXDefinition* clonedEffect = Cast< CFXDefinition >( effect->Clone( baseTemplate ) );
				if ( clonedEffect )
				{
					//LOG_ENGINE( TXT("Merged effect '%ls' from '%ls' to '%ls'"), effect->GetName().AsChar(), currentTemplate->GetFriendlyName().AsChar(), baseTemplate->GetFriendlyName().AsChar() );

					// Add to global list
					allEffects.PushBack( clonedEffect );
				}
			}
		}
	}

	// Process includes
	const TDynArray< THandle< CEntityTemplate > >& includes = currentTemplate->GetIncludes();
	for ( Uint32 j=0; j<includes.Size(); ++j )
	{
		CEntityTemplate* includedTemplate = includes[j].Get();
		if ( includedTemplate )
		{
			MergeIncludedEffects( baseTemplate, includedTemplate, allEffects );
		}
	}	
}

static void MergeAppearances( CEntityTemplate* baseTemplate, CEntityTemplate* currentTemplate, TDynArray< CEntityAppearance >& allAppearances )
{
	// Merge body parts
	if ( baseTemplate != currentTemplate )
	{
		const TDynArray< CEntityAppearance >& appearances = currentTemplate->GetAppearances();
		for ( Uint32 i=0; i<appearances.Size(); ++i )
		{
			const CEntityAppearance& appearance = appearances[i];

			// Do not process includes of includes here
			if ( appearance.WasIncluded() )
			{
				continue;
			}

			// Make sure it's not duplicated
			Bool isAlreadyAdded = false;
			for ( Uint32 j=0; j<allAppearances.Size(); ++j )
			{
				const CEntityAppearance& existingAppearance = allAppearances[j];
				if ( existingAppearance.GetName() == appearance.GetName() && existingAppearance.GetVoicetag() == appearance.GetVoicetag() )
				{
					isAlreadyAdded = true;
					break;
				}
			}

			// Add if not already added
			if ( !isAlreadyAdded )
			{
				//LOG_ENGINE( TXT("Merged appearance '%ls' from '%ls' to '%ls'"), appearance.GetName().AsChar(), currentTemplate->GetFriendlyName().AsChar(), baseTemplate->GetFriendlyName().AsChar() );

				// Create cloned appearance and mark it as included
				CEntityAppearance* newAppearance = ::new ( allAppearances ) CEntityAppearance( appearance, baseTemplate );
				newAppearance->SetWasIncluded();
			}
		}
	}

	// Process includes
	const TDynArray< THandle< CEntityTemplate > >& includes = currentTemplate->GetIncludes();
	for ( Uint32 j=0; j<includes.Size(); ++j )
	{
		CEntityTemplate* includedTemplate = includes[j].Get();
		if ( includedTemplate )
		{
			MergeAppearances( baseTemplate, includedTemplate, allAppearances );
		}
	}
}

static void MergeSlots( CEntityTemplate* baseTemplate, CEntityTemplate* currentTemplate, TDynArray< EntitySlot >& allSlots )
{
	// Merge body parts
	if ( baseTemplate != currentTemplate )
	{
		const TDynArray< EntitySlot >& slots = currentTemplate->GetSlots();
		for ( Uint32 i=0; i<slots.Size(); ++i )
		{
			const EntitySlot& slot = slots[i];

			// Do not process includes of includes here
			if ( slot.WasIncluded() )
			{
				continue;
			}

			// Make sure it's not duplicated
			Bool isAlreadyAdded = false;
			for ( Uint32 j=0; j<allSlots.Size(); ++j )
			{
				const EntitySlot& existingSlot = allSlots[j];
				if ( existingSlot.GetName() == slot.GetName() )
				{
					isAlreadyAdded = true;
					break;
				}
			}

			// Add if not already added
			if ( !isAlreadyAdded )
			{
				// Create cloned appearance and mark it as included
				EntitySlot* newSlot = ::new ( allSlots ) EntitySlot( slot );
				newSlot->SetIncluded();
			}
		}
	}

	// Process includes
	const TDynArray< THandle< CEntityTemplate > >& includes = currentTemplate->GetIncludes();
	for ( Uint32 j=0; j<includes.Size(); ++j )
	{
		CEntityTemplate* includedTemplate = includes[j].Get();
		if ( includedTemplate )
		{
			MergeSlots( baseTemplate, includedTemplate, allSlots );
		}
	}
}

static void MergeTemplateParameters( CEntityTemplate* baseTemplate, TDynArray< CEntityTemplateParam* >& allParameters )
{
	struct Local
	{
		struct IgnoredClassList
		{
		public:
			TDynArray< CClass* > m_ignoredClasses;									// NOTICE: its quite unhandy to iterate over and over that arrays - but they should be fairly limited in size
		private:
			TDynArray< CClass* > m_aditionalClassesIgnoredByCurrentTemplate;
		public:
			struct ExtensionGuardian
			{
				IgnoredClassList&		m_ignored;
				Uint32					m_initialSize;
				ExtensionGuardian( IgnoredClassList& ignored )
					: m_ignored( ignored )
					, m_initialSize( ignored.m_aditionalClassesIgnoredByCurrentTemplate.Size() ) {}

				void AddIgnoredClass( CClass* classPtr )
				{
					// Here it gots more tricky. We want to ignore top-most class that derive from CGameplayEntityParams that we derive from.
					CClass* gameplayEntityParamClass = CGameplayEntityParam::GetStaticClass();
					CClass* ignoredClass = classPtr;
					while( ignoredClass->GetBaseClass() != gameplayEntityParamClass )
					{
						if ( ( ignoredClass = ignoredClass->GetBaseClass() ) == nullptr )
						{
							return;
						}
					}

					auto& list = m_ignored.m_aditionalClassesIgnoredByCurrentTemplate;
					for ( Uint32 i = m_initialSize, n = list.Size(); i < n; ++i )
					{
						if ( list[ i ] == ignoredClass )
						{
							return;
						}
					}
					list.PushBack( ignoredClass );
				}

				~ExtensionGuardian()
				{
					auto& newClasses = m_ignored.m_aditionalClassesIgnoredByCurrentTemplate;
					auto& ignoredClasses = m_ignored.m_ignoredClasses;
					for ( Uint32 i = m_initialSize, n = newClasses.Size(); i < n; ++i )
					{
						ignoredClasses.PushBackUnique( newClasses[ i ] );
					}
					newClasses.ResizeFast( m_initialSize );
				}
			};
		};

		static void MergeTemplateParametersRec( CEntityTemplate* baseTemplate, CEntityTemplate* currentTemplate, TDynArray< CEntityTemplateParam* >& allParameters, IgnoredClassList& ignored, Bool exploreTemplateList )
		{
			Bool canProcessIncludes = true;
			{
				// At that code block we are browsing through parameters of current template. And so we need ExtensionGuardian to add classes that overrideInherited at this level to ignored.
				Local::IgnoredClassList::ExtensionGuardian guard( ignored );

				// look for template list - and process it first (as ignoreClass list is not yet filled up)
				const TDynArray< CEntityTemplateParam* >& paramsList = currentTemplate->GetTemplateParams();
				if ( exploreTemplateList )
				{
					for ( Int32 i = paramsList.Size() - 1; i >= 0; --i )
					{
						CEntityTemplateParam* param = paramsList[ i ];
						if ( param && param->IsA< CTemplateListParam >() )
						{
							// don't explore template lists on lower levels
							exploreTemplateList = false;

							// iterate over included templates
							CTemplateListParam* templateListParam = static_cast< CTemplateListParam* >( param );
							for ( const THandle< CEntityTemplate >& entityTemplateHandle : templateListParam->m_templateList )
							{
								if ( CEntityTemplate* entityTemplate = entityTemplateHandle.Get() )
								{
									Uint32 ignoreClassesSize =  ignored.m_ignoredClasses.Size();
									// merge gameplay parameters from included templates
									MergeTemplateParametersRec( baseTemplate, entityTemplate, allParameters, ignored, true );
									// don't ignore template parameters of given type on that level - yet
									for ( Uint32 classId = ignoreClassesSize, classN = ignored.m_ignoredClasses.Size(); classId < classN; ++classId )
									{
										guard.AddIgnoredClass( ignored.m_ignoredClasses[ classId ] );
									}
									ignored.m_ignoredClasses.ResizeFast( ignoreClassesSize );
								}
							}
						}
					}
				}

				// process class parameters
				if ( baseTemplate != currentTemplate )
				{
					// Merge template parameters as we are processing subclass
					for ( Uint32 i = 0, n = paramsList.Size(); i < n; ++i )
					{
						if ( CEntityTemplateParam* param = paramsList[i] )
						{
							CClass* paramClass = param->GetClass();

							if ( param->WasIncluded() )
							{
								// That entity template parameters where already flattened. We shouldn't proceed deeper
								canProcessIncludes = false;
							}

							if ( !param->IsCooked() )
							{
								continue;
							}

							// test ignored list
							Bool isOnIgnoredList = false;
							for ( CClass* ignoredClassPtr : ignored.m_ignoredClasses )
							{
								if ( paramClass->IsA( ignoredClassPtr ) )
								{
									isOnIgnoredList = true;
									break;
								}
							}
							if ( isOnIgnoredList )
							{
								continue;
							}

							// Stats
							//LOG_ENGINE( TXT("Merged parameter '%ls' from '%ls' to '%ls'"), param->GetFriendlyName().AsChar(), currentTemplate->GetFriendlyName().AsChar(), baseTemplate->GetFriendlyName().AsChar() );

							// Create cloned appearance and mark it as included
							CEntityTemplateParam* newParam = Cast< CEntityTemplateParam >( param->Clone( baseTemplate ) );
							if ( newParam )
							{
								allParameters.PushBack( newParam );
								newParam->SetWasIncluded();

								CGameplayEntityParam* gpParam = Cast< CGameplayEntityParam >( newParam );
								if( gpParam && gpParam->OverrideInherited() )
								{
									guard.AddIgnoredClass( paramClass );
								}
							}
						}
					}
				}
				else
				{
					// review parameters for cooking as we are processing base class
					for( Int32 i = 0, n = paramsList.Size(); i < n; ++i )
					{
						if ( CEntityTemplateParam* param = paramsList[i] )
						{
							if ( !param->IsCooked() )
							{
								continue;
							}

							allParameters.PushBack( param );

							if ( CGameplayEntityParam* gpParam = Cast< CGameplayEntityParam >( param ) )
							{
								if( gpParam->OverrideInherited() )
								{
									guard.AddIgnoredClass( gpParam->GetClass() );
								}
							}
						}
					}
				}
			}		// Local::IgnoredClassList::ExtensionGuardian guard( ignored );

			// Process includes
			if ( canProcessIncludes )
			{
				const TDynArray< THandle< CEntityTemplate > >& includes = currentTemplate->GetIncludes();
				for ( Uint32 j=0; j<includes.Size(); ++j )
				{
					if ( CEntityTemplate* includedTemplate = includes[j].Get() )
					{
						MergeTemplateParametersRec( baseTemplate, includedTemplate, allParameters, ignored, exploreTemplateList );
					}
				}	
			}
		}				// void MergeTemplateParametersRec()
	};					// struct Local

	Local::IgnoredClassList ignoredList;

	TDynArray< CEntityTemplateParam* > outParamList;

	Local::MergeTemplateParametersRec( baseTemplate, baseTemplate, outParamList, ignoredList, true );

	allParameters = outParamList;

}

void CEntityTemplate::CaptureData( CEntity* source, Bool keepStreamedComponents /* = false */, Bool emitModifiedEvent /* = true */ )
{
#ifndef NO_EDITOR
	ASSERT( m_entityObject != source );

	// Discard old object
	if ( m_entityObject )
	{
		m_entityObject->Discard();
		m_entityObject = NULL;
	}

	// Saved attachments from the entity
	SSavedAttachments extraAttachments;

	// Clone object locally
	if ( source )
	{
		ASSERT( !source->GetTemplate() );

		// Create the streaming components so we can re-grab them
		source->SetStreamingLock( false );
		source->CreateStreamedComponents( SWN_NotifyWorld );

		// Grab attachments
		extraAttachments.Add( source );

		// Rebuild overrides
		RebuildPropertyOverridesFromIncludes( source, true );
		m_properOverrides = true;

		// Create appearance attachments for the entity's current appearance
		CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( source );
		if ( appearanceComponent != nullptr )
		{
			appearanceComponent->UpdateCurrentAppearanceAttachments();
		}

		// Remove the "referenced" and the "transient" flag
		const TDynArray< CComponent* >& components = source->GetComponents();
		const Uint32 componentsSize = components.Size();
		for ( Uint32 i = 0; i < componentsSize; i++ )
		{
			CComponent* currentComponent = components[i];

			// If we have overrides for the component, make sure it isn't removed when saving it
			if ( HasOverridesForComponent( currentComponent ) )
			{
				currentComponent->CObject::ClearFlag( OF_Transient | OF_Referenced | NF_IncludedFromTemplate );
				continue;
			}

			// For non-included components or streamable components remove any conflicting flags
			if ( !currentComponent->HasFlag( NF_IncludedFromTemplate ) || ( source->ShouldBeStreamed() && currentComponent->IsStreamed() ) )
			{
				currentComponent->CObject::ClearFlag( OF_Transient | OF_Referenced );
			}

			// Create external proxy components for included and referenced components
			if ( currentComponent->HasFlag( NF_IncludedFromTemplate ) || currentComponent->HasFlag( OF_Referenced ) )
			{
				// Mark the component as transient so that it will be removed when serializing
				currentComponent->CObject::SetFlag( OF_Transient | OF_Referenced );

				// Create proxy
				CExternalProxyComponent* proxy = Cast< CExternalProxyComponent >( CExternalProxyComponent::GetProxyIfNeeded( *source, *currentComponent ) );
				if ( proxy )
				{
					proxy->SuckDataFromDestination( *source, *currentComponent );
				}
			}
			else // Not an external component, make sure we have no proxies pointing to it
			{
				for ( ComponentIterator<CExternalProxyComponent> it( source ); it; ++it )
				{
					CExternalProxyComponent* proxy = *it;
					if ( proxy->IsProxyFor( *currentComponent ) )
					{
						proxy->CObject::SetFlag( OF_Transient );
						break;
					}
				}
			}
		}

		// Grab new template data
		m_entityObject = SafeCast< CEntity >( source->Clone( this, false, false ) );
		RED_FATAL_ASSERT( m_entityObject != nullptr, "Unable to clone internal entity in template '%ls'", GetDepotPath().AsChar() );

		m_entityObject->SetGUID( CGUID::ZERO );
		m_entityClass = m_entityObject->GetClass()->GetName();
		m_entityObject->UpdateStreamedComponentDataBuffers();

		// Update the streaming distance in source
		m_entityObject->m_template = this; // this is needed to stream in included components
		m_entityObject->UpdateStreamingDistance();
		m_entityObject->m_template = nullptr;
	}

	// Mark template as modified
	MarkModified();

	// Compile data buffer
	CompileDataBuffer();

	// Compile full data buffer
	CreateFullDataBuffer( source->GetLayer() ? source->GetLayer() : nullptr, EntityTemplateInstancingInfo(), &extraAttachments );

	// Modified
	if ( emitModifiedEvent )
	{
		// Hack because "EntityTemplateModified" causes a second appearance attachment save
		// indirectly which clears the attachments that were removed from above
		extern Bool GLockAppearanceComponentAttachments;
		GLockAppearanceComponentAttachments = true;
		EDITOR_DISPATCH_EVENT( CNAME( EntityTemplateModified ), CreateEventData( this ) );
		GLockAppearanceComponentAttachments = false;
	}
#else // NO_EDITOR
	RED_UNUSED( source );
#endif
}

void CEntityTemplate::CompileDataBuffer()
{
	if ( IsCooked() )
		return;

	// Cleanup
	m_data.Clear();

	// Grab data
	if ( m_entityObject )
	{
		DependencySavingContext context( m_entityObject );
		context.m_saveTransient = true;
		SaveToMemoryDataArray( context, &m_data );
	}
}

void CEntityTemplate::CookDataBuffer( class ICookerFramework& cookerFramework, ECookingPlatform platform )
{
	// Merge crap
	{
		FillActorVoiceRandomize( this );
		MergeIncludedEffects( this, this, m_effects );
		MergeAppearances( this, this, m_appearances );
		MergeTemplateParameters( this, m_templateParams );

		MergeSlots( this, this, m_slots );
	}

	// Load
	EntityTemplateInstancingInfo instanceInfo;
	CLayer* layer = CreateObject< CLayer >();
	CEntity* createdEntity = CreateEntityUncached( layer, instanceInfo, nullptr, nullptr, true );
	if ( !createdEntity )
	{
		// report error
		ERR_ENGINE( TXT("!!! TEMPLATE COOKING FAILED !!!") );
		ERR_ENGINE( TXT("Unable to create uncached entity for template '%ls'"), GetDepotPath().AsChar() );

		// create default entity so we don't crash
		createdEntity = new CEntity();
		createdEntity->SetParent( layer );
	}

	// Throwing out unused appearances & components
	{
		// We cannot substract GetAllEnabledAppearances() from m_appearances because
		// the editor does NOT guarantee that ONLY the enabled appearances will
		// be used by other parts of the game, so we have to let them all go through.

		// Remove external proxy components.
		{
			TDynArray< CComponent* > componentsToRemove;
			for( Uint32 i = 0; i < createdEntity->GetComponents().Size(); ++i )
			{
				CExternalProxyComponent* epc = Cast<CExternalProxyComponent>( createdEntity->GetComponents()[i] );
				if ( epc )
				{
					componentsToRemove.PushBack( epc );
				}
			}
			for ( Uint32 i = 0; i < componentsToRemove.Size(); ++i )
			{
				createdEntity->RemoveComponent( componentsToRemove[i] );
			}
		}
	}

	// Clean up voice tags.
	{
		// Remove unused voice tag entries.
		for( Int32 i = m_voicetagAppearances.Size( ) - 1; i >= 0; --i )
		{
			VoicetagAppearancePair& pair = m_voicetagAppearances[ i ];
			if( pair.m_appearance == CName::NONE || pair.m_voicetag == CName::NONE )
			{
				m_voicetagAppearances.RemoveAt( i );
			}
		}

		// Default voice tag entries to the first available, or to NONE otherwise.
		// We could set them all to NONE, but someone might be expecting them to have a valid name.
		for( Uint32 i = 0; i < m_appearances.Size( ); ++i )
		{
			CEntityAppearance& app = m_appearances[ i ];
			app.SetVoicetag( CName::NONE );
			const CName& appearanceName = app.GetName( );
			for( TDynArray< VoicetagAppearancePair >::const_iterator voicetagAppearanceIter = m_voicetagAppearances.Begin( );
				voicetagAppearanceIter != m_voicetagAppearances.End( ); ++voicetagAppearanceIter )
			{
				const VoicetagAppearancePair& voicetagAppearancePair = *voicetagAppearanceIter;
				if( voicetagAppearancePair.m_appearance == appearanceName )
				{
					app.SetVoicetag( voicetagAppearancePair.m_voicetag );
					break;
				}
			}
		}
	}

	// Hell yeah, process all the effects
	{
		struct EffectBufferDesc
		{
			TDynArray< Uint8 > data;
			CName name;
			CName animName;
		};

		// Prepare data for final buffer
		TDynArray< EffectBufferDesc* > tempDataBuffers;
		Uint32 finalBufferSize = 0;

		for ( Uint32 i = 0; i < m_effects.Size(); ++i )
		{
			if ( m_effects[i] )
			{
				EffectBufferDesc* bufferDesc = new EffectBufferDesc();

				// Serialize to memory
				CMemoryFileWriter writer( bufferDesc->data );
				CDependencySaver saver( writer, NULL );

				// Setup cooking platform
				DependencySavingContext context( m_effects[i] );
				context.m_saveTransient = true;
				context.m_zeroNonDeterministicData = true;
				context.m_cooker = &cookerFramework;
				saver.SaveObjects( context );

				bufferDesc->name = m_effects[i]->GetName();
				bufferDesc->animName = m_effects[i]->GetAnimationName();

				tempDataBuffers.PushBack( bufferDesc );
				finalBufferSize += bufferDesc->data.Size();
			}
		}

		// Save temp data buffers to final buffer
		if( finalBufferSize > 0 )
		{
			// Allocate final buffer
			Uint32 offset = 0;

			// Fill final buffer with buffer desc data
			//for ( Uint32 i = 0; i < tempDataBuffers.Size(); ++i )

			for( auto bufDesc : tempDataBuffers )
			{
				SharedDataBuffer cookedBuffer( bufDesc->data.Data(), (Uint32)bufDesc->data.DataSize() );

				m_cookedEffects.PushBack( CEntityTemplateCookedEffect( bufDesc->name, bufDesc->animName, cookedBuffer) );
				offset += bufDesc->data.Size();

				// Delete temp buffer desc, we no longer need it
				delete bufDesc;
			}
		}

		m_cookedEffectsVersion = 1;	// New version of cooked effects

		m_effects.ClearFast();
	}

	// Now replace the old entity object (not flattened) with the newly flattened entity we just created
	createdEntity->SetParent( this );
	m_entityObject = createdEntity;

	// Regenerate buffer from the flattened entity
	CreateFullDataBufferFromEntity( createdEntity );

	// Resave streaming buffer using the cooker framework - this way we may be able to track dependencies to
	DependencySavingContext resavingContext( nullptr );
	//resavingContext.m_cooker = &cookerFramework;
	m_entityObject->ResaveStreamingBuffer( resavingContext );
	
	// cleanup
	layer->Discard();
}

#endif