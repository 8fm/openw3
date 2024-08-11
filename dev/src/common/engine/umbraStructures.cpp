#include "build.h"
#include "umbraIncludes.h"
#include "umbraStructures.h"

namespace UmbraHelpers
{
	// tileId	- 14 bits, value is in the range [0; 16384) (128 x 128 tiles)
	// objectId - 18 bits, value is in the range [0; 262144)
	TObjectIdType CompressObjectId( Uint16 tileId, Uint32 objectIdInTile )
	{
		TObjectIdType objectId = 0;

		objectId = ( tileId & 0x3fff ) << 18;
		objectId |= ( objectIdInTile & 0x0003ffff );

		return objectId;
	}

	void DecompressObjectId( TObjectIdType objectId, Uint16& tileId, Uint32& objectIdInTile )
	{
		Int32 mask = ( 0x3fff << 18 );
		tileId			= ( objectId & mask ) >> 18;
		objectIdInTile	= ( objectId & 0x0003ffff );
	}

	TObjectCacheKeyType CompressToKeyType( Uint32 meshId, Uint32 transformHash )
	{
		TObjectCacheKeyType result = meshId;

		result = result << 32;
		result |= transformHash;

		return result;
	}

	void DecompressFromKeyType( TObjectCacheKeyType keyType, Uint32& meshId, Uint32& transformHash )
	{
		meshId			= (Uint32)(( keyType & ( 0xffffffffffffffff << 32 ) ) >> 32);
		transformHash	= (Uint32)( keyType & 0xffffffffffffffff );
	}

	// modelID = [ meshID - 32 bits | lodID - 16 bits | chunkID - 16 bits ]
	Uint64 CompressModelId( Uint32 meshId, Uint16 lodId, Uint16 chunkId )
	{
		Uint64 res = meshId;
		res = res << 32;
		res |= ( lodId ) << 16;
		res |= chunkId;
		return res;
	}

	void DecompressModelId( Uint64 modelId, Uint32& meshId, Uint16& lodId, Uint16& chunkId )
	{
		meshId		= (Uint32)(( modelId & ( 0xffffffffffffffff << 32 ) ) >> 32);
		Uint64 mask = 0xffffffff << 16;
		Uint64 maskedLod = modelId & mask;
		Uint64 finalLod = maskedLod >> 16;
		lodId		= (Uint16)( finalLod );
		chunkId		= (Uint16)( modelId & 0xffffffff );
	}

#ifdef USE_UMBRA
	Umbra::CameraTransform CalculateCamera( const CRenderCamera& camera )
	{
		const Vector& camPos = camera.GetPosition();
		Umbra::Vector3 uCamPos;
		uCamPos.v[0] = camPos.X;
		uCamPos.v[1] = camPos.Y;
		uCamPos.v[2] = camPos.Z;
		Umbra::Matrix4x4 mtx;
		const Matrix& worldToCam = camera.GetWorldToScreen();
		Red::System::MemoryCopy( mtx.m, worldToCam.V, 4 * 4 * sizeof( Float ) );
		return Umbra::CameraTransform( mtx, uCamPos );
	}

	Umbra::CameraTransform CalculateShadowCamera( const CRenderCamera& camera )
	{
		const Vector& camPos = camera.GetPosition();
		Umbra::Vector3 uCamPos;
		uCamPos.v[0] = camPos.X;
		uCamPos.v[1] = camPos.Y;
		uCamPos.v[2] = camPos.Z;
		Umbra::Matrix4x4 mtx;
		const Matrix& worldToCam = camera.GetWorldToScreen();
		Red::System::MemoryCopy( mtx.m, worldToCam.V, 4 * 4 * sizeof( Float ) );
		return Umbra::CameraTransform( mtx, uCamPos );
	}
#endif // USE_UMBRA
}

#ifdef USE_UMBRA

void UmbraLogger::log( Umbra::Logger::Level level, const char* str )
{
	switch( level )
	{
	case Logger::LEVEL_DEBUG:
		RED_LOG_SPAM( UmbraDebug, ANSI_TO_UNICODE(str) );
		break;
	case Logger::LEVEL_INFO:
		RED_LOG( UmbraInfo, ANSI_TO_UNICODE(str) );
		break;
	case Logger::LEVEL_WARNING:
		RED_LOG_WARNING( UmbraWarning, ANSI_TO_UNICODE(str) );
		break;
	case Logger::LEVEL_ERROR:
		RED_ASSERT( false, TXT("Umbra error: %s"), ANSI_TO_UNICODE(str) );
		RED_LOG_ERROR( UmbraError, ANSI_TO_UNICODE(str) );
		break;
	}
}

void* UmbraAllocator::allocate( size_t size, const char* info /*=NULL*/ )
{
	return RED_MEMORY_ALLOCATE( MemoryPool_Umbra, m_memoryClass, size );
}

void UmbraAllocator::deallocate( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_Umbra, m_memoryClass, ptr );
}

void* UmbraFixedSizeAllocator::allocate( size_t size, const char* info /*=NULL*/ )
{
	RED_LOG_SPAM( UmbraInfo, TXT("Allocating memory for TomeCollection: %d bytes."), size );
	RED_FATAL_ASSERT( size <= m_allocationSize, "Trying to allocate more than specified." );
	RED_FATAL_ASSERT( m_numAllocations.GetValue() < 2, "Trying to create more than two TomeCollections!!" );
	m_numAllocations.Increment();
	return RED_MEMORY_ALLOCATE( MemoryPool_UmbraTC, MC_UmbraTomeCollection, m_allocationSize );
}

void UmbraFixedSizeAllocator::deallocate( void* ptr )
{
	RED_FATAL_ASSERT( m_numAllocations.GetValue() > 0, "" );	
	RED_MEMORY_FREE( MemoryPool_UmbraTC, MC_UmbraTomeCollection, ptr );
	m_numAllocations.Decrement();
	RED_FATAL_ASSERT( m_numAllocations.GetValue() >= 0, "" );
}

void* UmbraTomeCollectionScratchAllocator::allocate( size_t size, const char* info )
{
#ifndef RED_FINAL_BUILD
	++m_numAllocs;
	m_currentlyAllocated += size;
	m_allocationPeak = Max( m_currentlyAllocated, m_allocationPeak );
#endif // RED_FINAL_BUILD
	return RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_UmbraTomeCollection, size );
}

void UmbraTomeCollectionScratchAllocator::deallocate( void* ptr )
{
#ifndef RED_FINAL_BUILD
	++m_numDeallocs;
	m_currentlyAllocated -= Memory::GetBlockSize< MemoryPool_Default >( ptr );
#endif // RED_FINAL_BUILD
	RED_MEMORY_FREE( MemoryPool_Default, MC_UmbraTomeCollection, ptr );
}

//////////////////////////////////////////////////////////////////////////

const MemSize umbraTomeCollectionMaxSize = 20u * 1024u * 1024u; // keep in sync with r4Memory{PLATFORM}.h!!
UmbraFixedSizeAllocator CUmbraTomeCollection::s_tomeCollectionAllocator( umbraTomeCollectionMaxSize );

CUmbraTomeCollection::CUmbraTomeCollection()
{
	const MemSize bufferSize = s_tomeCollectionAllocator.GetAllocationSize();
	m_buffer = s_tomeCollectionAllocator.allocate( bufferSize );
	RED_ASSERT( m_buffer );
	m_tomeCollection = new Umbra::TomeCollection();
	m_tomeCollection->init( m_buffer, bufferSize );
}

CUmbraTomeCollection::~CUmbraTomeCollection()
{
	if ( m_tomeCollection )
	{
		delete m_tomeCollection;
		m_tomeCollection = nullptr;
	}
	if ( m_buffer )
	{
		s_tomeCollectionAllocator.deallocate( m_buffer );
		m_buffer = nullptr;
	}
}

#ifndef RED_FINAL_BUILD
void OutputErrorMessage( Umbra::TomeCollection::ErrorCode code )
{
	String message( TXT( "TomeCollection::build failed with error: " ) );
#define TEST(x) case x: message += TXT(#x); break;
	switch ( code )
	{
		TEST(Umbra::TomeCollection::ERROR_OUT_OF_MEMORY);
		TEST(Umbra::TomeCollection::ERROR_INVALID_PARAM);
		TEST(Umbra::TomeCollection::ERROR_NO_MATCHING_DATA);
		TEST(Umbra::TomeCollection::ERROR_CORRUPT_TOME);
		TEST(Umbra::TomeCollection::ERROR_OVERLAPPING_TOMES);
		TEST(Umbra::TomeCollection::ERROR_IO);
		TEST(Umbra::TomeCollection::ERROR_OLDER_VERSION);
		TEST(Umbra::TomeCollection::ERROR_NEWER_VERSION);
		TEST(Umbra::TomeCollection::ERROR_CORRUPTED);
		TEST(Umbra::TomeCollection::ERROR_UNBUILT);
		TEST(Umbra::TomeCollection::ERROR_BAD_ENDIAN);
		TEST(Umbra::TomeCollection::ERROR_INVALID_INPUT_TOMES);
	}
#undef TEST
	RED_LOG_ERROR( UmbraError, message.AsChar() );
}
#endif

Bool CUmbraTomeCollection::BuildTomeCollection( TDynArray< const Umbra::Tome* >& tomes, Umbra::Allocator& scratchAllocator, const CUmbraTomeCollection* previous )
{
	Umbra::TomeCollection::ErrorCode code = m_tomeCollection->build( tomes.TypedData(), tomes.SizeInt(), &scratchAllocator, previous ? previous->GetTomeCollection() : nullptr );
	Bool ret = code == Umbra::TomeCollection::SUCCESS;

#ifndef RED_FINAL_BUILD
	if ( !ret )
	{
		OutputErrorMessage( code );
	}
#endif // RED_FINAL_BUILD

	return ret;
}

//////////////////////////////////////////////////////////////////////////
#ifdef USE_UMBRA_COOKING

Red::Threads::CMutex CUmbraIntermediateResultStorage::s_storageMutex;

CUmbraIntermediateResultStorage::CUmbraIntermediateResultStorage( const String& directoryPath, const VectorI& tileId, Bool forceRegenerate )
	: m_dirPath( directoryPath )
	, m_tileId( tileId )
	, m_dirty( false )
	, m_forceRegenerate( forceRegenerate )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_storageMutex );

	String fileInput = String::Printf( TXT("%s\\%dx%d.intermediateResult"), directoryPath.AsChar(), tileId.X, tileId.Y );
	IFile* inputFile = GFileManager->CreateFileReader( fileInput, FOF_AbsolutePath );
	if ( inputFile )
	{
		Uint32 numElements;
		*inputFile << numElements;
		m_elements.Reserve( numElements );
		for ( Uint32 i = 0; i < numElements; ++i )
		{
			UmbraIntermediateResult result;
			result.Serialize( inputFile );
			m_elements.Insert( result.elementHash, result );
		}
		delete inputFile;
	}
}

Bool CUmbraIntermediateResultStorage::Get( Umbra::Builder& builder, Int32 i, Int32 j, Int32 k, const AnsiChar* prevHash, Umbra::TileResult& tileResult )
{
	if ( m_forceRegenerate )
	{
		// force all tiles to be regenerated
		return false;
	}

	Uint32 elementHash = GetHash( i, j, k );

	UmbraIntermediateResult result;
	Bool findResult;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_storageMutex );
		findResult = m_elements.Find( elementHash, result );
	}
	if ( findResult && result.subtileHash == prevHash )
	{
		UmbraInputStream inputStream( &result.tileResult );
		Umbra::Builder::Error err = builder.loadTileResult( tileResult, inputStream );
		return err == Umbra::Builder::SUCCESS;
	}

	return false;
}

void CUmbraIntermediateResultStorage::Add( Int32 i, Int32 j, Int32 k, const AnsiChar* hash, const Umbra::TileResult& tileResult )
{
	Uint32 elementHash = GetHash( i, j, k );

	UmbraIntermediateResult result;
	result.elementHash = elementHash;
	result.subtileHash = hash;
	
	{
		// data will be transferred to result.tileResult when leaving scope
		UmbraOuputStream outputStream( &result.tileResult );
		tileResult.serialize( outputStream );
	}
	
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_storageMutex );
		m_elements.Set( elementHash, result );
		m_dirty = true;
	}
}

void CUmbraIntermediateResultStorage::Save()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_storageMutex );

	if ( !m_dirty )
	{
		// nothing changed, do not save
		return;
	}

	String filepath = String::Printf( TXT("%s\\%dx%d.intermediateResult"), m_dirPath.AsChar(), m_tileId.X, m_tileId.Y );
	IFile* outputFile = GFileManager->CreateFileWriter( filepath, FOF_AbsolutePath );
	if ( outputFile )
	{
		Uint32 numElements = m_elements.Size();
		*outputFile << numElements;
		for ( auto it = m_elements.Begin(); it != m_elements.End(); ++it )
		{
			(*it).m_second.Serialize( outputFile );
		}
		delete outputFile;
		outputFile = nullptr;
	}
}

Uint32 CUmbraIntermediateResultStorage::GetHash( Int32 i, Int32 j, Int32 k )
{
	StringAnsi sHash = StringAnsi::Printf( "%d%d%d", i, j, k );
	Uint32 hash = Red::System::CalculateHash32( sHash.AsChar(), HASH32_BASE );
	return hash;	
}

//////////////////////////////////////////////////////////////////////////
UmbraOuputStream::UmbraOuputStream( DataBuffer* buffer )
	: m_buffer( buffer )
{
	m_writer = new CMemoryFileWriter( m_data );
}

UmbraOuputStream::~UmbraOuputStream()
{
	if ( m_buffer && m_writer )
	{
		m_buffer->Allocate( (Uint32)m_writer->GetSize() );
		Red::MemoryCopy( m_buffer->GetData(), m_writer->GetBuffer(), (Uint32)m_writer->GetSize() );
	}

	m_data.ClearFast();
	if ( m_writer )
	{
		delete m_writer;
		m_writer = nullptr;
	}
}

Uint32 UmbraOuputStream::write( const void* ptr, Uint32 numBytes )
{
	if ( m_writer && ptr )
	{
		void* data = const_cast< void* >( ptr );
		m_writer->Serialize( data, numBytes );
		return numBytes;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
UmbraInputStream::UmbraInputStream( DataBuffer* buffer )
	: m_buffer( buffer )
{
	size_t dataSize = m_buffer->GetSize();
	Uint8* data = static_cast< Uint8* >( m_buffer->GetData() );
	m_reader = new CMemoryFileReader( data, dataSize, 0 );
}

UmbraInputStream::~UmbraInputStream()
{
	if ( m_reader )
	{
		delete m_reader;
		m_reader = nullptr;
	}
}

Uint32 UmbraInputStream::read( void* ptr, Uint32 numBytes )
{
	if ( m_reader && ptr )
	{
		m_reader->Serialize( ptr, numBytes );
		return numBytes;
	}
	return 0;
}
#endif // USE_UMBRA_COOKING

#endif // USE_UMBRA
