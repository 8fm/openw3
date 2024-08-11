/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorWalkablePositionQuery.h"

#include "behTreeInstance.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorWalkableSpotRingQueryDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorWalkableSpotClosestQueryDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorWalkableSpotResultDefintion )

///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeDecoratorWalkableSpotQueryInstance
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorWalkableSpotQueryInstance::IBehTreeNodeDecoratorWalkableSpotQueryInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_useCombatTargetAsReference( def.m_useCombatTargetAsReference )
	, m_useTargetAsSourceSpot( def.m_useTargetAsSourceSpot )
	, m_requestPtr( owner, def.m_queryName )
{
	if ( def.m_stayInGuardArea )
	{
		m_guardDataPtr = CBehTreeGuardAreaDataPtr( owner );
	}
}

IBehTreeNodeDecoratorWalkableSpotQueryInstance::EQueryStatus IBehTreeNodeDecoratorWalkableSpotQueryInstance::UpdateQuery()
{
	switch ( m_requestPtr->GetQuery()->GetQueryState() )
	{
	case CPositioningFilterRequest::STATE_COMPLETED_FAILURE:
		return STATUS_FAILURE;
	case CPositioningFilterRequest::STATE_COMPLETED_SUCCESS:
		return STATUS_SUCCESS;
	case CPositioningFilterRequest::STATE_ONGOING:
		break;
	default:
	case CPositioningFilterRequest::STATE_DISPOSED:
	case CPositioningFilterRequest::STATE_SETUP:
		ASSERT( false );
		ASSUME( false );
	}
	return STATUS_IN_PROGRESS;
}



///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorWalkableSpotRingQueryInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeDecoratorWalkableSpotRingQueryInstance::EQueryStatus CBehTreeNodeDecoratorWalkableSpotRingQueryInstance::StartQuery()
{
	CNode* referenceObj =
		m_useCombatTargetAsReference
		? m_owner->GetCombatTarget().Get()
		: m_owner->GetActionTarget().Get();

	if ( !referenceObj )
	{
		return STATUS_FAILURE;
	}

	CActor* actor = m_owner->GetActor();
	CNode* sourceSpot =
		m_useTargetAsSourceSpot
		? referenceObj
		: actor;

	CAreaComponent* areaComponent = nullptr;
	CBehTreeGuardAreaData* guardAreaData = m_guardDataPtr.Get();
	if ( guardAreaData )
	{
		areaComponent = guardAreaData->GetGuardArea();
	}

	CBehTreePositioningRequest* data = m_requestPtr.Get();
	CPositioningFilterRequest* request = data->GetQuery();

	const Vector3& referencePos = referenceObj->GetWorldPositionRef().AsVector3();
	const Vector3& sourcePos = sourceSpot->GetWorldPositionRef().AsVector3();

	if ( !request->Setup( m_filter, GGame->GetActiveWorld(), referencePos, actor->GetRadius(), -1024.f, &sourcePos, areaComponent ) )
	{
		return STATUS_FAILURE;
	}
	data->SetValidFor( m_owner->GetLocalTime() + m_queryValidFor );
	return STATUS_IN_PROGRESS;
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorWalkableSpotClosestQueryInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeDecoratorWalkableSpotClosestQueryInstance::EQueryStatus CBehTreeNodeDecoratorWalkableSpotClosestQueryInstance::StartQuery()
{
	CNode* referenceObj =
		m_useCombatTargetAsReference
		? m_owner->GetCombatTarget().Get()
		: m_owner->GetActionTarget().Get();

	if ( !referenceObj )
	{
		return STATUS_FAILURE;
	}

	CActor* actor = m_owner->GetActor();
	CNode* sourceSpot =
		m_useTargetAsSourceSpot
		? referenceObj
		: actor;

	CAreaComponent* areaComponent = nullptr;
	CBehTreeGuardAreaData* guardAreaData = m_guardDataPtr.Get();
	if ( guardAreaData )
	{
		areaComponent = guardAreaData->GetGuardArea();
	}

	CBehTreePositioningRequest* data = m_requestPtr.Get();
	CPositioningFilterRequest* request = data->GetQuery();

	const Vector3& referencePos = referenceObj->GetWorldPositionRef().AsVector3();
	const Vector3& sourcePos = sourceSpot->GetWorldPositionRef().AsVector3();

	if ( !request->Setup( m_filter, GGame->GetActiveWorld(), referencePos, actor, &sourcePos, areaComponent ) )
	{
		return STATUS_FAILURE;
	}
	data->SetValidFor( m_owner->GetLocalTime() + m_queryValidFor );
	return STATUS_IN_PROGRESS;
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorWalkableSpotResultInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeDecoratorWalkableSpotResultInstance::CBehTreeNodeDecoratorWalkableSpotResultInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_requestPtr( owner, def.m_queryName )
	, m_customMoveDataPtr( owner )
{
}

Bool CBehTreeNodeDecoratorWalkableSpotResultInstance::IsAvailable()
{
	CBehTreePositioningRequest* data = m_requestPtr.Get();
	if ( !data->IsValid( m_owner->GetLocalTime() ) )
	{
		return false;
	}
	if ( data->GetQuery()->GetQueryState() != CPositioningFilterRequest::STATE_COMPLETED_SUCCESS )
	{
		return false;
	}
	return Super::IsAvailable();
}
Int32 CBehTreeNodeDecoratorWalkableSpotResultInstance::Evaluate()
{
	CBehTreePositioningRequest* data = m_requestPtr.Get();
	if ( !data->IsValid( m_owner->GetLocalTime() ) )
	{
		return false;
	}
	if ( data->GetQuery()->GetQueryState() != CPositioningFilterRequest::STATE_COMPLETED_SUCCESS )
	{
		return false;
	}
	return Super::Evaluate();
}

Bool CBehTreeNodeDecoratorWalkableSpotResultInstance::Activate()
{
	CPositioningFilterRequest* request = m_requestPtr->GetQuery();
	if ( request->GetQueryState() != CPositioningFilterRequest::STATE_COMPLETED_SUCCESS )
	{
		return false;
	}
	const Vector3& outputPos = request->GetComputedPosition();
	m_customMoveDataPtr->SetTarget( outputPos.X, outputPos.Y, outputPos.Z );
	return Super::Activate();
}