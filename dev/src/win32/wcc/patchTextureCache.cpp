/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "patchTextureCache.h"
#include "patchUtils.h"

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_TextureCache );


//////////////////////////////////////////////////////////////////////////


CPatchTextureCacheEntryToken::CPatchTextureCacheEntryToken( Uint32 index, CTextureCacheData& data )
	: m_data( data )
	, m_entryIndex( index )
	, m_dataCRC( 0 )
{
}

const Uint64 CPatchTextureCacheEntryToken::GetTokenHash() const
{
	return m_data.GetEntryHash( m_entryIndex );
}

const Uint64 CPatchTextureCacheEntryToken::GetDataCRC() const
{
	RED_FATAL_ASSERT( m_dataCRC != 0, "Token used with invalid CRC" );
	return m_dataCRC;
}

const Uint64 CPatchTextureCacheEntryToken::GetDataSize() const
{
	return m_data.GetEntryDiskSizeUnpadded( m_entryIndex );
}

const String CPatchTextureCacheEntryToken::GetInfo() const
{
	return m_data.GetEntryName( m_entryIndex );
}

void CPatchTextureCacheEntryToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	String absoluePath = dumpPath;
	absoluePath += m_data.GetEntryName( m_entryIndex );
	absoluePath += isBase ? TXT(".base") : TXT(".current");

	Red::TScopedPtr< IFile > srcFile( m_data.ResumeEntryData( m_entryIndex ) );
	Red::TScopedPtr< IFile > destFile( GFileManager->CreateFileWriter( absoluePath.AsChar(), FOF_AbsolutePath ) );
	if ( srcFile != nullptr && destFile != nullptr )
	{
		Uint8 copyBlock[ 64*1024 ];

		const Uint64 fileSize = srcFile->GetSize();
		LOG_WCC( TXT("Dumping %d bytes to '%ls'"), fileSize, absoluePath.AsChar() );

		while ( srcFile->GetOffset() < fileSize )
		{
			const Uint64 maxRead = Min< Uint64 >( sizeof(copyBlock), fileSize - srcFile->GetOffset() );
			srcFile->Serialize( copyBlock, maxRead );
			destFile->Serialize( copyBlock, maxRead );
		}
	}
}

Bool CPatchTextureCacheEntryToken::CalculateCRC()
{
	Red::TScopedPtr< IFile > file( m_data.ResumeEntryData( m_entryIndex ) );
	if ( file == nullptr )
	{
		return false;
	}

	Uint32 sizeOnDisk = m_data.GetEntryDiskSizeUnpadded( m_entryIndex );

	m_dataCRC = PatchUtils::CalcFileBlockCRC( file.Get(), sizeOnDisk, RED_FNV_OFFSET_BASIS64 );

	return true;
}


//////////////////////////////////////////////////////////////////////////


CPatchTextureCache::CPatchTextureCache()
{
}

CPatchTextureCache::~CPatchTextureCache()
{
	m_tokens.ClearPtr();
	m_textureCaches.ClearPtr();
}


void CPatchTextureCache::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	outTokens.Reserve( m_tokens.Size() );
	for ( auto ptr : m_tokens )
	{
		outTokens.PushBack( ptr );
	}
}

const Uint64 CPatchTextureCache::GetDataSize() const
{
	Uint64 totalSize = 0;
	for ( auto ptr : m_tokens )
	{
		totalSize += ptr->GetDataSize();
	}
	return totalSize;
}

const String CPatchTextureCache::GetInfo() const
{
	return TXT("Texture Cache");
}

extern Bool GPatchingMod;

CPatchTextureCache* CPatchTextureCache::LoadTextureCaches( const String& baseDirectory )
{
	TDynArray< String > texCachePaths;
	GFileManager->FindFiles( baseDirectory, TXT("texture.cache"), texCachePaths, true );

	CPatchTextureCache* newPatch = new CPatchTextureCache();

	THashMap<Uint64,CPatchTextureCacheEntryToken*> foundTokens;

	LOG_WCC( TXT("Found %u texture cache files in build"), texCachePaths.Size() );
	for ( const String& path : texCachePaths )
	{
		// skip non content files
		if ( !PatchUtils::IsContentOrDlcPath( path ) && !GPatchingMod )
		{
			LOG_WCC( TXT("File '%ls' will be skipped because it's not part of the main content or dlc"), path.AsChar() );
			continue;
		}

		LOG_WCC( TXT("Loading texture cache '%ls'..."), path.AsChar() );

		CTextureCacheData* cacheData = new CTextureCacheData();
		newPatch->m_textureCaches.PushBack( cacheData );

		if ( !cacheData->LoadFromFile( path ) )
		{
			ERR_WCC( TXT("Failed to load texture cache '%ls'"), path.AsChar() );
			delete newPatch;
			return nullptr;
		}


		const Uint32 numEntries = cacheData->GetNumEntries();
		LOG_WCC( TXT("Found %u entries in texture cache"), numEntries );

		for ( Uint32 i = 0; i < numEntries; ++i )
		{
			CPatchTextureCacheEntryToken* newToken = new CPatchTextureCacheEntryToken( i, *cacheData );

			const Uint64 tokenHash = newToken->GetTokenHash();
			if ( !foundTokens.Insert( tokenHash, newToken ) )
			{
				// Not a new token hash. If the token names match, then everything's find and we can just ignore this one.
				// If the names are different, then we've got problems.
				CPatchTextureCacheEntryToken* existingToken = nullptr;
				RED_VERIFY( foundTokens.Find( tokenHash, existingToken ) );
				if ( existingToken != nullptr && existingToken->GetEntryName() != newToken->GetEntryName() )
				{
					ERR_WCC( TXT("Duplicate token hash found 0x%016llX for entries '%ls', '%ls'"), tokenHash, newToken->GetEntryName(), existingToken->GetEntryName() );
					delete newPatch;
					return nullptr;
				}

				// Just delete the duplicate token and keep going.
				LOG_WCC( TXT("Duplicate token '%ls' found in '%ls'. First seen in '%ls'. Skipping."), newToken->GetEntryName().AsChar(), cacheData->GetSourceFilePath().AsChar(), existingToken->GetSourceData().GetSourceFilePath().AsChar() );
				delete newToken;
				continue;
			}

			newPatch->m_tokens.PushBack( newToken );
		}
	}


	LOG_WCC( TXT("Found %u total entries in all texture caches"), newPatch->m_tokens.Size() );

	LOG_WCC( TXT("Calculating token CRCs...") );
	Uint32 lastPrc = -1;
	const Uint32 numTokens = newPatch->m_tokens.Size();
	for ( Uint32 i = 0; i < numTokens; ++i )
	{
		// refresh task progress
		const Uint32 prc = ( 100 * i ) / numTokens;
		if ( prc != lastPrc )
		{
			LOG_WCC( TXT("Status: Calculating CRC... %u%%..."), prc );
			lastPrc = prc;
		}

		CPatchTextureCacheEntryToken* token = newPatch->m_tokens[ i ];
		if ( !token->CalculateCRC() )
		{
			ERR_WCC( TXT("Failed to calculate CRC for entry '%ls'"), token->GetEntryName().AsChar() );
			delete newPatch;
			return nullptr;
		}
	}

	LOG_WCC( TXT("Status: Done loading caches") );

	return newPatch;
}


//////////////////////////////////////////////////////////////////////////


CPatchBuilder_TextureCache::CPatchBuilder_TextureCache()
{
}

CPatchBuilder_TextureCache::~CPatchBuilder_TextureCache()
{
	m_loadedContent.ClearPtr();
}


String CPatchBuilder_TextureCache::GetContentType() const
{
	return TXT("textures");
}

CPatchBuilder_TextureCache::IContentGroup* CPatchBuilder_TextureCache::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	CPatchTextureCache* content = CPatchTextureCache::LoadTextureCaches( absoluteBuildPath );
	m_loadedContent.PushBack( content );
	return content;
}

Bool CPatchBuilder_TextureCache::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	const String outputFilePath = absoluteBuildPath + patchName + TXT("\\texture.cache");
	CTextureCacheDataSaver saver( outputFilePath );
	if ( !saver )
	{
		return false;
	}

	const Uint32 numTokens = patchContent.Size();

	LOG_WCC( TXT("Writing %u total entries to %ls"), numTokens, outputFilePath.AsChar() );

	Uint32 lastPrc = -1;
	for ( Uint32 i = 0; i < numTokens; ++i )
	{
		// refresh task progress
		const Uint32 prc = ( 100 * i ) / numTokens;
		if ( prc != lastPrc )
		{
			LOG_WCC( TXT("Status: Writing new patch texture.cache... %u%%..."), prc );
			lastPrc = prc;
		}

		const IContentToken* token = patchContent[ i ];

		const CPatchTextureCacheEntryToken* entryToken = static_cast< const CPatchTextureCacheEntryToken* >( token );
		if ( !saver.SaveEntry( entryToken->GetEntryIndex(), entryToken->GetSourceData() ) )
		{
			ERR_WCC( TXT("TextureCache Patch Builder failed while saving entries") );
			return false;
		}
	}

	return true;
}
