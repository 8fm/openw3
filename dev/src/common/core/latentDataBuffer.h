/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//-----------------------
// DO NOT USE THOSE CLASSES - deprecated
//-----------------------

class CJobLoadData;
class IFileLatentLoadingToken;
class CObject;

/// Generic buffer for resource data
class LegacyDataBuffer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	typedef Uint32 TSize; // Size is serialized to file. Don't change this to size_t etc without a massive resave.

protected:
	TSize										m_dataSize;			//!< Size of the data to load.
	void*										m_dataHandle;		//!< Handle to data
	Red::MemoryFramework::MemoryClass			m_memoryClass;		//!< Latent data memory class
	TSize										m_memoryAlignment;	//!< Memory alignment
	Red::Threads::CAtomic< Int32 >*				m_lock;				//!< For CopyHandle operation

public:
	RED_INLINE TSize GetSize() const { return m_dataSize; }
	RED_INLINE void* GetData() const { return m_dataHandle; }
	RED_INLINE void* GetHandle() const { return m_dataHandle; }
	RED_INLINE TSize GetInternalMemSize() const { return m_dataSize; }
	RED_INLINE Red::MemoryFramework::MemoryClass GetMemoryClass() const { return m_memoryClass; }

public:
	LegacyDataBuffer();
	LegacyDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, TSize size = 0, TSize alignment = 16 );
	LegacyDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, const void* data, TSize size, TSize alignment = 16 );
	LegacyDataBuffer( const LegacyDataBuffer& other );
	virtual ~LegacyDataBuffer();

	LegacyDataBuffer& operator=( const LegacyDataBuffer& other );
	void CopyHandle( LegacyDataBuffer& other );
	void MoveHandle( LegacyDataBuffer& other );
	virtual Bool ReallocateMemory( TSize size, Red::MemoryFramework::MemoryClass newMemoryClass, TSize alignment = 16 );

	virtual void Clear();
	virtual void Serialize( IFile& file );
	virtual void Serialize( const void* data );
	virtual void Allocate( TSize size );
	virtual Bool Load();
	virtual Bool Unload();
	virtual void Unlink();
};

/// Data buffer for latent data
// Only supports files of up to 4gb
class LatentDataBuffer : public LegacyDataBuffer
{
protected:
	IFileLatentLoadingToken*	m_loadingToken;			//!< Pointer to data in a source file

public:
	RED_INLINE IFileLatentLoadingToken* GetToken() const { return m_loadingToken; }

public:
	LatentDataBuffer();
	LatentDataBuffer( Red::MemoryFramework::MemoryClass memoryClass );
	LatentDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, const void* data, TSize size );
	LatentDataBuffer( Red::MemoryFramework::MemoryClass memoryClass, TSize size );
	LatentDataBuffer( const LatentDataBuffer& other );
	virtual ~LatentDataBuffer();

	void MoveHandle( LatentDataBuffer& other );

	virtual Bool Load() override;
	virtual Bool Unload() override;
	virtual void Unlink() override;
	virtual void Serialize( IFile& file, Bool allowStreaming, Bool preload );

	LatentDataBuffer& operator=( const LatentDataBuffer& other );


protected:
	virtual void Serialize( IFile& file );
	virtual void Serialize( const void* data );
};

