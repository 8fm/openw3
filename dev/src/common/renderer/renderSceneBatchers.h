/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderProxyDrawable.h"
#include "renderElementMeshChunk.h"
#include "renderVisibilityQuery.h"
#include "renderMesh.h"
#include "renderMaterial.h"
#include "renderOcclusion.h"

/// Group fragments by material params
template< class T >
class CRenderBatchByMaterialParams
{
public:
	CRenderMaterialParameters*		m_firstMaterial;		//!< First material in batch list

public:
	//! Group fragments by material
	RED_INLINE CRenderBatchByMaterialParams( T* batchList )
		: m_firstMaterial( NULL )
	{
		T* next = NULL;
		for ( T* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast< T* >( cur->GetBatchNext() );

			// Get the fragments material
			CRenderMaterialParameters* material = cur->GetMaterialParams();
			if ( material )
			{
				// New on list
				if ( !material->m_batchList )
				{
					material->m_batchNext = m_firstMaterial;
					m_firstMaterial = material;
				}	

				// Add to material's batch list
				cur->SetBatchNext( material->m_batchList );
				material->m_batchList = cur;
			}
		}
	}

	//! Group fragments by material, from input array
	RED_INLINE CRenderBatchByMaterialParams( const TDynArray< T* >& batchList )
		: m_firstMaterial( NULL )
	{
		const Uint32 size = batchList.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			// Get the current element
			T* cur = batchList[i];

			// Get the fragments material
			CRenderMaterialParameters* material = cur->GetMaterialParams();
			if ( material )
			{
				// New on list
				if ( !material->m_batchList )
				{
					material->m_batchNext = m_firstMaterial;
					m_firstMaterial = material;
				}	

				// Add to material's batch list
				cur->SetBatchNext( material->m_batchList );
				material->m_batchList = cur;
			}
		}
	}

	//! Pop the grouped fragment list
	RED_INLINE Bool Pop( CRenderMaterialParameters*& materialParams, CRenderMaterial*& material, T*& batchList )
	{
		// Pop the batch
		if ( m_firstMaterial )
		{
			// Pop the material parameters
			materialParams = m_firstMaterial;

			// Get the
			batchList = static_cast< T* >( m_firstMaterial->m_batchList );
			material = batchList->GetMaterial();

			// Walk the list, unlink
			CRenderMaterialParameters* next = m_firstMaterial->m_batchNext;
			m_firstMaterial->m_batchNext = NULL;
			m_firstMaterial->m_batchList = NULL;
			m_firstMaterial = next;
			return true;
		}

		// No batches left
		return false;
	}
};

/// Group based on whether the material is used for particles or not.
template< class T >
class CRenderBatchByParticleMaterial
{
	T* m_particles;
	T* m_nonParticles;

public:
	//! Group fragments by particle/non-particle materials.
	RED_INLINE CRenderBatchByParticleMaterial( T* batchList )
	{
		m_particles = m_nonParticles = NULL;

		T* next = NULL;
		for ( T* cur = batchList; cur; cur = next )
		{
			next = static_cast< T* >( cur->GetBatchNext() );

			CRenderMaterial* material = cur->GetMaterial();
			if ( material )
			{
				// If it can't be used on meshes, we assume it's a particle material.
				if ( material->CanUseOnMeshes() )
				{
					cur->SetBatchNext( m_nonParticles );
					m_nonParticles = cur;
				}
				else if ( material->CanUseOnParticles() )
				{
					cur->SetBatchNext( m_particles );
					m_particles = cur;
				}
			}
		}
	}

	//! Pop the grouped fragment list
	RED_INLINE Bool Pop( Bool& isParticleMaterial, T*& batchList )
	{
		if ( m_particles )
		{
			isParticleMaterial = true;
			batchList = m_particles;
			m_particles = NULL;
			return true;
		}

		if ( m_nonParticles )
		{
			isParticleMaterial = false;
			batchList = m_nonParticles;
			m_nonParticles = NULL;
			return true;
		}

		return false;
	}
};

/// Group based on whether the material is used for particles or not.
template< class T >
class CRenderBatchByUseDissolve
{
	T* m_solid;
	T* m_dissolve;

public:
	//! Group fragments by whether they need to be dissolved materials.
	RED_INLINE CRenderBatchByUseDissolve( T* batchList, Bool isShadowPass )
	{
		m_solid = m_dissolve = nullptr;

		T* next = nullptr;
		for ( T* cur = batchList; cur; cur = next )
		{
			next = static_cast< T* >( cur->GetBatchNext() );

			if ( cur->IsDissolved( isShadowPass ) )
			{
				cur->SetBatchNext( m_dissolve );
				m_dissolve = cur;
			}
			else
			{
				cur->SetBatchNext( m_solid );
				m_solid = cur;
			}
		}
	}

	//! Pop the grouped fragment list
	RED_INLINE Bool Pop( Bool& isDissolved, T*& batchList )
	{
		if ( m_solid )
		{
			isDissolved = false;
			batchList = m_solid;
			m_solid = nullptr;
			return true;
		}

		if ( m_dissolve )
		{
			isDissolved = true;
			batchList = m_dissolve;
			m_dissolve = nullptr;
			return true;
		}

		return false;
	}
};

/// Group fragments by selection
template< class T >
class CRenderBatchBySelection
{
public:
	T*		m_selectedList;		//!< List of selected proxies
	T*		m_unselectedList;	//!< List of not selected proxies

public:
	//! Group fragments by material
	RED_INLINE CRenderBatchBySelection( T* batchList, Bool supportsSelection )
		: m_selectedList( NULL )
		, m_unselectedList( NULL )
	{
		T* next = NULL;
		for ( T* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast< T* >( cur->GetBatchNext() );

			// Get the fragments material
			IRenderProxyDrawable* proxy = static_cast< IRenderProxyDrawable* >( cur->GetProxy() );
			if ( proxy->IsSelected() && supportsSelection )
			{
				cur->SetBatchNext( m_selectedList );
				m_selectedList = cur;
			}
			else
			{
				cur->SetBatchNext( m_unselectedList );
				m_unselectedList = cur;
			}
		}
	}

	//! Pop the grouped fragment list
	RED_INLINE Bool Pop( Bool& isSelected, T*& batchList )
	{
		// Pop the unselected list
		if ( m_unselectedList )
		{
			isSelected = false;
			batchList = m_unselectedList;
			m_unselectedList = NULL;
			return true;
		}

		// Pop the selected list
		if ( m_selectedList )
		{
			isSelected = true;
			batchList = m_selectedList;
			m_selectedList = NULL;
			return true;
		}

		// No batches left
		return false;
	}
};

/// Group fragments by proxy flags
class CRenderBatchByProxyFlags
{
private:
	// Ordered fine to coarse.
	enum Flag {
		VertexCollapse		= FLAG( 0 ),
		Skinned				= FLAG( 1 ),
		Dissolved			= FLAG( 2 ),

		// How many combinations we have.
		NUM					= FLAG( 3 ),
	};

	CRenderElement_MeshChunk*		m_buckets[NUM];
	Uint32							m_currBucket;
	ERenderMeshChunkFlags			m_dissolveFlag;
	
public:
	//! Group fragments by material
	RED_INLINE CRenderBatchByProxyFlags( CRenderElement_MeshChunk* batchList, ERenderMeshChunkFlags dissolveFlag )
		: m_currBucket( 0 )
		, m_dissolveFlag( dissolveFlag )
	{
		Red::System::MemoryZero( m_buckets, sizeof( m_buckets ) );

		CRenderElement_MeshChunk* next = nullptr;
		for ( CRenderElement_MeshChunk* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast< CRenderElement_MeshChunk* >( cur->GetBatchNext() );

			Uint32 bucket = 0;
			if ( cur->HasFlag( RMCF_UsesVertexCollapse ) )	bucket |= VertexCollapse;
			if ( cur->HasFlag( RMCF_UsesSkinning ) )		bucket |= Skinned;
			if ( cur->HasFlag( m_dissolveFlag ) )			bucket |= Dissolved;
			
			cur->SetBatchNext( m_buckets[ bucket ] );
			m_buckets[ bucket ] = cur;
		}
	}

	//! Pop the grouped fragment list
	//! All elements in the list will have the same flag value for:
	//!    RMCF_UsesSkinning
	//!    RMCF_UsesVertexCollapse
	RED_INLINE Bool Pop( CRenderElement_MeshChunk*& batchList )
	{
		// Advance to non-empty bucket.
		while ( m_currBucket < NUM && m_buckets[ m_currBucket ] == nullptr )
		{
			++m_currBucket;
		}

		// Return the next bucket.
		if ( m_currBucket < NUM )
		{
			batchList = m_buckets[ m_currBucket ];
			++m_currBucket;
			return true;
		}

		// No batches left
		return false;
	}
};

/// Group fragments by mesh
class CRenderBatchByMesh
{
public:
	CRenderMesh*		m_firstMesh;			//!< First mesh in batch list

public:
	//! Group fragments by mesh
	RED_INLINE CRenderBatchByMesh( CRenderElement_MeshChunk* batchList )
		: m_firstMesh( NULL )	
	{
		CRenderElement_MeshChunk* next = NULL;
		for ( CRenderElement_MeshChunk* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast<CRenderElement_MeshChunk* >( cur->GetBatchNext() );

			// Get the fragments material
			CRenderMesh* mesh = cur->GetMesh();
			if ( mesh )
			{
				// New on list
				if ( !mesh->m_batchList )
				{
					mesh->m_batchNext = m_firstMesh;
					m_firstMesh = mesh;
				}

				// Add to mesh's batch list
				cur->SetBatchNext( mesh->m_batchList );
				mesh->m_batchList = cur;
			}
		}
	}

	//! Pop the mesh batch list
	RED_INLINE Bool Pop( CRenderMesh*& mesh, CRenderElement_MeshChunk*& batchList )
	{
		// Pop meshes
		if ( m_firstMesh )
		{
			// Pop the mesh
			mesh = m_firstMesh;
			batchList = static_cast< CRenderElement_MeshChunk* >( m_firstMesh->m_batchList );

			// Walk the list, unlink
			CRenderMesh* next = m_firstMesh->m_batchNext;
			m_firstMesh->m_batchNext = NULL;
			m_firstMesh->m_batchList = NULL;
			m_firstMesh = next;
			return true;
		}

		// No more
		return false;
	}
};

// Group fragments by material vertex factory
class CRenderBatchByMeshVertexFactory
{
public:
	struct FactoryInfo
	{
		FactoryInfo*				m_batchNext;
		CRenderElement_MeshChunk*	m_batchList;
		EMaterialVertexFactory		m_vertexFactory;

		RED_INLINE FactoryInfo()
			: m_batchList( NULL )
			, m_batchNext( NULL )
			, m_vertexFactory( MVF_Invalid )
		{};
	};

	struct FactoryInfoList
	{
		FactoryInfo		m_factories[ MVF_Max ];	// Factories lists, one for each vertex factory

		RED_INLINE FactoryInfoList()
		{
			// Initialize factory enums
			for ( Uint32 i=0; i<MVF_Max; i++ )
			{
				m_factories[i].m_vertexFactory = (EMaterialVertexFactory) i;				
			}
		}
	};

public:
	FactoryInfo*		m_firstFactory;			//!< Per factory batch list	

public:
	//! Group fragments by factory
	RED_INLINE CRenderBatchByMeshVertexFactory( CRenderElement_MeshChunk* batchList )
		: m_firstFactory( NULL )
	{
		static FactoryInfoList theFactoryMap;

		// Distribute fragments
		CRenderElement_MeshChunk* next = NULL;
		for ( CRenderElement_MeshChunk* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast<CRenderElement_MeshChunk* >( cur->GetBatchNext() );

			// Get the vertex factory type
			const EMaterialVertexFactory vertexFactory = cur->GetVertexFactory();
			ASSERT( vertexFactory >= MVF_MeshStatic && vertexFactory <= MVF_MeshSkinned );
			FactoryInfo *info = &theFactoryMap.m_factories[ vertexFactory ];

			// New on list
			if ( !info->m_batchList )
			{
				info->m_batchNext = m_firstFactory;
				m_firstFactory = info;
			}						

			// Add to mesh's batch list
			cur->SetBatchNext( info->m_batchList );
			info->m_batchList = cur;
		}
	}

	//! Pop the static mesh batch list
	RED_INLINE Bool Pop( EMaterialVertexFactory& vertexFactory, CRenderElement_MeshChunk*& batchList )
	{
		// Pop factories
		if ( m_firstFactory )
		{
			// Pop the chunk info
			vertexFactory = m_firstFactory->m_vertexFactory;
			batchList = m_firstFactory->m_batchList;	

			// Walk the list, unlink
			FactoryInfo* next = m_firstFactory->m_batchNext;
			m_firstFactory->m_batchNext = NULL;
			m_firstFactory->m_batchList = NULL;
			m_firstFactory = next;
			return true;
		}

		// No more batches
		return false;
	}
};

// Group fragments by mesh chunk index
class CRenderBatchByMeshChunkIndex
{
public:
	struct ChunkInfo
	{
		ChunkInfo*					m_batchNext;
		CRenderElement_MeshChunk*	m_batchList;
		Uint32						m_chunkIndex;
		Uint32						m_fragCount;

		RED_INLINE ChunkInfo()
			: m_batchList( NULL )
			, m_batchNext( NULL )
			, m_chunkIndex( 0xFFFF )
			, m_fragCount( 0 )
		{};
	};

	enum
	{
		CHUNK_INDICES_RANGE	= 256
	};

	template< int N >
	struct ChunkList
	{
		ChunkInfo	m_chunks[ N ];	// Chunk lists, one for each chunk index

		RED_INLINE ChunkList()
		{
			// Initialize chunk indices
			for ( Uint32 i=0; i<N; i++ )
			{
				m_chunks[i].m_chunkIndex = i;
				m_chunks[i].m_fragCount = 0;
			}
		}
	};

public:
	ChunkInfo*			m_firstChunk;			//!< Per chunk batch list	

public:
	//! Group fragments by mesh
	RED_INLINE CRenderBatchByMeshChunkIndex( CRenderElement_MeshChunk* batchList )
		: m_firstChunk( NULL )
	{
		static ChunkList< CHUNK_INDICES_RANGE >	theChunkMap;

		// Distribute fragments
		CRenderElement_MeshChunk* next = NULL;
		for ( CRenderElement_MeshChunk* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast<CRenderElement_MeshChunk* >( cur->GetBatchNext() );

			// Get the fragments chunk index
			Uint32 chunkIndex = cur->GetChunkIndex();
			ChunkInfo* chunk = &theChunkMap.m_chunks[ chunkIndex ];

			// New on list
			if ( !chunk->m_batchList )
			{
				chunk->m_fragCount = 0;
				chunk->m_batchNext = m_firstChunk;
				m_firstChunk = chunk;
			}						

			// Add to mesh's batch list
			cur->SetBatchNext( chunk->m_batchList );
			chunk->m_batchList = cur;
			chunk->m_fragCount++;
		}
	}

	//! Pop the static mesh batch list
	RED_INLINE Bool Pop( Uint32& chunkIndex, Uint32& fragCount, CRenderElement_MeshChunk*& batchList )
	{
		// Pop
		if ( m_firstChunk )
		{
			// Pop the chunk info
			chunkIndex = m_firstChunk->m_chunkIndex;
			batchList = m_firstChunk->m_batchList;	
			fragCount = m_firstChunk->m_fragCount;

			// Walk the list, unlink
			ChunkInfo* next = m_firstChunk->m_batchNext;
			m_firstChunk->m_batchNext = NULL;
			m_firstChunk->m_batchList = NULL;
			m_firstChunk->m_fragCount = 0;
			m_firstChunk = next;
			return true;
		}

		// No more batches
		return false;
	}
};

/// Group fragments by whether they use a clipping ellipse
template< class T >
class CRenderBatchByClippingEllipse
{
public:
	T*		m_withEllipse;
	T*		m_noEllipse;

public:
	//! Group fragments by clipping ellipse usage
	RED_INLINE CRenderBatchByClippingEllipse( T* batchList )
		: m_withEllipse( nullptr )
		, m_noEllipse( nullptr )
	{
		T* next = nullptr;
		for ( T* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast< T* >( cur->GetBatchNext() );

			// Get the fragments material
			IRenderProxyDrawable* proxy = static_cast< IRenderProxyDrawable* >( cur->GetProxy() );
			if ( proxy->HasClippingEllipse() )
			{
				cur->SetBatchNext( m_withEllipse );
				m_withEllipse = cur;
			}
			else
			{
				cur->SetBatchNext( m_noEllipse );
				m_noEllipse = cur;
			}
		}
	}

	//! Pop the grouped fragment list
	RED_INLINE Bool Pop( Bool& hasClippingEllipse, T*& batchList )
	{
		// Pop non-clipped
		if ( m_noEllipse )
		{
			hasClippingEllipse = false;
			batchList = m_noEllipse;
			m_noEllipse = nullptr;
			return true;
		}

		// Pop clipped
		if ( m_withEllipse )
		{
			hasClippingEllipse = true;
			batchList = m_withEllipse;
			m_withEllipse = nullptr;
			return true;
		}

		// No batches left
		return false;
	}
};

/// Batch list iterator
template< class T >
class CRenderBatchListIterator
{
public:
	IRenderElement*	m_list;

public:
	RED_INLINE CRenderBatchListIterator( IRenderElement* list )
		: m_list( list )
	{};

	//! Pop the fragment to draw
	RED_INLINE Bool Pop( T*& frag )
	{
		// Pop the fragment
		if ( m_list )
		{
			// Pop the fragment
			frag = static_cast< T* >( m_list );

			// Go to next fragment
			IRenderElement* next = m_list->GetBatchNext();
			m_list->SetBatchNext( NULL );
			m_list = next;
			return true;
		}

		// No more fragments
		return false;
	}
};

// Group fragments by light group
class CRenderBatchByLightChannel
{
public:
	struct LightGroupInfo
	{
		LightGroupInfo*				m_batchNext;
		CRenderElement_MeshChunk*	m_batchList;
		Uint8						m_lightChannel;

		RED_INLINE LightGroupInfo()
			: m_batchList( NULL )
			, m_batchNext( NULL )
			, m_lightChannel( 0 )
		{};
	};

	struct LightGroupList
	{
		LightGroupInfo	m_lightGroup[ 256 ];	// Chunk lists, one for each chunk index

		RED_INLINE LightGroupList()
		{
			// Initialize chunk indices
			for ( Uint32 i=0; i<256; i++ )
			{
				m_lightGroup[i].m_lightChannel = (Uint8)i;
			}
		}
	};

public:
	LightGroupInfo*		m_firstChunk;			//!< Per chunk batch list	

public:
	//! Group fragments by mesh
	RED_INLINE CRenderBatchByLightChannel( CRenderElement_MeshChunk* batchList, Bool useLightChannels )
		: m_firstChunk( NULL )
	{
		static LightGroupList	theChunkMap;

		// Distribute fragments
		CRenderElement_MeshChunk* next = NULL;
		for ( CRenderElement_MeshChunk* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast<CRenderElement_MeshChunk* >( cur->GetBatchNext() );

			// Get the fragments chunk index
			IRenderProxyDrawable* proxy = cur->GetProxy();			
			const Uint32 lightChannel = useLightChannels ? ( proxy->GetLightChannels() ): 0 ;
			LightGroupInfo* chunk = &theChunkMap.m_lightGroup[ lightChannel ];

			// New on list
			if ( !chunk->m_batchList )
			{
				chunk->m_batchNext = m_firstChunk;
				m_firstChunk = chunk;
			}						

			// Add to mesh's batch list
			cur->SetBatchNext( chunk->m_batchList );
			chunk->m_batchList = cur;
		}
	}

	//! Pop the static mesh batch list
	RED_INLINE Bool Pop( Uint32& lightChannel, CRenderElement_MeshChunk*& batchList )
	{
		// Pop
		if ( m_firstChunk )
		{
			// Pop the chunk info
			lightChannel = m_firstChunk->m_lightChannel;
			batchList = m_firstChunk->m_batchList;	

			// Walk the list, unlink
			LightGroupInfo* next = m_firstChunk->m_batchNext;
			m_firstChunk->m_batchNext = NULL;
			m_firstChunk->m_batchList = NULL;
			m_firstChunk = next;
			return true;
		}

		// No more batches
		return false;
	}
};

/// Group fragments by mesh
class CRenderBatchByTwoSided
{
private:
	CRenderElement_MeshChunk*	m_twoSided;	//!< First mesh in batch list
	CRenderElement_MeshChunk*	m_oneSided;	//!< First mesh in batch list

	Float						m_closestTwoSided;
	Float						m_closestOneSided;

public:
	//! Group fragments by mesh
	RED_INLINE CRenderBatchByTwoSided( CRenderElement_MeshChunk* batchList )
		: m_twoSided( nullptr )
		, m_oneSided( nullptr )
		, m_closestTwoSided( FLT_MAX )
		, m_closestOneSided( FLT_MAX )
	{
		CRenderElement_MeshChunk* next = nullptr;
		for ( CRenderElement_MeshChunk* cur = batchList; cur; cur = next )
		{
			// Keep the loop going, get the next fragment while the link is still valid
			next = static_cast<CRenderElement_MeshChunk* >( cur->GetBatchNext() );
			
			CRenderMesh* renderMesh = cur->GetMesh();
			IRenderProxyDrawable* drawableProxy = cur->GetProxy();

			Bool isRMTwoSided = renderMesh->IsTwoSided();
			Bool isDPTwoSided = drawableProxy->IsTwoSided();
			Bool isTwoSided = isRMTwoSided || isDPTwoSided;

			// Get distance, for material binding. Instead of just the cached distance, we include the extents of the proxy's
			// bounds, so that we won't get large distances just because a mesh is big.
			const Float distance = drawableProxy->AdjustCameraDistanceSqForTextures( drawableProxy->GetCachedDistanceSquared() );

			// Get the fragments material
			if ( isTwoSided )
			{
				cur->SetBatchNext( m_twoSided );
				m_twoSided = cur;
				if( distance < m_closestTwoSided )
				{
					m_closestTwoSided = distance;
				}
			}
			else
			{
				cur->SetBatchNext( m_oneSided );
				m_oneSided = cur;
				if( distance < m_closestOneSided )
				{
					m_closestOneSided = distance;
				}
			}
		}
	}

	//! Pop the mesh batch list
	RED_INLINE Bool Pop( Bool& twoSided, CRenderElement_MeshChunk*& batchList, Float& closestDistance )
	{
		// Pop meshes
		if ( m_twoSided )
		{
			twoSided = true;
			batchList = m_twoSided;
			m_twoSided = nullptr;
			closestDistance = m_closestTwoSided;
			return true;
		}

		if ( m_oneSided )
		{
			twoSided = false;
			batchList = m_oneSided;
			m_oneSided = nullptr;
			closestDistance = m_closestOneSided;
			return true;
		}

		return false;
	}
};