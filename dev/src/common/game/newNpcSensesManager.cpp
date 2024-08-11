/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newNpcSensesManager.h"

#include "../../common/physics/physicsWorldPhysxImplBatchTrace.h"

CPhysicsEngine::CollisionMask	CNewNpcSensesManager::LOS_COLLISION_MASK	= 0;
Uint16							CNewNpcSensesManager::LOS_RAYCAST_FLAGS		= EBatchQueryQueryFlag::EQQF_BLOCKING_HIT;

CNewNpcSensesManager::CNewNpcSensesManager()
{
	LOS_COLLISION_MASK = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) );
}

VisibilityQueryId CNewNpcSensesManager::SubmitQuery( CEntity* caster, const Vector& startPos, const Vector& endPos )
{
	return GGame->GetActiveWorld()->GetPhysicsBatchQueryManager()->SubmitRaycastQuery( startPos, endPos, LOS_COLLISION_MASK, 0, 0, LOS_RAYCAST_FLAGS, caster );
}

CNewNpcSensesManager::EVisibilityQueryState CNewNpcSensesManager::GetQueryState( const VisibilityQueryId& queryId )
{
	const CPhysicsBatchQueryManager* queryManager = GGame->GetActiveWorld()->GetPhysicsBatchQueryManager();

	TDynArray< SRaycastHitResult > result;
	EBatchQueryState state = queryManager->GetRaycastQueryState( queryId, result );
	
	switch ( state )
	{
	case EBatchQueryState::BQS_Processed:
		{
			if ( result.Empty() )
			{
				return QS_True;
			}

			// there are some results, and there's no "caster" entity to skip -> we hit something
			void* caster = queryManager->GetRaycastUserData( queryId );
			if ( caster == nullptr )
			{
				return QS_False;
			}

			// otherwise, let's skip results that belong to "caster" entity
			for ( SRaycastHitResult& r : result )
			{
				CComponent* parent = r.m_component;
				// if no parent component, we hit terrain or cooked static mesh
				if ( parent == nullptr )
				{
					return QS_False;
				}
				// if there's parent component, but not belonging to "caster" entity -> we hit something
				if ( parent->GetEntity() != caster )
				{
					return QS_False;
				}
			}

			// all the results belong to "caster" entity, so we hit nothing
			return QS_True;
		}

	case EBatchQueryState::BQS_NotReady:
		return QS_NotReady;

	default:
		return QS_NotFound;
	}
}

#ifndef RED_FINAL_BUILD
Bool CNewNpcSensesManager::GetQueryDebugData( const VisibilityQueryId& queryId, SRaycastDebugData& debugData ) const
{
	const CPhysicsBatchQueryManager* queryManager = GGame->GetActiveWorld()->GetPhysicsBatchQueryManager();
	if ( queryManager != nullptr )
	{
		return queryManager->GetRaycastDebugData( queryId, debugData );
	}
	return false;
}
#endif