#include "build.h"
#include "behTreeDecoratorRiderHorseReachability.h"
#include "w3GenericVehicle.h"
#include "../../common/game/movableRepresentationPathAgent.h"


/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderHorseReachabilityDefinition
CBehTreeDecoratorRiderHorseReachabilityDefinition::CBehTreeDecoratorRiderHorseReachabilityDefinition() 
{

}
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorRiderHorseReachabilityDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderHorseReachabilityInstance
CBehTreeDecoratorRiderHorseReachabilityInstance::CBehTreeDecoratorRiderHorseReachabilityInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
	, m_riderData( owner, CNAME( RiderData ) )
{
	m_reachabilityDataPtr = new CReachabilityData();
}

CActor* CBehTreeDecoratorRiderHorseReachabilityInstance::GetHorseActor() const
{
	if ( !m_riderData )
	{
		return nullptr;
	}

	CHorseRiderSharedParams *const horseRiderSharedParam = m_riderData->m_sharedParams.Get();
	if ( !horseRiderSharedParam )
	{
		return nullptr;
	}

	return horseRiderSharedParam->m_horse.Get();
}

Bool CBehTreeDecoratorRiderHorseReachabilityInstance::Activate()
{
	if ( !Super::Activate() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	const CActor* actor = m_owner->GetActor();
	const CMovingAgentComponent* movingAgentComponent = actor->GetMovingAgentComponent();
	if ( !movingAgentComponent )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CPathAgent* pathAgent = movingAgentComponent->GetPathAgent();
	if ( pathAgent && pathAgent->TestLocation( movingAgentComponent->GetWorldPositionRef() ) )
	{
		CReachabilityData* reachabilityData = m_reachabilityDataPtr.Get();
		reachabilityData->Initialize( pathAgent->GetPathLib(), pathAgent->GetPersonalSpace() );
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeDecoratorRiderHorseReachabilityInstance::Update()
{
	if ( m_child->IsActive() )
	{
		Super::Update();
		return;
	}

	const CActor* horseActor = GetHorseActor();
	if ( !horseActor )
	{
		Complete( BTTO_FAILED );
		return;
	}

	const CActor* actor = m_owner->GetActor();
	const CMovingAgentComponent* movingAgentComponent = actor->GetMovingAgentComponent();
	const CPathAgent* pathAgent = movingAgentComponent->GetPathAgent();

	CReachabilityData* reachabilityData = m_reachabilityDataPtr.Get();
	PathLib::EPathfindResult result = reachabilityData->QueryReachable( pathAgent->GetPathLib(), movingAgentComponent->GetWorldPositionRef(), horseActor->GetMovingAgentComponent()->GetWorldPositionRef() );

	if ( result == PathLib::EPathfindResult::PATHRESULT_SUCCESS )
	{
		m_child->Activate();
	}
	else if ( result != PathLib::EPathfindResult::PATHRESULT_PENDING )
	{
		OnSubgoalCompleted( BTTO_FAILED );
	}
}

void CBehTreeDecoratorRiderHorseReachabilityInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( outcome == BTTO_FAILED )
	{
		CActor* horseActor = GetHorseActor();
		m_riderData->RegisterUnreachableHorse( horseActor );
		ComponentIterator< W3HorseComponent > it( horseActor );
		if ( it )
		{  
			W3HorseComponent*const horseComponent	= *it;
			horseComponent->Unpair();
		}
	}

	IBehTreeNodeDecoratorInstance::OnSubgoalCompleted( outcome );
}