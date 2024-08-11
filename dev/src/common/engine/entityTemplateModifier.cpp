#include "build.h"
#include "entityTemplateModifier.h"
#include "../core/depot.h"

/*
Ctor.
*/
CEntityTemplateModifierManager::CEntityTemplateModifierManager()
: m_active( false )
{}

/*
Dtor.
*/
CEntityTemplateModifierManager::~CEntityTemplateModifierManager()
{}

Bool CEntityTemplateModifierManager::IsModifierRegistered( const String& entityTemplatePath ) const
{
	return m_modifiers.KeyExist( entityTemplatePath.CalcHash() );
}

Bool CEntityTemplateModifierManager::RegisteModifier( const String& entityTemplateName, IEntityTemplateModifier* entityTemplateModifier )
{
	// All modifiers are registered before modifier manager is activated.
	RED_FATAL_ASSERT( !m_active, "" );

	Uint32 entityTemplateNameHash = entityTemplateName.CalcHash();
	TDynArray<IEntityTemplateModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
	if( modifiers == nullptr )
	{
		TDynArray<IEntityTemplateModifier*> newModifiers;
		newModifiers.PushBack( entityTemplateModifier );
		m_modifiers.Set( entityTemplateNameHash, newModifiers );
	}
	else
	{
		if( modifiers->FindPtr( entityTemplateModifier ) )
		{
			WARN_ENGINE( TXT("Entity template modifier already added to %ls"), entityTemplateName.AsChar() );
			return false;
		}
		modifiers->PushBack( entityTemplateModifier );
	}

	// If entity template is already loaded then we need to make sure it's on pending list or entity template will not be modified
	// when modifier manager is activated. Entity template is added to pending list when EntityTemplateOnPostLoad() is called but
	// entity template might have been loaded during previous game session (and not unloaded when session ended) which means that
	// EntityTemplateOnPostLoad() won't be called during this session.
	const CDiskFile* diskFile = GDepot->FindFile( entityTemplateName );
	if( diskFile && diskFile->IsLoaded() )
	{
		CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( diskFile->GetResource() );
		if( !m_pendingEntityTemplates.Exist( entityTemplate ) )
		{
			m_pendingEntityTemplates.PushBack( entityTemplate );
		}
	}

	return true;
}

Bool CEntityTemplateModifierManager::UnregisteModifier( const String& entityTemplateName, IEntityTemplateModifier* entityTemplateModifier )
{
	// All modifiers are unregistered after modifier manager is deactivated.
	RED_FATAL_ASSERT( !m_active, "" );

	Uint32 entityTemplateNameHash = entityTemplateName.CalcHash();
	TDynArray<IEntityTemplateModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
	if( modifiers != nullptr )
	{
		IEntityTemplateModifier** foundEntityTemplateModifie = modifiers->FindPtr( entityTemplateModifier );
		if( foundEntityTemplateModifie )
		{
			modifiers->Erase( foundEntityTemplateModifie );
			return true;
		}
	}

	WARN_ENGINE( TXT("Entity template modifier not registered for %ls"), entityTemplateName.AsChar() );
	return false;
}

void CEntityTemplateModifierManager::GetEntityTemplateIncludesNames( const CEntityTemplate* entityTemplate, TDynArray<Uint32>& entityTemplatesNameHashes )
{
	const TDynArray< THandle< CEntityTemplate > >& includeEntityTemplates = entityTemplate->GetIncludes();
	for( const THandle< CEntityTemplate >& entityTemplateHandle : includeEntityTemplates )
	{
		if( entityTemplateHandle )
		{
			entityTemplatesNameHashes.PushBackUnique( entityTemplateHandle->GetDepotPath().CalcHash() );
			GetEntityTemplateIncludesNames( entityTemplateHandle, entityTemplatesNameHashes );
		}
	}
}

void CEntityTemplateModifierManager::EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate )
{
	if( m_active )
	{
		// modifier manager is active so all pending entity templates must already have been processed
		RED_FATAL_ASSERT( m_pendingEntityTemplates.Empty(), "" );

		// on post load called twice for the same entity template
		RED_FATAL_ASSERT( !m_modifiedEntityTemplates.Exist( entityTemplate ), "" );

		const Bool entityTemplateModified = AddModification( entityTemplate );
		if( entityTemplateModified )
		{
			// record modification
			m_modifiedEntityTemplates.PushBack( entityTemplate );
		}
	}
	else
	{
		// Modifier manager is inactive so there surely are no modifications to entity template as:
		// - no modifications were done at all (because DLCs were not even activated yet),
		// - all modifications were already reverted (during DLC deactivation).
		RED_FATAL_ASSERT( m_modifiedEntityTemplates.Empty(), "" );

		// on post load called twice for the same entity template
		RED_FATAL_ASSERT( !m_pendingEntityTemplates.Exist( entityTemplate ), "" );

		// Modifier manager is inactive - push entity template to pending list.
		m_pendingEntityTemplates.PushBack( entityTemplate );
	}
}

void CEntityTemplateModifierManager::EntityTemplateOnFinalize( CEntityTemplate* entityTemplate )
{
	if( m_active )
	{
		// modifier manager is active so all pending entity templates must already have been processed
		RED_FATAL_ASSERT( m_pendingEntityTemplates.Empty(), "" );

		// remove modification from entity template if it was modified
		if( const Bool entityTemplateModified = m_modifiedEntityTemplates.Exist( entityTemplate ) )
		{
			RemoveModification( entityTemplate );
			m_modifiedEntityTemplates.Remove( entityTemplate );
		}
	}
	else
	{
		// Modifier manager is inactive so there surely are no modifications to entity template as:
		// - no modifications were done at all (because DLCs were not even activated yet),
		// - all modifications were already reverted (during DLC deactivation).
		RED_FATAL_ASSERT( m_modifiedEntityTemplates.Empty(), "" );

		// entity may be on pending list - remove it
		m_pendingEntityTemplates.Remove( entityTemplate );
	}
}

void CEntityTemplateModifierManager::Activate()
{
	// assert modifier manager is not activated twice
	RED_FATAL_ASSERT( !m_active, "" );

	// Modifier manager is inactive so there surely are no modifications to entity template as:
	// - no modifications were done at all (because DLCs were not even activated yet),
	// - all modifications were already reverted (during DLC deactivation).
	RED_FATAL_ASSERT( m_modifiedEntityTemplates.Empty(), "" );

	m_active = true;

	// process pending entity templates
	for( CEntityTemplate* et : m_pendingEntityTemplates )
	{
		const Bool entityTemplateModified = AddModification( et );
		if( entityTemplateModified )
		{
			// record modification
			m_modifiedEntityTemplates.PushBack( et );
		}
	}
	m_pendingEntityTemplates.ClearFast();
}

void CEntityTemplateModifierManager::Deactivate()
{
	// assert modifier manager is not deactivated twice
	RED_FATAL_ASSERT( m_active, "" );

	// modifier manager is active so all pending entity templates must already have been processed
	RED_FATAL_ASSERT( m_pendingEntityTemplates.Empty(), "" );

	// remove modifications
	for( CEntityTemplate* et : m_modifiedEntityTemplates )
	{
		RemoveModification( et );
	}
	m_modifiedEntityTemplates.ClearFast();

	m_active = false;
}

Bool CEntityTemplateModifierManager::IsActive() const
{
	return m_active;
}

/*
Adds modification to entity template.

\return True - entity template has been modified, false - no modifications have been made to entity template (most probably
there are no modifiers registered for this entity template).
*/
Bool CEntityTemplateModifierManager::AddModification( CEntityTemplate* entityTemplate )
{
	PC_SCOPE( EntityTemplateModifierManager );
	TDynArray<Uint32> entityTemplatesNameHashes;

	Uint32 hashNameOfModifiedEntityTemplat = entityTemplate->GetDepotPath().CalcHash();
	
	entityTemplatesNameHashes.PushBack( hashNameOfModifiedEntityTemplat );

	if( entityTemplate->IsCooked() )
	{
		GetEntityTemplateIncludesNames( entityTemplate,  entityTemplatesNameHashes );
	}

	Bool entityTemplateModified = false;

	for( const Uint32& entityTemplateNameHash : entityTemplatesNameHashes )
	{
		if( entityTemplateNameHash != 0 )
		{
			TDynArray<IEntityTemplateModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
			if( modifiers != nullptr )
			{
				m_modificationCauses[ entityTemplate ].PushBack( entityTemplateNameHash );
				TDynArray<IEntityTemplateModifier*>::const_iterator end = modifiers->End();
				for( TDynArray<IEntityTemplateModifier*>::iterator iter = modifiers->Begin(); iter != end; ++iter )
				{
					(*iter)->EntityTemplateOnPostLoad( entityTemplate );
				}

				entityTemplateModified = true;
			}
		}
		else
		{
			WARN_ENGINE( TXT("Entity template have empty depot path.") );
		}
	}  

	return entityTemplateModified;
}

void CEntityTemplateModifierManager::RemoveModification( CEntityTemplate* entityTemplate )
{
	PC_SCOPE( EntityTemplateModifierManager );

	// entity templates are only processed when modifier manager is active
	RED_FATAL_ASSERT( m_active, "" );

	// modifier manager is active so all pending entity templates must already have been processed
	RED_FATAL_ASSERT( !m_pendingEntityTemplates.Exist( entityTemplate ), "" );
	
	if( m_modificationCauses.KeyExist( entityTemplate ) )
	{
		for( const Uint32& entityTemplateNameHash : m_modificationCauses[ entityTemplate ] )
		{
			if( entityTemplateNameHash != 0 )
			{
				const TDynArray< IEntityTemplateModifier* >* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
				if( modifiers != nullptr )
				{
					for( auto iter = modifiers->Begin(), end = modifiers->End(); iter != end; ++iter )
					{
						( *iter )->EntityTemplateOnFinalize( entityTemplate );
					}
				}
			}
			else
			{
				WARN_ENGINE( TXT("Entity template have empty depot path.") );
			}
		}
		m_modificationCauses.Erase( entityTemplate );
	}
}

const CName& CEntityTemplateModifierManager::EntityTemplateOnGetAppearance( const CEntityTemplate* entityTemplate, const CName& appearanceName ) const
{
	PC_SCOPE( EntityTemplateModifierManager );

	if( m_modificationCauses.KeyExist( entityTemplate ) )
	{
		const TDynArray<Uint32>& entityTemplatesNameHashes = m_modificationCauses[ entityTemplate ];

		for( const Uint32& entityTemplateNameHash : entityTemplatesNameHashes )
		{
			if( entityTemplateNameHash != 0 )
			{
				const TDynArray<IEntityTemplateModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
				if( modifiers != nullptr )
				{
					TDynArray<IEntityTemplateModifier*>::const_iterator end = modifiers->End();
					for( TDynArray<IEntityTemplateModifier*>::const_iterator iter = modifiers->Begin(); iter != end; ++iter )
					{
						const CName& ret = (*iter)->EntityTemplateOnGetAppearance( entityTemplate, appearanceName );
						if( ret != CName::NONE )
						{
							return ret;
						}
					}
				}
			}
			else
			{
				WARN_ENGINE( TXT("Entity template have empty depot path.") );
			}
		}
	}

	return CName::NONE;
}
