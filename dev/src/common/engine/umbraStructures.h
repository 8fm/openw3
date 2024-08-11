#pragma once

#ifdef USE_UMBRA

#include "..\\..\\..\\external\\umbra3.3.13-change-26-2826-2\\interface\\umbraPlatform.hpp"

#include "../core/grid.h"
#include "../core/memoryFileWriter.h"
#include "../core/memoryFileReader.h"

namespace Umbra
{
	class Allocator;
	class Builder;
	class CameraTransform;
	class Logger;
	class Scene;
	class SceneModel;
	class TileResult;
	class TileInput;
	class Tome;
	class TomeCollection;
	class TomeLoader;
	struct SphereLight;
}

static const Float MIN_OCCLUSION_CAMERA_NEAR_PLANE = 0.4f;

class CUmbraTile;
class OcclusionDataBuffer;

#ifndef NO_UMBRA_DATA_GENERATION
struct UmbraChunk
{
	String				meshName;
	Umbra::SceneModel*	model;
	Uint32				meshId;
	Int32				lodNearDistanceQuantized;
	Int32				lodFarDistanceQuantized;
	Bool				twoSided;
	Bool				isTerrain;
	Uint8				lavaFlags;
	Uint8				flags;
};

#pragma pack(push)
#pragma pack(1)
// if you change anything in this structure be sure it's padded properly and update static_assert below
struct UmbraChunkHashGeneration // 14 bytes
{
	Uint32				meshId;
	Int32				lodNearDistanceQuantized;
	Int32				lodFarDistanceQuantized;
	Bool				isTerrain;
	Uint8				lavaFlags;
};
#pragma pack(pop)
static_assert( sizeof(UmbraChunkHashGeneration) == 3*sizeof(Uint32)+sizeof(Bool)+sizeof(Uint8), "Unexpected struct size for disk layout" );

struct UmbraObjectInfo
{
	Umbra::SceneModel*	model;
	Umbra::Matrix4x4	transform;
	Uint32				objectId;
	Uint32				flags;
	Bool				twoSided;
	Umbra::Vector2		distanceLimits;
	Uint8				lavaFlags;
};

typedef THashMap< Uint64, UmbraChunk* >						TModelCache;
#endif	//NO_UMBRA_DATA_GENERATION
#endif // USE_UMBRA

typedef Uint64												TObjectCacheKeyType;
typedef Uint32												TObjectIdType;
typedef THashMap< TObjectCacheKeyType, TObjectIdType, DefaultHashFunc< TObjectCacheKeyType >, DefaultEqualFunc< TObjectCacheKeyType >, MC_UmbraObjectCache, MemoryPool_Umbra >	TObjectCache;
typedef THashMap< TObjectCacheKeyType, TObjectIdType, DefaultHashFunc< TObjectCacheKeyType >, DefaultEqualFunc< TObjectCacheKeyType >, MC_UmbraObjectCache, MemoryPool_Default > TTileObjectCache;
typedef TDynArray< Int32, MC_UmbraVisibleChunks, MemoryPool_Umbra >	TVisibleChunksIndices;
typedef THashMap< TObjectIdType, Uint32, DefaultHashFunc< TObjectIdType >, DefaultEqualFunc< TObjectIdType >, MC_UmbraObjectIDMap, MemoryPool_Default >			TObjectIDToIndexMap;

#ifdef USE_UMBRA
typedef TGrid< THandle< CUmbraTile >, 2 >					TOcclusionGrid;
typedef TDynArray< THandle< CUmbraTile > >					TUmbraTileArray;

static const TObjectIdType INVALID_UMBRA_OBJECT_ID = NumericLimits<Uint32>::Max();

#ifndef NO_EDITOR
struct ComponentDesctiprion
{
	String layerPath;
	String entityName;
	String meshDepoPath;
	Uint32 chunkCount;
	Uint32 vertexCount;
	Uint32 triangleCount;	
};
typedef THashMap< TObjectIdType, ComponentDesctiprion >		TLoadedComponentsMap;
#endif // NO_EDITOR

struct UmbraLoadEntry
{
	TObjectCacheKeyType		m_first;
	TObjectIdType			m_second;
};

namespace UmbraSettings
{
	const Float SMALLEST_OCCLUDER	= 2.0f;
	const Float SMALLEST_HOLE		= 0.25f;
	const Float WORLD_TILE_SIZE		= 256.0f;
	const Uint32 UMBRA_TILE_SIZE	= 32;
}

struct SComputationParameters
{
	Float m_smallestHole;
	Float m_smallestOccluder;
	Float m_tileSize;
	Float m_umbraTileSize;

	Uint32 m_umbraTilesCountPerEdgeOfStreamingTile;

	SComputationParameters()
		: m_smallestHole( -1.0f )
		, m_smallestOccluder( -1.0f )
		, m_tileSize( -1.0f )
		, m_umbraTileSize( -1.0f )
		, m_umbraTilesCountPerEdgeOfStreamingTile( 0 )
	{}

	SComputationParameters( Float smallestOccluder, Float smallestHole, Float tileSize, Float umbraTileSize )
		: m_smallestOccluder( smallestOccluder )
		, m_smallestHole( smallestHole )
		, m_tileSize( tileSize )
		, m_umbraTileSize( umbraTileSize )
	{
		m_umbraTilesCountPerEdgeOfStreamingTile = (Uint32)m_tileSize / (Uint32)m_umbraTileSize;
	}
};

#ifndef NO_UMBRA_DATA_GENERATION
struct STomeDataGenerationContext
{
	VectorI						tileId;
	Vector						tileCorner;
	Float						umbraTileSize;

	String						fileName;
	String						intermediateDirectory;

	SComputationParameters		computationParameters;

	OcclusionDataBuffer*		dataBuffer;
	TObjectCache*				objectCache;

	CUmbraTile*					tile;

	Bool						forceRegenerate;
};

struct SDuplicatesFinderContext
{
	Bool						useOutputFile;
	String						outputFilePath;
};
#endif // NO_UMBRA_DATA_GENERATION

#endif // USE_UMBRA

RED_ALIGNED_STRUCT( TransformHashStructure, 16 )
{
	__m128i m_v[4];
};
static_assert(sizeof(TransformHashStructure) == 16*sizeof(Int32), "Unexpected struct size for disk layout");

RED_ALIGNED_STRUCT( BoundingBoxHashStructure, 16 )
{
	__m128i m_v[2];
};
static_assert(sizeof(BoundingBoxHashStructure) == 8*sizeof(Int32), "Unexpected struct size for disk layout");

namespace UmbraHelpers
{
	static const Uint16 NUMBER_OF_OBJECT_CACHE_BUCKETS = 60;

	// tileId	- 14 bits, value is in the range [0; 16384) (128 x 128 tiles)
	// objectId - 18 bits, value is in the range [0; 262144)
	TObjectIdType CompressObjectId( Uint16 tileId, Uint32 objectIdInTile );
	void DecompressObjectId( TObjectIdType objectId, Uint16& tileId, Uint32& objectIdInTile );

	TObjectCacheKeyType CompressToKeyType( Uint32 meshId, Uint32 transformHash );
	void DecompressFromKeyType( TObjectCacheKeyType keyType, Uint32& meshId, Uint32& transformHash );

	// modelID = [ meshID - 32 bits | lodID - 16 bits | chunkID - 16 bits ]
	Uint64 CompressModelId( Uint32 meshId, Uint16 lodId, Uint16 chunkId );
	void DecompressModelId( Uint64 modelId, Uint32& meshId, Uint16& lodId, Uint16& chunkId );

#ifdef USE_UMBRA
	RED_INLINE void QuantizeDistanceLimits( const Float lodNear, const Float lodFar, Int32& quantizedNear, Int32& quantizedFar )
	{
		quantizedNear	= (Int32)( lodNear * 1024.0f );
		quantizedFar	= (Int32)( lodFar * 1024.0f );
	}
	RED_INLINE Umbra::Vector2 DequantizeDistanceLimits( const Int32 quantizedNear, const Int32 quantizedFar )
	{
		Umbra::Vector2 res;
		res.v[0] = (Float)quantizedNear / 1024.0f;
		res.v[1] = (Float)quantizedFar / 1024.0f;
		return res;
	}

#ifndef NO_UMBRA_DATA_GENERATION
	RED_INLINE Uint64 CalculateUmbraChunkHash( const UmbraChunk* chunk )
	{
		RED_FATAL_ASSERT( chunk, "No chunk to calculate hash from" );
		UmbraChunkHashGeneration hashGenerator;
		hashGenerator.meshId = chunk->meshId;
		hashGenerator.isTerrain = chunk->isTerrain;
		hashGenerator.lavaFlags = chunk->lavaFlags;
		hashGenerator.lodNearDistanceQuantized = chunk->lodNearDistanceQuantized;
		hashGenerator.lodFarDistanceQuantized = chunk->lodFarDistanceQuantized;

		return Red::System::CalculateHash64( &hashGenerator, sizeof( UmbraChunkHashGeneration ) );
	}
#endif // NO_UMBRA_DATA_GENERATION

	Umbra::CameraTransform CalculateCamera( const CRenderCamera& camera );
	Umbra::CameraTransform CalculateShadowCamera( const CRenderCamera& camera );
#endif // USE_UMBRA
	
	RED_INLINE Uint32 CalculateTransformHash( const Matrix& transform )
	{
		static const __m128 multiplier = _mm_set_ps( 0.0f, 1024.0f, 1024.0f, 1024.0f );
		TransformHashStructure hashStruct;
		
		__m128 q0 = _mm_set_ps( 0.0f, transform.V[0].Z, transform.V[0].Y, transform.V[0].X );
		q0 = _mm_mul_ps( q0, multiplier );
		hashStruct.m_v[0] = _mm_cvttps_epi32 ( q0 );

		__m128 q1 = _mm_set_ps( 0.0f, transform.V[1].Z, transform.V[1].Y, transform.V[1].X );
		q1 = _mm_mul_ps( q1, multiplier );
		hashStruct.m_v[1] = _mm_cvttps_epi32 ( q1 );

		__m128 q2 = _mm_set_ps( 0.0f, transform.V[2].Z, transform.V[2].Y, transform.V[2].X );
		q2 = _mm_mul_ps( q2, multiplier );
		hashStruct.m_v[2] = _mm_cvttps_epi32 ( q2 );

		__m128 q3 = _mm_set_ps( 0.0f, transform.V[3].Z, transform.V[3].Y, transform.V[3].X );
		q3 = _mm_mul_ps( q3, multiplier );
		hashStruct.m_v[3] = _mm_cvttps_epi32 ( q3 );

		return Red::System::CalculateHash32( &hashStruct, sizeof( hashStruct ) );
	}

	RED_INLINE Uint64 CalculatePositionHash64( const Vector& pos )
	{
		static const __m128 multiplier = _mm_set_ps( 0.0f, 1024.0f, 1024.0f, 1024.0f );
		TransformHashStructure hashStruct;

		memset( &hashStruct, 0, sizeof(hashStruct) );

		__m128 q0 = _mm_set_ps( 0.0f, pos.Z, pos.Y, pos.X );
		q0 = _mm_mul_ps( q0, multiplier );
		hashStruct.m_v[0] = _mm_cvttps_epi32 ( q0 );

		return Red::System::CalculateHash64( &hashStruct, sizeof( hashStruct ) );
	}

	RED_INLINE Uint32 CalculateBoundingBoxHash( const Box& boundingBox )
	{
		static const __m128 multiplier = _mm_set_ps( 0.0f, 1024.0f, 1024.0f, 1024.0f );
		BoundingBoxHashStructure hashStruct;

		__m128 min128 = _mm_set_ps( 0.0f, boundingBox.Min.Z, boundingBox.Min.Y, boundingBox.Min.X );
		min128 = _mm_mul_ps( min128, multiplier );
		hashStruct.m_v[0] = _mm_cvttps_epi32 ( min128 );

		__m128 max128 = _mm_set_ps( 0.0f, boundingBox.Max.Z, boundingBox.Max.Y, boundingBox.Max.X );
		max128 = _mm_mul_ps( max128, multiplier );
		hashStruct.m_v[1] = _mm_cvttps_epi32 ( max128 );

		return Red::System::CalculateHash32( &hashStruct, sizeof( hashStruct ) );
	}
}

#ifdef USE_UMBRA

class UmbraLogger : public Umbra::Logger
{
	virtual void log( Umbra::Logger::Level level, const char* str );
};

class UmbraAllocator : public Umbra::Allocator
{
	virtual void* allocate( size_t size, const char* info = NULL );
	virtual void deallocate( void* ptr );

public:
	UmbraAllocator( EMemoryClass memoryClass = MC_UmbraGeneric )
		: m_memoryClass( memoryClass )
	{ }

private:
	EMemoryClass		m_memoryClass;
};

class UmbraFixedSizeAllocator : public Umbra::Allocator
{
public:
	virtual void* allocate( size_t size, const char* info = NULL );
	virtual void deallocate( void* ptr );

public:
	UmbraFixedSizeAllocator( const MemSize allocationSize )
		: m_allocationSize( allocationSize )
		, m_alignment( CalculateDefaultAlignment( m_allocationSize ) )
		, m_numAllocations( 0 )
	{ }

	RED_FORCE_INLINE const MemSize	GetAllocationSize() const { return m_allocationSize; }

private:
	const MemSize		m_allocationSize;
	const size_t		m_alignment;
	Red::Threads::CAtomic< Int32 > m_numAllocations;
};

class UmbraTomeCollectionScratchAllocator : public Umbra::Allocator
{
public:
	UmbraTomeCollectionScratchAllocator()
#ifndef RED_FINAL_BUILD
		: m_allocationPeak( 0 )
		, m_currentlyAllocated( 0 )
		, m_numAllocs( 0 )
		, m_numDeallocs( 0 )
#endif // RED_FINAL_BUILD
	{}

#ifndef RED_FINAL_BUILD
	virtual ~UmbraTomeCollectionScratchAllocator()
	{
		RED_LOG_SPAM( UmbraInfo, TXT("TomeCollectionScratchAllocationPeak: %d bytes [%d allocs, %d deallocs]"), m_allocationPeak, m_numAllocs, m_numDeallocs );
	}

	RED_FORCE_INLINE size_t GetAllocationPeak() const { return m_allocationPeak; }
#endif // RED_FINAL_BUILD

	virtual void* allocate( size_t size, const char* info = NULL );
	virtual void deallocate( void* ptr );

private:
	static const MemSize	c_largeAllocThreshold = 2 * 1024 * 1024;
#ifndef RED_FINAL_BUILD
	size_t					m_allocationPeak;
	size_t					m_currentlyAllocated;
	Uint32					m_numAllocs;
	Uint32					m_numDeallocs;
#endif // RED_FINAL_BUILD
};

//////////////////////////////////////////////////////////////////////////
// TomeCollectionWrapper

class CUmbraTomeCollection 
{
private:
	static UmbraFixedSizeAllocator s_tomeCollectionAllocator;

public:
	CUmbraTomeCollection();
	~CUmbraTomeCollection();

	Bool BuildTomeCollection( TDynArray< const Umbra::Tome* >& tomes, Umbra::Allocator& scratchAllocator, const CUmbraTomeCollection* previous );
	
public:
	RED_FORCE_INLINE Umbra::TomeCollection* GetTomeCollection() const { return m_tomeCollection; }

private:
	Umbra::TomeCollection*	m_tomeCollection;
	void*					m_buffer;
};

//////////////////////////////////////////////////////////////////////////
#ifdef USE_UMBRA_COOKING

struct UmbraIntermediateResult
{
	UmbraIntermediateResult()
		: elementHash( 0 )
		, subtileHash( 0 )
		, tileResult( TDataBufferAllocator< MC_UmbraGeneric >::GetInstance() )
	{
	}

	void Serialize( IFile* stream )
	{
		*stream << elementHash;
		*stream << subtileHash;
		tileResult.Serialize( *stream );
	}

	Uint32		elementHash;
	StringAnsi	subtileHash;
	DataBuffer	tileResult;
};

class CUmbraIntermediateResultStorage
{
public:
	static Red::Threads::CMutex s_storageMutex;

public:
	CUmbraIntermediateResultStorage( const String& directoryPath, const VectorI& tileId, Bool forceRegenerate );
	Bool Get( Umbra::Builder& builder, Int32 i, Int32 j, Int32 k, const AnsiChar* prevHash, Umbra::TileResult& tileResult );
	void Add( Int32 i, Int32 j, Int32 k, const AnsiChar* hash, const Umbra::TileResult& tileResult );
	
	void Save();

private:
	Uint32 GetHash( Int32 i, Int32 j, Int32 k );

	THashMap< Uint32, UmbraIntermediateResult > m_elements;
	String										m_dirPath;
	VectorI										m_tileId;
	Bool										m_dirty;
	Bool										m_forceRegenerate;
};

//////////////////////////////////////////////////////////////////////////

class UmbraOuputStream : public Umbra::OutputStream
{
public:
	UmbraOuputStream( DataBuffer* buffer );
	virtual ~UmbraOuputStream();

public:
	virtual Uint32 write( const void *ptr, Uint32 numBytes );

private:
	DataBuffer*			m_buffer;
	CMemoryFileWriter*	m_writer;
	TDynArray< Uint8 >	m_data;
};

class UmbraInputStream : public Umbra::InputStream
{
public:
	UmbraInputStream( DataBuffer* buffer );
	~UmbraInputStream();

public:
	virtual Uint32 read( void *ptr, Uint32 numBytes );

private:
	DataBuffer*			m_buffer;
	CMemoryFileReader*	m_reader;
};
#endif // USE_UMBRA_COOKING

#endif // USE_UMBRA
