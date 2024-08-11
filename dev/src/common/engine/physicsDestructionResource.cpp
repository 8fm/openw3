/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/


#include "build.h"
#include "physicsDestructionResource.h"
#include "collisionMesh.h"
#include "collisionCache.h"
#include "renderer.h"

IMPLEMENT_ENGINE_CLASS( CPhysicsDestructionResource );
IMPLEMENT_ENGINE_CLASS( SPhysicsDestructionAdditionalInfo );
IMPLEMENT_ENGINE_CLASS( SBoneIndiceMapping );

CPhysicsDestructionResource::CPhysicsDestructionResource(void)
{
}


CPhysicsDestructionResource::~CPhysicsDestructionResource(void)
{
}

CompiledCollisionPtr CPhysicsDestructionResource::CompileCollision(CObject* parent) const
{
	return m_collisionMesh->CompileCollision( parent );
}

void CPhysicsDestructionResource::OnSerialize(IFile& file)
{
	TBaseClass::OnSerialize( file );
	
	// GC
	if ( file.IsGarbageCollector() )
	{
		return;
	}

	// Write to file
	if( file.IsWriter() )
	{
		// Write number of events
		Uint32 numOfAdditionalInfos = m_additionalInfo.Size();
		file << numOfAdditionalInfos;

		// Serialize events
		for( auto addIter = m_additionalInfo.Begin(), addEnd = m_additionalInfo.End();
			addIter != addEnd; ++addIter )
		{
			SPhysicsDestructionAdditionalInfo info = *addIter;

			CFileSkipableBlock block( file );
			
			file.SerializeSimpleType( &info.m_simType, sizeof(EPhysicsDestructionSimType) );
			file.Serialize( &info.m_initialVelocity, sizeof(Vector) );
			file.SerializeSimpleType( &info.m_overrideCollisionMasks, sizeof( Bool ) );
			info.m_collisionType.Serialize( file );
		}
	}
	// Read from file
	else if ( file.IsReader() )
	{
		// Read number of events
		Uint32 serializedEvents = 0;
		file << serializedEvents;

		// Read events
		for( Uint32 i = 0; i < serializedEvents; ++i )
		{
			CFileSkipableBlock block( file );

			// Read type
			SPhysicsDestructionAdditionalInfo info;
			file.SerializeSimpleType( &info.m_simType, sizeof(EPhysicsDestructionSimType) );
			file.Serialize( &info.m_initialVelocity, sizeof(Vector) );
			file.SerializeSimpleType( &info.m_overrideCollisionMasks, sizeof( Bool ) );
			info.m_collisionType.Serialize( file );

			m_additionalInfo.PushBack( info );
		}
	}
}

SBoneIndicesHelper CPhysicsDestructionResource::GetIndicesForActiveBones( const TDynArray< Bool >& bonesActive )
{
	SBoneIndicesHelper helper;
	TDynArray< Uint16 > indices;

	// Reserve the max size of indices array - we'll shrink it later
	helper.m_activeIndices.Reserve( m_finalIndices.Size() );

	// Resize arrays for chunk info - we know the actual size
	RED_ASSERT( m_chunkNumber > 0 , TXT("CPhysicsDestructionResource [%ls] - no chunks in the resource. Possibly not reimported or didn't get into patch. Check that, please!"), GetFriendlyName().AsChar() );
	if( m_chunkNumber <= 0 )
	{
		return helper;
	}
	helper.m_chunkOffsets.Resize( m_chunkNumber );
	helper.m_chunkNumIndices.Resize( m_chunkNumber );

	Uint32 currentIndexOffset = 0;
	Uint32 currentChunkIndex = 0;

	Red::MemoryZero( &helper.m_chunkOffsets[0]		, m_chunkNumber * sizeof( Uint32 ) );
	Red::MemoryZero( &helper.m_chunkNumIndices[0]	, m_chunkNumber * sizeof( Uint32 ) );

	for( Uint32 i = 0, size = m_boneIndicesMapping.Size(); i < size; ++i )
	{
		const auto& boneMap = m_boneIndicesMapping[ i ];

		// If the bone is active (not faded out completely), copy it's indices into the new index buffer
		if( bonesActive[ boneMap.m_boneIndex ] )
		{
			currentChunkIndex = boneMap.m_chunkIndex;

			Uint32 indicesCount = boneMap.m_endingIndex - boneMap.m_startingIndex + 1;
			Uint32 offset = indicesCount * sizeof( Uint16 );

			// ResizeFast the array so the m_size is proper after we mem copy stuff into array
			helper.m_activeIndices.ResizeFast( currentIndexOffset + indicesCount );

			Red::MemoryCopy( &helper.m_activeIndices[ currentIndexOffset ], &m_finalIndices[ boneMap.m_startingIndex ] , offset  );
			currentIndexOffset += indicesCount;

			// Update per chunk offsets and indice count
			// Each chunk following the current one should have it's offset updated by the size
			// of indices added for the current chunk
			for( Uint32 j = currentChunkIndex + 1; j < m_chunkNumber; ++j )	
			{
				helper.m_chunkOffsets[ j ] += offset;
			}
			helper.m_chunkNumIndices[ currentChunkIndex ] += indicesCount;	
		}
		
	}

	// Shrink indices array
	helper.m_activeIndices.Shrink();

	return helper;
}

SPhysicsDestructionAdditionalInfo* CPhysicsDestructionResource::GetAdditionalInfoPtr(Uint16 index)
{
	if( m_additionalInfo.Size() <= index ) 
	{
		return nullptr;
	}
	return &m_additionalInfo[index];
}

const TDynArray<SPhysicsDestructionAdditionalInfo>& CPhysicsDestructionResource::GetAdditionalInfoArray()
{
	return m_additionalInfo;
}

EPhysicsDestructionSimType CPhysicsDestructionResource::GetSimTypeForChunk(Uint16 index)
{
	if( m_additionalInfo.Size() <= index ) 
	{
		return EPDST_Dynamic;
	}
	return m_additionalInfo[ index ].m_simType;
}

Vector CPhysicsDestructionResource::GetInitialVelocityForChunk(Uint16 chunkResIndex)
{
	if( m_additionalInfo.Size() <= chunkResIndex ) 
	{
		return Vector::ZEROS;
	}
	return m_additionalInfo[ chunkResIndex ].m_initialVelocity;
}

#ifndef NO_RESOURCE_COOKING
void CPhysicsDestructionResource::OnCook(ICookerFramework& cooker)
{
	TBaseClass::OnCook( cooker );
	//TODO: AD
}

CPhysicsDestructionResource* CPhysicsDestructionResource::Create(CPhysicsDestructionResource::FactoryInfo data)
{
	CPhysicsDestructionResource* obj = data.CreateResource();
	ASSERT( !data.m_reuse || obj == data.m_reuse );

	obj->m_authorName			= data.m_authorName;
	obj->m_baseResourceFilePath	= data.m_baseResourceFilePath;
	obj->m_entityProxy			= data.m_entityProxy;

	if ( !data.m_reuseMesh )
	{
		// Basic data
		obj->m_boneNames			= data.m_boneNames;
		obj->m_boneRigMatrices		= data.m_boneRigMatrices;
		obj->m_boneVertexEpsilons	= data.m_boneVertexEpsilons;
		obj->m_chunkNumber			= data.m_chunks.Size();

		// Create materials
		TDynArray< Uint32 > materialRemappingTable( data.m_materialNames.Size() );
		for ( Uint32 i=0; i<data.m_materialNames.Size(); i++ )
		{
			// Map imported material 
			const String& newMaterialName = data.m_materialNames[i];
			Uint32 materialIndex = obj->MapMaterial( newMaterialName );

			// Remember remapped index
			materialRemappingTable[ i ] = materialIndex;
		}

		struct BoneIndiceMappingSorter
		{
			static RED_INLINE Bool Less( SBoneIndiceMapping const & a, SBoneIndiceMapping const & b )
			{
				return a.m_startingIndex < b.m_startingIndex;
			}
		};

		TSortedArray< SBoneIndiceMapping, BoneIndiceMappingSorter> boneIndicesMapping;
		boneIndicesMapping.Reserve( data.m_boneNames.Size() );
		TDynArray< TDynArray< Uint16 > > tempIndiceArrays; // Used to fix the m_finalIndices to have the proper order.
		tempIndiceArrays.Reserve( data.m_boneNames.Size() );

		// Remap imported chunk materials
		Uint32 indexOffset = 0;
		TDynArray< Uint16 > finalIndices;
		

		for ( Uint32 i = 0; i < data.m_chunks.Size(); ++i )
		{
			SMeshChunk& chunk = const_cast< SMeshChunk& >( data.m_chunks[i] );
			ASSERT( chunk.m_materialID < materialRemappingTable.Size() );
			chunk.m_materialID = materialRemappingTable[ chunk.m_materialID ];
			ASSERT( chunk.m_materialID < obj->GetMaterials().Size() );

			const TDynArray< SMeshVertex, MC_BufferMesh >& vertexes = chunk.m_vertices;
			struct LastVertPerBoneHelper
			{
				Uint16 m_vertIndex;
				Uint8 m_boneIndex;
			};
			TDynArray< LastVertPerBoneHelper > lastVertPerBone;
			lastVertPerBone.Reserve( vertexes.Size() );

			if( vertexes.Size() == 0 )
			{
				continue;
			}

			LastVertPerBoneHelper current;
			current.m_vertIndex = 0;
			current.m_boneIndex = vertexes[0].m_indices[0];

			for( Uint32 j = 0, size = vertexes.Size(); j <= size; ++j )
			{
				if( j == size || vertexes[j].m_indices[0] != current.m_boneIndex)
				{
					current.m_vertIndex = j - 1;
					lastVertPerBone.PushBack( current );
					if( j != size )
					{
						current.m_boneIndex = vertexes[j].m_indices[0];
					}
				}
			}
			const TDynArray< Uint16, MC_BufferMesh >& indices = chunk.m_indices;
			Uint32 startingIndex = indexOffset;
			Uint32 endingIndex = indexOffset;
			Uint32 currentIndex =  0; 

			// Create bone mappings for current chunk.
			TDynArray< Uint16 > indicesTemp;
			for(Uint32 j = 0, size = indices.Size(); j <= size; j++ )
			{		
				if(  j == size || indices[j] > lastVertPerBone[ currentIndex ].m_vertIndex )
				{
					endingIndex = j + indexOffset - 1;
					boneIndicesMapping.PushBack( SBoneIndiceMapping( startingIndex, endingIndex, i, lastVertPerBone[ currentIndex ].m_boneIndex ) );
					tempIndiceArrays.PushBack( Move( indicesTemp ) );
					startingIndex = j + indexOffset;					
					++currentIndex;
					indicesTemp.Clear();
				}
				if(  j < size )
				{
					indicesTemp.PushBackUnique( indices[j] );
				}
			}	

			indexOffset += chunk.m_indices.Size();
		}

		--indexOffset;
		obj->m_boneIndicesMapping.Clear();
		obj->m_boneIndicesMapping = boneIndicesMapping;


		// Create mesh data, must be last (we need mapped materials first)
		if ( !obj->SetMeshData( data.m_chunks, data.m_lodLevels, false ) )
		{
			return false;
		}

		// Sort finalIndices bone wise. Without it, when we get the reduced index buffer for active bones, it will have proper
		// chunk offsets and indices counts, but wrong data in it (which will cause a mess instead of the mesh we'd expect).
		// TODO: AD - This could probably be done better and faster. Take a look at it if there's time. 
		// If no - it's only during asset import, so maybe not such a big deal.
		Uint32 indiceOffset = 0;
		for ( Uint32 i = 0; i < data.m_chunks.Size(); ++i )
		{
			SMeshChunk& chunk = const_cast< SMeshChunk& >( data.m_chunks[i] );
			for( Uint32 ind = 0, indSize = boneIndicesMapping.Size(); ind < indSize; ++ind )
			{
				SBoneIndiceMapping& boneMap = boneIndicesMapping[ind];
				if( boneMap.m_chunkIndex != i ) continue;
				const TDynArray< Uint16 >& indArray = tempIndiceArrays[ ind ];
				for( Uint32 finalInd = 0, sizeFinalInd = chunk.m_numIndices; finalInd < sizeFinalInd; ++finalInd )
				{
					Uint16 indice  = obj->m_finalIndices[ finalInd + indiceOffset ];
					if( indArray.FindPtr( indice ) )
					{
						finalIndices.PushBack( indice );
					}
				}
			}
			indiceOffset += chunk.m_numIndices;
		}
		
		obj->m_finalIndices = Move( finalIndices );
	}

	if ( !data.m_reuseVolumes )
	{
		obj->RemoveCollision();

		// Build or import collision
		if ( data.m_buildConvexCollision || ( data.m_importCollision && data.m_collisionMeshes.Empty() ) )
		{
			// Build convex ( forced )
			obj->AddConvexCollision();
		}
		else if ( data.m_importCollision && !data.m_collisionMeshes.Empty() )
		{
			// Create new collision
			CCollisionMesh* collisionMesh = ::CreateObject< CCollisionMesh>( obj );
			ASSERT( collisionMesh );

			const CName* names = obj->GetBoneNames();
			obj->m_additionalInfo.Resize( data.m_collisionMeshes.Size() );


			// Add parts
			for ( Uint32 i=0; i<data.m_collisionMeshes.Size(); i++ )
			{
				const CollisionMesh& partMesh = data.m_collisionMeshes[i];
				obj->m_additionalInfo[i] = SPhysicsDestructionAdditionalInfo();
				obj->m_additionalInfo[i].m_simType = EPDST_Dynamic;
				obj->m_additionalInfo[i].m_initialVelocity = Vector::ZEROS;
				obj->m_additionalInfo[i].m_overrideCollisionMasks = false;

				if ( !partMesh.m_vertices.Empty() )
				{
					const Vector* vertices = partMesh.m_vertices.TypedData();

					if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Convex )
					{
						collisionMesh->AddConvex( Matrix::IDENTITY, vertices, partMesh.m_vertices.Size(), partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Box )
					{
						collisionMesh->AddBox( Matrix( vertices[ 0 ], vertices[ 1 ], vertices[ 2 ], vertices[ 3 ] ), vertices[ 4 ], partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Sphere )
					{
						collisionMesh->AddSphere( Matrix( vertices[ 0 ], vertices[ 1 ], vertices[ 2 ], vertices[ 3 ] ), vertices[ 4 ].X, partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Capsule )
					{
						collisionMesh->AddCapsule( Matrix( vertices[ 0 ], vertices[ 1 ], vertices[ 2 ], vertices[ 3 ] ), vertices[ 4 ].X, vertices[ 4 ].Z, partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( !partMesh.m_indices.Empty() )
					{
						const Uint32* indices = partMesh.m_indices.TypedData();
						if( partMesh.m_physicalMaterialIndexes.Empty() )
						{
							collisionMesh->AddTriMesh( Matrix::IDENTITY, vertices, partMesh.m_vertices.Size(), indices, partMesh.m_indices.Size() / 3 );
						}
						else
						{
							collisionMesh->AddTriMesh( Matrix::IDENTITY, vertices, partMesh.m_vertices.Size(), indices, partMesh.m_indices.Size() / 3, partMesh.m_physicalMaterialIndexes, partMesh.m_physicalMaterials );
						} 
					}
				}
			}

			// Set as new collision mesh for imported mesh
			obj->m_collisionMesh = collisionMesh;
		}

#ifndef NO_OBSTACLE_MESH_DATA
		obj->m_navigationObstacle = data.m_navigationObstacle;
#endif
	}

	// Rebuild shadow info
	obj->RecomputeDefaultRenderMask();
	obj->RecomputeShadowFadeDistance();

	// Recreate rendering data
	obj->CreateRenderResource();
	return obj;	
}

void CPhysicsDestructionResource::SetSimTypeForChunk(Uint16 index, EPhysicsDestructionSimType simType)
{
	if( m_additionalInfo.Size() <= index ) 
	{
		return;
	}
	m_additionalInfo[ index ].m_simType = simType;
}

void CPhysicsDestructionResource::DecideToIncludeToCollisionCache()
{
	CompiledCollisionPtr compiled;
	GCollisionCache->Compile( compiled, this, GetDepotPath(), GetFileTime() );
}

#endif
