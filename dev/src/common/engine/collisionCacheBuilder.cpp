/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionCacheBuilder.h"
#include "collisionCacheFakeIO.h"

// for stats
#include "../physics/compiledCollision.h"
#include "collisionMesh.h"
#include "../redSystem/hash.h"
#include "../core/2darray.h"
#include "../core/memoryFileWriter.h"
#include "../core/memoryFileReader.h"
#include "../core/compression/zlib.h"
#include "../physics/physicsIncludes.h"

namespace IOHelper
{
	FILE* OpenCacheFile( const String& absoluteFilePath, const Bool forceNew = false )
	{
		FILE* file = nullptr;

		// Open cache file for both reading and writing
		if ( !forceNew )
		{
#ifndef RED_PLATFORM_ORBIS
			file = _wfopen( absoluteFilePath.AsChar(), TXT("rb+") );
#else
			file = fopen( UNICODE_TO_ANSI( absoluteFilePath.AsChar() ), "rb+" );
#endif
		}

		if ( !file )
		{
			// Create new file
#ifndef RED_PLATFORM_ORBIS
			file = _wfopen( absoluteFilePath.AsChar(), TXT("wb+") );
#else
			file = fopen( UNICODE_TO_ANSI( absoluteFilePath.AsChar() ), "wb+" );
#endif
		}

		return file;
	}
}

CCollisionCacheBuilder::CCollisionCacheBuilder()
	: m_file( nullptr )
	, m_dirty( false )
{
}

CCollisionCacheBuilder::~CCollisionCacheBuilder()
{
	// close index file
	if ( m_file )
	{
		fclose( m_file );
		m_file = nullptr;
	}

	m_tokens.Clear();
}

Bool CCollisionCacheBuilder::Initialize( const String& absoluteFilePath )
{
	// Open index file handle
	m_file = IOHelper::OpenCacheFile( absoluteFilePath );
	if ( !m_file )
	{
		ERR_ENGINE( TXT("Unable to open collision cache file. No collision cache data will be accessed.") );
		return false;
	}

	// Conditions to assume collision cache data is valid:
	//  - file has valid header
	//  - file has valid data CRC

	// If we managed to open source file load the existing data from it	
	Bool hasValidData = false;
	{
		CFakeIFileReaderWrapper reader( m_file );
		Red::System::DateTime readerTimeStamp;
		if ( CCollisionCacheData::ValidateHeader( reader, readerTimeStamp ) )
		{
			// load data buffers
			CCollisionCacheData data;
			if ( data.Load( reader ) )
			{
				// Create runtime representation
				m_tokens.Reserve( data.m_tokens.Size() );
				for ( Uint32 i=0; i<data.m_tokens.Size(); ++i )
				{
					RuntimeToken runtimeToken( data, data.m_tokens[i] );
					m_tokens.Insert( runtimeToken.m_name, runtimeToken );
				}

				// Preallocate buffers
				m_loadBuffer.Resize( data.m_header.m_loadBufferSize );
				m_readBuffer.Resize( data.m_header.m_readBufferSize );

				// We have valid data and can use existing file handles
				hasValidData = true;

				// stats
				LOG_ENGINE( TXT("Loaded %d existing entries from collision cache"), m_tokens.Size() );
				LOG_ENGINE( TXT("Collision cache buffers: %1.2f MB Read, %1.2f MB Load"), 
					m_loadBuffer.Size() / (1024.0f*1024.0f),
					m_readBuffer.Size() / (1024.0f*1024.0f) );
			}
			else
			{
				ERR_ENGINE( TXT("Collision cache corruption: index file data is corrupted") );
			}
		}
		else
		{
			WARN_ENGINE( TXT("Collision cache corruption: index file header is corrupted") );
		}
	}

	// Flush file handles
	if ( !hasValidData )
	{
		// Close existing handles
		fclose( m_file );

		// Open new cache file handle (force a new file creation)
		m_file = IOHelper::OpenCacheFile( absoluteFilePath, true );
		if ( !m_file )
		{
			ERR_ENGINE( TXT("Unable to create collision cache file. No collision cache data will be accessed.") );
			return false;
		}
	}

	// It's still OK to use this cache even 
	return true;
}

void CCollisionCacheBuilder::InvalidateCollision( const String& name )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// BIG TODO: that's a nasty memory allocation :(
	const String nameLowered = name.ToLower();

	// Remove matching token form cache
	auto iter = m_tokens.Find( nameLowered );
	if ( iter != m_tokens.End() )
	{
		// remove token from cache
		m_tokens.Erase( iter );
	}
}

ICollisionCache::EResult CCollisionCacheBuilder::HasCollision( const String& name, Red::System::DateTime time ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// BIG TODO: that's a nasty memory allocation :(
	const String nameLowered = name.ToLower();

	// find in cache
	auto it = m_tokens.Find( nameLowered );
	if ( it != m_tokens.End() )
	{
		const RuntimeToken & token = it->m_second;

		// is timestamp OK ?
		if ( !token.m_dateTime.IsValid() || token.m_dateTime >= time )
		{
			// do we have data ?
			if ( token.m_compiledMesh.Lock() || token.m_dataOffset )
			{
				return eResult_Valid;
			}
		}
	}

	// no collision
	return eResult_Invalid;
}

void* CCollisionCacheBuilder::LoadTokenData( const RuntimeToken& token )
{
	// token has no data
	if ( !token.m_dataOffset || !token.m_dataSizeInMemory )
		return nullptr;

	// no cache file
	if ( !m_file )
		return nullptr;

	// corrupted token
	if ( token.m_dataSizeOnDisk > token.m_dataSizeInMemory )
		return nullptr;

	// load data into the load buffer
	{
		// make sure reading buffer is large enough (should no happen)
		if ( m_readBuffer.Size() < token.m_dataSizeOnDisk )
			m_readBuffer.Resize( token.m_dataSizeOnDisk );

		// load the data from file
		fseek( m_file, token.m_dataOffset, SEEK_SET );
		size_t read = fread( m_readBuffer.Data(), 1, token.m_dataSizeOnDisk, m_file );

		// validate read size
		if ( read != token.m_dataSizeOnDisk )
			return nullptr;
	}

	// check data CRC
	const Uint64 dataCRC = Red::System::CalculateHash64( m_readBuffer.Data(), token.m_dataSizeOnDisk );
	if ( dataCRC != token.m_diskCRC )
	{
		ERR_ENGINE( TXT("Collision cache: CRC check for data for file '%ls' failed"), token.m_name.AsChar() );
		return nullptr;
	}

	// decompress the data
	void* resultPtr = m_readBuffer.Data();
	if ( token.m_dataSizeInMemory > token.m_dataSizeOnDisk )
	{
		// make sure reading buffer is large enough (should no happen)
		if ( m_loadBuffer.Size() < token.m_dataSizeInMemory )
			m_loadBuffer.Resize( token.m_dataSizeInMemory );

		// decompress data from READ buffer to LOAD buffer
		Red::Core::Decompressor::CZLib zlib;
		if ( Red::Core::Decompressor::Status_Success != zlib.Initialize( m_readBuffer.Data(), m_loadBuffer.Data(), token.m_dataSizeOnDisk, token.m_dataSizeInMemory ) )
			return nullptr;

		// let's hope decompression will not fail
		if ( Red::Core::Decompressor::Status_Success != zlib.Decompress() )
			return nullptr;

		// use the decompressed data, size is known so don't return it
		resultPtr = m_loadBuffer.Data();
	}

	return resultPtr;
}

ICollisionCache::EResult CCollisionCacheBuilder::FindCompiled( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time, Box2* bounds )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// BIG TODO: that's a nasty memory allocation :(
	const String nameLowered = name.ToLower();

	// find in cache
	auto it = m_tokens.Find( nameLowered );
	if ( it != m_tokens.End() )
	{
		RuntimeToken& token = it->m_second;

		// is timestamp OK ?
#ifndef NO_EDITOR
		if ( !token.m_dateTime.IsValid() || token.m_dateTime >= time )
#endif
		{
			if( bounds ) *bounds = token.m_boundingArea;

			// valid data
			CompiledCollisionPtr compiledMesh = token.m_compiledMesh.Lock();
			if ( compiledMesh )
			{
				result = compiledMesh;
				return eResult_Valid;
			}

			// restore mesh
			void* data = LoadTokenData( token );
			if ( data )
			{
				// Load the mesh
				CMemoryFileReader reader( (const Uint8*) data, token.m_dataSizeInMemory, 0 );
				CompiledCollisionPtr mesh( new CCompiledCollision() );
				mesh->SerializeToCache( reader );

				// Bind
				token.m_compiledMesh = mesh;
				// return loaded mesh
				result = mesh;
				return eResult_Valid;
			}
		}
	}

	// not found or unable to load
	return eResult_Invalid;
}

ICollisionCache::EResult CCollisionCacheBuilder::Compile( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject )
{
	// No mesh, no cooked mesh
	if ( !content )
	{
		return eResult_Invalid;
	}

	// BIG TODO: that's a nasty memory allocation :(
	const String nameLowered = name.ToLower();

	// Find existing collision
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	const EResult searchResult = FindCompiled( result, nameLowered, time );
	if ( searchResult != eResult_Invalid )
	{
		return searchResult;
	}

	// Compile new mesh
	CompiledCollisionPtr compiledMesh = content->CompileCollision( sourceObject );
	if ( !compiledMesh )
	{
		return eResult_Invalid;
	}

	// Define new token to add to cache list
	RuntimeToken newToken;
	newToken.m_name = nameLowered;
	newToken.m_compiledMesh = compiledMesh;
	newToken.m_dateTime = time;
	newToken.m_boundingArea = compiledMesh->GetBoundingArea();

	{
		if ( name.EndsWith( L"w2mesh") )
		{
			newToken.m_collisionType = RTT_Mesh;
		}
		else if ( name.EndsWith( L"w2ter") )
		{
			newToken.m_collisionType = RTT_Terrain;
		}
		else if ( name.EndsWith( L"redapex") )
		{
			newToken.m_collisionType = RTT_ApexDestruction;
		}
		else if ( name.EndsWith( L"redcloth") )
		{
			newToken.m_collisionType = RTT_ApexCloth;
		}
		else if ( name.EndsWith( L"reddest") )
		{
			newToken.m_collisionType = RTT_Destruction;
		}
		else
		{
			RED_FATAL( "Unknown type of stuff in the collision cache: '%ls'", name.AsChar() );
		}
	}

	// Keep additional reference until it's saved
	newToken.m_additionalRef = compiledMesh;

	// Add to hash map, note we use Insert to replace existing entry
	m_tokens.Set( nameLowered, newToken );

	// Return compiled mesh
	result = compiledMesh;

	// Mark as dirty (requires saving)
	m_dirty = true;
	return eResult_Valid;
}

C2dArray* CCollisionCacheBuilder::DumpStatistics( Bool createIfDoesntExist )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

#ifndef USE_PHYSX
	return nullptr;
#else
	C2dArray* array = C2dArray::FactoryInfo< C2dArray >().CreateResource();
	array->ParseData( TXT("Asset;RefCount;BufferSize;TrimeshCount;TrimeshVerticesCount;TrimeshTriangleCount;ConvexCount;ConvexVerticesCount;ConvexPolyCount;BoxCount;CapsuleCount;SphereCount") );

	for( auto i = m_tokens.Begin(); i != m_tokens.End(); ++i )
	{
		RuntimeToken& token = i->m_second;

		const String& name = token.m_name;
		CompiledCollisionPtr compiledMesh = token.m_compiledMesh.Lock();
		if ( !compiledMesh && createIfDoesntExist )
		{
			void* data = LoadTokenData( token );
			if ( data )
			{
				// Load the mesh
				CMemoryFileReader reader( (const Uint8*) data, token.m_dataSizeInMemory, 0 );
				compiledMesh.Reset( new CCompiledCollision() );
				compiledMesh->SerializeToCache( reader );
			}
		}

		const auto & geometries = compiledMesh->GetGeometries();
		if ( geometries.Empty() )
			continue;

		Uint32 bufferSize = 0;
		Uint32 trimeshCount = 0;
		Uint32 trimeshTriangleCount = 0;
		Uint32 trimeshVerticesCount = 0;
		Uint32 convexCount = 0;
		Uint32 convexPolyCount = 0;
		Uint32 convexVerticesCount = 0;
		Uint32 boxCount = 0;
		Uint32 capsuleCount = 0;
		Uint32 sphereCount = 0;
		Uint32 otherCount = 0;

		for( Uint32 j = 0; j != geometries.Size(); ++j )
		{
			const SCachedGeometry& geometry = geometries[ j ];

			bufferSize += geometry.GetCompiledDataSize();
			if( geometry.m_geometryType == ( char ) physx::PxGeometryType::eBOX )
			{
				++boxCount;
			}
			else if( geometry.m_geometryType == ( char ) physx::PxGeometryType::eCAPSULE )
			{
				++capsuleCount;
			}
			else if( geometry.m_geometryType == ( char ) physx::PxGeometryType::eSPHERE )
			{
				++sphereCount;
			}
			else if( geometry.m_geometryType == ( char ) physx::PxGeometryType::eCONVEXMESH )
			{
				++convexCount;
				physx::PxConvexMeshGeometry* convexMesh = ( physx::PxConvexMeshGeometry* ) geometry.GetGeometry();
				convexPolyCount += convexMesh->convexMesh->getNbPolygons();
				convexVerticesCount += convexMesh->convexMesh->getNbVertices();
			}
			else if( geometry.m_geometryType == ( char ) physx::PxGeometryType::eTRIANGLEMESH )
			{
				++trimeshCount;
				physx::PxTriangleMeshGeometry* triangleMesh = ( physx::PxTriangleMeshGeometry* ) geometry.GetGeometry();
				trimeshTriangleCount += triangleMesh->triangleMesh->getNbTriangles();
				trimeshVerticesCount += triangleMesh->triangleMesh->getNbVertices();
			}
			else if( geometry.m_geometryType == ( char ) physx::PxGeometryType::eGEOMETRY_COUNT || geometry.m_geometryType == ( char ) physx::PxGeometryType::eHEIGHTFIELD )
			{
				++otherCount;
			}


		}
		if( otherCount ) continue;
		
		array->AddRow();
		array->SetValue( name, String( TXT("Asset") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), otherCount ), String( TXT("RefCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), bufferSize ), String( TXT("BufferSize") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), trimeshCount ), String( TXT("TrimeshCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), trimeshVerticesCount ), String( TXT("TrimeshVerticesCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), trimeshTriangleCount ), String( TXT("TrimeshTriangleCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), convexCount ), String( TXT("ConvexCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), convexVerticesCount ), String( TXT("ConvexVerticesCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), convexPolyCount ), String( TXT("ConvexPolyCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), boxCount ), String( TXT("BoxCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), capsuleCount ), String( TXT("CapsuleCount") ), array->GetNumberOfRows() - 1 );
		array->SetValue( String::Printf( TXT( "%i" ), sphereCount ), String( TXT("SphereCount") ), array->GetNumberOfRows() - 1 );
	}

	return array;
#endif
}

void CCollisionCacheBuilder::Flush()
{
	CTimeCounter timer;

	// Not dirty
	if ( !m_dirty )
		return;

	// Lock file access
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// Write file header
	CFakeIFileWriterWrapper diskWriter( m_file );

	// Calculate the initial writing position for the newly added data
	Uint32 endOfFilePos = sizeof( CCollisionCacheData::RawHeader ) + sizeof( CCollisionCacheData::IndexHeader );
	for ( auto it = m_tokens.Begin(); it != m_tokens.End(); ++it )
	{
		const RuntimeToken& token = (*it).m_second;
		if ( token.m_dataSizeOnDisk > 0 )
		{
			const Uint32 dataEnd = token.m_dataOffset + token.m_dataSizeOnDisk;
			endOfFilePos = Red::Math::NumericalUtils::Max< Uint32 >( endOfFilePos, dataEnd );
		}
	}

	// Write new data from unsaved, valid tokens
	Uint32 sizeOfDataSaved = 0;
	Uint32 numTokensSaved = 0;
	for ( auto it = m_tokens.Begin(); it != m_tokens.End(); ++it )
	{
		RuntimeToken& token = (*it).m_second;

		// skip the ones that are already saved
		if ( token.m_dataOffset > 0 )
			continue;

		// skip the ones with invalid data
		CompiledCollisionPtr compiledMesh = token.m_compiledMesh.Lock();
		if ( !compiledMesh )
			continue;

		// seek to the current end of the file position
		diskWriter.Seek( endOfFilePos );

		// save data to memory buffer
		TInternalBuffer memoryData;
		CMemoryFileWriter memoryWriter( memoryData );
		compiledMesh->SerializeToCache( memoryWriter );
		RED_ASSERT( memoryWriter.GetSize() > 0, TXT("Unable to save data for collision cache entry %ls"), token.m_name.AsChar() );

		// compress collision data
		Red::Core::Compressor::CZLib zlib;
		if ( zlib.Compress( memoryData.Data(), (Uint32)memoryData.DataSize() ) && (zlib.GetResultSize() < (Uint32)memoryData.DataSize() ) )
		{
			// store compressed data if it's smaller than original and compression was successful
			token.m_dataSizeInMemory = (Uint32)memoryData.DataSize();
			token.m_dataSizeOnDisk = zlib.GetResultSize();
			token.m_dataOffset = endOfFilePos;

			// calculate CRC of the written data so we can validate the read
			token.m_diskCRC = Red::System::CalculateHash64( zlib.GetResult(), zlib.GetResultSize() );

			// write data
			diskWriter.Serialize( (void*) zlib.GetResult(), zlib.GetResultSize() );
		}
		else
		{
			// write uncompressed data
			token.m_dataSizeInMemory = (Uint32)memoryData.DataSize();
			token.m_dataSizeOnDisk = (Uint32)memoryData.DataSize();
			token.m_dataOffset = endOfFilePos;

			// calculate CRC of the written data so we can validate the read
			token.m_diskCRC = Red::System::CalculateHash64( memoryData.Data(), memoryData.DataSize() );

			// write data
			diskWriter.Serialize( memoryData.Data(), memoryData.DataSize() );
		}

		// release internal reference after token was written
		token.m_additionalRef.Reset();
		

		// update end offset
		numTokensSaved += 1;
		sizeOfDataSaved += token.m_dataSizeOnDisk;
		endOfFilePos += token.m_dataSizeOnDisk;
	}

	// Prepare meta data
	CCollisionCacheData data;
	CCollisionCacheDataBuilder builder( data );
	for ( auto it = m_tokens.Begin(); it != m_tokens.End(); ++it )
	{
		RuntimeToken& token = (*it).m_second;

		// store only valid tokens
		if ( token.m_dataOffset > 0 )
		{
			// setup token
			CCollisionCacheData::CacheToken saveToken;
			saveToken.m_name = builder.AddString( token.m_name );
			saveToken.m_dataOffset = token.m_dataOffset;
			saveToken.m_dataSizeInMemory = token.m_dataSizeInMemory;
			saveToken.m_dataSizeOnDisk = token.m_dataSizeOnDisk;
			saveToken.m_dateTime = token.m_dateTime;
			saveToken.m_diskCRC = token.m_diskCRC;
			saveToken.m_boundingArea = token.m_boundingArea;
			saveToken.m_collisionType = token.m_collisionType;
			builder.AddToken( saveToken );
		}
	}

	// Write the original header
	diskWriter.Seek( 0 );
	CCollisionCacheData::WriteHeader( diskWriter );

	// Save the tables to file, respect current end position
	data.Save( diskWriter, endOfFilePos );

	// Log the results
	static const Float oneMB = 1024.0f * 1024.0f;
	LOG_ENGINE( TXT("Flushed collision cache in %1.2fms, %1.3f MB added (%d entries), %1.3f MB in total (%d entries) "),
		timer.GetTimePeriodMS(),
		sizeOfDataSaved / oneMB, numTokensSaved,
		endOfFilePos / oneMB, data.m_tokens.Size() );

	// reset dirty flag
	m_dirty = false;
	return;
}
