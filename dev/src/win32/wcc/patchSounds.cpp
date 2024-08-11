/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchSounds.h"
#include "patchUtils.h"
#include "../../common/core/fileSys.h"
#include "../../common/core/dependencyMapper.h"

//-----------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_Sounds );

//-----------------------------------------------------------------------------

namespace Helper
{
	RED_INLINE void EnumerateSoundCaches( const String& baseDirectory, TDynArray<String>& outSoundCaches )
	{
		GFileManager->FindFiles( baseDirectory, TXT("sounds*.cache"), outSoundCaches, true );
		LOG_WCC( TXT("Found %d sound caches in build '%ls'"), outSoundCaches.Size(), baseDirectory.AsChar() );
	}

	void CleanRIFFHeader( void* buffer, Uint32 maxSize )
	{
		struct RIFFHeader
		{
			Uint32 m_riffMagic;
			Uint32 m_riffChunkLength;
			Uint32 m_waveMagic;
			Uint32 m_fmtMagic;
			Uint32 m_fmtSize;
			Uint16 m_fmtFormatCode;
			Uint16 m_fmtChannels;
			Uint32 m_fmtSampleRate;
			Uint32 m_fmtBytesPerSecond;
			Uint16 m_fmtBytesPerSample;
			Uint16 m_fmtBitsPerSample;

			Bool IsValid( )
			{
				return m_riffMagic == 'FFIR' && m_waveMagic == 'EVAW' && m_fmtMagic == ' tmf';
			}
		};

		// Memzero undocumented extension chunk in RIFF header.
		RIFFHeader* riffHeader = reinterpret_cast< RIFFHeader* >( buffer );
		if( riffHeader->IsValid( ) && riffHeader->m_fmtSize == 0x24 )			// Check if that's a case with 36 bytes
		{
			Red::MemoryZero( reinterpret_cast< void* >( reinterpret_cast< Uint8* >( riffHeader ) + sizeof( RIFFHeader ) + 0x22 - 0x10 ), 2 );		// Clear last 2 bytes
		}
	}

	void CleanAllRIFFHeaders( void* buffer, Uint32 size )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			void* curData = (void*)( (Uint64)( buffer ) + i );
			Uint32 maxSize = size - i;
			CleanRIFFHeader( curData, maxSize );
		}
	}
}

//-----------------------------------------------------------------------------

CPatchSounds::~CPatchSounds()
{
	m_tokens.ClearPtr();
}

void CPatchSounds::GetTokens(TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens) const 
{
	outTokens.Reserve( m_tokens.Size() );
	for ( auto* token : m_tokens )
	{
		outTokens.PushBack( token );
	}
}

const Uint64 CPatchSounds::GetDataSize() const 
{
	Uint64 totalSize = 0;
	for ( auto* token : m_tokens )
	{
		totalSize += token->GetDataSize();
	}
	return totalSize;
}

const String CPatchSounds::GetInfo() const 
{
	return TXT("Sounds");
}

void CPatchSounds::AddToken(CPatchSoundFileToken* token)
{
	m_tokens.PushBack( token );
}

//-----------------------------------------------------------------------------

CPatchSoundFileToken::CPatchSoundFileToken( const String& tokenName, Red::TSharedPtr<IFile> cacheFile, Red::TSharedPtr<CSoundCacheData> cacheData, Int32 cacheTokenIndex, Uint64 crc )
	: m_tokenName( tokenName )
	, m_tokenHash( Red::System::CalculateHash64( UNICODE_TO_ANSI( tokenName.AsChar() ) ) )
	, m_cacheTokenIndex( cacheTokenIndex )
	, m_cacheFile( cacheFile )
	, m_cacheDataFormat( cacheData )
	, m_cacheTokenCrc( crc )
{
	
}

CPatchSoundFileToken::~CPatchSoundFileToken()
{

}

const Uint64 CPatchSoundFileToken::GetTokenHash() const 
{
	return m_tokenHash;
}

const Uint64 CPatchSoundFileToken::GetDataCRC() const 
{
	return m_cacheTokenCrc;
}

const Uint64 CPatchSoundFileToken::GetDataSize() const 
{
	return m_cacheDataFormat->m_tokens[m_cacheTokenIndex].m_dataSize;
}

const String CPatchSoundFileToken::GetInfo() const 
{
	return String::Printf( TXT("Sound cache file '%ls'"), m_tokenName.AsChar() );
}

void CPatchSoundFileToken::DebugDump(const String& dumpPath, const Bool isBase) const 
{
	String absolutePath = dumpPath;
	absolutePath += m_tokenName;
	absolutePath += isBase ? TXT(".base") : TXT(".current");

	CSoundCacheData::CacheToken& cacheTokenData = m_cacheDataFormat->m_tokens[ m_cacheTokenIndex ];

	m_cacheFile->Seek( cacheTokenData.m_dataOffset );

	TDynArray< Uint8 > buffer;
	buffer.Resize( cacheTokenData.m_dataSize );
	m_cacheFile->Serialize( buffer.Data(), (Uint32)buffer.DataSize() );

	Helper::CleanAllRIFFHeaders( buffer.Data(), (Uint32)buffer.DataSize() );

	Red::TScopedPtr< IFile > destFile( GFileManager->CreateFileWriter( absolutePath.AsChar(), FOF_AbsolutePath ) );
	destFile->Serialize( buffer.Data(), (Uint32)buffer.DataSize() );
}

//-----------------------------------------------------------------------------

String CPatchBuilder_Sounds::GetContentType() const 
{
	return TXT("sounds");
}

IBasePatchContentBuilder::IContentGroup* CPatchBuilder_Sounds::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	TDynArray<String> soundCachePaths;
	Helper::EnumerateSoundCaches( absoluteBuildPath, soundCachePaths );
	
	CPatchSounds* patchSounds = new CPatchSounds();
	Bool result = FillPatchSounds(patchSounds, soundCachePaths);
	if( result == false )
	{
		return nullptr;
	}

	return patchSounds;
}

Bool CPatchBuilder_Sounds::SaveContent(const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IBasePatchContentBuilder::IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	if( patchContent.Size() == 0 )
	{
		LOG_WCC( TXT("No sound differences.") );
		return true;
	}

	String outputFilePath;
	
	if ( platform == PLATFORM_PC )
	{
		outputFilePath = absoluteBuildPath + patchName + TXT("\\sounds.cache");
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_PS4 )
	{
		outputFilePath = absoluteBuildPath + patchName + TXT("\\soundsps4.cache");
	}
	else if ( platform == PLATFORM_XboxOne )
	{
		outputFilePath = absoluteBuildPath + patchName + TXT("\\soundsxboxone.cache");
	}	
#endif

	IFile* outputFile = GFileManager->CreateFileWriter( outputFilePath, FOF_Buffered | FOF_AbsolutePath );
	if ( !outputFile )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), outputFilePath.AsChar() );
		return false;
	}

	// preallocate read buffer
	TDynArray< Uint8 > readBuffer;
	readBuffer.Resize( 1 << 20 );

	// write initial header - preserve time stamp
	CSoundCacheData::WriteHeader( *outputFile );

	// skip to start of first data block
	const Uint64 writePos = sizeof( CSoundCacheData::RawHeader ) + sizeof( CSoundCacheData::IndexHeader );
	outputFile->Seek( writePos );

	CSoundCacheData outData;
	CSoundCacheDataBuilder outDataBuilder( outData );
	for( Uint32 i=0; i<patchContent.Size(); ++i )
	{
		CPatchSoundFileToken* token = static_cast<CPatchSoundFileToken*>( patchContent[i] );
		const Uint32 tokenIndex = token->m_cacheTokenIndex;

		Red::TSharedPtr<IFile> soundCacheFile = token->m_cacheFile;
		Red::TSharedPtr<CSoundCacheData> soundCacheData = token->m_cacheDataFormat;

		// make sure reading buffer is large enough
		const CSoundCacheData::CacheToken& inToken = soundCacheData->m_tokens[ tokenIndex ];
		if ( inToken.m_dataSize > readBuffer.Size() )
			readBuffer.Resize( inToken.m_dataSize );

		// read source data from source file
		soundCacheFile->Seek( inToken.m_dataOffset );
		soundCacheFile->Serialize( readBuffer.Data(), inToken.m_dataSize );

		// write data to output file
		const Uint64 writePos = outputFile->GetOffset();
		outputFile->Serialize( readBuffer.Data(), inToken.m_dataSize );

		// create new token in output file
		CSoundCacheData::CacheToken outToken = inToken;
		outToken.m_name = outDataBuilder.AddString( &soundCacheData->m_strings[ inToken.m_name ] );
		outToken.m_dataOffset = writePos;
		outDataBuilder.AddToken( outToken );
	}

	// save the new tables
	const Uint64 endOfFilePos =  outputFile->GetOffset();
	outputFile->Seek( sizeof( CSoundCacheData::RawHeader ) );
	outData.Save( *outputFile, endOfFilePos );

	// close output file
	delete outputFile;
	return true;
}

Bool CPatchBuilder_Sounds::FillPatchSounds(CPatchSounds* patchSounds, const TDynArray<String>& soundCachePaths)
{
	CSoundCachePatchTokenExctactor tokenExtractor;

	for( const String& path : soundCachePaths )
	{
		CSoundCacheSimpleReader cacheReader( path );
		if( cacheReader.Load() == false )
		{
			return false;
		}

		tokenExtractor.SetSource( &cacheReader );
		tokenExtractor.Extract( patchSounds );
		tokenExtractor.ResetSource();
	}

	return true;
}

//-----------------------------------------------------------------------------

CSoundCacheSimpleReader::CSoundCacheSimpleReader(const String& filepath)
	: m_filepath( filepath )
	, m_version( 0 )
{

}

Bool CSoundCacheSimpleReader::Load()
{
	LOG_WCC( TXT("Loading cache '%ls'..."), m_filepath.AsChar() );

	if( CreateFileReader() == false )
		return false;

	if( ValidateSoundCacheHeader() == false )
		return false;

	if( LoadCacheData() == false )
		return false;

	return true;
}

TDynArray<CSoundCacheData::CacheToken>& CSoundCacheSimpleReader::GetTokens()
{
	return m_cacheData->m_tokens;
}

Red::TSharedPtr<IFile> CSoundCacheSimpleReader::GetFile()
{
	return m_cacheFile;
}

Red::TSharedPtr<CSoundCacheData> CSoundCacheSimpleReader::GetCacheData()
{
	return m_cacheData;
}

Bool CSoundCacheSimpleReader::CreateFileReader()
{
	m_cacheFile = Red::TSharedPtr<IFile>( GFileManager->CreateFileReader( m_filepath, FOF_AbsolutePath ) );
	if( m_cacheFile == nullptr )
	{
		ERR_WCC( TXT("Can't create file reader for: %ls"), m_filepath.AsChar() );
		return false;
	}

	return true;
}

Bool CSoundCacheSimpleReader::ValidateSoundCacheHeader()
{
	Red::System::DateTime originalTimestamp;

	if ( !CSoundCacheData::ValidateHeader( *m_cacheFile, originalTimestamp, m_version ) )
	{
		ERR_WCC( TXT("Sound cache header is invalid: %ls"), m_filepath.AsChar() );
		return false;
	}

	return true;
}

Bool CSoundCacheSimpleReader::LoadCacheData()
{
	m_cacheData = Red::TSharedPtr<CSoundCacheData>( new CSoundCacheData() );
	m_cacheData->m_version = m_version;
	if( m_cacheData->Load( *m_cacheFile ) == false )
	{
		ERR_WCC( TXT("Failed to load tokens from sound cache file: %ls"), m_filepath.AsChar() );
		return false;
	}

	return true;
}

TDynArray<AnsiChar>& CSoundCacheSimpleReader::GetStrings()
{
	return m_cacheData->m_strings;
}

Uint32 CSoundCacheSimpleReader::GetTokenCount()
{
	auto& tokens = GetTokens();
	return tokens.Size();
}

//-----------------------------------------------------------------------------

CSoundCachePatchTokenExctactor::CSoundCachePatchTokenExctactor()
	: m_source( nullptr )
{

}

void CSoundCachePatchTokenExctactor::SetSource(CSoundCacheSimpleReader* source)
{
	RED_FATAL_ASSERT( source != nullptr, "SoundPatcher: source file is invalid!" );
	m_source = source;
}

void CSoundCachePatchTokenExctactor::Extract(CPatchSounds* patchSounds)
{
	RED_FATAL_ASSERT( m_source != nullptr, "SoundPatcher: source file is invalid!" );

	Uint32 tokenCount = m_source->GetTokenCount();

	for( Uint32 tokenIdx = 0; tokenIdx < tokenCount; ++tokenIdx )
	{
		String tokenName;
		Uint64 tokenNameHash = 0;

		GetTokenNameAndNameHash( tokenIdx, tokenName, tokenNameHash );

		if ( CheckTokenAlreadyExtracted(tokenNameHash) )
			continue;

		CPatchSoundFileToken* patchToken = CreatePatchToken(tokenIdx, tokenName, tokenNameHash);

		patchSounds->AddToken( patchToken );
	}
}

void CSoundCachePatchTokenExctactor::ResetSource()
{
	m_source = nullptr;
}

void CSoundCachePatchTokenExctactor::GetTokenNameAndNameHash( const Uint32 tokenIndex, String& outName, Uint64& outNameHash )
{
	TDynArray<CSoundCacheData::CacheToken>& soundCacheTokens = m_source->GetTokens();
	TDynArray<AnsiChar>& soundCacheStrings = m_source->GetStrings();

	const CSoundCacheData::CacheToken& token = soundCacheTokens[tokenIndex];
	StringAnsi tokenName = StringAnsi( &soundCacheStrings[ token.m_name ] );

	outName = String( ANSI_TO_UNICODE( tokenName.AsChar() ) );
	outNameHash = Red::System::CalculateHash64( tokenName.AsChar() );
}

Bool CSoundCachePatchTokenExctactor::CheckTokenAlreadyExtracted( const Uint64 tokenHash )
{
	return m_fileTokensMap.KeyExist( tokenHash );
}

CPatchSoundFileToken* CSoundCachePatchTokenExctactor::CreatePatchToken(const Uint32 tokenIdx, const String& tokenName, const Uint64 tokenNameHash)
{
	Red::TSharedPtr<IFile> cacheFile = m_source->GetFile();
	Red::TSharedPtr<CSoundCacheData> cacheData = m_source->GetCacheData();

	Uint64 tokenCrc = GetTokenCrc(tokenIdx, tokenName);

	CPatchSoundFileToken* fileToken = new CPatchSoundFileToken( tokenName, cacheFile, cacheData, tokenIdx, tokenCrc );

	m_fileTokensMap.Insert( tokenNameHash, fileToken );

	return fileToken;
}

Uint64 CSoundCachePatchTokenExctactor::GetTokenCrc(const Uint32 tokenIdx, const String &tokenName)
{
	CSoundCacheData::CacheToken& cacheTokenData = GetTokenData( tokenIdx );
	ReadTokenContentToBuffer(cacheTokenData, m_readBuffer);
	Helper::CleanAllRIFFHeaders( m_readBuffer.Data(), (Uint32)m_readBuffer.DataSize() );
	return CalculateTokenCrc(tokenName, m_readBuffer);
}

CSoundCacheData::CacheToken& CSoundCachePatchTokenExctactor::GetTokenData(const Uint32 tokenIndex)
{
	TDynArray<CSoundCacheData::CacheToken>& allCacheTokens = m_source->GetTokens();
	return allCacheTokens[ tokenIndex ];
}

void CSoundCachePatchTokenExctactor::ReadTokenContentToBuffer(const CSoundCacheData::CacheToken &cacheTokenData, TDynArray< Uint8 >& readBuffer)
{
	Red::TSharedPtr<IFile> cacheFile = m_source->GetFile();

	readBuffer.Resize( cacheTokenData.m_dataSize );

	cacheFile->Seek( cacheTokenData.m_dataOffset );
	cacheFile->Serialize( readBuffer.Data(), cacheTokenData.m_dataSize );
}

Uint64 CSoundCachePatchTokenExctactor::CalculateTokenCrc(const String &tokenName, const TDynArray< Uint8 >& cacheTokenContent)
{
	Uint64 crc = Red::System::CalculateHash64( tokenName.Data(), tokenName.DataSize(), RED_FNV_OFFSET_BASIS64 );
	Uint64 cacheTokenCrc = Red::System::CalculateHash64( cacheTokenContent.Data(), cacheTokenContent.Size(), crc );
	return cacheTokenCrc;
}
