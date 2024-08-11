/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "packageBatchCommandWriterPS4.h"
#include "packageConstants.h"
#include "packageSkuPS4.h"
#include "../../common/redSystem/stringWriter.h"

CPackageBatchCommandWriterPS4::CPackageBatchCommandWriterPS4( IFile& fileWriter, const SPackageLanguagesPS4& packageLanguages )
	: m_fileWriter( fileWriter )
	, m_packageLanguages( packageLanguages )
{
}

Bool CPackageBatchCommandWriterPS4::PlayGoUpdateWithRawPlayGoLanguages( const String& playGoLanguages, const String& defaultPlayGoLangauge )
{
	StringAnsi batchCmd = StringAnsi::Printf("gp4_playgo_update --supported_languages \"%ls\" --default_language \"%ls\" --chunk_count %u\n",
		playGoLanguages.AsChar(), defaultPlayGoLangauge.AsChar(), (MAX_CHUNK_ID+1));

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::PlayGoUpdate( const TDynArray< String >& gameLanguages, const String& defaultGameLanguage )
{
	if ( !gameLanguages.Exist( defaultGameLanguage ) )
	{
		ERR_WCC(TXT("Default game language '%ls' not found in list of supported languages"), defaultGameLanguage.AsChar() );
		ERR_WCC(TXT("Game languages..."));
		for ( const String & gameLang : gameLanguages )
		{
			ERR_WCC(TXT("Game language: {%ls}"), gameLang.AsChar());
		}
		return false;
	}

	String playGoLanguages;
	if ( ! GetPlayGoLanguages( gameLanguages, playGoLanguages ) )
	{
		return false;
	}

	String defaultPlayGoLanguage;
	if ( ! GetPlayGoLanguage( defaultGameLanguage, defaultPlayGoLanguage ) )
	{
		return false;
	}

	StringAnsi batchCmd = StringAnsi::Printf("gp4_playgo_update --supported_languages \"%ls\" --default_language \"%ls\" --chunk_count %u\n",
		playGoLanguages.AsChar(), defaultPlayGoLanguage.AsChar(), (MAX_CHUNK_ID+1));

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::CreateAppPackage( const String& contentID, const String& passcode )
{
	StringAnsi batchCmd = StringAnsi::Printf("gp4_proj_create --app_type full --storage_type bd50 --volume_type pkg_ps4_app --content_id %ls --passcode %ls\n",
		contentID.AsChar(), passcode.AsChar() );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::CreatePatchPackage( const String& contentID, const String& passcode, const SPatchParamsPS4& params )
{
	if ( params.m_isDayOne && !params.m_latestPatchPath.Empty() )
	{
		ERR_WCC(TXT("Day one and a latest patch path '%ls'"), params.m_latestPatchPath.AsChar());
		return false;
	}

	Red::System::StackStringWriter<AnsiChar, 1024> buffer;

	buffer.Appendf("gp4_proj_create --app_type full --storage_type digital25 --volume_type pkg_ps4_patch --content_id %ls --passcode %ls",
		contentID.AsChar(), passcode.AsChar() );
	
	/*
	Publishing Tools Command Line Version User's Guide:
		If this patch is not a Day 1 patch or if there is no QA passed patch, specify ref_a. 
		If it is a Day 1 patch or if a QA passed patch exists, do not specify this option or specify "" (empty string) which means "no setting".
	*/
	if ( !params.m_isDayOne && params.m_latestPatchPath.Empty() )
	{
		buffer.Appendf(" --patch_type ref_a");
	}

	if ( !params.m_latestPatchPath.Empty() )
	{
		buffer.Appendf(" --latest_patch_path %ls", params.m_latestPatchPath.AsChar() );
	}

	buffer.Appendf(" --app_path %ls", params.m_appPkgPath.AsChar());
	buffer.Append("\n");

	Uint32 len = (Uint32)Red::System::StringLength( buffer.AsChar() );
	m_fileWriter.Serialize( (void*)buffer.AsChar(), len );

	return true;
}

Bool CPackageBatchCommandWriterPS4::CreateDlcPackage( const String& contentID, const String& passcode, const String& entitlementKey )
{
	StringAnsi batchCmd = StringAnsi::Printf("gp4_proj_create --volume_type pkg_ps4_ac_data --content_id %ls --passcode %ls --entitlement_key %ls\n",
		contentID.AsChar(), passcode.AsChar(), entitlementKey.AsChar() );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::ChunkAdd( const SChunkLayerInfoPS4& chunkLayerPair, 
												 const TDynArray< String >& gameLanguages /*=const TDynArray< String >()*/, 
												 const String& label /*= String::EMPTY*/ )
{
	String playGoLanguages;
	if ( ! GetPlayGoLanguages( gameLanguages, playGoLanguages ) )
	{
		return false;
	}

	const Uint32 chunkID = chunkLayerPair.m_chunkID;
	const Uint32 layer = chunkLayerPair.m_layer;
	StringAnsi batchCmd = StringAnsi::Printf("gp4_chunk_add --id \"%u\" --label \"%ls\" --languages \"%ls\" --layer_no %u\n", chunkID, label.AsChar(), playGoLanguages.AsChar(), layer );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::ChunkAddWithRawPlayGoLanguages( const SChunkLayerInfoPS4& chunkLayerPair, const String& playGoLanguages, const String& label /*= String::EMPTY*/ )
{
	const Uint32 chunkID = chunkLayerPair.m_chunkID;
	const Uint32 layer = chunkLayerPair.m_layer;
	StringAnsi batchCmd = StringAnsi::Printf("gp4_chunk_add --id \"%u\" --label \"%ls\" --languages \"%ls\" --layer_no %u\n", chunkID, label.AsChar(), playGoLanguages.AsChar(), layer );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::ChunkAdd( const SChunkLayerInfoPS4& chunkLayerPair, const String& label /*= String::EMPTY */ )
{
	return ChunkAdd( chunkLayerPair, TDynArray<String>(), label );
}

Bool CPackageBatchCommandWriterPS4::ChunkUpdate( const SChunkLayerInfoPS4& chunkLayerPair, const String& label /*= String::EMPTY*/ )
{
	const Uint32 chunkID = chunkLayerPair.m_chunkID;
	const Uint32 layer = chunkLayerPair.m_layer;

	// Not supporting languages in this batch cmd, since this is really just for chunk #0, and you can't even use --language "all".
	StringAnsi batchCmd = StringAnsi::Printf("gp4_chunk_update --id \"%u\" --label \"%ls\" --layer_no %u\n", chunkID, label.AsChar(), layer );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::ScenarioUpdate( Uint32 scenarioID, Uint32 initialChunkCount, const TDynArray< Uint32 >& installOrderChunkIDs, const String& label /*= String::EMPTY */ )
{
	if ( scenarioID > 1 )
	{
		ERR_WCC(TXT("Invalid scenario ID %u"), scenarioID );
		return false;
	}

	if ( initialChunkCount < 1 || initialChunkCount > (MAX_CHUNK_ID+1) )
	{
		ERR_WCC(TXT("Invalid initial chunk count %u (must be between 1 and %u"), initialChunkCount, (MAX_CHUNK_ID+1) );
		return false;
	}

	if ( installOrderChunkIDs.Empty() )
	{
		ERR_WCC(TXT("No install chunkIDs specified!"));
		return false;
	}

	AnsiChar chunkString[1024] = { '\0' };
	AnsiChar* chunkPtr = chunkString;
	const Int32 numChunkIDs = installOrderChunkIDs.SizeInt();
	Uint32 bufsz = ARRAY_COUNT_U32(chunkString);
	for ( Int32 i = 0; i < numChunkIDs; ++i )
	{
		const Uint32 chunkID = installOrderChunkIDs[ i ];
		const Int32 numCharsWritten = Red::System::SNPrintF( chunkPtr, bufsz, ( i < numChunkIDs-1 ) ? "%u " : "%u", chunkID );
		if ( numCharsWritten < 1 )
		{
			ERR_WCC(TXT("Error formatting chunkID!"));
			return false;
		}
		chunkPtr += numCharsWritten;
		bufsz -= numCharsWritten;
	}

	StringAnsi batchCmd = StringAnsi::Printf("gp4_scenario_update --id \"%u\" --label \"%ls\" --initial_chunk_count %u --scenario \"%hs\"\n",
		scenarioID, label.AsChar(), initialChunkCount, chunkString );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );

	return true;
}

Bool CPackageBatchCommandWriterPS4::GetPlayGoLanguages( const TDynArray< String >& gameLanguages, String& outPlayGoLanguages ) const
{
	String playGoLanguages;
	for ( Uint32 i = 0; i < gameLanguages.Size(); ++i )
	{
		const String& gameLang = gameLanguages[i];
		String playGoLang;
		if ( ! m_packageLanguages.m_gameToPlayGoLangMap.Find( gameLang.ToLower(), playGoLang ) )
		{
			ERR_WCC(TXT("Supported game language '%ls' not mapped to a PlayGo language"), gameLang.AsChar() );
			return false;
		}
		playGoLanguages += playGoLang.AsChar();

		if ( i < gameLanguages.Size()-1 )
		{
			playGoLanguages += TXT(" ");
		}
	}

	outPlayGoLanguages = Move( playGoLanguages );
	if ( outPlayGoLanguages.Empty() )
	{
		outPlayGoLanguages = TXT("all");
	}
	return true;
}

Bool CPackageBatchCommandWriterPS4::GetPlayGoLanguage( const String& gameLanguage, String& outPlayGoLanguage ) const
{
	return m_packageLanguages.m_gameToPlayGoLangMap.Find( gameLanguage.ToLower(), outPlayGoLanguage );
}

Bool CPackageBatchCommandWriterPS4::AddFile( const String& origPath, const String& targPath, const SChunkLayerInfoPS4& chunkLayerPair, Bool compressed /*=false*/ )
{
	const Uint32 chunkID = chunkLayerPair.m_chunkID;
	const Uint32 layer = chunkLayerPair.m_layer;

	StringAnsi batchCmd = StringAnsi::Printf("gp4_file_add --force %ls --chunks \"%u\" --layer_no %u \"%ls\" \"%ls\"\n",
		( compressed ? TXT("--pfs_compression enable") : TXT("") ),
		chunkID, layer, origPath.AsChar(), targPath.AsChar() );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );
	return true;
}

Bool CPackageBatchCommandWriterPS4::AddFileNoChunk( const String& origPath, const String& targPath, Bool compressed /*= false */ )
{
	StringAnsi batchCmd = StringAnsi::Printf("gp4_file_add --force %ls \"%ls\" \"%ls\"\n",
		( compressed ? TXT("--pfs_compression enable") : TXT("") ), origPath.AsChar(), targPath.AsChar() );

	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );
	return true;
}

Bool CPackageBatchCommandWriterPS4::SortFiles()
{
	StringAnsi batchCmd("gp4_file_sort\n");
	m_fileWriter.Serialize( (void*)batchCmd.AsChar(), batchCmd.GetLength() );
	return true;
}

