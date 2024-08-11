
#include "build.h"
#include "animationPreviewGhost.h"
#include "animationEditorUtils.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/engine/behaviorGraphStack.h"

//////////////////////////////////////////////////////////////////////////

PreviewGhostSkeleton::PreviewGhostSkeleton()
	: m_activation( 0.f )
	, m_color( Color( 128, 255, 128 ) )
{

}

void PreviewGhostSkeleton::UpdatePose( const CAnimatedComponent* component )
{
	SkeletonUtils::CopyPoseToBuffer( component, m_pose );
}

void PreviewGhostSkeleton::Update( Float dt )
{
	
}

void PreviewGhostSkeleton::Draw( CRenderFrame* frame, const CAnimatedComponent* component )
{
	if ( m_activation > 0.f && m_pose.Size() > 0 )
	{
		Color color = m_color;
		color.Mul3( m_activation );
		
		SkeletonRenderingUtils::DrawSkeleton( m_pose, component, color, frame );
	}
}

void PreviewGhostSkeleton::SetActivation( Float val )
{
	m_activation = val;
}

//////////////////////////////////////////////////////////////////////////

PreviewGhostEntity::PreviewGhostEntity()
	: m_visibility( false )
	, m_componentH( NULL )
	, m_template( NULL )
	, m_isReady( false )
{

}

PreviewGhostEntity::~PreviewGhostEntity()
{
	CAnimatedComponent* comp = m_componentH.Get();
	if ( comp )
	{
		comp->GetEntity()->Destroy();
		m_componentH = NULL;
	}
}

void PreviewGhostEntity::UpdatePose( const CAnimatedComponent* component )
{
	if ( m_template != component )
	{
		RecreateEntity( component );

		m_template = component;
	}

	if( m_isReady )
	{
		SyncEntityTo( component );
	}
	else
	{
		CAnimatedComponent* ghostComponent = m_componentH.Get();

		if( ghostComponent )
		{
			CEntity* entity = ghostComponent->GetEntity();

			// Send them initially to someplace far away so that they don't interfere with the original entity whilst they might still have collisions
			entity->SetRawPlacement( &Vector( 0.0f, 0.0f, 0.0f, 0.0f ), NULL, NULL );
			entity->ForceUpdateTransformNodeAndCommitChanges();
			entity->ForceUpdateBoundsNode();

			if( ghostComponent->IsAttached() )
			{
				CMovingAgentComponent* movingComponent = Cast< CMovingAgentComponent >( m_componentH.Get() );

				if( movingComponent )
				{
					movingComponent->ForceEntityRepresentation( true );
					movingComponent->SetCollidable( false );
				}

				CMovingPhysicalAgentComponent* physicalComponent = Cast< CMovingPhysicalAgentComponent >( m_componentH.Get() );

				if( physicalComponent )
				{
					physicalComponent->EnableCharacterCollisions( false );
				}

				m_isReady = true;
			}
		}

	}
}

void PreviewGhostEntity::SetActivation( Float val )
{
	m_visibility = val > 0.f;
}

void PreviewGhostEntity::RecreateEntity( const CAnimatedComponent* component )
{
	CAnimatedComponent* currComponent = m_componentH.Get();
	if ( currComponent )
	{
		currComponent->GetEntity()->Destroy();
	}

	m_componentH = AnimationEditorUtils::CloneEntity( component );

	currComponent = m_componentH.Get();

	if( currComponent )
	{
		CEntity* oldEntity = component->GetEntity();
		CEntity* newEntity = currComponent->GetEntity();
		
		if( oldEntity->IsA< CGameplayEntity >() )
		{
			ASSERT( newEntity->IsA< CGameplayEntity >() );

			CInventoryComponent* oldInventory = static_cast< CGameplayEntity* >( oldEntity )->GetInventoryComponent();
			CInventoryComponent* newInventory = static_cast< CGameplayEntity* >( newEntity )->GetInventoryComponent();
			
			if( oldInventory && newInventory )
			{
				for( Uint32 i = 0; i < oldInventory->GetItemCount(); ++i )
				{
					const SInventoryItem* oldItem = oldInventory->GetItem( (SItemUniqueId)i );
					SItemUniqueId id = newInventory->AddItem( *oldItem ) [ 0 ];

					if( oldItem->IsMounted() )
					{
						CInventoryComponent::SMountItemInfo mountInfo;
						newInventory->MountItem( id, mountInfo );
					}
				}
			}
		}

		// Send them initially to someplace far away so that they don't interfere with the original entity whilst they might still have collisions
		newEntity->SetRawPlacement( &Vector( 0.0f, 0.0f, 0.0f, 0.0f ), NULL, NULL );
		newEntity->ForceUpdateTransformNodeAndCommitChanges();
		newEntity->ForceUpdateBoundsNode();

		if ( currComponent->GetBehaviorStack() )
		{
			currComponent->GetBehaviorStack()->Deactivate();
		}
	}

}

void PreviewGhostEntity::SyncEntityTo( const CAnimatedComponent* component )
{
	CAnimatedComponent* currComponent = m_componentH.Get();
	if ( currComponent )
	{
		AnimationEditorUtils::SyncComponentsPoses( component, currComponent );
		
		const CEntity* entitySrc = component->GetEntity();
		CEntity* entityDest = currComponent->GetEntity();

		entityDest->Teleport( entitySrc->GetWorldPosition(), entitySrc->GetWorldRotation() );

		currComponent->ForceUpdateTransformNodeAndCommitChanges();
		currComponent->ForceUpdateBoundsNode();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PreviewGhostContainer::PreviewGhostContainer()
	: m_currGhost( 0 )
	, m_accTime( 0.f )
	, m_currentType( GhostType_Max )
{
	SetTimeStep( 1.f / 30.f );
}

void PreviewGhostContainer::InitGhosts( Uint32 numberOfGhosts, EGhostType ghostType )
{
	Uint32 startingIndex = m_ghosts.Size();

	if( ghostType != m_currentType )
	{
		DestroyGhosts();
		startingIndex = 0;
	}
	else
	{
		for( Uint32 i = numberOfGhosts; i < m_ghosts.Size(); ++i )
		{
			delete m_ghosts[ i ];
		}
	}

	m_ghosts.Resize( numberOfGhosts );

	for ( Uint32 i = startingIndex; i < m_ghosts.Size(); ++i )
	{
		switch( ghostType )
		{
		case GhostType_Entity:
			m_ghosts[ i ] = new PreviewGhostEntity();
			break;

		case GhostType_Skeleton:
			m_ghosts[ i ] = new PreviewGhostSkeleton();
			break;
		}
	}

	m_currentType = ghostType;
}

void PreviewGhostContainer::DestroyGhosts()
{
	m_ghosts.ClearPtr();
}

Bool PreviewGhostContainer::HasGhosts() const
{
	return m_ghosts.Size() > 0;
}

void PreviewGhostContainer::UpdateGhosts( Float dt, const CAnimatedComponent* component )
{
	if ( m_ghosts.Empty() )
	{
		return;
	}

	m_accTime += dt;

	if ( m_accTime > m_timeStep )
	{
		// Do the bounds check for the current ghost here, as the array may have been resized between updates
		if ( m_currGhost >= m_ghosts.Size() )
		{
			m_currGhost = 0;
		}

		IPreviewGhost* curr = m_ghosts[ m_currGhost ];
		curr->UpdatePose( component );

		{
			const Int32 size = m_ghosts.SizeInt();
			for ( Int32 i=0; i<size; ++i )
			{
				IPreviewGhost* g = m_ghosts[ i ];

				Int32 dist = i - (Int32)m_currGhost;
				if ( dist < 0 )
				{
					dist = size + dist;
				}

				Float p = (Float)dist / (Float)size;
				ASSERT( p >= 0.f && p <= 1.f );

				g->SetActivation( p );
			}
		}

		curr->SetActivation( 1.f );

		m_currGhost += 1;

		m_accTime = 0.f;
	}

	const Uint32 size = m_ghosts.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		IPreviewGhost* g = m_ghosts[ i ];
		g->Update( dt );
	}
}

void PreviewGhostContainer::Reset()
{
	// TODO
}

void PreviewGhostContainer::Draw( CRenderFrame *frame, const CAnimatedComponent* component )
{
	const Uint32 size = m_ghosts.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_ghosts[ i ]->Draw( frame, component );
	}
}

void PreviewGhostContainer::SetTimeStep( Float time )
{
	m_timeStep = time;
}
