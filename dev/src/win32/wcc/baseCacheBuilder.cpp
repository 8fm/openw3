/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/class.h"
#include "../../common/core/rttiSystem.h"
#include "../../common/core/depot.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyLinkerFactory.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/garbageCollector.h"
#include "cookDataBase.h"
#include "cookDataBaseHelper.h"
#include "cacheBuilderCommandlet.h"
#include "baseCacheBuilder.h"

IMPLEMENT_ENGINE_CLASS( ICacheBuilder );
IMPLEMENT_ENGINE_CLASS( IFileBasedCacheBuilder );
IMPLEMENT_ENGINE_CLASS( IResourceBasedCacheBuilder );

//----

ICacheBuilder::ICacheBuilder()
{
}

//----

IFileBasedCacheBuilder::IFileBasedCacheBuilder()
	: m_ignoreErrors( false )
{
}

Bool IFileBasedCacheBuilder::Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions )
{
	// ignore errors
	if ( additonalOptions.HasOption( TXT("noerrors") ) )
	{
		LOG_WCC( TXT("File processing errors will be ignored!") );
		m_ignoreErrors = true;
	}

	return true;
}

Bool IFileBasedCacheBuilder::Process( const CCookerDataBase& db, const TDynArray< CCookerDataBase::TCookerDataBaseID >& filesToProcess )
{
	// Process objects
	for ( Uint32 fileIndex = 0; fileIndex < filesToProcess.Size(); ++fileIndex )
	{
		// query file entry
		CCookerResourceEntry fileEntry;
		CCookerDataBase::TCookerDataBaseID dbId = filesToProcess[ fileIndex ];
		if ( !db.GetFileEntry( dbId, fileEntry ) )
			continue; // should not happen

		// update status (will show up as the console title)
		const String depotPath = ANSI_TO_UNICODE( fileEntry.GetFilePath().AsChar() );
		LOG_WCC( TXT("Processing '%ls'..."), depotPath.AsChar() );
		LOG_WCC( TXT("Status: [%d/%d] Processing '%ls'..."), 
			fileIndex, filesToProcess.Size(), depotPath.AsChar() );

		// process file
		if ( !ProcessFile( db, dbId, depotPath ) )
		{
			ERR_WCC( TXT("Failed to process '%ls' in cache '%ls'"), depotPath.AsChar(), GetName() );

			if ( !m_ignoreErrors )
				return false;
		}
	}

	// files processed
	return true;
}

//----

IResourceBasedCacheBuilder::IResourceBasedCacheBuilder()
	: m_gcFrequency( 100 )
	, m_gcCount( 0 )
{
}

Bool IResourceBasedCacheBuilder::Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions )
{
	if ( !IFileBasedCacheBuilder::Initialize( outputFilePath, platform, additonalOptions ) )
		return false;

	// frequency (number of files) of calling the memory cleanup functions
	String value;
	if ( additonalOptions.GetSingleOptionValue( TXT("gcfrequency"), value ) )
	{
		if ( FromString< Uint32 >( value, m_gcFrequency ) )
		{
			LOG_WCC( TXT("Automatic GC frequency changed to %d"), m_gcFrequency );
		}
	}
		
	return true;
}

Bool IResourceBasedCacheBuilder::ProcessFile( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, const String& depotPath )
{
	// load the resource
	CResource* resource = LoadResource< CResource >( depotPath );
	if ( !resource )
	{
		// is the file there ?
		CDiskFile* file = GDepot->FindFileUseLinks( depotPath, 0 );
		if ( file )
		{
			ERR_WCC( TXT("!!! CRTICAL COOKING ERROR !!! Failed to deserialize '%ls'. File may be corrupted."), depotPath.AsChar() );
		}
		else
		{
			ERR_WCC( TXT("Missing file '%ls'"), depotPath.AsChar() );
		}

		return false;
	}

	resource->AddToRootSet();

	// process the resource
	const Bool result = ProcessResource( db, dbId, resource );

	resource->RemoveFromRootSet();
	resource->Discard();
	GObjectsDiscardList->ProcessList(true);

	// call the GC
	if ( m_gcFrequency && (m_gcCount++ >= m_gcFrequency) )
	{
		m_gcCount = 0;

		SGarbageCollector::GetInstance().CollectNow();
		GObjectsDiscardList->ProcessList(true);
	}

	// return the processing result
	return result;
}
