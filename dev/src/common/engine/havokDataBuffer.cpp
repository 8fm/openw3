/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#if defined(USE_HAVOK_ANIMATION) || defined(USE_HAVOK_DATA_IMPORT)

#include "havokDataBuffer.h"

Red::Threads::CMutex	HavokDataBuffer::m_accessMutex;

HavokDataBuffer::HavokDataBuffer( EMemoryClass memoryClass, Bool keepSourceData )
	: DataBuffer( memoryClass )
	, m_havokObject( NULL )
	, m_havokClass( NULL )
	, m_keepSourceData( keepSourceData )
	, m_sourceDataCopy( NULL )
	, m_sourceDataSize( NULL )
{}

HavokDataBuffer::~HavokDataBuffer()
{
	FreeSourceDataBuffer();
}

void HavokDataBuffer::FreeSourceDataBuffer()
{
	// Free copy
	if ( m_sourceDataCopy )
	{
		RED_MEMORY_FREE( MemoryPool_Default, m_memoryClass, m_sourceDataCopy );
		m_sourceDataCopy = NULL;
		m_sourceDataSize = 0;
	}
}

void HavokDataBuffer::Clear()
{
	// Clear base
	DataBuffer::Clear();

	// Free buffer
	FreeSourceDataBuffer();

	// Unlink shit
	m_havokObject = NULL;
	m_havokClass = NULL;
}

Bool HavokDataBuffer::Load()
{
	// Already loaded
	if ( m_havokObject )
	{
		ASSERT( m_dataSize );
		ASSERT( m_dataHandle );
		ASSERT( m_havokClass );
		return true;
	}

	// Load raw data
	if ( !DataBuffer::Load() )
	{
		return false;
	}

	// Deserialize
	if ( GetData() && GetSize() )
	{
		return CreateHavokDataObjects();
	}

	// Not loaded
	return false;
}

Bool HavokDataBuffer::Unload()
{
	// Unload raw data
	if ( DataBuffer::Unload() )
	{
		// Unlink shit
		m_havokObject = NULL;
		m_havokClass = NULL;

		// Free buffer
		FreeSourceDataBuffer();

		// Unloaded
		return true;
	}

	// Not unloaded
	return false;
}

void HavokDataBuffer::Unlink()
{
	// Unlink raw data
	DataBuffer::Unlink();
}

void HavokDataBuffer::Serialize( IFile& file )
{
	// GC
	if ( file.IsGarbageCollector() )
	{
		return;
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessMutex ); 

	// Loading
	if ( file.IsReader() )
	{
		// Release current copy buffer
		FreeSourceDataBuffer();

		// Load raw data
		DataBuffer::Serialize( file );

		// We have some data
		if ( GetData() )
		{			
			// Create copy of source data so we can create instances very fast
			if ( m_keepSourceData )
			{
				// Allocate copy buffer
				m_sourceDataSize = (Uint32)GetSize();
				m_sourceDataCopy = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, m_memoryClass, m_sourceDataSize, 16 );

				// Copy the data
				Red::System::MemoryCopy( m_sourceDataCopy, GetData(), m_sourceDataSize );
			}

			// Deserialize data buffer in place to create the havok object
			CreateHavokDataObjects();
		}
	}

	// Saving
#ifdef USE_HAVOK_ANIMATION
	if ( file.IsWriter() )
	{
		// Preload data
		Load();

		// Save only valid data
		if ( m_havokObject )
		{
			Uint32 sizeOffset = static_cast< Uint32 >( file.GetOffset() );

			// Save empty data size
			Uint32 dataSize = 0;
			file << dataSize;

			// Remember start of the data buffer
			Uint32 dataStart = static_cast< Uint32 >( file.GetOffset() );

			// Save data to file via IFile adapter
			hkBinaryPackfileWriter writer;
			hkPackfileWriter::Options options;
			options.m_writeMetaInfo = false;

			// If cooking change data layout
			if ( file.IsCooker() )
			{
				if( GCookingPlatform == PLATFORM_Xbox360 )
				{
					options.m_layout = hkStructureLayout::Xbox360LayoutRules;
				}
				else if( GCookingPlatform == PLATFORM_PS3 )
				{
					options.m_layout = hkStructureLayout::GccPs3LayoutRules;
				}
			}

			// Save
			CIFileHavokAdapter adapter( file );
			writer.setContents( m_havokObject, *m_havokClass );
			writer.save( &adapter, options );

			// Calculate data size
			Uint32 dataEnd = static_cast< Uint32 >( file.GetOffset() );
			dataSize = dataEnd - dataStart;

			// Write data size
			file.Seek( sizeOffset );
			file << dataSize;
			file.Seek( dataEnd );
		}
		else
		{
			// Save empty buffer
			Uint32 zeroSize = 0;
			file << zeroSize;
		}
	}
#endif
}
	
void HavokDataBuffer::Serialize( const void* data )
{
	// Load raw data
	DataBuffer::Serialize( data );

	// Create the havok data
	if ( GetData() )
	{
		CreateHavokDataObjects();
	}
}

/////////////////////////////////////////////////////////////////////////////////////
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4512 )
// Havok stream writer writing to TDynArray
class CDynArrayHavokStreamWriter2 : public hkStreamWriter
{
	TDynArray< Uint8 >		&m_array;
	Uint32					m_startOffset;
	Uint32					m_offset;

public:
	CDynArrayHavokStreamWriter2( TDynArray< Uint8 > &arr )
		: m_array( arr )
		, m_offset( arr.Size() )
		, m_startOffset( arr.Size() )
	{
	}

	hkBool isOk() const
	{
		return true;
	}

	int write( const void *buf, int nbytes )
	{
		Int32 growSize = nbytes - (m_array.Size() - m_offset);
		if ( growSize > 0 )
		{
			// temp: speedup for game... we shouldn't use write in game anyway...
			m_array.Resize( m_array.Size() + 10000 );

			m_array.CBaseArray::GrowBuffer( MC_DynArray, growSize, sizeof( Uint8 ) );
		}

		Red::System::MemoryCopy( m_array.TypedData() + m_offset, buf, nbytes);

		m_offset += nbytes;

		return nbytes;
	}

	void flush()
	{
		// ..
	}

	hkBool seekTellSupported() const
	{
		return true;
	}

	hkResult seek( int offset, hkStreamWriter::SeekWhence whence )
	{
		if ( whence == hkStreamWriter::STREAM_SET )
		{
			m_offset = offset + m_startOffset;
			return HK_SUCCESS;
		}
		else if ( whence == hkStreamWriter::STREAM_CUR )
		{
			m_offset += offset;
			return HK_SUCCESS;
		} 
		else if ( whence == hkStreamWriter::STREAM_END )
		{
			m_offset = m_array.Size() - offset;
			return HK_SUCCESS;
		}

		return HK_FAILURE;
	}

	int tell() const
	{
		return m_offset - m_startOffset;
	}
};
RED_WARNING_POP()
/////////////////////////////////////////////////////////////////////////////////////

/// Havok stream reading from raw memory
class CHavokMemoryReader : public hkStreamReader
{
private:
	const void*		m_memory;		//!< Memory buffer to read from
	Int32				m_offset;		//!< Current read offset in the memory buffer
	Int32				m_size;			//!< Size of the memory buffer

public:
	CHavokMemoryReader( const void* data, Uint32 size )
		: m_memory( data )
		, m_size( size )
		, m_offset( 0 )
	{};

	virtual hkBool isOk() const
	{
		return true;
	}

	virtual int read(void* buf, int nbytes)
	{
		Red::System::MemoryCopy( buf, OffsetPtr( m_memory, m_offset ), nbytes );
		m_offset += nbytes;
		return nbytes;
	}

	virtual int skip(int nbytes)
	{
		m_offset += nbytes;
		return nbytes;
	}

	virtual hkBool seekTellSupported() const
	{
		return true;
	}

	virtual hkResult seek(int offset, SeekWhence whence)
	{
		if ( whence == STREAM_SET )
		{
			m_offset = offset;
			return HK_SUCCESS;
		}
		else if ( whence == STREAM_END )
		{
			m_offset = m_size - offset;
			return HK_SUCCESS;
		}
		else if ( whence == STREAM_CUR )
		{
			m_offset += offset;
			return HK_SUCCESS;
		}

		return HK_FAILURE;
	}

	virtual int tell() const
	{
		return m_offset;
	}
};

/////////////////////////////////////////////////////////////////////////////////////

void HavokDataBuffer::CopyFromContainer( const void* containerToCopy, const hkClass* dataClass/*=NULL*/, Bool writeMetaInfo/*=true*/ )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("HAVOK"), TXT("Slow CopyFromContainer") );

	ASSERT( dataClass );
	m_havokClass = dataClass;

	// Initialize serializer
	hkBinaryPackfileWriter writer;
	hkPackfileWriter::Options options;
	options.m_writeMetaInfo = writeMetaInfo;

	// Serialize source data
	TDynArray< Uint8 > arrayData;
	CDynArrayHavokStreamWriter2 arrayStreamWriter( arrayData );
	writer.setContents( containerToCopy, *m_havokClass );
	writer.save( &arrayStreamWriter, options );
	ASSERT( arrayData.Size() );

	// Create local data
	Allocate( arrayData.Size() );
	Red::System::MemoryCopy( GetData(), arrayData.Data(), arrayData.Size() );

	// deserialize into local buffer
	hkBinaryPackfileReader reader;
	reader.loadEntireFileInplace( GetData(), (Uint32)GetSize() );

	// Restore data
	void* dataRead = reader.getContentsWithRegistry( m_havokClass->getName(), hkBuiltinTypeRegistry::getInstance().getLoadedObjectRegistry() );
	reader.getAllocatedData()->disableDestructors();

	// Get the object
	m_havokObject = static_cast< hkBaseObject* >( dataRead ); 
	ASSERT( dataRead );
}

Bool HavokDataBuffer::CreateHavokDataObjects()
{
	PC_CHECKER_SCOPE( 0.005f, TXT("HAVOK"), TXT("Slow CreateHavokDataObjects") );

	// Deserialize in place
	hkBinaryPackfileReader reader;
	reader.loadEntireFileInplace( GetData(), (Uint32)GetSize() );

	// Fixup data
	if ( !reader.isVersionUpToDate() )
	{
		WARN_ENGINE( TXT("Havok data is not in valid version") );
		return false;
	}

	// Get the data
	const char *className = reader.getContentsClassName();
	void* dataRead = reader.getContentsWithRegistry( className, hkBuiltinTypeRegistry::getInstance().getLoadedObjectRegistry() ); 
	reader.getAllocatedData()->disableDestructors();

	// No data was read
	if ( !dataRead )
	{
		WARN_ENGINE( TXT("Couldn't load havok data") );
		return false;
	}

	// Get the object
	m_havokObject = static_cast< hkBaseObject* >( dataRead );
	ASSERT( m_havokObject );

	// Get the class name
	m_havokClass = hkBuiltinTypeRegistry::getInstance().getClassNameRegistry()->getClassByName( className );
	ASSERT( m_havokClass );

	// Done
	return true;
}

HavokDataBuffer* HavokDataBuffer::CreateInstance() const
{
	PC_CHECKER_SCOPE( 0.002f, TXT("HAVOK"), TXT("Slow CreateInstance") );

	// We have source data, it's a fast way !
	if ( m_sourceDataCopy )
	{
		// Create in place buffer
		HavokDataBuffer* inplaceBuffer = new HavokDataBuffer( MC_HavokInstance, false );
		inplaceBuffer->Allocate( m_sourceDataSize );
		Red::System::MemoryCopy( inplaceBuffer->GetData(), m_sourceDataCopy, m_sourceDataSize );

		// Deserialize
		inplaceBuffer->CreateHavokDataObjects();
		return inplaceBuffer;
	}
	else
	{
		// No source data copy, use the slow instancing
		hkBinaryPackfileWriter writer;
		hkPackfileWriter::Options options;
		options.m_writeMetaInfo = true;

		// Serialize source data
		TDynArray< Uint8 > arrayData;
		CDynArrayHavokStreamWriter2 arrayStreamWriter( arrayData );
		writer.setContents( m_havokObject, *m_havokClass );
		writer.save( &arrayStreamWriter, options );
		ASSERT( arrayData.Size() );

		// Create in place buffer
		HavokDataBuffer* inplaceBuffer = new HavokDataBuffer( MC_HavokInstance, false );
		inplaceBuffer->Allocate( arrayData.Size() );
		Red::System::MemoryCopy( inplaceBuffer->GetData(), arrayData.Data(), arrayData.Size() );

		// Deserialize
		inplaceBuffer->CreateHavokDataObjects();
		return inplaceBuffer;
	}
}

void HavokDataBuffer::MoveHandleWithObject( HavokDataBuffer& other )
{
	if ( m_keepSourceData != other.m_keepSourceData || m_sourceDataCopy || other.m_sourceDataCopy )
	{
		// Not supported
		GPUAPI_LOG_WARNING"HavokDataBuffer::MoveHandleWithObject Error") );
	}

	if ( m_havokObject )
	{
		// Not supported
		GPUAPI_LOG_WARNING"HavokDataBuffer::MoveHandleWithObject Error") );
	}

	DataBuffer::MoveHandle( other );

	// Move
	m_havokObject = other.m_havokObject;
	m_havokClass = other.m_havokClass;

	// Clear
	other.m_havokObject = NULL;
	other.m_havokClass = NULL;
}

#endif