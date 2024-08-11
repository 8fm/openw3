/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "baseCacheSplitter.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/collisionCache.h"
#include "../../common/engine/collisionCacheDataFormat.h"

///------

class CPhysicsCacheSplitter : public IBaseCacheSplitter
{
	DECLARE_RTTI_SIMPLE_CLASS( CPhysicsCacheSplitter );

public:
	CPhysicsCacheSplitter();
	~CPhysicsCacheSplitter();

	// interface
	virtual Bool Initialize( const ICommandlet::CommandletOptions& additonalOptions ) override;
	virtual Bool LoadInput( const String& absolutePath ) override;
	virtual void GetEntries( TDynArray< IBaseCacheEntry* >& allEntries ) const override;
	virtual Bool SaveOutput( const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries ) const override;

	// description
	virtual const Char* GetName() const override{ return TXT("physics"); }
	virtual const Char* GetDescription() const override { return TXT("Split collision data for physics"); }

private:
	IFile*						m_inputFile;
	CCollisionCacheData			m_data;
	Red::System::DateTime		m_originalTimeStamp;

};

BEGIN_CLASS_RTTI( CPhysicsCacheSplitter )
	PARENT_CLASS( IBaseCacheSplitter );
END_CLASS_RTTI()

// wrapper for a single entry in the physics cache
class CPhysicsCacheSplitEntry : public IBaseCacheEntry
{
public:
	CPhysicsCacheSplitEntry( const Uint32 entryIndex, const CCollisionCacheData* data )
		: m_entryIndex( entryIndex )
		, m_data( data )
	{
	}

	virtual String GetResourcePath() const override
	{
		const CCollisionCacheData::CacheToken& token = m_data->m_tokens[ m_entryIndex ];
		const AnsiChar* path = &m_data->m_strings[ token.m_name ];
		return ANSI_TO_UNICODE( path );
	}

	virtual Uint32 GetApproxSize() const override
	{
		const CCollisionCacheData::CacheToken& token = m_data->m_tokens[ m_entryIndex ];
		return token.m_dataSizeOnDisk;
	}

public:
	Uint32							m_entryIndex;
	const CCollisionCacheData*		m_data;
};

///------

IMPLEMENT_ENGINE_CLASS( CPhysicsCacheSplitter );

CPhysicsCacheSplitter::CPhysicsCacheSplitter()
{
}

CPhysicsCacheSplitter::~CPhysicsCacheSplitter()
{
	if ( m_inputFile )
	{
		delete m_inputFile;
		m_inputFile = NULL;
	}
}

Bool CPhysicsCacheSplitter::Initialize( const ICommandlet::CommandletOptions& additonalOptions )
{
	// no additional options in here
	return true;
}

Bool CPhysicsCacheSplitter::LoadInput( const String& absolutePath )
{
	// open file
	m_inputFile = GFileManager->CreateFileReader( absolutePath, FOF_AbsolutePath | FOF_Buffered );
	if ( !m_inputFile )
	{
		return false;
	}

	// load file header
	if ( !CCollisionCacheData::ValidateHeader( *m_inputFile, m_originalTimeStamp ) )
	{
		ERR_WCC( TXT("Specified file is NOT a collision cache") );
		return false;
	}

	// load the entries
	if ( !m_data.Load( *m_inputFile ) )
	{
		ERR_WCC( TXT("Specified to load tokens from specified collision cache file") );
		return false;
	}

	// valid
	return true;
}

void CPhysicsCacheSplitter::GetEntries( TDynArray< IBaseCacheEntry* >& allEntries ) const
{
	// create an entry wrapper for each cache entry
	for ( Uint32 i=0; i<m_data.m_tokens.Size(); ++i )
	{
		allEntries.PushBack( new CPhysicsCacheSplitEntry( i, &m_data ) );
	}
}

Bool CPhysicsCacheSplitter::SaveOutput( const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries ) const
{
	// open output file
	IFile* outputFile = GFileManager->CreateFileWriter( absolutePath, FOF_Buffered | FOF_AbsolutePath );
	if ( !outputFile )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), absolutePath.AsChar() );
		return false;
	}

	// preallocate read buffer
	TDynArray< Uint8 > readBuffer;
	readBuffer.Resize( 1 << 20 );

	// write initial header - preserve time stamp
	CCollisionCacheData::WriteHeader( *outputFile, m_originalTimeStamp );

	// skip to start of first data block
	const Uint32 writePos = sizeof( CCollisionCacheData::RawHeader ) + sizeof( CCollisionCacheData::IndexHeader );
	outputFile->Seek( writePos );

	// transfer data from entries
	CCollisionCacheData outData;
	CCollisionCacheDataBuilder outDataBuilder( outData );
	for ( Uint32 i=0; i<allEntries.Size(); ++i )
	{
		const Uint32 tokenIndex = static_cast< CPhysicsCacheSplitEntry* >( allEntries[i] )->m_entryIndex;

		// make sure reading buffer is large enough
		const CCollisionCacheData::CacheToken& inToken = m_data.m_tokens[ tokenIndex ];
		if ( inToken.m_dataSizeOnDisk > readBuffer.Size() )
			readBuffer.Resize( inToken.m_dataSizeOnDisk );

		// read source data from source file
		m_inputFile->Seek( inToken.m_dataOffset );
		m_inputFile->Serialize( readBuffer.Data(), inToken.m_dataSizeOnDisk );

		// write data to output file
		const Uint32 writePos = (Uint32) outputFile->GetOffset();
		outputFile->Serialize( readBuffer.Data(), inToken.m_dataSizeOnDisk );

		// create new token in output file
		CCollisionCacheData::CacheToken outToken = inToken;
		outToken.m_name = outDataBuilder.AddString( &m_data.m_strings[ inToken.m_name ] );
		outToken.m_dataOffset = writePos;
		outDataBuilder.AddToken( outToken );
	}

	// save the new tables
	const Uint32 endOfFilePos = (Uint32) outputFile->GetOffset();
	outData.Save( *outputFile, endOfFilePos );

	// close output file
	delete outputFile;
	return true;
}
