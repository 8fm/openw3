#include "build.h"
#include "meshCache.h"
#include "../core/loadingJobManager.h"
#include "../core/latentDataBuffer.h"

MeshCache* GMeshCache = 0;

class CMeshCacheInitializationJob : public ILoadJob
{
	friend class MeshCache;

private:
	IFile**						m_file;
	TDynArray< MeshCacheEntry > m_entries;
	Uint32						m_entriesOffset;
	Uint32						m_offset;
	String						m_filePath;

public:
	CMeshCacheInitializationJob( IFile** fileHandle, String filePath ) : ILoadJob( JP_Immediate ), m_file( fileHandle ), m_entriesOffset( 0 ), m_offset( 0 ), m_filePath( filePath ) {}
	virtual ~CMeshCacheInitializationJob() {}

	virtual const Char* GetDebugName() const override { return TXT("MeshCache"); }

protected:
	//! Process the job
	virtual EJobResult Process()
	{
		CTimeCounter timer;

		// Open file
		*m_file = GFileManager->CreateFileReader( m_filePath, FOF_AbsolutePath | FOF_Buffered );
		IFile* file = *m_file;
		
		if ( !file )
		{
			WARN_ENGINE( TXT("Unable to load mesh cache from '%ls'"), m_filePath.AsChar() );
			return JR_Failed;
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
			WARN_ENGINE( TXT("Unable to load anim cache") );
			return JR_Failed;
		}

		// Load number of entries
		Uint32 numEntries = 0;
		*file << numEntries;

		// Resize entries array
		m_entries.Resize( numEntries );

		// Load entries offset
		*file << m_entriesOffset;

		// Cache mesh offset
		m_offset = static_cast< Uint32>( file->GetOffset() );

		// Go to entries place
		file->Seek( m_entriesOffset );

		// Check magic value - separator
		*file << magic;
		if ( magic != MESH_CACHE_MAGIC_SEP )
		{
			WARN_ENGINE( TXT("Anim cache is corrupted") );
			return JR_Failed;
		}

		// Load entires
		for ( Uint32 i=0; i<numEntries; ++i )
		{
			MeshCacheEntry& entry = m_entries[ i ];
			entry.OnSerialize( *file );
		}

		// Data sizes
		const Uint32 dataSize = static_cast< Uint32 >( file->GetSize() );
		Uint32 meshDataSize = dataSize - m_offset - sizeof( Uint32 ); // - footer

		// Check magic value - footer
		*file << magic;
		if ( magic != MESH_CACHE_MAGIC_FOOTER )
		{
			WARN_ENGINE( TXT("Anim cache is corrupted") );
			return JR_Failed;
		}

		Float period = timer.GetTimePeriod();
		// Stats
		Float inv = 1.f / ( 1024.0f * 1024.f );
		LOG_ENGINE( TXT("Loaded %i entries, file size %1.2f MB, mesh entires ( %1.2f MB ), mesh data ( %1.2f MB ) in mesh cache in %1.2fs "), 
			m_entries.Size(), dataSize * inv, (m_entriesOffset-m_offset) * inv, meshDataSize * inv, period );

		return JR_Finished;
	}
};

void MeshCache::Initialize( const String& absoluteFileName )
{
	if( m_initializationJob ) return;

	if ( absoluteFileName.ToLower().ContainsSubstring( TXT("lod") ) )
	{
		m_isLowMeshCache = true;
	}

	m_entriesOffset = 0;
	m_file = NULL;
	m_offset = 0;
	m_entries.Clear();

	m_initializationJob = new CMeshCacheInitializationJob( &m_file, absoluteFileName );
	SJobManager::GetInstance().Issue( m_initializationJob );
}

Bool MeshCache::IsInitialized()
{
	if( m_initializationJob )
	{
		if( !m_initializationJob->HasEnded() ) return false;

		m_entriesOffset = m_initializationJob->m_entriesOffset;
		m_offset = m_initializationJob->m_offset;
		m_entries = m_initializationJob->m_entries;

		m_initializationJob->Release();
		m_initializationJob = 0;
	}

	return m_file != 0;
}

void MeshCache::Destroy()
{
	// Close file
	delete m_file;
	m_file = NULL;

	// Reset offset
	m_offset = 0;

	// Delete all entries
	m_entries.Clear();
}

void MeshCache::FillDeviceBufferLoadingToken( LatentDataBuffer& latendDataBuffer, Int32 id )
{
	// Open only valid entries
	if ( id >= 0 && id < (Int32)m_entries.Size() )
	{
		const MeshCacheEntry& entry = m_entries[ id ];

		Uint32 nextOffset = m_entriesOffset;
		if( id < ( Int32 )m_entries.Size() - 1 )
		{
			nextOffset = m_entries[ id + 1 ].m_deviceBufferOffset;
		}

		// Return loading token
		//latendDataBuffer.PushToken( new CMeshCacheLoadingToken( m_file, entry.m_deviceBufferOffset ), nextOffset - entry.m_deviceBufferOffset );
	}
}


Int32 MeshCache::GetBufferUncompressedSize( Int32 id )
{
	if ( id < 0 || id >= (Int32)m_entries.Size() )
	{
		return -1;
	}
	MeshCacheEntry& entry = m_entries[ id ];
	return entry.m_deviceBufferUncompressedSize;
}