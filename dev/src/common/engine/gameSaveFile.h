/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/file.h"

/// Game save file writer (legacy)
class CGameStorageWriter : public ISaveFile
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_GameSave, MC_Gameplay );

private:
	typedef ISaveFile TBaseClass;

protected:
	IFileEx*							m_writer;

private:
	THashMap< ISerializable*, Uint32 >	m_objectsIndexMap;

public:
	CGameStorageWriter( IFileEx* writer, CNamesRemapper& reporter, Uint32 extraFileFlags = 0 );

	virtual ~CGameStorageWriter()
	{
		delete m_writer;
	}

public:
	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size ) { m_writer->Serialize( buffer, size ); };

	// Get position in file stream
	virtual Uint64 GetOffset() const { return m_writer->GetOffset(); };

	// Get size of the file stream
	virtual Uint64 GetSize() const { return m_writer->GetSize(); }

	// Seek to file position
	virtual void Seek( Int64 offset ) { m_writer->Seek( offset ); }

	// Save file to HDD
	virtual void Close() { m_writer->Close(); }

	// Get data buffer
	virtual const void* GetBuffer() const { return m_writer->GetBuffer(); }

	// Get the buffer allocation size
	virtual size_t GetBufferCapacity() const { return m_writer->GetBufferCapacity(); }

public:
	// Pointer serialization
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer );
};
	
/// Game save file reader (legacy)
class CGameStorageReader : public ISaveFile
{
private:
	typedef ISaveFile TBaseClass;

private:
	IFile*								m_reader;
	TDynArray< CObject* >				m_parents;
	THashMap< Uint32, ISerializable* >	m_indexObjectsMap;

public:
	CGameStorageReader( IFile* reader, CNamesRemapper& reporter, Uint32 additionalFlags = 0 );

	virtual ~CGameStorageReader()
	{
		delete m_reader;
	}

public:
	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size ) { m_reader->Serialize( buffer, size ); };

	// Get position in file stream
	virtual Uint64 GetOffset() const { return m_reader->GetOffset(); };

	// Get size of the file stream
	virtual Uint64 GetSize() const { return m_reader->GetSize(); }

	// Seek to file position
	virtual void Seek( Int64 offset ) { m_reader->Seek( offset ); }

public:
	// Pointer serialization
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer );

private:
	void SetDefaultParent( CObject* obj )
	{
		ASSERT( m_parents.Size() == 0 );
		m_parents.PushBack( obj );		
	}

	void ClearDefaultParent()
	{
		ASSERT( m_parents.Size() == 1 );
		m_parents.Clear();
	}

	void LoadObject(BaseSafeHandle* handle, CObject*& obj);

public:
	class ScopedDefault
	{
		CGameStorageReader* m_reader;

	public:
		ScopedDefault( CGameStorageReader* reader, CObject* defaultParent )
			: m_reader( reader )
		{
			m_reader->SetDefaultParent( defaultParent );
		}

		~ScopedDefault()
		{
			m_reader->ClearDefaultParent();
		}
	};

public:
	// IFileEx interface, this shouldn't be used in reader
	virtual void Close() { HALT("dont use IFileEx in loader"); }
	virtual const void* GetBuffer() const { HALT("dont use IFileEx in loader"); return nullptr; }
	virtual size_t GetBufferCapacity() const { HALT("dont use IFileEx in loader"); return 0; }
};

// Game-save file writer with skip-block cache written to end of stream
class CGameSaveFileWriterNoInlineSkip : public CGameStorageWriter, public IFileSkipBlockCache
{
public:
	CGameSaveFileWriterNoInlineSkip( IFileEx* writer, CNamesRemapper& reporter, Uint32 skipBlockCacheStartSize = 1024 * 16 );
	virtual ~CGameSaveFileWriterNoInlineSkip();

	// Seeks are disabled
	virtual void Seek( Int64 offset ) { RED_FATAL_ASSERT( false, "No seeking allowed in save-game-writer" ); }
	virtual void Close();

	// Skip-block data is cached internally.
	virtual Uint32 QuerySkippableBlockOffset( Uint32 blockStartOffset );
	virtual void RegisterSkippableBlockOffset( Uint32 blockStartOffset, Uint32 blockSkipOffset );
	virtual IFileSkipBlockCache* QuerySkippableBlockCache() { return this; }

private:
	struct SkipBlockData
	{
		Uint32 m_blockStartOffset;
		Uint32 m_blockSkipOffset;
	};
	TDynArray< SkipBlockData > m_skipBlockCache;
};

// Game-save file reader with skip-block cache written at end of stream
class CGameSaveFileReaderNoInlineSkip : public CGameStorageReader, public IFileSkipBlockCache
{
public:
	CGameSaveFileReaderNoInlineSkip( IFile* reader, CNamesRemapper& reporter );
	virtual ~CGameSaveFileReaderNoInlineSkip();

	// Skip block queries
	virtual Uint32 QuerySkippableBlockOffset( Uint32 blockStartOffset );
	virtual void RegisterSkippableBlockOffset( Uint32 blockStartOffset, Uint32 blockSkipOffset );
	virtual IFileSkipBlockCache* QuerySkippableBlockCache() { return this; }

	// Get size of the file stream
	virtual Uint64 GetSize() const { return m_endMetadataOffset; }		// Take into account end-of-file metadata (i.e. hide footer data)

private:
	struct SkipBlockData
	{
		Uint32 m_blockStartOffset;
		Uint32 m_blockSkipOffset;
	};
	THashMap< Uint32, Uint32 > m_skipBlockOffsetLookup;
	Uint32 m_endMetadataOffset;		// Required to fudge the file-size minus metadata for skipblocks
};