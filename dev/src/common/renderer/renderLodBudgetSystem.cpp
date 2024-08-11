/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderLodBudgetSystem.h"
#include "renderProxyDrawable.h"
#include "renderProxyMesh.h"
#include "renderElementMeshChunk.h"
#include "renderCollector.h"
#include "renderMesh.h"
#include "../engine/renderVisibilityQuery.h"
#include "../engine/worldIterators.h"
#include "../engine/entity.h"

CRenderLodBudgetSystem::CRenderLodBudgetSystem( Uint32 chunkBudget, Uint32 triangleBudget, Uint32 proxyBudget, Uint32 frameDelay )
	: m_prevCharacterChunks( 0 )
	, m_prevCharacterTriangles( 0 )
	, m_currCharacterChunks( 0 )
	, m_currCharacterTriangles( 0 )
	, m_chunkBudget( chunkBudget )
	, m_triangleBudget( triangleBudget )
	, m_proxyBudget( proxyBudget )
	, m_frameDelay( frameDelay )
	, m_currentFrame( 0 )
	, m_on( false )
	, justTicked( false )
	, m_lodManagementPolicy( LDP_GentleDrop )
{
}

void CRenderLodBudgetSystem::CollectVisibleCharactersInfo()
{
	if ( GGame && GGame->GetActiveWorld() )
	{
		for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it )
		{
			CEntity *entity = *it;
			if ( entity->WasVisibleLastFrame() )
			{
				++m_currCharacterProxies;
			}
		}
	}	
}

void CRenderLodBudgetSystem::Tick( const CRenderCollector& collector )
{
#if 0
	PC_SCOPE_PIX( LodBudgetingSystem_Tick );
	if ( !m_on )
	{
		// turned off, return
		return;
	}

	m_initialized = true;
	if ( ++m_currentFrame > m_frameDelay )
	{
		m_currentFrame = 0;
		justTicked = true;
	}
	else
	{
		justTicked = false;
		return;
	}

	m_prevCharacterChunks = m_currCharacterChunks;
	m_prevCharacterTriangles = m_currCharacterTriangles;
	m_prevCharacterProxies = m_currCharacterProxies;
	m_currCharacterChunks = 0;
	m_currCharacterTriangles = 0;
	m_currCharacterProxies = 0;

	{
		PC_SCOPE_PIX( LodBudgetingSystem_Tick_CollectVisibleCharactersInfo );
		CollectVisibleCharactersInfo();
	}
	
	{
		PC_SCOPE_PIX( LodBudgetingSystem_Tick_CollectTriangles );
		for ( Uint32 i = 0; i < ARRAYSIZE( collector.m_renderCollectorData->m_elements ); ++i )
		{
			for ( Uint32 j = 0; j < ARRAYSIZE( collector.m_renderCollectorData->m_elements[i].m_elements ); ++j )
			{
				// count potentially character mesh chunks
				TDynArray< CRenderElement_MeshChunk* >::const_iterator it = collector.m_renderCollectorData->m_elements[i].m_elements[j].m_meshes.Begin();
				TDynArray< CRenderElement_MeshChunk* >::const_iterator end = collector.m_renderCollectorData->m_elements[i].m_elements[j].m_meshes.End();

				while ( it != end )
				{
					if ( (*it)->GetProxy()->GetType() != RPT_Mesh )
					{
						continue;
					}

					const CRenderProxy_Mesh* rpm = static_cast< const CRenderProxy_Mesh* >( (*it)->GetProxy() );

					if ( rpm && rpm->CheckMeshProxyFlag( RMPF_IsCharacterProxy ) )
					{
						++m_currCharacterChunks;
						ASSERT( (*it)->GetMesh() );

						const TDynArray< CRenderMesh::Chunk >& chunks = (*it)->GetMesh()->GetChunks();
						ASSERT( (*it)->GetChunkIndex() >= 0 );
						ASSERT( (*it)->GetChunkIndex() < chunks.Size() );

						// triangle counting taken from dx9mesh.cxx, line 236, method DrawChunkNoBind
						m_currCharacterTriangles += ( chunks[ (*it)->GetChunkIndex() ].m_numIndices - 2 );
					}

					++it;
				}
			}
		}
	}
#endif
}

void CRenderLodBudgetSystem::CalculateLodCorrection( const CRenderProxy_Mesh* rpMesh, Int32& targetLodIndex )
{
	Uint32 higherLODChunksNum = 0;
	Uint32 lowerLODChunksNum = 0;

	switch ( m_lodManagementPolicy )
	{
		case LDP_None:
			// do nothing
			break;

		case LDP_AgressiveDrop:
			higherLODChunksNum = rpMesh->GetLodGroups()[ targetLodIndex ].GetChunkCount();
			lowerLODChunksNum = rpMesh->GetLodGroups()[ rpMesh->GetNumLods() - 1 ].GetChunkCount();

			// difference in chunks count between higher and lower LOD
			if ( justTicked )
			{
				DecreaseChunksCount( higherLODChunksNum - lowerLODChunksNum );
			}
			targetLodIndex = rpMesh->GetNumLods() - 1;
			break;

		case LDP_GentleDrop:
			if ( targetLodIndex < rpMesh->GetNumLods() - 1 )
			{
				// drop one LOD of current mesh, update stats of current system tick
				higherLODChunksNum = rpMesh->GetLodGroups()[ targetLodIndex ].GetChunkCount();
				lowerLODChunksNum = rpMesh->GetLodGroups()[ targetLodIndex + 1 ].GetChunkCount();

				// difference in chunks count between higher and lower LOD
				if ( justTicked )
				{
					DecreaseChunksCount( higherLODChunksNum - lowerLODChunksNum );
				}							
				++targetLodIndex;
			}
			break;

		case LDP_AdaptiveDrop:
			// TODO: LDP Adaptive
			break;

		case LDP_AgressiveEnhance:
			if ( rpMesh->GetNumLods() != 0 && targetLodIndex != 0 )
			{
				// enhance LOD of current mesh to highest, update stats of current system tick
				higherLODChunksNum = rpMesh->GetLodGroups()[ 0 ].GetChunkCount();
				lowerLODChunksNum = rpMesh->GetLodGroups()[ targetLodIndex ].GetChunkCount();

				// difference in chunks count between higher and lower LOD
				if ( justTicked )
				{
					DecreaseChunksCount( higherLODChunksNum - lowerLODChunksNum );
				}							
				targetLodIndex = 0;
			}
			break;

		default:
			// Do nothing
			break;
	}
}