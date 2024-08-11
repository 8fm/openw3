#include "build.h"
#include "behTreeNodeWorkAtomic.h"

#include "../engine/renderFrame.h"

#include "behTreeInstance.h"
#include "communitySystem.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicTeleportToActionPointDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicMoveToActionPointDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionTeleportToWorkDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAlreadyAtWorkDefinition )

namespace
{
	static Bool ComputeTargetAndHeading( CBehTreeInstance* owner, const CBehTreeWorkDataPtr& ptr, Vector& outTarget, Float& outHeading )
	{
		CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
		if ( !actionPointManager )
		{
			return false;
		}
		if ( !actionPointManager->GetSafePosition( ptr->GetSelectedAP(), &outTarget, &outHeading ) )
		{
			return false;
		}

		return true;
	}
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicTeleportToActionPointInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeAtomicTeleportToActionPointInstance::CBehTreeNodeAtomicTeleportToActionPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_workData( owner )
{}
Bool CBehTreeNodeAtomicTeleportToActionPointInstance::IsAvailable()
{
	// TODO: Moooore logic
	if ( m_workData && m_workData->IsTryingToSpawnToWork( m_owner ) )
	{
		// check ap distance
		Vector apPos;
		Float apHeading;
		if ( ::ComputeTargetAndHeading( m_owner, m_workData, apPos, apHeading ) )
		{
			const Vector& myPos = m_owner->GetActor()->GetWorldPositionRef();
			Float distLimitSq = m_workData->GetSpawnToWorkAPDistance();
			distLimitSq *= distLimitSq;
			if ( (myPos.AsVector2() - apPos.AsVector2()).SquareMag() <= distLimitSq )
			{
				return true;
			}
		}
	}

	DebugNotifyAvailableFail();
	return false;
}
Bool CBehTreeNodeAtomicTeleportToActionPointInstance::ComputeTargetAndHeading( Vector& outTarget, Float& outHeading )
{
	return ::ComputeTargetAndHeading( m_owner, m_workData, outTarget, outHeading );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicMoveToActionPointInstance
////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicMoveToActionPointInstance::CBehTreeNodeAtomicMoveToActionPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) 
	: Super( def, owner, context, parent )
	, m_workData( owner )
{
}

Bool CBehTreeNodeAtomicMoveToActionPointInstance::ComputeTargetAndHeading()
{
	return ::ComputeTargetAndHeading( m_owner, m_workData, m_target, m_heading );
}

Bool CBehTreeNodeAtomicMoveToActionPointInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( OnTimetableChanged ) )
	{
		Complete( BTTO_FAILED );
		return false;
	}

	return Super::OnEvent( e );
}

void CBehTreeNodeAtomicMoveToActionPointInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	CActor* npc = m_owner->GetActor();
	SActionPointId apId = m_workData->GetSelectedAP();

	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( !actionPointManager )
	{
		return;
	}
	Vector apPosition;
	float heading;
	if ( !actionPointManager->GetSafePosition( apId, &apPosition, &heading ) )
	{
		return;
	}

	FixedCapsule capsule( apPosition, 0.5f, 1.5f );		

	frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::GREEN );	
	frame->AddDebugLineWithArrow( npc->GetWorldPosition(), apPosition, 0.5f, 0.1f, 0.1f, Color::RED, true );
	String apName = actionPointManager->GetFriendlyAPName( apId );
	String apLayerName = TXT("unknown");
	CLayer *apLayer = GGame->GetActiveWorld()->FindLayer( actionPointManager->GetLayerGUID( apId ) );
	if( apLayer )
	{
		apLayerName = apLayer->GetFriendlyName();
	}	
	Vector textPos = apPosition;
	textPos.Y += 1;
	frame->AddDebugText( textPos, apName, 0, 0, true, Color::WHITE );

	textPos.Y -= 0.75f;	
	frame->AddDebugText( textPos, apLayerName, 0, 0, true, Color::WHITE );

	Super::OnGenerateDebugFragments( frame );
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionTeleportToWorkInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionTeleportToWorkInstance::CBehTreeNodeConditionTeleportToWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_workData( owner )
{
}
Bool CBehTreeNodeConditionTeleportToWorkInstance::ConditionCheck()
{
	CBehTreeWorkData* workData = m_workData.Get();
	if ( m_workData->IsInImmediateActivation( m_owner ) || m_workData->IsTryingToSpawnToWork( m_owner ) )
	{
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAlreadyAtWorkDefinition
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAlreadyAtWorkDefinition::CBehTreeNodeAlreadyAtWorkDefinition()
: m_acceptRotationRequired(180.0f)
{
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAlreadyAtWorkInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeNodeAlreadyAtWorkInstance::CBehTreeNodeAlreadyAtWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_workData( owner )
{
	m_acceptDistanceSq = def.m_acceptDistance.GetVal( context ); m_acceptDistanceSq *= m_acceptDistanceSq;
	m_acceptRotationRequired = def.m_acceptRotationRequired.GetVal(context);
}

Bool CBehTreeNodeAlreadyAtWorkInstance::IsAvailable()
{
	CBehTreeWorkData* workData = m_workData.Get();
	if ( m_workData->IsInImmediateActivation( m_owner ) || m_workData->IsTryingToSpawnToWork( m_owner ) )
	{
		return true;
	}

	CActionPointManager* actionPointManager = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( !actionPointManager )
	{
		return false;
	}
	Vector targetPos;
	Float heading;
	if ( !actionPointManager->GetGoToPosition( workData->GetSelectedAP(), &targetPos, &heading ) )
	{
		return false;
	}
	CActor* actor = m_owner->GetActor();
	const Vector& actorPos = actor->GetWorldPositionRef();

	Float distSq = (actorPos.AsVector2() - targetPos.AsVector2()).SquareMag();
	if ( distSq < m_acceptDistanceSq && Abs( actorPos.Z - targetPos.Z ) < 1.0f )
	{
		Float yawOff = Abs( EulerAngles::NormalizeAngle180( actor->GetWorldYaw() - heading ) );
		if ( yawOff <= m_acceptRotationRequired || m_acceptRotationRequired == 0.0f )
		{
			return true;
		}
	}

	return false;
}
