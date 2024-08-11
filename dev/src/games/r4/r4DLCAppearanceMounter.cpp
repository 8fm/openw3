#include "build.h"
#include "r4DLCAppearanceMounter.h"
#include "../../common/core/depot.h"
#include "../../common/engine/entityExternalAppearance.h"

IMPLEMENT_ENGINE_CLASS( CR4EntityExternalAppearanceDLC );
IMPLEMENT_ENGINE_CLASS( CR4EntityExternalAppearanceDLCMounter );

CR4EntityExternalAppearanceDLCMounter::CR4EntityExternalAppearanceDLCMounter() : m_mounterStarted( false )
{

}

CR4EntityExternalAppearanceDLCMounter::~CR4EntityExternalAppearanceDLCMounter()
{}

void CR4EntityExternalAppearanceDLCMounter::EntityTemplateOnPostLoad( CEntityTemplate* entityTemplate )
{
	// Modify entity template. Don't do this if entity template is already modified by this mounter.
	const Bool alreadyModified = m_entityTemplates.KeyExist( entityTemplate );
	if( !alreadyModified )
	{
		m_entityTemplates.Insert( entityTemplate, TDynArray<CR4EntityExternalAppearanceDLC*>() );

		if( m_mounterStarted )
		{
			AddAppearances( entityTemplate, m_entityTemplates[entityTemplate] );
		}
	}
}
const CName& CR4EntityExternalAppearanceDLCMounter::EntityTemplateOnGetAppearance( const CEntityTemplate* entityTemplate, const CName& appearanceName ) const
{
	const TDynArray< CR4EntityExternalAppearanceDLC* >* modifications = m_entityTemplates.FindPtr( (CEntityTemplate*)entityTemplate );
	if( modifications )
	{
		if( m_mounterStarted )
		{
			for( auto& enityExternalAppearance : *modifications )
			{
				if( enityExternalAppearance->m_appearanceToRepleace == appearanceName )
				{
					if( enityExternalAppearance->m_entityExternalAppearance )
					{
						return enityExternalAppearance->m_entityExternalAppearance->GetName();
					}
				}
			}
		}
	}
	return CName::NONE;
}

void CR4EntityExternalAppearanceDLCMounter::EntityTemplateOnFinalize( CEntityTemplate* entityTemplate )
{
	TDynArray<CR4EntityExternalAppearanceDLC*>* entityTemplateFound = m_entityTemplates.FindPtr( entityTemplate );
	if( entityTemplateFound )
	{
		if( m_mounterStarted )
		{
			RemoveAppearances( entityTemplate, *entityTemplateFound );
		}	
		m_entityTemplates.Erase( entityTemplate );
	}	
}


void CR4EntityExternalAppearanceDLCMounter::OnAppearanceChange( CAppearanceComponent* appearanceComponent )
{	
	if( m_mounterStarted )
	{
		RemoveAppearanceAttachments( appearanceComponent );
		AddAppearanceAttachments( appearanceComponent );
	}
}

void CR4EntityExternalAppearanceDLCMounter::AppearanceComponentOnFinalize( CAppearanceComponent* appearanceComponent )
{
	RemoveAppearanceAttachments( appearanceComponent );
}

bool CR4EntityExternalAppearanceDLCMounter::OnCheckContentUsage()
{
	//! nothing
	return false;
}

void CR4EntityExternalAppearanceDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4EntityExternalAppearanceDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4EntityExternalAppearanceDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4EntityExternalAppearanceDLCMounter::OnEditorStopped()
{
	Deactivate();
}

#endif // !NO_EDITOR

void CR4EntityExternalAppearanceDLCMounter::AddAppearances( CEntityTemplate* entityTemplate, TDynArray<CR4EntityExternalAppearanceDLC*>& entityExternalAppearances )
{
	TEntityExternalAppearances::const_iterator endAppearance = m_entityExternalAppearances.End();
	for( TEntityExternalAppearances::iterator iterAppearance = m_entityExternalAppearances.Begin(); iterAppearance != endAppearance; ++iterAppearance )
	{		
		entityExternalAppearances.PushBackUnique( (*iterAppearance) );
		if( (*iterAppearance)->m_entityExternalAppearance )
			entityTemplate->ImportAppearance( (*iterAppearance)->m_entityExternalAppearance->GetName(), (*iterAppearance)->m_entityExternalAppearance->GetAppearance() );
	}
}

void CR4EntityExternalAppearanceDLCMounter::RemoveAppearances( CEntityTemplate* entityTemplate, TDynArray<CR4EntityExternalAppearanceDLC*>& entityExternalAppearances )
{
	TDynArray<CR4EntityExternalAppearanceDLC*>::const_iterator endAppearance = entityExternalAppearances.End();
	for( TDynArray<CR4EntityExternalAppearanceDLC*>::iterator iterAppearance = entityExternalAppearances.Begin(); iterAppearance != endAppearance; ++iterAppearance )
	{
		if( (*iterAppearance)->m_entityExternalAppearance )
			entityTemplate->RemoveAppearance( (*iterAppearance)->m_entityExternalAppearance->GetAppearance() );
	}
	entityExternalAppearances.Clear();
}

void CR4EntityExternalAppearanceDLCMounter::AddAppearanceAttachments( CAppearanceComponent* appearanceComponent )
{
	const CName& currentAppearanceName = appearanceComponent->GetAppearance();
	TEntityExternalAppearances::const_iterator endAppearance = m_entityExternalAppearances.End();
	for( TEntityExternalAppearances::iterator iterAppearance = m_entityExternalAppearances.Begin(); iterAppearance != endAppearance; ++iterAppearance )
	{		
		if( (*iterAppearance)->m_entityExternalAppearance )
		{
			if( (*iterAppearance)->m_entityExternalAppearance->GetName() == currentAppearanceName )
			{
				(*iterAppearance)->m_entityExternalAppearance->PrintLogInfo();
				appearanceComponent->UpdateCurrentAppearanceAttachments();
				m_appearanceComponents.Set( appearanceComponent, (*iterAppearance) );
			}
		}
	}
}

void CR4EntityExternalAppearanceDLCMounter::RemoveAppearanceAttachments( CAppearanceComponent* appearanceComponent )
{
	CR4EntityExternalAppearanceDLC** entityExternalAppearance = m_appearanceComponents.FindPtr( appearanceComponent );
	if ( entityExternalAppearance != nullptr )
	{
		if( (*entityExternalAppearance)->m_entityExternalAppearance )
			appearanceComponent->RemoveAppearanceAttachments( (*entityExternalAppearance)->m_entityExternalAppearance->GetName() );
		m_appearanceComponents.Erase( appearanceComponent );
	}
}

#ifndef NO_EDITOR
static Bool IsIncludeEntityTemplate( const CEntityTemplate* entityTemplate, const Char* baseEntityTemplateDepotPath )
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

void CR4EntityExternalAppearanceDLCMounter::OnSerialize( class IFile& file )
{
	//! cooker have to search all entity templates which use entity templates declared in CR4EntityExternalAppearanceDLCMounter
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

bool CR4EntityExternalAppearanceDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{		
	//! nothing
	return true;
}

#endif // !NO_EDITOR

void CR4EntityExternalAppearanceDLCMounter::Activate()
{
	// assert that modifier manager is not yet be started
	RED_FATAL_ASSERT( !SEntityTemplateModifierManager::GetInstance().IsActive(), "" );

	// we're just about to activate this mounter so there surely are no entity templates already modified
	RED_FATAL_ASSERT( m_entityTemplates.Empty(), "" );

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().RegisteModifier( entityTemplatePath, this );
		SAppearanceComponentModifierManager::GetInstance().RegisteModifier( entityTemplatePath, this );
	}

	m_mounterStarted = true;
}

void CR4EntityExternalAppearanceDLCMounter::Deactivate()
{
	m_mounterStarted = false;

	TAppearanceComponentEntries::iterator appearanceComponentEnd = m_appearanceComponents.End();
	for( TAppearanceComponentEntries::iterator iterAppearanceComponent = m_appearanceComponents.Begin(); iterAppearanceComponent != appearanceComponentEnd; ++iterAppearanceComponent )
	{
		CAppearanceComponent* appearanceComponent = iterAppearanceComponent->m_first;
		if( iterAppearanceComponent->m_second->m_entityExternalAppearance )
			appearanceComponent->RemoveAppearanceAttachments( iterAppearanceComponent->m_second->m_entityExternalAppearance->GetName() );
	}
	m_appearanceComponents.Clear();

	for( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SEntityTemplateModifierManager::GetInstance().UnregisteModifier( entityTemplatePath, this );
		SAppearanceComponentModifierManager::GetInstance().UnregisteModifier( entityTemplatePath, this );
	}
}
