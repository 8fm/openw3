/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "worldTreeTraverser.h"
#include "collisionMemToolModel.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/staticMeshComponent.h"

using namespace CollisionMem;

//** *******************************
//
void CWorldTreeTraverser::TraverseWorldTree( CWorld * pWorld, CCollisionMemToolModel * pModel )
{
	ASSERT( pWorld );

	m_MeshMap.Clear();

	CLayerGroup * pLayerGroup = pWorld->GetWorldLayers();

	if( pLayerGroup )
	{
		GFeedback->BeginTask( TEXT( "Analyzing world structure" ), false );

		// Process layer sub groups
		GFeedback->UpdateTaskProgress( 0, 1 );

		const CLayerGroup::TGroupList& groups = pLayerGroup->GetSubGroups();

		Uint32 size = groups.Size();

		for( Uint32 i = 0; i < size; ++i )
		{
			String name = groups[ i ]->GetName();

			ProcessNode( groups[ i ], pModel );

			GFeedback->UpdateTaskInfo( TEXT("Processing %s (%d out of %d)"), name.AsChar(), i, size );
			GFeedback->UpdateTaskProgress( i, size );
		}

		// Process set of layer info objects
		GFeedback->UpdateTaskProgress( 0, 1 );

		const CLayerGroup::TLayerList& layers = pLayerGroup->GetLayers();

		size = layers.Size();

		for( Uint32 i = 0; i < size; ++i )
		{
			String name = layers[ i ]->GetShortName();

			ProcessNode( layers[ i ], pModel );

			GFeedback->UpdateTaskInfo( TEXT("Processing %s (%d out of %d)"), name.AsChar(), i, size );
			GFeedback->UpdateTaskProgress( i, size + 1 );
		}

		// Add some structure to gathered data
		GFeedback->UpdateTaskInfo( TEXT("Processing gathered data"), size, size + 1 );

		pModel->PrepareUniqueMeshData( m_MeshMap );

		GFeedback->EndTask	();
	}
}

//** *******************************
//
void	CWorldTreeTraverser::ProcessNode		( CLayerGroup * pNode, CCollisionMemToolModel * pModel )
{
	if( pNode )
	{
		const CLayerGroup::TGroupList& groups = pNode->GetSubGroups();
		Uint32 size = groups.Size();

		for( Uint32 i = 0; i < size; ++i )
		{
			ProcessNode( groups[ i ], pModel );
		}

		const CLayerGroup::TLayerList& layers = pNode->GetLayers();
		size = layers.Size();

		for( Uint32 i = 0; i < size; ++i )
		{
			ProcessNode( layers[ i ], pModel );
		}
	}
}

//** *******************************
//
void	CWorldTreeTraverser::ProcessNode		( CLayerInfo * pNode, CCollisionMemToolModel * pModel )
{
	if( pNode && pNode->IsLoaded() )
	{
		CLayer * pLayer = pNode->GetLayer();
		ASSERT( pLayer );

		// Enumerate entities
		TDynArray< CEntity* > entities;
		pLayer->GetEntities( entities );

		for( Uint32 i = 0; i < entities.Size(); ++i )
		{
			CEntity * pEntity = entities[ i ];

			ASSERT( pEntity );

			// Enumerate components for this entity
			TDynArray< CComponent * > components;
			CollectEntityComponents( pEntity, components );
			
			for( Uint32 j = 0; j < components.Size(); ++j )
			{
				CComponent * pComponent = components[ j ];

				ASSERT( pComponent );

				if( pComponent->IsA< CStaticMeshComponent >() )
				{
					CStaticMeshComponent * pSMC = Cast< CStaticMeshComponent >( pComponent );

					pModel->ReadWorldNodeData( pSMC, m_MeshMap );
				}
			}
		}
	}
}
