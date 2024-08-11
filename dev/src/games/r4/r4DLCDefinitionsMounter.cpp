#include "build.h"
#include "r4DLCDefinitionsMounter.h"
#include "../../common/core/depot.h"

IMPLEMENT_ENGINE_CLASS( CR4DefinitionsDLCMounter );

namespace
{
CDirectory* GetDirectory( const String& path )
{
	static String backslash = TXT( "\\" );
	const Char* filename = nullptr;
	CDirectory* dir = nullptr;
	if ( !path.EndsWith( backslash ) )
	{
		String fixedPath = path + backslash;
		dir = GDepot->FindPath( fixedPath.AsChar(), &filename );
	}
	else
	{
		dir = GDepot->FindPath( path.AsChar(), &filename );
	}
	// if there's a path but it's not a file
	if ( dir != nullptr && ( filename == nullptr || Red::System::StringLength( filename ) == 0 ) )
	{
		return dir;
	}
	return nullptr;
}
}

CR4DefinitionsDLCMounter::CR4DefinitionsDLCMounter()
	: m_xmlFileLoaded( false )
{
	m_creatorTag = Red::System::GUID::Create();
}

bool CR4DefinitionsDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4DefinitionsDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4DefinitionsDLCMounter::OnGameEnding()
{
	Deactivate();
}


void CR4DefinitionsDLCMounter::OnDefinitionsReloaded()
{
#ifndef NO_EDITOR
	m_xmlFileLoaded = false;
	LoadDefinitions();
#endif
}

void CR4DefinitionsDLCMounter::LoadDefinitions()
{
	if( GCommonGame && GCommonGame->GetDefinitionsManager() )
	{
		if( !m_definitionXmlFilePath.Empty() )
		{
			if ( GDepot->FileExist( m_definitionXmlFilePath ) )
			{
				GCommonGame->GetDefinitionsManager()->LoadDefinitions( m_definitionXmlFilePath, m_creatorTag );
				m_xmlFileLoaded = true;
			}
			else if ( CDirectory* dir = GetDirectory( m_definitionXmlFilePath) )
			{
				TFiles files = dir->GetFiles();
				for ( const auto& file : files )
				{
					GCommonGame->GetDefinitionsManager()->LoadDefinitions( file->GetDepotPath(), m_creatorTag );
					m_xmlFileLoaded = true;
				}
			}
		}
	}
}

void CR4DefinitionsDLCMounter::UnloadDefinitions()
{
	if( GCommonGame && GCommonGame->GetDefinitionsManager() )
	{
		if( m_xmlFileLoaded )
		{
			GCommonGame->GetDefinitionsManager()->RemoveDefinitions( m_creatorTag );
			m_xmlFileLoaded = false;
		}
	}
}

#ifndef NO_EDITOR

void CR4DefinitionsDLCMounter::OnEditorStarted()
{
	if( GCommonGame && GCommonGame->GetDefinitionsManager() )
	{
		GCommonGame->GetDefinitionsManager()->AddListener( this );
	}
	
	Activate();
}

void CR4DefinitionsDLCMounter::OnEditorStopped()
{
	Deactivate();

	if( GCommonGame && GCommonGame->GetDefinitionsManager() )
	{
		GCommonGame->GetDefinitionsManager()->RemoveListener( this );
	}
}

Bool CR4DefinitionsDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	if( !m_definitionXmlFilePath.Empty() )
	{
		if( GDepot->FileExist( m_definitionXmlFilePath ) )
		{
			const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( m_definitionXmlFilePath.AsChar() ) );
			outputList.AddFile( actualDepotPathAnsi );						
		}
		else if ( CDirectory* dir = GetDirectory( m_definitionXmlFilePath ) )
		{
			TFiles files = dir->GetFiles();
			for ( const auto& file : files )
			{
				const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( file->GetDepotPath().AsChar() ) );
				outputList.AddFile( actualDepotPathAnsi );
			}
		}
	}
	return true;
}

void CR4DefinitionsDLCMounter::OnSerialize( class IFile& file )
{
	TBaseClass::OnSerialize( file );
	
	const CName readableItemTag( TXT("ReadableItem") );

	//! Used by cooker for collect strings keyIds
	if ( file.IsWriter() && file.IsCooker() )
	{
		CSpeechCollector* speechCollector = file.QuerySpeechCollector();
		if( speechCollector )
		{
			if( !m_definitionXmlFilePath.Empty() )
			{		
				CDefinitionsManager definitionManager;

				//! Remove all default item definitions
				definitionManager.RemoveDefinitions( definitionManager.GetDefaultCreatorTag() );

				//! Load definitions only for DLC
				if ( GDepot->FileExist( m_definitionXmlFilePath ) )
				{
					definitionManager.LoadDefinitions( m_definitionXmlFilePath, Red::System::GUID::Create() );
				}
				else if ( CDirectory* dir = GetDirectory( m_definitionXmlFilePath ) )
				{
					CGUID tmpGuid = Red::System::GUID::Create();
					TFiles files = dir->GetFiles();
					for ( const auto& file : files )
					{
						definitionManager.LoadDefinitions( file->GetDepotPath(), tmpGuid );
					}
				}

				TDynArray< CName > items;

				definitionManager.GetItemList( items );

				for( CName& itemName : items )
				{
					const SItemDefinition* itemDefinition = definitionManager.GetItemDefinition( itemName );
					const String& localizationKeyName = itemDefinition->GetLocalizationKeyName();
					speechCollector->ReportStringKey( localizationKeyName );
					speechCollector->ReportStringKey( itemDefinition->GetLocalizationKeyDesc() );
					if( itemDefinition->GetItemTags().Exist( readableItemTag ) && (localizationKeyName.Empty() == false) ) //! has "ReadableItem" tag
					{
						speechCollector->ReportStringKey( localizationKeyName + TXT("_text") );
					}

				}		
			}
		}
	}
}

#endif // !NO_EDITOR

Bool CR4DefinitionsDLCMounter::ShouldLoad()
{
	return GCommonGame->IsNewGamePlus() == false;
}

void CR4DefinitionsDLCMounter::Activate()
{
	if( ShouldLoad() )
		LoadDefinitions();
}

void CR4DefinitionsDLCMounter::Deactivate()
{
	if( ShouldLoad() )
		UnloadDefinitions();
}
