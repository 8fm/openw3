/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchPhysics.h"
#include "patchUtils.h"
#include "../../common/engine/collisionCacheDataFormat.h"
#include "../../common/core/compression/zlib.h"
#include "../../common/physics/compiledCollision.h"

//-----------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_Physics );

//-----------------------------------------------------------------------------

/// Collison cache information
class CPatchPhysicsFile
{
public:
	~CPatchPhysicsFile();

	// Load single collison cache from given path
	static CPatchPhysicsFile* LoadPysicsCache( const String& absolutePath );

	Bool LoadEntryDataDecompressed( Uint32 entryIndex, TDynArray< Uint8 >& outBuffer ) const;

	Bool LoadEntryData(Uint32 entryIndex, TDynArray< Uint8 >& readBuffer) const;

	// Get collision cache path
	RED_INLINE const String& GetAbsolutePath() const { return m_absoluteFilePath; }

	// Get collision cache entries
	RED_INLINE const Uint32 GetEntriesNumber() const { return m_data.m_tokens.Size(); }

	// Get collision cache entry path
	RED_INLINE StringAnsi GetEntryName( Uint32 index ) const 
	{
		const CCollisionCacheData::CacheToken& token = m_data.m_tokens[ index ];
		const StringAnsi path = &m_data.m_strings[ token.m_name ];
		return path;
	}

	// Get bundle file names
	RED_INLINE const Uint64 GetEntryNameHash( Uint32 index ) const { return m_data.m_tokens[index].m_nameHash; }

	RED_INLINE const CCollisionCacheData * GetCollisionCacheDataPtr() const { return &m_data; }

private:
	Bool Load( const String& absolutePath );


private:
	String						m_absoluteFilePath;

	CCollisionCacheData			m_data;
	IFile*						m_file;
	Red::System::DateTime		m_originalTimeStamp;
};

//-----------------------------------------------------------------------------

CPatchPhysicsFile::~CPatchPhysicsFile()
{
	// close the file
	if ( m_file )
	{
		delete m_file;
		m_file = nullptr;
	}
}

CPatchPhysicsFile* CPatchPhysicsFile::LoadPysicsCache( const String& absolutePath )
{
	// the output file
	CPatchPhysicsFile* ret = new CPatchPhysicsFile();

	if( ret->Load( absolutePath ) )
	{
		return ret;
	}
	else
	{
		delete ret;
		return nullptr;
	}
}

Bool CPatchPhysicsFile::LoadEntryDataDecompressed( Uint32 entryIndex, TDynArray< Uint8 >& outBuffer ) const
{
	TDynArray< Uint8 > readBuffer;
	const CCollisionCacheData::CacheToken& token = m_data.m_tokens[ entryIndex ];

	if( !LoadEntryData(entryIndex, readBuffer) )
		return false;

	// decompress the data
	if ( token.m_dataSizeInMemory > token.m_dataSizeOnDisk )
	{
		// make sure reading buffer is large enough (should no happen)
		if ( outBuffer.Size() < token.m_dataSizeInMemory )
		{
			outBuffer.Resize( token.m_dataSizeInMemory );
		}

		// decompress data from READ buffer to out buffer
		Red::Core::Decompressor::CZLib zlib;
		if ( Red::Core::Decompressor::Status_Success != zlib.Initialize( readBuffer.Data(), outBuffer.Data(), token.m_dataSizeOnDisk, token.m_dataSizeInMemory ) )
		{
			return false;
		}

		// let's hope decompression will not fail
		if ( Red::Core::Decompressor::Status_Success != zlib.Decompress() )
		{
			return false;
		}
	}
	return true;
}

Bool CPatchPhysicsFile::LoadEntryData(Uint32 entryIndex, TDynArray< Uint8 >& outBuffer) const
{
	const CCollisionCacheData::CacheToken& token = m_data.m_tokens[ entryIndex ];

	// load data into the load buffer
	{
		if ( token.m_dataSizeOnDisk > outBuffer.Size() )
			outBuffer.Resize( token.m_dataSizeOnDisk );

		// read source data from source file
		m_file->Seek( token.m_dataOffset );
		m_file->Serialize( outBuffer.Data(), token.m_dataSizeOnDisk );
	}

	// check data CRC
	const Uint64 dataCRC = Red::System::CalculateHash64( outBuffer.Data(), token.m_dataSizeOnDisk );
	if ( dataCRC != token.m_diskCRC )
	{
		ERR_WCC( TXT("Collision cache: CRC check for data for file '%ls' failed"), GetEntryName( entryIndex ).AsChar() );
		return false;
	}

	return true;
}

Bool CPatchPhysicsFile::Load( const String& absolutePath )
{
	// open file
	m_file = GFileManager->CreateFileReader( absolutePath, FOF_AbsolutePath | FOF_Buffered );
	if ( !m_file )
	{
		return false;
	}

	// load file header
	if ( !CCollisionCacheData::ValidateHeader( *m_file, m_originalTimeStamp ) )
	{
		ERR_WCC( TXT("Specified file is NOT a collision cache") );
		return false;
	}

	// load the entries
	if ( !m_data.Load( *m_file ) )
	{
		ERR_WCC( TXT("Failed to load tokens from specified collision cache file") );
		return false;
	}

	m_absoluteFilePath = absolutePath;

	// valid
	return true;
}

//-----------------------------------------------------------------------------

void CPatchPhysics::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	outTokens.Reserve( m_tokens.Size() );
	for ( auto* ptr : m_tokens )
	{
		outTokens.PushBack( ptr );
	}
}

const Uint64 CPatchPhysics::GetDataSize() const
{
	Uint64 totalSize = 0;
	for ( auto* ptr : m_tokens )
	{
		totalSize += ptr->GetDataSize();
	}

	return totalSize;
}

const String CPatchPhysics::GetInfo() const
{
	return TXT("Physics");
}

CPatchPhysics* CPatchPhysics::LoadCollisionCaches(const String& baseDirectory)
{
	// enumerate collision cache files at given directory
	TDynArray< String > colCachePaths;
	GFileManager->FindFiles( baseDirectory, TXT("collision.cache"), colCachePaths, true );

	// map with current tokens
	THashMap< Uint64, CPatchPhysicsFileToken* > fileTokensMap;

	// loaded collision cache files
	CPatchPhysics* collisionCaches = new CPatchPhysics();
	collisionCaches->m_basePath = baseDirectory;

	// load the collision cache files
	LOG_WCC( TXT("Found %d collision.cache files in build"), colCachePaths.Size() );
	for ( const String& path : colCachePaths )
	{
		// load the collision cache file
		LOG_WCC( TXT("Loading collision.cache '%ls'..."), path.AsChar() );
		CPatchPhysicsFile* collisionCache = CPatchPhysicsFile::LoadPysicsCache( path );
		if ( !collisionCache )
		{
			delete collisionCache;
			ERR_WCC( TXT("Failed to load bundle from '%ls'. The build is not valid."), path.AsChar() );
			return nullptr;
		}

		// add to list
		collisionCaches->m_collisionCaches.PushBack( collisionCache );

		// process entries
		const Uint32 numFilesInBundle = collisionCache->GetEntriesNumber();
		LOG_WCC( TXT("Found %d entries in cache"), numFilesInBundle );
		for ( Uint32 i=0; i< numFilesInBundle; ++i )
		{
			// get file name (path)
			const StringAnsi fileName = collisionCache->GetEntryName(i);

			// hash the path
			const Uint64 fileNameHash = Red::CalculateHash64( fileName.AsChar() );

			// already has a token ?
			if ( fileTokensMap.KeyExist( fileNameHash ) )
			{
				continue;
			}

			// create file mapping
			CPatchPhysicsFileToken* fileToken = new CPatchPhysicsFileToken( collisionCache, i, fileName, fileNameHash );

			// insert into token map
			fileTokensMap.Insert( fileNameHash, fileToken );
			collisionCaches->m_tokens.PushBack( fileToken );
		}
	}
	return collisionCaches;
}

//-----------------------------------------------------------------------------

CPatchPhysicsFileToken::CPatchPhysicsFileToken( const CPatchPhysicsFile* data, Uint32 entryIndex, StringAnsi filePath, Uint64 hash )
	: m_entryIndex( entryIndex )
	, m_tokenHash( hash )
	, m_filePath( filePath )
{
	m_ownerCacheFile = data;
	m_data = m_ownerCacheFile->GetCollisionCacheDataPtr();

	// Calculate dataCrc

	const CCollisionCacheData::CacheToken& token = m_data->m_tokens[ entryIndex ];
	
	if( token.m_collisionType == RTT_ApexCloth || token.m_collisionType == RTT_ApexDestruction )
	{
		TDynArray< Uint8 > tempBuffer;

		// load data into the load buffer
		{
			if ( token.m_dataSizeOnDisk > tempBuffer.Size() )
				tempBuffer.Resize( token.m_dataSizeOnDisk );

			m_ownerCacheFile->LoadEntryDataDecompressed(entryIndex, tempBuffer );

		}
		// Clear the version bytes
		tempBuffer[90] = 0;
		tempBuffer[91] = 0;

		// For destruction, find the "apexXXX" string
		if( token.m_collisionType == RTT_ApexDestruction )
		{
			Uint8 apexChar[4] = {'a','p','e','x'};

			Uint32 size = tempBuffer.Size();
			Uint32 current = 0;
			Uint32 start = -1;
			Uint32 end = -1;
			for( Uint32 i = 92; i < size; )
			{
				if( tempBuffer[i] == apexChar[current] )
				{
					do
					{
						++i;
						++current;
					}
					while( tempBuffer[i] == apexChar[current] && current < 4);

					// possible success
					if( current == 4 )
					{
						// still need to check what's after it
						if( tempBuffer[i] <= '9' && tempBuffer[i] >= '0' )
						{
							// success, find end index
							start = i;
							end = i;
							do
							{								
								end++;
							}
							while( tempBuffer[ end ] <= '9' && tempBuffer[ end ] >= '0' );

							break;
						}
						else
						{
							// we found word apex, but it's not the unique name from compiled collision
							current = 0;
						}
					}
					else // we found word starting like apex, but it's not the unique name from compiled collision
					{
						current = 0;
					}
				}
				else
				{
					++i;
				}
			}

			if( start > 0 )
			{
				// Clear the current index bytes
				while( start < end )
				{
					tempBuffer[ start++ ] = 0;
				}
			}
		}

		m_dataCRC = Red::System::CalculateHash64( tempBuffer.Data(), token.m_dataSizeInMemory );
	}
	else
	{
		// for non apex, just take the CRC calculated when compiling collision
		m_dataCRC = token.m_diskCRC;
	}
}

CPatchPhysicsFileToken::~CPatchPhysicsFileToken()
{
}

const StringAnsi& CPatchPhysicsFileToken::GetFilePath() const
{
	return m_filePath;
}

const Uint64 CPatchPhysicsFileToken::GetTokenHash() const
{
	return m_tokenHash;
}

const Uint64 CPatchPhysicsFileToken::GetDataCRC() const
{
	RED_FATAL_ASSERT( m_dataCRC != 0, "Token used with invalid CRC" );	
	return m_dataCRC;
}

const Uint64 CPatchPhysicsFileToken::GetDataSize() const
{
	const CCollisionCacheData::CacheToken& token = m_data->m_tokens[ m_entryIndex ];
	return token.m_dataSizeOnDisk;
}

const String CPatchPhysicsFileToken::GetInfo() const
{
	return String::Printf( TXT("Collision cache entry '%hs'"), GetFilePath().AsChar() );
}

void CPatchPhysicsFileToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	String absoluePath = dumpPath;
	absoluePath += ANSI_TO_UNICODE( m_ownerCacheFile->GetEntryName( m_entryIndex ).AsChar() );
	absoluePath += isBase ? TXT(".base") : TXT(".current");
	
	TDynArray< Uint8 > loadBuffer;
	const CCollisionCacheData::CacheToken& token = m_data->m_tokens[ m_entryIndex ];

	m_ownerCacheFile->LoadEntryDataDecompressed( m_entryIndex, loadBuffer );
	IFile * destFile( GFileManager->CreateFileWriter( absoluePath.AsChar(), FOF_AbsolutePath ) );
	if ( loadBuffer.Size() && destFile != nullptr )
	{
		const Uint64 decCRC = Red::System::CalculateHash64( loadBuffer.Data(), token.m_dataSizeInMemory );
		LOG_WCC( TXT("Dumping %d bytes - CRC [%llu] - decompressed CRC [%llu] - to '%ls'"), loadBuffer.Size(), token.m_diskCRC, decCRC , absoluePath.AsChar() );
		destFile->Serialize( loadBuffer.Data(), token.m_dataSizeInMemory );
	}
}

//-----------------------------------------------------------------------------

CPatchBuilder_Physics::CPatchBuilder_Physics()
{
}

CPatchBuilder_Physics::~CPatchBuilder_Physics()
{
}

String CPatchBuilder_Physics::GetContentType() const
{
	return TXT("physics");
}

CPatchBuilder_Physics::IContentGroup* CPatchBuilder_Physics::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	return CPatchPhysics::LoadCollisionCaches( absoluteBuildPath );
}

Bool CPatchBuilder_Physics::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName  )
{
	// Create merged cache with all of the tokens
	if ( !patchContent.Empty() )
	{
		// save the collision cache
		const String outputFilePath = absoluteBuildPath + patchName + TXT("\\collision.cache");

		// open output file
		IFile* outputFile = GFileManager->CreateFileWriter( outputFilePath, FOF_Buffered | FOF_AbsolutePath );
		if ( !outputFile )
		{
			ERR_WCC( TXT("Failed to create output file '%ls'"), outputFilePath.AsChar() );
			return false;
		}

		// preallocate read buffer
		TDynArray< Uint8 > readBuffer;
		readBuffer.Resize( 1 << 20 );

		Red::System::DateTime timeStamp;
		Red::System::Clock::GetInstance().GetLocalTime( timeStamp );
		CCollisionCacheData::WriteHeader( *outputFile, timeStamp);

		// skip to start of first data block
		const Uint32 writePos = sizeof( CCollisionCacheData::RawHeader ) + sizeof( CCollisionCacheData::IndexHeader );
		outputFile->Seek( writePos );

		// transfer data from entries
		CCollisionCacheData outData;
		CCollisionCacheDataBuilder outDataBuilder( outData );

		// add entries
		for ( Uint32 i=0; i<patchContent.Size(); ++i )
		{
			CPatchPhysicsFileToken* token = static_cast< CPatchPhysicsFileToken* >( patchContent[i] );
			const Uint32 tokenIndex = token->m_entryIndex;

			// make sure reading buffer is large enough
			const CCollisionCacheData::CacheToken& inToken = token->m_data->m_tokens[ tokenIndex ];

			if( !token->m_ownerCacheFile->LoadEntryData(tokenIndex, readBuffer ) )
			{
				continue;
			}

			// write data to output file
			const Uint32 writePos = (Uint32) outputFile->GetOffset();
			outputFile->Serialize( readBuffer.Data(), inToken.m_dataSizeOnDisk );

			// create new token in output file
			CCollisionCacheData::CacheToken outToken = inToken;
			outToken.m_name = outDataBuilder.AddString( &(token->m_data->m_strings[ inToken.m_name ]) );
			outToken.m_dataOffset = writePos;
			outDataBuilder.AddToken( outToken );
		}

		// save the new tables
		const Uint32 endOfFilePos = (Uint32) outputFile->GetOffset();
		outData.Save( *outputFile, endOfFilePos );

		// close output file
		delete outputFile;
	}

	// Good news everyone! Collision.cache for patch ready.
	return true;
}