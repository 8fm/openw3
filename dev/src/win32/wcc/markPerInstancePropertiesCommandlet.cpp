/**
* Copyright (c) 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/depot.h"
#include "../../common/core/commandlet.h"

#include "../../common/engine/entity.h"
#include "../../common/engine/dimmerComponent.h"

#include "wccVersionControl.h"

/// Sets useForShadowMesh flag for the last LOD in selected meshes
class CMarkPerInstancePropertiesCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CMarkPerInstancePropertiesCommandlet, ICommandlet, 0 );

public:
	CMarkPerInstancePropertiesCommandlet()
	{
		m_commandletName = CName( TXT("markPerInstanceProperties") );
	}

	~CMarkPerInstancePropertiesCommandlet()
	{
	}

	// ICommandlet interface
	virtual const Char* GetOneLiner() const 
	{ 
		return TXT("Mark all properties in light entities as instance properties"); 
	}

	virtual void PrintHelp() const
	{
		LOG_WCC( TXT("Usage:") );
		LOG_WCC( TXT("  markPerInstanceProperties -dir=<path>") );
	}

	virtual bool Execute( const CommandletOptions& options )
	{
		String dirPath;
		if ( !options.GetSingleOptionValue( TXT("dir"), dirPath ) )
		{
			ERR_WCC( TXT("Missing required parameter 'dir=<path>'") );
			return false;
		}

		if ( !dirPath.EndsWith( TXT("\\") ) )
		{
			dirPath += TXT("\\");
		}

		SChangelist changelist = SChangelist::DEFAULT;
		if ( InitializeWCCVersionControl( options ) )
		{
			// Get changelist, if specified
			Uint32 clNumber = 0;
			String clString;
			if ( !options.GetSingleOptionValue( TXT( "cl" ), clString ) || !::FromString( clString, clNumber ) )
			{
				clNumber = 0;
			}
			if( !static_cast< CSourceControlP4* >( GVersionControl )->CreateChangelistWithNumber( clNumber, changelist ) )
			{
				changelist = SChangelist::DEFAULT;
			}
		}
		else
		{
			WARN_WCC( TXT("Failed to initialize P4 version control. Commandlet will not checkout modified files.") );
		}


		String depotPath;
		GDepot->GetAbsolutePath( depotPath );

		TDynArray< String > filePaths;
		GFileManager->FindFiles( depotPath + dirPath, TXT("*.w2ent"), filePaths, true );

		for ( String& path : filePaths )
		{
			String dirAbsolutePath = path.StringBefore( TXT("\\"), true ).ToLower() + TXT("\\");
			CDirectory* dir = GDepot->FindPath( dirAbsolutePath.RightString( dirAbsolutePath.Size() - depotPath.Size() ) );
			if ( !dir->IsPopulated() )
			{
				dir->ForceRepopulate();
			}
			CDiskFile* file = dir->FindLocalFile( path.StringAfter( TXT("\\"), true ) );
			THandle< CResource > resource = file->Load();
			if ( !file->LoadThumbnail() ) // this is required to not lose current thumbnail after saving changes
			{
				WARN_WCC( TXT("Couldn't load thumbnail for %ls (either it will be lost or it wasn't there in the first place)"), file->GetAbsolutePath().AsChar() );
			}

			CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( resource );
			CEntity* entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
			
			Bool hadChanges = false;

			if ( entity != nullptr )
			{
				for ( CComponent* component : entity->GetComponents() )
				{
					if ( component->IsA< CDimmerComponent >() )
					{
						CDimmerComponent* dimmerComponent = Cast< CDimmerComponent >( component );
						entityTemplate->AddInstancePropertyEntry( CName( dimmerComponent->GetName() ), CName( TXT("isAreaMarker") ) );
						hadChanges = true;
					}
				}
				entity->Discard();
			}
			else
			{
				ERR_WCC( TXT("Couldn't create instance of entity template: %s. Skipping this entity."), entityTemplate->GetFriendlyName() );
				continue;
			}

			if ( !hadChanges )
			{
				file->Unload();
				continue;
			}

			if ( GVersionControl->IsSourceControlDisabled() )
			{
				file->SetForcedOverwriteFlag( ESaveReadonlyFileBehaviour::eOverwrite );
			}
			else
			{
				file->SetChangelist( changelist );
				file->CheckOut();
			}

			if ( !entityTemplate->Save() )
			{
				ERR_WCC( TXT("Couldn't save changes to %ls"), path.AsChar() );
			}

			file->Unload();
		}

		return true;
	}
};


BEGIN_CLASS_RTTI( CMarkPerInstancePropertiesCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMarkPerInstancePropertiesCommandlet );