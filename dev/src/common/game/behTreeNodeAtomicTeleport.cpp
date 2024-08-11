#include "build.h"
#include "behTreeNodeAtomicTeleport.h"

#include "behTreeInstance.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicTeleportInstance
////////////////////////////////////////////////////////////////////////
void CBehTreeNodeAtomicTeleportInstance::Update()
{
	CActor* actor = m_owner->GetActor();
	Vector target;
	EulerAngles heading( 0, 0, 0 );
	if ( actor && ComputeTargetAndHeading( target, heading.Yaw ) )
	{
		if ( actor->Teleport( target, heading ) )
		{
			Complete( BTTO_SUCCESS );
			return;
		}
	}

	Complete( BTTO_FAILED );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTeleportToActionTargetDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeTeleportToActionTargetDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTeleportToActionTargetInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeTeleportToActionTargetInstance::ComputeTargetAndHeading( Vector& outTarget, Float& outHeading )
{
	CNode* node = m_owner->GetActionTarget().Get();
	if ( node )
	{
		outTarget = node->GetWorldPositionRef();
		outHeading = node->GetWorldYaw();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTeleportToActionTargetCheckPositionDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeTeleportToActionTargetCheckPositionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTeleportToActionTargetCheckPositionInstance
////////////////////////////////////////////////////////////////////////
void CBehTreeNodeTeleportToActionTargetCheckPositionInstance::LazySpawnQueryRequest()
{
	if ( !m_queryRequest )
	{
		m_queryRequest = new CPositioningFilterRequest();
	}
}

Bool CBehTreeNodeTeleportToActionTargetCheckPositionInstance::ComputeTargetAndHeading( Vector& outTarget, Float& outHeading )
{
	if ( !FindSpot() )
	{
		return false;
	}
	
	CNode* node = m_owner->GetActionTarget().Get();
	if ( !node )
	{
		return false;
	}
	
	// collect position from request
	outTarget.AsVector3() = m_queryRequest->GetComputedPosition();

	// face target
	Vector2 diff = node->GetWorldPositionRef().AsVector2() - outTarget.AsVector2();
	outHeading = EulerAngles::YawFromXY( diff.X, diff.Y );
		
	return true;
}

Bool CBehTreeNodeTeleportToActionTargetCheckPositionInstance::FindSpot()
{
	LazySpawnQueryRequest();

	switch( m_queryRequest->GetQueryState() )
	{
	case CPositioningFilterRequest::STATE_COMPLETED_SUCCESS:
		if ( m_queryRequestValidTimeout >= m_owner->GetLocalTime() )
		{
			return true;
		}
		// no break
	case CPositioningFilterRequest::STATE_DISPOSED:
	case CPositioningFilterRequest::STATE_SETUP:
		// run new query
		{
			Float time = m_owner->GetLocalTime();
			if ( m_queryLockTimeout < time )
			{
				CNode* node = m_owner->GetActionTarget().Get();
				if ( !node )
				{
					return false;
				}

				const Vector& targetPos = node->GetWorldPositionRef();

				CActor* actor = m_owner->GetActor();
				Float direction = -1024.f;
				if ( m_filter.m_angleLimit < 180.f )
				{
					const Vector& myPos = actor->GetWorldPositionRef();
					Vector2 diff = myPos.AsVector2() - targetPos.AsVector2();
					direction = EulerAngles::YawFromXY( diff.X, diff.Y );
				}
				m_queryRequest->Setup( m_filter, GGame->GetActiveWorld(), targetPos, actor->GetRadius(), direction );
				CWorld* world = GGame->GetActiveWorld();
				m_queryRequest->Submit( *world->GetPathLibWorld() );
				m_queryRequestValidTimeout = time + 0.5f;

				m_parent->MarkParentSelectorDirty();
			}
		}
		
		break;

	case CPositioningFilterRequest::STATE_ONGOING:
		m_parent->MarkParentSelectorDirty();
		break;
	
	case CPositioningFilterRequest::STATE_COMPLETED_FAILURE:
		m_queryRequest->Dispose();
		m_queryLockTimeout = m_owner->GetLocalTime() + m_queryDelay;
		break;
	}
	return false;


}

Bool CBehTreeNodeTeleportToActionTargetCheckPositionInstance::IsAvailable()
{
	if ( !FindSpot() )
	{
		return false;
	}
	return true;
}
Int32 CBehTreeNodeTeleportToActionTargetCheckPositionInstance::Evaluate()
{
	if ( !FindSpot() )
	{
		return -1;
	}
	return m_priority;
}

void CBehTreeNodeTeleportToActionTargetCheckPositionInstance::Deactivate()
{
	m_queryRequest->Dispose();
	m_queryLockTimeout = m_owner->GetLocalTime() + m_queryDelay;

	Super::Deactivate();
}

