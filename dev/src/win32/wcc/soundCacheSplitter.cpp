/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "baseCacheSplitter.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/soundFileLoader.h"
#include "soundBankHelper.h"
#include "cookSplitList.h"
#include "../../common/engine/soundCacheDataFormat.h"

class CSoundCacheSplitter : public IBaseCacheSplitter
{
	DECLARE_RTTI_SIMPLE_CLASS( CSoundCacheSplitter );

public:

	// Description
	virtual const Char* GetName() const { return TXT("sounds"); }
	virtual const Char* GetDescription() const { return TXT("Split sound cache and create caches per level and common sound cache"); }

	// Interface
	virtual Bool Initialize(const ICommandlet::CommandletOptions& additonalOptions);
	virtual Bool LoadInput(const String& absolutePath);
	virtual void GetEntries(TDynArray< IBaseCacheEntry* >& allEntries) const;
	virtual Bool SaveOutput(const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries) const;

private:
	String					m_splitListFilePath;
	CCookerSplitFile		m_splitFile;

	CSoundCacheData			m_soundCacheData;
	IFile*					m_soundCacheFile;
	Red::System::DateTime	m_originalTimeStamp;

};
BEGIN_CLASS_RTTI( CSoundCacheSplitter )
	PARENT_CLASS( IBaseCacheSplitter );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CSoundCacheSplitter );

// wrapper for a single entry in the physics cache
class CSoundCacheSplitEntry : public IBaseCacheEntry
{
public:
	CSoundCacheSplitEntry( const Uint32 tokenIndex, const CSoundCacheData* data )
		: m_tokenIndex( tokenIndex )
		, m_data( data )
	{
	}

	virtual String GetResourcePath() const override
	{
		const CSoundCacheData::CacheToken& token = m_data->m_tokens[ m_tokenIndex ];
		const AnsiChar* path = &m_data->m_strings[ token.m_name ];
		return ANSI_TO_UNICODE( path );
	}

	virtual Uint32 GetApproxSize() const override
	{
		const CSoundCacheData::CacheToken& token = m_data->m_tokens[ m_tokenIndex ];
		return (Uint32)token.m_dataSize;
	}

public:
	Uint32						m_tokenIndex;
	const CSoundCacheData*		m_data;
};

Bool CSoundCacheSplitter::Initialize(const ICommandlet::CommandletOptions& additonalOptions)
{
	if( additonalOptions.GetSingleOptionValue( TXT("split"), m_splitListFilePath ) == false )
	{
		ERR_WCC( TXT("No split option. Try -split=\"splitlistfilepath\"") );
		return false;
	}
	return true;
}

Bool CSoundCacheSplitter::LoadInput(const String& absolutePath)
{
	/***** Load split list file *****/
	if( m_splitFile.LoadFromFile( m_splitListFilePath ) == false )
	{
		ERR_WCC( TXT("Can't load split list file: %ls"), m_splitListFilePath.AsChar() );
		return false;
	}

	/***** Load sound cache *****/
	m_soundCacheFile = GFileManager->CreateFileReader( absolutePath, FOF_AbsolutePath );
	if( m_soundCacheFile == nullptr )
	{
		return false;
	}

	// Dummy read.
	Uint32 originalVersion = 0;
	
	// load file header
	if ( !CSoundCacheData::ValidateHeader( *m_soundCacheFile, m_originalTimeStamp, originalVersion ) )
	{
		ERR_WCC( TXT("Specified file is NOT a sound cache") );
		return false;
	}

	if( m_soundCacheData.Load( *m_soundCacheFile ) == false )
	{
		ERR_WCC( TXT("Failed to load tokens from specified sound cache file") );
		return false;
	}

	return true;
}

void CSoundCacheSplitter::GetEntries(TDynArray< IBaseCacheEntry* >& allEntries) const
{
	for( Uint32 entryIdx = 0; entryIdx < m_splitFile.GetNumEntries(); ++entryIdx )
	{
		const CCookerSplitFileEntry* entry = m_splitFile.GetEntry( entryIdx );
		for( Uint32 tokenIdx = 0; tokenIdx < m_soundCacheData.m_tokens.Size(); ++tokenIdx )
		{
			const CSoundCacheData::CacheToken& token = m_soundCacheData.m_tokens[tokenIdx];
			if( StringAnsi( &m_soundCacheData.m_strings[ token.m_name ] ) == entry->GetFilePath() )
			{
				allEntries.PushBack( new CSoundCacheSplitEntry( tokenIdx, &m_soundCacheData ) );
				break;
			}
		}
	}
}

Bool CSoundCacheSplitter::SaveOutput(const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries) const
{
	// open output file
	IFile* outputFile = GFileManager->CreateFileWriter( absolutePath, FOF_Buffered | FOF_AbsolutePath );
	if ( !outputFile )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), absolutePath.AsChar() );
		return false;
	}

	LOG_WCC( TXT("Content chunk file '%ls'"), absolutePath.AsChar() );

	// preallocate read buffer
	TDynArray< Uint8 > readBuffer;
	readBuffer.Resize( 1 << 20 );

	// write initial header - preserve time stamp
	CSoundCacheData::WriteHeader( *outputFile, m_originalTimeStamp );

	// skip to start of first data block
	const Uint32 writePos = sizeof( CSoundCacheData::RawHeader ) + sizeof( CSoundCacheData::IndexHeader );
	outputFile->Seek( writePos );

	// transfer data from entries
	CSoundCacheData outData;
	CSoundCacheDataBuilder outDataBuilder( outData );
	for ( Uint32 i=0; i<allEntries.Size(); ++i )
	{
		const Uint32 tokenIndex = static_cast< CSoundCacheSplitEntry* >( allEntries[i] )->m_tokenIndex;

		// make sure reading buffer is large enough
		const CSoundCacheData::CacheToken& inToken = m_soundCacheData.m_tokens[ tokenIndex ];
		if ( inToken.m_dataSize > readBuffer.Size() )
			readBuffer.Resize( inToken.m_dataSize );

		// read source data from source file
		m_soundCacheFile->Seek( inToken.m_dataOffset );
		m_soundCacheFile->Serialize( readBuffer.Data(), inToken.m_dataSize );

		// write data to output file
		const Uint32 writePos = (Uint32) outputFile->GetOffset();
		outputFile->Serialize( readBuffer.Data(), inToken.m_dataSize );

		// create new token in output file
		CSoundCacheData::CacheToken outToken = inToken;
		outToken.m_name = outDataBuilder.AddString( &m_soundCacheData.m_strings[ inToken.m_name ] );
		outToken.m_dataOffset = writePos;
		outDataBuilder.AddToken( outToken );

		LOG_WCC( TXT("    Sound resource file: %ls"), ANSI_TO_UNICODE( &m_soundCacheData.m_strings[ inToken.m_name ] ));
	}

	// save the new tables
	const Uint32 endOfFilePos = (Uint32) outputFile->GetOffset();
	outputFile->Seek( sizeof( CSoundCacheData::RawHeader ) );
	outData.Save( *outputFile, endOfFilePos );

	// close output file
	delete outputFile;
	return true;
}
