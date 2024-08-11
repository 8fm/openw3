/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "lootDefinitions.h"
#include "../../common/core/depot.h"
#include "../../common/core/xmlFileReader.h"

namespace
{
	const String LOOT_DEFINITIONS_DIR = TXT( "gameplay\\items\\" );
}

const String CLootDefinitions::NODE_ROOT				= TXT( "redxml" );
const String CLootDefinitions::NODE_DEFINITIONS			= TXT( "definitions" );
const String CLootDefinitions::NODE_LOOT_DEFINITIONS	= TXT( "loot_definitions" );

IMPLEMENT_ENGINE_CLASS( CLootDefinitionBase )

Bool CLootDefinitions::IsDefinitionNameUnique( const CName& name ) const
{
	return GetDefinition( name ) == NULL;
}

Bool CLootDefinitions::GetLootDefinitionsFilenames( TDynArray< String > & filenames ) const
{
	// Find directory with definition files
	CDirectory* dir = GDepot->FindPath( LOOT_DEFINITIONS_DIR.AsChar() );
	if ( !dir )
	{
		ERR_GAME( TXT("CLootDefinitions::GetLootDefinitionsFilenames() - loot definitions directiory '%ls' not found!"), LOOT_DEFINITIONS_DIR.AsChar() );
		return false;
	}

	// iterate definition files and load item/ability data
	const TFiles & files = dir->GetFiles();
	for ( TFiles::const_iterator it = files.Begin(); it!=files.End(); ++it )
	{
		String path = (*it)->GetDepotPath();
		if ( path.EndsWith( TXT( ".xml" ) ) && FileHasOnlyOneLootDefinitionNode( path ) )
		{
			filenames.PushBack( path );
		}
	}

	return filenames.Size() > 0;
}

Bool CLootDefinitions::FileHasOnlyOneLootDefinitionNode( const String& filename ) const
{
	CDiskFile *diskFile = GDepot->FindFile( filename );
	if ( diskFile == NULL )
	{
		return false;
	}

	IFile* file = diskFile->CreateReader();
	if ( file == NULL )
	{
		return false;
	}

	CXMLFileReader* xmlReader = new CXMLFileReader( *file );

	Bool isOk = false;
	if ( xmlReader->BeginNode( NODE_ROOT ) )
	{
		if ( xmlReader->BeginNode( NODE_DEFINITIONS, true ) )
		{
			// can have only one child and this needs to be a loot definitions node
			if ( xmlReader->GetChildCount() == 1 )
			{
				if ( xmlReader->BeginNode( NODE_LOOT_DEFINITIONS, true ) )
				{
					xmlReader->EndNode( false );
					isOk = true;
				}
			}
			xmlReader->EndNode( false );
		}
	}

	delete xmlReader;
	delete file;
	return isOk;
}