/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/class.h"
#include "../../common/core/commandlet.h"
#include "cookDataBase.h"
#include "cookDataBaseHelper.h"
#include "cacheSplitterCommandlet.h"
#include "baseCacheSplitter.h"
#include "playgoHelper.h"

IMPLEMENT_ENGINE_CLASS( CCacheSplitterCommandlet );
IMPLEMENT_ENGINE_CLASS( IBaseCacheSplitter );

//----

CCacheSplitterCommandlet::Settings::Settings()
{
	PlayGoHelper::CChunkResolver resolver;
	m_fallBackChunk = resolver.GetFallBackChunkName();
}

Bool CCacheSplitterCommandlet::Settings::Parse( const CommandletOptions& options )
{
	// output path
	if ( !options.GetSingleOptionValue( TXT("outdir"), m_rootOutputDirectory ) )
	{
		ERR_WCC( TXT("Expecting output file path") );
		return false;
	}

	// split list path
	if ( !options.GetSingleOptionValue( TXT("split"), m_splitFilePath ) )
	{
		ERR_WCC( TXT("Expecting a path to a split list file") );
		return false;
	}	

	// input file
	if ( !options.GetSingleOptionValue( TXT("file"), m_inputFile ) )
	{
		ERR_WCC( TXT("Expecting input file name") );
		return false;
	}

	// fallback chunk
	String fallbackChunkName;
	if ( options.GetSingleOptionValue( TXT("fallback"), fallbackChunkName ) )
	{
		m_fallBackChunk = CName( fallbackChunkName.AsChar() );
	}

	// should we strip content from cache that is not cooked ?
	if ( options.HasOption( TXT("strip") ) )
	{
		m_stripNotCooked = true;
		LOG_WCC( TXT("Not cooked content will be stripped") );
	}

	// we need the tool name
	if ( options.GetFreeArguments().Empty() )
	{
		ERR_WCC( TXT("Expecting tool name") );
		return false;
	}

	// get the tool name (first free arg for now)
	m_builderName = options.GetFreeArguments()[0];

	// all settings parsed
	return true;
}

//----

CCacheSplitterCommandlet::CCacheSplitterCommandlet()
{
	m_commandletName = CName( TXT("splitcache") );
}

CCacheSplitterCommandlet::~CCacheSplitterCommandlet()
{
}

Bool CCacheSplitterCommandlet::Execute( const CommandletOptions& options )
{
	CTimeCounter timer;

	// parse settings
	if ( !m_settings.Parse(options))
		return false;

	// find tool class
	Red::TUniquePtr< IBaseCacheSplitter > splitter;
	TDynArray< CClass* > toolClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IBaseCacheSplitter >(), toolClasses );
	for ( CClass* toolClass : toolClasses )
	{
		IBaseCacheSplitter* splitterTool = toolClass->GetDefaultObject< IBaseCacheSplitter >();
		if ( splitterTool && ( m_settings.m_builderName == splitterTool->GetName() ) )
		{
			splitter.Reset( toolClass->CreateObject< IBaseCacheSplitter >() );
			break;
		}
	}

	// no tool
	if ( !splitter )
	{
		ERR_WCC( TXT("Cache splitter for '%ls' not found"), m_settings.m_builderName.AsChar() );
		return false;
	}

	// load split list
	if ( !m_splitFile.LoadFromFile( m_settings.m_splitFilePath ) )
	{
		ERR_WCC( TXT("Failed to load split list file '%ls'"), m_settings.m_splitFilePath.AsChar() );
		return false;
	}

	// make sure output path exists
	if ( !GFileManager->CreatePath( m_settings.m_rootOutputDirectory ) )
	{
		ERR_WCC( TXT("Failed to create output path '%ls'"), m_settings.m_rootOutputDirectory.AsChar() );
		return false;
	}

	// initialize the cache builder
	if ( !splitter->Initialize( options ) )
	{
		ERR_WCC( TXT("Failed to initialize cache splitter '%ls'"), m_settings.m_builderName.AsChar() );
		return false;
	}

	///---

	// Actual splitting - start by loading the input file
	if ( !splitter->LoadInput( m_settings.m_inputFile ) )
	{
		ERR_WCC( TXT("Failed to loading cache '%ls' for splitting"), m_settings.m_inputFile.AsChar() );
		return false;
	}

	// Collect entries
	TDynArray< IBaseCacheEntry* > entries;
	splitter->GetEntries( entries );
	LOG_WCC( TXT("Found %d entries in the cache"), entries.Size() );

	// Output caches
	TSortedMap< CName, OutputCache* > outputCaches;

	// Process cache entries
	Uint64 numStrippedDataSize = 0;
	for ( Uint32 i=0; i<entries.Size(); ++i )
	{
		IBaseCacheEntry* entry = entries[i];

		// Update progress
		const String depotPath = entry->GetResourcePath();
		LOG_WCC( TXT("Status: [%d/%d] Processing '%ls'..."), 
			i, entries.Size(), depotPath.AsChar() );

		// Final content chunk name
		CName contentChunkName = CName::NONE;

		// Find the resource in the split file
		Red::Core::ResourceManagement::CResourceId resourceId( depotPath );
		const CCookerSplitFileEntry* splitEntry = m_splitFile.GetEntry( resourceId );
		if ( !splitEntry )
		{
			if ( m_settings.m_stripNotCooked )
			{
				numStrippedDataSize += entry->GetApproxSize();
				WARN_WCC( TXT("Resource '%ls' is not present in cook - the data will be stripped"), depotPath.AsChar() );
				continue;
			}
			else
			{
				WARN_WCC( TXT("Resource '%ls' is not present in cook - fallback chunk will be assigned"), depotPath.AsChar() );
			}
		}
		else
		{
			// get the information about file from the split list
			contentChunkName = splitEntry->GetFileChunkID();
		}

		// no valid chunk resolved - use the fallback chunk
		if ( contentChunkName == CName::NONE )
		{
			contentChunkName = m_settings.m_fallBackChunk;
		}

		// find/create output cache
		OutputCache* outputCache = nullptr;
		if ( !outputCaches.Find( contentChunkName, outputCache ) )
		{
			outputCache = new OutputCache( contentChunkName );
			outputCaches.Insert( contentChunkName, outputCache );
		}

		// add entry to the output cache
		outputCache->m_entries.PushBack( entry );
	}

	// Result table
	Uint32 cacheIndex = 0;
	Uint32 numTotalEntries = 0;
	Uint64 numTotalSize = 0;
	LOG_WCC( TXT("Split table:") );
	for ( auto it = outputCaches.Begin(); it != outputCaches.End(); ++it )
	{
		OutputCache* info = (*it).m_second;

		Uint64 approxEntriesSize = 0;
		for ( IBaseCacheEntry* entry : info->m_entries )
			approxEntriesSize += entry->GetApproxSize();

		LOG_WCC( TXT(" %d) %10s: %d entries (%1.2fMB approx)"),
			cacheIndex, info->m_name.AsChar(), info->m_entries.Size(), approxEntriesSize / (1024.0f*1024.0f) );

		numTotalSize += approxEntriesSize;
		numTotalEntries += info->m_entries.Size();
		cacheIndex += 1;
	}
	LOG_WCC( TXT("Total: %d entries, (%1.2fMB approx)"),
		numTotalEntries, (Double)numTotalSize / (1024.0*1024.0) );

	// Stats about stripped entries
	if ( numTotalEntries < entries.Size() )
	{
		LOG_WCC( TXT("Stripped: %d entries (%1.2fMB approx)"),
			(entries.Size() - numTotalEntries), (Double)numStrippedDataSize / (1024.0*1024.0) );
	}

	// Start writing caches
	for ( auto it = outputCaches.Begin(); it != outputCaches.End(); ++it )
	{
		OutputCache* info = (*it).m_second;

		LOG_WCC( TXT("Status: Writing part '%ls'..."), 
			info->m_name.AsChar() );

		// format the output path
		String outputFilePath = m_settings.m_rootOutputDirectory;
		outputFilePath += info->m_name.AsChar();
		outputFilePath += TXT("\\");

		// base file path
		const CFilePath basePath( m_settings.m_inputFile );
		outputFilePath += basePath.GetFileNameWithExt().AsChar();
		LOG_WCC( TXT("Writing cache to '%ls' (%d entries)..."), outputFilePath.AsChar(), info->m_entries.Size() );

		// create output path
		if ( !GFileManager->CreatePath( outputFilePath ) )
		{
			ERR_WCC( TXT("Failed to create path for '%ls'"), outputFilePath.AsChar() );
			return false;
		}

		// store
		if ( !splitter->SaveOutput( outputFilePath, info->m_entries ) )
		{
			ERR_WCC( TXT("Failed to save splitted cache to '%ls'"), outputFilePath.AsChar() );
			return false;
		}
	}

	// Done
	LOG_WCC( TXT("Splitting of cache '%ls' done in %1.2fs"), 
		m_settings.m_inputFile.AsChar(), timer.GetTimePeriod() );
	return true;
}

void CCacheSplitterCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  splitcache <type> -file=<file> -db=<cook.db> -outdir=<path>") );
	LOG_WCC( TXT("Options:") );
	LOG_WCC( TXT("  <type> - what kind of cache you want to split (physics, texture, etc)") );
	LOG_WCC( TXT("  -file=<file> - input cache file") );
	LOG_WCC( TXT("  -db=<cook.db> - path to the cook.db file to use as a reference") );
	LOG_WCC( TXT("  -outdir=<path> - where to place output caches") );
	LOG_WCC( TXT("  -strip - remove data from non cooked files") );
	LOG_WCC( TXT("  -fallback=<name> - use custom fallback chunk name") );
}
