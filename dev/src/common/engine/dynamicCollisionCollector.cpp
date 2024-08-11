/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "dynamicCollisionCollector.h"
#include "dynamicColliderComponent.h"
#include "renderCommands.h"
#include "world.h"
#include "foliageEditionController.h"

//TODO
#define DYNAMIC_COLLIDERS_WATER_PROXIMITY_CHECK 2.0f

// when changed change also in renderProxySpeedtree.h
//#define GRASS_DYNAMIC_COLLISION_LIMIT	(Uint32)16

// max size of the array with active colliders
#define DYNAMIC_COLLIDERS_MAX_SIZE 16
// max size of the array with all (scene attached) colliders
#define NON_ACTIVE_DYNAMIC_COLLIDERS_MAX_SIZE 1024

IMPLEMENT_ENGINE_CLASS( SDynamicCollider );

CDynamicCollisionCollector::CDynamicCollisionCollector( CWorld* world )
{	
	m_pereferableTickRange = 30;	
	m_currentTickBlock = 0;
	m_currentTickBlockEnd = 0;
	m_currentTickBlockBegin = 0;

	m_currentTimeAcum = 0.0f;
	m_forceOcclude = false;

	m_dynamicCollection.Reserve( DYNAMIC_COLLIDERS_TYPES_NUMBER );
	for( Uint32 i=0; i<DYNAMIC_COLLIDERS_TYPES_NUMBER; i++ )
	{
		m_dynamicCollection.PushBack( Red::TUniquePtr< SDynamicCollidersCollection >( new SDynamicCollidersCollection() ) );
	}

	for(Uint32 i=0; i<m_dynamicCollection.Size(); i++)
	{
		// static number of active collisions
		m_dynamicCollection[i]->m_activeCollisions.Resize( DYNAMIC_COLLIDERS_MAX_SIZE );
		m_dynamicCollection[i]->m_colliderPair.Clear();
	}
}

void CDynamicCollisionCollector::Add( CDynamicColliderComponent* cmp )
{	
	if( cmp->m_useInWaterDisplacement )		AddCollider( DYNAMIC_COLLIDERS_TYPE_WATER_DISPLACEMENT, cmp );
	if( cmp->m_useInWaterNormal )			AddCollider( DYNAMIC_COLLIDERS_TYPE_WATER_NORMAL, cmp );
	if( cmp->m_useInGrassDisplacement )		AddCollider( DYNAMIC_COLLIDERS_TYPE_GRASS, cmp );
}

void CDynamicCollisionCollector::Remove( CDynamicColliderComponent* cmp, Bool forceRemoval )
{
	if( cmp->m_useInWaterDisplacement )		ReleaseCollider( DYNAMIC_COLLIDERS_TYPE_WATER_DISPLACEMENT, cmp, forceRemoval );
	if( cmp->m_useInWaterNormal )			ReleaseCollider( DYNAMIC_COLLIDERS_TYPE_WATER_NORMAL, cmp, forceRemoval );
	if( cmp->m_useInGrassDisplacement )		ReleaseCollider( DYNAMIC_COLLIDERS_TYPE_GRASS, cmp, forceRemoval );
}

Bool CDynamicCollisionCollector::ReleaseCollider( Uint32 typeIndex, CDynamicColliderComponent* cmp, Bool forceRemoval ) 
{
	if( cmp != nullptr )
	{
		for( Int32 i=m_dynamicCollection[ typeIndex ]->m_activeCollisions.Size()-1; i>=0; --i )
		{
			if( m_dynamicCollection[ typeIndex ]->m_activeCollisions[i].m_parent == cmp )
			{
				m_dynamicCollection[ typeIndex ]->m_activeCollisions[i].m_parent = nullptr;											

				if( !forceRemoval )
				{
					// fade out nicely
					m_dynamicCollection[ typeIndex ]->m_activeCollisions[i].m_decaySpeed = 0.5f;				
					break;
				}
				// force removal now
				else
				{
					m_dynamicCollection[ typeIndex ]->m_activeCollisions[i].m_decaySpeed = 0.0f;					
					m_dynamicCollection[ typeIndex ]->m_activeCollisions[i].m_intensity = 0.0f;
					break;
				}				
			}
		}
		return( m_dynamicCollection[ typeIndex ]->m_colliderPair.Erase( cmp ) );
	}
	return false;
}

void CDynamicCollisionCollector::AddCollider( Uint32 typeIndex, CDynamicColliderComponent* cmp ) 
{	
	if( m_dynamicCollection[ typeIndex ]->m_colliderPair.Size() < NON_ACTIVE_DYNAMIC_COLLIDERS_MAX_SIZE )
	{
		m_dynamicCollection[ typeIndex ]->m_colliderPair.Insert( cmp, -1 );	
		m_forceOcclude = true;
	}
	else
	{
		RED_ASSERT(false, TXT("Cannot add Dynamic Collider Componet, limit exceeded!"));
	}
}

void CDynamicCollisionCollector::TryActivateCollider( Uint32 typeIndex, const SDynamicCollider &c )
{
	Int32 ind = GetFreeSlot( typeIndex );
	if( ind > -1 )
	{
		m_dynamicCollection[ typeIndex ]->m_activeCollisions[ ind ].m_intensity = 1.0f;
		m_dynamicCollection[ typeIndex ]->m_activeCollisions[ ind ].m_transformMatrix = c.m_transformMatrix;
		m_dynamicCollection[ typeIndex ]->m_activeCollisions[ ind ].m_decaySpeed = c.m_decaySpeed;
		m_dynamicCollection[ typeIndex ]->m_activeCollisions[ ind ].m_parent =  nullptr;
	}
}

/*
void CDynamicCollisionsCollector::CollectAll( CWorld* world )
{
	RED_ASSERT( world );		

	for( WorldAttachedComponentsIterator it( world ); it; ++it )
	{
		CDynamicColliderComponent* comp = Cast< CDynamicColliderComponent >( *it );
		if( comp ) 
		{
			if( comp->m_useInWaterDisplacement ) m_waterDisplacements.PushBack( comp );
			if( comp->m_useInWaterNormal ) m_dynamicWaterColliders.PushBack( comp );
			if( comp->m_useInGrassDisplacement ) m_grassColliders.PushBack( comp );
		}
	}
}
*/

void CDynamicCollisionCollector::RefreshActiveCollider( Uint32 type, Float deltaTime )
{

	for( Int32 i= m_dynamicCollection[ type ]->m_activeCollisions.SizeInt()-1; i >= 0; i-- )
	{
		// update active collider
		if( m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_intensity > 0.0f )
		{
			m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_intensity -= m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_decaySpeed*deltaTime;

			Vector previousTranslation = m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_transformMatrix.GetRow(3);
			Float previousVelocity = previousTranslation.W;

			Vector currentTranslation = previousTranslation;

			// might be null for collisions triggered by scripts
			if( m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_parent != nullptr )
			{
				currentTranslation = m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_parent->GetLocalToWorld().GetRow(3);
				m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_transformMatrix = m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_parent->GetLocalToWorld();
			}

			currentTranslation.W = 0.0f;
			
			// assume non zero velocity (to allow smooth decay = better visuals)
			Float currentVelocity = Clamp<Float>( (currentTranslation - previousTranslation).SquareMag3() / deltaTime, 0.f, 1000.f );
			currentVelocity *= Clamp<Float>( m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_intensity, 0.f, 1.f );
			currentTranslation.W = currentVelocity;
			m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_transformMatrix.SetRow( 3, currentTranslation );
		}
		else
		{			
			m_dynamicCollection[ type ]->m_activeCollisions[ i ].m_decaySpeed = 0.0f;
		}
	}
}

void CDynamicCollisionCollector::OccludeColliders( const Vector &cameraPosition, Bool forceOcclude )
{
	for( Uint32 t = 0; t<m_dynamicCollection.Size(); t++ )
	{
		Float effectiveDistance = 30.0f;

		Uint32 endInd = m_dynamicCollection[t]->m_colliderPair.Size();
		Uint32 beginInd = 0;

		if( !forceOcclude && m_dynamicCollection[t]->m_colliderPair.Size() )
		{
			endInd = m_currentTickBlockEnd % ( m_dynamicCollection[t]->m_colliderPair.Size() );
			beginInd = m_currentTickBlockBegin % ( m_dynamicCollection[t]->m_colliderPair.Size() );

			if( beginInd > endInd ) endInd = m_dynamicCollection[t]->m_colliderPair.Size();
		}	
		
		THashMap< CDynamicColliderComponent*, Int32 >::iterator it;

		for( it = m_dynamicCollection[ t ]->m_colliderPair.Begin(); it != m_dynamicCollection[ t ]->m_colliderPair.End(); ++it )
		{
			RED_ASSERT( it->m_first, TXT("Dynamic collider component nulled prematurely!") );

			Vector localPos = Vector::ZEROS;
			// should never happen
			if( it->m_first != nullptr )
			{
				localPos = it->m_first->GetLocalToWorld().GetTranslation();
			}

			// collision is in AABB range
			// add this component to active list
			if( fabs( localPos.X - cameraPosition.X ) < effectiveDistance && fabs( localPos.Y - cameraPosition.Y ) < effectiveDistance )
			{
				// not already active
				if( it->m_second < 0 )
				{
					Int32 ind = GetFreeSlot( t );

					if( ind > -1 )
					{
						m_dynamicCollection[ t ]->m_activeCollisions[ ind ].m_decaySpeed = 0.0f;
						m_dynamicCollection[ t ]->m_activeCollisions[ ind ].m_intensity = 1.0f;
						m_dynamicCollection[ t ]->m_activeCollisions[ ind ].m_transformMatrix.SetRow( 3, localPos );
						m_dynamicCollection[ t ]->m_activeCollisions[ ind ].m_useHideFactor = it->m_first->m_useHideFactor;

						m_dynamicCollection[ t ]->m_activeCollisions[ ind ].m_parent = it->m_first;
						m_dynamicCollection[ t ]->m_colliderPair.Set( it->m_first, ind );
					}
				}
			}
			// collision is out of range
			else
			{
				// was active before?
				// mark as unused
				Int32 ind = it->m_second;
				if( ind > -1 ) 
				{
					m_dynamicCollection[ t ]->m_activeCollisions[ ind ].m_parent = nullptr;
					m_dynamicCollection[ t ]->m_activeCollisions[ ind ].m_intensity = 0.0f;

					m_dynamicCollection[ t ]->m_colliderPair.Set( it->m_first, -1 );
				}
			}
		}		
	}	
}

Int32 CDynamicCollisionCollector::GetFreeSlot( Uint32 type )
{
	for( Uint32 i=0; i<DYNAMIC_COLLIDERS_MAX_SIZE; i++ )
	{
		// this might prefer to actually replace script based collision (bombs, ard...) with component based collision
		if( m_dynamicCollection[ type ]->m_activeCollisions[i].m_intensity < 0.0001f || m_dynamicCollection[ type ]->m_activeCollisions[i].m_parent == nullptr ) 
			return i;
	}
	return -1;
}

void CDynamicCollisionCollector::Tick( CWorld* world, Float timeDelta )
{
	PC_SCOPE(DynamicCollisionCollector_Tick);

	RED_ASSERT(world);
	
	++m_currentTickBlock;	
	if( m_currentTickBlock > NON_ACTIVE_DYNAMIC_COLLIDERS_MAX_SIZE/m_pereferableTickRange ) m_currentTickBlock = 0;

	m_currentTickBlockBegin = m_currentTickBlock * m_pereferableTickRange;
	m_currentTickBlockEnd = ( m_currentTickBlock + 1 ) * m_pereferableTickRange;

	m_currentTimeAcum += timeDelta;

	if( timeDelta > 0.0001f ) 
	{		
		for( Uint32 i=0; i<m_dynamicCollection.Size(); i++ )
		{
			RefreshActiveCollider( i, timeDelta );
		}
	}

	if( m_currentTimeAcum > DYNAMIC_COLLIDERS_WATER_PROXIMITY_CHECK || m_forceOcclude )
	{
		OccludeColliders( world->GetCameraPosition(), m_forceOcclude );
		m_currentTimeAcum = 0.0f;
		m_forceOcclude = false;
	}

	FetchToSpeedtree( world, timeDelta );
}

void CDynamicCollisionCollector::FetchToSpeedtree( CWorld* world, Float timeDelta )
{
	if( !m_dynamicCollection[ DYNAMIC_COLLIDERS_TYPE_GRASS ]->m_activeCollisions.Empty() )
	{			
		world->GetFoliageEditionController().UpdateDynamicGrassCollision( m_dynamicCollection[ DYNAMIC_COLLIDERS_TYPE_GRASS ]->m_activeCollisions );	
	}
}

void CDynamicCollisionCollector::Shutdown()
{	
	for(Uint32 i=0; i<m_dynamicCollection.Size(); i++)
	{
		m_dynamicCollection[i]->m_activeCollisions.ClearFast();
		m_dynamicCollection[i]->m_colliderPair.ClearFast();		
	}
	m_dynamicCollection.ClearFast();
}

CDynamicCollisionCollector::~CDynamicCollisionCollector()
{
}

void CDynamicCollisionCollector::GenerateEditorFragments( CRenderFrame* frame )
{
	const Uint32 tempSize = m_dynamicCollection.Size();
	if( frame->GetFrameInfo().IsShowFlagOn( SHOW_DynamicCollector ) )
	{
		for( Uint32 type=0; type<tempSize; type++ )
		{
			for( Uint32 i=0; i<DYNAMIC_COLLIDERS_MAX_SIZE; i++ )
			{
				if( m_dynamicCollection[ type ]->m_activeCollisions[i].m_intensity > 0.0f )
				{
					const Matrix& gtm = m_dynamicCollection[ type ]->m_activeCollisions[i].m_transformMatrix;
					Vector row0 = gtm.GetRow(0);
					Vector row1 = gtm.GetRow(1);
					Vector row2 = gtm.GetRow(2);
					Matrix rot( row0, row1, row2, Vector::ZERO_3D_POINT );
					frame->AddDebugSphere( gtm.GetTranslation(), 1.f, rot, Color::BLACK );
				}
			}
		}
	}
}
