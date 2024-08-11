/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/file.h"
#include "../../common/core/string.h"
#include "../../common/core/xmlFileReader.h"
#include "../../common/core/playGoPackage.h"
#include "../../common/core/stringConversion.h"

#include "packagePlayGoChunksBuilder.h"

namespace PackageHelpers
{

CPackagePlayGoChunksBuilder::CPackagePlayGoChunksBuilder( CXMLReader& xmlReader )
	: m_xmlReader( xmlReader )
{}

Bool CPackagePlayGoChunksBuilder::ParsePackage( SPackageInfo& outPackage ) const
{
	if ( m_xmlReader.BeginNode(TXT("psproject")) )
	{
		if ( ! ParseProject( m_xmlReader, outPackage ) )
		{
			return false;
		}
		m_xmlReader.EndNode();
	}

	return true;
}

Bool CPackagePlayGoChunksBuilder::ParseProject( CXMLReader& xmlReader, SPackageInfo& outPackage ) const
{
	const String EXPECTED_FORMAT(TXT("gp4"));
	const Uint32 EXPECTED_VERSION = 1000;

	String format;
	Uint32 version = 0;

	xmlReader.Attribute( TXT("fmt"), format);
	String attrVersion;
	if ( xmlReader.Attribute( TXT("version"), attrVersion ) )
	{
		FromString( attrVersion, version );
	}
	if ( format != EXPECTED_FORMAT )
	{
		ERR_WCC(TXT("CPackagePlayGoChunksBuilder: Parse error. <psproject> fmt '%ls', expected '%ls'"), format, EXPECTED_FORMAT.AsChar() );
		return false;
	}
	if ( version != EXPECTED_VERSION )
	{
		ERR_WCC(TXT("CPackagePlayGoChunksBuilder: Parse error. <psproject> version '%u', expected '%u'"), version, EXPECTED_VERSION );
		return false;
	}

	if ( xmlReader.BeginNode(TXT("volume")) )
	{
		if ( ! ParseVolume( xmlReader, outPackage ) )
		{
			return false;
		}
		xmlReader.EndNode();
	}
	else
	{
		ERR_WCC(TXT("CPackagePlayGoChunksBuilder: Parse error. <volume> not found"));
		return false;
	}

	return true;
}

Bool CPackagePlayGoChunksBuilder::ParseVolume( CXMLReader& xmlReader, SPackageInfo& outPackage ) const
{
	if ( xmlReader.BeginNode(TXT("chunk_info")) )
	{
		if ( ! ParseChunkInfo( xmlReader, outPackage ) )
		{
			return false;
		}
		xmlReader.EndNode();
		return true;
	}

	ERR_WCC(TXT("CPackagePlayGoChunksBuilder: Parse error. <chunk_info> not found"));
	return false;
}

Bool CPackagePlayGoChunksBuilder::ParseChunkInfo( CXMLReader& xmlReader, SPackageInfo& outPackage ) const
{
	Uint32 numChunks = 0;
	Uint32 numScenarios = 0;

	String attrChunkCount;
	if ( xmlReader.Attribute( TXT("chunk_count"), attrChunkCount ) )
	{
		FromString( attrChunkCount, numChunks );
	}
	String attrScenarioCount;
	if ( xmlReader.Attribute( TXT("scenario_count"), attrScenarioCount ) )
	{
		FromString( attrScenarioCount, numScenarios );
	}

	if ( xmlReader.BeginNode( TXT("chunks")) )
	{
		if ( ! ParseChunks( xmlReader, outPackage ) )
		{
			return false;
		}
		xmlReader.EndNode();
	}

	if ( outPackage.m_chunks.Size() != numChunks )
	{
		ERR_WCC(TXT("CPackagePlayGoChunksBuilder: Wrong number of chunks. Expected %u, parsed %u"), numChunks, outPackage.m_chunks.Size() );
		return false;
	}

	if ( xmlReader.BeginNode( TXT("scenarios")) )
	{
		if ( ! ParseScenarios( xmlReader, outPackage ) )
		{
			return false;
		}
		xmlReader.EndNode();
	}

	return true;
}

Bool CPackagePlayGoChunksBuilder::ParseChunks( CXMLReader& xmlReader, SPackageInfo& outPackage ) const
{
	if ( xmlReader.Attribute(TXT("supported_languages"), outPackage.m_rawSupportedPlayGoLanguages ) )
	{
		if ( !xmlReader.Attribute(TXT("default_language"), outPackage.m_rawDefaultPlayGoLanguage ) )
		{
			ERR_WCC(TXT("Failed to parse default_language"));
			return false;
		}
	}
	else
	{
		ERR_WCC(TXT("Failed to parse supported_languages"));
		return false;
	}

	for( ; xmlReader.BeginNode(TXT("chunk")); xmlReader.EndNode() )
	{
		SPackageChunk chunk;
		if ( ! ParseChunk( xmlReader, chunk ) )
		{
			return false;
		}
		outPackage.m_chunks.PushBack( Move(chunk) );
	}
	return true;
}

Bool CPackagePlayGoChunksBuilder::ParseChunk( CXMLReader& xmlReader, SPackageChunk& outChunk ) const
{
	String attrId;
	if ( !xmlReader.Attribute(TXT("id"), attrId) || ! FromString( attrId, outChunk.m_chunkID) )
	{
		ERR_WCC(TXT("CPackagePlayGoChunksBuilder: Failed to parse chunk ID"));
		return false;
	}

	xmlReader.Attribute(TXT("languages"), outChunk.m_rawPlayGoLanguages);
	if ( outChunk.m_rawPlayGoLanguages.Empty())
	{
		outChunk.m_rawPlayGoLanguages = TXT("all");
	}
	
	xmlReader.Attribute(TXT("label"), outChunk.m_label );

	return true;
}

Bool CPackagePlayGoChunksBuilder::ParseScenarios( CXMLReader& xmlReader, SPackageInfo& outPackage ) const
{
	// FIXME: clobbering scenarios, but really just want one!
	for( ; xmlReader.BeginNode(TXT("scenario")); xmlReader.EndNode() )
	{
		SPackageScenario scenario;
		if ( ! ParseScenario( xmlReader, scenario) )
		{
			return false;
		}
		outPackage.m_scenario = Move(scenario);
	}

	return true;
}

Bool CPackagePlayGoChunksBuilder::ParseScenario( CXMLReader& xmlReader, SPackageScenario& outScenario ) const
{
	String attrInitialChunkCount;
	if ( !xmlReader.Attribute(TXT("initial_chunk_count"), attrInitialChunkCount) || !FromString( attrInitialChunkCount, outScenario.m_numInitialChunks ) )
	{
		ERR_WCC(TXT("Failed to parse initial_chunk_count"));
		return false;
	}

	String attrID;
	if ( !xmlReader.Attribute(TXT("id"), attrID) || !FromString(attrID, outScenario.m_scenarioID) )
	{
		ERR_WCC(TXT("Failed to parse scenario ID"));
		return false;
	}

	xmlReader.Attribute(TXT("label"), outScenario.m_label);

	String scenarioChunkIds;
	xmlReader.Value( scenarioChunkIds );
	if ( !ParsePlayGoIDRange(scenarioChunkIds, outScenario.m_installOrderChunkIDs) )
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool CPackagePlayGoChunksBuilder::ParsePlayGoIDRange( const String& src, TDynArray< Uint32 >& outIDs ) const
{
	const Char* stream = src.AsChar();
	while ( *stream )
	{
		Uint32 rangeStart = 0;
		if ( !GParseInteger( stream, rangeStart ) )
		{
			break;
		}
		Uint32 rangeEnd = rangeStart;
		if ( !GParseKeyword(stream, TXT("-")) || !GParseInteger(stream, rangeEnd) )
		{
			rangeEnd = rangeStart;
		}

		for ( Uint32 i = rangeStart; i <= rangeEnd; ++i )
		{
			outIDs.PushBack( i );
		}
	}

	if ( *stream )
	{
		ERR_WCC(TXT("CPackagePlayGoChunksBuilder: Error parsing IDs at '%ls'"), stream );
		return false;
	}

	return true;
}

} // namespace PackageHelpers