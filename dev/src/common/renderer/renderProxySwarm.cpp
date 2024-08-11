/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxySwarm.h"
#include "renderSkinningData.h"
#include "renderSwarmData.h"
#include "renderTerrainShadows.h"
#include "renderShadowManager.h"
#include "renderElementMap.h"
#include "renderElementMeshChunk.h"
#include "renderScene.h"
#include "renderHelpers.h"
#include "renderMaterial.h"
#include "renderMesh.h"
#include "renderTexture.h"
#include "renderOcclusion.h"
#include "renderSwarmAnimationManager.h"

#include "../engine/mesh.h"
#include "../engine/swarmRenderComponent.h"

#include "../core/dataError.h"
#include "../core/priqueue.h"

static const Uint32 MAX_NUM_CHUNKS = 256;

CRenderProxy_Swarm::CRenderProxy_Swarm( const RenderProxyInitInfo& initInfo )
	: IRenderProxyDrawable( RPT_Swarm, initInfo )
{
	m_massiveAnimationSet.Reset();

	const CSwarmRenderComponent* swarmRenderComponent = nullptr;

	if ( initInfo.m_component && initInfo.m_component->IsA< CSwarmRenderComponent >() )
	{
		swarmRenderComponent = static_cast< const CSwarmRenderComponent* >( initInfo.m_component );
	}

	// null the mesh chunk pointers
	for( Uint32 i=0; i<NUM_LODS; ++i )
	{
		m_chunks[ i ] = nullptr;
	}

	// Initialize from swarm render component
	CMeshTypeResource* mesh = nullptr;
	if ( swarmRenderComponent != nullptr )
	{
		// get swarm mesh
		mesh = swarmRenderComponent->GetMesh();

		// register animset & skeleton to swarm animation manager
		CSkeleton* skeleton = swarmRenderComponent->GetSkeleton();

		RED_ASSERT( mesh != nullptr && skeleton != nullptr && swarmRenderComponent->GetAnimationSet() != nullptr, TXT("Data error: init info for swarm proxy is not valid.") );

		if( mesh != nullptr && skeleton != nullptr && swarmRenderComponent->GetAnimationSet() != nullptr )
		{
			m_massiveAnimationSet = SwarmMassiveAnimation::GMassiveAnimationRegister::GetInstance().GetMassiveAnimationSet( mesh, skeleton, swarmRenderComponent->GetAnimationSet() );
		}
	}

	if ( mesh )
	{
		const Uint32 numLODLevels = mesh->GetNumLODLevels();
		if( numLODLevels != NUM_LODS )
		{
			RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "The swarm mesh: %s needs to contain %i LODs and has %i LOD(s)." ), mesh->GetFriendlyName().AsChar(), NUM_LODS, numLODLevels );
		}

		for ( Uint32 i=0; i<NUM_LODS; i++ )
		{
			// Get LOD info
			Uint32 iLOD = (i >= numLODLevels ? numLODLevels - 1 : i);
			//const CMesh::LODLevel& level = mesh->GetMeshLODLevels()[iLOD];

			//// check that we only have 1 chunk in this LOD level
			//if( level.m_chunks.Size() != 1 )
			//{
			//	RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "The swarm mesh: %s - LOD: %i needs to have a single chunk ONLY, but has %i." ), mesh->GetFriendlyName().AsChar(), iLOD, level.m_chunks.Size() );
			//	continue;
			//}

			// Get render resources
			CRenderMesh* renderMesh = static_cast< CRenderMesh* >( mesh->GetRenderResource() );
			if ( !renderMesh )
			{
				continue;
			}

			// Create rendering elements for each chunk defined at given LOD
			const CRenderMesh::Chunk& chunk = renderMesh->GetChunks()[ 0 ];

			// Get material to use
			IMaterial* material = mesh->GetMaterials()[ chunk.m_materialId ].Get();
			if ( !material || !material->GetMaterialDefinition() )
			{
				continue;
			}

			// Create new element
			m_chunks[ i ] = new CRenderElement_SwarmChunk( this, renderMesh, 0, material, i );
		}
	}
}

CRenderProxy_Swarm::~CRenderProxy_Swarm()
{
	for ( Uint8 i=0; i<NUM_LODS; i++ )
	{
		if( m_chunks[ i ] )
		{
			SAFE_RELEASE( m_chunks[i] );
		}
	}
}


void CRenderProxy_Swarm::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	// TODO : Filter out only the LODs needed for the prefetch camera.

	// When rendering, we use distance 0, since the textures are generally tiny.
	const Float distance = 0.0f;
	for( Uint32 i=0; i<NUM_LODS; ++i )
	{
		if ( m_chunks[ i ] != nullptr )
		{
			prefetch->AddMaterialBind( m_chunks[ i ]->GetMaterial(), m_chunks[ i ]->GetMaterialParams(), distance );
		}
	}
}


void CRenderProxy_Swarm::CollectElements( CRenderCollector& collector )
{
	// Collect meshes
	if ( !collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_Meshes ) || !IsVisible() )
		return;

	// Update once per frame stuff
	UpdateOncePerFrame( collector );

	// Make sure it's visible
	if ( !IsVisibleInCurrentFrame() )
		return;

	for( Uint32 i=0; i<NUM_LODS; ++i )
	{
		if ( m_chunks[ i ] )
		{
			if ( m_chunks[i]->GetMesh()->IsFullyLoaded() )
			{
				collector.PushElement( m_chunks[ i ] );
			}
		}
	}
}

const EFrameUpdateState CRenderProxy_Swarm::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyDrawable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	const Bool wasVisibleLastFrame = ( ret == FUS_UpdatedLastFrame );
	CalculateLOD( collector, wasVisibleLastFrame );

	return ret;
}

void CRenderProxy_Swarm::CalculateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
	// Calculate distance to pivot
	const Vector& cameraPosition = collector.GetRenderCamera().GetPosition();
	const Vector meshPosition = GetLocalToWorld().GetTranslation();

	// Take FOV into account in cutscenes and dialogs
	const Float fovFactor = collector.m_camera->GetFOVMultiplier();

	// update LODs for all boids here
	// calculate squared distance to camera & fill priority queue
	struct SCrowdAgentComperator
	{
		static RED_INLINE Bool Less( const CSwarmBoidData* ca1, const CSwarmBoidData* ca2 )	{ return ca1->m_distToCameraSquared < ca2->m_distToCameraSquared; }
	};

	static TPriQueue<CSwarmBoidData*, SCrowdAgentComperator> sortedBoids;
	sortedBoids.Clear();
	Uint32 numBoidsInFrustum = 0;


	// frustum cull agents
	CFrustum frustum( collector.GetRenderCamera().GetWorldToScreen() );
	for( Uint32 i=0; i<m_numBoidsToRender; ++i )
	{
		CSwarmBoidData& boid = m_swarmBoidsToRender[ i ];

		if( m_massiveAnimationSet == nullptr )
		{
			continue;
		}

		SwarmMassiveAnimation::CMassiveAnimation* animation = m_massiveAnimationSet->GetAnimation( boid.m_currentAnimation );

		// First update animation instance if animation has changed
		if ( boid.m_currentAnimation != boid.m_previousAnimation && animation != nullptr )
		{
			CSwarmBoidData& boidToSynchronize = m_swarmBoidsToSynchonize[ i ];
			boid.m_animationInstance = animation->GetBeginningInstanceId( GGame->GetEngineTime() );	// Get instance closest to zero animation time
			boid.m_previousAnimation = boid.m_currentAnimation;
			boidToSynchronize.m_previousAnimation = boid.m_currentAnimation;
			boidToSynchronize.m_animationInstance = boid.m_animationInstance;
		}

		// test boid bounding box (TODO: get size from lair somehow?)
		Box box( boid.m_position, 1.0f );
		if( !frustum.TestBox( box ) )
			continue;

		// update distance to camera for this agent
		boid.m_distToCameraSquared = ( boid.m_position - cameraPosition ).SquareMag() * fovFactor;

		if( animation != nullptr )
		{
			animation->UpdateInstance( GGame->GetEngineTime(), boid.m_animationInstance, collector.m_frameIndex );
		}

		// added to priority queue
		sortedBoids.Push( &boid );
		numBoidsInFrustum++;
	}

	// divide agents into LODs
	{
		// calculate LOD limits
		Float LOD_DIST_SQ[ NUM_LODS - 1 ];
		for( Uint32 i=0; i<NUM_LODS; ++i )
		{
			m_swarmLODs[ i ].ClearFast();
			if( i > 0 )
			{
				LOD_DIST_SQ[ i - 1 ] = ( i * 10.0f ) * ( i * 10.0f );
			}
		}

		// sort boids into groups
		while( !sortedBoids.Empty() )
		{
			// get "top" boid
			CSwarmBoidData* boid = sortedBoids.Top();
			sortedBoids.Pop();

			// select correct LOD based on distance
			for( Uint32 i=0; i<NUM_LODS; ++i )
			{
				if( i == NUM_LODS - 1 || boid->m_distToCameraSquared < LOD_DIST_SQ[ i ] )
				{
					m_swarmLODs[ i ].PushBack( boid );
					break;
				}
			}
		}
	}
}

void CRenderProxy_Swarm::UpdateBoids( CRenderSwarmData* data, Uint32 numBoids )
{
	m_swarmBoidsToRender = data->GetReadData();
	m_swarmBoidsToSynchonize = data->GetWriteData();		// Only for animationInstanceId
	m_numBoidsToRender = numBoids;
}

void CRenderProxy_Swarm::AttachToScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::AttachToScene( scene );
	m_drawableFlags |= RPF_Dynamic;
}

const CRenderSkinningData* CRenderProxy_Swarm::GetSkinningDataForBoid(const CSwarmBoidData* boid) const
{
	SwarmMassiveAnimation::CMassiveAnimation* animation = m_massiveAnimationSet->GetAnimation( boid->m_currentAnimation );
	if( animation != nullptr )
	{
		return animation->GetSkinningDataForInstance( boid->m_animationInstance );
	}

	animation = m_massiveAnimationSet->GetDefaultAnimation();
	return animation->GetSkinningDataForInstance( 0 );			// Default instance
}
