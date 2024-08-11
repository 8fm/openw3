#include "build.h"
#include "meshCacheCooker.h"
#include "../core/dependencyMapper.h"

//#include "../../../external/dexzip/dzip.h"

MeshCacheCooker* GMeshCacheCooker = NULL;
MeshCacheCooker* GMeshCacheCookerLow = NULL;

MeshCacheCooker::MeshCacheCooker( const String& absoluteFileName, ECookingPlatform platform )
	: m_file( NULL )
	, m_cacheStartOffset( 0 )
	, m_headerDataOffset( 0 )
{
	if ( GFileManager->GetFileSize( absoluteFileName ) == 0 || !LoadFromFile( absoluteFileName ) )
	{
		m_file = GFileManager->CreateFileWriter( absoluteFileName, FOF_AbsolutePath );
		if ( !m_file )
		{
			WARN_ENGINE( TXT("Unable to create anim cache file '%ls'"), absoluteFileName.AsChar() );
			return;
		}	

		CreateFileHeader( platform );
	}
	else
	{
		ASSERT( m_file );
	}
}

MeshCacheCooker::~MeshCacheCooker()
{
	m_entriesList.Clear();

	CloseFile();
}

void MeshCacheCooker::CreateFileHeader( ECookingPlatform platform )
{
	// Set cooking flags
	if ( platform != PLATFORM_None )
	{
		m_file->SetCooker( true );
	}

	// Save header
	Uint32 magic = MESH_CACHE_MAGIC_HEADER;
	*m_file << magic;

	// Save entires num and entires offset
	m_headerDataOffset = static_cast< Uint32 >( m_file->GetOffset() );
	Uint32 temp = 0;
	*m_file << temp;
	*m_file << temp;

	// Save offset
	m_cacheStartOffset = static_cast< Uint32 >( m_file->GetOffset() );
}

void MeshCacheCooker::CloseFile()
{
	if ( m_file )
	{
		delete m_file;
		m_file = NULL;
	}
}

void MeshCacheCooker::AddMesh( Int32& index, DataBuffer* deviceBuffer, String fileName )
{
	Uint64 defineHash = HASH64_BASE;
	Uint64 hash = ACalcBufferHash64Merge( fileName, defineHash );

	Uint32 count = m_entriesList.Size(); 
	for( Uint32 i = 0; i != count; ++i )
	{
		MeshCacheEntry& entry = m_entriesList[ i ];
		if( entry.m_hash == hash )
		{
			//already created
			index = i;
			return;
		}
	}

	//deviceBuffer->Load();
	void* deviceBufferData = deviceBuffer->GetData();
	if( !deviceBufferData ) 
	{
		return;
	}

//	void* compressedDeviceBuffer = malloc( deviceBuffer->GetSize() * 2 );

	index = static_cast< Int32 >( m_entriesList.Grow( 1 ) );
	MeshCacheEntry& entry = m_entriesList[ index ];
	entry.m_hash = hash;
	entry.m_deviceBufferOffset = static_cast< Uint32 >( m_file->GetOffset() );
	
//	int compressedSize = fastlz_compress_level( 2, deviceBufferData, deviceBuffer->GetSize(), compressedDeviceBuffer );

/*	if( (float)compressedSize / (float)deviceBuffer->GetSize() < 0.666f )
	{
		entry.m_deviceBufferUncompressedSize = deviceBuffer->GetSize();
		m_file->Serialize( compressedDeviceBuffer, compressedSize );
		LOG_ENGINE( TXT( "MESH COMPRESSION %ls %i>>%i %f" ), fileName.AsChar(), deviceBuffer->GetSize(), compressedSize, (float)compressedSize / (float)deviceBuffer->GetSize() );
	}
	else*/
	{
		m_file->Serialize( deviceBufferData, deviceBuffer->GetSize() );
	}

	//deviceBuffer->Unlink();

//	free( compressedDeviceBuffer );
}


Uint32 MeshCacheCooker::SaveEntries()
{
	ASSERT( m_file );

	Uint32 start = static_cast< Uint32 >( m_file->GetOffset() );

	for ( Uint32 i=0; i<m_entriesList.Size(); ++i )
	{
		MeshCacheEntry& entry = m_entriesList[ i ];
		entry.OnSerialize( *m_file );
	}

	Uint32 end = static_cast< Uint32 >( m_file->GetOffset() );

	return end - start;
}

void MeshCacheCooker::Save()
{
	ASSERT( m_file );

	Uint32 currentFileOffset = static_cast< Uint32 >( m_file->GetOffset() );

	// Animation data size
	Uint32 animDataSize = currentFileOffset - m_cacheStartOffset;

	// All animation were saved. Entries will be saved now.
	Uint32 entriesOffset = currentFileOffset;

	// Save magic value - separator
	Uint32 magicSep = MESH_CACHE_MAGIC_SEP;
	*m_file << magicSep;

	Uint32 toContinue = static_cast< Uint32 >( m_file->GetOffset() );

	// Go to header data
	m_file->Seek( m_headerDataOffset );

	// Save entires num and entires offset
	Uint32 entriesNum = m_entriesList.Size();

	*m_file << entriesNum;
	*m_file << entriesOffset;

	// Next, save entires
	m_file->Seek( toContinue );
	Uint32 entriesDataSize = SaveEntries();

	// Footer
	Uint32 magicNum = MESH_CACHE_MAGIC_FOOTER;
	*m_file << magicNum;

	// Stats
	const Uint32 dataSize = static_cast< Uint32 >( m_file->GetSize() );
	Float inv = 1.f / ( 1024.0f * 1024.f );
	LOG_ENGINE( TXT("Saved %i entries, file size %1.2f MB, anim entires ( %1.2f MB ), anim data ( %1.2f MB ) in anim cache"), 
		entriesNum, dataSize * inv, entriesDataSize * inv, animDataSize * inv );

	// Return to proper place
	m_file->Seek( currentFileOffset );
}

Bool MeshCacheCooker::LoadFromFile( const String& absoluteFileName )
{
	// Try to load existing file
	IFile* file = GFileManager->CreateFileReader( absoluteFileName, FOF_AbsolutePath | FOF_Buffered );
	if ( !file )
	{
		return false;
	}

	// Check magic value - header
	Uint32 magic = 0;
	*file << magic;

	if ( magic == MESH_CACHE_MAGIC_HEADER_BS )
	{
		// Restart file, this time with byte swapping enabled
		file->Seek( 0 );
		file->SetByteSwapping( true );

		// Read the magic once more
		*file << magic;
	}

	if ( magic != MESH_CACHE_MAGIC_HEADER )
	{
		WARN_ENGINE( TXT("Anim cache cooker file is corrupted") );
		delete file;
		return false;
	}

	// Save header offset
	m_headerDataOffset = static_cast< Uint32 >( file->GetOffset() );

	// Load number of entries
	Uint32 numEntries = 0;
	*file << numEntries;

	// Resize entries array
	m_entriesList.Resize( numEntries );

	// Load entries offset
	Uint32 entriesOffset = 0;
	*file << entriesOffset;

	// Cache anim offset
	m_cacheStartOffset = static_cast< Uint32 >( file->GetOffset() );

	// Go to entries place
	file->Seek( entriesOffset );

	// Check magic value - separator
	*file << magic;
	if ( magic != MESH_CACHE_MAGIC_SEP )
	{
		WARN_ENGINE( TXT("Mesh cache cooker file is corrupted") );
		delete file;
		return false;
	}

	// Load entires
	for ( Uint32 i=0; i<numEntries; ++i )
	{
		MeshCacheEntry& entry = m_entriesList[ i ];

		entry.OnSerialize( *file );
	}

	// Check magic value - footer
	*file << magic;
	if ( magic != MESH_CACHE_MAGIC_FOOTER )
	{
		WARN_ENGINE( TXT("Mesh cache cooker file is corrupted") );
		delete file;
		return false;
	}

	// Delete file reader
	delete file;
	file = NULL;

	// Create file writer
	m_file = GFileManager->CreateFileWriter( absoluteFileName, FOF_AbsolutePath | FOF_Append );
	if ( !m_file )
	{
		ASSERT( !m_file );
		return false;
	}

	// Seek to last animation place
	m_file->Seek( entriesOffset );

	return true;
}
