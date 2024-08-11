/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/utility.h"

#include "packageLanguagesPS4.h"
#include "packageChunkLayerPairPS4.h"

struct SPatchParamsPS4;

class CPackageBatchCommandWriterPS4 : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	IFile&									m_fileWriter;
	const SPackageLanguagesPS4&				m_packageLanguages;

public:
	CPackageBatchCommandWriterPS4( IFile& fileWriter, const SPackageLanguagesPS4& packageLanguages );

public:
	Bool CreateAppPackage( const String& contentID, const String& passcode );
	Bool CreatePatchPackage( const String& contentID, const String& passcode, const SPatchParamsPS4& params );
	Bool CreateDlcPackage( const String& contentID, const String& passcode, const String& entitlementKey );
	Bool PlayGoUpdate( const TDynArray< String >& gameLanguages, const String& defaultGameLanguage );
	Bool PlayGoUpdateWithRawPlayGoLanguages( const String& playGoLanguages, const String& defaultPlayGoLangauge );
	Bool ChunkAdd( const SChunkLayerInfoPS4& chunkLayerPair, const TDynArray< String >& gameLanguages, const String& label = String::EMPTY );
	Bool ChunkAddWithRawPlayGoLanguages( const SChunkLayerInfoPS4& chunkLayerPair, const String& playGoLanguages, const String& label = String::EMPTY );
	Bool ChunkAdd( const SChunkLayerInfoPS4& chunkLayerPair, const String& label = String::EMPTY );
	Bool ChunkUpdate( const SChunkLayerInfoPS4& chunkLayerPair, const String& label = String::EMPTY );
	Bool ScenarioUpdate( Uint32 scenarioID, Uint32 initialChunkCount, const TDynArray< Uint32 >& installOrderChunkIDs, const String& label = String::EMPTY );
	Bool AddFile( const String& origPath, const String& targPath, const SChunkLayerInfoPS4& chunkLayerPair, Bool compressed = false );
	Bool AddFileNoChunk( const String& origPath, const String& targPath, Bool compressed = false );
	Bool SortFiles();

private:
	Bool GetPlayGoLanguages( const TDynArray< String >& gameLanguages, String& outPlayGoLanguages ) const;
	Bool GetPlayGoLanguage( const String& gameLanguage, String& outPlayGoLanguage ) const;
};
