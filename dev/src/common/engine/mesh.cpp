/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mesh.h"
#include "navigationObstacle.h"
#include "renderer.h"
#include "collisionMesh.h"
#include "collisionShape.h"
#include "meshCacheCooker.h"
#include "meshDataBuilder.h"
#include "../physics/compiledCollision.h"
#include "materialCompiler.h"
#include "drawableComponent.h"
#include "meshEnum.h"
#include "renderResource.h"
#include "collisionCache.h"

#include "../core/gatheredResource.h"
#include "../core/loadingJobManager.h"
#include "../../common/renderer/renderMesh.h"
#include "../../common/redMath/float16compressor.h"

IMPLEMENT_ENGINE_CLASS( CMesh );
IMPLEMENT_ENGINE_CLASS( SMeshChunkPacked );
IMPLEMENT_ENGINE_CLASS( SMeshCookedData );
IMPLEMENT_ENGINE_CLASS( SMeshSoundInfo );
IMPLEMENT_RTTI_BITFIELD( EMeshChunkRenderMask );

CGatheredResource resDefaultMesh( TXT("engine\\meshes\\editor\\debug.w2mesh"), RGF_Startup );

const AllocateFunction deviceAllocate = []( Uint32 size, Uint16 alignment ){ return GpuApi::AllocateInPlaceMemory( GpuApi::INPLACE_Buffer, size, alignment ); };
const DeallocateFunction deviceDealloc = []( void * ptr ){ GpuApi::ReleaseInPlaceMemory( GpuApi::INPLACE_Buffer, ptr ); };

#ifndef NO_EDITOR
Bool CMesh::s_pipelineActive = false;
#endif

const Uint8 CMesh::MESH_VERSION_CURRENT = 2;
const Uint8 CMesh::MESH_VERSION_SHADOW_MASK_PER_CHUNK = 1;
const Uint8 CMesh::MESH_VERSION_PER_CHUNK_SHADOW_MASK_OPTIONS = 2;

#if VER_MINIMAL <= VER_REARRANGE_MESH_BONE_DATA_LAYOUT

// Skeleton bone
struct SkeletonBone
{
	Matrix		m_rigMatrix;
	Float		m_vertexEpsilon;
	CName		m_name;

	RED_INLINE SkeletonBone()
		: m_rigMatrix( Matrix::IDENTITY )
		, m_vertexEpsilon( 0.0f )
	{};

	friend IFile& operator<<( IFile& file, SkeletonBone& bone )
	{
		file << bone.m_rigMatrix;
		file << bone.m_name;

		file << bone.m_vertexEpsilon;

		return file;
	}
};

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SMeshCookedData::SMeshCookedData()
	: m_collisionInitPositionOffset(0,0,0)
	, m_dropOffset(0,0,0)
	, m_quantizationScale(0,0,0)
	, m_quantizationOffset(0,0,0)
	, m_vertexBufferSize( 0 )
	, m_indexBufferSize( 0 )
	, m_indexBufferOffset( 0 )
{
	m_renderBuffer.SetAllocateFunction( deviceAllocate );
	m_renderBuffer.SetDeallocateFunction( deviceDealloc );
}

const Uint32 SMeshCookedData::CalcSize() const
{
	Uint32 ret = 0;
	ret += (Uint32)m_renderChunks.DataSize();
	ret += m_renderBuffer.GetSize();
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CMesh::CMesh()
	: CMeshTypeResource()
	, m_useExtraStreams( false )
	, m_generalizedMeshRadius( 1.0f )
	, m_batchOperationCounter( 0 )
#ifndef NO_DEBUG_PAGES
	, m_resourceLoadError( false )
#endif
	, m_collisionMesh( NULL )
	, m_soundInfo( nullptr )
	, m_entityProxy( false )
	, m_isOccluder( true )
	, m_smallestHoleOverride( -1.0f )
	, m_mergeInGlobalShadowMesh( true )
	, m_internalVersion( 0 )
{
}

CMesh::~CMesh()
{
	// Release rendering resource
	SAFE_RELEASE( m_renderResource );
}

void CMesh::OnSerialize( IFile& file )
{	
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Do not do this crap in GC
	if ( file.IsGarbageCollector() )
	{
		return;
	}

	// Data conversion: TO BE REMOVED AFTER MESH RESAVE
	if ( file.GetVersion() < VER_UPDATED_RESOURCE_FORMAT )
	{
		if ( file.IsReader() )
		{
			// the chunk buffer was not deserialized from file, we need to convert it on the fly
			if( !m_chunksBuffer.HACK_WasDeserializedFromFile() )
			{
				SerializeDeferredDataBufferFromLatentLoaderData( file, m_chunksBuffer );
			}
		}
		else if ( file.IsWriter() )
		{
			// do nothing - the data will be saved as a property
			RED_FATAL( "Trying to save mesh in old format - this should not happen" );
		}
	}

	// LOD infos
	file << m_lodLevelInfo;

	// Persistent data
	if ( file.IsReader() && file.GetVersion() < VER_REARRANGE_MESH_BONE_DATA_LAYOUT )
	{
		TDynArray< SkeletonBone > bones;
		file << bones;

		m_boneNames.Resize( bones.Size() );
		m_boneRigMatrices.Resize( bones.Size() );
		m_boneVertexEpsilons.Resize( bones.Size() );
		for ( Uint32 i = 0; i < bones.Size(); ++i )
		{
			m_boneNames[ i ]			= bones[ i ].m_name;
			m_boneRigMatrices[ i ]		= bones[ i ].m_rigMatrix;
			m_boneVertexEpsilons[ i ]	= bones[ i ].m_vertexEpsilon;
		}
	}
	else
	{
		file << m_boneNames;
		file << m_boneRigMatrices;
		file << m_boneVertexEpsilons;
	}
	file << m_usedBones;

#ifndef NO_RESOURCE_IMPORT
	// Update old meshes so that their skinning data is properly sorted.
	if ( file.IsReader() && file.GetVersion() < VER_SEPARATED_PAYLOAD_FROM_MESH_CHUNKS )
	{
		// import old data
		typedef TDynArray< SMeshChunk > TOldMeshData;
		auto oldData = m_chunksBuffer.ReadObject< TOldMeshData >( file.GetVersion() );
		if ( oldData )
		{
			SetMeshData( *oldData, m_lodLevelInfo, false );
		}
	}
	else if ( file.IsReader() && !IsCooked() )
	{
		Bool hasValidData = false;

		// check data size
		Uint32 numIndices = 0;
		for ( Uint32 i=0; i<m_chunks.Size(); ++i )
		{
			numIndices += m_chunks[i].m_numIndices;
		}

		// check index buffer (corrupted the most)
		if ( numIndices == (m_rawIndices.GetSize() / sizeof(Uint16)) )
		{
			BufferHandle indexData = m_rawIndices.AcquireBufferHandleSync();
			if ( indexData )
			{
				hasValidData = true;

				// validate indices
				const Uint16* indices = (const Uint16*) indexData->GetData();
				for ( Uint32 i=0; i<m_chunks.Size() && hasValidData; ++i )
				{
					const Uint16 maxVertex = m_chunks[i].m_numVertices - 1;
					const Uint16 numIndices = m_chunks[i].m_numIndices;

					const Uint16* index = &indices[ m_chunks[i].m_firstIndex ];
					for ( Uint32 j=0; j<numIndices; ++j, ++index )
					{
						if ( *index > maxVertex )
						{
							hasValidData = false;
							break;
						}
					}
				}
			}
		}

		// restore
		if ( !hasValidData )
		{
			ERR_ENGINE( TXT("!!! CORRUPTED MESH DATA !!!") );
			ERR_ENGINE( TXT("Mesh '%ls' contains corrupted data, will be restored from original"), GetDepotPath().AsChar() );
		}
	}
#endif
}

Bool CMesh::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	// Wind flag
	if ( propertyName == TXT("soundTypeIdentification") || propertyName == TXT("soundSizeIdentification") )
	{
		CName value;
		if( readValue.AsType< CName >( value ) )
		{
			if(value != CName::NONE)
			{
				if( !m_soundInfo )
				{
					m_soundInfo = new SMeshSoundInfo();
				}

				if( propertyName == TXT("soundTypeIdentification") )
				{
					m_soundInfo->m_soundTypeIdentification = value;
				}
				else if( propertyName == TXT("soundSizeIdentification") )
				{
					m_soundInfo->m_soundSizeIdentification = value;
				}
			}
		}
	}
	if ( propertyName == TXT("useWind") || propertyName == TXT("useSpeedtreeWind") )
	{
		Bool useSpeedtreeWind;
		readValue.AsType< Bool >( useSpeedtreeWind );
		return true;
	}	

	// Pass to base class
	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

void CMesh::OnPropertyPostChange( IProperty* property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

	// Check the property
	Bool recreateMesh = false;
	Bool regenerateProxies = false;
	if ( property->GetName() == TXT("autoHideDistance") ) { regenerateProxies = true; }
	if ( property->GetName() == TXT("lodDegenerationStartDistance") ) { regenerateProxies = true; }
	if ( property->GetName() == TXT("lodDissolveTransitionTime") ) { regenerateProxies = true; }
	if ( property->GetName() == TXT("editorOnly") ) { regenerateProxies = true; }
	if ( property->GetName() == TXT("shadowRenderingDistance") ) { regenerateProxies = true; }
	if ( property->GetName() == TXT("forceNoShadowLOD") ) { regenerateProxies = true; }
	if ( property->GetName() == TXT("useSpeedtreeWind") ) { recreateMesh = true; }
	if ( property->GetName() == TXT("useVertexCloth") ) { recreateMesh = true; }
	if ( property->GetName() == TXT("useVertexCollapse") ) { recreateMesh = true; }
	if ( property->GetName() == TXT("useVertexColor") ) { recreateMesh = true; }
	if ( property->GetName() == TXT("useSecondUV") ) { recreateMesh = true; }

	// Recreate mesh
	if ( recreateMesh )
	{
		CreateRenderResource();
		regenerateProxies = true;
	}

	// Regenerate drawable proxies
	if ( regenerateProxies )
	{
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
}

void CMesh::OnPreSave()
{
	TBaseClass::OnPreSave();

	// reset version on resave
	m_internalVersion = MESH_VERSION_CURRENT;
}

void CMesh::OnPostLoad()
{
	// Pass to base class
	TBaseClass::OnPostLoad();

#ifndef NO_EDITOR
	if( s_pipelineActive )
	{
		return;
	}
#endif

	// Data repairs
	Bool isCooked = IsCooked();
#ifndef NO_RESOURCE_IMPORT
	if( !isCooked )
	{
		// Make sure materialNames and material list have the same size
		RebuildMaterialMap();

		// Repair mesh author name
		if (m_authorName.Empty())
			m_authorName = String(TXT("No Author Name"));
	}
#endif // NO_RESOURCE_IMPORT

	// Recompute the shadow mask
#ifndef NO_RESOURCE_IMPORT
	if ( m_internalVersion < MESH_VERSION_SHADOW_MASK_PER_CHUNK )
	{
		RecomputeDefaultRenderMask();
		RecomputeShadowFadeDistance();
	}
	if ( m_internalVersion < MESH_VERSION_PER_CHUNK_SHADOW_MASK_OPTIONS )
	{
		RecomputePerChunkShadowMaskOptions();
	}
#endif
}

void CMesh::ForceFullyLoad()
{
	if (!m_renderResource)
	{
		CreateRenderResource();
	}
}

CGatheredResource * CMesh::GetDefaultResource()
{
	return &resDefaultMesh;
}

void CMesh::CreateRenderResource()
{
#ifndef NO_EDITOR
	if( s_pipelineActive )
	{
		return;
	}
#endif
	// Release previous resource
	SAFE_RELEASE( m_renderResource );

	if ( GRender && !GRender->IsDeviceLost() )
	{
		m_renderResource = GRender->UploadMesh( this );
	}
}

const SMeshTypeResourceLODLevel& CMesh::GetLODLevel( Uint32 level ) const
{
	if ( level < m_lodLevelInfo.Size() )
	{
		return m_lodLevelInfo[level].m_meshTypeLOD;
	}
	return TBaseClass::GetLODLevel( level );
}

void CMesh::GetAdditionalInfo( TDynArray< String >& info ) const
{	
#ifndef NO_RESOURCE_IMPORT
	if ( IsCooked() )
		return;

	if ( m_entityProxy )
	{
		// If this is an entity proxy, then the first LOD should be empty. So instead we'll give stats for LOD 1 if we can.
		Uint32 lod = m_lodLevelInfo.Size() > 1 ? 1 : 0;
		info.PushBack( String::Printf( TXT("%i triangles, %i vertices"), CountLODTriangles(lod), CountLODVertices(lod) ) );
		info.PushBack( String::Printf( TXT("%i materials, %i chunks"), CountLODMaterials(lod), CountLODChunks(lod) ) );
		info.PushBack( String::Printf( TXT("%i bones, %i LODs"), m_boneNames.Size(), m_lodLevelInfo.Size() ) );
		info.PushBack( TXT("Entity Proxy") );
	}
	else
	{
		info.PushBack( String::Printf( TXT("%i triangles, %i vertices"), CountLODTriangles(0), CountLODVertices(0) ) );
		info.PushBack( String::Printf( TXT("%i materials, %i chunks"), CountLODMaterials(0), CountLODChunks(0) ) );
		info.PushBack( String::Printf( TXT("%i bones, %i LODs"), m_boneNames.Size(), m_lodLevelInfo.Size() ) );
	}
#endif
}

const Vector& CMesh::GetInitPositionOffset() const
{
	if ( IsCooked() )
		return m_cookedData.m_collisionInitPositionOffset;

#ifndef NO_RESOURCE_IMPORT
	CCollisionMesh* collisionMesh = const_cast< CCollisionMesh* >( GetCollisionMesh() );
	if( collisionMesh )
	{
		const TDynArray< ICollisionShape*>& collisionShapes = collisionMesh->GetShapes();
		if ( collisionShapes.Size() > 0 )
		{
			return collisionShapes[0]->GetPose().GetTranslation();
		}
	}
#endif

	return Vector::ZEROS;
}

void CMesh::GetChunksForShadowMesh( TDynArray< Uint32 >& outChunks ) const
{
	// no chunks allowed
	if ( !m_mergeInGlobalShadowMesh )
		return;

	// filter chunks
	for ( Uint32 i=0; i<m_chunks.Size(); ++i )
	{
		if ( m_chunks[i].m_useForShadowmesh )
		{
			outChunks.PushBack( i );
		}
	}
}

Bool CMesh::HasCollision() const
{
#ifndef NO_RESOURCE_IMPORT
	if ( m_collisionMesh )
		return true;
#endif

	RED_MESSAGE( "TODO: this is not supporting asynchronous collision cache, STALLS on main thread possible" );
	return GCollisionCache->HasCollision_Sync( GetDepotPath(), GetFileTime() );
}

void CMesh::GetBonePositions( TDynArray< Vector >& positions ) const
{
	const Uint32 numBones = m_boneRigMatrices.Size();

	positions.Resize( numBones );

	for ( Uint32 i = 0; i < numBones; ++i )
	{
		Vector& pos = positions[i];
		const Matrix& m = m_boneRigMatrices[i];

		// This is extracted from doing matrix.Inverted().GetTranslation()
		pos.A[0] = -( m.V[3].A[0] * m.V[0].A[0] + m.V[3].A[1] * m.V[0].A[1] + m.V[3].A[2] * m.V[0].A[2] );
		pos.A[1] = -( m.V[3].A[0] * m.V[1].A[0] + m.V[3].A[1] * m.V[1].A[1] + m.V[3].A[2] * m.V[1].A[2] );
		pos.A[2] = -( m.V[3].A[0] * m.V[2].A[0] + m.V[3].A[1] * m.V[2].A[1] + m.V[3].A[2] * m.V[2].A[2] );
		pos.A[3] = 1.0f;
	}
}

void CMesh::SetCookedData( const SMeshCookedData& data )
{
	m_cookedData = data;
}

static Red::Threads::CMutex st_meshDataLock;
static Uint64 st_totalMeshUnpackedData = 0;
static TDynArray< THandle< CMesh > > st_unpackedMeshes;

MeshUnpackedDataHandle CMesh::GetUnpackedData() const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock ( st_meshDataLock );

	// create and cache unpacked data if needed
	if ( !m_cachedUnpackedChunks )
	{		
		Uint64 consumedMemory = 0;

		// extract actual data
		if ( m_rawVertices.IsValid() && m_rawIndices.IsValid() )
		{
			// load mesh data
			BufferHandle loadedVerticesData = m_rawVertices.AcquireBufferHandleSync();
			BufferHandle loadedIndicesData = m_rawIndices.AcquireBufferHandleSync();

			// get loaded vertex data
			const SMeshVertex* loadedVertices = (const SMeshVertex*) loadedVerticesData->GetData();
			const Uint32 numLoadedVertices = loadedVerticesData->GetSize() / sizeof(SMeshVertex);

			// get loaded index data
			const Uint16* loadedIndices = (const Uint16*) loadedIndicesData->GetData();
			const Uint32 numLoadedIndices = loadedIndicesData->GetSize() / sizeof(Uint16);

			// create unpacked data
			const auto& packedChunks = GetChunks();
			m_cachedUnpackedChunks.Reset( new MeshUnpackedData() );

			// unpack chunks
			m_cachedUnpackedChunks->Resize( packedChunks.Size() );
			for ( Uint32 i=0; i<packedChunks.Size(); ++i )
			{
				const SMeshChunkPacked& srcChunk = packedChunks[i];
				SMeshChunk& destChunk = (*m_cachedUnpackedChunks)[i];

				// copy common stuff
				destChunk.m_vertexType = srcChunk.m_vertexType;
				destChunk.m_materialID = srcChunk.m_materialID;
				destChunk.m_numBonesPerVertex = srcChunk.m_numBonesPerVertex;
				destChunk.m_renderMask = srcChunk.m_renderMask;

				// extract data
				if ( (srcChunk.m_numVertices + srcChunk.m_firstVertex <= numLoadedVertices) && 
					(srcChunk.m_numIndices + srcChunk.m_firstIndex <= numLoadedIndices) )
				{
					// copy vertices
					const Uint32 vertexDataSize = sizeof(SMeshVertex) * srcChunk.m_numVertices;
					const SMeshVertex* vertexDataPtr = loadedVertices + srcChunk.m_firstVertex;
					destChunk.m_vertices.Resize( srcChunk.m_numVertices );
					Red::MemoryCopy( destChunk.m_vertices.Data(), vertexDataPtr, vertexDataSize );

					// copy indices
					const Uint32 indexDataSize = sizeof(Uint16) * srcChunk.m_numIndices;
					const Uint16* indexDataPtr = loadedIndices + srcChunk.m_firstIndex;
					destChunk.m_indices.Resize( srcChunk.m_numIndices );
					Red::MemoryCopy( destChunk.m_indices.Data(), indexDataPtr, indexDataSize );
				}

				// use actual counts
				destChunk.m_numVertices = destChunk.m_vertices.Size();
				destChunk.m_numIndices = destChunk.m_indices.Size();

				// count memory
				consumedMemory += destChunk.m_vertices.DataSize();
				consumedMemory += destChunk.m_indices.DataSize();
			}

			// unload source data
			loadedVerticesData.Reset();
			loadedIndicesData.Reset();
		}
		else
		{
			// create empty chunk list
			m_cachedUnpackedChunks.Reset( new MeshUnpackedData() );
		}

		// count memory
		st_totalMeshUnpackedData += consumedMemory;

		// limit memory usage to 256MB
		if ( st_totalMeshUnpackedData > (1024*1024*256) && st_unpackedMeshes.Size() > 0 )
		{
			// Need to run through the array backwards since we modify it during the loop
			for ( Int64 i = st_unpackedMeshes.Size() - 1; i >= 0; --i )
			{
				const auto& mesh = st_unpackedMeshes[ static_cast< Uint32 >( i ) ];
				if ( mesh && mesh != this )
				{
					mesh->ClearUnpackedData();
				}
			}

			st_unpackedMeshes.ClearFast();
		}

		// add to the list of unpacked meshes
		st_unpackedMeshes.PushBack( this );
	}

	// return cached data
	return m_cachedUnpackedChunks;
}

#ifndef NO_RESOURCE_IMPORT

namespace
{
	void UnpackIndices( GpuApi::eBufferChunkType type, const void* indexData, Uint32 numIndices, Uint16* outIndices )
	{
		if ( type == GpuApi::BCT_IndexUShort )
		{
			Red::System::MemoryCopy( outIndices, indexData, numIndices * sizeof( Uint16 ) );
		}
		else if ( type == GpuApi::BCT_IndexUInt )
		{
			// TODO: (maybe)
		}
		else
		{
			HALT( "Invalid index pack format" );
		}
	}


	class CRawBufferReader : public GpuApi::VertexPacking::IBufferReader
	{
		const Uint8* m_data;
		Uint32 m_offset;

	public:
		CRawBufferReader( const Uint8* data )
			: m_data( data )
			, m_offset( 0 )
		{}


		// Get current offset
		virtual Uint32 GetOffset() const override
		{
			return m_offset;
		}

		virtual void Align( Uint32& size ) override
		{
			HALT( "NOT IMPLEMENTED" );
		}

		virtual void ReadFloat( Float& data ) override
		{
			Read( &data, sizeof( data ) );
		}

		virtual void ReadUnsignedByte( Uint8& data ) override
		{
			Read( &data, sizeof( data ) );
		}

		virtual void ReadUnsignedByteN( Float& data ) override
		{
			Uint8 tmp;
			ReadUnsignedByte( tmp );
			data = (Float)tmp / 255.f;
		}

		virtual void ReadSignedByteN( Float& data ) override
		{
			Uint8 tmp;
			ReadUnsignedByte( tmp );
			data = ( (Float)data - 127.f ) / 127.f;
		}

		virtual void ReadUnsignedShort( Uint16& data ) override
		{
			Read( &data, sizeof( data ) );
		}

		virtual void ReadUnsignedShortN( Float& data ) override
		{
			Uint16 tmp;
			ReadUnsignedShort( tmp );
			data = (Float)tmp / NumericLimits< Uint16 >::Max();
		}

		virtual void ReadUnsignedInt( Uint32& data ) override 
		{
			Read( &data, sizeof( data ) );
		}

		virtual void ReadFloat16( Float& f ) override
		{
			Uint16 tmp;
			ReadUnsignedShort( tmp );
			f = Float16Compressor::Decompress( tmp );
		}

		virtual void ReadDec3N( Float& x, Float& y, Float& z ) override
		{
			Uint32 tmp;
			ReadUnsignedInt( tmp );
			x = (Float)( tmp & 1023 ) / 1023;
			y = (Float)( ( tmp >> 10 ) & 1023 ) / 1023;
			z = (Float)( ( tmp >> 20 ) & 1023 ) / 1023;
		}

		virtual void ReadDec4N( Float& x, Float& y, Float& z, Float& w ) override
		{
			Uint32 tmp;
			ReadUnsignedInt( tmp );
			x = (Float)( tmp & 1023 ) / 1023;
			y = (Float)( ( tmp >> 10 ) & 1023 ) / 1023;
			z = (Float)( ( tmp >> 20 ) & 1023 ) / 1023;
			w = (Float)( ( tmp >> 30 ) & 1023 ) / 1023;
		}

		virtual void ReadColor( Uint32& data ) override
		{
			Read( &data, sizeof( data ) );
		}

	private:
		void Read( void* dest, Uint32 size )
		{
			Red::MemoryCopy( dest, m_data + m_offset, size );
			m_offset += size;
		}
	};
}

void CMesh::UncookData( BufferHandle renderData )
{
	// load raw data
	Uint8* rawIndexData = (Uint8*) renderData->GetData()  + m_cookedData.m_indexBufferOffset;
	Uint8* rawVertexData = (Uint8*) renderData->GetData() + 0;

	// load rendering chunks
	TDynArray< CRenderMesh::Chunk > renderChunks;
	CMemoryFileReaderExternalBuffer systemDataReader( m_cookedData.m_renderChunks.Data(), (Uint32)m_cookedData.m_renderChunks.DataSize() );
	systemDataReader << renderChunks;

	// count chunk vertices & indices in original mesh
	Uint32 numVertices = 0;
	Uint32 numIndices = 0;
	for ( const auto& chunk : m_chunks )
	{
		numVertices += chunk.m_numVertices;
		numIndices += chunk.m_numIndices;
	}

	// create memory in raw buffer
	TDynArray< SMeshVertex > rawVertices( numVertices );
	TDynArray< Uint16 >      rawIndices ( numIndices );
 
 	// extract data from chunks
	for ( const auto& renderChunk : renderChunks )
	{
		// do not process merged chunks - they are generated during cooking and are not part of source data!
 		//if ( renderChunk.m_mergedRenderMask != 0 )
 		//	continue;

		// find source chunk 
		const SMeshChunkPacked* sourceChunk = nullptr;
		for ( Uint32 i=0; i<m_lodLevelInfo.Size(); ++i )
		{
			for ( const auto& it : m_chunks )
			{
				if ( it.m_materialID == renderChunk.m_materialId 
					&& renderChunk.m_numVertices == it.m_numVertices
					&& renderChunk.m_numIndices  == it.m_numIndices
					&& (renderChunk.m_lodMask & (1 << i) ) 
					)
				{
					sourceChunk = &it;
					break;
				}
			}
 
			if ( sourceChunk != nullptr )
			{
				const Uint16* packedIndexData = (const Uint16*)( rawIndexData + renderChunk.m_chunkIndices.byteOffset );

 				Uint16* targetIndexData = &rawIndices[ sourceChunk->m_firstIndex ];
 				SMeshVertex* targetVertexData = &rawVertices[ sourceChunk->m_firstVertex ];

				UnpackIndices( renderChunk.m_chunkIndices.type, packedIndexData, renderChunk.m_numIndices, targetIndexData );

				GpuApi::VertexPacking::PackingParams packingParams;
				const Box& meshBounds = GetBoundingBox();
				GpuApi::VertexPacking::InitPackingParams( 
					packingParams,
					meshBounds.Min.X, meshBounds.Min.Y, meshBounds.Min.Z,
					meshBounds.Max.X, meshBounds.Max.Y, meshBounds.Max.Z );

				GpuApi::VertexPacking::PackingVertexLayout vertexLayout = MeshUtilities::GetVertexLayout();

				auto packingElement = GpuApi::GetPackingForFormat( renderChunk.m_chunkVertices.type );
				Uint32 streamCount = GpuApi::VertexPacking::GetStreamCount( packingElement );
				for ( Uint32 streamIndex = 0; streamIndex < streamCount; ++streamIndex )
				{
					Uint8* packedVertexData = rawVertexData + renderChunk.m_chunkVertices.byteOffsets[ streamIndex ];
					CRawBufferReader reader( packedVertexData );
					GpuApi::VertexPacking::UnpackVertices( &reader, packingElement, packingParams, vertexLayout, targetVertexData, renderChunk.m_numVertices, streamIndex );
				}
 			}
 		}
 	}

	// Note: either of those will work
	//m_cookedData.m_renderBuffer.SetBufferContent( renderData->GetData(), renderData->GetSize() );
	ClearFlag( OF_WasCooked );

	RebuildMaterialMap();

	ClearUnpackedData();
	m_rawVertices.SetBufferContent( rawVertices.Data(), (Uint32)rawVertices.DataSize() );
	m_rawIndices.SetBufferContent( rawIndices.Data(), (Uint32)rawIndices.DataSize() );

	CreateRenderResource();
}

#endif //NO_RESOURCE_IMPORT

void CMesh::ClearUnpackedData()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock ( st_meshDataLock );

	if ( m_cachedUnpackedChunks )
	{
		Uint64 consumedMemory = 0;
		const Uint32 numChunks = m_cachedUnpackedChunks->Size();
		for ( Uint32 i=0; i<numChunks; ++i )
		{
			SMeshChunk& destChunk = (*m_cachedUnpackedChunks)[i];
			consumedMemory += destChunk.m_vertices.DataSize();
			consumedMemory += destChunk.m_indices.DataSize();
		}

		m_cachedUnpackedChunks.Reset();
		st_totalMeshUnpackedData -= consumedMemory;
		st_unpackedMeshes.Remove( this );
	}
}

CMesh::BatchOperation::BatchOperation( CMesh* mesh )
	: m_mesh( mesh )
	, m_enabled( true )
{
	++m_mesh->m_batchOperationCounter;
}

CMesh::BatchOperation::~BatchOperation()
{
	if ( m_enabled )
	{
		if ( !--m_mesh->m_batchOperationCounter )
		{
			m_mesh->CreateRenderResource();
		}
	}
}

void CMesh::BatchOperation::Disable()
{
	if ( m_enabled )
	{
		--m_mesh->m_batchOperationCounter;
		m_enabled = false;
	}
}


namespace MeshUtilities
{
	GpuApi::eBufferChunkType GetVertexTypeForMeshChunk( const Bool extraStreams, const SMeshChunk& sourceChunk )
	{
		// Get vertex type
		switch ( sourceChunk.m_vertexType )
		{
			case MVT_StaticMesh: return extraStreams ? GpuApi::BCT_VertexMeshStaticExtended : GpuApi::BCT_VertexMeshStaticSimple;
			case MVT_DestructionMesh: return extraStreams ? GpuApi::BCT_VertexMeshSkinnedExtended : GpuApi::BCT_VertexMeshSkinnedSimple;
			case MVT_SkinnedMesh: return extraStreams ? GpuApi::BCT_VertexMeshSkinnedExtended : GpuApi::BCT_VertexMeshSkinnedSimple;
			default: RED_HALT( "Unsupported chunk vertex type" );
		}

		return GpuApi::BCT_VertexMeshStaticSimple;
	}

	EMaterialVertexFactory GetVertexFactoryForMeshChunk( const SMeshChunk& sourceChunk )
	{
		// Get vertex type
		switch ( sourceChunk.m_vertexType )
		{
			case MVT_StaticMesh: return MVF_MeshStatic;
			case MVT_SkinnedMesh: return MVF_MeshSkinned;
			case MVT_DestructionMesh: return MVF_MeshDestruction;
			default: RED_HALT( "Unsupported chunk vertex type" );
		}

		return MVF_MeshStatic;
	}

	GpuApi::VertexPacking::PackingVertexLayout GetVertexLayout()
	{
		GpuApi::VertexPacking::PackingVertexLayout vertexLayout;
		vertexLayout.m_stride = sizeof( SMeshVertex );

		// Standard streams
		vertexLayout.Init( offsetof( SMeshVertex, m_position ),	GpuApi::VertexPacking::PS_Position,		0, GpuApi::VertexPacking::PT_Float3 );
		vertexLayout.Init( offsetof( SMeshVertex, m_normal ),	GpuApi::VertexPacking::PS_Normal,		0, GpuApi::VertexPacking::PT_Float3 );
		vertexLayout.Init( offsetof( SMeshVertex, m_tangent ),	GpuApi::VertexPacking::PS_Tangent,		0, GpuApi::VertexPacking::PT_Float3 );
		vertexLayout.Init( offsetof( SMeshVertex, m_binormal ),	GpuApi::VertexPacking::PS_Binormal,		0, GpuApi::VertexPacking::PT_Float3 );
		vertexLayout.Init( offsetof( SMeshVertex, m_uv0 ),		GpuApi::VertexPacking::PS_TexCoord,		0, GpuApi::VertexPacking::PT_Float2 );
		vertexLayout.Init( offsetof( SMeshVertex, m_uv1 ),		GpuApi::VertexPacking::PS_TexCoord,		1, GpuApi::VertexPacking::PT_Float2 );
		vertexLayout.Init( offsetof( SMeshVertex, m_color),		GpuApi::VertexPacking::PS_Color,		0, GpuApi::VertexPacking::PT_Color  );
		vertexLayout.Init( offsetof( SMeshVertex, m_indices ),	GpuApi::VertexPacking::PS_SkinIndices,	0, GpuApi::VertexPacking::PT_UByte4 );
		vertexLayout.Init( offsetof( SMeshVertex, m_weights ),	GpuApi::VertexPacking::PS_SkinWeights,	0, GpuApi::VertexPacking::PT_Float4 );

		// Special streams
		vertexLayout.Init( offsetof( SMeshVertex, m_extraData0 ),	GpuApi::VertexPacking::PS_ExtraData,	0, GpuApi::VertexPacking::PT_Float4 );
		vertexLayout.Init( offsetof( SMeshVertex, m_extraData1 ),	GpuApi::VertexPacking::PS_ExtraData,	1, GpuApi::VertexPacking::PT_Float4 );
		vertexLayout.Init( offsetof( SMeshVertex, m_extraData2 ),	GpuApi::VertexPacking::PS_ExtraData,	2, GpuApi::VertexPacking::PT_Float4 );
		vertexLayout.Init( offsetof( SMeshVertex, m_extraData3 ),	GpuApi::VertexPacking::PS_ExtraData,	3, GpuApi::VertexPacking::PT_Float4 );

		return vertexLayout;
	}
}

