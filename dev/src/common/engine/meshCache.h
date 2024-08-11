#pragma once

#include "../core/fileLatentLoadingToken.h"

#define MESH_CACHE_MAGIC_HEADER		'MCMH'
#define MESH_CACHE_MAGIC_HEADER_BS	'MMCA'
#define MESH_CACHE_MAGIC_FOOTER		'MCMF'
#define MESH_CACHE_MAGIC_SEP		'MCMS'

//////////////////////////////////////////////////////////////////////////

class LatentDataBuffer;
class CMeshCacheTask;

struct MeshCacheEntry
{
	Uint64		m_hash;
	Uint32		m_deviceBufferOffset;
	Uint32		m_deviceBufferUncompressedSize;

	static const Uint32 INVALID = 0xFFFFFFFF;

	MeshCacheEntry()
		: m_hash( INVALID )
		, m_deviceBufferOffset( INVALID )
		, m_deviceBufferUncompressedSize( INVALID )
	{}

	void OnSerialize( IFile& file )
	{
		file << m_hash;
		file << m_deviceBufferOffset;
		file << m_deviceBufferUncompressedSize;
	}

	Bool IsValid() const
	{
		return m_deviceBufferOffset != INVALID && m_hash != 0 && m_deviceBufferUncompressedSize != INVALID;
	}
};

/// Mesh cache file wrapper
class CMeshCacheFileWrapper : public IFile
{
protected:
	IFile*		m_file;
	Uint64		m_baseOffset;

public:
	CMeshCacheFileWrapper( IFile* source, Uint64 offset )
		: IFile( FF_Reader | FF_Buffered | FF_FileBased )
		, m_file( source )
		, m_baseOffset( offset )
	{
		// Seek to the beginning
		m_file->Seek( m_baseOffset );
	};

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
		return m_file->Serialize( buffer, size );
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
		return m_file->GetOffset() - m_baseOffset;
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
		return m_file->GetSize();
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
		m_file->Seek( offset + m_baseOffset );
	}
};

//////////////////////////////////////////////////////////////////////////

class CMeshCacheLoadingToken : public IFileLatentLoadingToken
{
protected:
	IFile*			m_file;


public:
	CMeshCacheLoadingToken( IFile* file, Uint64 offset )
		: IFileLatentLoadingToken( offset )
		, m_file( file )
	{}

	//! Resume loading, returns valid IFile that can be used to continue file loading
	virtual IFile* Resume( Uint64 relativeOffset )
	{
		return new CMeshCacheFileWrapper( m_file, m_offset );
	}

	//! Clone token. Required as a token can be passed between threads.
	virtual IFileLatentLoadingToken* Clone() const
	{
		return new CMeshCacheLoadingToken( m_file, m_offset );
	}

	//! Describe loading token
	virtual String GetDebugInfo() const
	{
		return String::Printf( TXT("MeshCache, offset %") RED_PRIWu64, m_offset );
	}
};

//////////////////////////////////////////////////////////////////////////

class MeshCache
{
private:
	TDynArray< MeshCacheEntry >			m_entries;
	IFile*								m_file;
	Uint32								m_offset;
	Uint32								m_entriesOffset;
	Bool								m_isLowMeshCache;
	class CMeshCacheInitializationJob*	m_initializationJob;

	typedef THashMap< Int32, CMeshCacheTask* > IntToMeshCacheTaskTMap;
	IntToMeshCacheTaskTMap m_collectedMeshCacheTasksToProcess;

public:
	MeshCache() : m_file( 0 ), m_offset( 0 ), m_entriesOffset( 0 ), m_isLowMeshCache( false ), m_initializationJob( 0 ) {}

	void Initialize( const String& absoluteFileName );
	Bool IsInitialized();
	void Destroy();

	Bool IsLowMeshCache() const { return m_isLowMeshCache; }

	void FillDeviceBufferLoadingToken( LatentDataBuffer& latendDataBuffer, Int32 id );

	Int32 GetBufferUncompressedSize( Int32 id );

};

typedef TSingleton< MeshCache > SMeshCache;
