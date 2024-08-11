/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchCommandlet.h"
#include "patchUtils.h"

#include "../../common/core/dependencyMapper.h"

IMPLEMENT_ENGINE_CLASS( CPatchCommandlet );

//--------------------------------------------------------------------------------

CPatchCommandlet::Settings::Settings()
	: m_patchName( TXT("patch") )
	, m_platform( PLATFORM_PC )
	, m_isCurrentBuildAMod( false )
{
}

Bool CPatchCommandlet::Settings::Parse( const CommandletOptions& options )
{
	if ( !options.GetSingleOptionValue( TXT("base"), m_baseBuildPath ) )
	{
		ERR_WCC( TXT("Expected base build directory") );
		return false;
	}

	if ( !m_baseBuildPath.EndsWith( TXT("\\") ) )
		m_baseBuildPath += TXT("\\");

	if ( !GFileManager->FileExist( m_baseBuildPath + TXT("content\\metadata.store") ) )
	{
		ERR_WCC( TXT("Directory '%ls' does not contain valid build"), m_baseBuildPath.AsChar() );
		return false;
	}

	// we can patch another build or a mod
	if ( options.HasOption( TXT("current") ) )
	{	
		if ( !options.GetSingleOptionValue( TXT("current"), m_currentBuildPath ) )
		{
			ERR_WCC( TXT("Expected current build directory") );
			return false;
		}

		if ( !m_currentBuildPath.EndsWith( TXT("\\") ) )
			m_currentBuildPath += TXT("\\");

		if ( !GFileManager->FileExist( m_currentBuildPath + TXT("content\\metadata.store") ) )
		{
			ERR_WCC( TXT("Directory '%ls' does not contain valid build"), m_currentBuildPath.AsChar() );
			return false;
		}

		m_patchName = TXT("patch0");
		m_isCurrentBuildAMod = false;
	}
	else if ( options.HasOption( TXT("mod") ) )
	{
		if ( !options.GetSingleOptionValue( TXT("mod"), m_currentBuildPath ) )
		{
			ERR_WCC( TXT("Expected mod build directory") );
			return false;
		}

		if ( !m_currentBuildPath.EndsWith( TXT("\\") ) )
			m_currentBuildPath += TXT("\\");

		m_patchName = TXT("mod0");
		m_isCurrentBuildAMod = true;
	}

	// two options
	if ( options.HasOption( TXT("mod") ) && options.HasOption( TXT("current") ) )
	{
		ERR_WCC( TXT("-mod and -current options cannot be used at the same time") );
		return false;
	}

	// custom package name
	if ( options.HasOption( TXT("name") ) )
	{
		options.GetSingleOptionValue( TXT("name"), m_patchName );
		LOG_WCC( TXT("Output package name set to '%ls'"), m_patchName.AsChar() );
	}
	else
	{
		LOG_WCC( TXT("Output package name: '%ls'. Use -name to change."), m_patchName.AsChar() );
	}

	if ( !options.GetSingleOptionValue( TXT("outdir"), m_outputPath ) )
	{
		ERR_WCC( TXT("Expected output directory") );
		return false;
	}

	if ( !m_outputPath.EndsWith( TXT("\\") ) )
		m_outputPath += TXT("\\");

	if ( options.GetSingleOptionValue( TXT("dump"), m_dumpPath ) )
	{
		if ( !m_dumpPath.EndsWith( TXT("\\") ) )
			m_dumpPath += TXT("\\");
	}

	// we need the tool name
	if ( options.GetFreeArguments().Empty() )
	{
		ERR_WCC( TXT("Expecting patch builder name (see usage for list of builders)") );
		return false;
	}

	String platformName;
	if ( !options.GetSingleOptionValue( TXT("platform"), platformName ) )
	{
		ERR_WCC( TXT("Expecting platform name") );
		return false;
	}

	// match platform name
	if ( platformName == TXT("pc") )
	{
		m_platform = PLATFORM_PC;
	}
#ifndef WCC_LITE
	else if ( platformName == TXT("xboxone") || platformName == TXT("durango") )
	{
		m_platform = PLATFORM_XboxOne;
	}
	else if ( platformName == TXT("ps4") || platformName == TXT("orbis") )
	{
		m_platform = PLATFORM_PS4;
	}
#endif
	else
	{
		ERR_WCC( TXT("Invalid platform name: %ls"), platformName.AsChar() );
		return false;
	}

	// patch name
	options.GetSingleOptionValue( TXT("name"), m_patchName );
	m_patchName = TXT("content\\") + m_patchName;

	// get the tool name (first free arg for now)
	m_builderName = options.GetFreeArguments()[0];

	// parsed
	return true;
}

//--------------------------------------------------------------------------------

// HACK
Bool GPatchingMod = false;

CPatchCommandlet::CPatchCommandlet()
{
	m_commandletName = CName( TXT("patch") );
}

CPatchCommandlet::~CPatchCommandlet()
{
}

Bool CPatchCommandlet::Execute( const CommandletOptions& options )
{
	// allocate the helper buffer
	PatchUtils::AllocateTempMemory();

	// parse the options
	if ( !m_settings.Parse( options ) )
		return false;

	// find the builder
	IBasePatchContentBuilder* builder = nullptr;
	{
		TDynArray< CClass* > classes;
		SRTTI::GetInstance().EnumClasses( ClassID< IBasePatchContentBuilder >(), classes );
		for ( CClass* builderClass : classes )
		{
			IBasePatchContentBuilder* currentBuilder = builderClass->GetDefaultObject<IBasePatchContentBuilder>();
			if ( currentBuilder->GetContentType() == m_settings.m_builderName )
			{
				builder = builderClass->CreateObject< IBasePatchContentBuilder >();
				break;
			}
		}
	}

	// unknown builder
	if ( !builder )
	{
		ERR_WCC( TXT("Unknown patch content type: '%ls'"), m_settings.m_builderName.AsChar() );
		return false;
	}

	// some builders cannot be used in mod configurations
	if ( m_settings.m_isCurrentBuildAMod && !builder->CanUseWithMods() )
	{
		ERR_WCC( TXT("Patch content builder '%ls' cannot be used with mods"), m_settings.m_builderName.AsChar() );
		return false;
	}

	// hack
	GPatchingMod = m_settings.m_isCurrentBuildAMod;	

	// load content from the second build
	TDynArray< IBasePatchContentBuilder::IContentToken* > currentTokensList;
	THashMap< Uint64, IBasePatchContentBuilder::IContentToken* > currentTokensMap;
	IBasePatchContentBuilder::IContentGroup* currentData = builder->LoadContent( m_settings.m_platform, m_settings.m_currentBuildPath );
	if ( !currentData )
	{
		ERR_WCC( TXT("Failed to load current build content for patch builder '%ls' from '%ls'"), 
			m_settings.m_builderName.AsChar(), m_settings.m_baseBuildPath.AsChar() );
		delete builder;
		return false;
	}
	else
	{
		// build the base token table
		currentData->GetTokens( currentTokensList );
		currentTokensMap.Reserve( currentTokensList.Size() );
		for ( auto* token : currentTokensList )
		{
			IBasePatchContentBuilder::IContentToken* existingToken = nullptr;
			if ( currentTokensMap.Find( token->GetTokenHash(), existingToken ) )
			{
				ERR_WCC( TXT("Duplicated content token at hash 0x%016llX, collision of '%ls' with '%ls'. Unable to generate conclusive patch."),
					token->GetTokenHash(), token->GetInfo().AsChar(), existingToken->GetInfo().AsChar() );
				delete builder;
				return false;
			}

			currentTokensMap.Insert( token->GetTokenHash(), token );
		}
	}

	// disable hack
	GPatchingMod = false;

	// load content from the first build
	TDynArray< IBasePatchContentBuilder::IContentToken* > baseTokensList;
	THashMap< Uint64, IBasePatchContentBuilder::IContentToken* > baseTokensMap;
	IBasePatchContentBuilder::IContentGroup* baseData = builder->LoadContent( m_settings.m_platform, m_settings.m_baseBuildPath );
	if ( !baseData )
	{
		ERR_WCC( TXT("Failed to load base build content for patch builder '%ls' from '%ls'"), 
			m_settings.m_builderName.AsChar(), m_settings.m_baseBuildPath.AsChar() );
		delete builder;
		return false;
	}
	else
	{
		// build the base token table
		baseData->GetTokens( baseTokensList );
		baseTokensMap.Reserve( baseTokensList.Size() );
		for ( auto* token : baseTokensList )
		{
			IBasePatchContentBuilder::IContentToken* existingToken = nullptr;
			if ( baseTokensMap.Find( token->GetTokenHash(), existingToken ) )
			{
				ERR_WCC( TXT("Duplicated content token at hash 0x%016llX, collision of '%ls' with '%ls'. Unable to generate conclusive patch."),
					token->GetTokenHash(), token->GetInfo().AsChar(), existingToken->GetInfo().AsChar() );
				delete builder;
				return false;
			}

			baseTokensMap.Insert( token->GetTokenHash(), token );
		}
	}

	// changed pairs
	struct DeltaPair
	{
		IBasePatchContentBuilder::IContentToken*	m_base;
		IBasePatchContentBuilder::IContentToken*	m_current;

		DeltaPair( IBasePatchContentBuilder::IContentToken* base, IBasePatchContentBuilder::IContentToken* current )
		{
			m_base = base;
			m_current = current;
		}
	};

	// generate sets of data to added/changed
	Uint32 numAdded = 0, numRemoved = 0, numChanged = 0, numChangedAdditionalData = 0;
	Uint64 addedDataSize = 0, changedDataSize = 0, removedDataSize = 0;
	TDynArray< DeltaPair > deltaTokens;
	for ( IBasePatchContentBuilder::IContentToken* currentToken : currentTokensList )
	{
		// find token in the base data
		IBasePatchContentBuilder::IContentToken* baseToken = nullptr;
		baseTokensMap.Find( currentToken->GetTokenHash(), baseToken );

		// added or changed ?
		if ( !baseToken )
		{
			numAdded += 1;
			addedDataSize += currentToken->GetDataSize();
			deltaTokens.PushBack( DeltaPair( nullptr, currentToken ) );
		}
		else if ( baseToken->GetDataCRC() != currentToken->GetDataCRC() )
		{
			numChanged += 1;
			changedDataSize += currentToken->GetDataSize();
			deltaTokens.PushBack( DeltaPair( baseToken, currentToken ) );
		}
		else if ( baseToken->GetAdditionalData() != currentToken->GetAdditionalData() )
		{
			// right now the GetAdditionalData() was used only by shader patching mechanism, the "additional data" was the CRC of includes folder.
			// because the includes folder changed, all shaders were determined to be patched even if their code did not change at all (eg. change in terrain shader in the includes does not affect 99% of the regular materials)
			// leaving the mechanism for now to minimize the scope of change
			numChangedAdditionalData += 1;
		}
	}

	// generate set of data that was removed
	for ( IBasePatchContentBuilder::IContentToken* baseToken : baseTokensList )
	{
		// find token in the current data
		IBasePatchContentBuilder::IContentToken* currentToken = nullptr;
		currentTokensMap.Find( baseToken->GetTokenHash(), currentToken );

		// added or changed ?
		if ( !currentToken )
		{
			numRemoved += 1;
			removedDataSize += baseToken->GetDataSize();
			deltaTokens.PushBack( DeltaPair( baseToken, nullptr ) );
		}
	}

	// stats
	LOG_WCC( TXT("Differential data:") );
	LOG_WCC( TXT("  Added %d objects (%1.2f KB of data)"), numAdded, addedDataSize / 1024.0f );
	LOG_WCC( TXT("  Removed %d objects (%1.2f KB of data)"), numRemoved, removedDataSize / 1024.0f );
	LOG_WCC( TXT("  Changed %d objects (%1.2f KB of data)"), numChanged, changedDataSize / 1024.0f );

	// save the report
	{
		const String reportPath = m_settings.m_outputPath + m_settings.m_builderName + TXT("_diff.txt");

		GFileManager->CreatePath( reportPath );
		FILE* f = _wfopen( reportPath.AsChar(), L"w" );
		if ( f ) 
		{
			// stats
			fwprintf( f, TXT("Differential report for '%ls':\n"), m_settings.m_builderName.AsChar() );
			fwprintf( f, TXT("  Added %d objects (%1.2f KB of data)\n"), numAdded, addedDataSize / 1024.0f );
			fwprintf( f, TXT("  Removed %d objects (%1.2f KB of data)\n"), numRemoved, removedDataSize / 1024.0f );
			fwprintf( f, TXT("  Changed %d objects (%1.2f KB of data)\n"), numChanged, changedDataSize / 1024.0f );

			// added stuff
			if ( numAdded > 0 )
			{
				fwprintf( f, TXT("Added:\n") );
				Uint32 index = 0;
				for ( const auto& delta : deltaTokens )
				{
					if ( !delta.m_base && delta.m_current )
					{
						fwprintf( f, TXT("  [%d]: '%ls' (CRC: 0x%016llX), Size: %d %ls\n"),
							index++, delta.m_current->GetInfo().AsChar(), delta.m_current->GetDataCRC(), delta.m_current->GetDataSize(), delta.m_current->GetAdditionalInfo().AsChar() );
					}
				}
				fwprintf( f, TXT("\n") );
			}

			// removed stuff
			if ( numRemoved > 0 )
			{
				fwprintf( f, TXT("Removed:\n") );
				Uint32 index = 0;
				for ( const auto& delta : deltaTokens )
				{
					if ( delta.m_base && !delta.m_current )
					{
						fwprintf( f, TXT("  [%d]: '%ls' (CRC: 0x%016llX), Size: %d %ls\n"),
							index++, delta.m_base->GetInfo().AsChar(), delta.m_base->GetDataCRC(), delta.m_base->GetDataSize(), delta.m_base->GetAdditionalInfo().AsChar() );
					}
				}
				fwprintf( f, TXT("\n") );
			}

			// changed stuff
			if ( numChanged > 0 )
			{
				fwprintf( f, TXT("Changed:\n") );
				Uint32 index = 0;
				for ( const auto& delta : deltaTokens )
				{
					if ( delta.m_base && delta.m_current )
					{
						fwprintf( f, TXT("  [%d]: '%ls':\n"), index++, delta.m_base->GetInfo().AsChar() );
						fwprintf( f, TXT("     Base: (CRC: 0x%016llX), Size: %d %ls\n"), delta.m_base->GetDataCRC(), delta.m_base->GetDataSize(), delta.m_base->GetAdditionalInfo().AsChar() );
						fwprintf( f, TXT("     Cur:  (CRC: 0x%016llX), Size: %d %ls\n"), delta.m_current->GetDataCRC(), delta.m_current->GetDataSize(), delta.m_current->GetAdditionalInfo().AsChar() );
					}
				}
				fwprintf( f, TXT("\n") );
			}

			fclose(f);
		}
	}

	// dump differences
	if ( !m_settings.m_dumpPath.Empty() )
	{
		LOG_WCC( TXT("Dumping difference...") );
		for ( const auto& delta : deltaTokens )
		{
			if ( delta.m_base && delta.m_current )
			{
				LOG_WCC( TXT("Writing dump for '%ls'..."), delta.m_base->GetInfo().AsChar() );

				delta.m_base->DebugDump( m_settings.m_dumpPath, true );
				delta.m_current->DebugDump( m_settings.m_dumpPath, false  );
			}
		}
	}

	// collect the changed data size
	TDynArray< IBasePatchContentBuilder::IContentToken* > finalListToSave;
	for ( const auto& delta : deltaTokens )
	{
		// save the modified data
		if ( delta.m_current )
		{
			finalListToSave.PushBack( delta.m_current );
		}
	}

	// save the changed data
	if ( !finalListToSave.Empty() )
	{
		if ( !builder->SaveContent( m_settings.m_platform, baseData, currentData, finalListToSave, m_settings.m_outputPath, m_settings.m_patchName ) )
		{
			ERR_WCC( TXT("Failed to store patched content in '%ls'"), m_settings.m_outputPath.AsChar() );
			return false;
		}
	}
	else
	{
		WARN_WCC( TXT("There were no differences. Nothing saved.") );
	}

	// cleanup
	delete builder;
	return true;
}

void CPatchCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  patch <contenttype> -base=path -current=path|-mod=path [-name=packagename] -outdir=path") );
	LOG_WCC( TXT("Content types:") );

	{
		TDynArray< CClass* > classes;
		SRTTI::GetInstance().EnumClasses( ClassID< IBasePatchContentBuilder >(), classes );
		for ( CClass* builderClass : classes )
		{
			IBasePatchContentBuilder* builder = builderClass->GetDefaultObject<IBasePatchContentBuilder>();
			LOG_WCC( TXT("   \"%ls\" %ls"), 
				builder->GetContentType().AsChar(),
				builder->CanUseWithMods() ? TXT("MODS + PATCH") : TXT("PATCH ONLY") );
		}
	}

	LOG_WCC( TXT("Required parameters:") );
	LOG_WCC( TXT("  -base=path     - Path to the base build (GoldMaster)") );
	LOG_WCC( TXT("  -current=path  - (pro) Path to the current build (latest cook)") );
	LOG_WCC( TXT("  -mod=path      - Path to the cooked mod directory") );
	LOG_WCC( TXT("  -path=path     - Output directory where the patched content will be dumped.") );
	LOG_WCC( TXT("  -name=name     - Output using custom directory name.") );
	LOG_WCC( TXT("Note: you can use -mod OR -current params") );
}
