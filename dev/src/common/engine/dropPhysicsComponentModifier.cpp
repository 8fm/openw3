#include "build.h"
#include "dropPhysicsComponentModifier.h"
#include "dropPhysicsComponent.h"

Bool CDropPhysicsComponentModifierManager::RegisterModifier( const String& entityTemplateName, IDropPhysicsComponentModifier* dropPhysicsComponentModifierModifier )
{
	Uint32 entityTemplateNameHash = entityTemplateName.CalcHash();
	TDynArray<IDropPhysicsComponentModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
	if( modifiers == nullptr )
	{
		TDynArray<IDropPhysicsComponentModifier*> newModifiers;
		newModifiers.PushBack( dropPhysicsComponentModifierModifier );
		m_modifiers.Set( entityTemplateNameHash, newModifiers );
	}
	else
	{
		if( modifiers->FindPtr( dropPhysicsComponentModifierModifier ) )
		{
			WARN_ENGINE( TXT("Drop physics component modifier already added to %ls"), entityTemplateName.AsChar() );
			return false;
		}
		modifiers->PushBack( dropPhysicsComponentModifierModifier );
	}
	return true;
}

Bool CDropPhysicsComponentModifierManager::UnregisterModifier( const String& entityTemplateName, IDropPhysicsComponentModifier* dropPhysicsComponentModifierModifier )
{
	Uint32 entityTemplateNameHash = entityTemplateName.CalcHash();
	TDynArray<IDropPhysicsComponentModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
	if( modifiers != nullptr )
	{
		IDropPhysicsComponentModifier** foundDropPhysicsComponentModifier = modifiers->FindPtr( dropPhysicsComponentModifierModifier );
		if( foundDropPhysicsComponentModifier )
		{
			modifiers->Erase( foundDropPhysicsComponentModifier );
			return true;
		}
	}

	WARN_ENGINE( TXT("Drop physics component modifier not registered for %ls"), entityTemplateName.AsChar() );
	return false;
}

Bool CDropPhysicsComponentModifierManager::IsModifierRegistered( const String& entityTemplatePath ) const
{
	return m_modifiers.KeyExist( entityTemplatePath.CalcHash() );
}

static void GetEntityTemplateIncludesNames( const CEntityTemplate* entityTemplate, TDynArray<Uint32>& entityTemplatesNameHashes )
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

void CDropPhysicsComponentModifierManager::DropPhysicsComponentOnInitialized( CDropPhysicsComponent* dropPhysicsComponent )
{
	PC_SCOPE( CDropPhysicsComponentModifierManager );
	CEntity* entity = dropPhysicsComponent->GetEntity();
	if( entity )
	{
		CEntityTemplate*  entityTemplate = entity->GetEntityTemplate();
		if( entityTemplate )
		{
			TDynArray<Uint32> entityTemplatesNameHashes;
			entityTemplatesNameHashes.PushBack( entityTemplate->GetDepotPath().CalcHash() );

			GetEntityTemplateIncludesNames( entityTemplate,  entityTemplatesNameHashes );

			for( const Uint32& entityTemplateNameHash : entityTemplatesNameHashes )
			{
				if( entityTemplateNameHash != 0 )
				{
					TDynArray<IDropPhysicsComponentModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
					if( modifiers != nullptr )
					{
						TDynArray<IDropPhysicsComponentModifier*>::const_iterator end = modifiers->End();
						for( TDynArray<IDropPhysicsComponentModifier*>::iterator iter = modifiers->Begin(); iter != end; ++iter )
						{
							(*iter)->DropPhysicsComponentOnInitialized( dropPhysicsComponent );
						}
					}
				}
				else
				{
					WARN_ENGINE( TXT("Entity template have empty depot path.") );
				}
			}
		}
	}
}

void CDropPhysicsComponentModifierManager::DropPhysicsComponentOnFinalize( CDropPhysicsComponent* dropPhysicsComponent )
{
	PC_SCOPE( CDropPhysicsComponentModifierManager );
	CEntity* entity = dropPhysicsComponent->GetEntity();
	if( entity )
	{
		CEntityTemplate*  entityTemplate = entity->GetEntityTemplate();
		if( entityTemplate )
		{
			TDynArray<Uint32> entityTemplatesNameHashes;
			entityTemplatesNameHashes.PushBack( entityTemplate->GetDepotPath().CalcHash() );

			GetEntityTemplateIncludesNames( entityTemplate,  entityTemplatesNameHashes );

			for( const Uint32& entityTemplateNameHash : entityTemplatesNameHashes )
			{
				if( entityTemplateNameHash != 0 )
				{
					TDynArray<IDropPhysicsComponentModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
					if( modifiers != nullptr )
					{
						TDynArray<IDropPhysicsComponentModifier*>::const_iterator end = modifiers->End();
						for( TDynArray<IDropPhysicsComponentModifier*>::iterator iter = modifiers->Begin(); iter != end; ++iter )
						{
							(*iter)->DropPhysicsComponentOnFinalize( dropPhysicsComponent );
						}
					}
				}
				else
				{
					WARN_ENGINE( TXT("Entity template have empty depot path.") );
				}
			}
		} 
	}
}
