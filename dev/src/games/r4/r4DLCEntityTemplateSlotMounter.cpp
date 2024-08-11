#include "build.h"
#include "r4DLCEntityTemplateSlotMounter.h"
#include "../../common/core/depot.h"

IMPLEMENT_ENGINE_CLASS( CR4EntityTemplateSlotDLCMounter );

CR4EntityTemplateSlotDLCMounter::CR4EntityTemplateSlotDLCMounter() : m_mounterStarted( false )
{

}

CR4EntityTemplateSlotDLCMounter::~CR4EntityTemplateSlotDLCMounter()
{}

void CR4EntityTemplateSlotDLCMounter::EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate )
{
	// Modify entity template. Don't do this if entity template is already modified by this mounter.
	const Bool alreadyModified = m_entityTemplates.KeyExist( entityTemplate );
	if( !alreadyModified )
	{
		m_entityTemplates.Set( entityTemplate, TDynArray<EntitySlot*>() );

		if( m_mounterStarted )
		{
			AddSlots( entityTemplate, m_entityTemplates[entityTemplate] );
		}
	}
}

void CR4EntityTemplateSlotDLCMounter::EntityTemplateOnFinalize( CEntityTemplate* entityTemplate )
{
	TDynArray< EntitySlot* >* modifications = m_entityTemplates.FindPtr( entityTemplate );
	if( modifications )
	{
		if( m_mounterStarted )
		{
			RemoveSlots( entityTemplate, *modifications );
		}	
		m_entityTemplates.Erase( entityTemplate );
	}	
}

void CR4EntityTemplateSlotDLCMounter::AddSlots( CEntityTemplate* entityTemplate, TDynArray<EntitySlot*>& entityTemplateSlotInstances )
{
	TEntityTemplateSlotEntries::const_iterator endSlot = m_entityTemplateSlots.End();
	for( TEntityTemplateSlotEntries::iterator iterSlot = m_entityTemplateSlots.Begin(); iterSlot != endSlot; ++iterSlot )
	{
		EntitySlot *entityTemplateSlot = *iterSlot;

		entityTemplateSlotInstances.PushBackUnique( entityTemplateSlot );

		entityTemplate->AddSlot( entityTemplateSlot->GetName(), *entityTemplateSlot, false );
	}
}

void CR4EntityTemplateSlotDLCMounter::RemoveSlots( CEntityTemplate* entityTemplate, TDynArray<EntitySlot*>& entityTemplateSlotInstances )
{
	TEntityTemplateSlotEntries::const_iterator endSlot = entityTemplateSlotInstances.End();
	for( TEntityTemplateSlotEntries::iterator iterSlot = entityTemplateSlotInstances.Begin(); iterSlot != endSlot; ++iterSlot )
	{
		EntitySlot *entityTemplateSlot = *iterSlot;
		entityTemplate->RemoveSlot( entityTemplateSlot->GetName(), false );
	}
	entityTemplateSlotInstances.Clear();
}

bool CR4EntityTemplateSlotDLCMounter::OnCheckContentUsage()
{
	//! nothing
	return false;
}

void CR4EntityTemplateSlotDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4EntityTemplateSlotDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4EntityTemplateSlotDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4EntityTemplateSlotDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4EntityTemplateSlotDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{		
	//! nothing
	return true;
}

Bool CR4EntityTemplateSlotDLCMounter::IsIncludeEntityTemplate( const CEntityTemplate* entityTemplate, const Char* baseEntityTemplateDepotPath ) const
{
	if( entityTemplate )
	{		
		const TDynArray< THandle< CEntityTemplate > >& includedEntityTemplates = entityTemplate->GetIncludes();
		for ( auto& includedEntityTemplate : includedEntityTemplates )
		{
			if( includedEntityTemplate )
			{
				String includedEntityTemplatePath = includedEntityTemplate->GetDepotPath();
				if( Red::System::StringCompareNoCase( baseEntityTemplateDepotPath, includedEntityTemplatePath.AsChar() ) == 0 )
				{
					return true;
				}
				else
				{
					if( IsIncludeEntityTemplate( includedEntityTemplate, baseEntityTemplateDepotPath )  )
					{
						return true;
					}
				}
			}								
		}
	}
	return false;
}

void CR4EntityTemplateSlotDLCMounter::OnSerialize( class IFile& file )
{
	//! cooker have to search all entity templates which use entity templates declared in CR4EntityTemplateParamDLCMounter
	//! reason: entity templates in cook have are flatted 
	if( GIsCooker && file.IsWriter() && !file.IsGarbageCollector() )
	{
		TDynArray<String> entityTemplatePathsInDepot;
		TDynArray<String> entityTemplatePathsToAdd;
		//! search all entity templates in depot
		GFileManager->FindFiles( GFileManager->GetDataDirectory(), String( TXT("*.") )  + ResourceExtension< CEntityTemplate >(), entityTemplatePathsInDepot, true );

		const Char* entityTemplatePathStr = m_baseEntityTemplatePath.AsChar();

		//! search entity templates with included entity template from m_entityTemplatePaths
		for( auto& entityTemplatePathInDepot : entityTemplatePathsInDepot )
		{
			String entityTemplateLocalPathInDepot;
			GDepot->ConvertToLocalPath( entityTemplatePathInDepot, entityTemplateLocalPathInDepot );
			if( Red::System::StringCompareNoCase( entityTemplatePathStr, entityTemplateLocalPathInDepot.AsChar() ) != 0 )
			{
				CDiskFile* entityTemplateFileInDepot = GDepot->FindFile( entityTemplateLocalPathInDepot );
				if( entityTemplateFileInDepot )
				{
					CEntityTemplate* entityTemplateInDepot = Cast<CEntityTemplate>( entityTemplateFileInDepot->Load() );
					if( IsIncludeEntityTemplate( entityTemplateInDepot, entityTemplatePathStr ) )
					{
						entityTemplatePathsToAdd.PushBackUnique( entityTemplateLocalPathInDepot );							
					}
				}
			}			
		}
		m_entityTemplatePaths.PushBackUnique( entityTemplatePathsToAdd );		
	}	

	TBaseClass::OnSerialize( file );

	//! Used by cooker for collect strings keyIds
}

#endif // !NO_EDITOR

void CR4EntityTemplateSlotDLCMounter::Activate()
{
	// assert that modifier manager is not yet be started
	RED_FATAL_ASSERT( !SEntityTemplateModifierManager::GetInstance().IsActive(), "" );

	// we're just about to activate this mounter so there surely are no entity templates already modified
	RED_FATAL_ASSERT( m_entityTemplates.Empty(), "" );

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().RegisteModifier( entityTemplatePath, this );
	}

	if( !m_baseEntityTemplatePath.Empty() )
	{
		SEntityTemplateModifierManager::GetInstance().RegisteModifier( m_baseEntityTemplatePath, this );
	}

	m_mounterStarted = true;
}

void CR4EntityTemplateSlotDLCMounter::Deactivate()
{
	// assert that modifier manager must already be deactivated
	RED_FATAL_ASSERT( !SEntityTemplateModifierManager::GetInstance().IsActive(), "" );

	// must be empty as all modifications should already be removed during modifier manager deactivation
	RED_FATAL_ASSERT( m_entityTemplates.Empty(), "" );

	m_mounterStarted = false;

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().UnregisteModifier( entityTemplatePath, this );
	}

	if( !m_baseEntityTemplatePath.Empty() )
	{
		SEntityTemplateModifierManager::GetInstance().UnregisteModifier( m_baseEntityTemplatePath, this );
	}
}
