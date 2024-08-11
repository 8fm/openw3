#include "build.h"
#include "r4DLCEntityTemplateParamMounter.h"
#include "../../common/core/depot.h"

IMPLEMENT_ENGINE_CLASS( CR4EntityTemplateParamDLCMounter );

CR4EntityTemplateParamDLCMounter::CR4EntityTemplateParamDLCMounter() : m_mounterStarted( false )
{

}

CR4EntityTemplateParamDLCMounter::~CR4EntityTemplateParamDLCMounter()
{}

void CR4EntityTemplateParamDLCMounter::EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate )
{
	// Modify entity template. Don't do this if entity template is already modified by this mounter.
	const Bool alreadyModified = m_entityTemplates.KeyExist( entityTemplate );
	if( !alreadyModified )
	{
		m_entityTemplates.Insert( entityTemplate, TDynArray<CEntityTemplateParam*>() );

		if( m_mounterStarted )
		{
			AddParameters( entityTemplate, m_entityTemplates[entityTemplate] );
		}
	}
}

void CR4EntityTemplateParamDLCMounter::EntityTemplateOnFinalize( CEntityTemplate* entityTemplate )
{
	TDynArray<CEntityTemplateParam*>* modifications = m_entityTemplates.FindPtr( entityTemplate );
	if( modifications )
	{
		if( m_mounterStarted )
		{
			RemoveParameters( entityTemplate, *modifications );
		}	
		m_entityTemplates.Erase( entityTemplate );
	}	
}

void CR4EntityTemplateParamDLCMounter::AddParameters( CEntityTemplate* entityTemplate, TDynArray<CEntityTemplateParam*>& entityTemplateParamInstances  )
{
	TEntityTemplateParamEntries::const_iterator endParam = m_entityTemplateParams.End();
	for( TEntityTemplateParamEntries::iterator iterParam = m_entityTemplateParams.Begin(); iterParam != endParam; ++iterParam )
	{
		CEntityTemplateParam *entityTemplateParam = *iterParam;

		CEntityTemplateParam *entityTemplateParamNewInstance = Cast< CEntityTemplateParam >( entityTemplateParam->Clone( entityTemplate ) );

		entityTemplateParamInstances.PushBackUnique( entityTemplateParamNewInstance );

		entityTemplate->AddParameter( entityTemplateParamNewInstance );
	}
}

void CR4EntityTemplateParamDLCMounter::RemoveParameters( CEntityTemplate* entityTemplate, TDynArray<CEntityTemplateParam*>& entityTemplateParamInstances )
{
	TEntityTemplateParamEntries::const_iterator endParam = entityTemplateParamInstances.End();
	for( TEntityTemplateParamEntries::iterator iterParam = entityTemplateParamInstances.Begin(); iterParam != endParam; ++iterParam )
	{
		CEntityTemplateParam *entityTemplateParam = *iterParam;
		entityTemplate->RemoveParameter( entityTemplateParam );
	}
	entityTemplateParamInstances.Clear();
}

bool CR4EntityTemplateParamDLCMounter::OnCheckContentUsage()
{
	//! nothing
	return false;
}

void CR4EntityTemplateParamDLCMounter::OnGameStarting()
{
	Activate();
}

void CR4EntityTemplateParamDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4EntityTemplateParamDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4EntityTemplateParamDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4EntityTemplateParamDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{		
	//! nothing
	return true;
}

Bool CR4EntityTemplateParamDLCMounter::IsIncludeEntityTemplate( const CEntityTemplate* entityTemplate, const Char* baseEntityTemplateDepotPath ) const
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

void CR4EntityTemplateParamDLCMounter::OnSerialize( class IFile& file )
{
	//! cooker have to search all entity templates which use entity templates declared in CR4EntityTemplateParamDLCMounter
	//! reason: entity templates in cook have are flatted 
	if( GIsCooker && file.IsWriter() && !file.IsGarbageCollector() )
	{
		TDynArray<String> entityTemplatePathsInDepot;
		TDynArray<String> entityTemplatePathsToAdd;
		//! search all entity templates in depot
		GFileManager->FindFiles( GFileManager->GetDataDirectory(), String( TXT("*.") )  + ResourceExtension< CEntityTemplate >(), entityTemplatePathsInDepot, true );

		for( Uint32 entityTemplatePathIndex = 0; entityTemplatePathIndex < m_entityTemplatePaths.Size(); ++entityTemplatePathIndex )
		{
			const Char* entityTemplatePathStr = m_entityTemplatePaths[entityTemplatePathIndex].AsChar();

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
		}
		m_entityTemplatePaths.PushBackUnique( entityTemplatePathsToAdd );		
	}	

	TBaseClass::OnSerialize( file );

	//! Used by cooker for collect strings keyIds
}

#endif // !NO_EDITOR

void CR4EntityTemplateParamDLCMounter::Activate()
{
	// assert that modifier manager is not yet be started
	RED_FATAL_ASSERT( !SEntityTemplateModifierManager::GetInstance().IsActive(), "" );

	// we're just about to activate this mounter so there surely are no entity templates already modified
	RED_FATAL_ASSERT( m_entityTemplates.Empty(), "" );

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().RegisteModifier( entityTemplatePath, this );
	}

	m_mounterStarted = true;
}

void CR4EntityTemplateParamDLCMounter::Deactivate()
{
	// modifier manager must already be deactivated
	RED_FATAL_ASSERT( !SEntityTemplateModifierManager::GetInstance().IsActive(), "" );

	// must be empty as all modifications should already be removed during modifier manager deactivation
	RED_FATAL_ASSERT( m_entityTemplates.Empty(), "" );

	m_mounterStarted = false;

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().UnregisteModifier( entityTemplatePath, this );
	}
}
