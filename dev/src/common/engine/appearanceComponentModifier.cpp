#include "build.h"
#include "appearanceComponentModifier.h"
#include "appearanceComponent.h"

Bool CAppearanceComponentModifierManager::RegisteModifier( const String& entityTemplateName, IAppearanceComponentModifierModifier* appearanceComponentModifier )
{
	Uint32 entityTemplateNameHash = entityTemplateName.CalcHash();
	TDynArray<IAppearanceComponentModifierModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
	if( modifiers == nullptr )
	{
		TDynArray<IAppearanceComponentModifierModifier*> newModifiers;
		newModifiers.PushBack( appearanceComponentModifier );
		m_modifiers.Set( entityTemplateNameHash, newModifiers );
	}
	else
	{
		if( modifiers->FindPtr( appearanceComponentModifier ) )
		{
			WARN_ENGINE( TXT("Appearance component modifier already added to %ls"), entityTemplateName.AsChar() );
			return false;
		}
		modifiers->PushBack( appearanceComponentModifier );
	}
	return true;
}

Bool CAppearanceComponentModifierManager::UnregisteModifier( const String& entityTemplateName, IAppearanceComponentModifierModifier* appearanceComponentModifier )
{
	Uint32 entityTemplateNameHash = entityTemplateName.CalcHash();
	TDynArray<IAppearanceComponentModifierModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
	if( modifiers != nullptr )
	{
		IAppearanceComponentModifierModifier** foundAppearanceComponentModifie = modifiers->FindPtr( appearanceComponentModifier );
		if( foundAppearanceComponentModifie )
		{
			modifiers->Erase( foundAppearanceComponentModifie );
			return true;
		}
	}

	WARN_ENGINE( TXT("Appearance component  modifier not registered for %ls"), entityTemplateName.AsChar() );
	return false;
}

Bool CAppearanceComponentModifierManager::IsModifierRegistered( const String& entityTemplatePath ) const
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

void CAppearanceComponentModifierManager::OnAppearanceChange( CAppearanceComponent* appearanceComponent )
{
	PC_SCOPE( AppearanceComponentModifierManager );
	CEntity* entity = appearanceComponent->GetEntity();
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
					TDynArray<IAppearanceComponentModifierModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
					if( modifiers != nullptr )
					{
						TDynArray<IAppearanceComponentModifierModifier*>::const_iterator end = modifiers->End();
						for( TDynArray<IAppearanceComponentModifierModifier*>::iterator iter = modifiers->Begin(); iter != end; ++iter )
						{
							(*iter)->OnAppearanceChange( appearanceComponent );
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

void CAppearanceComponentModifierManager::AppearanceComponentOnFinalize( CAppearanceComponent* appearanceComponent )
{
	PC_SCOPE( AppearanceComponentModifierManager );
	CEntity* entity = Cast< CEntity >( appearanceComponent->GetParent() );
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
					TDynArray<IAppearanceComponentModifierModifier*>* modifiers = m_modifiers.FindPtr( entityTemplateNameHash );
					if( modifiers != nullptr )
					{
						TDynArray<IAppearanceComponentModifierModifier*>::const_iterator end = modifiers->End();
						for( TDynArray<IAppearanceComponentModifierModifier*>::iterator iter = modifiers->Begin(); iter != end; ++iter )
						{
							(*iter)->AppearanceComponentOnFinalize( appearanceComponent );
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
